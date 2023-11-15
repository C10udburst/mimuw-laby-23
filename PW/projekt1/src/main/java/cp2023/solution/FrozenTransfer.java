package cp2023.solution;

import cp2023.base.ComponentTransfer;

import java.util.concurrent.CyclicBarrier;
import java.util.concurrent.Semaphore;

public class FrozenTransfer {
    public final ComponentTransfer transfer;
    public final Semaphore semaphore;
    public CyclicBarrier barrier = null;
    public Semaphore chainWait = null;
    public Semaphore chainWake = null;

    public boolean updateSrc = false;

    public FrozenTransfer(ComponentTransfer transfer, Semaphore semaphore) {
        this.transfer = transfer;
        this.semaphore = semaphore;
    }

    public void semAcquire()  {
        try {
            semaphore.acquire();
        } catch (InterruptedException e) {
            throw new RuntimeException("panic: unexpected thread interruption");
        }
    }
}
