/**
***************************************************************************
* @addtogroup Tasks
* @{
* @file         adi_vsm_cycle_count.h
* @author       ADI
* @version      V1.0.0
* @date         22-Sept-2020
* @brief        Header file of sqi processing APIs.
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

#ifndef CYCLE_COUNT_H
#define CYCLE_COUNT_H

#include <stdint.h>
/* ------------------------- STRUCTS  -------------------------------------- */

/*! structure instance for storing cycle count register values*/
typedef struct _ADI_CYCLE_COUNT_PARAM
{
  uint32_t startTick;   /*!starting value of cycle count register*/
  uint32_t endTick;     /*!Ending value of cycle count register*/
  uint32_t tickDiff;    /*!Difference between start and end values*/
}tAdiCycleCountInst;


/*------------------------- Public functions ------------------------------- */
void adi_CycleCountInit(void);
void adi_EnableCycleCounter(void);
void adi_DisableCycleCounter(void);
void adi_ResetCycleCounter(void);
uint32_t adi_GetCycleCount(void);

uint32_t adi_GetStartCycleCount(void);
uint32_t adi_GetEndCycleCount(void);
#endif //CYCLE_COUNT_H