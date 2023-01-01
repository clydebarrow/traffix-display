//
// Created by Clyde Stubbs on 26/10/2022.
//

#ifndef TRAFFIX_DISPLAY_EVENTS_H
#define TRAFFIX_DISPLAY_EVENTS_H

#include "esp_event.h"

const ESP_EVENT_DECLARE_BASE(EVENT_BASE);

extern esp_event_loop_handle_t loopHandle;

enum {
    EVENT_WIFI_CHANGE,
    EVENT_GDL90_CHANGE,
};

extern void initEvents();
extern int postMessage(int32_t id, void * data, size_t len);
extern void dispatchEvents(uint32_t delay);

#endif //TRAFFIX_DISPLAY_EVENTS_H
