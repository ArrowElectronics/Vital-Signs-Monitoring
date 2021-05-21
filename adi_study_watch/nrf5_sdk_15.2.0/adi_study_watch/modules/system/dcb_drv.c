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
#include "DCB_drv.h"


//Offset values of DCB
static int16_t index_offset_array[18]={
                           0,
                           4,
                          44,
                          1068,
                          1580,
                          2092,
                          2604,
                          3116,
                          3628,
                          4140,
                          4652,
                          4684,
                          4716,
                          4748,
                          4876,
                          -1,
                          -1,
                          -1,
                          };


NRF_FSTORAGE_DEF(nrf_fstorage_t fstorage) =
{
    /* Set a handler for fstorage events. */
    .evt_handler = fstorage_evt_handler,

    /* These below are the boundaries of the flash space assigned to this instance of fstorage.
     * You must set these manually, even at runtime, before nrf_fstorage_init() is called.
     * The function nrf5_flash_end_addr_get() can be u sed to retrieve the last address on the
     * last page of flash available to write data. */
    .start_addr = DCB_START_ADDR,
    .end_addr   = DCB_END_ADDR,
};

static void fstorage_evt_handler(nrf_fstorage_evt_t * p_evt)
{
    if (p_evt->result != NRF_SUCCESS)
    {
        NRF_LOG_INFO("--> Event received: ERROR while executing an fstorage operation.");
        return;
    }

    switch (p_evt->id)
    {
        case NRF_FSTORAGE_EVT_WRITE_RESULT:
        {
            NRF_LOG_INFO("--> Event received: wrote %d bytes at address 0x%x.",
                         p_evt->len, p_evt->addr);
            
        } break;

        case NRF_FSTORAGE_EVT_ERASE_RESULT:
        {
            NRF_LOG_INFO("--> Event received: erased %d page from address 0x%x.",
                         p_evt->len, p_evt->addr);
            
        } break;

        default:
            break;
    }
}

void wait_for_flash_ready(nrf_fstorage_t const * p_fstorage)
{
    /* While fstorage is busy, sleep and wait for an event. */
    while (nrf_fstorage_is_busy(p_fstorage))
    {
        power_manage();
    }
}

void power_manage(void)
{
    __WFE();
}

/**@brief   Function for initializing DCB.
 *
 * @param None
 * @return None
 */
void dcb_flash_init()
{
    NRF_LOG_INFO("Accessing Flash Memory to store DCB.");
    nrf_fstorage_api_t * p_fs_api;
    NRF_LOG_INFO("SoftDevice not present.");
    NRF_LOG_INFO("Initializing nrf_fstorage_nvmc implementation...");
    p_fs_api = &nrf_fstorage_nvmc;
    ret_code_t rc = nrf_fstorage_init(&fstorage, p_fs_api, NULL);
    APP_ERROR_CHECK(rc);  
}

/**@brief   Function for writing data to backup region of flash memory.
 *
 * @param[in]   ADDR        Starting address of the backup region in the flash memory
 * @param[in]   data        Data to be written.
 * @return None
*/
void WriteConfigBackUp(uint32_t nAddr,void const *pData)
{
    ret_code_t rc_;
    NRF_LOG_INFO("Writing \"%x\" to flash at \"%x\".", pData,nAddr);
    rc_ = nrf_fstorage_write(&fstorage,nAddr,pData,DCB_MEMORY_SIZE, NULL); //Writing data to flash 
    APP_ERROR_CHECK(rc_);
    wait_for_flash_ready(&fstorage);
}

/**@brief   Function for writing data to flash.
 *
 * @param[in]   data        Data to be written.
 * @param[in]   size        Length of the data (in bytes).
 * @return None
*/
void WriteConfig(uint8_t const *nData,uint32_t nSize)
{
    uint32_t ADDR=DCB_START_ADDR;
    ret_code_t rc_;
    NRF_LOG_INFO("Writing \"%x\" to flash at \"%x\".", nData,ADDR);
    rc_ = nrf_fstorage_write(&fstorage,ADDR,nData,nSize, NULL); //Writing data to flash 
    APP_ERROR_CHECK(rc_);
    wait_for_flash_ready(&fstorage);
}

/**@brief   Function for erasing flash pages.
 *
 * @details This function erases @p page_count pages starting from the page at address @p ADDR.
 *          The erase operation must be initiated on a page boundary.
 *
 * @param[in]   ADDR          Address of the page to erase.
 * @param[in]   page_count    Number of pages to erase.
 * @return None                                  
 */
void EraseFlash(uint32_t nAddr,uint8_t nPageCount)
{
   
  ret_code_t rc = nrf_fstorage_erase(&fstorage,nAddr,nPageCount, NULL);
     if (rc != NRF_SUCCESS)
     {
       NRF_LOG_INFO("Erase unsuccessful");
     }
     else
     {
      NRF_LOG_INFO("Erase successful");
     }
}

static uint8_t gsConfigBkupBuf[DCB_MEMORY_SIZE];

/**@brief   Function for reading data from flash.
 *
 * @param[in]   ADDR    Address in flash where to read from.
 * @param[in]   len     Length of the data to be copied (in bytes).
 *
 * @return None
 */
void ReadFlash(uint8_t * pBuf,uint32_t nLen)
{
      ret_code_t rc;
      rc = nrf_fstorage_read(&fstorage, DCB_START_ADDR, pBuf, nLen);//Read Data from DCB start address and store it in pBuf
      if (rc != NRF_SUCCESS)
       {
        NRF_LOG_INFO( "nrf_fstorage_read() returned: %s\r\n",
                        nrf_strerror_get(rc));
        return;
        } 
}

/**@brief   Function for taking backup of DCB(8KB) and storing it in a DCB-backup (8KB) region.
 *
 * @param None
 *
 * @return None
 */
void DCBBackUp()
{
  ReadFlash(gsConfigBkupBuf,DCB_MEMORY_SIZE);
  WriteConfigBackUp(BACKUP_START_ADDR,gsConfigBkupBuf);//8Kb of backup
}

/**@brief   FuFunction for erasing flash pages.
 *
 * DCB_START_ADDR macro to set the address of the flash memory to be erased
 * PAGE_CNT macro to set Number of pages to erase.
 * @param None
 *
 * @return None
 */
void DCBErase()
{
    uint32_t page3_addr=DCB_START_ADDR;
    EraseFlash(page3_addr,PAGE_CNT);
} 

/**@brief   FuFunction for erasing backup region of flash
 *
 * BACKUP_START_ADDR macro to set the address of the flash memory to be erased  
 * PAGE_CNT macro to set Number of pages to erase.
 * @param None
 *
 * @return None
 */
void DCBEraseBackUp()
{
   uint32_t page3_addr=BACKUP_START_ADDR;
   EraseFlash(page3_addr,PAGE_CNT);
}   

/**@brief   Function for writing DCB to flash.
 *
 *This function will first erase the flash bacup region,take back up of the DCB region and erase the DCB region followed by writing DCB region with the given @p data
 * @param[in]   data        Data to be written.
 * @param[in]   size        Length of the data (in bytes).
 * @return None
*/
void DCBWrite(uint8_t *pData,uint32_t nSize)
{
   DCBEraseBackUp();//Erases 8Kb of backup flash memory ranging from 0xf5000 to 0xf6FFF
   DCBBackUp();//A copy of the data stored in flash memory ranging from 0xf3000 to 0xf4fff will be stored in the flash memory ranging from 0xf5000 to 0xf6FFF. 
   DCBErase();//Erase 8kb of DCB flash memory
   WriteConfig(pData,nSize);//Writing data to complete 8kb of flash memory 
}

/**@brief   Function for reading DCB from flash.
 *
 * @param[in]   id          Device/parameter id
 * @param[in]   pConfig     Data to be written.
 * @param[in]   size        Length of the data to be copied (in bytes).
 * @retval  DCB_SUCCESS            If read operation is successful.
 * @retval  DCB_ERROR              If the read operation is not successful. 
 * @retval  DCB_LENGTH_ERROR        If there is mismatch in the size with respect to the offset size.
*/
int16_t DCBRead(eDcbConfigIndex_t eIndex,uint8_t *pConfig, uint32_t nSize)
{
   uint32_t ADDR;
   int16_t current_offset_value,next_index_offset,size_of_offset_index=0;
   current_offset_value=index_offset_array[eIndex];
   next_index_offset=index_offset_array[(eIndex+1)];
   size_of_offset_index=next_index_offset-current_offset_value;
   ADDR=DCB_START_ADDR+current_offset_value;
   if(nSize>(size_of_offset_index))
    {
     NRF_LOG_INFO("Size Mismatch-Data Size exceeding the given limit");
     return DCB_LENGTH_ERROR;
    }
   else 
    {
     ret_code_t rc;
     rc = nrf_fstorage_read(&fstorage, ADDR, pConfig, nSize);
     if (rc != NRF_SUCCESS)
       {
        NRF_LOG_INFO( "nrf_fstorage_read() returned: %s\r\n",
                        nrf_strerror_get(rc));
        return DCB_ERROR;
        }
      return DCB_SUCCESS;
   }
}   
