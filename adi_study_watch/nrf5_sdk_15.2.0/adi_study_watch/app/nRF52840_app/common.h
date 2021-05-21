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

#ifndef _COMMON_H_
#define _COMMON_H_
#include <stdint.h>
#include <string.h>
#include <stdio.h>

#ifdef _USING_RTOS_
#include "TaskIncludes.h"
#endif //_USING_RTOS_
#include "hw_if_config.h"

/* --------------------------- definitions -------------------------------- */
#ifndef ABS
#define ABS(i_x) (((i_x) > 0) ? (i_x): -(i_x))
#endif

#define BUF_SIZE (256)

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

/* ------------------------- Global typedef ------------------------------- */
typedef enum {

  AppErrPass        = 0x00,
  AppErrFail,
  AppProgress,
} App_ErrCode_t;

typedef struct TMSTMP {

  uint32_t Yr;
  uint32_t Mt;
  uint32_t Dy;
  uint32_t Hr;
  uint32_t Min;
  uint32_t Sec;

} TimeStamp;

/* ------------------------- Global Variable ------------------------------ */
extern uint16_t  FirmWare_Version;
extern uint16_t  DebugVal[16];
extern char _SBZ[BUF_SIZE];
/* ------------------------- Function Prototype --------------------------- */
extern void adi_printf (uint8_t*, ...);
extern uint8_t adi_strcpy (uint8_t*, uint8_t*);
extern uint8_t adi_strcat (uint8_t *dst, uint8_t *src);
extern uint8_t adi_strncmp(uint8_t *strg1, uint8_t *strg2, uint8_t n);
extern void adi_memset(uint16_t  *src, uint16_t  setValue, uint16_t  count);
extern uint8_t  CharToInt(uint8_t  ch);
extern uint32_t IncCharsToInt32(uint8_t *ch);
extern uint8_t  HexCharsToInt8(uint8_t  *ch);
extern uint16_t HexCharsToInt16(uint8_t *ch);

extern void consoleSendCh(uint8_t );
extern void consoleSendString(uint8_t *);

#endif
