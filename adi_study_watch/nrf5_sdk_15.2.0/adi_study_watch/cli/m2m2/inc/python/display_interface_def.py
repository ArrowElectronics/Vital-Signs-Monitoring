from ctypes import *

from common_application_interface_def import *

from m2m2_core_def import *


class M2M2_DISPLAY_APP_CMD_ENUM_t(c_ubyte):
    _M2M2_DISPLAY_APP_CMD_LOWEST = 0x40
    M2M2_DISPLAY_APP_CMD_SET_DISPLAY_REQ = 0x42
    M2M2_DISPLAY_APP_CMD_SET_DISPLAY_RESP = 0x43
    M2M2_DISPLAY_APP_CMD_BACKLIGHT_CNTRL_REQ = 0x44
    M2M2_DISPLAY_APP_CMD_BACKLIGHT_CNTRL_RESP = 0x45
    M2M2_DISPLAY_APP_CMD_KEY_TEST_REQ = 0x46
    M2M2_DISPLAY_APP_CMD_KEY_TEST_RESP = 0x47
    M2M2_DISPLAY_APP_CMD_KEY_STREAM_DATA = 0x48

class M2M2_DISPLAY_SET_COMMAND_ENUM_t(c_ubyte):
    M2M2_DISPLAY_SET_WHITE = 0x0
    M2M2_DISPLAY_SET_BLACK = 0x1
    M2M2_DISPLAY_SET_RED = 0x2
    M2M2_DISPLAY_SET_GREEN = 0x3
    M2M2_DISPLAY_SET_BLUE = 0x4

class M2M2_BACKLIGHT_CNTRL_COMMAND_ENUM_t(c_ubyte):
    M2M2_BACKLIGHT_CNTRL_OFF = 0x0
    M2M2_BACKLIGHT_CNTRL_ON = 0x1

class M2M2_KEY_TEST_COMMAND_ENUM_t(c_ubyte):
    M2M2_KEY_TEST_SELECT_BUTTON = 0x0
    M2M2_KEY_TEST_NAVIGATE_BUTTON = 0x1

class m2m2_display_set_command_t(Structure):
    _pack_ = 1
    _fields_ = [
              ("command", c_ubyte),
              ("status", c_ubyte),
              ("colour", c_ubyte),
              ]

class m2m2_backlight_cntrl_command_t(Structure):
    _pack_ = 1
    _fields_ = [
              ("command", c_ubyte),
              ("status", c_ubyte),
              ("control", c_ubyte),
              ]

class m2m2_key_test_command_t(Structure):
    _pack_ = 1
    _fields_ = [
              ("command", c_ubyte),
              ("status", c_ubyte),
              ("enable", c_ubyte),
              ]

class m2m2_pm_sys_key_test_data_t(Structure):
    _pack_ = 1
    _fields_ = [
              ("command", c_ubyte),
              ("status", c_ubyte),
              ("key_value", c_ubyte),
              ]

