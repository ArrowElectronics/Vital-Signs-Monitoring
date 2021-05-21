// #############################################################################
// THIS IS AN AUTOMATICALLY GENERATED FILE, DO NOT MODIFY IT!
// #############################################################################
#pragma once

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

enum POST_OFFICE_CFG_CMD_ENUM_t:uint8_t {
  POST_OFFICE_CFG_CMD_ADD_MAILBOX = 1,
  POST_OFFICE_CFG_CMD_REMOVE_MAILBOX = 2,
  POST_OFFICE_CFG_CMD_MAILBOX_SUBSCRIBE = 3,
  POST_OFFICE_CFG_CMD_MAILBOX_UNSUBSCRIBE = 4,
};
static_assert(sizeof(POST_OFFICE_CFG_CMD_ENUM_t) == 1, "Enum 'POST_OFFICE_CFG_CMD_ENUM_t' has an incorrect size!");

struct post_office_config_t {
  POST_OFFICE_CFG_CMD_ENUM_t  cmd; 
  M2M2_ADDR_ENUM_t  box; 
  M2M2_ADDR_ENUM_t  sub; 
};

// Reset struct packing outside of this file
#pragma pack()
