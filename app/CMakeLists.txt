idf_component_register(
    SRCS 
        "main.c"
        "zapp/zapp.c"
    INCLUDE_DIRS 
        "zapp"
    
    REQUIRES 
        nvs_flash
        mid
)