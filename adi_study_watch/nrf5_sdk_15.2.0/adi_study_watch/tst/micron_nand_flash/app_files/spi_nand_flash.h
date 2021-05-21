#ifndef _SPI_NAND_FLASH_H_
#define _SPI_NAND_FLASH_H_

#include "stdint.h"

typedef uint32_t NAND_RESULT;
#define FEAT_OTP_ECCEN                 1<<4

#define FLASH_MANUFACTURE_ID (0x2C)
#define FLASH_DEVICE_ID (0x35)
/* NAND Flash command */
#define NAND_WRITEENABLE				(0x06) 
#define NAND_WRITEDISABLE				(0x04) 
#define NAND_GETREGS					(0x0F) 
#define NAND_SETREGS					(0x1F) 
#define NAND_PAGEREADTOCATCH                            (0x13) 
#define NAND_READFROMCATCH				(0x03) 
#define NAND_READID					(0x9F)
#define NAND_PROGRAMLOAD				(0x02) 
#define NAND_PROGRAMEXECUTE				(0x10)
#define NAND_PROGRAMLOADRANDOMDATA                      (0x84)
#define NAND_BLOCKERASE					(0xD8)  
#define NAND_RESET					(0xFF)

/* NAND Flash parameters */
#define NAND_PAGE_SIZE             ((uint16_t)0x1000) /*4096 bytes per page w/o Spare Area + 256byte*/
#define NAND_PAGE_COUNT             ((uint32_t)0x20000) /*4096 bytes per page w/o Spare Area + 256byte*/
#define NAND_BLOCK_SIZE            ((uint16_t)0x0040) /* 64 pages per block */
#define NAND_SPARE_AREA_SIZE       ((uint16_t)0x0100) /* last 256byte as spare area */
#define NAND_BLOCK_COUNT            ((uint16_t)0x0800) /* 2048 blocks */
#define NAND_PAGE_TOTAL_SIZE		(NAND_PAGE_SIZE + NAND_SPARE_AREA_SIZE)	 /*4096 bytes per page w/o Spare Area + 256byte*/
 
/* NAND Flash Register address */
typedef enum {
  NAND_REG_PROTECTION       = 0xA0,
  NAND_REG_FEATURE1         = 0xB0,
  NAND_REG_STATUS1          = 0xC0,
  NAND_REG_FEATURE2         = 0xD0,
//  NAND_REG_STATUS2          = 0xF0
} NAND_REGS;

/*
* @brief  read the register of NAND flash.
* @param[in]:
*   @regs:the register address of NAND flash, refer enum NAND_REGS .
* @param[out]:
*   @outVal: return the value of the NAND flash register
* @return NAND_RESULT A 32-bit integer: 0 - success; < 0 - failure
*/
NAND_RESULT NandFlash_GetFeature(NAND_REGS regs, uint8_t * outVal);

/*
* @brief  set the register of NAND flash.
* @param[in]:
*   @regs:the register address of NAND flash, refer enum NAND_REGS .
*   @value: set the value of the NAND flash register
* @return NAND_RESULT A 32-bit integer: 0 - success; < 0 - failure
*/
NAND_RESULT NandFlash_SetFeature(NAND_REGS regs, uint8_t value);

/*
* @brief  write enable..
* @return NAND_RESULT A 32-bit integer: 0 - success; < 0 - failure
*/
NAND_RESULT NandFlash_WRITE_ENABLE(void);

/*
* @brief  Erase the block of NAND flash.
* @param[in]:
*   @_ulBlockNo:the block address of NAND flash, range:0~ NAND_BLOCK_COUNT .
* @return NAND_RESULT A 32-bit integer: 0 - success; < 0 - failure
*/
NAND_RESULT NandFlash_BlockErase(uint32_t _ulBlockNo);

/*
* @brief  wait till the NAND flash ready
* @return NAND_RESULT A 32-bit integer: 0 - success; < 0 - failure
*/
NAND_RESULT NandFlash_WaitTilLReady(void);

/*
* @brief  get the status of NAND flash.
* @param[out]:
*   @status return the status of NAND flash
* @return NAND_RESULT A 32-bit integer: 0 - success; < 0 - failure
*/
NAND_RESULT NandFlash_GetStatus(uint8_t * status);

/*
* @brief  reset the NAND flash.
* @return NAND_RESULT A 32-bit integer: 0 - success; < 0 - failure
*/
NAND_RESULT NandFlash_Reset(void);

/*
* @brief  write disable.
* @return NAND_RESULT A 32-bit integer: 0 - success; < 0 - failure
*/
NAND_RESULT NandFlash_WRITE_DISABLE(void);

/*
* @brief  read data from the Flash to the cache of NAND flash.
* @param[in]:
*   @pageIndex:page address of flash.  range:0~NAND_PAGE_COUNT.
* @return NAND_RESULT A 32-bit integer: 0 - success; < 0 - failure
*/
NAND_RESULT NandFlash_PageRead(uint32_t pageIndex);

/*
* @brief  read data from the cache of NAND flash.
* @param[in]:
*   @column:column address. The value must multiple of 4, range:0~NAND_PAGE_TOTAL_SIZE.
*   @size: data size. The value must multiple of 4, range:0~NAND_PAGE_TOTAL_SIZE.
* @param[out]:
*   @outArray: read data buffer.
* @return NAND_RESULT A 32-bit integer: 0 - success; < 0 - failure
*/
NAND_RESULT NandFlash_ReadFromCache (uint32_t column,  uint32_t size, uint8_t * outArray);//size must be a multiple of 4 bytes

/*
* @brief  write data from the cache of NAND flash to the Flash.
* @param[in]:
*   @pageIndex:page address of flash.  range:0~NAND_PAGE_COUNT.
* @return NAND_RESULT A 32-bit integer: 0 - success; < 0 - failure
*/
NAND_RESULT NandFlash_ProgramExecute(uint32_t pageIndex);

/*
* @brief  write data to the cache of NAND flash.
* @param[in]:
*   @column:column address. The value must multiple of 4, range:0~NAND_PAGE_TOTAL_SIZE.
*   @size: data size. The value must multiple of 4, range:0~512.
*   @inArray: write data buffer.
* @return NAND_RESULT A 32-bit integer: 0 - success; < 0 - failure
*/
NAND_RESULT NandFlash_ProgramLoad(uint32_t column,  uint32_t size, uint8_t * inArray);//size must be a multiple of 4 bytes

/*
* @brief  modify the data of the NAND flash cache.
* @param[in]:
*   @column:cache address.  range:0~NAND_PAGE_TOTAL_SIZE.
*   @size: data size. range:0~NAND_PAGE_TOTAL_SIZE.
*   @inArray: write data buffer.
* @return NAND_RESULT A 32-bit integer: 0 - success; < 0 - failure
*/
NAND_RESULT NandFlash_ProgramLoadRandom(uint32_t column,uint32_t size,uint8_t * inArray);

/*
* @brief  read NAND flash manufacture ID and Device ID.
* @param[out]:
*   @mfacture_id: return manufacture ID.
*   @device_id: return device ID.
* @return NAND_RESULT A 32-bit integer: 0 - success; < 0 - failure
*/
NAND_RESULT NandFlash_ReadID(uint8_t *mfacture_id,uint8_t *device_id);

void flash_init_test(void);

#endif

