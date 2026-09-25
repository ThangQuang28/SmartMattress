/***************************************************************************************************************
 * @auth            : phucdh                                                                                
 * @day             : 17-Jun-2026
 * @file            : zschedule.h                                                                            
 * @dissaption      : Time service driver board support package.                                           
***************************************************************************************************************/

/*  Indent using spaces
    Tab width: 4 spaces
    Line width: 100 characters
    Line ending: LF (0x0A) */

#ifndef _ZSCHEDULE_H
#define _ZSCHEDULE_H

/* Public macros ---------------------------------------------------------------------------------*/
/* Include ---------------------------------------------------------------------------------------*/

#ifdef __cplusplus
extern "C" {
#endif

/* Define ----------------------------------------------------------------------------------------*/
#define MAX_SCHEDULE 24
/* Private data types ----------------------------------------------------------------------------*/
/* Public data types -----------------------------------------------------------------------------*/
typedef struct
{
    uint8_t hour;
    uint8_t minute;
    uint8_t speed;

    bool enable;
    bool executed;

} fan_schedule_t;
/* Private function prototypes -------------------------------------------------------------------*/
/* Public function prototypes --------------------------------------------------------------------*/

void zschedule_set(
    uint8_t index,
    uint8_t hour,
    uint8_t minute,
    uint8_t speed);

bool zschedule_get(
    uint8_t index,
    fan_schedule_t *schedule);

void zschedule_set_executed(
    uint8_t index,
    bool executed);
void zschedule_clear(void);
#ifdef __cplusplus
}
#endif

#endif /* _ZSCHEDULE_H */

/* END OF FILE ************************************************************************************/
