from ctypes import *

from common_application_interface_def import *

from common_sensor_interface_def import *

class M2M2_SENSOR_AD5940_COMMAND_ENUM_t(c_ubyte):
    _M2M2_SENSOR_AD5940_COMMAND_LOWEST = 0x40
    M2M2_SENSOR_AD5940_COMMAND_LOAD_CFG_REQ = 0x42
    M2M2_SENSOR_AD5940_COMMAND_LOAD_CFG_RESP = 0x43
    M2M2_SENSOR_AD5940_COMMAND_SET_ODR_REQ = 0x44
    M2M2_SENSOR_AD5940_COMMAND_SET_ODR_RESP = 0x45
    M2M2_SENSOR_AD5940_COMMAND_SET_WG_FREQ_REQ = 0x46
    M2M2_SENSOR_AD5940_COMMAND_SET_WG_FREQ_RESP = 0x47

class M2M2_SENSOR_AD5940_NSAMPLES_ENUM_t(c_ubyte):
    M2M2_SENSOR_AD5940_NSAMPLES_BCM = 0x4
    M2M2_SENSOR_AD5940_NSAMPLES_EDA = 0x4
    M2M2_SENSOR_AD5940_NSAMPLES_ECG = 0x2

class M2M2_SENSOR_AD5940_RAW_DATA_TYPES_ENUM_t(c_ubyte):
    M2M2_SENSOR_RAW_DATA_AD5940_BCM = 0x0
    M2M2_SENSOR_RAW_DATA_AD5940_EDA = 0x1
    M2M2_SENSOR_RAW_DATA_AD5940_ECG = 0x2

class M2M2_SENSOR_AD5940_DATARATE_ENUM_t(c_ubyte):
    M2M2_SENSOR_AD5940_DATARATE_25 = 0x0
    M2M2_SENSOR_AD5940_DATARATE_50 = 0x1
    M2M2_SENSOR_AD5940_DATARATE_100 = 0x2
    M2M2_SENSOR_AD5940_DATARATE_200 = 0x3
    M2M2_SENSOR_AD5940_DATARATE_500 = 0x4

class ad5940_data_header_t(Structure):
    _pack_ = 1
    _fields_ = [
              ("command", c_ubyte),
              ("status", c_ubyte),
              ("sequence_num", c_ushort),
              ("datatype", c_ubyte),
              ("timestamp", c_ulong),
              ]

class m2m2_sensor_ad5940_set_odr_resp_t(Structure):
    _pack_ = 1
    _fields_ = [
              ("command", c_ubyte),
              ("status", c_ubyte),
              ("datarate", c_ubyte),
              ]

class m2m2_sensor_ad5940_set_wg_freq_resp_t(Structure):
    _pack_ = 1
    _fields_ = [
              ("command", c_ubyte),
              ("status", c_ubyte),
              ("wg_freq", c_ulong),
              ]

