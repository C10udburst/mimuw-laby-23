package cp2023.tests.bartix.cykle;

import org.junit.jupiter.api.Test;

import static org.junit.jupiter.api.Assertions.assertDoesNotThrow;

public class CykleTest {
    @Test
    public void test() throws InterruptedException {
        var t = new Thread(() -> {
            assertDoesNotThrow(() -> TestyWspobieznie.main(new String[]{}));
        });
        t.start();
        Thread.sleep(10_000);
        assert(!t.isAlive());
        t.interrupt();
    }
}
