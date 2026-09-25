idf_component_register(
    SRCS
        "common/src/utility.c"
        "espnow/espnow.c"
        "espnow/package.c"
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
        "scr/scr.c"
        "scr/bitmaps.c"
        "btn/btn.c"
        "scr/oled_096/oled_096.c"
        "ztime/ztime.c"
        "zschedule/zschedule.c"
        "send_frame/fan_send.c"
    INCLUDE_DIRS
        "common/inc"
        "dc_motor"
        "temperature/aht20"
        "temperature/bmp280"
        "temperature/sht3x"
        "btn"
        "scr"
        "scr/oled_096"
        "espnow"
        "firebase"
        "smartconfig"
        "wifi_prov"
        "mqtt"
        "esp"
        "ztime"
        "zschedule"
        "send_frame"
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