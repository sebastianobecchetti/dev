package lezione_stream_char;

import java.io.BufferedWriter;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.OutputStreamWriter;
import java.io.PrintWriter;

import java.util.List;

public class WriteCSV {

    public static void print(String fileName, String charset, List<String[]> lstSA) throws IOException {
        PrintWriter printWriter = new PrintWriter(
                new BufferedWriter(new OutputStreamWriter(new FileOutputStream(fileName), charset)), true);

        for (String[] sArr : lstSA)
            for (int i = 0; i < sArr.length; i++)
                if (i < (sArr.length - 1))
                    printWriter.print(sArr[i] + ";");
                else
                    printWriter.print(sArr[i] + "\n");

        printWriter.close();
    }
} // end class


