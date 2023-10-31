package cp2023.solution;

import java.util.concurrent.Semaphore;

public class Device {

    private int freeSlotsFuture;
    private int freeSlots;

    // critical section
    private final Semaphore mutex = new Semaphore(1);

    private final Semaphore reservationSemaphore = new Semaphore(0, true);
    private final Semaphore acquireSemaphore = new Semaphore(0, true);


    public Device(int freeSlots) {
        this.freeSlots = freeSlots;
        this.freeSlotsFuture = freeSlots;
    }

    public void acquireFutureSlot() throws InterruptedException {
        mutex.acquire();
        assert freeSlotsFuture >= 0;
        if (freeSlotsFuture == 0) {
            mutex.release();
            reservationSemaphore.acquire();
            //mutex inherited
        }
        // freeSlotsFuture > 0
        assert freeSlotsFuture > 0;
        freeSlotsFuture--;
        mutex.release();
    }

    public void acquireSlot() throws InterruptedException {
        mutex.acquire();
        assert freeSlots >= 0;
        if (freeSlots == 0) {
            mutex.release();
            acquireSemaphore.acquire();
            //mutex inherited
        }
        // freeSlots > 0
        assert freeSlots > 0;
        freeSlots--;
        mutex.release();
    }

    public void releaseFutureSlot() {
        mutex.acquireUninterruptibly();
        assert freeSlotsFuture >= 0;
        freeSlotsFuture++;
        if(reservationSemaphore.hasQueuedThreads()) {
            reservationSemaphore.release();
            //mutex inherited
        } else {
            mutex.release();
        }
    }

    public void releaseSlot() {
        mutex.acquireUninterruptibly();
        assert freeSlots >= 0;
        freeSlots++;
        if(acquireSemaphore.hasQueuedThreads()) {
            acquireSemaphore.release();
            //mutex inherited
        } else {
            mutex.release();
        }
    }
}
