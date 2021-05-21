/*
Copyright (c) 2012 Analog Devices, Inc.

All rights reserved.

*/

#ifndef _COMMON_H_
#define _COMMON_H_
#include <stdint.h>
#include <string.h>
#include <stdio.h>

#include "hw_if_config.h"

/* --------------------------- definitions -------------------------------- */
#ifndef ABS
#define ABS(i_x) (((i_x) > 0) ? (i_x): -(i_x))
#endif

/* ------------------------- Global typedef ------------------------------- */

/* ------------------------- Global Variable ------------------------------ */

/* ------------------------- Function Prototype --------------------------- */
extern void adi_printf (uint8_t*, ...);
extern uint8_t adi_strcpy (uint8_t*, uint8_t*);
extern uint8_t adi_strcat (uint8_t *dst, uint8_t *src);
extern uint8_t adi_strncmp(uint8_t *strg1, uint8_t *strg2, uint8_t n);
extern void adi_memset(uint16_t  *src, uint16_t  setValue, uint16_t  count);
extern uint8_t  CharToInt(uint8_t  ch);
extern uint32_t IncCharsToInt32(uint8_t *ch);
extern uint8_t  HexCharsToInt8(uint8_t  *ch);
extern uint16_t HexCharsToInt16(uint8_t *ch);

#endif
