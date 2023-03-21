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
)

add_compile_definitions(
        LV_CONF_PATH=${CMAKE_CURRENT_SOURCE_DIR}/lv_conf.h
        LV_LVGL_H_INCLUDE_SIMPLE
)

