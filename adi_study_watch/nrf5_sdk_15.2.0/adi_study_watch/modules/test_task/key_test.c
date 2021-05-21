#ifdef ENABLE_WATCH_DISPLAY
#include "key_test.h"

#include "display_interface.h"
#include "key_detect.h"

static M2M2_ADDR_ENUM_t src_address;
static M2M2_ADDR_ENUM_t dest_address;

void send_key_value(uint8_t  k_value);

static void key_data_send(uint8_t value)
{
    m2m2_hdr_t *stream_mail = NULL;
    ADI_OSAL_STATUS  err;
    stream_mail = post_office_create_msg(M2M2_HEADER_SZ + sizeof(m2m2_pm_sys_key_test_data_t));
    if(NULL != stream_mail)
    {
        m2m2_pm_sys_key_test_data_t *data = (m2m2_pm_sys_key_test_data_t *)&stream_mail->data[0];
        stream_mail->src = src_address;
        stream_mail->dest = dest_address;
        data->command = M2M2_DISPLAY_APP_CMD_KEY_STREAM_DATA;
        data->status = M2M2_APP_COMMON_STATUS_OK;
        data->key_value = value;

        post_office_send(stream_mail, &err);
        stream_mail = NULL;
    }
}

void key_test_func(m2m2_hdr_t *p_msg)
{
    m2m2_hdr_t *response_mail = NULL;
    ADI_OSAL_STATUS  err;
    response_mail = post_office_create_msg(M2M2_HEADER_SZ + sizeof(m2m2_key_test_command_t));
    if(response_mail != NULL)
    {
        m2m2_key_test_command_t *resp = (m2m2_key_test_command_t *)&response_mail->data[0];
        m2m2_key_test_command_t *req1 = (m2m2_key_test_command_t *)&p_msg->data[0];
        /* send response packet */
        response_mail->src = p_msg->dest;
        src_address = p_msg->dest;
        response_mail->dest = p_msg->src;
        dest_address = p_msg->src;

        resp->command = M2M2_DISPLAY_APP_CMD_KEY_TEST_RESP;
        resp->status = M2M2_APP_COMMON_STATUS_OK;
        resp->enable = req1->enable;
        if(req1->enable != 0)
        {
            Clear_register_key_func();
            Register_key_send_func(key_data_send);
        }
        else
        {
            Unregister_key_send_func(key_data_send);
            Register_key_send_func(send_key_value);
        }

        post_office_send(response_mail, &err);
    }
}
#endif