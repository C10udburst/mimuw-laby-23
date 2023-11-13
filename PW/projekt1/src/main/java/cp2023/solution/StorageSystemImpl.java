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
        for (var device : deviceTotalSlots.keySet()) {
            this.waitingTransfers.put(device, new ConcurrentLinkedDeque<>());
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

        if (src == dst) {
            throw new ComponentDoesNotNeedTransfer(transfer.getComponentId(), src);
        }

        checkDevice(src);
        checkDevice(dst);

        if (src == null && componentPlacement.containsKey(transfer.getComponentId())) {
            throw new ComponentAlreadyExists(transfer.getComponentId());
        }

        var currentSrc = componentPlacement.getOrDefault(transfer.getComponentId(), null);

        if ((currentSrc != null && !currentSrc.equals(src)) || (currentSrc == null && src != null)) {
            assert src != null; // src == null => componentPlacement[transfer.getComponentId()] == null (the previous if)
            throw new ComponentDoesNotExist(transfer.getComponentId(), src);
        }

        if (isBusy
                .computeIfAbsent(transfer.getComponentId(), k -> new AtomicBoolean(false))
                .getAndSet(true)) {
            throw new ComponentIsBeingOperatedOn(transfer.getComponentId());
        }

        // we are now the only ones operating on this component

        try {
            mutex.acquire();
        } catch (InterruptedException e) {
            throw new RuntimeException("panic: unexpected thread interruption");
        }

        if (dst == null || freeSlots.get(dst) > 0) {
            moveChain(transfer);
        } else {
            moveCycle(transfer);
        }
    }

    private void checkDevice(DeviceId device) throws TransferException {
        if (device == null) return;
        if (!waitingTransfers.containsKey(device) || !freeSlots.containsKey(device)) {
            throw new DeviceDoesNotExist(device);
        }
    }

    private void moveChain(ComponentTransfer transfer) {
        var barrier = chainBarrier(transfer.getSourceDeviceId(), 1);
        if (barrier == null) {
            // no transfers depend on this one
            transfer.prepare();
            transfer.perform(); // TODO: this could be done in parallel
            moveOrDelete(transfer);
        } else {
            try {
                barrier.await();
            } catch (InterruptedException | BrokenBarrierException e) {
                throw new RuntimeException("panic: unexpected thread interruption");
            }
            transfer.prepare();
            try {
                barrier.await();
            } catch (InterruptedException | BrokenBarrierException e) {
                throw new RuntimeException("panic: unexpected thread interruption");
            }
            transfer.perform();
            try {
                barrier.await();
            } catch (InterruptedException | BrokenBarrierException e) {
                throw new RuntimeException("panic: unexpected thread interruption");
            }
            moveOrDelete(transfer);
        }
    }

    private void moveCycle(ComponentTransfer transfer) {
        var dst = transfer.getDestinationDeviceId();
        var src = transfer.getSourceDeviceId();
        var ft = cycleDFSBarrier(src, new HashSet<>(), 1, dst);
        if (ft == null) {
            // no cycle, we have to wait for a free slot
            ft = new FrozenTransfer(transfer, new Semaphore(0));
            waitingTransfers.get(dst).add(ft);
            mutex.release();
            ft.semAcquire();
            // we are now in the cycle or a chain
            waitingTransfers.get(dst).remove(ft);
            ft.barrierWait();
            transfer.prepare();
            ft.barrierWait();
            transfer.perform();
            ft.barrierWait();
            componentPlacement.put(transfer.getComponentId(), dst);
            // we dont update freeSlots, cuz another transfer took our slot
            isBusy.remove(transfer.getComponentId());
        } else {
            mutex.release(); // we release the mutex, because we are the representative of the cycle
            ft.barrierWait();
            transfer.prepare();
            ft.barrierWait();
            transfer.perform();
            ft.barrierWait();
            componentPlacement.put(transfer.getComponentId(), dst);
            isBusy.remove(transfer.getComponentId());
        }

    }

    private void moveOrDelete(ComponentTransfer transfer) {
        var dst = transfer.getDestinationDeviceId();
        freeSlots.computeIfPresent(transfer.getSourceDeviceId(), (k, v) -> v + 1);
        if (dst == null) {
            mutex.release();
            componentPlacement.remove(transfer.getComponentId());
            isBusy.remove(transfer.getComponentId());
        } else {
            freeSlots.computeIfPresent(dst, (k, v) -> v - 1);
            mutex.release();
            componentPlacement.put(transfer.getComponentId(), dst);
            isBusy.remove(transfer.getComponentId());
        }
    }

    private CyclicBarrier chainBarrier(DeviceId start, int depth) {
        if (start == null) return null;
        var transfer = waitingTransfers.get(start).peek();
        if (transfer == null) return null;
        var barrier = chainBarrier(transfer.transfer.getSourceDeviceId(), depth+1);
        if (barrier == null) {
            transfer.barrier = new CyclicBarrier(depth);
            transfer.semaphore.release(); // wake up each thread in the cycle
            freeSlots.computeIfPresent(transfer.transfer.getSourceDeviceId(), (k, v) -> v + 1);
            return transfer.barrier;
        } else {
            transfer.barrier = barrier;
            transfer.semaphore.release(); // wake up each thread in the cycle
            return barrier;
        }
    }

    private FrozenTransfer cycleDFSBarrier(DeviceId start, HashSet<FrozenTransfer> visited, int depth, DeviceId target) {
        if (start == null) return null;
        var waiting = waitingTransfers.get(start);
        for (var transfer: waiting) {
            if (visited.contains(transfer)) continue;
            visited.add(transfer);
            if (transfer.transfer.getSourceDeviceId().equals(target)) {
                transfer.barrier = new CyclicBarrier(depth + 1);
                transfer.semaphore.release(); // wake up each thread in the cycle
                return transfer;
            }
            var res = cycleDFSBarrier(transfer.transfer.getSourceDeviceId(), visited, depth+1, target);
            if (res != null) {
                transfer.barrier = res.barrier;
                transfer.semaphore.release(); // wake up each thread in the cycle
                return res;
            }
        }
        return null;
    }
}
