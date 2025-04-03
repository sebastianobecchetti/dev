
import java.io.FileInputStream;
import java.io.FileNotFoundException;
import java.io.FileOutputStream;
import java.io.IOException;

    public class CopyBytes {

        private static int BUFFER_SIZE = 1000;
        private static String USAGE_MSG = "\nUsage: java CopyBytes <source_file> <destination_file> ...\n";

        public static void main(String[] args) throws IOException, IllegalArgumentException {
            if (args.length != 2) {
                throw new IllegalArgumentException(USAGE_MSG); //lancia l'eccezione e mostra un messaggio all'utente di come eseguire il programma
            }

            FileInputStream in = null;
            FileOutputStream out = null;
            try{
                in = new FileInputStream(args[0]);
                out = new FileOutputStream(args[1]);

                byte[] buffer = new byte[BUFFER_SIZE];
                int bytesRead;
                while((bytesRead = in.read(buffer)) != -1){
                    out.write(buffer, 0, bytesRead);
                }

            }
            catch(FileNotFoundException e){
                e.printStackTrace();
            }
            catch(IOException e){
                e.printStackTrace();
            }
            finally{
                if(in != null)
                    in.close();
                if(out != null)
                    out.close();
            }
        }
    }