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
#include "manufacture_date.h"
#include "fds_drv.h"
#include "fds.h"
#include "nrf_log.h"
#if NRF_LOG_ENABLED
#include "nrf_log_ctrl.h"
#include "nrf_log_default_backends.h"
#endif
/* 
  * @brief: Function to save the manufacture date.
*/
uint32_t manufacture_date_save(manufacture_date_t *date)
{
    if(date == NULL)
    {
        return FDS_ERR_NULL_ARG;
    }
    NRF_LOG_INFO("FDS write date: %04d-%02d-%02d",date->year, date->month, date->day);
    return (adi_fds_update_entry(ADI_RTC_FILE, MANUFACTURE_DATE_BLOCK_IDX, date, sizeof(manufacture_date_t)));
}

/* 
  * @brief: Function to read the manufacture date.
*/
uint32_t manufacture_date_read(manufacture_date_t *date)
{
    if(date == NULL)
    {
        return FDS_ERR_NULL_ARG;
    }
    uint16_t len = sizeof(manufacture_date_t);
    return (adi_fds_read_entry(ADI_RTC_FILE, MANUFACTURE_DATE_BLOCK_IDX, date, &len));
}