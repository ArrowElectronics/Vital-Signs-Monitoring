#include "key_detect.h"
#include "pin_config.h"
#include "app_timer.h"
#include "nrf_drv_gpiote.h"
#include "app_error.h"
#include "sdk_common.h"

#if NRF_LOG_ENABLED

#include "nrf_log.h"
#include "nrf_log_ctrl.h"
#include "nrf_log_default_backends.h"
#endif
static uint64_t m_pin_state;
static uint64_t m_pin_transition;

APP_TIMER_DEF(m_key_long_tmr);
APP_TIMER_DEF(m_key_short_tmr);  /**< Polling timer id. */

Send_key_func send_key_handle = NULL;

void Register_key_send_func(Send_key_func hander)
{
    send_key_handle = hander;
}
static void key_event_handler(uint8_t pin_no, uint8_t button_action)
{
    uint32_t           err_code;
    static uint8_t     current_long_push_pin_no;              /**< Pin number of a currently pushed button, that could become a long push if held long enough. */ 
    uint8_t key_value = 0;

    key_value = (pin_no|(button_action << 4));
    switch (button_action)
    {
        case APP_KEY_PUSH:
            
            err_code = app_timer_start(m_key_long_tmr, APP_TIMER_TICKS(LONG_PRESS_TIMEOUT_MS), (void*)&current_long_push_pin_no);
            if (err_code == NRF_SUCCESS)
            {
                current_long_push_pin_no = pin_no;
            }
            break;
        case APP_KEY_RELEASE:
            (void)app_timer_stop(m_key_long_tmr);
            break;
        case APP_KEY_LONG_PUSH:
        break;
        default:return;
    }
    if(NULL != send_key_handle)
    {
        send_key_handle(key_value);
    }
}
static void key_short_timeout_handler(void * p_context)
{
    uint8_t pin_is_set;
    if((1ULL << KEY_SELECT_PIN)&m_pin_transition)
    {
        m_pin_transition &= ~(1ULL << KEY_SELECT_PIN);
        pin_is_set = nrf_drv_gpiote_in_is_set(KEY_SELECT_PIN);
        if ((m_pin_state & (1ULL << KEY_SELECT_PIN)) == (((uint64_t)pin_is_set) << KEY_SELECT_PIN))
        {
            key_event_handler(KEY_SELECT_VALUE, pin_is_set);   
        }
    }
    if((1ULL << KEY_NAVIGATIONPIN)&m_pin_transition)
    {
        m_pin_transition &= ~(1ULL << KEY_NAVIGATIONPIN);
        pin_is_set = nrf_drv_gpiote_in_is_set(KEY_NAVIGATIONPIN);
        if ((m_pin_state & (1ULL << KEY_NAVIGATIONPIN)) == (((uint64_t)pin_is_set) << KEY_NAVIGATIONPIN))
        {
            key_event_handler(KEY_NAVIGATION_VALUE, pin_is_set);   
        }
    }
}
static void gpiote_event_handler(nrf_drv_gpiote_pin_t pin, nrf_gpiote_polarity_t action)
{
    uint32_t err_code;
    uint64_t pin_mask = 1ULL << pin;

    // Start detection timer. If timer is already running, the detection period is restarted.
    // NOTE: Using the p_context parameter of app_timer_start() to transfer the pin states to the
    //       timeout handler (by casting event_pins_mask into the equally sized void * p_context
    //       parameter).
    err_code = app_timer_stop(m_key_short_tmr);
    if (err_code != NRF_SUCCESS)
    {
        // The impact in app_button of the app_timer queue running full is losing a button press.
        // The current implementation ensures that the system will continue working as normal.
        return;
    }

    if (!(m_pin_transition & pin_mask))//fist detect the event
    {
        if (nrf_drv_gpiote_in_is_set(pin))
        {
            m_pin_state |= pin_mask;//pin status
        }
        else
        {
            m_pin_state &= ~(pin_mask);
        }
        m_pin_transition |= (pin_mask);

        err_code = app_timer_start(m_key_short_tmr, APP_TIMER_TICKS(SHORT_PRESS_TIME_MS), NULL);
        if (err_code != NRF_SUCCESS)
        {
            return;
            // The impact in app_button of the app_timer queue running full is losing a button press.
            // The current implementation ensures that the system will continue working as normal.
        }
    }
    else
    {
        m_pin_transition &= ~pin_mask;
    }
}


static void key_long_timer_handler(void * p_context)
{
    key_event_handler(*(uint8_t *)p_context, APP_KEY_LONG_PUSH);
}

uint32_t Key_detect_init(void)
{
    uint32_t err_code = NRF_SUCCESS;

    m_pin_state      = 0;
    m_pin_transition = 0;
    
    nrf_drv_gpiote_in_config_t config = GPIOTE_CONFIG_IN_SENSE_TOGGLE(false);

            
    if (!nrf_drv_gpiote_is_init())
    {
        err_code = nrf_drv_gpiote_init();
        if(NRF_SUCCESS != err_code)
        {
            NRF_LOG_INFO("key_detect gpiote init failue");
            return err_code;
        }
    }
    
    config.pull = NRF_GPIO_PIN_PULLUP;
    err_code = nrf_drv_gpiote_in_init(KEY_SELECT_PIN, &config, gpiote_event_handler);
    if(NRF_SUCCESS != err_code)
    {
        NRF_LOG_INFO("KEY_SELECT_PIN init failue,err_code = %d",err_code);
        return err_code;
    }

    config.pull = NRF_GPIO_PIN_PULLUP;
    err_code = nrf_drv_gpiote_in_init(KEY_NAVIGATIONPIN, &config, gpiote_event_handler);
    if(NRF_SUCCESS != err_code)
    {
        NRF_LOG_INFO("KEY_NAVIGATIONPIN init failue,err_code = %d",err_code);
        return err_code;
    }

    err_code = app_timer_create(&m_key_short_tmr,APP_TIMER_MODE_SINGLE_SHOT,key_short_timeout_handler);
    VERIFY_SUCCESS(err_code);

    nrf_drv_gpiote_in_event_enable(KEY_SELECT_PIN, true);
    nrf_drv_gpiote_in_event_enable(KEY_NAVIGATIONPIN, true);

    
    err_code = app_timer_create(&m_key_long_tmr,APP_TIMER_MODE_SINGLE_SHOT,key_long_timer_handler);
    VERIFY_SUCCESS(err_code);

    NRF_LOG_INFO("key_detect init success");
    return err_code;
    
}




