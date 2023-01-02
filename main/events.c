//
// Created by Clyde Stubbs on 26/10/2022.
//

#include "events.h"
#include "ui.h"
#include "wifi_prov.h"
#include "main.h"
#include "status.h"


static const char *TAG = "Events";

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
    switch (eventId) {

        case EVENT_WIFI_CHANGE:

            switch (wifiState) {

                case WIFI_UNKNOWN:
                    provisionWiFi(isButtonPressed(0));
                    break;

                case WIFI_PROVISIONING:
                    setUiState(UI_STATE_WIFI_PROVISION);
                    break;

                case WIFI_CONNECTING:
                    setUiState(UI_STATE_WIFI_CONNECT);
                    break;

                case WIFI_CONNECTED:
                    setUiState(UI_STATE_READY);
                    break;

                default:    // connected, all good.
                    break;
            }

        case EVENT_GDL90_CHANGE:
            break;
        default:
            ESP_LOGI(TAG, "Event %d received", (int)eventId);
            break;
    }
    statusUpdate();
}

/**
 * Listen for events and manage state accordingly
 */
void initEvents() {
    esp_event_loop_args_t loopArgs = {
            .queue_size = 10,
    };
    esp_event_loop_create(&loopArgs, &loopHandle);
    esp_event_handler_register_with(loopHandle, EVENT_BASE, ESP_EVENT_ANY_ID, eventHandler, NULL);
}

int postMessage(int32_t id, void *data, size_t len) {
    return esp_event_post_to(loopHandle, EVENT_BASE, id, data, len, 0);
}

void dispatchEvents(uint32_t delay) {
    esp_event_loop_run(loopHandle, delay);
}
