//
// Created by Clyde Stubbs on 26/10/2022.
//

#include <esp_timer.h>
#include <math.h>
#include "ui.h"
#include "gdltask.h"
#include "riemann.h"
#include "main.h"
#include "units.h"
#include "ownship.h"

LV_IMG_DECLARE(ownship)
LV_IMG_DECLARE(north)
LV_IMG_DECLARE(target)



static const char *const TAG = "main_display";
static float range = 40000.f;     // display range in meters. This display box will be 2*range wide and high
static lv_coord_t height, width;
static lv_coord_t xOffset, yOffset;
static bool isPortrait = false;
typedef struct {
    lv_obj_t *image;        // target image
    lv_obj_t *label;        // the label
} target_t;
target_t targets[MAX_TARGETS_SHOWN];
static lv_obj_t *northImage;
static lv_obj_t *rangeLabel;
static lv_obj_t *trafficLabel;
static lv_style_t rangeStyle;
static lv_style_t labelStyle;
static lv_style_t arcStyle;

static float cosT, sinT;

static void mainSetup(lv_obj_t *tile) {

    lv_obj_clear_flag(tile, LV_OBJ_FLAG_SCROLLABLE);
    northImage = lv_img_create(tile);
    lv_obj_align(northImage, LV_ALIGN_CENTER, xOffset, yOffset - height / 2);
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
    lv_style_init(&labelStyle);
    lv_style_set_text_font(&labelStyle, &lv_font_montserrat_12);
    lv_style_set_bg_color(&labelStyle, lv_color_black());
    lv_style_set_text_color(&labelStyle, lv_color_white());
    lv_style_set_text_align(&labelStyle, LV_TEXT_ALIGN_LEFT);
    lv_style_set_align(&labelStyle, LV_ALIGN_LEFT_MID);
    FOREACH(targets, tp, target_t) {
        tp->image = lv_img_create(tile);
        lv_obj_set_align(tp->image, LV_ALIGN_CENTER);
        lv_obj_add_flag(tp->image, LV_OBJ_FLAG_HIDDEN);
        tp->label = lv_label_create(tile);
        lv_obj_add_style(tp->label, &labelStyle, 0);
        lv_obj_set_align(tp->image, LV_ALIGN_CENTER);
        lv_obj_add_flag(tp->label, LV_OBJ_FLAG_HIDDEN);
    }
    rangeLabel = lv_label_create(tile);
    lv_obj_align(rangeLabel, LV_ALIGN_TOP_LEFT, 0, 0);
    lv_style_init(&rangeStyle);
    ESP_LOGI(TAG, "Range style inited");
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
 * cosT and sinT should be set to the corresponding values for the rotation relative to deltaNorth
 * @param obj   The object
 * @param deltaEast  Distance in pixels deltaEast/west of center
 * @param deltaNorth  Distance in pixels deltaNorth/west of center
 */
static void translate(lv_obj_t *obj, float deltaNorth, float deltaEast) {
    lv_coord_t x = (lv_coord_t) (deltaEast * cosT + deltaNorth * sinT + (float) xOffset);
    lv_coord_t y = (lv_coord_t) (-(int) (-deltaEast * sinT + deltaNorth * cosT) + yOffset);
    lv_obj_set_pos(obj, x, y);
}


static void mainUpdate() {
    char textBuf[512];
    traffic_t traffic[MAX_TARGETS_SHOWN];
    int textLen;
    range = 1000;
    ownship_t ourPosition;
    bool posValid = getOwnshipPosition(&ourPosition);
    getTraffic(traffic, MAX_TARGETS_SHOWN);
    // get the range of the most distant of the first 4 targets
    // we assume they are sorted by ascending distance
    size_t tcnt = 0;
    for (traffic_t *ptr = traffic + MAX_TARGETS_SHOWN; ptr-- != traffic;)
        if (ptr->active) {
            range = ptr->distance;
            tcnt = ptr - traffic + 1;
            break;
        }
    range *= 1.1f;
    // convert to a whole number of nm or km as appropriate
    int rangeInUnits = (int) ceilf(convertToUserUnit(range, &prefDistanceUnit));
    range = convertFromUserUnit((float) rangeInUnits, &prefDistanceUnit);
    float scale = (float) height / range / 2;
    textLen = 0;
    textBuf[0] = 0;
    ESP_LOGI(TAG, "mainUpdate - traffic sorted, active %d, range %d", tcnt, rangeInUnits);
    lv_label_set_text_fmt(rangeLabel, "%d%s", rangeInUnits, userUnitName(&prefDistanceUnit));
    if (!posValid) {
        lv_obj_add_flag(northImage, LV_OBJ_FLAG_HIDDEN);
        for (int idx = 0; idx != MAX_TARGETS_SHOWN; idx++) {
            lv_obj_add_flag(targets[idx].image, LV_OBJ_FLAG_HIDDEN);
            lv_obj_add_flag(targets[idx].label, LV_OBJ_FLAG_HIDDEN);
        }
        ESP_LOGI(TAG, "mainUpdate - GPS disconnected");
        return;
    }
    lv_obj_clear_flag(northImage, LV_OBJ_FLAG_HIDDEN);
    // precalculate rotation
    cosT = cosf(toRadians(-ourPosition.report.track));
    sinT = sinf(toRadians(-ourPosition.report.track));
    lv_img_set_angle(northImage, (int16_t) (-ourPosition.report.track * 10));
    translate(northImage, (float) height / 2, 0);
    for (int idx = 0; idx != MAX_TARGETS_SHOWN; idx++) {
        traffic_t *trafp = traffic + idx;
        target_t * targp = targets + idx;
        if (!trafp->active) {
            lv_obj_add_flag(targp->image, LV_OBJ_FLAG_HIDDEN);
            lv_obj_add_flag(targp->label, LV_OBJ_FLAG_HIDDEN);
        } else {
            //printf("Process target %s, altitude %f\n", trafp->report.callsign, trafp->report.altitude);
            lv_obj_clear_flag(targp->image, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_flag(targp->label, LV_OBJ_FLAG_HIDDEN);
            textLen += sprintf(textBuf + textLen, "\n%.8s %.1f", trafp->report.callsign, trafp->distance / 1000);
            int angle = (int32_t) (trafp->report.track - ourPosition.report.track) % 360;
            if (angle < 0)
                angle += 360;
            lv_img_set_angle(targp->image, (int16_t) (angle * 10));
            lv_img_set_src(targp->image, &target);
            float altDiff = convertToUserUnit(trafp->report.altitude - ourPosition.report.altitude, &prefAltitudeUnit);
            if (prefAltitudeUnit.currentValue.intValue == ALTITUDE_FEET)
                altDiff /= 100;
            else
                altDiff /= 10;
            const char *callsign = trafp->report.callsign;
            //printf("Process target %s, altDiff %f\n", callsign, altDiff);
            //lv_label_set_text(targp->label, callsign);
            char buffer[40];
            snprintf(buffer, sizeof buffer,  "%+04.0f\n%.3s", altDiff, callsign);
            lv_label_set_text(targp->label, buffer);
            float deltaEast = (trafficEasting(&ourPosition.report, &trafp->report) * scale);
            float deltaNorth = (trafficNorthing(&ourPosition.report, &trafp->report) * scale);
            translate(targp->image, deltaNorth, deltaEast);
            lv_obj_align_to(targp->label, targp->image, LV_ALIGN_LEFT_MID, target.header.w, 0);
        }
    }
    lv_label_set_text(trafficLabel, textBuf);
}

static void mainTeardown() {
    // objects will be deleted, so remove references to them
    memset(targets, 0, sizeof(targets));
}

const uiScreen_t uiMain = {
        .name = "Main screen",
        .setup = mainSetup,
        .update = mainUpdate,
        .refreshMs = 1000,
        .teardown = mainTeardown
};
