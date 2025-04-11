package lezione_stream_char;

import java.util.LinkedList;
import java.io.BufferedReader;
import java.io.InputStreamReader;
import java.io.FileInputStream;
import java.io.FileNotFoundException;
import java.io.IOException;

public class ReadCSV {

	public static LinkedList<String[]> getRows(String fileName, String charset) throws FileNotFoundException, IOException {
		LinkedList<String[]> lstRows = null;

		BufferedReader br = null;
		try {
			br = new BufferedReader(new InputStreamReader(new FileInputStream(fileName), charset));

			lstRows = new LinkedList<String[]>();
			String s = null;

			while ((s = br.readLine()) != null)
				if (s.contains(";"))
					lstRows.add(s.trim().split(";"));
		}
		catch(IOException ioe) {
			ioe.printStackTrace();
		}
		finally {
			if (br != null)
				br.close();
		}

		return lstRows;
	}
} // end class
