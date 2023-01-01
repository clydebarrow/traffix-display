//
// Created by Clyde Stubbs on 23/12/2022.
//

#ifndef TRAFFIX_DISPLAY_OWNSHIP_H
#define TRAFFIX_DISPLAY_OWNSHIP_H

#include <stdbool.h>
#include <stdint-gcc.h>
#include "gdl90.h"

typedef struct {
    gdl90PositionReport_t report;
    uint32_t timestampMs;   // when report last updated in ms
} ownship_t;

typedef enum {
    GPS_FIX_NONE = 1,
    GPS_FIX_2D,
    GPS_FIX_3D,
} gpsFix_t;

typedef struct {
    gpsFix_t gpsFix;
    float baroAltitude;
    float geoAltitude;
    int satellitesUsed;
    float accuracyH;
    float accuracyV;
    uint32_t gpsTime;
    int batteryPercent;
} gpsStatus_t;


extern bool isGpsConnected();

extern void setOwnshipPosition(gdl90PositionReport_t *position);

extern bool getOwnshipPosition(ownship_t *position);

extern void setHeartbeat(gdl90Heartbeat_t *heartbeat);

extern bool getHeartbeat(gdl90Heartbeat_t *heartbeat);

extern void setStatus(gpsStatus_t *status);

extern bool getStatus(gpsStatus_t *status);

extern void initOwnship();

#endif //TRAFFIX_DISPLAY_OWNSHIP_H
