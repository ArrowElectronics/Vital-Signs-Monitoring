from ctypes import *

from common_application_interface_def import *

from m2m2_core_def import *

class M2M2_SENSOR_SYNC_DATA_TYPES_ENUM_t(c_ubyte):
    M2M2_SENSOR_ADPD_ADXL_SYNC_DATA = 0x0

class adpd_adxl_sync_data_format_t(Structure):
    _pack_ = 1
    _fields_ = [
              ("ppgTS", c_ulong),
              ("adxlTS", c_ulong),
              ("incPpgTS", c_ushort * 3),
              ("incAdxlTS", c_ushort * 3),
              ("ppgData", c_ulong * 4),
              ("xData", c_short * 4),
              ("yData", c_short * 4),
              ("zData", c_short * 4),
              ]

class adpd_adxl_sync_data_stream_t(Structure):
    _pack_ = 1
    _fields_ = [
              ("command", c_ubyte),
              ("status", c_ubyte),
              ("sequence_num", c_ushort),
              ("syncData", adpd_adxl_sync_data_format_t),
              ]

