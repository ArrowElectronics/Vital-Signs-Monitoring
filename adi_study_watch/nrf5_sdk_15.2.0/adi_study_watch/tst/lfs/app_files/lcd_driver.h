#ifndef _LCD_DRIVER_H_
#define _LCD_DRIVER_H_

#include <stdint.h>

#define DISPLAY_OUT_4BIT (0x24)
#define DISPLAY_OUT_1BIT (0x22)
#define DISPLAY_OUT_3BIT (0x20)

#define NO_UPDATE_MODE  (0x28)
#define ALL_CLEAR_MODE  (0x08) //low 3 bit can be H or L.
#define DISPLAY_WHILE_COLOR  (0x06)
#define DISPLAY_BLACK_COLOR  (0x04)
#define COLOR_INVERSION_MODE  (0x05)

#define LENGTH_SIZE (208)
#define HIGH_SIZE   (208)

#define X_AXIS_MAX  (LENGTH_SIZE - 1)
#define Y_AXIS_MAX  (HIGH_SIZE - 1)

#define COLOR_BIT   (4) //先暂时用4，后续再尝试使用3

#define COLOR_WHITE (0x03)
#define COLOR_BLACK (0x00)
#define COLOR_DEFAULT (0x02)
extern uint8_t dis_buf[HIGH_SIZE][LENGTH_SIZE/8*COLOR_BIT];

uint8_t Get_color_bit(void);
void Set_color_bit(uint8_t color_bit);

void LCD_disp_on(void);
void LCD_disp_off(void);
#endif


