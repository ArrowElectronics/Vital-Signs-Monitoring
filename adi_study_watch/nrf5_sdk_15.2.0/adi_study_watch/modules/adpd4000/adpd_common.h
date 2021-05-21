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
* This software is intended for use with the ADUX1020 and derivative parts    *
* only                                                                        *
*                                                                             *
******************************************************************************/
#ifndef __ADPDLIBCOMMON_H__
#define __ADPDLIBCOMMON_H__

#ifdef __cplusplus /* If this is a C++ compiler, use C linkage */
extern "C" {
#endif

#include "printf.h"

#define BUF_SIZE (80)
extern char _SBZ[BUF_SIZE];

#if 0
#ifdef _USING_RTOS_
#define debug(M, ...)  {adi_osal_MutexPend(DebugTransferLock, ADI_OSAL_TIMEOUT_FOREVER); \
						_SBZ[0] = 0; \
                        snprintf(_SBZ, BUF_SIZE, "" M "", ##__VA_ARGS__); \
                        LoggerMessage(DEBUG_LEVEL1, TX_USB_UART_AND_BT, "Application", _SBZ, NOT_USED); \
                        adi_osal_MutexPost(DebugTransferLock); }

#define debugSD(M, ...)  {adi_osal_MutexPend(DebugTransferLock, ADI_OSAL_TIMEOUT_FOREVER); \
						_SBZ[0] = 0; \
                        snprintf(_SBZ, BUF_SIZE, "" M "", ##__VA_ARGS__); \
                        LoggerMessage(DEBUG_LEVEL1, TX_SDCARD, "Application", _SBZ, NOT_USED); \
                        adi_osal_MutexPost(DebugTransferLock); }
#else
#define debug(M, ...)  {_SBZ[0] = 0; \
                        snprintf(_SBZ, BUF_SIZE, "" M "", ##__VA_ARGS__); \
                        adi_printf("%s", _SBZ);}

/*Print to SD card from here*/
#define debugSD(M, ...)  {_SBZ[0] = 0; \
                        snprintf(_SBZ, BUF_SIZE, "" M "", ##__VA_ARGS__);}
#endif //_USING_RTOS_
#endif // if 0
// #define debug(M, ...)  {_SBZ[0] = 0; \
                        snprintf(_SBZ, BUF_SIZE, "" M "", ##__VA_ARGS__); \
                        adi_printf("%s", _SBZ);}
  // sprintf(aDebugString,__VA_ARGS__);
#define debug(M, ...)  {_SBZ[0] = 0; \
                        snprintf(_SBZ, BUF_SIZE, ##__VA_ARGS__); \
                        adi_printf("%s", _SBZ);}

#define AdpdMwLibSetMode(a, b, c)   \
              AdpdDrvSetSlot(b, c); adpd_buff_reset(0); AdpdDrvSetOperationMode(a);

/* External Variables -------------------------------------------------------*/
typedef enum {
    IERR_SUCCESS                    =  0,
    IERR_SUCCESS_WITH_RESULT        =  1,
    IERR_FAIL                       =  2,
    IERR_IN_PROGRESS                = -10
} INT_ERROR_CODE_t;

typedef struct AdpdLibConfig {
/*  AdpdLib2 specific parameters  */
/*  New elements to be added at the end of the structure  */
    uint16_t partNum;
    uint8_t  targetChs;
    uint16_t devicemode;

    uint8_t  dcLevelMaxPercent;
    uint8_t  ledMaxCurrent;
    uint8_t  maxPulseNum;
    uint8_t  floatModeCtr;
    uint8_t  ledB_Vol;
    uint8_t  dcLevelPercentA;
} AdpdLibConfig_t;

/* External Functions -------------------------------------------------------*/
extern AdpdLibConfig_t *g_lcfg_PmOnly;

#ifdef __cplusplus /* If this is a C++ compiler, end C linkage */
}
#endif

#endif /*__ADPDLIBCOMMON_H__*/
