from ctypes import *

from common_application_interface_def import *

from common_sensor_interface_def import *

from dcb_interface_def import *

from m2m2_core_def import *


class M2M2_PPG_APP_CMD_ENUM_t(c_ubyte):
    _M2M2_PPG_APP_CMD_LOWEST = 0x40
    M2M2_PPG_APP_CMD_GET_LAST_STATES_REQ = 0x42
    M2M2_PPG_APP_CMD_GET_LAST_STATES_RESP = 0x43
    M2M2_PPG_APP_CMD_GET_STATES_INFO_REQ = 0x44
    M2M2_PPG_APP_CMD_GET_STATES_INFO_RESP = 0x45
    M2M2_PPG_APP_CMD_GET_ALGO_VENDOR_VERSION_REQ = 0x46
    M2M2_PPG_APP_CMD_GET_ALGO_VENDOR_VERSION_RESP = 0x47
    M2M2_PPG_APP_CMD_SYNC_DATA_REQ = 0x48
    M2M2_PPG_APP_CMD_SYNC_DATA_RESP = 0x49
    M2M2_PPG_APP_CMD_DEBUG_DATA_REQ = 0x4A
    M2M2_PPG_APP_CMD_DEBUG_DATA_RESP = 0x4B
    M2M2_PPG_APP_CMD_GET_CTRVALUE_REQ = 0x4C
    M2M2_PPG_APP_CMD_GET_CTRVALUE_RESP = 0x4D
    M2M2_PPG_APP_CMD_GET_SMETRICS_REQ = 0x4E
    M2M2_PPG_APP_CMD_GET_SMETRICS_RESP = 0x4F

class M2M2_PPG_APP_SYNC_ENUM_t(c_ubyte):
    M2M2_PPG_APP_SOFTWARE_SYNC = 0x0
    M2M2_PPG_APP_HARDWARE_SYNC = 0x3

class M2M2_SENSOR_PPG_SYNC_NSAMPLES_ENUM_t(c_ubyte):
    M2M2_SENSOR_PPG_SYNC_NSAMPLES = 0x3

class M2M2_SENSOR_HRV_NSAMPLES_ENUM_t(c_ubyte):
    M2M2_SENSOR_HRV_NSAMPLES = 0x4

class M2M2_SENSOR_PPG_SYNC_DATA_TYPES_ENUM_t(c_ubyte):
    M2M2_SENSOR_PPG_SYNC_DATA_TYPES_NO_SYNC = 0x0
    M2M2_SENSOR_PPG_SYNC_DATA_TYPES_HW_SYNC1 = 0x1
    M2M2_SENSOR_PPG_SYNC_DATA_TYPES_HW_SYNC2 = 0x2
    M2M2_SENSOR_PPG_SYNC_DATA_TYPES_HW_SYNC3 = 0x3
    M2M2_SENSOR_PPG_SYNC_DATA_TYPES_SW_SYNC = 0x4

class M2M2_SENSOR_PPG_LCFG_ID_ENUM_t(c_ubyte):
    M2M2_SENSOR_PPG_LCFG_ID_ADPD4000 = 0x28
    M2M2_SENSOR_PPG_LCFG_ID_ADPD107 = 0x6B
    M2M2_SENSOR_PPG_LCFG_ID_ADPD108 = 0x6C
    M2M2_SENSOR_PPG_LCFG_ID_ADPD185 = 0xB9
    M2M2_SENSOR_PPG_LCFG_ID_ADPD188 = 0xBC

class ppg_app_lib_state_t(Structure):
    _pack_ = 1
    _fields_ = [
              ("command", c_ubyte),
              ("status", c_ubyte),
              ("states", c_ubyte * 10),
              ]

class ppg_app_ctrValue_t(Structure):
    _pack_ = 1
    _fields_ = [
              ("command", c_ubyte),
              ("status", c_ubyte),
              ("ctrValue", c_ushort),
              ]

class ppg_app_signal_metrics_t(Structure):
    _pack_ = 1
    _fields_ = [
              ("command", c_ubyte),
              ("status", c_ubyte),
              ("metrics", c_ushort * 3),
              ]

class ppg_app_lcfg_op_t(Structure):
    _pack_ = 1
    _fields_ = [
              ("field", c_ubyte),
              ("value", c_ulong),
              ]

def ppg_app_lcfg_op_hdr_t(array_size):
  class ppg_app_lcfg_op_hdr_t_internal(Structure):
    _pack_ = 1
    _fields_ = [
              ("command", c_ubyte),
              ("status", c_ubyte),
              ("num_ops", c_ubyte),
              ("ops", ppg_app_lcfg_op_t * array_size),
              ]
  return ppg_app_lcfg_op_hdr_t_internal()

class ppg_app_hr_debug_stream_t(Structure):
    _pack_ = 1
    _fields_ = [
              ("command", c_ubyte),
              ("status", c_ubyte),
              ("sequence_num", c_ushort),
              ("timestamp", c_ulong),
              ("adpdlibstate", c_ushort),
              ("hr", c_ushort),
              ("confidence", c_ushort),
              ("hr_type", c_ushort),
              ("rr_interval", c_ushort),
              ("debugInfo", c_ushort * 10),
              ]

class ppg_app_state_info_t(Structure):
    _pack_ = 1
    _fields_ = [
              ("command", c_ubyte),
              ("status", c_ubyte),
              ("state", c_ubyte),
              ("info", c_ubyte * 20),
              ]

class ppg_app_cmd_t(Structure):
    _pack_ = 1
    _fields_ = [
              ("command", c_ubyte),
              ("status", c_ubyte),
              ("commandVal", c_ushort),
              ]

class ppg_app_set_lcfg_req_t(Structure):
    _pack_ = 1
    _fields_ = [
              ("command", c_ubyte),
              ("status", c_ubyte),
              ("lcfgid", c_ubyte),
              ]

class ppg_app_set_lcfg_resp_t(Structure):
    _pack_ = 1
    _fields_ = [
              ("command", c_ubyte),
              ("status", c_ubyte),
              ]

class ppg_app_agc_info_t(Structure):
    _pack_ = 1
    _fields_ = [
              ("command", c_ubyte),
              ("status", c_ubyte),
              ("sequence_num", c_ushort),
              ("timestamp", c_ulong),
              ("mts", c_ushort * 6),
              ("setting", c_ushort * 10),
              ]

class hrv_data_set_t(Structure):
    _pack_ = 1
    _fields_ = [
              ("timestamp", c_ushort),
              ("rr_interval", c_short),
              ("is_gap", c_ushort),
              ("rmssd", c_ushort),
              ]

class ppg_app_hrv_info_t(Structure):
    _pack_ = 1
    _fields_ = [
              ("command", c_ubyte),
              ("status", c_ubyte),
              ("sequence_num", c_ushort),
              ("timestamp", c_ulong),
              ("first_rr_interval", c_short),
              ("first_is_gap", c_ushort),
              ("first_rmssd", c_ushort),
              ("hrv_data", hrv_data_set_t * 3),
              ]

class m2m2_ppg_lcfg_data_t(Structure):
    _pack_ = 1
    _fields_ = [
              ("command", c_ubyte),
              ("status", c_ubyte),
              ("size", c_ubyte),
              ("lcfgdata", c_long * 56),
              ]

