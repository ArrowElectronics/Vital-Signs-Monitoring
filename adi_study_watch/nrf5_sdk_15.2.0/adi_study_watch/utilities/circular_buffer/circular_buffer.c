/**
****************************************************************************
* @file     circular_buffer.c
* @author   ADI
* @version  V0.1
* @date     04-Dec-2019
* @brief    This source file file is used for circular buffer implementation.
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

#include <circular_buffer.h>

CIRC_BUFF_STATUS_t circular_buffer_init(circular_buffer_t *p_cbuff,
                                        uint8_t *p_mem,
                                        uint32_t mem_sz,
                                        uint32_t element_sz) {
  if (p_cbuff == NULL || p_mem == NULL) {
    return CIRC_BUFF_STATUS_NULL_PTR;
  }
  if (element_sz == 0) {
    return CIRC_BUFF_STATUS_ERROR;
  }
  if (mem_sz < element_sz) {
    // There isn't enough memory to hold a single element
    return CIRC_BUFF_STATUS_OVERFLOW;
  }
  p_cbuff->buffer = p_mem;
  p_cbuff->buffer_sz = mem_sz;
  p_cbuff->buffer_end = p_cbuff->buffer + p_cbuff->buffer_sz;
  p_cbuff->element_sz = element_sz;
  // The capacity will be the floor of (buffer size / element size)
  p_cbuff->capacity = p_cbuff->buffer_sz / p_cbuff->element_sz;
  p_cbuff->num_elements = 0;
  p_cbuff->head = &p_cbuff->buffer[0];
  p_cbuff->tail = p_cbuff->head;
  return CIRC_BUFF_STATUS_OK;
}

CIRC_BUFF_STATUS_t circular_buffer_reset(circular_buffer_t *p_cbuff, uint32_t element_sz) {
  if (p_cbuff == NULL) {
    return CIRC_BUFF_STATUS_NULL_PTR;
  }
  if (element_sz == 0) {
    p_cbuff->num_elements = 0;
    p_cbuff->head = &p_cbuff->buffer[0];
    p_cbuff->tail = p_cbuff->head;
    return CIRC_BUFF_STATUS_ERROR;
  }
  if (p_cbuff->buffer_sz < element_sz) {
    return CIRC_BUFF_STATUS_OVERFLOW;
  }
  p_cbuff->element_sz = element_sz;
  p_cbuff->capacity = p_cbuff->buffer_sz / p_cbuff->element_sz;
  p_cbuff->num_elements = 0;
  p_cbuff->head = &p_cbuff->buffer[0];
  p_cbuff->tail = p_cbuff->head;
  return CIRC_BUFF_STATUS_OK;
}

CIRC_BUFF_STATUS_t circular_buffer_put(circular_buffer_t *p_cbuff, void *item) {
  if (p_cbuff == NULL || item == NULL) {
    return CIRC_BUFF_STATUS_NULL_PTR;
  }
  if (p_cbuff->num_elements >= p_cbuff->capacity) {
    return CIRC_BUFF_STATUS_FULL;
  }
  memcpy(p_cbuff->head, item, p_cbuff->element_sz);
  p_cbuff->head = (uint8_t*)p_cbuff->head + p_cbuff->element_sz;
  // Make sure there's enough room between the head and the buffer end to place a new item
  if (((uint8_t*)p_cbuff->head + p_cbuff->element_sz) > p_cbuff->buffer_end) {
    p_cbuff->head = p_cbuff->buffer;
  }
  p_cbuff->num_elements++;
  return CIRC_BUFF_STATUS_OK;
}

CIRC_BUFF_STATUS_t circular_buffer_get(circular_buffer_t *p_cbuff, void *item) {
  if (p_cbuff == NULL || item == NULL) {
    return CIRC_BUFF_STATUS_NULL_PTR;
  }
  if (p_cbuff->num_elements == 0) {
    return CIRC_BUFF_STATUS_EMPTY;
  }
  memcpy(item, p_cbuff->tail, p_cbuff->element_sz);
  p_cbuff->tail = (uint8_t*)p_cbuff->tail + p_cbuff->element_sz;
  // Make sure we wrap around properly
  if (((uint8_t*)p_cbuff->tail + p_cbuff->element_sz) > p_cbuff->buffer_end) {
    p_cbuff->tail = p_cbuff->buffer;
  }
  p_cbuff->num_elements--;
  return CIRC_BUFF_STATUS_OK;
}
