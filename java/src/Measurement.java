

import org.apache.commons.math3.stat.correlation.PearsonsCorrelation;
import org.apache.commons.math3.stat.inference.TTest;

import java.util.concurrent.ThreadLocalRandom;

public class Measurement {
    private Boolean testMode = false;
	private Integer device_id = 0;
	private Integer light = 0;
	private Integer temperature = 0;
	private Integer humidity = 0;
	private Integer moisture = 0;
	private Integer score = 0;
	private Integer motor = 0;
	private Double corelation = 0.;


	void update(String measuredData){
	    System.out.println("Received data: "+measuredData);

        if (testMode) {
            randomize();
        } else {
            String[] parts = measuredData.split(",");
			Integer devid =Integer.parseInt(parts[0].trim());
			Integer light = Integer.parseInt(parts[1].trim());
			Integer temperature = Integer.parseInt(parts[2].trim());
			Integer humidity = Integer.parseInt(parts[3].trim());
			Integer moisture = Integer.parseInt(parts[4].trim());
            setDevice_id(devid);
            setLight(light);
            setTemperature(temperature);
            setHumidity(humidity);
            setMoisture(moisture);
			//es
            double satVPD = 0.6108 * Math.exp((17.27*temperature)/(temperature+237.3));
			//ea
            double actVPD = humidity*satVPD/100;
            //vvpd
			double vVPD = satVPD- actVPD;
			double vSR = ((2500*255/light) -500)/3.3;
			double transpiration =  (vSR*.5+vVPD*8.5)/40;
			//get the reuired amount of water assuming radius to be 2inches
			double waterEvaporatedFuture = transpiration* 12.571/48;
			double presentWater_content = moisture/1000 * 2;//mss of the soil to be 2 kg
			double requiredWater = waterEvaporatedFuture- presentWater_content;
			setScore((int) requiredWater);
			//check for light
			if (requiredWater>0){
				Double timeD = requiredWater/  0.7854;
				timeD = Math.max(5,timeD);
				Integer time = timeD.intValue();
				byte[] payload = new byte[2];
				payload[1] = Byte.valueOf(time.toString());
				if(devid==1){
					payload[0] = Byte.valueOf("1");
				}
				else {
					payload[0] = Byte.valueOf("2");
				}
				AutomatedPlantWateringSystem.sendMessage(payload);
			}

        }
	}

	Integer getDevice_id() {
		return this.device_id;
	}

	void setDevice_id(Integer device_id) {
		this.device_id = device_id;
	}

	Integer getLight() {
		return this.light;
	}

	void setLight(Integer light) {
		this.light = light;
	}

	Integer getTemperature() {
		return this.temperature;
	}

	void setTemperature(Integer temperature) {
		this.temperature = temperature;
	}

	Integer getHumidity() {
		return this.humidity;
	}

	void setHumidity(Integer humidity) {
		this.humidity = humidity;
	}

	Integer getMoisture() {
		return this.moisture;
	}

	void setMoisture(Integer moisture) {
		this.moisture = moisture;
	}

	Integer getScore() {
		return this.score;
	}

	void setScore(Integer score) {
		this.score = score;
	}

	Integer getMotor() {
		return this.motor;
	}

	void setMotor(Integer motor) {
		this.motor = motor;
	}

	//error in the measurement
	public void setCorelation(Double corelation) {
		this.corelation = corelation;
	}
	
	void randomize() {
        int randomNumber;
        randomNumber = getRandomNumber();
        setDevice_id(randomNumber);
        randomNumber = getRandomNumber();
        setLight(randomNumber);
        randomNumber = getRandomNumber();
        setTemperature(randomNumber);
        randomNumber = getRandomNumber();
        setHumidity(randomNumber);
        randomNumber = getRandomNumber();
        setMoisture(randomNumber);
        randomNumber = getRandomNumber();
        setScore(randomNumber);
        randomNumber = getRandomNumber();
        setMotor(randomNumber);
	}

	Integer getRandomNumber() {
	    return ThreadLocalRandom.current().nextInt(0, 255 + 1);
    }

    static Integer getDeviceIDFromString(String measuredData) {
        String[] parts = measuredData.split(",");

        return Integer.parseInt(parts[0].trim());
    }
}
