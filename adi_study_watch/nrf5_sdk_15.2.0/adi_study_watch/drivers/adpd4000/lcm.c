/**
***************************************************************************
* @file         lcm.c
* @author       ADI
* @version
* @date         26-Jun-2019
* @brief        Source file contains lcm wrapper
*
***************************************************************************
* @attention
***************************************************************************
*/
/******************************************************************************
*                                                                             *
* License Agreement                                                           *
*                                                                             *
* Copyright (c) 2021 Analog Devices Inc.                                      *
* All rights reserved.                                                        *
*                                                                             *
* This source code is intended for the recipient only under the guidelines of *
* the non-disclosure agreement with Analog Devices Inc.                       *
*                                                                             *
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR  *
* IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,    *
* FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE *
* AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER      *
* LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING     *
* FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER         *
* DEALINGS IN THE SOFTWARE.                                                   *
*                                                                             *
*                                                                             *
* This software is intended for use with the ADPD400x part                    *
* only                                                                        *
*                                                                             *
******************************************************************************/
#include <string.h>
#include "adpd400x_drv.h"
#include "lcm.h"

/*----------------------------- Defines --------------------------------------*/
#define MAX_SLOT               12  //!< Maximum number of slots in ADPD4100

/*--------------------- Private Function Prototypes --------------------------*/
static uint32_t _gcdofnumbers(uint32_t val1, uint32_t val2);
static uint32_t _lcmofnumbers(uint32_t num1, uint16_t num2);

/** @brief Find the LCM value for given sequence
*
* @param  p_slot pointer to slot information
* @param  highest_slot_num
* @return uint32_t A 32-bit integer: LCM Value
*/
uint32_t calculate_lcm(adpd400xDrv_slot_t *p_slot, uint16_t highest_slot_num) {
  bool bIsSameValue = true;
  uint32_t nLcmValue = 1;
  uint32_t nCount = 0;

  while (nCount <= highest_slot_num){
    if (p_slot[nCount].decimation != 0)	{
      nLcmValue = _lcmofnumbers(nLcmValue, p_slot[nCount].decimation);
    }
    if (nCount > 0) {
      if (p_slot[nCount].decimation != p_slot[nCount - 1].decimation) {
      	bIsSameValue = false;
      }
    }
    nCount++;
  }

  if (bIsSameValue) {
    for (int i = 0; i < MAX_SLOT; i++){
      p_slot[i].decimation = 1;
    }
    nLcmValue = 1;
  }
  return nLcmValue;
}

/** @brief Find the pattern for given sequence
*
* @param  p_seq_no pointer to sequence number
* @param  p_slot pointer to the slot number
* @param  lcm_value 
* @param  highest_slot_num the highest active slot
* @param  p_adpd_cl_datapattern slot information
* @return ADI_HAL_STATUS_t HAL error status
*/
ADI_HAL_STATUS_t get_current_datapattern(uint32_t *p_seq_no, adpd400xDrv_slot_t *p_slot,
                                          uint32_t lcm_value, uint8_t highest_slot_num,
                                  fifo_data_pattern* p_adpd_cl_datapattern) {
 uint32_t nSampleSize = 0;
 uint32_t nSequenceNumber = 0;
 uint8_t nIndex;
 memset(&p_adpd_cl_datapattern->slot_info, 48, SLOT_NUM);  //48 -> ASCII value for '0' Char

 if(p_seq_no == NULL || p_slot == NULL || lcm_value == 0 || \
   highest_slot_num >= MAX_SLOT || p_adpd_cl_datapattern == NULL) {
   // ToDo
   return ADI_HAL_ERROR;
 }
 nSequenceNumber = *p_seq_no;
Get_Current_Pattern:
 for (nIndex = 0; nIndex <= highest_slot_num; nIndex++) {
   if (p_slot[nIndex].decimation != 0){
     if (((nSequenceNumber) % (p_slot[nIndex].decimation)) == 0) {
       p_adpd_cl_datapattern->slot_info[nIndex] = nIndex + 'A';
       /* Get the Lit, Dart & Signal size from 16-bit array element
          *---------*---------*--------*--------*
          * Lit     * Impulse * Dark   * Signal *
          *---------*---------*--------*--------*
          * [15:12] * [11:8]  * [7:4]  * [3:0]  *
          *---------*---------*--------*--------*
          */
       nSampleSize += ((p_slot[nIndex].slotFormat & 0xF) +
                       ((p_slot[nIndex].slotFormat >> 4) & 0xF) +
                        ((p_slot[nIndex].slotFormat >> 12) & 0xF));
       if (p_slot[nIndex].channelNum == 3)    // 2 channels are active
         nSampleSize += ((p_slot[nIndex].slotFormat & 0xF) +
                         ((p_slot[nIndex].slotFormat >> 4) & 0xF) +
                          ((p_slot[nIndex].slotFormat >> 12) & 0xF));
     }
     else {
       p_adpd_cl_datapattern->slot_info[nIndex] = '0';
     }
   }
   else {
     p_adpd_cl_datapattern->slot_info[nIndex] = '0';
   }
 }

 p_adpd_cl_datapattern->sample_size = nSampleSize;
 nSequenceNumber++;

 if(nSequenceNumber > lcm_value) {
   nSequenceNumber = 1;
 }

 // Check the slot size for given sequence number if it's '0' go and process again for the next sequence number
 if(nSampleSize == 0 && nIndex <= 12)
   goto Get_Current_Pattern;

 // Assign updated sequence number
 *p_seq_no = nSequenceNumber;

 // Check the status using slot size, if it's '0' something went wrong in process
 if(nSampleSize != 0)
   return ADI_HAL_OK;
 else
   return ADI_HAL_ERROR;
}

/** @brief Find the sample size for given sequence
*
* @param  no_of_samples Number of samples
* @param  p_slot pointer to the slot information
* @param  p_seq_no pointer to the sequence number
* @param  lcm_value LCM of the slot decimation
* @param  high_slot_num Highest slot number
* @return uint16_t: Total size 16 bit value
*/
uint16_t get_samples_size(uint16_t no_of_samples, adpd400xDrv_slot_t *p_slot,
                           uint32_t *p_seq_no,
                              uint32_t lcm_value, uint16_t high_slot_num) {
  uint32_t nSampleSize = 0;
  uint32_t nSequenceNumber;
  uint8_t nIndex;
  uint16_t nTemp = 0;

  if(p_seq_no == NULL || p_slot == NULL || lcm_value == 0 || high_slot_num >= MAX_SLOT) {
    // ToDo
    return 0;
  }
  nSequenceNumber = *p_seq_no;
  for(int i=0; i < no_of_samples; i++)  {
    nTemp = 0;
  Get_Sample_Size:
    for (nIndex = 0; nIndex <= high_slot_num; nIndex++) {
      if (p_slot[nIndex].decimation != 0){
        if (((nSequenceNumber) % (p_slot[nIndex].decimation)) == 0) {
          /* Get the Lit, Dart & Signal size from 16-bit array element
          *---------*---------*--------*--------*
          * Lit     * Impulse * Dark   * Signal *
          *---------*---------*--------*--------*
          * [15:12] * [11:8]  * [7:4]  * [3:0]  *
          *---------*---------*--------*--------*
          */
          nSampleSize += ((p_slot[nIndex].slotFormat & 0xF) +
                          ((p_slot[nIndex].slotFormat >> 4) & 0xF) +
                           ((p_slot[nIndex].slotFormat >> 12) & 0xF));

          nTemp += ((p_slot[nIndex].slotFormat & 0xF) +
                    ((p_slot[nIndex].slotFormat >> 4) & 0xF) +
                     ((p_slot[nIndex].slotFormat >> 12) & 0xF));
          if (p_slot[nIndex].channelNum == 3) {    // 2 channels are active
            nSampleSize += ((p_slot[nIndex].slotFormat & 0xF) +
                            ((p_slot[nIndex].slotFormat >> 4) & 0xF) +
                             ((p_slot[nIndex].slotFormat >> 12) & 0xF));
            nTemp += ((p_slot[nIndex].slotFormat & 0xF) +
                      ((p_slot[nIndex].slotFormat >> 4) & 0xF) +
                       ((p_slot[nIndex].slotFormat >> 12) & 0xF));
          }
        }
      }
    }

    nSequenceNumber++;
    if(nSequenceNumber > lcm_value){
      nSequenceNumber = 1;
    }
    if(nTemp == 0 && nIndex <= 12)
      goto Get_Sample_Size;
  }

  // Assign updated sequence number
  *p_seq_no = nSequenceNumber;

  return nSampleSize;
}

/** @brief Find the sample size for given sequence
*
* @param  seq_no Sequence number
* @param  p_slot pointer to slot information
* @param  high_slot_num Highest slot number
* @return uint16_t A 16-bit integer: Sample size Value
*/
uint16_t get_max_sample_size(uint32_t seq_no, adpd400xDrv_slot_t *p_slot, uint8_t high_slot_num) {
  uint16_t nTotal_slots_size = 0;
  uint8_t nIndex;
  if(p_slot == NULL || seq_no == 0 ||  high_slot_num >= MAX_SLOT) {
    // ToDo
    return 0;
  }
  for (nIndex = 0; nIndex <= high_slot_num; nIndex++) {
    if (p_slot[nIndex].decimation != 0){
      if (((seq_no) % (p_slot[nIndex].decimation)) == 0) {
        /* Get the Lit, Dart & Signal size from 16-bit array element
          *---------*---------*--------*--------*
          * Lit     * Impulse * Dark   * Signal *
          *---------*---------*--------*--------*
          * [15:12] * [11:8]  * [7:4]  * [3:0]  *
          *---------*---------*--------*--------*
          */
        nTotal_slots_size += ((p_slot[nIndex].slotFormat & 0xF) +
                              ((p_slot[nIndex].slotFormat >> 4) & 0xF) +
                               ((p_slot[nIndex].slotFormat >> 12) & 0xF));
        if (p_slot[nIndex].channelNum == 3)    // 2 channels are active
          nTotal_slots_size += ((p_slot[nIndex].slotFormat & 0xF) +
                                ((p_slot[nIndex].slotFormat >> 4) & 0xF) +
                                 ((p_slot[nIndex].slotFormat >> 12) & 0xF));
      }
    }
  }
  return nTotal_slots_size;
}

/** @brief Find the GCD value for given sequence
*
* @param  val1 First value to find GCD of
* @param  val2 Second value to find GCD of
* @return uint32_t A 32-bit integer: GCD Value
*/
static uint32_t _gcdofnumbers(uint32_t val1, uint32_t val2) {
  uint32_t temp;
  if (val1 > val2) {
    temp = val1;
    val1 = val2;
    val2 = temp;
  }
  if (val2 % val1 == 0){
    return val1;
  }
  else{
    return _gcdofnumbers(val2 % val1, val1);
  }
}

/** @brief Find the LCM value for given sequence
*
* @param  num1 First number to find LCM of
* @param  num2 Second number to find LCM of
* @return uint32_t A 32-bit integer: LCM Value
*/
static uint32_t _lcmofnumbers(uint32_t num1, uint16_t num2){
  uint32_t gcd = _gcdofnumbers(num1, num2);
  return (num1 * num2) / gcd;
}
