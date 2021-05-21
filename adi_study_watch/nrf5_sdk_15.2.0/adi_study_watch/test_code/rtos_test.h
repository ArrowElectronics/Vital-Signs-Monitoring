/********************************************************************************

 **** Copyright (C), 2020, xx xx xx xx info&tech Co., Ltd.                ****

 ********************************************************************************
 * File Name     : rtos_test.c
 * Date          : 2020-07-20
 * Description   : .use to calculate and print the info of stack and CPU usage of each task.
 * Version       : 1.0
 * Function List :
 * 
 * Record        :
 * 1.Date        : 2020-07-20
 *   Modification: Created file

*************************************************************************************************************/
#ifndef _RTOS_TEST_H_
#define _RTOS_TEST_H_

#include "stdint.h"
/*****************************************************************************
 * Function      : rtos_test_init
 * Description   : Task into calculate and print function initialize
 * Input         : void  
 * Output        : None
 * Return        : 
 * Others        : 
 * Record
 * 1.Date        : 20200720
 *   Modification: Created function

*****************************************************************************/
void rtos_test_init(void);
void rtos_test_thread(void * arg);
#endif