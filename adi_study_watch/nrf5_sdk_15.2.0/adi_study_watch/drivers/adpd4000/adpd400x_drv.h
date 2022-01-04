/**
***************************************************************************
* @file         adpd400x_drv.h
* @author       ADI
* @version
* @date         26-Jun-2019
* @brief        Include file for reference design device driver to access
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

#ifndef __ADPD400x_Drv__H
#define __ADPD400x_Drv__H

#include <stdint.h>
#include <adi_adpd_result.h>

/* ------------------------- Defines  -------------------------------------- */
#define SLOT_NUM                    (12)    //!< total number of slots on ADPD4100
#define ADPD400xDrv_SUCCESS         (0)   //!< return code for SUCCESS
#define ADPD400xDrv_ERROR           (-1)  //!< return code for ERROR
/*  REGISTER Values */
#define ADPD400x_OP_IDLE_MODE       0     //!< ENUM for Idle mode of ADPD4100
#define ADPD400x_OP_RUN_MODE        1     //!< ENUM for Run/active mode of ADPD4100
#define SLOT_BASE                   ADPD400x_REG_DATA1_A //!< Address of the data register of SlotA
#define ADPD400x_SPI_WRITE          (0x0001) //!< used for setting LS bit high for register write operation
#define ADPD400x_SPI_READ           (0xFFFE) //!< used for setting LS bit low for register read operation
#define ADPD400x_SLOT_BASE_ADDR_DIFF 0x20  //!< Difference between slot base register addresses

// #define ADI_ADPD_DRV_SUCCESS          (0U)       /*!< Driver operation success */
// #define ADI_ADPD_DRV_ERROR            (0x10F0U)  /*!< Driver error */
// #define ADI_ADPD_DRV_PARAM_ERROR      (0x1080U)  /*!< Driver Param error */
// #define ADI_ADPD_DRV_READ_ERROR       (0x1040U)  /*!< Driver READ  error */
// #define ADI_ADPD_DRV_WRITE_ERROR      (0x1020U)  /*!< Driver Write  error */
// #define ADI_ADPD_DRV_FIFO_ERROR       (0x1010U)  /*!< Driver Fifo Not Full error */


#define ADPD400x_I2C_LONG_ADDRESS     (0x8000U)  /*!< ADPD I2C LONG ADDRESS BIT */
/*! BUFFER MANAGER MACROS */
#define ADPA410x_FIFO_SIZE            (512U)      /*!< FIFO threshold of Adpd41x */
#define ADPD400x_FIFO_SIZE            (256U)      /*!< FIFO threshold of Adpd40x */
#define ADPD400x_ID                   (0x00C0U)   /*!< Adpd40x device ID */
#define ADPD410x_ID                   (0x00C2U)   /*!< Adpd41x device ID */

/* Enum defines ADPD4K Slot channels */
typedef enum{
  CH1,
  CH2,
  NUM_CH_PER_SLOT
} ADPD4K_SLOT_CH_t;

/*!
 * @brief:  Data type to store ADPD slot information
            See data sheet for explanation.
 */
typedef struct {
  uint8_t  activeSlot;      //!< Make the slot Active from sleep
  uint8_t  pre_activeSlot;  //!< Whether the slot was active earlier
  uint8_t  channelNum;      //!< Check whether only channel1 is enabled or both channels of a slot
  uint8_t  decimation;      //!< decimation factor for each slot
  uint16_t odr;             //!< Slot ODR
  uint16_t slotFormat;      //!< Dark,Signal,Lit,Total bytes of each slot
} adpd400xDrv_slot_t;

/*!
 * @brief:  Data type to store the various modes of operation of ADPD
 */
typedef enum {
  ADPD400xDrv_MODE_IDLE     =   0U,    //!< Idle mode, usually used for configuring registers
  ADPD400xDrv_MODE_PAUSE    =   1U,    //!< Same as idle mode
  ADPD400xDrv_MODE_PWR_OFF  =   2U,    //!< Power off mode of ADPD4100
  ADPD400xDrv_MODE_SAMPLE   =   3U     //!< Active or running mode of ADPD4100
} ADPD400xDrv_Operation_Mode_t;

/*!
 * @brief:  Data type to store the data formats in ADPD4100
 */
typedef enum {
  ADPD400xDrv_SIZE_0  = 0x00, //!< Data is not there
  ADPD400xDrv_SIZE_8  = 0x01, //!< Data is 8bit wide
  ADPD400xDrv_SIZE_16 = 0x02, //!< Data is 16bit wide
  ADPD400xDrv_SIZE_24 = 0x03, //!< Data is 24bit wide
  ADPD400xDrv_SIZE_32 = 0x04, //!< Data is 32bit wide
} ADPD400XDrv_FIFO_SIZE_t;

/*!
 * @brief:  ADPD4100 Get and Set Parameters Enums
 */
typedef enum {
  ADPD400x_WATERMARKING = 0, //!< Setting/Getting ADPD FIFO watermark
  ADPD400x_FIFOLEVEL,        //!< Setting/Getting ADPD FIFO level
  ADPD400x_OUTPUTDATARATE,   //!< Setting/Getting ADPD Output Data Rate
  ADPD400x_TIMEGAP,          //!< Getting ADPD Intersample timestamp difference
  ADPD400x_LATEST_SLOT_DATASIZE, //!< Getting ADPD Latest slot datasize after a load configuration
  ADPD400x_THIS_SLOT_DATASIZE,   //!< Getting Current Slot Size in Sample mode
  ADPD400x_SUM_SLOT_DATASIZE,    // !< Getting total slots size
  ADPD400x_IS_SLOT_ACTIVE,       //!< Check if a slot is active
  ADPD400x_IS_SLOT_SELECTED,     //!< Check if slot selected is active
  ADPD400x_HIGHEST_SLOT_NUM,     //!< Get the highest selected slot
  ADPD400x_THIS_SLOT_CHANNEL_NUM, //!< Get Channel configuration of this slot
  ADPD400x_TEST_DATA              //!< Used for testing
} Adpd400xCommandStruct_t;

/*!
 * @brief:  ADPD4100 Signal data format: See Data Sheet
 */
typedef enum {
  ADPD400xDrv_SIGNAL = 0x00, //!< Signal Part of data
  ADPD400xDrv_DARK   = 0x01  //!< Dark part of data
} ADPDDrvCl_SignalDark_t;

/*!
 * @brief:  Enum for ADPD4100 Slot numbering
*/
typedef enum {
  ADPD400xDrv_SLOTA = 0x00U, /*!< Slot-A ID Value*/
  ADPD400xDrv_SLOTB = 0x01U, /*!< Slot-B ID Value*/
  ADPD400xDrv_SLOTC = 0x02U, /*!< Slot-C ID Value*/
  ADPD400xDrv_SLOTD = 0x03U, /*!< Slot-D ID Value*/
  ADPD400xDrv_SLOTE = 0x04U, /*!< Slot-E ID Value*/
  ADPD400xDrv_SLOTF = 0x05U, /*!< Slot-F ID Value*/
  ADPD400xDrv_SLOTG = 0x06U, /*!< Slot-G ID Value*/
  ADPD400xDrv_SLOTH = 0x07U, /*!< Slot-H ID Value*/
  ADPD400xDrv_SLOTI = 0x08U, /*!< Slot-I ID Value*/
  ADPD400xDrv_SLOTJ = 0x09U, /*!< Slot-J ID Value*/
  ADPD400xDrv_SLOTK = 0x0AU, /*!< Slot-K ID Value*/
  ADPD400xDrv_SLOTL = 0x0BU  /*!< Slot-L ID Value*/
} ADPD400xDrv_SlotNum_t;

/*!
 * @brief:  Enum for ADPD4100 LEDs: There are 4 LEDs on watch
 */
typedef enum {
  ADPD400xDrv_LED_OFF = 0x00U, //!< No LED
  ADPD400xDrv_LED1    = 0x01U, //!< First LED
  ADPD400xDrv_LED2    = 0x02U, //!< Second LED
  ADPD400xDrv_LED3    = 0x03U, //!< Third LED
  ADPD400xDrv_LED4    = 0x04U  //!< Fourth LED
} ADPD400xDrv_LedId_t;

/*!
 * @brief:  Enum for ADPD4100 type of connection: used while checking the connection type
 */
typedef enum {
  ADPD400x_I2C_BUS      = 0x00U,    //!< I2C connection
  ADPD400x_SPI_BUS      = 0x01U,    //!< SPI connection
  ADPD400x_UNKNOWN_BUS  = 0xFFU     //!> unknown connection
} Adpd400xComMode_t;

/*! \struct tAdiAdpdDrvInst ""
    ADPD driver Object
 */
typedef struct {
  Adpd400xComMode_t nAdpd400xCommMode; /*!< Communication mode for read/write operations*/
  uint32_t nAccessCnt[5];              /*!< Debug buffer to hold the information for developer regarding fifo transactions */
  uint16_t nFifoLevel;                 /*!< This member will hold the fifo bytes count */
#ifndef NDEBUG
  uint32_t nOverFlowCnt; /*!< This member will hold the Fifo overflow count while sensor is in sample mode */
#endif
  uint16_t nChipID;  /*!< This member will hold the Chip ID information */
} tAdiAdpdDrvInst;
/*
Slot ID Identifier

0 --> Slot A
1 --> Slot B
*/
typedef uint8_t Adpd400xSlotId;

/* Adpd control functions */

Adpd400xComMode_t adi_adpddrv_GetComMode(void);
uint16_t adi_adpddrv_OpenDriver(void);
uint16_t adi_adpddrv_CloseDriver(void);
uint16_t adi_adpddrv_SoftReset(void);
adi_adpd_result_t adi_adpddrv_RegRead(uint16_t nAddr, uint16_t *pnData);
adi_adpd_result_t adi_adpddrv_RegRead32B(uint16_t nAddr, uint32_t *pnData);
adi_adpd_result_t adi_adpddrv_RegWrite(uint16_t nAddr, uint16_t nRegValue);
adi_adpd_result_t adi_adpddrv_SetIdleMode(void);
int16_t adi_adpddrv_SetOperationPause(uint8_t nEnable);
adi_adpd_result_t adi_adpddrv_ReadFifoData(uint16_t nDataSetSize, uint8_t *pnData);
adi_adpd_result_t adi_adpddrv_ReadRegData(ADPD400xDrv_SlotNum_t nSlotNum, 
                                          uint8_t nSignalDark, uint8_t nChNum, 
                                          uint32_t *pnData);
void adi_adpddrv_DataReadyCallback(void (*pfAdpdDataReady)());
int16_t adi_adpddrv_SetLedCurrent(uint16_t nLedCurrent, 
                                 ADPD400xDrv_LedId_t nLedId,
                                 ADPD400xDrv_SlotNum_t nSlotNum);
int16_t adi_adpddrv_GetLedCurrent(uint16_t *pLedCurrent, 
                                 ADPD400xDrv_LedId_t nLedId,
                                 ADPD400xDrv_SlotNum_t nSlotNum);
uint16_t adi_adpddrv_GetChipId(void);
void adi_adpddrv_ISR();
#endif
