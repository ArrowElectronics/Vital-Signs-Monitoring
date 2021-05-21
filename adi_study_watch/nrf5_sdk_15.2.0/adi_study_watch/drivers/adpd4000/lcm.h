/**
***************************************************************************
* @file         lcm.h
* @author       ADI
* @version
* @date         26-Jun-2019
* @brief        Header file contains lcm wrapper
*
***************************************************************************
* @attention
***************************************************************************
*/
/******************************************************************************
*                                                                             *
* License Agreement                                                           *
*                                                                             *
* Copyright (c) 2021 Analog Devices Inc.                                      *
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
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <hw_if_config.h>

/*!
 * @brief:  Data type to store ADPD slot and sample size 
 */
typedef struct {
  char slot_info[SLOT_NUM]; //!< Slot number
  uint32_t sample_size;     //!< Slot sample size
}fifo_data_pattern;

uint32_t calculate_lcm(adpd400xDrv_slot_t *slot, uint16_t highest_slot_num);
ADI_HAL_STATUS_t get_current_datapattern(uint32_t *seq_no, adpd400xDrv_slot_t *slot,
                                          uint32_t lcm_value, uint8_t highest_slot_num,
                                  fifo_data_pattern* p_adpd_cl_datapattern);
uint16_t get_samples_size(uint16_t no_of_samples, adpd400xDrv_slot_t *slot,
                           uint32_t *seq_no,
                              uint32_t lcm_value, uint16_t high_slot_num);
uint16_t get_max_sample_size(uint32_t seq_no, adpd400xDrv_slot_t *slot, uint8_t high_slot_num);
