package gestione_di_file_di_configurazdione;

import java.io.IOException;

public class ConfigTest {
    public static void main(String[] args) throws IOException {
        System.out.println(Config.getInstance().getNumberOfLives());
    }
}
