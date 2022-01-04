/**
****************************************************************************
* @file     memory_management.c
* @author   ADI
* @version  V0.1
* @date     04-Feb-2020
* @brief    This source file used to implement memory management functions
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

#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <adi_osal.h>
#include "memory_management.h"
#ifdef ENABLE_TEMP_DEBUG_STREAM
#include "m2m2_core.h"
#endif
#include <mem_pool.h>

mem_pool_handler_t h_mem_blk_type2;
mem_pool_handler_t h_mem_blk_type4;
mem_pool_handler_t h_mem_blk_type5;

//uint8_t memory_block_type1[MEM_BLK_TYPE1_CNT * SZ_MEM_BLK_TYPE1];
uint8_t memory_block_type2[MEM_BLK_TYPE2_CNT * SZ_MEM_BLK_TYPE2];
//uint8_t memory_block_type3[MEM_BLK_TYPE3_CNT * SZ_MEM_BLK_TYPE3];
uint8_t memory_block_type4[MEM_BLK_TYPE4_CNT * SZ_MEM_BLK_TYPE4];
uint8_t memory_block_type5[MEM_BLK_TYPE5_CNT * SZ_MEM_BLK_TYPE5];

#ifdef PO_MEM_UTIL_STATS
uint16_t min_num_free_blks_type2 = MEM_BLK_TYPE2_CNT;
uint16_t min_num_free_blks_type4 = MEM_BLK_TYPE4_CNT;
uint16_t min_num_free_blks_type5 = MEM_BLK_TYPE5_CNT;
uint32_t block_2_allocated=0;
uint32_t block_4_allocated=0;
uint32_t block_5_allocated=0;
uint32_t block_2_freed=0;
uint32_t block_4_freed=0;
uint32_t block_5_freed=0;
#endif

/*!
  *@brief       get memory block from handler determined by message size.
  *@param pp_memory   The address of the pointer to assign memory block with.
  *@param size        The size of memory requested.
  *@return      Status
 */
 #ifdef ADPD_SEM_CORRUPTION_DEBUG
uint8_t *p_slot_bkup = NULL;
uint8_t *p_slot_data_sz_0 = NULL;
uint32_t gHighest_slot_num = 0; 
uint8_t *last_mem_allocated=NULL;
uint8_t *last_mem_allocated_type_2=NULL;
uint32_t block_2_allocated=0;
uint32_t block_4_allocated=0;
uint32_t gnSamples_in_fifo =0;
uint32_t gnBytes_in_fifo =0;
#endif
#ifdef ENABLE_TEMP_DEBUG_STREAM
extern uint32_t g_temp_debugInfo[];
extern uint32_t disp_app_packet_count;
extern uint32_t batt_app_packet_count;
extern uint32_t temp_app_packet_count;
extern uint32_t usb_pend_start_time;
extern uint32_t usb_delay_time;
#endif
MEM_MANAGER_ERR_STATUS_t memory_block_get(void **pp_memory, uint16_t size) {
  if (size <= SZ_MEM_BLK_TYPE2) {
    if (mem_pool_get(&h_mem_blk_type2, pp_memory) == MEM_POOL_SUCCESS) {
#ifdef ENABLE_TEMP_DEBUG_STREAM
      if(size == 20){
        g_temp_debugInfo[0]  = usb_pend_start_time;
        g_temp_debugInfo[1]  = usb_delay_time;
      }else if(size == 10){
        g_temp_debugInfo[2]  = (h_mem_blk_type2.nBlkFree << 16 | M2M2_ADDR_DISPLAY);
        g_temp_debugInfo[3]  = disp_app_packet_count;
      }else if(size == 18){
        g_temp_debugInfo[4]  = (h_mem_blk_type2.nBlkFree << 16 | M2M2_ADDR_SYS_BATT_STREAM);
        g_temp_debugInfo[5]  = batt_app_packet_count;
      }
#endif
#ifdef ADPD_SEM_CORRUPTION_DEBUG
      last_mem_allocated_type_2 = (uint8_t *)*pp_memory;
      block_2_allocated += 1;
#endif
#ifdef PO_MEM_UTIL_STATS
      block_2_allocated += 1;
      if (h_mem_blk_type2.nBlkFree < min_num_free_blks_type2){
      min_num_free_blks_type2 = h_mem_blk_type2.nBlkFree;
      }
#endif
      return MEM_MANAGER_ERR_SUCCESS;
    }
#ifdef ENABLE_TEMP_DEBUG_STREAM    
    else{
      if(size == 20){
        g_temp_debugInfo[0]  = usb_pend_start_time;
        g_temp_debugInfo[1]  = usb_delay_time;
      }else if(size == 10){
        g_temp_debugInfo[2]  = (0xFF << 16 | M2M2_ADDR_DISPLAY);
        g_temp_debugInfo[3]  = disp_app_packet_count;
      }else if(size == 18){
        g_temp_debugInfo[4]  = (0xFF << 16 | M2M2_ADDR_SYS_BATT_STREAM);
        g_temp_debugInfo[5]  = batt_app_packet_count;
      }
    }   
#endif
  } else if (size <= SZ_MEM_BLK_TYPE4) {
    if (mem_pool_get(&h_mem_blk_type4, pp_memory) == MEM_POOL_SUCCESS) {
#ifdef ADPD_SEM_CORRUPTION_DEBUG
      last_mem_allocated = (uint8_t *)*pp_memory;
      block_4_allocated += 1;
#endif
#ifdef PO_MEM_UTIL_STATS
      block_4_allocated += 1;
      if (h_mem_blk_type4.nBlkFree < min_num_free_blks_type4){
      min_num_free_blks_type4 = h_mem_blk_type4.nBlkFree;
      }
#endif
      return MEM_MANAGER_ERR_SUCCESS;
    }
  } else if (size <= SZ_MEM_BLK_TYPE5) {
    if (mem_pool_get(&h_mem_blk_type5, pp_memory) == MEM_POOL_SUCCESS) {
#ifdef PO_MEM_UTIL_STATS
      block_5_allocated += 1;
      if (h_mem_blk_type5.nBlkFree < min_num_free_blks_type5){
        min_num_free_blks_type5 = h_mem_blk_type5.nBlkFree;
      }
#endif
      return MEM_MANAGER_ERR_SUCCESS;
    }
  } else {
    *pp_memory = (void*)malloc(size);
    if (*pp_memory != NULL) {
      return MEM_MANAGER_ERR_SUCCESS;
    }
  }
  return MEM_MANAGER_ERR_MEM_GET;
}

/*!
  *@brief       free memory block handler determined by message size.
  *@param p_memory   The pointer of memory block to be freed.
  *@param size       The length of message to determine which memory partition blk belongs to.
  *@return      Status
 */
 #ifdef ADPD_SEM_CORRUPTION_DEBUG
 uint8_t *last_mem_freed=NULL;
  uint8_t *last_mem_freed_type_2=NULL;
uint32_t block_2_freed=0;
uint32_t block_4_freed=0;
#endif
MEM_MANAGER_ERR_STATUS_t memory_block_free(void *p_memory, uint16_t size) {
  if (size <= SZ_MEM_BLK_TYPE2) {
    if (mem_pool_free(&h_mem_blk_type2, p_memory) == MEM_POOL_SUCCESS) {
#ifdef ENABLE_TEMP_DEBUG_STREAM
      if(size == 20){
        g_temp_debugInfo[6]  = usb_pend_start_time;
        g_temp_debugInfo[7]  = usb_delay_time;
      }else if(size == 10){
        g_temp_debugInfo[8]  = (h_mem_blk_type2.nBlkFree << 16 | M2M2_ADDR_DISPLAY);
        g_temp_debugInfo[9]  = disp_app_packet_count;
      }else if(size == 18){
        g_temp_debugInfo[10]  = (h_mem_blk_type2.nBlkFree << 16 | M2M2_ADDR_SYS_BATT_STREAM);
        g_temp_debugInfo[11]  = batt_app_packet_count;
      }
#endif
#ifdef ADPD_SEM_CORRUPTION_DEBUG
      block_2_freed += 1;
      last_mem_freed_type_2 = (uint8_t *)p_memory;
#endif
#ifdef PO_MEM_UTIL_STATS
      block_2_freed += 1;
#endif
      return MEM_MANAGER_ERR_SUCCESS;
    }
#ifdef ENABLE_TEMP_DEBUG_STREAM    
    else{
      if(size == 20){
        g_temp_debugInfo[6]  = usb_pend_start_time;
        g_temp_debugInfo[7]  = usb_delay_time;
      }else if(size == 10){
        g_temp_debugInfo[8]  = ( 0xFF << 16 | M2M2_ADDR_DISPLAY);
        g_temp_debugInfo[9]  = disp_app_packet_count;
      }else if(size == 18){
        g_temp_debugInfo[10]  = ( 0xFF << 16 | M2M2_ADDR_SYS_BATT_STREAM);
        g_temp_debugInfo[11]  = batt_app_packet_count;
      } 
    }  
#endif
  } else if (size <= SZ_MEM_BLK_TYPE4) {
    if (mem_pool_free(&h_mem_blk_type4, p_memory) == MEM_POOL_SUCCESS) {
#ifdef ADPD_SEM_CORRUPTION_DEBUG
      block_4_freed += 1;
      last_mem_freed = (uint8_t *)p_memory;
#endif
#ifdef PO_MEM_UTIL_STATS
      block_4_freed += 1;
#endif
      return MEM_MANAGER_ERR_SUCCESS;
    }
  } else if (size <= SZ_MEM_BLK_TYPE5) {
    if (mem_pool_free(&h_mem_blk_type5, p_memory) == MEM_POOL_SUCCESS) {
#ifdef PO_MEM_UTIL_STATS
      block_5_freed += 1;
#endif
      return MEM_MANAGER_ERR_SUCCESS;
    }
  } else {
    free(p_memory);
  }
  return MEM_MANAGER_ERR_MEM_FREE;
}

/*!
  *@brief       Initialization of memory partitions.
  *@return      Status
 */
MEM_MANAGER_ERR_STATUS_t memory_manager_init( void ) {
  if (mem_pool_init(&h_mem_blk_type2, (void *)memory_block_type2, MEM_BLK_TYPE2_CNT, SZ_MEM_BLK_TYPE2) != MEM_POOL_SUCCESS) {
    return MEM_MANAGER_ERR_MEM_INIT;
  }
  if (mem_pool_init(&h_mem_blk_type4, (void *)memory_block_type4, MEM_BLK_TYPE4_CNT, SZ_MEM_BLK_TYPE4) != MEM_POOL_SUCCESS) {
    return MEM_MANAGER_ERR_MEM_INIT;
  }
  if (mem_pool_init(&h_mem_blk_type5, (void *)memory_block_type5, MEM_BLK_TYPE5_CNT, SZ_MEM_BLK_TYPE5) != MEM_POOL_SUCCESS) {
    return MEM_MANAGER_ERR_MEM_INIT;
  }
  return MEM_MANAGER_ERR_SUCCESS;
}

