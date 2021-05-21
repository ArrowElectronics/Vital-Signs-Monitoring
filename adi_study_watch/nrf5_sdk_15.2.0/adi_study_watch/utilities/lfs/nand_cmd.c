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
#include "nand_cmd.h"
#include "nrf_drv_qspi.h"
#include "us_tick.h"
#ifdef VSM_MBOARD
#include "watch_vsm_motherboard_pin_config.h"
#elif PCBA
#include "watch_board_pcba_pin_config.h"
#else
#include "watch_board_evt_pin_config.h"
#endif

#include "nrf_delay.h"
#include "hw_if_config.h"
#include "math.h"

#include "nrf_log_ctrl.h"
#define NRF_LOG_MODULE_NAME NAND_CMD

#if NAND_CMD_CONFIG_LOG_ENABLED
#ifdef NRF_LOG_DEFAULT_LEVEL
#undef NRF_LOG_DEFAULT_LEVEL
#define NRF_LOG_DEFAULT_LEVEL NAND_CMD_CONFIG_LOG_LEVEL
#endif
#define NRF_LOG_INFO_COLOR  NAND_CMD_CONFIG_INFO_COLOR
#define NRF_LOG_DEBUG_COLOR  NAND_CMD_CONFIG_DEBUG_COLOR
#else /* NAND_CMD_CONFIG_LOG_ENABLED */
#define NRF_LOG_LEVEL       0
#endif /* NAND_CMD_CONFIG_LOG_ENABLED */

#include "nrf_log.h"

/* Enable nrf logger */
NRF_LOG_MODULE_REGISTER();


/************************************************* Private variables ***********************************************************/
#ifdef PROFILE_TIME_ENABLED
uint32_t qspi_512_bytes_write_time = 0,tick,program_load_random_api_time = 0;
#endif

uint32_t TimeOutCount = 50;

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
        .readoc     = (nrf_qspi_readoc_t)QSPI_IFCONFIG0_READOC_READ4O,       \
        .writeoc    = (nrf_qspi_writeoc_t)QSPI_IFCONFIG0_WRITEOC_PP4O,     \
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

/*!
  ****************************************************************************
 *  @brief            Set qspi read/ write configuration , if select is '1'
                      qspi transfer supports 512 bytes at a stretch, if '0'
                      qspi supports 256 bytes transfer
  *
  *@param[in]         select: 1/0
  *@return[out]       None
******************************************************************************/
__STATIC_INLINE void nrf_qspi_ifconfig0_set_page_size(uint8_t select) {
    if(select == 0) {
        NRF_QSPI->IFCONFIG0 &= 0xffffefff;
    }
    else  {
        NRF_QSPI->IFCONFIG0 |= 0x00001000;
    }
}

static volatile bool m_evt_finished = false;

/*!
  ****************************************************************************
 *  @brief            QSPI call back function. This function is called when
                      erase/write/read operation is completed
  *
  *@param[in]         nrf_drv_qspi_evt_t event: qspi event structure
                      p_context: void pointer
  *@return[out]       None
******************************************************************************/
static void qspi_event_call_back(nrf_drv_qspi_evt_t event, void * p_context)  {
    if (event == NRF_DRV_QSPI_EVENT_DONE) {
        m_evt_finished = true;
    }
}

/*!
  ****************************************************************************
  *@brief             QSPI Initialization. This is first function to be called
  *                   whenever QSPI module of nordic has to be used. This inits
  *                   QSPI drivers of flash
  *@param[in]         None
  *@return[out]       None
******************************************************************************/
uint32_t nand_flash_qspi_init(void) {
    uint32_t err_code = NAND_SUCCESS;
    nrf_drv_qspi_config_t config = QSPI_NAND_FLASH_CONFIG;

    /* qspi driver init function */
    /* init is done with call back, hence qspi transfer is non
      blocking call */
    err_code = nrf_drv_qspi_init(&config,qspi_event_call_back, NULL);
    if(NRFX_SUCCESS != err_code)  {
        NRF_LOG_INFO("QSPI init error,err_code=%d",err_code);
        return err_code;
    }
    nrf_qspi_ifconfig0_set_page_size(1);
    return NRFX_SUCCESS;
}

/*!
  ****************************************************************************
  *@brief            QSPI deinit; This function releases QSPI drivers
  *
  *@param[in]         None
  *@return[out]       None
******************************************************************************/
void nand_flash_qspi_uninit(void) {
    nrfx_qspi_uninit();
}

/*!
  *********************************************************************************
  *@brief             Function to reset the flash memory. This is the very first
  *                   function that should be called. It will introduce a 2ms delay.
  *@param[in]         None
  *@return[out]       None
***********************************************************************************/
eNand_Result nand_flash_reset(void)
{
    eNand_Result ret = NAND_SUCCESS;
    /* custom command instruction which calls with opcode reset */
    nrf_qspi_cinstr_conf_t cinstr_cfg = {
        .opcode    = NAND_RESET,
        .length    = NRF_QSPI_CINSTR_LEN_1B, /* 1 byte instruction */
        .io2_level = true,
        .io3_level = true,
        .wipwait   = false, /* disable wait operation as waiting is given separately */
        .wren      = false /* disable write */
    };

    /* perform qspi transfer operation with above custom command configuration */
    if(NRFX_SUCCESS != nrf_drv_qspi_cinstr_xfer(&cinstr_cfg, NULL, NULL)) {
        NRF_LOG_INFO("QSPI nrf driver error!");
        return NAND_NRF_DRIVER_ERROR;
    }
    /* minimum delay required for nand reset, as seen from data sheet */
    MCU_HAL_Delay(2);
    /* wait for reset to get over */
    ret = nand_flash_wait_till_ready();

    return ret;
}


/*!
  *********************************************************************************
  *@brief             Send the write enable command to the memory. This has to be
  *                   called before the program execute command.
  *@param[in]         None
  *@return[out]       NRF_SUCCESS (0) / NAND_NRF_DRIVER_ERROR (-1)
***********************************************************************************/
eNand_Result nand_flash_write_enable(void)  {
    eNand_Result ret = NAND_SUCCESS;
    nrf_qspi_cinstr_conf_t cinstr_cfg = {
         /* custom command instruction which calls with opcode write enable */
        .opcode    = NAND_WRITEENABLE,
        .length    = NRF_QSPI_CINSTR_LEN_1B, /* 1 byte instruction */
        .io2_level = true,
        .io3_level = true,
        .wipwait   = false,  /* disbale wait operation */
        .wren      = false   /* disable write enable as opcode is sent separately */
    };

    /* perform qspi transfer operation with above custom command configuration */
    if(NRFX_SUCCESS != nrf_drv_qspi_cinstr_xfer(&cinstr_cfg, NULL, NULL)) {
        NRF_LOG_INFO("QSPI nrf driver error!");
        return NAND_NRF_DRIVER_ERROR;
    }
    return ret;
}

/*!
  *********************************************************************************
  *@brief             Function to send the write disable command to the memory. As the
  *                   WEL bit is automatically cleared once the page execute command is
  *                   called, it is not necessary to use it.
  *@param[in]         None
  *@return[out]       NRF_SUCCESS (0) / NAND_NRF_DRIVER_ERROR (-1)
***********************************************************************************/
eNand_Result nand_flash_write_disable(void) {
    eNand_Result ret = NAND_SUCCESS;
    nrf_qspi_cinstr_conf_t cinstr_cfg = {
        /* custom command instruction which calls with opcode write disable */
        .opcode    = NAND_WRITEDISABLE,
        .length    = NRF_QSPI_CINSTR_LEN_1B,   /* 1 byte instruction */
        .io2_level = true,
        .io3_level = true,
        .wipwait   = false,  /* disable wait operation  */
        .wren      = false   /* write disable */
    };

     /* perform qspi transfer operation with above custom command configuration */
    if(NRFX_SUCCESS != nrf_drv_qspi_cinstr_xfer(&cinstr_cfg, NULL, NULL)) {
        NRF_LOG_INFO("QSPI nrf driver error!");
        return NAND_NRF_DRIVER_ERROR;
    }
    return ret;

}

/*!
  *********************************************************************************
  *@brief            Function to read the memory ID. This will return 2 bytes that
  *                  identify the memory.
  *@param[in]        *mfacture_id: Pointer to Manufacturing ID
                     *device_id: Pointer to device ID
  *@return[out]       NRF_SUCCESS (0) / NAND_NRF_DRIVER_ERROR (-1)
***********************************************************************************/
eNand_Result nand_flash_read_id(uint8_t *mfacture_id, uint8_t *device_id) {
    eNand_Result ret = NAND_SUCCESS;
    nrf_qspi_cinstr_conf_t cinstr_cfg = {
	/* custom command instruction which calls with opcode read ID */
        .opcode    = NAND_READID,
        .length    = NRF_QSPI_CINSTR_LEN_4B,/* 4 bytes instruction */
        .io2_level = true,
        .io3_level = true,
        .wipwait   = false,/* disable wait operation  */
        .wren      = false  /* disable write */
    };

    /* 3 bytes tx, 1 byte for mID , 1 byte for device ID , 1 byte to send opcode */
    uint8_t tx_data[3] = {0};
    uint8_t rx_data[3] = {0};

    tx_data[0] = 0;

    /* perform qspi transfer operation with above custom command configuration */
    if(NRFX_SUCCESS != nrf_drv_qspi_cinstr_xfer(&cinstr_cfg, tx_data, rx_data)) {
        NRF_LOG_INFO("QSPI nrf driver error!");
        return NAND_NRF_DRIVER_ERROR;
    }
    /* assign mID variable */
    if(NULL != mfacture_id) {
        *mfacture_id = rx_data[1];
    }
     /* assign device ID variable */
    if(NULL != device_id) {
        *device_id = rx_data[2];
    }
    return (ret);
}

/*!
  *********************************************************************************
  *@brief             Function to set one of the features of the memory. There are
  *                   bit helpers available on the header file that can be used to
  *                   set the value of the elements of each feature to be set.
  *@param[in]         eNand_Regs reg: The feature to set
                      value: The value to set to the feature
  *@return[out]       NRF_SUCCESS (0) / NAND_NRF_DRIVER_ERROR (-1)
***********************************************************************************/
eNand_Result nand_flash_set_feature(eNand_Regs regs, uint8_t value) {
	eNand_Result ret = NAND_SUCCESS;
	nrf_qspi_cinstr_conf_t cinstr_cfg = {
	/* custom command instruction which calls with opcode set register */
        .opcode    = NAND_SETREGS,
        .length    = NRF_QSPI_CINSTR_LEN_3B, /* 3 byte instruction */
        .io2_level = true,
        .io3_level = true,
        .wipwait   = false,/* disable wait operation  */
        .wren      = false /* disable write  */
    };

    /* 2 bytes tx, 1 byte register address to set, 1 byte register value to set */
    uint8_t tx_data[2] = {0};
    tx_data[0] = regs;
    tx_data[1] = value;

    /* perform qspi transfer operation with above custom command
     configuration */
    if(NRFX_SUCCESS != nrf_drv_qspi_cinstr_xfer(&cinstr_cfg, tx_data, NULL))  {
        NRF_LOG_INFO("QSPI nrf driver error!");
        return NAND_NRF_DRIVER_ERROR;
    }
    return ret;
}

/*!
  *********************************************************************************
  *@brief             Function to initialize qspi driver, reset nand flash controller,
                      reset flash register, This is first function to be called
                      before using flash
  *@param[in]         None
  *@return[out]       NRF_SUCCESS (0) / NAND_NRF_DRIVER_ERROR (-1)
***********************************************************************************/
eNand_Result nand_flash_init(void)  {
    eNand_Result ret = NAND_SUCCESS;
    uint8_t out_val;
    uint8_t facture_id,device_id;

    /* Initialize QSPI driver */
    if(NRFX_SUCCESS != nand_flash_qspi_init())  {
        NRF_LOG_INFO("QSPI nrf driver error!");
        return NAND_NRF_DRIVER_ERROR;
    }

    /* Reset flash controller */
    ret = nand_flash_reset();
    if(NAND_SUCCESS != ret) {
        NRF_LOG_INFO("NandFlash_Reset error");
        return ret;
    }

    /* clear registers, iterate till status register
    returns ready */
    do  {
        /* clear protection register of flash */
        ret = nand_flash_set_feature(NAND_REG_PROTECTION,0x00);
        if(NAND_SUCCESS != ret) {
            return ret;
        }

        /* clear feature 1 register */
        ret = nand_flash_set_feature(NAND_REG_FEATURE1,0x00);
        if(NAND_SUCCESS != ret) {
            return ret;
        }

        /* clear feature 2 register */
        ret = nand_flash_set_feature(NAND_REG_FEATURE2,0x00);
        if(NAND_SUCCESS != ret) {
            return ret;
        }

        /* read status register */
        ret = nand_flash_get_feature(NAND_REG_STATUS1,&out_val);
        if(NAND_SUCCESS != ret) {
            return ret;
        }
    }while((out_val&0x01)!=0);

    /* read manufacture ID, device ID */
    ret = nand_flash_read_id(&facture_id,&device_id);
    NRF_LOG_INFO("manufacture ID = 0x%x,Device ID = 0x%x",facture_id,device_id);
    return ret;
}

/*!
  *********************************************************************************
  *@brief            Function to obtain one of the features of the memory. There are
  *                  bit helpers available on the header file that can be used to
  *                  retrieve the value of one of the elements of each feature.
  *@param[in]        eNand_Regs: The feature to read from the memory
  *param[out]        outVal: The value read from the memory
  *@return[out]       NRF_SUCCESS (0) / NAND_NRF_DRIVER_ERROR (-1)
***********************************************************************************/
eNand_Result nand_flash_get_feature(eNand_Regs regs, uint8_t * outVal)  {
    eNand_Result ret = NAND_SUCCESS;
    nrf_qspi_cinstr_conf_t cinstr_cfg = {
	/* custom command instruction which calls with opcode get register */
        .opcode    = NAND_GETREGS,
        .length    = NRF_QSPI_CINSTR_LEN_3B,/* 3 byte instruction */
        .io2_level = true,
        .io3_level = true,
        .wipwait   = false,/* disable wait operation  */
        .wren      = false/* disable write  */
    };

     /* 2 bytes tx, 1 byte register address to read, 1 byte register value where its read */
    uint8_t tx_data[2] = {0};
    uint8_t rx_data[2] = {0};

    tx_data[0] = regs;

    /* perform qspi transfer operation with above custom command configuration */
    if(NRFX_SUCCESS != nrf_drv_qspi_cinstr_xfer(&cinstr_cfg, tx_data, rx_data)) {
        NRF_LOG_INFO("QSPI nrf driver error!");
        return NAND_NRF_DRIVER_ERROR;
    }

    if(NULL != outVal) {
        *outVal = rx_data[1];
    }
    return (ret);
}

/*!
  *********************************************************************************
  *@brief             Read the status register and return its value.
  *@param[in]         *status: Pointer to status register value
  *@return[out]       NRF_SUCCESS (0) / NAND_NRF_DRIVER_ERROR (-1)
***********************************************************************************/
eNand_Result nand_flash_get_status(uint8_t * status)  {
    return (nand_flash_get_feature(NAND_REG_STATUS1,status));
}

/*!
*********************************************************************************
  *@brief             Wait until the OIP element within the status register is cleared.
  *@param[in]         None
  *@return[out]       NRF_SUCCESS (0) / NAND_NRF_DRIVER_ERROR (-1)
***********************************************************************************/
eNand_Result nand_flash_wait_till_ready(void) {
    eNand_Result ret = NAND_SUCCESS;
    /* read 50 times about equal 1ms */
    uint8_t ulTimeout = 50;
    uint8_t ucStatus;
    uint8_t ecc_error;

    /* read status register */
    ret = nand_flash_get_status(&ucStatus);
    if(NAND_SUCCESS != ret) {
        NRF_LOG_INFO("read status return error.ret = %d",ret);
    }

    /* if busy and time out count is still left, or
     failure in reading status register, read status register
     again and decrement time out */
    while(((ucStatus&0x01)&&( ulTimeout != 0x00))||(NAND_SUCCESS != ret)) {
        ret = nand_flash_get_status(&ucStatus);
        ulTimeout--;
    }

    /* code added for debug to find  global minimum time out */
    if(TimeOutCount > ulTimeout){
      TimeOutCount = ulTimeout;
    }

    /* if status register shows error */
    if(ucStatus&0x7c) {
        /* program error */
        if(ucStatus&0x08) {
            NRF_LOG_INFO("program error.ucStatus = 0x%x",ucStatus);
            ret |= NAND_ECC_PROGRAM_ERROR;
        }
        /* erase error */
        if(ucStatus&0x04) {
            NRF_LOG_INFO("erase error.ucStatus = 0x%x",ucStatus);
            ret |= NAND_ECC_ERASE_ERROR;
        }
        /* ecc errors */
        if(ucStatus&0x70) {
            ecc_error = ((ucStatus&0x70)>>4);
            /* correctable errors */
            switch(ecc_error) {
                case 1: {
                    NRF_LOG_INFO("1~3 bit errors detected and \
                                corrected.ucStatus = 0x%x",ucStatus);

                }
                break;
                case 3:
                case 5: {
                    NRF_LOG_INFO("4~8 bit errors detected and \
                                corrected.ucStatus = 0x%x",ucStatus);
                    ret |= NAND_ECC_CORRECTABLE_ERROR;
                }
                break;
                /* not corrected errors */
                default:  {
                    NRF_LOG_INFO("Bit errors greater than 8 bits detected and\
                                not corrected.ucStatus = 0x%x",ucStatus);
                    ret |= NAND_ECC_UNCORRECTABLE_ERROR;
                }
                break;
            }

        }

    }

    /* time out elapses , return time out error */
    if(ulTimeout == 0x00) {
        NRF_LOG_INFO("time out.ucStatus = 0x%x",ucStatus);
        ret |= NAND_TIMEOUT_ERROR;
    }
    return ret;
}


/*!
*********************************************************************************
  *@brief             program load writes data from soft buffer to cache buffer
  *@param[in]         column: column address in page where it has to start writing.
                      size: write size
                      *in_array: Input array to write
  *@return[out]       NRF_SUCCESS (0) / NAND_NRF_DRIVER_ERROR (-1)
***********************************************************************************/
eNand_Result nand_flash_program_load(uint32_t column,  uint32_t size,\
                                    uint8_t * in_array) {
    eNand_Result ret = NAND_SUCCESS;
    uint32_t flash_address;

    /* size to write is within range of 512 bytes
    as QSPI supports max size of 512 bytes transfer ,
    this is due to nordic QSPI driver limitation between
    flash and NRF processor */
    if((size == 0)||(size > 512)) {
        return NAND_PARAM_ERROR;
    }

    /* size to write is supported with multiples of 4 */
    if((size&0x03) != 0)  {
        size = (int)(floor((size+3)/4))*4;
#ifdef PRINTS_OUT
        NRF_LOG_INFO("Size calculated=%d",size);
#endif
    }

    /* input array to write is valid and column
    address is valid */
    if((NULL == in_array)||((column&0x03) != 0))  {
        return NAND_PARAM_ERROR;
    }

    /* calculate column address to be written */
    flash_address = (column << 8);

     /* minimum size to write is 4 bytes */
    if(size > 4) {
#ifdef PROFILE_TIME_ENABLED
    if(size == 512){
    tick = get_micro_sec();
    }
#endif
        /* QSPI write with position array+4 is called as
        QSPI write loses first 4 bytes */
        if(NRFX_SUCCESS != nrf_drv_qspi_write(in_array+4,size-4,flash_address)) {
            NRF_LOG_INFO("QSPI nrf driver error!");
            return NAND_NRF_DRIVER_ERROR;
        }
    }
    else
    {
      /* Write some dummy data using qspi-write,
       so that susequent call for load-random works*/
      uint8_t dummy_data[4] = {0xFF,0xFF,0xFF,0xFF};

        if(NRFX_SUCCESS != nrf_drv_qspi_write(dummy_data,4,flash_address)) {
            NRF_LOG_INFO("QSPI nrf driver error!");
            return NAND_NRF_DRIVER_ERROR;
        }
    }
#ifdef PROFILE_TIME_ENABLED
    if(size == 512){

      qspi_512_bytes_write_time = get_micro_sec() - tick;
    //NRF_LOG_INFO("*********  Time taken for 512 bytes write = %d***************",qspi_512_bytes_write_time);
    }
    tick =  get_micro_sec();
#endif
    /* first 4 bytes are written on to cache with load random */
    ret = nand_flash_program_load_random(column,4,in_array);

#ifdef PROFILE_TIME_ENABLED
    program_load_random_api_time = get_micro_sec() - tick;
     //  NRF_LOG_INFO("*********  Time taken for 512 bytes program load random = %d***************",program_load_random_api_time);
#endif

    return ret;
}

  /*!
  *********************************************************************************
  *@brief             Write a byte array into the cache register without modifying the
  *                   data already present on the cache
  *@param[in]         column Byte within the cache to start writing
  *@param[in]         size Number of bytes to be written of the cache
  *@param[in]         in_array Array of bytes to be written
  *@return[out]       NRF_SUCCESS (0) / NAND_NRF_DRIVER_ERROR (-1)
***********************************************************************************/
eNand_Result nand_flash_program_load_random(uint32_t column,uint32_t size,\
                                          uint8_t * in_array) {
    eNand_Result ret = NAND_SUCCESS;
    nrf_qspi_cinstr_conf_t cinstr_cfg = {
        .opcode    = NAND_PROGRAMLOADRANDOMDATA, /* custom command instruction which calls
                                                  with opcode get random data */
        .length    = NRF_QSPI_CINSTR_LEN_3B, /* 3 byte instruction */
        .io2_level = true,
        .io3_level = true,
        .wipwait   = false,/* disable wait operation  */
        .wren      = false /* disable write  */
    };
    uint8_t tx_data[8] = {0};
    uint32_t offset=0;

    /* input buffer to write is valid */
    if(NULL == in_array)  {
        return NAND_PARAM_ERROR;
    }

    /* if number of bytes to write is greater than 6; its written in terms
    of 6 bytes */
    for(offset = 0;size - offset > 6;offset+=6) {
        cinstr_cfg.length = NRF_QSPI_CINSTR_LEN_9B;
        tx_data[0] = (uint8_t)(((column + offset)>>8)&0xFF);
        tx_data[1] = (uint8_t)((column + offset)&0xFF);
        memcpy(&tx_data[2],(in_array+offset),6);

        /* perform qspi transfer operation with above custom command configuration */
        if(NRFX_SUCCESS != nrf_drv_qspi_cinstr_xfer(&cinstr_cfg, tx_data, NULL))  {
            NRF_LOG_INFO("QSPI nrf driver error!");
            return NAND_NRF_DRIVER_ERROR;
        }
    }

    /* total length of instruction opcode length + number of bytes to write */
    cinstr_cfg.length = (nrf_qspi_cinstr_len_t )(NRF_QSPI_CINSTR_LEN_3B + (size - offset));
    /* 13 bit column address sent in terms of group of 8 bits */
    tx_data[0] = (uint8_t)(((column + offset)>>8)&0xFF);
    tx_data[1] = (uint8_t)((column + offset)&0xFF);
    memcpy(&tx_data[2],(in_array+offset),(size - offset));

    if(NRFX_SUCCESS != nrf_drv_qspi_cinstr_xfer(&cinstr_cfg, tx_data, NULL))  {
        NRF_LOG_INFO("QSPI nrf driver error!");
        return NAND_NRF_DRIVER_ERROR;
    }
    return (ret);
}

  /*!
  *********************************************************************************
  *@brief             Write the cache array to the memory. It will be written to the page
  *                   indicated by the argument.
  *@param[in]         page_index page index.It can be from
                      0 to 2047 blocks * 64 pages/block -1 = 26214 pages
  *@return[out]       NRF_SUCCESS (0) / NAND_NRF_DRIVER_ERROR (-1)
***********************************************************************************/
eNand_Result nand_flash_program_execute(uint32_t page_index)  {
    eNand_Result ret = NAND_SUCCESS;
    nrf_qspi_cinstr_conf_t cinstr_cfg = {
	/* custom command instruction which calls with opcode program execute */
        .opcode    = NAND_PROGRAMEXECUTE,
        .length    = NRF_QSPI_CINSTR_LEN_4B, /* 3 byte instruction */
        .io2_level = true,
        .io3_level = true,
        .wipwait   = false, /* disable wait operation  */
        .wren      = true /* enable write to write on to memory from cache */
    };

    uint8_t tx_data[3] = {0};
    /* 13 bit column address sent in group of 8 bits */
    tx_data[0] = (uint8_t)((page_index>>16)&0xFF);
    tx_data[1] = (uint8_t)((page_index>>8)&0xFF);
    tx_data[2] = (uint8_t)((page_index)&0xFF);

    /* perform qspi transfer operation with above custom command configuration */
    if(NRFX_SUCCESS != nrf_drv_qspi_cinstr_xfer(&cinstr_cfg, tx_data, NULL))  {
        NRF_LOG_INFO("QSPI nrf driver error!");
        return NAND_NRF_DRIVER_ERROR;
    }

    return (ret);
}

  /*!
  *********************************************************************************
  *@brief             Function to erase a block of the memory. The page/block address
  *                   has to be provided. This is the page index within the memory
  *@param[in]         page_block_index page index.It can be from
  *                   0 to 2047 blocks * 64 pages/block -1 = 262143
  *                   eg: 5 block has to be erased , (5*64) has to be sent
  *@return[out]       NRF_SUCCESS (0) / NAND_NRF_DRIVER_ERROR (-1)
***********************************************************************************/
eNand_Result nand_flash_block_erase(uint32_t page_block_index)  {
    eNand_Result ret = NAND_SUCCESS;
    uint8_t i = 0;
    nrf_qspi_cinstr_conf_t cinstr_cfg = {
	    /* custom command instruction which calls with opcode block erase */
        .opcode    = NAND_BLOCKERASE,
        .length    = NRF_QSPI_CINSTR_LEN_4B, /* 4 byte instruction */
        .io2_level = true,
        .io3_level = true,
        .wipwait   = false, /* disable wait operation  */
        .wren      = true /* enable write for block erase */
    };
    uint8_t tx_data[3] = {0};
    /* 13  bit column address of page index within block which
    has to be erased */
    tx_data[0] = (page_block_index>>16)&0xFF;
    tx_data[1] = (page_block_index>>8)&0xFF;
    tx_data[2] = (page_block_index)&0xC0;

    if(NRFX_SUCCESS != nrf_drv_qspi_cinstr_xfer(&cinstr_cfg, tx_data, NULL))  {
        NRF_LOG_INFO("QSPI nrf driver error!");
        return NAND_NRF_DRIVER_ERROR;
    }

    /* wait time for erase */
    do{
        /* mimimum of 2 milli sec required to erase block */
        MCU_HAL_Delay(2);
        /* wait for block erase over bit to get set in status register */
        ret = nand_flash_wait_till_ready();
      /* typical value for block erase is 2ms,Max is 10ms */
    }while((NAND_TIMEOUT_ERROR == ret)&&(i++<4));

    return (ret);
}

  /*!
  *********************************************************************************
  *@brief             read from cache line is custom command feature to read maximum of
                      5 bytes from memory
  *@param[in]         column: column address in page where it has to start reading.
                      size: read size
                      *out_array: Output array to be read
  *@return[out]       NRF_SUCCESS (0) / NAND_NRF_DRIVER_ERROR (-1)
***********************************************************************************/
static eNand_Result nand_flash_read_from_cache_one_line(uint32_t column,  \
                  uint8_t size, uint8_t * out_array)  {
	eNand_Result ret = NAND_SUCCESS;
	nrf_qspi_cinstr_conf_t cinstr_cfg = {
	/* custom command instruction which calls with opcode read from cache */
        .opcode    = NAND_READFROMCACHE,
        .length    = NRF_QSPI_CINSTR_LEN_4B, /* 4 byte instruction */
        .io2_level = true,
        .io3_level = true,
        .wipwait   = false,/* disable wait operation  */
        .wren      = false /* disable write  */
    };
    uint8_t tx_data[8] = {0};
    uint8_t rx_data[8] = {0};

    /* output array to have memory allocated */
    if(NULL == out_array) {
        return NAND_PARAM_ERROR;
    }

    /* custom command supports max of 5 bytes transfer */
    if(size > 5)  {
        size = 5;
    }
    cinstr_cfg.length += (size);

    /* 13 bit column address split and sent */
    tx_data[0] = (column >> 8)&0xff;
    tx_data[1] = (column)&0xff;
    tx_data[2] = 0;

    /* read into rx_data */
    if(NRFX_SUCCESS != nrf_drv_qspi_cinstr_xfer(&cinstr_cfg, tx_data, rx_data)) {
        NRF_LOG_INFO("QSPI nrf driver error!");
        return NAND_NRF_DRIVER_ERROR;
    }
    /* copy to out array */
    memcpy(out_array,&rx_data[3],size);
    return (ret);
}

  /*!
  *********************************************************************************
  *@brief             Read information from the cache register
  * @param[in]        column Byte within the cache to start reading
  * @param[in]        size Number of bytes to be read of the cache
  * @param[out]       out_array Data read from the cache.
  *@return[out]       NRF_SUCCESS (0) / NAND_NRF_DRIVER_ERROR (-1)
***********************************************************************************/
eNand_Result nand_flash_read_from_cache (uint32_t column,  uint32_t size, \
                                        uint8_t * out_array)  {
    eNand_Result ret = NAND_SUCCESS;
    uint32_t flash_address;

    /* if size to read is 0, return failure */
    if(size == 0) {
        return NAND_PARAM_ERROR;
    }
    /* size to read to be in multiples of '4' */
    if((size & 0x03) != 0)  {
      size = (int)(floor((size+3)/4))*4;
#ifdef PRINTS_OUT
      NRF_LOG_INFO("Size calculated=%d",size);
#endif
    }

    /* column address should be in multiples of '4',
    out array to be valid where memory is allocated */
    if((NULL == out_array)||((column&0x03) != 0)) {
        return NAND_PARAM_ERROR;
    }

    /* if column address >= 4 */
    if(column >= 4) {
        column -= 4;
        flash_address = (column << 8);
        /* perfom qspi read */
        if(NRFX_SUCCESS != nrf_drv_qspi_read(out_array,size,flash_address)) {
            NRF_LOG_INFO("QSPI nrf driver error!");
            return NAND_NRF_DRIVER_ERROR;
        }
    } else {
        /* read line by line first 4 bytes */
        if(NRFX_SUCCESS != nand_flash_read_from_cache_one_line(column,4 - column,out_array))  {
            NRF_LOG_INFO("QSPI nrf driver error!");
            return NAND_NRF_DRIVER_ERROR;
        }
        flash_address = (column << 8);
        if(NRFX_SUCCESS != nrf_drv_qspi_read(out_array+(4-column),size-4+column,flash_address)) {
            NRF_LOG_INFO("QSPI nrf driver error!");
            return NAND_NRF_DRIVER_ERROR;
        }
    }

    return ret;
}

/*!
*********************************************************************************
  *@brief             Read one page of the memory given its index.
  *@param[in]         page_index page index.It can be from
  *                   0 to 2047 blocks * 64 pages/block -1 = 262143
  *@return[out]       NRF_SUCCESS (0) / NAND_NRF_DRIVER_ERROR (-1)
***********************************************************************************/
eNand_Result nand_flash_page_read(uint32_t page_index)  {
    eNand_Result ret = NAND_SUCCESS;
    nrf_qspi_cinstr_conf_t cinstr_cfg = {
	 /* custom command instruction which calls with opcode page read to cache */
        .opcode    = NAND_PAGEREADTOCACHE,
        .length    = NRF_QSPI_CINSTR_LEN_4B, /* 4 byte instruction */
        .io2_level = true,
        .io3_level = true,
        .wipwait   = false, /* disable wait operation */
        .wren      = false  /* disable write  */
    };

    /* 3 bytes tx, page index sent , split in terms of 8 bytes */
    uint8_t tx_data[3] = {0};
    tx_data[0] = (page_index>>16)&0xFF;
    tx_data[1] = (page_index>>8)&0xFF;
    tx_data[2] = (page_index)&0xFF;

    /* perform qspi transfer operation with above custom command configuration */
    if(NRFX_SUCCESS != nrf_drv_qspi_cinstr_xfer(&cinstr_cfg, tx_data, NULL))  {
        NRF_LOG_INFO("QSPI nrf driver error!");
        return NAND_NRF_DRIVER_ERROR;
    }

    /* wait for read to get over ;
    read time is 25us(disable ecc),135us(enable ecc) */
    ret = nand_flash_wait_till_ready();

    /* if correctable error */
    if(ret & NAND_ECC_CORRECTABLE_ERROR)  {
        /* perform program execute */
        if(nand_flash_program_execute(page_index) == NAND_SUCCESS)  {
            /* if success, mask correctable error and send return code */
            ret &= (~NAND_ECC_CORRECTABLE_ERROR);
        }
    }
    return ret;
}
#endif
