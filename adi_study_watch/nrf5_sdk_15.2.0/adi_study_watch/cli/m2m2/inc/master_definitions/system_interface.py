#!/usr/bin/env python3

from ctypes import *

import m2m2_core

import common_application_interface

import file_system_interface

"""
The macro is used to specify the number of applications in
the system, for which Info maybe obtained. This is based on
the entries in m2m2_routing_table[] in post_office.c file,
excluding the indices from 0-13 which is for system upkeeping.
As of now there are 20 applications, including the status which
could be queried from Slot A-L from ADPD application.
"""

PM_APPS_COUNT = (21)

class M2M2_PM_SYS_COMMAND_ENUM_t(c_uint8):
    __M2M2_PM_SYS_COMMAND_LOWEST = 0x40
    M2M2_PM_SYS_COMMAND_SET_DATE_TIME_REQ = 0x42
    M2M2_PM_SYS_COMMAND_SET_DATE_TIME_RESP = 0x43
    M2M2_PM_SYS_COMMAND_GET_BAT_INFO_REQ = 0x44
    M2M2_PM_SYS_COMMAND_GET_BAT_INFO_RESP = 0x45
    M2M2_PM_SYS_COMMAND_SET_BAT_THR_REQ = 0x46
    M2M2_PM_SYS_COMMAND_SET_BAT_THR_RESP = 0x47
    M2M2_PM_SYS_COMMAND_SET_PWR_STATE_REQ = 0x48
    M2M2_PM_SYS_COMMAND_SET_PWR_STATE_RESP = 0x49
    M2M2_PM_SYS_COMMAND_GET_INFO_REQ = 0x4A
    M2M2_PM_SYS_COMMAND_GET_INFO_RESP = 0x4B
    M2M2_PM_SYS_COMMAND_ENABLE_BAT_CHARGE_REQ = 0x4C
    M2M2_PM_SYS_COMMAND_ENABLE_BAT_CHARGE_RESP = 0x4D
    M2M2_PM_SYS_COMMAND_DISABLE_BAT_CHARGE_REQ = 0x4E
    M2M2_PM_SYS_COMMAND_DISABLE_BAT_CHARGE_RESP = 0x4F
    M2M2_PM_SYS_COMMAND_USB_PWR_REQ = 0x50
    M2M2_PM_SYS_COMMAND_USB_PWR_RESP = 0x51
    M2M2_PM_SYS_COMMAND_GET_DATE_TIME_REQ = 0x52
    M2M2_PM_SYS_COMMAND_GET_DATE_TIME_RESP = 0x53
    M2M2_PM_SYS_COMMAND_GET_BOARD_INFO_REQ = 0x54
    M2M2_PM_SYS_COMMAND_GET_BOARD_INFO_RESP = 0x55
    M2M2_PM_SYS_COMMAND_THERMISTOR_STATE_CHANGE_REQ = 0x56
    M2M2_PM_SYS_COMMAND_THERMISTOR_STATE_CHANGE_RESP = 0x57
    M2M2_PM_SYS_COMMAND_GET_MCU_VERSION_REQ = 0x58
    M2M2_PM_SYS_COMMAND_GET_MCU_VERSION_RESP = 0x59
    M2M2_PM_SYS_COMMAND_BOOST_REQ = 0x5A
    M2M2_PM_SYS_COMMAND_BOOST_RESP = 0x5B
    M2M2_PM_SYS_COMMAND_ADP5258_WRITE_RDAC_REQ = 0x5C
    M2M2_PM_SYS_COMMAND_ADP5258_WRITE_RDAC_RESP = 0x5D
    M2M2_PM_SYS_COMMAND_BLUETOOTH_REQ = 0x5E
    M2M2_PM_SYS_COMMAND_BLUETOOTH_RESP = 0x5F
    M2M2_PM_SYS_COMMAND_ENABLE_USER_CONFIG_LOG_REQ = 0x60
    M2M2_PM_SYS_COMMAND_ENABLE_USER_CONFIG_LOG_RESP = 0x61
    M2M2_PM_SYS_COMMAND_DISABLE_USER_CONFIG_LOG_REQ = 0x62
    M2M2_PM_SYS_COMMAND_DISABLE_USER_CONFIG_LOG_RESP = 0x63
    M2M2_PM_SYS_COMMAND_STORE_TO_EEPROM_REQ = 0x64
    M2M2_PM_SYS_COMMAND_STORE_TO_EEPROM_RESP = 0x65
    M2M2_PS_SYS_COMMAND_GET_LOW_TOUCH_LOGGING_STATUS_REQ = 0x66
    M2M2_PS_SYS_COMMAND_GET_LOW_TOUCH_LOGGING_STATUS_RESP = 0x67
    M2M2_PM_SYS_COMMAND_WRITE_EEPROM_REQ = 0x68
    M2M2_PM_SYS_COMMAND_WRITE_EEPROM_RESP = 0x69
    M2M2_PM_SYS_COMMAND_READ_EEPROM_REQ = 0x6A
    M2M2_PM_SYS_COMMAND_READ_EEPROM_RESP = 0x6B
    M2M2_PM_SYS_COMMAND_AD5110_WRITE_RDAC_REQ = 0x6C
    M2M2_PM_SYS_COMMAND_AD5110_WRITE_RDAC_RESP = 0x6D
    M2M2_PM_SYS_COMMAND_AD5110_READ_RDAC_REQ = 0x6E
    M2M2_PM_SYS_COMMAND_AD5110_READ_RDAC_RESP = 0x6F
    M2M2_PM_SYS_COMMAND_ADP5258_READ_RDAC_REQ = 0x70
    M2M2_PM_SYS_COMMAND_ADP5258_READ_RDAC_RESP = 0x71
    M2M2_PM_SYS_COMMAND_ADP5350_OVP_SETTING_REQ = 0x72
    M2M2_PM_SYS_COMMAND_ADP5350_OVP_SETTING_RESP = 0x73
    M2M2_PM_SYS_COMMAND_AD5110_Store_to_EEPROM_REQ = 0x74
    M2M2_PM_SYS_COMMAND_AD5110_Store_to_EEPROM_RESP = 0x75
    M2M2_PM_SYS_COMMAND_SYSTEM_RESET_REQ = 0x76
    M2M2_PM_SYS_COMMAND_SYSTEM_RESET_RESP = 0x77
    M2M2_PM_SYS_COMMAND_DG2502_SW_CNTRL_REQ = 0x78
    M2M2_PM_SYS_COMMAND_DG2502_SW_CNTRL_RESP = 0x79
    M2M2_PM_SYS_COMMAND_LDO_CNTRL_REQ = 0x7A
    M2M2_PM_SYS_COMMAND_LDO_CNTRL_RESP = 0x7B
    M2M2_PM_SYS_COMMAND_CHIP_ID_REQ = 0x7C
    M2M2_PM_SYS_COMMAND_CHIP_ID_RESP = 0x7D
    M2M2_PM_SYS_COMMAND_CAP_SENSE_TEST_REQ = 0x7E
    M2M2_PM_SYS_COMMAND_CAP_SENSE_TEST_RESP = 0x7F
    M2M2_PM_SYS_COMMAND_ENTER_BOOTLOADER_REQ = 0x80
    M2M2_PM_SYS_COMMAND_ENTER_BOOTLOADER_RESP = 0x81
    M2M2_PM_SYS_COMMAND_CAP_SENSE_STREAM_DATA = 0x82
    M2M2_PM_SYS_COMMAND_GET_APPS_INFO_REQ = 0x84
    M2M2_PM_SYS_COMMAND_GET_APPS_INFO_RESP = 0x85
    M2M2_PM_SYS_COMMAND_ACTIVATE_TOUCH_SENSOR_REQ = 0x86
    M2M2_PM_SYS_COMMAND_ACTIVATE_TOUCH_SENSOR_RESP = 0x87
    M2M2_PM_SYS_COMMAND_DEACTIVATE_TOUCH_SENSOR_REQ = 0x88
    M2M2_PM_SYS_COMMAND_DEACTIVATE_TOUCH_SENSOR_RESP = 0x89
    M2M2_PM_SYS_COMMAND_FLASH_RESET_REQ = 0x8A
    M2M2_PM_SYS_COMMAND_FLASH_RESET_RESP = 0x8B
    M2M2_PM_SYS_COMMAND_SYSTEM_HW_RESET_REQ = 0x8C
    M2M2_PM_SYS_COMMAND_SYSTEM_HW_RESET_RESP = 0x8D
    M2M2_PM_SYS_COMMAND_FORCE_STREAM_STOP_REQ = 0x8E
    M2M2_PM_SYS_COMMAND_FORCE_STREAM_STOP_RESP = 0x8F
    M2M2_PM_GET_APPS_HEALTH_REQ = 0x90
    M2M2_PM_GET_APPS_HEALTH_RESP = 0x91
    M2M2_PM_SYS_BLE_GET_MAX_TX_PKT_COMB_CNT_REQ = 0x92
    M2M2_PM_SYS_BLE_GET_MAX_TX_PKT_COMB_CNT_RESP = 0x93
    M2M2_PM_SYS_BLE_SET_MAX_TX_PKT_COMB_CNT_REQ = 0x94
    M2M2_PM_SYS_BLE_SET_MAX_TX_PKT_COMB_CNT_RESP = 0x95
    M2M2_PM_SYS_COMMAND_SET_MANUFACTURE_DATE_REQ = 0x96
    M2M2_PM_SYS_COMMAND_SET_MANUFACTURE_DATE_RESP = 0x97
    M2M2_PM_SYS_COMMAND_GET_MANUFACTURE_DATE_REQ = 0x98
    M2M2_PM_SYS_COMMAND_GET_MANUFACTURE_DATE_RESP = 0x99
    M2M2_PM_SYS_GET_HIBERNATE_MODE_STATUS_REQ = 0x9A
    M2M2_PM_SYS_GET_HIBERNATE_MODE_STATUS_RESP = 0x9B
    M2M2_PM_SYS_SET_HIBERNATE_MODE_STATUS_REQ = 0x9C
    M2M2_PM_SYS_SET_HIBERNATE_MODE_STATUS_RESP = 0x9D
    M2M2_PM_SYS_BATTERY_LEVEL_ALERT = 0x9E

class M2M2_PM_SYS_STATUS_ENUM_t(c_uint8):
    __M2M2_PM_SYS_STATUS_LOWEST = 0x40
    M2M2_PM_SYS_STATUS_OK = 0x41
    M2M2_PM_SYS_STATUS_ERR_ARGS = 0x42
    M2M2_PM_SYS_STATUS_LOW_TOUCH_LOGGING_ALREADY_STARTED = 0x43
    M2M2_PM_SYS_STATUS_CONFIG_FILE_NOT_FOUND = 0x44
    M2M2_PM_SYS_STATUS_CONFIG_FILE_READ_ERR = 0x45
    M2M2_PM_SYS_STATUS_ENABLE_USER_CONFIG_LOG_FAILED = 0x46
    M2M2_PM_SYS_STATUS_USER_CONFIG_LOG_ENABLED = 0x47
    M2M2_PM_SYS_STATUS_DISABLE_USER_CONFIG_LOG_FAILED = 0x48
    M2M2_PM_SYS_STATUS_USER_CONFIG_LOG_DISABLED = 0x49
    M2M2_PM_SYS_STATUS_LOG_STOPPED_THROUGH_BUTTON_A = 0x4A
    M2M2_PM_SYS_STATUS_LOW_TOUCH_LOGGING_IN_PROGRESS = 0x4B
    M2M2_PM_SYS_STATUS_LOW_TOUCH_LOGGING_NOT_STARTED = 0x4C
    M2M2_PM_SYS_STATUS_LOW_TOUCH_MAX_FILE_ERR = 0x4D
    M2M2_PM_SYS_STATUS_LOW_TOUCH_MEMORY_FULL_ERR = 0x4E
    M2M2_PM_SYS_ERR_RESET = 0x4F
#ifdef DCB
    M2M2_PM_SYS_STATUS_ENABLE_DCB_CONFIG_LOG_FAILED = 0x50
    M2M2_PM_SYS_STATUS_DCB_CONFIG_LOG_ENABLED = 0x51
    M2M2_PM_SYS_STATUS_DISABLE_DCB_CONFIG_LOG_FAILED = 0x52
    M2M2_PM_SYS_STATUS_DCB_CONFIG_LOG_DISABLED = 0x53
#endif    
    M2M2_PM_SYS_STATUS_BATTERY_LEVEL_LOW = 0x54
    M2M2_PM_SYS_STATUS_BATTERY_LEVEL_CRITICAL = 0x55
    M2M2_PM_SYS_STATUS_BATTERY_LEVEL_FULL = 0x56
    M2M2_PM_SYS_STATUS_ERR_NOT_CHKD = 0xFF

class M2M2_PM_SYS_BAT_STATE_ENUM_t(c_uint8):
    M2M2_PM_SYS_BAT_STATE_NOT_AVAIL = 0x0
    M2M2_PM_SYS_BAT_STATE_NOT_CHARGING = 0x1
    M2M2_PM_SYS_BAT_STATE_CHARGING = 0x2
    M2M2_PM_SYS_BAT_STATE_COMPLETE = 0x3
    M2M2_PM_SYS_BAT_STATE_CHARGE_LDO_MODE = 0x4
    M2M2_PM_SYS_BAT_STATE_CHARGE_TIMER_EXPIRED = 0x5
    M2M2_PM_SYS_BAT_STATE_DETECTION = 0x6
    M2M2_PM_SYS_BAT_STATE_CHARGE_ERR = 0x7

class M2M2_PM_SYS_USB_PWR_ENUM_t(c_uint8):
    M2M2_PM_SYS_USB_PWR_ENABLE = 0x0
    M2M2_PM_SYS_USB_PWR_DISABLE = 0x1

class M2M2_PM_SYS_THERMISTOR_STATE_ENUM_t(c_uint8):
    M2M2_PM_SYS_THERMISTOR_ENABLE = 0x0
    M2M2_PM_SYS_THERMISTOR_DISABLE = 0x1

class M2M2_PM_SYS_USB_PWR_ACTION_ENUM_t(c_uint8):
    M2M2_PM_SYS_USB_PWR_SET = 0x0
    M2M2_PM_SYS_USB_PWR_GET = 0x1

class M2M2_PM_SYS_PWR_STATE_ENUM_t(c_uint8):
    M2M2_PM_SYS_PWR_STATE_ACTIVE = 0x0
    #M2M2_PM_SYS_PWR_STATE_FLEXI = 0x1
    M2M2_PM_SYS_PWR_STATE_HIBERNATE = 0x2
    M2M2_PM_SYS_PWR_STATE_SHUTDOWN = 0x3

class M2M2_PM_SYS_MCU_TYPE_ENUM_t(c_uint8):
    M2M2_PM_SYS_MCU_INVALID = 0x0
    M2M2_PM_SYS_MCU_M3 = 0x1
    M2M2_PM_SYS_MCU_M4 = 0x2

class M2M2_PM_SYS_OVP_ENUM_t(c_uint8):
    M2M2_PM_SYS_OVP_18_5V = 0x0
    M2M2_PM_SYS_OVP_15V = 0x1
    M2M2_PM_SYS_OVP_10V = 0x2
    M2M2_PM_SYS_OVP_5_6V = 0x3

class M2M2_PM_SYS_BOOST_ENUM_t(c_uint8):
    M2M2_PM_SYS_BOOST_ENABLE = 0x0
    M2M2_PM_SYS_BOOST_DISABLE = 0x1

class M2M2_PM_SYS_DG2502_SELECT_ENUM_t(c_uint8):
    M2M2_PM_SYS_DG2502_8233_SW = 0x0
    M2M2_PM_SYS_DG2502_5940_SW = 0x1
    M2M2_PM_SYS_DG2502_4K_SW = 0x2

class M2M2_PM_SYS_DG2502_EN_ENUM_t(c_uint8):
    M2M2_PM_SYS_DG2502_ENABLE = 0x1
    M2M2_PM_SYS_DG2502_DISABLE = 0x0

class M2M2_PM_SYS_CHIP_ID_ENUM_t(c_uint8):
    M2M2_PM_SYS_ADXL362 = 0x1
    M2M2_PM_SYS_ADPD4K = 0x2
    M2M2_PM_SYS_ADP5360 = 0x3
    M2M2_PM_SYS_AD5940 = 0x4
    M2M2_PM_SYS_NAND_FLASH = 0x5
    M2M2_PM_SYS_AD7156 = 0x6

class ADI_PM_BOARD_TYPE_t(c_uint8):
    ADI_PM_BOARD_TYPE_UNKNOWN = 0x0
    ADI_PM_BOARD_TYPE_ADPD107_WATCH = 0x1
    ADI_PM_BOARD_TYPE_ADPD107_CHEST_STRAP = 0x2
    ADI_PM_BOARD_TYPE_ADPD185_WATCH = 0x3
    ADI_PM_BOARD_TYPE_ADPD188_WATCH = 0x4

class m2m2_pm_sys_cmd_t(Structure):
    fields = [
              (None, common_application_interface._m2m2_app_common_cmd_t),
              ]

class m2m2_file_reset_cmd_t(Structure):
    fields = [
              (None, common_application_interface._m2m2_app_common_cmd_t),
              ]

class m2m2_pm_sys_pwr_state_t(Structure):
    fields = [
              (None, common_application_interface._m2m2_app_common_cmd_t),
              ("state", c_uint8),
              ]

class m2m2_pm_sys_info_t(Structure):
    fields = [
              (None, common_application_interface._m2m2_app_common_cmd_t),
              ("version", c_uint16),
              ("mac_addr", c_uint8 * 6),
              ("device_id", c_uint32),
              ("model_number", c_uint32),
              ("hw_id", c_uint16),
              ("bom_id", c_uint16),
              ("batch_id", c_uint8),
              ("date", c_uint32),
              ("board_type", ADI_PM_BOARD_TYPE_t),
              ]

class m2m2_pm_sys_date_time_req_t(Structure):
    fields = [
              (None, common_application_interface._m2m2_app_common_cmd_t),
              ("year", c_uint16),
              ("month", c_uint8),
              ("day", c_uint8),
              ("hour", c_uint8),
              ("minute", c_uint8),
              ("second", c_uint8),
              ("TZ_sec", c_uint32),
              ]

class m2m2_manufacture_date_t(Structure):
    fields = [
              (None, common_application_interface._m2m2_app_common_cmd_t),
              ("year", c_uint16),
              ("month", c_uint8),
              ("day", c_uint8),
              ]

class m2m2_pm_sys_bat_info_resp_t(Structure):
    fields = [
              (None, common_application_interface._m2m2_app_common_cmd_t),
              ("timestamp", c_uint32),
              ("bat_chrg_stat", M2M2_PM_SYS_BAT_STATE_ENUM_t),
              ("bat_lvl", c_uint8),
              ("bat_mv", c_uint16),
              #("bat_temp", c_uint16),
              ]

class m2m2_pm_sys_bat_thr_req_t(Structure):
    fields = [
              (None, common_application_interface._m2m2_app_common_cmd_t),
              ("bat_level_low", c_uint8),
              ("bat_level_critical", c_uint8),
              ]

class m2m2_bat_crit_info_resp_t(Structure):
    fields = [
              (None, common_application_interface._m2m2_app_common_cmd_t),
              ("bat_mv", c_uint16),
              ]

class m2m2_pm_sys_usb_pwr_cmd_t(Structure):
    fields = [
              (None, common_application_interface._m2m2_app_common_cmd_t),
              ("enable", M2M2_PM_SYS_USB_PWR_ENUM_t),
              ("action", M2M2_PM_SYS_USB_PWR_ACTION_ENUM_t),
              ]

class m2m2_pm_sys_bat_thermistor_cmd_t(Structure):
    fields = [
              (None, common_application_interface._m2m2_app_common_cmd_t),
              ("enable", M2M2_PM_SYS_THERMISTOR_STATE_ENUM_t),
              ]

class m2m2_pm_sys_mcu_version_t(Structure):
    fields = [
              (None, common_application_interface._m2m2_app_common_cmd_t),
              ("mcu", M2M2_PM_SYS_MCU_TYPE_ENUM_t),
              ]

class m2m2_pm_sys_boost_cmd_t(Structure):
    fields = [
              (None, common_application_interface._m2m2_app_common_cmd_t),
              ("enable", M2M2_PM_SYS_BOOST_ENUM_t),
              ("ovp", M2M2_PM_SYS_OVP_ENUM_t),
              ]

class m2m2_pm_sys_powerboost_cmd_t(Structure):
    fields = [
              (None, common_application_interface._m2m2_app_common_cmd_t),
              ("enable", M2M2_PM_SYS_BOOST_ENUM_t),
              ("ovp", c_uint8),
              ]

class m2m2_pm_sys_eeprom_cmd_t(Structure):
    fields = [
              (None, common_application_interface._m2m2_app_common_cmd_t),
              ("size", c_uint8),
              ("byte_stream", c_uint8 * 16),
              ]

class m2m2_pm_sys_dg2502_sw_ctrl_cmd_t(Structure):
    fields = [
              (None, common_application_interface._m2m2_app_common_cmd_t),
              ("sw_name", M2M2_PM_SYS_DG2502_SELECT_ENUM_t),
              ("sw_enable", M2M2_PM_SYS_DG2502_EN_ENUM_t),
              ]

class m2m2_pm_sys_ldo_ctrl_cmd_t(Structure):
    fields = [
              (None, common_application_interface._m2m2_app_common_cmd_t),
              ("ldo_num", c_uint8),
              ("ldo_enable", c_uint8),
              ]

class m2m2_pm_sys_chip_id_cmd_t(Structure):
    fields = [
              (None, common_application_interface._m2m2_app_common_cmd_t),
              ("chip_name", M2M2_PM_SYS_CHIP_ID_ENUM_t),
              ("chip_id", c_uint16),
              ]

class m2m2_pm_sys_cap_sense_test_cmd_t(Structure):
    fields = [
              (None, common_application_interface._m2m2_app_common_cmd_t),
              ("enable", c_uint8),
              ]

class m2m2_pm_sys_cap_sense_test_data_t(Structure):
    fields = [
              (None, common_application_interface._m2m2_app_common_cmd_t),
              ("touch_position", c_uint8),
              ("touch_value", c_uint8),
              ]

class m2m2_pm_sys_enter_bloader_cmd_t(Structure):
    fields = [
              (None, common_application_interface._m2m2_app_common_cmd_t),
              ]

class m2m2_pm_sys_sensor_app_status(Structure):
    fields = [
              ("sensor_app_id", m2m2_core.M2M2_ADDR_ENUM_t),
              ("sensor_stream", m2m2_core.M2M2_ADDR_ENUM_t),
              ("num_subscribers", c_uint8),
              ("num_start_reqs", c_uint8),
              ("fs_sub_stat", c_uint16),
              ]

class m2m2_pm_sys_sensor_apps_info_req_t(Structure):
    fields = [
              (None, common_application_interface._m2m2_app_common_cmd_t),
              ("num_sensor_apps", c_uint16),
              ("app_info", m2m2_pm_sys_sensor_app_status * PM_APPS_COUNT),
              ]

class m2m2_pm_force_stream_stop_cmd_t(Structure):
    fields = [
              (None, common_application_interface._m2m2_app_common_cmd_t),
    ]

class m2m2_get_apps_running_stat_req_cmd_t(Structure):
    fields = [
              (None, common_application_interface._m2m2_app_common_cmd_t),
              ]

class m2m2_get_apps_running_stat_resp_cmd_t(Structure):
    fields = [
              (None, common_application_interface._m2m2_app_common_cmd_t),
                ("ad5940_isr_cnt", c_uint32),
                ("adpd4000_isr_cnt", c_uint32),
                ("adxl_isr_cnt", c_uint32),
              ]

class m2m2_ble_max_tx_pkt_comb_cnt_resp_cmd_t(Structure):
    fields = [
              (None, common_application_interface._m2m2_app_common_cmd_t),
                ("max_tx_pkt_comb_cnt", c_uint8),
              ]

class m2m2_hibernate_mode_status_resp_cmd_t(Structure):
    fields = [
              (None, common_application_interface._m2m2_app_common_cmd_t),
                ("hib_mode_status", c_uint8),
              ]
