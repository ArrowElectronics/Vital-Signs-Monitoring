from ctypes import *

from common_application_interface_def import *

from m2m2_core_def import *


class M2M2_USER0_CONFIG_APP_COMMAND_ENUM_t(c_ubyte):
    _M2M2_USER0_CONFIG_APP_COMMAND_LOWEST = 0x40
    M2M2_USER0_CONFIG_APP_COMMAND_SET_STATE_REQ = 0x42
    M2M2_USER0_CONFIG_APP_COMMAND_SET_STATE_RESP = 0x43
    M2M2_USER0_CONFIG_APP_COMMAND_GET_STATE_REQ = 0x44
    M2M2_USER0_CONFIG_APP_COMMAND_GET_STATE_RESP = 0x45
    M2M2_USER0_CONFIG_APP_COMMAND_ID_OP_REQ = 0x46
    M2M2_USER0_CONFIG_APP_COMMAND_ID_OP_RESP = 0x47
    M2M2_USER0_CONFIG_APP_CLEAR_PREV_ST_EVT_REQ = 0x48
    M2M2_USER0_CONFIG_APP_CLEAR_PREV_ST_EVT_RESP = 0x49
    M2M2_USER0_CONFIG_APP_GET_PREV_ST_EVT_REQ = 0x4A
    M2M2_USER0_CONFIG_APP_GET_PREV_ST_EVT_RESP = 0x4B
    M2M2_USER0_CONFIG_APP_BYPASS_USER0_TIMINGS_REQ = 0x4C
    M2M2_USER0_CONFIG_APP_BYPASS_USER0_TIMINGS_RESP = 0x4D

class M2M2_USER0_CONFIG_APP_STATUS_ENUM_t(c_ubyte):
    _M2M2_USER0_CONFIG_APP_STATUS_LOWEST = 0x40
    M2M2_USER0_CONFIG_APP_STATUS_OK = 0x41
    M2M2_USER0_CONFIG_APP_STATUS_ERR_ARGS = 0x42
    M2M2_USER0_CONFIG_APP_STATUS_ERR_NOT_CHKD = 0xFF

class USER0_CONFIG_APP_STATE_t(c_ubyte):
    STATE_ADMIT_STANDBY = 0x0
    STATE_START_MONITORING = 0x1
    STATE_SLEEP = 0x2
    STATE_INTERMITTENT_MONITORING = 0x3
    STATE_INTERMITTENT_MONITORING_START_LOG = 0x4
    STATE_INTERMITTENT_MONITORING_STOP_LOG = 0x5
    STATE_END_MONITORING = 0x6
    STATE_CHARGING_BATTERY = 0x7
    STATE_OUT_OF_BATTERY_STATE_BEFORE_START_MONITORING = 0x8
    STATE_OUT_OF_BATTERY_STATE_DURING_INTERMITTENT_MONITORING = 0x9

class USER0_CONFIG_APP_EVENT_t(c_ubyte):
    EVENT_INVALID = 0x0
    EVENT_NAV_BUTTON_RESET = 0x1
    EVENT_WATCH_ON_CRADLE_NAV_BUTTON_RESET = 0x2
    EVENT_BATTERY_DRAINED = 0x3
    EVENT_BLE_DISCONNECT_UNEXPECTED = 0x4
    EVENT_BLE_DISCONNECT_NW_TERMINATED = 0x5
    EVENT_WATCH_OFF_CRADLE_BLE_DISCONNECT_NW_TERMINATED = 0x6
    EVENT_RTC_TIMER_INTERRUPT = 0x7
    EVENT_BLE_ADV_TIMEOUT = 0x8
    EVENT_USB_DISCONNECT_UNEXPECTED = 0x9
    EVENT_BATTERY_FULL = 0xA
    EVENT_FINISH_LOG_TRANSFER = 0xB
    EVENT_WATCH_OFF_CRADLE_BLE_CONNECTION = 0xC

class ID_SELECTION_ENUM_t(c_ubyte):
    ID_HW_ID = 0x0
    ID_EXP_ID = 0x1

class ID_OPERATION_MODE_ENUM_t(c_ubyte):
    ID_OPERATION_MODE_READ = 0x0
    ID_OPERATION_MODE_WRITE = 0x1
    ID_OPERATION_MODE_DELETE = 0x2

class m2m2_user0_config_app_cmd_t(Structure):
    _pack_ = 1
    _fields_ = [
              ("command", c_ubyte),
              ("status", c_ubyte),
              ]

class user0_config_app_lcfg_op_t(Structure):
    _pack_ = 1
    _fields_ = [
              ("field", c_ubyte),
              ("value", c_ulong),
              ]

def user0_config_app_lcfg_op_hdr_t(array_size):
  class user0_config_app_lcfg_op_hdr_t_internal(Structure):
    _pack_ = 1
    _fields_ = [
              ("command", c_ubyte),
              ("status", c_ubyte),
              ("num_ops", c_ubyte),
              ("ops", user0_config_app_lcfg_op_t * array_size),
              ]
  return user0_config_app_lcfg_op_hdr_t_internal()

class user0_config_app_set_state_t(Structure):
    _pack_ = 1
    _fields_ = [
              ("command", c_ubyte),
              ("status", c_ubyte),
              ("state", c_ubyte),
              ]

class user0_config_app_id_t(Structure):
    _pack_ = 1
    _fields_ = [
              ("command", c_ubyte),
              ("status", c_ubyte),
              ("id_sel", c_ubyte),
              ("id_op", c_ubyte),
              ("id_num", c_ushort),
              ]

class user0_app_prev_state_event_t(Structure):
    _pack_ = 1
    _fields_ = [
              ("prev_state", c_ubyte),
              ("prev_event", c_ubyte),
              ("prev_timestamp", c_ulong),
              ]

class user0_app_prev_state_event_pkt_t(Structure):
    _pack_ = 1
    _fields_ = [
              ("command", c_ubyte),
              ("status", c_ubyte),
              ("prev_st_evt", user0_app_prev_state_event_t * 4),
              ("intermittent_op_cnt", c_ushort),
              ]

class user0_app_bypass_user0_timings_pkt_t(Structure):
    _pack_ = 1
    _fields_ = [
              ("command", c_ubyte),
              ("status", c_ubyte),
              ("enable", c_ubyte),
              ]

