package cp2023.solution;

import java.util.concurrent.ConcurrentHashMap;

import cp2023.base.ComponentTransfer;
import cp2023.base.DeviceId;
import cp2023.base.StorageSystem;
import cp2023.exceptions.TransferException;

public class StorageSystemImpl implements StorageSystem {

    public final ConcurrentHashMap<DeviceId, Device> devices;

    public StorageSystemImpl(ConcurrentHashMap<DeviceId, Device> devices) {
        this.devices = devices;
    }

    @Override
    public void execute(ComponentTransfer transfer) throws TransferException {
        
        
    }
    
}
