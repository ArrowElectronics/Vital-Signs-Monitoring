/* $Revision: 29132 $
 * $Date: 2014-12-10 07:39:40 -0500 (Wed, 10 Dec 2014) $
******************************************************************************
Copyright (c), 2008-2011 - Analog Devices Inc. All Rights Reserved.
This software is PROPRIETARY & CONFIDENTIAL to Analog Devices, Inc.
and its licensors.
*****************************************************************************/

/*!
    @file adi_osal_ucos_thread_queue.c

    Operating System Abstraction Layer - OSAL for uCOS-III - Thread
    functions

    This file contains the Thread APIs for the uCOS-III implementation of
    OSAL

*/

/*=============  I N C L U D E S   =============*/

#include <adi_osal.h>
#include "osal_ucos.h"
#include "osal_common.h"
//#include <os_cpu.h>
//#include <cpu.h>
#include <string.h> 
#include <FreeRTOS.h>
#include <queue.h>
#include <task.h>

#ifdef _MISRA_RULES
/*Rule 14.7 indicates that a function shall have a single exit point */
#pragma diag(suppress:misra_rule_14_7:"Allowing several point of exit (mostly for handling parameter error checking) increases the code readability and therefore maintainability")
/* Rule-5.1 indicates that all identifiers shall not rely on more than 31 characters of significance */
#pragma diag(suppress:misra_rule_5_1:"prefixes added in front of identifiers quickly increases their size. In order to keep the code self explanatory, and since ADI tools are the main targeted tools, this restriction is not really a necessity")
/* Rule-11.3 indicates that typecast of a integer value into pointer is invalid */
#pragma diag(suppress:misra_rule_11_3 : "typecasting is necessary every time a predefined value is written to a return pointer during error conditions")
#pragma diag(suppress:misra_rule_6_3 : "We need an int8 which gives problems in SHARC")
#pragma diag(suppress:misra_rule_10_1_a : "OS_OPT_TIME_DLY is a different type from what OSTimeFly needs")
#endif

/* In uCOS the macro CPU_CFG_STK_GROWTH specifies in which direction the stack
 * grows. In this implementation we assume downwards (as it happens in ADI
 * current processors) so CPU_CFG_STK_GROWTH must be CPU_STK_GROWTH_HI_TO_LO.
 * If this is not the case the code needs to be reimplemented.
 */
#if (CPU_CFG_STK_GROWTH != CPU_STK_GROWTH_HI_TO_LO)
#error This OSAL implementation relies on the stack growing downwards
#endif

#if (ADI_OSAL_MAX_NUM_TLS_SLOTS>32u)
#error "ADI_OSAL_MAX_THREAD_SLOTS must be less than 32 because it is used as a bitmask to store TLS state"
#endif
/*=============  D E F I N E S  =============*/

#if defined (__ECC__)
#pragma file_attr(  "libGroup=adi_osal.h")
#pragma file_attr(  "libName=libosal")
#pragma file_attr(  "prefersMem=any")
#pragma file_attr(  "prefersMemNum=50")
#endif



/*=============  D A T A  =============*/




/*=============  C O D E  =============*/
/*!
  ****************************************************************************
    @brief Sends a message to the specified task message queue.

    @param[in] hThread     - Handle of the task queue to use.
    @param[in] pMsg      - Pointer to the message to send

    @return ADI_OSAL_SUCCESS    - If message queued successfully
    @return ADI_OSAL_FAILED     - If failed to queue the message
    @return ADI_OSAL_BAD_HANDLE - If the specified message queue handle is 
                                  invalid
    @return ADI_OSAL_QUEUE_FULL - If queue is full
*****************************************************************************/

ADI_OSAL_STATUS  adi_osal_ThreadQueuePost(ADI_OSAL_THREAD_HANDLE const hThread, ADI_OSAL_QUEUE_OPTS nOption, void *pMsg, uint32_t nMsgSize)
{
    OS_ERR nErr = OS_ERR_NONE;
    ADI_OSAL_STATUS eRetStatus;

#ifdef _MISRA_RULES
#pragma diag(push)
#pragma diag(suppress:misra_rule_11_4 : "typecasting is necessary to convert the handle type into a useful type")
#endif
    ADI_OSAL_THREAD_INFO_PTR hThreadNode = (ADI_OSAL_THREAD_INFO_PTR) hThread;
    OS_TCB *hCurrNativeThread;
    OS_OPT option = OS_OPT_POST_FIFO;

#ifdef OSAL_DEBUG
    /* check validity of the handle */
    if ((NULL == hThreadNode) || (ADI_OSAL_INVALID_THREAD == hThread))
    {
        return (ADI_OSAL_BAD_HANDLE);
    }

    if ( !_adi_osal_IsOSALThread(hThreadNode) )
    {
        return (ADI_OSAL_BAD_HANDLE);
    }

#endif

    eRetStatus = adi_osal_ThreadGetNativeHandle((void**) (&hCurrNativeThread));
    if (ADI_OSAL_SUCCESS != eRetStatus)
    {
        return (ADI_OSAL_FAILED);
    }

#ifdef _MISRA_RULES
#pragma diag(pop)
#endif

    if ((nOption & ADI_OSAL_OPT_POST_LIFO) == ADI_OSAL_OPT_POST_LIFO) {
        option |= OS_OPT_POST_LIFO;
    }
    if ((nOption & ADI_OSAL_OPT_POST_NO_SCHED) == ADI_OSAL_OPT_POST_NO_SCHED) {
        option |= OS_OPT_POST_NO_SCHED;
    }

    OSTaskQPost(&(hThreadNode->pNativeThread),
                pMsg, 
                (OS_MSG_SIZE)nMsgSize, /*size of message*/
                option, 
                &nErr);

    switch (nErr)
    {
        case OS_ERR_NONE:
            eRetStatus = ADI_OSAL_SUCCESS;
            break;
#ifdef OSAL_DEBUG
        /* we give the same error if the queue was full or if there were no
           OS_MSG left */
        case OS_ERR_MSG_POOL_EMPTY: /* FALLTHROUGH */
        case OS_ERR_Q_MAX:
            eRetStatus = ADI_OSAL_QUEUE_FULL ;
            break;

        case OS_ERR_OBJ_TYPE:     /* FALLTHROUGH */
        case OS_ERR_OBJ_PTR_NULL:
            eRetStatus = ADI_OSAL_BAD_HANDLE;
            break;
#endif /* OSAL_DEBUG */
        default :
            eRetStatus =  ADI_OSAL_FAILED;
            break;
    } /* end of switch */

    return( eRetStatus );
}


/*!
  ****************************************************************************
    @brief Receives a message from the specified task message queue.

    @param[in]  hThread             -  handle of the task queue to retrieve 
                                     the message from
    @param[out] ppMsg             -  Pointer to a location to store the message
    @param[in]  nTimeoutInTicks   -  Timeout in system ticks for retrieving the
                                     message.

      Valid timeouts are:

         ADI_OSAL_TIMEOUT_NONE     -   No wait. Results in an immediate return
                                       from this service regardless of whether
                                       or not it was successful

         ADI_OSAL_TIMEOUT_FOREVER  -   suspends the calling thread indefinitely
                                       until a message is obtained

         1 ... 0xFFFFFFFE          -   Selecting a numeric value specifies the
                                       maximum time limit (in system ticks) for
                                       obtaining a message from the queue

    @return ADI_OSAL_SUCCESS      - If message is received and copied to ppMsg 
                                    buffer and removed from queue.
    @return ADI_OSAL_FAILED       - If failed to get a message.
    @return ADI_OSAL_TIMEOUT      - If failed get message due to timeout.
    @return ADI_OSAL_BAD_HANDLE   - If the specified message queue is invalid
    @return ADI_OSAL_CALLER_ERROR - If the function is invoked from an invalid 
                                    location (i.e an ISR)
*****************************************************************************/

ADI_OSAL_STATUS  adi_osal_ThreadQueuePend(void **ppMsg, uint32_t *pnMsgSize, ADI_OSAL_TICKS nTimeoutInTicks)
{
//    OS_ERR nErr = OS_ERR_NONE;
    ADI_OSAL_STATUS eRetStatus;
//    OS_MSG_SIZE nOsMsgSize;
//    OS_OPT eBlockingOption = OS_OPT_NONE;
    void * pMsg = NULL;
    TaskHandle_t *hCurrNativeThread = NULL;
    BaseType_t nErr;
#ifdef OSAL_DEBUG
    if((nTimeoutInTicks > ADI_OSAL_MAX_TIMEOUT) && 
       (nTimeoutInTicks != ADI_OSAL_TIMEOUT_FOREVER))
    {
         return (ADI_OSAL_BAD_TIME);
    }

    if ((CALLED_FROM_AN_ISR) || (CALLED_IN_SCHED_LOCK_REGION) ||(CALLED_BEFORE_OS_RUNNING))
    {
        return (ADI_OSAL_CALLER_ERROR);
    }

#endif
    eRetStatus = adi_osal_ThreadGetNativeHandle((void**) (hCurrNativeThread));
    if (ADI_OSAL_SUCCESS != eRetStatus)
    {
        return (ADI_OSAL_FAILED);
    }
    
//    switch (nTimeoutInTicks)
//    {
//        case ADI_OSAL_TIMEOUT_NONE:
//            nTimeoutInTicks = 0u;
//            eBlockingOption  += OS_OPT_PEND_NON_BLOCKING;
//            break;
//        case ADI_OSAL_TIMEOUT_FOREVER:
//            nTimeoutInTicks = 0u;
//            eBlockingOption  += OS_OPT_PEND_BLOCKING;
//            break;
//        default:
//            eBlockingOption  += OS_OPT_PEND_BLOCKING;
//            break;
//    } /* end of switch */

	nErr = xQueueReceive(hCurrNativeThread,ppMsg,nTimeoutInTicks);
//						eBlockingOption, 
//						&nOsMsgSize, 
//						NULL,             /*timestamp not required */
//						&nErr);
 
    switch (nErr)
    {
        case pdTRUE:
            *ppMsg = pMsg;
//            *pnMsgSize = (uint32_t)nOsMsgSize;
            eRetStatus = ADI_OSAL_SUCCESS;
            break;

//        /* We specified non-blocking pend but the call would have blocked. 
//           This will have written NULL to pMsgPtr */
//        case OS_ERR_PEND_WOULD_BLOCK:
//
//        case OS_ERR_TIMEOUT:
//            eRetStatus = ADI_OSAL_TIMEOUT;
//            break;
//
//#ifdef OSAL_DEBUG
//        case OS_ERR_OBJ_PTR_NULL: /* FALLTHROUGH */
//        case OS_ERR_OBJ_TYPE:
//            eRetStatus = ADI_OSAL_BAD_HANDLE;
//            break;
//
//        case OS_ERR_PEND_ISR:           /* FALLTHROUGH */
//        case OS_ERR_SCHED_LOCKED:
//            eRetStatus = ADI_OSAL_CALLER_ERROR;
//            break;
//#endif /*OSAL_DEBUG */
    case errQUEUE_FULL:
        default:
            eRetStatus = ADI_OSAL_FAILED;
            break;
    } /* end of switch */

    return( eRetStatus );
}

/*!
  ****************************************************************************
    @brief Receives a message from the specified task message queue.

    @param[in]  hThread             -  handle of the task queue to flush the
                                       message from, Specifying a NULL pointer indicates that you wish to
                                       flush the message queue of the calling task.
    @param[out] pMsgCount           -  Pointer to a location to store number of
                                       messages freed

    @return ADI_OSAL_SUCCESS      - If message is received and copied to ppMsg
                                    buffer and removed from queue.
    @return ADI_OSAL_FAILED       - If failed to get a message.
    @return ADI_OSAL_TIMEOUT      - If failed get message due to timeout.
    @return ADI_OSAL_BAD_HANDLE   - If the specified message queue is invalid
    @return ADI_OSAL_CALLER_ERROR - If the function is invoked from an invalid
                                    location (i.e an ISR)
*****************************************************************************/

/*
**
** EOF:
**
*/
