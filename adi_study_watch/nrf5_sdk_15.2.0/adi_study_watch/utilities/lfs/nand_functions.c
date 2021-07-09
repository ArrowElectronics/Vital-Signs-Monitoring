/**
***************************************************************************
* @addtogroup Tasks
* @{
* @file         nand_functions.c
* @author       ADI
* @version      V1.0.0
* @date         17-April-2019
* @brief        Source file contains NAND Flash driver wrapper APIs.
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
#ifdef USE_FS
#include "nand_functions.h"
#include "nrf_drv_qspi.h"

#include "nrf_log_ctrl.h"
#include "us_tick.h"
#include "light_fs.h"

#define NRF_LOG_MODULE_NAME NAND_FUNCTIONS

#if NAND_FUNCTIONS_CONFIG_LOG_ENABLED
#ifdef NRF_LOG_DEFAULT_LEVEL
#undef NRF_LOG_DEFAULT_LEVEL
#define NRF_LOG_DEFAULT_LEVEL NAND_FUNCTIONS_CONFIG_LOG_LEVEL
#endif
#define NRF_LOG_INFO_COLOR  NAND_FUNCTIONS_CONFIG_INFO_COLOR
#define NRF_LOG_DEBUG_COLOR  NAND_FUNCTIONS_CONFIG_DEBUG_COLOR
#else /*NAND_FUNCTIONS_CONFIG_LOG_ENABLED */
#define NRF_LOG_LEVEL       0
#endif /*NAND_FUNCTIONS_CONFIG_LOG_ENABLED */

#include "nrf_log.h"


/* Enable nrf logger */
NRF_LOG_MODULE_REGISTER();

/************************************************* Private variables ***********************************************************/
#ifdef PROFILE_TIME_ENABLED
extern const nrfx_timer_t TIMER_INC_MICRO_SEC;
uint32_t get_micro_sec(void);

uint32_t nTick1,nTick2,nTick3,nTick4,load_time_diff_main,load_time_diff_spare,nTickSpare1,nTickSpare2;
uint32_t total_program_load_time,total_program_exe_time,total_wait_till_ready_time;
#endif

#ifdef FORMAT_DEBUG_INFO_CMD
extern fs_format_debug_info tmp_fs_format_debug_info;
#endif

/*!
  ****************************************************************************
 *@brief Read page using qspi from flash to soft buffer
  *
  *@param[in]      _pBuffer: Buffer where data has to be read.
  *@param[in]      _ulPageNo: Page no to be read
  *@param[in]      _usAddrInPage: Offset in the page from where data has to be read
  *@param[in]      _usByteCount:  Number of bytes to be read
  *@return Result of the function - Error or success
******************************************************************************/
eNand_Func_Result spi_nand_read_page(uint8_t *_pBuffer,uint32_t _ulPageNo,\
                                    uint16_t _usAddrInPage,uint16_t _usByteCount) {
    eNand_Result ret = NAND_SUCCESS;
    /* perform page read */
    ret = nand_flash_page_read(_ulPageNo);
    if(ret == NAND_SUCCESS) {
        /* if page read is success, perform cache read onto soft buffer*/
        ret = nand_flash_read_from_cache( _usAddrInPage, _usByteCount,_pBuffer);
    }
    /* if failure in page/cache read, return error, else return
      success*/
    if(ret != NAND_SUCCESS) {
        return NAND_FUNC_ERROR;
    }
    else  {
        return NAND_FUNC_SUCCESS;
    }
}

/*!
  ****************************************************************************
  *@brief : Nand function to write page and spare area together
  *
   @param[in]      writeData Struct with the information of the operation:
  *                         pageDest    Destination page within the memory
  *                         dataBuf     Data buffer to be written to the page
  *                         dataSize    Number of bytes to be written to the page
  *                         offset      Offset of the data within the page
  *                         spareBuf    Buffer to be written to the spare zone
  *                         spareSize   Number of bytes to be written to the spare zone
  *                         spareOffset Offset within the spare zone.
  *@return        Result of the function - Error or success
******************************************************************************/
eNand_Func_Result nand_func_page_and_spare_write(struct _page_write * write_data) {
    eNand_Result ret = NAND_SUCCESS;
    uint32_t write_size = 0;
    uint32_t data_size = 0;

    /* write data */
    uint32_t size_written = 0;
    data_size = write_data->data_size;
#ifdef PROFILE_TIME_ENABLED
    total_wait_till_ready_time= total_program_load_time = total_program_exe_time =\
    load_time_diff_main = load_time_diff_spare = 0;
#endif
    /* if data size to write is greater than 0 */
    /* program load calls an api qspi_write which has limitation of 512 bytes transfer */
    while(data_size > 0)  {
      write_size = data_size > MAX_QSPI_WRITE_LEN ? MAX_QSPI_WRITE_LEN:data_size;
#ifdef PROFILE_TIME_ENABLED
      nTick1 = get_micro_sec();
#endif
      /* load data corresponding to data area to cache */
      ret = nand_flash_program_load((write_data->offset)+size_written,write_size,\
                                    (write_data->data_buf)+size_written);

      /* return if program load is an error */
      if(NAND_SUCCESS != ret) {
          NRF_LOG_INFO("ProgramLoad error,ret=%d",ret);
          return NAND_FUNC_ERROR;
      }
#ifdef PROFILE_TIME_ENABLED
      nTick2 = get_micro_sec();
      total_program_load_time += (nTick2 - nTick1);
#endif
      /* execute does writing of data to flash from cache */
      ret = nand_flash_program_execute(write_data->page_dest);

      /* return if program execute is an error */
      if(NAND_SUCCESS != ret) {
          NRF_LOG_INFO("ProgramExe error,ret=%d",ret);
          return NAND_FUNC_ERROR;
      }
#ifdef PROFILE_TIME_ENABLED
      nTick3 = get_micro_sec();
      total_program_exe_time += (nTick3 - nTick2);
#endif
      /* wait for writing to get over */
      ret = nand_flash_wait_till_ready();
      if(ret != NAND_SUCCESS) {
        NRF_LOG_INFO("Error in wait till ready");
      }

      /* data size is decremented by write size */
      data_size -= write_size;
      /* size written is incremented by write size */
      size_written += write_size;
#ifdef PROFILE_TIME_ENABLED
      nTick4 = get_micro_sec();
      total_wait_till_ready_time += (nTick4 - nTick3);
#endif
    }

#ifdef PROFILE_TIME_ENABLED
      NRF_LOG_INFO("Total program load time =  %d, Total program exe time = %d,wait till read time = %d",
          total_program_load_time,total_program_exe_time,
          total_wait_till_ready_time);
      load_time_diff_main = total_wait_till_ready_time+total_program_load_time+total_program_exe_time;

      /*reset timer for spare area*/
     nTickSpare1 = get_micro_sec();

#endif
    /* load data corresponding to spare area to cache */
    ret = nand_flash_program_load(write_data->spare_offset,\
                                  write_data->spare_size,write_data->spare_buf);
    /* return if program load is an error */
    if(NAND_SUCCESS != ret) {
        NRF_LOG_INFO("ProgramLoadrandom error,ret=%d",ret);
        return NAND_FUNC_ERROR;
    }

    /* execute does writing of data to flash from cache */
    ret = nand_flash_program_execute(write_data->page_dest);
    /* return if program execute is an error */
    if(NAND_SUCCESS != ret) {
        NRF_LOG_INFO("Program execute error");
        return NAND_FUNC_ERROR;
    }

   /* wait for writing to get over */
   ret = nand_flash_wait_till_ready();
   if(ret != NAND_SUCCESS)  {
        NRF_LOG_INFO("Error in wait till ready");
    }
#ifdef PROFILE_TIME_ENABLED
     nTickSpare2 = get_micro_sec();

    load_time_diff_spare = nTickSpare2 -  nTickSpare1;
    NRF_LOG_INFO("*** Time taken to write main area = %d, spare area = %d",load_time_diff_main,load_time_diff_spare);
#endif
    return NAND_FUNC_SUCCESS;
}

/*!
  ****************************************************************************
  *@brief : Enable / Disable the internal memory ECC
  *
  *@param[in]      enable true = enable, false = disable
  *@return Result of the function - Error or success
******************************************************************************/
eNand_Func_Result nand_func_enable_ecc(bool enable) {
  eNand_Result result = NAND_SUCCESS;
  uint8_t OTPValue;

  /* get feature from status register */
  result = nand_flash_get_feature(NAND_REG_FEATURE1, &OTPValue);
  if(enable)  {
    /* set feature to status register which is ecc enable */
    result = nand_flash_set_feature(NAND_REG_FEATURE1, OTPValue | FEAT_OTP_ECCEN);
  }
  else  {
    /* clear feature to status register which is ecc enable */
    result |= nand_flash_set_feature(NAND_REG_FEATURE1, OTPValue & ~(FEAT_OTP_ECCEN));
  }
  if( result != NAND_SUCCESS) {
    return NAND_FUNC_ERROR;
  }

  return NAND_FUNC_SUCCESS;
}

/*!
  ****************************************************************************
  *@brief         check if ecc feature enabled/disabled
  *
  *@param[in]      enable true = enable, false = disable
  *@return          Result of the function - Error or success
******************************************************************************/
eNand_Func_Result nand_func_check_ecc(uint8_t *status)  {
  eNand_Result result = NAND_SUCCESS;
  uint8_t OTPValue;

  /* get feature register */
  result = nand_flash_get_feature(NAND_REG_FEATURE1, &OTPValue);
  if( result != NAND_SUCCESS) {
    return NAND_FUNC_ERROR;
  }
  /* check ecc enabled bit, if set to '1' ecc is enabled,
    else disabled */
  if(OTPValue & FEAT_OTP_ECCEN) {
    NRF_LOG_INFO("ECC enabled");
    *status = 1;
  }
  else  {
    NRF_LOG_INFO("ECC disabled");
    *status = 0;
  }
  return NAND_FUNC_SUCCESS;
}

/*!
  ****************************************************************************
  *     @brief          Write a byte array to the memory within the spare ECC
  *                     zone available  in each page.
  *
  *     @param[in]      page_pos: Byte start position within the memory. This can be
  *                     any value within 0 and 4096*64*2048-1 = 536870911
  *                     offset: spare area offset
  *     @param[in]      size: number of bytes to be transmitted to the memory
  *     @param[in]      in_array: Array with the information to be written
  *     @return Result of the function - Error or success
******************************************************************************/
eNand_Func_Result nand_func_write_ecc_zone(uint32_t page_pos,uint32_t offset,\
                                          uint32_t size,uint8_t * in_array) {
   uint8_t status;
   eNand_Result result=NAND_SUCCESS;

  /* load data corresponding to data area to cache */
  result = nand_flash_program_load(offset,size,in_array);
  /* return if program load is an error */
  if(result != NAND_SUCCESS)  {
    NRF_LOG_INFO("Error in program load");
    return NAND_FUNC_ERROR;
  }

   /* execute does writing of data to flash from cache */
  result = nand_flash_program_execute(page_pos);
  /* return if program execute is an error */
  if(result != NAND_SUCCESS)  {
    NRF_LOG_INFO("Error in program execute");
    return NAND_FUNC_ERROR;
  }

  /* wait for writing to get over */
  result = nand_flash_wait_till_ready();
  if( result != NAND_SUCCESS)
  {
    NRF_LOG_INFO("Error in wait till ready");
    return NAND_FUNC_ERROR;
  }

  /* get status register to check program fail bit is set, if bit set is
    '1', writing has failed return error, else return success */
  result = nand_flash_get_status(&status);
  if((status & FEAT_STATUS_P_FAIL) == FEAT_STATUS_P_FAIL) {
    NRF_LOG_INFO("Error in program fail");
    return NAND_FUNC_ERROR;
  }

  return NAND_FUNC_SUCCESS;
}

/*!
  ****************************************************************************
  * @brief              Read a byte array from the memory within the spare
  *                     ECC zone available  in each page.
  *
  * @param[in]          page_pos: page position to be read
  * @param[in]          offset:  offset address in page
  * @param[in]          size:    number of bytes to be read from the memory
  * @param[out]         out_array: Array to store the information in
  * @return Result of the function - Error or success
******************************************************************************/
eNand_Func_Result nand_func_read_ecc_zone(uint32_t page_pos,uint32_t offset,\
                                        uint32_t size,uint8_t * out_array)  {
  eNand_Result result = NAND_SUCCESS;
  uint8_t status;
  /* perform page read */
  result = nand_flash_page_read(page_pos);
  /* return error if page read fails */
  if(result != NAND_SUCCESS)  {
    NRF_LOG_INFO("Page read error");
    return NAND_FUNC_ERROR;
  }
  /* read status register */
  result = nand_flash_get_status(&status);
  /* return error if reading status register fails */
  if(result != NAND_SUCCESS)  {
    NRF_LOG_INFO("Error in get status");
    return NAND_FUNC_ERROR;
  }

  /* if ECC bits which are uncorrectable return error */
  if((status >> FEAT_STATUS_ECC_STATUS_SHIFT) == 2) {
    NRF_LOG_INFO("Status reg read error");
    return NAND_FUNC_ERROR;
  }

  /* perform read from cache */
  result = nand_flash_read_from_cache(offset,size,out_array);

  /*return error if read from cache fails */
  if( result != NAND_SUCCESS) {
    NRF_LOG_INFO("Read from Cache error");
    return NAND_FUNC_ERROR;
  }
  return NAND_FUNC_SUCCESS;
}

/*!
  ****************************************************************************
  * @brief          Check if a block is bad
  *
  * @param[in]        *mem_prop: memory properties structure
  *                   block: Block to retrieve if it is good or bad
  * @param[out]       isBad: Status of the block: true = bad, false = good
  * @return           Result of the function - Error or success
******************************************************************************/
eNand_Func_Result nand_func_is_bad_block(_memory_properties *mem_prop,\
                                        uint32_t block, bool *isBad)  {
  uint8_t bad_block[BLOCK_HEADER_SIZE];

   /* read bad block spare area */
  if(nand_func_read_ecc_zone((block*mem_prop->pages_per_block),\
                              BAD_BLOCK_AREA_LOC,BLOCK_HEADER_SIZE,bad_block)\
                             != NAND_FUNC_SUCCESS)  {
    NRF_LOG_INFO("Error in reading bad block headers");
    return NAND_FUNC_ERROR;
  }
  /* if a byte around 4096 memory location of first page of given block is not FF,
    its bad block, else good block */
  if(bad_block[0] != 0xFF)
    *isBad = true;
  else
    *isBad = false;
  return NAND_FUNC_SUCCESS;
}

/*!
  ****************************************************************************
  * @brief        Erase a certain number of blocks of the memory.
  *
  * @param[in]      memProp Properties of the memory used
  * @param[in]      block_init Block to start erasing
  * @param[in]      numberOfBlocks Number of blocks to erase.
  * @return         Result of the function - Error or success
******************************************************************************/
eNand_Func_Result nand_func_erase(_memory_properties *mem_prop, \
                                  uint32_t block_init, uint32_t number_of_blocks) {
  eNand_Result result = NAND_SUCCESS;
  uint8_t status;
  uint32_t i;
  bool is_block_bad=false;
#ifdef FORMAT_DEBUG_INFO_CMD
  tmp_fs_format_debug_info.erase_failed_due_bad_block_check=0;
#endif

  /* Iterate through total number of blocks */
  for(i=0;i<number_of_blocks;i++) {
#ifdef PRINTS_OUT
    NRF_LOG_INFO("Block Index: %d, Block Status: %s",block_init+i,
                            (is_block_bad==true)?"bad":"good");

#endif
    nand_func_is_bad_block(mem_prop,(block_init+i), &is_block_bad);
    if(is_block_bad==false) {
      /* erase current block index */
      result = nand_flash_block_erase((block_init+i)*mem_prop->pages_per_block);
        
      /* read status register */
      result = nand_flash_get_status(&status);

      /* if status reg read fails , return error */
      if( result != NAND_SUCCESS)
        return NAND_FUNC_ERROR;

      /* if, erase fail bit is set in status register,
      erase has failed for current block */
      if((status & FEAT_STATUS_E_FAIL) == FEAT_STATUS_E_FAIL) {
        NRF_LOG_INFO("Error in Block Erase");
        return NAND_FUNC_ERROR;
      }
    }
    else  {
      /* block is bad, return error */
      NRF_LOG_INFO("Error in formatting Block as its bad:%d",(block_init+i));
#ifdef FORMAT_DEBUG_INFO_CMD
      tmp_fs_format_debug_info.erase_failed_due_bad_block_check=1;
#endif
      return NAND_FUNC_ERROR;
    }
  }
  return NAND_FUNC_SUCCESS;
}

/*!
  ****************************************************************************
  * @brief               Write a byte array to the memory. The user has to take
  *                     into account  that it is not possible to rewrite data
  *                     directly without erasing it in first place. Moreover
  *                     the user should not attempt to write multiple times to
  *                     the same page, as this may in wrong data being stored on
  *                     the memory.
  *
  * @param[in]          memProp Properties of the memory used
  * @param[in]          mem_pos Byte start position within the memory. This can be
  *                     any value within 0 and 4096*64*2048-1 = 536870911
  * @param[in]          size number of bytes to be transmitted to the memory
  * @param[in]          in_array Array with the information to be written
  * @return             Result of the function - Error or success
******************************************************************************/
eNand_Func_Result nand_func_write(_memory_properties *mem_prop,uint32_t mem_pos,\
                                  uint32_t size, uint8_t *in_array) {
  uint32_t page_pos;
  uint32_t column;
  uint32_t transmit_size;
  uint32_t transmitted_bytes = 0;
  uint8_t status;
  eNand_Result result = NAND_SUCCESS;

  while(size > 0) {
    /* calculate page index based on current memory position and transmitted bytes */
    page_pos  = (mem_pos+transmitted_bytes)/mem_prop->page_size;
    /* column index is calculated which indicates position in page
     it has to start writing */
    column   = (mem_pos+transmitted_bytes)%mem_prop->page_size;
    /* transmit size is truncated to 512 bytes if size > 512 else completely
     transferred as indicated by size variable */
    transmit_size = (size > MAX_QSPI_WRITE_LEN) ?  MAX_QSPI_WRITE_LEN : size;

    /* load data corresponding to data area to cache */
    result = nand_flash_program_load(column, transmit_size, &(in_array[transmitted_bytes]));
    if(result != NAND_SUCCESS)  {
      NRF_LOG_INFO("Error in program load");
    }

    /* return if program execute is an error */
    result = nand_flash_program_execute(page_pos);
    if(result !=  NAND_SUCCESS) {
      NRF_LOG_INFO("Error in program exe");
    }

    /* wait for writing to get over */
    result = nand_flash_wait_till_ready();
    if(result != NAND_SUCCESS)
    {
      NRF_LOG_INFO("Error in wait till ready");
    }

    /*read status register */
    result = nand_flash_get_status(&status);
    if(result != NAND_SUCCESS)  {
      NRF_LOG_INFO("Error in get status");
    }
    /* if program fail return error */
    if((status & FEAT_STATUS_P_FAIL) == FEAT_STATUS_P_FAIL) {
      NRF_LOG_INFO("Error in P Fail");
      return NAND_FUNC_ERROR;
    }
    /* increment transmitted bytes */
    transmitted_bytes += transmit_size;
    /* decrement size */
    size -= transmit_size;
    if( result != NAND_SUCCESS) {
      NRF_LOG_INFO("Error in nand func write,returning errors");
      return NAND_FUNC_ERROR;
    }
  }
  return NAND_FUNC_SUCCESS;
}

/*!
  ****************************************************************************
  * @brief                Read a byte array from the memory.
  *
  * @param[in]            memProp Properties of the memory used
  * @param[in]            mem_pos Byte start position within the memory. This can be
  *                       any value within 0 and 4096*64*2048-1 = 536870911
  * @param[in]            size number of bytes to be read from the memory
  * @param[out]           out_array Array to store the information in
  * @return               Result of the function - Error or success
******************************************************************************/
eNand_Func_Result nand_func_read(_memory_properties *mem_prop,\
                                uint32_t mem_pos, uint32_t size, \
                                uint8_t * out_array)  {
  uint32_t page_pos=0;
  uint32_t column=0;
  uint32_t transmit_size=0;
  uint32_t transmitted_bytes = 0;
  uint8_t status;
  eNand_Result result = NAND_SUCCESS;

  while(size > 0) {
#ifdef PROFILE_TIME_ENABLED
      nTick1 = get_micro_sec();
#endif
    /* calculate page index based on current memory position and transmitted bytes */
    page_pos = (mem_pos+transmitted_bytes)/mem_prop->page_size;
    /* column index is calculated which indicates position in page
     it has to start reading */
    column  =  (mem_pos+transmitted_bytes)%mem_prop->page_size;
    /* transmit size is truncated to 4096 bytes if size > 4096 else completely
     transferred as indicated by size variable */
    transmit_size = (size > MAX_QSPI_READ_LEN) ? MAX_QSPI_READ_LEN:size;

    /* page read performs page read from flash to cache */
    result = nand_flash_page_read(page_pos);
    if(result != NAND_SUCCESS)  {
      NRF_LOG_INFO("Error in page read");
      return NAND_FUNC_ERROR;
    }

    /* read status register */
    result = nand_flash_get_status(&status);
    if(result != NAND_SUCCESS)  {
      NRF_LOG_INFO("Error in get status");
      return NAND_FUNC_ERROR;
    }

    /* if status register ecc bits is uncorrectable */
    if(((status & FEAT_STATUS_ECC_STATUS) >> FEAT_STATUS_ECC_STATUS_SHIFT) == 2)  {
      NRF_LOG_INFO("Error in ECC");
      return NAND_FUNC_ERROR;
    }

    /* if read from cache error, return error */
    result = nand_flash_read_from_cache(column,transmit_size, &(out_array[transmitted_bytes]));
    if(result != NAND_SUCCESS)  {
      NRF_LOG_INFO("Error in read from cache");
      return NAND_FUNC_ERROR;
    }
    /* decrement size variable by number of bytes read */
    size -= transmit_size;

    /* increment size variable by number of bytes read */
    transmitted_bytes += transmit_size;
#ifdef PROFILE_TIME_ENABLED
      uint32_t page_read_time = get_micro_sec() - nTick1;
      NRF_LOG_INFO("********* Page read time = %d **********",page_read_time);
#endif
  }
  return NAND_FUNC_SUCCESS;
}

/*!
  ****************************************************************************
  * @brief          Function to erase the entire memory.
  *
  * @param[in]      memProp Properties of the memory used
  * @return         Result of the function - Error or success
******************************************************************************/
eNand_Func_Result nand_func_erase_memory(_memory_properties *mem_prop)  {
  uint32_t i;
  uint8_t status;
  eNand_Result result=NAND_SUCCESS;
  bool is_block_bad=false;
  uint32_t iterations = mem_prop->mem_size/mem_prop->block_size;


  for(i=0;i<iterations;i++) {
    nand_func_is_bad_block(mem_prop,i,&is_block_bad);
    if(is_block_bad == false) {
      result = nand_flash_block_erase(i*mem_prop->pages_per_block);
      if(result != NAND_SUCCESS)  {
       	NRF_LOG_INFO("Error in block erase");
      }
      result = nand_flash_get_status(&status);
      if(result != NAND_SUCCESS)  {
        NRF_LOG_INFO("Error in gets status");
      }
      if((status & FEAT_STATUS_E_FAIL) == FEAT_STATUS_E_FAIL) {
        NRF_LOG_INFO("Error in erase E_FAIL");
       	return NAND_FUNC_ERROR;
      }
      if(result != NAND_SUCCESS)  {
       	NRF_LOG_INFO("Error in erasing block no:%d",i);
       	return NAND_FUNC_ERROR;
      }
    }
  }
  return NAND_FUNC_SUCCESS;
}

/*!
  ****************************************************************************
  * @brief                Mark a block as bad. This will write the first two bytes
  *                      of the  spare area of the first page of the block to 00
  *
  * @param[in]            memProp Properties of the memory used
  * @param[in]            block Block to be set as bad
  * @return               Result of the function - Error or success
******************************************************************************/
eNand_Func_Result nand_func_mark_bad_block(_memory_properties *mem_prop,\
                                          uint32_t block)
{
  uint8_t bad_block[4] = {0,0,0,0};
  /* write spare area to '0' */
  if(nand_func_write_ecc_zone((block*mem_prop->pages_per_block),\
                                BAD_BLOCK_AREA_LOC,1, bad_block) \
                                != NAND_FUNC_SUCCESS) {
    NRF_LOG_INFO("Error in writing bad block header");
    return NAND_FUNC_ERROR;
  }
  return NAND_FUNC_SUCCESS;
}

/*!
  ***********************************************************************************
  * @brief                Move a page internally to another page. It is not possible
  *                       to move pages among different planes. In other words,
  *                       the source page and the destination page both have to be within
  *                       an even numbered or an  odd numebered block.
  *
  * @param[in]            memProp Properties of the memory used
  * @param[in]            page_init Source page to move
  * @param[in]            page_end Destination to move the page
  * @return               Result of the function - Error or success
**************************************************************************************/
eNand_Func_Result nand_func_page_move(uint32_t page_init, uint32_t page_end)  {
  uint8_t status;
  eNand_Result result=NAND_SUCCESS;

  /* read source  page, this reads data from flash to cache */
  result = nand_flash_page_read(page_init);
  if(result != NAND_SUCCESS)  {
     /* error in page read, return error */
     NRF_LOG_INFO("Error in reading page for page move");
     return NAND_FUNC_ERROR;
  }

  /* write it page destination from cache to flash */
  result = nand_flash_program_execute(page_end);
  if(result != NAND_SUCCESS)  {
    /* error in program execute */
    NRF_LOG_INFO("Error in program execute for page move");
  }

  /* wait till operation to get over */
  result = nand_flash_wait_till_ready();
  if(result != NAND_SUCCESS)
  {
    NRF_LOG_INFO("Error in wait till ready");
  }

  /* read status register */
  result = nand_flash_get_status(&status);

  /* if program fail, return error */
  if((status & FEAT_STATUS_P_FAIL) == FEAT_STATUS_P_FAIL) {
    NRF_LOG_INFO("Error in program fail");
    return NAND_FUNC_ERROR;
  }
  if( result != NAND_SUCCESS) {
    NRF_LOG_INFO("Error in page move");
    return NAND_FUNC_ERROR;
  }
  return NAND_FUNC_SUCCESS;
}

/*!
  ***********************************************************************************
  * @brief                  Configure the block lock register to avoid unintended access to
  *                         a subset of the blocks within the memory.
  *
  * @param[in]      lock_config locking configuration. Possible values:
  *                         NAND_FUNC_BLOCK_LOCK_ALL_UNLOCKED
  *                         NAND_FUNC_BLOCK_LOCK_UPPER_1_64_LOCKED
  *                         NAND_FUNC_BLOCK_LOCK_UPPER_1_32_LOCKED
  *                         NAND_FUNC_BLOCK_LOCK_UPPER_1_16_LOCKED
  *                         NAND_FUNC_BLOCK_LOCK_UPPER_1_8_LOCKED
  *                         NAND_FUNC_BLOCK_LOCK_UPPER_1_4_LOCKED
  *                         NAND_FUNC_BLOCK_LOCK_UPPER_1_2_LOCKED
  *                         NAND_FUNC_BLOCK_LOCK_LOWER_1_2_LOCKED
  *                         NAND_FUNC_BLOCK_LOCK_LOWER_1_4_LOCKED
  *                         NAND_FUNC_BLOCK_LOCK_LOWER_1_8_LOCKED
  *                         NAND_FUNC_BLOCK_LOCK_LOWER_1_16_LOCKED
  *                         NAND_FUNC_BLOCK_LOCK_LOWER_1_32_LOCKED
  *                         NAND_FUNC_BLOCK_LOCK_LOWER_63_64_LOCKED
  *                         NAND_FUNC_BLOCK_LOCK_LOWER_31_32_LOCKED
  *                         NAND_FUNC_BLOCK_LOCK_LOWER_15_16_LOCKED
  *                         NAND_FUNC_BLOCK_LOCK_LOWER_7_8_LOCKED
  *                         NAND_FUNC_BLOCK_LOCK_LOWER_1_2_LOCKED
  *                         NAND_FUNC_BLOCK_LOCK_UPPER_63_64_LOCKED
  *                         NAND_FUNC_BLOCK_LOCK_UPPER_31_32_LOCKED
  *                         NAND_FUNC_BLOCK_LOCK_UPPER_15_16_LOCKED
  *                         NAND_FUNC_BLOCK_LOCK_UPPER_7_8_LOCKED
  *                         NAND_FUNC_BLOCK_LOCK_UPPER_1_2_LOCKED
  *                         NAND_FUNC_BLOCK_LOCK_LOWER_63_64_LOCKED
  *                         NAND_FUNC_BLOCK_LOCK_LOWER_31_32_LOCKED
  *                         NAND_FUNC_BLOCK_LOCK_LOWER_15_16_LOCKED
  *                         NAND_FUNC_BLOCK_LOCK_LOWER_7_8_LOCKED
  *                         NAND_FUNC_BLOCK_0_LOCKED
  *                         NAND_FUNC_BLOCK_LOCK_ALL_LOCKED
  *                       [BP2 BP1 BP0 CMP0 INV]
  * @return               Result of the function - Error or success
**************************************************************************************/
eNand_Func_Result nand_func_set_block_lock(eNand_Func_Block_Lock lock_config) {
  uint8_t block_lock;
  eNand_Result result=NAND_SUCCESS;
  /* read feature register */
  result = nand_flash_get_feature(NAND_REG_PROTECTION, &block_lock);
  if(result != NAND_SUCCESS)  {
    NRF_LOG_INFO("Error in get feature");
  }

  /* extract block lock bits */
  block_lock = (block_lock & 0x81) | (lock_config<<1);
  /* set feature block lock bits */
  result = nand_flash_set_feature(NAND_REG_PROTECTION, block_lock);

  if(result != NAND_SUCCESS)  {
    NRF_LOG_INFO("Error in set feature");
    return NAND_FUNC_ERROR;
  }
  return NAND_FUNC_SUCCESS;
}

/*!
  ***********************************************************************************
  * @brief                  Obtain the block locking configuration currently set on
                            the memory.
  *
  * @param[out]             lock_config Current block lock configuration
  * @return                 Result of the function - Error or success
**************************************************************************************/
eNand_Func_Result nand_func_get_block_lock(eNand_Func_Block_Lock * lock_config) {
  /* read protection register for block lock configuration */
  if(nand_flash_get_feature(NAND_REG_PROTECTION, (uint8_t*)lock_config) \
    != NAND_SUCCESS)  {
    /* If error in reading block lock return error */
     NRF_LOG_INFO("Error in getting block lock");
     return NAND_FUNC_ERROR;
  }
  return NAND_FUNC_SUCCESS;
}

/*!
  ***********************************************************************************
  * @brief            Bad block marked is reset.
  *
  * @param[out]      lock_config Current block lock configuration
  * @return          Result of the function - Error or success
**************************************************************************************/
eNand_Func_Result nand_func_bad_block_mark_reset(_memory_properties *mem_prop,\
                                                uint32_t block) {
  eNand_Result result = NAND_SUCCESS;
  uint8_t status;

  /* erase block given by block index */
  result = nand_flash_block_erase(block*mem_prop->pages_per_block);
  if( result != NAND_SUCCESS) {
    NRF_LOG_INFO("Error in block erase");
    return NAND_FUNC_ERROR;
  }

  /* read status register */
  result = nand_flash_get_status(&status);
  if( result != NAND_SUCCESS) {
    NRF_LOG_INFO("Error in get status");
    return NAND_FUNC_ERROR;
  }

  /* if erase fail , return error */
  if((status & FEAT_STATUS_E_FAIL) == FEAT_STATUS_E_FAIL) {
    /* return error if erase fails */
    NRF_LOG_INFO("Error in Erase fail");
    return NAND_FUNC_ERROR;
  }
  return NAND_FUNC_SUCCESS;
}
#endif