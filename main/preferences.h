//
// Created by Clyde Stubbs on 18/11/2022.
//

#ifndef TRAFFIX_DISPLAY_PREFERENCES_H
#define TRAFFIX_DISPLAY_PREFERENCES_H

// forward reference
typedef struct preference_struct preference_t;

#include <stdint-gcc.h>
#include "units.h"

/**
 * Available preferences
 */

typedef enum {
    PREF_INT32,
    PREF_STRING,
    PREF_ENUM,      // stored as int, refers to an enum and a list of names
    PREF_UNIT,      // choose a unit
    PREF_VALUE      // Double value with associated unit
} prefType_t;

/**
 * Store current pref values in this union. Strings will be dynamically allocated.
 */
typedef union {
    int32_t intValue;
    float floatValue;
    const char *stringValue;
} prefData_t;

typedef struct {
    uint32_t count;
    const unitData_t * choices;
} prefUnit_t;

typedef struct {
    uint32_t count;
    const char * choices;
} prefEnum_t;
struct  preference_struct{
    const char *key;      // the key used to store in flash
    const char *name;      //user-visible name
    prefType_t type;        // what data type is it?
    prefData_t defaultValue;    // a default value
    prefData_t currentValue;    // the current value. This implies the whole structure must be in RAM
    union {
        const prefUnit_t * units;       // PREF_UNIT
        const prefEnum_t * choices;     // PREF_ENUM
        const preference_t * valueUnit;      // PREF_VALUE
    } data;
};

extern void initPrefs();        // initialise the preferences

// preferences, here so values can be accessed.
extern preference_t
        prefDistanceUnit,
        prefAltitudeUnit,
        prefTrafficTimeout,
        prefRangeH,
        prefRangeV;
#endif //TRAFFIX_DISPLAY_PREFERENCES_H
