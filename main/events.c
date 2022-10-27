//
// Created by Clyde Stubbs on 26/10/2022.
//

#include <esp_event.h>
#include "events.h"
#include "ui.h"
#include "wifi_prov.h"
#include "main.h"


const ESP_EVENT_DEFINE_BASE(EVENT_BASE);
esp_event_loop_handle_t loopHandle;

/**
 * Handle change in wifi state
 * @param handler_arg
 * @param base
 * @param eventId
 * @param event_data
 */
static void eventHandler(void *handler_arg, esp_event_base_t base, int32_t eventId, void *event_data) {
    switch(wifiState) {

        case WIFI_UNKNOWN:
            provisionWiFi(buttonPressed(0));
            return;

        case WIFI_PROVISIONING:
            setUiState(UI_STATE_WIFI_PROVISION);
            return;

        case WIFI_CONNECTING:
            setUiState(UI_STATE_WIFI_CONNECT);
            return;

        case WIFI_CONNECTED:
            setUiState(UI_STATE_READY);
            return;

        default:    // connected, all good.
            break;
    }
}
/**
 * Listen for events and manage state accordingly
 */
void initEvents() {
    esp_event_loop_args_t loopArgs = {
            .queue_size = 20,
            .task_name = "UILoop",
            .task_priority = 2,
            .task_core_id = APP_CPU_NUM,
            .task_stack_size = 16000
    };
    esp_event_loop_create(&loopArgs, &loopHandle);
    esp_event_handler_register_with(loopHandle, EVENT_BASE, EVENT_WIFI_CHANGE, eventHandler, NULL);

}
int postMessage(int32_t id, void * data, size_t len) {
    return esp_event_post_to(loopHandle, EVENT_BASE, id, data, len, 0);
}
