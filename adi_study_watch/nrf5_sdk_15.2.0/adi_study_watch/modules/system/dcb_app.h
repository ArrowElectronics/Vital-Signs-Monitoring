#ifndef DCB_APP_H__
#define DCB_APP_H__


#include "FreeRTOS.h"
#include <nrfx.h>
#include "DCB_drv.h"
#include "task.h"
#include "nrf.h"
#include "nordic_common.h"
#include "boards.h"
#include "app_timer.h"
#include "app_util.h"
#include "nrf_sdh.h"
#include "nrf_sdh_ble.h"
#include "nrf_log.h"
#include "nrf_log_ctrl.h"
#include "nrf_log_default_backends.h"
#define BUTTON_DETECTION_DELAY  APP_TIMER_TICKS(50)
#define APP_BLE_CONN_CFG_TAG    1

static void clock_init(void);
static void log_init(void);
#endif