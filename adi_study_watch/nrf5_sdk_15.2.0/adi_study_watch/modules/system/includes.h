/*
*********************************************************************************************************
*                                              EXAMPLE CODE
*
*                             (c) Copyright 2014; Micrium, Inc.; Weston, FL
*
*                   All rights reserved.  Protected by international copyright laws.
*                   Knowledge of the source code may not be used to write a similar
*                   product.  This file may only be used in accordance with a license
*                   and should not be redistributed in any way.
*********************************************************************************************************
*/

/*
*********************************************************************************************************
*
*                                           MASTER INCLUDES
*
* Filename      : includes.h
* Version       : V1.00
* Programmer(s) : DC
*********************************************************************************************************
*/

#ifndef  INCLUDES_PRESENT
#define  INCLUDES_PRESENT


/*
*********************************************************************************************************
*                                         STANDARD LIBRARIES
*********************************************************************************************************
*/

#include  <stdarg.h>
#include  <stdio.h>
#include  <stdlib.h>
#include  <math.h>


/*
*********************************************************************************************************
*                                              LIBRARIES
*********************************************************************************************************
*/

/*
*********************************************************************************************************
*                                              APP / BSP
*********************************************************************************************************
*/


#include  <app_cfg.h>
#include  <adi_osal.h>
#include  <task_includes.h>
#include  <memory_management.h>
#include  <post_office.h>

#include  <led_interface.h>

extern ADI_OSAL_MUTEX_HANDLE LEDControlLock;

/*
*********************************************************************************************************
*                                              Functions
*********************************************************************************************************
*/
/*!
 * @brief:  Function sends m2m2 command to set LED pattern to the LED task.
 */
ADI_OSAL_STATUS set_led_pattern(M2M2_ADDR_ENUM_t src,
                                M2M2_LED_PATTERN_ENUM_t r_pattern,
                                M2M2_LED_PATTERN_ENUM_t g_pattern);
void get_led_pattern(M2M2_LED_PATTERN_ENUM_t *r_pattern,
                     M2M2_LED_PATTERN_ENUM_t *g_pattern);

/*
*********************************************************************************************************
*                                            INCLUDES END
*********************************************************************************************************
*/

#endif