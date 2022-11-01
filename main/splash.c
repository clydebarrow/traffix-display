//
// Created by Clyde Stubbs on 25/10/2022.
//

#include "ui.h"
/**
* Create a splash screen on the given tile
*/

/**
 * Styles for the title and the whole tile.
 */

static const char *TAG = "Splash";


LV_IMG_DECLARE(traffix_logo)

static void splashCreate(lv_obj_t *tile) {
    ESP_LOGI(TAG, "create splash");

    lv_obj_t *logoImage;
    logoImage = lv_img_create(tile);
    lv_img_set_src(logoImage, &traffix_logo);
    lv_obj_align(logoImage, LV_ALIGN_CENTER, 0, 0);
    ESP_LOGI(TAG, "Splash image added");
}

const uiScreen_t uiSplash = {
        .name = "Splash",
        .setup = splashCreate,
        .update = NULL,
        .teardown = NULL
};