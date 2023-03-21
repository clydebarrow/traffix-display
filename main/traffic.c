//
// Created by Clyde Stubbs on 4/11/2022.
//

#include <esp_timer.h>
#include <freertos/FreeRTOS.h>
#include <math.h>
#include <pthread.h>
#include <string.h>
#include "traffic.h"
#include "gdltask.h"
#include "preferences.h"
#include "main.h"
#include "ownship.h"

/**
 * The currently tracked traffic list.
 */
static traffic_t traffic[MAX_TRAFFIC_TRACKED];
static pthread_mutex_t trafficMutex;        // control access to the traffic array

static void printPosition(bool ownship, const gdl90PositionReport_t *report, float distance, uint32_t timestampMs) {
    uint32_t now = (uint32_t) (esp_timer_get_time() / 1000);

    printf("%s: %s dist %.1fnm, speed %.0f alt %.0f trk %.1f age %.1fsecs",
           ownship ? "Ownship" : "Traffic",
           report->callsign,
           distance / 1852.0f,
           report->groundSpeed * 3600.0f / 1852.0f,
           report->altitude / 0.3048f,
           report->track,
           (float) (now - timestampMs) / 1000.0f);
}

static void updateTraffic(traffic_t *tp, const gdl90PositionReport_t *report, float distance) {
    tp->timestampMs = (uint32_t) (esp_timer_get_time() / 1000);
    tp->report = *report;
    tp->distance = distance;
    tp->active = true;
}


/**
 * Compare two traffic entries. Active targets compare smaller than inactive, otherwise it is
 * based on distance.
 * @param tp1
 * @param tp2
 * @return
 */
static int compareTraffic(const void *tp1, const void *tp2) {
    if (!((traffic_t *) tp1)->active && !((traffic_t *) tp2)->active)
        return 0;
    if (!((traffic_t *) tp2)->active)
        return -1;
    if (!((traffic_t *) tp1)->active)
        return 1;
    return (int) ((((traffic_t *) tp1)->distance - ((traffic_t *) tp2)->distance) * 1000.0f);
}

// sort the traffic by nearest first.
// old traffic will first be marked as inactive.

static void sortTraffic() {
    uint32_t oldMs = esp_timer_get_time() / 1000 - MAX_TRAFFIC_AGE_MS;
    TARGETS_FOREACH(tp) {
        if (tp->active && tp->timestampMs < oldMs)
            tp->active = false;
    }
    qsort(traffic, ARRAY_SIZE(traffic), sizeof(traffic_t), compareTraffic);
}

/**
 * Find a free slot to store a traffic report. Either return an unused/expired slot, or
 * the one that is furthest away
 * @return
 */
static traffic_t *getEntry() {
    sortTraffic();
    // the last one is the furthest away, or unused.
    return traffic + ARRAY_SIZE(traffic) - 1;
}

/**
 * Set up the traffic system.
 */
void initTraffic() {
    pthread_mutex_init(&trafficMutex, NULL);
}

/**
 * Receive a traffic report and update the traffic list
 * An existing target will be updated.
 * if the target is not currently in the list, it will be added if there is an inactive slot available,
 * or the new target is closer than the farthest currently active target, in which case that slot will
 * be evicted and filled with the new target.
 * @param report
 */
void processTraffic(const gdl90PositionReport_t *report) {
    ownship_t ourPosition;
    bool posValid = getOwnshipPosition(&ourPosition);
    if(!posValid)
        return;
    pthread_mutex_lock(&trafficMutex);
    float distance = trafficDistance(&ourPosition.report, report);     // pre-calculate distance in meters
    // is the target currently in our list? If so just update it
    TARGETS_FOREACH(tp) {
        if (tp->report.addressType == report->addressType && tp->report.address == report->address) {
            updateTraffic(tp, report, distance);
            pthread_mutex_unlock(&trafficMutex);
            return;
        }
    }
    // filter it out if too far away, unless overridden.
    float vDiff = fabsf(report->altitude - ourPosition.report.altitude);
    if ((distance > prefRangeH.currentValue.floatValue || vDiff > prefRangeV.currentValue.floatValue) &&
        !isButtonPressed(0)) {
        pthread_mutex_unlock(&trafficMutex);
        return;
    }
    traffic_t *tp = getEntry();
    // commandeer the slot if it's outdated, or further away than the new target
    if (!tp->active || tp->distance > distance)
        updateTraffic(tp, report, distance);
    pthread_mutex_unlock(&trafficMutex);
}

/**
 * Get the closest cnt traffic targets and return copies of them in the buffer. The number of targets
 * returned will always be equal to cnt, but not all may be active.
 * @param buffer receives the data
 * @param cnt Number of targets to return.
 */

void getTraffic(traffic_t *buffer, size_t cnt) {
    pthread_mutex_lock(&trafficMutex);
    sortTraffic();
    memcpy(buffer, traffic, cnt * sizeof(traffic_t));
    pthread_mutex_unlock(&trafficMutex);
}

/**
 * Print a representation of the current traffic list.
 */
void showTraffic() {
    uint32_t oldMs = esp_timer_get_time() / 1000 - MAX_TRAFFIC_AGE_MS;
    // move cursor up and clear screen
    //printf("\r\033[%dA\033[J", cnt+2);
    ownship_t ourPosition;
    getOwnshipPosition(&ourPosition);
    printPosition(true, &ourPosition.report, 0.0f, ourPosition.timestampMs);
    printf("\n");
    TARGETS_FOREACH(tp) {
        if (tp->timestampMs >= oldMs) {
            printPosition(false, &tp->report, tp->distance, tp->timestampMs);
            printf("\n");
        }
    }
}