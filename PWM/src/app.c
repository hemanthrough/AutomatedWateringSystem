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
#include <halPwm.h>
#include <pwm.h>
#include <sysUtils.h>

static AppState_t appstate = APP_INIT_STATE ;
//for identifying the type of device

static HAL_PwmDescriptor_t descriptor;
static HAL_AppTimer_t postTimer; 

void APL_TaskHandler(void){
	
		switch ( appstate ){
			case APP_INIT_STATE :
				//initilizes the serial port
				appInitUsartManager ();
				initPwm();
				//inits timers
				//inits LEDs
				BSP_OpenLeds ();
				//sets the state to join nw
				appstate = APP_START_JOIN_NETWORK_STATE ; 
				//posts the task
				SYS_PostTask ( APL_TASK_ID );
				
				break ;
			case APP_START_JOIN_NETWORK_STATE :
			{
				
				//setPwmDutyCycle(50);
				//appstate = APP_INIT_ENDPOINT_STATE ;
				DDRE ^= (1<<DDRE3);
 				delayedPost(21);
			}
				break ;
			case APP_NOTHING_STATE :
				break ;
		}
		
}
static void postTimerCallback(){
	SYS_PostTask(APL_TASK_ID);
}


void initPwm(void) {
	
			//Set PE3 as output
		DDRE |= (1<<DDRE3); //PE4

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
		OCR3A = 1100 ;

		//Enable output on compare match (output level HIGH) for channel A (PE3)
		TCCR3A |= (1<<COM3A1);//constant high else just a spike

}


void delayedPost(uint16_t ms){
	postTimer.interval = ms;
	postTimer.mode = TIMER_ONE_SHOT_MODE;
	postTimer.callback = postTimerCallback;
	HAL_StartAppTimer(&postTimer);
}




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
