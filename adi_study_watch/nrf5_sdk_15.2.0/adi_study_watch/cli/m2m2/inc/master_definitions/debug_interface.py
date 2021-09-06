#!/usr/bin/env python3

from ctypes import *

import m2m2_core

class m2m2_debug_data_t(Structure):
    fields = [
              ("str", c_uint8 * 127),
              ]
