package cp2023.solution;

import cp2023.base.ComponentId;
import cp2023.base.ComponentTransfer;
import cp2023.base.DeviceId;

public record ComponentCreate(
    ComponentId componentId,
    DeviceId deviceId
) implements ComponentTransfer {

    @Override
    public ComponentId getComponentId() {
        return componentId;
    }

    @Override
    public DeviceId getSourceDeviceId() {
        return null;
    }

    @Override
    public DeviceId getDestinationDeviceId() {
        return deviceId;
    }

    @Override
    public void prepare() { }

    @Override
    public void perform() { }
    
}
