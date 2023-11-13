package cp2023.tests.v2;

import cp2023.base.ComponentId;
import cp2023.base.ComponentTransfer;
import cp2023.base.DeviceId;
import cp2023.base.StorageSystem;
import cp2023.solution.StorageSystemFactory;

import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Objects;
import java.util.concurrent.*;
import java.util.stream.Collectors;
import java.util.stream.Stream;

import static org.junit.jupiter.api.Assertions.assertDoesNotThrow;

public class Utils {
    public static List<Exception> execute(StorageSystem system, List<CustomTransfer> transfers, int pool, int limit) {
        var errors = new ConcurrentLinkedQueue<Exception>();
        var latch = new CountDownLatch(transfers.size());
        var runnables = new ArrayList<Callable<Void>>();
        for (int i = 0; i < transfers.size(); i++) {
            var t = transfers.get(i);
            var sleepTime = (long) i * (limit*50L) / transfers.size();
            runnables.add(() -> {
                try {
                    Thread.sleep(sleepTime);
                    t.sleepInit();
                    System.out.println("Sleeping " + t + " for " + sleepTime + "ms");
                    system.execute(t);
                } catch (Exception e) {
                    if (e instanceof InterruptedException || e.getMessage().equals("panic: unexpected thread interruption")) {
                        assertDoesNotThrow(() -> { throw e; });
                    }
                    errors.add(e);
                } finally {
                    latch.countDown();
                }
                return null;
            });
        }
        var executor = Executors.newFixedThreadPool(pool);
        assertDoesNotThrow(() -> executor.invokeAll(runnables, limit*100000, TimeUnit.SECONDS));
        assertDoesNotThrow(() -> latch.await());
        return new ArrayList<>(errors);
    }

    public static void assertComponentOnDevice(StorageSystem system, int component, int device) {
        var transferTest = new ComponentTransfer() {
            @Override
            public ComponentId getComponentId() {
                return new ComponentId(component);
            }

            @Override
            public DeviceId getSourceDeviceId() {
                return new DeviceId(device);
            }

            @Override
            public DeviceId getDestinationDeviceId() {
                return null;
            }

            @Override
            public void prepare() { }

            @Override
            public void perform() { }
        };
        assertDoesNotThrow(() -> system.execute(transferTest));
    }

    public static StorageSystem filledSystem(List<? extends ComponentTransfer> transfers) {
        var deviceCapacity = new HashMap<DeviceId, Integer>();
        var componentPlacement = new HashMap<ComponentId, DeviceId>();
        for (var transfer : transfers) {
            var src = transfer.getSourceDeviceId();
            if (src != null) {
                deviceCapacity.put(src, deviceCapacity.getOrDefault(src, 0) + 1);
            }
            componentPlacement.put(transfer.getComponentId(), src);
        }
        return StorageSystemFactory.newSystem(deviceCapacity, componentPlacement);
    }

    public static void unlockSeconds(Semaphore sem, int time) {
        Thread t = new Thread(() -> {
            try {
                Thread.sleep(time * 1000L);
            } catch (InterruptedException e) {
                throw new RuntimeException(e);
            }
            sem.release();
        });
        t.start();
    }
}
