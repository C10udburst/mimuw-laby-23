//package lab05.assignments;

import java.util.ArrayList;
import java.util.concurrent.*;
import java.util.function.IntBinaryOperator;


public class MatrixRowSumsThreadpool {

    private static final int NUM_ROWS = 10;
    private static final int NUM_COLUMNS = 100;
    private static final int NUM_THREADS = 4;

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
            ExecutorService pool = Executors.newFixedThreadPool(NUM_THREADS);
            try {
                // FIXME: Create Job futures and submit them to the pool.
                // FIXME: Do summing of the returned values.
                // make each task calculate only one cell
                
                ArrayList<Future<Integer>> futures = new ArrayList<>();
                for (int i = 0; i < numRows; i++) {
                    for (int j = 0; j < numColumns; j++) {
                        futures.add(pool.submit(new Job(i, j, definition)));
                    }
                }

                for (int i = 0; i < futures.size(); i++) {
                    rowSums[i / numColumns] += futures.get(i).get();
                }
                
                return rowSums;
            }
            catch (ExecutionException e) {
                throw new InterruptedException(e.toString());
            }
            finally {
                pool.shutdown();
            }
            
        }
        
        private class Job implements Callable<Integer> {

            private final int rowNo;
            private final int columnNo;
            private final IntBinaryOperator definition;

            private Job(int rowNo, int columnNo, IntBinaryOperator definition) {
                this.rowNo = rowNo;
                this.columnNo = columnNo;
                this.definition = definition;
            }

            @Override
            public Integer call() {
                return definition.applyAsInt(rowNo, columnNo);
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
