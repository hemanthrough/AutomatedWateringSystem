/**************************************************************************//**
  \file app.c

  \brief Basis-Anwendung.

  \author Markus Krauﬂe

******************************************************************************/
#include <appTimer.h>
#include <zdo.h>
#include <app.h>
#include <sysTaskManager.h>
#include <usartManager.h>
#include <leds.h>
#include <adc.h>
#include <i2cPacket.h>
#include <configuration.h>

#pragma region CallbacksDefs
static void readLDRValue(bool result);
static void readSensorTemperatre(bool result);
static void readSensorHumidity(bool result);
static void writeSensorHumidity(bool result);
static void readSensorMoisture(bool result);
static void usartReadConf(void);
//callback when the data arrives
void APS_DataInd ( APS_DataInd_t * indData );
//when net is establoshed or connected
static void ZDO_StartNetworkConf ( ZDO_StartNetworkConf_t * confirmInfo );

//initilizes endpoints
static void initEndpoint ( void );
//initilize data params
static void initTransmitData ( void );
static void APS_DataConf ( APS_DataConf_t * confInfo );

//for serial callback
static void usartReadCb(uint16_t bytesReceived);

#pragma endregion CallbacksDefs

//timer and its handler
static HAL_AppTimer_t postTimer; //Definition of the timer
// enum for state
static state_t state = APP_INIT_STATE;

#pragma region Sensor_variableplaceHolders
//store light value
static uint8_t ldrValue[1];
//store temperature value
static uint8_t tempValue[2];
//store RH
static uint8_t humidityValue[3];
//have moisture
static uint8_t moistureValue [2];

//store light value
static uint16_t finalLdrValue;
//store temperature value
static uint16_t finalTempValue;
//store RH
static uint16_t finalHumidityValue;
//have moisture
static uint16_t finalMoistureValue;

//function to start the transmission
APS_DataReq_t dataReq ;
//for identifying the type of device
static uint8_t deviceType ;

//end ponit  obj
static APS_RegisterEndpointReq_t endPoint ;


static ZDO_StartNetworkReq_t networkParams ;

//descriptior of source and destination
static SimpleDescriptor_t simpleDescriptor ;

//defining the datastruture
//defining the struture of the payload
BEGIN_PACK
typedef struct _AppMessage_t {
	uint8_t header [ APS_ASDU_OFFSET ]; // APS header
	uint8_t data [18];
	uint8_t footer [ APS_AFFIX_LENGTH - APS_ASDU_OFFSET ]; // footer
} PACK AppMessage_t ;
END_PACK
//variable for the payload
static AppMessage_t transmitData ;
//check if initilized
static bool isNWSetup =false;


static uint8_t usartRXBuffer[2];

#pragma endregion Sensor_variableplaceHolders

#pragma region Sensor_inits
//struct LDR

static HAL_AdcDescriptor_t adcdescriptor ={
	.resolution = RESOLUTION_8_BIT ,
	.sampleRate = ADC_4800SPS ,
	.voltageReference = AVCC ,
	.bufferPointer = &ldrValue ,
	.selectionsAmount = 1,
	.callback = readLDRValue
}; 
//struct LM73 temperature
static HAL_I2cDescriptor_t i2cdescriporTemp ={
	.tty = TWI_CHANNEL_0 ,
	. clockRate = I2C_CLOCK_RATE_62 ,
	.f = readSensorTemperatre ,
	.id = LM73_DEVICE_ADDRESS ,
	. data = tempValue ,
	. length = 2,
	. lengthAddr = HAL_NO_INTERNAL_ADDRESS
};
//struct for humidity sht21
static HAL_I2cDescriptor_t i2cdescriptorHumidity ={
	.tty = TWI_CHANNEL_0 ,
	. clockRate = I2C_CLOCK_RATE_62 ,
	.f = readSensorHumidity ,
	.id = SHT21_DEVICE_ADDRESS ,
	. data = humidityValue ,
	. length = 3,
	. lengthAddr =  HAL_NO_INTERNAL_ADDRESS
};

//struct moisture
static HAL_I2cDescriptor_t i2cdescriptorMoisture ={
	.tty = TWI_CHANNEL_0 ,
	. clockRate = I2C_CLOCK_RATE_62 ,
	.f = readSensorMoisture ,
	.id = catnip_DEVICE_ADDRESS ,
	. data = moistureValue ,
	. length = 2,
	. lengthAddr = HAL_NO_INTERNAL_ADDRESS
};
#pragma endregion Sensor_inits


void APL_TaskHandler(void){
	
	switch (state){
		case APP_INIT_STATE:
			//init LEDs
			BSP_OpenLeds();
			//init Usart
			appInitUsartManager();
			usartDescriptor.rxBuffer = usartRXBuffer;
			usartDescriptor.rxBufferLength = sizeof(usartRXBuffer);
			usartDescriptor.rxCallback = usartReadCb;
			//defines the nw call back
			networkParams.ZDO_StartNetworkConf = ZDO_StartNetworkConf ;
			//starts the setup/ joining of nw
			ZDO_StartNetworkReq (& networkParams );
			//start reading sensors
			state = APP_OPEN_ADC;
			CS_ReadParameter ( CS_DEVICE_TYPE_ID , & deviceType );
			if(deviceType == DEVICE_TYPE_COORDINATOR){
				//skips reading of the sensors
				state = APP_START_JOIN_NETWORK_STATE ;
			}
			delayedPost(200);
			break;
		case APP_OPEN_ADC:
			//sanity
			HAL_CloseI2cPacket(&i2cdescriptorMoisture);
			//log
			//appWriteDataToUsart((uint8_t*)"APP_OPEN_ADC\r\n", sizeof("APP_OPEN_ADC\r\n") - 1);
			//open Adc
			HAL_OpenAdc(&adcdescriptor);
			//change state
			state = APP_READ_ADC;
			delayedPost(200);
			break;
		case APP_READ_ADC:
			//log
			//appWriteDataToUsart((uint8_t*)"APP_READ_ADC\r\n", sizeof("APP_READ_ADC\r\n") - 1);
			//read value
			HAL_ReadAdc(&adcdescriptor, HAL_ADC_CHANNEL1);
			//change state
			state = APP_OPEN_LM73;
			delayedPost(200);
			break;
		case APP_OPEN_LM73:
			//sanity
			HAL_CloseI2cPacket(&adcdescriptor);
			//appWriteDataToUsart((uint8_t*)"APP_OPEN_LM73\r\n", sizeof("APP_OPEN_LM73\r\n") - 1);
			//open the packet
			HAL_OpenI2cPacket (&i2cdescriporTemp);
			//start reading values
			state = APP_READ_LM73;
			delayedPost(200);
			break;
		case APP_READ_LM73:
			//log
			//appWriteDataToUsart((uint8_t*)"APP_READ_LM73\r\n", sizeof("APP_READ_LM73\r\n") - 1);
			//read value
			HAL_ReadI2cPacket (&i2cdescriporTemp);
			//start reading humidity
			state = APP_OPEN_SHT21;
			delayedPost(200);
			break;
		case APP_OPEN_SHT21:
			//just a sanity
			HAL_CloseAdc(&adcdescriptor);
			//appWriteDataToUsart((uint8_t*)"APP_OPEN_SHT21\r\n", sizeof("APP_OPEN_SHT21\r\n") - 1);
			//sedning the command for RH
			humidityValue[0]=0b11110101;
			//defining write handler
			i2cdescriptorHumidity.f=writeSensorHumidity;
			//opening the i2c packet
			HAL_OpenI2cPacket (&i2cdescriptorHumidity);
			//writing the command
			HAL_WriteI2cPacket (&i2cdescriptorHumidity);
			//switcing read the value
			state = APP_READ_SHT21;
			delayedPost(200);
			break;
		case APP_READ_SHT21:
			//log
			//appWriteDataToUsart((uint8_t*)"APP_READ_SHT21\r\n", sizeof("APP_READ_SHT21\r\n") - 1);
			//define read handler
			i2cdescriptorHumidity.f=readSensorHumidity;
			HAL_OpenI2cPacket (&i2cdescriptorHumidity);
			//read via i2c
			HAL_ReadI2cPacket (&i2cdescriptorHumidity);
			state = APP_OPEN_CHIRP;
			delayedPost(200);
			break;
		case APP_OPEN_CHIRP:
			//sanity
			HAL_CloseI2cPacket(&i2cdescriptorHumidity);
			//appWriteDataToUsart((uint8_t*)"APP_OPEN_CHIRP\r\n", sizeof("APP_OPEN_CHIRP\r\n") - 1);
			//sedning the command for RH
			moistureValue[0]=0x00;
			//defining write handler
			i2cdescriptorMoisture.f=writeSensorHumidity;
			//opening the i2c packet
			HAL_OpenI2cPacket (&i2cdescriptorMoisture);
			//writing the command
			HAL_WriteI2cPacket (&i2cdescriptorMoisture);
			state = APP_READ_CHIRP;
			delayedPost(200);
			break;
		case APP_READ_CHIRP:
			///appWriteDataToUsart((uint8_t*)"APP_READ_CHIRP\r\n", sizeof("APP_READ_CHIRP\r\n") - 1);
			i2cdescriptorMoisture.f=readSensorMoisture;
			//open i2c packet
			HAL_OpenI2cPacket (&i2cdescriptorMoisture);
			//read via i2c
			HAL_ReadI2cPacket (&i2cdescriptorMoisture);
			state = APP_START_JOIN_NETWORK_STATE ;
			delayedPost(200);
			break;
		case APP_START_JOIN_NETWORK_STATE:
			if(isNWSetup){
				state = APP_TRANSMIT_STATE;
				SYS_PostTask ( APL_TASK_ID );
				break;
			}
			
			//changes the state to setting up of endpoints
			state = APP_INIT_ENDPOINT_STATE ;
			SYS_PostTask ( APL_TASK_ID );
			break ;
		case APP_INIT_ENDPOINT_STATE:
			///initilizes the endpoint
			initEndpoint ();
			//check for type od device
			CS_ReadParameter ( CS_DEVICE_TYPE_ID , & deviceType );
			if(deviceType == DEVICE_TYPE_COORDINATOR){
				
				state = APP_EMPTY ;
			}
			else{
				state = APP_INIT_TRANSMITDATA_STATE ;
			}
			SYS_PostTask ( APL_TASK_ID );
			break ;
		case APP_INIT_TRANSMITDATA_STATE :
			//comes here only if its not cooridinator
			//inits data payload headers and footer
			initTransmitData ();
			//then does nothin until the data is successfuly transmitted or there is a callback
			state = APP_TRANSMIT_STATE ;
			isNWSetup =true;
			//starts the timer
			delayedPost(200);
			
			SYS_PostTask ( APL_TASK_ID );
			break ;
		case APP_TRANSMIT_STATE:
			//defines the payload based on type
			{
			static uint8_t placeHolder[3];
			uint8_t devid =1 ;
			//scaling the value to within a range of 1000 3 digits
			convert2Payload(placeHolder,finalMoistureValue/66);
			transmitData . data [17]= '\n' ;
			transmitData . data [16]= placeHolder[2];
			transmitData . data [15]= placeHolder[1];
			transmitData . data [14]= placeHolder[0];
			transmitData . data [13]= ',' ;
			convert2Payload(placeHolder,finalHumidityValue);
			transmitData . data [12]=  placeHolder[2];
			transmitData . data [11]=  placeHolder[1];
			transmitData . data [10]=  placeHolder[0];
			transmitData . data [9]= ',' ;
			convert2Payload(placeHolder,finalTempValue);
			transmitData . data [8]=  placeHolder[2];
			transmitData . data [7]=  placeHolder[1];
			transmitData . data [6]=  placeHolder[0];
			convert2Payload(placeHolder,finalLdrValue);
			transmitData . data [5]= ',' ;
			transmitData . data [4]=  placeHolder[2];;
			transmitData . data [3]=  placeHolder[1];;
			transmitData . data [2]=  placeHolder[0];;
			//find a way to find the dev id
			convert2Payload(placeHolder,devid);
			transmitData . data [1]= ',' ;
			transmitData . data [0]=  placeHolder[0];;
			}
			APS_DataReq (&dataReq);
			state = APP_OPEN_ADC;
			break ;
		case APP_EMPTY:
			break;
	}	
}

#pragma region timer
static void postTimerCallback(){
	SYS_PostTask(APL_TASK_ID);
}

void delayedPost(uint16_t ms){
	postTimer.interval = ms;
	postTimer.mode = TIMER_ONE_SHOT_MODE;
	postTimer.callback = postTimerCallback;
	HAL_StartAppTimer(&postTimer);
}
#pragma endregion timer


#pragma region readCallbacks
static void readLDRValue(bool result){
	//log
	//appWriteDataToUsart((uint8_t*)"APP_DONE_ADC\r\n", sizeof("APP_DONE_ADC\r\n") - 1);
	//save it to var
	finalLdrValue= ldrValue[0];
	//closing the packet
	HAL_CloseAdc(&adcdescriptor);
}

static void readSensorTemperatre(bool result){
	//converstion from 8 bit to 16
	uint16_t i= tempValue[0];
	i <<= 8;
	i |= tempValue[1];
	i >>= 7;
	//storing the integer part of data
	finalTempValue= i;
	//closing the packet everything
	HAL_CloseI2cPacket(&i2cdescriporTemp);
}

static void readSensorHumidity(bool result){
	delayedPost(50);
	uint16_t humidty = humidityValue[0];
	humidty  <<= 8;
	humidty|= humidityValue[1];
	humidty&= 0xFFFC;
	//saving it n the first byte of the data
	//converting to relative humidity
	finalHumidityValue=  -6.0+(125*((float)humidty/65536)) ;
	HAL_CloseI2cPacket(&i2cdescriptorHumidity);
}


static void readSensorMoisture(bool result){
	uint16_t i=0;
	i=moistureValue[0];
	i <<= 8;
	i |= moistureValue[1];
	finalMoistureValue = i;
	HAL_CloseI2cPacket(&i2cdescriptorMoisture);
}


static void usartReadConf(void){
	uint16_t i=0;
	i=moistureValue[0];
	i <<= 8;
	i |= moistureValue[1];
	finalMoistureValue = i;
}
#pragma endregion readCallbacks

//we consider data to be max of 3 digit it convets to ascii code 
//takes in array size of 4
void convert2Payload(uint8_t* placeHolder,uint8_t value){
	if(value >99){
		placeHolder[2] ='0' +( value % 10) ;
		value /= 10;
	}
	else
	{
		////ascii for space
		placeHolder[2] ='0' ;
	}
	if(value >9){
		placeHolder[1] ='0' +( value % 10) ;
		value /= 10;
	}
	else{
		//ascii for space
		placeHolder[1] ='0' ;
	}
	placeHolder[0] ='0' +( value % 10) ;
}
//write callback
static void writeSensorHumidity(bool result){
	delayedPost(500);
	HAL_CloseI2cPacket(&i2cdescriptorHumidity);
	
}
static uint8_t counter =0;
static uint8_t rdata[2];
static void usartReadCb(uint16_t bytesReceived){
	BSP_ToggleLed ( LED_RED );
	
	
	uint8_t byte;
	HAL_ReadUsart (& usartDescriptor ,&byte ,bytesReceived);
	//sendMessage2Motor(rdata[0],rdata[1]);
	rdata[counter] = byte;
	counter= counter +1;
	if(counter ==2){
		sendMessage2Motor(rdata[0],rdata[1]);
		counter = 0;
	}
	
	HAL_WriteUsart (& usartDescriptor , rdata ,2);
}

//this is send message to motor
void sendMessage2Motor(uint8_t dst, uint8_t time){
	//fake logic
	uint8_t placeHolder[3];

	//HAL_WriteUsart (& usartDescriptor ,dst ,1);
	//HAL_WriteUsart (& usartDescriptor ,time ,1);
	initTransmitData ();
	if(dst==2){
		dataReq . dstAddress . extAddress = CPU_TO_LE16(0x10000000A05LL);
	}
	else{
		dataReq . dstAddress . extAddress = CPU_TO_LE16(0x10000000A04LL);
	}
	convert2Payload(placeHolder,time);
	transmitData . data [0]= placeHolder[0] ;
	transmitData . data [1]= placeHolder[1] ;
	transmitData . data [2]= placeHolder[2] ;
	dataReq . asdu = transmitData . data ;
	APS_DataReq (&dataReq);
}

#pragma region network_commm
void ZDO_StartNetworkConf ( ZDO_StartNetworkConf_t * confirmInfo ){
	//status of nw config
	if ( ZDO_SUCCESS_STATUS == confirmInfo -> status ){
		//read the tpe of device
		CS_ReadParameter ( CS_DEVICE_TYPE_ID , & deviceType );
		if ( deviceType == DEVICE_TYPE_END_DEVICE ){
			//send out a serial message
			//appWriteDataToUsart (( uint8_t *)" Coordinator \r\n", sizeof (" Coordinator \r\n") -1);
		}
			BSP_OnLed ( LED_YELLOW );
	}
	else {
		//appWriteDataToUsart (( uint8_t *)" Error \r\n", sizeof (" Error \r\n") -1);
		//state = APP_START_JOIN_NETWORK_STATE ;
			
	}
	SYS_PostTask ( APL_TASK_ID );
}

//define the endpoint sort of defining the port no
static void initEndpoint ( void ){
	simpleDescriptor . AppDeviceId = 1;
	simpleDescriptor . AppProfileId = 1;
	simpleDescriptor . endpoint = 1;
	simpleDescriptor . AppDeviceVersion = 1;
	endPoint . simpleDescriptor = & simpleDescriptor ;
	endPoint . APS_DataInd = APS_DataInd ;
	APS_RegisterEndpointReq (& endPoint );
}

//called when the data is received
void APS_DataInd ( APS_DataInd_t * indData ){
	//consider removing the led commands
	BSP_ToggleLed ( LED_RED );
	appWriteDataToUsart ( indData ->asdu , indData -> asduLength );
	//appWriteDataToUsart (( uint8_t *)"\r\n" ,2);
}

//setup profile of the data
//write another function to get address dynamically
static void initTransmitData ( void ){
	dataReq . profileId = 1;
	dataReq . dstAddrMode = APS_EXT_ADDRESS ;
	dataReq . dstAddress . extAddress = CPU_TO_LE16(0x10000000A01LL);
	dataReq . dstEndpoint = 1;
	dataReq . asdu = transmitData . data ;
	dataReq . asduLength = sizeof ( transmitData . data );
	dataReq . srcEndpoint = 1;
	dataReq . APS_DataConf = APS_DataConf ;
}
// not required consider deleetin
static void APS_DataConf ( APS_DataConf_t * confInfo ){
	if ( confInfo -> status == APS_SUCCESS_STATUS ){
		state = APP_OPEN_ADC ;
		
	}
	SYS_PostTask ( APL_TASK_ID );
}
#pragma endregion network_comm





/*******************************************************************************
  \brief The function is called by the stack to notify the application about 
  various network-related events. See detailed description in API Reference.
  
  Mandatory function: must be present in any application.

  \param[in] nwkParams - contains notification type and additional data varying
             an event
  \return none
*******************************************************************************/
void ZDO_MgmtNwkUpdateNotf(ZDO_MgmtNwkUpdateNotf_t *nwkParams)
{
  nwkParams = nwkParams;  // Unused parameter warning prevention
}

/*******************************************************************************
  \brief The function is called by the stack when the node wakes up by timer.
  
  When the device starts after hardware reset the stack posts an application
  task (via SYS_PostTask()) once, giving control to the application, while
  upon wake up the stack only calls this indication function. So, to provide 
  control to the application on wake up, change the application state and post
  an application task via SYS_PostTask(APL_TASK_ID) from this function.

  Mandatory function: must be present in any application.
  
  \return none
*******************************************************************************/
void ZDO_WakeUpInd(void)
{
}

#ifdef _BINDING_
/***********************************************************************************
  \brief The function is called by the stack to notify the application that a 
  binding request has been received from a remote node.
  
  Mandatory function: must be present in any application.

  \param[in] bindInd - information about the bound device
  \return none
 ***********************************************************************************/
void ZDO_BindIndication(ZDO_BindInd_t *bindInd)
{
  (void)bindInd;
}

/***********************************************************************************
  \brief The function is called by the stack to notify the application that a 
  binding request has been received from a remote node.

  Mandatory function: must be present in any application.
  
  \param[in] unbindInd - information about the unbound device
  \return none
 ***********************************************************************************/
void ZDO_UnbindIndication(ZDO_UnbindInd_t *unbindInd)
{
  (void)unbindInd;
}
#endif //_BINDING_

/**********************************************************************//**
  \brief The entry point of the program. This function should not be
  changed by the user without necessity and must always include an
  invocation of the SYS_SysInit() function and an infinite loop with
  SYS_RunTask() function called on each step.

  \return none
**************************************************************************/
int main(void)
{
  //Initialization of the System Environment
  SYS_SysInit();




  //The infinite loop maintaing task management
  for(;;)
  {
	  
	  
    //Each time this function is called, the task
    //scheduler processes the next task posted by one
    //of the BitCloud components or the application
    SYS_RunTask();
  }
}





//eof app.c
