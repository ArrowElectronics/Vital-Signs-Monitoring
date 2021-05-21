/**
******************************************************************************
* @file     sensor_internal.h
* @author   ADI
* @version  V1.0.0
* @date     12-August-2016
* @brief    Include file for common internal structures across all sensor
*           applications
******************************************************************************
* @attention
******************************************************************************
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
* This software is intended for use with all sensors                          *
*                                                                             *
*                                                                             *
******************************************************************************/


#ifndef _SENSOR_INTERNAL_H_
#define _SENSOR_INTERNAL_H_

/*!
 * @brief:  Status enum that is common to all sensors.
 */
typedef enum {
  // Inherit from M2M2_SENSOR_COMMON_STATUS_ENUM_t
  M2M2_SENSOR_INTERNAL_STATUS_LOWEST = 0x40,
  // Application Packetization statuses
  M2M2_SENSOR_INTERNAL_STATUS_PKT_ERROR,
  M2M2_SENSOR_INTERNAL_STATUS_PKT_NO_READY,
  M2M2_SENSOR_INTERNAL_STATUS_PKT_READY,
  // Allow to inherit from this
  M2M2_SENSOR_INTERNAL_STATUS_HIGHEST = 0x50,
}M2M2_SENSOR_INTERNAL_STATUS_ENUM_t;

#endif  //  _SENSOR_INTERNAL_H_
