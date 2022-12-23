/* INCLUDES ------------------------------------------------------------------*/
#include <sys/cdefs.h>
#include "freertos/FreeRTOS.h"
#include <freertos/task.h>
#include <esp_event.h>
#include "ui.h"
#include "events.h"
#include "main.h"
#include "status.h"
#include "esp_task_wdt.h"
#include "gdltask.h"
#include "traffic.h"
#include "udp.h"
#include "wifi_prov.h"

static const char *TAG = "ui";

#define PING_UPDATE_MS 20000        // delay between sending UDP pings to wake up data providers


const ESP_EVENT_DEFINE_BASE(UI_EVENT_BASE);

/* VARIABLES -----------------------------------------------------------------*/

const uiScreen_t *currentScreen;

static const uiScreen_t uiInit = {
        .name = "uiInit"
};
/**
 * Order must match the enum
 */
static const uiScreen_t *const uiScreens[] = {
        &uiInit,
        &uiSplash,
        &uiWiFiProvision,
        &uiWiFiConnect,
        &uiMain
};
uiState_t uiState = UI_STATE_INIT;
static lv_style_t bgStyle;

/**
 * Set the UI state
 */

void setUiState(uiState_t state) {
    esp_event_post_to(loopHandle, UI_EVENT_BASE, state, NULL, 0, 0);
}

static void tileDeleteCb(lv_event_t *event) {
    uiScreen_t *uis = event->user_data;
    ESP_LOGI(TAG, "delete tile for %s", uis->name);
    if (uis->teardown)
        uis->teardown(event->target);
}

/**
 * Process UI state changes
 */

static void uiEventHandler(void *handler_arg, esp_event_base_t base, int32_t newState, void *event_data) {
    if (newState >= ARRAY_SIZE(uiScreens)) {
        ESP_LOGI(TAG, "Request for unknown UI state %d", newState);
        return;
    }
    currentScreen = uiScreens[newState];

    if (newState == UI_STATE_INIT) {
        // show the new tile
        ESP_LOGI(TAG, "UIState initialization");

        //create style to manipulate objects characteristics implicitly
        lv_style_init(&bgStyle);
        lv_style_set_bg_color(&bgStyle, lv_color_black());
        lv_obj_t *curTile = lv_obj_create(NULL);
        lv_obj_add_style(curTile, &bgStyle, 0);
        lv_scr_load(curTile);
        uiState = newState;
        return;
    }
    if (newState == uiState) {
        //ESP_LOGI(TAG, "UIState unchanged at %s", currentScreen->name);
        return;
    }
    ESP_LOGI(TAG, "UIState changes to %s", currentScreen->name);
    printf("heap free, = %d smallest  = %d\n", xPortGetFreeHeapSize(), xPortGetMinimumEverFreeHeapSize());
    uiState = newState;
    lv_obj_t *curTile = lv_obj_create(NULL);
    lv_obj_add_style(curTile, &bgStyle, 0);
    currentScreen->setup(curTile);
    ESP_LOGI(TAG, "setup complete for %s", currentScreen->name);
    lv_obj_add_event_cb(curTile, tileDeleteCb, LV_EVENT_DELETE, (void *) currentScreen);
    // show the new tile
    lv_scr_load_anim(curTile, LV_SCR_LOAD_ANIM_FADE_IN, 400, 0, true);
    setBacklightState(true);
}

void showStats() {
    TaskStatus_t tasks[30];
    uint32_t runTime;
    size_t count = uxTaskGetSystemState(tasks, ARRAY_SIZE(tasks), &runTime);
    for (size_t i = 0; i != count; i++) {
        TaskStatus_t *tp = tasks + i;
        if (strcmp(tp->pcTaskName, "IDLE") == 0)
            printf("IDLE Task usage %d%%\n", tp->ulRunTimeCounter / (runTime/100));
        //printf("Task %s, runtime %d, stackHW %X\n", tp->pcTaskName, tp->ulRunTimeCounter, tp->usStackHighWaterMark);
    }
}
/**
 * This task manages the different ui screens
 */
_Noreturn void uiLoop() {

    esp_event_handler_register_with(loopHandle, UI_EVENT_BASE, ESP_EVENT_ANY_ID, uiEventHandler, NULL);
    setUiState(UI_STATE_INIT);      // perform initialisation
    vTaskDelay(2);   // short delay for backlight
    setUiState(UI_STATE_SPLASH);
    esp_task_wdt_add(NULL);
    uint64_t lastTimeUs = 0;
    uint64_t nextUpdateUs = 0;
    uint64_t nextPingUs = 0;
    ESP_LOGI(TAG, "uiLoop starting loop");
    for (;;) {
        uint32_t maxDelayMs = lv_timer_handler();
        if (maxDelayMs > 50)
            maxDelayMs = 50;     // don't sleep for more than this
        else if (maxDelayMs < portTICK_PERIOD_MS)
            maxDelayMs = portTICK_PERIOD_MS;
        dispatchEvents(maxDelayMs / portTICK_PERIOD_MS);        // dispatch events for this long
        uint64_t now = esp_timer_get_time();
        if (now >= nextUpdateUs) {
            if (currentScreen && currentScreen->update) {
                ESP_LOGI(TAG, "Update screen %s", currentScreen->name);
                currentScreen->update(lv_scr_act());
                nextUpdateUs = now + currentScreen->refreshMs * 1000LL;
            } else {
                nextUpdateUs = now + 1000000LL;
            }
            statusUpdate();
            //showTraffic();
            //showStats();
        }
        // periodically try to register with GDL90 sources
        if (wifiState == WIFI_CONNECTED && now >= nextPingUs) {
            nextPingUs = now + PING_UPDATE_MS * 1000LL;
            pingUdp();
        }
        uint32_t elapsedMs = ((now - lastTimeUs) & 0xFFFFFFFF) / 1000;
        lastTimeUs = now;
        esp_task_wdt_reset();
        lv_tick_inc(elapsedMs); // update LVGL with the current time.
    }
}