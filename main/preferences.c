//
// Created by Clyde Stubbs on 18/11/2022.
//

#include <nvs.h>
#include <string.h>
#include "preferences.h"
#include "units.h"
#include "main.h"
#include "flarm.h"

#define PREF_NAMESPACE  "pref"


preference_t prefRangeH = {
        // horizontal range to filter on. Stored value is in meters
        "prefRangeH",
        "Max horizontal range",
        PREF_VALUE,
        {.floatValue = 8000},
        {},
        {.valueUnit = &prefDistanceUnit}
};
preference_t prefRangeV = {
        // vertical range to filter on. Stored value is in meters
        "prefRangeV",
        "Max vertical range",
        PREF_VALUE,
        {.floatValue = 1500},
        {},
        {.valueUnit = &prefAltitudeUnit}
};
preference_t prefTrafficTimeout = {
        // Discard traffic if no data after this many seconds
        "prefTrfcTout",
        "Max traffic data age (secs)",
        PREF_INT32,
        {.intValue = 10},
        {},
        {}
};


preference_t *const preferences[] = {
        &prefAltitudeUnit,
        &prefDistanceUnit,
        &prefRangeH,
        &prefRangeV,
        &prefTrafficTimeout,
        &prefFlarmBaudRate,
        &prefUDPPort,
};

static nvs_handle_t prefHandle;

/**
 * Initialise preferences on startup
 */
void initPrefs() {
    ESP_ERROR_CHECK(nvs_open(PREF_NAMESPACE, NVS_READWRITE, &prefHandle));
    for (int i = 0; i != ARRAY_SIZE(preferences); i++) {
        preference_t *pp = preferences[i];
        esp_err_t err = ESP_ERR_INVALID_ARG;
        size_t length;
        switch (pp->type) {
            case PREF_INT32:
            case PREF_ENUM:
            case PREF_VALUE:
                err = nvs_get_i32(prefHandle, pp->key, &pp->currentValue.intValue); // works for float as well
                if (err != ESP_OK)
                    pp->currentValue = pp->defaultValue;
                break;

            case PREF_STRING:
                err = nvs_get_str(prefHandle, pp->key, NULL, &length);
                if (err == ESP_ERR_NVS_INVALID_LENGTH) {
                    char *buffer = malloc(length);
                    pp->currentValue.stringValue = buffer;
                    err = nvs_get_str(prefHandle, pp->key, buffer, &length);
                }
                if (err != ESP_OK)
                    pp->currentValue.stringValue = strdup(pp->defaultValue.stringValue);
                break;

            default:
                break;
        }
    }
}