file(GLOB_RECURSE IMAGE_SOURCES images/*.c)
file(GLOB MAIN_SOURCES *.c)



idf_component_register(
        REQUIRES
            lvgl gdl90 esp_websocket_client
            json
            esp_common
            nvs_flash
            esp_event
            esp_hw_support
            esp_netif
            esp_partition
            esp_psram
            esp_ringbuf
            esp_rom
            esp_system
            esp_timer
            esp_wifi
            esp_lcd
            driver
            protocomm
            wifi_provisioning

        SRCS ${IMAGE_SOURCES} ${MAIN_SOURCES}
        INCLUDE_DIRS "." )
            #"${IDF_PATH}/components/json/cJSON"
            #"${IDF_PATH}/components/esp_common/include"
            #"${IDF_PATH}/components/nvs_flash/include"
            #"${IDF_PATH}/components/esp_event/include"
            #"${IDF_PATH}/components/esp_hw_support/include"
            #"${IDF_PATH}/components/esp_netif/include"
            #"${IDF_PATH}/components/esp_partition/include"
            #"${IDF_PATH}/components/esp_psram/include"
            #"${IDF_PATH}/components/esp_ringbuf/include"
            #"${IDF_PATH}/components/esp_rom/include"
            #"${IDF_PATH}/components/esp_system/include"
            #"${IDF_PATH}/components/esp_timer/include"
            #"${IDF_PATH}/components/esp_wifi/include"
            #"${IDF_PATH}/components/esp_lcd/include"
            #"${IDF_PATH}/components/driver/include"
            #"${IDF_PATH}/components/protocomm/include/common"
            #"${IDF_PATH}/components/protocomm/include/security"
            #"${IDF_PATH}/components/protocomm/include/transports"
            #"${IDF_PATH}/components/wifi_provisioning/include"
            #"../../traffix-display/components/gdl90/include"

add_compile_definitions(
        LV_CONF_PATH=${CMAKE_CURRENT_SOURCE_DIR}/lv_conf.h
        LV_LVGL_H_INCLUDE_SIMPLE
)

