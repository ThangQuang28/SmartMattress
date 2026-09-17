/***************************************************************************************************************
 * @auth            : duonghd                                                                                
 * @day             : 10-Mar-2026
 * @file            : scr.h                                                                            
 * @dissaption      : DC motor driver board support package.                                           
***************************************************************************************************************/

/*  Indent using spaces
    Tab width: 4 spaces
    Line width: 100 characters
    Line ending: LF (0x0A) */

#ifndef _SCR_H
#define _SCR_H

/* Public macros ---------------------------------------------------------------------------------*/
/* Include ---------------------------------------------------------------------------------------*/

#ifdef __cplusplus
extern "C" {
#endif

/* Define ----------------------------------------------------------------------------------------*/
/* Private data types ----------------------------------------------------------------------------*/
/* Public data types -----------------------------------------------------------------------------*/

/* Data struct */
typedef struct
{
    float temperature;
    float humidity;
    float pressure;
} scr_env_t;

typedef enum {
    SCR_TEMP=0u,
    SCR_HUM,
    SCR_LIMIT
} scr_type_t;

/* Private function prototypes -------------------------------------------------------------------*/
/* Public function prototypes --------------------------------------------------------------------*/

void scr_open(void);

void scr_update(scr_type_t type, scr_env_t *env);

void scr_write(void);

void scr_draw_bitmap(
    uint8_t x,
    uint8_t y,
    uint8_t width,
    uint8_t height,
    const uint8_t *bitmap);

void scr_cls(void);

#ifdef __cplusplus
}
#endif

#endif /* _SCR_H */

/* END OF FILE ************************************************************************************/
