# The following lines of boilerplate have to be in your project's CMakeLists
# in this exact order for cmake to work correctly
cmake_minimum_required(VERSION 3.22)

set(EXTRA_COMPONENT_DIRS
    ../app
    ../mid
    ../zdriver
)

# Only build components if need.
set(COMPONENTS
    app
    freertos
    esp_system
    esp_timer
    log
    
    esp_event
    esp_netif
    esp_wifi
    wifi_provisioning
    
    nvs_flash
    mqtt
    lwip
    json
    esp_http_client
    esp_http_server
    esp_https_ota
    app_update
)

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
# "Trim" the build. Include the minimal set of components, main, and anything it depends on.
project(zenFWCore)