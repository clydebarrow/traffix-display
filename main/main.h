#ifndef MAIN_H_
#define MAIN_H_


/* INCLUDES ------------------------------------------------------------------*/

/* MACROS --------------------------------------------------------------------*/

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////// Please update the following configuration according to your LCD spec //////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#define TRAFFIX_LCD_PIXEL_CLOCK_HZ     (6528000)//(10 * 1000 * 1000)

#if defined(CONFIG_TRAFFIX_TARGET_T_EMBED)

#define TRAFFIX_LCD_BK_LIGHT_ON_LEVEL  1
#define TRAFFIX_PIN_NUM_BK_LIGHT       15
#define TRAFFIX_BUTTON_0               0     // button on board
#define TRAFFIX_PIN_NUM_RST            9
#define TRAFFIX_PIN_NUM_CS             10
#define TRAFFIX_PIN_NUM_DATA0          11
#define TRAFFIX_PIN_NUM_PCLK           12
#define TRAFFIX_PIN_NUM_DC             17
// Screen size in pixels
#define TRAFFIX_LCD_H_RES              320
#define TRAFFIX_LCD_V_RES              170
#define PARALLEL_LINES                 16

#elif defined(CONFIG_TRAFFIX_TARGET_T_DISPLAY_S3)
#define TRAFFIX_LCD_BK_LIGHT_ON_LEVEL  1

#define TRAFFIX_PIN_NUM_DATA0          39
#define TRAFFIX_PIN_NUM_DATA1          40
#define TRAFFIX_PIN_NUM_DATA2          41
#define TRAFFIX_PIN_NUM_DATA3          42
#define TRAFFIX_PIN_NUM_DATA4          45
#define TRAFFIX_PIN_NUM_DATA5          46
#define TRAFFIX_PIN_NUM_DATA6          47
#define TRAFFIX_PIN_NUM_DATA7          48

#define TRAFFIX_PIN_RD                 9
#define TRAFFIX_PIN_PWR                15
#define TRAFFIX_PIN_NUM_PCLK           8        //LCD_WR
#define TRAFFIX_PIN_NUM_CS             6
#define TRAFFIX_PIN_NUM_DC             7
#define TRAFFIX_PIN_NUM_RST            5
#define TRAFFIX_PIN_NUM_BK_LIGHT       38
#define TRAFFIX_BUTTON_0               14     // button on board

// Screen size in pixels
#define TRAFFIX_LCD_H_RES              320
#define TRAFFIX_LCD_V_RES              170
#endif

#define TRAFFIX_LCD_BK_LIGHT_OFF_LEVEL !TRAFFIX_LCD_BK_LIGHT_ON_LEVEL
#define LVGL_LCD_BUF_SIZE            (TRAFFIX_LCD_H_RES * TRAFFIX_LCD_V_RES)
// Bit number used to represent command and parameter
#define TRAFFIX_LCD_CMD_BITS           8
#define TRAFFIX_LCD_PARAM_BITS         8

/**
 * Events
 */

#define ARRAY_SIZE(arr) ((sizeof(arr)/sizeof(arr[0])))

/* FUNCTIONS DECLARATION -----------------------------------------------------*/

extern bool buttonPressed(int index);
extern void setBacklightState(bool on);


#endif /* MAIN_H_ */

/**************************  Useful Electronics  ****************END OF FILE***/
