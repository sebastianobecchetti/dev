package gestione_di_file_di_configurazdione;

import java.io.*;
import java.util.Properties;

public class Config {
    //PRIVATE CONSTANT
    private final static String CONFIG_FILE_FULL_PATH = "/home/gay/dev/IdeaProjects/LezioniJava/src/gestione_di_file_di_configurazdione/config.txt";
//    private final static String CONFIG_FILE_RELATIVE_PATH = "/src/gestione_di_file_di_configurazdioneconfig.txt";
    //STATIC FIELDS
    private static Config instance = null;


    //INSTANCE FIELDS
    private Properties properties;

    private Config() throws IOException, FileNotFoundException {
        BufferedReader br = null;
        try{
            br = new BufferedReader(new InputStreamReader(new FileInputStream(CONFIG_FILE_FULL_PATH ), "UTF-8" ));

            this.properties = new Properties();
            this.properties.load(br);
        }
        catch(Exception e){
            e.printStackTrace();
        }
        finally{
            if(br != null){
                br.close();
            }
        }
    }
    //INSTANCE METHODS
    public int getNumberOfLives() {
        return Integer.valueOf(this.properties.getProperty("numlives"));
    }

    //STATIC METHODS
    public static Config getInstance() throws IOException {
        if (instance == null)
        {
            instance = new Config();
        }
        return instance;
    }

}
