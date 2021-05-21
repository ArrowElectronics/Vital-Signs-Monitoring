/**
    ****************************************************************************
    * @addtogroup Device_Drivers Device Drivers
    * @{
    * @file     adxl362.c
    * @author   ADI
    * @version  V2.0.0
    * @date     19-Feb-2016
    * @brief    Reference design device driver to access ADI ADXL362 chip.
    ****************************************************************************
    * @attention
*/
/*******************************************************************************
 Copyright (c) 2019 Analog Devices, Inc.

 All rights reserved.

 Redistribution and use in source and binary forms, with or without modification,
 are permitted provided that the following conditions are met:
 - Redistributions of source code must retain the above copyright notice,
 this list of conditions and the following disclaimer.
 - Redistributions in binary form must reproduce the above copyright notice,
 this list of conditions and the following disclaimer in the documentation
 and/or other materials provided with the distribution.
 - Modified versions of the software must be conspicuously marked as such.
 - This software is licensed solely and exclusively for use with processors
 manufactured by or for Analog Devices, Inc.
 - This software may not be combined or merged with other code in any manner
 that would cause the software to become subject to terms and conditions
 which differ from those listed here.
 - Neither the name of Analog Devices, Inc. nor the names of its
 contributors may be used to endorse or promote products derived
 from this software without specific prior written permission.
 - The use of this software may or may not infringe the patent rights of one
 or more patent holders.  This license does not release you from the
 requirement that you obtain separate licenses from these patent holders
 to use this software.

 THIS SOFTWARE IS PROVIDED BY ANALOG DEVICES, INC. AND CONTRIBUTORS "AS IS" AND ANY
 EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, NON-INFRINGEMENT,
 TITLE, MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN
 NO EVENT SHALL ANALOG DEVICES, INC. OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
 INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, PUNITIVE OR CONSEQUENTIAL DAMAGES
 (INCLUDING, BUT NOT LIMITED TO, DAMAGES ARISING OUT OF CLAIMS OF INTELLECTUAL
 PROPERTY RIGHTS INFRINGEMENT; PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
 OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
 EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
*******************************************************************************/

/******************************************************************************/
/*    Date:		2016-02-19                                            */
/*    PCB:              NO defined                                            */
/*    Development Tool: IAR Embedded.                                         */
/*    Name:             Adxl362.c (library)                                   */
/*    Version:          2.0.0                                                   */
/*    Description:      This file includes all necesary functions to control  */
/*                      ADXL362 accelerometer This library can be used from   */
/*                      any microcontroller, but user must create two         */
/*                      functions to implement the SPI protocol.              */
/******************************************************************************/
/* ------------------------- Includes -------------------------------------- */
#include "Adxl362.h"  // .h of this file
#include "printf.h"
/** @addtogroup Adxl_Driver  ADXL 
  Source file contains driver task for wearable device framework.
  This will read data from senor devices and put them into data buffers.
  @{
 */
/** @brief  This function returns the Analog Devices device ID.This nValue must be 0xAD.
  *  
  * @return uint8_t A 8-bit ADXL362's Analog Device device ID
  */
uint8_t GetDevIdAdAdxl362(void) {
    uint8_t        nLengthWrite       =     2;
    uint8_t        nLengthRead        =     1;
    uint8_t        nTxData[3]         =     {COMMAND_READ_REG, REG_DEVID_AD};
    uint8_t        nRxData[SPI_ADXL_DUMMY_BYTES+3];
    uint8_t        nValue;
    /**
        The first argument to the function ADXL362_SPI_Receive is the address of
        the data read command.
        The 2nd argument is the pointer to the buffer of received data.
        The 3rd argument is the size of the trasmitted data and the 4th argument 
        is the size of the received data in bytes.
    */
    ADXL362_SPI_Receive(nTxData, nRxData, nLengthWrite, nLengthRead);
    nValue = nRxData[SPI_ADXL_DUMMY_BYTES+0];
    return nValue;
}

/** @brief  This function returns an Analog Devices MEMS device ID. This nValue must be 0x1D. 
  *  
  * @return uint8_t ADXL362's MEMs device ID
  */
uint8_t GetDevIdMstAdxl362(void) {
    uint8_t        nLengthWrite       =     2;
    uint8_t        nLengthRead        =     1;
    uint8_t        nTxData[2]         =     {COMMAND_READ_REG, REG_DEVID_MST};
    uint8_t        nRxData[SPI_ADXL_DUMMY_BYTES+3];
    uint8_t        nValue;
    ADXL362_SPI_Receive(nTxData, nRxData, nLengthWrite, nLengthRead);
    nValue = nRxData[SPI_ADXL_DUMMY_BYTES+0];
    return nValue;
}

/** @brief  This function returns the device ID. This nValue must be 0xF2. 
  *  
  * @return uint8_t ADXL362's device ID 
  */

uint8_t GetPartIdAdxl362(void) {
    uint8_t        nLengthWrite       =     2;
    uint8_t        nLengthRead        =     1;
    uint8_t        nTxData[3]         =     {COMMAND_READ_REG, REG_PARTID};
    uint8_t        nRxData[SPI_ADXL_DUMMY_BYTES+3];
    uint8_t        nValue;
    ADXL362_SPI_Receive(nTxData, nRxData, nLengthWrite, nLengthRead);
    nValue = nRxData[SPI_ADXL_DUMMY_BYTES+0];
    return nValue;
}

/** @brief  This function returns the product revision ID, 
  * beginning with 0x00 and is changed for each subsequent revision.
  *  
  * @return uint8_t ADXL362's product revision ID  
  */
uint8_t GetRevIdAdxl362(void) {
    uint8_t        nLengthWrite       =     2;
    uint8_t        nLengthRead        =     1;
    uint8_t        nTxData[3]         =     {COMMAND_READ_REG, REG_REVID};
    uint8_t        nRxData[SPI_ADXL_DUMMY_BYTES+3];
    uint8_t        nValue;
    ADXL362_SPI_Receive(nTxData, nRxData, nLengthWrite, nLengthRead);
    nValue = nRxData[SPI_ADXL_DUMMY_BYTES+0];
    return nValue;
}

/** @brief  This function returns the 8 most-significant bits of the x-axis acceleration data 
  *  
  * @return int8_t X axis nValue (only 8 most significant bits)
  */
int8_t GetXdata8BAdxl362(void) {
    uint8_t        nLengthWrite       =     2;
    uint8_t        nLengthRead        =     1;
    uint8_t        nTxData[3]         =     {COMMAND_READ_REG, REG_XDATA};
    uint8_t        nRxData[SPI_ADXL_DUMMY_BYTES+3];
    uint8_t        nValue;
    ADXL362_SPI_Receive(nTxData, nRxData, nLengthWrite, nLengthRead);
    nValue = nRxData[SPI_ADXL_DUMMY_BYTES+0];
    return nValue;
}

/** @brief  This function returns the 8 most-significant bits of the y-axis acceleration data 
  *  
  * @return int8_t Y axis nValue (only 8 most significant bits)
  */  
int8_t GetYdata8BAdxl362(void) {
    uint8_t        nLengthWrite        =     2;
    uint8_t        nLengthRead         =     1;
    uint8_t        nTxData[3]          =     {COMMAND_READ_REG, REG_YDATA};
    uint8_t        nRxData[SPI_ADXL_DUMMY_BYTES+3];
    uint8_t        nValue;
    ADXL362_SPI_Receive(nTxData, nRxData, nLengthWrite, nLengthRead);
    nValue = nRxData[SPI_ADXL_DUMMY_BYTES+0];
    return nValue;
}

/** @brief  This function returns the 8 most-significant bits of the z-axis acceleration data
  *  
  * @return int8_t Z axis nValue (only 8 most significant bits)
  */
int8_t GetZdata8BAdxl362(void) {
    uint8_t        nLengthWrite          =     2;
    uint8_t        nLengthRead           =     1;
    uint8_t        nTxData[3]            =     {COMMAND_READ_REG, REG_ZDATA};
    uint8_t        nRxData[SPI_ADXL_DUMMY_BYTES+3];
    uint8_t        nValue;
    ADXL362_SPI_Receive(nTxData, nRxData, nLengthWrite, nLengthRead);
    nValue = nRxData[SPI_ADXL_DUMMY_BYTES+0];
    return nValue;
}

/** @brief  This function returns the 8 most-significant bits of the x-axis,
  * y-axis and z-axis acceleration data.   
  * These 3 parameters must be passed by reference.
  * This function obtains the same nValues than:
  * GetXdata8BAdxl362();GetYdata8BAdxl362();GetZdata8BAdxl362;     
  * This function is more efficient (less time)
  * 
  * @param[out]  *pnDataX pointer to X axis nValue. only the 8 MSB
  * @param[out]  *pnDataY pointer to Y axis nValue. only the 8 MSB
  * @param[out]  *pnDataZ pointer to Z axis nValue. only the 8 MSB
  * @return None
  */
void GetXYZdata8BAdxl362(int8_t *pnDataX, int8_t *pnDataY, int8_t *pnDataZ) {
    uint8_t        nLengthWrite          =     2;
    uint8_t        nLengthRead           =     3;
    uint8_t        nTxData[5]            =     {COMMAND_READ_REG, REG_XDATA};
    uint8_t        nRxData[SPI_ADXL_DUMMY_BYTES+5];
    ADXL362_SPI_Receive(nTxData, nRxData, nLengthWrite, nLengthRead);
    *pnDataX = nRxData[SPI_ADXL_DUMMY_BYTES+0];
    *pnDataY = nRxData[SPI_ADXL_DUMMY_BYTES+1];
    *pnDataZ = nRxData[SPI_ADXL_DUMMY_BYTES+2];
}
/** @brief  This function returns the ADXL362's status register.
  * This register includes the following bits that descibe various conditions   
  *  
  * @return uint8_t ADXL362's status register
  */

uint8_t GetStatusAdxl362(void) {
    uint8_t        nLengthWrite          =     2;
    uint8_t        nLengthRead           =     1;
    uint8_t        nTxData[3]            =     {COMMAND_READ_REG, REG_STATUS};
    uint8_t        nRxData[SPI_ADXL_DUMMY_BYTES+3];
    uint8_t        nValue;
    ADXL362_SPI_Receive(nTxData, nRxData, nLengthWrite, nLengthRead);
    nValue = nRxData[SPI_ADXL_DUMMY_BYTES+0];
    return nValue;
}

/** @brief  This function returns the number of valid data samples present in the FIFO buffer. This number ranges from 0 to 512  
  *  
  * @return uint16_t number of valid samples present in the FIFO
  */
uint16_t GetFifoEntriesAdxl362(void) {
    uint8_t        nLengthWrite          =     2;
    uint8_t        nLengthRead           =     2;
    uint8_t        nTxData[4]            =     {COMMAND_READ_REG, REG_FIFO_ENTRIES_L};
    uint8_t        nRxData[SPI_ADXL_DUMMY_BYTES+4];
    uint16_t        nValue;
    ADXL362_SPI_Receive(nTxData, nRxData, nLengthWrite, nLengthRead);
    nValue = nRxData[SPI_ADXL_DUMMY_BYTES+1] << 8;
    nValue += nRxData[SPI_ADXL_DUMMY_BYTES+0];
    return nValue;
}

/** @brief  This function returns the 12b x-axis acceleration data  
  *  
  * @return int16_t X axis nValue 
  */
int16_t GetXdata12BAdxl362(void) {
    uint8_t        nLengthWrite          =     2;
    uint8_t        nLengthRead           =     2;
    uint8_t        nTxData[4]            =     {COMMAND_READ_REG, REG_XDATA_L};
    uint8_t        nRxData[SPI_ADXL_DUMMY_BYTES+4];
    int16_t       nValue;
    ADXL362_SPI_Receive(nTxData, nRxData, nLengthWrite, nLengthRead);
    nValue = nRxData[SPI_ADXL_DUMMY_BYTES+1] << 8;
    nValue += nRxData[SPI_ADXL_DUMMY_BYTES+0];
    return nValue;
}

/** @brief  This function returns the 12b y-axis acceleration data  
  *  
  * @return int16_t Y axis nValue 
  */
int16_t GetYdata12BAdxl362(void) {
    uint8_t        nLengthWrite           =     2;
    uint8_t        nLengthRead            =     2;
    uint8_t        nTxData[4]             =     {COMMAND_READ_REG, REG_YDATA_L};
    uint8_t        nRxData[SPI_ADXL_DUMMY_BYTES+4];
    int16_t       nValue;
    ADXL362_SPI_Receive(nTxData, nRxData, nLengthWrite, nLengthRead);
    nValue = nRxData[SPI_ADXL_DUMMY_BYTES+1] << 8;
    nValue += nRxData[SPI_ADXL_DUMMY_BYTES+0];
    return nValue;
}

/** @brief  This function returns the 12b z-axis acceleration data  
  *  
  * @return int16_t Z axis nValue 
  */
int16_t GetZdata12BAdxl362(void) {
    uint8_t        nLengthWrite           =     2;
    uint8_t        nLengthRead            =     2;
    uint8_t        nTxData[4]             =     {COMMAND_READ_REG, REG_ZDATA_L};
    uint8_t        nRxData[SPI_ADXL_DUMMY_BYTES+4];
    int16_t       nValue;
    ADXL362_SPI_Receive(nTxData, nRxData, nLengthWrite, nLengthRead);
    nValue = nRxData[SPI_ADXL_DUMMY_BYTES+1] << 8;
    nValue += nRxData[SPI_ADXL_DUMMY_BYTES+0];
    return nValue;
}

/** @brief  This function returns the 12b z-axis nTemperature data  
  *  
  * @return int16_t Z Temperature nValue  
  */
int16_t GetTdata12BAdxl362(void) {
    uint8_t        nLengthWrite           =     2;
    uint8_t        nLengthRead            =     2;
    uint8_t        nTxData[4]             =     {COMMAND_READ_REG, REG_TEMP_L};
    uint8_t        nRxData[SPI_ADXL_DUMMY_BYTES+4];
    int16_t       nValue;
    ADXL362_SPI_Receive(nTxData, nRxData, nLengthWrite, nLengthRead);
    nValue = nRxData[SPI_ADXL_DUMMY_BYTES+1] << 8;
    nValue += nRxData[SPI_ADXL_DUMMY_BYTES+0];
    return nValue;
}

/** @brief  This function returns the x-axis, y-axis and z-axis acceleration data.  
  * These 3 parameters must be passed by reference.This function obtains the same nValues than:                       
  * GetXdata12BAdxl362();GetYdata12BAdxl362();GetZdata12BAdxl362; 
  * This function is more efficient (less time)
  * @param[out]  *pnDataX pointer to X axis nValue. 12 bits.
  * @param[out]  *pnDataY pointer to Y axis nValue. 12 bits.
  * @param[out]  *pnDataZ pointer to Z axis nValue. 12 bits.
  * @return None
  */
void GetXYZdata12BAdxl362(int16_t *pnDataX, int16_t *pnDataY, 
			int16_t *pnDataZ) {
    uint8_t        nLengthWrite          =     2;
    uint8_t        nLengthRead           =     6;
    uint8_t        nTxData[8]            =     {COMMAND_READ_REG, REG_XDATA_L};
    uint8_t        nRxData[SPI_ADXL_DUMMY_BYTES+8];
    ADXL362_SPI_Receive(nTxData, nRxData, nLengthWrite, nLengthRead);
    *pnDataX = nRxData[SPI_ADXL_DUMMY_BYTES+1] << 8;
    *pnDataX += nRxData[SPI_ADXL_DUMMY_BYTES+0];
    *pnDataY = nRxData[SPI_ADXL_DUMMY_BYTES+3] << 8;
    *pnDataY += nRxData[SPI_ADXL_DUMMY_BYTES+2];
    *pnDataZ = nRxData[SPI_ADXL_DUMMY_BYTES+5] << 8;
    *pnDataZ += nRxData[SPI_ADXL_DUMMY_BYTES+4];
    nLengthWrite = 1;
}

/** @brief  This function returns the x-axis, y-axis and z-axis acceleration data.  
  * These 3 parameters must be passed by reference.
  * This function obtains the same nValues than: 
  * GetXdata12BAdxl362();GetYdata12BAdxl362();GetZdata12BAdxl362;                      
  * This function is more efficient (less time)
  * @param[out]  *pnDataX pointer to Y axis nValue. 12 bits.
  * @param[out]  *pnDataY pointer to Z axis nValue. 12 bits. 
  * @param[out]  *pnDataZ pointer to X axis nValue. 12 bits.
  * @param[out]  *pnTemperature pointer to Y axis Temperature nValue. 12 bits.
  * @return None
  */
void GetXYZTdata12BAdxl362(int16_t *pnDataX,
                           int16_t *pnDataY,
                           int16_t *pnDataZ,
                           int16_t *pnTemperature) {
    uint8_t        nLengthWrite          =     2;
    uint8_t        nLengthRead           =     8;
    uint8_t        nTxData[10]           =     {COMMAND_READ_REG, REG_XDATA_L};
    uint8_t        nRxData[SPI_ADXL_DUMMY_BYTES+10];
    ADXL362_SPI_Receive(nTxData, nRxData, nLengthWrite, nLengthRead);
    *pnDataX = nRxData[SPI_ADXL_DUMMY_BYTES+1] << 8;
    *pnDataX += nRxData[SPI_ADXL_DUMMY_BYTES+0];
    *pnDataY = nRxData[SPI_ADXL_DUMMY_BYTES+3] << 8;
    *pnDataY += nRxData[SPI_ADXL_DUMMY_BYTES+2];
    *pnDataZ = nRxData[SPI_ADXL_DUMMY_BYTES+5] << 8;
    *pnDataZ += nRxData[SPI_ADXL_DUMMY_BYTES+4];
    *pnTemperature = nRxData[SPI_ADXL_DUMMY_BYTES+7] << 8;
    *pnTemperature += nRxData[SPI_ADXL_DUMMY_BYTES+6];
}

/** @brief  This function will reset the sensor  
  *  
  * @return None  
  */
void SetSoftRestAdxl362(void) {
    uint8_t        nLenghtWrite  =     3;
    uint8_t        nTxData[3]    =     {COMMAND_WRITE, REG_SOFT_RESET, 0x52};
    ADXL362_SPI_Transmit(nTxData, nLenghtWrite);
}

/** @brief  This function sets all parameters to configure the FIFO function                                          
  *  
  * @param[in]  nTemperature:   This parameter  is written in       
  *             the bit called FIFO pnTemperature EMP inside FIFO_CONTROL 
  *             register. User should use constants below to enable or  
  *             disable the storing nTemperature data in the FIFO        
  *             PRM_FIFO pnTemperature EMP_ENABLE => Enables storing temp data       
  *             PRM_FIFO pnTemperature EMP_DISABLE=> Disables storing temp data  
  
  * @param[in]  nFifoMode: This parameter is written in             
  *             the bit called FIFO_MODE inside FIFO_CONTROL            
  *             register. User should use constants below to enable the 
  *             FIFO and selects the mode.                              
  *             PRM_FIFO_MODE_DISABLE      =>  FIFO is disabled         
  *             PRM_FIFO_MODE_FIFO_MODE    =>  FIFO Mode is enabled     
  *             PRM_FIFO_MODE_STREAM_MODE  =>  FIFO stream mode is enab.
  *             PRM_FIFO_MODE pnTemperature RIG_MODE    =>  FIFO trig. mode is enab. 
                   
  * @param[in]  nFifoSamples: This parameter is written in    
                FIFO_SAMPLES and AH (parameter iniside FIFO_CONTROL reg)
                The nValue in this register specifies number of samples 
                to store in the FIFO. The full range of FIFO Samples is 
                0 to 511.  
  
  * @return None  
  */
void SetFifoConfAdxl362(uint8_t nTemperature,
                        uint8_t nFifoMode,
                        uint16_t nFifoSamples) {
    uint8_t        nLenghtWrite            =     4;
    uint8_t        nTxData[4];
    uint16_t   nAux               =     (nFifoSamples & 0x0100) >> 5;
    nTxData[0]          =     COMMAND_WRITE;
    nTxData[1]          =     REG_FIFO_CONTROL;
    nTxData[2]          =     nAux | nTemperature | nFifoMode;
    nTxData[3]          =     (uint8_t)(nFifoSamples & 0xFF);
    /**
        The first argument to the function ADXL362_SPI_Transmit is the  the address
        of data read command and the register address of the ADXL device from where
        the data is to be read. The following address contains the data nValue to be
        written to the register. If more data is to be written to the consecutive
        register address, it can be written to the following address of the transmit
        buffer and the driver will write that data to the next register address.
        The 2nd argument is the size of the trasmitted data in bytes.
        ADXL362_SPI_Transmit() should be implemented in such a way that it transmits
        the data from nTxData buffer of size nLenghtWrite.
    */
    ADXL362_SPI_Transmit(nTxData, nLenghtWrite);
}

/** @brief  This function gets all parameters to configure the FIFO function                                          
  *  
  * @param[out]  *nTemperature:   This parameter  is written in       
  *              the bit called FIFO pnTemperature EMP inside FIFO_CONTROL 
  *              register. User should use constants below to enable or  
  *              disable the storing nTemperature data in the FIFO        
  *              PRM_FIFO pnTemperature EMP_ENABLE => Enables storing temp data       
  *              PRM_FIFO pnTemperature EMP_DISABLE=> Disables storing temp data  
  
  * @param[out]  *nFifoMode: This parameter is written in             
  *              the bit called FIFO_MODE inside FIFO_CONTROL            
  *              register. User should use constants below to enable the 
  *              FIFO and selects the mode.                              
  *              PRM_FIFO_MODE_DISABLE      =>  FIFO is disabled         
  *              PRM_FIFO_MODE_FIFO_MODE    =>  FIFO Mode is enabled     
  *              PRM_FIFO_MODE_STREAM_MODE  =>  FIFO stream mode is enab.
  *              PRM_FIFO_MODE pnTemperature RIG_MODE =>  FIFO trig.mode is enab. 
                   
  * @param[out]  *nFifoSamples: This parameter is written in    
                 FIFO_SAMPLES and AH (parameter iniside FIFO_CONTROL reg)
                 The nValue in this register specifies number of samples 
                 to store in the FIFO. The full range of FIFO Samples is 
                 0 to 511.    
  * @return None  
  */
void GetFifoConfAdxl362(uint8_t *nTemperature,
                        uint8_t *nFifoMode,
                        uint16_t *nFifoSamples) {
    uint8_t        nLengthWrite     =     2;
    uint8_t        nLengthRead      =     2;
    uint8_t        nTxData[4]       =     {COMMAND_READ_REG, REG_FIFO_CONTROL};
    uint8_t        nRxData[SPI_ADXL_DUMMY_BYTES+4];
    ADXL362_SPI_Receive(nTxData, nRxData, nLengthWrite, nLengthRead);
    *nTemperature         = nRxData[SPI_ADXL_DUMMY_BYTES+0] & PRM_FIFO_TEMP_MASK;
    *nFifoMode            = nRxData[SPI_ADXL_DUMMY_BYTES+0] & PRM_FIFO_MODE_MASK;
    *nFifoSamples         = ((((uint16_t)nRxData[SPI_ADXL_DUMMY_BYTES+0])&PRM_AH_MASK) << 5) 
			    + nRxData[SPI_ADXL_DUMMY_BYTES+1];
}

/** @brief  This function configures INT1 interrupt pin.  
  *
  * @param[in]  nValue: This parameter  is written in INTMAP1 register. 
  * User can use constants below to enable or 
  * disable each INTMAP1 parameter: 
  * | bit7  | PRM_INT_LOW_ACTIVE_HIGH or PRM_INT_LOW_ACTIVE_LOW 
  * | bit6  | PRM_AWAKE_ENABLE or PRM_AWAKE_DISABLE 
  * | bit5  | PRM_INACT_ENABLE or  PRM_INACT_DISABLE
  * | bit4  | PRM_ACT_ENABLE or  PRM_ACT_DISABLE
  * | bit3  | PRM_FIFO_OVERRUN_ENABLE or PRM_FIFO_OVERRUN_DISABLE
  * | bit2  | PRM_FIFO_WATERMARK_ENABLE or PRM_FIFO_WATERMARK_DISABLE
  * | bit1  | PRM_FIFO_READY_ENABLE or PRM_FIFO_READY_DISABLE 
  * | bit0  | PRM_DATA_READY_ENABLE or PRM_DATA_READY_DISABLE 
  * @return None
  */
void SetIntMap1Adxl362(uint8_t nValue) {
    uint8_t        nLenghtWrite            =     3;
    uint8_t        nTxData[3]    =     {COMMAND_WRITE, REG_INTMAP1, nValue};
    ADXL362_SPI_Transmit(nTxData, nLenghtWrite);
}

/** @brief  This function returns the ADXL362's INTMAP1 register.This register 
  *	includes the following bits that describe various conditions.     
  *
  * @return ADXL362's INTMAP1 register  
  */
uint8_t GetIntMap1Adxl362(void) {
    uint8_t        nLengthWrite          =     2;
    uint8_t        nLengthRead           =     1;
    uint8_t        nTxData[3]            =     {COMMAND_READ_REG, REG_INTMAP1};
    uint8_t        nRxData[SPI_ADXL_DUMMY_BYTES+3];
    uint8_t        nValue;
    ADXL362_SPI_Receive(nTxData, nRxData, nLengthWrite, nLengthRead);
    nValue = nRxData[SPI_ADXL_DUMMY_BYTES+0];
    return nValue;
}

/** @brief  This function configures INT2 interrupt pin.     
  *
  * @param[in] nValue This parameter  is written in INTMAP2 register.
  *  User can use constants below to enable or disable each INTMAP2 parameter:
  * | bit7  | PRM_INT_LOW_ACTIVE_HIGH or PRM_INT_LOW_ACTIVE_LOW 
  * | bit6  | PRM_AWAKE_ENABLE or PRM_AWAKE_DISABLE
  * | bit5  | PRM_INACT_ENABLE or  PRM_INACT_DISABLE 
  * | bit4  | PRM_ACT_ENABLE or  PRM_ACT_DISABLE 
  * | bit3  | PRM_FIFO_OVERRUN_ENABLE or PRM_FIFO_OVERRUN_DISABLE 
  * | bit2  | PRM_FIFO_WATERMARK_ENABLE or PRM_FIFO_WATERMARK_DISABLE 
  * | bit1  |  PRM_FIFO_READY_ENABLE or PRM_FIFO_READY_DISABLE 
  * | bit0  | PRM_DATA_READY_ENABLE or PRM_DATA_READY_DISABLE   
  * @return    None 
  */
void SetIntMap2Adxl362(uint8_t nValue) {
    uint8_t        nLenghtWrite            =     3;
    uint8_t        nTxData[3]    =     {COMMAND_WRITE, REG_INTMAP2, nValue};
    ADXL362_SPI_Transmit(nTxData, nLenghtWrite);
}

/** @brief  This function returns the ADXL362's INTMAP2 register. This register includes the following bits that descibe  various conditions.    
  *
  * @return uint8_t ADXL362's INTMAP2 register  
  */
uint8_t GetIntMap2Adxl362(void) {
    uint8_t        nLengthWrite           =     2;
    uint8_t        nLengthRead            =     1;
    uint8_t        nTxData[3]            =     {COMMAND_READ_REG, REG_INTMAP2};
    uint8_t        nRxData[SPI_ADXL_DUMMY_BYTES+3];
    uint8_t        nValue;
    ADXL362_SPI_Receive(nTxData, nRxData, nLengthWrite, nLengthRead);
    nValue = nRxData[SPI_ADXL_DUMMY_BYTES+0];
    return nValue;
}

/** @brief  This function configures ADXL362's filter.    
  *
  *@param nValue: This parameter  is written in FILTER_CTL register.
  *                       User can use constants below to enable or      
  *                       disable each FILTER_CTL parameter:                       
  *                       | bit7  | bit6  |                                  
  *                       PRM_RANGE_2GEE                                     
  *                       PRM_RANGE_4GEE                                     
  *                       PRM_RANGE_8GEE                                     
  *                       | bit5  | bit4  |                                  
  *                            X                                              
  *                       | bit3  |                                          
  *                       PRM_EXT_SAMPLE_ENABLE or PRM_EXT_SAMPLE_DISABLE    
  *                       | bit2  | bit1  | bit0  |                          
  *                       PRM_ODR_12_5                                       
  *                       PRM_ODR_25                                         
  *                       PRM_ODR_50                                         
  *                       PRM_ODR_100                                        
  *                       PRM_ODR_200                                        
  *                       PRM_ODR_400                                                                                                                    
  * @return  None  
  */
void SetFilterAdxl362(uint8_t nValue) {
    uint8_t        nLenghtWrite            =     3;
    uint8_t        nTxData[3]    =     {COMMAND_WRITE, REG_FILTER_CTL, nValue};
    ADXL362_SPI_Transmit(nTxData, nLenghtWrite);
}
/** @brief  This function reads all parameters which are related to filter.  
  *
  * @return  uint8_t ADXL362's FILTER_CTL register  User should use the
  * below masks to know the nValue of each parameter PRM_RANGE_MASK 
  * PRM_EXT_SAMPLE_MASK PRM_EXT_SAMPLE_MASK
  */
uint8_t GetFilterAdxl362(void) {
    uint8_t        nLengthWrite       =     2;
    uint8_t        nLengthRead        =     1;
    uint8_t        nTxData[3]         =     {COMMAND_READ_REG, REG_FILTER_CTL};
    uint8_t        nRxData[SPI_ADXL_DUMMY_BYTES+3];
    uint8_t        nValue;
    ADXL362_SPI_Receive(nTxData, nRxData, nLengthWrite, nLengthRead);
    nValue = nRxData[SPI_ADXL_DUMMY_BYTES+0];
    return nValue;
}

/** @brief  This function configures ADXL362's power control.
  *
  * @param  nValue: This parameter  is written in POWER_CTL register. 
  * User can use constants below to enable or 
  * disable each FILTER_CTL parameter:
  *                        | bit7  |                                          
  *                             X                                              
  *                        | bit6  |                                          
  *                        PRM_EXT_CLK_ENABLE                                 
  *                         PRM_EXT_CLK_DISABLE                                
  *                        | bit5  | bit4  |                                  
  *                        PRM_LOW_NOISE_ULTRA_LOW                            
  *                         PRM_LOW_NOISE_LOW_POWER                            
  *                        PRM_LOW_NOISE_LOW_NOISE                            
  *                         | bit3  |                                         
  *                        PRM_SLEEP_ENABLE or PRM_SLEEP_DISABLE              
  *                        | bit2  |                                          
  *                         PRM_AUTOSLEEP_ENABLE or PRM_AUTOSLEEP_DISABLE      
  *                         | bit1  | bit0  |                                  
  *                         PRM_MEASURE_STANDBY_MODE                          
  *                         PRM_MEASURE_MEASURE_MODE                           
  * @return  None
  */
void SetPowerControlAdxl362(uint8_t nValue) {
    uint8_t        nLenghtWrite            =     3;
    uint8_t        nTxData[3]    =     {COMMAND_WRITE, REG_POWER_CTL, nValue};
    ADXL362_SPI_Transmit(nTxData, nLenghtWrite);
}

/** @brief  This function sets the FIFO samples of ADXL362's   
  *
  * @param  nValue: This parameter  is written in REG_FIFO_SAMPLES register.                         
  * @return  None
  */
void SetFifoSamplesAdxl362(uint8_t nValue) {
    uint8_t        nLenghtWrite            =     3;
    uint8_t        nTxData[3]    =     {COMMAND_WRITE, REG_FIFO_SAMPLES, nValue} ;
    ADXL362_SPI_Transmit(nTxData, nLenghtWrite);
}

/** @brief  This function reads all parameters which are related to ADXL362's power settings
  *
  * @return  (uint8_t) ADXL362's POWER_CTL register. User should use the 
  *                   below masks to know the nValue of each parameter         
  *                   PRM_EXT_CLK_MASK                                        
  *                   PRM_LOW_NOISE_MASK                                      
  *                   PRM_SLEEP_MASK                                          
  *                   PRM_AUTOSLEEP_MASK                                      
  *                   PRM_MEASURE_MASK
*/
uint8_t GetPowerControlAdxl362(void) {
    uint8_t        nLengthWrite           =     2;
    uint8_t        nLengthRead            =     1;
    uint8_t        nTxData[3]            =     {COMMAND_READ_REG, REG_POWER_CTL};
    uint8_t        nRxData[SPI_ADXL_DUMMY_BYTES+3];
    uint8_t        nValue;
    ADXL362_SPI_Receive(nTxData, nRxData, nLengthWrite, nLengthRead);
    nValue = nRxData[SPI_ADXL_DUMMY_BYTES+0];
    return nValue;
}

/** @brief  This function configures ADXL362's self test
  *
  * @param  nValue: This parameter  is written in SELF pnTemperature EST register. 
  * User can use constants below to enable or disable each SELF pnTemperature EST parameter: 
  * | bit7  | bit6  | bit5  | bit4  | bit3  | bit2  | bit1  | X  | bit0  | 
  * PRM_ST_ENABLE or PRM_ST_DISABLE    
  * @return  None 
*/
void SetSelfTestAdxl362(uint8_t nValue) {
    uint8_t        nLenghtWrite            =     3;
    uint8_t        nTxData[3]    =     {COMMAND_WRITE, REG_SELF_TEST, nValue};
    ADXL362_SPI_Transmit(nTxData, nLenghtWrite);
}

/** @brief  This function reads SELF pnTemperature EST registar and we can know if ADXL362's test is enabled. 
  *
  * @return  (uint8_t) ADXL362's SELF pnTemperature EST register. User should use the 
  *  below masks to know the nValue of each parameter         
  *  PRM_ST_MASK  
*/
uint8_t GetSelfTestAdxl362(void) {
    uint8_t        nLengthWrite         =     2;
    uint8_t        nLengthRead          =     1;
    uint8_t        nTxData[3]           =     {COMMAND_READ_REG, REG_SELF_TEST};
    uint8_t        nRxData[SPI_ADXL_DUMMY_BYTES+3];
    uint8_t        nValue;
    ADXL362_SPI_Receive(nTxData, nRxData, nLengthWrite, nLengthRead);
    nValue = nRxData[SPI_ADXL_DUMMY_BYTES+0];
    return nValue;
}

/** @brief  This function reads nReadFifoLength bytes from ADXL362's FIFO.
  * Those bytes are stored in *pFifoData. 
  *
  * @param  nReadFifoLength: number of bytes to read from FIFO.      
  *         this nValue must be between 0 - 511. 
  * @param  *pFifoData: This parameter  stores bytes read from
  *         the ADXL362's FIFO. 
  * @return  None
*/
void GetFifo(uint16_t nReadFifoLength, 
             uint8_t * pFifoData) {
    uint8_t        nLengthWrite    =     1;
    uint16_t       nLengthRead     =     nReadFifoLength;
    uint8_t        nTxData[1];
    nTxData[0] = COMMAND_READ_BUFFER;
    /**
        The first argument to the function ADXL362_SPI_Receive is the address of
        the data read command.
        The 2nd argument is the pointer to the buffer of received data.
        The 3rd argument is the size of the trasmitted data and the 4th argument 
        is the size of the received data in bytes.
    */
    ADXL362_SPI_Receive(nTxData, pFifoData, nLengthWrite, nLengthRead);

}

/** @brief  This function reads nFifoBytes bytes from ADXL362's FIFO.
  *         Those bytes are arranged and loaded in its corresponding array.
  *
  * @param[in]  nFifoBytes: number of bytes to read from FIFO.       
  *             this nValue must be between 0 - 511. In this function,
  *             nFifoBytes should be multiple of 6. 
  * @param[out]  *pnDataX: This parameter  stores X-axis
  *              tnValues which were stored in the FIFO between the position
  *              0 and nReadFifoLength of the ADXL362's FIFO.   
  * @param[out] *pnDataY: This parameter  stores Y-axis
  *              tnValues which were stored in the FIFO between the position
  *              0 and nReadFifoLength of the ADXL362's FIFO.  
  * @param[out]  *pnDataZ: This parameter  stores Z-axis
  *              tnValues which were stored in the FIFO between the position
  *              0 and nReadFifoLength of the ADXL362's FIFO.  
  * @return  return uint8_t A 8-bit integer: 0 - success; < 0 - failure 
*/

uint8_t  anReadDataArray[MAX_WATER_MARK_BYTES + 1];
uint8_t *pReadDataArray  = &anReadDataArray[1];
uint8_t GetFifoXYZ(uint16_t  nFifoBytes,
                   uint16_t *pnDataX      ,
                   uint16_t *pnDataY      ,
                   uint16_t *pnDataZ      ) {

    uint8_t  nErrorStatus = 0;
    uint16_t  nBytesToRead = 0;
//    pReadDataArray = &pReadDataArray1[1];    
    while (nFifoBytes > 0) {
        if (nFifoBytes >= MAX_WATER_MARK_BYTES) {
            nBytesToRead = MAX_WATER_MARK_BYTES;
        }
        else {
            nBytesToRead = nFifoBytes;
        }
        GetFifo(nBytesToRead, anReadDataArray);
        uint16_t   nAux  = 0;
        for (uint16_t i = 0; i < (nBytesToRead >> 1); i++) {
            nAux      =     pReadDataArray[(i << 1) + 1];
            nAux      =     (nAux << 8) + pReadDataArray[(i << 1) + 0];
            if (  (nAux & PRM_DATA_TYPE_MASK) == PRM_DATA_TYPE_X_AXIS ) {
                *pnDataX = (nAux & PRM_DATA_VALUE_MASK) | 
				((nAux & PRM_SIGN_EXTENSION_MASK) << 2);
                pnDataX++;
            }
            else if (  (nAux & PRM_DATA_TYPE_MASK) == PRM_DATA_TYPE_Y_AXIS ) {
                *pnDataY = (nAux & PRM_DATA_VALUE_MASK) | 
				((nAux & PRM_SIGN_EXTENSION_MASK) << 2);
                pnDataY++;
            }
            else if (  (nAux & PRM_DATA_TYPE_MASK) == PRM_DATA_TYPE_Z_AXIS ) {
                *pnDataZ = (nAux & PRM_DATA_VALUE_MASK) | 
				((nAux & PRM_SIGN_EXTENSION_MASK) << 2);
                pnDataZ++;
            }
            else {
                nErrorStatus = 1;
            }
        }
        if (nFifoBytes > MAX_WATER_MARK_BYTES) {
            nFifoBytes -= MAX_WATER_MARK_BYTES;
        }
        else {
            nFifoBytes = 0;
        }
    }
    return nErrorStatus;
}

/** @brief  This function reads nFifoBytes bytes from ADXL362's FIFO.
  *         Those bytes are arranged and loaded in its corresponding array.
  *
  * @param[in]  nReadFifoLength: number of bytes to read from FIFO.       
  *             this nValue must be between 0 - 511. In this function,
  *             nFifoBytes should be multiple of 6. 
  * @param[out]  *pnDataX: This parameter  stores X-axis
  *              tnValues which were stored in the FIFO between the position
  *              0 and nReadFifoLength of the ADXL362's FIFO.   
  * @param[out]  *pnDataY: This parameter  stores Y-axis
  *              tnValues which were stored in the FIFO between the position
  *              0 and nReadFifoLength of the ADXL362's FIFO.  
  * @param[out]  *pnDataZ: This parameter  stores Z-axis
  *              tnValues which were stored in the FIFO between the position
  *              0 and nReadFifoLength of the ADXL362's FIFO.  
  * @param[out]  *pnTemperature: This parameter  stores nTemperature
  *             nValues which were stored in the FIFO between the position
  *             nValues which were stored in the FIFO between the position
  * @return  None 
*/

void GetFifoXYZT(uint16_t nReadFifoLength,
                 int16_t *pnDataX      ,
                 int16_t *pnDataY      ,
                 int16_t *pnDataZ      ,
                 int16_t *pnTemperature      ) {
    uint8_t  anReadDataArray[512];
    GetFifo(nReadFifoLength, anReadDataArray);
    uint16_t   nAux  = 0;
    for (uint16_t i = 0; i < (nReadFifoLength >> 1); i++) {
        nAux      =     anReadDataArray[(i << 1) + 1];
        nAux      =     (nAux << 8) + anReadDataArray[(i << 1) + 0];
        if (  (nAux & PRM_DATA_TYPE_MASK) == PRM_DATA_TYPE_X_AXIS ) {
            *pnDataX = (nAux & PRM_DATA_VALUE_MASK) | 
			                 ((nAux & PRM_SIGN_EXTENSION_MASK) << 2);
            pnDataX++;
        } else if (  (nAux & PRM_DATA_TYPE_MASK) == PRM_DATA_TYPE_Y_AXIS ) {
            *pnDataY = (nAux & PRM_DATA_VALUE_MASK) | 
					   ((nAux & PRM_SIGN_EXTENSION_MASK) << 2);
            pnDataY++;
        } else if (  (nAux & PRM_DATA_TYPE_MASK) == PRM_DATA_TYPE_Z_AXIS ) {
            *pnDataZ = (nAux & PRM_DATA_VALUE_MASK) | 
			           ((nAux & PRM_SIGN_EXTENSION_MASK) << 2);
            pnDataZ++;
        }
        else { // if (  (nAux & PRM_DATA_TYPE_MASK) == PRM_DATA_TYPE_T_AXIS )
            *pnTemperature = (nAux & PRM_DATA_VALUE_MASK) | 
			                 ((nAux & PRM_SIGN_EXTENSION_MASK) << 2);
            pnTemperature++;
        }

    }
}
/******************************************************************************/

/******************************************************************************/
/*                    INTERNAL FUNCTIONS                                      */
/******************************************************************************/
/** @brief  This function sets register for Adxl362.
  *  
  * @param  nReg  Register Address 
  * @param  nValue: This parameter is written in the register.    
  * @return None
  */
void SetRegisterAdxl362(uint8_t nReg, uint8_t nValue) {
    uint8_t        nLenghtWrite            =     3;
    uint8_t        nTxData[3]    =     {COMMAND_WRITE, nReg, nValue} ;
    ADXL362_SPI_Transmit(nTxData, nLenghtWrite);
}

/** @brief  This function gets the register for Adxl362.
  *  
  * @param[in]  nReg  Register Address 
  * @param[in]  nReadFifoLength  Read FIFO Length  
  * @param[out] *pnValue: This parameter get the register value.    
  * @return None
  */
void GetRegisterAdxl362(uint8_t nReg, uint16_t nReadFifoLength, 
						uint8_t *pnValue) {
    uint8_t        nLengthWrite           =     2;
    uint8_t        nLengthRead            =     1;
    uint8_t        nTxData[3]            =     {COMMAND_READ_REG, nReg} ;
    uint8_t        nRxData[SPI_ADXL_DUMMY_BYTES+3];
    ADXL362_SPI_Receive(nTxData, nRxData, nLengthWrite, nLengthRead);
    *pnValue = nRxData[SPI_ADXL_DUMMY_BYTES+0];
}

/******************************************************************************/
/** @brief  This function gets the Adxl Ids.
  *  
  * @param  *nDataA pointer to X axis nValue
  * @param  *nDataB pointer to Y axis nValue
  * @param  *nDataC pointer to Z axis nValue
  * @return None 
  */
void GetIdsAdxl362(int16_t *nDataA, int16_t *nDataB, int16_t *nDataC) {
    uint8_t        nLengthWrite           =     2;
    uint8_t        nLengthRead            =     3;
    uint8_t        nTxData[8]            =     {COMMAND_READ_REG, REG_DEVID_AD} ;
    uint8_t        nRxData[SPI_ADXL_DUMMY_BYTES+8];
    ADXL362_SPI_Receive(nTxData, nRxData, nLengthWrite, nLengthRead);
    *nDataA = nRxData[SPI_ADXL_DUMMY_BYTES+0];
    *nDataB = nRxData[SPI_ADXL_DUMMY_BYTES+1];
    *nDataC = nRxData[SPI_ADXL_DUMMY_BYTES+2];
    nLengthWrite = 1;
}
/**@}*/ /* end of group Adxl_Driver */
/**@}*/ /* end of group Device_Drivers */
