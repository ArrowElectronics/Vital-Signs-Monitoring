from ctypes import *

from m2m2_core_def import *

class M2M2_LED_COMMAND_ENUM_t(c_ubyte):
    M2M2_LED_COMMAND_GET = 0x0
    M2M2_LED_COMMAND_SET = 0x1

class M2M2_LED_PATTERN_ENUM_t(c_ubyte):
    M2M2_LED_PATTERN_OFF = 0x0
    M2M2_LED_PATTERN_SLOW_BLINK_DC_12 = 0x1
    M2M2_LED_PATTERN_SLOW_BLINK_DC_12_N = 0xFE
    M2M2_LED_PATTERN_FAST_BLINK_DC_50 = 0xAA
    M2M2_LED_PATTERN_FAST_BLINK_DC_50_N = 0x55
    M2M2_LED_PATTERN_MED_BLINK_DC_50 = 0xCC
    M2M2_LED_PATTERN_SLOW_BLINK_DC_50 = 0xF0
    M2M2_LED_PATTERN_ON = 0xFF

class M2M2_LED_PRIORITY_ENUM_t(c_ubyte):
    M2M2_LED_PRIORITY_LOW = 0x0
    M2M2_LED_PRIORITY_MED = 0x1
    M2M2_LED_PRIORITY_HIGH = 0x2
    M2M2_LED_PRIORITY_CRITICAL = 0x3

class m2m2_led_ctrl_t(Structure):
    _pack_ = 1
    _fields_ = [
              ("command", c_ubyte),
              ("priority", c_ubyte),
              ("r_pattern", c_ubyte),
              ("g_pattern", c_ubyte),
              ("b_pattern", c_ubyte),
              ]

