/**
****************************************************************************
* @file     memory_management.h
* @author   ADI
* @version  V0.1
* @date     04-Feb-2020
* @brief    This header file used to implement memory management functions
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

#ifndef _MEMORY_MANAGEMENT_H_
#define _MEMORY_MANAGEMENT_H_

//#define MEM_BLK_TYPE1_CNT	( 32u )
#define MEM_BLK_TYPE2_CNT	( 24u )
//#define MEM_BLK_TYPE3_CNT	( 96u )
#define MEM_BLK_TYPE4_CNT	( 70u )
#define MEM_BLK_TYPE5_CNT	( 8u )//40

/* Size should be in ascending order */
//#define SZ_MEM_BLK_TYPE1	( 16u )
#define SZ_MEM_BLK_TYPE2	( 32u )
//#define SZ_MEM_BLK_TYPE3	( 64u )
#define SZ_MEM_BLK_TYPE4	( 80u )
#define SZ_MEM_BLK_TYPE5	( 528 )

typedef enum {
  MEM_MANAGER_ERR_SUCCESS 	= 0x00,
  MEM_MANAGER_ERR_MEM_INIT	= 0x01,
  MEM_MANAGER_ERR_MEM_GET       = 0x02,
  MEM_MANAGER_ERR_MEM_FREE	= 0x03,
  MEM_MANAGER_ERR_MEM_LIMIT	= 0x04,
  MEM_MANAGER_ERR_MEM_INVALID	= 0xFF,
} MEM_MANAGER_ERR_STATUS_t;

MEM_MANAGER_ERR_STATUS_t memory_block_get(void **pp_memory, uint16_t size);
MEM_MANAGER_ERR_STATUS_t memory_block_free(void *p_memory, uint16_t size);
MEM_MANAGER_ERR_STATUS_t memory_manager_init( void );

#endif // _MEMORY_MANAGEMENT_H_
