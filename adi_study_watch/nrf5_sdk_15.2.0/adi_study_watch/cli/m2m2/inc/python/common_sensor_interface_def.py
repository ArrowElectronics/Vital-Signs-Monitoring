from ctypes import *

from common_application_interface_def import *

from m2m2_core_def import *

class M2M2_SENSOR_COMMON_STATUS_ENUM_t(c_ubyte):
    __M2M2_SENSOR_COMMON_STATUS_LOWEST = 0x20
    M2M2_SENSOR_COMMON_STATUS_RUNNING = 0x21
    __M2M2_SENSOR_COMMON_STATUS_HIGHEST = 0x40

class M2M2_SENSOR_COMMON_CMD_ENUM_t(c_ubyte):
    __M2M2_SENSOR_COMMON_CMD_LOWEST = 0x20
    M2M2_SENSOR_COMMON_CMD_READ_REG_16_REQ = 0x21
    M2M2_SENSOR_COMMON_CMD_READ_REG_16_RESP = 0x22
    M2M2_SENSOR_COMMON_CMD_WRITE_REG_16_REQ = 0x23
    M2M2_SENSOR_COMMON_CMD_WRITE_REG_16_RESP = 0x24
    M2M2_SENSOR_COMMON_CMD_GET_DCFG_REQ = 0x25
    M2M2_SENSOR_COMMON_CMD_GET_DCFG_RESP = 0x26
    M2M2_SENSOR_COMMON_CMD_STREAM_TIMESTAMP = 0x27
    M2M2_SENSOR_COMMON_CMD_STREAM_DATA = 0x28
    M2M2_SENSOR_COMMON_CMD_GET_STREAM_DEC_FACTOR_REQ = 0x29
    M2M2_SENSOR_COMMON_CMD_GET_STREAM_DEC_FACTOR_RESP = 0x2A
    M2M2_SENSOR_COMMON_CMD_SET_STREAM_DEC_FACTOR_REQ = 0x2B
    M2M2_SENSOR_COMMON_CMD_SET_STREAM_DEC_FACTOR_RESP = 0x2C
    M2M2_SENSOR_COMMON_CMD_READ_REG_32_REQ = 0x2D
    M2M2_SENSOR_COMMON_CMD_READ_REG_32_RESP = 0x2E
    M2M2_SENSOR_COMMON_CMD_WRITE_REG_32_REQ = 0x2F
    M2M2_SENSOR_COMMON_CMD_WRITE_REG_32_RESP = 0x30
    _M2M2_SENSOR_COMMON_CMD_HIGHEST_ = 0x40

class m2m2_sensor_common_reg_op_16_t(Structure):
    _pack_ = 1
    _fields_ = [
              ("address", c_ushort),
              ("value", c_ushort),
              ]

def m2m2_sensor_common_reg_op_16_hdr_t(array_size):
  class m2m2_sensor_common_reg_op_16_hdr_t_internal(Structure):
    _pack_ = 1
    _fields_ = [
              ("command", c_ubyte),
              ("status", c_ubyte),
              ("num_ops", c_ubyte),
              ("ops", m2m2_sensor_common_reg_op_16_t * array_size),
              ]
  return m2m2_sensor_common_reg_op_16_hdr_t_internal()

class m2m2_sensor_common_reg_op_32_t(Structure):
    _pack_ = 1
    _fields_ = [
              ("address", c_ushort),
              ("value", c_ulong),
              ]

def m2m2_sensor_common_reg_op_32_hdr_t(array_size):
  class m2m2_sensor_common_reg_op_32_hdr_t_internal(Structure):
    _pack_ = 1
    _fields_ = [
              ("command", c_ubyte),
              ("status", c_ubyte),
              ("num_ops", c_ubyte),
              ("ops", m2m2_sensor_common_reg_op_32_t * array_size),
              ]
  return m2m2_sensor_common_reg_op_32_hdr_t_internal()

class m2m2_sensor_common_timestamp_t(Structure):
    _pack_ = 1
    _fields_ = [
              ("command", c_ubyte),
              ("status", c_ubyte),
              ("timestamp", c_ulong),
              ]

class m2m2_sensor_common_decimate_stream_t(Structure):
    _pack_ = 1
    _fields_ = [
              ("command", c_ubyte),
              ("status", c_ubyte),
              ("stream", c_ushort),
              ("dec_factor", c_ubyte),
              ]

