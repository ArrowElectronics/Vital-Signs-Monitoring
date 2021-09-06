#ifndef __SYSTEM_TASK_H
#define __SYSTEM_TASK_H

#define ADI_APP_SENSOR_ADPD185_ID   0x8002

/*Enums to store battery charging levels status*/
typedef enum{
  E_CRITICAL_LEVEL = 0x00,    //!< Battery Charge critical level (1% by default)
  E_LOW_LEVEL,                //!< Battery Charge low level (5% by default)
  E_NORMAL_LEVEL,             //!< Battery Charge normal level ( >low level && < 100)
  E_FULL_LEVEL,               //!< Battery Charge Full(100%) level
}CHARGE_STATUS_ENUM_t;

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
void update_hw_id_in_system_info(uint16_t hw_id);
#endif  // __SYSTEM_TASK_H