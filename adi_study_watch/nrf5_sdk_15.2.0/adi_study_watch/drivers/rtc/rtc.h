/**
****************************************************************************
* @file     rtc.h
* @author   ADI
* @version  V0.1
* @date     20-June-2020
* @brief    This header file is used to implement rtc functions
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

#ifndef _RTC_H_
#define _RTC_H_

#include "stdint.h"
#include "m_time.h"
#include "adi_calendar.h"



/*No: of entries in RTC file within FDS which are stored/retreived */
#define FDS_RTC_ENTRIES (3) //current_rtc count, current_timestamp and current_timezone- 3 entries are stored

#define NUMBER_OF_TICKS_FOR_24_HOUR (uint32_t)(2831155200)  //computed using this formula 24 * 3600 * (32768 ticks/sec)

#define MAX_RTC_TICKS_FOR_24_HOUR (uint32_t)(2764800000) /* = (NUMBER_OF_TICKS_FOR_24_HOUR*1000)<<10) */ //for 32KHz ticks resolution

#define SB_RTC_DEFAULT_CONFIG \
  {                             \
    .tm_year = 2019,            \
    .tm_mon  = 10,            \
    .tm_mday = 15,            \
    .tm_hour = 8,            \
    .tm_min  = 0,             \
    .tm_sec  = 0,             \
  }
  
typedef enum{
    RTC_PRESCALER_1 = 0,//accuracy = 30.51us.LCLK = 32.768kHz
    RTC_PRESCALER_2 = 1,//accuracy = 61.03us
    RTC_PRESCALER_4 = 3,//accuracy = 122.07us
    RTC_PRESCALER_8 = 7,//accuracy = 244.14us
    RTC_PRESCALER_16 = 15,//accuracy = 488.28us
    RTC_PRESCALER_32 = 31,//accuracy = 976.56us
    RTC_PRESCALER_64 = 63,//accuracy = 1953.12us
    RTC_PRESCALER_128 = 127,//accuracy = 3906.25us
    RTC_PRESCALER_4096 = 4095,//accuracy = 125ms
}rtc_prescaler_option_t;

extern char *week_str[];
extern char *month_str[];
/*
* @brief  rtc initialize.
*/
void rtc_init(void);
/*
* @brief  get the current date time.
* @return tm* refer to tm struction.
*/
m_time_struct *rtc_date_time_get(void);
/*
* @brief  get the current date time..

* @param[out]: 
*   @timezone: precision is second
*/
void rtc_timezone_get(int32_t *timezone);
/*
* @brief  get the current microsecond..

* @return:represent microsecond,accuracy = 122.07us
*/
uint32_t rtc_microsecond_get(void);
/*
* @brief  set the current date time.

* @param[in]:
*   @timestamp: since 2000
*/
void rtc_timestamp_set(uint32_t timestamp);
/*
* @brief  set the current timezone.

* @param[in]:
*   @timezone: precision is second
*/
void rtc_timezone_set(int32_t timezone);
/*
* @brief  store current date time.
* @input
    offset:offset time= the time from time store to reset. precision = 122us=(1<<13)s
* @note: only can be called before MCU reset
*/
void rtc_timestamp_store(uint32_t offset);

/*
* @brief  restore current date time.
* @input
    offset:offset time = the time from start to time restore. precision = 122us=(1<<13)s
*/
void rtc_timestamp_restore(uint32_t offset);

/*
* @brief  returns 32Khz clock tick counts; counter overflows after 35 hours

*/
uint32_t get_sensor_time_stamp(void);


/*
* @brief  Provides system time in 1hz resolution; 

*/

uint32_t get_log_time_stamp(time_t *timestamp,int16_t *timezone_offset);

/* 
  *  @brief: Function to get the current timestamp in ms resolution, by the sensor tasks
*/
uint32_t get_ms_time_stamp(void);

#endif
