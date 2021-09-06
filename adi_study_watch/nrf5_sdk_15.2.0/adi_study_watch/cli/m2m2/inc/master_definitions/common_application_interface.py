#!/usr/bin/env python3

from ctypes import *

import m2m2_core
import dcb_interface

MAXTXRXDCFGSIZE = (57)  #MAXTXRXDCFGSIZE is equal to MAXADPD4000DCBSIZE in dcb_interface.h

"""
BLE_NUS_MAX_DATA_LEN is defined in ble_nus.h (244)
PING_PKT_SZ = (BLE_NUS_MAX_DATA_LEN-8-6)//Max pkt size=BLE_NUS_MAX_DATA_LEN
Max ping pkt size can be BLE_NUS_MAX_DATA_LEN-8(header length)-1-1-4(remaining field in m2m2_app_common_ping_t)=230
"""
PING_PKT_SZ = (230)

class M2M2_APP_COMMON_STATUS_ENUM_t(c_uint8):
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
    __M2M2_APP_COMMON_STATUS_HIGHEST = 0x20

class M2M2_APP_COMMON_CMD_ENUM_t(c_uint8):
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
    __M2M2_APP_COMMON_CMD_HIGHEST = 0x20

class _m2m2_app_common_cmd_t(Structure):
    fields = [
              ("command", c_uint8),
              ("status", c_uint8),
              ]

class m2m2_app_common_ver_req_t(Structure):
    fields = [
              ("command", c_uint8),
              ("status", c_uint8),
              ]

class m2m2_app_common_ping_t(Structure):
    fields = [
              (None, _m2m2_app_common_cmd_t),
              ("sequence_num", c_uint32),
              ("data", c_uint8 * PING_PKT_SZ),
              ]

class m2m2_app_common_version_t(Structure):
    fields = [
              (None, _m2m2_app_common_cmd_t),
              ("major", c_uint16),
              ("minor", c_uint16),
              ("patch", c_uint16),
              ("verstr", c_uint8 * 10),
              ("str", c_uint8 * 40),
              ]

class m2m2_app_common_status_t(Structure):
    fields = [
              (None, _m2m2_app_common_cmd_t),
              ("stream", c_uint16),
              ("num_subscribers", c_uint8),
              ("num_start_reqs", c_uint8),
              ]

class m2m2_app_common_sub_op_t(Structure):
    fields = [
              (None, _m2m2_app_common_cmd_t),
              ("stream", c_uint16),
              ]

class m2m2_sensor_dcfg_data_t(Structure):
    fields = [
              (None, _m2m2_app_common_cmd_t),
              ("size", c_uint8),
              ("num_tx_pkts" ,c_uint8),
              ("dcfgdata", c_uint32 * 57),
              ]

class m2m2_app_lcfg_data_t(Structure):
    fields = [
              (None, _m2m2_app_common_cmd_t),
              ("size", c_uint8),
              ("lcfgdata", c_uint8 * 224),
              ]

class _m2m2_app_data_stream_hdr_t(Structure):
    fields = [
              (None, _m2m2_app_common_cmd_t),
              ("sequence_num", c_uint16),
              ]