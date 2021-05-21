#include "spi_nand_flash.h"
#include "nrf_drv_qspi.h"
#include "watch_board_evt_pin_config.h"
#include "nrf_delay.h"

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
static void qspi_init(void)
{
    NAND_RESULT err_code;
    nrf_drv_qspi_config_t config = QSPI_NAND_FLASH_CONFIG;

    err_code = nrf_drv_qspi_init(&config, NULL, NULL);
    if(NRFX_SUCCESS != err_code)
    {
        NRF_LOG_INFO("QSPI init error,err_code=%d",err_code);
    }
    nrf_qspi_ifconfig0_set_page_size(1);
}

NAND_RESULT NandFlash_Reset(void)
{
    NAND_RESULT ret;
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
    vTaskDelay(2);
#endif
    ret = NandFlash_WaitTilLReady();

    return ret;
}


NAND_RESULT NandFlash_WRITE_ENABLE(void)
{
    NAND_RESULT ret;
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

NAND_RESULT NandFlash_WRITE_DISABLE(void)
{
	NAND_RESULT ret;
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

NAND_RESULT NandFlash_SetFeature(NAND_REGS regs, uint8_t value)
{

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
    nrf_drv_qspi_cinstr_xfer(&cinstr_cfg, tx_data, NULL);
}


void NandFlash_Init(void)
{
    NAND_RESULT ret;
    uint8_t out_val;
    uint8_t facture_id,device_id;
    qspi_init();
    
 
    do
    {
#ifdef BLOCKING_CALL
    nrf_delay_ms(5);
#else
    vTaskDelay(5);
#endif
        NandFlash_ReadID(&facture_id,&device_id);
        NRF_LOG_INFO("manufacture ID = 0x%x,Device ID = 0x%x",facture_id,device_id);
    }while((facture_id != FLASH_MANUFACTURE_ID)||(FLASH_DEVICE_ID != device_id));
    NandFlash_SetFeature(NAND_REG_FEATURE1,0x00);

    if(NRFX_SUCCESS != NandFlash_Reset())
    {
        NRF_LOG_INFO("NandFlash_Reset error");
    }
    do
    {
        NandFlash_SetFeature(NAND_REG_PROTECTION,0x00);
        NandFlash_SetFeature(NAND_REG_FEATURE1,0x10);
        NandFlash_SetFeature(NAND_REG_FEATURE2,0x00);
        NandFlash_GetFeature(NAND_REG_STATUS1,&out_val);
    }
    while((out_val&0x01)!=0);
    
    uint8_t value1,value2;
    NandFlash_ReadID(&value1,&value2);
    NRF_LOG_INFO("manufacture ID = 0x%x,Device ID = 0x%x",value1,value2);
}


NAND_RESULT NandFlash_GetFeature(NAND_REGS regs, uint8_t * outVal)
{
	NAND_RESULT ret;
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

NAND_RESULT NandFlash_ReadID(uint8_t *mfacture_id,uint8_t *device_id)
{
    NAND_RESULT ret;
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
NAND_RESULT NandFlash_GetStatus(uint8_t * status)
{
    return (NandFlash_GetFeature(NAND_REG_STATUS1,status));	
}

NAND_RESULT NandFlash_WaitTilLReady(void)
{
    NAND_RESULT ret;
    uint32_t ulTimeout = 0x20;
    uint8_t ucStatus;
    
    ret = NandFlash_GetStatus(&ucStatus);
    if(NRFX_SUCCESS != ret)
    {
        NRF_LOG_INFO("read status return error.ret = %d",ret);
    }

    while(((ucStatus&0x7d)&&( ulTimeout != 0x00))||(NRFX_SUCCESS != ret))
    {
//        NRF_LOG_INFO("read status.time = %d,ucStatus = 0x%x",ulTimeout,ucStatus);
//        nrf_delay_ms(1);
        ret = NandFlash_GetStatus(&ucStatus);
        ulTimeout--;
        
    }

    if(ulTimeout == 0x00)
    {
        NRF_LOG_INFO("time out error.ucStatus = 0x%x",ucStatus);
        return NRFX_ERROR_TIMEOUT;
    }
    return ret;
}
NAND_RESULT NandFlash_ProgramLoad(uint32_t column,  uint32_t size, uint8_t * inArray)
{
    NAND_RESULT ret;
    uint32_t flash_address;
    if(((size&0x03) != 0)||(size == 0)||(size > 512))
    {
        return NRFX_ERROR_INVALID_LENGTH;
    }
    if((NULL == inArray)||((column&0x03) != 0))
    {
        return NRFX_ERROR_INVALID_PARAM;
    }
    flash_address = (column << 8);
    if(size >= 4)
    {
        ret = nrf_drv_qspi_write(inArray+4,size-4,flash_address);
        if(NRFX_SUCCESS != ret)
        {
            NRF_LOG_INFO("nrf_drv_qspi_write error,ret=%d",ret);
            return (ret);
        }
    }

    return (NandFlash_ProgramLoadRandom(column,4,inArray));
}

NAND_RESULT NandFlash_ProgramLoadRandom(uint32_t column,uint32_t size,uint8_t * inArray)
{
    NAND_RESULT ret;
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
        memcpy(&tx_data[2],(inArray+offset),6);
        ret = nrf_drv_qspi_cinstr_xfer(&cinstr_cfg, tx_data, NULL);
        if(NRFX_SUCCESS != ret)
        {
            NRF_LOG_INFO("nrf_drv_qspi_cinstr_xfer error,ret=%d",ret);
            return (ret);
        }
    }

    cinstr_cfg.length = NRF_QSPI_CINSTR_LEN_3B + (size - offset);
    tx_data[0] = (uint8_t)(((column + offset)>>8)&0xFF);
    tx_data[1] = (uint8_t)((column + offset)&0xFF);
    memcpy(&tx_data[2],(inArray+offset),(size - offset));    
    return (nrf_drv_qspi_cinstr_xfer(&cinstr_cfg, tx_data, NULL));
}

NAND_RESULT NandFlash_ProgramExecute(uint32_t pageIndex)
{
    NAND_RESULT ret;

    nrf_qspi_cinstr_conf_t cinstr_cfg = {
        .opcode    = NAND_PROGRAMEXECUTE,
        .length    = NRF_QSPI_CINSTR_LEN_4B,
        .io2_level = true,
        .io3_level = true,
        .wipwait   = false,
        .wren      = false
    };
    uint8_t tx_data[3] = {0};
    tx_data[0] = (uint8_t)((pageIndex>>16)&0xFF);
    tx_data[1] = (uint8_t)((pageIndex>>8)&0xFF);
    tx_data[2] = (uint8_t)((pageIndex)&0xFF);
    ret = nrf_drv_qspi_cinstr_xfer(&cinstr_cfg, tx_data, NULL);
    if(NRFX_SUCCESS != ret)
    {
        NRF_LOG_INFO("ProgramExecute error,ret=%d",ret);
        return (ret);
    }
#ifdef BLOCKING_CALL
    nrf_delay_ms(1);
#else
    vTaskDelay(1);
#endif
    ret = NandFlash_WaitTilLReady();    
    return (ret);
}



NAND_RESULT NandFlash_BlockErase(uint32_t _ulBlockNo)
{
    NAND_RESULT ret;
    
    nrf_qspi_cinstr_conf_t cinstr_cfg = {
        .opcode    = NAND_BLOCKERASE,
        .length    = NRF_QSPI_CINSTR_LEN_4B,
        .io2_level = true,
        .io3_level = true,
        .wipwait   = false,
        .wren      = true
    };
    uint8_t tx_data[3] = {0};
    tx_data[0] = (_ulBlockNo>>16)&0xFF;
    tx_data[1] = (_ulBlockNo>>8)&0xFF;
    tx_data[2] = (_ulBlockNo)&0xC0;
    ret = nrf_drv_qspi_cinstr_xfer(&cinstr_cfg, tx_data, NULL);
    if(NRFX_SUCCESS != ret)
    {
        NRF_LOG_INFO("EraseBlock error,ret=%d",ret);
        return (ret);
    }
#ifdef BLOCKING_CALL
    nrf_delay_ms(3);
#else
    vTaskDelay(3);
#endif
    ret = NandFlash_WaitTilLReady();

    return (ret);
}

static NAND_RESULT NandFlash_ReadFromCache_oneLine(uint32_t column,  uint8_t size, uint8_t * outArray)
{
	NAND_RESULT ret;
	nrf_qspi_cinstr_conf_t cinstr_cfg = {
        .opcode    = NAND_READFROMCATCH,
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

NAND_RESULT NandFlash_ReadFromCache (uint32_t column,  uint32_t size, uint8_t * outArray)
{
    NAND_RESULT ret;
    uint32_t flash_address;
    
    if(((size&0x03) != 0)||(size == 0))
    {
        return NRFX_ERROR_INVALID_LENGTH;
    }
    if((NULL == outArray)||((column&0x03) != 0))
    {
        return NRFX_ERROR_INVALID_PARAM;
    }
    
    if(column >= 4)
    {
        column -= 4;
        flash_address = (column << 8);
        ret = nrf_drv_qspi_read(outArray,size,flash_address);
    }
    else
    {
        NandFlash_ReadFromCache_oneLine(column,4 - column,outArray);
        flash_address = (column << 8);
        ret = nrf_drv_qspi_read(outArray+(4-column),size-4+column,flash_address);
    }
    
    return ret;
}

NAND_RESULT NandFlash_PageRead(uint32_t pageIndex)
{
    NAND_RESULT ret;
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
    tx_data[0] = (pageIndex>>16)&0xFF;
    tx_data[1] = (pageIndex>>8)&0xFF;
    tx_data[2] = (pageIndex)&0xFF;
    do{
        nrf_drv_qspi_cinstr_xfer(&cinstr_cfg, tx_data, NULL);

        ret = NandFlash_WaitTilLReady();
    }while((NRFX_SUCCESS != ret)&&(i++<5));
    return ret;
}

#if 1  /**test code**/
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"

#define COMPARE_LEN (4352)
//char test_tx[4096] = {"HelloIDT"};
char test_tx[4352] = {"12345678901234567890"};
char test_tx1[4352] = {"12345678901234567890"};
char test_tx2[4352] = {"aaaaaaaaaaaaaaaaaaaaaaaa"};
char test_rx[4352] = {0};
uint32_t flash_address = 0;
uint32_t page_address = 0;

NAND_RESULT SPI_NAND_WritePage(uint8_t *_pBuffer, uint32_t _ulPageNo, uint32_t _usAddrInPage, uint32_t _usByteCount)
{    
    NAND_RESULT ret;
   
    NandFlash_WRITE_ENABLE();        
    ret = NandFlash_ProgramLoad(_usAddrInPage,_usByteCount,_pBuffer);
    if(NRFX_SUCCESS != ret)
    {
        NRF_LOG_INFO("ProgramLoad error,ret=%d",ret);
        return (ret);
    }
    ret = NandFlash_ProgramExecute(_ulPageNo);  
    return (ret);
}

NAND_RESULT SPI_NAND_ReadPage(uint8_t *_pBuffer, uint32_t _ulPageNo, uint16_t _usAddrInPage, uint16_t _usByteCount)
{
    NAND_RESULT ret;


    ret = NandFlash_PageRead(_ulPageNo);
 
    if(ret == NRFX_SUCCESS)   
    {
        ret = NandFlash_ReadFromCache( _usAddrInPage, _usByteCount,_pBuffer);
    }  	
    return (ret);
}

void NandFlash_send_data(void)
{
    NAND_RESULT ret = NRFX_SUCCESS;

    flash_address += 64;
    if(flash_address%64 == 0)
    {
        ret = NandFlash_BlockErase(flash_address);
        if(NRFX_SUCCESS == ret)
        {
            NRF_LOG_INFO("BlockErase success");
        }
        else
        {
            NRF_LOG_INFO("BlockErase error=%d,address = %x",ret,flash_address);
        }

    }
    if((flash_address == 64)||(flash_address == 192))
    {
        for(uint16_t i = 0;i<272; i++)
        {
            memcpy(&test_tx[16*i],test_tx1,16);
        }
    }
    else
    {
        for(uint16_t i = 0;i<272; i++)
        {
            memcpy(&test_tx[16*i],test_tx2,16);
        }
    }
   
    test_tx[511] = 0x02;
    test_tx[4096+128] = 'a';
    page_address = flash_address;

// 64 to 74 page write
  for(uint8_t j=0;j<10;j++)
  {
      NRF_LOG_INFO("****** Page Ind being written = %d *************",flash_address+j);
      for(uint8_t i=0;i<8;i++)
      {
          ret = SPI_NAND_WritePage(&test_tx[512*i],flash_address+j,512*i, 512);
      }
      ret = SPI_NAND_WritePage(&test_tx[4096],flash_address+j,512*8,256);
      if(NRFX_SUCCESS == ret)
      {
  //        NRF_LOG_INFO("test_tx success");
      }
      else
      {
          NRF_LOG_INFO("write error=%d",ret);
     }
  }
}
/**     @brief  Enable / Disable the internal memory ECC
  *             
  *     @param[in]      enable true = enable, false = disable
  *     @return Result of the function - Error or success
  */
void nand_func_enable_ecc(bool enable)
{
  NAND_RESULT result = NRFX_SUCCESS;
  uint8_t OTPValue;

  result = NandFlash_GetFeature(NAND_REG_FEATURE1, &OTPValue);
  if(enable)
  {
    result = NandFlash_SetFeature(NAND_REG_FEATURE1, OTPValue | FEAT_OTP_ECCEN);
  }
  else
  {
    result |= NandFlash_SetFeature(NAND_REG_FEATURE1, OTPValue & ~(FEAT_OTP_ECCEN));
  } 
}

void NandFlash_receive_data(void)
{
    
  for(uint8_t j=0;j<10;j++)
  {
     NRF_LOG_INFO("Page address=%d",page_address+j);
    if(NRFX_SUCCESS == SPI_NAND_ReadPage(test_rx,page_address+j, 0,4096+256))
    {
        for(uint16_t i = 0;i<COMPARE_LEN;i++)
        {
            if(test_rx[i] != test_tx[i])
            {
                NRF_LOG_INFO("not same, error!!,i = %d",i);
                break;
            }
        }
 
        NRF_LOG_HEXDUMP_INFO(test_rx,40);
        NRF_LOG_HEXDUMP_INFO(&test_rx[4096+128],40);
//        NRF_LOG_INFO("test_rx[512]=%c",test_rx[512]);
    }
  }  
}

uint16_t Init=0;
uint8_t i=0;
void flash_test_thread(void * arg)
{
    ret_code_t err_code;

    UNUSED_PARAMETER(arg);

    if(!Init){
      NandFlash_Init();
      nand_func_enable_ecc(false);
      Init = 1;
    }
    while(1)
//    while(i < 1)
    {
        vTaskDelay(9000);
        NandFlash_send_data();
        vTaskDelay(1000);
        NandFlash_receive_data();
        i++;
        vTaskDelay(60000);
    //        NRF_LOG_INFO("NRF_DRV_RTC_INT_TICK rtc_cnt = %d",rtc_cnt);  
    
    }
}
#define test_STACK_SIZE   512
#define test_PRIORITY   7
static TaskHandle_t m_rtc_thread;        /**< USB stack thread. */
void flash_init_test(void)
{
    
    if (pdPASS != xTaskCreate(flash_test_thread,"flash",test_STACK_SIZE, NULL,test_PRIORITY,&m_rtc_thread))
    {
        APP_ERROR_HANDLER(NRF_ERROR_NO_MEM);
    }
    else
    {
//        NRF_LOG_INFO("test_init create success");
    }
}
#endif



