/* INCLUDES ------------------------------------------------------------------*/
#include <sys/cdefs.h>
#include <nvs_flash.h>
#include <esp_event.h>
#include "esp_lcd_panel_io.h"
#include "esp_lcd_panel_vendor.h"
#include "esp_lcd_panel_ops.h"
#include "driver/gpio.h"
#include "esp_err.h"
#include "esp_log.h"
#include "lvgl.h"

#include "main.h"
#include "ui.h"
#include "events.h"
#include "gdltask.h"
#include "wifi_prov.h"
#include "preferences.h"
#include "ownship.h"
#include "flarm.h"
/* PRIVATE STRUCTURES ---------------------------------------------------------*/

/* VARIABLES -----------------------------------------------------------------*/
static const char *TAG = "main";
static lv_disp_draw_buf_t disp_buf; // contains internal graphic buffer(s) called draw buffer(s)
static lv_disp_drv_t disp_drv;      // contains callback functions
static esp_lcd_panel_handle_t panel_handle = NULL;
static bool lcdOn;

/* DEFINITIONS ---------------------------------------------------------------*/

/* MACROS --------------------------------------------------------------------*/

/* PRIVATE FUNCTIONS DECLARATION ---------------------------------------------*/


static bool
notifyLvglFlushReady(esp_lcd_panel_io_handle_t panel_io, esp_lcd_panel_io_event_data_t *edata, void *user_ctx);

static void lvglFlushCb(lv_disp_drv_t *drv, const lv_area_t *area, lv_color_t *color_map);


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

void setBacklightState(bool on) {
    if(on != lcdOn) {
        esp_lcd_panel_disp_on_off(panel_handle, on);
        lcdOn = on;
    }
    gpio_set_level(TRAFFIX_PIN_NUM_BK_LIGHT,
                   on ? TRAFFIX_LCD_BK_LIGHT_ON_LEVEL : TRAFFIX_LCD_BK_LIGHT_OFF_LEVEL);
}

static void initLcd(void) {
    //GPIO configuration
    esp_lcd_panel_io_handle_t io_handle = NULL;
    //ESP_LOGI(TAG, "Turn off LCD backlight");
    gpio_config_t bk_gpio_config = {
            .mode = GPIO_MODE_OUTPUT,
            .pin_bit_mask = 1ULL << TRAFFIX_PIN_NUM_BK_LIGHT
    };
    ESP_ERROR_CHECK(gpio_config(&bk_gpio_config));

    gpio_pad_select_gpio(TRAFFIX_PIN_NUM_BK_LIGHT);
    setBacklightState(false);

#ifdef  CONFIG_TRAFFIX_TARGET_T_EMBED
    spi_bus_config_t buscfg = {
            .sclk_io_num = TRAFFIX_PIN_NUM_PCLK,
            .mosi_io_num = TRAFFIX_PIN_NUM_DATA0,
            .miso_io_num = -1,
            .quadwp_io_num = -1,
            .quadhd_io_num = -1,
            .max_transfer_sz = PARALLEL_LINES * TRAFFIX_LCD_H_RES * 2 + 8
    };
    ESP_ERROR_CHECK(spi_bus_initialize(SPI2_HOST, &buscfg, SPI_DMA_CH_AUTO));
    esp_lcd_panel_io_spi_config_t io_config = {
            .dc_gpio_num = TRAFFIX_PIN_NUM_DC,
            .cs_gpio_num = TRAFFIX_PIN_NUM_CS,
            .pclk_hz = TRAFFIX_LCD_PIXEL_CLOCK_HZ,
            .lcd_cmd_bits = TRAFFIX_LCD_CMD_BITS,
            .lcd_param_bits = TRAFFIX_LCD_PARAM_BITS,
            .spi_mode = 0,
            .trans_queue_depth = 10,
    };
    ESP_ERROR_CHECK(esp_lcd_new_panel_io_spi((esp_lcd_spi_bus_handle_t) SPI2_HOST, &io_config, &io_handle));
    esp_lcd_panel_dev_config_t panel_config =
            {
                    .reset_gpio_num = TRAFFIX_PIN_NUM_RST,
                    .color_space = ESP_LCD_COLOR_SPACE_RGB,
                    .bits_per_pixel = 16,
            };

    ESP_ERROR_CHECK(esp_lcd_new_panel_st7789(io_handle, &panel_config, &panel_handle));
#endif
#ifdef CONFIG_TRAFFIX_TARGET_T_DISPLAY_S3
    gpio_pad_select_gpio(TRAFFIX_PIN_RD);
    gpio_pad_select_gpio(TRAFFIX_PIN_PWR);

    gpio_set_direction(TRAFFIX_PIN_RD, GPIO_MODE_OUTPUT);
    gpio_set_direction(TRAFFIX_PIN_PWR, GPIO_MODE_OUTPUT);

    gpio_set_level(TRAFFIX_PIN_RD, true);
    gpio_set_level(TRAFFIX_PIN_PWR, true);

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
    esp_lcd_panel_dev_config_t panel_config =
            {
                    .reset_gpio_num = TRAFFIX_PIN_NUM_RST,
                    .rgb_endian = LCD_RGB_ENDIAN_RGB,
                    .bits_per_pixel = 16,
            };

    ESP_ERROR_CHECK(esp_lcd_new_panel_st7789(io_handle, &panel_config, &panel_handle));
#endif
    ESP_LOGI(TAG, "REset panel");
    esp_lcd_panel_reset(panel_handle);
    esp_lcd_panel_init(panel_handle);
    esp_lcd_panel_invert_color(panel_handle, true);

    esp_lcd_panel_swap_xy(panel_handle, true);
    esp_lcd_panel_mirror(panel_handle, false, true);
    // the gap is LCD panel specific, even panels with the same driver IC, can have different gap value
    esp_lcd_panel_set_gap(panel_handle, 0, 35);
    ESP_LOGI(TAG, "InitLCD done");
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
    lv_disp_drv_register(&disp_drv);

    lv_obj_clean(lv_scr_act());
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

bool isButtonPressed(int index) {
    return gpio_get_level(buttonGpios[index]) == 0;
}

/**
 * The application code task.
 */


static void mainTask(void *param) {
    ESP_LOGI(TAG, "Main task starts");
    initEvents();
    initGraphics();
    initTraffic();
    initOwnship();
    gdlInit();
    flarmInit();
    ESP_LOGI(TAG, "Start UI loop");
    uiLoop();
}

/**
 * STartup task, runs on core 0.
 */
void app_main(void) {
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    initNvs();
    initPrefs();
    initLcd();
    initGpio();
    initWiFi();
    xTaskCreatePinnedToCore(mainTask, "Main", 12000, NULL, 4, NULL, 1);
}
