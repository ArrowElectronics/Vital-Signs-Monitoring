#include <adi_osal.h>
#include <stdio.h>
#include <nrfx_timer.h>


uint32_t get_micro_sec(void);
uint32_t us_timer_init(void);
void us_timer_deinit(void);
void us_timer_reset(void);
void enable_adxl_ext_trigger(uint8_t nFreq);
void disable_adxl_ext_trigger(uint8_t nFreq);
void set_adxl_trigger_freq(uint8_t nFreq);
void enable_adpd_ext_trigger(uint16_t nOdr);
void disable_adpd_ext_trigger(uint16_t nOdr);
void enable_ecg_ext_trigger(uint16_t nOdr);
void disable_ecg_ext_trigger(uint16_t nOdr);
