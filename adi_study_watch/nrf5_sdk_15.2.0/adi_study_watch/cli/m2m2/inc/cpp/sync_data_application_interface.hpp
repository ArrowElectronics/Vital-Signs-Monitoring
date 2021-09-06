// #############################################################################
// THIS IS AN AUTOMATICALLY GENERATED FILE, DO NOT MODIFY IT!
// #############################################################################
#pragma once

#include "common_application_interface.hpp"
#include "m2m2_core.hpp"
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


enum M2M2_SENSOR_SYNC_DATA_TYPES_ENUM_t:uint8_t {
  M2M2_SENSOR_ADPD_ADXL_SYNC_DATA = 0,
};
static_assert(sizeof(M2M2_SENSOR_SYNC_DATA_TYPES_ENUM_t) == 1, "Enum 'M2M2_SENSOR_SYNC_DATA_TYPES_ENUM_t' has an incorrect size!");

struct adpd_adxl_sync_data_format_t {
  uint32_t  ppgTS; 
  uint32_t  adxlTS; 
  uint16_t  incPpgTS[3]; 
  uint16_t  incAdxlTS[3]; 
  uint32_t  ppgData[4]; 
  int16_t  xData[4]; 
  int16_t  yData[4]; 
  int16_t  zData[4]; 
};

struct adpd_adxl_sync_data_stream_t {
  uint8_t  command; 
  uint8_t  status; 
  uint16_t  sequence_num; 
  adpd_adxl_sync_data_format_t  syncData; 
};

// Reset struct packing outside of this file
#pragma pack()
