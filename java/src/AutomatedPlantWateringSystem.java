import java.util.Timer;
import java.util.TimerTask;

public class AutomatedPlantWateringSystem {
    private static  Communicator com = null;
    public static void main(String[] args) {
        // Register measurement objects.
        Measurement sensor1 = new Measurement();
        Measurement sensor2 = new Measurement();

        // Start interface.
        GUI gui = new GUI(sensor1, sensor2);

        Database.openConnection();

        Communicator communicator = new Communicator(gui, sensor1, sensor2);
        communicator.searchForPorts();
        com = communicator;
        if (communicator.connect("COM5", 2000))
        {
            if (communicator.initIOStream())
            {
                communicator.initListener();
            }
        }



        //communicator.disconnect();
    }
    public  static  void sendMessage(byte[] payload ){
        if (com != null&& payload!=null){
            com.writeData(payload);
        }
    }

}
