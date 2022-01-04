/**
****************************************************************************
* @file     lygl_graph.c
* @author   ADI
* @version  V0.1
* @date     11-Mar-2021
* @brief    This is the source file used to draw waveform display.
****************************************************************************
* @attention
******************************************************************************
* Copyright (c) 2021 Analog Devices, Inc.  All rights reserved.
*
* Redistribution and use in source and binary forms, with or without
* modification, are permitted provided that the following conditions are met:
* - Redistributions of source code must retain the above copyright notice, this
*   list of conditions and the following disclaimer.
* - Redistributions in binary form must reproduce the above copyright notice,
*   this list of conditions and the following disclaimer in the documentation
*   and/or other materials provided with the distribution.
* - Modified versions of the software must be conspicuously marked as such.
* - This software is licensed solely and exclusively for use with
*   processors/products manufactured by or for Analog Devices, Inc.
* - This software may not be combined or merged with other code in any manner
*   that would cause the software to become subject to terms and conditions
*   which differ from those listed here.
* - Neither the name of Analog Devices, Inc. nor the names of its contributors
*   may be used to endorse or promote products derived from this software
*   without specific prior written permission.
* - The use of this software may or may not infringe the patent rights of one
**   or more patent holders.  This license does not release you from the
*   requirement that you obtain separate licenses from these patent holders to
*   use this software.
*
* THIS SOFTWARE IS PROVIDED BY ANALOG DEVICES, INC. AND CONTRIBUTORS "AS IS"
* AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
* NONINFRINGEMENT, TITLE, MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
* ARE DISCLAIMED. IN NO EVENT SHALL ANALOG DEVICES, INC. OR CONTRIBUTORS BE
* LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, PUNITIVE OR
* CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, DAMAGES ARISING OUT OF
* CLAIMS OF INTELLECTUAL PROPERTY RIGHTS INFRINGEMENT; PROCUREMENT OF
* SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
* INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
* CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
* ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
* POSSIBILITY OF SUCH DAMAGE.
******************************************************************************/
#ifdef ENABLE_WATCH_DISPLAY
#include "lygl_common.h"
#include "lygl.h"

/* ------------------------- Defines  -------------------------------------- */

#define ADJUST_RANGE (3) //!<  Defines the adjust percentage for min,max adjust params

/*------------------------- Private Variables --------------------------------*/

static lygl_graph_param_t m_graph_param;//!<Graph Parameters like coordinates,colors
static uint16_t x_start = 0xffff;       //!<Graph coordinates
static uint16_t y_start = 0xffff;       //!<Graph coordinates
static uint16_t x_end = 0xffff;         //!<Graph coordinates
static uint16_t y_end = 0xffff;         //!<Graph coordinates
static uint32_t y_diff = 0xffff;        //!<Range of y coordinate available for application

/* More range of values will be covered in 32 bit data values */
static uint32_t max_value;              //!<Maximum value
static uint32_t min_value;              //!<Minimum value

static uint32_t max_adjust_value;       //!<Max adjust value
static uint32_t min_adjust_value;       //!<Min adjust value

static uint32_t max_record_value;       //!<Max recorded value
static uint32_t min_record_value;       //!<Min recorded Value

static uint16_t up_adjust_cnt = 0;      //!<Up adjust counter
static uint16_t down_adjust_cnt = 0;    //!<Down adjust counter

static uint16_t adjust_max = 0;         //!<Threshold for adjust counters

/**
  * @brief Draws line from one coordinate to other with the required color
  * @param  Start Coordinate in Horizontal axis X1
  * @param  Start Coordinates Vertical Axis Y1
  * @param  Stop Coordinate in Horizontal axis X2
  * @param  Stop Coordinate in vertical axis Y2
  * @param  Colour of the line
  * @retval none.
  */
void lygl_draw_line(uint8_t x1,uint8_t y1,uint8_t x2,uint8_t y2,uint8_t color)
{
    uint8_t x_d,y_d,x,y,y_f;
    int16_t d,v;
    if((x1>X_AXIS_MAX)||(x2>X_AXIS_MAX)||(y1>Y_AXIS_MAX)||(y2>Y_AXIS_MAX))
    {
        return;//error
    }
    x_d = (x2>=x1)? (x2-x1):(x1-x2);
    y_d = (y2>=y1)? (y2-y1):(y1-y2);

    if(x_d >= y_d)
    {
        if(x2>=x1)
        {
            x=x1;
            y=y1;
            d=y2-y1;
        }
        else
        {
            x=x2;
            y=y2;
            d=y1-y2;
        }
        for(uint8_t i = 0;i <=x_d;i++)
        {
            v = i*d/x_d;
            lygl_draw_dot(x+i,y+v,color);
        }
    }
    else
    {
        if(x2>=x1)
        {
            x=x1;
            y=y1;
            y_f=y2;
        }
        else
        {
            x=x2;
            y=y2;
            y_f=y1;
        }
        if(y_f >= y)
        {
            for(uint8_t i = 0;i <=y_d;i++)
            {
                v = i*x_d/y_d;
                lygl_draw_dot(x+v,y+i,color);
            }
        }
        else
        {
            for(uint8_t i = 0;i <=y_d;i++)
            {
                v = i*x_d/y_d;
                lygl_draw_dot(x+v,y-i,color);
            }
        }
    }
}

/**
  * @brief Initializes display graph structures with application parameters
  * @param  Application graph parameters structure which provides graph coordinates,color
  * @retval none.
  */
void lygl_creat_graph(lygl_graph_param_t *graph_param)
{
    m_graph_param.x0 = graph_param->x0;
    m_graph_param.y0 = graph_param->y0;
    m_graph_param.x1 = graph_param->x1;
    m_graph_param.y1 = graph_param->y1;
    m_graph_param.x_space = graph_param->x_space;
    m_graph_param.y_unit = graph_param->y_unit;
    m_graph_param.wf_color = graph_param->wf_color;
    m_graph_param.y_offset= graph_param->y_offset;
    m_graph_param.bg_color = graph_param->bg_color;

    lygl_draw_rectangle(m_graph_param.x0,m_graph_param.y1,m_graph_param.x1,m_graph_param.y0,m_graph_param.bg_color);

    x_start = m_graph_param.x0;
    y_start = 0xffff;

    min_value = m_graph_param.y_offset;
    y_diff = m_graph_param.y0 - m_graph_param.y1;
    min_adjust_value = min_value + y_diff*m_graph_param.y_unit*ADJUST_RANGE/10;

    max_value = min_value +y_diff*m_graph_param.y_unit;
    max_adjust_value = max_value - y_diff*m_graph_param.y_unit*ADJUST_RANGE/10;

    down_adjust_cnt = 0;
    up_adjust_cnt = 0;
    adjust_max = (m_graph_param.x1 - m_graph_param.x0)/m_graph_param.x_space;//About one page.
}

/**
  * @brief Valid_bits variable added  enables the applications to give information
  *        about data bits to the display functions.This will make the display graph
  *        algorithm flexible for any number of bits data ranging from 0 to 32.
  * @param  Application data
  * @param  Length of application data
  * @param  Valid bits in the application data
  * @retval none.
  */

void lygl_send_graph_data(uint32_t *data,uint16_t len,uint32_t valid_bits)
{
    uint16_t cnt = 0;
    uint32_t value;
    uint8_t up_adjust_flag = 0;//0:not adjust,0xff:adjust
    uint8_t down_adjust_flag = 0;//0:not adjust,0xff:adjust
    uint32_t mid_value = 0;

    for(cnt=0;cnt<len;cnt++)
    {
        x_end = x_start+ m_graph_param.x_space;
        value = ((*(data+cnt)) & valid_bits);

        /* adjust down limit*/
        if(value > min_adjust_value)
        {
            if(0 == down_adjust_cnt)
            {
                min_record_value = value;
            }
            else
            {
                if(value < min_record_value)
                {
                    min_record_value = value;
                }
            }
            if(++down_adjust_cnt > adjust_max)
            {
                down_adjust_cnt = 0;
                down_adjust_flag = 0xff;
                min_value = min_record_value;
            }
        }
        else if(value >= min_value)
        {
            down_adjust_cnt = 0;
        }
        else
        {
                /* Conditional check to make sure that min_value 
                   won't be negative */
                if(value < (y_diff*m_graph_param.y_unit/5))
                  min_value = 0;
                else
                  min_value = value - y_diff*m_graph_param.y_unit/5;

            down_adjust_cnt = 0;
            down_adjust_flag = 0xff;
            //m_graph_param.y_offset = min_value;
        }

        /* adjust up limit*/
        if(value <  max_adjust_value)//in the range;
        {
            if(0 == up_adjust_cnt)
            {
                max_record_value = value;
            }
            else
            {
                if(value > max_record_value)
                {
                    max_record_value = value;
                }
            }
            if(++up_adjust_cnt >=  adjust_max)
            {
                up_adjust_cnt = 0;
                up_adjust_flag = 0xff;
                max_value = max_record_value;
            }
        }
        else if(value <=  max_value)
        {
            up_adjust_cnt = 0;
        }
        else
        {
                /* For a given application max value will only be
                  highest value attained by valid bits,by usage valid_bits
                  variable will have maximum value stored by default*/

                if((value + y_diff*m_graph_param.y_unit/5) > valid_bits)
                  max_value = valid_bits;
                else
                  max_value = value + y_diff*m_graph_param.y_unit/5;
            up_adjust_cnt = 0;
            up_adjust_flag = 0xff;
        }

        if((0 != up_adjust_flag)||(0 != down_adjust_flag))
        {
            if((max_value - min_value) > y_diff)
            {
                m_graph_param.y_unit = (max_value - min_value)/y_diff;
            }
            else
            {
                m_graph_param.y_unit = 1;
                /* If the amplitude unit becomes unity,then the max value
                   will not be valid.As the total range available now
                   (y_diff*m_graph_param.y_unit + min) will be more than max in
                   most of cases*/

                mid_value = (max_value + min_value) / 2;
                /* So adjusting  min and max values depending on midvalue so that
                y_unit = 1 will not hold both values(min,max) close together*/

                if((mid_value - (y_diff/2)) > 0)
                    min_value = mid_value - (y_diff/2);
                else
                    min_value = 0;

                if((mid_value + (y_diff/2)) < valid_bits)
                    max_value = mid_value + (y_diff/2);
                else
                    max_value = valid_bits;
            }
            m_graph_param.y_offset = min_value;
            min_adjust_value = m_graph_param.y_offset + y_diff*m_graph_param.y_unit*ADJUST_RANGE/10;
            max_adjust_value = max_value - y_diff*m_graph_param.y_unit*ADJUST_RANGE/10;
        }

        if(value <  m_graph_param.y_offset)
        {
            value = m_graph_param.y_offset;
        }
        y_end = m_graph_param.y0 - (value-m_graph_param.y_offset)/m_graph_param.y_unit;
        if((y_end < m_graph_param.y1)||(y_end > m_graph_param.y0))
        {
            y_end = m_graph_param.y1;
        }
        if((y_start == 0xffff)&&(x_start == m_graph_param.x0))
        {
            x_start = m_graph_param.x0;
            y_start = y_end;
            continue;
        }
        lygl_draw_rectangle(x_start,m_graph_param.y1,x_end,m_graph_param.y0,m_graph_param.bg_color);
        lygl_draw_line(x_start,y_start,x_end,y_end,m_graph_param.wf_color);
        if(x_end < m_graph_param.x1)
        {
            x_start = x_end;
            y_start = y_end;
        }
        else
        {
            x_start = m_graph_param.x0;
            y_start = 0xffff;
        }
    }
}
#endif