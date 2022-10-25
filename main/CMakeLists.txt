file(GLOB_RECURSE IMAGE_SOURCES images/*.c)
file(GLOB MAIN_SOURCES *.c)


idf_component_register(SRCS ${IMAGE_SOURCES} ${MAIN_SOURCES}
        INCLUDE_DIRS
        "."
        "../managed_components/lvgl__lvgl"
    )

add_compile_definitions(
        LV_CONF_PATH=${CMAKE_CURRENT_SOURCE_DIR}/lv_conf.h
        LV_LVGL_H_INCLUDE_SIMPLE
)
