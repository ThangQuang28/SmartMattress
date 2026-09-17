/**
  **************************************************************************************************
  * @file   cmd.h
  * @brief  Starting point of this project.
  * @by     duonghd | 13-Mar-2026
  * @note   This file contains the command handling functions and definitions used across the project.
  **************************************************************************************************
  */

/*  Indent using spaces
    Tab width   : 4 spaces
    Line width  : 100 characters
    Line ending : LF (0x0A) */

#ifndef _CMD_H
#define _CMD_H

#ifdef __cplusplus
extern "C" {
#endif

/* Public macros ---------------------------------------------------------------------------------*/

#define MODE_SEND               0xAB // Command from MCU to ESP.
#define MODE_RECV               0xAA // Command from ESP to MCU.
								
#define CMD_FAN_POWER          	0x01 // Command to turn the fan on.
#define	POWER_DATA_ON			0x01 // On
#define POWER_DATA_OFF			0x00 // Off
								
#define CMD_FAN_CTRL_SPEED      0x03 // Command to control the fan speed.

#define FAN_SPEED_0		        0x00 // Fan speed 1.
#define FAN_SPEED_1		        0x01 // Fan speed 1.
#define FAN_SPEED_2		        0x02 // Fan speed 2.
#define FAN_SPEED_3		        0x03 // Fan speed 3.
#define FAN_SPEED_4  	        0x04 // Fan speed 4.
#define FAN_SPEED_5		        0x05 // Fan speed 5.
#define FAN_SPEED_6		        0x06 // Fan speed 6.
#define FAN_SPEED_7		        0x07 // Fan speed 7.
#define FAN_SPEED_8		        0x08 // Fan speed 8.
#define FAN_SPEED_9		        0x09 // Fan speed 9.
#define FAN_SPEED_10	        0x0A // Fan speed 10.
#define FAN_SPEED_11	        0x0B // Fan speed 11.
#define FAN_SPEED_12	        0x0C // Fan speed 12.


#define CMD_FAN_CTRL_MODE       0x04 // Command to control the fan mode.

#define	FAN_MODE_DATA_ON		0x01 // On
#define FAN_MODE_DATA_OFF		0x00 // Off
								
#define CMD_FAN_SWING   		0x05 // Command to switch the fan to windy mode.

#define SWING_DATA_OFF			0x00 // Command to swing up, down off.
#define SWING_DATA_DOWN			0x02 // Command to swing up off, down on.
#define SWING_DATA_UP			0x01 // Command to swing up on, down off.
#define SWING_DATA_UP_DOWN		0x03 // Command to swing up, down on.
								
#define CMD_FAN_SET_TIMER       0x08 // Command to set the timer for the fan.


#define CMD_FAN_SET_ANGLE       0x09 // Command to set the angle for the fan.

#define CMD_FAN_PEO_DETECT      0x10 // Command to detect people presence using the fan's sensors.
#define CMD_FAN_PEO_SEN_DATA    0x0B // Command to send the people sensor data from the fan to the ESP.

#define CMD_SENSOR_AHT			0x11

#define CMD_FAN_TEMP            0x11 // Command to send the temperature data from the fan to the ESP.
#define CMD_FAN_HUMIDITY        0x12 // Command to send the humidity data from the fan to the ESP.
								
#define CRC_SUCCESS             0x00 // Command with valid CRC.
#define CRC_FAIL                0x01 // Command with invalid CRC.

/* Public data types -----------------------------------------------------------------------------*/

/* Public function prototypes --------------------------------------------------------------------*/

#ifdef __cplusplus
}
#endif

#endif /* _CMD_H */

/* END OF FILE ************************************************************************************/