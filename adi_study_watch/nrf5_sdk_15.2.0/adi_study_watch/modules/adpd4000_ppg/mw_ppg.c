/**
***************************************************************************
* @file         MwPPG.c
* @author       ADI
* @version      V1.1.0
* @date         01-Aug-2016
* @brief        Sample showing how to use the HRM Library.
*
***************************************************************************
* @attention
***************************************************************************
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
* This software is intended for use with the ADPD and derivative parts        *
* only                                                                        *
*                                                                             *
******************************************************************************/
#ifdef ENABLE_PPG_APP
#include "nrf_log_ctrl.h"
#define NRF_LOG_MODULE_NAME PPG

#include "nrf_log.h"
NRF_LOG_MODULE_REGISTER();

#include <assert.h>
//#include "adpd_lib.h"
#include "adpd400x_lib.h"
#include "Adxl362.h"
#include "app_sync.h"
#include "struct_operations.h"
#include "mw_ppg.h"
#include "ppg_lcfg.h"
#include <string.h>

#ifdef DCB
#include <dcb_interface.h>
#include <adi_dcb_config.h>
#endif

#define PPG_ALGO_MAJOR_VERSION     1
#define PPG_ALGO_MINOR_VERSION     2
#define PPG_ALGO_PATCH_VERSION     0

/* Private macros -----------------------------------------------------------*/
#define STATE_NUM                         10
#define STATE_INFO_SIZE                   10

typedef struct _stat_info_t {
  uint8_t               state;                    /**< adpdlib state */
  uint16_t              info[STATE_INFO_SIZE];    /**< state info.*/
  struct _stat_info_t   *next;
} stat_info_t;
static stat_info_t gsStateInfo[STATE_NUM], *gspStateInfo;
/////////////////////////////////////////
#ifdef DCB
static volatile bool g_ppg_dcb_Present = false;
static Adpd400xLibConfig_t AdpdLibdcbLcfg4000;
#endif
/////////////////////////////////////////


/* Public variables ---------------------------------------------------------*/
//volatile uint8_t gnAdpdIRQReady;
/*  initialized to 8KhZ tick resolution for 50Hz ODR */
uint16_t gnAdpdTimeGap = 160;
uint16_t gnAdxlTimeGap = 160;

/* Private variables---------------------------------------------------------*/
static uint8_t gsLastLibState = 0;
static uint8_t gsCurrLibState = 0;
static LibResultX_t gsResult;

static void MwPPG_GetStateInfoInit(void);
static void MwPPG_LoadStateInfo(uint8_t state);
static ADPDLIB_ERROR_CODE_t (*gfLibHRCall)(LibResultX_t *, uint32_t *, \
                                                  int16_t *, \
                                                    TimeStamps_t );
static uint8_t (*gfLibGetStateCall)(void);
static ADPDLIB_ERROR_CODE_t (*gfLibGetStateInfoCall)(uint8_t , uint16_t *);
static ADPDLIB_ERROR_CODE_t (*gfLibOpenHRCall)(void);
static ADPDLIB_ERROR_CODE_t (*gfLibCloseHRCall)(void);

static StructOpErrorStatus (*gfMwPpgLcfgStructure)(int32_t *pValue, uint8_t *nSize);
static StructOpErrorStatus (*gfMwPpgReadLcfgStructureRaw)(uint32_t field, int32_t *value);
static StructOpErrorStatus (*gfModifyLcfgStructureRaw)(uint32_t field, int32_t val);
/*----------------- Public function prototype --------------------------------*/
void RegisterMwPpgLibHRCB(ADPDLIB_ERROR_CODE_t (*LibCall)(LibResultX_t *, uint32_t *, \
                                                  int16_t *, \
                                                    TimeStamps_t ));
void RegisterMwPpgLibGetStateCB(uint8_t (*LibCall)(void));
void RegisterMwPpgLibGetStateInfoCB(ADPDLIB_ERROR_CODE_t (*LibCall)(uint8_t, uint16_t *));
void RegisterMwPpgLibOpenHRCB(ADPDLIB_ERROR_CODE_t (*LibCall)(void));
void RegisterMwPpgLibCloseHRCB(ADPDLIB_ERROR_CODE_t (*LibCall)(void));
void RegisterMwPpgGetLCFGRegCB(StructOpErrorStatus (*pfn)(int32_t *, uint8_t *));
void RegisterMwPpgReadLCFGRegCB(StructOpErrorStatus (*pfn)(uint32_t , int32_t *));
void RegisterMwPpgModifyLCFGRegCB(StructOpErrorStatus (*pfn)(uint32_t , int32_t ));
ADPDLIB_ERROR_CODE_t MwPPG_HeartRate(LibResultX_t*,
                             uint32_t *slotB,
                             int16_t *accl,
                             TimeStamps_t ts);

/* External variables -------------------------------------------------------*/
//extern AdpdLibConfig_t gAdpdLibCfg;
extern Adpd400xLibConfig_t gAdpd400xLibCfg;
/* External functions -------------------------------------------------------*/
extern ADPDLIB_ERROR_CODE_t AdpdLibGetStateInfo(uint8_t state,
                                                uint16_t* debugData);
extern int16_t PpgSetOperationMode(uint8_t eOpState);

/****** Driver related declarations start ********/

/* Function prototypes */

/* Private function prototypes --------------------------------------------- */



/**
  * @brief Set the device library configuration used by the library.
  *        This must be called before AdpdLibOpenHr is called.
  * @param lcfg pointer to the LCFG array
  * @retval none.
  */
#if 0
void MwPPG_ApplyLCFG1(AdpdLibConfig_t *lcfg)  {
  AdpdLibApplyLCFG(lcfg);
}
#endif

PPG_ERROR_CODE_t MwPpg_LoadppgLCFG(uint16_t device_id) {

 PPG_ERROR_CODE_t ppg_lcfg_sts = PPG_ERROR;

#ifdef DCB
 bool dcb_cfg = false;
 dcb_cfg = ppg_get_dcb_present_flag();

 if(dcb_cfg == true)
    {
    //load dcb settings
    ppg_lcfg_sts = load_ppg_dcb(device_id);
    if(ppg_lcfg_sts != PPG_SUCCESS)
        {
            NRF_LOG_INFO("Failed in Loading ppg DCB lcfg");
        }
        NRF_LOG_INFO("Load ppg DCB lcfg");
    }
 else
    {
#endif
      //Load lcfg Settings
      ppg_lcfg_sts = MwPpg_LoadLCFG(device_id);
      if(ppg_lcfg_sts != PPG_SUCCESS)
        {
            NRF_LOG_INFO("Failed in Loading ppg Default f/w lcfg");
        }
        NRF_LOG_INFO("Load ppg Default f/w lcfg");

#ifdef DCB
    }
#endif

    return ppg_lcfg_sts;
}


void MwPPG_ApplyLCFG(void *lcfg)  {
  Adpd400xLibApplyLCFG(lcfg);
}
/**
  * @internal
  * @brief    Example of how to initialize HRM function
  * @retval   ADPDLIB_ERROR_CODE_t
  */
ADPDLIB_ERROR_CODE_t MwPPG_HeartRateInit()  {
  ADPDLIB_ERROR_CODE_t retVal;
  gsLastLibState = 0;
  gsCurrLibState = 0;
  MwPPG_GetStateInfoInit();
  memset(&gsResult, 0, sizeof(gsResult));
  Adpd400xLibApplyLCFG(&gAdpd400xLibCfg);
  RegisterMwPpgLibOpenHRCB(Adpd400xLibOpenHr);
  RegisterMwPpgLibCloseHRCB(Adpd400xLibCloseHr);
  RegisterMwPpgLibHRCB(Adpd400xLibGetHr);
  RegisterMwPpgLibGetStateCB(Adpd400xLibGetState);
  RegisterMwPpgLibGetStateInfoCB(Adpd400xLibGetStateInfo);

  //SyncInit();
  retVal = (*gfLibOpenHRCall)();
  return retVal;
}

/**
  * @internal
  * @brief    Example of how to call HRM function
  * @param    Pointer to slotB data
  * @param    Pointer to accelerometer data
  * @param    Timestamp structure containing ADPD, ADXL and Algo call timestamp
  * @retval   Library result structure
  */
ADPDLIB_ERROR_CODE_t MwPPG_HeartRate(LibResultX_t* hrResult,
                             uint32_t *slotB,
                             int16_t *accl,
                             TimeStamps_t ts) {
  ADPDLIB_ERROR_CODE_t ret = (ADPDLIB_ERROR_CODE_t)0;

  (*gfLibHRCall)(&gsResult, slotB, accl, ts); // Call Get HR Lib function
  gsLastLibState = gsCurrLibState;
  //gsCurrLibState = (uint8_t)AdpdLibGetState();
  gsCurrLibState = (*gfLibGetStateCall)();
  if (gsCurrLibState != gsLastLibState) {   // state change
    MwPPG_LoadStateInfo(gsCurrLibState);
    //PpgSetOperationMode(ADPDDrv_MODE_SAMPLE); // sample mode set not needed during state change
  }

  memcpy(hrResult, &gsResult, sizeof(LibResultX_t));
  return ret;
}

/**
  * @internal
  * @brief    Example of how to close the PPG application
  * @retval   Success/Error
  */
uint8_t MwPPG_HeartRateDeInit() {
  ADPDLIB_ERROR_CODE_t retVal;
  retVal = (*gfLibCloseHRCall)();
  SyncDeInit();
  return retVal;
}

/**
  * @internal
  * @brief    Example to get the various states within AdpdLib
  * @param    Pointer to states array
  * @retval   Success/Error
  */
PPG_ERROR_CODE_t MwPPG_GetStates(uint8_t *states) {
  uint8_t i;
  stat_info_t *pStateInfo;

  if(gspStateInfo == NULL)
  {
     return PPG_ERROR;
  }
  pStateInfo = gspStateInfo;
  for (i = 0; i < STATE_NUM; i++)  {
    *states = pStateInfo->state;
    states++;
    pStateInfo = pStateInfo->next;
  }

  return PPG_SUCCESS;
}

/**
  * @internal
  * @brief    Example to get the states information within AdpdLib
  * @param    State whose information is needed
  * @param    Pointer where state information array is to be populated
  * @retval   None
  */
void MwPPG_GetStateInfo(uint8_t state, uint16_t *info) {
  uint8_t i;
  stat_info_t *pStateInfo;

  pStateInfo = gspStateInfo;
  for (i = 0; i < STATE_NUM; i++)  {
    if (pStateInfo->state == state)
      memcpy((void *)info, (void *)&(pStateInfo->info[0]), STATE_INFO_SIZE*2);
    pStateInfo = pStateInfo->next;
  }
}

/**
  * @internal
  * @brief    Get LibState function
  * @retval   None
  */
uint8_t MwPpgGetLibState (void) {
  return gsCurrLibState;
}

/**
  * @internal
  * @brief    Example of how to load the state information into AdpdLib
  * @brief    This is done during state change
  * @param    State whose information is to be loaded
  * @retval   None
  */
static void MwPPG_LoadStateInfo(uint8_t state) {
  (*gfLibGetStateInfoCall)(state, &(gspStateInfo->info[0]));
  gspStateInfo->state = state;
  gspStateInfo = gspStateInfo->next;
}

/**
  * @internal
  * @brief    Example to initialize the GetStateInfo function
  * @retval   None
  */
static void MwPPG_GetStateInfoInit(void) {
  uint8_t i;

  gspStateInfo = &gsStateInfo[0];
  for (i = 0; i < STATE_NUM - 1; i++) {
    gspStateInfo->next = &gsStateInfo[i+1];
    gspStateInfo = gspStateInfo->next;
  }
  gspStateInfo->next = &gsStateInfo[0];
  gspStateInfo = &gsStateInfo[0];
}

/**
  * @internal
  * @brief    Example of how to write an LCFG parameter
  * @param    LCFG field that has to be written
  * @param    Value to be written
  * @retval   PPG_ERROR_CODE_t
  */
PPG_ERROR_CODE_t MwPPG_WriteLCFG(uint8_t field, uint32_t value) {
  if ((*gfModifyLcfgStructureRaw)(field, (int32_t) value) == STRUCTOPSUCCESS)
    return PPG_SUCCESS;
  return PPG_ERROR;
}

/**
  * @internal
  * @brief    Read LCFG parameter
  * @param    LCFG field
  * @param    Returned corresponding LCFG value
  * @retval   PPG_ERROR_CODE_t
  */
PPG_ERROR_CODE_t MwPPG_ReadLCFG(uint8_t index, uint32_t *value) {
  if ((*gfMwPpgReadLcfgStructureRaw)(index, (int32_t*) value) == STRUCTOPSUCCESS)
    return PPG_SUCCESS;
  return PPG_ERROR;
}

/**
  * @internal
  * @brief    Read LCFG structure
  * @param    LCFG buffer pointer
  * @retval   PPG_ERROR_CODE_t
  */
PPG_ERROR_CODE_t MwPPG_ReadLCFGStruct(uint32_t *value, uint8_t *nStructSize) {
  if ((*gfMwPpgLcfgStructure)((int32_t*) value, nStructSize) == STRUCTOPSUCCESS)
    return PPG_SUCCESS;
  return PPG_ERROR;
}

/**
  * @internal
  * @brief    Example of how to load the LCFG
  * @param    Sensor device id
  * @retval   Success/Error
  */
PPG_ERROR_CODE_t MwPpg_LoadLCFG(uint16_t device_id) {
  InitOffsetsAdpd400xLcfgStruct();
  switch(device_id) {
    case 40:  {
      memcpy((void *)&gAdpd400xLibCfg, (const void *)&AdpdLibCfg4000, sizeof(AdpdLibCfg4000));
        MwPPG_ApplyLCFG(&gAdpd400xLibCfg);
    }
    break;
    default: {
      return PPG_ERROR;
    }
    break;
  }
  return PPG_SUCCESS;
}

/**
  * @brief Gets the vendor name and version number of the ppg hrm algorithm
  *        embedded in the library
  * @param pointer to the structure where the vendor name and version number
  *   are returned
  * @retval None.
  */
void PpgLibGetAlgorithmVendorAndVersion(PpgAlgoVersion_t *pAlgoInfo) {
  if (pAlgoInfo == NULL) {
    return;
  }
    strcpy((char *)pAlgoInfo->aNameStr, "ADI HRM;");
    pAlgoInfo->nMajor = PPG_ALGO_MAJOR_VERSION;
    pAlgoInfo->nMinor = PPG_ALGO_MINOR_VERSION;
    pAlgoInfo->nPatch = PPG_ALGO_PATCH_VERSION;
}

ADPDLIB_ERROR_CODE_t MwPpgGetLibStateInfo(uint8_t LibState, uint16_t* debugInfo) {
    return (*gfLibGetStateInfoCall)(LibState, debugInfo);
}

void MwPpgGetLib_AGC_SIGM(uint16_t *sig_val) {
  Adpd400xLibGetLibStat_AGC_SIGM(sig_val);
}

void MwPpgGetLib_CTR_Value(uint16_t *ctr_val) {
  Adpd400xLibGetLibStat_CTR_Value(ctr_val);
}

void RegisterMwPpgLibHRCB(ADPDLIB_ERROR_CODE_t (*LibCall)(LibResultX_t *, uint32_t *, int16_t *, TimeStamps_t )) {
  gfLibHRCall = LibCall;
}

void RegisterMwPpgLibGetStateCB(uint8_t (*LibCall)(void)) {
  gfLibGetStateCall = LibCall;
}

void RegisterMwPpgLibGetStateInfoCB(ADPDLIB_ERROR_CODE_t (*LibCall)(uint8_t, uint16_t *)) {
  gfLibGetStateInfoCall = LibCall;
}

void RegisterMwPpgLibOpenHRCB(ADPDLIB_ERROR_CODE_t (*LibCall)(void)) {
  gfLibOpenHRCall = LibCall;
}

void RegisterMwPpgLibCloseHRCB(ADPDLIB_ERROR_CODE_t (*LibCall)(void)) {
  gfLibCloseHRCall = LibCall;
}

static StructOpErrorStatus (*gfMwPpgLcfgStructure)(int32_t *pValue, uint8_t *nSize);
static StructOpErrorStatus (*gfMwPpgReadLcfgStructureRaw)(uint32_t field, int32_t *value);
static StructOpErrorStatus (*gfModifyLcfgStructureRaw)(uint32_t field, int32_t val);

void RegisterMwPpgGetLCFGRegCB(StructOpErrorStatus (*pfn)(int32_t *pValue, uint8_t *nSize)) {
  gfMwPpgLcfgStructure = pfn;
}

void RegisterMwPpgReadLCFGRegCB(StructOpErrorStatus (*pfn)(uint32_t field, int32_t *value)) {
  gfMwPpgReadLcfgStructureRaw = pfn;
}

void RegisterMwPpgModifyLCFGRegCB(StructOpErrorStatus (*pfn)(uint32_t field, int32_t val)) {
  gfModifyLcfgStructureRaw = pfn;
}

//////////////////////////////////////////////////////////////
#ifdef DCB

PPG_ERROR_CODE_t load_ppg_dcb(uint16_t device_id)
{
   InitOffsetsAdpd400xLcfgStruct();
   switch(device_id) {
    case 40:  {

     if(stage_ppg_dcb(&device_id) != PPG_SUCCESS)
     {
     return PPG_ERROR;
     }

      memcpy((void *)&gAdpd400xLibCfg, (const void *)&AdpdLibdcbLcfg4000, sizeof(AdpdLibdcbLcfg4000));
      MwPPG_ApplyLCFG(&gAdpd400xLibCfg);
    }
    break;
    default: {
      return PPG_ERROR;
    }
    break;
  }
  return PPG_SUCCESS;
}

/**
* @brief    Stage default LCFG to buffer
* @param    p_device_id - pointer to a device ID
* @retval   Success/Error
*/

PPG_ERROR_CODE_t stage_ppg_dcb(uint16_t *p_device_id)
{

  PPG_ERROR_CODE_t dcb_status = PPG_ERROR;
  if(p_device_id == NULL)
   {
      return dcb_status;
   }

  switch(*p_device_id) {

  case 40:  {
    static uint32_t ppg_dcb_lcfg[MAXPPGDCBSIZE] = {'\0'};
    uint16_t size = MAXPPGDCBSIZE;

    ppg_dcb_clear();
//   InitOffsetsAdpd400xLcfgStruct();
    if(read_ppg_dcb(ppg_dcb_lcfg, &size) == PPG_SUCCESS)
    {
        if(GetAdpd400xLcfgarray_to_struct(&AdpdLibdcbLcfg4000, ppg_dcb_lcfg) == STRUCTOPERROR)
        {
          dcb_status = PPG_ERROR;
        }
        else
        {
          dcb_status = PPG_SUCCESS;
        }
    }
    else
    {
        dcb_status = PPG_ERROR;
    }
    }
    break;

  default: {
      dcb_status = PPG_ERROR;
      }
      break;
    }

    return dcb_status;
}

/**
* @brief    Gets the entire ppg DCB configuration written in flash
* @param    Data - pointer to dcb struct variable, in_Size - size of data in Double Word (32-bits)
*           rec_size - The Size of the Record to be returned to the user
* @retval   Status
*/
PPG_ERROR_CODE_t read_ppg_dcb(uint32_t *ppg_dcb_data, uint16_t* read_size)
{
    PPG_ERROR_CODE_t dcb_status = PPG_ERROR;

    if(adi_dcb_read_from_fds(ADI_DCB_PPG_BLOCK_IDX, ppg_dcb_data, read_size) == DEF_OK)
    {
        dcb_status = PPG_SUCCESS;
    }
    return dcb_status;
}

/**
* @brief    Sets the entire ppg DCB configuration in flash
* @param    Data - pointer to dcb struct variable, in_Size - size of data in Double Word (32-bits)
* @retval   Status
*/
PPG_ERROR_CODE_t write_ppg_dcb(uint32_t *ppg_dcb_data, uint16_t write_Size)
{
    PPG_ERROR_CODE_t dcb_status = PPG_ERROR;

    if(adi_dcb_write_to_fds(ADI_DCB_PPG_BLOCK_IDX, ppg_dcb_data, write_Size) == DEF_OK)
    {
        dcb_status = PPG_SUCCESS;
    }

    return dcb_status;
}

/**
* @brief    Delete the entire ppg DCB configuration in flash
* @param    void
* @retval   Status
*/
PPG_ERROR_CODE_t delete_ppg_dcb(void)
{
    PPG_ERROR_CODE_t dcb_status = PPG_ERROR;

    if(adi_dcb_delete_fds_settings(ADI_DCB_PPG_BLOCK_IDX) == DEF_OK)
    {
        dcb_status = PPG_SUCCESS;
    }

    return dcb_status;
}

void ppg_dcb_clear(void)
{
  uint32_t ppg_dcb[MAXPPGDCBSIZE]={0xFF};
  if(GetAdpd400xLcfgarray_to_struct(&AdpdLibdcbLcfg4000, ppg_dcb) == STRUCTOPSUCCESS)
  {
  return;
  }
}

void ppg_set_dcb_present_flag(bool set_flag)
{
   g_ppg_dcb_Present = set_flag;
   NRF_LOG_INFO("PPG DCB present set: %s",(g_ppg_dcb_Present == true ? "TRUE" : "FALSE"));
}

bool ppg_get_dcb_present_flag(void)
{
    NRF_LOG_INFO("PPG DCB present: %s", (g_ppg_dcb_Present == true ? "TRUE" : "FALSE"));
    return g_ppg_dcb_Present;
}

void ppg_update_dcb_present_flag(void)
{
    g_ppg_dcb_Present = adi_dcb_check_fds_entry(ADI_DCB_PPG_BLOCK_IDX);
    NRF_LOG_INFO("Updated. PPG DCB present: %s", (g_ppg_dcb_Present == true ? "TRUE" : "FALSE"));
}
#endif
#endif//ENABLE_PPG_APP