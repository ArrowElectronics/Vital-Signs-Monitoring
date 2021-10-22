// #############################################################################
// THIS IS AN AUTOMATICALLY GENERATED FILE, DO NOT MODIFY IT!
// #############################################################################
#pragma once

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

#ifndef STATIC_ASSERT_PROJ
#define STATIC_ASSERT_PROJ(COND, MSG) typedef char static_assertion_##MSG[(COND)?1:-1]
#endif // STATIC_ASSERT_PROJ


typedef enum M2M2_ADDR_ENUM_t {
  M2M2_ADDR_UNDEFINED = 0,
  M2M2_ADDR_POST_OFFICE = 49152,
  M2M2_ADDR_EXTERNAL = 49153,
  M2M2_ADDR_SENSOR_ADXL = 49410,
  M2M2_ADDR_SENSOR_AD5940 = 49412,
  M2M2_ADDR_SENSOR_ADPD4000 = 49424,
  M2M2_ADDR_SENSOR_ADXL_STREAM = 49666,
  M2M2_ADDR_SENSOR_AD5940_STREAM = 49668,
  M2M2_ADDR_SENSOR_ADPD_STREAM1 = 49681,
  M2M2_ADDR_SENSOR_ADPD_STREAM2 = 49682,
  M2M2_ADDR_SENSOR_ADPD_STREAM3 = 49683,
  M2M2_ADDR_SENSOR_ADPD_STREAM4 = 49684,
  M2M2_ADDR_SENSOR_ADPD_STREAM5 = 49685,
  M2M2_ADDR_SENSOR_ADPD_STREAM6 = 49686,
  M2M2_ADDR_SENSOR_ADPD_STREAM7 = 49687,
  M2M2_ADDR_SENSOR_ADPD_STREAM8 = 49688,
  M2M2_ADDR_SENSOR_ADPD_STREAM9 = 49689,
  M2M2_ADDR_SENSOR_ADPD_STREAM10 = 49690,
  M2M2_ADDR_SENSOR_ADPD_STREAM11 = 49691,
  M2M2_ADDR_SENSOR_ADPD_STREAM12 = 49692,
  M2M2_ADDR_SENSOR_ADPD_OPTIONAL_BYTES_STREAM = 49693,
  M2M2_ADDR_MED_PPG = 49920,
  M2M2_ADDR_MED_ECG = 49921,
  M2M2_ADDR_MED_EDA = 49922,
  M2M2_ADDR_MED_PED = 49924,
  M2M2_ADDR_MED_SYNC_ADPD_ADXL = 49925,
  M2M2_ADDR_MED_TEMPERATURE = 49926,
  M2M2_ADDR_MED_BIA = 49927,
  M2M2_ADDR_MED_PPG_STREAM = 50176,
  M2M2_ADDR_MED_ECG_STREAM = 50177,
  M2M2_ADDR_MED_EDA_STREAM = 50178,
  M2M2_ADDR_MED_PED_STREAM = 50180,
  M2M2_ADDR_MED_SYNC_ADPD_ADXL_STREAM = 50181,
  M2M2_ADDR_MED_TEMPERATURE_STREAM = 50182,
  M2M2_ADDR_MED_BIA_STREAM = 50183,
  M2M2_ADDR_SYS_PM = 50432,
  M2M2_ADDR_SYS_FS = 50433,
  M2M2_ADDR_SYS_LED_0 = 50434,
  M2M2_ADDR_DISPLAY = 50435,
  M2M2_ADDR_SYS_WDT = 50576,
  M2M2_ADDR_SYS_PM_STREAM = 50688,
  M2M2_ADDR_SYS_FS_STREAM = 50689,
  M2M2_ADDR_SYS_LED_0_STREAM = 50690,
  M2M2_ADDR_SYS_WDT_STREAM = 50832,
  M2M2_ADDR_SYS_BATT_STREAM = 50833,
  M2M2_ADDR_SYS_DBG_STREAM = 50848,
  M2M2_ADDR_SYS_DYNAMIC_AGC_STREAM = 50864,
  M2M2_ADDR_SYS_STATIC_AGC_STREAM = 50865,
  M2M2_ADDR_SYS_HRV_STREAM = 50880,
  M2M2_ADDR_APP_DROID = 50944,
  M2M2_ADDR_APP_IOS = 50945,
  M2M2_ADDR_APP_VS = 50946,
  M2M2_ADDR_APP_WT = 50947,
  M2M2_ADDR_APP_NFE = 50948,
  M2M2_ADDR_APP_CLI = 50949,
  M2M2_ADDR_APP_DROID_STREAM = 51200,
  M2M2_ADDR_APP_IOS_STREAM = 51201,
  M2M2_ADDR_APP_VS_STREAM = 51202,
  M2M2_ADDR_APP_WT_STREAM = 51203,
  M2M2_ADDR_APP_NFE_STREAM = 51204,
  M2M2_ADDR_APP_CLI_STREAM = 51205,
  M2M2_ADDR_SYS_BLE = 51206,
  M2M2_ADDR_SYS_BLE_STREAM = 51207,
  M2M2_ADDR_APP_CLI_BLE = 51208,
  M2M2_ADDR_APP_DISPLAY_STREAM = 51209,
  M2M2_ADDR_APP_LT_APP = 51210,
  M2M2_ADDR_SENSOR_AD7156 = 51211,
  M2M2_ADDR_MED_SQI = 51212,
  M2M2_ADDR_MED_SQI_STREAM = 51213,
  M2M2_ADDR_BLE_SERVICES_SENSOR = 51214,
  M2M2_ADDR_USER0_CONFIG_APP = 51215,
  M2M2_ADDR_MED_MOTION_DETECT = 51216,
  M2M2_ADDR_MED_MOTION_DETECT_STREAM = 51217,
  M2M2_ADDR_MED_SWO2 = 51218,
  M2M2_ADDR_MED_SWO2_STREAM = 51219,
  M2M2_ADDR_BCM_ALGO_STREAM = 51220,
  M2M2_ADDR_SENSOR_AD7156_STREAM = 51221,
  M2M2_ADDR_MED_TEMPERATURE_STREAM1 = 51222,
  M2M2_ADDR_MED_TEMPERATURE_STREAM2 = 51223,
  M2M2_ADDR_MED_TEMPERATURE_STREAM3 = 51224,
  M2M2_ADDR_MED_TEMPERATURE_STREAM4 = 51225,
  M2M2_ADDR_MED_TEMPERATURE_STREAM5 = 51226,
  M2M2_ADDR_MED_TEMPERATURE_STREAM6 = 51227,
  M2M2_ADDR_MED_TEMPERATURE_STREAM7 = 51228,
  M2M2_ADDR_MED_TEMPERATURE_STREAM8 = 51229,
  M2M2_ADDR_MED_TEMPERATURE_STREAM9 = 51230,
  M2M2_ADDR_MED_TEMPERATURE_STREAM10 = 51231,
  M2M2_ADDR_MED_TEMPERATURE_STREAM11 = 51232,
  M2M2_ADDR_MED_TEMPERATURE_STREAM12 = 51233,  
  M2M2_ADDR_GLOBAL = 65535,
} M2M2_ADDR_ENUM_t;
STATIC_ASSERT_PROJ(sizeof(M2M2_ADDR_ENUM_t) == 2, INCORRECT_SIZE_M2M2_ADDR_ENUM_t);

typedef enum M2M2_STATUS_ENUM_t {
  M2M2_OK = 0,
  M2M2_ERROR_INVALID_MAILBOX = 1,
  M2M2_ERROR_INVALID_ADDRESS = 2,
} M2M2_STATUS_ENUM_t;
STATIC_ASSERT_PROJ(sizeof(M2M2_STATUS_ENUM_t) == 1, INCORRECT_SIZE_M2M2_STATUS_ENUM_t);

// @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
// @@  NOTE: THE FIELDS IN THIS STRUCTURE ARE BIG ENDIAN!  @@
// @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
typedef struct _m2m2_hdr_t {
  M2M2_ADDR_ENUM_t  src; 
  M2M2_ADDR_ENUM_t  dest; 
  uint16_t  length; 
  uint16_t  checksum; 
  uint8_t  data[0]; 
} m2m2_hdr_t;

// Reset struct packing outside of this file
#pragma pack()
