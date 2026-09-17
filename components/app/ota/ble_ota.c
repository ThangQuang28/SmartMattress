#include <stdio.h>
#include <string.h>
#include <stdbool.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "esp_log.h"
#include "esp_err.h"
#include "esp_ota_ops.h"
#include "esp_partition.h"
#include "esp_system.h"

#include "nimble/nimble_port.h"
#include "nimble/nimble_port_freertos.h"

#include "host/ble_hs.h"
#include "host/ble_hs_id.h"
#include "host/ble_gap.h"
#include "host/ble_gatt.h"
#include "host/ble_uuid.h"

#include "services/gap/ble_svc_gap.h"
#include "services/gatt/ble_svc_gatt.h"

#include "os/os_mbuf.h"

#include "ble_ota.h"

#define TAG "BLE_OTA"

/* ============================================================
 * DEVICE NAME
 * ============================================================ */
#define BLE_OTA_DEVICE_NAME "SB350-OTA_VER1"

/* ============================================================
 * UUID
 * ============================================================ */
#define BLE_OTA_SERVICE_UUID \
    0x9A, 0x4B, 0x20, 0x00, \
    0x4F, 0x62, \
    0x44, 0x75, \
    0x9B, 0x4D, \
    0x2C, 0x6F, \
    0x12, 0x34, \
    0x56, 0x78

#define BLE_OTA_CONTROL_UUID \
    0x01, 0x00, 0x81, 0x9D, \
    0x6F, 0x2C, \
    0x4D, 0x9B, \
    0x75, 0x44, \
    0x62, 0x4F, \
    0x01, 0x20, \
    0x4B, 0x9A

#define BLE_OTA_DATA_UUID \
    0x01, 0x00, 0x81, 0x9D, \
    0x6F, 0x2C, \
    0x4D, 0x9B, \
    0x75, 0x44, \
    0x62, 0x4F, \
    0x02, 0x20, \
    0x4B, 0x9A

static const ble_uuid128_t s_service_uuid = BLE_UUID128_INIT(BLE_OTA_SERVICE_UUID);
static const ble_uuid128_t s_control_uuid = BLE_UUID128_INIT(BLE_OTA_CONTROL_UUID);
static const ble_uuid128_t s_data_uuid    = BLE_UUID128_INIT(BLE_OTA_DATA_UUID);

/* ============================================================
 * COMMAND
 * ============================================================ */
#define BLE_OTA_CMD_START  0x01
#define BLE_OTA_CMD_FINISH 0x02
#define BLE_OTA_CMD_ABORT  0x03

/* ============================================================
 * OTA STATE
 * ============================================================ */
typedef enum {
    BLE_OTA_IDLE = 0,
    BLE_OTA_RECEIVING,
    BLE_OTA_COMPLETE,
    BLE_OTA_ERROR
} ble_ota_state_t;

/* ============================================================
 * OTA VARIABLES
 * ============================================================ */
static ble_ota_state_t s_state = BLE_OTA_IDLE;
static bool s_ota_unlocked = true;
static esp_ota_handle_t s_ota_handle = 0;
static const esp_partition_t *s_ota_partition = NULL;
static size_t s_expected_size = 0;
static size_t s_received_size = 0;
static uint16_t s_conn_handle = BLE_HS_CONN_HANDLE_NONE;

/* ============================================================
 * FORWARD DECLARATION
 * ============================================================ */
static int ble_ota_control_access(uint16_t conn_handle, uint16_t attr_handle, struct ble_gatt_access_ctxt *ctxt, void *arg);
static int ble_ota_data_access(uint16_t conn_handle, uint16_t attr_handle, struct ble_gatt_access_ctxt *ctxt, void *arg);
static int ble_ota_gap_event(struct ble_gap_event *event, void *arg);
static int ble_ota_start_image(uint8_t *command, uint16_t len);
static int ble_ota_finish_image(void);
static void ble_ota_abort(void);

/* ============================================================
 * GATT SERVICE
 * ============================================================ */
static const struct ble_gatt_svc_def s_gatt_services[] = {
    {
        .type = BLE_GATT_SVC_TYPE_PRIMARY,
        .uuid = &s_service_uuid.u,
        .characteristics = (struct ble_gatt_chr_def[]) {
            {
                .uuid = &s_control_uuid.u,
                .access_cb = ble_ota_control_access,
                .flags = BLE_GATT_CHR_F_WRITE | BLE_GATT_CHR_F_WRITE_NO_RSP
            },
            {
                .uuid = &s_data_uuid.u,
                .access_cb = ble_ota_data_access,
                .flags = BLE_GATT_CHR_F_WRITE | BLE_GATT_CHR_F_WRITE_NO_RSP
            },
            {
                0
            }
        }
    },
    {
        0
    }
};

/* ============================================================
 * ADVERTISING
 * ============================================================ */
static void ble_ota_advertise(void)
{
    struct ble_hs_adv_fields fields;
    struct ble_hs_adv_fields rsp_fields;
    uint8_t own_addr_type;
    int rc;

    /* 1. Gói Advertising chính (Chứa Flags và Tên thiết bị) */
    memset(&fields, 0, sizeof(fields));

    const char *name = BLE_OTA_DEVICE_NAME;
    fields.name = (uint8_t *)name;
    fields.name_len = strlen(name);
    fields.name_is_complete = 1;
    fields.flags = BLE_HS_ADV_F_DISC_GEN | BLE_HS_ADV_F_BREDR_UNSUP;

    rc = ble_gap_adv_set_fields(&fields);
    if (rc != 0) {
        ESP_LOGE(TAG, "ble_gap_adv_set_fields failed: %d", rc);
        return;
    }

    /* 2. Gói Scan Response (Chứa Service UUID 128-bit) */
    memset(&rsp_fields, 0, sizeof(rsp_fields));
    rsp_fields.uuids128 = (ble_uuid128_t *)&s_service_uuid;
    rsp_fields.num_uuids128 = 1;
    rsp_fields.uuids128_is_complete = 1;

    rc = ble_gap_adv_rsp_set_fields(&rsp_fields);
    if (rc != 0) {
        ESP_LOGE(TAG, "ble_gap_adv_rsp_set_fields failed: %d", rc);
        return;
    }

    /* 3. Lấy địa chỉ và Bắt đầu phát BLE */
    rc = ble_hs_id_infer_auto(0, &own_addr_type);
    if (rc != 0) {
        ESP_LOGE(TAG, "ble_hs_id_infer_auto failed: %d", rc);
        return;
    }

    struct ble_gap_adv_params adv_params;
    memset(&adv_params, 0, sizeof(adv_params));
    adv_params.conn_mode = BLE_GAP_CONN_MODE_UND;
    adv_params.disc_mode = BLE_GAP_DISC_MODE_GEN;

    rc = ble_gap_adv_start(own_addr_type, NULL, BLE_HS_FOREVER, &adv_params, ble_ota_gap_event, NULL);
    if (rc != 0) {
        ESP_LOGE(TAG, "ble_gap_adv_start failed: %d", rc);
        return;
    }

    ESP_LOGI(TAG, "BLE advertising started: %s", BLE_OTA_DEVICE_NAME);
}

/* ============================================================
 * GAP EVENT
 * ============================================================ */
static int ble_ota_gap_event(struct ble_gap_event *event, void *arg)
{
    (void)arg;

    switch (event->type) {
        case BLE_GAP_EVENT_CONNECT: {
            if (event->connect.status == 0) {
                s_conn_handle = event->connect.conn_handle;
                ESP_LOGI(TAG, "BLE connected, handle=%d", s_conn_handle);
            } else {
                ESP_LOGW(TAG, "BLE connection failed: status=%d", event->connect.status);
                ble_ota_advertise();
            }
            return 0;
        }

        case BLE_GAP_EVENT_DISCONNECT: {
            ESP_LOGW(TAG, "BLE disconnected: reason=%d", event->disconnect.reason);
            s_conn_handle = BLE_HS_CONN_HANDLE_NONE;

            if (s_state == BLE_OTA_RECEIVING) {
                ESP_LOGW(TAG, "Disconnect during OTA -> aborting");
                ble_ota_abort();
            }
            ble_ota_advertise();
            return 0;
        }

        case BLE_GAP_EVENT_MTU: {
            ESP_LOGI(TAG, "BLE MTU updated: conn=%d mtu=%d", event->mtu.conn_handle, event->mtu.value);
            return 0;
        }

        default:
            return 0;
    }
}

/* ============================================================
 * OTA STATUS
 * ============================================================ */
static void ble_ota_notify_status(void)
{
    // Cập nhật thông báo qua Notification nếu cần
}

/* ============================================================
 * OTA ABORT
 * ============================================================ */
static void ble_ota_abort(void)
{
    ESP_LOGW(TAG, "BLE OTA abort");
    if (s_ota_handle != 0) {
        esp_ota_abort(s_ota_handle);
        s_ota_handle = 0;
    }
    s_ota_partition = NULL;
    s_expected_size = 0;
    s_received_size = 0;
    s_state = BLE_OTA_IDLE;
    ble_ota_notify_status();
}

/* ============================================================
 * OTA START IMAGE
 * ============================================================ */
static int ble_ota_start_image(uint8_t *command, uint16_t len)
{
    if (!s_ota_unlocked) return BLE_ATT_ERR_INSUFFICIENT_AUTHOR;
    if (len != 5) return BLE_ATT_ERR_INVALID_ATTR_VALUE_LEN;

    if (s_state == BLE_OTA_RECEIVING) {
        ble_ota_abort();
    }

    uint32_t firmware_size = ((uint32_t)command[1]) |
                             ((uint32_t)command[2] << 8) |
                             ((uint32_t)command[3] << 16) |
                             ((uint32_t)command[4] << 24);

    if (firmware_size == 0) return BLE_ATT_ERR_INVALID_ATTR_VALUE_LEN;

    ESP_LOGI(TAG, "BLE OTA START: Expected size=%lu", (unsigned long)firmware_size);

    s_ota_partition = esp_ota_get_next_update_partition(NULL);
    if (s_ota_partition == NULL) {
        ESP_LOGE(TAG, "No OTA partition available");
        return BLE_ATT_ERR_UNLIKELY;
    }

    if (firmware_size > s_ota_partition->size) {
        ESP_LOGE(TAG, "Firmware too large: %lu > %lu", (unsigned long)firmware_size, (unsigned long)s_ota_partition->size);
        s_ota_partition = NULL;
        return BLE_ATT_ERR_INVALID_ATTR_VALUE_LEN;
    }

    // FIX CHÍNH: Sử dụng OTA_WITH_SEQUENTIAL_WRITES để tránh ESP32 bị treo xóa toàn bộ flash, gây mất kết nối BLE
    esp_err_t err = esp_ota_begin(s_ota_partition, OTA_WITH_SEQUENTIAL_WRITES, &s_ota_handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "esp_ota_begin failed: %s", esp_err_to_name(err));
        s_ota_handle = 0;
        s_ota_partition = NULL;
        return BLE_ATT_ERR_UNLIKELY;
    }

    s_expected_size = firmware_size;
    s_received_size = 0;
    s_state = BLE_OTA_RECEIVING;
    ble_ota_notify_status();

    return 0;
}

/* ============================================================
 * DATA
 * ============================================================ */
static int ble_ota_data_access(uint16_t conn_handle, uint16_t attr_handle, struct ble_gatt_access_ctxt *ctxt, void *arg)
{
    (void)conn_handle; (void)attr_handle; (void)arg;

    if (!s_ota_unlocked) return BLE_ATT_ERR_INSUFFICIENT_AUTHOR;
    if (ctxt->op != BLE_GATT_ACCESS_OP_WRITE_CHR) return BLE_ATT_ERR_UNLIKELY;
    if (s_state != BLE_OTA_RECEIVING) return BLE_ATT_ERR_UNLIKELY;

    uint16_t len = OS_MBUF_PKTLEN(ctxt->om);
    if (len == 0) return BLE_ATT_ERR_INVALID_ATTR_VALUE_LEN;

    size_t remaining = s_expected_size - s_received_size;
    if ((size_t)len > remaining) {
        ESP_LOGE(TAG, "OTA packet too large: len=%u remaining=%lu", len, (unsigned long)remaining);
        return BLE_ATT_ERR_INVALID_ATTR_VALUE_LEN;
    }

    // FIX: Tăng buffer lên 512 để hỗ trợ các thiết bị set MTU lớn hơn 247
    uint8_t data[512]; 
    if (len > sizeof(data)) {
        ESP_LOGE(TAG, "OTA packet exceeds local buffer: %u", len);
        return BLE_ATT_ERR_INVALID_ATTR_VALUE_LEN;
    }

    if (os_mbuf_copydata(ctxt->om, 0, len, data) != 0) {
        ESP_LOGE(TAG, "os_mbuf_copydata failed");
        return BLE_ATT_ERR_UNLIKELY;
    }

    esp_err_t err = esp_ota_write(s_ota_handle, data, len);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "esp_ota_write failed: %s", esp_err_to_name(err));
        esp_ota_abort(s_ota_handle);
        s_ota_handle = 0;
        s_state = BLE_OTA_ERROR;
        ble_ota_notify_status();
        return BLE_ATT_ERR_UNLIKELY;
    }

    s_received_size += len;

    static size_t last_log_size = 0;
    if (s_received_size - last_log_size >= (32 * 1024) || s_received_size == s_expected_size) {
        ESP_LOGI(TAG, "OTA progress: %lu/%lu", (unsigned long)s_received_size, (unsigned long)s_expected_size);
        last_log_size = s_received_size;
    }

    return 0;
}

/* ============================================================
 * OTA FINISH
 * ============================================================ */
static int ble_ota_finish_image(void)
{
    if (!s_ota_unlocked) return BLE_ATT_ERR_INSUFFICIENT_AUTHOR;
    
    if (s_state != BLE_OTA_RECEIVING) {
        ESP_LOGW(TAG, "OTA FINISH rejected: state=%d", s_state);
        return BLE_ATT_ERR_UNLIKELY;
    }

    if (s_received_size != s_expected_size) {
        ESP_LOGE(TAG, "OTA incomplete: %lu/%lu", (unsigned long)s_received_size, (unsigned long)s_expected_size);
        return BLE_ATT_ERR_UNLIKELY;
    }

    esp_err_t err = esp_ota_end(s_ota_handle);
    s_ota_handle = 0;

    if (err != ESP_OK) {
        ESP_LOGE(TAG, "esp_ota_end failed: %s", esp_err_to_name(err));
        s_state = BLE_OTA_ERROR;
        ble_ota_notify_status();
        return BLE_ATT_ERR_UNLIKELY;
    }

    err = esp_ota_set_boot_partition(s_ota_partition);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "esp_ota_set_boot_partition failed: %s", esp_err_to_name(err));
        s_state = BLE_OTA_ERROR;
        return BLE_ATT_ERR_UNLIKELY;
    }

    s_state = BLE_OTA_COMPLETE;
    ble_ota_notify_status();

    ESP_LOGI(TAG, "========================================");
    ESP_LOGI(TAG, "BLE OTA validated; restarting...");
    ESP_LOGI(TAG, "========================================");

    vTaskDelay(pdMS_TO_TICKS(500)); // Nghỉ một chút để gói tin BLE báo thành công kịp gửi đi
    esp_restart();

    return 0;
}

/* ============================================================
 * CONTROL
 * ============================================================ */
static int ble_ota_control_access(uint16_t conn_handle, uint16_t attr_handle, struct ble_gatt_access_ctxt *ctxt, void *arg)
{
    (void)conn_handle; (void)attr_handle; (void)arg;

    if (ctxt->op != BLE_GATT_ACCESS_OP_WRITE_CHR) return BLE_ATT_ERR_UNLIKELY;
    if (!s_ota_unlocked) return BLE_ATT_ERR_INSUFFICIENT_AUTHOR;

    uint16_t len = OS_MBUF_PKTLEN(ctxt->om);
    if (len == 0 || len > 5) return BLE_ATT_ERR_INVALID_ATTR_VALUE_LEN;

    uint8_t command[5];
    if (os_mbuf_copydata(ctxt->om, 0, len, command) != 0) {
        return BLE_ATT_ERR_INVALID_ATTR_VALUE_LEN;
    }

    switch (command[0]) {
        case BLE_OTA_CMD_START:
            if (len != 5) return BLE_ATT_ERR_INVALID_ATTR_VALUE_LEN;
            return ble_ota_start_image(command, len);

        case BLE_OTA_CMD_FINISH:
            if (len != 1) return BLE_ATT_ERR_INVALID_ATTR_VALUE_LEN;
            return ble_ota_finish_image();

        case BLE_OTA_CMD_ABORT:
            if (len != 1) return BLE_ATT_ERR_INVALID_ATTR_VALUE_LEN;
            ble_ota_abort();
            return 0;

        default:
            return BLE_ATT_ERR_UNLIKELY;
    }
}

/* ============================================================
 * BLE HOST SYNC
 * ============================================================ */
static void ble_ota_on_sync(void)
{
    // Đảm bảo địa chỉ đã được xác định trước khi quảng cáo
    ble_ota_advertise();
}

static void ble_ota_on_reset(int reason)
{
    ESP_LOGE(TAG, "BLE host reset; reason=%d", reason);
}

/* ============================================================
 * BLE HOST TASK
 * ============================================================ */
static void ble_ota_host_task(void *param)
{
    (void)param;
    ESP_LOGI(TAG, "NimBLE host task started");
    nimble_port_run();
    nimble_port_freertos_deinit();
}

/* ============================================================
 * PUBLIC START
 * ============================================================ */
esp_err_t ble_ota_start(void)
{
    int rc;

    ESP_LOGI(TAG, "Starting BLE OTA...");
    nimble_port_init();

    ble_svc_gap_init();
    ble_svc_gatt_init();
    ble_svc_gap_device_name_set(BLE_OTA_DEVICE_NAME);

    rc = ble_gatts_count_cfg(s_gatt_services);
    if (rc != 0) {
        ESP_LOGE(TAG, "ble_gatts_count_cfg failed: %d", rc);
        return ESP_FAIL;
    }

    rc = ble_gatts_add_svcs(s_gatt_services);
    if (rc != 0) {
        ESP_LOGE(TAG, "ble_gatts_add_svcs failed: %d", rc);
        return ESP_FAIL;
    }

    ble_hs_cfg.reset_cb = ble_ota_on_reset;
    ble_hs_cfg.sync_cb  = ble_ota_on_sync;

    nimble_port_freertos_init(ble_ota_host_task);

    ESP_LOGI(TAG, "BLE OTA initialized");
    return ESP_OK;
}