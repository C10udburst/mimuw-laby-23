//package lab02.assignments;

import java.util.Arrays;
import java.util.Random;

public class Vector {

    private static final int SUM_CHUNK_LENGTH = 10;
    private static final int DOT_CHUNK_LENGTH = 10;

    private final int[] elements;

    public Vector(int length) {
        this.elements = new int[length];
    }

    public Vector(int[] elements) {
        this.elements = Arrays.copyOf(elements, elements.length);
    }

    public Vector sum(Vector other) throws InterruptedException {
        if (this.elements.length != other.elements.length) {
            throw new IllegalArgumentException("different lengths of summed vectors");
        }
        Vector result = new Vector(this.elements.length);
        
        int numChunks = (this.elements.length + SUM_CHUNK_LENGTH - 1) / SUM_CHUNK_LENGTH; // oblicz liczbę kawałków, dzieląc z zaokrągleniem w górę
        Thread[] threads = new Thread[numChunks];
        for (int i = 0; i < numChunks; ++i) {
            int startPosIncl = i * SUM_CHUNK_LENGTH;
            int len = Math.min(SUM_CHUNK_LENGTH, this.elements.length - startPosIncl);
            threads[i] = new Thread(new Summer(this, other, startPosIncl, len, result));
            threads[i].start();
        }
        
        for (int i = 0; i < numChunks; ++i) {
            threads[i].join(); // interrupcja wątku i-tego powoduje przerwanie całej operacji
        }

        return result;
    }

    private static class Summer implements Runnable {

        private final Vector leftVec;
        private final Vector rightVec;
        private final int startPosIncl;
        private final int len;
        private final Vector resVec;

        public Summer(Vector leftVec, Vector rightVec, int startPosIncl, int len, Vector resVec) {
            this.resVec = resVec;
            this.leftVec = leftVec;
            this.rightVec = rightVec;
            this.startPosIncl = startPosIncl;
            this.len = len;
        }

        @Override
        public void run() {
            for (int i = this.startPosIncl; i < this.startPosIncl + this.len; ++i) {
                this.resVec.elements[i] = this.leftVec.elements[i] + this.rightVec.elements[i];
            }
        }

    }

    public int dot(Vector other) throws InterruptedException {
        if (this.elements.length != other.elements.length) {
            throw new IllegalArgumentException("different lengths of dotted vectors");
        }
        int result = 0;
        
        int numChunks = (this.elements.length + DOT_CHUNK_LENGTH - 1) / DOT_CHUNK_LENGTH; // oblicz liczbę kawałków, dzieląc z zaokrągleniem w górę
        Thread[] threads = new Thread[numChunks];
        int[] resVec = new int[numChunks];
        for (int i = 0; i < numChunks; ++i) {
            int startPosIncl = i * DOT_CHUNK_LENGTH;
            int len = Math.min(DOT_CHUNK_LENGTH, this.elements.length - startPosIncl);
            threads[i] = new Thread(new Dotter(this, other, startPosIncl, len, resVec, i));
            threads[i].start();
        }

        for (int i = 0; i < numChunks; ++i) {
            threads[i].join(); // interrupcja wątku i-tego powoduje przerwanie całej operacji
            result += resVec[i];
        }

        return result;
    }

    private static class Dotter implements Runnable {

        private final Vector leftVec;
        private final Vector rightVec;
        private final int startPosInc;
        private final int len;
        private final int[] resVec;
        private final int resPos;

        public Dotter(Vector leftVec, Vector rightVect, int startPosInc, int len, int[] resVec, int resPos) {
            this.leftVec = leftVec;
            this.rightVec = rightVect;
            this.startPosInc = startPosInc;
            this.len = len;
            this.resVec = resVec;
            this.resPos = resPos;
        }

        @Override
        public void run() {
            int result = 0;
            for (int i = this.startPosInc; i < this.startPosInc + this.len; ++i) {
                result += this.leftVec.elements[i] * this.rightVec.elements[i];
            }
            this.resVec[this.resPos] = result;
        }

    }

    @Override
    public boolean equals(Object obj) {
        if (! (obj instanceof Vector)) {
            return false;
        }
        Vector other = (Vector)obj;
        return Arrays.equals(this.elements, other.elements);
    }

    @Override
    public int hashCode() {
        return Arrays.hashCode(this.elements);
    }

    @Override
    public String toString() {
        StringBuilder s = new StringBuilder("[");
        for (int i = 0; i < this.elements.length; ++i) {
            if (i > 0) {
                s.append(", ");
            }
            s.append(this.elements[i]);
        }
        s.append("]");
        return s.toString();
    }

    // ----------------------- TESTS -----------------------
    
    private static final Random RANDOM = new Random();

    private static Vector generateRandomVector(int length) {
        int[] a = new int[length];
        for (int i = 0; i < length; ++i) {
            a[i] = RANDOM.nextInt(10);
        }
        return new Vector(a);
    }

    private final Vector sumSequential(Vector other) {
        if (this.elements.length != other.elements.length) {
            throw new IllegalArgumentException("different lengths of summed vectors");
        }
        Vector result = new Vector(this.elements.length);
        for (int i = 0; i < result.elements.length; ++i) {
            result.elements[i] = this.elements[i] + other.elements[i];
        }
        return result;
    }
    
    private final int dotSequential(Vector other) {
        if (this.elements.length != other.elements.length) {
            throw new IllegalArgumentException("different lengths of dotted vectors");
        }
        int result = 0;
        for (int i = 0; i < this.elements.length; ++i) {
            result += this.elements[i] * other.elements[i];
        }
        return result;
    }

    public static void main(String[] args) {
        try {
            Vector a = generateRandomVector(33);
            System.out.println(a);
            Vector b = generateRandomVector(33);
            System.out.println(b);
            Vector c = a.sum(b);
            System.out.println(c);
            assert(c.equals(a.sumSequential(b)));
            int d = a.dot(b);
            System.out.println(d);
            assert(d == a.dotSequential(b));
        } catch (InterruptedException e) {
            Thread.currentThread().interrupt();
            System.err.println("computations interrupted");
        }
    }

}
