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
 
#include "adi_calendar.h"
#include "nrf.h"
#include "nrf_log.h"
#include <stdint.h>
#include <system_interface.h>

CLK_TZ_TYPE gsTimeZoneOffset = 0;

const uint16_t DaysInYear[2u] = {
    365u, 366u
};

typedef struct _calendar_table_t {
  uint8_t DaysInMonth;
  uint16_t CompletedDaysInYear;
}calendar_table_t;

calendar_table_t calendar [2u][12u] = {
  // Non-Leap year table
  {{THIRTY_ONE_DAYS, 31u}, {TWENTY_EIGHT_DAYS, 59u}, {THIRTY_ONE_DAYS, 90u}, \
    {THIRTY_DAYS, 120u}, {THIRTY_ONE_DAYS, 151u},{THIRTY_DAYS, 181u},\
    {THIRTY_ONE_DAYS, 212u},{THIRTY_ONE_DAYS, 243u},{THIRTY_DAYS, 273u},\
    {THIRTY_ONE_DAYS, 304u},{THIRTY_DAYS, 334u},{THIRTY_ONE_DAYS, 365u}},

    // Leap year table
   {{THIRTY_ONE_DAYS, 31u}, {TWENTY_NINE_DAYS, 60u}, {THIRTY_ONE_DAYS, 91u},\
     {THIRTY_DAYS, 121u}, {THIRTY_ONE_DAYS, 152u},{THIRTY_DAYS, 182u},\
     {THIRTY_ONE_DAYS, 213u},{THIRTY_ONE_DAYS, 244u},{THIRTY_DAYS, 274u},\
     {THIRTY_ONE_DAYS, 305u},{THIRTY_DAYS, 335u},{THIRTY_ONE_DAYS, 366u}}
};

#ifdef DATE_TIME
static uint8_t IsLeapYear (uint16_t  nYear)
{
    uint8_t  nLeapYear;
    nLeapYear = (((nYear % 4u) == 0u) && (((nYear % 100u) != 0u) || ((nYear % 400u) == 0u))) ? 1 : 0;

    return (nLeapYear);
}
#endif

int32_t DateTimeToTs(time_t *pTsSec, m2m2_pm_sys_date_time_req_t *pDateTime)
{
    uint32_t  nTsSec;
#ifdef DATE_TIME
    uint16_t  i;
    uint8_t   nLeapYear = 0;
    uint32_t  nNumDays = 0;
    uint32_t  nTzSecAbs = 0;

    // Find the number of days elapsed from the year of YEAR_START to till last year
    for (i = YEAR_START; i < pDateTime->year; i++) {
        nLeapYear     =  IsLeapYear(i);
        nNumDays   +=  DaysInYear[nLeapYear];
    }

    // Find the days completed in current year upto last month
    nLeapYear    =  IsLeapYear(pDateTime->year);

    // Find the completed days in current year if month is other than January
    if(pDateTime->month > 1)
      nNumDays += calendar[nLeapYear][pDateTime->month - 2].CompletedDaysInYear;

    // Adding current month completed days & remove one day from total days, because
    // for today we are going count number hours elapsed. So the one day should be
    // removed from total number of days
    nNumDays += pDateTime->day - 1;

    nTsSec  = nNumDays          * (24*60*60);
    nTsSec += pDateTime->hour   * (60*60);
    nTsSec += pDateTime->minute * 60;
    nTsSec += pDateTime->second;

    gsTimeZoneOffset = (int32_t) pDateTime->TZ_sec;
    nTzSecAbs = ABS(pDateTime->TZ_sec);

    if ((int32_t)pDateTime->TZ_sec < 0) {
        nTsSec += nTzSecAbs;
        if (nTsSec < nTzSecAbs) {
            return -1;
        }
    } else {
        if (nTsSec < nTzSecAbs) {
            return -1;
        }
        nTsSec -= nTzSecAbs;
    }
#else
    nTsSec = pDateTime->hour   * (60*60);
    nTsSec += pDateTime->minute * 60;
    nTsSec += pDateTime->second;
#endif
   *pTsSec = nTsSec;

    return 0;
}
int32_t TsToDateTime(time_t *pTsSec, ADI_CLK_DATE_TIME *pDateTime)
{
  pDateTime->year = YEAR_START;
  uint32_t nSecToRemove = 31536000;
  uint8_t nNumDaysInMonth = 0;

  /* Taking care of Time Zone conversion */
    if (gsTimeZoneOffset < 0) {
        if (*pTsSec < gsTimeZoneOffset) {
          // Should not get in here
        }
        *pTsSec -= gsTimeZoneOffset;

    } else {
        *pTsSec += gsTimeZoneOffset;
        if (*pTsSec < gsTimeZoneOffset) {
          // Should not get in here
        }
    }
                              /*Find the Year*/
  /*decide the number of seconds elapsed based on wheher it is leap year or not*/
  nSecToRemove = IsLeapYear(pDateTime->year) ? 31622400 : 31536000;
  while (*pTsSec >= nSecToRemove)
  {
    /*decide the number of seconds elapsed based on wheher it is leap year or not*/
    nSecToRemove = IsLeapYear(pDateTime->year) ? 31622400 : 31536000;   /*number of seconds in 365 days = 31536000*/
    *pTsSec -= nSecToRemove;
    INCREMENTING_YEAR(pDateTime);
  }

                              /*Find the Month*/
  pDateTime->month = MONTH_START;
  /*decide the number of seconds elapsed based on wheher it is leap year or not*/
  nNumDaysInMonth = calendar[IsLeapYear(pDateTime->year)][pDateTime->month].DaysInMonth;
  nSecToRemove = nNumDaysInMonth * 86400;       /*number of seconds in a day = 86400*/
  while (*pTsSec >= nSecToRemove && pDateTime->month < 12)
  {
    *pTsSec -= nSecToRemove;
    INCREMENTING_MONTH(pDateTime);
    /*decide the number of seconds elapsed based on wheher it is leap year or not*/
    nNumDaysInMonth = calendar[IsLeapYear(pDateTime->year)][pDateTime->month].DaysInMonth;
    nSecToRemove = nNumDaysInMonth * 86400;
  }
                              /*Find the Day*/
  pDateTime->day = DAY_START;
  pDateTime->day += (*pTsSec / NUM_SEC_PER_DAY);
  nSecToRemove = (*pTsSec % NUM_SEC_PER_DAY);
  *pTsSec  = nSecToRemove;

                              /*Find the hour*/
  pDateTime->hour = (*pTsSec / NUM_SEC_PER_HOUR);
  nSecToRemove = (*pTsSec % NUM_SEC_PER_HOUR);
  *pTsSec  = nSecToRemove;

                              /*Find the min*/
  pDateTime->minute = (*pTsSec / NUM_SEC_PER_MIN);
                              /*Find the sec*/
  pDateTime->second = (*pTsSec % NUM_SEC_PER_MIN);

  pDateTime->TZ_sec = gsTimeZoneOffset;

  return DEF_OK;

}

/** @brief: 
 *    Function for Setting Time Zone.
 */
uint8_t set_time_zone(CLK_TZ_TYPE value)
{
    gsTimeZoneOffset = value;
    NRF_LOG_INFO("Setting time zone:0x%X..\n", gsTimeZoneOffset);
    return 0;
}

/** @brief: 
 *    Function for Getting Time Zone.
 */
uint8_t get_time_zone(CLK_TZ_TYPE *value)
{
    *value = gsTimeZoneOffset;
    NRF_LOG_INFO("Getting time zone:0x%X..\n", gsTimeZoneOffset);
    return 0;
}

/*uint8_t Clk_GetDateTime(ADI_CLK_DATE_TIME* p_date_time)
{
  uint32_t nTicks =0;
  nTicks = HAL_RTC_GetTick();
  TsToDateTime(&nTicks,p_date_time);
  return DEF_OK;
}*/
