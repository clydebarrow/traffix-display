/* INCLUDES ------------------------------------------------------------------*/
#include <sys/cdefs.h>
#include "freertos/FreeRTOS.h"
#include <freertos/task.h>
#include <esp_event.h>
#include "ui.h"
#include "events.h"
#include "main.h"

static const char *TAG = "ui";

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

static void tileDeleteCb(lv_event_t * event) {
    uiScreen_t * uis = event->user_data;
    ESP_LOGI(TAG, "delete tile for %s", uis->name);
    if(uis->teardown)
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
    //if(uiScreens[uiState]->teardown != NULL)
    //   uiScreens[uiState]->teardown(curTile);
    uiState = newState;
    lv_obj_t *curTile = lv_obj_create(NULL);
    lv_obj_add_style(curTile, &bgStyle, 0);
    currentScreen->setup(curTile);
    lv_obj_add_event_cb(curTile, tileDeleteCb, LV_EVENT_DELETE, (void *)currentScreen);
    // show the new tile
    lv_scr_load_anim(curTile, LV_SCR_LOAD_ANIM_FADE_IN, 400, 0, true);
    setBacklightState(true);
}

/**
 * This function manages the different ui screens
 */
_Noreturn void uiLoop() {

    esp_event_handler_register_with(loopHandle, UI_EVENT_BASE, ESP_EVENT_ANY_ID, uiEventHandler, NULL);
    setUiState(UI_STATE_INIT);      // perform initialisation
    vTaskDelay(100 / portTICK_PERIOD_MS);
    setUiState(UI_STATE_SPLASH);
    vTaskDelay(2000 / portTICK_PERIOD_MS);
    postMessage(EVENT_WIFI_CHANGE, NULL, 0);
    for (;;) {
        vTaskDelay(250 / portTICK_PERIOD_MS);       // update the screen every this long
        if(currentScreen && currentScreen->update)
            currentScreen->update(lv_scr_act());
        //ESP_LOGI(TAG, "task tick");
    }
}