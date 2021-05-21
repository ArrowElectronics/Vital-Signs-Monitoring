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
#include "nrf_delay.h"
#include "nrf_drv_clock.h"

#include "nrf_log.h"
#if NRF_LOG_ENABLED
#include "nrf_log_ctrl.h"
#include "nrf_log_default_backends.h"
#endif


static nrf_drv_rtc_t const m_rtc = NRF_DRV_RTC_INSTANCE(1);
static uint32_t current_time=0; //holds current system time in secs resolution
static int32_t current_timezone=0; //holds current system timezone in secs resolution
static uint32_t microsecond = 0;
static uint8_t rtc_store_flash = 0;


static void rtc_handler(nrf_drv_rtc_int_type_t int_type)
{
    if(NRFX_RTC_INT_OVERFLOW == int_type)
    {
        microsecond += (1<<24);
        current_time += (microsecond >> 13);
        microsecond &= 0x1FFF;
    }
}
void rtc_init(void)
{
    ret_code_t err_code;
    static nrf_drv_rtc_config_t m_rtc_config = NRF_DRV_RTC_DEFAULT_CONFIG;
    m_rtc_config.prescaler = RTC_PRESCALER_4;
    
    nrf_drv_clock_lfclk_request(NULL);//prevent the lfclk be disable by nrf_sdh_disable_request();

    err_code = nrf_drv_rtc_init(&m_rtc, &m_rtc_config, rtc_handler);//can count 24 day.
    if (err_code != NRF_SUCCESS)                      
    { 
        NRF_LOG_INFO("rtc init failure!");
        return;                              
    } 
    rtc_store_flash = 1;
    rtc_timestamp_restore(0x2000);
    nrf_drv_rtc_overflow_enable(&m_rtc,true);
    
    nrf_drv_rtc_enable(&m_rtc);
}

void rtc_uninit(void)
{
    uint32_t mask = NRF_RTC_INT_TICK_MASK     |
                    NRF_RTC_INT_OVERFLOW_MASK |
                    NRF_RTC_INT_COMPARE0_MASK |
                    NRF_RTC_INT_COMPARE1_MASK |
                    NRF_RTC_INT_COMPARE2_MASK |
                    NRF_RTC_INT_COMPARE3_MASK;

    NRFX_IRQ_DISABLE(m_rtc.irq);

    nrf_rtc_task_trigger(m_rtc.p_reg, NRF_RTC_TASK_STOP);
    nrf_rtc_event_disable(m_rtc.p_reg, mask);
    nrf_rtc_int_disable(m_rtc.p_reg, mask);
    nrf_drv_clock_lfclk_release();
}

void rtc_timestamp_store(uint32_t offset)
{
    uint32_t rtc_cnt;
    uint32_t w_data[FDS_RTC_ENTRIES];
    if(0 != rtc_store_flash)
    {
    
        rtc_cnt = nrf_drv_rtc_counter_get(&m_rtc);
        nrf_drv_rtc_counter_clear(&m_rtc);        
        rtc_uninit();
        microsecond += (rtc_cnt+offset);
        current_time += (microsecond >> 13);
        microsecond &= 0x1FFF;

        w_data[0] = microsecond;
        w_data[1] = (uint32_t) current_timezone;
        w_data[2] = (uint32_t) (current_time);   
        if(2 == rtc_store_flash)
        {
            adi_fds_update_entry(ADI_RTC_FILE, ADI_DCB_RTC_TIME_BLOCK_IDX,w_data,sizeof(w_data));
        }
    }
    
}

void rtc_timestamp_restore(uint32_t offset)
{
    uint32_t r_data[FDS_RTC_ENTRIES];
    uint16_t len = sizeof(r_data);
    if (adi_fds_read_entry(ADI_RTC_FILE, ADI_DCB_RTC_TIME_BLOCK_IDX,r_data,&len) == NRF_SUCCESS)
    {
        current_time = (uint32_t) r_data[2];
        if(current_time == 0xffffffff)
        {
            m_time_struct clock = SB_RTC_DEFAULT_CONFIG;
            current_time = m_date_time_to_sec(&clock);
            current_timezone = 0;
            microsecond = 0;
        }
        else
        {
            microsecond = (r_data[0]&0x7FFF) + offset;
            current_timezone = (int32_t) (r_data[1]&0xFFFF);
            rtc_store_flash = 2;
        }
        
    }
    else
    {
        current_time = 0;
        current_timezone = 0;
        microsecond = 0;

    }
}


