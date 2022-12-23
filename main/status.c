//
// Created by Clyde Stubbs on 1/11/2022.
//

#include "status.h"
#include "ui.h"
#include "wifi_prov.h"
#include "gdltask.h"
#include "main.h"
#include "ownship.h"

LV_IMG_DECLARE(wifi_icon)
LV_IMG_DECLARE(gps_icon)
LV_IMG_DECLARE(traffic_icon)

static lv_style_t activeStyle, inActiveStyle;
static lv_color_t activeColor, inActiveColor;
static bool statusInited;

// the list of status icons to display
static struct {
    lv_obj_t *image;            // the image created at run time
    const lv_img_dsc_t *src;    // the source for the image
    bool (*enabled)();          // a function returning active status
} images[] = {
        {
                .src = &wifi_icon,
                .enabled = isWifiConnected
        },
        {
                .src = &gps_icon,
                .enabled = isGpsConnected
        },
        {
                .src = &traffic_icon,
                .enabled = isGdl90TrafficConnected
        },
};


#define TAG "status"

static void setState(lv_obj_t *image, bool enabled) {
    if (enabled)
        lv_obj_add_state(image, LV_STATE_USER_1);
    else
        lv_obj_clear_state(image, LV_STATE_USER_1);
}

/**
 * Update the status images
 */
void statusUpdate() {
    if (statusInited)
        for (int i = 0; i != ARRAY_SIZE(images); i++) {
            setState(images[i].image, images[i].enabled());
        }
}

/**
 *Initialise the status images.
 */
void statusInit() {
    activeColor = lv_color_make(0x30, 0x60, 0xFF);      // blue
    inActiveColor = lv_color_make(0x40, 0x40, 0x40);    // grey
    lv_obj_t *top = lv_layer_top();
    lv_obj_clean(top);

    lv_style_init(&activeStyle);
    lv_style_set_img_recolor(&activeStyle, activeColor);
    lv_style_set_img_recolor_opa(&activeStyle, LV_OPA_COVER);

    lv_style_init(&inActiveStyle);
    lv_style_set_img_recolor(&inActiveStyle, inActiveColor);
    lv_style_set_img_recolor_opa(&inActiveStyle, LV_OPA_COVER);

    for (int i = 0; i != ARRAY_SIZE(images); i++) {
        lv_obj_t *img = images[i].image = lv_img_create(top);
        lv_img_set_src(img, images[i].src);
        lv_obj_align(img, LV_ALIGN_TOP_RIGHT, (lv_coord_t) (-i * 19 - 1), 1);
        lv_obj_add_style(img, &inActiveStyle, LV_STATE_DEFAULT);
        lv_obj_add_style(img, &activeStyle, LV_STATE_USER_1);
    }
    statusInited = true;
    statusUpdate();
}