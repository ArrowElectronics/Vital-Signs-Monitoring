from ctypes import *

from common_application_interface_def import *

from common_sensor_interface_def import *

from m2m2_core_def import *


class M2M2_SENSOR_AD7156_COMMAND_ENUM_t(c_ubyte):
    _M2M2_SENSOR_AD7156_COMMAND_LOWEST = 0x40
    M2M2_SENSOR_AD7156_COMMAND_LOAD_CFG_REQ = 0x42
    M2M2_SENSOR_AD7156_COMMAND_LOAD_CFG_RESP = 0x43

class m2m2_sensor_ad7156_resp_t(Structure):
    _pack_ = 1
    _fields_ = [
              ("command", c_ubyte),
              ("status", c_ubyte),
              ]

class m2m2_sensor_ad7156_data_t(Structure):
    _pack_ = 1
    _fields_ = [
              ("command", c_ubyte),
              ("status", c_ubyte),
              ("sequence_num", c_ushort),
              ("timestamp", c_ulong),
              ("ch1_cap", c_ushort),
              ("ch2_cap", c_ushort),
              ]

