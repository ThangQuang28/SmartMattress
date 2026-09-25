/***************************************************************************************************************
 * @auth            : duonghd                                                                                
 * @day             : 10-Mar-2026
 * @file            : dc_motor_bsp.h                                                                            
 * @dissaption      : DC motor driver board support package.                                           
***************************************************************************************************************/

/*  Indent using spaces
    Tab width: 4 spaces
    Line width: 100 characters
    Line ending: LF (0x0A) */

/*  Global configuration
    C standard library
    Applicationware
    wCat_FWCore V1.00 */

/* Includes --------------------------------------------------------------------------------------*/

/* Global configuration */
#include "zcfg.h"

/* BSP. */
#include "dc_motor_bsp.h"

/* ESP-IDF driver */
#include "driver/gpio.h"
#include "driver/ledc.h"

/* Define ----------------------------------------------------------------------------------------*/

#define MOTOR1_IN1 GPIO_NUM_19
#define MOTOR1_IN2 GPIO_NUM_17
#define MOTOR1_EN  GPIO_NUM_5

#define PWM_FREQ 5000
#define PWM_RES  LEDC_TIMER_8_BIT

/* Private data types ----------------------------------------------------------------------------*/

/* Public data types -----------------------------------------------------------------------------*/
/* Private function prototypes -------------------------------------------------------------------*/
/* Public function prototypes --------------------------------------------------------------------*/

/* Dc open.
 * @param[in]  type: type to be open.
 * @retval     None.
 * Note: This function should be called before using any other function of this module. 
 */
 void dc_motor_bsp_open(dc_motor_type_t type)
 {
    switch (type)
    {
    case DC_MOTOR_TYPE_1:
    {
        gpio_set_direction(MOTOR1_IN1, GPIO_MODE_OUTPUT);
        gpio_set_direction(MOTOR1_IN2, GPIO_MODE_OUTPUT);
        ledc_timer_config_t timer = {
            // .speed_mode = LEDC_LOW_SPEED_MODE,
            .timer_num = LEDC_TIMER_0,
            .duty_resolution = PWM_RES,
            .freq_hz = PWM_FREQ,
            .clk_cfg = LEDC_AUTO_CLK
        };
        ledc_timer_config(&timer);

        ledc_channel_config_t channel = {
            .gpio_num = MOTOR1_EN,
            // .speed_mode = LEDC_LOW_SPEED_MODE,
            .channel = LEDC_CHANNEL_0,
            .timer_sel = LEDC_TIMER_0,
            .duty = 0
        };
        ledc_channel_config(&channel);
    }
        break;
    case DC_MOTOR_TYPE_2:
    {   
    
    }
        break;
    default:
        break;
    }
 }

 /* Dc forward.
    * @param[in]  type: type to be forward.
    * @param[in]  speed: speed to be forward (0-100%).
    * @retval     None.
    * Note: This function should be called after dc_motor_bsp_open() function. 
    */
void dc_motor_bsp_forward(dc_motor_type_t type, uint8_t speed) 
{
    switch (type)
    {
    case DC_MOTOR_TYPE_1:
    {
        gpio_set_level(MOTOR1_IN1, 1);
        gpio_set_level(MOTOR1_IN2, 0);

        // ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, speed);
        // ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0);

    }
        break;
    case DC_MOTOR_TYPE_2:
    {
        
    }
        break;
    default:
        break;
    }     
}

/* Dc backward.
 * @param[in]  type: type to be backward.
 * @param[in]  speed: speed to be backward (0-100%).
 * @retval     None.
 * Note: This function should be called after dc_motor_bsp_open() function. 
 */
void dc_motor_bsp_backward(dc_motor_type_t type, uint8_t speed)
{
    switch (type)
    {
    case DC_MOTOR_TYPE_1:
    {
        gpio_set_level(MOTOR1_IN1, 0);
        gpio_set_level(MOTOR1_IN2, 1);

        // ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, speed);
        // ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0);

    }
        break;
    case DC_MOTOR_TYPE_2:
    {
        
    }
        break;
    default:
        break;
    }     
}

/* Dc stop.
 * @param[in]  type: type to be stop.
 * @retval     None.
 * Note: This function should be called after dc_motor_bsp_open() function. 
 */
void dc_motor_bsp_stop(dc_motor_type_t type) 
{
    switch (type)
    {
    case DC_MOTOR_TYPE_1:
    {
        gpio_set_level(MOTOR1_IN1, 0);
        gpio_set_level(MOTOR1_IN2, 0);
        // ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, 0);
        // ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0);

    }
        break;
    case DC_MOTOR_TYPE_2:
    {
        
    }
        break;
    default:
        break;
    }
}


/* END OF FILE ************************************************************************************/