//
// Created by Clyde Stubbs on 21/12/2022.
//

#ifndef TRAFFIX_DISPLAY_UNITS_H
#define TRAFFIX_DISPLAY_UNITS_H

typedef struct unitData_t_struct unitData_t;

#include "preferences.h"

struct unitData_t_struct {
    double factor;                  // How many standard units this unit is.
    const char *name;          // the name of this unit
};

typedef enum {
    ALTITUDE_FEET,
    ALTITUDE_METERS,
} altitudeUnit_t;

extern unitData_t const altitudeUnitData[];

typedef enum {
    DISTANCE_NM,
    DISTANCE_KM
} distanceUnit_t;

extern const unitData_t distanceUnitData[];

extern float convertToUnit(float value, unitData_t *unit);

extern float convertFromUnit(float value, unitData_t *unit);

extern float convertToUserUnit(float value, preference_t *unit);

extern float convertFromUserUnit(float value, preference_t *unit);
extern const char * userUnitName(preference_t * unit);

#endif //TRAFFIX_DISPLAY_UNITS_H
