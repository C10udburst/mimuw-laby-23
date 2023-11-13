package cp2023.tests.v1;


import cp2023.base.ComponentId;
import cp2023.base.DeviceId;
import cp2023.solution.StorageSystemFactory;
import org.junit.jupiter.api.Test;

import java.util.List;
import java.util.Map;
import java.util.concurrent.atomic.AtomicInteger;

import static cp2023.tests.v1.Utils.*;
import static org.junit.jupiter.api.Assertions.*;

public class V1CyclesTest {
    @Test
    public void shrimple() {
        var prepareCount = new AtomicInteger(0);
        var performCount = new AtomicInteger(0);
        var transfers = List.of(
                createTransfer(1, 2, 1, prepareCount, performCount, 0),
                createTransfer(2, 3, 2, prepareCount, performCount, 0),
                createTransfer(3, 1, 3, prepareCount, performCount, 0)
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
        execute(system, transfers, 3, 3, 0);
        assertEquals(3, prepareCount.get(), "Prepare");
        assertEquals(3, performCount.get(), "Perform");
        assertComponentOnDevice(system, new ComponentId(1), new DeviceId(2));
        assertComponentOnDevice(system, new ComponentId(2), new DeviceId(3));
        assertComponentOnDevice(system, new ComponentId(3), new DeviceId(1));
    }

    @Test
    public void moodle1() {
        // https://moodle.mimuw.edu.pl/mod/forum/discuss.php?d=9212
        var prepareCount = new AtomicInteger(0);
        var performCount = new AtomicInteger(0);
        var transfers = List.of(
                createTransfer(1, 2, 1, prepareCount, performCount, 0),
                createTransfer(4, 2, 2, prepareCount, performCount, 0),
                createTransfer(3, 4, 3, prepareCount, performCount, 0),
                createTransfer(3, 1, 4, prepareCount, performCount, 0),
                createTransfer(2, 3, 5, prepareCount, performCount, 1000)
        );

        var system = StorageSystemFactory.newSystem(
                Map.of(
                        new DeviceId(1), 1,
                        new DeviceId(2), 1,
                        new DeviceId(3), 2,
                        new DeviceId(4), 1
                ),
                Map.of(
                        new ComponentId(1), new DeviceId(1),
                        new ComponentId(2), new DeviceId(4),
                        new ComponentId(3), new DeviceId(3),
                        new ComponentId(4), new DeviceId(3),
                        new ComponentId(5), new DeviceId(2)
                )
        );

        execute(system, transfers, 10, 3, 2);
        // transfers 1, 5, 4 should be performed, but not 2, 3
        assertEquals(3, prepareCount.get(), "Prepare");
        assertEquals(3, performCount.get(), "Perform");
        assertComponentOnDevice(system, new ComponentId(1), new DeviceId(2));
        assertComponentOnDevice(system, new ComponentId(5), new DeviceId(3));
        assertComponentOnDevice(system, new ComponentId(4), new DeviceId(1));
    }

    @Test
    public void moodle2() {
        // https://moodle.mimuw.edu.pl/mod/forum/discuss.php?d=9217

        var prepareCount = new AtomicInteger(0);
        var performCount = new AtomicInteger(0);
        var transfers = List.of(
                createTransfer(5, 6, 1, prepareCount, performCount, 0),
                createTransfer(3, 4, 2, prepareCount, performCount, 1),
                createTransfer(1, 3, 3, prepareCount, performCount, 2),
                createTransfer(1, 2, 4, prepareCount, performCount, 3),
                createTransfer(2, 5, 5, prepareCount, performCount, 4),
                createTransfer(1, 4, 6, prepareCount, performCount, 5),
                createTransfer(3, 5, 7, prepareCount, performCount, 6),
                createTransfer(4, 5, 8, prepareCount, performCount, 7),
                createTransfer(2, 3, 9, prepareCount, performCount, 8),
                createTransfer(6, 1, 10, prepareCount, performCount, 9)
        );
        var system = StorageSystemFactory.newSystem(
                Map.of(
                        new DeviceId(1), 3,
                        new DeviceId(2), 2,
                        new DeviceId(3), 2,
                        new DeviceId(4), 1,
                        new DeviceId(5), 1,
                        new DeviceId(6), 1
                ),
                Map.of(
                        new ComponentId(1), new DeviceId(5),
                        new ComponentId(2), new DeviceId(3),
                        new ComponentId(3), new DeviceId(1),
                        new ComponentId(4), new DeviceId(1),
                        new ComponentId(5), new DeviceId(2),
                        new ComponentId(6), new DeviceId(1),
                        new ComponentId(7), new DeviceId(3),
                        new ComponentId(8), new DeviceId(4),
                        new ComponentId(9), new DeviceId(2),
                        new ComponentId(10), new DeviceId(6)
                )
        );
        execute(system, transfers, 10, 10, transfers.size() - 4); // all but 4 should fail
        assertEquals(4, prepareCount.get(), "Prepare");
        assertEquals(4, performCount.get(), "Perform");
        // transfers 10, 1, 5, 4 should be performed
        assertComponentOnDevice(system, new ComponentId(1), new DeviceId(6));
        assertComponentOnDevice(system, new ComponentId(5), new DeviceId(5));
        assertComponentOnDevice(system, new ComponentId(4), new DeviceId(2));
        assertComponentOnDevice(system, new ComponentId(10), new DeviceId(1));
    }

    @Test
    public void moodle21() {
        // https://moodle.mimuw.edu.pl/mod/forum/discuss.php?d=9217

        var prepareCount = new AtomicInteger(0);
        var performCount = new AtomicInteger(0);
        var transfers = List.of(
                createTransfer(5, 6, 1, prepareCount, performCount, 0),
                createTransfer(3, 4, 2, prepareCount, performCount, 10),
                createTransfer(1, 3, 3, prepareCount, performCount, 20),
                //createTransfer(1, 2, 4, prepareCount, performCount, 30),
                createTransfer(2, 5, 5, prepareCount, performCount, 40),
                createTransfer(1, 4, 6, prepareCount, performCount, 50),
                createTransfer(3, 5, 7, prepareCount, performCount, 60),
                createTransfer(4, 5, 8, prepareCount, performCount, 70),
                createTransfer(2, 3, 9, prepareCount, performCount, 80),
                createTransfer(6, 1, 10, prepareCount, performCount, 90)
        );
        var system = StorageSystemFactory.newSystem(
                Map.of(
                        new DeviceId(1), 2,
                        new DeviceId(2), 2,
                        new DeviceId(3), 2,
                        new DeviceId(4), 1,
                        new DeviceId(5), 1,
                        new DeviceId(6), 1
                ),
                Map.of(
                        new ComponentId(1), new DeviceId(5),
                        new ComponentId(2), new DeviceId(3),
                        new ComponentId(3), new DeviceId(1),
                        //new ComponentId(4), new DeviceId(1),
                        new ComponentId(5), new DeviceId(2),
                        new ComponentId(6), new DeviceId(1),
                        new ComponentId(7), new DeviceId(3),
                        new ComponentId(8), new DeviceId(4),
                        new ComponentId(9), new DeviceId(2),
                        new ComponentId(10), new DeviceId(6)
                )
        );
        execute(system, transfers, 10, 10, transfers.size() - 4); // all but 4 should fail
        assertEquals(4, prepareCount.get(), "Prepare");
        assertEquals(4, performCount.get(), "Perform");
        // transfers 10, 1, 7, 3
        assertComponentOnDevice(system, new ComponentId(1), new DeviceId(6));
        assertComponentOnDevice(system, new ComponentId(7), new DeviceId(5));
        assertComponentOnDevice(system, new ComponentId(3), new DeviceId(3));
        assertComponentOnDevice(system, new ComponentId(10), new DeviceId(1));
    }
}
