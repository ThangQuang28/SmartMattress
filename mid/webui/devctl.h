/***************************************************************************************************************
 * @auth            : duonghd
 * @file            : devctl.h
 * @description     : Device state shared by the sensor task, the web server and the USB serial link:
 *                    latest pressures, zero offsets, valve outputs, and the JSON commands that change them.
***************************************************************************************************************/

/*  Indent using spaces
    Tab width: 4 spaces
    Line width: 100 characters
    Line ending: LF (0x0A) */

#ifndef DEVCTL_H
#define DEVCTL_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes --------------------------------------------------------------------------------------*/
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include "zerr.h"
#include "pressure_array.h"

/* Definitions -----------------------------------------------------------------------------------*/

/* Valve outputs on 4 x 74HC595 (DS 23 / SHCP 18 / STCP 5, see valve.c). Set to 0 to lock them:
   the web page still shows the panel but every valve command is refused and no GPIO is driven. */
#define DEVCTL_VALVE_ENABLE         1

/* Manual commands stay latched until explicitly changed. Set nonzero to enable idle timeout. */
#define DEVCTL_VALVE_IDLE_OFF_MS    0U

#define DEVCTL_JSON_MAX             2048U

/* Named pressure sets (user 28.10): saved after P3 is reached, read with GET /api/cfgs. */
#define DEVCTL_CFG_NUM              8U
#define DEVCTL_CFG_NAME_LEN         40U          /* Bytes incl. end, UTF-8. */
#define DEVCTL_CFG_JSON_MAX         1600U

/* Public functions ------------------------------------------------------------------------------*/

/**
 * @brief  Load zero offsets from NVS and, if enabled, open the valve driver (all off).
 */
zerr_t devctl_open(void);

/**
 * @brief  Time between two sensor scans, set from the web page (200..10000 ms).
 */
uint32_t devctl_period_ms(void);

/**
 * @brief  Store one scan. p_raw are the sensor readings; p_out receives them minus the zero offsets.
 */
void devctl_update(const float *p_raw, uint32_t ok_mask, float *p_out);

/**
 * @brief  Note that a web client is connected (state request or command).
 */
void devctl_touch(void);

/**
 * @brief  Called after every scan: switches all valves off when no client was seen for
 *         DEVCTL_VALVE_IDLE_OFF_MS.
 */
void devctl_tick(void);

/**
 * @brief  Write the current state as one JSON object (no newline). Returns the length.
 */
size_t devctl_state_json(char *p_buf, size_t size);

/**
 * @brief  Run one JSON command, e.g. {"cmd":"tare"}. Writes a JSON reply {"ok":..,"msg":..}.
 * @retval ZERR_OK when the command was accepted.
 */
zerr_t devctl_command(const char *p_json, char *p_reply, size_t reply_size);

/**
 * @brief  Write the named pressure sets as {"cfgs":[{"slot":n,"name":"..","kpa":[13]}],"max":8}.
 */
size_t devctl_cfgs_json(char *p_buf, size_t size);

/**
 * @brief  Start the task that reads "@cmd {json}" lines from the USB serial port and answers with
 *         "@J {json}" lines. The sensor task also prints "@J {state}" after every scan.
 */
zerr_t devctl_serial_open(void);

/**
 * @brief  Print the state as one "@J {json}" line on the USB serial port.
 */
void devctl_serial_push_state(void);

#ifdef __cplusplus
}
#endif

#endif /* DEVCTL_H */

/* END OF FILE ************************************************************************************/
