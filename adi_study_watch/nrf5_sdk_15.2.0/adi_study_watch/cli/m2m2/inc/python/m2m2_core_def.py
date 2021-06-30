from ctypes import *

class M2M2_ADDR_ENUM_t(c_ushort):
    M2M2_ADDR_UNDEFINED = 0x0
    M2M2_ADDR_POST_OFFICE = 0xC000
    M2M2_ADDR_EXTERNAL = 0xC001
    M2M2_ADDR_SENSOR_ADXL = 0xC102
    M2M2_ADDR_SENSOR_AD5940 = 0xC104
    M2M2_ADDR_SENSOR_ADPD4000 = 0xC110
    M2M2_ADDR_SENSOR_ADXL_STREAM = 0xC202
    M2M2_ADDR_SENSOR_AD5940_STREAM = 0xC204
    M2M2_ADDR_SENSOR_ADPD_STREAM1 = 0xC211
    M2M2_ADDR_SENSOR_ADPD_STREAM2 = 0xC212
    M2M2_ADDR_SENSOR_ADPD_STREAM3 = 0xC213
    M2M2_ADDR_SENSOR_ADPD_STREAM4 = 0xC214
    M2M2_ADDR_SENSOR_ADPD_STREAM5 = 0xC215
    M2M2_ADDR_SENSOR_ADPD_STREAM6 = 0xC216
    M2M2_ADDR_SENSOR_ADPD_STREAM7 = 0xC217
    M2M2_ADDR_SENSOR_ADPD_STREAM8 = 0xC218
    M2M2_ADDR_SENSOR_ADPD_STREAM9 = 0xC219
    M2M2_ADDR_SENSOR_ADPD_STREAM10 = 0xC21A
    M2M2_ADDR_SENSOR_ADPD_STREAM11 = 0xC21B
    M2M2_ADDR_SENSOR_ADPD_STREAM12 = 0xC21C
    M2M2_ADDR_SENSOR_ADPD_OPTIONAL_BYTES_STREAM = 0xC21D
    M2M2_ADDR_MED_PPG = 0xC300
    M2M2_ADDR_MED_ECG = 0xC301
    M2M2_ADDR_MED_EDA = 0xC302
    M2M2_ADDR_MED_PED = 0xC304
    M2M2_ADDR_MED_SYNC_ADPD_ADXL = 0xC305
    M2M2_ADDR_MED_TEMPERATURE = 0xC306
    M2M2_ADDR_MED_BCM = 0xC307
    M2M2_ADDR_MED_PPG_STREAM = 0xC400
    M2M2_ADDR_MED_ECG_STREAM = 0xC401
    M2M2_ADDR_MED_EDA_STREAM = 0xC402
    M2M2_ADDR_MED_PED_STREAM = 0xC404
    M2M2_ADDR_MED_SYNC_ADPD_ADXL_STREAM = 0xC405
    M2M2_ADDR_MED_TEMPERATURE_STREAM = 0xC406
    M2M2_ADDR_MED_BCM_STREAM = 0xC407
    M2M2_ADDR_SYS_PM = 0xC500
    M2M2_ADDR_SYS_FS = 0xC501
    M2M2_ADDR_SYS_LED_0 = 0xC502
    M2M2_ADDR_DISPLAY = 0xC503
    M2M2_ADDR_SYS_WDT = 0xC590
    M2M2_ADDR_SYS_PM_STREAM = 0xC600
    M2M2_ADDR_SYS_FS_STREAM = 0xC601
    M2M2_ADDR_SYS_LED_0_STREAM = 0xC602
    M2M2_ADDR_SYS_WDT_STREAM = 0xC690
    M2M2_ADDR_SYS_BATT_STREAM = 0xC691
    M2M2_ADDR_SYS_DBG_STREAM = 0xC6A0
    M2M2_ADDR_SYS_AGC_STREAM = 0xC6B0
    M2M2_ADDR_SYS_HRV_STREAM = 0xC6C0
    M2M2_ADDR_APP_DROID = 0xC700
    M2M2_ADDR_APP_IOS = 0xC701
    M2M2_ADDR_APP_VS = 0xC702
    M2M2_ADDR_APP_WT = 0xC703
    M2M2_ADDR_APP_NFE = 0xC704
    M2M2_ADDR_APP_CLI = 0xC705
    M2M2_ADDR_APP_DROID_STREAM = 0xC800
    M2M2_ADDR_APP_IOS_STREAM = 0xC801
    M2M2_ADDR_APP_VS_STREAM = 0xC802
    M2M2_ADDR_APP_WT_STREAM = 0xC803
    M2M2_ADDR_APP_NFE_STREAM = 0xC804
    M2M2_ADDR_APP_CLI_STREAM = 0xC805
    M2M2_ADDR_SYS_BLE = 0xC806
    M2M2_ADDR_SYS_BLE_STREAM = 0xC807
    M2M2_ADDR_APP_CLI_BLE = 0xC808
    M2M2_ADDR_APP_DISPLAY_STREAM = 0xC809
    M2M2_ADDR_APP_LT_APP = 0xC80A
    M2M2_ADDR_SENSOR_AD7156 = 0xC80B
    M2M2_ADDR_MED_SQI = 0xC80C
    M2M2_ADDR_MED_SQI_STREAM = 0xC80D
    M2M2_ADDR_BLE_SERVICES_SENSOR = 0xC80E
    M2M2_ADDR_GLOBAL = 0xFFFF

class M2M2_STATUS_ENUM_t(c_ubyte):
    M2M2_OK = 0x0
    M2M2_ERROR_INVALID_MAILBOX = 0x1
    M2M2_ERROR_INVALID_ADDRESS = 0x2

def m2m2_hdr_t(array_size):
  class m2m2_hdr_t_internal(BigEndianStructure):
    _pack_ = 1
    _fields_ = [
              ("src", c_ushort),
              ("dest", c_ushort),
              ("length", c_ushort),
              ("checksum", c_ushort),
              ("data", c_ubyte * array_size),
              ]
  return m2m2_hdr_t_internal()

