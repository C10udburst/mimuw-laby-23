package cp2023.tests.v1;

import cp2023.base.ComponentId;
import cp2023.base.ComponentTransfer;
import cp2023.base.DeviceId;
import cp2023.solution.StorageSystemFactory;
import org.junit.jupiter.api.RepeatedTest;
import org.junit.jupiter.api.RepetitionInfo;

import java.util.List;
import java.util.Map;
import java.util.Random;
import java.util.concurrent.Semaphore;

import static cp2023.tests.v1.Utils.*;

public class V1InterlacedTest {
    private static class SemaphoreTransfer implements ComponentTransfer {

        private final Semaphore semaphore;
        private final ComponentId componentId;
        private final DeviceId sourceDeviceId;
        private final DeviceId destinationDeviceId;


        public SemaphoreTransfer(Semaphore semaphore, int componentId, Integer sourceDeviceId, Integer destinationDeviceId) {
            this.semaphore = semaphore;
            this.componentId = new ComponentId(componentId);
            this.sourceDeviceId = (sourceDeviceId == null) ? null : new DeviceId(sourceDeviceId);
            this.destinationDeviceId = (destinationDeviceId == null) ? null : new DeviceId(destinationDeviceId);
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

    @RepeatedTest(value=5, name = "cycleShrimple {currentRepetition}/{totalRepetitions}")
    public void cycleShrimple(RepetitionInfo ri) {
        var semaphores = List.of(new Semaphore(0), new Semaphore(0), new Semaphore(0));
        List<ComponentTransfer> transfers = List.of(
                new SemaphoreTransfer(semaphores.get(0), 1, 1, 2),
                new SemaphoreTransfer(semaphores.get(1), 2, 2, 3),
                new SemaphoreTransfer(semaphores.get(2), 3, 3, 1)
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

        var seed = ri.getCurrentRepetition() * 2137L;
        var random = new Random(seed);

        var t = new Thread(() -> {
            while (true) {
                try {
                    Thread.sleep(100);
                } catch (InterruptedException e) {
                    Thread.currentThread().interrupt();
                }
                semaphores.get(random.nextInt(3)).release();
            }
        });
        t.start();
        execute(system, transfers, 3, 20, 0);
        assertComponentOnDevice(system, new ComponentId(1), new DeviceId(2));
        assertComponentOnDevice(system, new ComponentId(2), new DeviceId(3));
        assertComponentOnDevice(system, new ComponentId(3), new DeviceId(1));
    }

    @RepeatedTest(value=5, name = "simpleSingleDevice {currentRepetition}/{totalRepetitions}")
    public void simpleSingleDevice(RepetitionInfo ri) {
        var semaphores = List.of(new Semaphore(0), new Semaphore(0), new Semaphore(0));
        List<ComponentTransfer> transfers = List.of(
                /*createTransfer(1, null, 1, prepareCount, performCount, 2),
                createTransfer(null, 1, 2, prepareCount, performCount, 0)*/
                new SemaphoreTransfer(semaphores.get(0), 1, 1, null),
                new SemaphoreTransfer(semaphores.get(1), 2, null, 1)
        );
        var system = StorageSystemFactory.newSystem(
                Map.of(
                        new DeviceId(1), 1
                ),
                Map.of(
                        new ComponentId(1), new DeviceId(1)
                )
        );

        var seed = ri.getCurrentRepetition() * 2137L;
        var random = new Random(seed);

        var t = new Thread(() -> {
            while (true) {
                try {
                    Thread.sleep(100);
                } catch (InterruptedException e) {
                    Thread.currentThread().interrupt();
                }
                semaphores.get(random.nextInt(2)).release();
            }
        });
        t.start();
        execute(system, transfers, 3, 20, 0);
        assertComponentOnDevice(system, new ComponentId(2), new DeviceId(1));
    }
}

