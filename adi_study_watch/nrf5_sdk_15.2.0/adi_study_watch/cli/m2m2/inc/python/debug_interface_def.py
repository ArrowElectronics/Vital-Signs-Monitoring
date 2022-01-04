from ctypes import *

from common_application_interface_def import *

from m2m2_core_def import *


class m2m2_debug_data_t(Structure):
    _pack_ = 1
    _fields_ = [
              ("str", c_ubyte * 127),
              ]

class m2m2_app_debug_stream_t(Structure):
    _pack_ = 1
    _fields_ = [
              ("command", c_ubyte),
              ("status", c_ubyte),
              ("sequence_num", c_ushort),
              ("timestamp", c_ulong),
              ("debuginfo", c_ulong * 12),
              ]

