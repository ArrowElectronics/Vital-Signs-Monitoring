/**
***************************************************************************
* @file         hal_spi.c
* @author       ADI
* @version
* @date         26-Jun-2019
* @brief        Hardware abstraction layer for SPI communiction with
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

#include "hw_if_config.h"
#include "nrf_drv_spi.h"

#ifdef VSM_MBOARD
#include "watch_vsm_motherboard_pin_config.h"
#elif PCBA
#include "watch_board_pcba_pin_config.h"
#else
#include "watch_board_evt_pin_config.h"
#endif

#ifdef EVTBOARD

#define SPI_INSTANCE  2 //!< SPI instance index
static const nrf_drv_spi_t spi2 = NRF_DRV_SPI_INSTANCE(SPI_INSTANCE);  //!< SPI instance
static volatile bool spi_xfer_done;  //!< Flag used to indicate that SPI instance completed the transfer
ADI_OSAL_MUTEX_HANDLE SPI2TransferLock; //!< Handle of mutex to be acquired
ADI_OSAL_SEM_HANDLE Spi2TxrComplete; //!< Handle of semaphore to be posted

static void SPI2_Init(void);
static void SPI2_UnInit(void);
/**
 * @brief SPI user event handler.
 * @param p_event
 * @param p_context
 * @retval none
 */
void spi2_event_handler(nrf_drv_spi_evt_t const * p_event,
                       void *                    p_context)
{
    if(p_event->type == NRF_DRV_SPI_EVENT_DONE)
    {
      spi_xfer_done = true;
      //SPI2_UnInit();
      adi_osal_SemPost(Spi2TxrComplete);
    }
//    NRF_LOG_INFO("Transfer completed.");
//    if (m_rx_buf[0] != 0)
//    {
//        NRF_LOG_INFO(" Received:");
//        NRF_LOG_HEXDUMP_INFO(m_rx_buf, strlen((const char *)m_rx_buf));
//    }
}


/**
  * @brief Function implemented in such a way that it transmits
           the data from txData buffer of size specified in second argument.
  * @param  pData pointer to the buffer of the size of three bytes in which
  *         first byte is the register address of the ADXL362 device. The
  *         second and the third bytes are the 16 bits data value to be written
  *         to the device register
  * @param  Size size of the buffer in bytes (3 bytes).
  * @retval nStatus of type ADI_HAL_STATUS_t:
            ADI_HAL_OK-> Success
            ADI_HAL_ERROR -> Failure
  */
ADI_HAL_STATUS_t ADXL362_SPI_Transmit(uint8_t *pData, uint16_t Size)
{
  ADI_HAL_STATUS_t nStatus = ADI_HAL_OK;
  adi_osal_MutexPend(SPI2TransferLock, ADI_OSAL_TIMEOUT_FOREVER);
  SPI2_Init();
  set_adxl_cs_line(false);
  nrf_drv_spi_transfer(&spi2, pData, Size, NULL, 0);
  adi_osal_SemPend(Spi2TxrComplete, ADI_OSAL_TIMEOUT_FOREVER);
  SPI2_UnInit();
  set_adxl_cs_line(true);
  adi_osal_MutexPost(SPI2TransferLock);
  return (nStatus);
}

/**
  * @brief Function implemented in such a way that it transmits
    the register address from the first argument and receives the data
    specified by the address in the second argument. The received data will
    be of size specified by 4th argument.
  * @param  pTxAddr register addr of the ADXL362 device from where the data
            is to be read.
  * @param  pRxAddr pointer to the buffer of received data. Size of this buffer
            should be equal to the number of data requested.
  * @param  TxSize size of transmit data in bytes.
  * @param  RxSize size of requested data in bytes.
  * @retval nStatus of type ADI_HAL_STATUS_t:
            ADI_HAL_OK-> Success
            ADI_HAL_ERROR -> Failure
  */
ADI_HAL_STATUS_t ADXL362_SPI_Receive(uint8_t *pTxAddr, uint8_t *pRxAddr, uint16_t TxSize,uint16_t RxSize)
{
  ADI_HAL_STATUS_t nStatus = ADI_HAL_OK;
  adi_osal_MutexPend(SPI2TransferLock, ADI_OSAL_TIMEOUT_FOREVER);
  SPI2_Init();
  set_adxl_cs_line(false);
  nrf_drv_spi_transfer(&spi2, pTxAddr, TxSize, pRxAddr, TxSize+RxSize);
  adi_osal_SemPend(Spi2TxrComplete, ADI_OSAL_TIMEOUT_FOREVER);
  SPI2_UnInit();
  set_adxl_cs_line(true);
  adi_osal_MutexPost(SPI2TransferLock);
  return (nStatus);
}

/**
  * @brief Function implemented in such a way that it transmits
    the register address from the first argument and receives the data
    specified by the address in the second argument. The received data will
    be of size specified by 4th argument.
  * @param  pTxAddr register addr of the ADPD400x device from where the data
            is to be read.
  * @param  pRxAddr pointer to the buffer of received data. Size of this buffer
            should be equal to the number of data requested.
  * @param  TxSize size of transmit data in bytes.
  * @param  RxSize size of requested data in bytes.
  * @retval nStatus of type ADI_HAL_STATUS_t:
            ADI_HAL_OK-> Success
            ADI_HAL_ERROR -> Failure
  */
ADI_HAL_STATUS_t Adpd400x_SPI_Receive(uint8_t *pTxAddr, uint8_t *pRxAddr, uint16_t TxSize,uint16_t RxSize)
{
  ADI_HAL_STATUS_t nStatus = ADI_HAL_OK;
  adi_osal_MutexPend(SPI2TransferLock, ADI_OSAL_TIMEOUT_FOREVER);
  SPI2_Init();
  set_adpd4k_cs_line(false);
  spi_xfer_done = false;
  //TODO: Need to test the order of semPend and MutexPost
  nrf_drv_spi_transfer(&spi2, pTxAddr, TxSize, pRxAddr, TxSize+RxSize);
  adi_osal_SemPend(Spi2TxrComplete, ADI_OSAL_TIMEOUT_FOREVER);
  SPI2_UnInit();
  set_adpd4k_cs_line(true);
  adi_osal_MutexPost(SPI2TransferLock);

  return (nStatus);
}

/**
  * @brief Function implemented in such a way that it transmits
           the data from txData buffer of size specified in second argument.
  * @param  pData pointer to the buffer of the size of three bytes in which
  *         first byte is the register address of the ADPD400x device. The
  *         second and the third bytes are the 16 bits data value to be written
  *         to the device register
  * @param  Size size of the buffer in bytes (3 bytes).
  * @retval nStatus of type ADI_HAL_STATUS_t:
            ADI_HAL_OK-> Success
            ADI_HAL_ERROR -> Failure
  */
ADI_HAL_STATUS_t Adpd400x_SPI_Transmit(uint8_t *pData, uint16_t Size)
{
  ADI_HAL_STATUS_t nStatus = ADI_HAL_OK;
  adi_osal_MutexPend(SPI2TransferLock, ADI_OSAL_TIMEOUT_FOREVER);
  SPI2_Init();
  set_adpd4k_cs_line(false);
  spi_xfer_done = false;
  APP_ERROR_CHECK(nrf_drv_spi_transfer(&spi2, pData, Size, NULL, 0));
  adi_osal_SemPend(Spi2TxrComplete, ADI_OSAL_TIMEOUT_FOREVER);
  SPI2_UnInit();
  set_adpd4k_cs_line(true);
  adi_osal_MutexPost(SPI2TransferLock);
  return (nStatus);
}

nrf_drv_spi_config_t spi2_config = NRF_DRV_SPI_DEFAULT_CONFIG; //!< spi configuration structure 

/**
 * @brief SPI2 initialization
 * @retval none
 */
static void SPI2_Init(void)
{
    spi2_config.ss_pin   = NRF_DRV_SPI_PIN_NOT_USED;
    spi2_config.miso_pin = ADPD4K_SPI_MISO_PIN;
    spi2_config.mosi_pin = ADPD4K_SPI_MOSI_PIN;
    spi2_config.sck_pin  = ADPD4K_SPI_SCLK_PIN;
    spi2_config.frequency = NRF_DRV_SPI_FREQ_2M;
    nrf_drv_spi_init(&spi2, &spi2_config, spi2_event_handler, NULL);
}

/**
 @brief SPI2 de-initialization
 * @retval none
 */
static void SPI2_UnInit(void)
{
    nrf_drv_spi_uninit(&spi2);
}

/**
 * @brief SPI initialization to create mutex and semaphore
 * @retval none
 */
void spi_init(void)
{
  adi_osal_MutexCreate(&SPI2TransferLock);
  adi_osal_SemCreate(&Spi2TxrComplete, 0U);
}

#else
#define SPI_INSTANCE  0 /**< SPI instance index. */
static const nrf_drv_spi_t spi0 = NRF_DRV_SPI_INSTANCE(SPI_INSTANCE);  /**< SPI instance. */
static volatile bool spi_xfer_done;  /**< Flag used to indicate that SPI instance completed the transfer. */
ADI_OSAL_MUTEX_HANDLE SPI0TransferLock;
ADI_OSAL_SEM_HANDLE Spi0TxrComplete;

/**
 * @brief SPI user event handler.
 * @param event
 */
void spi0_event_handler(nrf_drv_spi_evt_t const * p_event,
                       void *                    p_context)
{
    spi_xfer_done = true;
    adi_osal_SemPost(Spi0TxrComplete);
//    NRF_LOG_INFO("Transfer completed.");
//    if (m_rx_buf[0] != 0)
//    {
//        NRF_LOG_INFO(" Received:");
//        NRF_LOG_HEXDUMP_INFO(m_rx_buf, strlen((const char *)m_rx_buf));
//    }
}


ADI_HAL_STATUS_t ADXL362_SPI_Receive(uint8_t *pTxAddr, uint8_t *pRxAddr, uint16_t TxSize,uint16_t RxSize)
{
  ADI_HAL_STATUS_t nStatus = ADI_HAL_OK;
  adi_osal_MutexPend(SPI0TransferLock, ADI_OSAL_TIMEOUT_FOREVER);
  SetMuxSelectLine(false);      //select adxl
  //TODO: Need to test the order of semPend and MutexPost and also the CS line
  nrf_drv_spi_transfer(&spi0, pTxAddr, TxSize, pRxAddr, TxSize+RxSize);
  adi_osal_SemPend(Spi0TxrComplete, ADI_OSAL_TIMEOUT_FOREVER);
  SetMuxSelectLine(true);       //set back to adpd as default
  adi_osal_MutexPost(SPI0TransferLock);


  return (nStatus);
}
ADI_HAL_STATUS_t ADXL362_SPI_Transmit(uint8_t *pData, uint16_t Size)
{
  ADI_HAL_STATUS_t nStatus = ADI_HAL_OK;
  adi_osal_MutexPend(SPI0TransferLock, ADI_OSAL_TIMEOUT_FOREVER);
  SetMuxSelectLine(false);      //select adxl
  //TODO: Need to test the order of semPend and MutexPost; need to change the CS line
  nrf_drv_spi_transfer(&spi0, pData, Size, NULL, 0);
  adi_osal_SemPend(Spi0TxrComplete, ADI_OSAL_TIMEOUT_FOREVER);
  SetMuxSelectLine(true);       // set back to adpd device as default
  adi_osal_MutexPost(SPI0TransferLock);
//  while(!spi_xfer_done);
  return (nStatus);
}

ADI_HAL_STATUS_t Adpd400x_SPI_Receive(uint8_t *pTxAddr, uint8_t *pRxAddr, uint16_t TxSize,uint16_t RxSize)
{
  ADI_HAL_STATUS_t nStatus = ADI_HAL_OK;
  adi_osal_MutexPend(SPI0TransferLock, ADI_OSAL_TIMEOUT_FOREVER);
//  SetMuxSelectLine(true);       //select adpd; anyway this is optional
  spi_xfer_done = false;
  //TODO: Need to test the order of semPend and MutexPost
  nrf_drv_spi_transfer(&spi0, pTxAddr, TxSize, pRxAddr, TxSize+RxSize);
  adi_osal_SemPend(Spi0TxrComplete, ADI_OSAL_TIMEOUT_FOREVER);
//  SetMuxSelectLine(false);       //select adpd; anyway this is optional
  adi_osal_MutexPost(SPI0TransferLock);

//  while(!spi_xfer_done);

  return (nStatus);
}
ADI_HAL_STATUS_t Adpd400x_SPI_Transmit(uint8_t *pData, uint16_t Size)
{
  ADI_HAL_STATUS_t nStatus = ADI_HAL_OK;
  adi_osal_MutexPend(SPI0TransferLock, ADI_OSAL_TIMEOUT_FOREVER);
//  SetMuxSelectLine(true);       //select adpd; anyway this is optional
  spi_xfer_done = false;
  //TODO: Need to test the order of semPend and MutexPost
  nrf_drv_spi_transfer(&spi0, pData, Size, NULL, 0);
  adi_osal_SemPend(Spi0TxrComplete, ADI_OSAL_TIMEOUT_FOREVER);
//  SetMuxSelectLine(false);       //select adpd; anyway this is optional
  adi_osal_MutexPost(SPI0TransferLock);
//  while(!spi_xfer_done);
  return (nStatus);
}

nrf_drv_spi_config_t spi0_config = NRF_DRV_SPI_DEFAULT_CONFIG;

static void SPI0_Init(void)
{
    adi_osal_MutexCreate(&SPI0TransferLock);
    adi_osal_SemCreate(&Spi0TxrComplete, 0U);
    spi0_config.ss_pin   = SPI_SS_PIN;
    spi0_config.miso_pin = SPI_MISO_PIN;
    spi0_config.mosi_pin = SPI_MOSI_PIN;
    spi0_config.sck_pin  = SPI_SCK_PIN;
    spi0_config.frequency = NRF_DRV_SPI_FREQ_2M;
    nrf_drv_spi_init(&spi0, &spi0_config, spi0_event_handler, NULL);
}

void spi_init(void)
{
  SPI0_Init();
}
#endif //EVTBOARD
