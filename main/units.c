//
// Created by Clyde Stubbs on 21/12/2022.
//

#include <stdint-gcc.h>
#include "units.h"
#include "preferences.h"
#include "main.h"

const unitData_t altitudeUnitData[] = {
        {.factor = 0.3048, "ft"},
        {.factor = 1.0,    "m"},
};
const unitData_t distanceUnitData[] = {
        {.factor = 1852.0, "nm"},
        {.factor = 1000.0, "km"},
};

const prefUnit_t prefDistanceData = {
        ARRAY_SIZE(distanceUnitData),
        distanceUnitData
};

const prefUnit_t prefAltitudeData = {
        ARRAY_SIZE(altitudeUnitData),
        altitudeUnitData
};

preference_t prefDistanceUnit = {
        // what unit to use for distance values
        "prefDistanceUnit",
        "Unit for distance",
        PREF_UNIT,
        {.intValue = DISTANCE_KM},
        {},
        {.units = &prefDistanceData}
};

preference_t prefAltitudeUnit = {
        // what unit to use for distance values
        "prefAltitudeUnit",
        "Unit for altitude",
        PREF_UNIT,
        {.intValue = ALTITUDE_FEET},
        {},
        {.units = &prefAltitudeData}
};

float convertToUnit(float value, unitData_t * unit) {
    return value / unit->factor;
}

float convertFromUnit(float value, unitData_t * unit) {
    return value * unit->factor;
}

float convertToUserUnit(float value, preference_t *unit) {
    return value / unit->data.units->choices[unit->currentValue.intValue].factor;
}
float convertFromUserUnit(float value, preference_t *unit) {
    return value * unit->data.units->choices[unit->currentValue.intValue].factor;
}

const char * userUnitName(preference_t * unit) {
    return unit->data.units->choices[unit->currentValue.intValue].name;
}
