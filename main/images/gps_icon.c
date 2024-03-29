#if defined(LV_LVGL_H_INCLUDE_SIMPLE)
#include "lvgl.h"
#else
#include "lvgl/lvgl.h"
#endif


#ifndef LV_ATTRIBUTE_MEM_ALIGN
#define LV_ATTRIBUTE_MEM_ALIGN
#endif

#ifndef LV_ATTRIBUTE_IMG_GPS_ICON
#define LV_ATTRIBUTE_IMG_GPS_ICON
#endif

const LV_ATTRIBUTE_MEM_ALIGN LV_ATTRIBUTE_LARGE_CONST LV_ATTRIBUTE_IMG_GPS_ICON uint8_t gps_icon_map[] = {
  0x00, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x06, 0xe4, 0x00, 0x00, 
  0x00, 0x2f, 0xfe, 0x00, 0x00, 
  0x00, 0x7d, 0x1f, 0x40, 0x00, 
  0x00, 0xf4, 0x07, 0xc0, 0x00, 
  0x00, 0xf0, 0x03, 0xc0, 0x00, 
  0x00, 0xf0, 0x03, 0xc0, 0x00, 
  0x00, 0xb8, 0x0b, 0x80, 0x00, 
  0x00, 0x3f, 0xff, 0x00, 0x00, 
  0x00, 0x2f, 0xfe, 0x00, 0x00, 
  0x00, 0x0f, 0xfc, 0x00, 0x00, 
  0x00, 0x0b, 0xf8, 0x00, 0x00, 
  0x00, 0x03, 0xf0, 0x00, 0x00, 
  0x00, 0x02, 0xe0, 0x00, 0x00, 
  0x00, 0x00, 0xc0, 0x00, 0x00, 
  0x00, 0x00, 0x40, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x00, 0x00, 
};

const lv_img_dsc_t gps_icon = {
  .header.cf = LV_IMG_CF_ALPHA_2BIT,
  .header.always_zero = 0,
  .header.reserved = 0,
  .header.w = 17,
  .header.h = 17,
  .data_size = 85,
  .data = gps_icon_map,
};
