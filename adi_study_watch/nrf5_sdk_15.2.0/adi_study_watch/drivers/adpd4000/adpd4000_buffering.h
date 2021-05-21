/**
*******************************************************************************
* @file         adpd4000_buffering.h
* @author       ADI
* @version
* @date         26-Jun-2019
* @brief        Buffering scheme to get data from ADI ADPD400x chip
*
*******************************************************************************
* @attention
*******************************************************************************
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
* This software is intended for use with the ADPD400x part only               *
*                                                                             *
******************************************************************************/
#ifndef __ADPD4000_BUFFERING_H
#define __ADPD4000_BUFFERING_H
#include <string.h>
#include <adpd400x_drv.h>
#include <circular_buffer.h>



#define ADPD_TS_DATA_TYPE uint32_t //!< assign to unsigned integer


/* Enforce struct packing so that the nested structs and unions are laid out
    as expected. */
#if defined __CC_ARM || defined __IAR_SYSTEMS_ICC__  || defined __SES_ARM
#pragma pack(push,1)
#else
#error "WARNING! Your compiler might not support '#pragma pack(1)'! \
  You must add an equivalent compiler directive to pack structs in this file!"
#endif  // defined __CC_ARM || defined __IAR_SYSTEMS_ICC__


// Reset packing outside of this file.
#if defined __CC_ARM || defined __IAR_SYSTEMS_ICC__ || defined __SES_ARM
#pragma pack(pop)
#else
#endif  // defined __CC_ARM || defined __IAR_SYSTEMS_ICC__

CIRC_BUFF_STATUS_t adpd4000_buff_get(uint8_t *p_data,
                                ADPD_TS_DATA_TYPE *p_timestamp,
                                uint32_t *p_data_len);
void adpd4000_buff_init(uint32_t data_sample_sz);
void adpd4000_buff_reset(uint32_t data_sample_sz);
uint8_t adpd4000_read_data_to_buffer(uint16_t *p_slot_sz, uint16_t *max_slot, uint16_t *ch);
uint32_t* Adpd400xDrvGetDebugInfo();
uint32_t Adpd400xDrvGetISRDebugInfo();
#endif  // __ADPD4000_BUFFERING_H
