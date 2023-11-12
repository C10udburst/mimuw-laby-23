package cp2023.solution;

import cp2023.base.ComponentTransfer;

import java.util.concurrent.BrokenBarrierException;
import java.util.concurrent.CyclicBarrier;
import java.util.concurrent.Semaphore;

public class FrozenTransfer {
    public final ComponentTransfer transfer;
    public final Semaphore semaphore;
    public CyclicBarrier barrier;

    public FrozenTransfer(ComponentTransfer transfer, Semaphore semaphore) {
        this.transfer = transfer;
        this.semaphore = semaphore;
        this.barrier = null;
    }

    public void semAcquire()  {
        try {
            semaphore.acquire();
        } catch (InterruptedException e) {
            throw new RuntimeException("panic: unexpected thread interruption");
        }
    }

    public void barrierWait() {
        try {
            barrier.await();
        } catch (InterruptedException | BrokenBarrierException e) {
            throw new RuntimeException("panic: unexpected thread interruption");
        }
    }
}
