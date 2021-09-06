from ctypes import *

from common_application_interface_def import *

from common_sensor_interface_def import *

from m2m2_core_def import *


class M2M2_ECG_APP_CMD_ENUM_t(c_ubyte):
    _M2M2_ECG_APP_CMD_LOWEST = 0x5D
    M2M2_ECG_APP_CMD_GET_ALGO_VENDOR_VERSION_REQ = 0x5E
    M2M2_ECG_APP_CMD_GET_ALGO_VENDOR_VERSION_RESP = 0x5F

class M2M2_SENSOR_ECG_NSAMPLES_ENUM_t(c_ubyte):
    M2M2_SENSOR_ECG_NSAMPLES = 0xB

class M2M2_SENSOR_ECG_RAW_DATA_TYPES_ENUM_t(c_ubyte):
    M2M2_SENSOR_ECG_MONITOR = 0x0
    M2M2_SENSOR_ECG_SPORT = 0x1

class M2M2_SENSOR_ECG_APP_INFO_BITSET_ENUM_t(c_ubyte):
    M2M2_SENSOR_ECG_APP_INFO_BITSET_LEADSOFF = 0x0
    M2M2_SENSOR_ECG_APP_INFO_BITSET_LEADSON = 0x1

class ecg_app_lib_state_t(Structure):
    _pack_ = 1
    _fields_ = [
              ("command", c_ubyte),
              ("status", c_ubyte),
              ("states", c_ubyte * 10),
              ]

class ecg_app_lcfg_op_t(Structure):
    _pack_ = 1
    _fields_ = [
              ("field", c_ubyte),
              ("value", c_ushort),
              ]

def ecg_app_lcfg_op_hdr_t(array_size):
  class ecg_app_lcfg_op_hdr_t_internal(Structure):
    _pack_ = 1
    _fields_ = [
              ("command", c_ubyte),
              ("status", c_ubyte),
              ("num_ops", c_ubyte),
              ("ops", ecg_app_lcfg_op_t * array_size),
              ]
  return ecg_app_lcfg_op_hdr_t_internal()

class ecg_data_set_t(Structure):
    _pack_ = 1
    _fields_ = [
              ("timestamp", c_ushort),
              ("ecgdata", c_ushort),
              ]

class ecg_app_stream_t(Structure):
    _pack_ = 1
    _fields_ = [
              ("command", c_ubyte),
              ("status", c_ubyte),
              ("sequence_num", c_ushort),
              ("datatype", c_ubyte),
              ("timestamp", c_ulong),
              ("ecg_info", c_ubyte),
              ("HR", c_ubyte),
              ("firstecgdata", c_ushort),
              ("ecg_data", ecg_data_set_t * 10),
              ]

class ecg_app_sync_t(Structure):
    _pack_ = 1
    _fields_ = [
              ("command", c_ubyte),
              ("status", c_ubyte),
              ("sync", c_ubyte),
              ]

class ecg_app_dcb_lcfg_t(Structure):
    _pack_ = 1
    _fields_ = [
              ("command", c_ubyte),
              ("status", c_ubyte),
              ]

