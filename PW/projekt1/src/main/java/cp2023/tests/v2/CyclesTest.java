package cp2023.tests.v2;

import jdk.jshell.execution.Util;
import org.junit.jupiter.api.Test;

import java.util.ArrayList;
import java.util.List;
import java.util.concurrent.Semaphore;

public class CyclesTest {
    @Test
    public void shrimple() {
        var sem = new Semaphore(0);
        var transfers = List.of(
            new CustomTransfer(1, 1, 2),
            new CustomTransfer(2, 2, 3),
            new CustomTransfer(3, 3, 1, sem)
        );
        var system = Utils.filledSystem(transfers);
        Utils.unlockSeconds(sem, 2);
        Utils.execute(system, transfers, 3, 3);
        Utils.assertComponentOnDevice(system, 1, 2);
        Utils.assertComponentOnDevice(system, 2, 3);
    }

    @Test
    public void moodle1() {
        var cycleDone = new Semaphore(0);
        var transfers = new ArrayList<>(List.of(
                new CustomTransfer(1, 1, 2),
                new CustomTransfer(2, 4, 2),
                new CustomTransfer(3, 3, 4),
                new CustomTransfer(4, 3, 1),
                new CustomTransfer(5, 2, 3, null, cycleDone)
        ));
        var system = Utils.filledSystem(transfers);
        transfers.add(new CustomTransfer(1, 2, null, cycleDone)); // free up a slot after the cycle is done)
        Utils.execute(system, transfers, 100, 3);
        Utils.assertComponentOnDevice(system, 5, 3);
        Utils.assertComponentOnDevice(system, 4, 1);
        Utils.assertComponentOnDevice(system, 3, 4);
        Utils.assertComponentOnDevice(system, 2, 2);
    }
}
