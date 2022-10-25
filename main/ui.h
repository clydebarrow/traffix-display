
/**
 ******************************************************************************
 * @Channel Link    :  https://www.youtube.com/user/wardzx1
 * @file    		:  uiLoop.h
 * @author  		:  Ward Almasarani - Useful Electronics
 * @version 		:  v.1.0
 * @date    		:  Aug 21, 2022
 * @brief   		:
 *
 ******************************************************************************/

#ifndef MAIN_LVGL_DEMO_UI_H_
#define MAIN_LVGL_DEMO_UI_H_


/* INCLUDES ------------------------------------------------------------------*/
#include <math.h>
#include "lvgl.h"
#include "esp_log.h"

/* MACROS --------------------------------------------------------------------*/
#ifndef PI
#define PI  (3.14159f)
#endif

LV_IMG_DECLARE(traffix_logo)

//LV_IMG_DECLARE(ue_text)

LV_IMG_DECLARE(earth)

/* ENUMERATIONS --------------------------------------------------------------*/

/**
 * UI states
 *
 * This list must match the uiStateList.
 */
enum {
    UI_STATE_INIT = 0,
    UI_STATE_SPLASH,            // show splash screen
    UI_STATE_WIFI_PROVISION,    // show provisioning QR code
    UI_STATE_WIFI_CONNECT,      // show wifi connection status
    UI_STATE_RADAR,             // show the main display screen.
    UI_STATE_ALARM,             // show the alarm screen
    UI_STATE_SETTINGS,          // show the settings screen.
};

/* STRUCTURES & TYPEDEFS -----------------------------------------------------*/
typedef struct {
    lv_obj_t *scr;
    int count_val;
} my_timer_context_t;

/**
 * An interface structure for a UI state.
 */

typedef struct {
    const char * name;      // a name for this state
    void (*setup)(lv_obj_t * tile);
    void (*update)(lv_obj_t * tile);
    void (*teardown)(lv_obj_t * tile);
} uiState;


/* VARIABLES -----------------------------------------------------------------*/

extern uiState uiStateList[];

extern uiState uiSplash, uiWiFiProvision, uiWiFiConnect, uiRadar;

/* FUNCTIONS DECLARATION -----------------------------------------------------*/

void uiLoop(lv_obj_t *scr);     // runs the ui


#endif /* MAIN_LVGL_DEMO_UI_H_ */

/**************************  Useful Electronics  ****************END OF FILE***/
