#ifndef __BLE_TASK_H
#define __BLE_TASK_H
#include <post_office.h>

/* enum to hold current status of BLE connection in gb_ble_status variable */
enum {
  BLE_DISCONNECTED = 0x00, /* disconnected from Central */
  BLE_CONNECTED = 0x01,    /* connected to Central */
  BLE_PORT_OPENED = 0x02,  /* NUS start */
  BLE_PORT_CLOSED = 0x03,  /* NUS stop */
};

void ble_application_task_init(void);
void ble_services_sensor_task_init();
void send_message_ble_tx_task(m2m2_hdr_t *p_pkt);
void send_message_ble_services_sensor_task(m2m2_hdr_t *p_pkt);
#ifdef BLE_PEER_ENABLE
void ble_peer_password_get(uint8_t *passwd);
#endif
uint32_t enter_bootloader_and_restart(void);
uint8_t set_max_tx_pkt_comb_cnt(uint8_t max_tx_kt_comb_cnt);
uint8_t get_max_tx_pkt_comb_cnt(void);
int get_ble_hr_service_status();
int set_ble_hr_service_sensor(bool enab);
uint8_t get_ble_nus_status();
void battery_level_update(void);
/*****************************************************************************
 * Function      : ble_disable_softdevice
 * Description   : Disable softdevice
 * Input         : void
 * Output        : None
 * Return        :
 * Others        :
 * Record
 * 1.Date        : 20200624
 *   Modification: Created function

*****************************************************************************/
void ble_disable_softdevice(void);

/*****************************************************************************
 * Function      : ble_disconnect_and_unbond
 * Description   : disconnect and unbond
 * Input         : void
 * Output        : None
 * Return        :
 * Others        :
 * Record
 * 1.Date        : 20200624
 *   Modification: Created function

*****************************************************************************/
void ble_disconnect_and_unbond(void);

#ifdef CUST4_SM
void turn_on_BLE();
void turn_off_BLE();
void change_ble_adv_duration(uint32_t new_adv_duration);
#endif
#endif  // __BLE_TASK_H