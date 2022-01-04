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
#include "adi_adpd_ssm.h"

ADI_HAL_STATUS_t Adpd400x_I2C_TxRx(uint8_t *register_address, uint8_t * buffer, uint16_t txsize, uint16_t rxsize);
ADI_HAL_STATUS_t Adpd400x_I2C_Transmit(uint8_t *register_address, uint16_t txsize);
ADI_HAL_STATUS_t Adpd400x_SPI_Receive(uint8_t *pTxData, uint8_t *pRxData, uint16_t TxSize, uint16_t RxSize);
ADI_HAL_STATUS_t Adpd400x_SPI_Transmit(uint8_t *pTxData, uint16_t TxSize);
void MCU_HAL_Delay(uint32_t ms_delay);

/* ------------------------- Defines  -------------------------------------- */
/* define the following macro for DEBUG */
//#define DEBUG_DRV                 1
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
tAdiAdpdDrvInst gAdiAdpdDrvInst;  /*!< Driver instance object */
uint16_t gAdpdSlotMaxODR = 0;         //!< Maximum ODR among of all the active slots
/* ------------------------- Public Function Prototypes -------------------- */
uint32_t* Adpd400xDrvGetDebugInfo();
/* ------------------------- Private variables ----------------------------- */
//static uint16_t pre_active_setting[SLOT_NUM][SLOT_DISABLE_SETTINGS];
//static const uint16_t ADPD400x_ID = 0x00C0;

#ifndef NDEBUG
static uint32_t gnOverFlowCnt = 0; //!< Debug variable to check if there is ADPD FIFO overflow
#endif  // NDEBUG
/* ------------------------- Private Function Prototypes ------------------- */
static void _adi_adpddrv_Init(void);
static void _adi_adpddrv_SetSlotSize(uint8_t nSlotNum, uint16_t nSlotFormat);
static void (*gpfnADPDCallBack)();
static adi_adpd_result_t _adi_adpddrv_SelComMode(void);
static adi_adpd_result_t _adi_adpddrv_SetInterrupt(void);
static void _adi_adpddrv_CheckFifoOvFl(void);
/*!****************************************************************************
*
*  \brief       Synchronous register write to the ADPD400x register with the given value
*
*  \param[in]   nAddr: 16-bit register address
*
*  \param[in]   nRegValue: 16-bit register data value
*
*  \return      ADI_ADPD_DRV_SUCCESS=success, ADI_ADPD_DRV_ERROR=error
******************************************************************************/
adi_adpd_result_t adi_adpddrv_RegWrite(uint16_t nAddr, uint16_t nRegValue)
{
  /* Declare variable to track the status of routine */
  adi_adpd_result_t nRetCode = ADI_ADPD_DRV_SUCCESS;
  /* Declare variable to store number of byte to be transmit */
  uint16_t nTxSize = 0U;
  /* Declare variable to store register address */
  uint16_t nTmpAddr = 0U;
  /* Declare variable to prepare transmit buffer */
  uint8_t anTxData[4] = {0U, 0U, 0U, 0U};
  /* Check the given register address, if it's software reset register then
  do the special operation else do normal register write operation */
  if((nAddr == ADPD400x_REG_SYS_CTL) && ((nRegValue & BITM_SYS_CTL_SW_RESET) == BITM_SYS_CTL_SW_RESET))
  {
    nRetCode = adi_adpddrv_SoftReset();
  }
  else
  {
    /* Check the communication type and do register write */
    if (gAdiAdpdDrvInst.nAdpd400xCommMode == ADPD400x_SPI_BUS) {
      /* To set the last bit high for write operation */
      nTmpAddr = ((nAddr) << 1U) | ADPD400x_SPI_WRITE;
      /* Prepare the transmit buffer */
      anTxData[nTxSize++] = (uint8_t)(nTmpAddr >> 8U);
      anTxData[nTxSize++] = (uint8_t)(nTmpAddr);
      anTxData[nTxSize++] = (uint8_t)(nRegValue >> 8U);
      anTxData[nTxSize++] = (uint8_t)(nRegValue);

      /*
      The first argument to the function ADPD400x_SPI_Transmit is the register
      address of the ADPD400x device and the 16 bits data value to be written to the
      device register.
      The 1st argument to the function ADPD400x_SPI_Transmit is the pointer to the
      buffer of the size of three bytes in which first byte is the register
      address of the ADPD400x device.
      The second and the third bytes are the 16 bits data value to be written
      to the device register
      The 2nd argument is the size of the buffer in bytes (3 bytes).
      ADPD400x_SPI_Transmit() should be implemented in such a way that it transmits
      the data from anTxData buffer of size specified in the second argument.
      */
      if (Adpd400x_SPI_Transmit(anTxData, nTxSize)!= ADI_ADPD_DRV_SUCCESS)
      {
        /* Update the trace variable with failure code, so the caller will get
        status of their request */
        nRetCode = ADI_ADPD_DRV_WRITE_ERROR;
      }
    }
    else if(gAdiAdpdDrvInst.nAdpd400xCommMode == ADPD400x_I2C_BUS)
    {
      /* Prepare the transmit buffer with register address, if register address
      above 127 then we need to set bit-15 as '1'. Refer datasheet for more information*/
      if (nAddr > 0x7FU)
      {
        nAddr = nAddr | ADPD400x_I2C_LONG_ADDRESS;
        anTxData[nTxSize++] = (uint8_t)(nAddr >> 8U);
        anTxData[nTxSize++] = (uint8_t)nAddr;
      }
      else
      {
        anTxData[nTxSize++] = (uint8_t)nAddr;
      }
      /* add the register value in transmit buffer */
      anTxData[nTxSize++] = (uint8_t)(nRegValue >> 8U);
      anTxData[nTxSize++] = (uint8_t)(nRegValue);
      /*
      The first argument to the function ADPD400x_I2C_Transmit is the register
      address of the ADPD400x device and the 16 bits data value to be written to the
      device register.
      The 1st argument to the function ADPD400x_I2C_Transmit is the pointer to the
      buffer of the size of three bytes in which first byte is the register
      address of the ADPD400x device.
      The second and the third bytes are the 16 bits data value to be written
      to the device register
      The 2nd argument is the size of the buffer in bytes (3 bytes).
      ADPD400x_I2C_Transmit() should be implemented in such a way that it transmits
      the data from anTxData buffer of size specified in the second argument.
      */

      if (Adpd400x_I2C_Transmit((uint8_t *) anTxData, nTxSize) != ADI_ADPD_DRV_SUCCESS)
      {
        /* Update the trace variable with failure code, so the caller will get
        status of their request */
        nRetCode = ADI_ADPD_DRV_WRITE_ERROR;
      }
    }
    else
    {
      /*
      1. This block will get execute when the communication type set as none.
      2. Update the trace variable with failure code, so the caller will get
      status of their request */
      nRetCode = ADI_ADPD_DRV_PARAM_ERROR;
    }
  }
  /* Return routine status to caller function */
  return nRetCode;
}
/*!****************************************************************************
*
*  \brief       Synchronous register read of the ADPD400x register into the given pointer
*
*  \param[in]   nAddr: 16-bit register address
*
*  \param[in]   pnData: Pointer to 16-bit register data value
*
*  \return      ADI_ADPD_DRV_SUCCESS=success, ADI_ADPD_DRV_ERROR=error
******************************************************************************/
adi_adpd_result_t adi_adpddrv_RegRead(uint16_t nAddr, uint16_t *pnData)
{
  /* Declare variable to track the status of routine */
  adi_adpd_result_t nRetCode = ADI_ADPD_DRV_SUCCESS;
  /* Declare variable to store number of byte to be transmit */
  uint16_t nTxSize = 0U;
  /* Declare variable to store register address */
  uint16_t nTmpAddr = 0U;
  /* Declare variable to store value from receive buffer */
  uint8_t anRxData[2 + SPI_DUMMY_BYTES] = {0U};
  /* Declare variable to prepare transmit buffer */
  uint8_t anTxData[2] = {0U, 0U};
  /* Check the communication type and proceed with selected peripheral */
  if (gAdiAdpdDrvInst.nAdpd400xCommMode ==  ADPD400x_SPI_BUS )
  {
    /* To set the last bit low for read operation */
    nTmpAddr = (nAddr << 1U) & ADPD400x_SPI_READ;
    /* Prepare the transmit buffer with register address */
    anTxData[nTxSize++] = (uint8_t)(nTmpAddr >> 8U);
    anTxData[nTxSize++] = (uint8_t)(nTmpAddr);
    /*
    The first argument to the function is the register address of the
    ADPD400x device from where the data is to be read.
    The 2nd argument is the pointer to the buffer of received data.
    The size of this buffer should be equal to the number of data requested.
    The 3rd argument is the size of transmit data in bytes.
    The 4th argument is the size of requested data in bytes.
    ADPD400x_SPI_Receive() should be implemented in such a way that it transmits
    the register address from the first argument and receives the data
    specified by the address in the second argument. The received data will
    be of size specified by 3rd argument.
    */
    if (Adpd400x_SPI_Receive(anTxData, anRxData, nTxSize, 2U)!= ADI_ADPD_DRV_SUCCESS)
    {
      /* Update the trace variable with failure code, so the caller will get
      status of their request	*/
      nRetCode = ADI_ADPD_DRV_READ_ERROR;
    }
  }  
  else if (gAdiAdpdDrvInst.nAdpd400xCommMode == ADPD400x_I2C_BUS)
  {
    /* Prepare the transmit buffer with register address, if register address
    above 127 then we need to set bit-15 as '1'. Refer datasheet for more information*/
    if (nAddr > 0x7FU)
    {
      nAddr = nAddr | ADPD400x_I2C_LONG_ADDRESS;
      anTxData[nTxSize++] = (uint8_t)(nAddr >> 8U);
      anTxData[nTxSize++] = (uint8_t)nAddr;
    } else {
      anTxData[nTxSize++] = (uint8_t)nAddr;
    }
    /*
    The first argument to the function is the register address of the
    ADPD400x device from where the data is to be read.
    The 2nd argument is the pointer to the buffer of received data.
    The size of this buffer should be equal to the number of data requested.
    The 3rd argument is the size of transmit data in bytes.
    The 4th argument is the size of requested data in bytes.
    ADPD400x_I2C_TxRx() should be implemented in such a way that it transmits
    the register address from the first argument and receives the data
    specified by the address in the second argument. The received data will
    be of size specified by 3rd argument.
    */
    if (Adpd400x_I2C_TxRx((uint8_t *) anTxData, (uint8_t *) anRxData, nTxSize, 2U) != ADI_ADPD_DRV_SUCCESS)
    {
      /* Update the trace variable with failure code, so the caller will get
      status of their request */
      nRetCode = ADI_ADPD_DRV_READ_ERROR;
    }
  }
  else
  {
    /*
    1. This block will get execute when the communication type set as none.
    2. Update the trace variable with failure code, so the caller will get
    status of their request */
    nRetCode = ADI_ADPD_DRV_PARAM_ERROR;
  }
  /* Copy the register value from receive buffer to output parameter with byte order [15:0]*/
  //*pnData = (((uint16_t)anRxData[0]) << (8U)) + (anRxData[1]);
  *pnData = (anRxData[0+SPI_DUMMY_BYTES] << 8) + anRxData[1+SPI_DUMMY_BYTES];
  /* Return routine status to caller function */
  return nRetCode;
}

static adi_adpd_result_t _adi_adpddrv_SetInterrupt(void)
{
  /* Declare variable to track the status of routine */
  adi_adpd_result_t nRetCode = ADI_ADPD_DRV_SUCCESS;
  /* Declare the variable to store register value */
  uint16_t nRegValue = 0U;
  /* read the register value */
  nRetCode = adi_adpddrv_RegRead(ADPD400x_REG_INT_ENABLE_XD, &nRegValue);
  /* if register read succeed, prepare the value to write on INT X register */
  if(nRetCode == ADI_ADPD_DRV_SUCCESS)
  {
    /* Define INTERRUPT_ENABLE macro for the definition of FIFO_INT_ENA macros. */
    nRegValue = (nRegValue & 0x7FFFU) | FIFO_TH_INT_ENA;
    /* write register with prepared value on INT X */
    nRetCode = adi_adpddrv_RegWrite(ADPD400x_REG_INT_ENABLE_XD, nRegValue);  /* Enable INTX */
  }
  if(nRetCode == ADI_ADPD_DRV_SUCCESS)
  {
    /* For now, set INTY as well. Check later if this is required. */
    nRetCode = adi_adpddrv_RegRead(ADPD400x_REG_INT_ENABLE_YD, &nRegValue);
    if(nRetCode == ADI_ADPD_DRV_SUCCESS)
    {
      /*  Define INTERRUPT_ENABLE macro for the definition of FIFO_INT_ENA macros. */
      /* if register read succeed, prepare the value to write on INT X register */
      nRegValue = (nRegValue & 0x9FFFU) | FIFO_UF_INT_ENA | FIFO_OF_INT_ENA;
      /* write register with prepared value on INT Y */
      nRetCode = adi_adpddrv_RegWrite(ADPD400x_REG_INT_ENABLE_YD, nRegValue);  /* Enable INTY */
    }
  }
  /* Return routine status to caller function */
  return nRetCode;
}


static adi_adpd_result_t _adi_adpddrv_SelComMode(void)
{
  /* Declare variable to track the status of routine */
  adi_adpd_result_t nRetCode = ADI_ADPD_DRV_PARAM_ERROR;
  /* Declare variable to store device ID */
  uint16_t nChipID = 0U;
  /* assign the communication type as I2C */
  gAdiAdpdDrvInst.nAdpd400xCommMode = ADPD400x_SPI_BUS; 
  /* read the device ID register */
  if (adi_adpddrv_RegRead(ADPD400x_REG_CHIP_ID, &nChipID) == ADI_ADPD_DRV_SUCCESS)
  {
    /* check the device ID value is valid */
    if ((nChipID & ADPD400x_ID) == ADPD400x_ID)
    {
      gAdiAdpdDrvInst.nChipID = ADPD400x_ID;

      /* Update the trace variable with success code, so the caller will get
      status of their request */
      nRetCode = ADI_ADPD_DRV_SUCCESS;
    }
  }
  /* if the previous device ID read operation failed then try the SPI peripheral */
  if(nRetCode != ADI_ADPD_DRV_SUCCESS)
  {
     /* assign the communication type as SPI */
    gAdiAdpdDrvInst.nAdpd400xCommMode = ADPD400x_SPI_BUS;
    /* read the device ID register */
    if (adi_adpddrv_RegRead(ADPD400x_REG_CHIP_ID, &nChipID) == ADI_ADPD_DRV_SUCCESS)
    {
      if ((nChipID & ADPD400x_ID) == ADPD400x_ID)
      {
        gAdiAdpdDrvInst.nChipID = ADPD400x_ID;
        /* Update the trace variable with success code, so the caller will get
        status of their request */
        nRetCode = ADI_ADPD_DRV_SUCCESS;
      }
    }
  }

  if(nRetCode != ADI_ADPD_DRV_SUCCESS)
  {
    /* assign communication type as none, because the device ID validation got failed */
    gAdiAdpdDrvInst.nAdpd400xCommMode = ADPD400x_UNKNOWN_BUS;
  }
  // else
  // {
  //   /* assign valid device ID value to global variable */
  //   gAdiAdpdDrvInst.nChipID = nChipID;
  // }
  /* Return routine status to caller function */
  return nRetCode;
}
/*!****************************************************************************
*
*  \brief       Set device to Idle mode
*
*  \return      ADI_ADPD_DRV_SUCCESS=success, ADI_ADPD_DRV_ERROR=error
******************************************************************************/
adi_adpd_result_t adi_adpddrv_SetIdleMode(void)
{
  /* Declare variable to track the status of routine */
  adi_adpd_result_t nRetCode = ADI_ADPD_DRV_ERROR;
  /* Declare variable to store register value */
  uint16_t nRegValue = 0U;
  /* Set to standby Mode */
  nRetCode = adi_adpddrv_RegRead(ADPD400x_REG_OPMODE, &nRegValue);
  /* check the register read status, if succeed then do register write */
  if(nRetCode == ADI_ADPD_DRV_SUCCESS)
  {
    /* Mask the register mode bit */
    nRegValue &= (~BITM_OPMODE_OP_MODE);
    /* set the mode bit as IDLE mode */
    nRegValue |= ADPD400x_OP_IDLE_MODE;
    /* write the prepared value to operation mode register */
    nRetCode = adi_adpddrv_RegWrite(ADPD400x_REG_OPMODE, nRegValue);
  }
  /* Return routine status to caller function */
  return nRetCode;
}

/*!****************************************************************************
*
*  \brief       Synchronous register read of 32bit ADPD400x register into the given pointer
*
*  \param[in]   nAddr: 16-bit register address
*
*  \param[in]   pnData: Pointer to 32-bit register data value
*
*  \return      ADI_ADPD_DRV_SUCCESS=success, ADI_ADPD_DRV_ERROR=error
*
******************************************************************************/
adi_adpd_result_t adi_adpddrv_RegRead32B(uint16_t nAddr, uint32_t *pnData)
{
  /* Declare variable to track the status of routine */
  adi_adpd_result_t nRetCode = ADI_ADPD_DRV_SUCCESS;
  /* Declare variable to store number of byte to be transmit */
  uint16_t nTxSize = 0U;
  /* Declare variable to store register address */
  uint16_t nTmpAddr = 0U;
  /* Declare variable to store value from receive buffer */
  uint8_t anRxData[4 + SPI_DUMMY_BYTES] = {0U, 0U, 0U, 0U};
  /* Declare variable to prepare transmit buffer */
  uint8_t anTxData[2] = {0U, 0U};
  /* Check the communication type and proceed with selected peripheral */
  if (gAdiAdpdDrvInst.nAdpd400xCommMode == ADPD400x_SPI_BUS)
  {
    /* To set the last bit low for read operation */
    nTmpAddr = ((nAddr) << 1U) & ADPD400x_SPI_READ;
    /* Prepare the transmit buffer with register address */
    anTxData[nTxSize++] = (uint8_t)(nTmpAddr >> 8U);
    anTxData[nTxSize++] = (uint8_t)(nTmpAddr);
    /*
    The first argument to the function is the register address of the
    ADPD400x device from where the data is to be read.
    The 2nd argument is the pointer to the buffer of received data.
    The size of this buffer should be equal to the number of data requested.
    The 3rd argument is the size of transmit data in bytes.
    The 4th argument is the size of requested data in bytes.
    ADPD400x_SPI_Receive() should be implemented in such a way that it transmits
    the register address from the first argument and receives the data
    specified by the address in the second argument. The received data will
    be of size specified by 3rd argument.
    */
    if (Adpd400x_SPI_Receive(anTxData, anRxData, nTxSize, 4U)!= ADI_ADPD_DRV_SUCCESS)
    {
      /* Update the trace variable with failure code, so the caller will get
      status of their request */
      nRetCode = ADI_ADPD_DRV_READ_ERROR;
    }
  }
  else if (gAdiAdpdDrvInst.nAdpd400xCommMode == ADPD400x_I2C_BUS)
  {
    /* Prepare the transmit buffer with register address, if register address
    above 127 then we need to set bit-15 as '1'. Refer datasheet for more information*/
    if (nAddr > 0x7FU)
    {
      nAddr = nAddr | ADPD400x_I2C_LONG_ADDRESS;
      anTxData[nTxSize++] = (uint8_t)(nAddr >> 8U);
      anTxData[nTxSize++] = (uint8_t)nAddr;
    }
    else
    {
      anTxData[nTxSize++] = (uint8_t)nAddr;
    }
    /*
    The first argument to the function is the register address of the
    ADPD400x device from where the data is to be read.
    The 2nd argument is the pointer to the buffer of received data.
    The size of this buffer should be equal to the number of data requested.
    The 3rd argument is the size of transmit data in bytes.
    The 4th argument is the size of requested data in bytes.
    ADPD400x_I2C_TxRx() should be implemented in such a way that it transmits
    the register address from the first argument and receives the data
    specified by the address in the second argument. The received data will
    be of size specified by 3rd argument.
    */
    if (Adpd400x_I2C_TxRx((uint8_t *) anTxData, (uint8_t *) anRxData, nTxSize, 4U) != ADI_ADPD_DRV_SUCCESS)
    {
    /* Update the trace variable with failure code, so the caller will get
      status of their request */
      nRetCode = ADI_ADPD_DRV_READ_ERROR;
    }
  }
  else
  {
    /*
    1. This block will get execute when the communication type set as none.
    2. Update the trace variable with failure code, so the caller will get
    status of their request */
    nRetCode = ADI_ADPD_DRV_PARAM_ERROR;
  }
  /* Copy the register value from receive buffer to ouptut parameter with byte order [31:0]*/
  *pnData = ((uint32_t)anRxData[0 + SPI_DUMMY_BYTES] << 8U) + ((uint32_t)anRxData[1 + SPI_DUMMY_BYTES]) + ((uint32_t)anRxData[2 + SPI_DUMMY_BYTES] << 24U) + ((uint32_t)anRxData[3 + SPI_DUMMY_BYTES] << 16U);
  /* Return routine status to caller function */
  return nRetCode;
}
/** @brief  Open Driver, setting up the interrupt and I2C lines
*
* @param  addr 32-bit register address
* @return int16_t A 16-bit integer: 0 - success; < 0 - failure
*/
adi_adpd_result_t adi_adpddrv_OpenDriver(void)
{
  /* Declare variable to track the status of routine */
  adi_adpd_result_t nRetCode = ADI_ADPD_DRV_SUCCESS;

  /* check the communication status */
  /* Update the trace variable with failure code, so the caller will get
  status of their request */
  nRetCode = _adi_adpddrv_SelComMode();

  /* Put the device into idle mode */
  gAdiAdpdDrvInst.nAdpd400xCommMode = ADPD400x_SPI_BUS;


  /* If device put idle mode successfully do software reset */
  if(nRetCode == ADI_ADPD_DRV_SUCCESS)
  {
    /* Check the communication mode and do the software reset with selected peripheral*/
    if (gAdiAdpdDrvInst.nAdpd400xCommMode == ADPD400x_I2C_BUS)
    {
      /* prepare the transmit buffer with software reset value */
      uint8_t txData[3] = { 0x0f, 0x80, 0x00 };
      /* Do register write with software reset value buffer */
      if (Adpd400x_I2C_Transmit(txData, 3U) != ADI_ADPD_DRV_SUCCESS)
      {
        /* Update the trace variable with failure code, so the caller will get
        status of their request */
        nRetCode = ADI_ADPD_DRV_WRITE_ERROR;
      }
    }
    else
    {
      /* prepare the transmit buffer with software reset value */
      uint8_t txData[4] = { 0x00, 0x1f, 0x80, 0x00 };
      /* Do register write with software reset value buffer */
      if (Adpd400x_SPI_Transmit(txData, 4U) != ADI_ADPD_DRV_SUCCESS)
      {
        /* Update the trace variable with failure code, so the caller will get
        status of their request */
        nRetCode = ADI_ADPD_DRV_WRITE_ERROR;
      }
    }
  }

  /* If Device software reset completed successfully give 500ms time delay
  before doing configurations*/
  if(nRetCode == ADI_ADPD_DRV_SUCCESS)
  {
    /* Put device sleep mode for 500ms */
    MCU_HAL_Delay(SLEEP_TIME);
    _adi_adpddrv_Init();

    /* Make default interrupt configuration */
    nRetCode = _adi_adpddrv_SetInterrupt();  /* Default mode */
  }
  /* Return routine status to caller function */
  return nRetCode;
}
/** @brief  Close Driver, Clear up before existing
*
* @param  None
* @return int16_t A 16-bit integer: 0 - success; < 0 - failure
*/
adi_adpd_result_t adi_adpddrv_CloseDriver(void)
{
  /* Put the device in Idle mode and return the status to caller function */
  return adi_adpddrv_SetIdleMode();
}

/* int16_t Adpd400xDrvCloseDriver() {
  return adi_adpddrv_SetIdleMode();
} */

/** @brief  Returns the communication bus: I2C, SPI or Uknown
*           See the typedef Adpd400xComMode
*
* @param  None
* @return Adpd400xComMode
*/
Adpd400xComMode_t adi_adpddrv_GetComMode() {
  return gAdiAdpdDrvInst.nAdpd400xCommMode;
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
void adi_adpddrv_DataReadyCallback(void (*pfADPDDataReady)()) {
  gpfnADPDCallBack = pfADPDDataReady;
}

/** @brief  ADPD400x interrupt service routine will tell the application that data ready to read via callback
* @return None
*/
void adi_adpddrv_ISR() {
  if (gpfnADPDCallBack != NULL) {
    (*gpfnADPDCallBack)();
  }
  gAdiAdpdDrvInst.nAccessCnt[0]++;
}

/*!****************************************************************************
*
*  \brief       Get the Device ID for setting proper device specific params

*  \return      16-bit Device ID
******************************************************************************/
uint16_t adi_adpddrv_GetChipId(void)
{
  uint16_t nChipID;
  adi_adpddrv_RegRead(ADPD400x_REG_CHIP_ID, &nChipID);
  /* Return the chip ID to caller function */
  return nChipID;
}

/** @brief  Debug function. Read out debug info
* @return uint32_t* Debug info pointer
*/
uint32_t* Adpd400xDrvGetDebugInfo() {
  return gAdiAdpdDrvInst.nAccessCnt;
}

/** @brief  Debug function. Read out debug info
* @param  None
* @return uint32_t* Debug info pointer
*/
uint32_t Adpd400xDrvGetISRDebugInfo() {
  return gAdiAdpdDrvInst.nAccessCnt[0];
}

/*!****************************************************************************
*
*  \brief       Check the Fifo Level if the size has exceeded the Max Fifo Depth
*
*  \return       None
*****************************************************************************/
static void _adi_adpddrv_CheckFifoOvFl(void)
{
  /* check the connected device based on device ID check fifo overflow count */
  if(gAdiAdpdDrvInst.nChipID == ADPD400x_ID)
  {
    if (gAdiAdpdDrvInst.nFifoLevel >= ADPD400x_FIFO_SIZE)
    {
#ifndef NDEBUG
      gAdiAdpdDrvInst.nOverFlowCnt++;
#endif
    }
  }
  else
  {
    if (gAdiAdpdDrvInst.nFifoLevel >= ADPA410x_FIFO_SIZE)
    {
#ifndef NDEBUG
      gAdiAdpdDrvInst.nOverFlowCnt++;
#endif
    }
  }
}
/*!****************************************************************************
*
*  \brief       Read data out from ADPD400x FIFO
*
*  \param[in]   nDataSetSize: DataSet Size to be get
*
*  \param[out]  *pnData: 8-bit pointer which points to data buffer
*
*  \return      ADI_ADPD_DRV_SUCCESS=success, ADI_ADPD_DRV_ERROR=error
******************************************************************************/
adi_adpd_result_t adi_adpddrv_ReadFifoData(uint16_t nDataSetSize, uint8_t *pnData)
{
  /* Declare variable to track the status of routine */
  adi_adpd_result_t nRetCode = ADI_ADPD_DRV_SUCCESS;
  /* Declare variable to store register address */
  uint16_t nTmpAddr = 0U;
  /* Declare variable to prepare transmit buffer */
  uint8_t anTxData[2];
  /* Declare variable to store register address */
  uint8_t nAddr;
  /* Read the fifo count register */
  nRetCode = adi_adpddrv_RegRead(ADPD400x_REG_INT_STATUS_FIFO, &gAdiAdpdDrvInst.nFifoLevel);
  /* if register read get success do the fifo read operation */
  if(nRetCode == ADI_ADPD_DRV_SUCCESS)
  {
    /* extract the sample byte count from register value */
    gAdiAdpdDrvInst.nFifoLevel = gAdiAdpdDrvInst.nFifoLevel & 0x7FFU;
#ifndef NDEBUG
    _adi_adpddrv_CheckFifoOvFl();
#endif
    /* check the requested bytes of data with fifo byte count. if the requested
    bytes not available in fifo then return error to caller function */
    if (gAdiAdpdDrvInst.nFifoLevel >= nDataSetSize)
    {
#ifndef NDEBUG
      /* Increment debug access count with every fifo read operation */
      gAdiAdpdDrvInst.nAccessCnt[2]++;
#endif
      /* assign the fifo register address */
      nAddr = ADPD400x_REG_FIFO_DATA;
      switch(gAdiAdpdDrvInst.nAdpd400xCommMode)
      {
      case ADPD400x_SPI_BUS:
        /* To set the last bit low for read operation */
        nTmpAddr = (((uint16_t)(nAddr) << 1U ) & (ADPD400x_SPI_READ));
        anTxData[0] = (uint8_t)(nTmpAddr >> 8U);
        anTxData[1] = (uint8_t)(nTmpAddr);
        /*
        The first argument to the function is the register address of the
        ADPD400x device from where the data is to be read.
        The 2nd argument is the pointer to the buffer of received data.
        The size of this buffer should be equal to the number of data requested.
        The 3rd argument is the size of transmit data in bytes.
        The 4th argument is the size of requested data in bytes.
        ADPD400x_SPI_Receive() should be implemented in such a way that it transmits
        the register address from the first argument and receives the data
        specified by the address in the second argument. The received data will
        be of size specified by 3rd argument.
        */
        if (Adpd400x_SPI_Receive(anTxData, pnData, 2U, nDataSetSize) != ADI_ADPD_DRV_SUCCESS)
        {
          /* Update the trace variable with failure code, so the caller will get
          status of their request */
          nRetCode = ADI_ADPD_DRV_READ_ERROR;
        }
        break;
      case ADPD400x_I2C_BUS:
        /*
        The first argument to the function is the register address of the
        ADPD400x device from where the data is to be read.
        The 2nd argument is the pointer to the buffer of received data.
        The size of this buffer should be equal to the number of data requested.
        The 3rd argument is the size of transmit data in bytes.
        The 4th argument is the size of requested data in bytes.
        ADPD400x_I2C_TxRx() should be implemented in such a way that it transmits
        the register address from the first argument and receives the data
        specified by the address in the second argument. The received data will
        be of size specified by 3rd argument.
        */
        if (Adpd400x_I2C_TxRx(&nAddr, pnData, 1U, nDataSetSize) != ADI_ADPD_DRV_SUCCESS)
        {
          /* Update the trace variable with failure code, so the caller will get
          status of their request */
          nRetCode = ADI_ADPD_DRV_READ_ERROR;
        }
        break;
      default:
        /*
        1. This block will get execute when the communication type set as none.
        2. Update the trace variable with failure code, so the caller will get
        status of their request */
        nRetCode = ADI_ADPD_DRV_PARAM_ERROR;
        break;
      }
    }
    else
    {
      /* Update the trace variable with failure code, so the caller will get
      status of their request */
      nRetCode = ADI_ADPD_DRV_FIFO_ERROR;
    }
  }
  /* Return routine status to caller function */
  return nRetCode;
}
/*!****************************************************************************
*  \brief       Synchronous register read from the ADPD400x data registers
*
*  \param[in]   nSlotNum: 8-bit slot number
*
*  \param[in]   nSignalDark: 8-bit signal/dark flag
*
*  \param[in]   nChNum: 8-bit channel number info
*
*  \param[out]  *pnData: Pointer to 32-bit register data value
*
*  \return      ADI_ADPD_DRV_SUCCESS=success, ADI_ADPD_DRV_ERROR=error
******************************************************************************/
adi_adpd_result_t adi_adpddrv_ReadRegData(ADPD400xDrv_SlotNum_t nSlotNum, uint8_t nSignalDark, uint8_t nChNum, uint32_t *pnData)
{
  /* Declare variable to track the status of routine */
  adi_adpd_result_t nRetCode = ADI_ADPD_DRV_SUCCESS;
  /* Declare variable to store register address */
  uint16_t nAddr = 0U;
  /* Declare variable to store number of byte to be transmit */
  uint16_t nTxSize = 0U;
  /* Declare variable to store register address */
  uint16_t nTmpAddr = 0U;
  /* Declare variable to store value from receive buffer */
  uint8_t anRxData[4 + SPI_DUMMY_BYTES] = {0U, 0U, 0U, 0U};
  /* Declare variable to prepare transmit buffer */
  uint8_t anTxData[2] = {0U, 0U};
  /* Declare variable to store the status of slot data */
  uint16_t nIntStatus;
   /* Read IntStatus from ADPD400x_REG_INT_STATUS_DATA */
  nRetCode = adi_adpddrv_RegRead(ADPD400x_REG_INT_STATUS_DATA, &nIntStatus);
  /* Error check the read of register ADPD400x_REG_INT_STATUS_DATA */
  if(nRetCode != ADI_ADPD_DRV_SUCCESS )
  {
    return nRetCode;
  }
  if(nIntStatus & (1<<nSlotNum)) 
  {
    /* get register address from user input parameter */
    nAddr = ADPD400x_REG_SIGNAL1_L_A + (uint16_t)(((uint16_t)nSlotNum << 3U) + (uint16_t)((uint16_t)nSignalDark << 2U) + (uint16_t)((uint16_t)nChNum << 1U));
    /* check the communication type */
    if (gAdiAdpdDrvInst.nAdpd400xCommMode == ADPD400x_SPI_BUS)
    {
      /* To set the last bit low for read operation*/
      nTmpAddr = (nAddr << 1U) & ADPD400x_SPI_READ;
      /* Prepare the transmit buffer with register address */
      anTxData[nTxSize++] = (uint8_t)(nTmpAddr >> 8);
      anTxData[nTxSize++] = (uint8_t)(nTmpAddr);
      /*
      The first argument to the function is the register address of the
      ADPD400x device from where the data is to be read.
      The 2nd argument is the pointer to the buffer of received data.
      The size of this buffer should be equal to the number of data requested.
      The 3rd argument is the size of transmit data in bytes.
      The 4th argument is the size of requested data in bytes.
      ADPD400x_SPI_Receive() should be implemented in such a way that it transmits
      the register address from the first argument and receives the data
      specified by the address in the second argument. The received data will
      be of size specified by 3rd argument.
      */
      if (Adpd400x_SPI_Receive(anTxData, anRxData, nTxSize, 4U)!= ADI_ADPD_DRV_SUCCESS)
      {
        /* Update the trace variable with failure code, so the caller will get
        status of their request */
        nRetCode = ADI_ADPD_DRV_READ_ERROR;
      }
    }
    else if (gAdiAdpdDrvInst.nAdpd400xCommMode == ADPD400x_I2C_BUS)
    {
      /* Prepare the transmit buffer with register address, if register address
      above 127 then we need to set bit-8 as '1'. Refer datasheet for more information*/
      if (nAddr > 0x7FU)
      {
        anTxData[nTxSize++] = (uint8_t)((nAddr >> 8U) | 0x80U);
        anTxData[nTxSize++] = (uint8_t)nAddr;
      }
      else {
        anTxData[nTxSize++] = (uint8_t)nAddr;
      }
      /*
      The first argument to the function is the register address of the
      ADPD400x device from where the data is to be read.
      The 2nd argument is the pointer to the buffer of received data.
      The size of this buffer should be equal to the number of data requested.
      The 3rd argument is the size of transmit data in bytes.
      The 4th argument is the size of requested data in bytes.
      ADPD400x_I2C_TxRx() should be implemented in such a way that it transmits
      the register address from the first argument and receives the data
      specified by the address in the second argument. The received data will
      be of size specified by 3rd argument.
      */
      if (Adpd400x_I2C_TxRx((uint8_t *)anTxData, (uint8_t *)anRxData, nTxSize, 4U) != ADI_ADPD_DRV_SUCCESS)
      {
        /* Update the trace variable with failure code, so the caller will get
        status of their request */
        nRetCode = ADI_ADPD_DRV_READ_ERROR;
      }
    }
    else {
      /*
      1. This block will get execute when the communication type set as none.
      2. Update the trace variable with failure code, so the caller will get
      status of their request */
      nRetCode = ADI_ADPD_DRV_PARAM_ERROR;
    }
    /* if register read succeed assign register value to output parameter variable */
    if(nRetCode == ADI_ADPD_DRV_SUCCESS)
    {
      *pnData = ((uint32_t)anRxData[0 + SPI_DUMMY_BYTES] << 8U) + (uint32_t)anRxData[1 + SPI_DUMMY_BYTES] + ((uint32_t)anRxData[2 +  SPI_DUMMY_BYTES] << 24U) + ((uint32_t)anRxData[3 + SPI_DUMMY_BYTES] << 16U);
      /* Update the trace variable with success code, so the caller will get
      status of their request */
      nRetCode = ADI_ADPD_DRV_SUCCESS;
    }
  }
  else
  {
    nRetCode = ADI_ADPD_DRV_ERROR;
  }
  /* Return routine status to caller function */
  return nRetCode;
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
int16_t adi_adpddrv_SetLedCurrent(uint16_t nLedCurrent, ADPD400xDrv_LedId_t nLedId, ADPD400xDrv_SlotNum_t nSlotNum) {
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
  if (adi_adpddrv_RegRead(nReg, &nAdpd400xData) != ADPD400xDrv_SUCCESS)
    return ADPD400xDrv_ERROR;

  nAdpd400xData = (nAdpd400xData & nMask) | (nLedCurrent << nBitPos);


  if (adi_adpddrv_RegWrite(nReg, nAdpd400xData) != ADPD400xDrv_SUCCESS)
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
int16_t adi_adpddrv_GetLedCurrent(uint16_t *pLedCurrent, ADPD400xDrv_LedId_t nLedId, ADPD400xDrv_SlotNum_t nSlotNum) {
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
  if (adi_adpddrv_RegRead(nReg, &nAdpd400xData) != ADPD400xDrv_SUCCESS)
    return ADPD400xDrv_ERROR;

  *pLedCurrent = (nAdpd400xData & nMask) >> nBitPos;

  return nRetCode;
}
/*!****************************************************************************
*
*  \brief       Soft reset the ADPD400x device
*
*  \return      ADI_ADPD_DRV_SUCCESS=success, ADI_ADPD_DRV_ERROR=error
******************************************************************************/
uint16_t adi_adpddrv_SoftReset(void)
{
#if (USER_DEFINED_SOFTWARE_RESET == 0U)
  return adi_adpddrv_OpenDriver();
#else
  int16_t nRetCode = ADI_ADPD_DRV_SUCCESS;

  /* check the communication status */
  if(gAdiAdpdDrvInst.nAdpd400xCommMode == ADPD400x_UNKNOWN_BUS)
  {
      nRetCode = _adi_adpddrv_SelComMode();
  }

  /* Put the device into idle mode */
  if(nRetCode == ADI_ADPD_DRV_SUCCESS)
  {
    nRetCode = adi_adpddrv_SetIdleMode();
  }

  /* If device is put in idle mode successfully, then do software reset */
  if(nRetCode == ADI_ADPD_DRV_SUCCESS)
  {
    if (gAdiAdpdDrvInst.nAdpd400xCommMode == ADPD400x_I2C_BUS)
    {
      uint8_t txData[3] = { 0x0f, 0x80, 0x00 };
      if (Adpd400x_I2C_Transmit(txData, 3U) != ADI_ADPD_DRV_SUCCESS)
      {
        nRetCode = ADI_ADPD_DRV_WRITE_ERROR;
      }
    }
    else
    {
      uint8_t txData[4] = { 0x00, 0x1f, 0x80, 0x00 };
      if (Adpd400x_SPI_Transmit(txData, 4U) != ADI_ADPD_DRV_SUCCESS)
      {
        nRetCode = ADI_ADPD_DRV_WRITE_ERROR;
      }
    }
  }
  return nRetCode;
#endif
}

/** @brief Driver Initialization.
*
* @return None
*/
static void _adi_adpddrv_Init(void) {
  adi_adpdssm_slotinit();
}
