//package lab03.assignments;

import java.util.concurrent.CyclicBarrier;
import java.util.concurrent.Semaphore;
import java.util.function.IntBinaryOperator;

public class MatrixRowSumsConcurrent {

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
            int[] rowSums = new int[numRows];
            Semaphore[] rowSemaphores = new Semaphore[numRows];
            for (int i = 0; i < numRows; i++) {
                rowSemaphores[i] = new Semaphore(0);
            }

            CyclicBarrier columnBarrier = new CyclicBarrier(numColumns);
            int[][] matrix = new int[numRows][numColumns];

            // Utwórz i uruchom wątek dla każdej kolumny, który obliczy wartości w kolumnie
            Thread[] columnThreads = new Thread[numColumns];
            for (int i = 0; i < numColumns; i++) {
                columnThreads[i] = new Thread(
                        new PerColumnDefinitionApplier(i, rowSemaphores, matrix, definition, columnBarrier));
                columnThreads[i].start();
            }

            // Utwórz i uruchom wątek dla każdego wiersza, który obliczy sumę wiersza
            Thread[] rowThreads = new Thread[numRows];
            for (int i = 0; i < numRows; i++) {
                rowThreads[i] = new Thread(new RowSummer(i, rowSums, matrix[i], rowSemaphores[i]));
                rowThreads[i].start();
            }

            // Połącz wątki kolumn
            for (Thread rowThread : rowThreads) {
                rowThread.join();
            }

            return rowSums;
        }

        private class PerColumnDefinitionApplier implements Runnable {

            private final int myColumnNo;
            private final Semaphore[] rowSemaphores;
            private final int[][] matrix;
            private final IntBinaryOperator definition;
            private final CyclicBarrier columnBarrier;

            private PerColumnDefinitionApplier(int myColumnNo, Semaphore[] rowSemaphores, int[][] matrix,
                    IntBinaryOperator definition, CyclicBarrier columnBarrier) {
                this.myColumnNo = myColumnNo;
                this.rowSemaphores = rowSemaphores;
                this.matrix = matrix;
                this.definition = definition;
                this.columnBarrier = columnBarrier;
            }

            @Override
            public void run() {
                for (int row = 0; row < matrix.length; row++) {
                    matrix[row][myColumnNo] = definition.applyAsInt(row, myColumnNo);
                    rowSemaphores[row].release();
                }

                try {
                    columnBarrier.await(); // Poczekaj na pozostałe kolumny
                } catch (Exception e) {
                    e.printStackTrace();
                }
            }
        }

        private static class RowSummer implements Runnable {
            private final int rowNo;
            private final int[] rowSums;
            private final int[] row;
            private final Semaphore rowSemaphore;

            public RowSummer(int rowNo, int[] rowSums, int[] row, Semaphore rowSemaphore) {
                this.rowNo = rowNo;
                this.rowSums = rowSums;
                this.row = row;
                this.rowSemaphore = rowSemaphore;
            }

            @Override
            public void run() {
                try {
                    for (int i = 0; i < row.length; i++) {
                        rowSemaphore.acquire();
                        rowSums[rowNo] += row[i];
                    }
                } catch (InterruptedException e) {
                    e.printStackTrace();
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