package cp2023.solution;

import java.util.concurrent.locks.Condition;
import java.util.concurrent.locks.ReentrantLock;

public class DeviceLock {
    public final ReentrantLock lock = new ReentrantLock(true);
    public final Condition freeFuture = lock.newCondition();
    public final Condition freeNow = lock.newCondition();
}