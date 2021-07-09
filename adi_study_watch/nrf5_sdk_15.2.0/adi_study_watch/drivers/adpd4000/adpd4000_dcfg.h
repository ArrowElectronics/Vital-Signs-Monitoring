/**
***************************************************************************
* @file         adpd4000_dcfg.h
* @author       ADI
* @version
* @date         26-Jun-2019
* @brief        ADPD400x device configuration header file
*
***************************************************************************
* @attention
***************************************************************************
*/
/******************************************************************************
*                                                                             *
* License Agreement                                                           *
*                                                                             *
* Copyright (c) 2020 Analog Devices Inc.                                      *
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
* This software is intended for use with the ADPD400x part                    *
* only                                                                        *
*                                                                             *
******************************************************************************/


#ifndef __ADPD4000DCFG_H
#define __ADPD4000DCFG_H

#include <stdint.h>
#include <printf.h>

typedef enum {
  ADPD4000_DCFG_STATUS_OK = 0,
  ADPD4000_DCFG_STATUS_ERR,
  ADPD4000_DCFG_STATUS_NULL_PTR,
} ADPD4000_DCFG_STATUS_t;

ADPD4000_DCFG_STATUS_t load_adpd4000_cfg(uint16_t device_id);
ADPD4000_DCFG_STATUS_t stage_adpd4000_dcfg(uint16_t *p_device_id);
ADPD4000_DCFG_STATUS_t load_adpd4000_dcfg(uint16_t device_id);
ADPD4000_DCFG_STATUS_t get_adpd4k_dcfg(uint16_t slot_id, uint16_t app_id, uint8_t index, uint8_t slot_nums);
ADPD4000_DCFG_STATUS_t read_adpd4000_dcfg(uint32_t *p_dcfg, uint16_t *p_dcfg_size);
ADPD4000_DCFG_STATUS_t write_adpd4000_dcfg(uint32_t *p_dcfg);
ADPD4000_DCFG_STATUS_t load_temperature_dcfg ();
void adpd4000_fw_dcfg_clear(void);

typedef enum 
{
  ADPD4000_DCB_STATUS_OK = 0,
  ADPD4000_DCB_STATUS_ERR,
  ADPD4000_DCB_STATUS_NULL_PTR,
} ADPD4000_DCB_STATUS_t;

void adpd4000_update_dcb_present_flag(void);
void adpd4000_set_dcb_present_flag(bool set_flag);
bool adpd4000_get_dcb_present_flag(void);
ADPD4000_DCB_STATUS_t load_adpd4000_dcb(uint16_t device_id); 
ADPD4000_DCB_STATUS_t read_adpd4000_dcb(uint32_t *adpd4000_dcb_data, uint16_t* read_size);
ADPD4000_DCB_STATUS_t write_adpd4000_dcb(uint32_t *adpd4000_dcb_data, uint16_t in_size);
ADPD4000_DCB_STATUS_t delete_adpd4000_dcb(void);
ADPD4000_DCB_STATUS_t clear_dcb(void);
void adpd4000_dcb_dcfg_clear(void);

typedef enum {
  ECG4k_LCFG_FS = 0,
  ECG4k_LCFG_MAX,
} ECG4k_LCFG_t;

ADPD4000_DCFG_STATUS_t Set_adpd4000_SamplingFreq(uint16_t value);
ADPD4000_DCFG_STATUS_t Ecg4kSetLCFG(uint8_t field, uint16_t value);
ADPD4000_DCFG_STATUS_t Ecg4kgGetLCFG(uint8_t field, uint16_t *value);
void DG2502_SW_control_ADPD4000(uint8_t sw_enable);
//#define MAXADPD4000DCBSIZE (50) //Max size of adpd4000 DCB size in double word length; 50 uint32_t elements in dcfg

#endif // __ADPD4000DCFG_H
