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

#define MODULE ("GetCTR.c")

#define A_SATURATE          0x1F00  // use a value less than 0x2000
#define BPF_GAIN2           797     // scale up by 1k
#define BPF_GAIN3           750     // Band pass filter gain with 3uS LED
#define SKIP_NUM            3       // skip the first few data for process
#define PULSE_NUM           1
#define CTR_TIMEOUT         2000     // 500ms

#define AdpdMwLibSetMode(a, b, c)   \
              AdpdDrvSetSlot(b, c); adpd_buff_reset(0); AdpdDrvSetOperationMode(a);

/* Public function prototypes -----------------------------------------------*/
void GetCtrInit_PmOnly(void);
void GetCtrDeInit_PmOnly(void);
INT_ERROR_CODE_t GetCtrValue_PmOnly(uint32_t *rawDataB);

/* Public or external variables ---------------------------------------------*/
uint16_t g_CtrValue;
uint16_t g_TiaG_Init_PmOnly;

AdpdLibConfig_t gLCFG, *g_lcfg_PmOnly;

extern ADI_OSAL_SEM_HANDLE adpd_task_getCtr_sem;

/* Private function prototypes ----------------------------------------------*/
static void adpd_ctr_data_ready_cb(void);

/* Private Variables --------------------------------------------------------*/
static struct Register {
  uint16_t x12;
  uint16_t x15;
  uint16_t x23;
  uint16_t x25;
  uint16_t x34;
  uint16_t x35;
  uint16_t x36;
  uint16_t x44;
}Reg;

static uint8_t gsOperateState;
static uint8_t gsSkipCnt;
static uint16_t gsTiaGain;
static uint16_t gsLedCurrentB;
static uint16_t gsLedPulseB;
static uint16_t gsPreDataVal;

int8_t AdpdDoGetCtr(uint16_t *ctrValue)  {
  INT_ERROR_CODE_t errorCode = IERR_SUCCESS;
  ADPDDrv_Operation_Slot_t slotA_mode, slotB_mode;
  uint16_t slotA_size, slotB_size;
  uint32_t  timestamp = 0;
  uint8_t   tmp[32];
  uint32_t  len, ppgData[4];
  CIRC_BUFF_STATUS_t  status;
  uint32_t nTimeVal0, nTimeVal1;
  uint8_t retCode, i;

  AdpdDrvDataReadyCallback(adpd_ctr_data_ready_cb);
  GetCtrInit_PmOnly();
  AdpdDrvSetOperationMode(ADPDDrv_MODE_IDLE);
  AdpdDrvSetSlot( ADPDDrv_SLOT_OFF, ADPDDrv_4CH_16);
  adpd_buff_reset(0);
  AdpdDrvSetOperationMode(ADPDDrv_MODE_SAMPLE);
  nTimeVal0 = MCU_HAL_GetTick();
  retCode = 1;
  len = sizeof(tmp);
  while(retCode)  {
    // get slotBData
    adi_osal_SemPend(adpd_task_getCtr_sem, ADI_OSAL_TIMEOUT_FOREVER);
    adpd_read_data_to_buffer(&slotA_mode, &slotB_mode, &slotA_size, &slotB_size);
    len = sizeof(tmp);
    status = adpd_buff_get(&tmp[0], &timestamp, &len);
    nTimeVal1 = MCU_HAL_GetTick();
    if ((ABS(nTimeVal1 - nTimeVal0)) > CTR_TIMEOUT)
        break;

    while (status == CIRC_BUFF_STATUS_OK)  {
      for (i=0; i<slotB_size; i+=2) {
        ppgData[i/2] = (tmp[i+1] << 8) | tmp[i];
      }
      adi_printf("ctr chsum data=%x\r\n", ppgData[0]+ppgData[1]+ppgData[2]+ppgData[3]);
      errorCode = GetCtrValue_PmOnly(ppgData);

      if (errorCode != IERR_IN_PROGRESS) {
        retCode = 0;
        *ctrValue = g_CtrValue;
        break;
      }
      status = adpd_buff_get(&tmp[0], &timestamp, &len);
    }
  }
  GetCtrDeInit_PmOnly();
  AdpdDrvSetSlot(ADPDDrv_SLOT_OFF, ADPDDrv_SLOT_OFF);
  AdpdDrvSetOperationMode(ADPDDrv_MODE_IDLE);
  adi_printf("CTR value=%d, start=%d, end=%d, err=%d\r\n", g_CtrValue, nTimeVal0, nTimeVal1,errorCode);
  // debug(MODULE, "CTR value=%d\r\n", g_CtrValue);
  return 0;
}


/**
  * @internal
  * @brief Get CTR Init routine
  * @param none
  * @retval None
  */
void GetCtrInit_PmOnly() {
  AdpdDrvRegRead(REG_SAMPLING_FREQ, &Reg.x12);
  AdpdDrvRegRead(REG_DEC_MODE, &Reg.x15);
  AdpdDrvRegRead(REG_LED1_DRV, &Reg.x23);
  AdpdDrvRegRead(REG_LED_TRIM, &Reg.x25);
  AdpdDrvRegRead(REG_PULSE_MASK, &Reg.x34);
  AdpdDrvRegRead(REG_PULSE_OFFSET_B, &Reg.x35);
  AdpdDrvRegRead(REG_PULSE_PERIOD_B, &Reg.x36);
  AdpdDrvRegRead(REG_AFE_TRIM_B, &Reg.x44);
  gsTiaGain = Reg.x44 & 0xFFFC;   // 200k
  gsLedCurrentB = Reg.x23;
  gsLedPulseB = Reg.x36;
  gsOperateState = 0;
  gsSkipCnt = 0;

  g_lcfg_PmOnly = &gLCFG;
  g_lcfg_PmOnly->devicemode = ADPDDrv_4CH_16;
  g_lcfg_PmOnly->partNum = 108;
}

/**
  * @internal
  * @brief Get CTR DeInit routine
  * @param None
  * @retval None
  */
void GetCtrDeInit_PmOnly() {
  AdpdMwLibSetMode(ADPDDrv_MODE_IDLE, ADPDDrv_SLOT_OFF, ADPDDrv_SLOT_OFF);
  AdpdDrvRegWrite(REG_SAMPLING_FREQ, Reg.x12);
  AdpdDrvRegWrite(REG_DEC_MODE, Reg.x15);
  AdpdDrvRegWrite(REG_LED1_DRV, Reg.x23);
  AdpdDrvRegWrite(REG_LED_TRIM, Reg.x25);
  AdpdDrvRegWrite(REG_PULSE_MASK, Reg.x34);
  AdpdDrvRegWrite(REG_PULSE_PERIOD_B, Reg.x36);
  AdpdDrvRegWrite(REG_AFE_TRIM_B, Reg.x44);
  AdpdMwLibSetMode(ADPDDrv_MODE_SAMPLE,
                   ADPDDrv_SLOT_OFF,
                   ADPDDrv_SUM_32);
}

/**
  * @internal
  * @brief Get CTR value
  * @param 4 channel rawData
  * @retval IN_PROGRESS=in progress, SUCCESS=done, FAIL=error
  */
INT_ERROR_CODE_t GetCtrValue_PmOnly(uint32_t *rawDataB) {
  uint32_t dataVal, tempVal32, temp32;
  uint16_t pulseWidth, afeGainNum, bpfGainNum, temp16;

  if (gsSkipCnt > 0)  {
    gsSkipCnt--;
    return IERR_IN_PROGRESS;
  }
  if (gsOperateState == 0) {
    AdpdMwLibSetMode(ADPDDrv_MODE_IDLE,
                    ADPDDrv_SLOT_OFF,
                    ADPDDrv_SLOT_OFF);
    AdpdDrvRegWrite(REG_SAMPLING_FREQ, 0x000A);
    AdpdDrvRegWrite(REG_DEC_MODE, 0x0550);      // use 800/32=25Hz
    AdpdDrvRegWrite(REG_LED1_DRV, (gsLedCurrentB&0xFFF0)|0x2000);   // use 50mA
    AdpdDrvRegWrite(REG_LED_TRIM, 0x630C);      // LED default Trim
    AdpdDrvRegWrite(REG_PULSE_PERIOD_B, (gsLedPulseB&0xFF)|(PULSE_NUM<<8));
    AdpdDrvRegWrite(REG_PULSE_MASK, 0x0100);    // Turn on Green LED

    gsOperateState = 1;
  }

  if (gsOperateState == 1) {
    AdpdMwLibSetMode(ADPDDrv_MODE_IDLE,
                    ADPDDrv_SLOT_OFF,
                    ADPDDrv_SLOT_OFF);
    AdpdDrvRegWrite(REG_AFE_TRIM_B, gsTiaGain);
    AdpdMwLibSetMode(ADPDDrv_MODE_SAMPLE,
                    ADPDDrv_SLOT_OFF,
                    ADPDDrv_4CH_16);
    gsOperateState = 2;
    gsSkipCnt = SKIP_NUM;
    return IERR_IN_PROGRESS;
  }

  if (gsOperateState == 2) {
    dataVal = (rawDataB[0] > rawDataB[1])? rawDataB[0] : rawDataB[1];
    if (dataVal < rawDataB[2])
      dataVal = rawDataB[2];
    if (dataVal < rawDataB[3])
      dataVal = rawDataB[3];
    gsPreDataVal = dataVal;

    if (dataVal < A_SATURATE)  {   // First check is not staturated
      gsOperateState = 3;
    }  // else stay in this state
    gsTiaGain++;
    if ((gsTiaGain & 0x3) == 0)  { // 25K, should not saturated with 25mA
      gsOperateState = 10;
      gsTiaGain |= 0x3;   // set to 25k
    }

    AdpdMwLibSetMode(ADPDDrv_MODE_IDLE,
                    ADPDDrv_SLOT_OFF,
                    ADPDDrv_SLOT_OFF);
    AdpdDrvRegWrite(REG_AFE_TRIM_B, gsTiaGain);
    AdpdMwLibSetMode(ADPDDrv_MODE_SAMPLE,
                    ADPDDrv_SLOT_OFF,
                    ADPDDrv_4CH_16);
    gsSkipCnt = SKIP_NUM;
    return IERR_IN_PROGRESS;
  }

  if (gsOperateState == 3) {  // Check Saturation
    dataVal = (rawDataB[0] > rawDataB[1])? rawDataB[0] : rawDataB[1];
    if (dataVal < rawDataB[2])
      dataVal = rawDataB[2];
    if (dataVal < rawDataB[3])
      dataVal = rawDataB[3];
    if ((gsTiaGain & 0x3) < 2)    // 1=100k, 0=200
      temp32 = (uint32_t)(dataVal*1.8); // should ~= 2x of the previous tia
    else
      temp32 = (uint32_t)(dataVal*1.4); // 50k and below is not linear
    // Signal double when Gain Double, 100k signal should > 0x1000
    if (dataVal >= A_SATURATE || ((temp32 > gsPreDataVal) & (temp32 > 0x1000))) {
      gsPreDataVal = dataVal;
      gsTiaGain++;            // gain to half. Bit[1:0] is the gain setting
      if ((gsTiaGain & 0x3) == 0)
        return IERR_FAIL;     // saturated in 25K
      AdpdMwLibSetMode(ADPDDrv_MODE_IDLE,
                    ADPDDrv_SLOT_OFF,
                    ADPDDrv_SLOT_OFF);
      AdpdDrvRegWrite(REG_AFE_TRIM_B, gsTiaGain);
      AdpdMwLibSetMode(ADPDDrv_MODE_SAMPLE,
                    ADPDDrv_SLOT_OFF,
                    ADPDDrv_4CH_16);
      gsSkipCnt = SKIP_NUM;
      return IERR_IN_PROGRESS;
    } else {
      g_TiaG_Init_PmOnly = gsTiaGain;
      gsOperateState = 10;
    }
  }

  if (gsOperateState == 10) {
    //  0=processing 1=with result 2=invalid
    pulseWidth = Reg.x35>>8;
    if (pulseWidth == 2)
      bpfGainNum = BPF_GAIN2;
    else
      bpfGainNum = BPF_GAIN3;
    afeGainNum = 1 << (gsTiaGain&0x3);         // R4/Rf=200/x.
    // should x2 here for R5. Double dynamic range
    if (g_lcfg_PmOnly->partNum != 103 && g_lcfg_PmOnly->partNum != 174) {
      afeGainNum *= 2;                // R4/Rf=400/x.
      temp16 = 46;        // LED current * degradation factor
    } else {
      temp16 = 24;        // degradation factor ~= 0.00156
    }

    // Equation: dominator = bpfGainNum*pulseWidth*LED_Current*PULSE_NUM;
    tempVal32 = bpfGainNum*pulseWidth*temp16*PULSE_NUM;  // afeGainNum is inverted
    // should use sum of all ch to calculate ctr.
    dataVal = rawDataB[0] + rawDataB[1] + rawDataB[2] + rawDataB[3];
    temp32 = dataVal*460*afeGainNum;
    // Equation: g_CtrValue = dataVal*460*afeGainNum/tempVal32;
    g_CtrValue = (uint16_t)(temp32/tempVal32);

    debug(MODULE, "CTR=%d, mean=%u\r\n", g_CtrValue, tempVal32);

    return IERR_SUCCESS;
  }
  return IERR_IN_PROGRESS;
}

static void adpd_ctr_data_ready_cb(void) {
  adi_osal_SemPost(adpd_task_getCtr_sem);
}
