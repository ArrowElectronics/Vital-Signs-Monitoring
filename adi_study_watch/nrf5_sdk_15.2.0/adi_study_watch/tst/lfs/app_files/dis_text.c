#include "lcd_driver.h"
#include "dis_driver.h"

void draw_dot(uint8_t x,uint8_t y,uint8_t color)
{
    uint8_t color_bit;
    color_bit = Get_color_bit();
#if 0    
    
    if(DISPLAY_OUT_4BIT == color_bit)
    {
        if(0 == x%2)
        {
            dis_buf[y][x/2] &= 0x0f;
            dis_buf[y][x/2] |= (color << 5);
        }
        else
        {
            dis_buf[y][x/2] &= 0xf0;
            dis_buf[y][x/2] |= (color << 1);
        }
    }
    else if(DISPLAY_OUT_1BIT == color_bit)
    {
        dis_buf[y][x/8] &= ~(0x80 >> (x%8));
        dis_buf[y][x/8] |= ((color&0x01) << (7 - x%8));
    }
    else//3bit color//
    {
    }
#else
    if(DISPLAY_OUT_4BIT == color_bit)
    {
        if(0 == x%2)
        {
            dis_buf[Y_AXIS_MAX-y][(X_AXIS_MAX-x)/2] &= 0xf0;
            dis_buf[Y_AXIS_MAX-y][(X_AXIS_MAX-x)/2] |= (color << 1);
        }
        else
        {
            dis_buf[Y_AXIS_MAX-y][(X_AXIS_MAX-x)/2] &= 0x0f;
            dis_buf[Y_AXIS_MAX-y][(X_AXIS_MAX-x)/2] |= (color << 5);
        }
    }
    else if(DISPLAY_OUT_1BIT == color_bit)
    {
        dis_buf[Y_AXIS_MAX-y][(X_AXIS_MAX-x)/8] &= ~(0x80 >> (7 - x%8));
        dis_buf[Y_AXIS_MAX-y][(X_AXIS_MAX-x)/8] |= ((color&0x01) << (x%8));
    }
    else//3bit color//
    {
    }
#endif
}

void Display_string(const GUI_FONT *font,uint8_t x,uint8_t y,uint8_t color,char *string)
{
    const GUI_FONT_PROP *font_prop;
    uint16_t x_spot,x_len,y_len,x_byte;
    uint16_t offset;
    uint8_t h,l,l_d,l_s;
    char *ptr;

    font_prop = font->p.pProp;
    y_len = font->YSize;
    
    for(ptr = string,x_spot = x;*ptr != '\0';ptr++)
    {
        offset = *ptr - font_prop->First;
        x_len = font_prop->paCharInfo[offset].XSize;
        x_byte = font_prop->paCharInfo[offset].BytesPerLine;
        for(l = 0;l < x_len;l++)
        {
            l_d = l/8;
            l_s = l%8;
            for(h = 0;h < y_len;h++)
            {
                if(0 != (font_prop->paCharInfo[offset].pData[l_d+x_byte*h]&(0x80>>l_s)))
                {
                    draw_dot(x_spot+l,y+h,color);
                }
            }
        }
	x_spot += x_len;   
    }
}

void Display_string_middle(const GUI_FONT *font,uint8_t x,uint8_t y,uint8_t color,char *string)//x为显示字符的中间点。
{
    const GUI_FONT_PROP *font_prop;
    uint16_t x_spot,x_len,y_len;
    uint16_t x_spot_int,x_spot_remain,x_byte;
    uint16_t offset;
    uint8_t h,l,l_d,l_s;
    char *ptr;

    font_prop = font->p.pProp;
    y_len = font->YSize;
    for(x_spot = 0,ptr = string;*ptr != '\0';ptr++)
    {
        offset = *ptr - font_prop->First;
        x_spot += font_prop->paCharInfo[offset].XSize;
    }
    if(x >= x_spot/2)
    {
        x -= x_spot/2;
    }
    else
    {
        return;//错误
    }
    if(y > y_len/2)
    {
        y -= y_len/2;
    }
    else
    {
        return;//错误
    }
    for(ptr = string,x_spot = x;*ptr != '\0';ptr++)
    {
        offset = *ptr - font_prop->First;
        x_len = font_prop->paCharInfo[offset].XSize;
        x_byte = font_prop->paCharInfo[offset].BytesPerLine;
        for(l = 0;l < x_len;l++)
        {
            l_d = l/8;
            l_s = l%8;
            for(h = 0;h < y_len;h++)
            {
                if(0 != (font_prop->paCharInfo[offset].pData[l_d+x_byte*h]&(0x80>>l_s)))
                {
                    draw_dot(x_spot+l,y+h,color);
                }
            }
        }
	x_spot += x_len;   
    }
}

void Display_value_middle(const GUI_FONT *font,uint8_t x,uint8_t y,uint8_t color,int32_t value)
{
    char v_string[12] = {0};
    uint8_t i = 0;
    uint8_t valid_flg = 0;
    if(value < 0)
    {
        v_string[i++] = '-';
        value = 0 - value;
    }
    if(value > 1000000)
    {
        v_string[i++] = value/1000000 + '0';
        valid_flg = 1;
        value = value%1000000;
    }
    else
    {
        if(1 == valid_flg)
        {
            v_string[i++] = '0';
        }
    }
    if(value > 100000)
    {
        v_string[i++] = value/100000 + '0';
        valid_flg = 1;
        value = value%100000;
    }
    else
    {
        if(1 == valid_flg)
        {
            v_string[i++] = '0';
        }
    }
    if(value > 10000)
    {
        v_string[i++] = value/10000 + '0';
        valid_flg = 1;
        value = value%10000;
    }
    else
    {
        if(1 == valid_flg)
        {
            v_string[i++] = '0';
        }
    }
    if(value > 1000)
    {
        v_string[i++] = value/1000 + '0';
        valid_flg = 1;
        value = value%1000;
    }
    else
    {
        if(1 == valid_flg)
        {
            v_string[i++] = '0';
        }
    }
    if(value > 100)
    {
        v_string[i++] = value/100 + '0';
        valid_flg = 1;
        value = value%100;
    }
    else
    {
        if(1 == valid_flg)
        {
            v_string[i++] = '0';
        }
    }
    if(value > 10)
    {
        v_string[i++] = value/10 + '0';
        valid_flg = 1;
        value = value%10;
    }
    else
    {
        if(1 == valid_flg)
        {
            v_string[i++] = '0';
        }
    }
    
    v_string[i++] = value + '0';
    v_string[i++] = '\0';

	Display_string_middle(font,x,y,color,v_string);
}

