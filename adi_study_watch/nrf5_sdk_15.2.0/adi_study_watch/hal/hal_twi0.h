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

#ifndef __HAL_TWI0_H__
#define __HAL_TWI0_H__

#include <stdint.h>
#include <nrfx.h>
/*
* @brief  twi0 interface initialize.
* @param[in]:NULL
*   
* @return RESULT A 32-bit integer: 0 - success; < 0 - failure
*/
uint32_t twi0_init(void);
/*
* @brief  twi0 interface de-initialize.
*/
void twi0_uninit(void);
/*
* @brief  enable mutex function of twi0.
*/
void twi0_mutex_enable(void);
/*
* @brief  through twi0 to read register value.
* @param[in]:
*   @deviceAddr device address.
*   @registerAddr register address.
*   @number number of read operation
* @param[out]:
*   @data pointer of read buffer.
* @return RESULT A 32-bit integer: 0 - success; < 0 - failure
*/
ret_code_t twi0_read_register(uint8_t deviceAddr,uint8_t registerAddr,uint8_t *data,uint8_t number);
/*
* @brief  through twi0 to write register value.
* @param[in]:
*   @deviceAddr device address.
*   @registerAddr register address.
*   @data pointer of write buffer.
*   @number number of write operation
* @return RESULT A 32-bit integer: 0 - success; < 0 - failure
*/
ret_code_t twi0_write_register(uint8_t deviceAddr,uint8_t registerAddr,uint8_t *data,uint8_t number);
/*
* @brief  through twi0 to send data.
* @param[in]:
*   @deviceAddr device address.
*   @data pointer of send buffer.
*   @number number of send operation
* @return RESULT A 32-bit integer: 0 - success; < 0 - failure
*/
ret_code_t twi0_send(uint8_t deviceAddr,uint8_t *data,uint8_t number);
#endif
