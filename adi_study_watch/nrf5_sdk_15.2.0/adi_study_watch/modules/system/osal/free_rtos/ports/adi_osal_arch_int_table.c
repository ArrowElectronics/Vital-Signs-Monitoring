/* $Revision: 27678 $
 * $Date: 2014-09-30 07:08:59 -0400 (Tue, 30 Sep 2014) $
******************************************************************************
Copyright (c), 2009-2012 - Analog Devices Inc. All Rights Reserved.
This software is PROPRIETARY & CONFIDENTIAL to Analog Devices, Inc.
and its licensors.
******************************************************************************

Title:      OSAL interrupt table definition on Cortex-M4

Description:
           This is the definition of the OSAL interrupt table in which pointers
           to handlers written in C for the given interrupt are stored.

*****************************************************************************/

/*=============  I N C L U D E S   =============*/
#include "adi_osal.h"
#include <adi_osal_arch.h>

/*=============  D E F I N E S  =============*/


/*!
    @internal
    @var _adi_osal_gHandlerTable
	This is the OSAL dispatch table. It is an array of function pointers and 
    callback arguments, of the type corresponding to OSAL's plain C interrupt 
    handlers (i.e. the high-level handlers). The table needs to be large enough 
    for any index that we can get back from adi_nvic_RegisterHandler(). 
    @endinternal
 */

#define OSAL_HANDLER_TABLE_SIZE 255u

uint32_t _adi_osal_gHandlerTableSize = OSAL_HANDLER_TABLE_SIZE;

/*!
    @internal
    @var _adi_osal_gHandlerTableSize
    The size of the OSAL dispatch table. The size needs to be large enough for 
    any index that we can get back from adi_nvic_RegisterHandler(). 
    @endinternal
 */

ADI_OSAL_HANDLER_ENTRY _adi_osal_gHandlerTable[OSAL_HANDLER_TABLE_SIZE];


/*
**
** EOF: 
**
*/

