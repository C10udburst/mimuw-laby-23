package cp2023.solution;

import java.util.concurrent.Semaphore;

public class WorkingTransfer {
    public Semaphore childSemaphore;

    public WorkingTransfer() {

    }

    public void waitForPerform() {
        try {
            childSemaphore.acquire();
        } catch (InterruptedException e) {
            throw new RuntimeException("panic: unexpected thread interruption");
        }
    }
}
