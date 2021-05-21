/**
****************************************************************************
* @file     display_m2m2_protocol.c
* @author   ADI
* @version  V0.1
* @date     11-Mar-2021
* @brief    This is the source file used by display task to issue m2m2 protocol.
****************************************************************************
* @attention
******************************************************************************
* Copyright (c) 2021 Analog Devices, Inc.  All rights reserved.
*
* Redistribution and use in source and binary forms, with or without
* modification, are permitted provided that the following conditions are met:
* - Redistributions of source code must retain the above copyright notice, this
*   list of conditions and the following disclaimer.
* - Redistributions in binary form must reproduce the above copyright notice,
*   this list of conditions and the following disclaimer in the documentation
*   and/or other materials provided with the distribution.
* - Modified versions of the software must be conspicuously marked as such.
* - This software is licensed solely and exclusively for use with
*   processors/products manufactured by or for Analog Devices, Inc.
* - This software may not be combined or merged with other code in any manner
*   that would cause the software to become subject to terms and conditions
*   which differ from those listed here.
* - Neither the name of Analog Devices, Inc. nor the names of its contributors
*   may be used to endorse or promote products derived from this software
*   without specific prior written permission.
* - The use of this software may or may not infringe the patent rights of one
**   or more patent holders.  This license does not release you from the
*   requirement that you obtain separate licenses from these patent holders to
*   use this software.
*
* THIS SOFTWARE IS PROVIDED BY ANALOG DEVICES, INC. AND CONTRIBUTORS "AS IS"
* AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
* NONINFRINGEMENT, TITLE, MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
* ARE DISCLAIMED. IN NO EVENT SHALL ANALOG DEVICES, INC. OR CONTRIBUTORS BE
* LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, PUNITIVE OR
* CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, DAMAGES ARISING OUT OF
* CLAIMS OF INTELLECTUAL PROPERTY RIGHTS INFRINGEMENT; PROCUREMENT OF
* SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
* INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
* CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
* ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
* POSSIBILITY OF SUCH DAMAGE.
******************************************************************************/
#ifdef ENABLE_WATCH_DISPLAY
#include "display_m2m2_protocol.h"

#include "sensor_adpd_application_interface.h"
#include "common_sensor_interface.h"
#include "ppg_application_interface.h"
#include "ecg_application_interface.h"
#include "sync_data_application_interface.h"
#include "file_system_interface.h"
#include <post_office.h>

#include "nrf_log.h"
#include "nrf_log_ctrl.h"
#include "nrf_log_default_backends.h"

logging_option_u logging_status_value = {0};
logging_option_u logging_set_value = {0};
logging_option_u logging_check_flag = {0};

extern uint8_t dvt2;

uint32_t m2m2_ecg_write_lcfg(uint8_t *status)
{

    m2m2_hdr_t *p_m2m2_t = NULL;
    m2m2_hdr_t *p_m2m2_r = NULL;

    ecg_app_lcfg_op_hdr_t *data_t = NULL;
    ecg_app_lcfg_op_hdr_t *data_r = NULL;
    ADI_OSAL_STATUS       err = ADI_OSAL_SUCCESS;;
    p_m2m2_t = post_office_create_msg(M2M2_HEADER_SZ+sizeof(ecg_app_lcfg_op_hdr_t));

    if (p_m2m2_t == NULL)
    {
        return M2M2_NO_MEMORY;
    }
    /* swap from network byte order to little endian */
    p_m2m2_t->src = M2M2_ADDR_DISPLAY;
    p_m2m2_t->dest = M2M2_ADDR_MED_ECG;
    p_m2m2_t->length = M2M2_HEADER_SZ+sizeof(ecg_app_lcfg_op_hdr_t);

    data_t = (ecg_app_lcfg_op_hdr_t *)p_m2m2_t->data;
    data_t->command = M2M2_APP_COMMON_CMD_WRITE_LCFG_REQ;
    data_t->num_ops = 1;
    data_t->ops[0].field = 0;
    data_t->ops[0].value = 100;
    post_office_send(p_m2m2_t,&err);
    if(ADI_OSAL_SUCCESS != err)
    {
        return M2M2_SEND_ERROR;
    }

    while(1)
    {
        p_m2m2_r = post_office_get(M2M2_TIMEOUT_PERIOD, APP_OS_CFG_DISPLAY_TASK_INDEX);//wait 10 ms
        if((p_m2m2_r == NULL)||(p_m2m2_r->data == NULL))
        {
            NRF_LOG_INFO("post_office_get error!");
            return M2M2_WAIT_TIMEOUT;
        }
        if((M2M2_ADDR_MED_ECG == p_m2m2_r->src)&&(M2M2_APP_COMMON_CMD_WRITE_LCFG_RESP == p_m2m2_r->data[0]))
        {
            break;
        }
        else
        {
            post_office_consume_msg(p_m2m2_r);
        }
    }

    data_r = (ecg_app_lcfg_op_hdr_t *)&p_m2m2_r->data[0];
    if(NULL != status)
    {
        *status = data_r->status;
    }
    post_office_consume_msg(p_m2m2_r);
    return M2M2_SUCCESS;

}

uint32_t m2m2_ppg_load_adpd_cfg(uint8_t *status)
{

    m2m2_hdr_t *p_m2m2_t = NULL;
    m2m2_hdr_t *p_m2m2_r = NULL;
    m2m2_sensor_adpd_resp_t *data_t = NULL;
    m2m2_sensor_adpd_resp_t *data_r = NULL;
    ADI_OSAL_STATUS       err = ADI_OSAL_SUCCESS;;
    p_m2m2_t = post_office_create_msg(M2M2_HEADER_SZ+sizeof(m2m2_sensor_adpd_resp_t));
    if (p_m2m2_t == NULL)
    {
        return M2M2_NO_MEMORY;
    }
    /* swap from network byte order to little endian */
    p_m2m2_t->src = M2M2_ADDR_DISPLAY;
    p_m2m2_t->dest = M2M2_ADDR_SENSOR_ADPD4000;
    p_m2m2_t->length = M2M2_HEADER_SZ+sizeof(m2m2_sensor_adpd_resp_t);

    data_t = (m2m2_sensor_adpd_resp_t *)p_m2m2_t->data;
    data_t->command = M2M2_SENSOR_ADPD_COMMAND_LOAD_CFG_REQ;
    data_t->deviceid = M2M2_SENSOR_ADPD4000_DEVICE_4000_G;
    post_office_send(p_m2m2_t,&err);
    if(ADI_OSAL_SUCCESS != err)
    {
        return M2M2_SEND_ERROR;
    }

    while(1)
    {
        p_m2m2_r = post_office_get(M2M2_TIMEOUT_PERIOD, APP_OS_CFG_DISPLAY_TASK_INDEX);//wait 10 ms
        if((p_m2m2_r == NULL)||(p_m2m2_r->data == NULL))
        {
            NRF_LOG_INFO("post_office_get error!");
            return M2M2_WAIT_TIMEOUT;
        }
        if((M2M2_ADDR_SENSOR_ADPD4000 == p_m2m2_r->src)&&(M2M2_SENSOR_ADPD_COMMAND_LOAD_CFG_RESP == p_m2m2_r->data[0]))
        {
            break;
        }
        else
        {
            post_office_consume_msg(p_m2m2_r);
        }
    }

    data_r = (m2m2_sensor_adpd_resp_t *)&p_m2m2_r->data[0];
    if(NULL != status)
    {
        *status = data_r->status;
    }
    post_office_consume_msg(p_m2m2_r);
    return M2M2_SUCCESS;

}

uint32_t m2m2_ppg_clock_calibration(uint8_t *status)
{
    m2m2_hdr_t *p_m2m2_t = NULL;
    m2m2_hdr_t *p_m2m2_r = NULL;
    m2m2_sensor_clockcal_resp_t *data_t = NULL;
    m2m2_sensor_clockcal_resp_t *data_r = NULL;
    ADI_OSAL_STATUS       err = ADI_OSAL_SUCCESS;
    p_m2m2_t = post_office_create_msg(M2M2_HEADER_SZ+sizeof(m2m2_sensor_clockcal_resp_t));
    if (p_m2m2_t == NULL)
    {
        return M2M2_NO_MEMORY;
    }

    p_m2m2_t->src = M2M2_ADDR_DISPLAY;
    p_m2m2_t->dest = M2M2_ADDR_SENSOR_ADPD4000;
    p_m2m2_t->length = M2M2_HEADER_SZ+sizeof(m2m2_sensor_clockcal_resp_t);

    data_t = (m2m2_sensor_clockcal_resp_t *)p_m2m2_t->data;
    data_t->command = M2M2_SENSOR_ADPD_COMMAND_CLOCK_CAL_REQ;
    /*On DVT2 with ADPD4100, clock calibration with clk id = 2 works*/
    if(dvt2)
      data_t->clockcalid = 2;
    /*On DVT1 & other boards with ADPD4000, clock calibration with clk id = 6 works*/
    else
      data_t->clockcalid = 6;
    post_office_send(p_m2m2_t,&err);
    if(ADI_OSAL_SUCCESS != err)
    {
        return M2M2_SEND_ERROR;
    }

    while(1)
    {
        p_m2m2_r = post_office_get(M2M2_TIMEOUT_PERIOD, APP_OS_CFG_DISPLAY_TASK_INDEX);//wait 10 ms
        if((p_m2m2_r == NULL)||(p_m2m2_r->data == NULL))
        {
            NRF_LOG_INFO("post_office_get error!");
            return M2M2_WAIT_TIMEOUT;
        }
        if((M2M2_ADDR_SENSOR_ADPD4000 == p_m2m2_r->src)&&(M2M2_SENSOR_ADPD_COMMAND_CLOCK_CAL_RESP == p_m2m2_r->data[0]))
        {
            break;
        }
        else
        {
            post_office_consume_msg(p_m2m2_r);
        }
    }


    data_r = (m2m2_sensor_clockcal_resp_t *)&p_m2m2_r->data[0];
    if(NULL != status)
    {
        *status = data_r->status;
    }
    post_office_consume_msg(p_m2m2_r);
    return M2M2_SUCCESS;

}

uint32_t m2m2_set_ppg_configuration(uint8_t *status)
{
    m2m2_hdr_t *p_m2m2_t = NULL;
    m2m2_hdr_t *p_m2m2_r = NULL;
    ppg_app_set_lcfg_req_t *data_t = NULL;
    ppg_app_set_lcfg_req_t *data_r = NULL;
    ADI_OSAL_STATUS       err = ADI_OSAL_SUCCESS;
    p_m2m2_t = post_office_create_msg(M2M2_HEADER_SZ+sizeof(ppg_app_set_lcfg_req_t));
    if (p_m2m2_t == NULL)
    {
        return M2M2_NO_MEMORY;
    }

    p_m2m2_t->src = M2M2_ADDR_DISPLAY;
    p_m2m2_t->dest = M2M2_ADDR_MED_PPG;
    p_m2m2_t->length = M2M2_HEADER_SZ+sizeof(ppg_app_set_lcfg_req_t);

    data_t = (ppg_app_set_lcfg_req_t *)p_m2m2_t->data;
    data_t->command = M2M2_APP_COMMON_CMD_SET_LCFG_REQ;
    data_t->lcfgid = M2M2_SENSOR_PPG_LCFG_ID_ADPD4000;
    post_office_send(p_m2m2_t,&err);
    if(ADI_OSAL_SUCCESS != err)
    {
        return M2M2_SEND_ERROR;
    }
    while(1)
    {
        p_m2m2_r = post_office_get(M2M2_TIMEOUT_PERIOD, APP_OS_CFG_DISPLAY_TASK_INDEX);//wait 10 ms
        if((p_m2m2_r == NULL)||(p_m2m2_r->data == NULL))
        {
            NRF_LOG_INFO("post_office_get error!");
            return M2M2_WAIT_TIMEOUT;
        }
        if((M2M2_ADDR_MED_PPG == p_m2m2_r->src)&&(M2M2_APP_COMMON_CMD_SET_LCFG_RESP == p_m2m2_r->data[0]))
        {
            break;
        }
        else
        {
            post_office_consume_msg(p_m2m2_r);
        }
    }


    data_r = (ppg_app_set_lcfg_req_t *)&p_m2m2_r->data[0];
    if(NULL != status)
    {
        *status = data_r->status;
    }
    post_office_consume_msg(p_m2m2_r);
    return M2M2_SUCCESS;

}

uint32_t m2m2_start_fs_log(uint8_t *status)
{
    m2m2_hdr_t *p_m2m2_t = NULL;
    m2m2_hdr_t *p_m2m2_r = NULL;
    m2m2_app_common_sub_op_t *data_t = NULL;
    m2m2_app_common_sub_op_t *data_r = NULL;
    ADI_OSAL_STATUS       err = ADI_OSAL_SUCCESS;
    p_m2m2_t = post_office_create_msg(M2M2_HEADER_SZ+sizeof(m2m2_app_common_sub_op_t));
    if (p_m2m2_t == NULL)
    {
        return M2M2_NO_MEMORY;
    }

    p_m2m2_t->src = M2M2_ADDR_DISPLAY;
    p_m2m2_t->dest = M2M2_ADDR_SYS_FS;
    p_m2m2_t->length = M2M2_HEADER_SZ+sizeof(m2m2_app_common_sub_op_t);

    data_t = (m2m2_app_common_sub_op_t *)p_m2m2_t->data;
    data_t->command = M2M2_FILE_SYS_CMD_START_LOGGING_REQ;
    post_office_send(p_m2m2_t,&err);
    if(ADI_OSAL_SUCCESS != err)
    {
        return M2M2_SEND_ERROR;
    }
    while(1)
    {
        p_m2m2_r = post_office_get(M2M2_TIMEOUT_PERIOD, APP_OS_CFG_DISPLAY_TASK_INDEX);//wait 10 ms
        if((p_m2m2_r == NULL)||(p_m2m2_r->data == NULL))
        {
            NRF_LOG_INFO("post_office_get error!");
            return M2M2_WAIT_TIMEOUT;
        }
        if((M2M2_ADDR_SYS_FS == p_m2m2_r->src)&&(M2M2_FILE_SYS_CMD_START_LOGGING_RESP == p_m2m2_r->data[0]))
        {
            break;
        }
        else
        {
            post_office_consume_msg(p_m2m2_r);
        }
    }


    data_r = (m2m2_app_common_sub_op_t *)&p_m2m2_r->data[0];
    if(NULL != status)
    {
        *status = data_r->status;
    }
    post_office_consume_msg(p_m2m2_r);
    return M2M2_SUCCESS;
}

uint32_t m2m2_add_fs_subscribe(M2M2_ADDR_ENUM_t sub_addr,uint8_t *status)
{
    m2m2_hdr_t *p_m2m2_t = NULL;
    m2m2_hdr_t *p_m2m2_r = NULL;
    m2m2_file_sys_log_stream_t *data_t = NULL;
    m2m2_file_sys_log_stream_t *data_r = NULL;
    ADI_OSAL_STATUS       err = ADI_OSAL_SUCCESS;
    p_m2m2_t = post_office_create_msg(M2M2_HEADER_SZ+sizeof(m2m2_file_sys_log_stream_t));
    if (p_m2m2_t == NULL)
    {
        return M2M2_NO_MEMORY;
    }

    p_m2m2_t->src = M2M2_ADDR_DISPLAY;
    p_m2m2_t->dest = M2M2_ADDR_SYS_FS;
    p_m2m2_t->length = M2M2_HEADER_SZ+sizeof(m2m2_file_sys_log_stream_t);

    data_t = (m2m2_file_sys_log_stream_t *)p_m2m2_t->data;
    data_t->command = M2M2_FILE_SYS_CMD_LOG_STREAM_REQ;
    data_t->stream = sub_addr;
    post_office_send(p_m2m2_t,&err);
    if(ADI_OSAL_SUCCESS != err)
    {
        return M2M2_SEND_ERROR;
    }
    while(1)
    {
        p_m2m2_r = post_office_get(M2M2_TIMEOUT_PERIOD, APP_OS_CFG_DISPLAY_TASK_INDEX);//wait 10 ms
        if((p_m2m2_r == NULL)||(p_m2m2_r->data == NULL))
        {
            NRF_LOG_INFO("post_office_get error!");
            return M2M2_WAIT_TIMEOUT;
        }
        if((M2M2_ADDR_SYS_FS == p_m2m2_r->src)&&(M2M2_FILE_SYS_CMD_LOG_STREAM_RESP == p_m2m2_r->data[0]))
        {
            break;
        }
        else
        {
            post_office_consume_msg(p_m2m2_r);
        }
    }


    data_r = (m2m2_file_sys_log_stream_t *)&p_m2m2_r->data[0];
    if(NULL != status)
    {
        *status = data_r->status;
    }
    post_office_consume_msg(p_m2m2_r);
    return M2M2_SUCCESS;

}

uint32_t m2m2_stop_fs_log(uint8_t *status)
{
    m2m2_hdr_t *p_m2m2_t = NULL;
    m2m2_hdr_t *p_m2m2_r = NULL;
    m2m2_file_sys_stop_log_cmd_t *data_t = NULL;
    m2m2_file_sys_stop_log_cmd_t *data_r = NULL;
    ADI_OSAL_STATUS       err = ADI_OSAL_SUCCESS;
    p_m2m2_t = post_office_create_msg(M2M2_HEADER_SZ+sizeof(m2m2_file_sys_stop_log_cmd_t));
    if (p_m2m2_t == NULL)
    {
        return M2M2_NO_MEMORY;
    }

    p_m2m2_t->src = M2M2_ADDR_DISPLAY;
    p_m2m2_t->dest = M2M2_ADDR_SYS_FS;
    p_m2m2_t->length = M2M2_HEADER_SZ+sizeof(m2m2_file_sys_stop_log_cmd_t);

    data_t = (m2m2_file_sys_stop_log_cmd_t *)p_m2m2_t->data;
    data_t->stop_type = M2M2_FILE_SYS_STOP_LOGGING;
    data_t->command = M2M2_FILE_SYS_CMD_STOP_LOGGING_REQ;
    post_office_send(p_m2m2_t,&err);
    if(ADI_OSAL_SUCCESS != err)
    {
        return M2M2_SEND_ERROR;
    }
    while(1)
    {
        p_m2m2_r = post_office_get(M2M2_TIMEOUT_LONG_WAIT, APP_OS_CFG_DISPLAY_TASK_INDEX);//wait 10 ms
        if((p_m2m2_r == NULL)||(p_m2m2_r->data == NULL))
        {
            NRF_LOG_INFO("post_office_get error!");
            return M2M2_WAIT_TIMEOUT;
        }
        if((M2M2_ADDR_SYS_FS == p_m2m2_r->src)&&(M2M2_FILE_SYS_CMD_STOP_LOGGING_RESP == p_m2m2_r->data[0]))
        {
            break;
        }
        else
        {
            post_office_consume_msg(p_m2m2_r);
        }
    }


    data_r = (m2m2_file_sys_stop_log_cmd_t *)&p_m2m2_r->data[0];
    if(NULL != status)
    {
        *status = data_r->status;
    }
    post_office_consume_msg(p_m2m2_r);
    return M2M2_SUCCESS;
}

uint32_t m2m2_remove_fs_subscribe(M2M2_ADDR_ENUM_t sub_addr,uint8_t *status)
{
    m2m2_hdr_t *p_m2m2_t = NULL;
    m2m2_hdr_t *p_m2m2_r = NULL;
    m2m2_file_sys_log_stream_t *data_t = NULL;
    m2m2_file_sys_log_stream_t *data_r = NULL;
    ADI_OSAL_STATUS       err = ADI_OSAL_SUCCESS;
    p_m2m2_t = post_office_create_msg(M2M2_HEADER_SZ+sizeof(m2m2_file_sys_log_stream_t));
    if (p_m2m2_t == NULL)
    {
        return M2M2_NO_MEMORY;
    }

    p_m2m2_t->src = M2M2_ADDR_DISPLAY;
    p_m2m2_t->dest = M2M2_ADDR_SYS_FS;
    p_m2m2_t->length = M2M2_HEADER_SZ+sizeof(m2m2_file_sys_log_stream_t);

    data_t = (m2m2_file_sys_log_stream_t *)p_m2m2_t->data;
    data_t->command = M2M2_FILE_SYS_CMD_STOP_STREAM_REQ;
    data_t->stream = sub_addr;
    post_office_send(p_m2m2_t,&err);

    if(ADI_OSAL_SUCCESS != err)
    {
        return M2M2_SEND_ERROR;
    }

    while(1)
    {
        p_m2m2_r = post_office_get(M2M2_TIMEOUT_PERIOD, APP_OS_CFG_DISPLAY_TASK_INDEX);//wait 10 ms
        if((p_m2m2_r == NULL)||(p_m2m2_r->data == NULL))
        {
            NRF_LOG_INFO("post_office_get error!");
            return M2M2_WAIT_TIMEOUT;
        }
        if((M2M2_ADDR_SYS_FS == p_m2m2_r->src)&&(M2M2_FILE_SYS_CMD_STOP_STREAM_RESP == p_m2m2_r->data[0]))
        {
            break;
        }
        else
        {
            post_office_consume_msg(p_m2m2_r);
        }
    }


    data_r = (m2m2_file_sys_log_stream_t *)&p_m2m2_r->data[0];
    if(NULL != status)
    {
        *status = data_r->status;
    }
    post_office_consume_msg(p_m2m2_r);
    return M2M2_SUCCESS;
}

uint32_t m2m2_check_sensor_status(M2M2_ADDR_ENUM_t dest_addr,uint8_t *status,uint8_t *subs_num,uint8_t *starts_num)
{
    m2m2_hdr_t *p_m2m2_t = NULL;
    m2m2_hdr_t *p_m2m2_r = NULL;
    m2m2_app_common_status_t *data_t = NULL;
    m2m2_app_common_status_t *data_r = NULL;
    ADI_OSAL_STATUS       err = ADI_OSAL_SUCCESS;
    p_m2m2_t = post_office_create_msg(M2M2_HEADER_SZ+sizeof(m2m2_app_common_status_t));
    if (p_m2m2_t == NULL)
    {
        return M2M2_NO_MEMORY;
    }

    p_m2m2_t->src = M2M2_ADDR_DISPLAY;
    p_m2m2_t->dest = dest_addr;
    p_m2m2_t->length = M2M2_HEADER_SZ+sizeof(m2m2_app_common_status_t);

    data_t = (m2m2_app_common_status_t *)p_m2m2_t->data;
    data_t->command = M2M2_APP_COMMON_CMD_SENSOR_STATUS_QUERY_REQ;
    post_office_send(p_m2m2_t,&err);
    if(ADI_OSAL_SUCCESS != err)
    {
        return M2M2_SEND_ERROR;
    }
    while(1)
    {
        p_m2m2_r = post_office_get(M2M2_TIMEOUT_PERIOD, APP_OS_CFG_DISPLAY_TASK_INDEX);//wait 10 ms
        if((p_m2m2_r == NULL)||(p_m2m2_r->data == NULL))
        {
            NRF_LOG_INFO("post_office_get error!");
            return M2M2_WAIT_TIMEOUT;
        }
        if((dest_addr == p_m2m2_r->src)&&(M2M2_APP_COMMON_CMD_SENSOR_STATUS_QUERY_RESP == p_m2m2_r->data[0]))
        {
            break;
        }
        else
        {
            post_office_consume_msg(p_m2m2_r);
        }
    }


    data_r = (m2m2_app_common_status_t *)&p_m2m2_r->data[0];
    if(NULL != status)
    {
        *status = data_r->status;
    }
    if(NULL != subs_num)
    {
        *subs_num = data_r->num_subscribers;
    }
    if(NULL != starts_num)
    {
        *starts_num = data_r->num_start_reqs;
    }
    post_office_consume_msg(p_m2m2_r);
    return M2M2_SUCCESS;
}

extern uint8_t get_fs_logging_status(void);
uint32_t m2m2_check_fs_status(uint8_t *status)
{
  if(status != NULL)
  {
    *status = get_fs_logging_status();
    return M2M2_SUCCESS;
  }
  else
  {
    return M2M2_STATUS_ERROR;
  }
}

uint32_t m2m2_check_fs_sub_status(M2M2_ADDR_ENUM_t sub_addr,uint8_t *status)
{
    m2m2_hdr_t *p_m2m2_t = NULL;
    m2m2_hdr_t *p_m2m2_r = NULL;
    m2m2_app_common_sub_op_t *data_t = NULL;
    m2m2_file_sys_get_subs_status_resp_t *data_r = NULL;
    ADI_OSAL_STATUS       err = ADI_OSAL_SUCCESS;
    p_m2m2_t = post_office_create_msg(M2M2_HEADER_SZ+sizeof(m2m2_app_common_sub_op_t));
    if (p_m2m2_t == NULL)
    {
        return M2M2_NO_MEMORY;
    }

    p_m2m2_t->src = M2M2_ADDR_DISPLAY;
    p_m2m2_t->dest = M2M2_ADDR_SYS_FS;
    p_m2m2_t->length = M2M2_HEADER_SZ+sizeof(m2m2_app_common_sub_op_t);

    data_t = (m2m2_app_common_sub_op_t *)p_m2m2_t->data;
    data_t->command = M2M2_FILE_SYS_CMD_GET_FS_STREAM_SUB_STATUS_REQ;
    data_t->stream = sub_addr;
    post_office_send(p_m2m2_t,&err);
    if(ADI_OSAL_SUCCESS != err)
    {
        return M2M2_SEND_ERROR;
    }
    while(1)
    {
        p_m2m2_r = post_office_get(M2M2_TIMEOUT_PERIOD, APP_OS_CFG_DISPLAY_TASK_INDEX);//wait 10 ms
        if((p_m2m2_r == NULL)||(p_m2m2_r->data == NULL))
        {
            NRF_LOG_INFO("post_office_get error!");
            return M2M2_WAIT_TIMEOUT;
        }
        if((M2M2_ADDR_SYS_FS == p_m2m2_r->src)&&(M2M2_FILE_SYS_CMD_GET_FS_STREAM_SUB_STATUS_RESP == p_m2m2_r->data[0]))
        {
            break;
        }
        else
        {
            post_office_consume_msg(p_m2m2_r);
        }
    }


    data_r = (m2m2_file_sys_get_subs_status_resp_t *)&p_m2m2_r->data[0];

    if(NULL != status)
    {
        *status = data_r->subs_state;
    }
    post_office_consume_msg(p_m2m2_r);
    return M2M2_SUCCESS;
}

uint32_t m2m2_set_fs_status(uint8_t en)
{
    uint32_t err;
    uint8_t status;

    err = m2m2_check_fs_status(&status);
    APP_ERROR_CHECK(err);
    if(0 != en)
    {
        if(M2M2_FILE_SYS_STATUS_LOGGING_IN_PROGRESS != status)
        {
            err = m2m2_start_fs_log(&status);
            APP_ERROR_CHECK(err);
            if((M2M2_FILE_SYS_STATUS_LOGGING_IN_PROGRESS != status)
            &&(M2M2_FILE_SYS_STATUS_OK != status))
            {
                NRF_LOG_INFO("fs log start fail,status = %d",status);
                return M2M2_STATUS_ERROR;
            }
        }
    }
    else
    {
        if(M2M2_FILE_SYS_STATUS_LOGGING_IN_PROGRESS == status)
        {
            err = m2m2_stop_fs_log(&status);
            APP_ERROR_CHECK(err);
            if(M2M2_FILE_SYS_STATUS_LOGGING_STOPPED != status)
            {
                NRF_LOG_INFO("fs log stop fail,status = %d",status);
                return M2M2_STATUS_ERROR;
            }
#ifdef LOW_TOUCH_FEATURE
extern uint8_t gStopCmdEnable;
        gStopCmdEnable =1;      //Enable sending stop commands
#endif //LOW_TOUCH_FEATURE
        }
    }
    return M2M2_SUCCESS;
}

uint32_t m2m2_set_fs_subscribe(uint8_t sensor,uint8_t en)
{
    uint32_t err;
    uint8_t status;
    M2M2_ADDR_ENUM_t sub_addr;

    switch(sensor)
    {
        case SENSOR_PPG:
        {
            sub_addr = M2M2_ADDR_MED_PPG_STREAM;
        }
        break;
        case SENSOR_ECG:
        {
            sub_addr = M2M2_ADDR_MED_ECG_STREAM;
        }
        break;
        default:
        {
            return M2M2_VALUE_OVERFLOW;
        }
    }
    if(0 != en)
    {
        err = m2m2_add_fs_subscribe(sub_addr,&status);
        APP_ERROR_CHECK(err);
        if(M2M2_APP_COMMON_STATUS_SUBSCRIBER_ADDED != status)
        {
            NRF_LOG_INFO("add_fs_subscribe fail,status = %d",status);
            return M2M2_STATUS_ERROR;
        }
    }
    else
    {
        err = m2m2_remove_fs_subscribe(sub_addr,&status);
        APP_ERROR_CHECK(err);
        if((M2M2_APP_COMMON_STATUS_SUBSCRIBER_REMOVED != status)
        &&(M2M2_APP_COMMON_STATUS_SUBSCRIBER_COUNT_DECREMENT != status))
        {
            NRF_LOG_INFO("remove fs subscribe ppg fail,status = %d",status);
            return M2M2_STATUS_ERROR;
        }
    }
    return M2M2_SUCCESS;
}

uint32_t m2m2_set_stream_status(uint8_t sensor,uint8_t en)
{
    uint8_t status;
    uint8_t subs_num;
    uint32_t err;
    switch(sensor)
    {
        case SENSOR_PPG:
        {
            err = m2m2_check_sensor_status(M2M2_ADDR_MED_PPG,&status,&subs_num,NULL);
            APP_ERROR_CHECK(err);
            NRF_LOG_INFO("check stream,status = %d,subs_num = %d",status,subs_num);
            if(0 != en)
            {
                if((M2M2_APP_COMMON_STATUS_STREAM_IN_PROGRESS != status)
                &&(M2M2_APP_COMMON_STATUS_STREAM_STARTED != status))
                {
                    err = m2m2_ppg_load_adpd_cfg(&status);
                    APP_ERROR_CHECK(err);
                    if(M2M2_APP_COMMON_STATUS_OK != status)
                    {
                        NRF_LOG_INFO("load adpd configuration fail,status = %d",status);
                        return M2M2_STATUS_ERROR;
                    }
                    err = m2m2_ppg_clock_calibration(&status);
                    APP_ERROR_CHECK(err);
                    if(M2M2_APP_COMMON_STATUS_OK != status)
                    {
                        NRF_LOG_INFO("clock calibration fail,status = %d",status);
                        return M2M2_STATUS_ERROR;
                    }
                    err = m2m2_set_ppg_configuration(&status);
                    APP_ERROR_CHECK(err);
                    if(M2M2_APP_COMMON_STATUS_OK != status)
                    {
                        NRF_LOG_INFO("set ppg configuration fail,status = %d",status);
                        return M2M2_STATUS_ERROR;
                    }

                }
                err = m2m2_start_stream(M2M2_ADDR_MED_PPG,&status);//
                APP_ERROR_CHECK(err);
                if((M2M2_APP_COMMON_STATUS_STREAM_STARTED != status)&&
                (M2M2_APP_COMMON_STATUS_STREAM_IN_PROGRESS != status))
                {
                    NRF_LOG_INFO("sensor ppg start fail,status = %d",status);
                    return M2M2_STATUS_ERROR;
                }
            }
            else
            {
                //if((M2M2_APP_COMMON_STATUS_STREAM_IN_PROGRESS == status)
                //||(M2M2_APP_COMMON_STATUS_STREAM_STARTED == status))
                {
                    err = m2m2_stop_stream(M2M2_ADDR_MED_PPG,&status);
                    APP_ERROR_CHECK(err);
                    NRF_LOG_INFO("stop stream,status = %d",status);
                    if(M2M2_APP_COMMON_STATUS_STREAM_STOPPED != status)
                    {
                        NRF_LOG_INFO("stop stream fail,status = %d",status);
                        return M2M2_STATUS_ERROR;
                    }
                }
            }
        }
        break;
        case SENSOR_ECG:
        {
            err = m2m2_check_sensor_status(M2M2_ADDR_MED_ECG,&status,&subs_num,NULL);
            NRF_LOG_INFO("ECG check stream,status = %d,subs_num = %d",status,subs_num);
            APP_ERROR_CHECK(err);
            //if(0 == subs_num)

            if(0 != en)
            {
                if((M2M2_APP_COMMON_STATUS_STREAM_IN_PROGRESS != status)
                &&(M2M2_APP_COMMON_STATUS_STREAM_STARTED != status))
                {
                    err = m2m2_ecg_write_lcfg(&status);
                    APP_ERROR_CHECK(err);
                    if(M2M2_APP_COMMON_STATUS_OK != status)
                    {
                        NRF_LOG_INFO("check_sensor_status fail,status = %d",status);
                        return M2M2_STATUS_ERROR;
                    }
                }
                err = m2m2_start_stream(M2M2_ADDR_MED_ECG,&status);//
                APP_ERROR_CHECK(err);
                if((M2M2_APP_COMMON_STATUS_STREAM_STARTED != status)&&
                (M2M2_APP_COMMON_STATUS_STREAM_IN_PROGRESS != status))
                {
                    NRF_LOG_INFO("sensor ecg start fail,status = %d",status);
                    return M2M2_STATUS_ERROR;
                }
            }
            else
            {
                err = m2m2_stop_stream(M2M2_ADDR_MED_ECG,&status);
                NRF_LOG_INFO("ECG stop stream,status = %d",status);
                APP_ERROR_CHECK(err);
                if(M2M2_APP_COMMON_STATUS_STREAM_STOPPED != status)
                {
                    NRF_LOG_INFO("stop stream fail,status = %d",status);
                    return M2M2_STATUS_ERROR;
                }
            }
        }
        break;
        default:
        {
            return M2M2_VALUE_OVERFLOW;
        }
    }
    return M2M2_SUCCESS;
}

uint32_t m2m2_subscribe_stream(M2M2_ADDR_ENUM_t dest_addr,M2M2_ADDR_ENUM_t sub_addr,uint8_t *status)
{
    m2m2_hdr_t *p_m2m2_t = NULL;
    m2m2_hdr_t *p_m2m2_r = NULL;
    m2m2_app_common_sub_op_t *data_t = NULL;
    m2m2_app_common_sub_op_t *data_r = NULL;
    ADI_OSAL_STATUS       err = ADI_OSAL_SUCCESS;
    p_m2m2_t = post_office_create_msg(M2M2_HEADER_SZ+sizeof(m2m2_app_common_sub_op_t));
    if (p_m2m2_t == NULL)
    {
        return M2M2_NO_MEMORY;
    }

    p_m2m2_t->src = M2M2_ADDR_DISPLAY;
    p_m2m2_t->dest = dest_addr;
    p_m2m2_t->length = M2M2_HEADER_SZ+sizeof(m2m2_app_common_sub_op_t);

    data_t = (m2m2_app_common_sub_op_t *)p_m2m2_t->data;
    data_t->command = M2M2_APP_COMMON_CMD_STREAM_SUBSCRIBE_REQ;
    data_t->stream = sub_addr;
    post_office_send(p_m2m2_t,&err);
    if(ADI_OSAL_SUCCESS != err)
    {
        return M2M2_SEND_ERROR;
    }

    while(1)
    {
        p_m2m2_r = post_office_get(M2M2_TIMEOUT_PERIOD, APP_OS_CFG_DISPLAY_TASK_INDEX);//wait 10 ms
        if((p_m2m2_r == NULL)||(p_m2m2_r->data == NULL))
        {
            NRF_LOG_INFO("post_office_get error!");
            return M2M2_WAIT_TIMEOUT;
        }
        if((dest_addr == p_m2m2_r->src)&&(M2M2_APP_COMMON_CMD_STREAM_SUBSCRIBE_RESP == p_m2m2_r->data[0]))
        {
            break;
        }
        else
        {
            post_office_consume_msg(p_m2m2_r);
        }
    }


    data_r = (m2m2_app_common_sub_op_t *)&p_m2m2_r->data[0];
    if(NULL != status)
    {
        *status = data_r->status;
    }
    post_office_consume_msg(p_m2m2_r);
    return M2M2_SUCCESS;

}

uint32_t m2m2_start_stream(M2M2_ADDR_ENUM_t dest_addr,uint8_t *status)
{
    m2m2_hdr_t *p_m2m2_t = NULL;
    m2m2_hdr_t *p_m2m2_r = NULL;
    m2m2_app_common_sub_op_t *data_t = NULL;
    m2m2_app_common_sub_op_t *data_r = NULL;
    ADI_OSAL_STATUS       err = ADI_OSAL_SUCCESS;
    p_m2m2_t = post_office_create_msg(M2M2_HEADER_SZ+sizeof(m2m2_app_common_sub_op_t));
    if (p_m2m2_t == NULL)
    {
        return M2M2_NO_MEMORY;
    }

    p_m2m2_t->src = M2M2_ADDR_DISPLAY;
    p_m2m2_t->dest = dest_addr;
    p_m2m2_t->length = M2M2_HEADER_SZ+sizeof(m2m2_app_common_sub_op_t);

    data_t = (m2m2_app_common_sub_op_t *)p_m2m2_t->data;
    data_t->command = M2M2_APP_COMMON_CMD_STREAM_START_REQ;
    post_office_send(p_m2m2_t,&err);
    if(ADI_OSAL_SUCCESS != err)
    {
        return M2M2_SEND_ERROR;
    }

    while(1)
    {
        p_m2m2_r = post_office_get(M2M2_TIMEOUT_PERIOD, APP_OS_CFG_DISPLAY_TASK_INDEX);//wait 10 ms
        if((p_m2m2_r == NULL)||(p_m2m2_r->data == NULL))
        {
            NRF_LOG_INFO("post_office_get error!");
            return M2M2_WAIT_TIMEOUT;
        }
        if((dest_addr == p_m2m2_r->src)&&(M2M2_APP_COMMON_CMD_STREAM_START_RESP == p_m2m2_r->data[0]))
        {
            break;
        }
        else
        {
            post_office_consume_msg(p_m2m2_r);
        }
    }


    data_r = (m2m2_app_common_sub_op_t *)&p_m2m2_r->data[0];
    if(NULL != status)
    {
        *status = data_r->status;
    }
    post_office_consume_msg(p_m2m2_r);
    return M2M2_SUCCESS;
}

uint32_t m2m2_stop_stream(M2M2_ADDR_ENUM_t dest_addr,uint8_t *status)
{
    m2m2_hdr_t *p_m2m2_t = NULL;
    m2m2_hdr_t *p_m2m2_r = NULL;
    m2m2_app_common_sub_op_t *data_t = NULL;
    m2m2_app_common_sub_op_t *data_r = NULL;
    ADI_OSAL_STATUS       err = ADI_OSAL_SUCCESS;
    p_m2m2_t = post_office_create_msg(M2M2_HEADER_SZ+sizeof(m2m2_app_common_sub_op_t));
    if (p_m2m2_t == NULL)
    {
        return M2M2_NO_MEMORY;
    }

    p_m2m2_t->src = M2M2_ADDR_DISPLAY;
    p_m2m2_t->dest = dest_addr;
    p_m2m2_t->length = M2M2_HEADER_SZ+sizeof(m2m2_app_common_sub_op_t);

    data_t = (m2m2_app_common_sub_op_t *)p_m2m2_t->data;
    data_t->command = M2M2_APP_COMMON_CMD_STREAM_STOP_REQ;
    post_office_send(p_m2m2_t,&err);
    if(ADI_OSAL_SUCCESS != err)
    {
        return M2M2_SEND_ERROR;
    }
    while(1)
    {
        p_m2m2_r = post_office_get(M2M2_TIMEOUT_PERIOD, APP_OS_CFG_DISPLAY_TASK_INDEX);//wait 10 ms
        if((p_m2m2_r == NULL)||(p_m2m2_r->data == NULL))
        {
            NRF_LOG_INFO("post_office_get error!");
            return M2M2_WAIT_TIMEOUT;
        }
        if((dest_addr == p_m2m2_r->src)&&(M2M2_APP_COMMON_CMD_STREAM_STOP_RESP == p_m2m2_r->data[0]))
        {
            break;
        }
        else
        {
            post_office_consume_msg(p_m2m2_r);
        }
    }


    data_r = (m2m2_app_common_sub_op_t *)&p_m2m2_r->data[0];
    if(NULL != status)
    {
        *status = data_r->status;
    }
    post_office_consume_msg(p_m2m2_r);
    return M2M2_SUCCESS;
}



uint32_t m2m2_unsubscribe_stream(M2M2_ADDR_ENUM_t dest_addr,M2M2_ADDR_ENUM_t sub_addr,uint8_t *status)
{
    m2m2_hdr_t *p_m2m2_t = NULL;
    m2m2_hdr_t *p_m2m2_r = NULL;
    m2m2_app_common_sub_op_t *data_t = NULL;
    m2m2_app_common_sub_op_t *data_r = NULL;
    ADI_OSAL_STATUS       err = ADI_OSAL_SUCCESS;
    p_m2m2_t = post_office_create_msg(M2M2_HEADER_SZ+sizeof(m2m2_app_common_sub_op_t));
    if (p_m2m2_t == NULL)
    {
        return M2M2_NO_MEMORY;
    }

    p_m2m2_t->src = M2M2_ADDR_DISPLAY;
    p_m2m2_t->dest = dest_addr;
    p_m2m2_t->length = M2M2_HEADER_SZ+sizeof(m2m2_app_common_sub_op_t);

    data_t = (m2m2_app_common_sub_op_t *)p_m2m2_t->data;
    data_t->command = M2M2_APP_COMMON_CMD_STREAM_UNSUBSCRIBE_REQ;
    data_t->stream = sub_addr;
    post_office_send(p_m2m2_t,&err);
    if(ADI_OSAL_SUCCESS != err)
    {
        return M2M2_SEND_ERROR;
    }
    while(1)
    {
        p_m2m2_r = post_office_get(M2M2_TIMEOUT_PERIOD, APP_OS_CFG_DISPLAY_TASK_INDEX);//wait 10 ms
        if((p_m2m2_r == NULL)||(p_m2m2_r->data == NULL))
        {
            NRF_LOG_INFO("post_office_get error!");
            return M2M2_WAIT_TIMEOUT;
        }
        if((dest_addr == p_m2m2_r->src)&&(M2M2_APP_COMMON_CMD_STREAM_UNSUBSCRIBE_RESP == p_m2m2_r->data[0]))
        {
            break;
        }
        else
        {
            post_office_consume_msg(p_m2m2_r);
        }
    }


    data_r = (m2m2_app_common_sub_op_t *)&p_m2m2_r->data[0];
    if(NULL != status)
    {
        *status = data_r->status;
    }
    post_office_consume_msg(p_m2m2_r);
    return M2M2_SUCCESS;
}

extern M2M2_FILE_SYS_STATUS_ENUM_t get_fs_available_memory(uint32_t *p_available_mem);
uint32_t m2m2_memory_usage_get(uint8_t *percent)
{
  uint32_t n_available_memory;
    if(get_fs_available_memory(&n_available_memory) != M2M2_FILE_SYS_STATUS_OK)
    {
        return M2M2_STATUS_ERROR;
    }
    if(NULL != percent)
    {
        *percent = 100 - n_available_memory;
    }
    return M2M2_SUCCESS;

}

uint32_t m2m2_check_logging_status(uint8_t *status)
{

    m2m2_hdr_t *p_m2m2_t = NULL;
    m2m2_hdr_t *p_m2m2_r = NULL;

    m2m2_pm_sys_cmd_t *data_t = NULL;
    m2m2_pm_sys_cmd_t *data_r = NULL;
    ADI_OSAL_STATUS       err = ADI_OSAL_SUCCESS;;
    p_m2m2_t = post_office_create_msg(M2M2_HEADER_SZ+sizeof(m2m2_pm_sys_cmd_t));

    if (p_m2m2_t == NULL)
    {
        return M2M2_NO_MEMORY;
    }
    /* swap from network byte order to little endian */
    p_m2m2_t->src = M2M2_ADDR_DISPLAY;
    p_m2m2_t->dest = M2M2_ADDR_SYS_PM;
    p_m2m2_t->length = M2M2_HEADER_SZ+sizeof(m2m2_pm_sys_cmd_t);

    data_t = (m2m2_pm_sys_cmd_t *)p_m2m2_t->data;
    data_t->command = M2M2_PS_SYS_COMMAND_GET_LOW_TOUCH_LOGGING_STATUS_REQ;
    post_office_send(p_m2m2_t,&err);

    if(ADI_OSAL_SUCCESS != err)
    {
        return M2M2_SEND_ERROR;
    }

    while(1)
    {
        p_m2m2_r = post_office_get(M2M2_TIMEOUT_SHORT_WAIT, APP_OS_CFG_DISPLAY_TASK_INDEX);//wait 10 ms

        if((p_m2m2_r == NULL)||(p_m2m2_r->data == NULL))
        {
            NRF_LOG_INFO("post_office_get error!");
            return M2M2_WAIT_TIMEOUT;
        }

        if((M2M2_ADDR_SYS_PM == p_m2m2_r->src)&&(M2M2_PS_SYS_COMMAND_GET_LOW_TOUCH_LOGGING_STATUS_RESP == p_m2m2_r->data[0]))
        {
            break;
        }
        else
        {
            post_office_consume_msg(p_m2m2_r);
        }
    }
    data_r = (m2m2_pm_sys_cmd_t *)&p_m2m2_r->data[0];

    if(NULL != status)
    {
        *status = data_r->status;
    }
    post_office_consume_msg(p_m2m2_r);
    return M2M2_SUCCESS;

}
#endif