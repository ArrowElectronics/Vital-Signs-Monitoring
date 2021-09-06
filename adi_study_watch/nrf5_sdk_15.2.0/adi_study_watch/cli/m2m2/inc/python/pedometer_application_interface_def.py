from ctypes import *

from common_application_interface_def import *

from common_sensor_interface_def import *

from m2m2_core_def import *


class M2M2_PED_APP_CMD_ENUM_t(c_ubyte):
    _M2M2_PED_APP_CMD_LOWEST = 0x5A
    M2M2_PED_APP_CMD_GET_ALGO_VENDOR_VERSION_REQ = 0x5C
    M2M2_PED_APP_CMD_GET_ALGO_VENDOR_VERSION_RESP = 0x5D

class ped_app_lcfg_op_t(Structure):
    _pack_ = 1
    _fields_ = [
              ("field", c_ubyte),
              ("value", c_ushort),
              ]

def ped_app_lcfg_op_hdr_t(array_size):
  class ped_app_lcfg_op_hdr_t_internal(Structure):
    _pack_ = 1
    _fields_ = [
              ("command", c_ubyte),
              ("status", c_ubyte),
              ("num_ops", c_ubyte),
              ("ops", ped_app_lcfg_op_t * array_size),
              ]
  return ped_app_lcfg_op_hdr_t_internal()

class pedometer_app_stream_t(Structure):
    _pack_ = 1
    _fields_ = [
              ("command", c_ubyte),
              ("status", c_ubyte),
              ("sequence_num", c_ushort),
              ("nNumSteps", c_long),
              ("nAlgoStatus", c_ushort),
              ("nTimeStamp", c_ulong),
              ("nReserved", c_byte),
              ]

