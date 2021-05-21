
/****/
#ifndef __ADXL_TASK_H__
#define __ADXL_TASK_H__
#include <stdint.h>
#ifdef _USING_RTOS_
#include "adi_osal.h"
#endif
#include "task_includes.h"
#include "sensor_adxl_application_interface.h"
#include <app_cfg.h>
/*----------------------------- Typedefs -------------------------------------*/
typedef struct _adxl_packetizer_t {
  m2m2_hdr_t                *p_pkt;
  uint16_t                  packet_max_nsamples;
  uint16_t                  sample_sz;
  uint16_t                  packet_nsamples;
  uint32_t                  prev_ts;
} adxl_packetizer_t;

typedef struct _g_state_adxl {
  uint16_t  num_subs;
  uint16_t  num_starts;
  uint8_t   decimation_factor;
  uint16_t  decimation_nsamples;
  uint16_t  data_pkt_seq_num;
  adxl_packetizer_t  adxl_pktizer;
} g_state_adxl_t;

void sensor_adxl_task_init(void);
void send_message_adxl_task(m2m2_hdr_t *p_pkt);

//#include "adxl_dcfg.h"

#endif // __ADXL_TASK_H__

