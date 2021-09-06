from ctypes import *

from common_application_interface_def import *

from m2m2_core_def import *


class M2M2_LT_COMMAND_ENUM_t(c_ubyte):
    _M2M2_LT_COMMAND_ENUM_t__M2M2_LT_COMMAND_LOWEST = 0x40
    M2M2_LT_COMMAND_ACTIVATE_TOUCH_SENSOR_REQ = 0x42
    M2M2_LT_COMMAND_ACTIVATE_TOUCH_SENSOR_RESP = 0x43
    M2M2_LT_COMMAND_DEACTIVATE_TOUCH_SENSOR_REQ = 0x44
    M2M2_LT_COMMAND_DEACTIVATE_TOUCH_SENSOR_RESP = 0x45
    M2M2_LT_COMMAND_RD_CH2_CAP_REQ = 0x46
    M2M2_LT_COMMAND_RD_CH2_CAP_RESP = 0x47

class M2M2_LT_STATUS_ENUM_t(c_ubyte):
    _M2M2_LT_STATUS_ENUM_t__M2M2_LT_STATUS_LOWEST = 0x40
    M2M2_LT_STATUS_OK = 0x41
    M2M2_LT_STATUS_ERR_ARGS = 0x42
    M2M2_LT_STATUS_ERR_NOT_CHKD = 0xFF

class m2m2_lt_cmd_t(Structure):
    _pack_ = 1
    _fields_ = [
              ("command", c_ubyte),
              ("status", c_ubyte),
              ]

class lt_app_lcfg_op_t(Structure):
    _pack_ = 1
    _fields_ = [
              ("field", c_ubyte),
              ("value", c_ushort),
              ]

def lt_app_lcfg_op_hdr_t(array_size):
  class lt_app_lcfg_op_hdr_t_internal(Structure):
    _pack_ = 1
    _fields_ = [
              ("command", c_ubyte),
              ("status", c_ubyte),
              ("num_ops", c_ubyte),
              ("ops", lt_app_lcfg_op_t * array_size),
              ]
  return lt_app_lcfg_op_hdr_t_internal()

class lt_app_rd_ch2_cap(Structure):
    _pack_ = 1
    _fields_ = [
              ("command", c_ubyte),
              ("status", c_ubyte),
              ("capVal", c_ushort),
              ]

