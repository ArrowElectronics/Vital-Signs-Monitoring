/**
***************************************************************************
* @file         hal_i2c.c
* @author       ADI
* @version
* @date         26-Jun-2019
* @brief        Hardware abstraction layer for I2C communiction with 
*               ADI ADPD400x chip
*
***************************************************************************
* @attention
***************************************************************************
*/
/******************************************************************************
*                                                                             *
* License Agreement                                                           *
*                                                                             *
* Copyright (c) 2020 Analog Devices Inc.                                      *
* All rights reserved.                                                        *
*                                                                             *
* This source code is intended for the recipient only under the guidelines of *
* the non-disclosure agreement with Analog Devices Inc.                       *
*                                                                             *
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR  *
* IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,    *
* FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE *
* AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER      *
* LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING     *
* FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER         *
* DEALINGS IN THE SOFTWARE.                                                   *
*                                                                             *
*                                                                             *
* This software is intended for use with the ADPD400x part                    *
* only                                                                        *
*                                                                             *
******************************************************************************/

/* Includes -----------------------------------------------------------------*/
#include "boards.h"
#include "nrf.h"
#include "nrf_drv_gpiote.h"
#include "nrf_drv_uart.h"
#include "sdk_config.h"
#include "FreeRTOS.h"
#include "task.h"
#include "timers.h"
#include "hw_if_config.h"
/* Private typedef ----------------------------------------------------------*/

/* Private define ------------------------------------------------------------*/
#define ADPD_I2C_ADDRESS    0x64 //!< ADPD4100 device address

/* Private macro -------------------------------------------------------------*/


/* TWI instance ID. */
#define TWI_INSTANCE_ID     0 //!< I2C instance number




static const nrf_drv_twi_t m_twi = NRF_DRV_TWI_INSTANCE(TWI_INSTANCE_ID); //!< I2C structure to hold instance
uint16_t data1; //!< used to hold the 2 bytes
uint8_t m_sample[2]; //!< array to send the two bytes 
volatile uint8_t gTxCompleteFlag =0; //!< Flags for checking Tx process completion
volatile uint8_t gRxCompleteFlag = 0; //!< Flags for checking Rx process completion
/**
 * @brief Data handler.
 * @param temp
 * @return none
 */
__STATIC_INLINE void data_handler(uint8_t *temp)
{
  data1 = temp[0] <<8 | temp[1];
}

/**
 * @brief TWI events handler.
 * @param p_event
 * @param p_context
 * @return none
 */
void twi_handler(nrf_drv_twi_evt_t const * p_event, void * p_context)
{
    switch (p_event->type)
    {
        case NRF_DRV_TWI_EVT_DONE:
            if (p_event->xfer_desc.type == NRF_DRV_TWI_XFER_RX)
            {
                data_handler(m_sample);
                gRxCompleteFlag = 1;
            }
            if (p_event->xfer_desc.type == NRF_DRV_TWI_XFER_TX)
            {
//                data_handler(m_sample);
                gTxCompleteFlag = 1;
            }
        break;
        default:
            break;
    }
}

/* ************************************************************************* */
/**
    * @brief    This function initializes all the I2C controllers used.
    * @retval   None
    */
void twi_init (void)
{
   // ret_code_t err_code;

    const nrf_drv_twi_config_t twi_adpd1080_config = {
       .scl                = ARDUINO_SCL_PIN,
       .sda                = ARDUINO_SDA_PIN,
       .frequency          = NRF_DRV_TWI_FREQ_400K,
       .interrupt_priority = APP_IRQ_PRIORITY_HIGH,
       .clear_bus_init     = false
    };

    nrf_drv_twi_init(&m_twi, &twi_adpd1080_config, twi_handler, NULL);
    nrf_drv_twi_enable(&m_twi);
}


  uint8_t reg[3]; //!< array to hold addr/data for register write

/**
  * @brief Function implemented in such a way that it transmits the data from
  *        txData buffer.
  * @param  adr is the register address of the ADPD400x device.
  * @param  data is the two bytes be written to the device register
  * @retval nStatus of type int:
            ADI_HAL_OK-> Success
            ADI_HAL_ERROR -> Failure
  */
int I2c_Reg_write(uint16_t adr,uint16_t data)
{
    ret_code_t err_code;
    reg[0] =(uint8_t) adr;
    reg[1] = (uint8_t) ((data >> 8) & 0xff);
    reg[2] = (uint8_t) (data&0x00FF);
    err_code = nrf_drv_twi_tx(&m_twi, ADPD_I2C_ADDRESS, (uint8_t*)reg, 3, false);
    while(!gTxCompleteFlag);
      gTxCompleteFlag=0;
    return err_code;
}

/**
  * @brief Function that transmits the reg address from the 1st argument and
  * receives data specified by reg addr at pointer specified by 2nd argument
  * @param  nAddr to the array containing ADPD4x00 register addr to be transmitted
  * @param  pnData is the pointer to the buffer of received data.
  * @retval nStatus of type int:
            ADI_HAL_OK-> Success
            ADI_HAL_ERROR -> Failure
  */
int I2c_Reg_read(uint16_t nAddr, uint16_t *pnData)
{

      ret_code_t ret;
      uint8_t anRxData[2];
      ret= nrf_drv_twi_tx(&m_twi, ADPD_I2C_ADDRESS, (uint8_t *)&nAddr, 1,false );
       if (NRF_SUCCESS != ret )
       {
          return 1;
       }
      while(!gTxCompleteFlag);
      gTxCompleteFlag = 0;

       nrf_drv_twi_rx(&m_twi, ADPD_I2C_ADDRESS, (uint8_t*)anRxData, 2);
      while(!gRxCompleteFlag);
      gRxCompleteFlag =0;
       *pnData = (uint16_t)anRxData[0]  << 8 | anRxData[1];
      return 0;

}

/**
  * @brief Create task delay for the specified time in millisec from the caller
  * @param  ms_time Delay in millisec to be created
  * @retval none.
  */
void MCU_HAL_Delay(uint32_t ms_time)
{
  HAL_delay_ms(ms_time);
}

/**
  * @brief Create task delay for the specified time in millisec from the caller
  * @param  ms_time Delay in millisec to be created
  * @retval none.
  */
void HAL_delay_ms(uint32_t ms_time)
{
  vTaskDelay(ms_time);
}
#if 0
ADI_HAL_STATUS_t ADPD_I2C_Transmit(uint8_t *pData, uint16_t Size) {
   uint16_t nAddr,pnData;
   nAddr = (uint8_t)(pData[0]);
    pnData = (uint16_t)pData[1]  << 8 | pData[2];
   if (!(I2c_Reg_write(nAddr,pnData))) {
          return ADI_HAL_OK;
    }

    return ADI_HAL_ERROR;
}

ADI_HAL_STATUS_t ADPD_I2C_TxRx(uint8_t *Addr, uint8_t *pRxData,
                               uint16_t RxSize)
{
      uint16_t pnData,Rxaddr;
      Rxaddr = (uint8_t) (Addr[0]);
      if (!(I2c_Reg_read(Rxaddr,&pnData))) {
          pRxData[0] = (uint8_t) ((pnData >> 8) & 0xff);
          pRxData[1] = (uint8_t) (pnData&0x00FF);
          return ADI_HAL_OK;
    }

    return ADI_HAL_ERROR;

}
#endif

/**
  * @brief Function implemented in such a way that it transmits the data from
  *        txData buffer of size specified in the second argument.
  * @param  pData is the pointer to the buffer of the size of three bytes in
  *               which first byte is the register address of the ADPD400x device.
  *               The second and the third bytes are the 16 bits data value to be
  *               written to the device register
  * @param  Size is the size of the buffer in bytes (3 bytes).
  * @retval nStatus of type ADI_HAL_STATUS_t:
            ADI_HAL_OK-> Success
            ADI_HAL_ERROR -> Failure
  */
ADI_HAL_STATUS_t Adpd400x_I2C_Transmit(uint8_t *pData, uint16_t Size) {
   uint16_t nAddr,pnData;
   nAddr = (uint8_t)(pData[0]);
    pnData = (uint16_t)pData[1]  << 8 | pData[2];
   if (!(I2c_Reg_write(nAddr,pnData))) {
          return ADI_HAL_OK;
    }

    return ADI_HAL_ERROR;
}

/**
  * @brief Function that transmits the reg address from the 1st argument and
  * receives data specified by reg addr at pointer specified by 2nd argument
  * The transmitted data will be of size specified by 3rd argument.
  * The received data will be of size specified by 4th argument.
  * @param  Addr pointer to array containing ADPD4x00 register addr to be transmitted
  * @param  pRxData is the pointer to the buffer of received data.
  * @param  TxSize is the size of transmit data in bytes.
  * @param  RxSize is the size of requested data in bytes.
  * @retval nStatus of type ADI_HAL_STATUS_t:
            ADI_HAL_OK-> Success
            ADI_HAL_ERROR -> Failure
  */
ADI_HAL_STATUS_t Adpd400x_I2C_TxRx(uint8_t *Addr, uint8_t *pRxData, uint16_t TxSize,
                               uint16_t RxSize)
{
      uint16_t pnData,Rxaddr;
      Rxaddr = (uint8_t) (Addr[0]);
      if (!(I2c_Reg_read(Rxaddr,&pnData))) {
          pRxData[0] = (uint8_t) ((pnData >> 8) & 0xff);
          pRxData[1] = (uint8_t) (pnData&0x00FF);
          return ADI_HAL_OK;
    }

    return ADI_HAL_ERROR;

}
