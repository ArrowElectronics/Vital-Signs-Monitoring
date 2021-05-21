
/**
    ***************************************************************************
    * @file    ad7156_dcfg.h
    * @author  ADI Team
    * @version V0.0.1
    * @date    17-August-2020
    * @brief   AD7156 default configuration header file
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

#ifndef __AD7156DCFG_H
#define __AD7156DCFG_H
#include <stdint.h>
#include <printf.h>

typedef enum {
  AD7156_DCFG_STATUS_OK = 0,
  AD7156_DCFG_STATUS_ERR,
  AD7156_DCFG_STATUS_NULL_PTR,
} AD7156_DCFG_STATUS_t;


AD7156_DCFG_STATUS_t load_ad7156_dcfg();
AD7156_DCFG_STATUS_t read_ad7156_dcfg(uint16_t *p_dcfg, uint8_t *p_dcfg_size);
AD7156_DCFG_STATUS_t write_ad7156_dcfg(uint16_t *p_dcfg, uint16_t Size);
AD7156_DCFG_STATUS_t stage_ad7156_dcfg();
static int8_t LoadDcfg(uint16_t *pCfg, uint16_t nBufSize);
void ad7156_dcfg_clear(void);

typedef enum {
  AD7156_DCB_STATUS_OK = 0,
  AD7156_DCB_STATUS_ERR,
  AD7156_DCB_STATUS_NULL_PTR,
} AD7156_DCB_STATUS_t;

AD7156_DCB_STATUS_t load_ad7156_cfg();
void ad7156_update_dcb_present_flag(void);
bool ad7156_get_dcb_present_flag(void);
void ad7156_set_dcb_present_flag(bool set_flag);
void ad7156_dcb_clear(void);
AD7156_DCB_STATUS_t stage_ad7156_dcb();
AD7156_DCB_STATUS_t load_ad7156_dcb();
AD7156_DCB_STATUS_t read_ad7156_dcb(uint32_t *ad7156_dcb_data, uint16_t* read_size);
AD7156_DCB_STATUS_t write_ad7156_dcb(uint32_t *ad7156_dcb_data, uint16_t write_Size);
AD7156_DCB_STATUS_t delete_ad7156_dcb(void);
AD7156_DCB_STATUS_t clear_ad7156_dcb(void);

#endif // __AD7156DCFG_H
