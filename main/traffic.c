//
// Created by Clyde Stubbs on 4/11/2022.
//

#include <esp_timer.h>
#include <freertos/FreeRTOS.h>
#include <math.h>
#include "traffic.h"
#include "gdltask.h"
#include "preferences.h"
#include "main.h"

traffic_t traffic[MAX_TRAFFIC_TRACKED];

static void printPosition(bool ownship, const traffic_t *pr) {
    uint32_t now = (uint32_t) (esp_timer_get_time() / 1000);

    printf("%s: %s dist %.1fnm, speed %.0f alt %.0f trk %.1f age %.1fsecs",
           ownship ? "Ownship" : "Traffic",
           pr->report.callsign,
           pr->distance / 1852.0f,
           pr->report.groundSpeed * 3600.0f / 1852.0f,
           pr->report.altitude / 0.3048f,
           pr->report.track,
           (float) (now - pr->timestampMs) / 1000.0f);
}

static void updateTraffic(traffic_t *tp, const gdl90PositionReport_t *report, float distance) {
    tp->timestampMs = (uint32_t) (esp_timer_get_time() / 1000);
    tp->report = *report;
    tp->distance = distance;
    tp->active = true;
}

/**
 * Find a free slot to store a traffic report. Either return an unused/expired slot, or
 * the one that is furthest away
 * @return
 */
static traffic_t *getEntry() {
    traffic_t *furthest = NULL;
    uint32_t oldMs = esp_timer_get_time() / 1000 - MAX_TRAFFIC_AGE_MS;
    TARGETS_FOREACH(tp) {
        if (tp->timestampMs < oldMs)
            return tp;
        if (furthest == NULL || tp->distance > furthest->distance)
            furthest = tp;
    }
    return furthest;
}

void processTraffic(const gdl90PositionReport_t *report) {
    float distance = trafficDistance(&ourPosition.report, report);     // pre-calculate distance in meters
    // is the target currently in our list? If so just update it
    TARGETS_FOREACH(tp) {
        if (tp->report.addressType == report->addressType && tp->report.address == report->address) {
            updateTraffic(tp, report, distance);
            return;
        }
    }
    // filter it out if too far away, unless overridden.
    float vDiff = fabsf(report->altitude - ourPosition.report.altitude);
    if ((distance > (float) prefRangeH.currentValue.intValue || vDiff > (float) prefRangeV.currentValue.intValue) &&
        !isButtonPressed(0))
        return;
    traffic_t *tp = getEntry();
    // commandeer the slot if it's outdated, or further away than the new target
    if (tp->timestampMs < getNow() + MAX_TRAFFIC_AGE_MS || tp->distance > distance)
        updateTraffic(tp, report, distance);
}

int compareTraffic(const void *tp1, const void *tp2) {
    if (((traffic_t *) tp1)->active && !((traffic_t *) tp2)->active)
        return -1;
    if (((traffic_t *) tp2)->active && !((traffic_t *) tp1)->active)
        return 1;
    return (int) ((((traffic_t *) tp1)->distance - ((traffic_t *) tp2)->distance) * 1000.0f);
}

void sortTraffic() {
    uint32_t oldMs = esp_timer_get_time() / 1000 - MAX_TRAFFIC_AGE_MS;
    TARGETS_FOREACH(tp) {
        if (tp->active && tp->timestampMs < oldMs)
            tp->active = false;
    }
    qsort(traffic, ARRAY_SIZE(traffic), sizeof(traffic_t), compareTraffic);
}

void showTraffic() {
    uint32_t oldMs = esp_timer_get_time() / 1000 - MAX_TRAFFIC_AGE_MS;
    // move cursor up and clear screen
    //printf("\r\033[%dA\033[J", cnt+2);
    printf("heap free, = %d smallest  = %d\n", xPortGetFreeHeapSize(), xPortGetMinimumEverFreeHeapSize());
    printPosition(true, &ourPosition);
    printf("\n");
    TARGETS_FOREACH(tp) {
        if (tp->timestampMs >= oldMs) {
            printPosition(false, tp);
            printf("\n");
        }
    }
}