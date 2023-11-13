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
        validateDeviceTotalSlots(deviceTotalSlots);
        validateComponentPlacement(componentPlacement);
        return new StorageSystemImpl(deviceTotalSlots, componentPlacement);
    }

    private static void validateDeviceTotalSlots(Map<DeviceId, Integer> deviceTotalSlots) {
        if (deviceTotalSlots == null) {
            throw new IllegalArgumentException("Device total slots cannot be null.");
        }
        for (var entry : deviceTotalSlots.entrySet()) {
            if (entry.getKey() == null) {
                throw new IllegalArgumentException("Device ID cannot be null.");
            }
            if (entry.getValue() == null) {
                throw new IllegalArgumentException("Device total slots cannot be null.");
            }
            if (entry.getValue() <= 0) {
                throw new IllegalArgumentException("Device total slots must be positive.");
            }
        }
    }

    private static void validateComponentPlacement(Map<ComponentId, DeviceId> componentPlacement) {
        if (componentPlacement == null) {
            throw new IllegalArgumentException("Component placement cannot be null.");
        }
        for (var entry : componentPlacement.entrySet()) {
            if (entry.getKey() == null) {
                throw new IllegalArgumentException("Component ID cannot be null.");
            }
            if (entry.getValue() == null) {
                throw new IllegalArgumentException("Device ID cannot be null.");
            }
        }
    }
}
