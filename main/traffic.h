//
// Created by Clyde Stubbs on 4/11/2022.
//

#ifndef TRAFFIX_DISPLAY_TRAFFIC_H
#define TRAFFIX_DISPLAY_TRAFFIC_H

#include <stdint-gcc.h>
#include <stdbool.h>
#include "gdl90.h"

#define MAX_TRAFFIC_TRACKED 20      // maximum number of aircraft to track
#define MAX_TRAFFIC_AGE_MS  10000   // max traffic age in ms
#define TARGETS_FOREACH(ptr)  for(traffic_t * ptr = traffic ; ptr != traffic + MAX_TRAFFIC_TRACKED ; ptr++)

/**
 * Store traffic information
 */
typedef struct {
    gdl90PositionReport_t report;
    float distance;       // calculated distance from us
    uint32_t timestampMs;   // when report last updated in ms
    bool    active;
} traffic_t;

extern void processTraffic(const gdl90PositionReport_t * report);
extern void showTraffic();
extern void sortTraffic();      // sorts traffic, closest first
extern traffic_t traffic[MAX_TRAFFIC_TRACKED];

#endif //TRAFFIX_DISPLAY_TRAFFIC_H
