/**
****************************************************************************
* @file     rtc.c
* @author   ADI
* @version  V0.1
* @date     20-June-2020
* @brief    This source file is used to implement rtc functions
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

#include "rtc.h"
#include <stdio.h>
#include "nrf_drv_rtc.h"
#include "fds_drv.h"
#include "fds.h"
#include "nrf_drv_clock.h"

#include "nrf_log.h"
#if NRF_LOG_ENABLED
#include "nrf_log_ctrl.h"
#include "nrf_log_default_backends.h"
#endif

char *week_str[] = {
    "Sun",
    "Mon",
    "Tues",//"Tue"
    "Wed",
    "Thur",
    "Fri",
    "Sat",
};

char *month_str[] = {
    "",
    "Jan",
    "Feb",
    "Mar",
    "Apr",
    "May",
    "Jun",
    "Jul",
    "Aug",
    "Sep",
    "Oct",
    "Nov",
    "Dec"
};

static uint32_t current_time = 0; //holds current system time in secs resolution
static int32_t  current_timezone = 0; //holds current system timezone in secs resolution
static uint32_t microsecond = 0;
static uint64_t gs_current_time = 0; //holds current system time in 32kHz resolution
#ifdef ENABLE_DEBUG_STREAM
#define M2M2_DEBUG_INFO_64_SIZE   4
#define DEBUG_INFO_64_SIZE    M2M2_DEBUG_INFO_64_SIZE + 6 // 6 here to avoid overflow   
static uint64_t gs_rtc_roll_over_handle_counter = 0; //number of time RTC roll over handled during the RTC ISR block
uint8_t g_adpd_ts_flag_set = 0;
uint8_t g_adpd_rtc_info_buffer_index = 0;
uint64_t g_adpd_rtc_info[DEBUG_INFO_64_SIZE];
#endif
/**
 * @brief RTC instance
 */
static nrf_drv_rtc_t const m_rtc = NRF_DRV_RTC_INSTANCE(2);

#ifdef CUST4_SM
#include "user0_config_app_task.h"
/* Variable to be used, to continue raising compare interrupts, since more
 * compares are required to reach the requested RTC wakeup timeout
 */
static uint8_t gn_cont_comp_int = false;
/* Variable to hold the remaining compare value, to  reach the requested
 * RTC wakeup timeout
*/
static uint32_t gn_pending_comp_value = 0;
/* Variable to keep count of no: of compare interrupts */
static uint8_t gn_comp_int_count = 0;

/*
 * Maximum secs that can be counted with RTC_PRESCALER_1, 30.51us resolution,
 * after which overflow happens
*/
#define MAX_SECS_COMPARED (512)

uint32_t get_rtc_wakeup_time_to_be_configured(uint32_t sleep_time);
#endif

static void rtc_handler(nrf_drv_rtc_int_type_t int_type)
{
    if(NRFX_RTC_INT_OVERFLOW == int_type)
    {
        microsecond += (1<<24);
        current_time += (microsecond >> 15);
        microsecond &= 0x7FFF;
        gs_current_time += (1<<24); //increment it by number of 32Khz ticks elapsed in 24 bit wide register
    }
#ifdef CUST4_SM
    if(NRF_DRV_RTC_INT_COMPARE0 ==  int_type)
    {
      gn_comp_int_count++;
      /*Still more comparisons required to reach requested RTC wakeup timeout*/
      if(gn_cont_comp_int)
      {
        uint32_t compare_val;

        if(gn_pending_comp_value > MAX_SECS_COMPARED)
        {
          compare_val = get_rtc_wakeup_time_to_be_configured(MAX_SECS_COMPARED);
          gn_pending_comp_value -= MAX_SECS_COMPARED;
          gn_cont_comp_int = true;
        }
        else
        {
          compare_val = get_rtc_wakeup_time_to_be_configured(gn_pending_comp_value);
          gn_pending_comp_value = 0;
          gn_cont_comp_int = false;
        }
        nrfx_rtc_cc_set(&m_rtc,NRFX_RTC_INT_COMPARE0,compare_val,true);
      }
      /*Reached the compare value requested*/
      else
      {
        //Register the event received
        set_user0_config_app_event(EVENT_RTC_TIMER_INTERRUPT);
        rtc_timestamp_store(320);
        NVIC_SystemReset();
      }
    }
#endif
}

void rtc_init(void)
{
    ret_code_t err_code;
    static nrf_drv_rtc_config_t m_rtc_config = NRF_DRV_RTC_DEFAULT_CONFIG;
    m_rtc_config.prescaler = RTC_PRESCALER_1;//ticks at 32Khz resolution;

    err_code = nrf_drv_rtc_init(&m_rtc, &m_rtc_config, rtc_handler);//can count 24 day.
    if (err_code != NRF_SUCCESS)
    {
        NRF_LOG_INFO("rtc init failure!");
        return;
    }
    rtc_timestamp_restore(0xA000);
    nrf_drv_rtc_overflow_enable(&m_rtc,true);
//    nrf_drv_rtc_tick_enable(&m_rtc,true);
//    nrfx_rtc_cc_set(&m_rtc,NRFX_RTC_INT_COMPARE0,480000,true);
    nrf_drv_rtc_enable(&m_rtc);
}

#ifdef CUST4_SM
/*!
 ****************************************************************************
 * @brief  Function to get the wakeup time which has to be configured in RTC
 * @desc   This function takes sleep_time in secs, finds out the compare time
 *         which has to be configured in the RTC, so that RTC compare interrupt
 *         can be raised. Its based on this that Watch needs to be awakened
 *         This function will be further called from RTC callback handler,
 *         to continue setting the compare value, till requested sleep_time is
 *         reached.
 * @param  sleep_time: specify the absolute time in seconds, after poweron,
           at which rtc wakeup needs to happen
 * @return wakeup time in secs
 ******************************************************************************/
uint32_t get_rtc_wakeup_time_to_be_configured(uint32_t sleep_time)
{
    static volatile uint64_t rtc_cnt;
    static volatile uint64_t sleep_time_ticks;
    static volatile uint32_t total_ticks;

    rtc_cnt = nrf_drv_rtc_counter_get(&m_rtc);//Current Watch operation time
    sleep_time_ticks = (sleep_time << 15);    //Expressing seconds in terms of 32Khz clock ticks
    /*Return Wakeup time in ticks which is equal to total of time watch has been
      in operation + sleep time required */
    total_ticks =  (rtc_cnt + sleep_time_ticks);

    return total_ticks;
}

/*!
 ****************************************************************************
 * @brief  Function to set the compare value in RTC to get NRF_DRV_RTC_INT_COMPARE0
           interrupt type
 * @desc   This function takes in the value in seconds, which starts from NOW,
 *         afer which the RTC compare interrupt should be triggered and after
 *         which the Watch needs to be awakened
 * @param  value: specify the time in seconds at which rtc wakeup needs to happen
 *         and Watch needs to poweron.
 * @return None
 ******************************************************************************/
void enable_rtc_wakeup(uint32_t value)
{
  uint32_t compare_val;

  gn_comp_int_count = 0;
  /*With RTC_PRESCALER_1, 30.5us counter resolution, overflow happens in 512 secs
    So for wakeup greater than 512 sec, NRFX_RTC_INT_COMPARE0 needs to be split and
    set.
  */
  if(value > MAX_SECS_COMPARED)
  {
    compare_val = get_rtc_wakeup_time_to_be_configured(MAX_SECS_COMPARED);
    gn_pending_comp_value = value - MAX_SECS_COMPARED;
    gn_cont_comp_int = true;
  }
  else
  {
    compare_val = get_rtc_wakeup_time_to_be_configured(value);
    gn_pending_comp_value = 0;
    gn_cont_comp_int = false;
  }
  nrfx_rtc_cc_set(&m_rtc,NRFX_RTC_INT_COMPARE0,compare_val,true);
}
#endif

/*
  *  @brief: Function to set the current timestamp of the system
  *  @timestamp: total of date, time and timezone in secs
*/
void rtc_timestamp_set(uint32_t timestamp)
{
    nrf_drv_rtc_counter_clear(&m_rtc);

    current_time = timestamp;
    microsecond = 0;
    gs_current_time = timestamp;
    gs_current_time = (gs_current_time << 15);  // Expressing seconds in terms of 32Khz clock ticks
}

/*
  *  @brief: Function to get the current timestamp in 32kHz ticks resolution, by the sensors for packet TS
*/
uint32_t get_sensor_time_stamp(void)
{
    uint64_t rtc_cnt;
    uint64_t time_ticks = 0;
#ifdef ENABLE_DEBUG_STREAM
    uint64_t rtc_cnt_capture;
#endif
    rtc_cnt = nrf_drv_rtc_counter_get(&m_rtc);
#ifdef ENABLE_DEBUG_STREAM
    rtc_cnt_capture = rtc_cnt;
#endif     
    time_ticks = gs_current_time + rtc_cnt + (current_timezone << 15);
    /* condition here to handle the RTC overflow in case of RTC ISR is not served at this time due to priority*/
    if (nrf_drv_rtc_int_is_enabled(m_rtc.p_reg,NRF_RTC_INT_OVERFLOW_MASK) &&
        nrf_drv_rtc_event_pending(m_rtc.p_reg, NRF_RTC_EVENT_OVERFLOW)) {
#ifdef ENABLE_DEBUG_STREAM
      gs_rtc_roll_over_handle_counter++;
#endif      
      time_ticks += (1<<24); //increment it by number of 32Khz ticks elapsed in 24 bit wide register
    }
    rtc_cnt = time_ticks % NUMBER_OF_TICKS_FOR_24_HOUR;
    rtc_cnt = (rtc_cnt * 1000) >> 10;
#ifdef ENABLE_DEBUG_STREAM
    if(g_adpd_ts_flag_set) {
      g_adpd_ts_flag_set = 0;
      g_adpd_rtc_info_buffer_index = 0;
      g_adpd_rtc_info[g_adpd_rtc_info_buffer_index++] = rtc_cnt_capture;
      g_adpd_rtc_info[g_adpd_rtc_info_buffer_index++] = time_ticks;
      g_adpd_rtc_info[g_adpd_rtc_info_buffer_index++] = rtc_cnt;
      g_adpd_rtc_info[g_adpd_rtc_info_buffer_index++] = gs_rtc_roll_over_handle_counter;
    }
#endif
    return ((uint32_t)rtc_cnt);
}

/*
  *  @brief: Function to get the current timestamp in ms resolution, by the sensor tasks
*/
uint32_t get_ms_time_stamp(void)
{
    uint64_t rtc_cnt;
    uint64_t time_ticks;
    rtc_cnt = nrf_drv_rtc_counter_get(&m_rtc);
    time_ticks = gs_current_time + rtc_cnt + (current_timezone << 15);
    time_ticks = time_ticks >> 5;
    return ((uint32_t)time_ticks);
}

/*
  *  @brief: Function to get the current timestamp and current timezone of the system
  *          in 1sec resolution,by FS task for file name creation and system task for get/set of dateTime
  *  @timestamp: total of date, time in secs
  *  @timezone_offset: total of timezone in secs
*/
uint32_t get_log_time_stamp(time_t *timestamp,int16_t *timezone_offset)
{
    uint32_t rtc_cnt;
    uint64_t time_ticks;
    if((NULL == timestamp)||(NULL == timezone_offset))
    {
        return -1;
    }
    rtc_cnt = nrf_drv_rtc_counter_get(&m_rtc);
    time_ticks = gs_current_time + rtc_cnt;
    time_ticks = time_ticks >> 15;    //Convert from 32Khz resolution to 1 Hz resolution
    time_ticks = (time_ticks & 0xFFFFFFFF);  //Get the last 32 bit values with 1 sec resolution; which can hold ticks count upto 135 years
    *timestamp = (uint32_t) time_ticks;
    *timezone_offset = current_timezone;
    return 0;
}

/*
  *  @brief: Function to set the current timezone of the system
*/
void rtc_timezone_set(int32_t timezone)
{
    current_timezone = timezone;
}

m_time_struct *rtc_date_time_get(void)
{
    uint32_t time;
    uint32_t rtc_cnt;
    rtc_cnt = microsecond + nrf_drv_rtc_counter_get(&m_rtc);
    time = current_time + (rtc_cnt >> 15) + current_timezone;
    return m_sec_to_date_time(time);
}

/*
  *  @brief: Function to get the current timezone of the system
*/
void rtc_timezone_get(int32_t *timezone)
{
    if(NULL != timezone)
    {
        *timezone = current_timezone;
    }
}

/*
  * @brief: Function to store the rtc details to FDS before system reset
  *         current_time in secs, current rtc count in seconds and current_timezone are saved
  * @param: offset offset value that needs to be updated with rtc count value
*/
void rtc_timestamp_store(uint32_t offset)
{
    uint32_t rtc_cnt;
    uint32_t w_data[FDS_RTC_ENTRIES];

    rtc_cnt = nrf_drv_rtc_counter_get(&m_rtc);
    nrf_drv_rtc_counter_clear(&m_rtc);
    microsecond += (rtc_cnt+offset);
    current_time += (microsecond >> 15);
    microsecond &= 0x7FFF;

    w_data[0] = microsecond;
    w_data[1] = (uint32_t) current_timezone;
    w_data[2] = (uint32_t) (current_time);

    NRF_LOG_INFO("FDS write values: %x %x %x",w_data[0], w_data[1], w_data[2]);
    adi_fds_update_entry(ADI_RTC_FILE, ADI_DCB_RTC_TIME_BLOCK_IDX, w_data, sizeof(w_data));
}

/*
  * @brief: Function to re-store the rtc details from FDS after system boot
  *         current_time in secs, current rtc count in seconds and current_timezone are saved
  * @param: offset offset value that needs to be updated with rtc count value
*/
void rtc_timestamp_restore(uint32_t offset)
{
    uint32_t r_data[FDS_RTC_ENTRIES];
    uint16_t len = sizeof(r_data);
    if (adi_fds_read_entry(ADI_RTC_FILE, ADI_DCB_RTC_TIME_BLOCK_IDX, r_data, &len) == NRF_SUCCESS)
    {
        NRF_LOG_INFO("FDS read values: %x %x %x",r_data[0], r_data[1], r_data[2]);
        current_time = (uint32_t) r_data[2];
        gs_current_time = current_time;
        gs_current_time = (gs_current_time << 15) + (uint32_t)offset;
        if(current_time == 0xffffffff)
        {
            m_time_struct clock = SB_RTC_DEFAULT_CONFIG;
            current_time = m_date_time_to_sec(&clock);
            gs_current_time = current_time;
            gs_current_time = (gs_current_time << 15);
            current_timezone = 0;
            microsecond = 0;
        }
        else
        {
            microsecond = (r_data[0]&0x7FFF) + offset;
            current_timezone = (int32_t) (r_data[1]);
        }
    }
    else
    {
        NRF_LOG_INFO("Nothing stored in FDS");
        m_time_struct clock = SB_RTC_DEFAULT_CONFIG;
        current_time = m_date_time_to_sec(&clock);
        gs_current_time = current_time;
        gs_current_time = (gs_current_time << 15);
        current_timezone = 0;
        microsecond = 0;
    }
}

/**@brief  Function for clearing the entire RTC settings written to flash(at time of factory reset)
 * @return return value of type ret_code_t
 */
uint8_t adi_rtc_clear_fds_settings()
{
    ret_code_t rRet = DEF_FAIL;
    rRet = adi_fds_clear_entries(ADI_RTC_FILE);
    rRet = (rRet == FDS_SUCCESS) ? DEF_OK : DEF_FAIL;

    return (uint8_t)rRet;
}
