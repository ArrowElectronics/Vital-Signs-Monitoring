/*!
 *****************************************************************************
 @file:    ImpSeqs.h
 @author:  $Author: nxu2 $
 @brief:   4-wire EDA measurement header file.
 @version: $Revision: 766 $
 @date:    $Date: 2017-08-21 14:09:35 +0100 (Mon, 21 Aug 2017) $
 -----------------------------------------------------------------------------

Copyright (c) 2017-2019 Analog Devices, Inc. All Rights Reserved.

This software is proprietary to Analog Devices, Inc. and its licensors.
By using this software you agree to the terms of the associated
Analog Devices Software License Agreement.

*****************************************************************************/
#ifndef _EDA_H_
#define _EDA_H_
#include "stdio.h"
#include "string.h"
#include "math.h"
#include "task_includes.h"
#include "post_office.h"
#include "ad5940.h"
#include "sensor_ad5940.h"
#include "app_eda.h"
#include "app_cfg.h"

typedef struct _eda_packetizer_t {
  m2m2_hdr_t                *p_pkt;
  uint16_t                  packet_max_nsamples;
  uint16_t                  sample_sz;
  uint16_t                  packet_nsamples;
  uint32_t                  prev_ts;
} eda_packetizer_t;

typedef struct _g_state_eda_t {
  uint16_t  num_subs;
  uint16_t  num_starts;
  uint8_t   decimation_factor;
  uint16_t  decimation_nsamples;
  uint16_t  data_pkt_seq_num;
  eda_packetizer_t  eda_pktizer;
} g_state_eda_t;

/* Do not modify following parameters */
#define LPTIAPA_PMOD    0                 /* Power Mode of PA and LPTIA, Set to Half Power Mode is better for power consumption, 0: normal. 0x18: boost power. BITM_AFE_ULPTIACON0_HALFPWR: half power */
#define LPF_RF          LPTIARF_20K       /* Set RF resistor of Low Pass Filter */
#define LPF_TIME        10.0              /* Unit is ms. Low Pass Filter need time to settle. 10ms is OK for now */

#define LPTIASW_VOLT    LPTIASW(5)|LPTIASW(6)|LPTIASW(7)|LPTIASW(8)|LPTIASW(9)|LPTIASW(13)
#define LPTIASW_CURR    LPTIASW(2)|LPTIASW(5)|LPTIASW(10)|LPTIASW(13)

/**
 * @brief The structure for sequencer patch.
*/
typedef struct
{
  enum __PatchType
  {
    PATCHTYPE_VOLT = 0,         /**< Generate patch for measuring voltage */
    PATCHTYPE_CURR,             /**< Generate patch for measuring current of body */
  }Type;
  uint32_t RtiaSel;             /**< LPTIA RTIA selection */
  const uint32_t *pSeqCmd;      /**< The sequence to measure voltage and current is similar. The difference is stored in a command patch. */
  uint32_t SeqLen;              /**< Length of patch sequence  */
  uint32_t SRAMAddr;            /**< Start address of the sequence command patch  */
  uint32_t Buffer[32];          /**< 32Byte should be enough for sequence generator */
  uint32_t BuffLen;       /**< The buffer length of Buffer */
}SeqPatchInfo_Type;

/*
  Note: this example will use SEQID_0 as measurment sequence, and use SEQID_1 as init sequence.
  SEQID_3 is used for calibration if there is need.
*/
typedef struct
{
/* Common configurations for all kinds of Application. */
  BoolFlag bParaChanged;        /**< Indicate to generate sequence again. It's auto cleared by AppEDAInit */
  uint32_t SeqStartAddr;        /**< Initialaztion sequence start address in SRAM of AD5940  */
  uint32_t MaxSeqLen;           /**< Limit the maximum sequence.   */
  uint32_t SeqStartAddrCal;     /**< Measurment sequence start address in SRAM of AD5940 */
  uint32_t MaxSeqLenCal;
/* Application related parameters */
  BoolFlag bBioElecBoard;       /**< Select between AD5941Sens1 board and BioElec board */
  BoolFlag ReDoRtiaCal;         /**< Set this flag to bTRUE when there is need to do calibration. */
  float SysClkFreq;             /**< The real frequency of system clock */
  float LfoscClkFreq;           /**< The clock frequency of Wakeup Timer in Hz. Typically it's 32kHz. Leave it here in case we calibrate clock in software method */
  float AdcClkFreq;             /**< The real frequency of ADC clock */
  uint32_t FifoThresh;          /**< FIFO threshold. Should be N*4 */
  float EDAODR;                 /**< in Hz. ODR decides the period of WakeupTimer who will trigger sequencer periodically. DFT number and sample frequency decides the maxim ODR. */
  int32_t NumOfData;            /**< By default it's '-1'. If you want the engine stops after get NumofData, then set the value here. Otherwise, set it to '-1' which means never stop. */
  uint32_t VoltCalPoints;       /**< Use how many points to calculate average excitation voltage */
  float RcalVal;                /**< Rcal value in Ohm */
  float SinFreq;                /**< Frequency of excitation signal */
  float SampleFreq;             /**< Sample Frequency in Hz. Clock source is 32kHz.*/
  float SinAmplitude;           /**< Signal in amplitude in mV unit. Range: 0Vp to 1100mVp (0Vpp to 2.2Vpp) */
  uint32_t DacUpdateRate;       /**< DAC update rate is SystemCLoock/Divider. The available value is 7 to 255. */
  uint32_t LptiaRtiaSel;        /**< Use internal RTIA, Select from LPTIARTIA_OPEN, LPTIARTIA_200R, ... , LPTIARTIA_512K */

  uint32_t DftNum;              /**< DFT number */
  BoolFlag HanWinEn;            /**< Enable Hanning window */

  BoolFlag RtiaAutoScaleEnable; /**< Automatically change RTIA value according to measurement results. 0: Set RTIA with RTIA_SEL. 1: Automatically choose RTIA in software */
  uint32_t RtiaAutoScaleMax;    /**< Limit the maximum RTIA value that auto scale function can use. Select from LPTIARTIA_OPEN, LPTIARTIA_200R, ... , LPTIARTIA_512K */
  uint32_t RtiaAutoScaleMin;    /**< Limit the minimum RTIA value that auto scale function can use. Select from LPTIARTIA_OPEN, LPTIARTIA_200R, ... , LPTIARTIA_512K */

/* Private variables for internal usage */
  fImpCar_Type  RtiaCurrValue;   /**< Calibrated Rtia value of current frequency */
  fImpCar_Type  RtiaCalTable[LPTIARTIA_512K+1];   /**< Calibrated Rtia Value table */
  fImpCar_Type  ImpEDABase;      /**< Impedance of EDA base line */
  fImpCar_Type  ImpSum;          /**< Sum of all measured results. Used to calculate base line of EDA */
  uint32_t      ImpSumCount;     /**< Count of data added to 'ImpSum' */
  uint32_t      RtiaIndexCurr;   /**< Index value 0 to 26 means Open, 200Ohm, to 512kOhm */
  uint32_t      RtiaIndexNext;
  BoolFlag      bChangeRtia;     /**< Auto scaling method says we need to change RTIA */

  SeqPatchInfo_Type SeqPatchInfo; /**< The sequence patch for different RTIA and both voltage/current measurement */
  fImpCar_Type ExcitVolt;       /**< Measured excitation voltage result */
  BoolFlag bDataIsVolt;         /**< Current DFT result is voltage */
  BoolFlag bMeasVoltReq;        /**< User says we need to measure voltage */

  BoolFlag bRunning;            /**< status of if EDA is running. Useful when send STOP_SYNC to detect if it's actually stopped. */
  BoolFlag EDAInited;           /**< If the program run firstly, generated sequence commands */
  SEQInfo_Type InitSeqInfo;
  SEQInfo_Type MeasureSeqInfo;
  BoolFlag StopRequired;        /**< After FIFO is ready, stop the measurment sequence */
  uint32_t FifoDataCount;       /**< Count how many times impedance have been measured */

  enum __EDAState{
    EDASTATE_INIT = 0,          /**< Initializing */
    EDASTATE_RTIACAL,           /**< Internal RTIA resistor calibrating. */
    EDASTATE_VOLT,              /**< Measuring excitation voltage */
    EDASTATE_CURR,              /**< Measuring respond current */
  }EDAStateCurr, EDAStateNext;  /**< When interrupt happens, the state is EDACurrState. At the end of interrupt function, go to EDANextState */
/* End */
}AppEDACfg_Type;

/* Common application control message */
#define APPCTRL_START                     0      /**< Start the measurement by starting Wakeup Timer */
#define APPCTRL_STOPNOW                   1      /**< Stop immediately by stop Wakeup Timer*/
#define APPCTRL_STOPSYNC                  2      /**< Stop the measurement when interrupt occured */
#define APPCTRL_SHUTDOWN                  3      /**< Note: shutdown here means turn off everything and put AFE to hibernate mode. The word 'SHUT DOWN' is only used here. */
#define APPCTRL_RUNNING                   4      /**< Is application running? */

#define EDACTRL_MEASVOLT                  100    /**< Measure Exciation voltage now */
#define EDACTRL_GETRTIAMAG                101    /**< Get the rtia magnitude for current measured data */

#define EDACTRL_RSTBASE                   102    /**< Reset base line of EDA result. */
#define EDACTRL_SETBASE                   103    /**< Set base line of EDA result */
#define EDACTRL_GETAVR                    104    /**< Get average value of all measured impedance */
#define EDACTRL_STATUS                    105    /**< Get if EDA is running. */

/* application related macros */
#define MAX_NUM_OF_FFT_POINTS             4

#ifdef EDA_DCFG_ENABLE

#define MAX_DEFAULT_DCFG_REGISTER_NUM     150
#define MAX_USER_CONFIG_REGISTER_NUM      20
#define SAVE_PATCH_CURRENT_VOLTAGE_SETTINGS_START_INDEX 3

#define SEQMEASRMT_LOOP_DEFAULT_REGISTERS_SIZE 6
#define SEQMEASRMT_LOOP_TOTAL_REGISTERS_SIZE 10

/* Register addresses to be configured as macros */
#define FIFO_CONFIGURATION_REGISTER                                                       0x000021D8
#define FIFO_SRC_CONFIGURATION_REGISTER                                                   0x00002008
#define INTERRUPT_CONTROLLER_SELECT_ZERO_REGISTER                                         0x00003008
#define INTERRUPT_CONTROLLER_SELECT_ONE_REGISTER                                          0x0000300C
#define LOW_POWER_DAC_CONFIGURATION_REGISTER                                              0x00002128
#define ADC_PGA_GAIN_CONFIGURATION_REGISTER                                               0x000021A8
#define FILTER_CONFIGURATION_REGISTER                                                     0x00002044
#define DFT_CONFIGURATION_REGISTER                                                        0x000020D0
#define GPIO_CONTROL_CONFIGURATION_REGISTER                                               0x00002054
#define LOW_POWER_TIA_CONFIGURATION_REGISTER                                              0x000020EC
#define POWER_DOWN_CONFIGURATION_REGISTER                                                 0x00002114

typedef enum {
  AD5940_DCFG_STATUS_OK = 0,
  AD5940_DCFG_STATUS_NULL_PTR,
  AD5940_DCFG_STATUS_ERR,
} AD5940_DCFG_STATUS_t;

#define MAX_DCFG_COMBINED 3
#endif

/* Error message */
#define EDAERR_ERROR            AD5940ERR_APPERROR    /**< General error */
#define EDAERR_VOLTMEASURE      AD5940ERR_APPERROR-1  /**< Excitation voltage measurment error. Points not match */

#define REAL_INDEX 0
#define IMAG_INDEX 1

#define RL_IM_PAIR 2

void InitCfg();
AD5940Err AppEDAGetCfg(void *pCfg);
AD5940Err AppEDAInit(uint32_t *pBuffer, uint32_t BufferSize);
AD5940Err AppEDAISR(void *pBuff, uint32_t *pCount);
AD5940Err AppEDACtrl(int32_t EDACtrl, void *pPara);
AD5940Err AppEDARtiaCal(void);
void AD5940EDAStructInit(void);
void ad5940_eda_task_init(void);
void send_message_ad5940_eda_task(m2m2_hdr_t *p_pkt);
AD5940Err EDACalculateImpedance(void *pData);
AD5940Err AppEDARegModify(int32_t * const pData, uint32_t *pDataCount);
AD5940Err AppEDADataProcess(int32_t * const pData, uint32_t *pDataCount);
void ad5940_eda_start(void);
AD5940Err AppEDASeqCfgGen(void) ;
AD5940Err AppEDASeqMeasureGen(void);
uint32_t EDARtiaAutoScaling(fImpCar_Type *const pImpedance, uint32_t uiDataCount);

#ifdef EDA_DCFG_ENABLE
void load_ad5940_default_config(uint64_t *cfg);
AD5940_DCFG_STATUS_t write_ad5940_init();
AD5940_DCFG_STATUS_t write_ad5940_seqcfg();
AD5940_DCFG_STATUS_t write_ad5940_seqmeasurement();
uint8_t mapaddrind(uint32_t *address);
uint8_t mapuserind(uint8_t *userind);
#endif//EDA_DCFG_ENABLE

#endif
