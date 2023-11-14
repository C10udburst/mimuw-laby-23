package cp2023.tests.jebacz;

import cp2023.base.ComponentId;
import cp2023.base.DeviceId;

public class HaltPrepare extends SimpleTransfer {
    public HaltPrepare(ComponentId componentId, DeviceId source, DeviceId target) {
        super(componentId, source, target);
    }

    @Override
    public void prepare() {
        while (true){
        }
    }
}
