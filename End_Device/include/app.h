/**************************************************************************//**
  \file app.h

  \brief Usart-Anwendung Headerdatei.

  \author
    Markus Krauﬂe

******************************************************************************/

#ifndef _APP_H
#define _APP_H

#define APP_SENDE_INTERVAL    1000
typedef enum{
	APP_INIT_STATE,
	APP_OPEN_ADC,
	APP_READ_ADC,
	APP_OPEN_SHT21,
	APP_OPEN_LM73,
	APP_OPEN_CHIRP,
	APP_READ_SHT21,
	APP_READ_LM73,
	APP_READ_CHIRP,
	APP_OPEN_BMP180,
	APP_READ_BMP180,
	APP_START_JOIN_NETWORK_STATE,
	APP_INIT_ENDPOINT_STATE,
	APP_INIT_TRANSMITDATA_STATE,
	APP_TRANSMIT_STATE,
	APP_EMPTY
} state_t;
#endif

//i2c device address
#define LM73_DEVICE_ADDRESS 0x4D
#define SHT21_DEVICE_ADDRESS 0x40
#define catnip_DEVICE_ADDRESS 0x20
// eof app.h