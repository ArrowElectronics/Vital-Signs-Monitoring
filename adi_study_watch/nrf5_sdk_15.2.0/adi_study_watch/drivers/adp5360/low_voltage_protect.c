/*! *****************************************************************************
    @file:    low_voltage_protect.c
    @brief:   Battery low voltage detect functions
    -----------------------------------------------------------------------------
    Copyright (c) 2019 Analog Devices, Inc.

    All rights reserved.

    Redistribution and use in source and binary forms, with or without modification,
    are permitted provided that the following conditions are met:
    - Redistributions of source code must retain the above copyright notice,
    this list of conditions and the following disclaimer.
    - Redistributions in binary form must reproduce the above copyright notice,
    this list of conditions and the following disclaimer in the documentation
    and/or other materials provided with the distribution.
    - Modified versions of the software must be conspicuously marked as such.
    - This software is licensed solely and exclusively for use with processors
    manufactured by or for Analog Devices, Inc.
    - This software may not be combined or merged with other code in any manner
    that would cause the software to become subject to terms and conditions
    which differ from those listed here.
    - Neither the name of Analog Devices, Inc. nor the names of its
    contributors may be used to endorse or promote products derived
    from this software without specific prior written permission.
    - The use of this software may or may not infringe the patent rights of one
    or more patent holders.  This license does not release you from the
    requirement that you obtain separate licenses from these patent holders
    to use this software.

    THIS SOFTWARE IS PROVIDED BY ANALOG DEVICES, INC. AND CONTRIBUTORS "AS IS"
    AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, NON-
    INFRINGEMENT, TITLE, MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
    DISCLAIMED. IN NO EVENT SHALL ANALOG DEVICES, INC. OR CONTRIBUTORS BE LIABLE
    FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, PUNITIVE OR
    CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, DAMAGES ARISING OUT OF
    CLAIMS OF INTELLECTUAL PROPERTY RIGHTS INFRINGEMENT; PROCUREMENT OF
    SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
    INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
    CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
    ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
    POSSIBILITY OF SUCH DAMAGE.

*****************************************************************************/

#include "low_voltage_protect.h"
#include "adp5360.h"
#include "display_app.h"

#ifdef VSM_MBOARD
#ifdef RJD2450
#define ONE_PERCENT_LEVEL (3150)//mV
#else
#define ONE_PERCENT_LEVEL (3300)//mV
#endif
#else
#define ONE_PERCENT_LEVEL (3300)//mV
#endif

void battery_low_voltage_detect_func(uint8_t value,ADP5360_INTERRUPT status)
{
    if((0 == value)&&(status.int1.batpro_int == 1))//when detect the battery protect, enter to shipment mode.
    {
        if(0 != Adp5360_pgood_pin_status_get())//confirm not connect the USB power.
        {
            Adp5360_enter_shipment_mode();//End shiment mode,Must need enter USB power to recover
        }
    }
    if((0 == value)&&(status.int1.soclow_int == 1))
    {
        if(0 != Adp5360_pgood_pin_status_get())//confirm not connect the USB power.
        {
#ifdef ENABLE_WATCH_DISPLAY
            send_global_type_value(DIS_VOLTAGE_LOW_ALARM);
#endif
        }
    }
}


void battery_low_voltage_protect_init(void)
{
    uint16_t battery_v;
    Adp5360_getBatVoltage(&battery_v);
    if((0 != Adp5360_pgood_pin_status_get())&&(battery_v < ONE_PERCENT_LEVEL))//if battery voltage low and not in charging status, enter shipment mode.
    {
        Adp5360_enter_shipment_mode();
    }
    Register_int_detect_func(battery_low_voltage_detect_func);
}