package cp2023.tests.jebacz;

import cp2023.base.ComponentTransfer;
import cp2023.base.StorageSystem;
import org.junit.jupiter.api.Test;

import java.util.concurrent.atomic.AtomicInteger;

import static org.junit.jupiter.api.Assertions.assertEquals;
import static org.junit.jupiter.api.Assertions.assertThrows;

public class CycleTests extends Generators {

    @Test
    void BasicCycle() throws InterruptedException {
        StorageSystem system = basicSystem2();
        UniqueCount callback = new UniqueCount();
        ComponentTransfer transfer = transfer4(101, 1, 2, callback);
        ComponentTransfer transfer2 = transfer4(102, 2, 1, callback);
        Thread t1 = new Thread(() -> execTransfer(system, transfer));
        Thread t2 = new Thread(() -> execTransfer(system, transfer2));
        t1.start();
        t2.start();

        // Ta ilość czasu powinna wystarczyć do zakończenia się cykli
        Thread.sleep(100);

        assert (!t2.isAlive());
        assert (!t1.isAlive());
        t1.interrupt();
        assert (callback.result() == 4);
        t2.interrupt();

    }

    @Test
    void CorrectCycle() throws InterruptedException {
        StorageSystem system = basicSystem2();
        UniqueCount callback = new UniqueCount();
        ComponentTransfer transfer = transfer2(101, 1, 2);
        ComponentTransfer transfer2 = transfer4(103, 1, 2, callback);
        ComponentTransfer transfer3 = transfer4(102, 2, 1, callback);

        Thread t1 = new Thread(() -> execTransfer(system, transfer));
        Thread t2 = new Thread(() -> execTransfer(system, transfer2));
        Thread t3 = new Thread(() -> execTransfer(system, transfer3));

        t1.start();

        Thread.sleep(100);

        t2.start();

        Thread.sleep(100);

        t3.start();

        // Ta ilość czasu powinna wystarczyć do zakończenia się cykli
        Thread.sleep(100);

        assert (callback.result() == 1);

        // Możemy sobie przerwać wątki i o nich zapomnieć, test już sprawdził, co miał sprawdzić,
        // O ile nie jest to w pełni zgodnie z treścią, wszystkie wątki powinny się wtedy zakończyć.
        t1.interrupt();
        t2.interrupt();
        t3.interrupt();
    }


    // Powinno działać niezależnie, w którą stronę szukamy cyklu (od przodu czy od tyłu)
    @Test
    void LocalPriorityOverGlobal() throws InterruptedException {
        StorageSystem system = basicSystem3(6);
        UniqueCount expected = new UniqueCount();
        UniqueCount notExpected = new UniqueCount();

        // Expected
        ComponentTransfer transferE1 = transfer4(101, 1, 2, expected);
        ComponentTransfer transferE2 = transfer4(104, 4, 3, expected);
        ComponentTransfer transferE3 = transfer4(103, 3, 1, expected);
        ComponentTransfer transferE4 = transfer4(102, 2, 4, expected);

        // Not expected
        ComponentTransfer transferN1 = transfer4(106, 6, 5, notExpected);
        ComponentTransfer transferN2 = transfer4(204, 4, 6, notExpected);
        ComponentTransfer transferN3 = transfer4(105, 5, 2, notExpected);

        Thread t1 = new Thread(() -> execTransfer(system, transferN1));
        Thread t2 = new Thread(() -> execTransfer(system, transferE1));
        Thread t3 = new Thread(() -> execTransfer(system, transferE2));
        Thread t4 = new Thread(() -> execTransfer(system, transferE3));
        Thread t5 = new Thread(() -> execTransfer(system, transferN2));
        Thread t6 = new Thread(() -> execTransfer(system, transferN3));
        Thread t7 = new Thread(() -> execTransfer(system, transferE4));

        t1.start();
        Thread.sleep(100);

        t2.start();
        Thread.sleep(100);

        t3.start();
        Thread.sleep(100);

        t4.start();
        Thread.sleep(100);

        t5.start();
        Thread.sleep(100);

        t6.start();
        Thread.sleep(100);

        t7.start();
        Thread.sleep(100);

        assert (expected.result() == 8);
        assert (notExpected.result() == 0);

        // Możemy sobie przerwać wątki i o nich zapomnieć, test już sprawdził, co miał sprawdzić,
        // O ile nie jest to w pełni zgodnie z treścią, wszystkie wątki powinny się wtedy zakończyć.
        t1.interrupt();
        t2.interrupt();
        t3.interrupt();
        t4.interrupt();
        t5.interrupt();
        t6.interrupt();
        t7.interrupt();

    }

    @Test
    void CycleUsesValidEdge() throws InterruptedException {
        StorageSystem system = basicSystem3(2);
        UniqueCount expected = new UniqueCount();
        CallbackLogger cb = new CallbackLogger(expected);

        ComponentTransfer transfer1 = transfer(101, 1, 2);
        ComponentTransfer transfer2 = transfer(102, 2, -1);
        ComponentTransfer transfer3 = transfer(301, -1, 1);

        ComponentTransfer transfer4 = transfer4(201, 1, 2, cb);
        ComponentTransfer transfer5 = transfer4(202, 2, 1, cb);

        Thread t1 = new Thread(() -> execTransfer(system, transfer1));
        Thread t2 = new Thread(() -> execTransfer(system, transfer2));
        Thread t3 = new Thread(() -> execTransfer(system, transfer3));
        Thread t4 = new Thread(() -> execTransfer(system, transfer4));
        Thread t5 = new Thread(() -> execTransfer(system, transfer5));


        t1.start();
        Thread.sleep(100);

        t2.start();
        t3.start();

        t1.join();
        System.out.println("t1 joined");
        t2.join();
        System.out.println("t2 joined");
        t3.join();
        System.out.println("t3 joined");

        t4.start();
        Thread.sleep(100);

        t5.start();
        Thread.sleep(100);

        assertEquals(4, expected.result());
        t4.interrupt();
        t5.interrupt();
    }

    @Test
    void isCorrectWakenUp() throws InterruptedException {
        StorageSystem system = basicSystem3(2);
        UniqueCount expected = new UniqueCount();
        UniqueCount notExpected = new UniqueCount();


        // Expected
        ComponentTransfer transferE1 = transfer4(101, 1, 2, expected);
        ComponentTransfer transferE2 = transfer4(102, 2, 1, expected);

        // Not expected
        ComponentTransfer transferN1 = transfer4(301, -1, 1, notExpected);
        ComponentTransfer transferN2 = transfer4(302, -1, 2, notExpected);

        Thread t1 = new Thread(() -> execTransfer(system, transferN1));
        Thread t2 = new Thread(() -> execTransfer(system, transferN2));
        Thread t3 = new Thread(() -> execTransfer(system, transferE1));
        Thread t4 = new Thread(() -> execTransfer(system, transferE2));

        t1.start();
        t2.start();

        Thread.sleep(100);

        t3.start();
        t4.start();

        Thread.sleep(100);

        assert (expected.result() == 4);
        assert (notExpected.result() == 0);

        t1.interrupt();
        t2.interrupt();
        t3.interrupt();
        t4.interrupt();


    }
}
