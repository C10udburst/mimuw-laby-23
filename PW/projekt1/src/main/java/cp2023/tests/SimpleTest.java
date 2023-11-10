package cp2023.tests;

import cp2023.base.ComponentId;
import cp2023.base.DeviceId;
import cp2023.solution.StorageSystemFactory;
import org.junit.jupiter.api.Test;

import java.util.List;
import java.util.Map;
import java.util.concurrent.atomic.AtomicInteger;

import static cp2023.tests.Utils.*;
import static org.junit.jupiter.api.Assertions.assertEquals;

public class SimpleTest {
    @Test
    public void multipleNoCycle() {
        var prepareCount = new AtomicInteger(0);
        var performCount = new AtomicInteger(0);
        var transfers = List.of(
            createTransfer(1, 2, 1, prepareCount, performCount, 0),
            createTransfer(2, 3, 2, prepareCount, performCount, 0),
            createTransfer(3, 1, 3, prepareCount, performCount, 0)
        );
        var system = StorageSystemFactory.newSystem(
            Map.of(
                new DeviceId(1), 2,
                new DeviceId(2), 2,
                new DeviceId(3), 2
            ),
            Map.of(
                new ComponentId(1), new DeviceId(1),
                new ComponentId(2), new DeviceId(2),
                new ComponentId(3), new DeviceId(3)
            )
        );
        execute(system, transfers, 3, 10);
        assertEquals(3, prepareCount.get(), "Prepare");
        assertEquals(3, performCount.get(), "Perform");
        assertComponentOnDevice(system, new ComponentId(1), new DeviceId(2));
        assertComponentOnDevice(system, new ComponentId(2), new DeviceId(3));
        assertComponentOnDevice(system, new ComponentId(3), new DeviceId(1));
    }

    @Test
    public void singleDevice() {
        var prepareCount = new AtomicInteger(0);
        var performCount = new AtomicInteger(0);
        var transfers = List.of(
            createTransfer(1, null, 1, prepareCount, performCount, 2),
            createTransfer(null, 1, 2, prepareCount, performCount, 0)
        );
        var system = StorageSystemFactory.newSystem(
            Map.of(
                new DeviceId(1), 1
            ),
            Map.of(
                new ComponentId(1), new DeviceId(1)
            )
        );
        execute(system, transfers, 3, 10);
        assertEquals(transfers.size(), prepareCount.get(), "Prepare");
        assertEquals(transfers.size(), performCount.get(), "Perform");
        assertComponentOnDevice(system, new ComponentId(2), new DeviceId(1));
    }
}
