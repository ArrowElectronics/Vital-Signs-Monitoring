#!/usr/bin/env python3

from ctypes import *

import m2m2_core

class POST_OFFICE_CFG_CMD_ENUM_t(c_uint8):
    POST_OFFICE_CFG_CMD_ADD_MAILBOX = 0x1
    POST_OFFICE_CFG_CMD_REMOVE_MAILBOX = 0x2
    POST_OFFICE_CFG_CMD_MAILBOX_SUBSCRIBE = 0x3
    POST_OFFICE_CFG_CMD_MAILBOX_UNSUBSCRIBE = 0x4

class post_office_config_t(Structure):
    fields = [
              ("cmd", c_uint8),
              ("box", c_uint16),
              ("sub", c_uint16),
              ]