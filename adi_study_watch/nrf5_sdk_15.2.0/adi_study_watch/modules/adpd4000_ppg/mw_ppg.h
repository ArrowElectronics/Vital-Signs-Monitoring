/**
***************************************************************************
* @file    /Inc/MwPPG.h
* @author  ADI Team
* @version V0.0.1
* @date    23-Aug-2016
* @brief   Header for MwPPG.c module
***************************************************************************
*/
/*!
 *  \copyright Analog Devices
 * ****************************************************************************
 *
 * License Agreement
 *
 * Copyright (c) 2019 Analog Devices Inc.
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
#ifndef _MWPPG_H_
#define _MWPPG_H_
#include "adpd400x_lib_common.h"
typedef enum {
  PPG_ERROR = 0,
  PPG_SUCCESS = 1,
} PPG_ERROR_CODE_t;

typedef struct {
  char      aNameStr[40];
  uint16_t  nMajor;
  uint16_t  nMinor;
  uint16_t  nPatch;
} PpgAlgoVersion_t;

ADPDLIB_ERROR_CODE_t MwPPG_HeartRateInit(void) ;
uint8_t MwPPG_HeartRateDeInit(void);
ADPDLIB_ERROR_CODE_t MwPPG_HeartRate(LibResultX_t*,
                             uint32_t *slotB,
                             int16_t *accl,
                             TimeStamps_t ts);


PPG_ERROR_CODE_t MwPPG_GetStates(uint8_t *states);
void MwPPG_GetStateInfo(uint8_t state, uint16_t *info);

PPG_ERROR_CODE_t MwPPG_WriteLCFG(uint8_t field, uint32_t value);
PPG_ERROR_CODE_t MwPPG_ReadLCFG(uint8_t index, uint32_t *value);
PPG_ERROR_CODE_t MwPpg_LoadLCFG(uint16_t device_id);
PPG_ERROR_CODE_t MwPPG_ReadLCFGStruct(uint32_t *value, uint8_t *nSize);

PPG_ERROR_CODE_t MwPpg_LoadppgLCFG(uint16_t device_id);

#ifdef DCB
void ppg_update_dcb_present_flag(void);
bool ppg_get_dcb_present_flag(void);
void ppg_set_dcb_present_flag(bool set_flag);
void ppg_dcb_clear(void);
PPG_ERROR_CODE_t delete_ppg_dcb(void);
PPG_ERROR_CODE_t write_ppg_dcb(uint32_t *ppg_dcb_data, uint16_t write_Size);
PPG_ERROR_CODE_t read_ppg_dcb(uint32_t *ppg_dcb_data, uint16_t* read_size);
PPG_ERROR_CODE_t load_ppg_dcb(uint16_t device_id);
PPG_ERROR_CODE_t stage_ppg_dcb(uint16_t *p_device_id);
#endif

void PpgLibGetAlgorithmVendorAndVersion(PpgAlgoVersion_t *pAlgoInfo);
uint8_t MwPpgGetLibState (void);
void MwPpgGetLib_CTR_Value(uint16_t *ctr_val);
void MwPpgGetLib_AGC_SIGM(uint16_t *sig_val);
#endif /* _MWPPG_H_ */
