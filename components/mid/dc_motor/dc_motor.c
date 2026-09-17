/***************************************************************************************************************
 * @auth            : duonghd                                                                                
 * @day             : 10-Mar-2026
 * @file            : dc_motor.c                                                                            
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

/* Include ---------------------------------------------------------------------------------------*/
/* Define ----------------------------------------------------------------------------------------*/

/* Private data types ----------------------------------------------------------------------------*/

/* Public data types -----------------------------------------------------------------------------*/
/* Private function prototypes -------------------------------------------------------------------*/
/* Public function prototypes --------------------------------------------------------------------*/

/* Dc open.
 * @param[in]  None.
 * @retval     None.
 * Note: This function should be called before using any other function of this module. 
 */
void dc_motor_open(void)
{
    dc_motor_bsp_open(DC_MOTOR_TYPE_1);
    dc_motor_bsp_open(DC_MOTOR_TYPE_2);
}

/* Dc forward.
 * @param[in]  type: type to be forward.
 * @param[in]  speed: speed to be forward (0-100%).
 * @retval     None.
 * Note: This function should be called after dc_motor_open() function. 
 */
void dc_motor_forward(dc_motor_type_t type, uint8_t speed) {
    dc_motor_bsp_forward(type, speed);
}

/* Dc backward.
 * @param[in]  type: type to be backward.
 * @param[in]  speed: speed to be backward (0-100%).
 * @retval     None.
 * Note: This function should be called after dc_motor_open() function. 
 */
void dc_motor_backward(dc_motor_type_t type, uint8_t speed) {
    dc_motor_bsp_backward(type, speed);
}

/* Dc stop.
 * @param[in]  type: type to be stop.
 * @retval     None.
 * Note: This function should be called after dc_motor_open() function. 
 */
void dc_motor_stop(dc_motor_type_t type) {
    dc_motor_bsp_stop(type);
}

pkg_err_t dc_motor_handle(pkg_t *pkg)
{
    pkg_err_t err = PKG_ERR_OK;
    if (NULL == pkg)
    {
        err = PKG_ERR_NULL;
    }

    if ( 3 > pkg->len)
    {
        err = PKG_ERR_LEN;
    }

    if (PKG_ERR_OK == err)
    {
        uint8_t ctrl = pkg->data[0];
        uint8_t speed_m1 = pkg->data[1];
        uint8_t speed_m2 = pkg->data[2];

        /* Decode */
        uint8_t dir_m1 = (ctrl >> 0) & 0x01;
        uint8_t dir_m2 = (ctrl >> 1) & 0x01;

        uint8_t en_m1  = (ctrl >> 2) & 0x01;
        uint8_t en_m2  = (ctrl >> 3) & 0x01;

        uint8_t brk_m1 = (ctrl >> 4) & 0x01;
        uint8_t brk_m2 = (ctrl >> 5) & 0x01;


        /* MOTOR 1 */
        if(!en_m1)
        {
            // motor1_stop();
        }
        else if(brk_m1)
        {
            // motor1_brake();
        }
        else
        {
            if(dir_m1 == 0)
            {
                // motor1_forward(speed_m1);
            }
            else
            {
                // motor1_backward(speed_m1);
            }
        }

        /* MOTOR 2 */
        if(!en_m2)
        {
            // motor2_stop();
        }
        else if(brk_m2)
        {
            // motor2_brake();
        }
        else
        {
            if(dir_m2 == 0)
            {
                // motor2_forward(speed_m2);
            }
            else
            {
                // motor2_backward(speed_m2);
            }
        }


    } else
    {
        ESP_LOGI("DC_MOTOR ", "Error code : %d", err);
    }

    return err;   
}
/* END OF FILE ************************************************************************************/