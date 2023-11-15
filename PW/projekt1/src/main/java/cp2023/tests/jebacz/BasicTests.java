package cp2023.tests.jebacz;

import cp2023.base.ComponentTransfer;
import cp2023.base.StorageSystem;
import cp2023.exceptions.TransferException;
import org.junit.jupiter.api.Test;

import static org.junit.jupiter.api.Assertions.assertEquals;

public class BasicTests extends Generators {

    @Test
    void RemoveFromFullDevice() throws TransferException {
        StorageSystem system = basicSystem3(2);
        UniqueCount count1 = new UniqueCount();
        ComponentTransfer transfer1 = transfer4(101, 1, -1, count1);

        system.execute(transfer1);

        assert (count1.result() == 2);
    }

    @Test
        // ... https://moodle.mimuw.edu.pl/mod/forum/discuss.php?d=9224
        // Chciałbym serdecznie podziękować osobie, która zadała to ****** pytanie.
    void noOvertakes() throws InterruptedException {
        StorageSystem system = basicSystem2();
        UniqueCount callback = new UniqueCount();
        ComponentTransfer transfer = transfer3(101, 1, -1);
        ComponentTransfer transfer2 = transfer(103, 1, -1);
        ComponentTransfer transfer3 = transfer4(102, 2, 1, callback);

        Thread t1 = new Thread(() -> execTransfer(system, transfer));
        Thread t2 = new Thread(() -> execTransfer(system, transfer2));
        Thread t3 = new Thread(() -> execTransfer(system, transfer3));

        t1.start();

        Thread.sleep(100);

        // t3 powinien zdążyć zarezerwować miejsce po t1, zanim pojawi się t2.
        t3.start();

        Thread.sleep(100);

        t2.start();

        // Ta ilość czasu powinna wystarczyć do zakończenia się tego, co ma się zakończyć
        Thread.sleep(100);

        assertEquals (1, callback.result());

        // Możemy sobie przerwać wątki i o nich zapomnieć, test już sprawdził, co miał sprawdzić,
        // O ile nie jest to w pełni zgodnie z treścią, wszystkie wątki powinny się wtedy zakończyć.
        t1.interrupt();
        t2.interrupt();
        t3.interrupt();
    }

    @Test
        // https://discord.com/channels/999695571575119902/1164480906371813426/1173215886493106296
        // Jak ktoś się o to zapytał to można od razu przetestować
    void Path3() throws InterruptedException {
        StorageSystem system = basicSystem3(2);
        UniqueCount count1 = new UniqueCount();
        UniqueCount count2 = new UniqueCount();
        ComponentTransfer transfer1 = transfer4(101, 1, 2, count1);
        ComponentTransfer transfer2 = transfer4(301, -1, 1, count2);
        ComponentTransfer transfer3 = transfer2(102, 2, -1);


        Thread t1 = new Thread(() -> execTransfer(system, transfer1));
        Thread t2 = new Thread(() -> execTransfer(system, transfer2));
        Thread t3 = new Thread(() -> { execTransfer(system, transfer3); });

        t1.start();
        Thread.sleep(100);

        t2.start();
        Thread.sleep(100);

        t3.start();
        Thread.sleep(100);


        assertEquals(1, count1.result());
        assertEquals(2, count2.result());

        // Możemy sobie przerwać wątki i o nich zapomnieć, test już sprawdził, co miał sprawdzić,
        // O ile nie jest to w pełni zgodnie z treścią, wszystkie wątki powinny się wtedy zakończyć.
        t1.interrupt();
        t2.interrupt();
        t3.interrupt();
    }
}
