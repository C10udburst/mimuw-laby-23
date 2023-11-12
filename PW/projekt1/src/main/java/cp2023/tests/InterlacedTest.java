package cp2023.tests;

import cp2023.base.ComponentId;
import cp2023.base.ComponentTransfer;
import cp2023.base.DeviceId;
import cp2023.solution.StorageSystemFactory;
import org.junit.jupiter.api.Test;

import java.util.List;
import java.util.Map;
import java.util.concurrent.Semaphore;

import static cp2023.tests.Utils.*;

public class InterlacedTest {
    private class SemaphoreTransfer implements ComponentTransfer {

        private final Semaphore semaphore;
        private final ComponentId componentId;
        private final DeviceId sourceDeviceId;
        private final DeviceId destinationDeviceId;


        public SemaphoreTransfer(Semaphore semaphore, int componentId, int sourceDeviceId, int destinationDeviceId) {
            this.semaphore = semaphore;
            this.componentId = new ComponentId(componentId);
            this.sourceDeviceId = new DeviceId(sourceDeviceId);
            this.destinationDeviceId = new DeviceId(destinationDeviceId);
        }

        @Override
        public ComponentId getComponentId() {
            semaphore.acquireUninterruptibly();
            System.out.println("getComponentId for " + componentId);
            return componentId;
        }

        @Override
        public DeviceId getSourceDeviceId() {
            semaphore.acquireUninterruptibly();
            System.out.println("getSourceDeviceId for " + componentId);
            return sourceDeviceId;
        }

        @Override
        public DeviceId getDestinationDeviceId() {
            semaphore.acquireUninterruptibly();
            System.out.println("getDestinationDeviceId for " + componentId);
            return destinationDeviceId;
        }

        @Override
        public void prepare() {
            semaphore.acquireUninterruptibly();
            System.out.println("prepare for " + componentId);
        }

        @Override
        public void perform() {
            semaphore.acquireUninterruptibly();
            System.out.println("perform for " + componentId);
        }
    }

    @Test
    public void test1() {
        var semaphore = new Semaphore(0, false);
        List<ComponentTransfer> transfers = List.of(
                new SemaphoreTransfer(semaphore, 1, 1, 2),
                new SemaphoreTransfer(semaphore, 2, 2, 3),
                new SemaphoreTransfer(semaphore, 3, 3, 1)
        );
        var system = StorageSystemFactory.newSystem(
                Map.of(
                        new DeviceId(1), 1,
                        new DeviceId(2), 1,
                        new DeviceId(3), 1
                ),
                Map.of(
                        new ComponentId(1), new DeviceId(1),
                        new ComponentId(2), new DeviceId(2),
                        new ComponentId(3), new DeviceId(3)
                )
        );

        var t = new Thread(() -> {
            while (true) {
                try {
                    Thread.sleep(100);
                } catch (InterruptedException e) {
                    Thread.currentThread().interrupt();
                }
                semaphore.release(Math.random() < 0.5 ? 1 : 2);
            }
        });
        t.start();
        execute(system, transfers, 3, 20, 0);
        assertComponentOnDevice(system, new ComponentId(1), new DeviceId(2));
        assertComponentOnDevice(system, new ComponentId(2), new DeviceId(3));
        assertComponentOnDevice(system, new ComponentId(3), new DeviceId(1));
    }
}

