import java.io.BufferedReader;
import java.io.IOException;
import java.io.InputStreamReader;

public class TestInputFromConsole {

    public static void main(String[] args) {

        int dim = Integer.valueOf(args[0]); //l'argomento si setta sulle impostazioni del run
        String[] strArray = new String[dim];

        BufferedReader buffRead = new BufferedReader(new InputStreamReader(System.in));

        int index = -1;
        String str = null;
        boolean end = false;
        while (!end) {
           try{
               System.out.println("Inserisci un indice i oppure q per terminare.");
            str = buffRead.readLine();
            if (str.toLowerCase().trim().equals("q")) {
                end = true;
                buffRead.close();
                break;
            }
            index = Integer.valueOf(str);
            System.out.println("Inserisci una parola per strArray[" + index + "].");
            str = buffRead.readLine();
            strArray[index] = str;
           }
           catch (IOException | ArrayIndexOutOfBoundsException | NumberFormatException e) {
               e.printStackTrace();
           }
        } // end while

        System.out.println("Valori inseriti:");
        for (int i = 0; i < strArray.length; i++) {
            try {
                System.out.println("strArray[" + i + "] = " + strArray[i]);
            }
            catch (Exception e) {
                e.printStackTrace();
            }
        }
    } // end method main()

} // end class