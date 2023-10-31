//package lab01.assignments;

public class Primes {

    private static final int MAX_NUMBER_TO_CHECK = 10000;

    private static final int[] MAIN_THREAD_PRIMES = new int[]{2, 3, 5, 7, 11, 13, 17, 19, 23, 29};
    private static final int[] THREAD_STARTS = new int[]{31, 37, 41, 43, 47, 49, 53, 59};

    private static volatile boolean isPrime = true;

    private static class FindDivisor implements Runnable {

        private final int n;
        private final int start;
        private final int step;
        private final int end;

        public FindDivisor(int n, int start, int step, int end) {
            this.n = n;
            this.start = start;
            this.step = step;
            this.end = end;
        }

        @Override
        public void run() {
            for (int i = start; i < end; i += step) {
                if (i*i > n) break;
                if (!isPrime) return;
                if (n % i == 0) isPrime = false;
            }
        }

    }

    public static boolean isPrime(int n) {
        isPrime = true;

        for (int i = 0; i < MAIN_THREAD_PRIMES.length; i++) {
            if (i*i > n) break;
            if (n % MAIN_THREAD_PRIMES[i] == 0) {
                return false;
            }
        }
        
        Thread[] threads = new Thread[THREAD_STARTS.length];
        for (int i = 0; i < THREAD_STARTS.length; i++) {
            threads[i] = new Thread(new FindDivisor(n, THREAD_STARTS[i], 30, n));
            threads[i].start();
        }

        for (int i = 0; i < THREAD_STARTS.length; i++) {
            while (threads[i].isAlive()) { } // join byłby lepszy?
        }

        return isPrime;
    }

    public static void main(String[] args) {
        int primesCount = 0;
        // A sample test.
        for (int i = 2; i <= MAX_NUMBER_TO_CHECK; i++) {
            if (isPrime(i)) {
                ++primesCount;
            }
        }
        System.out.println(primesCount);
        assert(primesCount == 1229);
    }

}
