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
#define FM_TIMEOUT  5000
#define LED_FM_CURRENT_MAX  100   // max 100mA
#define LED_FM_WIDTH_MAX    31    // max width
#define LED_FM_WIDTH_MIN    06    // max width

#define SAT_PTR_200K    8000
#define SAT_PTR_100K    8000
#define SAT_PTR_50K     8000

#define AdpdMwLibSetMode(a, b, c)   \
          AdpdDrvSetSlot(b, c); adpd_buff_reset(0); AdpdDrvSetOperationMode(a);

#define SlotA_Fl1_T     6   // slotA float time 1
#define SlotA_Fl2_T     26  // slotA float time 2
#define SlotB_Fl1_T     39  // slotB float time 1
#define SlotB_Fl2_T     39  // slotB float time 2
#define AMB_ADJUEST     20  // Adjust percentage after ambient

#define FLOAT_TO_FIXED10Q6(x) (int16_t)((x)*(float)(1<<6))
#define FIXED10Q6_TO_FLOAT(x)   (((float)(x))/64.0f)

/* Public function prototypes -----------------------------------------------*/
void FloatModeInit_PmOnly(void);
void FloatModeDeInit_PmOnly(void);
void FloatModeSetup_PmOnly(void);

/* Public or external variables ---------------------------------------------*/
extern AdpdLibConfig_t *g_lcfg_PmOnly, gLCFG;

extern uint8_t UtilGetCurrentRegValue_PmOnly(uint16_t, uint16_t*, uint16_t*);
extern uint16_t UtilGetCurrentValue_PmOnly(uint16_t coarseReg, uint16_t fineReg);

/* Private function prototypes ----------------------------------------------*/
static INT_ERROR_CODE_t FloatModeEnable(uint32_t*);
static void FloatLedSetFinalTiming(void);

/* Private Variables --------------------------------------------------------*/
static struct Register {
    uint16_t x11;
    uint16_t x12;
    uint16_t x14;
    uint16_t x15;

    uint16_t x30;
    uint16_t x31;
    uint16_t x39;
    uint16_t x42;
    uint16_t x43;
    uint16_t x5e;
    uint16_t x3e;
    uint16_t x5a;

    uint16_t x34;
    uint16_t x35;
    uint16_t x36;
    uint16_t x3B;
    uint16_t x44;
    uint16_t x45;
    uint16_t x59;
    uint16_t x58;
    uint16_t x23;
    uint16_t x25;
    uint16_t x3C;
    uint16_t x54;
    uint16_t x3F;
    uint16_t x5A;
}Reg;

static uint8_t gsLedTrimCur, gsFmState, gsSkipCnt;
static AdpdLibConfig_t gLCFG;
static uint16_t gsFmWidth, gsLedCurrent, gsDarkOffset, gsSaturateValue;
static uint16_t gsCurrentValue;
static uint8_t gsConT_A, gsIntT_A;
static uint8_t gsConT_B, gsIntT_B;
static uint8_t gsLedOffset_B;

extern ADI_OSAL_SEM_HANDLE adpd_task_floatmode_sem;
static void adpd_fm_data_ready_cb(void);

/**
  * @internal
  * @brief Float Mode Init routine
  * @param None
  * @retval None
  */
void FloatModeInit_PmOnly() {
  uint16_t tiaGain, selectCh;
  g_lcfg_PmOnly = &gLCFG;
  gsFmState = gsSkipCnt = 0;
  gsLedCurrent = 0x1030;
  gsLedTrimCur = 0xC;
  // find TIA Gain setting
  AdpdDrvRegRead(0x44, &tiaGain);
  tiaGain &= 0x3;
  if (tiaGain == 0)
    gsSaturateValue = SAT_PTR_200K;
  else if (tiaGain == 1)
    gsSaturateValue = SAT_PTR_100K;
  else
    gsSaturateValue = SAT_PTR_50K;

  g_lcfg_PmOnly->devicemode = ADPDDrv_4CH_16;
  g_lcfg_PmOnly->dcLevelPercentA = 50;

  AdpdDrvRegRead(0x1E, &selectCh);
  if (selectCh < 0x200)
    g_lcfg_PmOnly->targetChs = 0;   // Ch1 selected, some 108 case
  AdpdDrvRegRead(0x20, &selectCh);
  if (selectCh < 0x200)
    g_lcfg_PmOnly->targetChs = 2;   // Ch3 selected. 108 and 188 case
  AdpdDrvRegRead(0x21, &selectCh);
  if (selectCh < 0x200)
    g_lcfg_PmOnly->targetChs = 3;   // Ch4 selected. 108 and 188 case

  g_lcfg_PmOnly->partNum = 108;
  return;
}


int8_t AdpdSetFloatMode_PmOnly(uint16_t *fmCur, uint16_t *fmCurTrim, uint16_t *fmWid);

int8_t AdpdSetFloatMode_PmOnly(uint16_t *fmCur, uint16_t *fmCurTrim, uint16_t *fmWid)  {
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
  FloatModeInit_PmOnly();
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
      errorCode = FloatModeEnable(&ppgData[g_lcfg_PmOnly->targetChs]);

      if (errorCode == IERR_SUCCESS) {
        FloatLedSetFinalTiming();
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
  adi_printf("FM cur=%x, trim=%x, wid=%d, start=%d, end=%d, err=%d\r\n", \
        gsLedCurrent, gsLedTrimCur, gsFmWidth, nTimeVal0, nTimeVal1, retCode);
  *fmCur = gsLedCurrent;
  *fmCurTrim = gsLedTrimCur;
  *fmWid = gsFmWidth;
  // AdpdDrvRegWrite(0x3E, 0x4012 | (gsFmWidth << 8)); // set slotA to the same val
  if (retCode == 1)  {
    FloatModeDeInit_PmOnly();
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
static INT_ERROR_CODE_t FloatModeEnable(uint32_t *rawDataB) {
    uint32_t ledCurrentFactor, ledWidthFactor;
    uint16_t ledC, ledF, fmData1Ch, ledCurrent;
    uint16_t ledOffset;

    if (gsSkipCnt > 0)  {
        gsSkipCnt--;
        return IERR_IN_PROGRESS;
    }

    fmData1Ch = rawDataB[0];   // use the large output ch
    adi_printf("fm state=%d, ledCur=%x, trim=%x, data=%d\r\n", \
      gsFmState, gsLedCurrent, gsLedTrimCur, fmData1Ch);

    // use +-, high averaging, gsFmCurrent = 0 gsFmWidth = 12 find codeVal
    // compare with saturated value, assume linear, increase current
    // compare with 50% of saturated value, decrease gsFmWidth or current

    if (gsFmState == 0)   {
        // load float mode configure
        // 1 set=4 pulses. 100K tia
        // start from lowest LED current 8mA
        AdpdMwLibSetMode(ADPDDrv_MODE_IDLE, ADPDDrv_SLOT_OFF, ADPDDrv_SLOT_OFF);
        AdpdDrvRegRead(0x11, &Reg.x11);
        AdpdDrvRegRead(0x12, &Reg.x12);
        AdpdDrvRegRead(0x14, &Reg.x14);
        AdpdDrvRegRead(0x15, &Reg.x15);

        // SlotA related
        AdpdDrvRegRead(0x30, &Reg.x30);
        AdpdDrvRegRead(0x31, &Reg.x31);
        AdpdDrvRegRead(0x39, &Reg.x39);
        AdpdDrvRegRead(0x42, &Reg.x42);
        AdpdDrvRegRead(0x43, &Reg.x43);
        AdpdDrvRegRead(0x5e, &Reg.x5e);
        AdpdDrvRegRead(0x3e, &Reg.x3e);
        AdpdDrvRegRead(0x5a, &Reg.x5a);

        // SlotB related
        AdpdDrvRegRead(0x35, &Reg.x35);
        AdpdDrvRegRead(0x36, &Reg.x36);
        AdpdDrvRegRead(0x3b, &Reg.x3B);
        AdpdDrvRegRead(0x44, &Reg.x44);
        AdpdDrvRegRead(0x45, &Reg.x45);
        AdpdDrvRegRead(0x59, &Reg.x59);
        AdpdDrvRegRead(0x58, &Reg.x58);

        AdpdDrvRegRead(0x23, &Reg.x23);
        AdpdDrvRegRead(0x25, &Reg.x25);
        AdpdDrvRegRead(0x34, &Reg.x34);
        AdpdDrvRegRead(0x3C, &Reg.x3C);
        AdpdDrvRegRead(0x54, &Reg.x54);

        AdpdDrvRegRead(0x3F, &Reg.x3F);
        AdpdDrvRegRead(0x5a, &Reg.x5A);

        gsConT_A = Reg.x30>>8;
        gsIntT_A = Reg.x39>>11;
        gsConT_B = Reg.x35>>8;
        gsIntT_B = Reg.x3B>>11;
        // gsLedOffset_A = Reg.x3e & 0xFF;
        gsLedOffset_B = Reg.x3F & 0xFF;

        gsLedCurrent = 0x1030;
        gsFmWidth = 12;
        // disable LED
        FloatModeSetup_PmOnly();
        AdpdMwLibSetMode(ADPDDrv_MODE_SAMPLE,
                        ADPDDrv_SLOT_OFF,
                        (ADPDDrv_Operation_Slot_t)(g_lcfg_PmOnly->devicemode));
        gsFmState = 1;
        gsSkipCnt = SKIP_CNT + 3;
        return IERR_IN_PROGRESS;
    }

    if (gsFmState == 1)  {
        // record no LED value
        gsDarkOffset = fmData1Ch;

        AdpdMwLibSetMode(ADPDDrv_MODE_IDLE, ADPDDrv_SLOT_OFF, ADPDDrv_SLOT_OFF);
        AdpdDrvRegWrite(0x34, 0);               // Enable LED
        AdpdDrvRegWrite(0x23, gsLedCurrent);    // set to 5mA
        AdpdMwLibSetMode(ADPDDrv_MODE_SAMPLE,
                        ADPDDrv_SLOT_OFF,
                        (ADPDDrv_Operation_Slot_t)(g_lcfg_PmOnly->devicemode));
        gsFmState = 2;
        gsSkipCnt = SKIP_CNT;
        return IERR_IN_PROGRESS;
    }

    // inc or dec LED current
    if (gsFmState == 2)  {
        // Increase LED current to saturation point
        if (fmData1Ch < gsSaturateValue)  {
          if (fmData1Ch == 0 || (fmData1Ch - gsDarkOffset) == 0)  {
            ledCurrentFactor = 10 * 100;  // 10x the LED current
          } else {
            fmData1Ch -= gsDarkOffset;
            ledCurrentFactor = gsSaturateValue * 100 / fmData1Ch; // mul by 100
            gsFmState++;
          }
        } else {    // saturated
          return IERR_FAIL;    // no need for float mode
        }
        // calculate new LED current value
        AdpdMwLibSetMode(ADPDDrv_MODE_IDLE,
                         ADPDDrv_SLOT_OFF,
                         ADPDDrv_SLOT_OFF);
        AdpdDrvRegRead(0x23, &ledC);
        AdpdDrvRegRead(0x25, &ledF);
        ledCurrent = UtilGetCurrentValue_PmOnly(ledC, ledF);
        ledCurrent = ledCurrent * ledCurrentFactor / 100;   // div by 100
        gsCurrentValue = ledCurrent;
        adi_printf("S2 Current=%d, Factor=%d\r\n", ledCurrent, ledCurrentFactor);

        if (ledCurrent > LED_FM_CURRENT_MAX)  {
          ledCurrent = LED_FM_CURRENT_MAX;
          gsFmState = 4;
        }
        UtilGetCurrentRegValue_PmOnly(ledCurrent, &ledC, &ledF);
        AdpdDrvRegWrite(0x23, ledC);
        AdpdDrvRegWrite(0x25, ledF);
        gsLedCurrent = ledC;
        gsLedTrimCur = ledF;
        AdpdMwLibSetMode(ADPDDrv_MODE_SAMPLE,
                         ADPDDrv_SLOT_OFF,
                         (ADPDDrv_Operation_Slot_t)(g_lcfg_PmOnly->devicemode));
        gsSkipCnt = SKIP_CNT;
        return IERR_IN_PROGRESS;
    }

    // inc or dec by 10%
    if (gsFmState == 3)  {    // record width=25 values
        if (fmData1Ch < gsSaturateValue)
          ledCurrentFactor = 110;  // increase by a factor
        else  // saturated
          ledCurrentFactor = 95;  // decrease by a factor

        // saturated, check next width 25
        AdpdMwLibSetMode(ADPDDrv_MODE_IDLE,
                         ADPDDrv_SLOT_OFF,
                         ADPDDrv_SLOT_OFF);
        ledCurrent = gsCurrentValue * ledCurrentFactor / 100;
        UtilGetCurrentRegValue_PmOnly(ledCurrent, &ledC, &ledF);
        AdpdDrvRegWrite(0x23, ledC);
        AdpdDrvRegWrite(0x25, ledF);
        gsLedCurrent = ledC;
        gsLedTrimCur = ledF;
        AdpdMwLibSetMode(ADPDDrv_MODE_SAMPLE,
                         ADPDDrv_SLOT_OFF,
                         (ADPDDrv_Operation_Slot_t)(g_lcfg_PmOnly->devicemode));
        gsFmState = 4;
        gsSkipCnt = SKIP_CNT;
        return IERR_IN_PROGRESS;
    }

    // Increase width, do it twice
    if (gsFmState == 4 || gsFmState == 5)  {
        if (fmData1Ch < gsSaturateValue)  {
          fmData1Ch -= gsDarkOffset;
          ledWidthFactor = gsSaturateValue * 100 / fmData1Ch;
          ledWidthFactor = ledWidthFactor * g_lcfg_PmOnly->dcLevelPercentA / 100;
        } else {
          ledWidthFactor *= g_lcfg_PmOnly->dcLevelPercentA;             // 50%. x100
        }

        // saturated, check next width 25
        AdpdMwLibSetMode(ADPDDrv_MODE_IDLE,
                         ADPDDrv_SLOT_OFF,
                         ADPDDrv_SLOT_OFF);
        gsFmWidth = (gsFmWidth * ledWidthFactor + 50) / 100;    // div by 100
        adi_printf("S4 Width=%d, Factor=%d\r\n", gsFmWidth, ledWidthFactor);
        if (gsFmWidth > LED_FM_WIDTH_MAX)  {
          gsFmWidth = LED_FM_WIDTH_MAX;
        }
        if (gsFmWidth < LED_FM_WIDTH_MIN)  {
          gsFmWidth = LED_FM_WIDTH_MIN;
        }
        // AdpdDrvRegWrite(0x3F, 0x4012 | (gsFmWidth << 8));
        ledOffset = 0x4000 | gsLedOffset_B;
        ledOffset |= (gsFmWidth << 8);
        AdpdDrvRegWrite(0x3F, ledOffset);
        AdpdMwLibSetMode(ADPDDrv_MODE_SAMPLE,
                         ADPDDrv_SLOT_OFF,
                         (ADPDDrv_Operation_Slot_t)(g_lcfg_PmOnly->devicemode));
        gsFmState ++;
        gsSkipCnt = SKIP_CNT;
        return IERR_IN_PROGRESS;
    }
#if 0
    // fine tune by 10%
    if (gsFmState == 6) {    // record width = 15 values
        if (fmData1Ch < (gsSaturateValue * g_lcfg_PmOnly->dcLevelPercentA / 100))
          ledWidthFactor = 120;  // increase by factor
        else
          ledWidthFactor = 90; // decrease by factor

        gsFmWidth = gsFmWidth * ledWidthFactor / 100;
        adi_printf("S5 Width=%d, Factor=%d\r\n", gsFmWidth, ledWidthFactor);
        if (gsFmWidth > LED_FM_WIDTH_MAX)  {
          gsFmWidth = LED_FM_WIDTH_MAX;
        }
        if (gsFmWidth < LED_FM_WIDTH_MIN)  {
          gsFmWidth = LED_FM_WIDTH_MIN;
        }
        AdpdMwLibSetMode(ADPDDrv_MODE_IDLE,
                         ADPDDrv_SLOT_OFF,
                         ADPDDrv_SLOT_OFF);
        // AdpdDrvRegWrite(0x3F, 0x4012 | (gsFmWidth << 8));
        ledOffset = 0x4000 | gsLedOffset_B;
        ledOffset |= (gsFmWidth << 8);
        AdpdDrvRegWrite(0x3F, ledOffset);
        AdpdMwLibSetMode(ADPDDrv_MODE_SAMPLE,
                         ADPDDrv_SLOT_OFF,
                         (ADPDDrv_Operation_Slot_t)(g_lcfg_PmOnly->devicemode));
        gsFmState = 6;
        gsSkipCnt = SKIP_CNT;
        return IERR_IN_PROGRESS;
    }
#endif
    // Done
    if (gsFmState == 6) {
        return IERR_SUCCESS;
    }

    return IERR_IN_PROGRESS;
}

/**
  * @internal
  * @brief Setup Float Mode
  * @param none
  * @retval none
  */
void FloatModeSetup_PmOnly(void) {
  #if 0
// slotA setting for Ambient. 2 pulses, 20uS float time.
30 0235         # mod pulse at t = 53; precon = 16; float = 53-16+2 = 39
31 0427         # 39 uS period = 30uS + integrator clearance
39 1D10         # standard float mode timing
42 1C25         # 100K tia   200K R4
43 ae65         # TIA - INT - ADC
5e 7008         # Precondition ends at 16 uS
58 0044         # slotA=-+, slotB=-+

34 0000
3c 3206         # Don't Power down amps 2, 3, 4. set bit9.
54 0AA0         # a and b cathodes to vdelta

// SlotB related
14 0441         # slotB[3:2] = 0
35 0235         # mod pulse at t = 53; precon = 16; float = 53-16+2 = 39
36 0227         # 39 uS period = 30uS + integrator clearance
3b 1D10         # standard float mode timing
44 1C24         # 200K tia   400K R4
45 ae65         # TIA - INT - ADC
59 7008         # Precondition ends at 16 uS
58 0040         # -+ in Slot B

23 1530         # LED1
34 0000
3c 33C6         # Power down amps 2, 3, 4. For 1 ch mode. set bit9.
54 09A0         # a cathodes to vref  b to vdelta(?)

3F 5C12         # Slot B 28 uS LED, offset = 0x12 = 18us
5a 9F00         # a=no LED pulse b=float gesture led pulse mask

#endif
    uint16_t temp16;

    AdpdDrvRegWrite(0x12, 0x00028);         // 200hz/4 = 50Hz
    AdpdDrvRegWrite(0x15, 0x0220);
    temp16 = Reg.x14 & 0x0F00;
    if (temp16 == 0x100 || temp16 == 0x300 || temp16 == 0x500)  // use ch1-4
      temp16 = 0x500;   // set to individual channel mode
    else
      temp16 = 0x400;   // ch5-8. set to individual channel mode
    AdpdDrvRegWrite(0x14, temp16 | (Reg.x14 & 0xF0F3));  // must set to 2ch
    AdpdDrvRegWrite(0x34, 0x0300);          // disable LED for dark offset
    // AdpdDrvRegWrite(0x3C, Reg.x3C|0x200);   // set bit9
    // AdpdDrvRegWrite(0x54, 0x0AA0);          // SlotAB cathodes to vdelta
    AdpdDrvRegWrite(0x5a, 0xDF00);          // slotA led N, slotB led N_N_Y_N

    // SlotA fixed
    AdpdDrvRegWrite(0x42, 0x1C25);
    AdpdDrvRegWrite(0x43, 0xae65);
    AdpdDrvRegWrite(0x5e, 0x7008);      // Precondition ends at 16 uS
    AdpdDrvRegWrite(0x58, 0x0044);      // slotA=-+, slotB=-+
    // SetChannelFilter((uint8_t)REG_CH1_OFFSET_A, 0);   // set ch offset

    // SlotB fixed
    // AdpdDrvRegWrite(0x44, 0x1C24);      // 200k
    AdpdDrvRegWrite(0x45, 0xae65);
    AdpdDrvRegWrite(0x59, 0x7008);
    // SetChannelFilter((uint8_t)REG_CH1_OFFSET_B, 0);

    // SlotA timing related
    // temp16 = SlotA_Fl1_T + 16 - 2;      // dumpTime = fl_t1 + 16 - 2
    // temp16 |= 0x200;    // connection time = 2us
    temp16 = SlotA_Fl1_T + 16 - gsConT_A;       // dumpTime = fl_t1 + 16 - 2
    temp16 |= (Reg.x30 & 0xFF00);               // connection time = 2us
    AdpdDrvRegWrite(0x30, temp16);

    temp16 = SlotA_Fl2_T;
    temp16 |= 0x200;    // 2 pulses
    AdpdDrvRegWrite(0x31, temp16);          // 2 pulses, fl_t2 = 26

    // temp16 = SlotA_Fl1_T + 16 - 2 - 3 - 9;  // Int_time = dumpTime - 3 - 9
    temp16 = SlotA_Fl1_T + 16 - gsConT_A - gsIntT_A - 10;
    temp16 <<= 5;
    // temp16 |= 0x1800;     // Integration time = 3us
    temp16 |= (gsIntT_A << 11);
    temp16 |= 0x10;                         // 0.5uS for edge placement margin
    AdpdDrvRegWrite(0x39, temp16);          // Int_st = 20-3-9.5=7.5 0x18F0

    // SlotB timming related
    temp16 = SlotB_Fl1_T + 16 - gsConT_B;   // dumpTime = fl_t1 + 16 - 2
    temp16 |= (Reg.x35 & 0xFF00);           // connection time = 2us
    AdpdDrvRegWrite(0x35, temp16);          // 2us connection time, 0x0235

    temp16 = SlotB_Fl2_T;
    temp16 |= 0x0200;    // 2 pulses
    AdpdDrvRegWrite(0x36, temp16);          // 20/2=10 pulses, 0x1427

    // Int_time = dumpTime - 3 - 9 - edge
    temp16 = SlotB_Fl1_T + 16 - gsConT_B - gsIntT_B - 9 - 1;
    temp16 <<= 5;
    temp16 |= (gsIntT_B << 11);             // Integration time = 3us
    temp16 |= 0x10;                         // 0.5uS for edge placement margin
    AdpdDrvRegWrite(0x3b, temp16);          // 0x1D10

    AdpdDrvRegWrite(0x23, gsLedCurrent);
    AdpdDrvRegWrite(0x25, (Reg.x25 & 0xFFE0) | gsLedTrimCur); // mid Trim
    // AdpdDrvRegWrite(0x3F, 0x4012 | (gsFmWidth << 8));
    temp16 = 0x4000 | gsLedOffset_B;
    temp16 |= (gsFmWidth << 8);
    AdpdDrvRegWrite(0x3F, temp16);
}

/**
  * @internal
  * @brief Set up float LED mode timing base on Float LED width
  * @param none
  * @retval none
  */
static void FloatLedSetFinalTiming()  {
  uint16_t temp16;
  uint8_t dumpT;

  // 1st charge dump time
  dumpT = gsFmWidth + gsLedOffset_B + 1;   // LPW + LedOffset + edge
  temp16 = Reg.x35 & 0xFF00;
  temp16 |= dumpT;
  AdpdDrvRegWrite(0x35, temp16);

  // Pulse period
  temp16 = dumpT + gsConT_B - 16; // LPW + Conn_width + edge
  temp16 |= Reg.x36 & 0xFF00;
  AdpdDrvRegWrite(0x36, temp16);

  // Start Integrate time
  temp16 = dumpT - gsIntT_B - 10;
  temp16 = (temp16 << 5) | (Reg.x3B & 0xF800);
  AdpdDrvRegWrite(0x3B, temp16 | 0x10); // Edge = AFE fine offset = 0.5uS

  // Set to slotA = -+, slotB = -++-
  temp16 = 0x0444;
  AdpdDrvRegWrite(0x58, temp16);

  // Set LED on = -++-
  temp16 = 0x9F00;
  AdpdDrvRegWrite(0x5A, temp16);

  // Restore Ori PD connection
  AdpdDrvRegWrite(0x14, Reg.x14);

  AdpdDrvRegRead(0x35, &temp16);
  AdpdDrvRegRead(0x36, &temp16);
  AdpdDrvRegRead(0x3B, &temp16);
}


/**
  * @internal
  * @brief Float Mode DeInitilization
  * @param none
  * @retval none
  */
void FloatModeDeInit_PmOnly() {
    AdpdMwLibSetMode(ADPDDrv_MODE_IDLE, ADPDDrv_SLOT_OFF, ADPDDrv_SLOT_OFF);
    AdpdDrvRegWrite(0x11, Reg.x11);
    AdpdDrvRegWrite(0x12, Reg.x12);
    AdpdDrvRegWrite(0x15, Reg.x15);
    AdpdDrvRegWrite(0x14, Reg.x14);

    // SlotA related
    AdpdDrvRegWrite(0x30, Reg.x30);
    AdpdDrvRegWrite(0x31, Reg.x31);
    AdpdDrvRegWrite(0x39, Reg.x39);
    AdpdDrvRegWrite(0x42, Reg.x42);
    AdpdDrvRegWrite(0x43, Reg.x43);
    AdpdDrvRegWrite(0x5e, Reg.x5e);
    AdpdDrvRegWrite(0x3e, Reg.x3e);
    AdpdDrvRegWrite(0x5a, Reg.x5a);

    // SlotB related
    AdpdDrvRegWrite(0x35, Reg.x35);
    AdpdDrvRegWrite(0x36, Reg.x36);
    AdpdDrvRegWrite(0x3b, Reg.x3B);
    AdpdDrvRegWrite(0x44, Reg.x44);
    AdpdDrvRegWrite(0x45, Reg.x45);
    AdpdDrvRegWrite(0x59, Reg.x59);
    AdpdDrvRegWrite(0x58, Reg.x58);

    AdpdDrvRegWrite(0x23, Reg.x23);
    AdpdDrvRegWrite(0x25, Reg.x25);
    AdpdDrvRegWrite(0x34, Reg.x34);
    AdpdDrvRegWrite(0x3C, Reg.x3C);
    AdpdDrvRegWrite(0x54, Reg.x54);

    AdpdDrvRegWrite(0x3F, Reg.x3F);
    AdpdDrvRegWrite(0x5a, Reg.x5A);
    return;
}
