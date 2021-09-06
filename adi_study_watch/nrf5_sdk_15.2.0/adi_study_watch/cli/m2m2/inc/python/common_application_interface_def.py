from ctypes import *

from dcb_interface_def import *

from m2m2_core_def import *

MAXTXRXDCFGSIZE = (57)
PING_PKT_SZ = (230)

class M2M2_APP_COMMON_STATUS_ENUM_t(c_ubyte):
    M2M2_APP_COMMON_STATUS_OK = 0x0
    M2M2_APP_COMMON_STATUS_ERROR = 0x1
    M2M2_APP_COMMON_STATUS_STREAM_STARTED = 0x2
    M2M2_APP_COMMON_STATUS_STREAM_STOPPED = 0x3
    M2M2_APP_COMMON_STATUS_STREAM_IN_PROGRESS = 0x4
    M2M2_APP_COMMON_STATUS_STREAM_DEACTIVATED = 0x5
    M2M2_APP_COMMON_STATUS_STREAM_COUNT_DECREMENT = 0x6
    M2M2_APP_COMMON_STATUS_STREAM_NOT_STARTED = 0x7
    M2M2_APP_COMMON_STATUS_STREAM_NOT_STOPPED = 0x8
    M2M2_APP_COMMON_STATUS_SUBSCRIBER_ADDED = 0x9
    M2M2_APP_COMMON_STATUS_SUBSCRIBER_REMOVED = 0xA
    M2M2_APP_COMMON_STATUS_SUBSCRIBER_COUNT_DECREMENT = 0xB
    _M2M2_APP_COMMON_STATUS_ENUM_t__M2M2_APP_COMMON_STATUS_HIGHEST = 0x20

class M2M2_APP_COMMON_CMD_ENUM_t(c_ubyte):
    M2M2_APP_COMMON_CMD_GET_VERSION_REQ = 0x0
    M2M2_APP_COMMON_CMD_GET_VERSION_RESP = 0x1
    M2M2_APP_COMMON_CMD_GET_STATUS_REQ = 0x2
    M2M2_APP_COMMON_CMD_GET_STATUS_RESP = 0x3
    M2M2_APP_COMMON_CMD_STREAM_START_REQ = 0x4
    M2M2_APP_COMMON_CMD_STREAM_START_RESP = 0x5
    M2M2_APP_COMMON_CMD_STREAM_STOP_REQ = 0x6
    M2M2_APP_COMMON_CMD_STREAM_STOP_RESP = 0x7
    M2M2_APP_COMMON_CMD_STREAM_PAUSE_REQ = 0x8
    M2M2_APP_COMMON_CMD_STREAM_PAUSE_RESP = 0x9
    M2M2_APP_COMMON_CMD_STREAM_UNPAUSE_REQ = 0xA
    M2M2_APP_COMMON_CMD_STREAM_UNPAUSE_RESP = 0xB
    M2M2_APP_COMMON_CMD_STREAM_SUBSCRIBE_REQ = 0xC
    M2M2_APP_COMMON_CMD_STREAM_SUBSCRIBE_RESP = 0xD
    M2M2_APP_COMMON_CMD_STREAM_UNSUBSCRIBE_REQ = 0xE
    M2M2_APP_COMMON_CMD_STREAM_UNSUBSCRIBE_RESP = 0xF
    M2M2_APP_COMMON_CMD_SENSOR_STATUS_QUERY_REQ = 0x10
    M2M2_APP_COMMON_CMD_SENSOR_STATUS_QUERY_RESP = 0x11
    M2M2_APP_COMMON_CMD_GET_LCFG_REQ = 0x12
    M2M2_APP_COMMON_CMD_GET_LCFG_RESP = 0x13
    M2M2_APP_COMMON_CMD_SET_LCFG_REQ = 0x14
    M2M2_APP_COMMON_CMD_SET_LCFG_RESP = 0x15
    M2M2_APP_COMMON_CMD_READ_LCFG_REQ = 0x16
    M2M2_APP_COMMON_CMD_READ_LCFG_RESP = 0x17
    M2M2_APP_COMMON_CMD_WRITE_LCFG_REQ = 0x18
    M2M2_APP_COMMON_CMD_WRITE_LCFG_RESP = 0x19
    M2M2_APP_COMMON_CMD_PING_REQ = 0x1A
    M2M2_APP_COMMON_CMD_PING_RESP = 0x1B
    _M2M2_APP_COMMON_CMD_ENUM_t__M2M2_APP_COMMON_CMD_HIGHEST = 0x20

class _m2m2_app_common_cmd_t(Structure):
    _pack_ = 1
    _fields_ = [
              ("command", c_ubyte),
              ("status", c_ubyte),
              ]

class m2m2_app_common_ver_req_t(Structure):
    _pack_ = 1
    _fields_ = [
              ("command", c_ubyte),
              ("status", c_ubyte),
              ]

class m2m2_app_common_ping_t(Structure):
    _pack_ = 1
    _fields_ = [
              ("command", c_ubyte),
              ("status", c_ubyte),
              ("sequence_num", c_ulong),
              ("data", c_ubyte * 230),
              ]

class m2m2_app_common_version_t(Structure):
    _pack_ = 1
    _fields_ = [
              ("command", c_ubyte),
              ("status", c_ubyte),
              ("major", c_ushort),
              ("minor", c_ushort),
              ("patch", c_ushort),
              ("verstr", c_ubyte * 10),
              ("str", c_ubyte * 40),
              ]

class m2m2_app_common_status_t(Structure):
    _pack_ = 1
    _fields_ = [
              ("command", c_ubyte),
              ("status", c_ubyte),
              ("stream", c_ushort),
              ("num_subscribers", c_ubyte),
              ("num_start_reqs", c_ubyte),
              ]

class m2m2_app_common_sub_op_t(Structure):
    _pack_ = 1
    _fields_ = [
              ("command", c_ubyte),
              ("status", c_ubyte),
              ("stream", c_ushort),
              ]

class m2m2_sensor_dcfg_data_t(Structure):
    _pack_ = 1
    _fields_ = [
              ("command", c_ubyte),
              ("status", c_ubyte),
              ("size", c_ubyte),
              ("num_tx_pkts", c_ubyte),
              ("dcfgdata", c_ulong * 57),
              ]

class m2m2_app_lcfg_data_t(Structure):
    _pack_ = 1
    _fields_ = [
              ("command", c_ubyte),
              ("status", c_ubyte),
              ("size", c_ubyte),
              ("lcfgdata", c_ubyte * 224),
              ]

class _m2m2_app_data_stream_hdr_t(Structure):
    _pack_ = 1
    _fields_ = [
              ("command", c_ubyte),
              ("status", c_ubyte),
              ("sequence_num", c_ushort),
              ]

