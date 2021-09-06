#!/usr/bin/env python3

from ctypes import *

import m2m2_core

import common_application_interface

class M2M2_WDT_CMD_ENUM_t(c_uint8):
    M2M2_WDT_WDT_CMD_LOWEST = 0xC0
    M2M2_WDT_WDT_TASK_BROADCAST_CMD = 0xC1
    M2M2_WDT_PM_IS_ALIVE_CMD = 0xC2
    M2M2_WDT_PS_IS_ALIVE_CMD = 0xC3

class M2M2_WDT_STATUS_ENUM_t(c_uint8):
    M2M2_WDT_STATUS_OK = 0x0
    M2M2_WDT_STATUS_ERROR = 0x1

class m2m2_wdt_cmd_t(Structure):
    fields = [
              (None, common_application_interface._m2m2_app_common_cmd_t)
              ]