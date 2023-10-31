/*
 * University of Warsaw
 * Concurrent Programming Course 2023/2024
 * Java Assignment
 *
 * Author: Konrad Iwanicki (iwanicki@mimuw.edu.pl)
 */
package cp2023.solution;

import java.util.Map;
import java.util.concurrent.ConcurrentHashMap;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;

import cp2023.base.ComponentId;
import cp2023.base.DeviceId;
import cp2023.base.StorageSystem;
import cp2023.exceptions.TransferException;


public final class StorageSystemFactory {

    public static StorageSystem newSystem(
        Map<DeviceId, Integer> deviceTotalSlots,
        Map<ComponentId, DeviceId> componentPlacement) {
        
        ConcurrentHashMap<DeviceId, Device> devices = new ConcurrentHashMap<>();
        for (Map.Entry<DeviceId, Integer> entry : deviceTotalSlots.entrySet()) {
            devices.put(entry.getKey(), new Device(entry.getValue()));
        }
    
        var storage = new StorageSystemImpl(devices);

        for (Map.Entry<ComponentId, DeviceId> entry : componentPlacement.entrySet()) {
            try {
                storage.execute(new ComponentCreate(entry.getKey(), entry.getValue()));
            } catch (TransferException e) {
                assert false;
            }
        }

        return storage;
    }

}
