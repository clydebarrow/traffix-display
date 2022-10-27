//
// Created by Clyde Stubbs on 26/10/2022.
//

#include "main_display.h"
#include "ui.h"

static lv_obj_t * mainLabel;
static lv_style_t titleStyle;

static void mainShow(lv_obj_t *tv) {
    mainLabel = lv_label_create(tv);
    lv_style_init(&titleStyle);
    lv_style_set_text_font(&titleStyle, &lv_font_montserrat_26);
    lv_style_set_bg_color(&titleStyle, lv_color_black());
    lv_style_set_text_color(&titleStyle, lv_color_make(0, 0xFF, 0xFF));
    lv_style_set_text_align(&titleStyle, LV_TEXT_ALIGN_CENTER);
    lv_obj_center(mainLabel);
    lv_obj_add_style(mainLabel, &titleStyle, 0);
    lv_label_set_text(mainLabel, "System Ready");
}

const uiScreen_t uiMain = {
        .name = "Main screen",
        .setup = mainShow,
        .update = NULL,
        .teardown = NULL
};
