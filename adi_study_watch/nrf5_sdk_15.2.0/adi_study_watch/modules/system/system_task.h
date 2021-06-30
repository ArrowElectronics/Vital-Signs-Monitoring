#ifndef __SYSTEM_TASK_H
#define __SYSTEM_TASK_H

#define ADI_APP_SENSOR_ADPD185_ID   0x8002

void system_task_init(void);
void update_ble_system_info();
void send_message_system_task(m2m2_hdr_t *p_pkt);
void SetCfgCopyAvailableFlag(uint8_t nflag);
void SetCfgFileAvailableFlag(uint8_t nflag);
uint8_t GetCfgFileAvailableFlag(void);
void DG2502_SW_control_ADPD4000(uint8_t sw_enable);
void DG2502_SW_control_AD8233(uint8_t sw_enable);
void DG2502_SW_control_AD5940(uint8_t sw_enable);
void SendForceStopStreaming();
void LowTouchErr(void);
#endif  // __SYSTEM_TASK_H