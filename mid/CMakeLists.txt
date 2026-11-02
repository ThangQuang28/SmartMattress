idf_component_register(
    SRCS
        "common/src/utility.c"
        "common/src/zerr.c"
        "ota/zota.c"
        "firebase/firebase.c"
        "smartconfig/smart_config.c"
        "wifi_prov/wifi_prov.c"
        "mqtt/mqtt_app.c"
        "btn/btn.c"
        "flash/flash.c"

        "iic_devices/drivers/tca9548/tca9548.c"
        "iic_devices/screen/oled/driver/esp_lcd_panel_sh1106.c"
        "iic_devices/screen/oled/oled_lvgl.c"
        "iic_devices/pressure/agr12/agr12.c"
        "iic_devices/pressure/pressure_array/pressure_array.c"
        "logic_ic/74hc595/74hc595.c"
        "logic_ic/valve/valve.c"
        "webui/devctl.c"
        "webui/webui.c"
        "webui/captive_dns.c"
        "pctl/pctl.c"
    INCLUDE_DIRS
        "common/inc"
        "btn"
        "firebase"
        "smartconfig"
        "wifi_prov"
        "mqtt"
        "flash"
        "ota"

        "iic_devices/drivers/tca9548"
        "iic_devices/screen/oled/driver"
        "iic_devices/screen/oled"
        "iic_devices/pressure/agr12"
        "iic_devices/pressure/pressure_array"
        "logic_ic/74hc595"
        "logic_ic/valve"
        "webui"
        "pctl"
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
        esp_lcd
        esp_http_server
        espressif__mdns
        esp_driver_uart
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
        # Web app served at http://192.168.4.1/ (same file as smart_mattress_chart/index.html).
        "../smart_mattress_chart/index.html"
)