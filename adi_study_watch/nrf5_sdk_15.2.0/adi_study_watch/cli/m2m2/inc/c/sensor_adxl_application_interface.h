// #############################################################################
// THIS IS AN AUTOMATICALLY GENERATED FILE, DO NOT MODIFY IT!
// #############################################################################
#pragma once

#include "common_application_interface.h"
#include "common_sensor_interface.h"
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


typedef enum M2M2_SENSOR_ADXL_COMMAND_ENUM_t {
  _M2M2_SENSOR_ADXL_COMMAND_LOWEST = 64,
  M2M2_SENSOR_ADXL_COMMAND_LOAD_CFG_REQ = 66,
  M2M2_SENSOR_ADXL_COMMAND_LOAD_CFG_RESP = 67,
  M2M2_SENSOR_ADXL_COMMAND_SELF_TEST_REQ = 68,
  M2M2_SENSOR_ADXL_COMMAND_SELF_TEST_RESP = 69,
} M2M2_SENSOR_ADXL_COMMAND_ENUM_t;
STATIC_ASSERT_PROJ(sizeof(M2M2_SENSOR_ADXL_COMMAND_ENUM_t) == 1, INCORRECT_SIZE_M2M2_SENSOR_ADXL_COMMAND_ENUM_t);

typedef enum M2M2_SENSOR_ADXL_RAW_DATA_TYPES_ENUM_t {
  M2M2_SENSOR_RAW_DATA_ADXL = 0,
} M2M2_SENSOR_ADXL_RAW_DATA_TYPES_ENUM_t;
STATIC_ASSERT_PROJ(sizeof(M2M2_SENSOR_ADXL_RAW_DATA_TYPES_ENUM_t) == 1, INCORRECT_SIZE_M2M2_SENSOR_ADXL_RAW_DATA_TYPES_ENUM_t);

typedef enum M2M2_SENSOR_ADXL_NSAMPLES_ENUM_t {
  M2M2_SENSOR_ADXL_NSAMPLES_NO_COMPRESS = 5,
} M2M2_SENSOR_ADXL_NSAMPLES_ENUM_t;
STATIC_ASSERT_PROJ(sizeof(M2M2_SENSOR_ADXL_NSAMPLES_ENUM_t) == 1, INCORRECT_SIZE_M2M2_SENSOR_ADXL_NSAMPLES_ENUM_t);

typedef enum M2M2_SENSOR_ADXL_DEVICE_ID_ENUM_t {
  M2M2_SENSOR_ADXL_DEVICE_362 = 362,
} M2M2_SENSOR_ADXL_DEVICE_ID_ENUM_t;
STATIC_ASSERT_PROJ(sizeof(M2M2_SENSOR_ADXL_DEVICE_ID_ENUM_t) == 2, INCORRECT_SIZE_M2M2_SENSOR_ADXL_DEVICE_ID_ENUM_t);

typedef struct _m2m2_sensor_adxl_resp_t {
  uint8_t  command; 
  uint8_t  status; 
  M2M2_SENSOR_ADXL_DEVICE_ID_ENUM_t  deviceid; 
} m2m2_sensor_adxl_resp_t;

typedef struct _adxl_no_compress_format_t {
  uint16_t  incTS; 
  int16_t  xdata; 
  int16_t  ydata; 
  int16_t  zdata; 
} adxl_no_compress_format_t;

typedef struct _adxl_data_header_t {
  uint8_t  command; 
  uint8_t  status; 
  uint16_t  sequence_num; 
  M2M2_SENSOR_ADXL_RAW_DATA_TYPES_ENUM_t  data_type; 
  uint32_t  timestamp; 
  int16_t  first_xdata; 
  int16_t  first_ydata; 
  int16_t  first_zdata; 
} adxl_data_header_t;

typedef struct _m2m2_sensor_adxl_data_no_compress_stream_t {
  uint8_t  command; 
  uint8_t  status; 
  uint16_t  sequence_num; 
  M2M2_SENSOR_ADXL_RAW_DATA_TYPES_ENUM_t  data_type; 
  uint32_t  timestamp; 
  int16_t  first_xdata; 
  int16_t  first_ydata; 
  int16_t  first_zdata; 
  adxl_no_compress_format_t  adxldata[4]; 
} m2m2_sensor_adxl_data_no_compress_stream_t;

// Reset struct packing outside of this file
#pragma pack()
