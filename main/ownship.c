//
// Created by Clyde Stubbs on 23/12/2022.
//

#include <sys/types.h>
#include <pthread.h>
#include <esp_timer.h>
#include "ownship.h"

#define MAX_POSITION_AGE  5000   // max position age in ms

static gdl90Heartbeat_t lastHeartbeat;
static pthread_mutex_t ownshipMutex;        // control access to the traffic array
static ownship_t ourPosition;
static gpsStatus_t gpsStatus;

void initOwnship() {
    pthread_mutex_init(&ownshipMutex, NULL);
}

bool isGpsConnected() {
    return lastHeartbeat.gpsPosValid && ourPosition.timestampMs > (esp_timer_get_time() / 1000) - MAX_POSITION_AGE;
}

void setOwnshipPosition(gdl90PositionReport_t *position) {
    pthread_mutex_lock(&ownshipMutex);
    ourPosition.report = *position;
    ourPosition.timestampMs = esp_timer_get_time() / 1000;
    pthread_mutex_unlock(&ownshipMutex);
}

bool getStatus(gpsStatus_t *status) {
    pthread_mutex_lock(&ownshipMutex);
    *status = gpsStatus;
    bool valid = isGpsConnected();
    pthread_mutex_unlock(&ownshipMutex);
    return valid;
}

void setStatus(gpsStatus_t *status) {
    pthread_mutex_lock(&ownshipMutex);
    gpsStatus = *status;
    pthread_mutex_unlock(&ownshipMutex);
}

bool getOwnshipPosition(ownship_t *position) {
    pthread_mutex_lock(&ownshipMutex);
    *position = ourPosition;
    bool valid = isGpsConnected();
    pthread_mutex_unlock(&ownshipMutex);
    return valid;
}

void setHeartbeat(gdl90Heartbeat_t *heartbeat) {
    pthread_mutex_lock(&ownshipMutex);
    lastHeartbeat = *heartbeat;
    pthread_mutex_unlock(&ownshipMutex);
}

bool getHeartbeat(gdl90Heartbeat_t *heartbeat) {
    pthread_mutex_lock(&ownshipMutex);
    *heartbeat = lastHeartbeat;
    bool valid = isGpsConnected();
    pthread_mutex_unlock(&ownshipMutex);
    return valid;
}

