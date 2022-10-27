#ifndef TRAFFIX_UI_H_
#define TRAFFIX_UI_H_


/* INCLUDES ------------------------------------------------------------------*/
#include "lvgl.h"
#include "esp_log.h"
#include "esp_event_base.h"

LV_IMG_DECLARE(traffix_logo)

const ESP_EVENT_DECLARE_BASE(UI_EVENT_BASE);

/* ENUMERATIONS --------------------------------------------------------------*/

/**
 * UI states
 *
 * This list must match the uiStateList.
 */
typedef enum {
    UI_STATE_INIT = 0,
    UI_STATE_SPLASH,            // show splash screen
    UI_STATE_WIFI_PROVISION,    // show provisioning QR code
    UI_STATE_WIFI_CONNECT,      // show wifi connection status
    UI_STATE_READY,             // show the main display screen.
    UI_STATE_ALARM,             // show the alarm screen
    UI_STATE_SETTINGS,          // show the settings screen.
} uiState_t;

/* STRUCTURES & TYPEDEFS -----------------------------------------------------*/
typedef struct {
    lv_obj_t *scr;
    int count_val;
} my_timer_context_t;

/**
 * An interface structure for a UI state.
 */

typedef struct {
    const char *name;      // a name for this state
    void (*setup)(lv_obj_t *tile);

    void (*update)(lv_obj_t *tile);

    void (*teardown)(lv_obj_t *tile);
} uiScreen_t;


/* VARIABLES -----------------------------------------------------------------*/

extern const uiScreen_t uiSplash, uiWiFiProvision, uiWiFiConnect, uiMain;
extern uiState_t uiState;

/* FUNCTIONS DECLARATION -----------------------------------------------------*/

extern void uiLoop();     // runs the ui
extern void setUiState(uiState_t state);


#endif /* TRAFFIX_UI_H_ */

