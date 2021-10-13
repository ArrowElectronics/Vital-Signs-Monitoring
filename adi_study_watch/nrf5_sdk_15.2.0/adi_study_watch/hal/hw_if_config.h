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
#ifndef __IFCONFIG_H
#define __IFCONFIG_H

#include "stdint.h"
#include "nrf_drv_twi.h"
#include "nrf_delay.h"
#include "adpd400x_drv.h"
#include "adxl362.h"
#include "adi_types.h"
#include "adi_osal.h"
#include "FreeRTOS.h"
#include "task.h"
#include "rtc.h"
typedef enum {
  ADI_HAL_OK       = 0x00,
  ADI_HAL_ERROR    = 0x01,
  ADI_HAL_BUSY     = 0x02,
  ADI_HAL_TIMEOUT  = 0x03
} ADI_HAL_STATUS_t;

typedef enum {
  PWR_CTRL_SUCCESS = 0,
  PWR_CTRL_ERROR
} PWR_CTRL_STATUS_t;

#ifdef NRF52840_XXAA
    #define SPI_DUMMY_BYTES        2
#else
    #define SPI_DUMMY_BYTES        0
#endif //NRF52840_XXAA



/* Private function prototypes -----------------------------------------------*/
int I2c_Reg_read(uint16_t nAddr, uint16_t *pnData);
int I2c_Reg_write(uint16_t adr,uint16_t data);
void twi_init (void);
void spi_init(void);
void gpio_init(void);
ADI_HAL_STATUS_t GPIO_IRQ_ADXL362_Disable();
ADI_HAL_STATUS_t GPIO_IRQ_ADXL362_Enable();
ADI_HAL_STATUS_t GPIO_IRQ_ADPD_Enable();
ADI_HAL_STATUS_t GPIO_IRQ_ADPD_Disable();
#ifndef EVTBOARD
ADI_HAL_STATUS_t SetMuxSelectLine(bool_t bValue);
#else
ADI_HAL_STATUS_t set_adpd4k_cs_line(bool_t bValue);
ADI_HAL_STATUS_t set_adxl_cs_line(bool_t bValue);
#endif //EVTBOARD

void uart_init(void);
void uart_tx(uint8_t *pData, uint32_t nLen);
void uart_rx(uint8_t *pData, uint32_t nLen);
void HAL_delay_ms(uint32_t ms_time);
void MCU_HAL_Delay(uint32_t ms_time);


ADI_HAL_STATUS_t ADPD_I2C_Transmit(uint8_t *pData, uint16_t Size);
ADI_HAL_STATUS_t ADPD_I2C_TxRx(uint8_t *Addr, uint8_t *pRxData,
                               uint16_t RxSize);

ADI_HAL_STATUS_t ADPD400x_I2C_Transmit(uint8_t *pData, uint16_t Size);
ADI_HAL_STATUS_t ADPD400x_I2C_TxRx(uint8_t *Addr, uint8_t *pRxData, uint16_t TxSize,
                               uint16_t RxSize);
ADI_HAL_STATUS_t ADXL362_SPI_Receive(uint8_t *pTxAddr, uint8_t *pRxAddr, uint16_t TxSize,uint16_t RxSize);
ADI_HAL_STATUS_t ADXL362_SPI_Transmit(uint8_t *pData, uint16_t Size);


ADI_HAL_STATUS_t ADPD400x_SPI_Receive(uint8_t *pTxAddr, uint8_t *pRxAddr, uint16_t TxSize,uint16_t RxSize);
ADI_HAL_STATUS_t ADPD400x_SPI_Transmit(uint8_t *pData, uint16_t Size);
void AdpdDriverBringUp(uint8_t nSlotA, uint8_t nSlotB);

TickType_t MCU_HAL_GetTick();
uint32_t AdpdLibGetTick();
uint32_t AdpdLibGetSensorTimeStamp();
void GPIO_Clock_Cal_TriggerTS(void);
void UART_RegisterReceiverCallback( uint16_t (*pfReceiveInputChar)(uint8_t *pByte, uint16_t nRxLength) );
void UART_Write_nblk(uint8_t *pData, uint32_t size);
void UARTClearFifo();
uint8_t get_usbd_tx_pending_status (void);
void Adpd400xISR();
int low_touch_init();

void invert_adxl_trigger_signal();
void enable_adxl_trigger_pin();
void disable_adxl_trigger_pin();

void invert_adpd_trigger_signal();
void enable_adpd_trigger_pin();
void disable_adpd_trigger_pin();
void reset_trigger_pulses();
void reset_adxl_trigger_signal(bool n_state);
void reset_adpd_trigger_signal(bool n_state);

void invert_ad5940_trigger_signal();
void disable_ad5940_trigger_pin();
void reset_ad5940_trigger_signal(bool n_state);
void enable_ad5940_trigger_pin();
#endif /* __IFCONFIG_H */