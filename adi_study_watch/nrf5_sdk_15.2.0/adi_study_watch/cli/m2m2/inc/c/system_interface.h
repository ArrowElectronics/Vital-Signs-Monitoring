// #############################################################################
// THIS IS AN AUTOMATICALLY GENERATED FILE, DO NOT MODIFY IT!
// #############################################################################
#pragma once

#include "common_application_interface.h"
#include "file_system_interface.h"
#include "m2m2_core.h"
#include <stdint.h>


/* Explicitly enforce struct packing so that the nested structs and unions are laid out
    as expected. */
#if defined __CC_ARM || defined __IAR_SYSTEMS_ICC__ || __clang__ || defined _MSC_VER || defined __GNUC__
// DELIBERATELY BLANK
#else
#error "WARNING! Your compiler might not support '#pragma pack(1)'! \
  You must add an equivalent compiler directive to the file generator!"
#endif  // defined __CC_ARM || defined __IAR_SYSTEMS_ICC__ || __clang__ || defined _MSC_VER || defined __GNUC__
#pragma pack(1)

#ifndef STATIC_ASSERT_PROJ
#define STATIC_ASSERT_PROJ(COND, MSG) typedef char static_assertion_##MSG[(COND)?1:-1]
#endif // STATIC_ASSERT_PROJ

#define PM_APPS_COUNT	21

typedef enum M2M2_PM_SYS_COMMAND_ENUM_t {
  _M2M2_PM_SYS_COMMAND_ENUM_t__M2M2_PM_SYS_COMMAND_LOWEST = 64,
  M2M2_PM_SYS_COMMAND_SET_DATE_TIME_REQ = 66,
  M2M2_PM_SYS_COMMAND_SET_DATE_TIME_RESP = 67,
  M2M2_PM_SYS_COMMAND_GET_BAT_INFO_REQ = 68,
  M2M2_PM_SYS_COMMAND_GET_BAT_INFO_RESP = 69,
  M2M2_PM_SYS_COMMAND_SET_BAT_THR_REQ = 70,
  M2M2_PM_SYS_COMMAND_SET_BAT_THR_RESP = 71,
  M2M2_PM_SYS_COMMAND_SET_PWR_STATE_REQ = 72,
  M2M2_PM_SYS_COMMAND_SET_PWR_STATE_RESP = 73,
  M2M2_PM_SYS_COMMAND_GET_INFO_REQ = 74,
  M2M2_PM_SYS_COMMAND_GET_INFO_RESP = 75,
  M2M2_PM_SYS_COMMAND_ENABLE_BAT_CHARGE_REQ = 76,
  M2M2_PM_SYS_COMMAND_ENABLE_BAT_CHARGE_RESP = 77,
  M2M2_PM_SYS_COMMAND_DISABLE_BAT_CHARGE_REQ = 78,
  M2M2_PM_SYS_COMMAND_DISABLE_BAT_CHARGE_RESP = 79,
  M2M2_PM_SYS_COMMAND_USB_PWR_REQ = 80,
  M2M2_PM_SYS_COMMAND_USB_PWR_RESP = 81,
  M2M2_PM_SYS_COMMAND_GET_DATE_TIME_REQ = 82,
  M2M2_PM_SYS_COMMAND_GET_DATE_TIME_RESP = 83,
  M2M2_PM_SYS_COMMAND_GET_BOARD_INFO_REQ = 84,
  M2M2_PM_SYS_COMMAND_GET_BOARD_INFO_RESP = 85,
  M2M2_PM_SYS_COMMAND_THERMISTOR_STATE_CHANGE_REQ = 86,
  M2M2_PM_SYS_COMMAND_THERMISTOR_STATE_CHANGE_RESP = 87,
  M2M2_PM_SYS_COMMAND_GET_MCU_VERSION_REQ = 88,
  M2M2_PM_SYS_COMMAND_GET_MCU_VERSION_RESP = 89,
  M2M2_PM_SYS_COMMAND_BOOST_REQ = 90,
  M2M2_PM_SYS_COMMAND_BOOST_RESP = 91,
  M2M2_PM_SYS_COMMAND_ADP5258_WRITE_RDAC_REQ = 92,
  M2M2_PM_SYS_COMMAND_ADP5258_WRITE_RDAC_RESP = 93,
  M2M2_PM_SYS_COMMAND_BLUETOOTH_REQ = 94,
  M2M2_PM_SYS_COMMAND_BLUETOOTH_RESP = 95,
  M2M2_PM_SYS_COMMAND_ENABLE_USER_CONFIG_LOG_REQ = 96,
  M2M2_PM_SYS_COMMAND_ENABLE_USER_CONFIG_LOG_RESP = 97,
  M2M2_PM_SYS_COMMAND_DISABLE_USER_CONFIG_LOG_REQ = 98,
  M2M2_PM_SYS_COMMAND_DISABLE_USER_CONFIG_LOG_RESP = 99,
  M2M2_PM_SYS_COMMAND_STORE_TO_EEPROM_REQ = 100,
  M2M2_PM_SYS_COMMAND_STORE_TO_EEPROM_RESP = 101,
  M2M2_PS_SYS_COMMAND_GET_LOW_TOUCH_LOGGING_STATUS_REQ = 102,
  M2M2_PS_SYS_COMMAND_GET_LOW_TOUCH_LOGGING_STATUS_RESP = 103,
  M2M2_PM_SYS_COMMAND_WRITE_EEPROM_REQ = 104,
  M2M2_PM_SYS_COMMAND_WRITE_EEPROM_RESP = 105,
  M2M2_PM_SYS_COMMAND_READ_EEPROM_REQ = 106,
  M2M2_PM_SYS_COMMAND_READ_EEPROM_RESP = 107,
  M2M2_PM_SYS_COMMAND_AD5110_WRITE_RDAC_REQ = 108,
  M2M2_PM_SYS_COMMAND_AD5110_WRITE_RDAC_RESP = 109,
  M2M2_PM_SYS_COMMAND_AD5110_READ_RDAC_REQ = 110,
  M2M2_PM_SYS_COMMAND_AD5110_READ_RDAC_RESP = 111,
  M2M2_PM_SYS_COMMAND_ADP5258_READ_RDAC_REQ = 112,
  M2M2_PM_SYS_COMMAND_ADP5258_READ_RDAC_RESP = 113,
  M2M2_PM_SYS_COMMAND_ADP5350_OVP_SETTING_REQ = 114,
  M2M2_PM_SYS_COMMAND_ADP5350_OVP_SETTING_RESP = 115,
  M2M2_PM_SYS_COMMAND_AD5110_Store_to_EEPROM_REQ = 116,
  M2M2_PM_SYS_COMMAND_AD5110_Store_to_EEPROM_RESP = 117,
  M2M2_PM_SYS_COMMAND_SYSTEM_RESET_REQ = 118,
  M2M2_PM_SYS_COMMAND_SYSTEM_RESET_RESP = 119,
  M2M2_PM_SYS_COMMAND_DG2502_SW_CNTRL_REQ = 120,
  M2M2_PM_SYS_COMMAND_DG2502_SW_CNTRL_RESP = 121,
  M2M2_PM_SYS_COMMAND_LDO_CNTRL_REQ = 122,
  M2M2_PM_SYS_COMMAND_LDO_CNTRL_RESP = 123,
  M2M2_PM_SYS_COMMAND_CHIP_ID_REQ = 124,
  M2M2_PM_SYS_COMMAND_CHIP_ID_RESP = 125,
  M2M2_PM_SYS_COMMAND_CAP_SENSE_TEST_REQ = 126,
  M2M2_PM_SYS_COMMAND_CAP_SENSE_TEST_RESP = 127,
  M2M2_PM_SYS_COMMAND_ENTER_BOOTLOADER_REQ = 128,
  M2M2_PM_SYS_COMMAND_ENTER_BOOTLOADER_RESP = 129,
  M2M2_PM_SYS_COMMAND_CAP_SENSE_STREAM_DATA = 130,
  M2M2_PM_SYS_COMMAND_GET_APPS_INFO_REQ = 132,
  M2M2_PM_SYS_COMMAND_GET_APPS_INFO_RESP = 133,
  M2M2_PM_SYS_COMMAND_ACTIVATE_TOUCH_SENSOR_REQ = 134,
  M2M2_PM_SYS_COMMAND_ACTIVATE_TOUCH_SENSOR_RESP = 135,
  M2M2_PM_SYS_COMMAND_DEACTIVATE_TOUCH_SENSOR_REQ = 136,
  M2M2_PM_SYS_COMMAND_DEACTIVATE_TOUCH_SENSOR_RESP = 137,
  M2M2_PM_SYS_COMMAND_FLASH_RESET_REQ = 138,
  M2M2_PM_SYS_COMMAND_FLASH_RESET_RESP = 139,
  M2M2_PM_SYS_COMMAND_SYSTEM_HW_RESET_REQ = 140,
  M2M2_PM_SYS_COMMAND_SYSTEM_HW_RESET_RESP = 141,
  M2M2_PM_SYS_COMMAND_FORCE_STREAM_STOP_REQ = 142,
  M2M2_PM_SYS_COMMAND_FORCE_STREAM_STOP_RESP = 143,
  M2M2_PM_GET_APPS_HEALTH_REQ = 144,
  M2M2_PM_GET_APPS_HEALTH_RESP = 145,
  M2M2_PM_SYS_BLE_GET_MAX_TX_PKT_COMB_CNT_REQ = 146,
  M2M2_PM_SYS_BLE_GET_MAX_TX_PKT_COMB_CNT_RESP = 147,
  M2M2_PM_SYS_BLE_SET_MAX_TX_PKT_COMB_CNT_REQ = 148,
  M2M2_PM_SYS_BLE_SET_MAX_TX_PKT_COMB_CNT_RESP = 149,
  M2M2_PM_SYS_COMMAND_SET_MANUFACTURE_DATE_REQ = 150,
  M2M2_PM_SYS_COMMAND_SET_MANUFACTURE_DATE_RESP = 151,
  M2M2_PM_SYS_COMMAND_GET_MANUFACTURE_DATE_REQ = 152,
  M2M2_PM_SYS_COMMAND_GET_MANUFACTURE_DATE_RESP = 153,
  M2M2_PM_SYS_GET_HIBERNATE_MODE_STATUS_REQ = 154,
  M2M2_PM_SYS_GET_HIBERNATE_MODE_STATUS_RESP = 155,
  M2M2_PM_SYS_SET_HIBERNATE_MODE_STATUS_REQ = 156,
  M2M2_PM_SYS_SET_HIBERNATE_MODE_STATUS_RESP = 157,
  M2M2_PM_SYS_BATTERY_LEVEL_ALERT = 158,
} M2M2_PM_SYS_COMMAND_ENUM_t;
STATIC_ASSERT_PROJ(sizeof(M2M2_PM_SYS_COMMAND_ENUM_t) == 1, INCORRECT_SIZE_M2M2_PM_SYS_COMMAND_ENUM_t);

typedef enum M2M2_PM_SYS_STATUS_ENUM_t {
  _M2M2_PM_SYS_STATUS_ENUM_t__M2M2_PM_SYS_STATUS_LOWEST = 64,
  M2M2_PM_SYS_STATUS_OK = 65,
  M2M2_PM_SYS_STATUS_ERR_ARGS = 66,
  M2M2_PM_SYS_STATUS_LOW_TOUCH_LOGGING_ALREADY_STARTED = 67,
  M2M2_PM_SYS_STATUS_CONFIG_FILE_NOT_FOUND = 68,
  M2M2_PM_SYS_STATUS_CONFIG_FILE_READ_ERR = 69,
  M2M2_PM_SYS_STATUS_ENABLE_USER_CONFIG_LOG_FAILED = 70,
  M2M2_PM_SYS_STATUS_USER_CONFIG_LOG_ENABLED = 71,
  M2M2_PM_SYS_STATUS_DISABLE_USER_CONFIG_LOG_FAILED = 72,
  M2M2_PM_SYS_STATUS_USER_CONFIG_LOG_DISABLED = 73,
  M2M2_PM_SYS_STATUS_LOG_STOPPED_THROUGH_BUTTON_A = 74,
  M2M2_PM_SYS_STATUS_LOW_TOUCH_LOGGING_IN_PROGRESS = 75,
  M2M2_PM_SYS_STATUS_LOW_TOUCH_LOGGING_NOT_STARTED = 76,
  M2M2_PM_SYS_STATUS_LOW_TOUCH_MAX_FILE_ERR = 77,
  M2M2_PM_SYS_STATUS_LOW_TOUCH_MEMORY_FULL_ERR = 78,
  M2M2_PM_SYS_ERR_RESET = 79,
  M2M2_PM_SYS_STATUS_ENABLE_DCB_CONFIG_LOG_FAILED = 80,
  M2M2_PM_SYS_STATUS_DCB_CONFIG_LOG_ENABLED = 81,
  M2M2_PM_SYS_STATUS_DISABLE_DCB_CONFIG_LOG_FAILED = 82,
  M2M2_PM_SYS_STATUS_DCB_CONFIG_LOG_DISABLED = 83,
  M2M2_PM_SYS_STATUS_BATTERY_LEVEL_LOW = 84,
  M2M2_PM_SYS_STATUS_BATTERY_LEVEL_CRITICAL = 85,
  M2M2_PM_SYS_STATUS_BATTERY_LEVEL_FULL = 86,
  M2M2_PM_SYS_STATUS_ERR_NOT_CHKD = 255,
} M2M2_PM_SYS_STATUS_ENUM_t;
STATIC_ASSERT_PROJ(sizeof(M2M2_PM_SYS_STATUS_ENUM_t) == 1, INCORRECT_SIZE_M2M2_PM_SYS_STATUS_ENUM_t);

typedef enum M2M2_PM_SYS_BAT_STATE_ENUM_t {
  M2M2_PM_SYS_BAT_STATE_NOT_AVAIL = 0,
  M2M2_PM_SYS_BAT_STATE_NOT_CHARGING = 1,
  M2M2_PM_SYS_BAT_STATE_CHARGING = 2,
  M2M2_PM_SYS_BAT_STATE_COMPLETE = 3,
  M2M2_PM_SYS_BAT_STATE_CHARGE_LDO_MODE = 4,
  M2M2_PM_SYS_BAT_STATE_CHARGE_TIMER_EXPIRED = 5,
  M2M2_PM_SYS_BAT_STATE_DETECTION = 6,
  M2M2_PM_SYS_BAT_STATE_CHARGE_ERR = 7,
} M2M2_PM_SYS_BAT_STATE_ENUM_t;
STATIC_ASSERT_PROJ(sizeof(M2M2_PM_SYS_BAT_STATE_ENUM_t) == 1, INCORRECT_SIZE_M2M2_PM_SYS_BAT_STATE_ENUM_t);

typedef enum M2M2_PM_SYS_USB_PWR_ENUM_t {
  M2M2_PM_SYS_USB_PWR_ENABLE = 0,
  M2M2_PM_SYS_USB_PWR_DISABLE = 1,
} M2M2_PM_SYS_USB_PWR_ENUM_t;
STATIC_ASSERT_PROJ(sizeof(M2M2_PM_SYS_USB_PWR_ENUM_t) == 1, INCORRECT_SIZE_M2M2_PM_SYS_USB_PWR_ENUM_t);

typedef enum M2M2_PM_SYS_THERMISTOR_STATE_ENUM_t {
  M2M2_PM_SYS_THERMISTOR_ENABLE = 0,
  M2M2_PM_SYS_THERMISTOR_DISABLE = 1,
} M2M2_PM_SYS_THERMISTOR_STATE_ENUM_t;
STATIC_ASSERT_PROJ(sizeof(M2M2_PM_SYS_THERMISTOR_STATE_ENUM_t) == 1, INCORRECT_SIZE_M2M2_PM_SYS_THERMISTOR_STATE_ENUM_t);

typedef enum M2M2_PM_SYS_USB_PWR_ACTION_ENUM_t {
  M2M2_PM_SYS_USB_PWR_SET = 0,
  M2M2_PM_SYS_USB_PWR_GET = 1,
} M2M2_PM_SYS_USB_PWR_ACTION_ENUM_t;
STATIC_ASSERT_PROJ(sizeof(M2M2_PM_SYS_USB_PWR_ACTION_ENUM_t) == 1, INCORRECT_SIZE_M2M2_PM_SYS_USB_PWR_ACTION_ENUM_t);

typedef enum M2M2_PM_SYS_PWR_STATE_ENUM_t {
  M2M2_PM_SYS_PWR_STATE_ACTIVE = 0,
  M2M2_PM_SYS_PWR_STATE_HIBERNATE = 2,
  M2M2_PM_SYS_PWR_STATE_SHUTDOWN = 3,
} M2M2_PM_SYS_PWR_STATE_ENUM_t;
STATIC_ASSERT_PROJ(sizeof(M2M2_PM_SYS_PWR_STATE_ENUM_t) == 1, INCORRECT_SIZE_M2M2_PM_SYS_PWR_STATE_ENUM_t);

typedef enum M2M2_PM_SYS_MCU_TYPE_ENUM_t {
  M2M2_PM_SYS_MCU_INVALID = 0,
  M2M2_PM_SYS_MCU_M3 = 1,
  M2M2_PM_SYS_MCU_M4 = 2,
} M2M2_PM_SYS_MCU_TYPE_ENUM_t;
STATIC_ASSERT_PROJ(sizeof(M2M2_PM_SYS_MCU_TYPE_ENUM_t) == 1, INCORRECT_SIZE_M2M2_PM_SYS_MCU_TYPE_ENUM_t);

typedef enum M2M2_PM_SYS_OVP_ENUM_t {
  M2M2_PM_SYS_OVP_18_5V = 0,
  M2M2_PM_SYS_OVP_15V = 1,
  M2M2_PM_SYS_OVP_10V = 2,
  M2M2_PM_SYS_OVP_5_6V = 3,
} M2M2_PM_SYS_OVP_ENUM_t;
STATIC_ASSERT_PROJ(sizeof(M2M2_PM_SYS_OVP_ENUM_t) == 1, INCORRECT_SIZE_M2M2_PM_SYS_OVP_ENUM_t);

typedef enum M2M2_PM_SYS_BOOST_ENUM_t {
  M2M2_PM_SYS_BOOST_ENABLE = 0,
  M2M2_PM_SYS_BOOST_DISABLE = 1,
} M2M2_PM_SYS_BOOST_ENUM_t;
STATIC_ASSERT_PROJ(sizeof(M2M2_PM_SYS_BOOST_ENUM_t) == 1, INCORRECT_SIZE_M2M2_PM_SYS_BOOST_ENUM_t);

typedef enum M2M2_PM_SYS_DG2502_SELECT_ENUM_t {
  M2M2_PM_SYS_DG2502_8233_SW = 0,
  M2M2_PM_SYS_DG2502_5940_SW = 1,
  M2M2_PM_SYS_DG2502_4K_SW = 2,
} M2M2_PM_SYS_DG2502_SELECT_ENUM_t;
STATIC_ASSERT_PROJ(sizeof(M2M2_PM_SYS_DG2502_SELECT_ENUM_t) == 1, INCORRECT_SIZE_M2M2_PM_SYS_DG2502_SELECT_ENUM_t);

typedef enum M2M2_PM_SYS_DG2502_EN_ENUM_t {
  M2M2_PM_SYS_DG2502_DISABLE = 0,
  M2M2_PM_SYS_DG2502_ENABLE = 1,
} M2M2_PM_SYS_DG2502_EN_ENUM_t;
STATIC_ASSERT_PROJ(sizeof(M2M2_PM_SYS_DG2502_EN_ENUM_t) == 1, INCORRECT_SIZE_M2M2_PM_SYS_DG2502_EN_ENUM_t);

typedef enum M2M2_PM_SYS_CHIP_ID_ENUM_t {
  M2M2_PM_SYS_ADXL362 = 1,
  M2M2_PM_SYS_ADPD4K = 2,
  M2M2_PM_SYS_ADP5360 = 3,
  M2M2_PM_SYS_AD5940 = 4,
  M2M2_PM_SYS_NAND_FLASH = 5,
  M2M2_PM_SYS_AD7156 = 6,
} M2M2_PM_SYS_CHIP_ID_ENUM_t;
STATIC_ASSERT_PROJ(sizeof(M2M2_PM_SYS_CHIP_ID_ENUM_t) == 1, INCORRECT_SIZE_M2M2_PM_SYS_CHIP_ID_ENUM_t);

typedef enum ADI_PM_BOARD_TYPE_t {
  ADI_PM_BOARD_TYPE_UNKNOWN = 0,
  ADI_PM_BOARD_TYPE_ADPD107_WATCH = 1,
  ADI_PM_BOARD_TYPE_ADPD107_CHEST_STRAP = 2,
  ADI_PM_BOARD_TYPE_ADPD185_WATCH = 3,
  ADI_PM_BOARD_TYPE_ADPD188_WATCH = 4,
} ADI_PM_BOARD_TYPE_t;
STATIC_ASSERT_PROJ(sizeof(ADI_PM_BOARD_TYPE_t) == 1, INCORRECT_SIZE_ADI_PM_BOARD_TYPE_t);

typedef struct _m2m2_pm_sys_cmd_t {
  uint8_t  command; 
  uint8_t  status; 
} m2m2_pm_sys_cmd_t;

typedef struct _m2m2_file_reset_cmd_t {
  uint8_t  command; 
  uint8_t  status; 
} m2m2_file_reset_cmd_t;

typedef struct _m2m2_pm_sys_pwr_state_t {
  uint8_t  command; 
  uint8_t  status; 
  uint8_t  state; 
} m2m2_pm_sys_pwr_state_t;

typedef struct _m2m2_pm_sys_info_t {
  uint8_t  command; 
  uint8_t  status; 
  uint16_t  version; 
  uint8_t  mac_addr[6]; 
  uint32_t  device_id; 
  uint32_t  model_number; 
  uint16_t  hw_id; 
  uint16_t  bom_id; 
  uint8_t  batch_id; 
  uint32_t  date; 
  ADI_PM_BOARD_TYPE_t  board_type; 
} m2m2_pm_sys_info_t;

typedef struct _m2m2_pm_sys_date_time_req_t {
  uint8_t  command; 
  uint8_t  status; 
  uint16_t  year; 
  uint8_t  month; 
  uint8_t  day; 
  uint8_t  hour; 
  uint8_t  minute; 
  uint8_t  second; 
  uint32_t  TZ_sec; 
} m2m2_pm_sys_date_time_req_t;

typedef struct _m2m2_manufacture_date_t {
  uint8_t  command; 
  uint8_t  status; 
  uint16_t  year; 
  uint8_t  month; 
  uint8_t  day; 
} m2m2_manufacture_date_t;

typedef struct _m2m2_pm_sys_bat_info_resp_t {
  uint8_t  command; 
  uint8_t  status; 
  uint32_t  timestamp; 
  M2M2_PM_SYS_BAT_STATE_ENUM_t  bat_chrg_stat; 
  uint8_t  bat_lvl; 
  uint16_t  bat_mv; 
} m2m2_pm_sys_bat_info_resp_t;

typedef struct _m2m2_pm_sys_bat_thr_req_t {
  uint8_t  command; 
  uint8_t  status; 
  uint8_t  bat_level_low; 
  uint8_t  bat_level_critical; 
} m2m2_pm_sys_bat_thr_req_t;

typedef struct _m2m2_bat_crit_info_resp_t {
  uint8_t  command; 
  uint8_t  status; 
  uint16_t  bat_mv; 
} m2m2_bat_crit_info_resp_t;

typedef struct _m2m2_pm_sys_usb_pwr_cmd_t {
  uint8_t  command; 
  uint8_t  status; 
  M2M2_PM_SYS_USB_PWR_ENUM_t  enable; 
  M2M2_PM_SYS_USB_PWR_ACTION_ENUM_t  action; 
} m2m2_pm_sys_usb_pwr_cmd_t;

typedef struct _m2m2_pm_sys_bat_thermistor_cmd_t {
  uint8_t  command; 
  uint8_t  status; 
  M2M2_PM_SYS_THERMISTOR_STATE_ENUM_t  enable; 
} m2m2_pm_sys_bat_thermistor_cmd_t;

typedef struct _m2m2_pm_sys_mcu_version_t {
  uint8_t  command; 
  uint8_t  status; 
  M2M2_PM_SYS_MCU_TYPE_ENUM_t  mcu; 
} m2m2_pm_sys_mcu_version_t;

typedef struct _m2m2_pm_sys_boost_cmd_t {
  uint8_t  command; 
  uint8_t  status; 
  M2M2_PM_SYS_BOOST_ENUM_t  enable; 
  M2M2_PM_SYS_OVP_ENUM_t  ovp; 
} m2m2_pm_sys_boost_cmd_t;

typedef struct _m2m2_pm_sys_powerboost_cmd_t {
  uint8_t  command; 
  uint8_t  status; 
  M2M2_PM_SYS_BOOST_ENUM_t  enable; 
  uint8_t  ovp; 
} m2m2_pm_sys_powerboost_cmd_t;

typedef struct _m2m2_pm_sys_eeprom_cmd_t {
  uint8_t  command; 
  uint8_t  status; 
  uint8_t  size; 
  uint8_t  byte_stream[16]; 
} m2m2_pm_sys_eeprom_cmd_t;

typedef struct _m2m2_pm_sys_dg2502_sw_ctrl_cmd_t {
  uint8_t  command; 
  uint8_t  status; 
  M2M2_PM_SYS_DG2502_SELECT_ENUM_t  sw_name; 
  M2M2_PM_SYS_DG2502_EN_ENUM_t  sw_enable; 
} m2m2_pm_sys_dg2502_sw_ctrl_cmd_t;

typedef struct _m2m2_pm_sys_ldo_ctrl_cmd_t {
  uint8_t  command; 
  uint8_t  status; 
  uint8_t  ldo_num; 
  uint8_t  ldo_enable; 
} m2m2_pm_sys_ldo_ctrl_cmd_t;

typedef struct _m2m2_pm_sys_chip_id_cmd_t {
  uint8_t  command; 
  uint8_t  status; 
  M2M2_PM_SYS_CHIP_ID_ENUM_t  chip_name; 
  uint16_t  chip_id; 
} m2m2_pm_sys_chip_id_cmd_t;

typedef struct _m2m2_pm_sys_cap_sense_test_cmd_t {
  uint8_t  command; 
  uint8_t  status; 
  uint8_t  enable; 
} m2m2_pm_sys_cap_sense_test_cmd_t;

typedef struct _m2m2_pm_sys_cap_sense_test_data_t {
  uint8_t  command; 
  uint8_t  status; 
  uint8_t  touch_position; 
  uint8_t  touch_value; 
} m2m2_pm_sys_cap_sense_test_data_t;

typedef struct _m2m2_pm_sys_enter_bloader_cmd_t {
  uint8_t  command; 
  uint8_t  status; 
} m2m2_pm_sys_enter_bloader_cmd_t;

typedef struct _m2m2_pm_sys_sensor_app_status {
  M2M2_ADDR_ENUM_t  sensor_app_id; 
  M2M2_ADDR_ENUM_t  sensor_stream; 
  uint8_t  num_subscribers; 
  uint8_t  num_start_reqs; 
  uint16_t  fs_sub_stat; 
} m2m2_pm_sys_sensor_app_status;

typedef struct _m2m2_pm_sys_sensor_apps_info_req_t {
  uint8_t  command; 
  uint8_t  status; 
  uint16_t  num_sensor_apps; 
  m2m2_pm_sys_sensor_app_status  app_info[21]; 
} m2m2_pm_sys_sensor_apps_info_req_t;

typedef struct _m2m2_pm_force_stream_stop_cmd_t {
  uint8_t  command; 
  uint8_t  status; 
} m2m2_pm_force_stream_stop_cmd_t;

typedef struct _m2m2_get_apps_running_stat_req_cmd_t {
  uint8_t  command; 
  uint8_t  status; 
} m2m2_get_apps_running_stat_req_cmd_t;

typedef struct _m2m2_get_apps_running_stat_resp_cmd_t {
  uint8_t  command; 
  uint8_t  status; 
  uint32_t  ad5940_isr_cnt; 
  uint32_t  adpd4000_isr_cnt; 
  uint32_t  adxl_isr_cnt; 
} m2m2_get_apps_running_stat_resp_cmd_t;

typedef struct _m2m2_ble_max_tx_pkt_comb_cnt_resp_cmd_t {
  uint8_t  command; 
  uint8_t  status; 
  uint8_t  max_tx_pkt_comb_cnt; 
} m2m2_ble_max_tx_pkt_comb_cnt_resp_cmd_t;

typedef struct _m2m2_hibernate_mode_status_resp_cmd_t {
  uint8_t  command; 
  uint8_t  status; 
  uint8_t  hib_mode_status; 
} m2m2_hibernate_mode_status_resp_cmd_t;

// Reset struct packing outside of this file
#pragma pack()
