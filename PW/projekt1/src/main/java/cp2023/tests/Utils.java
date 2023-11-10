package cp2023.tests;

import cp2023.base.ComponentId;
import cp2023.base.ComponentTransfer;
import cp2023.base.DeviceId;
import cp2023.base.StorageSystem;
import cp2023.exceptions.TransferException;

import java.util.ArrayList;
import java.util.List;
import java.util.concurrent.Callable;
import java.util.concurrent.ConcurrentLinkedQueue;
import java.util.concurrent.Executors;
import java.util.concurrent.TimeUnit;
import java.util.concurrent.atomic.AtomicInteger;

import static org.junit.jupiter.api.Assertions.assertDoesNotThrow;
import static org.junit.jupiter.api.Assertions.assertTrue;

public class Utils {

    public static void assertComponentOnDevice(StorageSystem system, ComponentId component, DeviceId device) {
        assertDoesNotThrow(() -> system.execute(new ComponentTransfer() {
            @Override
            public ComponentId getComponentId() {
                return component;
            }

            @Override
            public DeviceId getSourceDeviceId() {
                return device;
            }

            @Override
            public DeviceId getDestinationDeviceId() {
                return null;
            }

            @Override
            public void prepare() { }

            @Override
            public void perform() { }
        }));
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

    public static void execute(StorageSystem system, List<ComponentTransfer> transfers, int pool, int limit) {
        var errors = new ConcurrentLinkedQueue<Exception>();
        List<Callable<Void>> runnables = new ArrayList<>();
        for (var transfer : transfers) {
            runnables.add(() -> {
                try {
                    system.execute(transfer);
                } catch (TransferException e) {
                    errors.add(e);
                }
                return null;
            });
        }
        var executor = Executors.newFixedThreadPool(pool);
        assertDoesNotThrow(() -> executor.invokeAll(runnables, limit, TimeUnit.SECONDS));

        assertTrue(errors.isEmpty(), "Errors: " + errors);
    }
}
