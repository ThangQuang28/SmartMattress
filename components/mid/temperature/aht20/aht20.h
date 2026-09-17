/***************************************************************************************************************
 * @auth            : duonghd                                                                                
 * @day             : 10-Mar-2026
 * @file            : aht20.h                                                                            
 * @dissaption      : DC motor driver board support package.                                           
***************************************************************************************************************/

/*  Indent using spaces
    Tab width: 4 spaces
    Line width: 100 characters
    Line ending: LF (0x0A) */

#ifndef _AHT20_H
#define _AHT20_H

/* Public macros ---------------------------------------------------------------------------------*/
/* Include ---------------------------------------------------------------------------------------*/

#ifdef __cplusplus
extern "C" {
#endif

/* Define ----------------------------------------------------------------------------------------*/
/* Private data types ----------------------------------------------------------------------------*/
/* Public data types -----------------------------------------------------------------------------*/

typedef struct {
    float hum;
    float temp;
} aht20_data_t;

/* Private function prototypes -------------------------------------------------------------------*/
/* Public function prototypes --------------------------------------------------------------------*/

void aht20_open(void);
void aht20_measure(void);
aht20_data_t aht20_read(void);
void aht20_reset(void);

#ifdef __cplusplus
}
#endif

#endif /* _AHT20_H */

/* END OF FILE ************************************************************************************/
