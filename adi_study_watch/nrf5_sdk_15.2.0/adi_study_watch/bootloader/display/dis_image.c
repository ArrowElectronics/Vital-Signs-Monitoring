#include "lcd_driver.h"
#include "dis_driver.h"

inline void draw_dot(uint8_t x,uint8_t y,uint8_t color)
{
    switch(x&0x07)
    {
        case 0:
        {
            dis_buf[y][CMD_OFFSET+((X_AXIS_MAX-x)>>3)].f1bit.pixel1 = color;
        }
        break;
        case 1:
        {
            dis_buf[y][CMD_OFFSET+((X_AXIS_MAX-x)>>3)].f1bit.pixel2 = color;
        }
        break;
        case 2:
        {
            dis_buf[y][CMD_OFFSET+((X_AXIS_MAX-x)>>3)].f1bit.pixel3 = color;
        }
        break;
        case 3:
        {
            dis_buf[y][CMD_OFFSET+((X_AXIS_MAX-x)>>3)].f1bit.pixel4 = color;
        }
        break;
        case 4:
        {
            dis_buf[y][CMD_OFFSET+((X_AXIS_MAX-x)>>3)].f1bit.pixel5 = color;
        }
        break;
        case 5:
        {
            dis_buf[y][CMD_OFFSET+((X_AXIS_MAX-x)>>3)].f1bit.pixel6 = color;
        }
        break;
        case 6:
        {
            dis_buf[y][CMD_OFFSET+((X_AXIS_MAX-x)>>3)].f1bit.pixel7 = color;
        }
        break;
        case 7:
        {
            dis_buf[y][CMD_OFFSET+((X_AXIS_MAX-x)>>3)].f1bit.pixel8 = color;
        }
        break;
        default:break;
    }
}

void Display_image(const lv_img_dsc_t * icon,uint8_t x,uint8_t y)//
{
    uint8_t x_size,y_size,x_byte,bits,pexel;
    uint8_t l,h,l_s,l_m;
    uint16_t l_d;
    uint8_t c_mask,mask,color;
    const uint8_t * iData;
    uint8_t x_spot,y_spot;
    
    if((x>X_AXIS_MAX)||(y>Y_AXIS_MAX))
    {
        return;
    }
    x_spot = x;
    x_size = icon->XSize;
    y_size = icon->YSize;
    x_byte = icon->BytesPerLine;

    bits = icon->BitsPerPixel;
    c_mask = (0xFF << (8-bits));
    iData = icon->pData;
    pexel = 8/bits;
    for(l = 0;l < x_size;l++,x_spot++)
    {
        l_d = l/pexel;
        l_s = l%pexel;
        mask = (c_mask >> (l_s*bits));
        l_m = 8 - bits-l_s*bits;
        for(h = 0,y_spot = y;h < y_size;h++,l_d+=x_byte,y_spot++)
        {
            color = ((iData[l_d]&mask) >> l_m);
            draw_dot(x_spot,y_spot,color);                
        }
    }
}
