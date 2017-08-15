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
#include <bspLeds.h>
#include <util/delay.h>
#include <nwkCommon.h>
#include <macCommon.h >

static AppState_t appstate = APP_INIT_STATE ;
//for identifying the type of device
static HAL_AppTimer_t postTimer; //Definition of the timer
static uint16_t time;
//call back after the config/ setup of nw
static void ZDO_StartNetworkConf ( ZDO_StartNetworkConf_t * confirmInfo );
static ZDO_StartNetworkReq_t networkParams ;
//descriptior of source and destination
static SimpleDescriptor_t simpleDescriptor ;
//end ponit  obj
static APS_RegisterEndpointReq_t endPoint ;
//initilizes endpoints
static void initEndpoint ( void );
//callback when the data arrives
void APS_DataInd ( APS_DataInd_t * indData );
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
static uint8_t deviceType ;
//function to start the transmission
APS_DataReq_t dataReq ;
//callback when data transmission is completed
static void APS_DataConf ( APS_DataConf_t * confInfo );
//initilize data params
static void initTransmitData ( void );

static void openGate();
static void holdGate();
static void closeGate();

void APL_TaskHandler(void){
	
		switch ( appstate ){
			case APP_INIT_STATE :
				//initilizes the serial port
				appInitUsartManager ();
				initPwm();
				//inits LEDs
				BSP_OpenLeds ();
				//sets the state to join nw
				appstate = APP_START_JOIN_NETWORK_STATE ; 
				//posts the task
				SYS_PostTask ( APL_TASK_ID );
				break ;
			case APP_START_JOIN_NETWORK_STATE :
				//defines the nw call back
				networkParams.ZDO_StartNetworkConf = ZDO_StartNetworkConf ;
				//starts the setup/ joining of nw
				ZDO_StartNetworkReq (& networkParams );
				//changes the state to setting up of endpoints
				appstate = APP_INIT_ENDPOINT_STATE ;
				break ;
			case APP_INIT_ENDPOINT_STATE :
				///initilizes the endpoint
				initEndpoint ();
				//check for type od device
				appstate =  APP_NOTHING_STATE;
				SYS_PostTask ( APL_TASK_ID );
				break ;
			case APP_NOTHING_STATE :
				break ;
		}
		
}



void initPwm(void) {
	
			//Set PE3 as output
		

		//Datasheet page 289:
		//Mode of operation for Counter 3:
		//Fast PWM, TOP == ICRn
		//The TOP value is how far the timer counts up in its' register. Section 18.9.3 Fast PWM Mode explains the behavior in detail
		TCCR3A |= (1<<WGM31);
		TCCR3B |= (1<<WGM33) | (1<<WGM32);
		TCCR3A &= ~(1<<WGM30);
		//Prescaler Counter 3: 8 , bring the range to 31250 (8000000/2^8)
		TCCR3B |= (1<<CS31);//pg 311 0x02 sets to clock/8

		//Period duration (TOP) for Counter 3
		ICR3 = 20000; //Fcpu / Prescaler = Cycles/sec.    Clk/sec / time = TOP //as we have set icr as our mode is 1110

		//Pulse duration Counter 3 Channel A

		//Enable output on compare match (output level HIGH) for channel A (PE3)
		TCCR3A |= (1<<COM3A1);//constant high else just a spike

}

static void openGate(){
		if(time > 30000){
		time = 5000;
		}
		OCR3A = 2000;
		DDRE |= (1<<DDRE3); //PE4
		appWriteDataToUsart (( uint8_t *)"open", sizeof ("open") -1);
		postTimer.callback = closeGate;
		delayedPost(time);
}

static void holdGate(){
		DDRE &= ~(1<<DDRE3); //PE4
}

static void closeGate(){
		OCR3A = 1400;
		DDRE |= (1<<DDRE3); //PE4
		postTimer.callback = holdGate;
		appWriteDataToUsart (( uint8_t *)"cccc\r\n", sizeof ("cccc\r\n") -1);
		delayedPost(2110);
		
}


void delayedPost(uint16_t ms){
	postTimer.interval = ms;
	postTimer.mode = TIMER_ONE_SHOT_MODE;
	HAL_StartAppTimer(&postTimer);
}





#pragma region nw
void ZDO_StartNetworkConf ( ZDO_StartNetworkConf_t * confirmInfo ){
	//status of nw config
		if ( ZDO_SUCCESS_STATUS == confirmInfo -> status ){
			//read the tpe of device
			CS_ReadParameter ( CS_DEVICE_TYPE_ID , & deviceType );
			if ( deviceType == DEVICE_TYPE_END_DEVICE ){
				//send out a serial message
				appWriteDataToUsart (( uint8_t *)" Coordinator \r\n", sizeof (" Coordinator \r\n") -1);
			}
				BSP_OnLed ( LED_YELLOW );
		}
		else {
			appWriteDataToUsart (( uint8_t *)" Error \r\n", sizeof (" Error \r\n") -1);
			//appstate = APP_START_JOIN_NETWORK_STATE ;
			
		}
		SYS_PostTask ( APL_TASK_ID );
}


//define the endpoint
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
	BSP_ToggleLed ( LED_RED );
	appWriteDataToUsart ( indData ->asdu , indData -> asduLength );
	appWriteDataToUsart (( uint8_t *)"\r\n" ,2);
	//assigning the ptr to the data
	time = ((*((*(indData)).asdu+2))-'0')+((*((*(indData)).asdu+1))-'0')*10+((*((*(indData)).asdu))-'0')*100;
	//converting to ms
	time = time*1000;
	if(time > 60000){
		time = 5000;
	}
	openGate();
}



//called after the transmission of data
static void APS_DataConf ( APS_DataConf_t * confInfo ){
	if ( confInfo -> status == APS_SUCCESS_STATUS ){
		BSP_OnLed ( LED_YELLOW );
		appstate = APP_NOTHING_STATE ;
		SYS_PostTask ( APL_TASK_ID );
	}
}

#pragma endregion nw


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
//	BSP_OnLed ( LED_RED);
  (void)unbindInd;
  //_delay_ms (500) ;
	//BSP_OffLed ( LED_RED);
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
