package cp2023.solution;

import cp2023.base.ComponentId;
import cp2023.base.ComponentTransfer;
import cp2023.base.DeviceId;
import cp2023.base.StorageSystem;
import cp2023.exceptions.*;

import java.util.HashSet;
import java.util.Map;
import java.util.concurrent.*;
import java.util.concurrent.atomic.AtomicBoolean;
import java.util.concurrent.atomic.AtomicInteger;

public class StorageSystemImpl implements StorageSystem {

    private final ConcurrentHashMap<DeviceId, AtomicInteger> freeSlots;
    private final ConcurrentHashMap<ComponentId, AtomicBoolean> isBusy;
    private final ConcurrentHashMap<ComponentId, DeviceId> componentPlacement;
    private final ConcurrentHashMap<DeviceId, ConcurrentLinkedDeque<FrozenTransfer>> waitingTransfers;
    private final ConcurrentHashMap<DeviceId, ConcurrentLinkedDeque<WorkingTransfer>> workingTransfers;
    private final Semaphore mutex = new Semaphore(1);

    public StorageSystemImpl(Map<DeviceId, Integer> deviceTotalSlots, Map<ComponentId, DeviceId> componentPlacement) {
        this.freeSlots = new ConcurrentHashMap<>(deviceTotalSlots.size());
        this.waitingTransfers = new ConcurrentHashMap<>();
        this.workingTransfers = new ConcurrentHashMap<>();
        this.isBusy = new ConcurrentHashMap<>();
        for (var dev: deviceTotalSlots.keySet()) {
            this.freeSlots.put(dev, new AtomicInteger(deviceTotalSlots.get(dev)));
        }
        for (var dev: deviceTotalSlots.keySet()) {
            this.waitingTransfers.put(dev, new ConcurrentLinkedDeque<>());
            this.workingTransfers.put(dev, new ConcurrentLinkedDeque<>());
        }
        this.componentPlacement = new ConcurrentHashMap<>(componentPlacement);
        for (var component : componentPlacement.keySet()) {
            if (!freeSlots.containsKey(componentPlacement.get(component))) {
                throw new IllegalArgumentException("Device " + componentPlacement.get(component) + " does not exist");
            }
            if (freeSlots.get(componentPlacement.get(component)).decrementAndGet() < 0) {
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

        mutexAcquire();

        if (src == null && componentPlacement.containsKey(transfer.getComponentId())) {
            mutexRelease();
            throw new ComponentAlreadyExists(transfer.getComponentId(), componentPlacement.get(transfer.getComponentId()));
        }

        var currentSrc = componentPlacement.getOrDefault(transfer.getComponentId(), null);

        if ((currentSrc != null && !currentSrc.equals(src)) || (currentSrc == null && src != null)) {
            //assert src != null; // src == null => componentPlacement[transfer.getComponentId()] == null (the previous if)
            mutexRelease();
            throw new ComponentDoesNotExist(transfer.getComponentId(), src);
        }

        if (src != null && src.equals(dst)) {
            mutexRelease();
            throw new ComponentDoesNotNeedTransfer(transfer.getComponentId(), src);
        }

        if (isBusy
                .computeIfAbsent(transfer.getComponentId(), k -> new AtomicBoolean(false))
                .getAndSet(true)) {
            mutexRelease();
            throw new ComponentIsBeingOperatedOn(transfer.getComponentId());
        }

        if (dst == null) {
            noDepend(transfer);
        } else if (freeSlots.get(dst).decrementAndGet() >= 0) {
            noDepend(transfer);
        } else {
            freeSlots.get(dst).incrementAndGet();
            hasDepend(transfer);
        }
    }


    // This method runs when ct does not depend on any other transfers
    private void noDepend(ComponentTransfer ct) {
        // we assume we have mutex
        var childSem = wakeChain(ct.getSourceDeviceId());
        var wt = new WorkingTransfer();
        if (childSem == null && ct.getSourceDeviceId() != null) {
            // no chain, so we allow threads to reserve spot after us
            workingTransfers.get(ct.getSourceDeviceId()).add(wt);
        }
        mutexRelease();
        ct.prepare();
        if (childSem == null) {
            // no chain, we free spot after us
            mutexAcquire();
            freeSlot(ct.getSourceDeviceId(), wt);
        } else
            childSem.release(); // we release the chain
        ct.perform();
        mutexAcquire();
        // we update the state
        transferCompleted(ct);
        mutexRelease();
    }

    private void hasDepend(ComponentTransfer ct) {
        // we assume we have mutex
        if (ct.getSourceDeviceId() == null) {
            // no src, therefore impossible to have a cycle
            var ft = new FrozenTransfer(ct);
            waitingTransfers.get(ct.getDestinationDeviceId()).add(ft);
            mutexRelease();
            ft.waitInQueue(); // we wait for the transfer to be woken up
            onFrozenWake(ft);
        } else {
            var parentSem = new Semaphore(0);
            var wt = workingTransfers.get(ct.getDestinationDeviceId()).poll();
            if (wt != null) {
                // we have successfully reserved a spot
                wt.childSemaphore = parentSem;
                var myWt = new WorkingTransfer();
                if (ct.getSourceDeviceId() != null)
                    workingTransfers.get(ct.getSourceDeviceId()).add(myWt);
                mutexRelease();
                ct.prepare();
                wt.waitForPerform(); // we wait for the parent transfer to perform
                mutexAcquire();
                freeSlot(ct.getSourceDeviceId(), myWt);
                ct.perform();
                mutexAcquire();
                transferCompleted(ct);
                mutexRelease();
            } else {
                // no free spot, we need to find a cycle
                var childSem = wakeCycle(ct.getSourceDeviceId(), new HashSet<>(), ct.getDestinationDeviceId(), parentSem);
                if (childSem == null) {
                    // no cycle, we need to wait
                    var ft = new FrozenTransfer(ct);
                    waitingTransfers.get(ct.getDestinationDeviceId()).add(ft);
                    mutexRelease();
                    ft.waitInQueue();
                    onFrozenWake(ft);
                } else {
                    // we found a cycle, we can execute
                    mutexRelease();
                    ct.prepare();
                    childSem.release(); // our dependant can start performing
                    try {
                        parentSem.acquire(); // our parent has prepared, we can start performing
                    } catch (InterruptedException e) {
                        throw new RuntimeException("panic: unexpected thread interruption");
                    }
                    ct.perform();
                    mutexAcquire();
                    transferCompleted(ct);
                    mutexRelease();
                }
            }
        }
    }

    private void onFrozenWake(FrozenTransfer ft) {
        // we dont have mutex
        if (ft.parentSemaphore == null) {
            // no parent, but we were woken up, so someone reserved a spot for us
            mutexAcquire();
            noDepend(ft.transfer);
        } else {
            // we have a parent, we need to wait for it to prepare, to start performing
            var wt = new WorkingTransfer();
            if (ft.transfer.getSourceDeviceId() != null && ft.childSemaphore == null) {
                mutexAcquire();
                workingTransfers.get(ft.transfer.getSourceDeviceId()).add(wt);
                mutexRelease();
            }
            ft.transfer.prepare();
            mutexAcquire();
            if (ft.childSemaphore != null) {
                mutexRelease();
                ft.childSemaphore.release();
            } else
                freeSlot(ft.transfer.getSourceDeviceId(), wt);
            ft.waitForParent();
            ft.transfer.perform();
            mutexAcquire();
            transferCompleted(ft.transfer);
            mutexRelease();
        }
    }

    // this method finds a chain of transfers that all depend on each other and all depend on start
    private Semaphore wakeChain(DeviceId start) {
        // we assume we have mutex
        if (start == null) return null;
        var ft = waitingTransfers.get(start).poll();
        if (ft == null) return null;
        var sem = wakeChain(ft.transfer.getSourceDeviceId());
        if (sem != null)
            ft.childSemaphore = sem;
        sem = new Semaphore(0);
        ft.parentSemaphore = sem;
        ft.queueLeft.release();
        return sem;
    }

    // this method finds a cycle of transfers that all depend on each other and all depend on start and the last one transfers out from target
    private Semaphore wakeCycle(DeviceId start, HashSet<FrozenTransfer> visited, DeviceId target, Semaphore startSem) {
        // we assume we have mutex
        // assert target != null;
        if (start == null) return null;
        for (var ft: waitingTransfers.get(start)) {
            if (visited.contains(ft)) continue;
            visited.add(ft);
            if (target.equals(ft.transfer.getSourceDeviceId())) {
                waitingTransfers.get(start).remove(ft);
                ft.childSemaphore = startSem;
                ft.parentSemaphore = new Semaphore(0);
                ft.queueLeft.release();
                return ft.parentSemaphore;
            }
            var childSem = wakeCycle(ft.transfer.getSourceDeviceId(), visited, target, startSem);
            if (childSem != null) {
                waitingTransfers.get(start).remove(ft);
                ft.childSemaphore = childSem;
                ft.parentSemaphore = new Semaphore(0);
                ft.queueLeft.release();
                return ft.parentSemaphore;
            }
        }
        return null;
    }

    // this method runs when a transfer prepare has finished
    // it releases the slot, either by waking up sth or increasing freeSlots
    private void freeSlot(DeviceId device, WorkingTransfer wt) {
        // we assume we have mutex
        if (device == null) {
            mutexRelease();
            return;
        }
        workingTransfers.get(device).remove(wt);
        if (wt.childSemaphore != null) {
            mutexRelease();
            wt.childSemaphore.release();
        } else {
            var ft = waitingTransfers.get(device).poll();
            if (ft == null) {
                freeSlots.get(device).incrementAndGet();
                mutexRelease();
            } else {
                mutexRelease();
                ft.queueLeft.release();
            }
        }
    }

    // this method runs when a transfer is completed (perfom() has finished)
    private void transferCompleted(ComponentTransfer ct) {
        // we assume we have mutex
        var dst = ct.getDestinationDeviceId();
        if (dst == null) {
            componentPlacement.remove(ct.getComponentId());
            isBusy.remove(ct.getComponentId());
        } else {
            componentPlacement.put(ct.getComponentId(), dst);
            isBusy.remove(ct.getComponentId());
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

    private void mutexRelease() {
        mutex.release();
    }
}
