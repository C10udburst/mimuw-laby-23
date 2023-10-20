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

                PerColumnDefinitionApplier[] perColumnDefinitionAppliers = new PerColumnDefinitionApplier[numColumns];
                for (int columnNo = 0; columnNo < numColumns; columnNo++) {
                    perColumnDefinitionAppliers[columnNo] = new PerColumnDefinitionApplier(columnNo, barrier, row, Thread.currentThread());
                }

                for (int rowNo = 0; rowNo < numRows; rowNo++) {
                    Thread[] threads = new Thread[numColumns];
                    for (int columnNo = 0; columnNo < numColumns; columnNo++) {
                        threads[columnNo] = new Thread(perColumnDefinitionAppliers[columnNo]);
                        threads[columnNo].start();
                    }
                    for (Thread thread : threads) {
                        thread.join();
                    }
                }

                return rowSums;
            }

            private class PerColumnDefinitionApplier implements Runnable {

                private int myColumnNo;
                private final CyclicBarrier barrier;
                private final int[] row;
                private final Thread mainThread;
                private int myRowNo;


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
//                    System.out.println("PerColumnDefinitionApplier: " + myRowNo + ", " + myColumnNo);
                    row[myColumnNo] = definition.applyAsInt(myRowNo, myColumnNo);

                    try {
                        barrier.await();
                    } catch (InterruptedException | BrokenBarrierException e) {
                        mainThread.interrupt();
                    }

                    myRowNo++;
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
//                    System.out.println("RowSummer: " + currentRowNo);
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

//            if (column == 23) {
//                Thread.currentThread().interrupt();
//            }

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
