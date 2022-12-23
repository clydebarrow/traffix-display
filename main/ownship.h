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


extern bool isGpsConnected();

extern void setOwnshipPosition(gdl90PositionReport_t * position, bool valid);
extern bool getOwnshipPosition(ownship_t * position);
extern void initOwnship();

#endif //TRAFFIX_DISPLAY_OWNSHIP_H
