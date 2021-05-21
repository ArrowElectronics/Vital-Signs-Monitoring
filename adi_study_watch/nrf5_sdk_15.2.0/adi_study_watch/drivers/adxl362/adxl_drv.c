/**
    ******************************************************************************
    * @addtogroup Device_Drivers Device Drivers
    * @{
    * @file     adxl_drv.c
    * @author   ADI
    * @version  V2.0
    * @date     20-October-2015
    * @brief    Reference design device driver to access ADI ADXL chip.
    ******************************************************************************
    * @attention
    ******************************************************************************
*/
/******************************************************************************
*                                                                             *
* License Agreement                                                           *
*                                                                             *
* Copyright (c) 2019 Analog Devices Inc.                                      *
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
* This software is intended for use with the ADXL and derivative parts        *
* only                                                                        *
*                                                                             *
******************************************************************************/

/******************************************************************************
* Version 0.9, Sept 24, 2014                                                  *
*   Change FIFO filled threshold                                              *
* Version 1.0, Oct 01, 2014                                                   *
*   Add efuse function                                                        *
* Version 1.1, Oct 29, 2014                                                   *
*   Add timestamp output                                                      *
* Version 1.2, Jan 28, 2015                                                   *
*   Fixed timestamp related bug                                               *
* Version 1.3, Mar  9, 2015                                                   *
*   Added setting of ODR and watermark at init                                *
* Version 1.3.1, Apr 2, 2015                                                  *
*   Fixed bug with timestamp when the watermark is greater than 1             *
* Version 1.3.2, Jun 17, 2015                                                 *
*   Removed unused argument of AdxlISR()                                      *
* Version 2.0, Oct 12, 2015                                                   *
*   Simplified the driver code by moving the ring buffer to the application   *
*   New functions and API added                                               *
******************************************************************************/


/* ------------------------- Includes -------------------------------------- */
#include <stdint.h>
#include <stdio.h>
#include <assert.h>
#include "Adxl362.h"
#include "printf.h"
#include "nrf_log.h"

/* ------------------------- Private Variables ------------------------------ */

// External clock must be disabled
// Normal operation (reset default). Current consumption is 1.8 uA
//                       (if ODR = 100 Hz & Vdd = 2v) in this mode.
// Not necessary in this application.
// Acceleromter will go to sleep mode if inactivity is detected.
// Measurement mode must be enabled.
static uint8_t powerCtlWord  = PRM_EXT_CLK_DISABLE     |
                               PRM_LOW_NOISE_LOW_NOISE |
                               PRM_WAKEUP_DISABLE      |
                               PRM_AUTOSLEEP_ENABLE    |
                               PRM_MEASURE_MEASURE_MODE;
static uint8_t gnAdxlFifoWaterMark = 0;
/* ------------------------- Public Function Prototypes -------------------- */
int16_t AdxlDrvOpenDriver(uint16_t nInRate, uint8_t nAdxlWaterMark);
int16_t AdxlDrvCloseDriver(void);
void AdxlISR();
uint16_t gnAdxlFifoLevel = 0;
static void ClearAdxlFifo();
extern void adxl_buff_reset(void);
static uint32_t gnAdxlISRCnt = 0;

/* ------------------------- Public Variables ------------------------------ */
uint8_t anXYZDataArray[2*ADXL_MAXDATASETSIZE + SPI_ADXL_DUMMY_BYTES];

/* ------------------------- Private Function Prototypes -------------------- */
static void (*gpfnAdxlCallBack)();
/*****************************************************************************/
/** @addtogroup Adxl_Driver  ADXL
  Source file contains driver task for wearable device framework.
  This will read data from senor devices and put them into data buffers.
  @{
 */
/*****************************************************************************/
/** @brief Read Adxl362 FIFO Data based on Fifo samples
  *
  * @param[in]  nFifoSamples FIFO 8-bit sample count
  * @param[out]  *panXDataArray Pointer to a 16-bit register X axis data value
  * @param[out]  *panYDataArray Pointer to a 16-bit register Y axis data value
  * @param[out]  *panZDataArray Pointer to a 16-bit register Z axis data value
  * @return  int16_t A 16-bit integer: 0 - success; < 0 - failure
  */
static int16_t ReadAdxl362Fifo(uint8_t nFifoSamples,
                               uint16_t *panXDataArray,
                               uint16_t *panYDataArray,
                               uint16_t *panZDataArray) {
    uint8_t nBytesPerSample = 6;
    uint16_t FifoLenghtBytes = 0;
    FifoLenghtBytes = nBytesPerSample * nFifoSamples;
    GetFifoXYZ(FifoLenghtBytes, panXDataArray, panYDataArray, panZDataArray);
    return 0;
}

static void ClearAdxlFifo()
{
  uint16_t nBytes;
  nBytes = GetFifoEntriesAdxl362() << 1;
    while (nBytes > 0) {
      if(nBytes > (2*ADXL_MAXDATASETSIZE))
      {
        GetFifo(2*ADXL_MAXDATASETSIZE, anXYZDataArray);
        nBytes -= (2 * ADXL_MAXDATASETSIZE);
      }
      else
      {
        /*read the residual samples in adxl and break the loop*/
        GetFifo(nBytes, anXYZDataArray);
        break;
      }
    }
}
/** @brief  Check Samples in the FIFO
  *
  * @return uint16_t A 16-bit integer: 0 - success; < 0 - failure
  */
static uint16_t checkSamplesInFIFO() {
    uint16_t nFifoSamples = 0;
    nFifoSamples = GetFifoEntriesAdxl362() / 3;
    return nFifoSamples;
}

/** @brief  Adxl362 Set Off for power control
  *
  * @return int32_t A 16-bit integer: 0 - success; < 0 - failure
  */
static int32_t Adxl362SetOff() {
    SetPowerControlAdxl362(powerCtlWord & 0xFC);
    return 0;
}

/** @brief  Adxl362 soft reset
  *
  * @return None
  */
void AdxlDrvSoftReset() {

    SetSoftRestAdxl362();
    MCU_HAL_Delay(1000);

}

/** @brief  Adxl362 Device Initialization
  *
  * @param  nInRate 8-bit Input sample rate
  * @param  nAdxlWaterMark 8-bit Watermark of ADXL FIFO
  * @return int32_t A 16-bit integer: 0 - success; < 0 - failure
  */
static int32_t AdxlDeviceInit(uint16_t nInRate, uint8_t nAdxlWaterMark) {
    uint8_t nFilterCtlWord;
//    uint32_t nFifoLevel;

    GPIO_IRQ_ADXL362_Disable();
    // g_ADXLDataReady = 0;
    // FILTER CONTROL REGISTER (Address: 0x2C)
    // Measurement Range Selection = �8 g
    // Not going to use an external clock signal to control ADXL362 sampling.
    nFilterCtlWord = PRM_RANGE_8GEE           | PRM_HALF_BW_ENABLE |
                     PRM_EXT_SAMPLE_DISABLE;
    if (nInRate <= 12) {
        nFilterCtlWord |= PRM_ODR_12_5;
    } else if (nInRate <= 25) {
        nFilterCtlWord |= PRM_ODR_25;
    } else if (nInRate <= 50) {
        nFilterCtlWord |= PRM_ODR_50;
    } else if (nInRate <= 100) {
        nFilterCtlWord |= PRM_ODR_100;
    } else if (nInRate <= 200) {
        nFilterCtlWord |= PRM_ODR_200;
    } else {    // if (nInRate <= 400)
        nFilterCtlWord |= PRM_ODR_400;
    }
    // ACTIVITY/INACTIVITY CONTROL REGISTER (Address: 0x27)     // Not used
    // LoopLink mode means the uC must read status register to clean interrrupts
    // Inactivity detection function operates in referenced mode.
    // This parameter enables the inactivity (underthreshold) functionality.
    // Activity detection function operates in referenced mode.
    // This parameter enables the activity (overthreshold) functionality.
    /*  uint8_t AAICtlWord = PRM_LOOPLINK_LINKED_MODE |
                         PRM_INACT_REF_REF_MODE   |
                         PRM_INACT_EN_DISABLE     |
                         PRM_ACT_REF_REF_MODE     |
                         PRM_ACT_EN_DISABLE;
    */

    //SetSoftRestAdxl362();  // ADXL362 software reset.
    //MCU_HAL_Delay(1000);

    // This function sets the FIFO's parameters.
    // Disable the temperature measurement.
    // Enable the FIFO in Stream mode.
    // The number of samples to store and FIFO nAdxlWaterMark.
    // 3 (axis X, axis Y and axis Z) * 4 (FiFOsamples) = 12.
    SetFifoConfAdxl362(PRM_FIFO_TEMP_DISABLE,
                       PRM_FIFO_MODE_STREAM,
                       3 * nAdxlWaterMark - 1);
    // This function configures INT1 interrupt pin.
    // First parameter means if INT1 = 1 ==> no event, if INT1 = 0 ==> new event
    // Awake interrupt must be disabled.
    // Inactivity interrupt must be disabled.
    // Activity interrupt must be disabled.
    // FIFO overrrun interrupt must be disabled.
    // FIFO nAdxlWaterMark is enabled.
    // FIFO ready interrupt must be disabled.
    // Data ready interrupt must be disabled.
    SetIntMap1Adxl362(PRM_INT_LOW_ACTIVE_LOW     |
                      PRM_AWAKE_DISABLE          |
                      PRM_INACT_DISABLE          |
                      PRM_ACT_DISABLE            |
                      PRM_FIFO_OVERRUN_DISABLE   |
                      PRM_FIFO_WATERMARK_ENABLE  |
                      PRM_FIFO_READY_DISABLE     |
                      PRM_DATA_READY_DISABLE);
    // This function configures the Filter Control Register
    SetFilterAdxl362(nFilterCtlWord);
    // This function configures the Power Control Register but the
    // accelerometer must remain in standby.
    SetPowerControlAdxl362(powerCtlWord & 0xFC);
//    nFifoLevel = checkSamplesInFIFO();
//    while (nFifoLevel-- > 0) {
//        GetFifo(2*ADXL_MAXDATASETSIZE, anXYZDataArray);
//    }
    ClearAdxlFifo();

    GPIO_IRQ_ADXL362_Enable();
    return 0;
}

/** @brief  Adxl Driver Probe function to get AD,MST and ID value
  *
  * @return int16_t A 16-bit integer: 0 - success; < 0 - failure
  */
int16_t AdxlDrvProbe(void) {
    uint8_t nAdxl362Ad;
    uint8_t nAdxl362Mst;
    uint8_t nAdxl362ParId;
    // This function obtain the AD value. if this value is not 0xAD,we have not
    // read correctly the ADXL362, that means we should check the SPI
    // communication, code, supply, connections...
    nAdxl362Ad = GetDevIdAdAdxl362();
    // This function obtain the MST value. if this value is not 0x1D,  we have
    // not read correctly the ADXL362, that means we should check the SPI
    // communication, code, supply, connections...
    nAdxl362Mst = GetDevIdMstAdxl362();
    // This function obtain the ID value. if this value is not 0xF2,  we have
    // not read correctly the ADXL362, that means we should check the SPI
    // communication, code, supply, connections...
    nAdxl362ParId = GetPartIdAdxl362();
    if ((nAdxl362Ad != 0xAD) || (nAdxl362Mst != 0x1D) ||
        (nAdxl362ParId != 0xF2)) {
        return ADXLDrv_ERROR;
    }
    return ADXLDrv_SUCCESS;
}

/** @brief  Adxl Open Driver, setting up the interrupt and SPI lines
  *
  * @param  nInRate 8-bit Input sample rate
  * @param  nAdxlWaterMark  8-bitWatermark of ADXL FIFO
  * @return int16_t A 16-bit integer: 0 - success; < 0 - failure
  */
int16_t AdxlDrvOpenDriver(uint16_t nInRate, uint8_t nAdxlWaterMark) {
    if (AdxlDrvProbe() != ADXLDrv_SUCCESS) {
        return ADXLDrv_ERROR;
    }
    AdxlDeviceInit(nInRate, nAdxlWaterMark);

    return ADXLDrv_SUCCESS;
}

/** @brief  Set Adxl operating mode
  *
  * @param  nOpMode operating mode such as standby and measurement mode.
  * @return int16_t A 16-bit integer: 0 - success; < 0 - failure
  */
int16_t AdxlDrvSetOperationMode(uint8_t nOpMode) {
     uint8_t nPowerCtlWord;
//    uint32_t nFifoLevel;

     if(nOpMode == PRM_MEASURE_MEASURE_MODE)
      {
        adxl_buff_reset();
        ClearAdxlFifo();
      }
     nPowerCtlWord = GetPowerControlAdxl362();
     nPowerCtlWord &= (~PRM_MEASURE_MASK);
     nPowerCtlWord |= nOpMode;
     SetPowerControlAdxl362(nPowerCtlWord);

//    nFifoLevel = checkSamplesInFIFO();
//     while (nFifoLevel-- > 0) {
//        GetFifo(2*ADXL_MAXDATASETSIZE, anXYZDataArray);
//     }
     return ADXLDrv_SUCCESS;
}
/*int16_t AdxlDrvSetOn() {
    Adxl362SetOn();
    return ADXLDrv_SUCCESS;
}

int16_t AdxlDrvSetOff() {
    Adxl362SetOff();
    return ADXLDrv_SUCCESS;
}*/

/** @brief  Adxl Close Driver. Clear up before existing
  *
  * @return int16_t A 16-bit integer: 0 - success; < 0 - failure
  */
int16_t AdxlDrvCloseDriver() {
    Adxl362SetOff();
    return ADXLDrv_SUCCESS;
}

/** @brief  Register data ready callback

    @param  *pfAdxlDataReady  Function Pointer callback for the reg data
    @return None
*/
void AdxlDrvDataReadyCallback(void (*pfAdxlDataReady)()) {
    gpfnAdxlCallBack = pfAdxlDataReady;
}

/** @brief  Adxl interrupt service routine
  *
  * @return None
  */
void AdxlISR() {
    if (gpfnAdxlCallBack != NULL) {
        (*gpfnAdxlCallBack)();
        gnAdxlISRCnt++;
    }
}

/** @brief  Debug function. Read out debug info
* @param  None
* @return uint32_t* Debug info pointer
*/
uint32_t AdxlDrvGetDebugInfo() {
  return gnAdxlISRCnt;
}

/** @brief Read Adxl FIFO Data based on Fifo level Size
  *
  * @param[out]  *pnXDataArray Pointer to a 16-bit register X axis data value
  * @param[out]  *pnYDataArray Pointer to a 16-bit register Y axis data value
  * @param[out]  *pnZDataArray Pointer to a 16-bit register Z axis data value
  * @param[in]  nFifoLevelSize 16-bit integer FIFO level size
  * @return[out]  int16_t A 16-bit integer: 0 - success; < 0 - failure
  */
int16_t AdxlDrvReadFifoData(uint16_t *pnXDataArray, uint16_t *pnYDataArray,
                            uint16_t *pnZDataArray, uint16_t nFifoLevelSize) {
    // fifoLevel = checkSamplesInFIFO();
    /*
        if (AdxlDataReady == 0)
        return ADXLDrv_ERROR;
    */
    if (nFifoLevelSize != 0) {
        ReadAdxl362Fifo(nFifoLevelSize, pnXDataArray,
                        pnYDataArray, pnZDataArray);
    }
    return ADXLDrv_SUCCESS;
}

/** @brief Adxl Set parameter for Watermark and Fifo data set Size
  *
  * @param  eCommand Adxl Command Enum for Watermark value
  * @param  nValue 16-bit value for Fifo Level data set size
  * @return  int16_t A 16-bit integer: 0 - success; < 0 - failure
  */
int16_t AdxlDrvSetParameter(AdxlCommandStruct eCommand, uint16_t nValue) {
    uint16_t nInRate, nFifoSamples;
    uint8_t nFilterCtlWord, nTemperature, nFifoMode;
    if (eCommand == ADXL_WATERMARKING) {
        GetFifoConfAdxl362(&nTemperature, &nFifoMode, &nFifoSamples);
        SetFifoConfAdxl362(nTemperature,
                           nFifoMode,
                           3 * nValue - 1);
        gnAdxlFifoWaterMark = nValue;
    } else if (eCommand == ADXL_ODRRATE) {

       nInRate = nValue;
       nFilterCtlWord = GetFilterAdxl362() & (~PRM_ODR_MASK);
      if (nInRate <= 12) {
          nFilterCtlWord |= PRM_ODR_12_5;
      } else if (nInRate <= 25) {
          nFilterCtlWord |= PRM_ODR_25;
      } else if (nInRate <= 50) {
          nFilterCtlWord |= PRM_ODR_50;
      } else if (nInRate <= 100) {
          nFilterCtlWord |= PRM_ODR_100;
      } else if (nInRate <= 200) {
          nFilterCtlWord |= PRM_ODR_200;
      } else {    // if (nInRate <= 400)
          nFilterCtlWord |= PRM_ODR_400;
      }
      SetFilterAdxl362(nFilterCtlWord);
    } else {
        return ADXLDrv_ERROR;
    }
    return ADXLDrv_SUCCESS;
}

/** @brief      Adxl Get parameter for Watermark, Fifo data set Size and ODR Rate
  *
  * @param[in]   eCommand Adxl Command Enum for Watermark value
  * @param[out]  pnValue 16-bit value for Fifo Level data set size
  * @return      int16_t A 16-bit integer: 0 - success; < 0 - failure
  */
int16_t AdxlDrvGetParameter(AdxlCommandStruct eCommand, uint16_t *pnValue) {
    uint16_t nRate;
    if (eCommand == ADXL_WATERMARKING) {
        *pnValue = gnAdxlFifoWaterMark;
    } else if (eCommand == ADXL_FIFOLEVEL) {
        gnAdxlFifoLevel = checkSamplesInFIFO();
        *pnValue = gnAdxlFifoLevel;
    } else if (eCommand == ADXL_ODRRATE) {
        nRate = (GetFilterAdxl362()) & PRM_ODR_MASK;
        *pnValue = nRate;
    } else {
        return ADXLDrv_ERROR;
    }
    return ADXLDrv_SUCCESS;
}

/** @brief  External sample mode routine
  *
  * @param  nEnableExtSample PRM_EXT_SAMPLE_ENABLE=enable external sample,
  *                          PRM_EXT_SAMPLE_DISABLE=disable external sample
  * @return None
  */
void AdxlDrvExtSampleMode(uint8_t nEnableExtSample) {

    uint8_t nReg = GetFilterAdxl362();
    nReg &= (~PRM_EXT_SAMPLE_MASK);
    nReg |= nEnableExtSample;
    SetFilterAdxl362(nReg);

}

/** @brief  Synchronous register read from the ADXL
  *
  * @param  nAddr 8-bit register address
  * @param  *pnData Pointer to 8-bit register data value
  * @return int16_t A 16-bit integer: 0 - success; < 0 - failure
  */
int16_t AdxlDrvRegRead(uint8_t nAddr, uint8_t *pnValue) {
    uint8_t        nTxData[2] = {COMMAND_READ_REG};
    uint8_t        nRxData[SPI_ADXL_DUMMY_BYTES+3];

    nTxData[1] = nAddr;

    if (ADXL362_SPI_Receive(nTxData, nRxData, 2, 1)!= ADI_HAL_OK) {
        return ADXLDrv_ERROR;
    }
    *pnValue = nRxData[SPI_ADXL_DUMMY_BYTES+0];
    return ADXLDrv_SUCCESS;
}

/** @brief  Synchronous register write to the ADXL
  *
  * @param  nAddr 8-bit register address
  * @param  nValue 8-bit register data value
  * @return int16_t A 16-bit integer: 0 - success; < 0 - failure
  */
int16_t AdxlDrvRegWrite(uint8_t nAddr, uint8_t nValue) {
    uint8_t        nTxData[3] = {COMMAND_WRITE};

    nTxData[1] = nAddr;
    nTxData[2] = nValue;

    if (ADXL362_SPI_Transmit(nTxData, 3)!= ADI_HAL_OK) {
        return ADXLDrv_ERROR;
    }

    return ADXLDrv_SUCCESS;
}



/** @brief Self test of the ADXL Driver
  *
  * @return uint16_t A 16-bit integer: 0 - success; 1 - failure
  */
uint16_t AdxlDrvSelfTest() {
    uint8_t nLoopCnt;
    uint8_t nFilterCtlWord;
    // uint32_t ltck;
    int16_t nXclData[3];
    int16_t nXclDataSt[3];
    int16_t nXclScaledDiff[3];
    // Measurement Range Selection = �8 g
    // Not going to use an external clock signal to control ADXL362 sampling.
    // ODR = 100 Hz
    nFilterCtlWord = PRM_RANGE_8GEE           |
                     PRM_EXT_SAMPLE_DISABLE   |
                     PRM_ODR_100;
    // ADXL362 software reset.
    SetSoftRestAdxl362();
    MCU_HAL_Delay(10);
    // This function obtain the AD value. if this value is not 0xAD,
    // we have not read correctly the ADXL362, that means we should check
    // the SPI communication, code, supply, connections...
    uint8_t nAdxl362Ad = GetDevIdAdAdxl362();
    // This function obtain the MST value. if this value is not 0x1D,
    // we have not read correctly the ADXL362, that means we should check
    // the SPI communication, code, supply, connections...
    uint8_t nAdxl362Mst = GetDevIdMstAdxl362();
    // This function obtain the ID value. if this value is not 0xF2,
    // we have not read correctly the ADXL362, that means we should check
    // the SPI communication, code, supply, connections...
    uint8_t nAdxl362ParId = GetPartIdAdxl362();
    NRF_LOG_DEBUG("\r\nAD: Expected value = 0xAD, Read value = 0x%02X\r\n", nAdxl362Ad);
    NRF_LOG_DEBUG("MST: Expected value = 0x1D, Read value = 0x%02X\r\n", nAdxl362Mst);
    NRF_LOG_DEBUG("PartID: Expected value = 0xF2, Read value = 0x%02X\r\n",
          nAdxl362ParId);
    if ((nAdxl362Ad != 0xAD) || (nAdxl362Mst != 0x1D) ||
        (nAdxl362ParId != 0xF2)) {
        NRF_LOG_INFO("\r\n:FAIL. ADXL Expected and read values do not match\r\n");
        return 1;
    } else {
        NRF_LOG_DEBUG("\r\n:PASS. ADXL Expected and read values match\r\n");
    }
    NRF_LOG_INFO("\r\nPerforming ADXL self test routine...\r\nPleasedo notmove the device.\r\n");
    MCU_HAL_Delay(2000);
    // This function configures the Filter Control Register
    SetFilterAdxl362(nFilterCtlWord);
    // External clock must be disabled
    // Normal operation (reset default).
    // Current consumption is 1.8 uA (if ODR = 100 Hz & Vdd = 2v) in this mode
    // Not necessary in this application.
    // Run mode.
    SetPowerControlAdxl362(PRM_EXT_CLK_DISABLE      |
                           PRM_LOW_NOISE_NORMAL     |
                           PRM_WAKEUP_DISABLE       |
                           PRM_MEASURE_MEASURE_MODE);
    // Wait for output to settle
    for (nLoopCnt = 0; nLoopCnt < 8; nLoopCnt++) {
        // ltck = MCU_HAL_GetTick();
        while ((GetStatusAdxl362() & PRM_DATA_READY) != PRM_DATA_READY) {
        }

        // NRF_LOG_INFO("%d\r\n", MCU_HAL_GetTick() - ltck);
        GetXYZdata12BAdxl362(&nXclData[0], &nXclData[1], &nXclData[2]);
    }
    SetSelfTestAdxl362(PRM_ST_ENABLE);
    // Wait for output to settle
    for (nLoopCnt = 0; nLoopCnt < 8; nLoopCnt++) {
        // ltck = MCU_HAL_GetTick();
        while ((GetStatusAdxl362() & PRM_DATA_READY) != PRM_DATA_READY) {
        }
        // NRF_LOG_INFO("%d\r\n", MCU_HAL_GetTick() - ltck);
        GetXYZdata12BAdxl362(&nXclDataSt[0], &nXclDataSt[1], &nXclDataSt[2]);
    }
    SetSelfTestAdxl362(PRM_ST_DISABLE);
    SetPowerControlAdxl362(0);  // Standby mode
    NRF_LOG_DEBUG("%-20s%15s%15s%15s", "\r\nAxis", "X", "Y", "Z\r\n");
    NRF_LOG_DEBUG("%-20s", "\r\nInitial values");
    for (nLoopCnt = 0; nLoopCnt < 3; nLoopCnt += 1) {
        NRF_LOG_INFO("%15d", nXclData[nLoopCnt]);
    }
    NRF_LOG_INFO("%-20s", "\r\nTest values");
    for (nLoopCnt = 0; nLoopCnt < 3; nLoopCnt += 1) {
        NRF_LOG_DEBUG("%15d", nXclDataSt[nLoopCnt]);
    }
    NRF_LOG_INFO("%-20s", "\r\nScaled difference");
    for (nLoopCnt = 0; nLoopCnt < 3; nLoopCnt += 1) {
        // Multiply by 4 for sensitivity
        nXclScaledDiff[nLoopCnt] = (int32_t) ((nXclDataSt[nLoopCnt]
                                         - nXclData[nLoopCnt]) * 4);
        NRF_LOG_INFO("%15d", nXclScaledDiff[nLoopCnt]);
    }
    NRF_LOG_INFO("%-20s", "\r\nExpected range");
    NRF_LOG_INFO("%15s%15s%15s\r\n", "200:2800", "-2800:+200", "200:2800");
    if ((nXclScaledDiff[0] > 2800) || (nXclScaledDiff[0] < 200) ||
            (nXclScaledDiff[1] > 200) || (nXclScaledDiff[1] < -2800) ||
            (nXclScaledDiff[2] > 2800)  || (nXclScaledDiff[2] < 200)) {
        NRF_LOG_INFO("\r\nFAIL: ADXL self test failed\r\n");
        return 1;
    } else {
        NRF_LOG_INFO("\r\nPASS. ADXL self test pass\r\n");
        return 0;
    }
}
/**@}*/ /* end of group Adxl_Driver */
/**@}*/ /* end of group Device_Drivers */
