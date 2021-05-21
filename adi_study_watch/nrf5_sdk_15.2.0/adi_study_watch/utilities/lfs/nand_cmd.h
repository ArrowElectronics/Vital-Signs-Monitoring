/**
***************************************************************************
* @addtogroup Tasks
* @{
* @file         nand_cmd.h
* @author       ADI
* @version      V1.0.0
* @date         17-April-2019
* @brief        Header file contains NAND Flash driver APIs.
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


#ifndef _NAND_CMD_H_
#define _NAND_CMD_H_

#include "stdint.h"

#include "nrf_log_ctrl.h"

typedef enum {
    NAND_SUCCESS = 0,
    NAND_ECC_UNCORRECTABLE_ERROR = 0x01,   
    NAND_ECC_CORRECTABLE_ERROR = 0x02,
    NAND_ECC_PROGRAM_ERROR = 0x04,
    NAND_ECC_ERASE_ERROR = 0x08,
    NAND_NRF_DRIVER_ERROR = 0x10,
    NAND_TIMEOUT_ERROR = 0x20,
    NAND_PARAM_ERROR = 0x40,
} eNand_Result;


/* NAND Flash command */
#define NAND_WRITEENABLE				(0x06) 
#define NAND_WRITEDISABLE				(0x04) 
#define NAND_GETREGS					(0x0F) 
#define NAND_SETREGS					(0x1F) 
#define NAND_PAGEREADTOCACHE            (0x13) 
#define NAND_READFROMCACHE				(0x03) 
#define NAND_READID						(0x9F)
#define NAND_PROGRAMLOAD				(0x02) 
#define NAND_PROGRAMEXECUTE				(0x10)
#define NAND_PROGRAMLOADRANDOMDATA      (0x84)
#define NAND_BLOCKERASE					(0xD8)  
#define NAND_RESET						(0xFF)

/* NAND Flash parameters */
#define NAND_PAGE_SIZE             ((uint16_t)0x1000) /*4096 bytes per page w/o Spare Area + 256byte*/
#define NAND_PAGE_COUNT             ((uint32_t)0x20000) /*4096 bytes per page w/o Spare Area + 256byte*/
#define NAND_BLOCK_SIZE            ((uint16_t)0x0040) /* 64 pages per block */
#define NAND_SPARE_AREA_SIZE       ((uint16_t)0x0100) /* last 256byte as spare area */
#define NAND_BLOCK_COUNT            ((uint16_t)0x0800) /* 2048 blocks */
#define NAND_PAGE_TOTAL_SIZE		(NAND_PAGE_SIZE + NAND_SPARE_AREA_SIZE)	 /*4096 bytes per page w/o Spare Area + 256byte*/

#define FLASH_MANUFACTURE_ID (0x2C)
#define FLASH_DEVICE_ID (0x35)

typedef struct _memory_properties {
  uint32_t page_size;
  uint32_t block_size;
  uint32_t mem_size;
  uint32_t pages_per_block;
  uint32_t num_of_blocks;
} _memory_properties;

/* NAND Flash Register address */
typedef enum {
  NAND_REG_PROTECTION       = 0xA0,
  NAND_REG_FEATURE1         = 0xB0,
  NAND_REG_STATUS1          = 0xC0,
  NAND_REG_FEATURE2         = 0xD0,
//  NAND_REG_STATUS2          = 0xF0
} eNand_Regs;


/**************************************** OTP helpers **************************************************************/
#define FEAT_OTP_ECCEN                 1<<4

/***************************************** Status helpers **************************************************************/
#define FEAT_STATUS_OIP                 1<<0
#define FEAT_STATUS_WEL                 1<<1
#define FEAT_STATUS_E_FAIL              1<<2
#define FEAT_STATUS_P_FAIL              1<<3       
#define FEAT_STATUS_ECC_STATUS          7<<4
#define FEAT_STATUS_ECC_STATUS_SHIFT    4

/*
* @brief  read the register of NAND flash.
* @param[in]:
*   @regs:the register address of NAND flash, refer enum NAND_REGS .
* @param[out]:
*   @outVal: return the value of the NAND flash register
* @return NAND_RESULT A 32-bit integer: 0 - success; < 0 - failure
*/
eNand_Result nand_flash_get_feature(eNand_Regs regs, uint8_t * out_val);

/*
* @brief  set the register of NAND flash.
* @param[in]:
*   @regs:the register address of NAND flash, refer enum NAND_REGS .
*   @value: set the value of the NAND flash register
* @return NAND_RESULT A 32-bit integer: 0 - success; < 0 - failure
*/
eNand_Result nand_flash_set_feature(eNand_Regs regs, uint8_t value);

/*
* @brief  write enable..
* @return NAND_RESULT A 32-bit integer: 0 - success; < 0 - failure
*/
eNand_Result nand_flash_write_enable(void);

/*
* @brief  Erase the block of NAND flash.
* @param[in]:
*   @_ulBlockNo:the block address of NAND flash, range:0~ NAND_BLOCK_COUNT .
* @return NAND_RESULT A 32-bit integer: 0 - success; < 0 - failure
*/
eNand_Result nand_flash_block_erase(uint32_t _ulBlockNo);

/*
* @brief  wait till the NAND flash ready
* @return NAND_RESULT A 32-bit integer: 0 - success; < 0 - failure
*/
eNand_Result nand_flash_wait_till_ready(void);

/*
* @brief  get the status of NAND flash.
* @param[out]:
*   @status return the status of NAND flash
* @return NAND_RESULT A 32-bit integer: 0 - success; < 0 - failure
*/
eNand_Result nand_flash_get_status(uint8_t * status);

/*
* @brief  reset the NAND flash.
* @return NAND_RESULT A 32-bit integer: 0 - success; < 0 - failure
*/
eNand_Result nand_flash_reset(void);

/*
* @brief  write disable.
* @return NAND_RESULT A 32-bit integer: 0 - success; < 0 - failure
*/
eNand_Result nand_flash_write_disable(void);

/*
* @brief  read data from the Flash to the cache of NAND flash.
* @param[in]:
*   @pageIndex:page address of flash.  range:0~NAND_PAGE_COUNT.
* @return NAND_RESULT A 32-bit integer: 0 - success; < 0 - failure
*/
eNand_Result nand_flash_page_read(uint32_t pageIndex);

/*
* @brief  read data from the cache of NAND flash.
* @param[in]:
*   @column:column address. The value must multiple of 4, range:0~NAND_PAGE_TOTAL_SIZE.
*   @size: data size. The value must multiple of 4, range:0~NAND_PAGE_TOTAL_SIZE.
* @param[out]:
*   @outArray: read data buffer.
* @return NAND_RESULT A 32-bit integer: 0 - success; < 0 - failure
*/
eNand_Result nand_flash_read_from_cache (uint32_t column,uint32_t size,uint8_t * outArray);//size must be a multiple of 4 bytes

/*
* @brief  write data from the cache of NAND flash to the Flash.
* @param[in]:
*   @pageIndex:page address of flash.  range:0~NAND_PAGE_COUNT.
* @return NAND_RESULT A 32-bit integer: 0 - success; < 0 - failure
*/
eNand_Result nand_flash_program_execute(uint32_t pageIndex);

/*
* @brief  write data to the cache of NAND flash.
* @param[in]:
*   @column:column address. The value must multiple of 4, range:0~NAND_PAGE_TOTAL_SIZE.
*   @size: data size. The value must multiple of 4, range:0~512.
*   @inArray: write data buffer.
* @return NAND_RESULT A 32-bit integer: 0 - success; < 0 - failure
*/
eNand_Result nand_flash_program_load(uint32_t column,uint32_t size,uint8_t * inArray);//size must be a multiple of 4 bytes

/*
* @brief  modify the data of the NAND flash cache.
* @param[in]:
*   @column:cache address.  range:0~NAND_PAGE_TOTAL_SIZE.
*   @size: data size. range:0~NAND_PAGE_TOTAL_SIZE.
*   @inArray: write data buffer.
* @return NAND_RESULT A 32-bit integer: 0 - success; < 0 - failure
*/
eNand_Result nand_flash_program_load_random(uint32_t column,uint32_t size,uint8_t * inArray);

/*
* @brief  read NAND flash manufacture ID and Device ID.
* @param[out]:
*   @mfacture_id: return manufacture ID.
*   @device_id: return device ID.
* @return NAND_RESULT A 32-bit integer: 0 - success; < 0 - failure
*/
eNand_Result nand_flash_read_id(uint8_t *mfacture_id, uint8_t *device_id);

eNand_Result nand_flash_init(void);

uint32_t nand_flash_qspi_init(void);
void nand_flash_qspi_uninit(void);



#endif

