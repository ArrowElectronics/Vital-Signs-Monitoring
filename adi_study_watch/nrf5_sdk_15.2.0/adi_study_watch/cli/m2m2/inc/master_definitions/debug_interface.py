#!/usr/bin/env python3

from ctypes import *

import m2m2_core
import common_application_interface

class m2m2_debug_data_t(Structure):
    fields = [
              ("str", c_uint8 * 127),
              ]
class m2m2_app_debug_stream_t(Structure):
    fields = [
              (None, common_application_interface._m2m2_app_common_cmd_t),
              ("sequence_num", c_uint16),
              ("timestamp" ,c_uint32),
              ("debuginfo_64", c_uint64 * 4),
              ("debuginfo", c_uint32 * 4),
  			  ]    
