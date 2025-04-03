public class IntegerMatrix {

    private int[][] matrix;

    public IntegerMatrix(int rows, int columns) {
        this.matrix = new int[rows][columns];
    }

    public void setValue(int i, int j, int value) {
        this.matrix[i][j] = value;
    }

    public int getValue(int i, int j) {
        return this.matrix[i][j];
    }

    public int getMaxOfRow(int i) {
        int max = this.matrix[i][0];
        for (int j = 1; j < this.matrix[i].length; j++)
            if (this.matrix[i][j] > max)
                max = this.matrix[i][j];
        return max;
    }

    public int getMaxOfColumn(int j) {
        int max = this.matrix[0][j];
        for (int i = 1; i < this.matrix.length; i++)
            if (this.matrix[i][j] > max)
                max = this.matrix[i][j];
        return max;
    }

    public int getMax() {
        int max = this.matrix[0][0];
        for (int i = 0; i < this.matrix.length; i++)
            for (int j = 0; j < this.matrix[i].length; j++)
                if (this.matrix[i][j] > max)
                    max = this.matrix[i][j];
        return max;
    }

} // end class