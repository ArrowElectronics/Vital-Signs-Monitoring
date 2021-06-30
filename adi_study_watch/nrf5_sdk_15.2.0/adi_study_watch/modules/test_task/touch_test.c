
#include "touch_test.h"
#include <system_interface.h>
#include "ad7156.h"
#include "low_touch_task.h"
#include "touch_detect.h"

static M2M2_ADDR_ENUM_t src_address;
static M2M2_ADDR_ENUM_t dest_address;

static void touch1_data_send(uint8_t value)
{
    m2m2_hdr_t *stream_mail = NULL;
    ADI_OSAL_STATUS  err;
    stream_mail = post_office_create_msg(M2M2_HEADER_SZ + sizeof(m2m2_pm_sys_cap_sense_test_data_t));
    if(NULL != stream_mail)
    {
        m2m2_pm_sys_cap_sense_test_data_t *data = (m2m2_pm_sys_cap_sense_test_data_t *)&stream_mail->data[0];
        stream_mail->src = src_address;
        stream_mail->dest = dest_address;
        data->command = M2M2_PM_SYS_COMMAND_CAP_SENSE_STREAM_DATA;
        data->status = M2M2_APP_COMMON_STATUS_OK;
        data->touch_position = 1;
        data->touch_value = value;

        post_office_send(stream_mail, &err);
        stream_mail = NULL;
    }
}

#ifdef LOW_TOUCH_FEATURE
static void touch2_data_send(uint8_t value)
{
    m2m2_hdr_t *stream_mail = NULL;
    ADI_OSAL_STATUS  err;
    stream_mail = post_office_create_msg(M2M2_HEADER_SZ + sizeof(m2m2_pm_sys_cap_sense_test_data_t));
    if(NULL != stream_mail)
    {
        m2m2_pm_sys_cap_sense_test_data_t *data = (m2m2_pm_sys_cap_sense_test_data_t *)&stream_mail->data[0];
        stream_mail->src = src_address;
        stream_mail->dest = dest_address;
        data->command = M2M2_PM_SYS_COMMAND_CAP_SENSE_STREAM_DATA;
        data->touch_position = 2;
        data->touch_value = value;

        post_office_send(stream_mail, &err);
        stream_mail = NULL;
    }
}
#endif

void touch_test_func(m2m2_hdr_t *p_msg)
{
    m2m2_hdr_t *response_mail = NULL;
    ADI_OSAL_STATUS  err;
    response_mail = post_office_create_msg(M2M2_HEADER_SZ + sizeof(m2m2_pm_sys_cap_sense_test_cmd_t));
    if(response_mail != NULL)
    {
        m2m2_pm_sys_cap_sense_test_cmd_t *resp = (m2m2_pm_sys_cap_sense_test_cmd_t *)&response_mail->data[0];
        m2m2_pm_sys_cap_sense_test_cmd_t *req1 = (m2m2_pm_sys_cap_sense_test_cmd_t *)&p_msg->data[0];
        /* send response packet */
        response_mail->src = p_msg->dest;
        src_address = p_msg->dest;
        response_mail->dest = p_msg->src;
        dest_address = p_msg->src;
        if(req1->enable != 0)
        {
            top_touch_deinit();
#ifdef LOW_TOUCH_FEATURE
        if(!get_low_touch_trigger_mode2_status())
        {
            EnableLowTouchDetection(false);
            bottom_touch_func_set(1);
        }
#endif
            Register_out1_pin_detect_func(touch1_data_send);
#ifdef LOW_TOUCH_FEATURE
            Register_out2_pin_detect_func(touch2_data_send);
#endif
        }
        else
        {
            Unregister_out1_pin_detect_func(touch1_data_send);
#ifdef LOW_TOUCH_FEATURE
            Unregister_out2_pin_detect_func(touch2_data_send);
            if(!get_low_touch_trigger_mode2_status())
                bottom_touch_func_set(0);
#endif
            top_touch_init();
        }
        resp->enable = req1->enable;
        resp->command = M2M2_PM_SYS_COMMAND_CAP_SENSE_TEST_RESP;
        resp->status = M2M2_APP_COMMON_STATUS_OK;
        post_office_send(response_mail, &err);
    }
    post_office_consume_msg(p_msg);
}