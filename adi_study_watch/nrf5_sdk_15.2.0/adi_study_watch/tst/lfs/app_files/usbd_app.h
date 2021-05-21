#ifndef _USBD_APP_H_
#define _USBD_APP_H_
#include <stdint.h>
#include "sdk_errors.h"

void Usbd_app_init(void);
ret_code_t app_usbd_cdc_send(const void * p_buf,uint16_t length);
uint8_t app_usb_cdc_is_open(void);
#endif

