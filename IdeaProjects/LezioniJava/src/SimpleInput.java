import java.io.IOException;

public class SimpleInput {
    public static void main(String[] args) throws IOException {
        byte[] byteArray = new byte[8];
        int numBytesRead;
        int car;
        /*while((car = System.in.read()) >= 0)
        {
            System.out.write(car);
        }*/
        while((numBytesRead = System.in.read(byteArray)) >= 0)
            System.out.write(byteArray, 0, byteArray.length);
    }
}
