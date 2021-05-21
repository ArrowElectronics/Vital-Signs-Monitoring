from ctypes import *

from common_application_interface_def import *

from m2m2_core_def import *

class M2M2_TEMPERATURE_APP_CMD_ENUM_t(c_ubyte):
    _M2M2_TEMPERATURE_APP_CMD_LOWEST = 0x60
    M2M2_TEMPERATURE_APP_CMD_SET_FS_REQ = 0x62
    M2M2_TEMPERATURE_APP_CMD_SET_FS_RESP = 0x63

class temperature_app_stream_t(Structure):
    _pack_ = 1
    _fields_ = [
              ("command", c_ubyte),
              ("status", c_ubyte),
              ("sequence_num", c_ushort),
              ("nTS", c_ulong),
              ("nTemperature1", c_ushort),
              ("nTemperature2", c_ushort),
              ]
