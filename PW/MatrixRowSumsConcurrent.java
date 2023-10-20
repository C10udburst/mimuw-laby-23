//package lab03.assignments;

import java.util.concurrent.BrokenBarrierException;
import java.util.concurrent.CyclicBarrier;
import java.util.function.IntBinaryOperator;


public class MatrixRowSumsConcurrent {

    private static final int NUM_ROWS = 10;
    private static final int NUM_COLUMNS = 100;

    private record Matrix(int numRows, int numColumns, IntBinaryOperator definition) {

        public int[] rowSums() throws InterruptedException {
                int[] rowSums = new int[numRows];
                int[] row = new int[numColumns];
                RowSummer rowSummer = new RowSummer(rowSums, row);
                CyclicBarrier barrier = new CyclicBarrier(numColumns, rowSummer);

                Thread[] threads = new Thread[numColumns];
                Thread mainThread = Thread.currentThread();
                for (int i = 0; i < numColumns; i++) {
                    threads[i] = new Thread(new PerColumnDefinitionApplier(i, barrier, row, mainThread));
                    threads[i].start();
                }

                for(Thread t: threads) {
                    t.join();
                }

                return rowSums;
            }

            private class PerColumnDefinitionApplier implements Runnable {

                private final int myColumnNo;
                private final CyclicBarrier barrier;
                private final int[] row;
                private final Thread mainThread;


                private PerColumnDefinitionApplier(
                        int myColumnNo,
                        CyclicBarrier barrier,
                        int[] row,
                        Thread mainThread
                ) {
                    this.myColumnNo = myColumnNo;
                    this.barrier = barrier;
                    this.row = row;
                    this.mainThread = mainThread;
                }

                @Override
                public void run() {
                    for (int rowNo = 0; rowNo < numRows; rowNo++) {
                        row[myColumnNo] = definition.applyAsInt(rowNo, myColumnNo);

                        try {
                            barrier.await();
                        } catch (InterruptedException | BrokenBarrierException e) {
                            if (!mainThread.isInterrupted())
                                mainThread.interrupt();
                            Thread.currentThread().interrupt();
                        }
                    }
                }

            }


            private static class RowSummer implements Runnable {

                private final int[] rowSums;
                private final int[] row;
                private int currentRowNo;

                private RowSummer(int[] rowSums, int[] row) {
                    this.rowSums = rowSums;
                    this.row = row;
                    this.currentRowNo = 0;
                }

                @Override
                public void run() {
                    //System.out.println("RowSummer: " + currentRowNo);
                    for (int j : row) {
                        rowSums[currentRowNo] += j;
                    }
                    currentRowNo++;
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
