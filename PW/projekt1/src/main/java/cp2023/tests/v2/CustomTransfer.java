package cp2023.tests.v2;

import cp2023.base.ComponentId;
import cp2023.base.ComponentTransfer;
import cp2023.base.DeviceId;

import java.util.concurrent.Semaphore;
import java.util.concurrent.atomic.AtomicInteger;

public class CustomTransfer implements ComponentTransfer {

    public final int componentId;
    public final Integer sourceDeviceId;
    public final Integer destinationDeviceId;
    public final AtomicInteger prepareCount;
    public final AtomicInteger performCount;
    public final Semaphore done;
    public final Semaphore sleepBefore;
    public final Semaphore sleepPrepare;
    public final Semaphore sleepPerform;

    public CustomTransfer(int componentId, Integer sourceDeviceId, Integer destinationDeviceId, AtomicInteger prepareCount, AtomicInteger performCount, Semaphore done, Semaphore sleepBefore, Semaphore sleepPrepare, Semaphore sleepPerform) {
        this.componentId = componentId;
        this.sourceDeviceId = sourceDeviceId;
        this.destinationDeviceId = destinationDeviceId;
        this.prepareCount = prepareCount;
        this.performCount = performCount;
        this.done = done;
        this.sleepBefore = sleepBefore;
        this.sleepPrepare = sleepPrepare;
        this.sleepPerform = sleepPerform;
    }

    public CustomTransfer(int componentId, Integer sourceDeviceId, Integer destinationDeviceId) {
        this(componentId, sourceDeviceId, destinationDeviceId, null, null, null, null, null, null);
    }

    public CustomTransfer(int componentId, Integer sourceDeviceId, Integer destinationDeviceId, Semaphore sleepBefore) {
        this(componentId, sourceDeviceId, destinationDeviceId, null, null, null, sleepBefore, null, null);
    }

    public CustomTransfer(int componentId, Integer sourceDeviceId, Integer destinationDeviceId, Semaphore sleepBefore, Semaphore done) {
        this(componentId, sourceDeviceId, destinationDeviceId, null, null, done, sleepBefore, null, null);
    }


    @Override
    public ComponentId getComponentId() {
        return new ComponentId(componentId);
    }

    @Override
    public DeviceId getSourceDeviceId() {
        return (sourceDeviceId == null) ? null : new DeviceId(sourceDeviceId);
    }

    @Override
    public DeviceId getDestinationDeviceId() {
        return (destinationDeviceId == null) ? null : new DeviceId(destinationDeviceId);
    }

    @Override
    public void prepare() {
        if (sleepPrepare != null) {
            sleepPrepare.acquireUninterruptibly();
        }
        if (prepareCount != null) {
            prepareCount.incrementAndGet();
        }
        System.out.println("prepare(" + componentId + "): " + sourceDeviceId + "->" + destinationDeviceId);
        if (done != null) {
            done.release();
        }
    }

    @Override
    public void perform() {
        if (sleepPerform != null) {
            sleepPerform.acquireUninterruptibly();
        }
        if (performCount != null) {
            performCount.incrementAndGet();
        }
        System.out.println("perform(" + componentId + "): " + sourceDeviceId + "->" + destinationDeviceId);
        if (done != null) {
            done.release();
        }
    }

    public void sleepInit() {
        if (sleepBefore != null) {
            sleepBefore.acquireUninterruptibly();
        }
    }

    @Override
    public String toString() {
        return "CT(" + componentId + "): " + sourceDeviceId + "->" + destinationDeviceId;
    }
}