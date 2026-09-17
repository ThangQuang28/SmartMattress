#ifndef BLE_OTA_H
#define BLE_OTA_H

#include "esp_err.h"

/* Starts the bonded BLE OTA GATT server and advertising. */
esp_err_t ble_ota_start(void);


#endif /* BLE_OTA_H */
