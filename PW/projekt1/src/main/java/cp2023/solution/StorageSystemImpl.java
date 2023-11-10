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
import java.util.concurrent.atomic.AtomicInteger;

public class StorageSystemImpl implements StorageSystem {

    private final ConcurrentHashMap<DeviceId, AtomicInteger> freeSlots;
    private final ConcurrentHashMap<ComponentId, AtomicBoolean> isBusy;
    private final ConcurrentHashMap<ComponentId, DeviceId> componentPlacement;
    private final ConcurrentHashMap<DeviceId, ConcurrentLinkedDeque<FrozenTransfer>> waitingTransfers;
    private final ConcurrentHashMap<DeviceId, Semaphore> nowSem;

    public StorageSystemImpl(Map<DeviceId, Integer> deviceTotalSlots, Map<ComponentId, DeviceId> componentPlacement) {
        this.freeFuture = new ConcurrentHashMap<>(deviceTotalSlots.size());
        this.nowSem = new ConcurrentHashMap<>(deviceTotalSlots.size());
        this.waitingTransfers = new ConcurrentHashMap<>();
        this.isBusy = new ConcurrentHashMap<>();
        for (var entry : deviceTotalSlots.entrySet()) {
            var device = entry.getKey();
            var slots = entry.getValue();
            this.waitingTransfers.put(device, new ConcurrentLinkedDeque<>());
            this.nowSem.put(device, new Semaphore(slots, true));
            this.freeFuture.put(device, new AtomicInteger(slots));
        }
        this.componentPlacement = new ConcurrentHashMap<>(componentPlacement);
        for (var component : componentPlacement.keySet()) {
            freeFuture.get(componentPlacement.get(component)).decrementAndGet();
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

        if (dst == null)
            delete(transfer);
        else
            move(transfer);
    }

    private void move(ComponentTransfer transfer) {
        var dst = transfer.getDestinationDeviceId();
        var src = transfer.getSourceDeviceId();

        var future = freeFuture.get(dst).getAndDecrement();
        if (future < 0) {
            freeFuture.get(dst).incrementAndGet(); // reservation failed, revert
            var ft = cycleDFS(dst, new HashSet<>(), 0, src);
            if (ft == null) {
                ft = new FrozenTransfer(transfer, new Semaphore(0));
                waitingTransfers.get(dst).add(ft);
                ft.semAcquire(); // wait for a slot to be freed
                waitingTransfers.get(dst).remove(ft);
                // we assume whoever woke us up has reserved a slot for us
            }

            assert ft.barrier != null;
            if (ft.isInCycle) {
                ft.barrierWait();
                transfer.prepare();
                ft.barrierWait();
                transfer.perform();
                ft.barrierWait();
            } else {
                transfer.prepare();
                ft.barrierWait();
                transfer.perform();
            }
            componentPlacement.put(transfer.getComponentId(), dst);
            isBusy.get(transfer.getComponentId()).set(false);

        } else {
            // we are not in a cycle, we can proceed
            transfer.prepare();
            try {
                nowSem.get(dst).acquire();
            } catch (InterruptedException e) {
                throw new RuntimeException(e);
            }
            if (src != null) {
                freeFuture.get(src).incrementAndGet();
            }
            // we have reserved a slot, we can proceed
            transfer.perform();
            if (src != null) {
                nowSem.get(src).release();
            }
            componentPlacement.put(transfer.getComponentId(), dst);
            isBusy.get(transfer.getComponentId()).set(false);
        }
    }

    private void delete(ComponentTransfer transfer) {
        var src = transfer.getSourceDeviceId();

        // TODO: is this safe
        if (waitingTransfers.get(src).isEmpty()) {
            freeFuture.get(src).incrementAndGet();
            transfer.prepare();
            transfer.perform();
            componentPlacement.remove(transfer.getComponentId());
            nowSem.get(src).release();
            isBusy.remove(transfer.getComponentId());
        } else {
            transfer.prepare();
            var ft = waitingTransfers.get(src).removeFirst();
            ft.barrier = new CyclicBarrier(2);
            ft.semaphore.release(); // wake up the thread waiting for a slot
            transfer.perform();
            componentPlacement.remove(transfer.getComponentId());
            isBusy.remove(transfer.getComponentId());
            ft.barrierWait();

        }
    }

    private void checkDevice(DeviceId device) throws TransferException {
        if (device == null) return;
        if (!freeFuture.containsKey(device) || !nowSem.containsKey(device)) {
            throw new DeviceDoesNotExist(device);
        }
    }

    private FrozenTransfer cycleDFS(DeviceId start, HashSet<FrozenTransfer> visited, int depth, DeviceId target) {
        var waiting = waitingTransfers.get(start);
        for (var transfer: waiting) {
            if (visited.contains(transfer)) continue;
            visited.add(transfer);
            if (transfer.transfer.getDestinationDeviceId().equals(target)) {
                transfer.barrier = new CyclicBarrier(depth);
                transfer.isInCycle = true;
                transfer.semaphore.release(); // wake up each thread in the cycle
                return transfer;
            }
            var res = cycleDFS(transfer.transfer.getDestinationDeviceId(), visited, depth+1, target);
            if (res != null) {
                transfer.barrier = res.barrier;
                transfer.isInCycle = true;
                transfer.semaphore.release(); // wake up each thread in the cycle
                return res;
            }
        }
        return null;
    }
}
