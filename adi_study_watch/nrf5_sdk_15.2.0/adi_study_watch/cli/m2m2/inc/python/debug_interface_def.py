from ctypes import *

from m2m2_core_def import *


class m2m2_debug_data_t(Structure):
    _pack_ = 1
    _fields_ = [
              ("str", c_ubyte * 127),
              ]

