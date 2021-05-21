/******************************************************************************
 * Copyright (c) 2019 Analog Devices, Inc.
 * All rights reserved.
 *****************************************************************************/

#include "stdint.h"
#include "adpd_common.h"
#include "printf.h"
#include "hw_if_config.h"
#include <m2m2_core.h>
#include <debug_interface.h>
#include <post_office.h>

typedef char* va_list_pf;
/* --------------------------- definitions -------------------------------- */
#define __va_argsiz(t)      (((sizeof(t) + sizeof(int) - 1) / sizeof(int)) * sizeof(int))
#define va_start_pf(ap,pN)  ((ap) = ((va_list_pf) (&pN) + __va_argsiz(pN)))
#define va_end_pf(ap)       ((void)0)
#define va_arg_pf(ap,t)     (((ap) = (ap) + __va_argsiz(t)), *((t*) (void*) ((ap) - __va_argsiz(t))))

char _SBZ[BUF_SIZE];

/* ------------------------- Function Prototype --------------------------- */

void adi_printf (uint8_t *FmtStrg, ...);
uint8_t adi_strcpy (uint8_t *dst, uint8_t *src);
void adi_memset(uint16_t *src, uint16_t setValue, uint16_t count);
uint8_t *BinToAsciiDecimal (uint8_t *Buf, uint32_t Num);

uint16_t adi_memcmp(const void * cs,const void * ct, uint16_t count);
uint16_t adi_memcpy(void *dst, void *src, uint32_t count);
uint8_t adi_strncmp(uint8_t *strg1, uint8_t *strg2, uint8_t n);
uint8_t adi_strcat(uint8_t *dst, uint8_t *src);
uint16_t adi_strlen (uint8_t *Strg);

void printhex (uint32_t Num, uint32_t Size);
void print_4bit_hex (uint8_t ch);
void m2m2_printf (uint8_t *str_buff);
static void UartSendCharString(uint8_t *str);
static void UartSendByte(uint8_t ch);

static uint8_t gsDebStr[BUF_SIZE + M2M2_HEADER_SZ] = {0};

/**
    * @brief    utility function.
    * @param    -
    * @retval   -
    */
uint8_t *BinToAsciiDecimal (uint8_t *Buf, uint32_t Num) {
    uint32_t Div = 1000000000;
    uint32_t Digit;
    uint32_t Found=0;

    while (Div) {
        Digit = Num / Div;
        Num-= (Digit * Div);
        Div = Div / 10;
        if (Digit) {
            Found = 1;
        }
        if (Found) {
            *Buf= ((char)Digit) + '0';
            Buf++;
        }
    }
    if (!Found) {
        *Buf='0';
        Buf++;
    }
    return (Buf);
}

/**
    * @brief    utility function.
    * @param    -
    * @retval   -
    */
void print_4bit_hex (uint8_t ch) {
    if (ch > 9) {
        ch-= 10;
        ch+= 'a';
    } else {
        ch+= '0';
    }
    UartSendByte(ch);
}

/**
    * @brief    utility function.
    * @param    -
    * @retval   -
    */
void printhex (uint32_t Num, uint32_t Size) {
    int i, j=0;
    uint8_t ch;

    if (Size > 8) {
        Size = 8;
    }
    Size = 8 - Size;
    for (i=28; i>=0; i-=4) {
        ch = (uint8_t)((((uint32_t)Num)>>i) & 0xf);
        if (ch || (Size == 0)) {
            j=1;
        }
        if (j || (i==0)) {
            print_4bit_hex(ch);
        }
        if (Size) {
            Size--;
        }
    }
}

/**
    * @brief    utility function.
    * @param    -
    * @retval   -
    */
void adi_printf (uint8_t *FmtStrg, ...) {
    va_list_pf vl;
    uint32_t i, j, Wid;
    uint8_t *Cptr, *EndPtr, Buf[12];
	uint8_t Ch, tempUChar;
    static const uint8_t Zeros[]="000000000";      /* Must be 9 zeros */

    memset(gsDebStr, 0, BUF_SIZE + M2M2_HEADER_SZ);
    /*=======================================================
     * FmtStrg is the last argument specified. All others
     * must be accessed using the variable-argument macros.
     *=======================================================*/
    Cptr = FmtStrg;
    va_start_pf (vl, FmtStrg);

    /*================================
     * Step through the list.
     *===============================*/
    for (i=0; *Cptr; i++) {
        Ch = *Cptr;
        Cptr++;
        if (Ch != '%') {
            UartSendByte(Ch);
            continue;
        }
        /*========================
         * Found '%'
         *=======================*/
        Ch = *Cptr;
        Cptr++;
        if (!Ch) {
            break;
        }
        if (Ch == '0') {
            Ch = *Cptr;
            Cptr++;
            if (!Ch)
                break;
        }
        Wid = 0;
        if ((Ch >= '0') && (Ch <= '9')) {
            Wid = (uint32_t)(Ch - '0') & 0xff;
            Ch = *Cptr;
            Cptr++;
        }
        if (Ch == 'l') {
            Ch = *Cptr;
            Cptr++;
        }
        switch (Ch) {
            case 'd':
            case 'u':
                EndPtr = BinToAsciiDecimal (Buf, (va_arg_pf (vl, int)));  /*  *(UINT32 *)(va_arg_pf (vl, int))); */
                *EndPtr = 0;
                j = (uint32_t) (EndPtr - Buf);
                if (Wid > j) {
                    Wid-= j;
                    UartSendCharString ((uint8_t *)(Zeros+9-Wid));
                }
                UartSendCharString ((uint8_t *)Buf);
                break;

            case 'x':
            case 'p':
                printhex (va_arg_pf (vl, int), Wid);
                break;

            case 'c':
                tempUChar = va_arg_pf (vl, char);
                /* UartSendByte ((va_arg (vl, char)); */
                UartSendByte (tempUChar);
                break;
            case 's':
                UartSendCharString ((uint8_t *)(va_arg_pf (vl, char *)));
                break;

            default:
                break;
        }
   }
   va_end_pf (vl);
   m2m2_printf(&gsDebStr[0]);
}

static void UartSendByte(uint8_t ch)  {
  strncat((char*)gsDebStr, (char*)&ch, 1);
}

static void UartSendCharString(uint8_t *str)  {
  // check size
  strcat((char*)gsDebStr, (char*)str);
}

/**
    * @brief    utility function.
    * @param    -
    * @retval   -
    */
uint16_t adi_memcmp(const void * cs,const void * ct, uint16_t count) {
    const uint8_t *su1, *su2;
    uint16_t res = 0;

    for(su1 = cs, su2 = ct; 0 < count; ++su1, ++su2, count--)
    if ((res = *su1 - *su2) != 0) {
        break;
    }
    return (res);
}

/**
    * @brief    utility function.
    * @param    -
    * @retval   -
    */
void adi_memset(uint16_t *src, uint16_t setValue, uint16_t count) {
    count >>= 1;
    while (count--)
        *src++ = setValue;
}

/**
    * @brief    utility function.
    * @param    -
    * @retval   -
    */
uint8_t adi_strcpy (uint8_t *dst, uint8_t *src) {
    while (*src) {
        *dst = *src;
        dst++;
        src++;
    }
    *dst = 0;
    return (1);
}

/**
    * @brief    utility function.
    * @param    -
    * @retval   -
    */
uint8_t adi_strcat (uint8_t *dst, uint8_t *src) {
    while (*dst)
        dst++;

    while (*src) {
        *dst = *src;
        dst++;
        src++;
    }
    *dst = 0;
    return (1);
}

/**
    * @brief    utility function.
    * @param    -
    * @retval   -
    */
uint16_t adi_strlen (uint8_t *Strg) {
    uint16_t i=0;

    while (*Strg) {
        Strg++;
        i++;
    }
    return (i);
}


/**
    * @brief    Convert a character to Integer
    * @param    -
    * @retval   -
    */
uint8_t CharToInt(uint8_t ch) {

    if (ch <= '9')
        return (ch-'0');
    else if (ch >= 'a')
        return (ch-'a'+10);
    else
        return (ch-'A'+10);
}

/**
    * @brief    Convert characters to a 32-bit Integer
    * @param    -
    * @retval   -
    */
uint32_t IncCharsToInt32(uint8_t *ch) {

    uint32_t retVal=0;

    while (1) {
        if (*ch == '\0')
            break;
        retVal += *ch-'0';
        retVal *= 10;
        ch++;
    }
    retVal /= 10;
    return retVal;
}

/**
    * @brief    Convert one/two hex character to a 16-bit Integer
    * @param    -
    * @retval   -
    */
uint8_t HexCharsToInt8(uint8_t *ch) {
    uint8_t retVal;

    if (*ch <= '9')
        retVal = *ch-'0';
    else if (*ch >= 'a')
        retVal = *ch-'a'+10;
    else
        retVal = *ch-'A'+10;

    if (*(++ch) == '\0')
        return retVal;

    retVal *= 16;

    if (*ch <= '9')
        retVal += *ch-'0';
    else if (*ch >= 'a')
        retVal += *ch-'a'+10;
    else
        retVal += *ch-'A'+10;

  return retVal;
}

/**
    * @brief    Convert one-four hex character to a 32-bit Integer
    * @param    -
    * @retval   -
    */
uint16_t HexCharsToInt16(uint8_t *ch) {
    uint16_t retVal, temp;

    retVal = 0;
    while (1) {
        if (*ch <= '9')
            temp = *ch-'0';
        else if (*ch >= 'a')
            temp = *ch-'a'+10;
        else
            temp = *ch-'A'+10;
        retVal += temp;
        if (*(++ch) == '\0')
            break;
        retVal *= 16;
    }
    return retVal;
}

void m2m2_printf (uint8_t *str_buff) {
  ADI_OSAL_STATUS         err;

  uint8_t str_len = strlen((char const*)str_buff);
  // uint8_t dbg_pkt[127];   // max size
  m2m2_hdr_t *pDbgPkt = NULL;

  if (str_len > 80)
    str_len = 80;    // max size

  pDbgPkt = post_office_create_msg(M2M2_HEADER_SZ + str_len);

  // m2m2_hdr_t *pDbgPkt = (m2m2_hdr_t *)(&dbg_pkt[0]);

  pDbgPkt->src = M2M2_ADDR_SYS_PM;
  pDbgPkt->dest = M2M2_ADDR_SYS_DBG_STREAM;
  pDbgPkt->length = str_len + M2M2_HEADER_SZ;

  memcpy(&(pDbgPkt->data[0]), str_buff, str_len);
  post_office_send(pDbgPkt, &err);
}
