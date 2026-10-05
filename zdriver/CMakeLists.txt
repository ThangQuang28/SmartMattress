idf_component_register(
    SRCS
        "zen/src/flash_bsp.c"
        "zen/src/dc_motor_bsp.c"
        "zen/src/iic_global_bsp.c"
        "zen/src/btn_bsp.c"
        "zen/src/esp_bsp.c"
    INCLUDE_DIRS
        "zen/inc"

    REQUIRES
        driver
        esp_driver_gpio 
        nvs_flash
        mid
)