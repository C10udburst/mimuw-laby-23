package cp2023.tests;

import cp2023.base.ComponentId;
import cp2023.base.ComponentTransfer;
import cp2023.base.DeviceId;
import cp2023.base.StorageSystem;

import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.concurrent.*;
import java.util.concurrent.atomic.AtomicInteger;

import static org.junit.jupiter.api.Assertions.*;

public class Utils {

    public static void assertComponentOnDevice(StorageSystem system, ComponentId component, DeviceId device) {
        // ugly reflection, assumes that there will exist a field of type HashMap<ComponentId, DeviceId> in StorageSystem
        for (var field: system.getClass().getDeclaredFields()) {
            try {
                var couldAcess = field.canAccess(system);
                field.setAccessible(true);
                try {
                    var componentPlacement = (HashMap<?, ?>) field.get(system);
                    if (componentPlacement.keySet().stream().findFirst().get().getClass() != ComponentId.class) {
                        continue;
                    }
                    if (componentPlacement.values().stream().findFirst().get().getClass() != DeviceId.class) {
                        continue;
                    }
                    assertEquals(device, componentPlacement.get(component), "Component " + component + " on device " + device);
                } catch (ClassCastException | IllegalAccessException | IllegalArgumentException ignored) {
                }
                field.setAccessible(couldAcess);
            } catch (IllegalArgumentException ignored) { }
        }
    }

    public static ComponentTransfer createTransfer(Integer from, Integer to, int component, AtomicInteger prepareCount, AtomicInteger performCount, int sleep) {
        return new ComponentTransfer() {
            @Override
            public ComponentId getComponentId() {
                return new ComponentId(component);
            }

            @Override
            public DeviceId getSourceDeviceId() {
                return (from == null) ? null : new DeviceId(from);
            }

            @Override
            public DeviceId getDestinationDeviceId() {
                return (to == null) ? null : new DeviceId(to);
            }

            @Override
            public void prepare() {
                try {
                    Thread.sleep(sleep);
                } catch (InterruptedException e) {
                    throw new RuntimeException(e);
                }
                System.out.println("prepare " + component + ": " + from + "->" + to);
                prepareCount.incrementAndGet();
            }

            @Override
            public void perform() {
                try {
                    Thread.sleep(sleep);
                } catch (InterruptedException e) {
                    throw new RuntimeException(e);
                }
                System.out.println("perform " + component + ": " + from + "->" + to);
                performCount.incrementAndGet();
            }
        };
    }

    public static void execute(StorageSystem system, List<ComponentTransfer> transfers, int pool, int limit, int interruptCount) {
        var errors = new ConcurrentLinkedQueue<Exception>();
        var interruptions = new AtomicInteger(0);
        List<Callable<Void>> runnables = new ArrayList<>();
        var latch = new CountDownLatch(transfers.size());
        for (var transfer : transfers) {
            runnables.add(() -> {
                try {
                    system.execute(transfer);
                } catch (Exception e) {
                    if (e.getMessage().equals("panic: unexpected thread interruption")) {
                        interruptions.incrementAndGet();
                        latch.countDown();
                    } else {
                        e.printStackTrace();
                        errors.add(e);
                    }
                }
                latch.countDown();
                return null;
            });
        }
        var executor = Executors.newFixedThreadPool(pool);
        assertDoesNotThrow(() -> executor.invokeAll(runnables, limit, TimeUnit.SECONDS));
        try {
            latch.await();
        } catch (InterruptedException e) {
            e.printStackTrace();
        }
        assertEquals(0, errors.size(), "Errors");
        assertEquals(interruptCount, interruptions.get(), "Interrupts");
    }
}
