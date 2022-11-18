//
// Created by Clyde Stubbs on 26/10/2022.
//

#include <esp_timer.h>
#include <math.h>
#include "ui.h"
#include "gdltask.h"
#include "riemann.h"

LV_IMG_DECLARE(ownship)
LV_IMG_DECLARE(north)
// list of target files
LV_IMG_DECLARE(target)


static float range = 40000.f;     // display range in meters. This display box will be 2*range wide and high
static int height, width;
static int xOffset, yOffset;
static bool isPortrait = false;
static lv_obj_t *targetImages[MAX_TRAFFIC_TRACKED];
static lv_obj_t *northImage;
static lv_obj_t *rangeLabel;
static lv_obj_t *trafficLabel;
static lv_style_t rangeStyle;
static lv_style_t arcStyle;

static float cosT, sinT;

static void mainSetup(lv_obj_t *tile) {

    lv_obj_clear_flag(tile, LV_OBJ_FLAG_SCROLLABLE);
    northImage = lv_img_create(tile);
    lv_obj_align(northImage, LV_ALIGN_CENTER, xOffset, yOffset - height/2);
    lv_img_set_src(northImage, &north);
    lv_obj_t *ownshipImage = lv_img_create(tile);
    lv_img_set_src(ownshipImage, &ownship);
    height = lv_obj_get_height(tile);
    width = lv_obj_get_width(tile);
    if (height > width) {
        isPortrait = true;      // TODO - not fully implemented.
        xOffset = 0;
        yOffset = -(height - width) / 2;
    } else {
        xOffset = -(width - height) / 2;
        yOffset = 0;
    }
    lv_obj_align(ownshipImage, LV_ALIGN_CENTER, (lv_coord_t) xOffset, (lv_coord_t) yOffset);
    for (int idx = 0; idx != MAX_TRAFFIC_TRACKED; idx++) {
        lv_obj_t * img = lv_img_create(tile);
        targetImages[idx] = img;
        lv_obj_align(img, LV_ALIGN_CENTER, 0, 0);
        lv_obj_add_flag(img, LV_OBJ_FLAG_HIDDEN);
    }
    rangeLabel = lv_label_create(tile);
    lv_style_set_text_font(&rangeStyle, &lv_font_montserrat_16);
    lv_style_set_bg_color(&rangeStyle, lv_color_black());
    lv_style_set_text_color(&rangeStyle, lv_color_white());
    lv_style_set_text_align(&rangeStyle, LV_TEXT_ALIGN_LEFT);
    lv_obj_add_style(rangeLabel, &rangeStyle, 0);
    lv_obj_align(rangeLabel, LV_ALIGN_TOP_LEFT, 0, 0);

    trafficLabel = lv_label_create(tile);
    lv_obj_add_style(trafficLabel, &rangeStyle, 0);
    lv_obj_align(trafficLabel, LV_ALIGN_BOTTOM_RIGHT, 0, 0);

    /*
    lv_style_init(&arcStyle);
    lv_style_set_arc_width(&arcStyle, 1);
    lv_style_set_bg_color(&arcStyle, lv_color_white());
    lv_obj_t * arc = lv_arc_create(tile);
    lv_obj_set_size(arc, height-2, height-2);
    lv_obj_add_style(arc, &arcStyle, LV_PART_MAIN);
    lv_obj_align(arc, LV_ALIGN_LEFT_MID, height/2, 0);
     */
    lv_obj_t *rangeArc = lv_spinner_create(tile, 1000, 60);
    lv_obj_set_size(rangeArc, (lv_coord_t) height, (lv_coord_t) height);
    lv_obj_align(rangeArc, LV_ALIGN_CENTER, (lv_coord_t) xOffset, (lv_coord_t) yOffset);
    lv_style_init(&arcStyle);
    lv_style_set_arc_width(&arcStyle, 1);
    lv_obj_add_style(rangeArc, &arcStyle, LV_PART_INDICATOR);
    lv_obj_add_style(rangeArc, &arcStyle, LV_PART_MAIN);
    lv_obj_set_style_arc_opa(rangeArc, LV_OPA_0, LV_PART_INDICATOR);
}

/**
 * translate an object relative to the display centre. The values xOffset, yOffset should be
 * set to the origin position of the display relative to its centre.
 * cosT and sinT should be set to the corresponding values for the rotation relative to north
 * @param obj   The object
 * @param east  Distance in pixels east/west of center
 * @param north  Distance in pixels north/west of center
 */
static void translate(lv_obj_t *obj, float north, float east) {
    lv_coord_t x = (lv_coord_t) (east * cosT + north * sinT + (float) xOffset);
    lv_coord_t y = (lv_coord_t) (-(int) (-east * sinT + north * cosT) + yOffset);
    lv_obj_set_pos(obj, x, y);
    lv_obj_align(obj, LV_ALIGN_CENTER, x, y);
}

static void mainUpdate() {
    char textBuf[512];
    int textLen;
    uint32_t oldMs = esp_timer_get_time() / 1000 - MAX_TRAFFIC_AGE_MS;
    range = 1000;
    int cnt = 0;
    TARGETS_FOREACH(tp) {
        if (tp->active && tp->timestampMs < oldMs)
            tp->active = false;
        if (tp->active && tp->distance > range) {
            range = tp->distance;
            if (++cnt == 4)
                break;
        }
    }
    range *= 1.1f;
    float scale = (float) height / range / 2;
    textLen = 0;
    textBuf[0] = 0;
    lv_label_set_text_fmt(rangeLabel, "%dkm", (int) (range / 1000));
    // precalculate rotation
    cosT = cosf(toRadians(-ourPosition.report.track));
    sinT = sinf(toRadians(-ourPosition.report.track));
    lv_img_set_angle(northImage, (int16_t) (-ourPosition.report.track * 10));
    if (isGpsConnected())
        lv_obj_clear_flag(northImage, LV_OBJ_FLAG_HIDDEN);
    else
        lv_obj_add_flag(northImage, LV_OBJ_FLAG_HIDDEN);
    translate(northImage, height / 2, 0);
    for (int idx = 0; idx != MAX_TRAFFIC_TRACKED; idx++) {
        lv_obj_t *ip = targetImages[idx];
        traffic_t *tp = traffic + idx;
        if (!tp->active) {
            lv_obj_add_flag(ip, LV_OBJ_FLAG_HIDDEN);
        } else {
            if (idx < 4) {
                textLen += sprintf(textBuf + textLen, "\n%.8s %.1f", tp->report.callsign, tp->distance / 1000);
            }
            lv_obj_clear_flag(ip, LV_OBJ_FLAG_HIDDEN);
            int angle = (int32_t) (tp->report.track - ourPosition.report.track) % 360;
            if (angle < 0)
                angle += 360;
            lv_img_set_angle(ip, (int16_t) (angle * 10));
            //lv_img_set_src(ip, targetList[angle]);
            //lv_obj_set_local_style_prop(ip, LV_STYLE_TRANSFORM_ANGLE, angle, LV_STATE_DEFAULT);
            lv_img_set_src(ip, &target);
            float east = (trafficEasting(&ourPosition.report, &tp->report) * scale);
            float north = (trafficNorthing(&ourPosition.report, &tp->report) * scale);
            translate(ip, north, east);
        }
    }
    lv_label_set_text(trafficLabel, textBuf);
}

static void mainTeardown() {
    memset(targetImages, 0, sizeof(targetImages));
}

const uiScreen_t uiMain = {
        .name = "Main screen",
        .setup = mainSetup,
        .update = mainUpdate,
        .refreshMs = 1000,
        .teardown = mainTeardown
};
