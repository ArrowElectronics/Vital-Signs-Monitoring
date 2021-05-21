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
#ifndef __STRUCTOPERATIONS_H_
#define __STRUCTOPERATIONS_H_

#include "hw_if_config.h"

typedef enum {
  STRUCTOPSUCCESS = 0,
  STRUCTOPERROR
}StructOpErrorStatus;

void InitOffsetsLcfgStruct();
StructOpErrorStatus ModifyLcfgStructure(char *field, int32_t val);
StructOpErrorStatus ModifyLcfgStructureRaw(uint32_t field, int32_t val);
StructOpErrorStatus ReadLcfgStructureRaw(uint32_t field, int32_t *value);
StructOpErrorStatus ReadLcfgStructure(char *field, int32_t *value);
StructOpErrorStatus GetLcfgStructure(int32_t *value, uint8_t *nSize);

StructOpErrorStatus GetAdpd400xLcfgarray_to_struct(Adpd400xLibConfig_t* dcblcfg, uint32_t* array);

void InitOffsetsAdpd400xLcfgStruct();
StructOpErrorStatus GetAdpd400xLcfgStructure(int32_t *pValue, uint8_t *nSize);
StructOpErrorStatus ReadAdpd400xLcfgStructureRaw(uint32_t field, int32_t *value);
StructOpErrorStatus ModifyAdpd400xLcfgStructureRaw(uint32_t field, int32_t val);
void ClearLcfgStruct();

#endif
