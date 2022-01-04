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

#include <stdio.h>
#include "rtos_test.h"
#include "nrf_log.h"

#if NRF_LOG_ENABLED
#include "nrf_log_ctrl.h"
#include "nrf_log_default_backends.h"
#endif

#include <adi_osal.h>
#include <app_cfg.h>
#include <task_includes.h>

#ifdef RTOS_TASK_INFO_PRINT
ADI_OSAL_STATIC_THREAD_ATTR rtos_test_task_attributes;
uint8_t rtos_test_task_stack[APP_OS_CFG_RTOS_TEST_TASK_STK_SIZE];
StaticTask_t rtos_test_task_tcb;
ADI_OSAL_THREAD_HANDLE rtos_task_task_handler;

#define MAX_TASK_NUM        25  
TaskStatus_t pxTaskStatusArray[MAX_TASK_NUM]; 
const char task_state[]={'r','R','B','S','D'};  
/*****************************************************************************
 * Function      : rtos_test_thread
 * Description   : Task info calculate and print function
 * Input         : void * arg  
 * Output        : None
 * Return        : 
 * Others        : 
 * Record
 * 1.Date        : 20200720
 *   Modification: Created function

*****************************************************************************/
void rtos_test_thread(void * arg)
{
    UBaseType_t uxArraySize, x;  
    uint32_t ulTotalRunTime,ulStatsAsPercentage;  

    while(1)
    {
        /* get the num of all tasks */
        uxArraySize = uxTaskGetNumberOfTasks();  
        if(uxArraySize>MAX_TASK_NUM)  
        {  
            NRF_LOG_INFO("task number over,task num = %d",uxArraySize);
        }
        else
        {
            NRF_LOG_INFO("task num = %d",uxArraySize);
        }

        /*get the status info of every task*/
        uxArraySize = uxTaskGetSystemState(pxTaskStatusArray, uxArraySize, &ulTotalRunTime );  

        NRF_LOG_INFO("name,status,ID,prio,stack,cpu");
        NRF_LOG_INFO("ulTotalRunTime = %d",ulTotalRunTime);
        /*prevent the zero error*/
        if( ulTotalRunTime > 0 )  
        {  
            /*transform the task status info to character string*/
            for( x = 0; x < uxArraySize; x++ )  
            {   
                /*Caculate the percent of the task run time  and total run time */
                ulStatsAsPercentage =(uint64_t)(pxTaskStatusArray[ x ].ulRunTimeCounter)*100 / ulTotalRunTime;  

                if( ulStatsAsPercentage > 0UL )  
                {  
                    NRF_LOG_INFO("%-8s%c   %-3d%-5d%-5d%d%%",pxTaskStatusArray[ x].pcTaskName,task_state[pxTaskStatusArray[ x ].eCurrentState],  
                    pxTaskStatusArray[ x ].xTaskNumber,pxTaskStatusArray[ x].uxCurrentPriority,  
                    pxTaskStatusArray[ x ].usStackHighWaterMark,ulStatsAsPercentage);  
                }  
                else  
                {  
                    /*if the percent less than 1%*/
                    NRF_LOG_INFO("%-8s%c   %-3d%-5d%-5dt<1%%",pxTaskStatusArray[x ].pcTaskName,task_state[pxTaskStatusArray[ x ].eCurrentState],  
                    pxTaskStatusArray[ x ].xTaskNumber,pxTaskStatusArray[ x].uxCurrentPriority,  
                    pxTaskStatusArray[ x ].usStackHighWaterMark);                 
                }   
            }  
        }  
        NRF_LOG_INFO("task status r-run R-ready B-block S-susspend D-delete");  
        adi_osal_ThreadSleep(5000);  
    } 

}
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
void rtos_test_init(void) 
{
    ADI_OSAL_STATUS eOsStatus;

    /* Create rtos_test thread */
    rtos_test_task_attributes.pThreadFunc = rtos_test_thread;
    rtos_test_task_attributes.nPriority = APP_OS_CFG_RTOS_TEST_TASK_PRIO;
    rtos_test_task_attributes.pStackBase = &rtos_test_task_stack[0];
    rtos_test_task_attributes.nStackSize = APP_OS_CFG_RTOS_TEST_TASK_STK_SIZE;
    rtos_test_task_attributes.pTaskAttrParam = NULL;
    /* Thread Name should be of max 10 Characters */
    rtos_test_task_attributes.szThreadName = "Rtos_Test";
    rtos_test_task_attributes.pThreadTcb = &rtos_test_task_tcb;

    eOsStatus = adi_osal_ThreadCreateStatic(&rtos_task_task_handler,
                                      &rtos_test_task_attributes);
    if (eOsStatus != ADI_OSAL_SUCCESS) 
    {
        Debug_Handler();
    }
}
#endif
