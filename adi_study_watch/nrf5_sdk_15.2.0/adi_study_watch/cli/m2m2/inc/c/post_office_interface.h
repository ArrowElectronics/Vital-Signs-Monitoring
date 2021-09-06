// #############################################################################
// THIS IS AN AUTOMATICALLY GENERATED FILE, DO NOT MODIFY IT!
// #############################################################################
#pragma once

#include "m2m2_core.h"
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


typedef enum POST_OFFICE_CFG_CMD_ENUM_t {
  POST_OFFICE_CFG_CMD_ADD_MAILBOX = 1,
  POST_OFFICE_CFG_CMD_REMOVE_MAILBOX = 2,
  POST_OFFICE_CFG_CMD_MAILBOX_SUBSCRIBE = 3,
  POST_OFFICE_CFG_CMD_MAILBOX_UNSUBSCRIBE = 4,
} POST_OFFICE_CFG_CMD_ENUM_t;
STATIC_ASSERT_PROJ(sizeof(POST_OFFICE_CFG_CMD_ENUM_t) == 1, INCORRECT_SIZE_POST_OFFICE_CFG_CMD_ENUM_t);

typedef struct _post_office_config_t {
  uint8_t  cmd; 
  uint16_t  box; 
  uint16_t  sub; 
} post_office_config_t;

// Reset struct packing outside of this file
#pragma pack()
