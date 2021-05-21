/******************************************************************************
*                                                                             *
* License Agreement                                                           *
*                                                                             *
* Copyright (c) 2019 Analog Devices Inc.  		                              *
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
*										                                      *
******************************************************************************/
#ifndef __TIMERS_H__
#define __TIMERS_H__

#include <inttypes.h>

#ifdef __cplusplus /* If this is a C++ compiler, use C linkage */
extern "C" {
#endif

uint32_t AdpdMwLibDeltaTime(uint32_t start, uint32_t current);
void AdpdMwLibStartCycleCount(void);
uint32_t AdpdMwLibStopCycleCount(void);

uint32_t deltaTime(unsigned long start, unsigned long current);
int32_t startTimer(const uint32_t slot);
int32_t stopTimer(const uint32_t slot, uint32_t *result);
int32_t cancelTimer(const uint32_t slot);
void startCycleCount();
uint32_t stopCycleCount();

#ifdef __cplusplus /* If this is a C++ compiler, end C linkage */
}
#endif

#endif /*__TIMERS_H__*/
