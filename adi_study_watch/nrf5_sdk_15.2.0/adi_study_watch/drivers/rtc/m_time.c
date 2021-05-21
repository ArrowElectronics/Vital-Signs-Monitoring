/**
****************************************************************************
* @file     m_time.c
* @author   ADI
* @version  V0.1
* @date     20-June-2020
* @brief    This source file is used to implement time conversion functions
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
*   or more patent holders.  This license does not release you from the
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

#include "m_time.h"

#define M_BASE_YEAR 2000

#define MONTH_PER_YEAR 12
#define DAY_PER_YEAR 365
#define SEC_PER_DAY 86400
#define SEC_PER_HOUR 3600
#define SEC_PER_MIN 60

const uint8_t g_day_per_mon[MONTH_PER_YEAR] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};

/*
* @brief  judge whether was leap year?

* @param[in]: 
*   @year: 
* @return:1:this year is leap year, 0:this year is not leap year.

*/
static uint8_t is_leap_year(uint16_t year)
{
    if(((year % 4 == 0) && (year % 100 != 0)) || (year % 400 == 0))
    {
        return 1;
    }
    else
    {
        return 0;
    }   
}

/*
* @brief  get the number of days in one month

* @param[in]: 
*   @month: 
*   @year: 
* @return:number of days

*/
static uint8_t day_of_mon(uint8_t month, uint16_t year)
{
    if (month != 2) 
    {
        return g_day_per_mon[month - 1];
    } 
    else 
    {
        return g_day_per_mon[1] + is_leap_year(year);
    }
}
/*
* @brief  return day of the week.

* @param[in]: 
*   @year: 
*   @month: 
*   @day:
* @return:day of the week

*/
static uint8_t day_to_week(uint16_t year, uint8_t month, uint8_t day)
{
    if ((month == 1) || (month == 2)) 
    { //month  13,14,3,...12
        year--;
        month += 12;
    }
    return ((day+2*month+3*(month+1)/5+year+year/4-year/100+year/400 + 1)%7);
}

m_time_struct * m_sec_to_date_time(uint32_t timestamp)
{
    static m_time_struct date_time;
    uint32_t sec, day;
    uint16_t y;
    uint8_t m;
    uint16_t d;    

    /* hour */    
    sec = timestamp % SEC_PER_DAY;
    date_time.tm_hour = sec / SEC_PER_HOUR;

    /* min */
    sec %= SEC_PER_HOUR;
    date_time.tm_min = sec / SEC_PER_MIN;

    /* sec */
    date_time.tm_sec = sec % SEC_PER_MIN;
   
    /* year, month, day */
    day = timestamp / SEC_PER_DAY;
    for (y = M_BASE_YEAR; day > 0; y++) 
    {
        d = (DAY_PER_YEAR + is_leap_year(y));
        if (day >= d)
        {
            day -= d;
        }
        else
        {
            break;
        }
    }
    date_time.tm_year = y;

    for (m = 1; m < MONTH_PER_YEAR; m++) 
    {
        d = day_of_mon(m, y);
        if (day >= d) 
        {
            day -= d;
        } 
        else 
        {
            break;
        }
    }

    date_time.tm_mon = m;
    date_time.tm_mday = (uint8_t) (day + 1);

    date_time.tm_wday = day_to_week(date_time.tm_year, date_time.tm_mon, date_time.tm_mday);
    return (&date_time);
}

uint32_t m_date_time_to_sec(m_time_struct *currTime)
{
    uint16_t year;
    uint8_t month;
    uint32_t no_of_days = 0;
    uint32_t timestamp;

    if (currTime->tm_year < M_BASE_YEAR) 
    {
        return 0;
    }

    /* year */
    for (year = M_BASE_YEAR; year < currTime->tm_year; year++) 
    {
        no_of_days += (DAY_PER_YEAR + is_leap_year(year));
    }

    /* month */
    for (month = 1; month < currTime->tm_mon; month++) 
    {
        no_of_days += day_of_mon(month, currTime->tm_year);
    }

    /* day */
    no_of_days += (currTime->tm_mday - 1);

    /* sec */
    timestamp =  no_of_days * SEC_PER_DAY + currTime->tm_hour * SEC_PER_HOUR 
                + currTime->tm_min * SEC_PER_MIN + currTime->tm_sec;

    return timestamp;
}
