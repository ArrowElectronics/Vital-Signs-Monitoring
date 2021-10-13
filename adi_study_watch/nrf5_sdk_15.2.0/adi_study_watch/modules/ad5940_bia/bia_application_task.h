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
#ifndef __BIA_TASK__H
#define __BIA_TASK__H
#include "stdio.h"
#include "string.h"
#include "math.h"
#include "bia_application_interface.h"
#include "task_includes.h"
#include "post_office.h"
#include "ad5940.h"
#include "sensor_ad5940.h"
#include "app_bia.h"
#include "app_cfg.h"
#include "adi_bcm_algo.h"


#define MAXSWEEP_POINTS   100           /* Need to know how much buffer is needed to save RTIA calibration result */
#define BCM_ARRAY_SIZE   4           /* Need to know how much buffer is needed to save RTIA calibration result */


typedef struct _bia_packetizer_t {
  m2m2_hdr_t                *p_pkt;
  uint16_t                  packet_max_nsamples;
  uint16_t                  sample_sz;
  uint16_t                  packet_nsamples;
  uint32_t                  prev_ts;
} bia_packetizer_t;

typedef struct _g_state_bia_t {
  uint16_t  num_subs;
  uint16_t  num_starts;
  uint8_t   decimation_factor;
  //uint8_t   algo_decimation_factor;
  uint16_t  decimation_nsamples;
  uint16_t  data_pkt_seq_num;
  bia_packetizer_t  bia_pktizer;
} g_state_bia_t;
/* 
  Note: this example will use SEQID_0 as measurment sequence, and use SEQID_1 as init sequence. 
  SEQID_3 is used for calibration.
*/

typedef struct
{
/* Common configurations for all kinds of Application. */
  BoolFlag bParaChanged;        /* Indicate to generate sequence again. It's auto cleared by AppBIAInit */
  uint32_t SeqStartAddr;        /* Initialaztion sequence start address in SRAM of AD5940  */
  uint32_t MaxSeqLen;           /* Limit the maximum sequence.   */
  uint32_t SeqStartAddrCal;     /* Measurment sequence start address in SRAM of AD5940 */
  uint32_t MaxSeqLenCal;
/* Application related parameters */ 
  //BoolFlag bBioElecBoard;     /* The code is same for BioElec board and AD5941Sens1 board. No changes are needed */
  BoolFlag ReDoRtiaCal;         /* Set this flag to bTRUE when there is need to do calibration. */
  float SysClkFreq;             /* The real frequency of system clock */
  float WuptClkFreq;            /* The clock frequency of Wakeup Timer in Hz. Typically it's 32kHz. Leave it here in case we calibrate clock in software method */
  float AdcClkFreq;             /* The real frequency of ADC clock */
  uint32_t FifoThresh;           /* FIFO threshold. Should be N*4 */   
  float BiaODR;                 /* in Hz. ODR decides the period of WakeupTimer who will trigger sequencer periodically. DFT number and sample frequency decides the maxim ODR. */
  int32_t NumOfData;            /* By default it's '-1'. If you want the engine stops after get NumofData, then set the value here. Otherwise, set it to '-1' which means never stop. */
  float SinFreq;                /* Frequency of excitation signal */
  float RcalVal;                /* Rcal value in Ohm */
  uint32_t PwrMod;              /* Control Chip power mode(LP/HP) */
  float DacVoltPP;              /* Final excitation voltage is DAC_VOLTpp*DAC_PGA*EXCIT_GAIN, DAC_PGA= 1 or 0.2, EXCIT_GAIN=2 or 0.25. DAC output voltage in mV peak to peak. Maximum value is 800mVpp. Peak to peak voltage  */
  uint32_t ExcitBufGain;        /* Select from  EXCITBUFGAIN_2, EXCITBUFGAIN_0P25 */     
  uint32_t HsDacGain;           /* Select from  HSDACGAIN_1, HSDACGAIN_0P2 */  
  uint32_t HsDacUpdateRate;     /* DAC update rate is SystemCLoock/Divider. The available value is 7 to 255. Set to 7 for better perfomance */
  uint32_t ADCPgaGain;          /* PGA Gain select from GNPGA_1, GNPGA_1_5, GNPGA_2, GNPGA_4, GNPGA_9 !!! We must ensure signal is in range of +-1.5V which is limited by ADC input stage */   
  uint8_t ADCSinc3Osr;          /* SINC3 OSR selection. ADCSINC3OSR_2, ADCSINC3OSR_4 */
  uint8_t ADCSinc2Osr;          /* SINC2 OSR selection. ADCSINC2OSR_22...ADCSINC2OSR_1333 */
  uint32_t HstiaRtiaSel;        /* Use internal RTIA, select from RTIA_INT_200, RTIA_INT_1K, RTIA_INT_5K, RTIA_INT_10K, RTIA_INT_20K, RTIA_INT_40K, RTIA_INT_80K, RTIA_INT_160K */
  uint32_t CtiaSel;             /* Select CTIA in pF unit from 0 to 31pF */

  uint32_t DftNum;              /* DFT number */
  uint32_t DftSrc;              /* DFT Source */
  BoolFlag HanWinEn;            /* Enable Hanning window */
  BoolFlag bRunning;              /**< status of if app is running. Useful when send STOP_SYNC to detect if it's actually stopped. */

  /* Sweep Function Control */
  SoftSweepCfg_Type SweepCfg;
/* Private variables for internal usage */
  float SweepCurrFreq;
  float SweepNextFreq;
  float RtiaCurrValue[2];                    /* Calibrated Rtia value of current frequency */
  float RtiaCalTable[MAXSWEEP_POINTS][2];   /* Calibrated Rtia Value table */
  float FreqofData;                         /* The frequency of latest data sampled */
  BoolFlag BIAInited;                       /* If the program run firstly, generated sequence commands */
  SEQInfo_Type InitSeqInfo;
  SEQInfo_Type MeasureSeqInfo;
  BoolFlag StopRequired;          /* After FIFO is ready, stop the measurment sequence */
  uint32_t FifoDataCount;         /* Count how many times impedance have been measured */
  uint32_t MeasSeqCycleCount;     /* How long the measurement sequence will take */
  float MaxODR;                   /* Max ODR for sampling in this config */       
/* End */
}AppBIACfg_Type;


#define BIACTRL_GETFREQ        100   /* Get Current frequency of returned data from ISR */

#ifndef APPCTRL_START
/* Common application control message */
#define APPCTRL_START          0      /**< Start the measurement by starting Wakeup Timer */
#define APPCTRL_STOPNOW        1      /**< Stop immediately by stop Wakeup Timer*/
#define APPCTRL_STOPSYNC       2      /**< Stop the measurement when interrupt occurred */
#define APPCTRL_SHUTDOWN       3      /**< Note: shutdown here means turn off everything and put AFE to hibernate mode. The word 'SHUT DOWN' is only used here. */
#define APPCTRL_RUNNING        4      /**< Is application running? */
#endif

AD5940Err AppBIAGetCfg(void *pCfg);
AD5940Err AppBIAInit(uint32_t *pBuffer, uint32_t BufferSize);
AD5940Err AppBIACtrl(int32_t BcmCtrl, void *pPara);
void AD5940BIAStructInit(void);
int32_t AD5940PlatformCfg(void);
BIA_ERROR_CODE_t BiaAppInit();
BIA_ERROR_CODE_t BiaAppDeInit();
void ad5940_bia_task_init(void);
void send_message_ad5940_bia_task(m2m2_hdr_t *p_pkt);
BIA_ERROR_CODE_t BiaReadLCFG(uint8_t index, float *value);
BIA_ERROR_CODE_t BiaWriteLCFG(uint8_t field, float value);
AD5940Err AppBIARegModify(int32_t * const pData, uint32_t *pDataCount);
AD5940Err AppBIADataProcess(int32_t * const pData, uint32_t *pDataCount);
bool bia_get_dcb_present_flag(void);
#ifdef BCM_ALGO
BCM_ALG_RETURN_CODE_t bcm_algo_init();
#endif
#endif // __ADPD4000_TASK__H
