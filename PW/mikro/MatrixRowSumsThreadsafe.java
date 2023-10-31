//package lab04.assignments;

import java.util.ArrayList;
import java.util.List;
import java.util.concurrent.ConcurrentHashMap;
import java.util.concurrent.atomic.AtomicInteger;
import java.util.function.IntBinaryOperator;


public class MatrixRowSumsThreadsafe {

    private static final int NUM_ROWS = 10;
    private static final int NUM_COLUMNS = 100;

    private static class Matrix {

        private final int numRows;
        private final int numColumns;
        private final IntBinaryOperator definition;

        public Matrix(int numRows, int numColumns, IntBinaryOperator definition) {
            this.numRows = numRows;
            this.numColumns = numColumns;
            this.definition = definition;
        }

        public int[] rowSums() throws InterruptedException {
            ConcurrentHashMap<Integer, AtomicInteger> rowSums = new ConcurrentHashMap<>();

            List<Thread> threads = new ArrayList<>();
            for (int columnNo = 0; columnNo < numColumns; ++columnNo) {
                Thread t = new Thread(new PerColumnDefinitionApplier(columnNo, rowSums));
                threads.add(t);
            }
            for (Thread t : threads) {
                t.start();
            }
            try {
                for (Thread t : threads) {
                    t.join();
                }
            } catch (InterruptedException e) {
                for (Thread t : threads) {
                    if (t.isAlive())
                        t.interrupt();
                }
                throw e;
            }
            int[] result = new int[numRows];
            for (int rowNo = 0; rowNo < numRows; ++rowNo) {
                result[rowNo] = rowSums.get(rowNo).get();
            }
            return result;
        }

        private class PerColumnDefinitionApplier implements Runnable {

            private final int myColumnNo;
            private final ConcurrentHashMap<Integer, AtomicInteger> rowSums;

            private PerColumnDefinitionApplier(
                    int myColumnNo,
                    ConcurrentHashMap<Integer, AtomicInteger> rowSums
            ) {
                this.myColumnNo = myColumnNo;
                this.rowSums = rowSums;
            }

            @Override
            public void run() {
                for (int rowNo = 0; rowNo < numRows; ++rowNo) {
                    int value = definition.applyAsInt(rowNo, myColumnNo);
                    rowSums
                        .computeIfAbsent(rowNo, (k) -> new AtomicInteger(0))
                        .addAndGet(value);
                }
            }

        }
    }


    public static void main(String[] args) {
        Matrix matrix = new Matrix(NUM_ROWS, NUM_COLUMNS, (row, column) -> {
            int a = 2 * column + 1;
            return (row + 1) * (a % 4 - 2) * a;
        });
        try {

            int[] rowSums = matrix.rowSums();

            for (int i = 0; i < rowSums.length; i++) {
                System.out.println(i + " -> " + rowSums[i]);
            }

        } catch (InterruptedException e) {
            System.err.println("Obliczenie przerwane");
            return;
        }
    }

}
