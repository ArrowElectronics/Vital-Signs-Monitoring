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
 /* Includes -----------------------------------------------------------------*/
#include <adi_osal.h>
#include "hal_twi0.h"
#ifdef VSM_MBOARD
#include "watch_vsm_motherboard_pin_config.h"
#elif PCBA
#include "watch_board_pcba_pin_config.h"
#else
#include "watch_board_evt_pin_config.h"
#endif//PCBA
#include "nrf_drv_twi.h"

#include "nrf_log.h"
#if NRF_LOG_ENABLED
#include "nrf_log_ctrl.h"
#include "nrf_log_default_backends.h"
#endif

#define TWI_INSTANCE_ID     0
static const nrf_drv_twi_t m_twi = NRF_DRV_TWI_INSTANCE(TWI_INSTANCE_ID);

ADI_OSAL_MUTEX_HANDLE twi_transfer_lock = NULL;
static uint8_t twi0_mutex_flag = 0;

extern void Debug_Handler(void);

void twi0_mutex_enable(void)
{
    ADI_OSAL_STATUS eOsStatus;
    eOsStatus = adi_osal_MutexCreate(&twi_transfer_lock);
    if (eOsStatus != ADI_OSAL_SUCCESS)
    {
        Debug_Handler();
    }
    twi0_mutex_flag = 0xff;
}

uint32_t twi0_init(void)
{

    uint32_t err_code;

    const nrf_drv_twi_config_t twi_config = {
       .scl                = AD7156_TWI_SCL_PIN,
       .sda                = AD7156_TWI_SDA_PIN,
       .frequency          = NRF_DRV_TWI_FREQ_400K,
       .interrupt_priority = APP_IRQ_PRIORITY_HIGH,
       .clear_bus_init     = false
    };

    err_code = nrf_drv_twi_init(&m_twi, &twi_config, NULL, NULL);
    if ((err_code != NRF_SUCCESS)&&(err_code != NRFX_ERROR_INVALID_STATE)) //
    {
        NRF_LOG_INFO("AD7156 I2C init failue,err_code = %d",err_code);
        return err_code;
    }
    if (err_code == NRF_SUCCESS)
    {
        nrf_drv_twi_enable(&m_twi);
    }
    return err_code;

}
void twi0_uninit(void)
{
    nrf_drv_twi_uninit(&m_twi);
}

ret_code_t twi0_read_register(uint8_t deviceAddr,uint8_t registerAddr,uint8_t *data,uint8_t number)
{
    ret_code_t ret;
    uint8_t address;
    ADI_OSAL_STATUS eOsStatus;

    if(0 != twi0_mutex_flag)
    {
        eOsStatus = adi_osal_MutexPend(twi_transfer_lock, ADI_OSAL_TIMEOUT_FOREVER);
        if (eOsStatus != ADI_OSAL_SUCCESS)
        {
            Debug_Handler();
        }
    }
    address = registerAddr;
    ret = nrf_drv_twi_tx(&m_twi, deviceAddr, &address, 1, 1);
    if (NRF_SUCCESS != ret)
    {
        if(0 != twi0_mutex_flag)
        {
            eOsStatus = adi_osal_MutexPost(twi_transfer_lock);
            if (eOsStatus != ADI_OSAL_SUCCESS)
            {
                Debug_Handler();
            }
        }
        return ret;
    }
    ret =  nrf_drv_twi_rx(&m_twi, deviceAddr, data, number);
    if(0 != twi0_mutex_flag)
    {
        eOsStatus = adi_osal_MutexPost(twi_transfer_lock);
        if (eOsStatus != ADI_OSAL_SUCCESS)
        {
            Debug_Handler();
        }
    }
    return ret;
}

ret_code_t twi0_write_register(uint8_t deviceAddr,uint8_t registerAddr,uint8_t *data,uint8_t number)
{
    ret_code_t ret;
    uint8_t buffer[12];
    ADI_OSAL_STATUS eOsStatus;

    if(0 != twi0_mutex_flag)
    {
        eOsStatus = adi_osal_MutexPend(twi_transfer_lock, ADI_OSAL_TIMEOUT_FOREVER);
        if (eOsStatus != ADI_OSAL_SUCCESS)
        {
            Debug_Handler();
        }
    }
    buffer[0] = registerAddr;
    memcpy(&buffer[1],data,number);
    ret = nrf_drv_twi_tx(&m_twi, deviceAddr, buffer, (number+1),0);
    if(0 != twi0_mutex_flag)
    {
        eOsStatus = adi_osal_MutexPost(twi_transfer_lock);
        if (eOsStatus != ADI_OSAL_SUCCESS)
        {
            Debug_Handler();
        }
    }
    return ret;
}
ret_code_t twi0_send(uint8_t deviceAddr,uint8_t *data,uint8_t number)
{
    ret_code_t ret;
    ADI_OSAL_STATUS eOsStatus;

    if(0 != twi0_mutex_flag)
    {
        eOsStatus = adi_osal_MutexPend(twi_transfer_lock, ADI_OSAL_TIMEOUT_FOREVER);
        if (eOsStatus != ADI_OSAL_SUCCESS)
        {
            Debug_Handler();
        }
    }
    ret = nrf_drv_twi_tx(&m_twi, deviceAddr, data, number,0);
    if(0 != twi0_mutex_flag)
    {
        eOsStatus = adi_osal_MutexPost(twi_transfer_lock);
        if (eOsStatus != ADI_OSAL_SUCCESS)
        {
            Debug_Handler();
        }
    }
    return ret;
}