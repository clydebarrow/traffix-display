//
// Created by Clyde Stubbs on 4/11/2022.
//

#include <esp_timer.h>
#include <freertos/FreeRTOS.h>
#include "traffic.h"
#include "gdltask.h"




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
           (float)(now-pr->timestamp)/ 1000.0f);
}

static void updateTraffic(traffic_t *tp, const gdl90PositionReport_t *report, float distance) {
    tp->timestamp = (uint32_t) (esp_timer_get_time() / 1000);
    tp->report = *report;
    tp->distance = distance;
    tp->active = true;
}

/**
 * Find a free slot to store a traffic report. Either return an unused/expired slot, or
 * the one that is furthest away
 * @return
 */
static traffic_t * getEntry() {
    traffic_t * furthest = NULL;
    uint32_t oldMs = esp_timer_get_time() / 1000 - MAX_TRAFFIC_AGE_MS;
    TARGETS_FOREACH(tp) {
        if(tp->timestamp < oldMs)
            return tp;
        if(furthest == NULL || tp->distance > furthest->distance)
            furthest = tp;
    }
    return furthest;
}

void processTraffic(const gdl90PositionReport_t *report) {
    float distance = trafficDistance(&ourPosition.report, report);     // pre-calculate
    // is the target currently in our list? If so just update it
    TARGETS_FOREACH(tp) {
        if (tp->report.addressType == report->addressType && tp->report.address == report->address) {
            updateTraffic(tp, report, distance);
            return;
        }
    }
    traffic_t * tp = getEntry();
    if(tp->distance == 0 || tp->distance > distance)
        updateTraffic(tp, report, distance);
}

void trafficUpdateActive() {
    uint32_t oldMs = esp_timer_get_time() / 1000 - MAX_TRAFFIC_AGE_MS;
    TARGETS_FOREACH(tp) {
        if(tp->active && tp->timestamp < oldMs)
            tp->active = false;
    }
}
void showTraffic() {
    uint32_t oldMs = esp_timer_get_time() / 1000 - MAX_TRAFFIC_AGE_MS;
    int cnt = MAX_TRAFFIC_TRACKED;
    TARGETS_FOREACH(tp) {
        if (tp->timestamp >= oldMs)
            cnt++;
    }
    // move cursor up and clear screen
    printf("\r\033[%dA\033[J", cnt+2);
    printf("heap free, = %d smallest  = %d\n", xPortGetFreeHeapSize(), xPortGetMinimumEverFreeHeapSize());
    printPosition(true, &ourPosition);
    printf("\n");
    TARGETS_FOREACH(tp) {
        if (tp->timestamp >= oldMs) {
            printPosition(false, tp);
            printf("\n");
        }
    }
}