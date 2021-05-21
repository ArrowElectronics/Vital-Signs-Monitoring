/**
***************************************************************************
* @file         adxl362.h
* @author       ADI
* @version
* @date         26-Jun-2019
* @brief    Include file for the ADXL device driver.
*           This library describes functions user can use to
*           control the accelerometer called ADXL362. User must
*           implement SPI's functions.
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
* This software is intended for use with the AD5950 and derivative parts     *
* only                                                                        *
*                                                                             *
******************************************************************************/


/**
******************************************************************************
* @file     adxl362.h
* @author   ADI
* @version  V2.0
* @date     20-Oct-2015
* @brief    Include file for the ADXL device driver.
*           This library describes functions user can use to
*           control the accelerometer called ADXL362. User must
*           implement SPI's functions.
******************************************************************************
* @attention
******************************************************************************
*/
/******************************************************************************
*                                                                             *
* License Agreement                                                           *
*                                                                             *
* Copyright (c) 2014-2015 Analog Devices Inc.                                 *
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
* This software is intended for use with the ADPD and derivative parts     *
* only                                                                        *
*                                                                             *
******************************************************************************/

#ifndef __ADXL362_H
#define __ADXL362_H

#include "stdint.h"
/******************************************************************************/
/*                    PROTOTYPE FUNCTIONS                                     */
/******************************************************************************/

              uint8_t GetDevIdAdAdxl362(void);
              uint8_t GetDevIdMstAdxl362(void);
              uint8_t GetPartIdAdxl362(void);
              uint8_t GetRevIdAdxl362(void);
              int8_t GetXdata8BAdxl362(void);
              int8_t GetYdata8BAdxl362(void);
              int8_t GetZdata8BADXL362(void);
              void GetXYZdata8BAdxl362(int8_t *nDataX, int8_t *nDataY, int8_t *nDataZ);
              uint8_t GetStatusAdxl362(void);
              uint16_t GetFifoEntriesAdxl362(void);
              int16_t GetXdata12BAdxl362(void);
              int16_t GetYdata12BAdxl362(void);
              int16_t GetZdata12BAdxl362(void);
              int16_t GetTdata12BAdxl362(void);
              void GetXYZdata12BAdxl362(int16_t *nDataX, int16_t *nDataY, int16_t *nDataZ);
              void GetXYZTdata12BAdxl362(int16_t *nDataX, int16_t *nDataY, int16_t *nDataZ, int16_t *_nTemperature);
              void SetSoftRestAdxl362(void);
	          void SetActivityAndInactivityAdxl362(uint8_t nControlByte, uint16_t nThresholdAct, uint8_t nTimeAct, uint16_t nThresholdInAct, uint16_t nTimeInact);
              void GetActivityAndInactivityAdxl362(uint8_t *nControlByte, uint16_t *nThresholdAct, uint8_t *nTimeAct, uint16_t *nThresholdInAct, uint16_t *nTimeInact);
              void SetFifoConfAdxl362(uint8_t nTemperature, uint8_t nFifoMode, uint16_t nFifoSamples);
	          void GetFifoConfAdxl362(uint8_t *nTemperature, uint8_t *nFifoMode, uint16_t *nFifoSamples);
              void SetIntMap1Adxl362(uint8_t nValue);
              uint8_t GetIntMap1Adxl362(void);
	          void SetIntMap2Adxl362(uint8_t nValue);
              uint8_t GetIntMap2Adxl362(void);
              void SetFilterAdxl362(uint8_t nValue);
              uint8_t GetFilterAdxl362(void);
              void SetPowerControlAdxl362(uint8_t nValue);
              uint8_t GetPowerControlAdxl362(void);
              void SetSelfTestAdxl362(uint8_t nValue);
              uint8_t GetSelfTestAdxl362(void);
              void GetFifo(uint16_t nReadFifoLength, uint8_t *anReadDataArray);
              uint8_t GetFifoXYZ(uint16_t nReadFifoLength, uint16_t *nDataX, uint16_t *nDataY, uint16_t *nDataZ);
              void GetFifoXYZT(uint16_t nReadFifoLength, int16_t *nDataX, int16_t *nDataY, int16_t *nDataZ, int16_t *nDataT);
              void SetRegisterAdxl362(uint8_t nReg, uint8_t nValue);
              void GetRegisterAdxl362( uint8_t nReg, uint16_t nReadFifoLength, uint8_t *nValue);
              void GetBufferAdxl362(uint16_t nReadFifoLength, uint8_t *anReadDataArray);
              void SetFifoSamplesAdxl362(uint8_t nValue);
              uint32_t AdxlDrvGetDebugInfo();

/******************************************************************************/


/******************************************************************************/
/*                    ADXL362 COMMANDS MAP                                    */
/******************************************************************************/
#define COMMAND_WRITE            0x0A                                           //  write register
#define COMMAND_READ_REG         0x0B                                           //  read register
#define COMMAND_READ_BUFFER      0x0D                                           //  read buffer
/******************************************************************************/


/******************************************************************************/
/*                    ADXL362 REGISTER MAP                                    */
/******************************************************************************/
#define REG_DEVID_AD              0x00                                          //  This register contains an Analog Devices device ID of 0xAD.
#define REG_DEVID_MST             0x01                                          //  This register contains an Analog Devices MEMS device ID of 0x1D.
#define REG_PARTID                0x02                                          //  This register contains the device ID of 0xF2.
#define REG_REVID                 0x03                                          //  This register contains a product revision ID, beginning with 0x00 and incrementing by 1 count for each subsequent revision.
#define REG_XDATA                 0x08                                          //  This register holds the 8 most-significant bits of the x-axis acceleration data. This is provided for use in power-conscious applications where 8 bits of data are sufficient: energy can be conserved by reading only 1 byte of data per axis, rather than 2
#define REG_YDATA                 0x09                                          //  This register holds the 8 most-significant bits of the y-axis acceleration data. This is provided for use in power-conscious applications where 8 bits of data are sufficient: energy can be conserved by reading only 1 byte of data per axis, rather than 2.
#define REG_ZDATA                 0x0A                                          //  This register holds the 8 most-significant bits of the z-axis acceleration data. This is provided for use in power-conscious applications where 8 bits of data are sufficient: energy can be conserved by reading only 1 byte of data per axis, rather than 2.
#define REG_STATUS                0x0B                                          //  This register includes the following bits that describe various conditions of the ADXL362.
#define REG_FIFO_ENTRIES_L        0x0C                                          //  These registers indicate the number of valid data samples present in the FIFO buffer. This number ranges from 0 to 512, or 0x00 to 0x200.
#define REG_FIFO_ENTRIES_H        0x0D                                          //  These registers indicate the number of valid data samples present in the FIFO buffer. This number ranges from 0 to 512, or 0x00 to 0x200.
#define REG_XDATA_L               0x0E                                          //  These two registers contain the x-axis acceleration data. XDATA_L contains the 8 least-significant bits (LSB's), and XDATA_H contains the 4 most-significant bits (MSB's) of the 12-bit value.
#define REG_XDATA_H               0x0F                                          //  These two registers contain the x-axis acceleration data. XDATA_L contains the 8 least-significant bits (LSB's), and XDATA_H contains the 4 most-significant bits (MSB's) of the 12-bit value.
#define REG_YDATA_L               0x10                                          //  These two registers contain the y-axis acceleration data. YDATA_L contains the 8 least-significant bits (LSB's), and YDATA_H contains the 4 most-significant bits (MSB's) of the 12-bit value.
#define REG_YDATA_H               0x11                                          //  These two registers contain the y-axis acceleration data. YDATA_L contains the 8 least-significant bits (LSB's), and YDATA_H contains the 4 most-significant bits (MSB's) of the 12-bit value.
#define REG_ZDATA_L               0x12                                          //  These two registers contain the z-axis acceleration data. ZDATA_L contains the 8 least-significant bits (LSB's), and ZDATA_H contains the 4 most-significant bits (MSB's) of the 12-bit value.
#define REG_ZDATA_H               0x13                                          //  These two registers contain the z-axis acceleration data. ZDATA_L contains the 8 least-significant bits (LSB's), and ZDATA_H contains the 4 most-significant bits (MSB's) of the 12-bit value.
#define REG_TEMP_L                0x14                                          //  These two registers contain the temperature sensor output. TEMP_L contains the 8 least-significant bits (LSB's), and TEMP_H contains the 4 most-significant bits (MSB's) of the 12-bit value. The value is sign-extended, so bits D15:D12 of TEMP_H will be all 0's or all 1's based on the value of bit D11.
#define REG_TEMP_H                0x15                                          //  These two registers contain the temperature sensor output. TEMP_L contains the 8 least-significant bits (LSB's), and TEMP_H contains the 4 most-significant bits (MSB's) of the 12-bit value. The value is sign-extended, so bits D15:D12 of TEMP_H will be all 0's or all 1's based on the value of bit D11.
#define REG_SOFT_RESET            0x1F                                          //  Writing the code 0x52 ("R") to this register will immediately reset the sensor. All register settings will be cleared and the sensor will be placed in standby mode.
#define REG_THRESH_ACT_L          0x20                                          //  This 11-bit unsigned value sets the threshold for activity detection. The actual value in Gees depends on the measurement range setting selected. To detect activity, the absolute value of the 12-bit acceleration data is compared with the 11-bit THRESH_ACT value.
#define REG_THRESH_ACT_H          0x21                                          //  This 11-bit unsigned value sets the threshold for activity detection. The actual value in Gees depends on the measurement range setting selected. To detect activity, the absolute value of the 12-bit acceleration data is compared with the 11-bit THRESH_ACT value.
#define REG_TIME_ACT              0x22                                          //  The value in this register sets the number of consecutive samples that must be greater than the activity threshold (set by THRESH_ACT) for an Activity event to be detected. more details in page 25 of 39
#define REG_THRESH_INACT_L        0x23                                          //  This 11-bit unsigned value sets the threshold for inactivity detection. The actual value in Gees depends on the measurement range setting selected. To detect inactivity, the absolute value of the 12-bit acceleration data is compared with the 11-bit THRESH_INACT value.
#define REG_THRESH_INACT_H        0x24                                          //  This 11-bit unsigned value sets the threshold for inactivity detection. The actual value in Gees depends on the measurement range setting selected. To detect inactivity, the absolute value of the 12-bit acceleration data is compared with the 11-bit THRESH_INACT value.
#define REG_TIME_INACT_L          0x25                                          //  The 16-bit value in these registers sets the number of consecutive samples that must be lower than the inactivity threshold (set by THRESH_INACT) for an Inactivity event to be detected. The 16-bit value allows for long inactivity detection times.
#define REG_TIME_INACT_H          0x26                                          //  The 16-bit value in these registers sets the number of consecutive samples that must be lower than the inactivity threshold (set by THRESH_INACT) for an Inactivity event to be detected. The 16-bit value allows for long inactivity detection times.
#define REG_ACT_INACT_CTL         0x27                                          //  This register controls the ADXL362's ACTIVITY/INACTIVITY.
#define REG_FIFO_CONTROL          0x28                                          //  This register controls the ADXL362's FIFO MODE.
#define REG_FIFO_SAMPLES          0x29                                          //  The value in this register specifies number of samples to store in the FIFO. The AH bit in the FIFO Control Register (Register 0x28) can be used as the MSB of this value, effectively doubling its range.
#define REG_INTMAP1               0x2A                                          //  These registers configures the INT1/INT2 interrupt pins, respectively. Bits B6:B0 select which function(s) will trigger an interrupt on the pin. If its corresponding bit is '1', the function will trigger an interrupt on the INT pin.  Bit D7 configures whether the pin will operate in active-high (B7 low) or active-low (B7 high) mode.
#define REG_INTMAP2               0x2B                                          //  Any number of functions can be selected simultaneously for each pin. If multiple functions are selected, their conditions are OR'ed together to determine the INT pin state. The status of each individual function can be determined by reading the STATUS register.
#define REG_FILTER_CTL            0x2C                                          //  This register controls the ADXL362's filter.
#define REG_POWER_CTL             0x2D                                          //  This registre controls ADXL362's status.
#define REG_SELF_TEST             0x2E                                          //  This registre enables the ADXL362's self test
/******************************************************************************/



/******************************************************************************/
/*                    MASK FOR STATUS REGISTER                                */
/******************************************************************************/
#define PRM_ERR_USER_REGS_MASK    ((0x1)<<7)                                    //  SEU Error Detect. A '1' indicates one of two things: either an SEU event, such as an alpha particle of power glitch, has disturbed a user register setting; or the ADXL362 is unconfigured.
#define PRM_AWAKE_MASK            ((0x1)<<6)                                    //  Indicates whether the accelerometer is in an "active" (AWAKE = 1) or "inactive" (AWAKE = 0) state, based on the Activity and Inactivity functionality, when the Link Mode is used. When Link Mode is not used, this bit defaults to a '1' and should be ignored.
#define PRM_INACT_MASK            ((0x1)<<5)                                    //  Inactivity. A '1' indicates that the inactivity function has detected an inactivity or a free-fall condition.
#define PRM_ACT_MASK              ((0x1)<<4)                                    //  Activity. A '1' indicates that the activity function has detected an over-threshold condition.
#define PRM_FIFO_OVERRUN          ((0x1)<<3)                                    //  FIFO Overrun. A '1' indicates that the FIFO has overrun. This may indicate a full FIFO that has not yet been emptied, or a clocking error caused by a slow SPI transaction.
#define PRM_FIFO_WATERMARK        ((0x1)<<2)                                    //  FIFO Watermark. A '1' indicates that the FIFO contains at least the desired number of samples, as set in the FIFO_SAMPLES register.
#define PRM_FIFO_READY            ((0x1)<<1)                                    //  FIFO Ready. A '1' indicates that there is at least one sample available in the FIFO output buffer.
#define PRM_DATA_READY            ((0x1)<<0)                                    //  Data Ready. A '1' indicates that a new valid sample is available to be read. This bit is reset when a read is performed. See datasheet page 22 of 39.
/******************************************************************************/

/******************************************************************************/
/*                    MASK FOR ACT_INACT_CTL REGISTER                         */
/******************************************************************************/
#define PRM_LOOPLINK_DEFAULT_MODE ((0x0)<<4)                                    //  Default Mode : Activity and Inactivity detection are both enabled and their interrupts (if mapped) must be acknowledged by the host processor by reading the STATUS register. Autosleep is disabled in this mode.
#define PRM_LOOPLINK_LINKED_MODE  ((0x1)<<4)                                    //  Linked Mode : Activity and Inactivity detection are linked sequentially such that only one is enabled at a time. Their interrupts (if mapped) must be acknowledged by the host processor by reading the STATUS register.
#define PRM_LOOPLINK_LOOP_MODE    ((0x3)<<4)                                    //  Loop Mode: Activity and Inactivity detection are linked sequentially such that only one is enabled at a time, and their interrupts are internally acknowledged (do not need to be serviced by the host processor).
#define PRM_LOOPLINK_MASK         ((0x3)<<4)

#define PRM_INACT_REF_REF_MODE    ((0x1)<<3)                                    //  Referenced / Absolute Inactivity Select. When '1', the Inactivity detection function operates in Referenced Mode. When ‘0’, the Inactivity detection function operates in Absolute Mode.
#define PRM_INACT_REF_ABS_MODE    ((0x0)<<3)
#define PRM_INACT_REF_MASK        ((0x1)<<3)

#define PRM_INACT_EN_ENABLE       ((0x1)<<2)                                    //  Inactivity Enable. When '1', enables the Inactivity (Under-Threshold) functionality.
#define PRM_INACT_EN_DISABLE      ((0x0)<<2)
#define PRM_INACT_EN_MASK         ((0x1)<<2)

#define PRM_ACT_REF_REF_MODE      ((0x1)<<1)                                    //  Referenced / Absolute Activity Select. When '1', the Activity detection function operates in Referenced Mode. When ‘0’, the Activity detection function operates in Absolute Mode.
#define PRM_ACT_REF_ABS_MODE      ((0x0)<<1)
#define PRM_ACT_REF_MASK          ((0x1)<<1)

#define PRM_ACT_EN_ENABLE         ((0x1)<<0)                                    //  Activity Enable. When '1', enables the Activity (Over-Threshold) functionality.
#define PRM_ACT_EN_DISABLE        ((0x0)<<0)
#define PRM_ACT_EN_MASK           ((0x1)<<0)
/******************************************************************************/


/******************************************************************************/
/*                    MASK FOR FIFO_CONTROL REGISTER                          */
/******************************************************************************/
#define PRM_AH_ENABLE             ((0x1)<<3)                                    //  Above Half. This bit can be used as the MSB of the FIFO_SAMPLES register, allowing FIFO Samples a range of 0 to 511
#define PRM_AH_DISABLE            ((0x0)<<3)
#define PRM_AH_MASK               ((0x1)<<3)

#define PRM_FIFO_TEMP_ENABLE      ((0x1)<<2)                                    //  Enables storing temp data in the FIFO.
#define PRM_FIFO_TEMP_DISABLE     ((0x0)<<2)
#define PRM_FIFO_TEMP_MASK        ((0x1)<<2)

#define PRM_FIFO_MODE_DISABLE      ((0x0)<<0)                                   //  FIFO is disabled.
#define PRM_FIFO_MODE_OLDEST_SAVED ((0x1)<<0)                                   //  FIFO mode.
#define PRM_FIFO_MODE_STREAM       ((0x2)<<0)                                   //  FIFO in stream mode.
#define PRM_FIFO_MODE_TRIG         ((0x3)<<0)                                   //  FIFO in triggered Mode.
#define PRM_FIFO_MODE_MASK         ((0x3)<<0)                                   //  FIFO Mask.
/******************************************************************************/

/******************************************************************************/
/*                    MASK FOR INTMAP1 and INTMAP2 REGISTER                   */
/******************************************************************************/
#define PRM_INT_LOW_ACTIVE_HIGH   ((0x0)<<7)                                    //  Interrupt Active HIGH
#define PRM_INT_LOW_ACTIVE_LOW    ((0x1)<<7)                                    //  Interrupt Active LOW
#define PRM_INT_LOW_MASK          ((0x1)<<7)                                    //  MASK INT_LOW

#define PRM_AWAKE_DISABLE         ((0x0)<<6)                                    //  Awake no interrupt
#define PRM_AWAKE_ENABLE          ((0x1)<<6)                                    //  Awake interrupt
#define PRM_AWAKE_MASK            ((0x1)<<6)                                    //  MASK AWAKE

#define PRM_INACT_DISABLE         ((0x0)<<5)                                    //  INACT no interrupt
#define PRM_INACT_ENABLE          ((0x1)<<5)                                    //  INACT interrupt
#define PRM_INACT_MASK            ((0x1)<<5)                                    //  MASK INACT

#define PRM_ACT_DISABLE           ((0x0)<<4)                                    //  ACT no interrupt
#define PRM_ACT_ENABLE            ((0x1)<<4)                                    //  ACT interrupt
#define PRM_ACT_MASK              ((0x1)<<4)                                    //  MASK ACT

#define PRM_FIFO_OVERRUN_DISABLE  ((0x0)<<3)                                    //  FIFO_OVERRUN no interrupt
#define PRM_FIFO_OVERRUN_ENABLE   ((0x1)<<3)                                    //  FIFO_OVERRUN interrupt
#define PRM_FIFO_OVERRUN_MASK     ((0x1)<<3)                                    //  MASK FIFO_OVERRUN

#define PRM_FIFO_WATERMARK_DISABLE ((0x0)<<2)                                   //  FIFO_WATERMARK no interrupt
#define PRM_FIFO_WATERMARK_ENABLE ((0x1)<<2)                                    //  FIFO_WATERMARK interrupt
#define PRM_FIFO_WATERMARK_MASK   ((0x1)<<2)                                    //  MASK FIFO_WATERMARK

#define PRM_FIFO_READY_DISABLE    ((0x0)<<1)                                    //  FIFO_READY no interrupt
#define PRM_FIFO_READY_ENABLE     ((0x1)<<1)                                    //  FIFO_READY interrupt
#define PRM_FIFO_READY_MASK       ((0x1)<<1)                                    //  MASK FIFO_READY

#define PRM_DATA_READY_DISABLE    ((0x0)<<0)                                    //  DATA_READY no interrupt
#define PRM_DATA_READY_ENABLE     ((0x1)<<0)                                    //  DATA_READY interrupt
#define PRM_DATA_READY_MASK       ((0x1)<<0)                                    //  MASK DATA_READY
/******************************************************************************/

/******************************************************************************/
/*                    MASK FOR FILTER_CTL REGISTER                            */
/******************************************************************************/
#define PRM_RANGE_2GEE            ((0x0)<<6)                                    //  Measurement Range Selection 2GEE
#define PRM_RANGE_4GEE            ((0x1)<<6)                                    //  Measurement Range Selection 4GEE
#define PRM_RANGE_8GEE            ((0x2)<<6)                                    //  Measurement Range Selection 8GEE
#define PRM_RANGE_MASK            ((0x3)<<6)                                    //  MASK Measurement Range

#define PRM_HALF_BW_ENABLE        ((0x1)<<4)                                    //  When ‘1’, the bandwidth of the anti-aliasing filters is set to ¼ the output data rate (ODR) for more conservative filtering.
#define PRM_HALF_BW_DISABLE       ((0x0)<<4)                                    //  When ‘0’, the bandwidth of the filters is set to ½ the ODR for a wider bandwidth.
#define PRM_HALF_BW_MASK          ((0x1)<<4)

#define PRM_EXT_SAMPLE_ENABLE     ((0x1)<<3)                                    //  When '1', the INT2 pin is used for External Conversion Control.
#define PRM_EXT_SAMPLE_DISABLE    ((0x0)<<3)
#define PRM_EXT_SAMPLE_MASK       ((0x1)<<3)

#define PRM_ODR_12_5              ((0x0)<<0)                                    //  Selects the ODR and configures the internal filters to a bandwidth of 1/2 the selected ODR: 12.5 Hz
#define PRM_ODR_25                ((0x1)<<0)                                    //  Selects the ODR and configures the internal filters to a bandwidth of 1/2 the selected ODR: 25 Hz
#define PRM_ODR_50                ((0x2)<<0)                                    //  Selects the ODR and configures the internal filters to a bandwidth of 1/2 the selected ODR: 50 Hz
#define PRM_ODR_100               ((0x3)<<0)                                    //  Selects the ODR and configures the internal filters to a bandwidth of 1/2 the selected ODR: 100 Hz
#define PRM_ODR_200               ((0x4)<<0)                                    //  Selects the ODR and configures the internal filters to a bandwidth of 1/2 the selected ODR: 200 Hz
#define PRM_ODR_400               ((0x5)<<0)                                    //  Selects the ODR and configures the internal filters to a bandwidth of 1/2 the selected ODR: 400 Hz
#define PRM_ODR_MASK              ((0x7)<<0)                                    //  MASK ODR
/******************************************************************************/

/******************************************************************************/
/*                    MASK FOR POWER_CTL REGISTER                             */
/******************************************************************************/
#define PRM_EXT_CLK_ENABLE        ((0x1)<<6)                                    //  When '1', the accelerometer runs off the external clock provided on the INT1 pin. Refer to the Using an External Clock section for additional details.
#define PRM_EXT_CLK_DISABLE       ((0x0)<<6)
#define PRM_EXT_CLK_MASK          ((0x1)<<6)

#define PRM_LOW_NOISE_NORMAL            ((0x0)<<4)                              //  Normal Operation (Reset Default)
#define PRM_LOW_NOISE_LOW_NOISE         ((0x1)<<4)                              //  Low Noise Mode
#define PRM_LOW_NOISE_ULTRA_LOW_NOISE   ((0x2)<<4)                              //  Ultra-Low Noise Mode
#define PRM_LOW_NOISE_MASK              ((0x3)<<4)                              //  MASK LOW_NOISE

#define PRM_WAKEUP_ENABLE         ((0x1)<<3)                                    //  When ‘1’, the part is operates in Wake-up Mode. Refer to the Operating Modes section for a detailed description of Wake-up Mode.
#define PRM_WAKEUP_DISABLE        ((0x0)<<3)
#define PRM_WAKEUP_MASK           ((0x1)<<3)

#define PRM_AUTOSLEEP_ENABLE      ((0x1)<<2)                                    //  When '1', Autosleep is enabled. If Link Mode or Loop Mode is activated, the device enters Sleep Mode automatically upon detection of Inactivity. Refer to the Motion-Activated Sleep section for details.
#define PRM_AUTOSLEEP_DISABLE     ((0x0)<<2)
#define PRM_AUTOSLEEP_MASK        ((0x1)<<2)

#define PRM_MEASURE_STANDBY_MODE  ((0x0)<<0)                                    //  Stand by mode
#define PRM_MEASURE_MEASURE_MODE  ((0x2)<<0)                                    //  Measurement Mode
#define PRM_MEASURE_MASK          ((0x3)<<0)                                    //  MASK MEASURE
/******************************************************************************/

/******************************************************************************/
/*                    MASK FOR SELF_TEST REGISTER                             */
/******************************************************************************/
#define PRM_ST_ENABLE             ((0x1)<<0)                                    //  When '1', a self-test force is applied to the x, y, and z axes.
#define PRM_ST_DISABLE            ((0x0)<<0)
#define PRM_ST_MASK               ((0x1)<<0)
/******************************************************************************/

/******************************************************************************/
/*                    MASK FOR FIFO MEMORY                                    */
/******************************************************************************/
#define PRM_SIGN_EXTENSION_MASK   ((0x3)<<12)
#define PRM_SIGN_NEGATIVE         ((0x3)<<12)
#define PRM_SIGN_EXT_NEG          ((0x3)<<14)
#define PRM_DATA_VALUE_MASK       ((0x3FFF)<<0)
#define PRM_DATA_TYPE_MASK        ((0x3)<<14)
#define PRM_DATA_TYPE_X_AXIS      ((0x0)<<0)                                    //
#define PRM_DATA_TYPE_Y_AXIS      ((0x1)<<14)                                   //
#define PRM_DATA_TYPE_Z_AXIS      ((0x1)<<15)                                   //
#define PRM_DATA_TYPE_T_AXIS      ((0x3)<<14)                                   //
/******************************************************************************/

#define ADXL_MAXDATASETSIZE                  (3)

#define MAX_WATER_MARK_BYTES                 (300) //Max number of bytes the adxl driver soft fifo can support
#define MAX_WATER_MARK_SAMPLES               (MAX_WATER_MARK_BYTES/6) //A sample set consists of X,Y and Z axis data each of 2 bytes size

#define ADXLDrv_SUCCESS (0)
#define ADXLDrv_ERROR (-1)

#ifdef NRF52840_XXAA
#define SPI_ADXL_DUMMY_BYTES    2
#else
#define SPI_ADXL_DUMMY_BYTES    0              
#endif //NRF52840_XXAA
              
              
/* ADXL set and get parameters */
/*  ADXL Parameter List
    Watermarking
    FifoLevel
	OdrRate
*/

typedef enum {
    ADXL_WATERMARKING = 0,
    ADXL_FIFOLEVEL,
    ADXL_ODRRATE
} AdxlCommandStruct;

extern int16_t AdxlDrvOpenDriver(uint16_t inrate, uint8_t watermark);
extern int16_t AdxlDrvCloseDriver();
extern int16_t AdxlDrvProbe(void);
extern int16_t AdxlDrvReadFifoData(uint16_t *pnXDataArray, uint16_t *pnYDataArray,
                            uint16_t *pnZDataArray, uint16_t nFifoLevelSize);
extern uint16_t ADXLDrv_SelfTest();
extern int16_t AdxlDrvRegRead(uint8_t nAddr, uint8_t *pRxData);
extern int16_t AdxlDrvRegWrite(uint8_t nAddr, uint8_t nValue);

int16_t AdxlDrvSetParameter(AdxlCommandStruct sCommand, uint16_t value);
int16_t AdxlDrvGetParameter(AdxlCommandStruct sCommand, uint16_t *pValue);
/* Adxl register interrupt callback */
void AdxlDrvDataReadyCallback(void (*pfAdxlDataReady)());
int16_t AdxlDrvReadFifoData(uint16_t *pnXDataArray, uint16_t *pnYDataArray, uint16_t *pnZDataArray, uint16_t nFifoLevelSize);
void AdxlDrvSoftReset(void);
void AdxlDrvExtSampleMode(uint8_t nEnableExtSample);
int16_t AdxlDrvSetOperationMode(uint8_t nOpMode);
void AdxlISR();
#endif // adxl362.h
