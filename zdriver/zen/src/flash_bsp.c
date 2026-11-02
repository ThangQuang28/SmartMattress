/***************************************************************************************************************
 * @auth            : duonghd                                                                                
 * @day             : 10-Mar-2026
 * @file            : flash_bsp.c                                                                           
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

/** BSP. */
#include "flash_bsp.h"

/** ESP-IDF */
#include "nvs.h"
#include "nvs_flash.h"
#include "esp_err.h"

/* Define ----------------------------------------------------------------------------------------*/

/* Private data types ----------------------------------------------------------------------------*/

static bool g_flash_bsp_initialized = false;

/* Public data types -----------------------------------------------------------------------------*/

/* Private function prototypes -------------------------------------------------------------------*/

/* Public function prototypes --------------------------------------------------------------------*/

zerr_t flash_bsp_open(void)
{
    esp_err_t err;

    if (g_flash_bsp_initialized)
    {
        return ZERR_OPEN_ALREADY;
    }

    err = nvs_flash_init();

    /*
     * This can happen after changing the partition table
     * or when the NVS partition has no free pages.
     */
    if ((err == ESP_ERR_NVS_NO_FREE_PAGES) || (err == ESP_ERR_NVS_NEW_VERSION_FOUND))
    {
        err = nvs_flash_erase();

        if (err != ESP_OK)
        {
            return ZERR_NOT_OPEN;
        }

        err = nvs_flash_init();
    }

    if (err != ESP_OK)
    {
        return ZERR_NOT_OPEN; 
    }

    g_flash_bsp_initialized = true;

    return ZERR_OK;
}

zerr_t flash_bsp_write(const char *p_key, const void *p_data, size_t size)
{
    zerr_t ret = ZERR_OK;
    nvs_handle_t handle;
    esp_err_t err;

    if ((p_key == NULL) || (p_data == NULL) || (size == 0U))
    {
        return ZERR_INVALID_PARAM;
    }

    if (!g_flash_bsp_initialized)
    {
        return ZERR_NOT_OPEN;
    }

    err = nvs_open(FLASH_BSP_NAMESPACE, NVS_READWRITE, &handle);
    if (err != ESP_OK)
    {
        return ZERR_EXIST;
    }

    err = nvs_set_blob(handle, p_key, p_data, size);

    if (err == ESP_OK)
    {
        err = nvs_commit(handle);
    }

    nvs_close(handle);

    if (err != ESP_OK)
    {
        ret = (err == ESP_ERR_NVS_NOT_ENOUGH_SPACE) ? ZERR_STORAGE_OUT : ZERR_ERROR;
    }

    return ret;
}


zerr_t flash_bsp_read(const char *p_key, void *p_data, size_t size, size_t *p_read_size)
{
    nvs_handle_t handle;
    esp_err_t err;
    size_t required_size;

    if ((p_key == NULL) || (p_data == NULL) || (size == 0U) || (p_read_size == NULL))
    {
        return ZERR_INVALID_PARAM;
    }

    *p_read_size = 0U;

    if (!g_flash_bsp_initialized)
    {
        return ZERR_NOT_OPEN;
    }

    err = nvs_open(FLASH_BSP_NAMESPACE, NVS_READONLY, &handle);

    if (err != ESP_OK)
    {
        return ZERR_EXIST;
    }

    required_size = size;

    err = nvs_get_blob(handle, p_key, p_data, &required_size);

    nvs_close(handle);

    if (err != ESP_OK)
    {
        return ZERR_EXIST;
    }

    *p_read_size = required_size;

    return ZERR_OK;
}


zerr_t flash_bsp_get_size(const char *p_key, size_t *p_size)
{
    zerr_t ret = ZERR_OK;
    nvs_handle_t handle;
    esp_err_t err;

    if ((p_key == NULL) ||
        (p_size == NULL))
    {
        return ZERR_INVALID_PARAM;
    }

    *p_size = 0U;

    if (!g_flash_bsp_initialized)
    {
        return ZERR_NOT_OPEN;
    }

    err = nvs_open(FLASH_BSP_NAMESPACE, NVS_READONLY, &handle);

    if (err != ESP_OK)
    {
        return ZERR_EXIST;
    }

    err = nvs_get_blob(handle, p_key, NULL, p_size);

    nvs_close(handle);

    if (err != ESP_OK)
    {
        ret = ZERR_NOT_FOUND;
    }

    return ret;
}


zerr_t flash_bsp_delete(const char *p_key)
{
    zerr_t ret = ZERR_OK;
    nvs_handle_t handle;
    esp_err_t err;

    if (p_key == NULL)
    {
        return ZERR_INVALID_PARAM;
    }

    if (!g_flash_bsp_initialized)
    {
        return ZERR_NOT_OPEN;
    }

    err = nvs_open(FLASH_BSP_NAMESPACE, NVS_READWRITE, &handle);

    if (err != ESP_OK)
    {
        return ZERR_EXIST;
    }

    err = nvs_erase_key(handle, p_key);

    if (err == ESP_OK)
    {
        err = nvs_commit(handle);
    }

    nvs_close(handle);

    if (err != ESP_OK)
    {
        ret = (err == ESP_ERR_NVS_NOT_FOUND) ? ZERR_NOT_FOUND : ZERR_ERROR;
    }

    return ret;
}

/* Private function prototypes -------------------------------------------------------------------*/

/* END OF FILE ************************************************************************************/