package cp2023.tests.jebacz;

import cp2023.base.ComponentId;

public record CallbackLogger(CallbackInterface realCb) implements CallbackInterface {

    @Override
    public void call(ComponentId transfer, boolean isSecondPhase) {
        if (isSecondPhase)
            System.out.println("Performing transfer " + transfer);
        else
            System.out.println("Preparing transfer " + transfer);
        this.realCb.call(transfer, isSecondPhase);
    }
}
