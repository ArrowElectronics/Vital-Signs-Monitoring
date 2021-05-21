/******************************************************************************
*                                                                             *
* License Agreement                                                           *
*                                                                             *
* Copyright (c) 2019 Analog Devices Inc.                                      *
* All rights reserved.                                                        *
*                                                                             *
* This source code is intended for the recipient only under the guidelines of *
* the non-disclosure agreement with Analog Devices Inc.                       *
*                                                                             *
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR  *
* IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,    *
* FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE *
* AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER      *
* LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING     *
* FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER         *
* DEALINGS IN THE SOFTWARE.                                                   *
*                                                                             *
*                                                                             *
* This software is intended for use with the ADPD and derivative parts        *
* only                                                                        *
*                                                                             *
******************************************************************************/
#include <stdio.h>
//#include "ADPDDrv.h"
#include "adpd_common.h"

uint8_t UtilGetCurrentRegValue_PmOnly(uint16_t, uint16_t*, uint16_t*);
uint16_t UtilGetCurrentValue_PmOnly(uint16_t coarseReg, uint16_t fineReg);

/**
  * @brief Conver LED current register setting to LED current value
  * @param LED Coase register setting
  * @param LED fine register setting
  * @retval uint16_t LED current value
  */
uint16_t UtilGetCurrentValue_PmOnly(uint16_t coarseReg, uint16_t fineReg)  {
  uint16_t temp16_c, temp16_f;

  if (g_lcfg_PmOnly->partNum != 108)  {
    temp16_c = (uint16_t)((coarseReg & 0xF) * 19.8 + 50.3);
    temp16_f = (uint16_t)((fineReg & 0x1F) * 2.2 + 74);
    temp16_c *= temp16_f;

    if ((coarseReg & 0x2000) == 0)
      temp16_c *= 0.4;
  } else {
    temp16_c = (uint16_t)((coarseReg & 0xF) * 19.8 + 50.3);
    temp16_f = (uint16_t)((fineReg & 0x1F) * 2.2 + 74);
    temp16_c *= temp16_f;

    if ((coarseReg & 0x2000) == 0)
      temp16_c *= 0.1;
  }
  return (temp16_c+50)/100;     // round to nearest digit
}

/**
  * @brief Conver LED current value to register setting
  * @param LED Coase register setting
  * @param LED fine register setting
  * @retval uint8_t result, pass or fail
  */
uint8_t UtilGetCurrentRegValue_PmOnly(uint16_t current, uint16_t* coarseReg, uint16_t* fineReg)  {
  uint16_t temp16_c, temp16_f;
  uint32_t temp32;
  uint16_t percentCur;

  if (g_lcfg_PmOnly->partNum != 108)  {
    percentCur = 250;      // 100/40%
  } else {
    percentCur = 1000;     // 100/10%
  }

  if (current < 4 || current > 493)
    return 1;   // out of range
  AdpdDrvRegRead(REG_LED1_DRV, coarseReg);
  // current = (Coarse*19.8 + 50.3)*(0.022*fine+0.74)*40%
  *coarseReg = *coarseReg & 0xDFF0;   // mask out current value bits
  // m = (c*19.8+50.3)*(0.74+0.022*f)*(0.4+0.6*a)
  // => m = (c*19.8+50.3)*F*A
  // => F=1. if A=1, 100m = 100(c*19.8+50.3); of A=0.4, 250m = 100(c*19.8+50.3)
  //
  if (current < 51)  {
    temp32 = current * percentCur;
  } else {
    *coarseReg |= 0x2000;    // choose 100%
    temp32 = current * 100;
  }
  // temp16_c = (current - 5030)  / 1980;
  temp16_c = (uint16_t)((temp32 - 3772)  / 1485);   // 75% from fine setting
  if (temp16_c > 15)
    temp16_c = 15;
  *coarseReg |= (temp16_c & 0xF);

  // look for fine register
  temp16_c = (uint16_t)(temp16_c * 19.8 + 50.3);    // coarse current
  temp16_f = temp32 / temp16_c;
  temp16_f = (uint16_t)((temp16_f - 74) / 2.2);
  AdpdDrvRegRead(REG_LED_TRIM, fineReg);
  *fineReg = *fineReg & 0xFFE0;
  *fineReg |= (temp16_f & 0x1F);

  return 0;
}
