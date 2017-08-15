import java.awt.*;
import java.awt.event.*;
import javax.swing.*;
import javax.swing.undo.AbstractUndoableEdit;

public class GUI extends JFrame implements ActionListener {
    private Measurement sensor1Measurement;
    private Measurement sensor2Measurement;

	JLabel DM1 = new JLabel("NONE");
    JLabel DM2 = new JLabel("NONE");
    JLabel DT1 = new JLabel("NONE");
    JLabel DT2 = new JLabel("NONE");
    JLabel DL1 = new JLabel("NONE");
    JLabel DL2 = new JLabel("NONE");
    JLabel DW1 = new JLabel("NONE");
    JLabel DW2 = new JLabel("NONE");

    JTextField enter1 = new JTextField("5");
    JTextField enter2 = new JTextField("5");

    JButton BN1 = new JButton("Sensor 1");
    JButton BN2 = new JButton("Sensor 2");

    JButton BS1 = new JButton("Watering Plant 1");
    JButton BS2 = new JButton("Watering Plant 2");
	
	public GUI(Measurement sensor1, Measurement sensor2){
        this.sensor1Measurement = sensor1;
        this.sensor2Measurement = sensor2;

		try {
			UIManager.setLookAndFeel(UIManager.getSystemLookAndFeelClassName());
		} catch (Exception e) {
			System.out.println(e.toString());
		}

        // Build the output window frame.
        JFrame frame = new JFrame("Automated Plant Watering System");
        frame.setDefaultCloseOperation(WindowConstants.EXIT_ON_CLOSE);
        frame.getContentPane().add(new MainPanel());
        frame.pack();
        frame.setVisible(true);

        // Add ActionListener.
        BN1.addActionListener(this);
        BN2.addActionListener(this);
        BS1.addActionListener(this);
        BS2.addActionListener(this);
	}

	public class MainPanel extends JPanel {
	    MainPanel() {
            setLayout(new BoxLayout(this, BoxLayout.PAGE_AXIS));
            add(new TopArea());
            add(new PanelMonitoring());
            add(new BottomArea());
        }
    }

    public class TopArea extends JPanel {
	    TopArea() {
            setLayout(new BoxLayout(this, BoxLayout.LINE_AXIS));
            setBorder(BorderFactory.createEmptyBorder(20, 20, 20, 20));

            JPanel north = new JPanel();
            north.setLayout(new GridLayout(1, 5, 20, 20));

            JPanel description = new JPanel();
            description.setLayout(new GridLayout(3, 1));
            description.add(new JLabel("Measured Values"));
            description.add(new JLabel("Sensor 1:"));
            description.add(new JLabel("Sensor 2:"));

            JPanel moisture = new JPanel();
            moisture.setLayout(new GridLayout(3, 1));
            moisture.add(new JLabel("Moisture:"));
            moisture.add(DM1);
            moisture.add(DM2);

            JPanel temperature = new JPanel();
            temperature.setLayout(new GridLayout(3, 1));
            temperature.add(new JLabel("Temperature:"));
            temperature.add(DT1);
            temperature.add(DT2);

            JPanel light = new JPanel();
            light.setLayout(new GridLayout(3, 1));
            light.add(new JLabel("Light:"));
            light.add(DL1);
            light.add(DL2);

            JPanel measurementButtons = new JPanel();
            measurementButtons.setLayout(new GridLayout(3, 1));
            measurementButtons.add(new JLabel("New Measurement:"));
            measurementButtons.add(BN1);
            measurementButtons.add(BN2);

            north.add(description);
            north.add(moisture);
            north.add(temperature);
            north.add(light);
            north.add(measurementButtons);

            this.add(north);
        }
    }

    private class PanelMonitoring extends JPanel {
        private PanelMonitoring() {
            setLayout(new BoxLayout(this, BoxLayout.LINE_AXIS));

            Chart chart1 = new Chart(sensor1Measurement);
            Chart chart2 = new Chart(sensor2Measurement);

            add(chart1.getChartPanel());
            add(chart2.getChartPanel());

            chart1.startChart();
            chart2.startChart();
        }
    }

    public class BottomArea extends JPanel {
        BottomArea() {
            setLayout(new BoxLayout(this, BoxLayout.LINE_AXIS));
            setBorder(BorderFactory.createEmptyBorder(20, 20, 20, 20));

            JPanel south = new JPanel();
            south.setLayout(new GridLayout(1, 4, 20, 20));

            JPanel description = new JPanel();
            description.setLayout(new GridLayout(3, 1));
            description.add(new JLabel("Watering"));
            description.add(new JLabel("Sensor 1:"));
            description.add(new JLabel("Sensor 2:"));

            JPanel idealWaterAmount = new JPanel();
            idealWaterAmount.setLayout(new GridLayout(3, 1));
            idealWaterAmount.add(new JLabel("Ideal Water Amount:"));
            idealWaterAmount.add(DW1);
            idealWaterAmount.add(DW2);

            JPanel manualWaterAmount = new JPanel();
            manualWaterAmount.setLayout(new GridLayout(3, 1));
            manualWaterAmount.add(new JLabel("Manual Water Amount:"));
            manualWaterAmount.add(enter1);
            manualWaterAmount.add(enter2);

            JPanel wateringPlants = new JPanel();
            wateringPlants.setLayout(new GridLayout(3, 1));
            wateringPlants.add(new JLabel("Watering Plants:"));
            wateringPlants.add(BS1);
            wateringPlants.add(BS2);

            south.add(description);
            south.add(idealWaterAmount);
            south.add(manualWaterAmount);
            south.add(wateringPlants);

            this.add(south);
        }
    }

	@Override
	public void actionPerformed(ActionEvent arg0) {
		if (arg0.getSource() == BN1 || arg0.getSource() == BN2) {
			updateGUI();
		} else if (arg0.getSource() == BS1 || arg0.getSource() == BS2) {
            byte b[]= new byte[2];
            b[1] = Byte.valueOf("5");
            Integer time;
            
            try {
    			if (arg0.getSource() == BS1) {
                    b[0] = Byte.valueOf("1");
                    time = Integer.parseInt(enter1.getText());
                } else {
                    b[0] = Byte.valueOf("2");
                    time = Integer.parseInt(enter2.getText());
                }
            } catch (Exception e) {
                System.out.println("Failed to convert number. ("+e.toString()+")");
            }
            
            Byte.valueOf(time.toString());
            AutomatedPlantWateringSystem.sendMessage(b);
		}
		int temperature = 28;
		int humidity = 38;
		int light =111;
		int moisture =73;
        //es
        double satVPD = 0.6108 * Math.exp((17.27*temperature)/(temperature+237.3));
        //ea
        double actVPD = humidity*satVPD/100;
        //vvpd
        double vVPD = satVPD- actVPD;
        double vSR = ((2500*255/light) -500)/3.3;
        double transpiration =  (vSR*.5+vVPD*8.5)/40;
        //get the reuired amount of water assuming radius to be 1inches
        double waterEvaporatedFuture = transpiration* 3.1428/48;
        double presentWater_content = ((double)moisture)/100;//mss of the soil to be 2 kg
        double requiredWater = waterEvaporatedFuture- presentWater_content;
        //setScore((int) requiredWater);


    }

	public void updateGUI() {
        DM1.setText(""+sensor1Measurement.getMoisture());
        DT1.setText(""+sensor1Measurement.getTemperature());
        DL1.setText(""+sensor1Measurement.getLight());
        DW1.setText(""+sensor1Measurement.getScore());

        DM2.setText(""+sensor2Measurement.getMoisture());
        DT2.setText(""+sensor2Measurement.getTemperature());
        DL2.setText(""+sensor2Measurement.getLight());
        DW2.setText(""+sensor2Measurement.getScore());
    }
}
