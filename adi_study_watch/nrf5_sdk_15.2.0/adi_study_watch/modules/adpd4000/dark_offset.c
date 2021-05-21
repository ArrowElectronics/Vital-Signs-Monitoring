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

#define MODULE      ("DarkOffset.c")
#define DARK_OFFSET_SKIP    10      // Amount of samples to skip after LED OFF
#define SKIP_NUM            3       // skip the first few data for process
#define DARK_OFFSET_AVG     10      // Amount of samples for DOS calculation
#define MODULE_TIMEOUT      2000     // 500ms

/* Public function prototypes -----------------------------------------------*/
void AdpdLibSetDarkOffset_PmOnly(void);

/* Public or external variables ---------------------------------------------*/
extern ADI_OSAL_SEM_HANDLE adpd_task_setDarkValue_sem;

/* Private function prototypes ----------------------------------------------*/
static void DarkOffsetCalculationInit(void);
static void DarkOffsetCalculationDeInit(void);
static uint8_t DarkOffsetCalculation(uint32_t *);
static void adpd_ctr_data_ready_cb(void);

/* Private Variables --------------------------------------------------------*/
static struct Register {
  uint16_t x12;
  uint16_t x15;
  uint16_t x14;
  uint16_t x34;
  uint16_t x36;
}Reg;

static uint8_t  gsRegDoc[4] = {0x1E, 0x1F, 0x20, 0x21};
static uint16_t gsSampleCnt = 0;

static uint32_t gnTempData[32];       // to compatible with adpdlib
static uint32_t *gsDarkOffsetSum;
static uint16_t *gsOldIniOffset, *gsDarkOffsetAvg;

int8_t AdpdSetDarkValue(uint16_t *darkValue)  {
  ADPDDrv_Operation_Slot_t slotA_mode, slotB_mode;
  uint16_t slotA_size, slotB_size;
  uint32_t  timestamp = 0;
  uint8_t   tmp[32];
  uint32_t  len, ppgData[4];
  CIRC_BUFF_STATUS_t  status;
  uint32_t nTimeVal0, nTimeVal1;
  uint8_t retCode, i;

  AdpdDrvDataReadyCallback(adpd_ctr_data_ready_cb);
  gsSampleCnt = 0;
  DarkOffsetCalculationInit();

  nTimeVal0 = MCU_HAL_GetTick();
  retCode = 1;
  len = sizeof(tmp);
  while(retCode)  {
    // get slotBData
    adi_osal_SemPend(adpd_task_setDarkValue_sem, ADI_OSAL_TIMEOUT_FOREVER);
    adpd_read_data_to_buffer(&slotA_mode, &slotB_mode, &slotA_size, &slotB_size);
    len = sizeof(tmp);
    status = adpd_buff_get(&tmp[0], &timestamp, &len);
    nTimeVal1 = MCU_HAL_GetTick();
    if ((ABS(nTimeVal1 - nTimeVal0)) > MODULE_TIMEOUT)
        break;

    while (status == CIRC_BUFF_STATUS_OK)  {
      for (i=0; i<slotB_size; i+=2)
        ppgData[i/2] = (tmp[i+1] << 8) | tmp[i];
      adi_printf("input raw data=%x, %x, %x, %x\r\n", ppgData[0], ppgData[1], ppgData[2], ppgData[3]);
      status = adpd_buff_get(&tmp[0], &timestamp, &len);

      if (gsSampleCnt++ < DARK_OFFSET_SKIP)
        continue;

      if (DarkOffsetCalculation(ppgData) == 0)
        continue;    // in progress

      if (darkValue != 0)  {
        for (i = 0; i < 4; i++)
          darkValue[i] = gsDarkOffsetAvg[i];
        retCode = 0;
        break;
      }

    }
  }
  // DarkOffsetCalculationDeInit();
  AdpdLibSetDarkOffset_PmOnly();
  AdpdDrvSetSlot(ADPDDrv_SLOT_OFF, ADPDDrv_SLOT_OFF);
  AdpdDrvSetOperationMode(ADPDDrv_MODE_IDLE);
  adi_printf("start=%d, end=%d\r\n", nTimeVal0, nTimeVal1);
  return 0;
}

/**
  * @brief set the dark offset values that was previous calculated
  * @param none.
  * @retval none
  */
void AdpdLibSetDarkOffset_PmOnly() {
  for (uint8_t i = 0; i < 4; i++) {
    if (gsDarkOffsetAvg[i] != 0)  {
      AdpdDrvRegWrite(gsRegDoc[i], gsDarkOffsetAvg[i]);
    }
  }
}

/**
  * @brief initialization routine for calculating the dark offset values
  * @param none.
  * @retval none
  */
static void DarkOffsetCalculationInit() {
  uint8_t i;

  gsDarkOffsetSum = &gnTempData[0];
  gsOldIniOffset = (uint16_t*)&gnTempData[8];
  gsDarkOffsetAvg = (uint16_t*)&gnTempData[16];

  for (i = 0; i < 4; i++)
    gsDarkOffsetSum[i] = 0;

  AdpdMwLibSetMode(ADPDDrv_MODE_IDLE, ADPDDrv_SLOT_OFF, ADPDDrv_SLOT_OFF);
  AdpdDrvRegRead(g_reg_base + REG_PULSE_PERIOD_B, &Reg.x36);
  AdpdDrvRegRead(g_reg_base + REG_PULSE_MASK, &Reg.x34);
  AdpdDrvRegRead(g_reg_base + REG_PD_SELECT, &Reg.x14);
  AdpdDrvRegRead(g_reg_base + REG_SAMPLING_FREQ, &Reg.x12);
  AdpdDrvRegRead(g_reg_base + REG_DEC_MODE, &Reg.x15);

  AdpdDrvRegWrite(g_reg_base + REG_PULSE_PERIOD_B, (Reg.x36&0xFF)|0x100);  // use 1 pulse
  AdpdDrvRegWrite(g_reg_base + REG_PULSE_MASK, (Reg.x34|0x0300));          // leds Off
  AdpdDrvRegWrite(g_reg_base + REG_PD_SELECT, (Reg.x14&0xF00F));           // Disconnect PD
  AdpdDrvRegWrite(g_reg_base + REG_SAMPLING_FREQ, 0x000A);
  AdpdDrvRegWrite(g_reg_base + REG_DEC_MODE, 0x0330);                      // 800/8 hez

  for (i = 0; i < 4; i++) {
    AdpdDrvRegRead(g_reg_base + gsRegDoc[i], &gsOldIniOffset[i]);
    // if (gsOldIniOffset[i] > 0x2000)
    //  continue;
    AdpdDrvRegWrite(g_reg_base + gsRegDoc[i], 0);
  }
  // enable both slot
  AdpdMwLibSetMode(ADPDDrv_MODE_SAMPLE, ADPDDrv_SLOT_OFF, ADPDDrv_4CH_16);
}

/**
  * @brief de-initialization routine of the dark offset calculation function
  * @param none.
  * @retval none
  */
static void DarkOffsetCalculationDeInit() {
  gsSampleCnt = 0;
  AdpdDrvRegWrite(REG_PULSE_PERIOD_B, Reg.x36);
  AdpdDrvRegWrite(REG_PULSE_MASK, Reg.x34);
  AdpdDrvRegWrite(REG_PD_SELECT, Reg.x14);
  AdpdDrvRegWrite(REG_SAMPLING_FREQ, Reg.x12);
  AdpdDrvRegWrite(REG_DEC_MODE, Reg.x15);
  // for (uint8_t i = 0; i < 4; i++)
  //   AdpdDrvRegWrite(gsRegDoc[i], gsOldIniOffset[i]);
}

/**
  * @brief returns the dark offset values by averaging the channel data from each slot
  *                using the DARK_OFFSET_AVG averaging factor
  * @param slotACh pointer to an array of uint32_t that contains the
  *                input data for which to calculate the dark offset. Where
  *                elements 0 to 3 indicates the channels of slot A data
  * @param slotBCh pointer to an array of uint32_t that contains the
  *                input data for which to calculate the dark offset. Where
  *                elements 0 to 3 indicates the channels of slot B data
  * @retval uint8_t  1 - success in processing the raw data
  *                  0 - calculation is not done as sufficient number of samples is
  *                      not obtained. Hence it indicates data collection in progress
  */
static uint8_t DarkOffsetCalculation(uint32_t *slotBCh) {
  uint8_t i;

  for (i = 0; i < 4; i++) {
    gsDarkOffsetSum[i] += slotBCh[i];
  }

  // averaging below after collecting 10 samples
  if (gsSampleCnt == DARK_OFFSET_SKIP + DARK_OFFSET_AVG) {
    for (i = 0; i < 4; i++)
      gsDarkOffsetAvg[i] = gsDarkOffsetSum[i]/DARK_OFFSET_AVG;

    DarkOffsetCalculationDeInit();
    return 1;
  }
  return 0;
}

static void adpd_ctr_data_ready_cb(void) {
  adi_osal_SemPost(adpd_task_setDarkValue_sem);
}
