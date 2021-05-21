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

enum M2M2_WDT_CMD_ENUM_t:uint8_t {
  M2M2_WDT_WDT_CMD_LOWEST = 192,
  M2M2_WDT_WDT_TASK_BROADCAST_CMD = 193,
  M2M2_WDT_PM_IS_ALIVE_CMD = 194,
  M2M2_WDT_PS_IS_ALIVE_CMD = 195,
};
static_assert(sizeof(M2M2_WDT_CMD_ENUM_t) == 1, "Enum 'M2M2_WDT_CMD_ENUM_t' has an incorrect size!");

enum M2M2_WDT_STATUS_ENUM_t:uint8_t {
  M2M2_WDT_STATUS_OK = 0,
  M2M2_WDT_STATUS_ERROR = 1,
};
static_assert(sizeof(M2M2_WDT_STATUS_ENUM_t) == 1, "Enum 'M2M2_WDT_STATUS_ENUM_t' has an incorrect size!");

struct m2m2_wdt_cmd_t {
  M2M2_WDT_CMD_ENUM_t  command; 
  M2M2_WDT_STATUS_ENUM_t  status; 
};

// Reset struct packing outside of this file
#pragma pack()
