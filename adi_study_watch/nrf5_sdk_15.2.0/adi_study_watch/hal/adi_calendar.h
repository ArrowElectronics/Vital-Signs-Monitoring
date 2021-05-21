/* Copyright (c) 2014 Nordic Semiconductor. All Rights Reserved.
 *
 * The information contained herein is property of Nordic Semiconductor ASA.
 * Terms and conditions of usage are described in detail in NORDIC
 * SEMICONDUCTOR STANDARD SOFTWARE LICENSE AGREEMENT.
 *
 * Licensees are granted free, non-transferable use of the information. NO
 * WARRANTY of ANY KIND is provided. This heading must NOT be removed from
 * the file.
 *
 */

#ifndef __NRF_CALENDAR_H__
#define __NRF_CALENDAR_H__

#include <stdint.h>
#include <stdbool.h>
#include "time.h"
#include <stdint.h>
#include <printf.h>
#include "system_interface.h"

#define  DATE_TIME                                         1u
#define  DEF_FAIL                                          0u
#define  DEF_OK                                            1u

#define TOTAL_MONTHS_IN_YEAR                               12u
#define TOTAL_HOURS_IN_A_DAY                               24u
#define TOTAL_HOURS_IN_HALF_DAY                            12u
#define MONTH_START                                        1u
#define DAY_START                                          0u
#define YEAR_START                                         1970u
#define NUM_SEC_PER_MIN                                    60u
#define NUM_SEC_PER_HOUR                                   3600u
#define NUM_SEC_PER_DAY                                    86400u    // 24 * NUM_SEC_PER_HOUR
#define NUM_SEC_PER_YEAR                                   31536000u // 365 * NUM_SEC_PER_DAY (except leap year)
#define NUM_SEC_PER_LEAP_YEAR                              31622400u
#define TWENTY_EIGHT_DAYS                                  28u
#define TWENTY_NINE_DAYS                                   29u
#define THIRTY_DAYS                                        30u
#define THIRTY_ONE_DAYS                                    31u


#define INCREMENTING_YEAR(P_CALENDER)                      P_CALENDER->year++
#define INCREMENTING_MONTH(P_CALENDER)                     P_CALENDER->month++
typedef  uint32_t  CLK_TS_SEC;
typedef int32_t CLK_TZ_TYPE;

typedef struct _date_time_type {
  uint16_t  year;
  uint8_t  month;
  uint8_t  day;
  uint8_t  hour;
  uint8_t  minute;
  uint8_t  second;
  int32_t  TZ_sec;
}ADI_CLK_DATE_TIME;

int32_t DateTimeToTs(time_t *pTsSec, m2m2_pm_sys_date_time_req_t *pDateTime);
int32_t TsToDateTime(time_t *pTsSec, ADI_CLK_DATE_TIME *pDateTime);
//uint8_t Clk_GetDateTime(ADI_CLK_DATE_TIME* p_date_time);
//void  Clk_ExtTS_Init (void);
//void BSP_ClockSetTS (uint32_t ts_sec);
uint8_t set_time_zone(CLK_TZ_TYPE value);
uint8_t get_time_zone(CLK_TZ_TYPE *value);

#endif
