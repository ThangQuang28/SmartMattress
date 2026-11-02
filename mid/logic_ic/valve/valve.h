/***************************************************************************************************************
 * @auth            : duonghd
 * @file            : valve.h
 * @description     : Valve outputs on a chain of 4 x 74HC595 (32 outputs, Q0 of the first IC = output 0).
***************************************************************************************************************/

/*  Indent using spaces
    Tab width: 4 spaces
    Line width: 100 characters
    Line ending: LF (0x0A) */

#ifndef VALVE_H
#define VALVE_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes --------------------------------------------------------------------------------------*/
#include <stdbool.h>
#include <stdint.h>
#include "zerr.h"

/* Definitions -----------------------------------------------------------------------------------*/
#define VALVE_IC_NUM                4U
#define VALVE_OUTPUT_NUM            (VALVE_IC_NUM * 8U)

/* Public functions ------------------------------------------------------------------------------*/

/**
 * @brief  Configure the 74HC595 pins and switch every output off.
 */
zerr_t valve_open(void);

/**
 * @brief  Switch one output (0..VALVE_OUTPUT_NUM-1) on or off.
 */
zerr_t valve_set(uint8_t index, bool on);

/**
 * @brief  Write all 32 outputs at once, bit n = output n.
 */
zerr_t valve_set_mask(uint32_t mask);

/**
 * @brief  Switch every output off.
 */
zerr_t valve_all_off(void);

/**
 * @brief  Move the chain to other GPIOs (DS, SHCP, STCP), all outputs off, saved in NVS.
 */
zerr_t valve_set_pins(uint8_t ds, uint8_t shcp, uint8_t stcp);

/**
 * @brief  Current pins: p_pins[0] = DS, [1] = SHCP, [2] = STCP.
 */
void valve_get_pins(uint8_t *p_pins);

/**
 * @brief  Current output state, bit n = output n.
 */
uint32_t valve_get_mask(void);

/**
 * @brief  Self test: turn each output on alone for on_ms, one after the other, then all off.
 *         Blocks for about VALVE_OUTPUT_NUM * on_ms.
 */
zerr_t valve_step_test(uint32_t on_ms);

#ifdef __cplusplus
}
#endif

#endif
/* END OF FILE ************************************************************************************/
