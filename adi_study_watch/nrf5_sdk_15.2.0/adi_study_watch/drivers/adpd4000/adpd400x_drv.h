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

/* ------------------------- Defines  -------------------------------------- */
#define SLOT_NUM                    12    //!< total number of slots on ADPD4100
#define ADPD400xDrv_SUCCESS         (0)   //!< return code for SUCCESS
#define ADPD400xDrv_ERROR           (-1)  //!< return code for ERROR
/*  REGISTER Values */
#define ADPD400x_OP_IDLE_MODE       0     //!< ENUM for Idle mode of ADPD4100
#define ADPD400x_OP_RUN_MODE        1     //!< ENUM for Run/active mode of ADPD4100
#define SLOT_BASE                   ADPD400x_REG_DATA1_A //!< Address of the data register of SlotA
#define ADPD400x_SPI_WRITE          (0x0001) //!< used for setting LS bit high for register write operation
#define ADPD400x_SPI_READ           (0xFFFE) //!< used for setting LS bit low for register read operation
#define ADPD400x_SLOT_BASE_ADDR_DIFF 0x20  //!< Difference between slot base register addresses

/* Exported types ---------------------------------------------------------- */

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
  ADPD400xDrv_MODE_IDLE = 0, //!< Idle mode, usually used for configuring registers
  ADPD400xDrv_MODE_PAUSE,    //!< Same as idle mode
  ADPD400xDrv_MODE_PWR_OFF,  //!< Power off mode of ADPD4100
  ADPD400xDrv_MODE_SAMPLE    //!< Active or running mode of ADPD4100
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
  ADPD400xDrv_DARK           //!< Dark part of data
} ADPDDrvCl_SignalDark_t;

/*!
 * @brief:  Enum for ADPD4100 Slot numbering
 */
typedef enum {
  ADPD400xDrv_SLOTA = 0x00, //!< First Slot
  ADPD400xDrv_SLOTB,        //!< Second Slot
  ADPD400xDrv_SLOTC,        //!< Third Slot
  ADPD400xDrv_SLOTD,        //!< Fourth Slot
  ADPD400xDrv_SLOTE,        //!< Fifth Slot
  ADPD400xDrv_SLOTF,        //!< Sixth Slot
  ADPD400xDrv_SLOTG,        //!< Seventh Slot
  ADPD400xDrv_SLOTH,        //!< Eighth Slot
  ADPD400xDrv_SLOTI,        //!< Ninth Slot
  ADPD400xDrv_SLOTJ,        //!< Tenth Slot
  ADPD400xDrv_SLOTK,        //!< Eleventh Slot
  ADPD400xDrv_SLOTL         //!< Twelfth Slot
} ADPD400xDrv_SlotNum_t;

/*!
 * @brief:  Enum for ADPD4100 LEDs: There are 4 LEDs on watch
 */
typedef enum {
  ADPD400xDrv_LED_OFF = 0x00, //!< No LED
  ADPD400xDrv_LED1,           //!< First LED
  ADPD400xDrv_LED2,           //!< Second LED
  ADPD400xDrv_LED3,           //!< Third LED
  ADPD400xDrv_LED4            //!< Fourth LED
} ADPD400xDrv_LedId_t;

/*!
 * @brief:  Enum for ADPD4100 type of connection: used while checking the connection type
 */
typedef enum {
  ADPD400x_I2C_BUS,    //!< I2C connection
  ADPD400x_SPI_BUS,    //!< SPI connection
  ADPD400x_UNKNOWN_BUS //!> unknown connection
} Adpd400xComMode_t;

/* Adpd control functions */
Adpd400xComMode_t Adpd400xDrvGetComMode(void);
int16_t Adpd400xDrvOpenDriver(void);
int16_t Adpd400xDrvCloseDriver(void);
int16_t Adpd400xDrvSoftReset(void);
int16_t Adpd400xDrvRegRead(uint16_t nAddr, uint16_t *pnData);
int16_t Adpd400xDrvRegRead32B(uint16_t nAddr, uint32_t *pnData);
int16_t Adpd400xDrvRegWrite(uint16_t nAddr, uint16_t nRegValue);
int16_t Adpd400xDrvSlotSetup(uint8_t nSlotNum, uint8_t nEnable,
                             uint16_t nSlotFormat, uint8_t nChannel);
int16_t Adpd400xDrvSlotSetActive(uint8_t nSlotNum, uint8_t nSleep);

int16_t Adpd400xDrvSetOperationMode(uint8_t nOpMode);
int16_t Adpd400xDrvSetOperationPause(uint8_t nEnable);
int16_t Adpd400xDrvReadFifoData(uint8_t *pnData, uint16_t nDataSetSize);
int16_t Adpd400xDrvReadRegData(uint32_t *pnData,
                               ADPD400xDrv_SlotNum_t nSlotNum,
                               uint8_t nSignalDark, uint8_t nChNum);
int16_t Adpd400xDrvSetParameter(Adpd400xCommandStruct_t eCommand,
                                uint8_t nPar, uint16_t nValue);
int16_t Adpd400xDrvGetParameter(Adpd400xCommandStruct_t eCommand,
                                uint8_t nPar, uint16_t *pnValue);
void Adpd400xDrvDataReadyCallback(void (*pfAdpdDataReady)());
int16_t Adpd400xDrvSetLedCurrent(uint16_t nLedCurrent,
                                 ADPD400xDrv_LedId_t nLedId,
                                 ADPD400xDrv_SlotNum_t nSlotNum);
int16_t Adpd400xDrvGetLedCurrent(uint16_t *pLedCurrent,
                                 ADPD400xDrv_LedId_t nLedId,
                                 ADPD400xDrv_SlotNum_t nSlotNum);
void Adpd400xISR();
#endif
