/* INCLUDES ------------------------------------------------------------------*/
#include <sys/cdefs.h>
#include "freertos/FreeRTOS.h"
#include <freertos/task.h>
#include <esp_event.h>
#include "ui.h"
#include "events.h"
#include "main.h"
#include "extra/widgets/tileview/lv_tileview.h"

static const char *TAG = "ui";

const ESP_EVENT_DEFINE_BASE(UI_EVENT_BASE);

/* VARIABLES -----------------------------------------------------------------*/

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


static lv_style_t style;
static lv_style_t bgStyle;

static lv_obj_t *tileView;

static lv_obj_t *tile0, *tile1;

/* DEFINITIONS ---------------------------------------------------------------*/

/* MACROS --------------------------------------------------------------------*/

/* PRIVATE FUNCTIONS DECLARATION ---------------------------------------------*/



/**
 * Set the UI state
 */

void setUiState(uiState_t state) {
    esp_event_post_to(loopHandle, UI_EVENT_BASE, state, NULL, 0, 0);
}

static void tileEventCb(lv_event_t *event) {
    switch (event->code) {
        case LV_EVENT_VALUE_CHANGED: {
            lv_obj_t *tile = lv_tileview_get_tile_act(tileView);
            ESP_LOGI(TAG, "Tile changed, new tile pos = %d", tile != tile0);
            // clean up the previous tile
        }
            break;
        default:
            //ESP_LOGI(TAG, "TileView event %d", event->code);
            break;
    }
}

/**
 * Process UI state changes
 */

static void uiEventHandler(void *handler_arg, esp_event_base_t base, int32_t newState, void *event_data) {
    lv_obj_t *scr = lv_scr_act();
    if (newState >= ARRAY_SIZE(uiScreens)) {
        ESP_LOGI(TAG, "Request for unknown UI state %d", newState);
        return;
    }
    const uiScreen_t *uis = uiScreens[newState];

    if (newState == UI_STATE_INIT) {
        ESP_LOGI(TAG, "UIState initialization");
        lv_obj_clean(scr);
        //Initialize 2 tiles that act as pages
        tileView = lv_tileview_create(scr);
        lv_obj_set_scrollbar_mode(tileView, LV_SCROLLBAR_MODE_OFF);
        lv_obj_add_event_cb(tileView, tileEventCb, LV_EVENT_ALL, NULL);
        lv_obj_align(tileView, LV_ALIGN_TOP_RIGHT, 0, 0);
        tile0 = lv_tileview_add_tile(tileView, 0, 0, LV_DIR_ALL);
        tile1 = lv_tileview_add_tile(tileView, 0, 1, LV_DIR_ALL);

        //create style to manipulate objects characteristics implicitly
        lv_style_init(&style);
        lv_style_init(&bgStyle);
        lv_style_set_bg_color(&bgStyle, lv_color_black());
        lv_obj_add_style(tileView, &bgStyle, 0);
        uiState = newState;
        return;
    }
    if (newState == uiState) {
        //ESP_LOGI(TAG, "UIState unchanged at %s", uis->name);
        return;
    }
    ESP_LOGI(TAG, "UIState changes to %s", uis->name);
    //if(uiScreens[uiState]->teardown != NULL)
    //   uiScreens[uiState]->teardown(curTile);
    uiState = newState;
    lv_obj_t *curTile = lv_tileview_get_tile_act(tileView);

    if (curTile == tile0)
        curTile = tile1;
    else
        curTile = tile0;
    lv_obj_clean(curTile);
    uis->setup(curTile);
    // show the new tile
    lv_obj_set_tile(tileView, curTile, LV_ANIM_ON);
}

/**
 * This function manages the different ui screens
 */
_Noreturn void uiLoop() {

    esp_event_handler_register_with(loopHandle, UI_EVENT_BASE, ESP_EVENT_ANY_ID, uiEventHandler, NULL);
    setUiState(UI_STATE_INIT);      // perform initialisation
    setUiState(UI_STATE_SPLASH);
    vTaskDelay(2000 / portTICK_PERIOD_MS);
    postMessage(EVENT_WIFI_CHANGE, NULL, 0);
    for (;;) {
        vTaskDelay(1000 / portTICK_PERIOD_MS);
        //ESP_LOGI(TAG, "task tick");
    }
}