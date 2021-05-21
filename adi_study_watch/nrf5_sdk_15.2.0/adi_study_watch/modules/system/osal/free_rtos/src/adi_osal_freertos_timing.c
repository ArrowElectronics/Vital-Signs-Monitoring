/*
Copyright (c) 2016, Analog Devices, Inc.  All rights reserved.

 Redistribution and use in source and binary forms, with or without
 modification, are permitted (subject to the limitations in the
 disclaimer below) provided that the following conditions are met:

 * Redistributions of source code must retain the above copyright
   notice, this list of conditions and the following disclaimer.

 * Redistributions in binary form must reproduce the above copyright
   notice, this list of conditions and the following disclaimer in the
   documentation and/or other materials provided with the
   distribution.

 * Neither the name of Analog Devices, Inc.  nor the names of its
   contributors may be used to endorse or promote products derived
   from this software without specific prior written permission.

 NO EXPRESS OR IMPLIED LICENSES TO ANY PARTY'S PATENT RIGHTS ARE
 GRANTED BY THIS LICENSE.  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT
 HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED
 WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
 LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
 BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
 OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
 IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

******************************************************************************
Copyright (c), 2010-2017 - Analog Devices Inc. All Rights Reserved.
This software is PROPRIETARY & CONFIDENTIAL to Analog Devices, Inc.
and its licensors.
******************************************************************************/
/*!
    @file adi_osal_freertos_timing.c

    Operating System Abstraction Layer - OSAL for Free RTOS - Timing
    functions

*/
/** @addtogroup ADI_OSAL_Timing ADI OSAL Timing
 *  @{
 *
 * This module contains the Timing APIs for the FreeRTOS implementation of
 * OSAL
 */

/*=============  I N C L U D E S   =============*/

#include <limits.h>

#include "osal_misra.h"
#include "adi_osal.h"
#include "osal_freertos.h"
#include "osal_common.h"

/*  disable misra diagnostics as necessary
 * Error[Pm073]:  a function should have a single point of exit
 *               (MISRA C 2004 rule 14.7)
 *
 * Error[Pm143]: a function should have a single point of exit at the end of
 *                the function (MISRA C 2004 rule 14.7)
 */

/*! @cond */
#if defined ( __ICCARM__ )
_Pragma ("diag_suppress= Pm001,Pm002,Pm073,Pm143")
#endif
/*! @endcond */

/*=============  D A T A  =============*/




/*=============  C O D E  =============*/


/*!
  ****************************************************************************
    @brief Returns the duration of a tick period in microseconds.

    @param[out] pnTickPeriod - pointer to a location to write the tick period
                               in microseconds.

    @return ADI_OSAL_SUCCESS - If the function successfully returned the
                               duration of the tick period in microseconds.
    @return ADI_OSAL_FAILED  - If the tick period is set to UINT_MAX
  Notes:
      This function  helps to convert  time units to system ticks which is
      needed by the pend APIs of message-Q,semaphore,mutex,event  and to
      put the task in "sleep" mode.

                                                   No. Microsec in one second
      Duration of the tick period (in micro second) =  -------------------------
                                                   No of ticks in one second
*****************************************************************************/

ADI_OSAL_STATUS  adi_osal_TickPeriodInMicroSec(uint32_t *pnTickPeriod)
{
    /* In Free RTOS there no API to get Tick Period in Micro Sec. For this reason
     *  they must be set by calling adi_osal_Config.
     */
    if (UINT_MAX == _adi_osal_gnTickPeriod)
    {
        *pnTickPeriod = UINT_MAX;
        return ADI_OSAL_FAILED ;
    }
    else
    {
        *pnTickPeriod = _adi_osal_gnTickPeriod;
        return ADI_OSAL_SUCCESS ;
    }
}



/*!
  ****************************************************************************
  @brief Processes a clock tick

  This indicates to the OS that a tick period is completed.

*****************************************************************************/

void adi_osal_TimeTick(void)
{
    /* In FreeRTOS the Tick ISR is provided by the port itself and hence just return*/
    return;
}



/*!
  ****************************************************************************
    @brief Returns the current value of the continuously incrementing timer
           tick counter.

    The counter increments once for every timer interrupt.

    @param[out] pnTicks - pointer to a location to write the current value of
                          the tick counter.

    @return ADI_OSAL_SUCCESS - If the function successfully returned the tick
                               counter value

*****************************************************************************/

ADI_OSAL_STATUS adi_osal_GetCurrentTick(uint32_t *pnTicks )
{

    /* FreeRTOS has separate tick count function for ISR */
    if(CALLED_FROM_AN_ISR)
    {
        *pnTicks = (uint32_t)xTaskGetTickCountFromISR();
    }
    else
    {
        *pnTicks = (uint32_t)xTaskGetTickCount();
    }

    return ADI_OSAL_SUCCESS;
}

/* enable misra diagnostics as necessary */
/*! @cond */
#if defined ( __ICCARM__ )
_Pragma ("diag_default= Pm001,Pm002,Pm073,Pm143")
#endif
/*! @endcond */
/*
**
** EOF:
**
*/
/*@}*/