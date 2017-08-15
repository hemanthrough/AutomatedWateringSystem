#include <appTimer.h>
#include <zdo.h>
#include <app.h>
#include <sysTaskManager.h>
#include <usartManager.h>
#include <bspLeds.h>

// The default state.
static AppState_t appstate = APP_INIT_STATE;

// Define network variables and functions.
static uint8_t deviceType;
static void ZDO_StartNetworkConf(ZDO_StartNetworkConf_t *confirmInfo);
static ZDO_StartNetworkReq_t networkParams;

// Define timer variables and functions.
HAL_AppTimer_t sendTimer;
static void sendTimerFired(void);

// Define other custom functions.
static void initializeSystem(void);
static void sendToUSART(void);

void APL_TaskHandler(void) {
	switch (appstate) {
		case APP_INIT_STATE:
			// Initialize system.
			initializeSystem();
			
			// Switch to network configuration state.
			appstate = APP_START_JOIN_NETWORK_STATE;
			
			SYS_PostTask(APL_TASK_ID);
			break;
		case APP_START_JOIN_NETWORK_STATE:
			// Set callback function.
			networkParams.ZDO_StartNetworkConf = ZDO_StartNetworkConf;
			
			// Start network configuration.
			ZDO_StartNetworkReq(&networkParams);
			appstate = APP_NOTHING_STATE;
			break;
		case APP_SEND_STATE:
			sendToUSART();
			appstate = APP_NOTHING_STATE;
			break;
		case APP_NOTHING_STATE:
			break;
	}
}

void initializeSystem(void) {
	// Read the device type.
	CS_ReadParameter(CS_DEVICE_TYPE_ID, &deviceType);
	
	// Initialize the timer.
	sendTimer.interval = 1000;
	sendTimer.mode = TIMER_REPEAT_MODE;
	sendTimer.callback = sendTimerFired;
	HAL_StartAppTimer(&sendTimer);
	
	// Initialize the USART manager.
	appInitUsartManager();
	
	// Initialize the LEDs.
	BSP_OpenLeds();
}

void sendTimerFired(void) {
	appstate = APP_SEND_STATE;
	SYS_PostTask(APL_TASK_ID);
}

// Let the coordinator regularly send some test-data to the USART interface.
void sendToUSART(void) {
	if (deviceType == DEVICE_TYPE_COORDINATOR) {
		BSP_OnLed(LED_YELLOW);
		appWriteDataToUsart((uint8_t*)"Hello from Coordinator!\r\n", sizeof("Hello from Coordinator!\r\n")-1);
		BSP_OffLed(LED_YELLOW);
	}
}

// Callback function for the network configuration.
// Sends information about the status to USART.
// Turns on RED or GREEN LED based on network status.
void ZDO_StartNetworkConf(ZDO_StartNetworkConf_t *confirmInfo) {
	if (ZDO_SUCCESS_STATUS == confirmInfo->status) {
		if (deviceType == DEVICE_TYPE_COORDINATOR) {
			appWriteDataToUsart((uint8_t*)"Coordinator Network Configuration Done!\r\n", sizeof("Coordinator Network Configuration Done!\r\n")-1);
		} else if (deviceType == DEVICE_TYPE_ROUTER) {
			appWriteDataToUsart((uint8_t*)"Router Network Configuration Done!\r\n", sizeof("Router Network Configuration Done!\r\n")-1);
		} else if (deviceType == DEVICE_TYPE_END_DEVICE) {
			appWriteDataToUsart((uint8_t*)"End Device Network Configuration Done!\r\n", sizeof("End Device Network Configuration Done!\r\n")-1);
		}
		
		BSP_OnLed(LED_GREEN);
	} else {
		appWriteDataToUsart((uint8_t*)"Network Configuration Error!\r\n", sizeof("Network Configuration Error!\r\n")-1);
		
		BSP_OnLed(LED_RED);
	}
	
	SYS_PostTask(APL_TASK_ID);
}

/*******************************************************************************
  \brief The function is called by the stack to notify the application about 
  various network-related events. See detailed description in API Reference.
  
  Mandatory function: must be present in any application.

  \param[in] nwkParams - contains notification type and additional data varying
             an event
  \return none
*******************************************************************************/
void ZDO_MgmtNwkUpdateNotf(ZDO_MgmtNwkUpdateNotf_t *nwkParams) {
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
void ZDO_WakeUpInd(void) {
}

#ifdef _BINDING_
/***********************************************************************************
  \brief The function is called by the stack to notify the application that a 
  binding request has been received from a remote node.
  
  Mandatory function: must be present in any application.

  \param[in] bindInd - information about the bound device
  \return none
 ***********************************************************************************/
void ZDO_BindIndication(ZDO_BindInd_t *bindInd) {
	(void)bindInd;
}

/***********************************************************************************
  \brief The function is called by the stack to notify the application that a 
  binding request has been received from a remote node.

  Mandatory function: must be present in any application.
  
  \param[in] unbindInd - information about the unbound device
  \return none
 ***********************************************************************************/
void ZDO_UnbindIndication(ZDO_UnbindInd_t *unbindInd) {
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
int main(void) {
	//Initialization of the System Environment
	SYS_SysInit();

	//The infinite loop maintaing task management
	for(;;) {
		//Each time this function is called, the task
		//scheduler processes the next task posted by one
		//of the BitCloud components or the application
		SYS_RunTask();
	}
}

//eof app.c