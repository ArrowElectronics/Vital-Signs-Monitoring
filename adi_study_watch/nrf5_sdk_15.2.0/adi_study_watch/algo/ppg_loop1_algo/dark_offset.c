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
* This software is intended for use with the ADPD and derivative parts    *
* only                                                                        *
*                                                                             *
******************************************************************************/
#include <stdio.h>
#include "adpd400x_lib.h"
#include "adpd400x_lib_common.h"
#include "variables.h"

#define MODULE      ("DarkOffset.c")
#define CHANNEL_NUM         2       // two channels per slot
#define DARK_OFFSET_SKIP    10      // Amount of samples to skip after LED OFF
#define DARK_OFFSET_AVG     10      // Amount of samples for DOS calculation

//================== LOG LEVELS=============================================//
#define NRF_LOG_MODULE_NAME PPG_LIB

#if PPG_LIB_CONFIG_LOG_ENABLED
#ifdef NRF_LOG_DEFAULT_LEVEL
#undef NRF_LOG_DEFAULT_LEVEL
#define NRF_LOG_DEFAULT_LEVEL  PPG_LIB_CONFIG_LOG_LEVEL
#endif
#define NRF_LOG_INFO_COLOR  PPG_LIB_CONFIG_INFO_COLOR
#define NRF_LOG_DEBUG_COLOR  PPG_LIB_CONFIG_DEBUG_COLOR
#else //PPG_LIB_CONFIG_LOG_ENABLED
#define NRF_LOG_LEVEL       0
#endif //PPG_LIB_CONFIG_LOG_ENABLED
#include "nrf_log.h"

/* Public function prototypes -----------------------------------------------*/
void Adpd400xLibGetDarkOffsetInit(void);
ADPDLIB_ERROR_CODE_t Adpd400xLibGetDarkOffset(uint32_t* rawData, uint16_t* dValue);
void Adpd400xLibSetDarkOffset(void);

/* Private function prototypes ----------------------------------------------*/
static void DarkOffsetCalculationInit(void);
static void DarkOffsetCalculationDeInit(void);
static uint8_t DarkOffsetCalculation(uint32_t *);

/* Private Variables --------------------------------------------------------*/
static struct Register {
  uint16_t x102;
  uint16_t x105;
  uint16_t x106;
  uint16_t x107;
}Reg;

static uint16_t  gsRegDoc[CHANNEL_NUM] = {ADPD400x_REG_ADC_OFF1_A, ADPD400x_REG_ADC_OFF2_A};
static uint16_t gsSampleCnt = 0;

static uint32_t *gsDarkOffsetSum;
static uint16_t *gsOldIniOffset, *gsDarkOffsetAvg;

/**
  * @brief Initializes the Dark Offset routine for calling
  *        ADPDLib_GetDarkOffset.
  * @retval void
  */
void Adpd400xLibGetDarkOffsetInit() {
  gsSampleCnt = 0;
}

/**
  * @brief returns the dark offset values given input data
  * @param rawData pointer to an array of uint32_t that contains the
  *                input data for which to calculate the dark offset. Where
  *                elements 0 to 3 contains slot A data and elements 4 to 7
  *                contains slot B data.
  * @param dValue    pointer to an array of uint32_t that contains the
  *                  resulting dark offset calculation. Where elements 0 to 3
  *                  contains slot A dark offset data and elements 4 to 7
  *                  contains slot B dark offset data.
  * @retval uint32_t ADPDLIB_ERR_SUCCESS - success in processing the raw data
  *                  ADPDLIB_ERR_SUCCESS_WITH_RESULT - success with dark offset
  *                                                    values.
  *                  ADPDLIB_ERR_SUCCESS - error in processing the raw data
  */
ADPDLIB_ERROR_CODE_t Adpd400xLibGetDarkOffset(uint32_t* rawData, uint16_t* dValue) {
  uint8_t i;

  if (gsSampleCnt == 0)
    DarkOffsetCalculationInit();

  // skip samples to make LED to go off completely
  if (gsSampleCnt++ < DARK_OFFSET_SKIP)
    return ADPDLIB_ERR_IN_PROGRESS;

  if (DarkOffsetCalculation(rawData) == 0)
    return ADPDLIB_ERR_IN_PROGRESS;

  if (dValue != 0)  {
    for (i = 0; i < CHANNEL_NUM; i++)
      dValue[i] = gsDarkOffsetAvg[i];
  }

  // DarkOffsetCalculationDeInit();
  gsSampleCnt = 0;
  return ADPDLIB_ERR_SUCCESS_WITH_RESULT;
}

/**
  * @brief set the dark offset values that was previous calculated
  * @param none.
  * @retval none
  */
void Adpd400xLibSetDarkOffset() {
  uint8_t i;
  g_reg_base = log2(gAdpd400x_lcfg->targetSlots) * 0x20;
  for (i = 0; i < CHANNEL_NUM; i++) {
    if (gsDarkOffsetAvg[i] != 0 && gsOldIniOffset[i] != 0)  {
      AdpdDrvRegWrite(g_reg_base + gsRegDoc[i], gsDarkOffsetAvg[i]);
      gAdpd400xOptmVal.darkOffset[i] = gsDarkOffsetAvg[i];
      //debug(MODULE, "Dark OS %d=%04x", i, gsDarkOffsetAvg[i]);
	  NRF_LOG_DEBUG("Dark OS %d=%04x", i, gsDarkOffsetAvg[i]);
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
  g_reg_base = log2(gAdpd400x_lcfg->targetSlots) * 0x20;
  gsDarkOffsetSum = &gnAdpd400xTempData[0];
  gsOldIniOffset = (uint16_t*)&gnAdpd400xTempData[8];
  gsDarkOffsetAvg = (uint16_t*)&gnAdpd400xTempData[16];

  for (i = 0; i < CHANNEL_NUM; i++)
    gsDarkOffsetSum[i] = 0;

  AdpdMwLibSetMode(ADPD400xDrv_MODE_IDLE, ADPD400xDrv_SIZE_0, ADPD400xDrv_SIZE_0);

  AdpdDrvRegRead(g_reg_base + ADPD400x_REG_INPUTS_A, &Reg.x102);
  AdpdDrvRegRead(g_reg_base + ADPD400x_REG_LED_POW12_A, &Reg.x105);
  AdpdDrvRegRead(g_reg_base + ADPD400x_REG_LED_POW34_A, &Reg.x106);
  AdpdDrvRegRead(g_reg_base + ADPD400x_REG_COUNTS_A, &Reg.x107);

  AdpdDrvRegWrite(g_reg_base + ADPD400x_REG_LED_POW12_A, 0);       // leds Off
  AdpdDrvRegWrite(g_reg_base + ADPD400x_REG_LED_POW34_A, 0);
  AdpdDrvRegWrite(g_reg_base + ADPD400x_REG_INPUTS_A, 0);          // Disconnect PD
  AdpdDrvRegWrite(g_reg_base + ADPD400x_REG_COUNTS_A, 0x0101);     // use 1 pulse

  for (i = 0; i < CHANNEL_NUM; i++) {
    AdpdDrvRegRead(g_reg_base + gsRegDoc[i], &gsOldIniOffset[i]);
    // if (gsOldIniOffset[i] > 0x2000)
    //  continue;
    AdpdDrvRegWrite(g_reg_base + gsRegDoc[i], 0);
  }
  // enable both slot
  AdpdMwLibSetMode(ADPD400xDrv_MODE_SAMPLE, ADPD400xDrv_SIZE_0, ADPD400xDrv_SIZE_32);
}

/**
  * @brief de-initialization routine of the dark offset calculation function
  * @param none.
  * @retval none
  */
static void DarkOffsetCalculationDeInit() {
  gsSampleCnt = 0;
  g_reg_base = log2(gAdpd400x_lcfg->targetSlots) * 0x20;
  AdpdDrvRegWrite(g_reg_base + ADPD400x_REG_INPUTS_A, Reg.x102);
  AdpdDrvRegWrite(g_reg_base + ADPD400x_REG_LED_POW12_A, Reg.x105);
  AdpdDrvRegWrite(g_reg_base + ADPD400x_REG_LED_POW34_A, Reg.x106);
  AdpdDrvRegWrite(g_reg_base + ADPD400x_REG_COUNTS_A, Reg.x107);

  for (uint8_t i = 0; i < CHANNEL_NUM; i++)
    AdpdDrvRegWrite(g_reg_base + gsRegDoc[i], gsOldIniOffset[i]);
}

/**
  * @brief returns the dark offset values by averaging the channel data from each slot
  *                using the DARK_OFFSET_AVG averaging factor
  * @param slotData pointer to an array of uint32_t that contains the
  *                input data for which to calculate the dark offset. Where
  *                elements 0 to 3 indicates the channels of slot B data
  * @retval uint8_t  1 - success in processing the raw data
  *                  0 - calculation is not done as sufficient number of samples is
  *                      not obtained. Hence it indicates data collection in progress
  */
static uint8_t DarkOffsetCalculation(uint32_t *slotData) {
  uint8_t i;

  for (i = 0; i < CHANNEL_NUM; i++) {
    gsDarkOffsetSum[i] += slotData[i];
  }

  // averaging below after collecting 10 samples
  if (gsSampleCnt == DARK_OFFSET_SKIP + DARK_OFFSET_AVG) {
    for (i = 0; i < CHANNEL_NUM; i++)
      gsDarkOffsetAvg[i] = gsDarkOffsetSum[i]/DARK_OFFSET_AVG;

    /* debug(MODULE, "Dark OS =%04x %04x %04x %04x\n",
          gsDarkOffsetAvg[0], gsDarkOffsetAvg[1], \
          gsDarkOffsetAvg[2], gsDarkOffsetAvg[3]);*/
    NRF_LOG_DEBUG("Dark OS =%04x %04x %04x %04x\n",
          gsDarkOffsetAvg[0], gsDarkOffsetAvg[1], \
          gsDarkOffsetAvg[2], gsDarkOffsetAvg[3]);

    DarkOffsetCalculationDeInit();
    return 1;
  }
  return 0;
}


