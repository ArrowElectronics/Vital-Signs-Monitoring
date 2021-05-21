/**
    ***************************************************************************
    * @file         AppSync.c
    * @author       ADI
    * @version      V1.0.0
    * @date         Oct-2015
    * @brief        Sync abstraction layer
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
* This software is intended for use with the ADPD142 and derivative parts     *
* only                                                                        *
*                                                                             *
******************************************************************************/

#ifndef __APPSYNC_H__
#define __APPSYNC_H__

/* Includes -----------------------------------------------------------------*/
#include <stdint.h>

#define WATERMARK_VALUE 4
#define ADPD_CHANNEL_CNT 8
#define ADXL_CHANNEL_CNT 3

typedef enum {
  NO_HW_SYNCH = 0,
  ADPD_INT_TRIGGERS,
  ADPD_PDSO_INT_TRIGGERS,
  ADPD_ODCS_TRIGGERS,
  ADPD_SW_SYNC4,
  ADXL_INT_TRIGGERS,
  EXTERNAL_32KHZ_CLOCK,
} SynchMode_t;

typedef enum {
    SYNC_SUCCESS = 0,
    SYNC_BUFFERING,
    SYNC_ERROR
}SyncErrorStatus;

typedef struct  {
	uint32_t nDataValue[ADPD_CHANNEL_CNT];
	uint32_t nTimeStamp;
} AdpdDataStruct;

typedef struct {
	int16_t nDataValue[ADXL_CHANNEL_CNT];
	uint32_t nTimeStamp;
} AdxlDataStruct;

extern SynchMode_t  GetSyncMode();
extern void SetSyncMode(SynchMode_t eSyncMode);
extern void DisplaySyncMode(SynchMode_t eSyncMode);
extern void SyncInit();
extern void SyncDeInit();
extern void AdpdSSTrigger();
extern void SyncClearDataBuffer();
extern SyncErrorStatus DoSync(uint32_t *pPpgData, uint32_t nAdpdTs, uint16_t *pnAccelData, uint32_t nAccelTs);
extern uint32_t *GetSyncAdpdData();
extern int16_t *GetSyncAccelData();
extern uint16_t *GetSyncRawAccelData();
extern uint32_t GetSyncAdpdTs();
extern uint32_t GetSyncAccelTs();
extern uint32_t GetSyncAvaiableSample();
extern uint32_t AdpdLibGetSetSampleIndex(uint8_t type);
extern uint8_t app_sync_timer_interval();
#endif

