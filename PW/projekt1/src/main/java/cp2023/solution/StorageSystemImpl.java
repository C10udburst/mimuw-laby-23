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


    private void noDepend(ComponentTransfer ct) {
        var childSem = wakeChain(ct.getSourceDeviceId());
        mutex.release();
        ct.prepare();
        if (childSem == null)
            freeSlot(ct.getSourceDeviceId());
        else
            childSem.release();
        ct.perform();
        mutexAcquire();
        transferCompleted(ct);
        mutex.release();
    }

    private void hasDepend(ComponentTransfer ct) {
        if (ct.getSourceDeviceId() == null) {
            var ft = new FrozenTransfer(ct);
            waitingTransfers.get(ct.getDestinationDeviceId()).add(ft);
            mutex.release();
            ft.waitInQueue();
            onFrozenWake(ft);
        } else {
            var parentSem = new Semaphore(0);
            var childSem = wakeCycle(ct.getSourceDeviceId(), new HashSet<>(), ct.getDestinationDeviceId(), parentSem);
            if (childSem == null) {
                var ft = new FrozenTransfer(ct);
                waitingTransfers.get(ct.getDestinationDeviceId()).add(ft);
                mutex.release();
                ft.waitInQueue();
                onFrozenWake(ft);
            } else {
                mutex.release();
                ct.prepare();
                childSem.release();
                try {
                    parentSem.acquire();
                } catch (InterruptedException e) {
                    throw new RuntimeException("panic: unexpected thread interruption");
                }
                ct.perform();
                mutexAcquire();
                transferCompleted(ct);
                mutex.release();
            }
        }
    }

    private void onFrozenWake(FrozenTransfer ft) {
        if (ft.parentSemaphore == null) {
            noDepend(ft.transfer);
        } else {
            ft.transfer.prepare();
            if (ft.childSemaphore != null)
                ft.childSemaphore.release();
            else
                freeSlot(ft.transfer.getSourceDeviceId());
            ft.waitForParent();
            ft.transfer.perform();
            mutexAcquire();
            transferCompleted(ft.transfer);
            mutex.release();
        }
    }

    private Semaphore wakeChain(DeviceId start) {
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

    private Semaphore wakeCycle(DeviceId start, HashSet<FrozenTransfer> visited, DeviceId target, Semaphore startSem) {
        assert target != null;
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

    private void freeSlot(DeviceId device) {
        if (device == null) return;
        mutexAcquire();
        var ft = waitingTransfers.get(device).poll();
        if (ft == null) {
            freeSlots.computeIfPresent(device, (k, v) -> v + 1);
        } else {
            ft.queueLeft.release();
        }
        mutex.release();
    }

    private void transferCompleted(ComponentTransfer ct) {
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
}
