/* FreeRTOS related */
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"
#include "nrf_drv_rtc.h"
#include "nrf_drv_clock.h"

#if NRF_LOG_ENABLED

#include "nrf_log.h"
#include "nrf_log_ctrl.h"
#include "nrf_log_default_backends.h"

static TaskHandle_t m_logger_thread;      /**< Logger thread. */
/**
 * The size of the stack for the Logger task (in 32-bit words).
 * Logger uses sprintf internally so it is a rather stack hungry process.
 */
#define LOGGER_STACK_SIZE 512
/**
 * The priority of the Logger task.
 */
#define LOGGER_PRIORITY 1

#endif

/**
 * @brief RTC instance number used for blinking
 *
 */
#define BLINK_RTC 2

/**
 * @brief RTC compare channel used
 *
 */
#define BLINK_RTC_CC 0

/**
 * @brief Number of RTC ticks between interrupts
 */
#define BLINK_RTC_TICKS   (RTC_US_TO_TICKS(500000ULL, RTC_DEFAULT_CONFIG_FREQUENCY))


/**
 * @brief Semaphore set in RTC event
 */
static SemaphoreHandle_t m_rtc_semaphore;

/**
 * @brief RTC instance
 *
 * Instance of the RTC used for led blinking
 */
static nrf_drv_rtc_t const m_rtc = NRF_DRV_RTC_INSTANCE(BLINK_RTC);


static void blink_rtc_handler(nrf_drv_rtc_int_type_t int_type)
{
    BaseType_t yield_req = pdFALSE;
    
    ret_code_t err_code;
    if (int_type == NRF_DRV_RTC_INT_COMPARE0)
    {
        err_code = nrf_drv_rtc_cc_set(&m_rtc,BLINK_RTC_CC,(nrf_rtc_cc_get(m_rtc.p_reg, BLINK_RTC_CC) + 8) & 0xFFFFF8UL,true);
        APP_ERROR_CHECK(err_code);
        
    }
    else if (int_type == NRF_DRV_RTC_INT_TICK)
    {
//        NRF_LOG_INFO("NRF_DRV_RTC_INT_TICK,rtc_cnt = %d",rtc_cnt);   
    }
    
   /* The returned value may be safely ignored, if error is returned it only means that
    * the semaphore is already given (raised). */
   UNUSED_VARIABLE(xSemaphoreGiveFromISR(m_rtc_semaphore, &yield_req));
//   portYIELD_FROM_ISR(yield_req);
}


void rtc_thread(void * arg)
{
    ret_code_t err_code;
    static nrf_drv_rtc_config_t m_rtc_config = NRF_DRV_RTC_DEFAULT_CONFIG;
    m_rtc_config.prescaler = 4095;//8Hz
    err_code = nrf_drv_rtc_init(&m_rtc, &m_rtc_config, blink_rtc_handler);
    APP_ERROR_CHECK(err_code);
    //Enable tick event & interrupt
//    nrf_drv_rtc_tick_enable(&m_rtc,true);
    err_code = nrf_drv_rtc_cc_set(&m_rtc, BLINK_RTC_CC, 8, true);
    APP_ERROR_CHECK(err_code);
    nrf_drv_rtc_enable(&m_rtc);

    UNUSED_PARAMETER(arg);
    
    while(1)
    {
        UNUSED_RETURN_VALUE(xSemaphoreTake(m_rtc_semaphore, portMAX_DELAY));
        
    //        NRF_LOG_INFO("NRF_DRV_RTC_INT_TICK rtc_cnt = %d",rtc_cnt);  
    
    }
}

#define test_STACK_SIZE   256
#define test_PRIORITY   7
static TaskHandle_t m_rtc_thread;        /**< USB stack thread. */
void rtc_init(void)
{
    m_rtc_semaphore = xSemaphoreCreateBinary();
    ASSERT(NULL != m_rtc_semaphore);
    if (pdPASS != xTaskCreate(rtc_thread,"RTC",test_STACK_SIZE, NULL,test_PRIORITY,&m_rtc_thread))
    {
        APP_ERROR_HANDLER(NRF_ERROR_NO_MEM);
    }
    else
    {
//        NRF_LOG_INFO("test_init create success");
    }
}
