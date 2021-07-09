
/**
    ***************************************************************************
    * @file    adxl_dcfg.h
    * @author  ADI Team
    * @version V0.0.1
    * @date    28-July-2016
    * @brief   ADXL default configuration headerfile
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

#ifndef __ADXLDCFG_H
#define __ADXLDCFG_H
#include <stdint.h>
#include <printf.h>

typedef enum {
  ADXL_DCFG_STATUS_OK = 0,
  ADXL_DCFG_STATUS_ERR,
  ADXL_DCFG_STATUS_NULL_PTR,
} ADXL_DCFG_STATUS_t;


ADXL_DCFG_STATUS_t load_adxl_dcfg(uint16_t device_id);
ADXL_DCFG_STATUS_t read_adxl_dcfg(uint32_t *p_dcfg, uint8_t *p_dcfg_size);
ADXL_DCFG_STATUS_t write_adxl_dcfg(uint16_t *p_dcfg, uint16_t Size);
ADXL_DCFG_STATUS_t stage_adxl_dcfg(uint16_t *p_device_id);
static int8_t LoadDcfg(uint16_t *pCfg, uint16_t nBufSize);
void adxl_dcfg_clear(void);

typedef enum {
  ADXL_DCB_STATUS_OK = 0,
  ADXL_DCB_STATUS_ERR,
  ADXL_DCB_STATUS_NULL_PTR,
} ADXL_DCB_STATUS_t;

ADXL_DCB_STATUS_t load_adxl_cfg(uint16_t device_id);
void adxl_update_dcb_present_flag(void);
bool adxl_get_dcb_present_flag(void);
void adxl_set_dcb_present_flag(bool set_flag);
void adxl_dcb_clear(void);
ADXL_DCB_STATUS_t stage_adxl_dcb(uint16_t *p_device_id);
ADXL_DCB_STATUS_t load_adxl_dcb(uint16_t device_id);
ADXL_DCB_STATUS_t read_adxl_dcb(uint32_t *adxl_dcb_data, uint16_t* read_size);
ADXL_DCB_STATUS_t write_adxl_dcb(uint32_t *adxl_dcb_data, uint16_t write_Size);
ADXL_DCB_STATUS_t delete_adxl_dcb(void);
ADXL_DCB_STATUS_t clear_adxl_dcb(void);

#endif // __ADXLDCFG_H
