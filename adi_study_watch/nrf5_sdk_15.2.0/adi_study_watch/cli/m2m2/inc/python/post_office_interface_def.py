from ctypes import *

from m2m2_core_def import *


class POST_OFFICE_CFG_CMD_ENUM_t(c_ubyte):
    POST_OFFICE_CFG_CMD_ADD_MAILBOX = 0x1
    POST_OFFICE_CFG_CMD_REMOVE_MAILBOX = 0x2
    POST_OFFICE_CFG_CMD_MAILBOX_SUBSCRIBE = 0x3
    POST_OFFICE_CFG_CMD_MAILBOX_UNSUBSCRIBE = 0x4

class post_office_config_t(Structure):
    _pack_ = 1
    _fields_ = [
              ("cmd", c_ubyte),
              ("box", c_ushort),
              ("sub", c_ushort),
              ]

