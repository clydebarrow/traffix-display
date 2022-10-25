#include <esp_types.h>
#include <sys/cdefs.h>
/**
 ******************************************************************************
 * @Channel Link    :  https://www.youtube.com/user/wardzx1
 * @file    		:  main1.c
 * @author  		:  Ward Almasarani - Useful Electronics
 * @version 		:  v.1.0
 * @date    		:  Aug 20, 2022
 * @brief   		:
 *
 ******************************************************************************/


/* INCLUDES ------------------------------------------------------------------*/
#include <nvs_flash.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_timer.h"
#include "esp_lcd_panel_io.h"
#include "esp_lcd_panel_vendor.h"
#include "esp_lcd_panel_ops.h"
#include "driver/gpio.h"
#include "esp_err.h"
#include "esp_log.h"
#include "lvgl.h"

#include "main.h"
#include "ui.h"
/* PRIVATE STRUCTURES ---------------------------------------------------------*/

/* VARIABLES -----------------------------------------------------------------*/
static const char *TAG = "main";
static lv_disp_draw_buf_t disp_buf; // contains internal graphic buffer(s) called draw buffer(s)
static lv_disp_drv_t disp_drv;      // contains callback functions
static esp_lcd_panel_handle_t panel_handle = NULL;
static lv_disp_t *disp;

/* DEFINITIONS ---------------------------------------------------------------*/

/* MACROS --------------------------------------------------------------------*/

/* PRIVATE FUNCTIONS DECLARATION ---------------------------------------------*/


static void increaseLvglTick(void *arg);

static bool
notifyLvglFlushReady(esp_lcd_panel_io_handle_t panel_io, esp_lcd_panel_io_event_data_t *edata, void *user_ctx);

static void lvglFlushCb(lv_disp_drv_t *drv, const lv_area_t *area, lv_color_t *color_map);


_Noreturn static void lvglTimerTask(void *param);

/* FUNCTION PROTOTYPES -------------------------------------------------------*/


static bool
notifyLvglFlushReady(esp_lcd_panel_io_handle_t panel_io, esp_lcd_panel_io_event_data_t *edata, void *user_ctx) {
    lv_disp_drv_t *disp_driver = (lv_disp_drv_t *) user_ctx;
    lv_disp_flush_ready(disp_driver);
    return false;
}

static void lvglFlushCb(lv_disp_drv_t *drv, const lv_area_t *area, lv_color_t *color_map) {
    int offsetx1 = area->x1;
    int offsetx2 = area->x2;
    int offsety1 = area->y1;
    int offsety2 = area->y2;
    // copy a buffer's content to a specific area of the display
    esp_lcd_panel_draw_bitmap(panel_handle, offsetx1, offsety1, offsetx2 + 1, offsety2 + 1, color_map);
}

static void increaseLvglTick(void *arg) {
    /* Tell LVGL how many milliseconds has elapsed */
    lv_tick_inc(TRAFFIX_LVGL_TICK_PERIOD_MS);
}

// A task to call the LVGL timer periodically.
_Noreturn static void lvglTimerTask(void *param) {
    static uint64_t lastTime = 0;
    for (;;) {
        // The task running lv_timer_handler should have lower priority than that running `lv_tick_inc`
        uint64_t nowTime = esp_timer_get_time();
        // calculate elapsed time,
        uint32_t elapsed = ((nowTime - lastTime) & 0xFFFFFFFF) / 1000;
        lv_tick_inc(elapsed);
        lastTime = nowTime;
        uint32_t  next = lv_timer_handler();
        if(next > 200)
            next = 200;     // don't sleep for more than 200ms

        vTaskDelay(next / portTICK_PERIOD_MS);
    }
}

/**
 * Initialise the non volatile memory
 */

static void initNvs() {
    /* Initialize NVS partition */
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        /* NVS partition was truncated
         * and needs to be erased */
        ESP_ERROR_CHECK(nvs_flash_erase());

        /* Retry nvs_flash_init */
        ESP_ERROR_CHECK(nvs_flash_init());
    }
}


/**
 * @brief Program starts from here
 *
 */
static void initLcd(void) {
    //GPIO configuration
    ESP_LOGI(TAG, "Turn off LCD backlight");
    gpio_config_t bk_gpio_config = {
            .mode = GPIO_MODE_OUTPUT,
            .pin_bit_mask = 1ULL << TRAFFIX_PIN_NUM_BK_LIGHT
    };
    ESP_ERROR_CHECK(gpio_config(&bk_gpio_config));

    gpio_pad_select_gpio(TRAFFIX_PIN_NUM_BK_LIGHT);
    gpio_pad_select_gpio(TRAFFIX_PIN_RD);
    gpio_pad_select_gpio(TRAFFIX_PIN_PWR);

    gpio_set_direction(TRAFFIX_PIN_RD, TRAFFIX_PIN_NUM_BK_LIGHT);
    gpio_set_direction(TRAFFIX_PIN_RD, GPIO_MODE_OUTPUT);
    gpio_set_direction(TRAFFIX_PIN_PWR, GPIO_MODE_OUTPUT);

    gpio_set_level(TRAFFIX_PIN_RD, true);
    gpio_set_level(TRAFFIX_PIN_NUM_BK_LIGHT, TRAFFIX_LCD_BK_LIGHT_OFF_LEVEL);

    ESP_LOGI(TAG, "Initialize Intel 8080 bus");
    esp_lcd_i80_bus_handle_t i80_bus = NULL;
    esp_lcd_i80_bus_config_t bus_config = {
            .dc_gpio_num = TRAFFIX_PIN_NUM_DC,
            .wr_gpio_num = TRAFFIX_PIN_NUM_PCLK,
            .clk_src    = LCD_CLK_SRC_PLL160M,
            .data_gpio_nums =
                    {
                            TRAFFIX_PIN_NUM_DATA0,
                            TRAFFIX_PIN_NUM_DATA1,
                            TRAFFIX_PIN_NUM_DATA2,
                            TRAFFIX_PIN_NUM_DATA3,
                            TRAFFIX_PIN_NUM_DATA4,
                            TRAFFIX_PIN_NUM_DATA5,
                            TRAFFIX_PIN_NUM_DATA6,
                            TRAFFIX_PIN_NUM_DATA7,
                    },
            .bus_width = 8,
            .max_transfer_bytes = LVGL_LCD_BUF_SIZE * sizeof(uint16_t)
    };
    ESP_ERROR_CHECK(esp_lcd_new_i80_bus(&bus_config, &i80_bus));
    esp_lcd_panel_io_handle_t io_handle = NULL;
    esp_lcd_panel_io_i80_config_t io_config = {
            .cs_gpio_num = TRAFFIX_PIN_NUM_CS,
            .pclk_hz = TRAFFIX_LCD_PIXEL_CLOCK_HZ,
            .trans_queue_depth = 20,
            .dc_levels = {
                    .dc_idle_level = 0,
                    .dc_cmd_level = 0,
                    .dc_dummy_level = 0,
                    .dc_data_level = 1,
            },
            .on_color_trans_done = notifyLvglFlushReady,
            .user_ctx = &disp_drv,
            .lcd_cmd_bits = TRAFFIX_LCD_CMD_BITS,
            .lcd_param_bits = TRAFFIX_LCD_PARAM_BITS,
    };
    ESP_ERROR_CHECK(esp_lcd_new_panel_io_i80(i80_bus, &io_config, &io_handle));

    ESP_LOGI(TAG, "Install LCD driver of st7789");

    esp_lcd_panel_dev_config_t panel_config =
            {
                    .reset_gpio_num = TRAFFIX_PIN_NUM_RST,
                    .color_space = ESP_LCD_COLOR_SPACE_RGB,
                    .bits_per_pixel = 16,
            };

    ESP_ERROR_CHECK(esp_lcd_new_panel_st7789(io_handle, &panel_config, &panel_handle));

    esp_lcd_panel_reset(panel_handle);
    esp_lcd_panel_init(panel_handle);
    esp_lcd_panel_invert_color(panel_handle, true);

    esp_lcd_panel_swap_xy(panel_handle, true);
    esp_lcd_panel_mirror(panel_handle, false, true);
    // the gap is LCD panel specific, even panels with the same driver IC, can have different gap value
    esp_lcd_panel_set_gap(panel_handle, 0, 35);
}

static void initGraphics() {
    ESP_LOGI(TAG, "Initialize LVGL library");
    lv_init();
    // alloc draw buffers used by LVGL
    // it's recommended to choose the size of the draw buffer(s) to be at least 1/10 screen sized
    lv_color_t *buf1 = heap_caps_malloc(LVGL_LCD_BUF_SIZE * sizeof(lv_color_t), MALLOC_CAP_DMA);
    assert(buf1);
//    lv_color_t *buf2 = heap_caps_malloc(LVGL_LCD_BUF_SIZE * sizeof(lv_color_t), MALLOC_CAP_DMA );
//    assert(buf2);
    // initialize LVGL draw buffers
    lv_disp_draw_buf_init(&disp_buf, buf1, NULL, LVGL_LCD_BUF_SIZE);

    ESP_LOGI(TAG, "Register display driver to LVGL");
    lv_disp_drv_init(&disp_drv);
    disp_drv.hor_res = TRAFFIX_LCD_H_RES;
    disp_drv.ver_res = TRAFFIX_LCD_V_RES;
    disp_drv.flush_cb = lvglFlushCb;
    disp_drv.draw_buf = &disp_buf;
    disp_drv.user_data = panel_handle;
    disp = lv_disp_drv_register(&disp_drv);

    lv_obj_clean(lv_scr_act());
    // start the animation timer on core 1 (APP)
    xTaskCreatePinnedToCore(lvglTimerTask, "lvgl Timer", 10000, NULL, 4, NULL, 1);
}

static int buttonGpios[] = {
        TRAFFIX_BUTTON_0,
};

static void initGpio() {
    ESP_LOGI(TAG, "Init GPIO");
    for (int i = 0; i != ARRAY_SIZE(buttonGpios); i++)
        gpio_set_direction(buttonGpios[i], GPIO_MODE_INPUT);
}

/**
 * Get the status of a button. All are assumed to be active low.
 * @param index  The button number
 * @return true if the button is pressed.
 */

bool buttonPressed(int index) {
    return gpio_get_level(buttonGpios[index]) == 0;
}

/**
 * The application code task.
 */


_Noreturn static void mainTask(void *param) {
    ESP_LOGI(TAG, "Main task starts");
    initGraphics();
    provisionWiFi(buttonPressed(0)); // does not return until WiFi connected.
    ESP_LOGI(TAG, "Display LVGL animation");
    lv_obj_t *scr = lv_disp_get_scr_act(disp);
    uiLoop(scr);
}

void app_main(void) {
    initNvs();
    initLcd();
    initGpio();
    initWiFi();
    xTaskCreatePinnedToCore(mainTask, "Main", 16384, NULL, 4, NULL, 1);
}
/**************************  Useful Electronics  ****************END OF FILE***/