/**
***************************************************************************
* @addtogroup Tasks
* @{
* @file         nand_functions.h
* @author       ADI
* @version      V1.0.0
* @date         17-April-2019
* @brief        Header file contains NAND Flash driver wrapper APIs.
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

#ifndef NAND_FUNCTIONS_H
#define NAND_FUNCTIONS_H

#include "stdint.h"
#include "nand_cmd.h"
#include "sdk_common.h"
#include "math.h"

#define MAX_QSPI_WRITE_LEN     512
#define MAX_QSPI_READ_LEN      4096
#define BLOCK_HEADER_SIZE       4
#define BAD_BLOCK_AREA_LOC      0x1000
#define PAGE_SIZE               4096

struct _page_write {
  uint32_t page_dest;
  uint8_t *data_buf;
  uint32_t data_size;
  uint32_t offset;
  uint8_t *spare_buf;
  uint32_t spare_size;
  uint32_t spare_offset;
};

typedef enum {
  NAND_FUNC_SUCCESS,   /*!< Successfully Completed */
  NAND_FUNC_ERROR,     /*!< General error */
} eNand_Func_Result;

typedef enum{
  NAND_FUNC_BLOCK_LOCK_ALL_UNLOCKED = 1,//[0 0 0 0 1]
  NAND_FUNC_BLOCK_LOCK_UPPER_1_1024_LOCKED = 2,//[0 0 0 1 0]
  NAND_FUNC_BLOCK_LOCK_UPPER_1_512_LOCKED = 4,//[0 0 1 0 0]
  NAND_FUNC_BLOCK_LOCK_UPPER_1_256_LOCKED = 6,//[0 0 0 1 1]
  NAND_FUNC_BLOCK_LOCK_UPPER_1_128_LOCKED = 8,//[0 1 0 0 0]
  NAND_FUNC_BLOCK_LOCK_UPPER_1_64_LOCKED = 10,//[0 0 0 1 0]
  NAND_FUNC_BLOCK_LOCK_UPPER_1_32_LOCKED = 12,//[0 0 0 1 0]
  NAND_FUNC_BLOCK_LOCK_UPPER_1_16_LOCKED = 14,//[0 0 0 1 0]
  NAND_FUNC_BLOCK_LOCK_UPPER_1_8_LOCKED = 16,//[0 0 0 1 0]
  NAND_FUNC_BLOCK_LOCK_UPPER_1_4_LOCKED = 18,//[0 0 0 1 0]
  NAND_FUNC_BLOCK_LOCK_UPPER_1_2_LOCKED = 20,//[0 0 0 1 0]
  NAND_FUNC_BLOCK_LOCK_LOWER_1_1024_LOCKED = 3,//[0 0 0 1 0]
  NAND_FUNC_BLOCK_LOCK_LOWER_1_512_LOCKED = 3,//[0 0 0 1 0]
  NAND_FUNC_BLOCK_LOCK_LOWER_1_256_LOCKED = 5,//[0 0 0 1 0]
  NAND_FUNC_BLOCK_LOCK_LOWER_1_128_LOCKED = 9,//[0 0 0 1 0]
  NAND_FUNC_BLOCK_LOCK_LOWER_1_64_LOCKED = 11,//[0 0 0 1 0]
  NAND_FUNC_BLOCK_LOCK_LOWER_1_32_LOCKED = 13,//[0 0 0 1 0]
  NAND_FUNC_BLOCK_LOCK_LOWER_1_16_LOCKED = 17,//[0 0 0 1 0]
  NAND_FUNC_BLOCK_LOCK_LOWER_1_4_LOCKED = 19,//[0 0 0 1 0]
  NAND_FUNC_BLOCK_LOCK_LOWER_1_2_LOCKED = 21,//[0 0 0 1 0]
  NAND_FUNC_BLOCK_ALL_LOCKED = 31,//[0 0 0 1 0]

}eNand_Func_Block_Lock;

eNand_Func_Result nand_func_page_and_spare_write(struct _page_write * write_data);
eNand_Func_Result nand_func_bad_block_mark_reset(_memory_properties *mem_prop,uint32_t block);
eNand_Func_Result nand_func_enable_ecc(bool enable);
eNand_Func_Result nand_func_check_ecc(uint8_t *status);
eNand_Func_Result nand_func_erase_memory(_memory_properties * mem_prop);
eNand_Func_Result nand_func_get_block_lock(eNand_Func_Block_Lock * lockConfig);
eNand_Func_Result nand_func_is_bad_block(_memory_properties *mem_prop,uint32_t block, bool *isBad);
eNand_Func_Result nand_func_mark_bad_block(_memory_properties *mem_prop,uint32_t block);
eNand_Func_Result nand_func_page_move(uint32_t page_init, uint32_t page_end);
eNand_Func_Result nand_func_read_ecc_zone(uint32_t pagePos,uint32_t offset,uint32_t size,uint8_t * out_array);
eNand_Func_Result nand_func_set_block_lock(eNand_Func_Block_Lock lockConfig);
eNand_Func_Result nand_func_write_ecc_zone(uint32_t pagePos,uint32_t offset,uint32_t size,uint8_t * in_array);
eNand_Func_Result spi_nand_read_page(uint8_t *_pBuffer,uint32_t _ulPageNo,uint16_t _usAddrInPage,uint16_t _usByteCount);
eNand_Func_Result nand_func_erase(_memory_properties *mem_prop, uint32_t block_init, uint32_t number_of_blocks);
eNand_Func_Result nand_func_write(_memory_properties *mem_prop,uint32_t mem_pos, uint32_t size, uint8_t *in_array);
eNand_Func_Result nand_func_read(_memory_properties *mem_prop,uint32_t mem_pos, uint32_t size, uint8_t *out_array);
#endif
