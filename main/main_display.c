//
// Created by Clyde Stubbs on 26/10/2022.
//

#include <esp_timer.h>
#include <math.h>
#include "main_display.h"
#include "ui.h"
#include "gdltask.h"
#include "riemann.h"

LV_IMG_DECLARE(ownship)
LV_IMG_DECLARE(target)
LV_IMG_DECLARE(target_filled)


static float range = 40000.f;     // display range in meters. This display box will be 2*range wide and high
static int height;
static lv_obj_t *targetImages[MAX_TRAFFIC_TRACKED];

static void mainSetup(lv_obj_t *tile) {
    lv_obj_t *ownshipImage;
    ownshipImage = lv_img_create(tile);
    lv_img_set_src(ownshipImage, &ownship);
    lv_obj_align(ownshipImage, LV_ALIGN_CENTER, 0, 0);
    height = lv_obj_get_width(tile);
    for (int idx = 0; idx != MAX_TRAFFIC_TRACKED; idx++) {
        targetImages[idx] = lv_img_create(tile);
    }
}

static void mainUpdate() {
    float scale = (float) height / range / 2;
    uint32_t oldMs = esp_timer_get_time() / 1000 - MAX_TRAFFIC_AGE_MS;
    for (int idx = 0; idx != MAX_TRAFFIC_TRACKED; idx++) {
        lv_obj_t *ip = targetImages[idx];
        traffic_t *tp = traffic + idx;
        if (tp->active && tp->timestamp < oldMs)
            tp->active = false;
        lv_style_value_t angle;
        if (!tp->active) {
            lv_obj_add_flag(ip, LV_OBJ_FLAG_HIDDEN);
        } else {
            lv_obj_clear_flag(ip, LV_OBJ_FLAG_HIDDEN);
            lv_img_set_src(ip, &target);
            angle.num = (int32_t) ((tp->report.track - ourPosition.report.track) * 10.0f);
            if (angle.num < 0)
                angle.num += 3600;
            lv_obj_set_local_style_prop(ip, LV_STYLE_TRANSFORM_ANGLE, angle, LV_STATE_DEFAULT);
            lv_coord_t east = (lv_coord_t) (trafficEasting(&ourPosition.report, &tp->report) * scale);
            lv_coord_t south = (lv_coord_t) (-trafficNorthing(&ourPosition.report, &tp->report) * scale);
            lv_obj_align(ip, LV_ALIGN_CENTER, east, south);
        }
    }
}

static void mainTeardown() {
    memset(targetImages, 0, sizeof(targetImages));
}

const uiScreen_t uiMain = {
        .name = "Main screen",
        .setup = mainSetup,
        .update = mainUpdate,
        .teardown = mainTeardown
};
