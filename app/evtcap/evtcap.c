/**
  **************************************************************************************************
  * @file   evtcap.c
  * @author duonghd
  * @date   22-Step-2026
  * @brief
  **************************************************************************************************
  */

/*  Indent using spaces
    Tab width: 4 spaces
    Line width: 100 characters
    Line ending: LF (0x0A) */

/* Includes --------------------------------------------------------------------------------------*/

/* Global configuration. */
#include "zcfg.h"

/** Application */
#include "evtcap.h"

/** Middle */
#include "zscript.h"
#include "flash.h"
#include "evtproc.h"
#include "firebase.h"
#include "cJSON.h"

/* Private macros --------------------------------------------------------------------------------*/

#define EVT_NUM_STEP_CB                 1
#define EVT_NUM_DATA_CB                 2

#define EVT_MAX_SCRIPT                  1024
#define EVT_SCRIPT_NAME                 "script"
#define MAX_SCRIPT_NUM_TOKEN            1400

/* Private data types ----------------------------------------------------------------------------*/

/* Private variables -----------------------------------------------------------------------------*/

static SemaphoreHandle_t g_sem_bin_http = 0; /* Wait for init http offline. */

static char gp_jscript[EVT_MAX_SCRIPT] = {0};

static size_t gp_jscript_size = 0;
static cJSON* gp_zscript = 0;
static char* current_step = 0;

static zerr_t step_proc(zscript_cb_param_t* p_param);

static zscript_cb_map_t gp_step_cb_map[EVT_NUM_STEP_CB] = 
{
    {"*",        step_proc },
};

static zscript_cb_map_t gp_data_cb_map[EVT_NUM_DATA_CB] = {
    {   0,              event_proc_eval         },
    {   "lcd",          event_proc_lcd          },
};
/* Private function prototypes -------------------------------------------------------------------*/

static void firebase_data_callback(const char* p_data, uint32_t data_len);
static void event_capture_loop(void* pParam);


/* Public function bodies ------------------------------------------------------------------------*/
zerr_t eventcap_open(void)
{
    zerr_t ret = ZERR_OK;

    zvar_init();

    g_sem_bin_http = xSemaphoreCreateBinary();

    // firebase_set_reg(firebase_data_callback);
    // firebase_get_data();

    // xSemaphoreTake(g_sem_bin_http, portMAX_DELAY);

    if (ZERR_OK == ret)
    {
        ret = flash_read(EVT_SCRIPT_NAME, gp_jscript, sizeof(gp_jscript), &gp_jscript_size);
        if(ZERR_OK == ret)
        {
            ZTRACE_DBG(TR_EVTCAP, ("DEBUG %s: Script: %.*s ", __FUNCTION__, gp_jscript_size, gp_jscript));
        }
    }
    if(ZERR_OK == ret)
    {
        ZTRACE_DBG(TR_EVTCAP, ("DEBUG %s: Loading script from flash ", __FUNCTION__));

        ret = load_scripts_flash(&gp_zscript);
    }
    if (ZERR_OK == ret)
    {
        if (pdPASS != xTaskCreate(event_capture_loop,
                                "event_capture",
                                ZAPP_STACK_DEEP_EVTCAP,
                                NULL,
                                ZAPP_PRIO_EVTCAP,
                                NULL))
        {
            ret =  ZERR_MEM_OUT;
        }
    }
    return ret;
}

zerr_t open_http_offline(void)
{
    xSemaphoreGive(g_sem_bin_http);
    return ZERR_OK;
}

/* Private function bodies -----------------------------------------------------------------------*/

void event_capture_loop(void* pParam)
{
    zscript_cb_param_t param;
    param.p_cb_data = 0;

    xSemaphoreTake(g_sem_bin_http, portMAX_DELAY);

    for (;;)
    {
        param.p_value = gp_zscript;
        param.p_key = 0;

        zscript_process_value(&param, gp_step_cb_map , EVT_NUM_STEP_CB);

        //ESP_LOGI(TAG, "%s: running..", __FUNCTION__);
        vTaskDelay(500 / portTICK_PERIOD_MS);
    }
}

zerr_t load_scripts_flash(cJSON** pp_zscript)
{
    cJSON* p_root;

    if (NULL == pp_zscript)
    {
        return ZERR_ERROR;
    }

    *pp_zscript = NULL;

    p_root = cJSON_ParseWithLength(gp_jscript, gp_jscript_size);

    if (NULL == p_root)
    {
        ZTRACE_DBG(TR_EVTCAP, ("DEBUG %s: Job not found! JSON syntax error at: %s", __FUNCTION__, cJSON_GetErrorPtr()));
        return ZERR_ERROR;
    }

    if (!cJSON_IsObject(p_root))
    {
        ZTRACE_DBG(TR_EVTCAP, ("Job not found! Root is not JSON object"));
        cJSON_Delete(p_root);
        return ZERR_ERROR;
    }

    *pp_zscript = p_root;

    ZTRACE_DBG(TR_EVTCAP, ("DEBUG %s: JSON parsed successfully", __FUNCTION__));
    
    return ZERR_OK;
}

static zerr_t step_proc(zscript_cb_param_t* p_param)
{
    zerr_t ret = ZERR_OK;
    if ((NULL == p_param) || (NULL == p_param->p_key) || (NULL == p_param->p_value))
    {
        return ZERR_ERROR;
    }

    ZTRACE_DBG(TR_EVTCAP, ("DEBUG %s: step = %s", __FUNCTION__, p_param->p_key->string));
    
    current_step = p_param->p_key->string;


    if (cJSON_IsObject(p_param->p_value))
    {
        zscript_cb_param_t param;

        memset(&param, 0, sizeof(param));

        param.p_value = p_param->p_value;
        param.p_cb_data = p_param->p_cb_data;

        ret = zscript_process_value(&param, gp_data_cb_map, EVT_NUM_DATA_CB);
    }
    
    return ret;
}

static void firebase_data_callback(const char* p_data, uint32_t data_len)
{
    zerr_t ret = ZERR_OK;

    cJSON *p_root = NULL;
    cJSON *p_script = NULL;

    const char *p_script_data;
    uint32_t script_len;

    if ((NULL == p_data) || (0U == data_len))
    {
        ret = ZERR_INVALID_PARAM;
    }

    /* p_data is already a complete Firebase response. */
    p_root = cJSON_ParseWithLength(p_data, data_len);

    if (NULL == p_root)
    {
        ZTRACE_DBG(TR_EVTCAP, ("DEBUG %s: cJSON parse failed", __FUNCTION__));
        ret = ZERR_INVALID_PARAM;
    }

    /*
     * Get:
     * {
     *     "info": "...",
     *     "script": "...",
     *     "status": false
     * }
     *             root["script"]
     */
    p_script = cJSON_GetObjectItemCaseSensitive(p_root, "script");
    if (NULL == p_script)
    {
        ZTRACE_DBG(TR_EVTCAP, ("DEBUG %s: script not found", __FUNCTION__));
        ret = ZERR_INVALID_PARAM;
    }

    if (!cJSON_IsString(p_script))
    {
        ZTRACE_DBG(TR_EVTCAP, ("DEBUG %s: script is not string", __FUNCTION__));
       ret = ZERR_INVALID_PARAM;
    }
    
    p_script_data = cJSON_GetStringValue(p_script);
    if (NULL == p_script_data)
    {
        ret = ZERR_INVALID_PARAM;
    }

    script_len = (uint32_t)strlen(p_script_data);

    ZTRACE_DBG(TR_EVTCAP, ("DEBUG %s: script len = %u", __FUNCTION__, (unsigned int)script_len));
    ZTRACE_DBG(TR_EVTCAP, ("DEBUG %s: script = %s", __FUNCTION__, p_script_data));

    /*
     * Write only the script string to flash.
     */
    ret = flash_write("script", p_script_data, script_len + 1U);
    ZTRACE_DBG(TR_EVTCAP, ("DEBUG %s: flash_write ret = %d, size = %u", __FUNCTION__, ret, (unsigned int)(script_len + 1U)));

    if (ZERR_OK == ret)
    {
        ZTRACE_DBG(TR_EVTCAP, ("DEBUG %s: SON: %s\r\n", __FUNCTION__, p_data));
        xSemaphoreGive(g_sem_bin_http);
    }
}   
/* END OF FILE ************************************************************************************/