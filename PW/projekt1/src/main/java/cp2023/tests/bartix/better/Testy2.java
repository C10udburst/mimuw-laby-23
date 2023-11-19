package cp2023.tests.bartix.better;

import cp2023.base.ComponentId;
import cp2023.base.DeviceId;
import cp2023.base.StorageSystem;
import cp2023.demo.TransferBurst;
import cp2023.exceptions.TransferException;
import cp2023.solution.StorageSystemFactory;

import java.io.File;
import java.io.FileNotFoundException;
import java.io.OutputStream;
import java.io.PrintStream;
import java.lang.reflect.InvocationTargetException;
import java.lang.reflect.Method;
import java.util.Collection;
import java.util.HashMap;

public class Testy2 {
    public static void main(String[] args) {
        for(int i = 1; i <= 15; i++){
            StorageSystem system = setupSystem();
            Collection<Thread> users = setupTransferers(system, i);
            runTransferers(users);
        }
    }

    private final static StorageSystem setupSystem() {
        HashMap<DeviceId, Integer> deviceCapacities = new HashMap<>(26);
        HashMap<ComponentId, DeviceId> initialComponentMapping = new HashMap<>(26);
        for (int i = 0; i < 19; i++){
            deviceCapacities.put(new DeviceId(i + 1), 5);
            for (int j = 0; j < 5; j++){
                initialComponentMapping.put(new ComponentId(5 * i + j), new DeviceId(i + 1));
            }
        }
        return StorageSystemFactory.newSystem(deviceCapacities, initialComponentMapping);
    }

    private final static Collection<Thread> setupTransferers(StorageSystem system, int i) {
        try {
            Class<?> clas = Class.forName("cp2023.tests.bartix.better.TestBetter" + Integer.toString(i));
            Method m = clas.getMethod("setupTransferers", StorageSystem.class);
            return (Collection<Thread>) m.invoke(null,system);
        } catch (ClassNotFoundException | NoSuchMethodException | InvocationTargetException | IllegalAccessException e){
            throw new RuntimeException("kurwa");
        }
    }

    private final static void runTransferers(Collection<Thread> users) {
        for (Thread t : users) {
            t.start();
        }
        for (Thread t : users) {
            try {
                t.join();
            } catch (InterruptedException e) {
                throw new RuntimeException("panic: unexpected thread interruption", e);
            }
        }
    }


    public final static Transfer executeTransfer(
            StorageSystem system,
            int compId,
            int srcDevId,
            int dstDevId,
            long duration
    ) {
        Transfer transfer =
                new Transfer(
                        new ComponentId(compId),
                        srcDevId > 0 ? new DeviceId(srcDevId) : null,
                        dstDevId > 0 ? new DeviceId(dstDevId) : null,
                        duration
                );
        try {
            system.execute(transfer);
        } catch (TransferException e) {
            throw new RuntimeException("Uexpected transfer exception: " + e.toString(), e);
        }
        return transfer;
    }

    private final static void sleep(long duration) {
        try {
            Thread.sleep(duration);
        } catch (InterruptedException e) {
            throw new RuntimeException("panic: unexpected thread interruption", e);
        }
    }

}
