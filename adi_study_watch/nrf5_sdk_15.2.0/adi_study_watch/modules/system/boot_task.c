#ifndef FreeRTOS_PM_ONLY
#include  <clk.h>
#if (OS_CFG_DYN_TICK_EN == DEF_ENABLED)
#include  <bsp_os_dyn_rtc.h>
#endif
#endif // FreeRTOS_PM_ONLY

#ifdef USE_FS
//#include "fs_dir.h"
//#include "fs_api.h"
#include <file_system_utils.h>
#include <nand_layout.h>
#endif // USE_FS

#if BUILD_FOR_M4
#ifndef FreeRTOS_PM_ONLY
	#include "bsp.h"
	#include "bsp_os.h"
#endif // FreeRTOS_PM_ONLY
    #include <eeprom_control_util.h>
#else

	#include <ps_application_interface.h>
	#include <file_system_task.h>
	#include <spi_comms_task.h>
	#include <led_task.h>
	#include <watchdog_task.h>
	#include "dialog14580.h"
        #include "AD5110/AD5110.h"
    #include "Nand_bsp.h"

#endif // BUILD_FOR_M4

#include <nand_layout.h>
#if defined(USE_PWR_MANAGER) && !defined(FreeRTOS_PM_ONLY)
#include <os_app_hooks.h>
#endif
#include <pm_application_interface.h>
#include <boot_task.h>
#include <pm_application_task.h>
#include <post_office.h>
#include <comms_task.h>
#include "hw_if_config.h"
#include <adi_osal.h>
#include "ble_pre_commands.h"
#include <m2m2_core.h>
#include <post_office.h>
#ifdef ADPD_PM
#include <adpd_task.h>
#endif //ADPD_PM

#ifdef ADPDCL_PM
#include <adpdCl_task.h>
#endif //ADPDCL_PM

#ifdef ADXL_PM
#include <adxl_task.h>
#endif //ADXL_PM

extern m2m2_pm_sys_info_t g_system_info;

#ifdef FreeRTOS_PM_ONLY
#pragma location = "dRAM_always_retained_region"
uint8_t ucHeap[ configTOTAL_HEAP_SIZE ];

#pragma location = "bank1_retained_ram"
static StaticTask_t    IdleTaskTCBbuf;
static StackType_t     IdleTaskStkbuf[configMINIMAL_STACK_SIZE];

void vApplicationGetIdleTaskMemory( StaticTask_t **ppxIdleTaskTCBBuffer, StackType_t **ppxIdleTaskStackBuffer, uint32_t *pulIdleTaskStackSize )
{
  *ppxIdleTaskTCBBuffer = &IdleTaskTCBbuf;
  *ppxIdleTaskStackBuffer = IdleTaskStkbuf;
  *pulIdleTaskStackSize = configMINIMAL_STACK_SIZE;

}
#endif //FreeRTOS_PM_ONLY

#ifdef FreeRTOS_PM_ONLY
#pragma location = "bank1_retained_ram"
uint8_t boot_task_stack[APP_OS_CFG_BOOT_TASK_STK_SIZE];
ADI_OSAL_THREAD_HANDLE boot_task_handler;
ADI_OSAL_STATIC_THREAD_ATTR boot_task_attributes;
StaticTask_t BootTaskTcb;
#else
#pragma location = "bank1_retained_ram"
uint8_t boot_task_stack[APP_OS_CFG_BOOT_TASK_STK_SIZE];
ADI_OSAL_THREAD_HANDLE boot_task_handler;
ADI_OSAL_THREAD_ATTR boot_task_attributes;
#endif //FreeRTOS_PM_ONLY

#ifdef BLE_PRE_COMMANDS_ENABLE
ADI_OSAL_SEM_HANDLE ble_cmd_send_sem;

static uint8_t *gsCmdFileReadPtr = (uint8_t *)&ble_pre_commands_arr[0];
#pragma location = "bank2_retained_ram"
static m2m2_hdr_t *gsCmdHdr;
//#pragma location = "bank1_retained_ram"
//static m2m2_hdr_t *gsTempCmdHdr;

#endif //BLE_PRE_COMMANDS_ENABLE

static void boot_task();
static void MCU_Init(void);

/** Frequency Variable */
uint32_t gnFreq = 168;

#ifdef FreeRTOS_PM_ONLY
ADI_OSAL_QUEUE_HANDLE  boot_task_msg_queue = NULL;
#endif //FreeRTOS_PM_ONLY

#ifdef BUILD_FOR_M4

/** This Variable is used to check the Bluetooth Status */
uint8_t BTRingEvent = 0;
#else
/** This variable is used checking the AD5110 chip is mounted or not on mother board*/
AD5110_RESULT gAd5110 = AD5110_ERROR;
#endif // BUILD_FOR_M4

/** This variable is used checking the M24C16 chip is mounted or not on mother board*/
uint8_t gnM24c16 = 0;

void boot_task_init(void) {
  ADI_OSAL_STATUS eOsStatus;

  boot_task_attributes.pThreadFunc = boot_task;
  boot_task_attributes.nPriority = APP_OS_CFG_BOOT_TASK_PRIO;
  boot_task_attributes.pStackBase = &boot_task_stack[0];
  boot_task_attributes.nStackSize = APP_OS_CFG_BOOT_TASK_STK_SIZE;
  boot_task_attributes.pTaskAttrParam = (void *)0;
  boot_task_attributes.szThreadName = "Bootup Task";

#ifndef FreeRTOS_PM_ONLY
      boot_task_attributes.nThreadQueueSize = 0u;
    #ifdef BLE_PRE_COMMANDS_ENABLE
      adi_osal_SemCreate(&ble_cmd_send_sem, 0U);
    #endif //BLE_PRE_COMMANDS_ENABLE
      eOsStatus = adi_osal_ThreadCreate(&boot_task_handler,
                                        &boot_task_attributes);
      if (eOsStatus != ADI_OSAL_SUCCESS) {
          Debug_Handler();
      }
#else
    #ifdef BLE_PRE_COMMANDS_ENABLE
      adi_osal_SemCreate(&ble_cmd_send_sem, 0U);
    #endif //BLE_PRE_COMMANDS_ENABLE
      boot_task_attributes.pThreadTcb = &BootTaskTcb;
      eOsStatus = adi_osal_MsgQueueCreate(&boot_task_msg_queue,NULL,
                                          5);
      if (eOsStatus != ADI_OSAL_SUCCESS) {
        Debug_Handler();
      }
      else
      {
        update_task_queue_list(APP_OS_CFG_BOOT_TASK_INDEX,boot_task_msg_queue);
      }
      eOsStatus = adi_osal_ThreadCreateStatic(&boot_task_handler,
                                        &boot_task_attributes);
      if (eOsStatus != ADI_OSAL_SUCCESS) {
          Debug_Handler();
      }
#endif //FreeRTOS_PM_ONLY


}
#ifdef BLE_PRE_COMMANDS_ENABLE
static uint16_t pkt_len = 0, gsCmdCnt = 0;
#endif // BLE_PRE_COMMANDS_ENABLE

static void boot_task() {
  memory_manager_init();
#if defined(USE_PWR_MANAGER ) && !defined(FreeRTOS_PM_ONLY)
  App_OS_SetAllHooks();
#endif

#ifndef FreeRTOS_PM_ONLY
  /* Global Initializations */
  CPU_Init();
#endif // FreeRTOS_PM_ONLY

#ifdef USE_PWR_MANAGER
  PWR_MANAGER_INIT();
  //PWR_MANAGER_ARMED();
#endif
  MCU_Init();

#if defined(BUILD_FOR_M4) && !defined(FreeRTOS_PM_ONLY)
  /* Initialize OS_Tick_Timer */
  BSP_Tick_Init();
#endif // BUILD_FOR_M4

  post_office_task_init();
  pm_application_task_init();
#ifdef ADPD_PM
  sensor_adpd_task_init();
#endif // ADPD_PM

#ifdef ADPDCL_PM
  sensor_adpdCl_task_init();
#endif // ADPDCL_PM

#ifdef ADXL_PM
  sensor_adxl_task_init();
#endif

#ifdef USE_FS
  file_system_task_init();
#endif //USE_FS

#ifndef BUILD_FOR_M4
  led_task_init();
#endif // BUILD_FOR_M4

#if !defined (ADPD_PM) && !defined(ADPDCL_PM)
  spi_comms_task_init();
#endif // ADPD_PM

#ifndef BUILD_FOR_M4
  watchdog_task_init();
#endif // BUILD_FOR_M4
  comms_task_init();

  while(1) {
#ifdef BLE_PRE_COMMANDS_ENABLE
    m2m2_hdr_t                            *pkt = NULL;
    ADI_OSAL_STATUS                       err;
    adi_osal_SemPend(ble_cmd_send_sem, ADI_OSAL_TIMEOUT_FOREVER);
//    MCU_HAL_Delay(2000);
    while(gsCmdFileReadPtr < &ble_pre_commands_arr[CMD_ARRAY_SIZE])
    {
        gsCmdHdr = (m2m2_hdr_t *) gsCmdFileReadPtr;
        pkt_len =  gsCmdHdr->length;
        pkt_len = BYTE_SWAP_16(pkt_len);
        gsCmdFileReadPtr += (pkt_len);
        pkt = post_office_create_msg(pkt_len);
        if(pkt != NULL) {
        memcpy(pkt,gsCmdHdr,pkt_len);
        pkt->src = (M2M2_ADDR_ENUM_t) BYTE_SWAP_16(pkt->src);
        pkt->dest = (M2M2_ADDR_ENUM_t) BYTE_SWAP_16(pkt->dest);
        pkt->length = pkt_len;
        pkt->checksum = BYTE_SWAP_16(pkt->checksum);
        post_office_send(pkt, &err);
        gsCmdCnt++;
        MCU_HAL_Delay(500);
        }
    }
#endif //BLE_PRE_COMMANDS_ENABLE
#ifndef FreeRTOS_PM_ONLY
      post_office_get(ADI_OSAL_TIMEOUT_FOREVER);
#else
      post_office_get(ADI_OSAL_TIMEOUT_FOREVER, APP_OS_CFG_BOOT_TASK_INDEX);
      //vTaskDelay(1000);
      //adi_osal_ThreadDestroy(boot_task_handler);
#endif //FreeRTOS_PM_ONLY
  }
//#endif //FreeRTOS_PM_ONLY
}



/**
* @brief    MCU Core and peripherals Initialization.
* @param    None
* @retval   None
*/
static void MCU_Init(void) {
#ifndef FreeRTOS_PM_ONLY
  CLK_ERR clk_err;
#endif // FreeRTOS_PM_ONLY
  uint8_t eeprom_info[sizeof(eeprom_layout_struct_t)];
#ifndef BUILD_FOR_M4
  uint8_t ble_ready = 0;
  //#ifndef ADPD_PM

  //#endif // ADPD_PM
#else
  /* Configure LED3, LED4, LED5 & LED6 */
  BSP_LED_Init(LED3);
  BSP_LED_Init(LED5);
  BSP_LED_Off(LED3);
  BSP_LED_Off(LED5);
  /* Configure the system clock TO 16 MHz*/
#endif

  SystemClock_Config(gnFreq);
#ifndef FreeRTOS_PM_ONLY
#if (OS_CFG_DYN_TICK_EN == DEF_ENABLED)
  BSP_OS_RTC_TickInit();
#endif

#if (OS_CFG_STAT_TASK_EN > 0)
    OS_ERR   nErr;
    OSStatTaskCPUUsageInit(&nErr);
#endif

  Clk_Init(&clk_err);
#endif // FreeRTOS_PM_ONLY
  GPIO_Init();                                                                //Should not be called after I2C_Init()

  UART_Init();
#if defined(FreeRTOS_PM_ONLY) && defined(BUILD_FOR_M4)
    NVIC_SetPriority(DMA2_Stream5_IRQn,5);
    NVIC_SetPriority(DMA2_Stream7_IRQn,5);
    NVIC_SetPriority(DMA1_Stream5_IRQn, 5);
    NVIC_SetPriority(DMA1_Stream6_IRQn, 5);
    NVIC_SetPriority(USART1_IRQn,5);
    NVIC_SetPriority(USART2_IRQn,5);
#endif //FreeRTOS_PM_ONLY
  //    UART_RegisterReceiverCallback(pkt_uart_cb);
  //    UART_ReceiverINTEnable();
#ifndef BUILD_FOR_M4
  /* Charger IC Configuration */
  Init_I2C();   //Should be called after GPIO_Init()
  ADP5350_init();
#if defined(ADPD_PM) || defined(ADPDCL_PM)
  SPI_Init();
#endif // ADPD_PM
#ifdef AD5110
  if (AD5110_init() != AD5110_SUCCESS)
  {
    gAd5110 = AD5110_ERROR;
    adi_osal_ThreadSleep(10);
    Uninit_I2C();
    Init_I2C();
  }
  else
  {
    gAd5110 = AD5110_SUCCESS;
  }
#else
#ifndef BUILD_FOR_M4
  gAd5110 = AD5110_ERROR;
#endif // BUILD_FOR_M4
#endif // AD5110
  /* BLE Flash firmware and set MAC ID*/
  if (ble_flash_firmware() == 0) {
    ble_ready = 1;
    adi_gpio_SetHigh(DA_ENABLE_SLEEP_PORT, DA_ENABLE_SLEEP_PIN);
    adi_gpio_OutputEnable(DA_ENABLE_SLEEP_PORT, DA_ENABLE_SLEEP_PIN, true);
  } else {
    Debug_Handler();
  }

#ifdef BUTTON_A
  ButtonAGpioInit();
#endif // BUTTON_A

#ifdef LOW_TOUCH_FEATURE
  LowTouchGpioInit();
#endif //LOW_TOUCH_FEATURE

  /* update BLE MAC address from ADuCM302x Device ID */
#define ADI_UID_BASE     (0x00040770)
  typedef struct _ADI_UID_TypeDef
  {
    __I     uint8_t ID[16];                  /*!< UID Registers */
  } ADI_UID_TypeDef;

  ADI_UID_TypeDef *pReg = ((ADI_UID_TypeDef *) ADI_UID_BASE);
  uint32_t hash = 0;
  for (int i = 0; i < 16; i++)
    hash = hash * 419u + pReg->ID[i]; // buf contains 16 bytes data from address range 0x40770..0x4077F
  uint32_t mac24 = hash & 0xFFFFFFu;
  g_system_info.mac_addr[5] = (uint8_t)(mac24 & 0xFF);
  g_system_info.mac_addr[4] = (uint8_t)((mac24 >> 8) & 0xFF);
  g_system_info.mac_addr[3] = (uint8_t)((mac24 >> 16) & 0xFF);

#ifdef USE_FS
  Init_SPI();
  if (fs_hal_init() != FS_STATUS_OK) {
    Debug_Handler();
  }

  /* Disable flash */
  fs_flash_powerOn(false);
#endif // USE_FS
  if (ble_ready == 1) {
    adi_osal_ThreadSleep(200);
#ifdef FreeRTOS_PM_ONLY
    /*keeping the uart-tx and rx interrupt prio < configMAX_SYSCALL_INTERRUPT_PRIORITY (4)*/
    NVIC_SetPriority(SPI2_EVT_IRQn,5);
    NVIC_SetPriority(DMA0_CH8_DONE_IRQn,5);
    NVIC_SetPriority(DMA0_CH9_DONE_IRQn,5);
#endif //
#ifdef USE_BLE_UART_ONLY
    ble_set_mux_configutaion();
    set_uart_configuration(BLE_UART);
    ble_set_mac_address(&g_system_info.mac_addr[0]);
#else
#ifdef BUTTON_A
    if(GetButtonStatus() == true)
    {
      ble_set_mux_configutaion();
      set_uart_configuration(BLE_UART);
      ble_set_mac_address(&g_system_info.mac_addr[0]);
    }
    else
#endif // BUTTON_A
    {
      if (get_uart_mux_selection() == BLE_UART) {
        set_uart_configuration(BLE_UART);
        ble_set_mac_address(&g_system_info.mac_addr[0]);
      } else {
        /* Below 3 functions called to cause power redution on Dialog with GPIO trigger */
        ble_set_mux_configutaion();
        set_uart_configuration(BLE_UART);
        ble_set_mac_address(&g_system_info.mac_addr[0]);

        /* Enable extended sleep on Dialog BLE since FTDI_UART is enabled */
        /* GPIO P1_14 made active high to trigger extended sleep mode entry in DA14580 */
        adi_gpio_SetLow(DA_ENABLE_SLEEP_PORT, DA_ENABLE_SLEEP_PIN);
        MCU_HAL_Delay(5);
        adi_gpio_SetHigh(DA_ENABLE_SLEEP_PORT, DA_ENABLE_SLEEP_PIN);

        /* Undo GPIO setting made in ble_set_mux_configutaion() */
        adi_gpio_SetHigh(ADI_GPIO_PORT2, ADI_GPIO_PIN_3);
        adi_gpio_OutputEnable(ADI_GPIO_PORT2, ADI_GPIO_PIN_3, true);

        set_uart_configuration(FTDI_UART);
      }
    }
#endif
  } else {
    Debug_Handler();
  }

#ifdef  PS_ALONE
  while(1);
#endif //PS_ALONE

  /* Initialize timers */
  TIM_Init();
  Clk_ExtTS_Init();
  RTC1_Init();

#ifdef USE_WDT
  /* Initialize WDT */
  WDT_Init();

  /* Set and enable WDT in msec */
  SetWDT(4000);
#endif //USE_WDT
#else
  I2C_Init();
  SPI_Init();
  TIM_Init();
  //SD_Init();
  RTC_Init();
  BTRingEvent = BT_Init();
  ADP5061_init();
#endif // BUILD_FOR_M4
  /* update board ID from EEPROM */
#ifdef ADPD_PM
  if (eeprom_page_read(&eeprom_info[0], sizeof(eeprom_info)) == 0) {
#else
    if (get_eeprom_info(&eeprom_info[0], sizeof(eeprom_info)) == 0) {
#endif
      eeprom_layout_struct_t *p_formatted_info = (eeprom_layout_struct_t *)&eeprom_info[0];
      /* Make sure some data is written to EEPROM*/
      if (p_formatted_info->hw != 0xFFFF) {
        g_system_info.hw_id       = BYTE_SWAP_16(p_formatted_info->hw);
        g_system_info.bom_id      = BYTE_SWAP_16(p_formatted_info->bom);
        g_system_info.batch_id    = p_formatted_info->batch;
        g_system_info.date        = BYTE_SWAP_32(p_formatted_info->date);
      }
    }
    else
    {
      gnM24c16 = 1;
#ifdef ADPD_PM
      adi_osal_ThreadSleep(10);
#endif

#ifndef BUILD_FOR_M4
      Uninit_I2C();
      Init_I2C();//not same for m3/m4
#endif
    }
#ifdef BUILD_FOR_M4
    for( int i = 0; i < 8; i++){
      BSP_LED_Toggle(LED3);
      BSP_LED_Toggle(LED4);
      BSP_LED_Toggle(LED5);
      BSP_LED_Toggle(LED6);
      MCU_HAL_Delay(250);
    }
    BSP_LED_Off(LED3);
    BSP_LED_Off(LED4);
    BSP_LED_Off(LED5);
    BSP_LED_Off(LED6);
#endif

  }
