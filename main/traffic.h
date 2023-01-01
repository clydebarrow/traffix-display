//
// Created by Clyde Stubbs on 4/11/2022.
//

#ifndef TRAFFIX_DISPLAY_TRAFFIC_H
#define TRAFFIX_DISPLAY_TRAFFIC_H

#include <stdint-gcc.h>
#include <stdbool.h>
#include "gdl90.h"

#define MAX_TRAFFIC_TRACKED 20      // maximum number of aircraft to track
#define MAX_TARGETS_SHOWN   4       // max number to show
#define MAX_TRAFFIC_AGE_MS  10000   // max traffic age in ms
#define TARGETS_FOREACH(ptr)  FOREACH(traffic, ptr, traffic_t)

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
extern void initTraffic();
extern void showTraffic();
extern void getTraffic(traffic_t * buffer, size_t cnt);      // get the nearest cnt traffic objects

#endif //TRAFFIX_DISPLAY_TRAFFIC_H
