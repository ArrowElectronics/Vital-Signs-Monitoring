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
#include <adpd_buffering.h>
#define MODULE ("FloatMode.c")

#define SKIP_CNT    2
#define FM_TIMEOUT  3000
#define LED_FM_CURRENT_MAX  100   // max 100mA

#define SAT_PTR_200K    8000
#define SAT_PTR_100K    8000
#define SAT_PTR_50K     8000

#define AdpdMwLibSetMode(a, b, c)   \
          AdpdDrvSetSlot(b, c); adpd_buff_reset(0); AdpdDrvSetOperationMode(a);


/* Public function prototypes -----------------------------------------------*/
void NormalModeSetup(void);
int8_t NormalMode_CurrentAdjustment(uint16_t*, uint16_t*, uint16_t*);

/* Public or external variables ---------------------------------------------*/
extern AdpdLibConfig_t *g_lcfg_PmOnly, gLCFG;

extern uint8_t UtilGetCurrentRegValue_PmOnly(uint16_t, uint16_t*, uint16_t*);
extern uint16_t UtilGetCurrentValue_PmOnly(uint16_t coarseReg, uint16_t fineReg);

/* Private function prototypes ----------------------------------------------*/
static void NM_Init(void);
static void NM_DeInit(void);

static INT_ERROR_CODE_t NM_Adjuest(uint32_t *rawDataB);
static void NM_SetFinalTiming(void);

/* Private Variables --------------------------------------------------------*/
static struct Register {
    uint16_t x11;
    uint16_t x12;
    uint16_t x14;
    uint16_t x15;

    uint16_t x34;
    uint16_t x36;
    uint16_t x3B;
    uint16_t x44;
    uint16_t x23;
    uint16_t x25;
    uint16_t x3C;
}Reg;

static uint8_t gsNmState, gsSkipCnt;
static AdpdLibConfig_t gLCFG;
static uint16_t gsLedCurrent, gsLedTrimCur, gsDarkOffset, gsSaturateValue;
static uint16_t gsCurrentValue, gsTiaGain;

extern ADI_OSAL_SEM_HANDLE adpd_task_floatmode_sem;
static void adpd_fm_data_ready_cb(void);


int8_t NormalMode_CurrentAdjustment(uint16_t *nmCur,
                                    uint16_t *nmTrim,
                                    uint16_t *nmTia)  {
  INT_ERROR_CODE_t errorCode = IERR_SUCCESS;
  ADPDDrv_Operation_Slot_t slotA_mode, slotB_mode;
  uint16_t slotA_size, slotB_size;
  uint32_t  timestamp = 0;
  uint8_t   tmp[32];
  uint32_t  len, ppgData[4];
  CIRC_BUFF_STATUS_t  status;
  uint32_t nTimeVal0, nTimeVal1;
  uint8_t retCode, i;

  AdpdDrvDataReadyCallback(adpd_fm_data_ready_cb);

  NM_Init();
  AdpdDrvSetSlot( ADPDDrv_SLOT_OFF, (ADPDDrv_Operation_Slot_t)(g_lcfg_PmOnly->devicemode));
  adpd_buff_reset(0);
  AdpdDrvSetOperationMode(ADPDDrv_MODE_SAMPLE);
  nTimeVal0 = MCU_HAL_GetTick();
  retCode = 1;
  len = sizeof(tmp);
  while(retCode)  {
    // get slotBData
    adi_osal_SemPend(adpd_task_floatmode_sem, ADI_OSAL_TIMEOUT_FOREVER);
    adpd_read_data_to_buffer(&slotA_mode, &slotB_mode, &slotA_size, &slotB_size);
    len = sizeof(tmp);
    status = adpd_buff_get(&tmp[0], &timestamp, &len);
    nTimeVal1 = MCU_HAL_GetTick();
    if ((ABS(nTimeVal1 - nTimeVal0)) > FM_TIMEOUT)
        break;

    while (status == CIRC_BUFF_STATUS_OK)  {
      for (i=0; i<slotB_size; i+=2)
        ppgData[i/2] = (tmp[i+1] << 8) | tmp[i];
      adi_printf("ch0 data=%x %x %x %x\r\n", ppgData[0], ppgData[1], ppgData[2], ppgData[3]);
      errorCode = NM_Adjuest(&ppgData[g_lcfg_PmOnly->targetChs]);

      if (errorCode == IERR_SUCCESS) {
        NM_SetFinalTiming();
        retCode = 0;
        break;
      }
      if (errorCode == IERR_FAIL)  {
        retCode = 1;
        break;
      }
      status = adpd_buff_get(&tmp[0], &timestamp, &len);
    }
    if (errorCode != IERR_IN_PROGRESS)
      break;
  }
  AdpdDrvSetSlot(ADPDDrv_SLOT_OFF, ADPDDrv_SLOT_OFF);
  AdpdDrvSetOperationMode(ADPDDrv_MODE_IDLE);
  adi_printf("NM cur=%x, trim=%x, tia=%d, start=%d, end=%d, err=%d\r\n", \
        gsLedCurrent, gsLedTrimCur, gsTiaGain, nTimeVal0, nTimeVal1, retCode);
  *nmCur = gsLedCurrent;
  *nmTrim = gsLedTrimCur;
  *nmTia = 200 / (1<<(gsTiaGain&0x3));

  if (retCode == 1)  {
    NM_DeInit();
  }
  return retCode;
}

static void adpd_fm_data_ready_cb(void) {
  adi_osal_SemPost(adpd_task_floatmode_sem);
}


/**
  * @internal
  * @brief Setup Float Mode routine
  * @param rawDataB pointer to an array of uint32_t
  * @retval SUCCESS = FloatMode setup successfully
  */
static INT_ERROR_CODE_t NM_Adjuest(uint32_t *rawDataB) {
  uint16_t ledC, ledF, nmData1Ch, temp16;
  uint32_t ledCurrent;

  if (gsSkipCnt > 0)  {
    gsSkipCnt--;
    return IERR_IN_PROGRESS;
  }

  nmData1Ch = rawDataB[0];   // use the large output ch
  adi_printf("fm state=%d, ledCur=%x, trim=%x, data=%d\r\n", \
            gsNmState, gsLedCurrent, gsLedTrimCur, nmData1Ch);

  // Start with 20mA, 1 pulse. If TIA saturated, half the TIA.
  // If the LED current is quite low = 25mA, change to lower TIA gain and
  //      double the current
  // If max current is used, may increase the LED Current Trim
  // Increase the pulses up to max if neccessary

  if (gsNmState == 0)   {
    // load float mode configure
    // 1 set=4 pulses. 100K tia
    // start from lowest LED current 8mA
    AdpdMwLibSetMode(ADPDDrv_MODE_IDLE, ADPDDrv_SLOT_OFF, ADPDDrv_SLOT_OFF);

    temp16 = (Reg.x36 & 0xFF) | 0x100;        // set to 1 pulse
    AdpdDrvRegWrite(0x36, temp16);

    AdpdDrvRegWrite(0x34, Reg.x34 | 0x200);   // disable LED

    ledC = gsLedCurrent;  // set to 20mA
    AdpdDrvRegWrite(0x23, ledC);

    ledF = gsLedTrimCur;
    AdpdDrvRegWrite(0x25, ledF);
    gsCurrentValue = UtilGetCurrentValue_PmOnly(ledC, ledF);

    AdpdMwLibSetMode(ADPDDrv_MODE_SAMPLE,
                     ADPDDrv_SLOT_OFF,
                     (ADPDDrv_Operation_Slot_t)(g_lcfg_PmOnly->devicemode));
    gsNmState = 1;
    gsSkipCnt = SKIP_CNT;
    return IERR_IN_PROGRESS;
  }

  if (gsNmState == 1)  {
    // record no LED value
    gsDarkOffset = nmData1Ch;

    AdpdMwLibSetMode(ADPDDrv_MODE_IDLE, ADPDDrv_SLOT_OFF, ADPDDrv_SLOT_OFF);
    AdpdDrvRegWrite(0x34, 0);               // Enable LED

    AdpdMwLibSetMode(ADPDDrv_MODE_SAMPLE,
                     ADPDDrv_SLOT_OFF,
                     (ADPDDrv_Operation_Slot_t)(g_lcfg_PmOnly->devicemode));
    gsNmState = 2;
    gsSkipCnt = SKIP_CNT;
    return IERR_IN_PROGRESS;
  }

  // inc or dec LED current
  if (gsNmState == 2 || gsNmState == 3)  {
    // Increase LED current to saturation point
    if (nmData1Ch < gsSaturateValue)  {
      nmData1Ch -= gsDarkOffset;
      // predict current for saturation
      ledCurrent = (gsSaturateValue*100 / nmData1Ch) * gsCurrentValue;
      // 50%, then rounded
      gsCurrentValue = (ledCurrent * g_lcfg_PmOnly->dcLevelPercentA + 5000) / 10000;
      AdpdMwLibSetMode(ADPDDrv_MODE_IDLE, ADPDDrv_SLOT_OFF, ADPDDrv_SLOT_OFF);

      UtilGetCurrentRegValue_PmOnly(gsCurrentValue, &ledC, &ledF);
      AdpdDrvRegWrite(0x23, ledC);
      AdpdDrvRegWrite(0x25, ledF);

      ledC &= 0x200F;
      ledF &= 0x001F;
      gsLedCurrent = (gsLedCurrent & 0xDFF0) | ledC;
      gsLedTrimCur = (gsLedTrimCur & 0xFFE0) | ledF;

      AdpdMwLibSetMode(ADPDDrv_MODE_SAMPLE,
                     ADPDDrv_SLOT_OFF,
                     (ADPDDrv_Operation_Slot_t)(g_lcfg_PmOnly->devicemode));
      gsNmState++;      // state=3, or 4
    } else {    // saturated
      gsNmState = 10;
      // reduce TIA gain
    }
    gsSkipCnt = SKIP_CNT;
    return IERR_IN_PROGRESS;
  }

  // Half Tia
  if (gsNmState == 10)  {
    AdpdMwLibSetMode(ADPDDrv_MODE_IDLE, ADPDDrv_SLOT_OFF, ADPDDrv_SLOT_OFF);

    AdpdDrvRegRead(0x44, &temp16);
    temp16 &= 0x3;
    if (temp16 != 3)
      temp16++;
    else
      return IERR_FAIL;
    gsTiaGain = gsTiaGain & 0xFFFC | temp16;

    AdpdDrvRegWrite(0x44, gsTiaGain);
    AdpdMwLibSetMode(ADPDDrv_MODE_SAMPLE,
                     ADPDDrv_SLOT_OFF,
                     (ADPDDrv_Operation_Slot_t)(g_lcfg_PmOnly->devicemode));
    gsNmState = 2;
    gsSkipCnt = SKIP_CNT;
    return IERR_IN_PROGRESS;
  }

  // Done
  if (gsNmState == 4) {
    return IERR_SUCCESS;
  }

  return IERR_IN_PROGRESS;
}

/**
  * @internal
  * @brief Set up float LED mode timing base on Float LED width
  * @param none
  * @retval none
  */
static void NM_SetFinalTiming()  {
  AdpdDrvRegWrite(0x36, Reg.x36);
  AdpdDrvRegWrite(0x23, gsLedCurrent);
  AdpdDrvRegWrite(0x25, gsLedTrimCur);
  AdpdDrvRegWrite(0x44, gsTiaGain);
}


/**
  * @internal
  * @brief Normal Mode Init routine
  * @param None
  * @retval None
  */
static void NM_Init() {
  uint16_t tiaGain, selectCh, temp16;

  g_lcfg_PmOnly = &gLCFG;
  gsNmState = gsSkipCnt = 0;
  gsDarkOffset = 0;

  AdpdDrvRegRead(0x11, &Reg.x11);
  AdpdDrvRegRead(0x12, &Reg.x12);
  AdpdDrvRegRead(0x14, &Reg.x14);
  AdpdDrvRegRead(0x15, &Reg.x15);

  // SlotB related
  AdpdDrvRegRead(0x36, &Reg.x36);
  AdpdDrvRegRead(0x44, &Reg.x44);

  AdpdDrvRegRead(0x23, &Reg.x23);
  AdpdDrvRegRead(0x25, &Reg.x25);
  AdpdDrvRegRead(0x34, &Reg.x34);

  // find TIA Gain setting
  gsTiaGain = Reg.x44 & 0xFFFC;   // start from 200k
  tiaGain = gsTiaGain & 0x3;
  if (tiaGain == 0)
    gsSaturateValue = SAT_PTR_200K;
  else if (tiaGain == 1)
    gsSaturateValue = SAT_PTR_100K;
  else
    gsSaturateValue = SAT_PTR_50K;

  g_lcfg_PmOnly->devicemode = ADPDDrv_4CH_16;
  g_lcfg_PmOnly->dcLevelPercentA = 50;

  AdpdDrvRegRead(0x1E, &selectCh);
  if (selectCh < 0x3000)
    g_lcfg_PmOnly->targetChs = 0;   // Ch1 selected, some 108 case
  AdpdDrvRegRead(0x20, &selectCh);
  if (selectCh < 0x3000)
    g_lcfg_PmOnly->targetChs = 3;   // Ch4 selected. 108 and 188 case

  AdpdDrvRegRead(0x8, &temp16);
  if ((temp16&0xFF00) == 0x900) {
    g_lcfg_PmOnly->partNum = 108;
    gsLedCurrent = (Reg.x23 & 0xDFF0) | 0x100a;
    gsLedTrimCur = (Reg.x25 & 0xFFE0) | 0x3;    // 20mA
  } else {
    g_lcfg_PmOnly->partNum = 107;
    gsLedCurrent = (Reg.x23 & 0xDFF0) | 0x1000;
    gsLedTrimCur = (Reg.x25 & 0xFFE0) | 0xC;    // 20mA
  }

  return;
}

/**
  * @internal
  * @brief Float Mode DeInitilization
  * @param none
  * @retval none
  */
static void NM_DeInit() {
    AdpdMwLibSetMode(ADPDDrv_MODE_IDLE, ADPDDrv_SLOT_OFF, ADPDDrv_SLOT_OFF);
    AdpdDrvRegWrite(0x11, Reg.x11);
    AdpdDrvRegWrite(0x12, Reg.x12);
    AdpdDrvRegWrite(0x14, Reg.x14);
    AdpdDrvRegWrite(0x15, Reg.x15);


    // SlotB related
    AdpdDrvRegWrite(0x36, Reg.x36);
    AdpdDrvRegWrite(0x44, Reg.x44);

    AdpdDrvRegWrite(0x23, Reg.x23);
    AdpdDrvRegWrite(0x25, Reg.x25);
    AdpdDrvRegWrite(0x34, Reg.x34);
    return;
}
