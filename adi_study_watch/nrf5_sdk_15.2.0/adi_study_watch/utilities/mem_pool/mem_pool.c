/**
****************************************************************************
* @file     mem_pool.c
* @author   ADI
* @version  V0.1
* @date     04-Feb-2020
* @brief    This source file is used to implement memory pool creation functions.
****************************************************************************
* @attention
******************************************************************************
* Copyright (c) 2021 Analog Devices, Inc.  All rights reserved.
*
* Redistribution and use in source and binary forms, with or without
* modification, are permitted provided that the following conditions are met:
* - Redistributions of source code must retain the above copyright notice, this
*   list of conditions and the following disclaimer.
* - Redistributions in binary form must reproduce the above copyright notice,
*   this list of conditions and the following disclaimer in the documentation
*   and/or other materials provided with the distribution.
* - Modified versions of the software must be conspicuously marked as such.
* - This software is licensed solely and exclusively for use with
*   processors/products manufactured by or for Analog Devices, Inc.
* - This software may not be combined or merged with other code in any manner
*   that would cause the software to become subject to terms and conditions
*   which differ from those listed here.
* - Neither the name of Analog Devices, Inc. nor the names of its contributors
*   may be used to endorse or promote products derived from this software
*   without specific prior written permission.
* - The use of this software may or may not infringe the patent rights of one
*   or more patent holders.  This license does not release you from the
*   requirement that you obtain separate licenses from these patent holders to
*   use this software.
*
* THIS SOFTWARE IS PROVIDED BY ANALOG DEVICES, INC. AND CONTRIBUTORS "AS IS"
* AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
* NONINFRINGEMENT, TITLE, MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
* ARE DISCLAIMED. IN NO EVENT SHALL ANALOG DEVICES, INC. OR CONTRIBUTORS BE
* LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, PUNITIVE OR
* CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, DAMAGES ARISING OUT OF
* CLAIMS OF INTELLECTUAL PROPERTY RIGHTS INFRINGEMENT; PROCUREMENT OF
* SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
* INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
* CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
* ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
* POSSIBILITY OF SUCH DAMAGE.
******************************************************************************/

#include <adi_osal.h>
#include <mem_pool.h>
#include <FreeRTOS.h>
#include <task.h>
#define void_ptr

#ifdef DEBUG_DCB
uint16_t MemNumBlks=0;
uint16_t MemFreeBlks=0;
#endif

/* segments the given memory based on the Blk size and Number of blocks and Initializes the memory handler */
mem_pool_result_t mem_pool_init (mem_pool_handler_t *hMem, void *pMemBlk, uint16_t nNumBlks, uint16_t nBlkSz)
{
  mem_pool_result_t nMemResult = MEM_POOL_SUCCESS;
  uint16_t index;
  uint8_t* pMem;
#ifdef void_ptr
  void **pMemTemp;
#else
  uint32_t* pMemTemp;
#endif //void_ptr
//  uint8_t alignMask;
  
  if ((hMem == NULL) | (pMemBlk == NULL))
    return MEM_POOL_FAILURE;
  
  if ((nNumBlks == NULL) | (nBlkSz == NULL))
    return MEM_POOL_FAILURE;
    
/*check if allocated memory is pointer aligned*/
  if( (int)pMemBlk % sizeof(void *) != NULL)
    return MEM_POOL_FAILURE;
  
/*check if block size is pointer aligned*/  
  if(nBlkSz % sizeof(void *) != NULL)
    return MEM_POOL_FAILURE;  

  /*critical section starts here*/
#ifdef ADI_CRITICAL_SECTION
  ADI_ENTER_CRITICAL_REGION();  
#else  
  adi_osal_EnterCriticalRegion();
#endif //ADI_CRITICAL_SECTION
  
  /*initialize the handler*/
  hMem->pMem = pMemBlk;
  hMem->nBlkSz = nBlkSz;
  hMem->nNumBlks = nNumBlks;
  hMem->nBlkFree = nNumBlks;
  hMem->pFreeListPtr = pMemBlk;
  hMem->pEndPtr = (void *)((uint32_t)pMemBlk + ((nNumBlks-1) * nBlkSz));
  
  pMem = pMemBlk;
  pMemTemp = pMemBlk;
  
  #ifdef DEBUG_DCB
  MemNumBlks =  hMem->nNumBlks;
  #endif

  for (index = 0; index < (nNumBlks - 1); index++ )
  {
//    *pMem = ++pMemBlk;
#ifdef void_ptr
    pMem += nBlkSz;
    *pMemTemp = (void *) pMem;
    pMemTemp = (void **) pMem;
#else
    pMem += nBlkSz;
    *pMemTemp = (uint32_t) pMem;
    pMemTemp = (uint32_t *) pMem;    
#endif //void_ptr
  }
  *pMemTemp = NULL;
  /*critical section ends here*/
#ifdef ADI_CRITICAL_SECTION
  ADI_EXIT_CRITICAL_REGION();
#else  
  adi_osal_ExitCriticalRegion();
#endif //ADI_CRITICAL_SECTION  
return nMemResult;
}


mem_pool_result_t mem_pool_get (mem_pool_handler_t *hMem, void **pMemBlk)
{
  mem_pool_result_t nMemResult = MEM_POOL_SUCCESS;
  void *pTempBlk;
  uint8_t* pMem;
  if(hMem->nBlkFree == 0u)
    return MEM_POOL_FAILURE;
  
  #ifdef DEBUG_DCB
  MemFreeBlks = hMem->nBlkFree;
  #endif

  if(hMem->pFreeListPtr != NULL)
  {
#ifdef ADI_CRITICAL_SECTION
  ADI_ENTER_CRITICAL_REGION();
#else  
  adi_osal_EnterCriticalRegion();
#endif //ADI_CRITICAL_SECTION
    /*critical section starts here*/
    // ToDo (sathishkumar)
    /*
     * Below condition check added to make-sure we are not trying to access
     * out-side of memory, this will happen when trying to access memory before
     * freed-up
     */
    if((hMem->pFreeListPtr < hMem->pMem) || (hMem->pFreeListPtr > hMem->pEndPtr)){
      pMem = hMem->pMem;
      while(((void *)pMem < hMem->pMem) || ((void *)pMem > hMem->pEndPtr)){
        pMem += hMem->nBlkSz;
      }
      hMem->pFreeListPtr = (void **) pMem;
    }

    pTempBlk = hMem->pFreeListPtr;
    hMem->pFreeListPtr = *(void **) pTempBlk;
    hMem->nBlkFree--;
    
    *pMemBlk = pTempBlk;
  /*critical section ends here*/
#ifdef ADI_CRITICAL_SECTION
  ADI_EXIT_CRITICAL_REGION();
#else  
  adi_osal_ExitCriticalRegion();
#endif //ADI_CRITICAL_SECTION  
  }
  else
  {
    /*Invalid FreeList Ptr*/
    return MEM_POOL_FAILURE;
  }
  return nMemResult;  
}


mem_pool_result_t mem_pool_free (mem_pool_handler_t *hMem, void *pMemBlk)
{
  mem_pool_result_t nMemResult = MEM_POOL_SUCCESS;
  //uint8_t *pMem;
  if(hMem->nBlkFree >= hMem->nNumBlks)
    return MEM_POOL_FAILURE;
  
//  pMemTemp = pMemBlk;
  /*critical section starts here*/
#ifdef ADI_CRITICAL_SECTION
  ADI_ENTER_CRITICAL_REGION();
#else  
  adi_osal_EnterCriticalRegion();
#endif //ADI_CRITICAL_SECTION
  // ToDo (sathishkumar)
  /*
  * Below condition check added to make-sure we are not trying to free the
  * out-side of memory, this will happen when trying to assign memory before
  * freed-up
  */
//  if((hMem->pFreeListPtr < hMem->pMem) || (hMem->pFreeListPtr > hMem->pEndPtr)){
//    pMem = hMem->pMem;
//    while(((void *)pMem < hMem->pMem) || ((void *)pMem > hMem->pEndPtr)){
//      pMem += hMem->nBlkSz;
//    }
//    hMem->pFreeListPtr = (void **) pMem;
//  }

  * (void **)pMemBlk = hMem->pFreeListPtr;
  hMem->pFreeListPtr = pMemBlk;
  hMem->nBlkFree++;
  /*critical section ends here*/
#ifdef ADI_CRITICAL_SECTION
  ADI_EXIT_CRITICAL_REGION();
#else  
  adi_osal_ExitCriticalRegion();
#endif //ADI_CRITICAL_SECTION  
  return nMemResult;  
}
