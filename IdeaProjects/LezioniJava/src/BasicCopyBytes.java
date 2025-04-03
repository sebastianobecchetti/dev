import java.io.FileInputStream;
import java.io.FileNotFoundException;
import java.io.FileOutputStream;
import java.io.IOException;

public class BasicCopyBytes {

    private static int BUFFER_SIZE = 1000;

    public static void main(String[] args) throws IOException, FileNotFoundException {
        String inputFile = "src/input.txt";
        String outputFile = "src/output.txt";
        FileInputStream in = new FileInputStream(inputFile);
        FileOutputStream out = new FileOutputStream(outputFile);

        byte[] buffer = new byte[BUFFER_SIZE];
        int bytesNumber;
        while((bytesNumber = in.read(buffer))>=0)
            out.write(buffer, 0, bytesNumber);
        in.close();
        out.close();
    }
}
