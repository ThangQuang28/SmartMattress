/***************************************************************************************************************
 * @auth            : phucdh                                                                                
 * @day             : 17-Jun-2026
 * @file            : ztime.h                                                                            
 * @dissaption      : Time service driver board support package.                                           
***************************************************************************************************************/

/*  Indent using spaces
    Tab width: 4 spaces
    Line width: 100 characters
    Line ending: LF (0x0A) */

#ifndef _ZTIME_H
#define _ZTIME_H

/* Public macros ---------------------------------------------------------------------------------*/
/* Include ---------------------------------------------------------------------------------------*/

#ifdef __cplusplus
extern "C" {
#endif

/* Define ----------------------------------------------------------------------------------------*/
/* Private data types ----------------------------------------------------------------------------*/
/* Public data types -----------------------------------------------------------------------------*/
/* Private function prototypes -------------------------------------------------------------------*/
/* Public function prototypes --------------------------------------------------------------------*/

void ztime_open(void);
// void fan_timer_start(uint32_t delay_ms, uint8_t cmd, uint8_t data);
void fan_timer_start(uint32_t delay_ms);

void fan_timer_stop(void);

#ifdef __cplusplus
}
#endif

#endif /* _ZTIME_H */

/* END OF FILE ************************************************************************************/
