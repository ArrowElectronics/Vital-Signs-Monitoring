/**
***************************************************************************
* @file         adpd400x_drv.c
* @author       ADI
* @version
* @date         26-Jun-2019
* @brief        Reference design device driver to access ADI ADPD400x chip
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

/* ------------------------- Includes -------------------------------------- */
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdlib.h>
#include "adpd400x_reg.h"
#include "adpd400x_drv.h"
#include "hw_if_config.h"
#include "lcm.h"
#include <adpd4000_buffering.h>

ADI_HAL_STATUS_t Adpd400x_I2C_TxRx(uint8_t *register_address, uint8_t * buffer, uint16_t txsize, uint16_t rxsize);
ADI_HAL_STATUS_t Adpd400x_I2C_Transmit(uint8_t *register_address, uint16_t txsize);
ADI_HAL_STATUS_t Adpd400x_SPI_Receive(uint8_t *pTxData, uint8_t *pRxData, uint16_t TxSize, uint16_t RxSize);
ADI_HAL_STATUS_t Adpd400x_SPI_Transmit(uint8_t *pTxData, uint16_t TxSize);
void MCU_HAL_Delay(uint32_t ms_delay);;

/* ------------------------- Defines  -------------------------------------- */
/* define the following macro for DEBUG */
//#define DEBUG_DRV                 1
#define SLOT_DISABLE_SETTINGS     6   //!< Size of buffer for each slot to store important LED settings
#define INTERRUPT_ENABLE              //!< Macro used to enable interrupt mode register settings
#define SLEEP_TIME                500 //!< Settling time in millisec required during device initialization

#ifdef INTERRUPT_ENABLE
/*  Interrupt pin is enabled
Define FIFO_TH_INT_ENA as 0 to disable it */
#define FIFO_TH_INT_ENA ((0x1) << 15) //!< Register 0x0014 setting bit 15 for enabling FIFO threshold interrupt
#define FIFO_UF_INT_ENA ((0x1) << 14) //!< Register 0x0014 setting bit 14 for enabling FIFO underflow interrupt
#define FIFO_OF_INT_ENA ((0x1) << 13) //!< Register 0x0014 setting bit 13 for enabling FIFO overflow interrupt
#endif

/*------------------------- Public Variables ------------------------------- */
uint32_t nReadSequence = 1;           //!< Sequence number maintained for reading slot data from the Buffer
uint32_t nInterruptSequence = 1;      //!< Sequence number maintained after handling DataRdy interrupt and slot data is packetised
uint32_t nWriteSequence = 1;          //!< Sequence number maintained while writing slot data from FIFO to the Buffer
uint32_t gnLcmValue = 0;
uint8_t  nFifoStatusByte = 0;
uint16_t gAdpdSlotMaxODR = 0;         //!< Maximum ODR among of all the active slots
adpd400xDrv_slot_t gsSlot[SLOT_NUM];  //!< Buffer for holding slot information
extern uint8_t dvt2;                  //!< flag to check whether DVT2 (watch version) or not
extern volatile uint8_t gsOneTimeValueWhenReadAdpdData; //! flag when cleared -> checks if adpd task prio to be increased based on the ODR
/* ------------------------- Public Function Prototypes -------------------- */
uint32_t* Adpd400xDrvGetDebugInfo();

/* ------------------------- Private variables ----------------------------- */
static uint8_t gsTotalSlotSize;         //!< Total active slot size in bytes
static uint16_t gsHighestSelectedSlot;  //!< Highest selected slot
static Adpd400xComMode_t nAdpd400xCommMode = ADPD400x_UNKNOWN_BUS; //!< Communication mode of sensor (I2C or SPI)
static uint8_t gnAdpdFifoWaterMark = 1; //!< FIFO watermark set to 1. Interrupt obtaimed as one sample is in FIFO
static uint32_t gnAccessCnt[5];   //!< Debug variable
static uint16_t gnFifoLevel;      //!< Stores number of bytes in FIFO
static uint16_t pre_active_setting[SLOT_NUM][SLOT_DISABLE_SETTINGS]; //!< stores LED settings of all slots
static uint16_t gsOutputDataRate; //!< Stores Output Data Rate,which is sampling rate/decimation

#ifndef NDEBUG
static uint32_t gnOverFlowCnt = 0; //!< Debug variable to check if there is ADPD FIFO overflow
#endif  // NDEBUG
/* ------------------------- Private Function Prototypes ------------------- */
static void _Adpd400xDrvInit(void);
static int16_t _Adpd400xDrvSetIdleMode(void);
static int16_t _Adpd400xDrvSetInterrupt(void);
static void _Adpd400xDrvGetSlotInfo(void);
static void _Adpd400xDrvSlotSaveCurrentSetting(uint8_t nSlotNum);
static void _Adpd400xDrvSlotApplyPreviousSetting(uint8_t nSlotNum);
static void _Adpd400xDrvSlotApplySkipSetting(uint8_t nSlotNum);
static void _Adpd400xDrvGetDataOutputRate(uint8_t nSlotNum);
static void _Adpd400xDrvSetSlotSize(uint8_t nSlotNum, uint16_t nSlotFormat);
static void (*gpfnADPDCallBack)(); //!< Function pointer to store ADPD data ready callback func address
//static int16_t _Adpd400xDrvSelComMode();
static uint16_t _FifoLevel(void);

/** @brief  Open Driver, setting up the interrupt and I2C/SPI lines
*           Give time for settling, Sets the device for interrupt mode
*
* @return int16_t A 16-bit integer: 0 - success; < 0 - failure
*/
int16_t Adpd400xDrvOpenDriver() {
  int16_t nRetCode = ADPD400xDrv_SUCCESS;
#ifdef BUILD_FOR_M4
  if (nAdpd400xCommMode == ADPD400x_UNKNOWN_BUS)
    if (_Adpd400xDrvSelComMode() != ADPD400xDrv_SUCCESS)
      return ADPD400xDrv_ERROR;
#else
    nAdpd400xCommMode = ADPD400x_I2C_BUS;
#endif // BUILD_FOR_M4
//  nRetCode = _Adpd400xDrvSetIdleMode();
    nAdpd400xCommMode = ADPD400x_SPI_BUS;
  if(nAdpd400xCommMode == ADPD400x_I2C_BUS) {
    uint8_t txData[3] = {0x0f, 0x80, 0x00};
    if (Adpd400x_I2C_Transmit(txData, 3) != ADI_HAL_OK) {
      return ADPD400xDrv_ERROR;
    }
  } else {
    uint8_t txData[4] = {0x00, 0x1f, 0x80, 0x00};
    if (Adpd400x_SPI_Transmit(txData, 4)!= ADI_HAL_OK) {
      return ADPD400xDrv_ERROR;
    }
  }
  MCU_HAL_Delay(SLEEP_TIME);
  _Adpd400xDrvInit();

  nRetCode |= _Adpd400xDrvSetInterrupt();  // Default mode
  return nRetCode;
}

/** @brief  Close Driver, Clear up before exiting by setting device to idle mode
*           See data sheet for explanation.
*
* @return int16_t A 16-bit integer: 0 - success; < 0 - failure
*/
int16_t Adpd400xDrvCloseDriver() {
  uint16_t nRetCode;
  nRetCode = _Adpd400xDrvSetIdleMode();
  return nRetCode;
}

/** @brief  Returns the communication bus type of sensor : I2C, SPI or Unknown
*           See the typedef Adpd400xComMode_t
*
* @return nAdpd400xComMode
*/
Adpd400xComMode_t Adpd400xDrvGetComMode() {
  return nAdpd400xCommMode;
}

/** @brief  Synchronous register write to the ADPD400x
*
* @param  nAddr 16-bit register address
* @param  nRegValue 16-bit register data value
* @return int16_t A 16-bit integer: 0 - success; < 0 - failure
*/
int16_t Adpd400xDrvRegWrite(uint16_t nAddr, uint16_t nRegValue) {
  uint8_t txData[4];
  uint8_t i = 0;

  /**
  This function Adpd400xDrvRegWrite() calls either the sub-function
  Adpd400x_SPI_Receive() or Adpd400x_I2C_TxRx()
  based on the communication mode ADPD400x_SPI_BUS / ADPD400x_I2C_BUS
  */

  if((nAddr == ADPD400x_REG_SYS_CTL) && ((nRegValue & BITM_SYS_CTL_SW_RESET) == BITM_SYS_CTL_SW_RESET)) {
      return Adpd400xDrvSoftReset();
  } else {
    if (nAdpd400xCommMode == ADPD400x_SPI_BUS) {
      uint16_t nTmpAddr = (nAddr << 1 ) | ADPD400x_SPI_WRITE; // To set the last bit high for write operation

    txData[i++] = (uint8_t)(nTmpAddr >> 8);
    txData[i++] = (uint8_t)(nTmpAddr);
    txData[i++] = (uint8_t)(nRegValue >> 8);
    txData[i++] = (uint8_t)(nRegValue);

    if (Adpd400x_SPI_Transmit(txData, i)!= ADI_HAL_OK) {
      return ADPD400xDrv_ERROR;
    }
  } else {
    if (nAddr > 0x7F) {
      txData[i++] = (uint8_t)((nAddr >> 8)|0x80);
      txData[i++] = (uint8_t)nAddr;
    } else {
      txData[i++] = (uint8_t)nAddr;
    }
    txData[i++] = (uint8_t)(nRegValue >> 8);
    txData[i++] = (uint8_t)(nRegValue);

      if (Adpd400x_I2C_Transmit((uint8_t *) txData, i) != ADI_HAL_OK) {
        return ADPD400xDrv_ERROR;
      }
    }
  }
  return ADPD400xDrv_SUCCESS;
}

/** @brief  Synchronous register read from the ADPD400x
*
* @param  nAddr 16-bit register address
* @param  *pnData Pointer to 16-bit register data value
* @return int16_t A 16-bit integer: 0 - success; < 0 - failure
*/
int16_t Adpd400xDrvRegRead(uint16_t nAddr, uint16_t *pnData) {
  uint8_t anRxData[2 + SPI_DUMMY_BYTES];
  uint8_t txData[2];
  uint16_t i = 0;

  if (nAdpd400xCommMode == ADPD400x_SPI_BUS) {
    uint16_t nTmpAddr = (nAddr << 1 ) & ADPD400x_SPI_READ; // To set the last bit low for read operation

    txData[i++] = (uint8_t)(nTmpAddr >> 8);
    txData[i++] = (uint8_t)(nTmpAddr);

    /**
    This function Adpd400xDrvRegRead() calls either the sub-function
    Adpd400x_SPI_Receive() or Adpd400x_I2C_TxRx()
    based on the communication mode ADPD400x_SPI_BUS / ADPD400x_I2C_BUS
    */

    if (Adpd400x_SPI_Receive(txData, anRxData, i, 2)!= ADI_HAL_OK) {
      return ADPD400xDrv_ERROR;
    }
  } else if (nAdpd400xCommMode == ADPD400x_I2C_BUS) {
    if (nAddr > 0x7F) {
      txData[i++] = (uint8_t)((nAddr >> 8)|0x80);
      txData[i++] = (uint8_t)nAddr;
    } else {
      txData[i++] = (uint8_t)nAddr;
    }

    // need to work on this for 15-bit address
    if (Adpd400x_I2C_TxRx((uint8_t *) txData, (uint8_t *) anRxData, i, 2) != ADI_HAL_OK) {
      return ADPD400xDrv_ERROR;
    }
  } else {
    return ADPD400xDrv_ERROR;
  }
  *pnData = (anRxData[0+SPI_DUMMY_BYTES] << 8) + anRxData[1+SPI_DUMMY_BYTES];
  return ADPD400xDrv_SUCCESS;
}

/** @brief  Synchronous register read from the ADPD400x
*
* @param  nAddr 16-bit register address
* @param  *pnData Pointer to 32-bit register data value
* @return int16_t A 16-bit integer: 0 - success; < 0 - failure
*/
int16_t Adpd400xDrvRegRead32B(uint16_t nAddr, uint32_t *pnData) {
  uint8_t anRxData[4+SPI_DUMMY_BYTES];
  uint8_t txData[2];
  uint16_t i = 0;
  uint16_t nTmpAddr = 0;

  /**
  This function Adpd400xDrvRegRead32B() calls either the sub-function
  Adpd400x_SPI_Receive() or Adpd400x_I2C_TxRx()
  based on the communication mode ADPD400x_SPI_BUS / ADPD400x_I2C_BUS
  */

  if (nAdpd400xCommMode == ADPD400x_SPI_BUS) {
    nTmpAddr = (nAddr << 1 ) & ADPD400x_SPI_READ; // To set the last bit low for read operation

    txData[i++] = (uint8_t)(nTmpAddr >> 8);
    txData[i++] = (uint8_t)(nTmpAddr);

    if (Adpd400x_SPI_Receive(txData, anRxData, i, 4)!= ADI_HAL_OK) {
      return ADPD400xDrv_ERROR;
    }
  } else if (nAdpd400xCommMode == ADPD400x_I2C_BUS) {
    if (nAddr > 0x7F) {
      txData[i++] = (uint8_t)((nAddr >> 8)|0x80);
      txData[i++] = (uint8_t)nAddr;
    } else {
      txData[i++] = (uint8_t)nAddr;
    }
    if (Adpd400x_I2C_TxRx((uint8_t *) txData, (uint8_t *) anRxData, i, 4) != ADI_HAL_OK) {
      return ADPD400xDrv_ERROR;
    }
  } else {
    return ADPD400xDrv_ERROR;
  }
  *pnData = (anRxData[0+SPI_DUMMY_BYTES] << 8) + anRxData[1+SPI_DUMMY_BYTES] + (anRxData[2+SPI_DUMMY_BYTES] << 24) + (anRxData[3+SPI_DUMMY_BYTES] << 16);
  return ADPD400xDrv_SUCCESS;
}

/** @brief  Set Adpd400x operating mode, clear FIFO if needed
*
* @param  nOpMode 8-bit operating mode
* @return int16_t A 16-bit integer: 0 - success; < 0 - failure
*/
int16_t Adpd400xDrvSetOperationMode(uint8_t nOpMode) {
  int16_t nRetCode = ADPD400xDrv_SUCCESS;
  uint16_t nTemp;
  uint16_t nSampleSize = 0,nMaximumFifoTh = 0;
  uint16_t nWaterMark = 1;

  if (nOpMode == ADPD400xDrv_MODE_IDLE) {
    nRetCode = _Adpd400xDrvSetIdleMode();
    //gnAdpdFifoWaterMark = 1;//need to handle it for worst case so set as 1.
    gsTotalSlotSize = 0;
  } else if (nOpMode == ADPD400xDrv_MODE_SAMPLE) {
    // update the Slot size info
    _Adpd400xDrvGetSlotInfo();

    /* Clearing this flag to run the code under if!gsOneTimeValueWhenReadAdpdData) loop
    in fetch adpd data since gAdpdSlotMaxODR is updated in this function */
    gsOneTimeValueWhenReadAdpdData = 0;

    // set the watermark for 2-bytes slotA data.
    /**   set the watermark value dynamically ,because
    *   data size(upto 4) ,
    *   channels (ch1 &ch2 )and slots(12 slots) are changed in runtime.
    */
    nReadSequence = 1;
    nInterruptSequence = 1;
    nWriteSequence = 1;
    // reset the optional bytes value while stopping stream
    nFifoStatusByte = 0;
    nSampleSize = _FifoLevel();

    //Check if its ADPD4000 or ADPD4100, there is change in 0x0006 register max FIFO bytes
    nMaximumFifoTh = (dvt2 == 0)?0xFF:0x1FF;

    if(nSampleSize > (nMaximumFifoTh + 1) || nSampleSize == 0) {
      return ADPD400xDrv_ERROR;
    } else {
      nSampleSize = nSampleSize + nFifoStatusByte;
      Adpd400xDrvGetParameter(ADPD400x_WATERMARKING, 0, &nWaterMark);
      nRetCode = Adpd400xDrvRegWrite(ADPD400x_REG_FIFO_CTL,  (nWaterMark*nSampleSize - 1) & nMaximumFifoTh);
    }

    nRetCode |= Adpd400xDrvRegWrite(ADPD400x_REG_INT_STATUS_FIFO, 0x8000); // Clear FIFO

    // set GO
    nTemp = gsHighestSelectedSlot << BITP_OPMODE_TIMESLOT_EN;
    nTemp |= ADPD400x_OP_RUN_MODE;
    nRetCode |= Adpd400xDrvRegWrite(ADPD400x_REG_OPMODE, nTemp);
  }

  return nRetCode;
}

/** @brief  Set the enable/disable of Adpd400x operating mode for Pause func
*
* @param  nEnable 1- enable pause 1- disable pause
* @return int16_t A 16-bit integer: 0 - success; < 0 - failure
*/
int16_t Adpd400xDrvSetOperationPause(uint8_t nEnable) {
  int16_t nRetCode = ADPD400xDrv_SUCCESS;
  uint16_t nTemp;

  nRetCode = Adpd400xDrvRegRead(ADPD400x_REG_OPMODE, &nTemp);
  nTemp &= 0xFFFE;
  nRetCode |= Adpd400xDrvRegWrite(ADPD400x_REG_OPMODE, nTemp | nEnable);

  return nRetCode;
}


/** @brief  Enable an operation time slot
*           See data sheet for explanation.
*
* @param  nSlotNum: The desired slot number (any of slots A(0) to L(11)) is passed
* @param  nEnable: 1 -> enable, 0 -> disable
* @param  nSlotFormat: Data format in FIFO = IDS (Impulse, Dark, Sig)
* @param  nChannel: Channel selection for SlotA-L:
*                                       1. For channel one  --> 1
*                                       2. For both channel --> 3
* @return int16_t A 16-bit integer: 0 - success; < 0 - failure
*/
int16_t Adpd400xDrvSlotSetup(uint8_t nSlotNum, uint8_t nEnable, \
                                uint16_t nSlotFormat, uint8_t nChannel) {
  uint16_t addr, value1, value2;
  // data size is set here, but the data shift location is set in upper layer
  if ((nSlotFormat & 0x100) == 0)  {    // not impulse mode
    if ((nSlotFormat & 0xF) > 4 || (nSlotFormat & 0xF0) > 0x40 )
      return ADPD400xDrv_ERROR;
  }

  gsTotalSlotSize = 0;          // reset the sum size. Indicate a slot change
  // disable a slot:
  //  disable every slots after this one
  Adpd400xDrvRegRead(ADPD400x_REG_OPMODE, &value1);
  value2 = value1 & BITM_OPMODE_TIMESLOT_EN;
  value2 >>= BITP_OPMODE_TIMESLOT_EN;
  // disable a slot:
  //  disable every slots after this one
  if (nEnable == 0)  {    // un_select all the slot after
    if (nSlotNum <= value2)  {
      value1 &= (~BITM_OPMODE_TIMESLOT_EN);
      if (nSlotNum != 0)  {
        value1 |= ((nSlotNum - 1) << BITP_OPMODE_TIMESLOT_EN);
        gsHighestSelectedSlot = nSlotNum - 1;
      } else {
        gsHighestSelectedSlot = 0;
      }
      Adpd400xDrvRegWrite(ADPD400x_REG_OPMODE, value1);
    }
  } else {    // select all the slots before
    if (nSlotNum > value2)  {
      value1 &= (~BITM_OPMODE_TIMESLOT_EN);
      value1 |= (nSlotNum << BITP_OPMODE_TIMESLOT_EN);
      Adpd400xDrvRegWrite(ADPD400x_REG_OPMODE, value1);
      gsHighestSelectedSlot = nSlotNum;
    }
  }

  if (nChannel >1)  {   // Choice is 0,
    gsSlot[nSlotNum].channelNum = 3;
    nChannel = 1;
  } else {
    gsSlot[nSlotNum].channelNum = 1;
    nChannel = 0;
  }

  // Get the requested TS_CTRL register address
  addr = nSlotNum * ADPD400x_SLOT_BASE_ADDR_DIFF + ADPD400x_REG_TS_CTRL_A;
  // Read the TS_CTRL value
  Adpd400xDrvRegRead(addr, &value1);
  // Mask register value
  value1 &= (~BITM_TS_CTRL_A_CH2_EN_A);
  // Update the Channel 2 bit with requested value
  value1 |= (nChannel<<BITP_TS_CTRL_A_CH2_EN_A);
  // Write back to TS_CTRL register
  Adpd400xDrvRegWrite(addr, value1);

  // set slot size
  _Adpd400xDrvSetSlotSize(nSlotNum, nSlotFormat);

  return ADPD400xDrv_SUCCESS;
}



/** @brief  Set a slot in sleep/active mode
*           This methods set the specified time-slot into sleep mode
*           See data sheet for explanation
*
* @param  nSlotNum: The desired slot number (any of slots A(0) to L(11)) is passed
* @param  nActive: 0 = sleep, 1 = awake
* @return int16_t A 16-bit integer: 0 - success; < 0 - failure
*/
int16_t Adpd400xDrvSlotSetActive(uint8_t nSlotNum, uint8_t nActive) {
  int16_t nRetCode = ADPD400xDrv_ERROR;
  if (nActive == 0)  {    // Put a slot to Sleep
    if (gsSlot[nSlotNum].pre_activeSlot == 1)  {    // do skip a slot
      _Adpd400xDrvSlotSaveCurrentSetting(nSlotNum);
      _Adpd400xDrvSlotApplySkipSetting(nSlotNum);
    }
    gsSlot[nSlotNum].activeSlot = 0;
    gsSlot[nSlotNum].pre_activeSlot = 0;
    nRetCode = ADPD400xDrv_SUCCESS;

  } else {    // Set a slot awake
    // if previous is Sleep
    if (gsSlot[nSlotNum].pre_activeSlot == 0)  {
      _Adpd400xDrvSlotApplyPreviousSetting(nSlotNum);
    } else {
      _Adpd400xDrvSlotSaveCurrentSetting(nSlotNum);
    }
    gsSlot[nSlotNum].activeSlot = 1;
    gsSlot[nSlotNum].pre_activeSlot = 1;
    nRetCode = ADPD400xDrv_SUCCESS;
  }

  return nRetCode;
}

/** @brief  Register data ready callback
* There is an interrupt pin available for ADPD, it will trigger whenever the data is
* ready in FIFO based on sampling period. To configure the interrupt pin, need to connect
* a GPIO pin to this ADPD INT pin from MCU. Then this GPIO pin from MCU should be configured as
* int handler. This means when this GPIO pin is high, a function handler has to be called to serve those interrupt.
* That handler is adpd_int_handler. This adpd_int_handler will call the ADPDISR function.
*
* @param  pfADPDDataReady  Function Pointer callback for the reg data
* @return None
*/
void Adpd400xDrvDataReadyCallback(void (*pfADPDDataReady)()) {
  gpfnADPDCallBack = pfADPDDataReady;
}

/** @brief  ADPD400x interrupt service routine will tell the application that data ready to read via callback
* @return None
*/
void Adpd400xISR() {
  if (gpfnADPDCallBack != NULL) {
    (*gpfnADPDCallBack)();
  }
  gnAccessCnt[0]++;
}

/** @brief  Debug function. Read out debug info
* @return uint32_t* Debug info pointer
*/
uint32_t* Adpd400xDrvGetDebugInfo() {
  return gnAccessCnt;
}

/** @brief  Debug function. Read out debug info
* @param  None
* @return uint32_t* Debug info pointer
*/
uint32_t Adpd400xDrvGetISRDebugInfo() {
  return gnAccessCnt[0];
}

/** @brief Set/Configure a parameter for the device
*          See data sheet for explanation of registers used
*
* @param eCommand command for Watermark value
* @param nPar: Not used now
* @param nValue DataSet Size to be set
* @return @return int16_t A 16-bit integer: 0 - success; < 0 - failure
*/
int16_t Adpd400xDrvSetParameter(Adpd400xCommandStruct_t eCommand, uint8_t nPar, uint16_t nValue) {
  int16_t nRetCode = ADPD400xDrv_SUCCESS;
  uint16_t nMaximumFifoTh = 0;
  switch(eCommand){
  case ADPD400x_WATERMARKING:
    // set the watermark for 2-bytes slotA data.
    // TODO: Change it for other data types and slot mode

    //Check if its ADPD4000 or ADPD4100, there is change in 0x0006 register max FIFO bytes
    nMaximumFifoTh = (dvt2 == 0)?0xFF:0x1FF;

    nRetCode = Adpd400xDrvRegWrite(ADPD400x_REG_FIFO_CTL,
                                   ((nValue * (gsTotalSlotSize==0?1:gsTotalSlotSize)) - 1) & nMaximumFifoTh);
    gnAdpdFifoWaterMark = nValue;
    break;
  case ADPD400x_TEST_DATA:
    Adpd400xDrvRegRead(0x1e, &nValue);
    gnAccessCnt[1] = nValue;
    Adpd400xDrvRegRead(0x1f, &nValue);
    gnAccessCnt[2] = nValue;
    break;
  default:
    break;
  }
  return nRetCode;
}

/** @brief Get a parameter from the device
*
* @param eCommand: Target in interest
* @param nPar: eCommand's argument which specify slot number
* @param pnValue: Output Target's value
* @return Pass or Fail
*/
int16_t Adpd400xDrvGetParameter(Adpd400xCommandStruct_t eCommand, uint8_t nPar, uint16_t *pnValue) {
  int16_t nRetCode = ADPD400xDrv_SUCCESS;
  uint16_t nStatData;
  uint16_t nRegValue1, nRegValue2;
  uint32_t nTemp;

  // In _Adpd400xDrvInit, SlotSetup, ChannelSetup, Idle mode: gsTotalSlotSize is set to 0
  switch(eCommand) {
  case ADPD400x_WATERMARKING:
    *pnValue = gnAdpdFifoWaterMark;
    break;
  case ADPD400x_OUTPUTDATARATE:
    _Adpd400xDrvGetDataOutputRate(nPar);
    *pnValue = gsOutputDataRate;
    break;
  case ADPD400x_FIFOLEVEL:
    nRetCode |= Adpd400xDrvRegRead(ADPD400x_REG_INT_STATUS_FIFO, &nStatData);
    gnFifoLevel = nStatData & 0x7FF;
    *pnValue = gnFifoLevel;
    break;
  case ADPD400x_TIMEGAP:
    // time between two samples in milliseconds
    nRetCode |= Adpd400xDrvRegRead(ADPD400x_REG_TS_FREQ, &nRegValue1);
    nRetCode |= Adpd400xDrvRegRead(ADPD400x_REG_TS_FREQH, &nRegValue2);
    nTemp = nRegValue1 | (nRegValue2 & 0x7F) << 8; // // time in microseconds
    nRetCode |= Adpd400xDrvRegRead(ADPD400x_REG_DECIMATE_A, &nRegValue2);
    nRegValue2 = nRegValue2 & 0xFF;
    *pnValue = (nTemp * (nRegValue2 + 1)) / 1000; // time in milliseconds
    break;
  case ADPD400x_LATEST_SLOT_DATASIZE:   // App calls it after DCFG_load. (GetSlot)
    _Adpd400xDrvGetSlotInfo();
    *pnValue = gsSlot[nPar].slotFormat;
    break;
  case ADPD400x_THIS_SLOT_DATASIZE:     // buffer_op calls in sample mode
    if (gsTotalSlotSize == 0)
      _Adpd400xDrvGetSlotInfo();
    *pnValue = gsSlot[nPar].slotFormat;
    break;
  case ADPD400x_SUM_SLOT_DATASIZE:
    if (gsTotalSlotSize == 0)
      _Adpd400xDrvGetSlotInfo();
    *pnValue = gsTotalSlotSize;
    break;
  case ADPD400x_IS_SLOT_ACTIVE:     // Check if this slot is active
    *pnValue = gsSlot[nPar].activeSlot;
    break;
  case ADPD400x_IS_SLOT_SELECTED:   // Check if this slot is active
    if ( nPar<= gsHighestSelectedSlot)
      *pnValue = gsSlot[nPar].activeSlot;
    break;
  case ADPD400x_HIGHEST_SLOT_NUM:
    if (gsTotalSlotSize == 0)
      _Adpd400xDrvGetSlotInfo();
    *pnValue = gsHighestSelectedSlot;
    break;
  case ADPD400x_THIS_SLOT_CHANNEL_NUM:
    *pnValue = gsSlot[nPar].channelNum;
    break;
  default:
    break;
  }
  return nRetCode;
}

/** @brief Read data out from Adpd400x FIFO
*
* @param  pnData pointer to an array address to read the data to.
* @param nDataSetSize DataSet Size to read
* @return int16_t A 16-bit integer: 0 - success; < 0 - failure
*/
int16_t Adpd400xDrvReadFifoData(uint8_t *pnData, uint16_t nDataSetSize) {
  uint8_t nAddr;
  uint16_t nTmpAddr = 0;
  uint8_t txData[2];
  uint8_t i = 0;

  /**
  This function Adpd400xDrvReadFifoData() calls Adpd400xDrvRegRead()
  It also calls either the sub-function Adpd400x_SPI_Receive() or Adpd400x_I2C_TxRx()
  based on the communication mode ADPD400x_SPI_BUS / ADPD400x_I2C_BUS
  */

  Adpd400xDrvRegRead(ADPD400x_REG_INT_STATUS_FIFO, &gnFifoLevel);
  gnFifoLevel = gnFifoLevel & 0x7FF;
#ifndef NDEBUG
  if (gnFifoLevel >= 256) {
    gnOverFlowCnt++;
  }
#endif  // NDEBUG
  if (gnFifoLevel >= nDataSetSize) {
    gnAccessCnt[2]++;
    nAddr = ADPD400x_REG_FIFO_DATA;
    switch(nAdpd400xCommMode){
    case ADPD400x_SPI_BUS:
      i = 0;
      nTmpAddr = (nAddr << 1 ) & ADPD400x_SPI_READ; // To set the last bit low for read operation
      txData[i++] = (uint8_t)(nTmpAddr >> 8);
      txData[i++] = (uint8_t)(nTmpAddr);

      if (Adpd400x_SPI_Receive(txData, pnData, 2, nDataSetSize) != ADI_HAL_OK) {
        return ADPD400xDrv_ERROR;
      }
      break;
    case ADPD400x_I2C_BUS:
      if (Adpd400x_I2C_TxRx(&nAddr, pnData, 1, nDataSetSize) != ADI_HAL_OK) {
        return ADPD400xDrv_ERROR;
      }
      break;
    default:
      return ADPD400xDrv_ERROR;
    }
  }else{
     return ADPD400xDrv_ERROR;
  }

  return ADPD400xDrv_SUCCESS;
}

/** @brief  Synchronous register read from the ADPD400x
*           See data sheet for explanation of register
*
* @param  *pnData:     Pointer to 32-bit register data value
* @param  nSlotNum:    8-bit slot number
* @param  nSignalDark: 8-bit signal/dark flag
* @param  nChNum:      8-bit channel number info
* @return int16_t A 16-bit integer: 0 - success; < 0 - failure
*/
int16_t Adpd400xDrvReadRegData(uint32_t *pnData, ADPD400xDrv_SlotNum_t nSlotNum, uint8_t nSignalDark, uint8_t nChNum) {
  uint8_t anRxData[4];
  uint8_t txData[2];
  uint16_t nAddr, i = 0;
  uint16_t nIntStatus;
  uint16_t nTmpAddr = 0;

  Adpd400xDrvRegRead(ADPD400x_REG_INT_STATUS_DATA, &nIntStatus);

  /**
  This function Adpd400xDrvReadRegData() calls Adpd400xDrvRegRead()
  It also calls either the sub-function Adpd400x_SPI_Receive() or Adpd400x_I2C_TxRx()
  based on the communication mode ADPD400x_SPI_BUS / ADPD400x_I2C_BUS
  */

  if(nIntStatus & (1<<nSlotNum)) {

    nAddr = ADPD400x_REG_SIGNAL1_L_A + (nSlotNum << 3) + (nSignalDark << 2) + (nChNum << 1);

    if (nAdpd400xCommMode == ADPD400x_SPI_BUS) {
      nTmpAddr = (nAddr << 1 ) & ADPD400x_SPI_READ; // To set the last bit low for read operation

      txData[i++] = (uint8_t)(nTmpAddr >> 8);
      txData[i++] = (uint8_t)(nTmpAddr);

      if (Adpd400x_SPI_Receive(txData, anRxData, i, 4)!= ADI_HAL_OK) {
        return ADPD400xDrv_ERROR;
      }
    } else if (nAdpd400xCommMode == ADPD400x_I2C_BUS) {
      if (nAddr > 0x7F) {
        txData[i++] = (uint8_t)((nAddr >> 8)|0x80);
        txData[i++] = (uint8_t)nAddr;
      } else {
        txData[i++] = (uint8_t)nAddr;
      }

      if (Adpd400x_I2C_TxRx((uint8_t *) txData, (uint8_t *) anRxData, i, 4) != ADI_HAL_OK) {
        return ADPD400xDrv_ERROR;
      }
    } else {
      return ADPD400xDrv_ERROR;
    }

    *pnData = (anRxData[0] << 8) + anRxData[1] + (anRxData[2] << 24) + (anRxData[3] << 16);
    return ADPD400xDrv_SUCCESS;
  } else {
    return ADPD400xDrv_ERROR;
  }
}

/**
* @brief Set the LED current level (pulse peak value)
*        See data sheet for explanation of registers
*
* @param        nLedCurrent:     0 --> disable
*                                1 --> 3mA
*                             0x7f --> 200mA
* @param        nLedId:          1 --> LED_1
*                                2 --> LED_2
*                                3 --> LED_3
*                                4 --> LED_4
* @param        nSlotNum: The desired slot number (any of slots A(0) to L(11)) is passed
*
* @return int16_t A 16-bit integer: 0 - success; < 0 - failure
*/
int16_t Adpd400xDrvSetLedCurrent(uint16_t nLedCurrent, ADPD400xDrv_LedId_t nLedId, ADPD400xDrv_SlotNum_t nSlotNum) {
  int16_t nRetCode = ADPD400xDrv_SUCCESS;
  uint16_t nMask;
  uint16_t nReg;
  uint8_t nBitPos;
  uint16_t nAdpd400xData;

  if (nLedId < ADPD400xDrv_LED1 || nLedId > ADPD400xDrv_LED4) {
    return ADPD400xDrv_ERROR;
  }

  nReg = ADPD400x_REG_LED_POW12_A + ((nLedId - 1)>>1) + nSlotNum*ADPD400x_SLOT_BASE_ADDR_DIFF;

  if (nLedCurrent > 0x7F) {
    return ADPD400xDrv_ERROR;
  }

  if((nLedId == ADPD400xDrv_LED1) ||(nLedId == ADPD400xDrv_LED3)) {
      nMask = ~BITM_LED_POW12_A_LED_CURRENT1_A;
      nBitPos = BITP_LED_POW12_A_LED_CURRENT1_A;
  } else {
      nMask = ~BITM_LED_POW12_A_LED_CURRENT2_A;
      nBitPos = BITP_LED_POW12_A_LED_CURRENT2_A;
  }


  // Read the current register value
  if (Adpd400xDrvRegRead(nReg, &nAdpd400xData) != ADPD400xDrv_SUCCESS)
    return ADPD400xDrv_ERROR;

  nAdpd400xData = (nAdpd400xData & nMask) | (nLedCurrent << nBitPos);


  if (Adpd400xDrvRegWrite(nReg, nAdpd400xData) != ADPD400xDrv_SUCCESS)
    return ADPD400xDrv_ERROR;

  return nRetCode;
}

/**
* @brief Get the LED current level (pulse peak value)
*        See data sheet for explanation of registers
*
* @param        *pLedCurrent:    0 --> disable
*                                1 --> 3mA
*                             0x7f --> 200mA
* @param        nLedId:          1 --> LED_1
*                                2 --> LED_2
*                                3 --> LED_3
*                                4 --> LED_4
* @param        nSlotNum: The desired slot number (any of slots A(0) to L(11)) is passed
*
* @return int16_t A 16-bit integer: 0 - success; < 0 - failure
*/
int16_t Adpd400xDrvGetLedCurrent(uint16_t *pLedCurrent, ADPD400xDrv_LedId_t nLedId, ADPD400xDrv_SlotNum_t nSlotNum) {
  int16_t nRetCode = ADPD400xDrv_SUCCESS;
  uint16_t nMask;
  uint16_t nReg;
  uint8_t nBitPos;
  uint16_t nAdpd400xData;

  if (nLedId < ADPD400xDrv_LED1 || nLedId > ADPD400xDrv_LED4) {
    return ADPD400xDrv_ERROR;
  }

  nReg = ADPD400x_REG_LED_POW12_A + ((nLedId - 1)>>1) + nSlotNum*ADPD400x_SLOT_BASE_ADDR_DIFF;

  if((nLedId == ADPD400xDrv_LED1) ||(nLedId == ADPD400xDrv_LED3)) {
      nMask = BITM_LED_POW12_A_LED_CURRENT1_A;
      nBitPos = BITP_LED_POW12_A_LED_CURRENT1_A;
  } else {
      nMask = BITM_LED_POW12_A_LED_CURRENT2_A;
      nBitPos = BITP_LED_POW12_A_LED_CURRENT2_A;
  }

  // Read the current register value
  if (Adpd400xDrvRegRead(nReg, &nAdpd400xData) != ADPD400xDrv_SUCCESS)
    return ADPD400xDrv_ERROR;

  *pLedCurrent = (nAdpd400xData & nMask) >> nBitPos;

  return nRetCode;
}

/**  @brief Stops all AFE operations and resets the device to its default values,
 *          and open the ADPD driver
 *          See data sheet for explanation
 *
 * @return int16_t success
 */
int16_t Adpd400xDrvSoftReset(void) {
  return Adpd400xDrvOpenDriver();
}

/** @brief Driver Initialization.
*
* @return None
*/
static void _Adpd400xDrvInit(void) {
  uint8_t i;
  gsTotalSlotSize = 0;
  gsHighestSelectedSlot = 0;
  for (i = 0; i < SLOT_NUM; i++)  {
    gsSlot[i].activeSlot = 1;
    gsSlot[i].pre_activeSlot = 1;
    gsSlot[i].channelNum = 1;
    gsSlot[i].slotFormat = 0x03;
    _Adpd400xDrvSlotSaveCurrentSetting(i);
  }
  gsSlot[0].activeSlot = 1;   // special case that slot0 is enable (Reg0x10)
}

/** @brief  Set ADPD to FIFO interrupt mode, so that the data can be read when the number of samples in FIFO
 *           crossed a threshold (watermark)
*
* @return int16_t A 16-bit integer: 0 - success; < 0 - failure
*/
static int16_t _Adpd400xDrvSetInterrupt() {
  int16_t nRetCode = ADPD400xDrv_SUCCESS;
  uint16_t nRegValue;
  nRetCode |= Adpd400xDrvRegRead(ADPD400x_REG_INT_ENABLE_XD, &nRegValue);
  /**  Define INTERRUPT_ENABLE macro for the definition of FIFO_INT_ENA macros. */
  nRegValue = (nRegValue & 0x7FFF) | FIFO_TH_INT_ENA;
  nRetCode |= Adpd400xDrvRegWrite(ADPD400x_REG_INT_ENABLE_XD, nRegValue);  // Enable INTX

  /* For now, set INTY as well. Check later if this is required. */
  nRetCode |= Adpd400xDrvRegRead(ADPD400x_REG_INT_ENABLE_YD, &nRegValue);
  /**  Define INTERRUPT_ENABLE macro for the definition of FIFO_INT_ENA macros. */
  nRegValue = (nRegValue & 0x9FFF) | FIFO_UF_INT_ENA | FIFO_OF_INT_ENA;
  nRetCode |= Adpd400xDrvRegWrite(ADPD400x_REG_INT_ENABLE_YD, nRegValue);  // Enable INTY
  return nRetCode;
}

/** @brief Set device to Idle mode. This mode can be used for powering down device or writing the registers
           See data sheet for explanation.
*
* @return int16_t A 16-bit integer: 0 - success; < 0 - failure
*/
static int16_t _Adpd400xDrvSetIdleMode() {
  uint16_t nRetCode, nTemp;
  /** Set to standby Mode */
  nRetCode = Adpd400xDrvRegRead(ADPD400x_REG_OPMODE, &nTemp);
  nTemp &= (~BITM_OPMODE_OP_MODE);
  nTemp |= ADPD400x_OP_IDLE_MODE;
  nRetCode = Adpd400xDrvRegWrite(ADPD400x_REG_OPMODE, nTemp);
  return nRetCode;
}

/** @brief Get status of 12 slots.
 *  Checks if only channel-1 or both channels are enabled.
 *  Checks the sample type:
 *  	Impulse mode: See data sheet for explanation of this mode.
 *  	Standard sampling mode: See data sheet for explanation of this mode.
 *  	Set slot format, slot size, total slot size and decimation factor for each slot
 *
*
* @return None
*/
static void _Adpd400xDrvGetSlotInfo()  {
  uint16_t tempD, value, slotSize;
  gsTotalSlotSize = 0;
  
  uint16_t temp16;
  uint32_t sampleFrq, lfOSC;
  Adpd400xDrvRegRead(ADPD400x_REG_SYS_CTL, &temp16);
  temp16 &= BITM_SYS_CTL_LFOSC_SEL;
  temp16 >>= BITP_SYS_CTL_LFOSC_SEL;
  if (temp16 == 1)
    lfOSC = 1000000; /* 1M clock */
  else
    lfOSC = 32000; /* 32k clock */

  Adpd400xDrvRegRead32B(ADPD400x_REG_TS_FREQ, &sampleFrq);
  uint16_t nSamplingRate = lfOSC / sampleFrq;

  Adpd400xDrvRegRead(ADPD400x_REG_OPMODE, &value);
  gsHighestSelectedSlot = (value & BITM_OPMODE_TIMESLOT_EN) >> BITP_OPMODE_TIMESLOT_EN;

  for (uint8_t i = 0; i < SLOT_NUM; i++)  {
    // take care of Impulse mode here.
    // if Impulse mode, read out from other register
    //    ....
    Adpd400xDrvRegRead(ADPD400x_REG_TS_CTRL_A+i*ADPD400x_SLOT_BASE_ADDR_DIFF, &value);
    // Update channel Information
    if((value & BITM_TS_CTRL_A_CH2_EN_A) == BITM_TS_CTRL_A_CH2_EN_A)
      gsSlot[i].channelNum = 3;
    else
      gsSlot[i].channelNum = 1;

    // Get the sample type from register value
    value = (value & BITM_TS_CTRL_A_SAMPLE_TYPE_A ) >> BITP_TS_CTRL_A_SAMPLE_TYPE_A;

    if(value == 0x03){
      Adpd400xDrvRegRead(ADPD400x_REG_COUNTS_A+i*ADPD400x_SLOT_BASE_ADDR_DIFF, &value);
      value = (value & BITM_COUNTS_A_NUM_INT_A) >> BITP_COUNTS_A_NUM_INT_A;
      /* Set the bit-9 as 1 to indicate impulse mode */
      gsSlot[i].slotFormat = value | (0x01 << BITP_COUNTS_A_NUM_INT_A);
    } else {
      Adpd400xDrvRegRead(ADPD400x_REG_DATA1_A+i*ADPD400x_SLOT_BASE_ADDR_DIFF, &value);
      gsSlot[i].slotFormat = (value & BITM_DATA1_A_SIGNAL_SIZE_A) >> BITP_DATA1_A_SIGNAL_SIZE_A;
      tempD = (value & BITM_DATA1_A_DARK_SIZE_A) >> BITP_DATA1_A_DARK_SIZE_A;
      gsSlot[i].slotFormat |= (tempD << 4);
      Adpd400xDrvRegRead(ADPD400x_REG_DATA2_A+i*ADPD400x_SLOT_BASE_ADDR_DIFF, &value);
      tempD = (value & BITM_DATA2_A_LIT_SIZE_A) >> BITP_DATA2_A_LIT_SIZE_A;
      gsSlot[i].slotFormat |= (tempD << 12);
    }

    Adpd400xDrvRegRead(ADPD400x_REG_DECIMATE_A + i * ADPD400x_SLOT_BASE_ADDR_DIFF, &value);
    // extracting r_decimation factor from the register value
    gsSlot[i].decimation = ((value & 0x7f0) >> 4) + 1;  //decimation factor = decimation_regvalue[4:10] + 1

    if (gsSlot[i].activeSlot == 1 && i <= gsHighestSelectedSlot)  {
      if (gsSlot[i].slotFormat & 0x100)  {
        slotSize = gsSlot[i].slotFormat & 0xFF;   // Impulse mode
        slotSize *= 2; // Impulse data size always a word
      } else {
        slotSize = ((gsSlot[i].slotFormat & 0xF) +
                      ((gsSlot[i].slotFormat >> 4) & 0xF) +
                       ((gsSlot[i].slotFormat >> 12) & 0xF));
        if (gsSlot[i].channelNum == 3)    // 2 channels are active
          slotSize *= 2;
      }
      gsTotalSlotSize += slotSize;
      if(slotSize > 0)
      {
        gsSlot[i].odr = nSamplingRate / gsSlot[i].decimation;
        if(gsSlot[i].odr >= gAdpdSlotMaxODR)
        {
           gAdpdSlotMaxODR = gsSlot[i].odr;
        }
      }
      else {
        gsSlot[i].odr = 0;
      }
    }
  }
}

/** @brief  Get the output data rate of the device. Checks the low frequency oscillator used.
 *          Based on that, the sampling rate is found out and the ODR is calculated as
 *          Sampling Rate/Decimation
 *          See data sheet for explanation
 *
 * @param  nSlotNum: The desired slot number (any of slots A(0) to L(11)) is passed
 * @return None
 */
static void _Adpd400xDrvGetDataOutputRate(uint8_t nSlotNum)  {
  uint16_t temp16, addrOffset;
  uint32_t sampleFrq, lfOSC;
  addrOffset = nSlotNum * ADPD400x_SLOT_BASE_ADDR_DIFF;
  Adpd400xDrvRegRead(ADPD400x_REG_SYS_CTL, &temp16);
  temp16 &= BITM_SYS_CTL_LFOSC_SEL;
  temp16 >>= BITP_SYS_CTL_LFOSC_SEL;
  if (temp16 == 1)
    lfOSC = 1000000;  // 1M clock
  else
    lfOSC = 32000;    // 32k clock
  Adpd400xDrvRegRead32B(ADPD400x_REG_TS_FREQ, &sampleFrq);
  sampleFrq = lfOSC / sampleFrq;
  Adpd400xDrvRegRead(ADPD400x_REG_DECIMATE_A + addrOffset, &temp16);
  temp16 &= BITM_DECIMATE_A_DECIMATE_FACTOR_A;
  temp16 >>= BITP_DECIMATE_A_DECIMATE_FACTOR_A;
  gsOutputDataRate = (uint16_t)(sampleFrq / (temp16 + 1));
}

/** @brief  Save current LED settings for a slot into pre_active_setting array.
*          See data sheet for details on the register contents
*
* @param  nSlotNum: The desired slot number (any of slots A(0) to L(11)) is passed
* @return None
*/
static void _Adpd400xDrvSlotSaveCurrentSetting(uint8_t nSlotNum) {
  uint16_t addrOffset, addr;
  addrOffset = nSlotNum * ADPD400x_SLOT_BASE_ADDR_DIFF;
  addr = addrOffset + ADPD400x_REG_TS_CTRL_A;
  Adpd400xDrvRegRead(addr, &pre_active_setting[nSlotNum][0]);
  addr = addrOffset + ADPD400x_REG_TS_PATH_A;
  Adpd400xDrvRegRead(addr, &pre_active_setting[nSlotNum][1]);
  addr = addrOffset + ADPD400x_REG_COUNTS_A;
  Adpd400xDrvRegRead(addr, &pre_active_setting[nSlotNum][2]);
  addr = addrOffset + ADPD400x_REG_LED_PULSE_A;
  Adpd400xDrvRegRead(addr, &pre_active_setting[nSlotNum][3]);
  addr = addrOffset + ADPD400x_REG_MOD_PULSE_A;
  Adpd400xDrvRegRead(addr, &pre_active_setting[nSlotNum][4]);
  addr = addrOffset + ADPD400x_REG_DIGINT_LIT_A;
  Adpd400xDrvRegRead(addr, &pre_active_setting[nSlotNum][5]);
}

/** @brief  Applies the previous settings to the slot by writing to its registers.
*          See data sheet for details on the register contents
*
* @param  nSlotNum: The desired slot number (any of slots A(0) to L(11)) is passed
* @return None
*/
static void _Adpd400xDrvSlotApplyPreviousSetting(uint8_t nSlotNum) {
  uint16_t addrOffset, addr;
  addrOffset = nSlotNum * ADPD400x_SLOT_BASE_ADDR_DIFF;
  addr = addrOffset + ADPD400x_REG_TS_CTRL_A;
  Adpd400xDrvRegWrite(addr, pre_active_setting[nSlotNum][0]);
  addr = addrOffset + ADPD400x_REG_TS_PATH_A;
  Adpd400xDrvRegWrite(addr, pre_active_setting[nSlotNum][1]);
  addr = addrOffset + ADPD400x_REG_COUNTS_A;
  Adpd400xDrvRegWrite(addr, pre_active_setting[nSlotNum][2]);
  addr = addrOffset + ADPD400x_REG_LED_PULSE_A;
  Adpd400xDrvRegWrite(addr, pre_active_setting[nSlotNum][3]);
  addr = addrOffset + ADPD400x_REG_MOD_PULSE_A;
  Adpd400xDrvRegWrite(addr, pre_active_setting[nSlotNum][4]);
  addr = addrOffset + ADPD400x_REG_DIGINT_LIT_A;
  Adpd400xDrvRegWrite(addr, pre_active_setting[nSlotNum][5]);
}


/** @brief  Applies the sleep mode settings to a slot.
*           This API is called when a slot is to be put into a sleep mode
*           See data sheet for details on the register contents
*
* @param  nSlotNum: The desired slot number (any of slots A(0) to L(11)) is passed
* @return None
*/
static void _Adpd400xDrvSlotApplySkipSetting(uint8_t nSlotNum) {
  uint16_t addrOffset, value, addr;
  addrOffset = nSlotNum * ADPD400x_SLOT_BASE_ADDR_DIFF;
  addr = addrOffset + ADPD400x_REG_TS_CTRL_A;
  value = 0x3000;   //<! impulse mode with offset = 0;
  Adpd400xDrvRegWrite(addr, value);

  addr = addrOffset + ADPD400x_REG_TS_PATH_A;
  value = 0x00E6;   //!< precond = 0, not INT
  Adpd400xDrvRegWrite(addr, value);

  addr = addrOffset + ADPD400x_REG_COUNTS_A;
  value = 0x0000;   //!< num_int = 0
  Adpd400xDrvRegWrite(addr, value);

  addr = addrOffset + ADPD400x_REG_LED_PULSE_A;
  value = 0x0000;   //!< LED width = 0
  Adpd400xDrvRegWrite(addr, value);

  addr = addrOffset + ADPD400x_REG_MOD_PULSE_A;
  value = 0x0000;   //!< modulation width = 0
  Adpd400xDrvRegWrite(addr, value);

  addr = addrOffset + ADPD400x_REG_DIGINT_LIT_A;
  value = 0x0000;   //!< lit offset = 0
  Adpd400xDrvRegWrite(addr, value);
}

/** @brief  Set the slot data size
* @param  nSlotNum: The desired slot number (any of slots A(0) to L(11)) is passed
* @param  nSlotFormat: Dark, Signal format. See data sheet for details
* @return none
*/
static void _Adpd400xDrvSetSlotSize(uint8_t nSlotNum, uint16_t nSlotFormat) {
  uint16_t addr1, addr2, value1, value2;

  //!< nSlotNum starts from 0, to 11. Index starts from 0.
  if(nSlotFormat & 0x100) {
    addr1 = nSlotNum * ADPD400x_SLOT_BASE_ADDR_DIFF + ADPD400x_REG_COUNTS_A;
    Adpd400xDrvRegRead(addr1, &value1);
    value1 &= (~BITM_COUNTS_A_NUM_INT_A);
    value1 |= (nSlotFormat & 0xFF) << BITP_COUNTS_A_NUM_INT_A;
    nSlotFormat &= 0xff;
  } else {
    addr1 = nSlotNum * ADPD400x_SLOT_BASE_ADDR_DIFF + ADPD400x_REG_DATA1_A;
    Adpd400xDrvRegRead(addr1, &value1);
    value1 &= (~BITM_DATA1_A_DARK_SIZE_A);
    value1 &= (~BITM_DATA1_A_SIGNAL_SIZE_A);
    value1 |= (nSlotFormat & 0x0F);
    value2 = (nSlotFormat & 0xF0) << (BITP_DATA1_A_DARK_SIZE_A - 4);
    value1 |= value2;
    // Get Lit size
    addr2 = nSlotNum * ADPD400x_SLOT_BASE_ADDR_DIFF + ADPD400x_REG_DATA2_A;
    Adpd400xDrvRegRead(addr2, &value2);
    value2 &= (~BITM_DATA2_A_LIT_SIZE_A);
    value2 |= ((nSlotFormat & 0xF000) >> 12);
    // Write value to register
    Adpd400xDrvRegWrite(addr2, value2);
    nSlotFormat &= 0xff;
  }
  gsSlot[nSlotNum].slotFormat = nSlotFormat;
  Adpd400xDrvRegWrite(addr1, value1);

  gsTotalSlotSize = 0;          //!< reset the sum size. Indicate a slot change
}

/** @brief  Calculate the fifo level to be configured to the ADPD FIFO with,
            based on the sample size from the slots
* @return uint16_t A 16-bit unsigned integer:which is the number of samples in the Fifo
*/
static uint16_t _FifoLevel(void) {
  uint16_t nTemp = 0;
  uint16_t nSampleSize = 0;

 // Get Fifo status byte register details
  Adpd400xDrvRegRead(ADPD400x_REG_FIFO_STATUS_BYTES, &nTemp);

  // Calculate byte size will append in the fifo for optional status
  while(nTemp != 0){
    nTemp = nTemp & (nTemp - 1);
    nFifoStatusByte++;
    if(nTemp == 0 || nTemp > 0x1FF)
      break;
  }

  //Calculating LCM for the decimations of all 12 slots
  gnLcmValue = calculate_lcm(gsSlot, gsHighestSelectedSlot);
  //total_samples_size = get_max_sample_size(datapattern_length, &slot_sz[0], &slot_channel_num[0], highest_slot_num);
  nSampleSize = get_max_sample_size(gnLcmValue, gsSlot, gsHighestSelectedSlot);


  nSampleSize += nFifoStatusByte;
  adpd4000_buff_reset(nSampleSize);
  nReadSequence = 1;
  nInterruptSequence = 1;
  nWriteSequence = 1;
  nSampleSize = get_samples_size(1, gsSlot, &nInterruptSequence,
                                  gnLcmValue, gsHighestSelectedSlot);
  return nSampleSize;
}

