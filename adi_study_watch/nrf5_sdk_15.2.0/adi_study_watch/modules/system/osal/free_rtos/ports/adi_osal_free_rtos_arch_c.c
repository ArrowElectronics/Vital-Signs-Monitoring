#include <stdlib.h>     /* for NULL definition */
#include <limits.h>
#include "adi_osal.h"
#include "adi_osal_arch.h"
#include "adi_osal_arch_internal.h"
#include "osal_common.h"
//#include <services/int/adi_nvic.h>
//#include <power_management.h>

void _adi_osal_stdWrapper (void)
{
    /* Get the interrupt number */
    uint32_t nIntNum = __get_IPSR();
//#ifdef USE_PWR_MANAGER
//    PWR_MANAGER_CLR_PWR_MODE();
//#endif
     /* Call the higher level callback */
    _adi_osal_gHandlerTable[nIntNum].pfOSALHandler(ADI_NVIC_IRQ_SID(nIntNum),  _adi_osal_gHandlerTable[nIntNum].pOSALArg);

    return;
}
