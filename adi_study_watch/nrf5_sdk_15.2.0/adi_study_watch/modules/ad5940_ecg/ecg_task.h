/*!
 *  \copyright Analog Devices
 * ****************************************************************************
 *
 * License Agreement
 *
 * Copyright (c) 2019 Analog Devices Inc.
 * All rights reserved.
 *
 * This source code is intended for the recipient only under the guidelines of
 * the non-disclosure agreement with Analog Devices Inc.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER  LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING  FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER  DEALINGS IN THE SOFTWARE.
 * ****************************************************************************
 */
#ifndef __ECG_TASK__H
#define __ECG_TASK__H
#include <ecg_application_interface.h>
#include <task_includes.h>
#include <post_office.h>
#include <ad5940.h>
#include <sensor_ad5940.h>
#include <app_ecg.h>
#include <app_cfg.h>

typedef struct _ecg_packetizer_t {
  m2m2_hdr_t                *p_pkt;
  uint16_t                  packet_max_nsamples;
  uint16_t                  sample_sz;
  uint16_t                  packet_nsamples;
  uint32_t                  prev_ts;
} ecg_packetizer_t;

typedef struct _g_state_ecg_t {
  uint16_t  num_subs;
  uint16_t  num_starts;
  uint8_t   decimation_factor;
  uint16_t  decimation_nsamples;
  uint16_t  data_pkt_seq_num;
  uint8_t  ecgHR;
  ecg_packetizer_t  ecg_pktizer;
  uint16_t  leads_on_samplecount;
  uint16_t  leads_off_samplecount;
}g_state_ecg_t;

/* app buffer size */
#define APPBUFF_SIZE 1024

 
  
/* Number of ECG samples send out for leadsoff indication*/
#define MAX_SAMPLES_LEADS_OFF M2M2_SENSOR_ECG_NSAMPLES * 2
 /*Number of ECG samples checked continously for proper leadson*/
#define MAX_SAMPLES_LEADS_ON  M2M2_SENSOR_ECG_NSAMPLES * 6


void ad5940_ecg_task_init(void);
void send_message_ad5940_ecg_task(m2m2_hdr_t *p_pkt);
void DG2502_SW_control_AD8233(uint8_t sw_enable);
void DG2502_SW_control_AD5940(uint8_t sw_enable);
#endif // __ADPD4000_TASK__H