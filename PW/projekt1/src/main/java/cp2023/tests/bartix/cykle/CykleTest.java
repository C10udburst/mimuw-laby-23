package cp2023.tests.bartix.cykle;

import cp2023.tests.bartix.better.Testy2;
import org.junit.jupiter.api.RepeatedTest;
import org.junit.jupiter.api.RepetitionInfo;
import org.junit.jupiter.api.parallel.Execution;
import org.junit.jupiter.api.parallel.ExecutionMode;

import static org.junit.jupiter.api.Assertions.assertDoesNotThrow;

public class CykleTest {

    @Execution(ExecutionMode.CONCURRENT)
    @RepeatedTest(value = 15, name = "Test{currentRepetition}")
    public void test(RepetitionInfo ri) throws InterruptedException {
        var t = new Thread(() -> {
            assertDoesNotThrow(() -> TestyWspobieznie.main(ri.getCurrentRepetition()));
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
