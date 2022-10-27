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

static const char * TAG = "Splash";

LV_IMG_DECLARE(traffix_logo)

static void splashCreate(lv_obj_t * tile) {
    ESP_LOGI(TAG, "create splash");
    static lv_obj_t *logoImage;

    /*
    static lv_obj_t * nameLabel;
    static lv_style_t titleStyle;
    lv_style_init(&titleStyle);
    lv_style_set_text_font(&titleStyle, &lv_font_montserrat_2);
    lv_style_set_bg_color(&titleStyle, lv_color_black());
    lv_style_set_text_color(&titleStyle, lv_color_make(0, 0, 0xFF));
    lv_style_set_text_align(&titleStyle, LV_TEXT_ALIGN_CENTER);
    nameLabel = lv_label_create(tile);
    lv_obj_add_style(nameLabel, &titleStyle, 0);
    lv_label_set_text(nameLabel, "TraffiX");
    lv_obj_align(nameLabel, LV_ALIGN_BOTTOM_MID, 0, 0);
     */
    logoImage = lv_img_create(tile);
    lv_img_set_src(logoImage, &traffix_logo);
    lv_obj_align(logoImage, LV_ALIGN_TOP_MID, 0, 0);

    ESP_LOGI(TAG, "Splash image added");
    /*
    title = lv_label_create(tile);
    lv_style_init(&titleStyle);
    lv_style_set_text_color(&titleStyle,lv_color_white());
    lv_style_set_text_font(&titleStyle,  &lv_font_montserrat_26);
    lv_obj_add_style(title, &titleStyle, 0);
    lv_textarea_set_text(title, "Traffix");
    lv_obj_align(title, LV_ALIGN_BOTTOM_MID, 0, 0); */
}

const uiScreen_t uiSplash = {
        .name = "Splash",
        .setup = splashCreate,
        .update = NULL,
        .teardown = NULL
};