package cp2023.tests.jebacz;

import cp2023.base.ComponentId;

public interface CallbackInterface {
    void call(ComponentId transfer, boolean isSecondPhase);
}
