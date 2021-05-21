ReadMe.txt
==========

ADXLDrv Version: 2.0

Release Notes
-------------
Version 0.9, Sept 24, 2014                                                  
  Change FIFO filled threshold                                              
Version 1.0, Oct 29, 2014                                                   
  Add timestamp output                                                      
Version 1.1, Jan 28, 2015                                                   
  Fixed timestamp related bug                                               
Version 1.2, Feb 27, 2015                                                   
  Bug fix in timestamp calculation for watermark more than one sample       
Version 1.3, Mar  9, 2015                                                   
  Added setting of ODR and watermark at init	
Version 1.3.1, Apr 2, 2015
  Fixed bug with timestamp when the watermark is greater than 1
Version 1.3.2, Jun 17, 2015
  Removed unused argument of AdxlISR()
Version 2.0, Oct 12, 2015                                                   
  Simplified the driver code by moving the ring buffer to the application   
  New functions and API added   

  
HAL Functions
-------------  

-----> Typedefs used <-----
typedef enum
{
  ADI_HAL_OK       = 0x00,
  ADI_HAL_ERROR    = 0x01,
  ADI_HAL_BUSY     = 0x02,
  ADI_HAL_TIMEOUT  = 0x03
} ADI_HAL_STATUS_t;

-----> void GPIO_IRQ_ADXL362_Disable(void) <-----
* Disable the interrupt line supporting the ADXL362

-----> void GPIO_IRQ_ADXL362_Enable(void) <-----
* Enable the interrupt line supporting the ADXL362

-----> void MCU_HAL_Delay(uint32_t delay) <-----
* Implement a millisecond delay, specified by the value "delay".

-----> uint32_t MCU_HAL_GetTick() <-----
* returns the current clock tick in milliseconds.

-----> ADXL362_SPI_Transmit <-----
* Transmits the buffer pointed to by "pData", where the size is specified by "Size".
ADI_HAL_STATUS_t ADXL362_SPI_Transmit(uint8_t *pData, uint16_t Size) 

Example:
void setINTMAP1_ADXL362(uint8_t _value) {

      uint8_t        lenghtWrite            =     3;
      uint8_t        pTxData[3]    =     {COMMAND_WRITE,REG_INTMAP1,_value} ;
      ADXL362_SPI_Transmit(pTxData,lenghtWrite);
}

-----> ADXL362_SPI_Receive <-----
* Does a transmit and receive, transmitting "pTxData" of "Size", receiving data in pRxData.
* It is assumed that the receiver knows the expected receive data size based on the command sent to the device.

Example:
uint8_t getREVID_ADXL362(void)
{
      uint8_t        lengthWrite           =     2;
      uint8_t        lengthRead            =     1;
      uint8_t        totalLength           =     lengthWrite + lengthRead;
      uint8_t        pTxData[3]            =     {COMMAND_READ_REG,REG_REVID} ;
      uint8_t        pRxData[3];
      uint8_t        value;
      ADXL362_SPI_Receive(pTxData,pRxData,totalLength);
      value = pRxData[0];
      return value;
}

Usage
-----  

#include "Adxl362.h"

static void DriverBringUp();

void main(void) {
 
    AdxlDrvDataReadyCallback(AdxlFifoCallBack);
    AdxlRxBufferInit();
    /* Opens the driver, sets operation mode and reads data */
    DriverBringUp();
    AdxlDrvCloseDriver();
}

/**
  * @internal
  * @brief    Read data from the ADXL362
  * @param    file: pointer to the source file name
  * @param    data: point to an array in which the data will be placed
  * @retval   error
  */
int32_t ReadADXL362Data(int16_t *data, uint32_t *pTimeStamp) {

    int16_t value[3] = {0};
    int32_t i = 0;
    uint16_t nAdxlFifoLevelSize = 0;
    uint32_t nTcv = 0;
    uint32_t nIndexCnt;
    int16_t nRetValue;
    uint32_t nAdxlTimeGap, nAdxlFifoLevelEvenSize;

    if (data == 0 || pTimeStamp == 0)
        return -1;

    if(gnAdxlDataReady){

            gnAdxlDataReady = 0;
            AdxlDrvGetParameter(ADXL_FIFOLEVEL, &nAdxlFifoLevelSize);
            nAdxlFifoLevelEvenSize = ((nAdxlFifoLevelSize>>1)<<1);
            nTcv = MCU_HAL_GetTick();
            nAdxlTimeGap = AdxlDrvMwGetTimeGap(gnAdxlTimeGap);
            if (nTcv > gnAdxlTimeCurVal)
                 gnAdxlTimeCurVal += (((nTcv - gnAdxlTimeCurVal)/nAdxlTimeGap) * nAdxlTimeGap);
            nTcv = gnAdxlTimeCurVal - ((nAdxlFifoLevelEvenSize * nAdxlTimeGap) - nAdxlTimeGap);

            if (nAdxlFifoLevelSize > 0) {
                nRetValue = AdxlDrvReadFifoData(&gnXDataArray[0],
                        &gnYDataArray[0], &gnZDataArray[0], nAdxlFifoLevelEvenSize);
                AdxlDrvMwDecimateSamples(&nAdxlFifoLevelEvenSize, &gnXDataArray[0], &gnYDataArray[0], &gnZDataArray[0]);
                if (nRetValue == ADXLDrv_SUCCESS) {
                  gnAdxlTimeIrqSet1 = 0;
                    for (nIndexCnt = 0; nIndexCnt < nAdxlFifoLevelEvenSize;
                                                            nIndexCnt++) {
                        AdxlRxBufferInsertData(nIndexCnt, nTcv);
                    }

                }
            }
    }

    if (AdxlDrvReadData(&value[0], pTimeStamp) != ADXLDrv_SUCCESS)
            return -2;

    for (i = 0; i < 3; i++) {

        data[i] = (int16_t)value[i];
    }

    return 0;
}


/**
      @brief  Create a Ring Buffer to store received data.
      @param  None
      @retval None
*/
static void AdxlRxBufferInit(void) {
    uint8_t i;
    AdxlWtBufferPtr = &AdxlRxBuff[0];
    AdxlRdBufferPtr = &AdxlRxBuff[0];
    for (i = 0; i < ADXL_RX_BUFFER_SIZE; i++) {
        AdxlRxBuff[i].Next = &AdxlRxBuff[i + 1];
    }
    AdxlRxBuff[ADXL_RX_BUFFER_SIZE - 1].Next = &AdxlRxBuff[0];
}

/**
      @brief  Move data from device FIFO to this Ring Buffer. If WtPtr reach RdPtr,
              host's reading is too slow. Toggle LED to indicate this event.
      @param  index Index of the data in the FIFO
      @param  tcv Timestamp of the oldest data in the FIFO
      @retval uint16_t A 16-bit integer: 0 - success; < 0 - failure
*/
uint16_t AdxlRxBufferInsertData(uint32_t nIndex, uint32_t nTcv) {
    uint32_t nTimeStamp;
    if (AdxlWtBufferPtr->Next == AdxlRdBufferPtr) {
        return ADXLDrv_ERROR;
    }

    if (gnAdxlTimeIrqSet == 1 && gnAdxlTimeIrqSet1 == 0) {
        nTimeStamp = nTcv;
        gnAdxlTimeIrqSet = 0;
    } else {
        nTimeStamp = gnAdxlTimePreVal + gnAdxlTimeGap;
    }
    gnAdxlTimePreVal = nTimeStamp;


    AdxlWtBufferPtr->TimeStamp = nTimeStamp;
    AdxlWtBufferPtr->DataValue[0] = gnXDataArray[nIndex];
    AdxlWtBufferPtr->DataValue[1] = gnYDataArray[nIndex];
    AdxlWtBufferPtr->DataValue[2] = gnZDataArray[nIndex];
    AdxlWtBufferPtr = AdxlWtBufferPtr->Next;
    return ADXLDrv_SUCCESS;
}


/**
* DriverBringUp(): Sets up the ADXL driver for reading data
*/

static void DriverBringUp() {

    uint32_t nIndexCnt;
    uint32_t timeADXL = 0, LoopCnt;
    int16_t anXclData[3] = {0};
    int16_t nRetValue;
    uint16_t nRate = 100, nWaterMarkLevel = 1, nAdxlFifoLevelSize, nSampleRate;
    AdxlDrvGetParameter(ADXL_ODRRATE,&nSampleRate);
    AdxlDrvSoftReset();

    AdxlDrvOpenDriver(nRate, nWaterMarkLevel);
    AdxlDrvSetOperationMode(PRM_MEASURE_MEASURE_MODE);

    while (1) {
        if (gnAdxlIRQReady == 1) {
          gnAdxlIRQReady = 0;
            AdxlDrvGetParameter(ADXL_FIFOLEVEL, &nAdxlFifoLevelSize);
            if (nAdxlFifoLevelSize > 0) {
                nRetValue = AdxlDrvReadFifoData(&gnXDataArray[0],
                        &gnYDataArray[0], &gnZDataArray[0], nAdxlFifoLevelSize);
                if (nRetValue == ADPDDrv_SUCCESS) {
                    for (nIndexCnt = 0; nIndexCnt < nAdxlFifoLevelSize;
                                                            nIndexCnt++) {
                        AdxlRxBufferInsertData(nIndexCnt,0);
                    }
                    AdxlDrvReadData(&anAlignXclData[0],0);
                    for (LoopCnt = 0; LoopCnt < 3; LoopCnt++) {
                        debug("%i ", anAlignXclData[LoopCnt]);
                    }
                    debug("\r\n");
                }
            }
        }
        MCU_HAL_Delay(5); //Give the CPU a break
    }

}
