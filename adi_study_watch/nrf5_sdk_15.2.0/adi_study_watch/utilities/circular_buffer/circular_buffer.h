/**
****************************************************************************
* @file     circular_buffer.h
* @author   ADI
* @version  V0.1
* @date     04-Dec-2019
* @brief    This header file file is used for circular buffer implementation.
****************************************************************************
* @attention
******************************************************************************
* Copyright (c) 2021 Analog Devices, Inc.  All rights reserved.
*
* Redistribution and use in source and binary forms, with or without
* modification, are permitted provided that the following conditions are met:
* - Redistributions of source code must retain the above copyright notice, this
*   list of conditions and the following disclaimer.
* - Redistributions in binary form must reproduce the above copyright notice,
*   this list of conditions and the following disclaimer in the documentation
*   and/or other materials provided with the distribution.
* - Modified versions of the software must be conspicuously marked as such.
* - This software is licensed solely and exclusively for use with
*   processors/products manufactured by or for Analog Devices, Inc.
* - This software may not be combined or merged with other code in any manner
*   that would cause the software to become subject to terms and conditions
*   which differ from those listed here.
* - Neither the name of Analog Devices, Inc. nor the names of its contributors
*   may be used to endorse or promote products derived from this software
*   without specific prior written permission.
* - The use of this software may or may not infringe the patent rights of one
*   or more patent holders.  This license does not release you from the
*   requirement that you obtain separate licenses from these patent holders to
*   use this software.
*
* THIS SOFTWARE IS PROVIDED BY ANALOG DEVICES, INC. AND CONTRIBUTORS "AS IS"
* AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
* NONINFRINGEMENT, TITLE, MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
* ARE DISCLAIMED. IN NO EVENT SHALL ANALOG DEVICES, INC. OR CONTRIBUTORS BE
* LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, PUNITIVE OR
* CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, DAMAGES ARISING OUT OF
* CLAIMS OF INTELLECTUAL PROPERTY RIGHTS INFRINGEMENT; PROCUREMENT OF
* SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
* INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
* CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
* ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
* POSSIBILITY OF SUCH DAMAGE.
******************************************************************************/

#ifndef __CIRCULAR_BUFFER_H
#define __CIRCULAR_BUFFER_H
#include <string.h>
#include <stdint.h>
typedef enum {
  CIRC_BUFF_STATUS_OK = 0,
  CIRC_BUFF_STATUS_ERROR,
  CIRC_BUFF_STATUS_EMPTY,
  CIRC_BUFF_STATUS_FULL,
  CIRC_BUFF_STATUS_OVERFLOW,
  CIRC_BUFF_STATUS_NULL_PTR,
} CIRC_BUFF_STATUS_t;


typedef struct _circular_buffer_t {
  uint8_t   *buffer;
  uint8_t   *buffer_end;    // Address of the last space in the buffer
  void      *head;
  void      *tail;
  uint32_t  num_elements;   // number of elements currently stored in the buffer
  uint32_t  capacity;       // number of elements that can be stored in the buffer
  uint32_t  buffer_sz;      // size of the internal byte buffer
  uint32_t  element_sz;     // size of the elements in the buffer
} circular_buffer_t;

CIRC_BUFF_STATUS_t circular_buffer_init(circular_buffer_t *p_cbuff,
                                        uint8_t *p_mem,
                                        uint32_t mem_sz,
                                        uint32_t element_sz);
CIRC_BUFF_STATUS_t circular_buffer_reset(circular_buffer_t *p_cbuff, uint32_t element_sz);
CIRC_BUFF_STATUS_t circular_buffer_put(circular_buffer_t *p_cbuff, void *item);
CIRC_BUFF_STATUS_t circular_buffer_get(circular_buffer_t *p_cbuff, void *item);
CIRC_BUFF_STATUS_t circular_buffer_get_chunk(circular_buffer_t *p_cbuff, 
                                             void **pItem, 
                                             uint32_t chunk_size);

#endif  // __CIRCULAR_BUFFER_H
