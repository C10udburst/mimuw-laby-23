package cp2023.solution;

import cp2023.base.ComponentTransfer;

import java.util.concurrent.CyclicBarrier;
import java.util.concurrent.Semaphore;

public class FrozenTransfer {
    public final ComponentTransfer transfer;

    // semaphore to signal that we were woken up from the waiting process'es queue
    public final Semaphore queueLeft = new Semaphore(0);

    public Semaphore parentSemaphore = null;
    public Semaphore childSemaphore = null;

    public FrozenTransfer(ComponentTransfer transfer) {
        this.transfer = transfer;
    }

    public void waitInQueue()  {
        try {
            queueLeft.acquire();
        } catch (InterruptedException e) {
            throw new RuntimeException("panic: unexpected thread interruption");
        }
    }

    public void waitForParent() {
        try {
            parentSemaphore.acquire();
        } catch (InterruptedException e) {
            throw new RuntimeException("panic: unexpected thread interruption");
        }
    }
}
