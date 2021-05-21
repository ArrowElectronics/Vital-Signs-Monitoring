#include "lcd_driver.h"
#include "dis_driver.h"

void draw_dot(uint8_t x,uint8_t y,uint8_t color);
void Display_image(const GUI_BITMAP * icon,uint8_t x,uint8_t y)//
{
    uint8_t x_size,y_size,x_byte,bits;
    uint8_t l,h,l_d,l_s;
    uint8_t mask,color;

    x_size = icon->XSize;
    y_size = icon->YSize;
    x_byte = icon->BytesPerLine;

    bits = icon->BitsPerPixel;
    mask = (0xFF << (8-bits));
    for(l = 0;l < x_size;l++)
    {
        l_d = l/(8/bits);
        l_s = l%(8/bits);
        for(h = 0;h < y_size;h++)
        {
            color = ((icon->pData[l_d+x_byte*h]&(mask>>(l_s*bits))) >> (8 - bits-l_s*bits));
            draw_dot(x+l,y+h,color);                
        }
    }
}
