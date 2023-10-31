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

    public void acquireFutureSlot() {
        mutex.acquireUninterruptibly();
        assert freeSlotsFuture >= 0;
        if (freeSlotsFuture == 0) {
            mutex.release();
            reservationSemaphore.acquireUninterruptibly();
            //mutex inherited
        }
        // freeSlotsFuture > 0
        assert freeSlotsFuture > 0;
        freeSlotsFuture--;
        mutex.release();
    }

    public void acquireSlot() {
        mutex.acquireUninterruptibly();
        assert freeSlots >= 0;
        if (freeSlots == 0) {
            mutex.release();
            acquireSemaphore.acquireUninterruptibly();
            //mutex inherited
        }
        // freeSlots > 0
        assert freeSlots > 0;
        freeSlots--;
        mutex.release();
    }

    public void releaseFutureSlot() {
        mutex.acquireUninterruptibly(); // TODO: maybe not uninterruptibly?
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
        mutex.acquireUninterruptibly();  // TODO: maybe not uninterruptibly?
        assert freeSlots >= 0;
        freeSlots++;
        if(acquireSemaphore.hasQueuedThreads()) {
            acquireSemaphore.release();
            //mutex inherited
        } else {
            mutex.release();
        }
    }

    public void releaseBothSlots() {
        mutex.acquireUninterruptibly(); // TODO: maybe not uninterruptibly?
        assert freeSlots >= 0;
        assert freeSlotsFuture >= 0;
        freeSlots++;
        freeSlotsFuture++;
        if (acquireSemaphore.hasQueuedThreads()) {
            acquireSemaphore.release();
            //mutex inherited
        } else if(reservationSemaphore.hasQueuedThreads()) {
            reservationSemaphore.release();
            //mutex inherited
        } else {
            mutex.release();
        }
    }
}
