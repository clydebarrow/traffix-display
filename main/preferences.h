//
// Created by Clyde Stubbs on 18/11/2022.
//

#ifndef TRAFFIX_DISPLAY_PREFERENCES_H
#define TRAFFIX_DISPLAY_PREFERENCES_H

#include <stdint-gcc.h>

/**
 * Available preferences
 */

typedef enum {
    PREF_INT32,
    PREF_DISTANCE,      // distance in meters
    PREF_HEIGHT,      // distance in meters
    PREF_STRING
} prefType_t;

/**
 * Store current pref values in this union. Strings will be dynamically allocated.
 */
typedef union {
    int32_t intValue;
    const char * stringValue;
} prefData_t;

typedef struct {
    const char * key;      // the key used to store in flash
    const char * name;      //user-visible name
    prefType_t type;        // what data type is it?
    prefData_t defaultValue;    // a default value
    prefData_t currentValue;    // the current value. This implies the whole structure must be in RAM
} preference_t;

extern void initPrefs();        // initialise the preferences

// preferences, here so values can be accessed.
extern preference_t
        prefTrafficTimeout,
        prefRangeH,
        prefRangeV;
#endif //TRAFFIX_DISPLAY_PREFERENCES_H
