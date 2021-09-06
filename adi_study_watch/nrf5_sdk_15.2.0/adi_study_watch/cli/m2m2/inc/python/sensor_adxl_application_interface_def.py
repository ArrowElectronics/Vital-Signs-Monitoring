from ctypes import *

from common_application_interface_def import *

from common_sensor_interface_def import *


class M2M2_SENSOR_ADXL_COMMAND_ENUM_t(c_ubyte):
    _M2M2_SENSOR_ADXL_COMMAND_LOWEST = 0x40
    M2M2_SENSOR_ADXL_COMMAND_LOAD_CFG_REQ = 0x42
    M2M2_SENSOR_ADXL_COMMAND_LOAD_CFG_RESP = 0x43
    M2M2_SENSOR_ADXL_COMMAND_SELF_TEST_REQ = 0x44
    M2M2_SENSOR_ADXL_COMMAND_SELF_TEST_RESP = 0x45

class M2M2_SENSOR_ADXL_RAW_DATA_TYPES_ENUM_t(c_ubyte):
    M2M2_SENSOR_RAW_DATA_ADXL = 0x0

class M2M2_SENSOR_ADXL_NSAMPLES_ENUM_t(c_ubyte):
    M2M2_SENSOR_ADXL_NSAMPLES_NO_COMPRESS = 0x5

class M2M2_SENSOR_ADXL_DEVICE_ID_ENUM_t(c_ushort):
    M2M2_SENSOR_ADXL_DEVICE_362 = 0x16A

class m2m2_sensor_adxl_resp_t(Structure):
    _pack_ = 1
    _fields_ = [
              ("command", c_ubyte),
              ("status", c_ubyte),
              ("deviceid", c_ushort),
              ]

class adxl_no_compress_format_t(Structure):
    _pack_ = 1
    _fields_ = [
              ("incTS", c_ushort),
              ("xdata", c_short),
              ("ydata", c_short),
              ("zdata", c_short),
              ]

class adxl_data_header_t(Structure):
    _pack_ = 1
    _fields_ = [
              ("command", c_ubyte),
              ("status", c_ubyte),
              ("sequence_num", c_ushort),
              ("data_type", c_ubyte),
              ("timestamp", c_ulong),
              ("first_xdata", c_short),
              ("first_ydata", c_short),
              ("first_zdata", c_short),
              ]

class m2m2_sensor_adxl_data_no_compress_stream_t(Structure):
    _pack_ = 1
    _fields_ = [
              ("command", c_ubyte),
              ("status", c_ubyte),
              ("sequence_num", c_ushort),
              ("data_type", c_ubyte),
              ("timestamp", c_ulong),
              ("first_xdata", c_short),
              ("first_ydata", c_short),
              ("first_zdata", c_short),
              ("adxldata", adxl_no_compress_format_t * 4),
              ]

