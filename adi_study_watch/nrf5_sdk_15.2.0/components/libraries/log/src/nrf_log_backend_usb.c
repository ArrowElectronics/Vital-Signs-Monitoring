/**
 * Copyright (c) 2016 - 2018, Nordic Semiconductor ASA
 *
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice, this
 *    list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form, except as embedded into a Nordic
 *    Semiconductor ASA integrated circuit in a product or a software update for
 *    such product, must reproduce the above copyright notice, this list of
 *    conditions and the following disclaimer in the documentation and/or other
 *    materials provided with the distribution.
 *
 * 3. Neither the name of Nordic Semiconductor ASA nor the names of its
 *    contributors may be used to endorse or promote products derived from this
 *    software without specific prior written permission.
 *
 * 4. This software, with or without modification, must only be used with a
 *    Nordic Semiconductor ASA integrated circuit.
 *
 * 5. Any software provided in binary form under this license must not be reverse
 *    engineered, decompiled, modified and/or disassembled.
 *
 * THIS SOFTWARE IS PROVIDED BY NORDIC SEMICONDUCTOR ASA "AS IS" AND ANY EXPRESS
 * OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY, NONINFRINGEMENT, AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL NORDIC SEMICONDUCTOR ASA OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
 * GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
 * OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */
#include "sdk_common.h"
#if NRF_MODULE_ENABLED(NRF_LOG) && NRF_MODULE_ENABLED(NRF_LOG_BACKEND_USB)
#include "nrf_log_backend_usb.h"
#include "nrf_log_backend_serial.h"
#include "nrf_log_internal.h"
#include "usbd_app.h"
#include "app_error.h"


static uint8_t m_string_buff[NRF_LOG_BACKEND_USB_TEMP_BUFFER_SIZE];

void nrf_log_backend_usb_init(void)
{
    
}

static void serial_tx(void const * p_context, char const * p_buffer, size_t len)
{
    uint8_t len8 = (uint8_t)(len & 0x000000FF);
    ret_code_t err_code = app_usbd_cdc_send((uint8_t *)p_buffer, len8);
//    APP_ERROR_CHECK(err_code);
}

static void nrf_log_backend_usb_put(nrf_log_backend_t const * p_backend,
                                     nrf_log_entry_t * p_msg)
{
    nrf_log_backend_serial_put(p_backend, p_msg, m_string_buff,
                               NRF_LOG_BACKEND_USB_TEMP_BUFFER_SIZE, serial_tx);
}

static void nrf_log_backend_usb_flush(nrf_log_backend_t const * p_backend)
{

}

static void nrf_log_backend_usb_panic_set(nrf_log_backend_t const * p_backend)
{
    
}

const nrf_log_backend_api_t nrf_log_backend_usb_api = {
        .put       = nrf_log_backend_usb_put,
        .flush     = nrf_log_backend_usb_flush,
        .panic_set = nrf_log_backend_usb_panic_set,
};
#endif //NRF_MODULE_ENABLED(NRF_LOG) && NRF_MODULE_ENABLED(NRF_LOG_BACKEND_UART)
