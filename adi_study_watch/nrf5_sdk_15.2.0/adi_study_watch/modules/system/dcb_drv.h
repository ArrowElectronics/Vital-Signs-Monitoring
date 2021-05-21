#ifndef FLASH_DRV_H__
#define FLASH_DRV_H__

#include <nrfx.h>
#include "nrf_soc.h"
#include "nrf_fstorage.h"
#include "nrf_fstorage_nvmc.h"
#include "nrf_log.h"
#include "nrf_log_ctrl.h"
#include "nrf_log_default_backends.h"

#define DCB_SUCCESS                0
#define DCB_ERROR                  -1
#define DCB_LENGTH_ERROR           -1
#define DCB_END_ADDR               0xF6FFF
#define DCB_SIZE                   0x03FFF
#define DCB_START_ADDR             (DCB_END_ADDR-DCB_SIZE)
#define INDEX_START_ADDR           (DCB_START_ADDR+INDEX_SIZE)
#define BACKUP_START_ADDR           0xF5000
#define PAGE_CNT                    2 /*8KB of data will be erased*/
#define DCB_MEMORY_SIZE           8192  /*DCB memory size in bytes*/
#define SIZE_OF_RAM                 2048

#define ANALOG_CONFIG_BLOCKS          (16)
#define RAM_WINDOW                     64
#define CHECKSUM_SIZE                   4

typedef enum eDcbConfigIndex {
  CHECKSUM_IDX,INDEX_IDX = 0,
  ANALOG_GENERAL_BLOCK_IDX,
  ANALOG_AD5940_BLOCK_IDX,
  ANALOG_ADPD4000_BLOCK_IDX,
  ANALOG_ADXL362_BLOCK_IDX,
  ANALOG_PPG_BLOCK_IDX,
  ANALOG_ECG_BLOCK_IDX,
  ANALOG_EDA_BLOCK_IDX,
  ANALOG_AD7146_BLOCK_IDX,
  ANALOG_PEDOMETER_BLOCK_IDX,
  ANALOG_TEMPERATURE_BLOCK_IDX,
  ANALOG_WRIST_DETECT_BLOCK_IDX,
  ANALOG_UI_CONFIG_BLOCK_IDX,
  ANALOG_USER0_BLOCK_IDX,
  ANALOG_USER1_BLOCK_IDX,
  ANALOG_USER2_BLOCK_IDX,
  ANALOG_USER3_BLOCK_IDX
} eDcbConfigIndex_t;
               
/*Device Configuration block*/
typedef struct {
     uint32_t checksum;
     uint16_t index[ANALOG_CONFIG_BLOCKS];
} analog_device_configuration_block_t;

void wait_for_flash_ready(nrf_fstorage_t const * p_fstorage);
void dcb_flash_init();
void power_manage(void);
static void fstorage_evt_handler(nrf_fstorage_evt_t * p_evt);
static void fstorage_evt_handler(nrf_fstorage_evt_t * p_evt);
void wait_for_flash_ready(nrf_fstorage_t const * p_fstorage);

void WriteConfig(uint8_t const *nData,uint32_t nSize);
void WriteConfigBackUp(uint32_t nAddr,void const *nData);
void EraseFlash(uint32_t nAddr,uint8_t nPageCount);
void ReadFlash(uint8_t * pBuf,uint32_t nLen);
void DCBErase();
void DCBEraseBackUp();
void DCBBackUp();
void DCBWrite(uint8_t *pData,uint32_t nSize);
int16_t DCBRead(eDcbConfigIndex_t nIndex,uint8_t *pConfig, uint32_t nSize);

#endif