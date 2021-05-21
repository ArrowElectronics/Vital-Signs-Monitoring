/**
***************************************************************************
* @addtogroup Tasks
* @{
* @file         nand_cmd.c
* @author       ADI
* @version      V1.0.0
* @date         17-April-2019
* @brief        Source file contains NAND Flash driver APIs.
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
#include "nand_cmd.h"
#include "nrf_drv_qspi.h"
#include "watch_board_evt_pin_config.h"
#include "nrf_delay.h"
#include "hw_if_config.h"
#include "math.h"

#if NRF_LOG_ENABLED
#include "nrf_log.h"
#include "nrf_log_ctrl.h"
#include "nrf_log_default_backends.h"
#endif

/* FreeRTOS related */
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"

//#define BLOCKING_CALL

#define QSPI_NAND_FLASH_CONFIG                                        \
{                                                                       \
    .xip_offset  = 0,                         \
    .pins = {                                                           \
       .sck_pin     = QSPI_NAND_FLASH_SCK_PIN,                                \
       .csn_pin     = QSPI_NAND_FLASH_CSN_PIN,                                \
       .io0_pin     = QSPI_NAND_FLASH_IO0_PIN,                                \
       .io1_pin     = QSPI_NAND_FLASH_IO1_PIN,                                \
       .io2_pin     = QSPI_NAND_FLASH_IO2_PIN,                                \
       .io3_pin     = QSPI_NAND_FLASH_IO3_PIN,                                \
    },                                                                  \
    .irq_priority   = (uint8_t)NRFX_QSPI_CONFIG_IRQ_PRIORITY,           \
    .prot_if = {                                                        \
        .readoc     = QSPI_IFCONFIG0_READOC_READ4O,       \
        .writeoc    = QSPI_IFCONFIG0_WRITEOC_PP4O,     \
        .addrmode   = (nrf_qspi_addrmode_t)NRFX_QSPI_CONFIG_ADDRMODE,   \
        .dpmconfig  = false,                                            \
    },                                                                  \
    .phy_if = {                                                         \
        .sck_freq   = NRF_QSPI_FREQ_32MDIV1, \
        .sck_delay  = (uint8_t)NRFX_QSPI_CONFIG_SCK_DELAY,              \
        .spi_mode   = (nrf_qspi_spi_mode_t)NRFX_QSPI_CONFIG_MODE,       \
        .dpmen      = false                                             \
    },                                                                  \
}
/*
    select: 1=512byte,0=256byte.
*/
__STATIC_INLINE void nrf_qspi_ifconfig0_set_page_size(uint8_t select)
{
    if(select == 0)
    {
        NRF_QSPI->IFCONFIG0 &= 0xffffefff;
    }
    else
    {
        NRF_QSPI->IFCONFIG0 |= 0x00001000;
    }
}
static int16_t qspi_init(void)
{
    eNand_Result err_code;
    nrf_drv_qspi_config_t config = QSPI_NAND_FLASH_CONFIG;

    err_code = nrf_drv_qspi_init(&config, NULL, NULL);
    if(NRFX_SUCCESS != err_code)
    {
        NRF_LOG_INFO("QSPI init error,err_code=%d",err_code);
		return err_code;
    }
    nrf_qspi_ifconfig0_set_page_size(1);
}

eNand_Result nand_flash_reset(void)
{
    eNand_Result ret;
    nrf_qspi_cinstr_conf_t cinstr_cfg = {
        .opcode    = NAND_RESET,
        .length    = NRF_QSPI_CINSTR_LEN_1B,
        .io2_level = true,
        .io3_level = true,
        .wipwait   = false,
        .wren      = false
    };
    nrf_drv_qspi_cinstr_xfer(&cinstr_cfg, NULL, NULL);
#ifdef BLOCKING_CALL
    nrf_delay_ms(2);
#else
    MCU_HAL_Delay(2);
#endif
    ret = nand_flash_wait_till_ready();

    return ret;
}


/**     @brief Send the write enable command to the memory. This has to be called
  *            before the program execute command.
  *
  *     @return Result of the function - Error or success
  */

eNand_Result nand_flash_write_enable(void)
{
    eNand_Result ret; 
	nrf_qspi_cinstr_conf_t cinstr_cfg = {
		.opcode    = NAND_WRITEENABLE,        
		.length    = NRF_QSPI_CINSTR_LEN_1B,        
		.io2_level = true,        
		.io3_level = true,        
		.wipwait   = false,        
		.wren      = false    
	};    
	
	ret = nrf_drv_qspi_cinstr_xfer(&cinstr_cfg, NULL, NULL);
	
	return ret;
}

/**     @brief Function to send the write disable command to the memory. As the
  *            WEL bit is automatically cleared once the page execute command is
  *            called, it is not necessary to use it.
  *            
  *     @return Result of the function - Error or success
  */

eNand_Result nand_flash_write_disable(void)
{
	eNand_Result ret;    
	nrf_qspi_cinstr_conf_t cinstr_cfg = {        
		.opcode    = NAND_WRITEDISABLE,        
		.length    = NRF_QSPI_CINSTR_LEN_1B,        
		.io2_level = true,        
		.io3_level = true,        
		.wipwait   = false,        
		.wren      = false    
	};  
	  
	ret = nrf_drv_qspi_cinstr_xfer(&cinstr_cfg, NULL, NULL);    
	
	return ret;
}




/**     @brief Function to read the memory ID. This will return 2 bytes that
  *            identify the memory.
  *
  *     @param[out]      mfacture_id:Manufacturing ID

  *  					 device_id: device ID		
  *     @return Result of the function - Error or success
  */

eNand_Result nand_flash_read_id(uint8_t *mfacture_id, uint8_t *device_id)
{    
	eNand_Result ret;    
	nrf_qspi_cinstr_conf_t cinstr_cfg = {        
		.opcode    = NAND_READID,
		.length    = NRF_QSPI_CINSTR_LEN_4B,
		.io2_level = true,
		.io3_level = true,
		.wipwait   = false,
		.wren      = false
	};
	uint8_t tx_data[3] = {0};
	uint8_t rx_data[3] = {0};
	
	tx_data[0] = 0;
	ret = nrf_drv_qspi_cinstr_xfer(&cinstr_cfg, tx_data, rx_data);
    *mfacture_id = rx_data[1];
    *device_id = rx_data[2];
    return (ret);
}

/**     @brief Function to set one of the features of the memory. There are
  *            bit helpers available on the header file that can be used to
  *            set the value of the elements of each feature to be set.
  *
  *     @param[in]      eNand_Regs: The feature to set
  *     @param[in]      value: The value to set to the feature
  *     @return Result of the function - Error or success
  */

eNand_Result nand_flash_set_feature(eNand_Regs regs, uint8_t value)
{    
	eNand_Result ret;
	nrf_qspi_cinstr_conf_t cinstr_cfg = {
        .opcode    = NAND_SETREGS,
        .length    = NRF_QSPI_CINSTR_LEN_3B,
        .io2_level = true,
        .io3_level = true,
        .wipwait   = false,
        .wren      = false
    };
    uint8_t tx_data[2] = {0};
    tx_data[0] = regs;
    tx_data[1] = value;
   	ret =  nrf_drv_qspi_cinstr_xfer(&cinstr_cfg, tx_data, NULL);
	return ret;
}

int16_t nand_flash_init(void)
{
    eNand_Result ret;  
    uint8_t out_val;
	
	err_code = qspi_init();
    if(err_code	!= NRFX_SUCCESS)
	{
		NRF_LOG_INFO("QSPI Init error");
		return err_code;
	}

    if(NRFX_SUCCESS != nand_flash_reset())
    {
        NRF_LOG_INFO("NandFlash_Reset error");
		return err_code;
    }
    do
    {
        nand_flash_set_feature(NAND_REG_PROTECTION,0x00);
        nand_flash_set_feature(NAND_REG_FEATURE1,0x10);
        nand_flash_set_feature(NAND_REG_FEATURE2,0x00);
        nand_flash_get_feature(NAND_REG_STATUS1,&out_val);
    }
    while((out_val&0x01)!=0);
    
    uint8_t value1,value2;
    nand_flash_read_ID(&value1,&value2);
    NRF_LOG_INFO("manufacture ID = 0x%x,Device ID = 0x%x",value1,value2);
}
/**     @brief Function to obtain one of the features of the memory. There are
  *            bit helpers available on the header file that can be used to
  *            rethrieve the value of one of the elements of each feature.
  *
  *     @param[in]      eNand_Regs: The feature to read from the memory
  *     @param[out]     outVal: The value read from the memory
  *     @return Result of the function - Error or success
  */
eNand_Result nand_flash_get_feature(eNand_Regs regs, uint8_t * outVal)
{
	eNand_Result ret;
	nrf_qspi_cinstr_conf_t cinstr_cfg = {
        .opcode    = NAND_GETREGS,
        .length    = NRF_QSPI_CINSTR_LEN_3B,
        .io2_level = true,
        .io3_level = true,
        .wipwait   = false,
        .wren      = false
    };
    uint8_t tx_data[2] = {0};
    uint8_t rx_data[2] = {0};
	
    tx_data[0] = regs;
    ret = nrf_drv_qspi_cinstr_xfer(&cinstr_cfg, tx_data, rx_data);
    *outVal = rx_data[1];
    return (ret);
}
/**     @brief  Read the status register and return its value.
  *     
  *     @param[out]      status returned value of the status register.
  *     @return Result of the function - Error or success
  */
  eNand_Result nand_flash_get_status(uint8_t * status)
  {
  	return (nand_flash_get_feature(NAND_REG_STATUS1,status));		  
  }

/**     @brief  Wait until the OIP element within the status register is cleared
  *             
  *
  *     @return Result of the function - Error or success
  */
eNand_Result nand_flash_wait_till_ready(void)
{
    eNand_Result ret;
    uint32_t ulTimeout = 0x20;
    uint8_t ucStatus;
    
    ret = nand_flash_get_status(&ucStatus);
    if(NRFX_SUCCESS != ret)
    {
        NRF_LOG_INFO("read status return error.ret = %d",ret);
    }

    while(((ucStatus&0x7d)&&( ulTimeout != 0x00))||(NRFX_SUCCESS != ret))
    {
//        NRF_LOG_INFO("read status.time = %d,ucStatus = 0x%x",ulTimeout,ucStatus);
//        nrf_delay_ms(1);
        ret = nand_flash_get_status(&ucStatus);
        ulTimeout--;
        
    }

    if(ulTimeout == 0x00)
    {
        NRF_LOG_INFO("time out error.ucStatus = 0x%x",ucStatus);
        return NRFX_ERROR_TIMEOUT;
    }
    return ret;
}


/**     @brief  Write a byte array into the cache register without modifying the
  *             data already present on the cache
  *
  *     @param[in]      column Byte within the cache to start writing
  *     @param[in]      size Number of bytes to be written of the cache
  *     @param[in]      in_array Array of bytes to be written
  *     @return Result of the function - Error or success
*/

eNand_Result nand_flash_program_load_random(uint32_t column,uint32_t size,uint8_t * in_array)
{
    eNand_Result ret;
    nrf_qspi_cinstr_conf_t cinstr_cfg = {
        .opcode    = NAND_PROGRAMLOADRANDOMDATA,
        .length    = NRF_QSPI_CINSTR_LEN_3B,
        .io2_level = true,
        .io3_level = true,
        .wipwait   = false,
        .wren      = false
    };
    uint8_t tx_data[8] = {0};
    uint32_t offset;
    for(offset = 0;size - offset > 6;offset+=6)
    {
        cinstr_cfg.length = NRF_QSPI_CINSTR_LEN_9B;
        tx_data[0] = (uint8_t)(((column + offset)>>8)&0xFF);
        tx_data[1] = (uint8_t)((column + offset)&0xFF);
        memcpy(&tx_data[2],(in_array+offset),6);
        ret = nrf_drv_qspi_cinstr_xfer(&cinstr_cfg, tx_data, NULL);
        if(NRFX_SUCCESS != ret)
        {
            NRF_LOG_INFO("nrf_drv_qspi_cinstr_xfer error,ret=%d",ret);
            return (ret);
        }
    }

    cinstr_cfg.length = (nrf_qspi_cinstr_len_t )(NRF_QSPI_CINSTR_LEN_3B + (size - offset));
    tx_data[0] = (uint8_t)(((column + offset)>>8)&0xFF);
    tx_data[1] = (uint8_t)((column + offset)&0xFF);
    memcpy(&tx_data[2],(in_array+offset),(size - offset));    
    return (nrf_drv_qspi_cinstr_xfer(&cinstr_cfg, tx_data, NULL));
}


































/**     @brief  Write a byte array into the cache register
  *
  *     @param[in]      column Byte within the cache to start writing
  *     @param[in]      size Number of bytes to be written of the cache
  *     @param[in]      in_array Array of bytes to be written
  *     @return Result of the function - Error or success
  */
eNand_Result nand_flash_program_load(uint32_t column,  uint32_t size, uint8_t * in_array)
{
    eNand_Result ret;
    uint32_t flash_address;
    if(((size == 0)||(size > 512))
    {
        return NRFX_ERROR_INVALID_LENGTH;
    }
	
	
	
	if((size&3) != 0)
    {
      size =  (int)(floor((size+3)/4))*4;
#ifdef PRINTS_OUT
      NRF_LOG_INFO("Size calculated=%d",size);
#endif
    }
    if((NULL == in_array)||((column&0x03) != 0))
    {
        return NRFX_ERROR_INVALID_PARAM;
    }
    flash_address = (column << 8);
    if(size > 4)
    {
        ret = nrf_drv_qspi_write(in_array+4,size-4,flash_address);
        if(NRFX_SUCCESS != ret)
        {
            NRF_LOG_INFO("nrf_drv_qspi_write error,ret=%d",ret);
            return (ret);
        }
    }

    return (nand_flash_program_load_random(column,4,in_array));
}
    
	
	
	
	
	
	
	
	
	
	
	
	
	
	
	
	
	
	
	
	
	
	
	
/**     @brief  Write the cache array to the memory. It will be written to the page
  *             indicated by the argument.
  *
  *     @param[in]      page_index page index.It can be from 
  *                     0 to 2047 blocks * 64 pages/block -1 = 26214
  *     @return Result of the function - Error or success
  */
eNand_Result nand_flash_program_execute(uint32_t page_index)
{
	eNand_Result ret;    
	nrf_qspi_cinstr_conf_t cinstr_cfg = {
		.opcode    = NAND_PROGRAMEXECUTE,
		.length    = NRF_QSPI_CINSTR_LEN_4B,
		.io2_level = true,
		.io3_level = true,
		.wipwait   = false,
		.wren      = false
	};    
	uint8_t tx_data[3] = {0};
    tx_data[0] = (uint8_t)((page_index>>16)&0xFF);
    tx_data[1] = (uint8_t)((page_index>>8)&0xFF);
    tx_data[2] = (uint8_t)((page_index)&0xFF);
    ret = nrf_drv_qspi_cinstr_xfer(&cinstr_cfg, tx_data, NULL);
    if(NRFX_SUCCESS != ret)
    {
        NRF_LOG_INFO("ProgramExecute error,ret=%d",ret);
        return (ret);
    }
#ifdef BLOCKING_CALL
    nrf_delay_ms(1);
#else
    MCU_HAL_Delay(1);
#endif
    ret = nand_flash_wait_till_ready();    
    return (ret);
}


/**     @brief  Function to erase a block of the memory. The page/block address
  *             has to be provided. This is the page index within the memory.
  *
  *     @param[in]      page_block_index page index.It can be from 
  *                     0 to 2047 blocks * 64 pages/block -1 = 262143
  *     eg: 5 block has to be erased , (5*64) has to be sent
  *     @return Result of the function - Error or success
  */
eNand_Result nand_flash_block_erase(uint32_t page_block_index)
{
    eNand_Result ret;
    
    nrf_qspi_cinstr_conf_t cinstr_cfg = {
        .opcode    = NAND_BLOCKERASE,
        .length    = NRF_QSPI_CINSTR_LEN_4B,
        .io2_level = true,
        .io3_level = true,
        .wipwait   = false,
        .wren      = true
    };
    uint8_t tx_data[3] = {0};
    tx_data[0] = (page_block_index>>16)&0xFF;
    tx_data[1] = (page_block_index>>8)&0xFF;
    tx_data[2] = (page_block_index)&0xC0;
    ret = nrf_drv_qspi_cinstr_xfer(&cinstr_cfg, tx_data, NULL);
    if(NRFX_SUCCESS != ret)
    {
        NRF_LOG_INFO("EraseBlock error,ret=%d",ret);
        return (ret);
    }
#ifdef BLOCKING_CALL
    nrf_delay_ms(3);
#else
    MCU_HAL_Delay(3);
#endif
    ret = nand_flash_wait_till_ready();

    return (ret);
}

static eNand_Result nand_flash_read_from_cache_one_line(uint32_t column,  uint8_t size, uint8_t * out_array)
{
	eNand_Result ret;
	nrf_qspi_cinstr_conf_t cinstr_cfg = {
        .opcode    = NAND_READFROMCACHE,
        .length    = NRF_QSPI_CINSTR_LEN_4B,
        .io2_level = true,
        .io3_level = true,
        .wipwait   = false,
        .wren      = false
    };
    uint8_t tx_data[8] = {0};
    uint8_t rx_data[8] = {0};

    if(size > 5)
    {
        size = 5;
    }
    cinstr_cfg.length += (size);
    tx_data[0] = (column >> 8)&0xff;
    tx_data[1] = (column)&0xff;
    tx_data[2] = 0;
    ret = nrf_drv_qspi_cinstr_xfer(&cinstr_cfg, tx_data, rx_data);
    memcpy(outArray,&rx_data[3],size);
    return (ret);
}

/**     @brief  Read information from the cache register
  *
  *     @param[in]      column Byte within the cache to start reading
  *     @param[in]      size Number of bytes to be read of the cache
  *     @param[out]     out_array Data read from the cache.
  *     @return Result of the function - Error or success
  */
eNand_Result nand_flash_read_from_cache (uint32_t column,  uint32_t size, uint8_t * out_array)
{
    eNand_Result ret;
    uint32_t flash_address;
    
    if(size == 0)
    {
        return NRFX_ERROR_INVALID_LENGTH;
    }
	if((size&0x03) != 0)
    {
      size = (int)(floor((size+3)/4))*4;
#ifdef PRINTS_OUT
      NRF_LOG_INFO("Size calculated=%d",size);
#endif
    }
    if((NULL == out_array)||((column&0x03) != 0))
    {
        return NRFX_ERROR_INVALID_PARAM;
    }
    
    if(column >= 4)
    {
        column -= 4;
        flash_address = (column << 8);
        ret = nrf_drv_qspi_read(out_array,size,flash_address);
    }
    else
    {
        nand_flash_read_from_cache_oneline(column,4 - column,out_array);
        flash_address = (column << 8);
        ret = nrf_drv_qspi_read(out_array+(4-column),size-4+column,flash_address);
    }
    
    return ret;
}

/**     @brief  Read one page of the memory given its index.
  *             
  *
  *     @param[in]      page_index page index.It can be from 
  *                     0 to 2047 blocks * 64 pages/block -1 = 262143
  *     @return Result of the function - Error or success
  */
eNand_Result nand_flash_page_read(uint32_t page_index)
{
    eNand_Result ret;
    uint8_t i;
    nrf_qspi_cinstr_conf_t cinstr_cfg = {
        .opcode    = NAND_PAGEREADTOCATCH,
        .length    = NRF_QSPI_CINSTR_LEN_4B,
        .io2_level = true,
        .io3_level = true,
        .wipwait   = false,
        .wren      = false
    };
    uint8_t tx_data[3] = {0};
    tx_data[0] = (page_index>>16)&0xFF;
    tx_data[1] = (page_index>>8)&0xFF;
    tx_data[2] = (page_index)&0xFF;
    do{
        nrf_drv_qspi_cinstr_xfer(&cinstr_cfg, tx_data, NULL);

        ret = nand_flash_wait_till_ready();
    }while((NRFX_SUCCESS != ret)&&(i++<5));
    return ret;
}



