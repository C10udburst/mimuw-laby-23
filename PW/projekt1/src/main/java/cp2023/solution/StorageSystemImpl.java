package cp2023.solution;

import cp2023.base.ComponentId;
import cp2023.base.ComponentTransfer;
import cp2023.base.DeviceId;
import cp2023.base.StorageSystem;
import cp2023.exceptions.*;

import java.util.HashMap;
import java.util.HashSet;
import java.util.Map;
import java.util.concurrent.*;
import java.util.concurrent.atomic.AtomicBoolean;

public class StorageSystemImpl implements StorageSystem {

    private final HashMap<DeviceId, Integer> freeSlots;
    private final ConcurrentHashMap<ComponentId, AtomicBoolean> isBusy;
    private final ConcurrentHashMap<ComponentId, DeviceId> componentPlacement;
    private final ConcurrentHashMap<DeviceId, ConcurrentLinkedDeque<FrozenTransfer>> waitingTransfers;
    private final Semaphore mutex = new Semaphore(1);

    public StorageSystemImpl(Map<DeviceId, Integer> deviceTotalSlots, Map<ComponentId, DeviceId> componentPlacement) {
        this.freeSlots = new HashMap<>(deviceTotalSlots);
        this.waitingTransfers = new ConcurrentHashMap<>();
        this.isBusy = new ConcurrentHashMap<>();
        for (var dev: deviceTotalSlots.keySet()) {
            this.waitingTransfers.put(dev, new ConcurrentLinkedDeque<>());
        }
        this.componentPlacement = new ConcurrentHashMap<>(componentPlacement);
        for (var component : componentPlacement.keySet()) {
            if (!freeSlots.containsKey(componentPlacement.get(component))) {
                throw new IllegalArgumentException("Device " + componentPlacement.get(component) + " does not exist");
            }
            freeSlots.computeIfPresent(componentPlacement.get(component), (k, v) -> v - 1);
            if (freeSlots.get(componentPlacement.get(component)) < 0) {
                throw new IllegalArgumentException("Device " + componentPlacement.get(component) + " has too many components");
            }
        }

    }

    @Override
    public void execute(ComponentTransfer transfer) throws TransferException {
        var src = transfer.getSourceDeviceId();
        var dst = transfer.getDestinationDeviceId();

        if (src == null && dst == null) {
            throw new IllegalTransferType(transfer.getComponentId());
        }

        checkDevice(src);
        checkDevice(dst);

        mutexAcquire(); // maybe we could do it after the 4 checks, but this guarantees that there isn't a weird configuration that breaks the system

        if (src == null && componentPlacement.containsKey(transfer.getComponentId())) {
            throw new ComponentAlreadyExists(transfer.getComponentId(), componentPlacement.get(transfer.getComponentId()));
        }

        var currentSrc = componentPlacement.getOrDefault(transfer.getComponentId(), null);

        if ((currentSrc != null && !currentSrc.equals(src)) || (currentSrc == null && src != null)) {
            assert src != null; // src == null => componentPlacement[transfer.getComponentId()] == null (the previous if)
            throw new ComponentDoesNotExist(transfer.getComponentId(), src);
        }

        if (src != null && src.equals(dst)) {
            throw new ComponentDoesNotNeedTransfer(transfer.getComponentId(), src);
        }

        if (isBusy
                .computeIfAbsent(transfer.getComponentId(), k -> new AtomicBoolean(false))
                .getAndSet(true)) {
            throw new ComponentIsBeingOperatedOn(transfer.getComponentId());
        }

        // mutexAcquire();

        if (dst == null) {
            noDepend(transfer);
        } else if (freeSlots.get(dst) > 0) {
            freeSlots.computeIfPresent(dst, (k, v) -> v - 1);
            noDepend(transfer);
        } else {
            hasDepend(transfer);
        }
    }

    /**
     * This function is called when our component has no dependencies.
     * We perform a whole chain of transfers that start from our component, or we perform the transfer by ourselves.
     */
    private void noDepend(ComponentTransfer transfer) {
        var src = transfer.getSourceDeviceId();
        var childSem = walkChain(src);
        mutex.release();
        if (childSem == null) {
            // we have not found a chain, we will transfer it by ourselves
            transfer.prepare();
            transfer.perform();
            mutexAcquire();
            if (src != null) {
                var next = waitingTransfers.get(src).poll();
                if (next != null) {
                    // in the time we were performing the transfer, a new transfer appeared
                    assert next.barrier == null;
                    updatePosition(transfer);
                    mutex.release();
                    next.semaphore.release();
                } else {
                    // no new transfer appeared, we can release the slot
                    freeSlots.computeIfPresent(src, (k, v) -> v + 1);
                    updatePosition(transfer);
                    mutex.release();
                }
            } else {
                updatePosition(transfer);
                mutex.release();
            }

        } else {
            // we found a chain, we will transfer it concurrently
            performWSem(transfer, childSem, null, true, false);
        }
    }

    /**
     * This function is called when our component can't be transferred immediately.
     * We either find a cycle and transfer it concurrently, or we sleep until we are woken up.
     */
    private void hasDepend(ComponentTransfer transfer) {
        var src = transfer.getSourceDeviceId();
        var dst = transfer.getDestinationDeviceId();
        if (src == null) {
            // we are creating a new component, there is no cycle, so we sleep
            var ft = new FrozenTransfer(transfer, new Semaphore(0));
            waitingTransfers.get(dst).add(ft);
            mutex.release();
            ft.semAcquire();
            onFrozenWake(ft);
        } else {
            // we are moving a component
            var barrier = walkCycle(src, new HashSet<>(), 1, dst);
            if (barrier == null) {
                // no cycle/chain exists yet, we sleep
                var ft = new FrozenTransfer(transfer, new Semaphore(0));
                waitingTransfers.get(dst).add(ft);
                mutex.release();
                ft.semAcquire();
                onFrozenWake(ft);
            } else {
                // we found a cycle, we will transfer it concurrently
                mutex.release();
                assert dst != null;
                performWBarrier(transfer, barrier, true, false);
            }
        }
    }

    /**
     * This function sets up a barrier for a chain and wakes all the components in the chain.
     */
    private Semaphore walkChain(DeviceId start) {
        if (start == null) return null;
        var transfer = waitingTransfers.get(start).poll();
        if (transfer == null) return null;
        var sem = walkChain(transfer.transfer.getSourceDeviceId());
        if (sem == null) {
            sem = new Semaphore(0);
            transfer.chainWait = sem;
            transfer.semaphore.release();
            return sem;
        } else {
            transfer.chainWake = sem;
            sem = new Semaphore(0);
            transfer.chainWait = sem;
            transfer.semaphore.release();
            return sem;
        }
    }


    /**
     * This function finds a cycle and sets up a barrier for it and wakes all the components in the cycle.
     * Or it returns null if no cycle exists.
     */
    private CyclicBarrier walkCycle(DeviceId start, HashSet<FrozenTransfer> visited, int depth, DeviceId target) {
        assert target != null;
        if (start == null) return null;
        var waiting = waitingTransfers.get(start);
        for (var transfer : waiting) {
            if (visited.contains(transfer)) continue;
            visited.add(transfer);
            if (target.equals(transfer.transfer.getSourceDeviceId())) {
                waitingTransfers.get(start).remove(transfer);
                var barrier = new CyclicBarrier(depth + 1);
                transfer.barrier = barrier;
                transfer.semaphore.release();
                return barrier;
            }
            var barrier = walkCycle(transfer.transfer.getSourceDeviceId(), visited, depth + 1, target);
            if (barrier != null) {
                waitingTransfers.get(start).remove(transfer);
                transfer.barrier = barrier;
                transfer.semaphore.release();
                return barrier;
            }
        }
        return null;
    }

    /**
     * This function is called when we are woken up from a sleep from the queue.
     */
    private void onFrozenWake(FrozenTransfer ft) {
        if (ft.barrier != null) {
            // we were woken up, because we are in a cycle
            assert ft.chainWake == null && ft.chainWait == null;
            performWBarrier(ft.transfer, ft.barrier, false, ft.updateSrc);
        } else if (ft.chainWake != null && ft.chainWait != null) {
            // we were woken up, because we are in a chain
            performWSem(ft.transfer, ft.chainWake, ft.chainWait, false, ft.updateSrc);
        } else {
            // we were woken up, but left to ourselves
            noDepend(ft.transfer);
        }
    }

    /**
     * This function performs the concurrent transfer.
     */
    private void performWBarrier(ComponentTransfer ct, CyclicBarrier barrier, boolean doMutex, boolean updateSrc) {
        waitBarrier(barrier);
        ct.prepare();
        waitBarrier(barrier);
        ct.perform();
        waitBarrier(barrier);
        if (doMutex)
            mutexAcquire();
        if (updateSrc)
            freeSlots.computeIfPresent(ct.getSourceDeviceId(), (k, v) -> v + 1); // we are moving the component away and no new component is coming
        updatePosition(ct);
        waitBarrier(barrier);
        if (doMutex) mutex.release();
    }

    private void performWSem(ComponentTransfer ct, Semaphore childSem, Semaphore waitSem, boolean doMutex, boolean updateSrc) {
        waitSem(waitSem);
        releaseSem(childSem);
        ct.prepare();
        waitSem(waitSem);
        releaseSem(childSem);
        ct.perform();
        waitSem(waitSem);
        releaseSem(childSem);
        if (doMutex)
            mutexAcquire();
        if (updateSrc)
            freeSlots.computeIfPresent(ct.getSourceDeviceId(), (k, v) -> v + 1); // we are moving the component away and no new component is coming
        updatePosition(ct);
        waitSem(waitSem);
        releaseSem(childSem);
        if (doMutex) mutex.release();
    }

    private void updatePosition(ComponentTransfer ct) {
        var dst = ct.getDestinationDeviceId();
        if (dst == null) {
            componentPlacement.remove(ct.getComponentId());
            isBusy.remove(ct.getComponentId());
        } else {
            componentPlacement.put(ct.getComponentId(), dst);
            isBusy.get(ct.getComponentId()).set(false);
        }
    }

    private void checkDevice(DeviceId device) throws TransferException {
        if (device == null) return;
        if (!waitingTransfers.containsKey(device) || !freeSlots.containsKey(device)) {
            throw new DeviceDoesNotExist(device);
        }
    }

    private void mutexAcquire() {
        try {
            mutex.acquire();
        } catch (InterruptedException e) {
            throw new RuntimeException("panic: unexpected thread interruption");
        }
    }

    private static void waitBarrier(CyclicBarrier barrier) {
        try {
            barrier.await();
        } catch (InterruptedException | BrokenBarrierException e) {
            throw new RuntimeException("panic: unexpected thread interruption");
        }
    }

    private static void waitSem(Semaphore sem) {
        if (sem == null) return;
        try {
            sem.acquire();
        } catch (InterruptedException e) {
            throw new RuntimeException("panic: unexpected thread interruption");
        }
    }

    private static void releaseSem(Semaphore sem) {
        if (sem == null) return;
        sem.release();
    }
}
