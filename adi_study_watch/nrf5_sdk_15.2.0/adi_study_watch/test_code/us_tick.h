/********************************************************************************

*/
/*!
*  copyright Analog Devices
* ****************************************************************************
*
* License Agreement
*
* Copyright (c) 2021 Analog Devices Inc.
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

 ********************************************************************************
 * File Name     : us_tick.h
 * Date          : 15-06-2021
 * Description   : .Offer a micro second precision counter.
 * Version       : 1.0
*************************************************************************************************************/
#include <adi_osal.h>
#include <stdio.h>
#include <nrfx_timer.h>


#define ECG_WATER_MARK_LEVEL    16U

/*FIFO_THRESHOLD_TRIGGERS_CNT = 2 * ECG_WATER_MARK_LEVEL/EDA_WATER_MARK_LEVEL */
#define FIFO_THRESHOLD_TRIGGERS_CNT       32U

uint32_t get_micro_sec(void);
uint32_t us_timer_init(void);
void us_timer_deinit(void);
void us_timer_reset(void);
void enable_adxl_ext_trigger(uint8_t nFreq);
void disable_adxl_ext_trigger(uint8_t nFreq);
void set_adxl_trigger_freq(uint8_t nFreq);
void enable_adpd_ext_trigger(uint16_t nOdr);
void disable_adpd_ext_trigger(uint16_t nOdr);
void enable_ad5940_ext_trigger(uint16_t nOdr);
void disable_ad5940_ext_trigger(uint16_t nOdr);
