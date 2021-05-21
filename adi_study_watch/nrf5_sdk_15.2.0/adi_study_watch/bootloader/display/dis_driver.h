#ifndef _DIS_DRIVER_H_
#define _DIS_DRIVER_H_

#include <stdint.h>
#include "lygl_def.h"

//void Display_string(const GUI_FONT *font,uint8_t x,uint8_t y,uint8_t color,char *string);
//void Display_string_middle(const GUI_FONT *font,uint8_t x,uint8_t y,uint8_t color,char *string);
//void Display_value_middle(const GUI_FONT *font,uint8_t x,uint8_t y,uint8_t color,int32_t value);
void draw_dot(uint8_t x,uint8_t y,uint8_t color);
void Display_image(const lv_img_dsc_t * icon,uint8_t x,uint8_t y);
#endif


