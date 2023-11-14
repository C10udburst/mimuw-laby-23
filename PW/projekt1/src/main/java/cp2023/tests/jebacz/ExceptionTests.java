package cp2023.tests.jebacz;

import cp2023.base.ComponentId;
import cp2023.base.ComponentTransfer;
import cp2023.base.DeviceId;
import cp2023.base.StorageSystem;
import cp2023.exceptions.*;
import cp2023.solution.StorageSystemFactory;
import org.junit.jupiter.api.Test;

import java.util.HashMap;

import static org.junit.jupiter.api.Assertions.assertEquals;
import static org.junit.jupiter.api.Assertions.assertThrows;

public class ExceptionTests extends Generators {


    @Test
        // https://moodle.mimuw.edu.pl/mod/forum/discuss.php?d=9191
    void nullSizeDevice() {
        DeviceId newDew = new DeviceId(1);
        HashMap<DeviceId, Integer> devices = new HashMap<>();
        devices.put(newDew, 0);
        assertThrows(IllegalArgumentException.class, () -> StorageSystemFactory.newSystem(devices, null));
    }

    @Test
    void noSpaceOnDevice() {
        DeviceId newDew = new DeviceId(1);
        HashMap<DeviceId, Integer> devices = new HashMap<>();

        devices.put(newDew, 1);
        ComponentId comp1 = new ComponentId(101);
        ComponentId comp2 = new ComponentId(102);

        HashMap<ComponentId, DeviceId> initialComponentMapping = new HashMap<>(2);

        initialComponentMapping.put(comp1, newDew);
        initialComponentMapping.put(comp2, newDew);

        assertThrows(IllegalArgumentException.class, () -> StorageSystemFactory.newSystem(devices, initialComponentMapping));
    }

    // transfer nie reprezentuje żadnej z trzech dostępnych operacji na komponentach lub nie wskazuje żadnego komponentu (wyjątek IllegalTransferType);

    @Test
    void InvalidType() {
        StorageSystem system = basicSystem1();
        ComponentTransfer transfer = transfer(2137, -1, -1);
        Exception e = assertThrows(IllegalTransferType.class, () -> system.execute(transfer));
        assertEquals("both source and destination devices are null for component COMP-2137",e.getMessage());
    }

    // urządzenie wskazane przez transfer jako źródłowe lub docelowe nie istnieje w systemie (wyjątek DeviceDoesNotExist);
    @Test
    void InvalidDest() {
        StorageSystem system = basicSystem1();
        ComponentTransfer transfer = transfer(101, 1, 2137);
        Exception e = assertThrows(DeviceDoesNotExist.class, () -> system.execute(transfer));
        assertEquals("device DEV-2137 does not exist",e.getMessage());
    }

    // komponent o identyfikatorze równym dodawanemu w ramach transferu komponentowi już istnieje w systemie (wyjątek ComponentAlreadyExists);

    @Test
    void ComponentExists() {
        StorageSystem system = basicSystem1();
        ComponentTransfer transfer = transfer(101, -1, 1);
        Exception e = assertThrows(ComponentAlreadyExists.class, () -> system.execute(transfer));
        assertEquals("component COMP-101 already exists on device DEV-1",e.getMessage());
    }

    // komponent o identyfikatorze równym usuwanemu lub przesuwanemu w ramach transferu komponentowi nie istnieje
    // w systemie komponent o identyfikatorze równym usuwanemu, lub przesuwanemu w ramach transferu komponentowi nie istnieje w systemie,

    @Test
    void ComponentDoesNotExist() {
        StorageSystem system = basicSystem1();
        ComponentTransfer transfer = transfer(69, 1, 2);
        Exception e = assertThrows(ComponentDoesNotExist.class, () -> system.execute(transfer));
        assertEquals("component COMP-69 does not exist on device DEV-1",e.getMessage());
    }

    // lub znajduje się na innym urządzeniu niż wskazane przez transfer (wyjątek ComponentDoesNotExist);

    @Test
    void ComponentExistsElsewhere() {
        StorageSystem system = basicSystem1();
        ComponentTransfer transfer = transfer(3, 1, 2);
        Exception e = assertThrows(ComponentDoesNotExist.class, () -> system.execute(transfer));
        assertEquals("component COMP-3 does not exist on device DEV-1",e.getMessage());
    }

    // komponent, którego dotyczy transfer, znajduje się już na urządzeniu wskazanym przez transfer jako docelowe (wyjątek ComponentDoesNotNeedTransfer);

    @Test
    void TransferNotNeeded() {
        StorageSystem system = basicSystem1();
        ComponentTransfer transfer = transfer(101, 1, 1);
        Exception e = assertThrows(ComponentDoesNotNeedTransfer.class, () -> system.execute(transfer));
        assertEquals("component COMP-101 does not need a transfer from device DEV-1 to the same device",e.getMessage());
    }

    // komponent, którego dotyczy transfer, jest jeszcze transferowany (wyjątek ComponentIsBeingOperatedOn).

    @Test
    void ComponentWorkedOn() throws InterruptedException {
        StorageSystem system = basicSystem1();
        ComponentTransfer transfer = transfer2(101, 1, 2);
        ComponentTransfer transfer2 = transfer(101, 1, 2);
        Thread t1 = new Thread(() -> execTransfer(system,transfer));
        t1.start();
        // Powinno to dać wystarczająco dużo czasu, żeby wątek 1 się rozpoczął i "zajął" COMP-101
        Thread.sleep(100);

        Exception e = assertThrows(ComponentIsBeingOperatedOn.class, () -> system.execute(transfer2));
        assertEquals("component COMP-101 is being operated on",e.getMessage());
        t1.interrupt();
    }
}
