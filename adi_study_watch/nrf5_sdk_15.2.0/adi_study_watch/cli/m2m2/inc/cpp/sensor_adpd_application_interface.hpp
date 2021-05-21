// #############################################################################
// THIS IS AN AUTOMATICALLY GENERATED FILE, DO NOT MODIFY IT!
// #############################################################################
#pragma once

#include "common_application_interface.hpp"
#include "common_sensor_interface.hpp"
#include <stdint.h>


/* Explicitly enforce struct packing so that the nested structs and unions are laid out
    as expected. */
#if defined __CC_ARM || defined __IAR_SYSTEMS_ICC__ || __clang__ || defined _MSC_VER || defined __GNUC__
// DELIBERATELY BLANK
#else
#error "WARNING! Your compiler might not support '#pragma pack(1)'! \
  You must add an equivalent compiler directive to the file generator!"
#endif  // defined __CC_ARM || defined __IAR_SYSTEMS_ICC__ || __clang__ || defined _MSC_VER || defined __GNUC__
#pragma pack(1)

enum M2M2_SENSOR_ADPD_COMMAND_ENUM_t:uint8_t {
  _M2M2_SENSOR_ADPD_COMMAND_LOWEST = 64,
  M2M2_SENSOR_ADPD_COMMAND_LOAD_CFG_REQ = 66,
  M2M2_SENSOR_ADPD_COMMAND_LOAD_CFG_RESP = 67,
  M2M2_SENSOR_ADPD_COMMAND_CLOCK_CAL_REQ = 68,
  M2M2_SENSOR_ADPD_COMMAND_CLOCK_CAL_RESP = 69,
  M2M2_SENSOR_ADPD_COMMAND_SET_SLOT_REQ = 70,
  M2M2_SENSOR_ADPD_COMMAND_SET_SLOT_RESP = 71,
  M2M2_SENSOR_ADPD_COMMAND_GET_SLOT_REQ = 72,
  M2M2_SENSOR_ADPD_COMMAND_GET_SLOT_RESP = 73,
  M2M2_SENSOR_ADPD_COMMAND_SET_DARK_OFFSET_REQ = 74,
  M2M2_SENSOR_ADPD_COMMAND_SET_DARK_OFFSET_RESP = 75,
  M2M2_SENSOR_ADPD_COMMAND_GET_CTR_REQ = 76,
  M2M2_SENSOR_ADPD_COMMAND_GET_CTR_RESP = 77,
  M2M2_SENSOR_ADPD_COMMAND_FLOATMODE_CFG_REQ = 78,
  M2M2_SENSOR_ADPD_COMMAND_FLOATMODE_CFG_RESP = 79,
  M2M2_SENSOR_ADPD_COMMAND_DO_TEST1_REQ = 96,
  M2M2_SENSOR_ADPD_COMMAND_DO_TEST1_RESP = 97,
  M2M2_SENSOR_ADPD_COMMAND_DO_TEST2_REQ = 98,
  M2M2_SENSOR_ADPD_COMMAND_DO_TEST2_RESP = 99,
  M2M2_SENSOR_ADPD_COMMAND_DO_TEST3_REQ = 100,
  M2M2_SENSOR_ADPD_COMMAND_DO_TEST3_RESP = 101,
  M2M2_SENSOR_ADPD_COMMAND_SET_PAUSE_REQ = 102,
  M2M2_SENSOR_ADPD_COMMAND_SET_PAUSE_RESP = 103,
  M2M2_SENSOR_ADPD_COMMUNICATION_MODE_REQ = 104,
  M2M2_SENSOR_ADPD_COMMUNICATION_MODE_RESP = 105,
  M2M2_SENSOR_ADPD_COMMAND_SET_SLOT_ACTIVE_REQ = 106,
  M2M2_SENSOR_ADPD_COMMAND_SET_SLOT_ACTIVE_RESP = 107,
  M2M2_SENSOR_ADPD_COMMAND_GET_SLOT_ACTIVE_REQ = 108,
  M2M2_SENSOR_ADPD_COMMAND_GET_SLOT_ACTIVE_RESP = 109,
  M2M2_SENSOR_ADPD_COMMAND_GET_EFUSE_REQ = 110,
  M2M2_SENSOR_ADPD_COMMAND_GET_EFUSE_RESP = 111,
  M2M2_SENSOR_ADPD_COMMAND_CREATE_DCFG_REQ = 112,
  M2M2_SENSOR_ADPD_COMMAND_CREATE_DCFG_RESP = 113,
  M2M2_SENSOR_ADPD_COMMAND_SET_FS_REQ = 114,
  M2M2_SENSOR_ADPD_COMMAND_SET_FS_RESP = 115,
  M2M2_SENSOR_ADPD_COMMAND_DISABLE_SLOTS_REQ = 116,
  M2M2_SENSOR_ADPD_COMMAND_DISABLE_SLOTS_RESP = 117,
  M2M2_SENSOR_ADPD_COMMAND_AGC_ON_OFF_REQ = 118,
  M2M2_SENSOR_ADPD_COMMAND_AGC_ON_OFF_RESP = 119,
  M2M2_SENSOR_ADPD_COMMAND_AGC_RECALIBRATE_REQ = 120,
  M2M2_SENSOR_ADPD_COMMAND_AGC_RECALIBRATE_RESP = 121,
  M2M2_SENSOR_ADPD_COMMAND_AGC_INFO_REQ = 122,
  M2M2_SENSOR_ADPD_COMMAND_AGC_INFO_RESP = 123,
  M2M2_SENSOR_ADPD_COMMAND_AGC_STATUS_REQ = 124,
  M2M2_SENSOR_ADPD_COMMAND_AGC_STATUS_RESP = 125,
  M2M2_SENSOR_ADPD_COMMAND_SET_EXT_DATA_STREAM_ODR_REQ = 126,
  M2M2_SENSOR_ADPD_COMMAND_SET_EXT_DATA_STREAM_ODR_RESP = 127,
  M2M2_SENSOR_ADPD_COMMAND_EXT_ADPD_DATA_STREAM = 128,
  M2M2_SENSOR_ADPD_COMMAND_UC_HR_ENAB_REQ = 130,
  M2M2_SENSOR_ADPD_COMMAND_UC_HR_ENAB_RESP = 131,
};
static_assert(sizeof(M2M2_SENSOR_ADPD_COMMAND_ENUM_t) == 1, "Enum 'M2M2_SENSOR_ADPD_COMMAND_ENUM_t' has an incorrect size!");

enum M2M2_SENSOR_ADPD_SLOTMODE_ENUM_t:uint8_t {
  M2M2_SENSOR_ADPD_SLOTMODE_DISABLED = 0,
  M2M2_SENSOR_ADPD_SLOTMODE_4CH_16b = 1,
  M2M2_SENSOR_ADPD_SLOTMODE_4CH_32b = 4,
  M2M2_SENSOR_ADPD_SLOTMODE_SUM_16b = 17,
  M2M2_SENSOR_ADPD_SLOTMODE_SUM_32b = 20,
};
static_assert(sizeof(M2M2_SENSOR_ADPD_SLOTMODE_ENUM_t) == 1, "Enum 'M2M2_SENSOR_ADPD_SLOTMODE_ENUM_t' has an incorrect size!");

enum M2M2_SENSOR_ADPD_NSAMPLES_ENUM_t:uint8_t {
  M2M2_SENSOR_ADPD_NSAMPLES_4CH_32 = 2,
  M2M2_SENSOR_ADPD_NSAMPLES_4CH_16 = 4,
  M2M2_SENSOR_ADPD_NSAMPLES_SUM_32 = 7,
  M2M2_SENSOR_ADPD_NSAMPLES_SUM_16 = 11,
  M2M2_SENSOR_ADPD_NSAMPLES_SUM_32_COMPRESS = 21,
};
static_assert(sizeof(M2M2_SENSOR_ADPD_NSAMPLES_ENUM_t) == 1, "Enum 'M2M2_SENSOR_ADPD_NSAMPLES_ENUM_t' has an incorrect size!");

enum M2M2_SENSOR_ADPD_RAW_DATA_TYPES_ENUM_t:uint8_t {
  M2M2_SENSOR_RAW_DATA_ADPD_A_4CH_16b = 0,
  M2M2_SENSOR_RAW_DATA_ADPD_A_4CH_32b = 1,
  M2M2_SENSOR_RAW_DATA_ADPD_A_SUM_16b = 2,
  M2M2_SENSOR_RAW_DATA_ADPD_A_SUM_32b = 3,
  M2M2_SENSOR_RAW_DATA_ADPD_B_4CH_16b = 8,
  M2M2_SENSOR_RAW_DATA_ADPD_B_4CH_32b = 9,
  M2M2_SENSOR_RAW_DATA_ADPD_B_SUM_16b = 10,
  M2M2_SENSOR_RAW_DATA_ADPD_B_SUM_32b = 11,
};
static_assert(sizeof(M2M2_SENSOR_ADPD_RAW_DATA_TYPES_ENUM_t) == 1, "Enum 'M2M2_SENSOR_ADPD_RAW_DATA_TYPES_ENUM_t' has an incorrect size!");

enum M2M2_SENSOR_ADPD_DEVICE_ID_ENUM_t:uint8_t {
  M2M2_SENSOR_ADPD_DEVICE_107 = 2,
  M2M2_SENSOR_ADPD_DEVICE_185 = 3,
  M2M2_SENSOR_ADPD_DEVICE_ECG_PPG_185 = 4,
  M2M2_SENSOR_ADPD_DEVICE_ECG_185 = 5,
  M2M2_SENSOR_ADPD_DEVICE_105 = 6,
  M2M2_SENSOR_ADPD_DEVICE_188 = 7,
  M2M2_SENSOR_ADPD_DEVICE_108 = 8,
  M2M2_SENSOR_ADPD_DEVICE_188F = 9,
  M2M2_SENSOR_ADPD_DEVICE_ECG_PPG_188 = 10,
  M2M2_SENSOR_ADPD_DEVICE_ECG_188 = 11,
  M2M2_SENSOR_ADPD4000_DEVICE_4000_G = 40,
  M2M2_SENSOR_ADPD4000_DEVICE_4000_R = 41,
  M2M2_SENSOR_ADPD4000_DEVICE_4000_IR = 42,
  M2M2_SENSOR_ADPD4000_DEVICE_4000_B = 43,
  M2M2_SENSOR_ADPD4000_DEVICE_4000_G_R_IR_B = 44,
};
static_assert(sizeof(M2M2_SENSOR_ADPD_DEVICE_ID_ENUM_t) == 1, "Enum 'M2M2_SENSOR_ADPD_DEVICE_ID_ENUM_t' has an incorrect size!");

struct m2m2_sensor_adpd_resp_t {
  uint8_t  command;
  uint8_t  status;
  M2M2_SENSOR_ADPD_DEVICE_ID_ENUM_t  deviceid;
  uint16_t  retdata[5];
};

struct m2m2_sensor_adpd_testcommand_resp_t {
  uint8_t  command;
  uint8_t  status;
  uint32_t  retdata[3];
};

struct adpd_data_header_t {
  uint8_t  command;
  uint8_t  status;
  uint16_t  sequence_num;
  M2M2_SENSOR_ADPD_RAW_DATA_TYPES_ENUM_t  data_type;
  uint32_t  timestamp;
};

struct adpdCl_data_header_t {
  uint8_t  command;
  uint8_t  status;
  uint16_t  sequence_num;
  uint16_t  data_format;
  uint8_t  channel_num;
  uint32_t  timestamp;
};

struct m2m2_sensor_adpdCl_data_stream_t {
  uint8_t  command;
  uint8_t  status;
  uint16_t  sequence_num;
  uint16_t  data_format;
  uint8_t  channel_num;
  uint32_t  timestamp;
  uint8_t  sample_num;
  uint8_t  adpddata[24];
};

struct m2m2_sensor_adpdCl_impulse_stream_t {
  uint8_t  command;
  uint8_t  status;
  uint16_t  sequence_num;
  uint16_t  data_format;
  uint8_t  channel_num;
  uint32_t  timestamp;
  uint8_t  sample_num;
  uint8_t  adpddata[256];
};

struct adpd_4ch32_data_set_t {
  uint32_t  ch1;
  uint32_t  ch2;
  uint32_t  ch3;
  uint32_t  ch4;
};

struct adpd_4ch16_data_set_t {
  uint16_t  ch1;
  uint16_t  ch2;
  uint16_t  ch3;
  uint16_t  ch4;
};

struct adpd_sum32_data_no_compress_format_t {
  uint16_t  incTS;
  uint32_t  sum32data;
};

struct adpd_4ch32_data_no_compress_format_t {
  uint16_t  incTS;
  adpd_4ch32_data_set_t  ch32data;
};

struct adpd_sum16_data_no_compress_format_t {
  uint16_t  incTS;
  uint16_t  sum16data;
};

struct adpd_4ch16_data_no_compress_format_t {
  uint16_t  incTS;
  adpd_4ch16_data_set_t  ch16data;
};

struct m2m2_sensor_adpd_data_sum32_no_compress_stream_t {
  uint8_t  command;
  uint8_t  status;
  uint16_t  sequence_num;
  M2M2_SENSOR_ADPD_RAW_DATA_TYPES_ENUM_t  data_type;
  uint32_t  timestamp;
  uint32_t  first_adpddata;
  adpd_sum32_data_no_compress_format_t  adpddata[6];
};

struct m2m2_sensor_adpd_data_4ch32_no_compress_stream_t {
  uint8_t  command;
  uint8_t  status;
  uint16_t  sequence_num;
  M2M2_SENSOR_ADPD_RAW_DATA_TYPES_ENUM_t  data_type;
  uint32_t  timestamp;
  adpd_4ch32_data_set_t  first_adpddata;
  adpd_4ch32_data_no_compress_format_t  adpddata[1];
};

struct m2m2_sensor_adpd_data_sum16_no_compress_stream_t {
  uint8_t  command;
  uint8_t  status;
  uint16_t  sequence_num;
  M2M2_SENSOR_ADPD_RAW_DATA_TYPES_ENUM_t  data_type;
  uint32_t  timestamp;
  uint16_t  first_adpddata;
  adpd_sum16_data_no_compress_format_t  adpddata[10];
};

struct m2m2_sensor_adpd_data_4ch16_no_compress_stream_t {
  uint8_t  command;
  uint8_t  status;
  uint16_t  sequence_num;
  M2M2_SENSOR_ADPD_RAW_DATA_TYPES_ENUM_t  data_type;
  uint32_t  timestamp;
  adpd_4ch16_data_set_t  first_adpddata;
  adpd_4ch16_data_no_compress_format_t  adpddata[3];
};

struct m2m2_sensor_adpd_data_sum32_compress_stream_t {
  uint8_t  command;
  uint8_t  status;
  uint16_t  sequence_num;
  M2M2_SENSOR_ADPD_RAW_DATA_TYPES_ENUM_t  data_type;
  uint32_t  timestamp;
  uint8_t  first_adpddata[3];
  int16_t  inc_sum[21];
};

struct m2m2_sensor_adpd_slot_resp_t {
  uint8_t  command;
  uint8_t  status;
  M2M2_SENSOR_ADPD_SLOTMODE_ENUM_t  slotA;
  M2M2_SENSOR_ADPD_SLOTMODE_ENUM_t  slotB;
};

struct m2m2_sensor_adpd4000_slot_resp_t {
  uint8_t  command;
  uint8_t  status;
  uint8_t  slot_num;
  uint8_t  slot_enable;
  uint16_t  slot_format;
  uint8_t  channel_num;
};

struct m2m2_sensor_adpd4000_slot_active_resp_t {
  uint8_t  command;
  uint8_t  status;
  uint8_t  slot_num;
  uint8_t  slot_active;
};

struct m2m2_sensor_clockcal_resp_t {
  uint8_t  command;
  uint8_t  status;
  uint8_t  clockcalid;
};

struct m2m2_sensor_com_mode_resp_t {
  uint8_t  command;
  uint8_t  status;
  uint8_t  com_mode;
};

struct m2m2_sensor_adpd4000_resp_t {
  uint8_t  command;
  uint8_t  status;
  M2M2_SENSOR_ADPD_DEVICE_ID_ENUM_t  deviceid;
  uint16_t  retdata[5];
};

struct m2m2_sensor_efuse_resp_t {
  uint8_t  command;
  uint8_t  status;
  uint8_t  efusedata[16];
};

struct m2m2_sensor_fifo_status_bytes_t {
  uint8_t  command;
  uint8_t  status;
  uint32_t  timestamp;
  uint8_t  sequence_num;
  uint16_t  data_int;
  uint16_t  level0_int;
  uint16_t  level1_int;
  uint16_t  tia_ch1_int;
  uint16_t  tia_ch2_int;
};

struct adpd_ext_data_stream_t {
  uint8_t  command;
  uint8_t  status;
  uint32_t sequence_num;
  uint32_t data;
  uint32_t timestamp;
};

struct adpd_ext_data_stream_odr_t {
  uint8_t  command;
  uint8_t  status;
  uint16_t sampling_freq;
};

struct m2m2_adpd_set_uc_hr_enab_t {
  uint8_t  command;
  uint8_t  status;
  uint8_t  control;
  uint16_t  slotNum;
};

// Reset struct packing outside of this file
#pragma pack()
