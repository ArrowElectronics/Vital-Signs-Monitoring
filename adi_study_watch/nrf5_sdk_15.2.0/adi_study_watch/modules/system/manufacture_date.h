#ifndef __MANUFACTURE_DATE_H__
#define __MANUFACTURE_DATE_H__

#include "stdint.h"

typedef struct{
    uint16_t year;
    uint8_t month;
    uint8_t day;
}manufacture_date_t; 

/* 
  * @brief: Function to save the manufacture date.
*/
uint32_t manufacture_date_save(manufacture_date_t *date);
/* 
  * @brief: Function to read the manufacture date.
*/
uint32_t manufacture_date_read(manufacture_date_t *date);



#endif 
