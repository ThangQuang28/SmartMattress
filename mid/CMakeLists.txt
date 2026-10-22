idf_component_register(
    SRCS
        "common/src/utility.c"
        "common/src/zerr.c"
        "espnow/espnow.c"
        "espnow/package.c"
        "ota/zota.c"
        "firebase/firebase.c"
        "smartconfig/smart_config.c"
        "wifi_prov/wifi_prov.c"
        "mqtt/mqtt_app.c"
        "esp/esp_at.c"
        "esp/esp_frame.c"
        "dc_motor/dc_motor.c"
        "temperature/aht20/aht20.c"
        "temperature/bmp280/bmp280.c"
        "temperature/sht3x/sht3x.c"
        "btn/btn.c"
        "flash/flash.c"
        "ztime/ztime.c"
        "zschedule/zschedule.c"
        "send_frame/fan_send.c"

        "iic_devices/drivers/tca9548/tca9548.c"
        "iic_devices/screen/oled/driver/esp_lcd_panel_sh1106.c"
        "iic_devices/screen/oled/oled_lvgl.c"
        "iic_devices/pressure/agr12/agr12.c"
        "logic_ic/74hc595/74hc595.c"
    INCLUDE_DIRS
        "common/inc"
        "dc_motor"
        "temperature/aht20"
        "temperature/bmp280"
        "temperature/sht3x"
        "btn"
        "espnow"
        "firebase"
        "smartconfig"
        "wifi_prov"
        "mqtt"
        "esp"
        "flash"
        "ota"
        "ztime"
        "zschedule"
        "send_frame"

        "iic_devices/drivers/tca9548"
        "iic_devices/screen/oled/driver"
        "iic_devices/screen/oled"
        "iic_devices/pressure/agr12"
        "logic_ic/74hc595"
    REQUIRES 
        esp_driver_gpio
        esp_event 
        esp_netif
        esp_wifi 
        wifi_provisioning
        esp_timer
        mqtt
        json
        log
        # esp_adc
        # esp-tls
        # esp_wifi
        esp_http_client
        esp_https_ota
        zdriver
    PRIV_REQUIRES 
        nvs_flash
        app_update

    EMBED_TXTFILES 
        # firebase_cert.pem
        # "mqtt/certs/oxi_client.crt"
        # "mqtt/certs/emqx_ca.crt"
        # "mqtt/certs/oxi_client.key"
        # "mqtt/certs/oxi_mqtt_eclipse_org.pem"      
        "mqtt/certs/device.crt"
        "mqtt/certs/device.key"
        "mqtt/certs/rootca.pem"      
)