package lezione_stream_char;

import java.util.LinkedList;

public class Test {

    public static void main(String[] args) throws Exception {

        String fileName = "src/lezione_stream/data.csv";
        String charset = "UTF-8";

        LinkedList<String[]> lstRows = ReadCSV.getRows(fileName, charset);

        PlayerData pData = new PlayerData();

        for (String[] sArr : lstRows)
            pData.add(new Player(sArr[0], sArr[1], Integer.parseInt(sArr[2])));

        WriteCSV.print("src/lezione_stream/data_writed.csv", charset, pData.asListOfStringArray());

    } // end method main()
} // end class

