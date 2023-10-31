package cp2023.solution;

import java.util.concurrent.ConcurrentHashMap;
import java.util.concurrent.atomic.AtomicBoolean;

import cp2023.base.ComponentId;
import cp2023.base.ComponentTransfer;
import cp2023.base.DeviceId;
import cp2023.base.StorageSystem;
import cp2023.exceptions.ComponentAlreadyExists;
import cp2023.exceptions.ComponentDoesNotExist;
import cp2023.exceptions.ComponentDoesNotNeedTransfer;
import cp2023.exceptions.ComponentIsBeingOperatedOn;
import cp2023.exceptions.DeviceDoesNotExist;
import cp2023.exceptions.IllegalTransferType;
import cp2023.exceptions.TransferException;

public class StorageSystemImpl implements StorageSystem {

    // TODO: maybe this doesn't need to be concurrent
    private final ConcurrentHashMap<DeviceId, Device> devices;
    private final ConcurrentHashMap<ComponentId, AtomicBoolean> isBusy = new ConcurrentHashMap<>();
    private final ConcurrentHashMap<ComponentId, DeviceId> componentToDevice = new ConcurrentHashMap<>();

    public StorageSystemImpl(ConcurrentHashMap<DeviceId, Device> devices) {
        this.devices = devices;
    }

    @Override
    public void execute(ComponentTransfer transfer) throws TransferException {
        var src = transfer.getSourceDeviceId();
        var dst = transfer.getDestinationDeviceId();

        if (src == null && dst == null) {
            throw new IllegalTransferType(transfer.getComponentId());
        }

        ensureDevExists(src);
        ensureDevExists(dst);
        
        acquireComponent(transfer.getComponentId());
        
        if (componentToDevice.get(transfer.getComponentId()) == dst && dst != null) {
            releaseComponent(transfer.getComponentId());
            throw new ComponentDoesNotNeedTransfer(transfer.getComponentId(), dst);
        }

        if (componentToDevice.get(transfer.getComponentId()) != src && src != null) {
            releaseComponent(transfer.getComponentId());
            throw new ComponentDoesNotExist(transfer.getComponentId(), src);
        }

        if (src == null && componentToDevice.containsKey(transfer.getComponentId())) {
            releaseComponent(transfer.getComponentId());
            throw new ComponentAlreadyExists(transfer.getComponentId());
        }

        if (dst != null)
            devices.get(dst).acquireFutureSlot();

        if (src != null)
            devices.get(src).releaseFutureSlot();

        transfer.prepare();

        if (dst != null) {
            devices.get(dst).acquireSlot();
            componentToDevice.put(transfer.getComponentId(), dst);
        }

        transfer.perform();

        if (src != null)
            devices.get(src).releaseSlot();

        releaseComponent(transfer.getComponentId());
    }
    

    private void ensureDevExists(DeviceId deviceId) throws DeviceDoesNotExist {
        if (deviceId != null) {
            if (!devices.containsKey(deviceId)) {
                throw new DeviceDoesNotExist(deviceId);
            }
        }
    }

    // TODO: maybe we should use computeIfPresent instead of get().getAndSet()
    private void acquireComponent(ComponentId componentId) throws ComponentIsBeingOperatedOn {
        isBusy.putIfAbsent(componentId, new AtomicBoolean(false));

        if (isBusy.get(componentId).getAndSet(true)) {
            throw new ComponentIsBeingOperatedOn(componentId);
        }
    }

    private void releaseComponent(ComponentId componentId) {
        isBusy.get(componentId).set(false);
    }
}
