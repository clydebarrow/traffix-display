//
// Created by Clyde Stubbs on 23/12/2022.
//

#include <sys/types.h>
#include <pthread.h>
#include <esp_timer.h>
#include "ownship.h"

#define MAX_POSITION_AGE  5000   // max position age in ms

static pthread_mutex_t ownshipMutex;        // control access to the traffic array
static ownship_t ourPosition;

void initOwnship() {
    pthread_mutex_init(&ownshipMutex, NULL);
}
bool isGpsConnected() {
    return ourPosition.timestampMs > (esp_timer_get_time() / 1000) - MAX_POSITION_AGE;
}

void setOwnshipPosition(gdl90PositionReport_t * position, bool valid) {
    pthread_mutex_lock(&ownshipMutex);
    ourPosition.report = *position;
    if(valid)
        ourPosition.timestampMs = esp_timer_get_time() / 1000;
    else
        ourPosition.timestampMs = 0;
    pthread_mutex_unlock(&ownshipMutex);
}

bool getOwnshipPosition(ownship_t * position) {
    pthread_mutex_lock(&ownshipMutex);
    *position = ourPosition;
    bool valid = isGpsConnected();
    pthread_mutex_unlock(&ownshipMutex);
    return valid;
}
