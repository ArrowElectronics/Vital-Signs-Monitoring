/**
***************************************************************************
* @addtogroup Tasks
* @{
* @file         adi_vsm_cycle_count.c
* @author       ADI
* @version      V1.0.0
* @date         22-Sept-2020
* @brief        Source file contains sqi processing APIs.
***************************************************************************
* @attention
***************************************************************************
*/
/*!
*  \copyright Analog Devices
* ****************************************************************************
*
* License Agreement
*
* Copyright (c) 2020 Analog Devices Inc.
* All rights reserved.
*
* This source code is intended for the recipient only under the guidelines of
* the non-disclosure agreement with Analog Devices Inc.
*

* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
* EXPRESS OR  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
* MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
* IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
* CLAIM, DAMAGES OR OTHER  LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
* TORT OR OTHERWISE, ARISING  FROM, OUT OF OR IN CONNECTION WITH THE
* SOFTWARE OR THE USE OR OTHER  DEALINGS IN THE SOFTWARE.
* ****************************************************************************
*/
#ifdef PROFILE_TIME
/* ------------------------- Includes -------------------------------------- */
#include <stdio.h>
#include <stdint.h>
#include "adi_vsm_cycle_count.h"

/*------------------------- Define ------------------------------- */
/*!macro to fetch DWT control register value*/
#define DWT_CTRL                (*((volatile uint32_t *)0xE0001000))
/*!macro to fetch DWT cycle count register value*/
#define DWT_CTRL_CYCCNT         (*((volatile uint32_t *)0xE0001004))
/*!macro to fetch debug exception and monitor control register value*/
#define DWT_DEMCR               (*((volatile uint32_t *)0xE000EDFC))
/*! bit position to enable the cycle count in DWT_CTRL register*/
#define DWT_CYCCNTENA_BIT       1UL<<0
/*! bit position to enable the trace unit of cortex-M4 core */
#define DWT_TRCENA_BIT          1UL<<24

/*!macro to enable the cycle counter oepration*/
#define ENABLE_CYCLE_COUNTER()          DWT_CTRL = DWT_CTRL | DWT_CYCCNTENA_BIT

/*!macro to disable the cycle counter oepration*/
#define DISABLE_CYCLE_COUNTER()         DWT_CTRL = DWT_CTRL & (~DWT_CYCCNTENA_BIT)

/*!macro to enable the Trace unit*/
#define ENABLE_TRC_UNIT()               DWT_DEMCR = DWT_DEMCR | DWT_TRCENA_BIT

/*!macro to reset the cycle count register value*/
#define RESET_CYCLE_COUNTER()           DWT_CTRL_CYCCNT = 0



/*------------------------- Private Variables ------------------------------- */
tAdiCycleCountInst goAdiCycleCount;

/*------------------------- Public Variables ------------------------------- */



/* ------------------------- Private Function ------------------- */


/*------------------------- Public functions ------------------------------- */

/*!****************************************************************************
*
*  \b              adi_CycleCountInit
*
*       Initializes the Trace unit of M4 core
*
*  \param[in]   None
*  \return      none
*
******************************************************************************/

void adi_CycleCountInit(void)
{
  ENABLE_TRC_UNIT();
}

/*!****************************************************************************
*
*  \b              adi_EnableCycleCounter
*
*       Enables the cycle counter operation 
*
*  \param[in]   None
*  \return      none
*
******************************************************************************/
void adi_EnableCycleCounter(void)
{
  ENABLE_CYCLE_COUNTER();
}

/*!****************************************************************************
*
*  \b              adi_DisableCycleCounter
*
*       Disables the cycle counter operation 
*
*  \param[in]   None
*  \return      none
*
******************************************************************************/

void adi_DisableCycleCounter(void)
{
  DISABLE_CYCLE_COUNTER();
}


/*!****************************************************************************
*
*  \b              adi_ResetCycleCounter
*
*       Resets the cycle counter register 
*
*  \param[in]   None
*  \return      none
*
******************************************************************************/

void adi_ResetCycleCounter(void)
{
  RESET_CYCLE_COUNTER();
}

/*!****************************************************************************
*
*  \b              adi_GetCycleCount
*
*       Returns the cycle count register value
*
*  \param[in]   None
*  \return      DWT_CTRL_CYCCNT: The cycle count register value
*
******************************************************************************/

uint32_t adi_GetCycleCount(void)
{
  return(DWT_CTRL_CYCCNT);
}


/*!****************************************************************************
*
*  \b              adi_GetStartCycleCount
*
* Initializes the trace unit, cycle count register, enables the cycle counting
*  operation and retruns the starting value of Cycle count register
*
*  \param[in]   None
*  \return      goAdiCycleCount.startTick: The starting value of cycle count operation
*
******************************************************************************/

uint32_t adi_GetStartCycleCount(void)
{
  ENABLE_TRC_UNIT();
  RESET_CYCLE_COUNTER();
  ENABLE_CYCLE_COUNTER();
  goAdiCycleCount.startTick = DWT_CTRL_CYCCNT;
  return(goAdiCycleCount.startTick);
}


/*!****************************************************************************
*
*  \b              adi_GetEndCycleCount
*
* Disables the cycle counting operation and retruns the ending value of 
* Cycle count register
*
*  \param[in]   None
*  \return      goAdiCycleCount.endTick: The Ending value of cycle count operation
*
******************************************************************************/

uint32_t adi_GetEndCycleCount(void)
{
  goAdiCycleCount.endTick = DWT_CTRL_CYCCNT;
  goAdiCycleCount.tickDiff = goAdiCycleCount.endTick - goAdiCycleCount.startTick;
  DISABLE_CYCLE_COUNTER();
  return(goAdiCycleCount.endTick);
}
#endif //#ifdef PROFILE_TIME