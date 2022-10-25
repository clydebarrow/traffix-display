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
static lv_style_t titleStyle, tileStyle;

LV_IMG_DECLARE(traffix_logo)
static void splashCreate(lv_obj_t * tile) {
    lv_obj_t *img_logo, * title;

    // setup tile style
    lv_style_init(&tileStyle);
    lv_style_set_bg_color(&tileStyle, lv_color_black());
    lv_obj_remove_style_all(tile);
    lv_obj_add_style(tile, &tileStyle, 0);

    img_logo = lv_img_create(tile);
    lv_img_set_src(img_logo, &traffix_logo);
    lv_obj_align(img_logo, LV_ALIGN_TOP_MID, 0, 0);

    title = lv_label_create(tile);
    lv_style_init(&titleStyle);
    lv_style_set_text_color(&titleStyle,lv_color_white());
    lv_style_set_text_font(&titleStyle,  &lv_font_montserrat_26);
    lv_textarea_add_text(title, "Traffix");
    lv_obj_align(title, LV_ALIGN_BOTTOM_MID, 0, 0);
}

uiState uiSplash = {
        .name = "Splash",
        .setup = splashCreate,
};