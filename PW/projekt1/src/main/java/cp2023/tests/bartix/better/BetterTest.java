package cp2023.tests.bartix.better;

import org.junit.jupiter.api.RepeatedTest;
import org.junit.jupiter.api.RepetitionInfo;
import org.junit.jupiter.api.Test;
import org.junit.jupiter.api.parallel.Execution;
import org.junit.jupiter.api.parallel.ExecutionMode;

import static org.junit.jupiter.api.Assertions.assertDoesNotThrow;

public class BetterTest {
    @RepeatedTest(value = 15, name = "TestBetter{currentRepetition}")
    @Execution(ExecutionMode.CONCURRENT)
    public void test(RepetitionInfo ri) throws InterruptedException {
        var t = new Thread(() -> {
            assertDoesNotThrow(() -> Testy2.main(ri.getCurrentRepetition()));
        });
        t.start();
        for (int i = 0; i < 5; i++) {
            if (t.isAlive()) Thread.sleep(1000);
            else break;
        }
        assert(!t.isAlive());
        t.interrupt();
    }
}
