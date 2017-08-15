import gnu.io.*;
import java.io.*;
import java.util.*;

public class Communicator implements SerialPortEventListener {
    private GUI guiObject;
    private Measurement sensor1Measurement;
    private Measurement sensor2Measurement;

    // Map port names to CommPortIdentifiers.
    private HashMap<String, CommPortIdentifier> portMap = new HashMap<>();

    // Object that contains the opened port.
    private SerialPort serialPort = null;

    // Input/Output streams for sending/receiving data.
    private InputStream input = null;
    private OutputStream output = null;

    Communicator(GUI guiObject, Measurement sensor1, Measurement sensor2) {
        this.guiObject = guiObject;
        this.sensor1Measurement = sensor1;
        this.sensor2Measurement = sensor2;
    }

    // Show all available serial ports.
    void searchForPorts() {
        System.out.println("List of available serial ports:");

        Enumeration ports = CommPortIdentifier.getPortIdentifiers();

        while (ports.hasMoreElements()) {
            CommPortIdentifier curPort = (CommPortIdentifier) ports.nextElement();

            // Make sure that we only get serial ports.
            if (curPort.getPortType() == CommPortIdentifier.PORT_SERIAL) {
                System.out.println(curPort.getName());
                portMap.put(curPort.getName(), curPort);
            }
        }
    }

    // Connect to the selected port.
    Boolean connect(String selectedPort, Integer timeout) {
        try {
            CommPortIdentifier selectedPortIdentifier = portMap.get(selectedPort);
            CommPort commPort = selectedPortIdentifier.open("PlantWateringPort", timeout);
            serialPort = (SerialPort)commPort;

            System.out.println(selectedPort+" opened successfully.");
            return true;
        } catch (PortInUseException e) {
            System.out.println(selectedPort+" is in use. ("+e.toString()+")");
        } catch (Exception e) {
            System.out.println("Failed to open "+selectedPort+"("+e.toString()+")");
        }

        return false;
    }

    // Open the input and output streams.
    boolean initIOStream() {
        try {
            input = serialPort.getInputStream();
            output = serialPort.getOutputStream();

            System.out.println("I/O Streams opened successfully.");
            return true;
        } catch (IOException e) {
            System.out.println("I/O Streams failed to open. ("+e.toString()+")");
        }

        return false;
    }

    // Starts the event listener that knows whenever data is available to be read.
    void initListener() {
        try {
            serialPort.addEventListener(this);
            serialPort.notifyOnDataAvailable(true);
            serialPort.setSerialPortParams(38400, SerialPort.DATABITS_8, SerialPort.STOPBITS_1, SerialPort.PARITY_NONE);

            System.out.println("Listener initialized successfully.");
        } catch (TooManyListenersException e) {
            System.out.println("Too many listeners. ("+e.toString()+")");
        } catch (UnsupportedCommOperationException e) {
            System.out.println("UnsupportedCommOperationException. ("+e.toString()+")");
        }
    }

    // Disconnect the serial port.
    void disconnect() {
        try {
            serialPort.removeEventListener();
            serialPort.close();
            input.close();
            output.close();

            System.out.println("Disconnected.");
        } catch (Exception e) {
            System.out.println("Failed to close "+serialPort.getName()+"("+e.toString()+")");
        }
    }

    private StringBuilder stringBuilder = new StringBuilder();

    // Reading received data.
    synchronized public void serialEvent(SerialPortEvent serialPortEvent) {
        if (serialPortEvent.getEventType() == SerialPortEvent.DATA_AVAILABLE) {
            try {
                byte singleData = (byte)input.read();

                // Check if a new-line ASCII got received.
                if (singleData == 10) {
                    String finalString = stringBuilder.toString();

                    if (Measurement.getDeviceIDFromString(finalString) == 1) {
                        sensor1Measurement.update(finalString);
                        Database.insertMeasurement(sensor1Measurement);
                    } else {
                        sensor2Measurement.update(finalString);
                        Database.insertMeasurement(sensor2Measurement);
                    }

                    guiObject.updateGUI();

                    stringBuilder = new StringBuilder();
                } else {
                    String receivedData = new String(new byte[] {singleData});
                    stringBuilder.append(receivedData);
                }
            } catch (Exception e) {
                System.out.println("serial event db:");
                System.out.println("Failed to read data. ("+e.toString()+")");
            }
        }
    }

    // Method to send data.
    void writeData(byte[] value) {
        try {
            output.write(value);
            System.out.println("writing to serial");
            output.flush();
        } catch (Exception e) {
            System.out.println("Failed to write data. ("+e.toString()+")");
        }
    }
}
