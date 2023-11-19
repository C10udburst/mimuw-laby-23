package cp2023.tests.bartix.better;

import org.junit.jupiter.api.Test;

import static org.junit.jupiter.api.Assertions.assertDoesNotThrow;

public class BetterTest {
    @Test
    public void test() throws InterruptedException {
        var t = new Thread(() -> {
            assertDoesNotThrow(() -> Testy2.main(new String[]{}));
        });
        t.start();
        Thread.sleep(10_000);
        assert(!t.isAlive());
        t.interrupt();
    }
}
