package cp2023.tests.bartix.better;

import cp2023.base.ComponentId;
import cp2023.base.ComponentTransfer;
import cp2023.base.DeviceId;

import static java.lang.Thread.sleep;

public final class Transfer implements ComponentTransfer {
    private static int uidGenerator = 0;
    private final int uid;
    private final long owningThread;
    private final Integer phantomSynchronizer;
    private final ComponentId compId;
    private final DeviceId srcDevId;
    private final DeviceId dstDevId;
    private final long duration;
    private boolean prepared;
    private boolean started;
    private boolean done;

    private final static synchronized int generateUID() {
        return ++uidGenerator;
    }

    public Transfer(
            ComponentId compId,
            DeviceId srcDevId,
            DeviceId dstDevId,
            long duration
    ) {
        this.uid = generateUID();
        this.phantomSynchronizer = 19;
        this.owningThread = Thread.currentThread().getId();
        this.compId = compId;
        this.srcDevId = srcDevId;
        this.dstDevId = dstDevId;
        this.duration = duration;
        this.prepared = false;
        this.started = false;
        this.done = false;
        System.out.println("Transferer " + this.owningThread +
                " is about to issue transfer " + this.uid +
                " of " + this.compId + " from " + this.srcDevId +
                " to " + this.dstDevId + ".");
    }

    @Override
    public ComponentId getComponentId() {
        return this.compId;
    }

    @Override
    public DeviceId getSourceDeviceId() {
        return this.srcDevId;
    }

    @Override
    public DeviceId getDestinationDeviceId() {
        return this.dstDevId;
    }

    @Override
    public void prepare() {
        synchronized (this.phantomSynchronizer) {
            if (this.prepared) {
                throw new RuntimeException(
                        "tests.Transfer " + this.uid + " is being prepared more than once!");
            }
            if (this.owningThread != Thread.currentThread().getId()) {
                throw new RuntimeException(
                        "tests.Transfer " + this.uid +
                                " is being prepared by a different thread that scheduled it!");
            }
            this.prepared = true;
        }
        System.out.println("tests.Transfer " + this.uid + " of " + this.compId +
                " from " + this.srcDevId + " to " + this.dstDevId +
                " has been prepared by user " + Thread.currentThread().getId() + ".");
    }

    @Override
    public void perform() {
        synchronized (this.phantomSynchronizer) {
            if (! this.prepared) {
                throw new RuntimeException(
                        "tests.Transfer " + this.uid + " has not been prepared " +
                                "before being performed!");
            }
            if (this.started) {
                throw new RuntimeException(
                        "tests.Transfer " + this.uid + " is being started more than once!");
            }
            if (this.owningThread != Thread.currentThread().getId()) {
                throw new RuntimeException(
                        "tests.Transfer " + this.uid +
                                " is being performed by a different thread that scheduled it!");
            }
            this.started = true;
        }
        System.out.println("tests.Transfer " + this.uid + " of " + this.compId +
                " from " + this.srcDevId + " to " + this.dstDevId + " has been started.");
        try {
            sleep(this.duration);
        } catch (InterruptedException e){
            Thread.currentThread().interrupt();
        }
        synchronized (this.phantomSynchronizer) {
            this.done = true;
        }
        System.out.println("tests.Transfer " + this.uid + " of " + this.compId +
                " from " + this.srcDevId + " to " + this.dstDevId + " has been completed.");
    }

}