idf_component_register(
    SRCS
        "app/zapp/zapp.c"
        "app/ota/ota.c"
        "app/ota/ble_ota.c"
        "mid/common/src/utility.c"
        "mid/espnow/espnow.c"
        "mid/espnow/package.c"
        "mid/firebase/firebase.c"
        "mid/smartconfig/smart_config.c"
        "mid/wifi_prov/wifi_prov.c"
        "mid/mqtt/mqtt_app.c"
        "mid/esp/esp_at.c"
        "mid/esp/esp_frame.c"
        "mid/dc_motor/dc_motor.c"
        "mid/temperature/aht20/aht20.c"
        "mid/temperature/bmp280/bmp280.c"
        "mid/temperature/sht3x/sht3x.c"
        "mid/scr/scr.c"
        "mid/scr/bitmaps.c"
        "mid/btn/btn.c"
        "mid/scr/oled_096/oled_096.c"
        "mid/ztime/ztime.c"
        "mid/zschedule/zschedule.c"
        "mid/send_frame/fan_send.c"
        "driver/zen/src/dc_motor_bsp.c"
        "driver/zen/src/iic_global_bsp.c"
        "driver/zen/src/btn_bsp.c"
        "driver/zen/src/esp_bsp.c"
    INCLUDE_DIRS
        "app/zapp"
        "app/ota"
        "mid/common/inc"
        "mid/dc_motor"
        "mid/temperature/aht20"
        "mid/temperature/bmp280"
        "mid/temperature/sht3x"
        "mid/btn"
        "mid/scr"
        "mid/scr/oled_096"
        "mid/espnow"
        "mid/firebase"
        "mid/smartconfig"
        "mid/wifi_prov"
        "mid/mqtt"
        "mid/esp"
        "mid/ztime"
        "mid/zschedule"
        "mid/send_frame"
        "driver/zen/inc"
    REQUIRES 
        driver
        esp_driver_gpio
        esp_event 
        esp_netif
        esp_wifi 
        wifi_provisioning
        esp_timer
        mqtt
        json
        # esp_adc
        # esp-tls
        # esp_wifi
        esp_http_client
        esp_https_ota
        mbedtls
        bt
    PRIV_REQUIRES 
        nvs_flash
        app_update
    EMBED_TXTFILES 
        # firebase_cert.pem
        # "mid/mqtt/certs/oxi_client.crt"
        # "mid/mqtt/certs/emqx_ca.crt"
        # "mid/mqtt/certs/oxi_client.key"
        # "mid/mqtt/certs/oxi_mqtt_eclipse_org.pem"      
        "mid/mqtt/certs/device.crt"
        "mid/mqtt/certs/device.key"
        "mid/mqtt/certs/rootca.pem"      
)
