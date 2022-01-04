/**
***************************************************************************
* @addtogroup Tasks
* @{
* @file         ble_services_m2m2_protocol.c
* @author       ADI
* @version      V1.0.0
* @date         15-Feb-2021
***************************************************************************
* @attention
***************************************************************************
*/
/*!
*  \copyright Analog Devices
* ****************************************************************************
*
* License Agreement
*
* Copyright (c) 2021 Analog Devices Inc.
* All rights reserved.
*
* This source code is intended for the recipient only under the guidelines of
* the non-disclosure agreement with Analog Devices Inc.
*

* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
* EXPRESS OR  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
* MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
* IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
* CLAIM, DAMAGES OR OTHER  LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
* TORT OR OTHERWISE, ARISING  FROM, OUT OF OR IN CONNECTION WITH THE
* SOFTWARE OR THE USE OR OTHER  DEALINGS IN THE SOFTWARE.
* ****************************************************************************
*/
#include "ble_services_m2m2_protocol.h"

#include "sensor_adpd_application_interface.h"
#include "common_sensor_interface.h"
#include "ppg_application_interface.h"
#include "ecg_application_interface.h"
#include "sync_data_application_interface.h"
#include "ble_cus1.h"
#include <post_office.h>

#include "nrf_log.h"
#include "nrf_log_ctrl.h"
#include "nrf_log_default_backends.h"

extern uint8_t dvt2;

#ifdef BLE_CUSTOM_PROFILE1
/**
  * @brief  Function for sending M2M2_APP_COMMON_CMD_WRITE_LCFG_REQ m2m2 cmd REQ
            and get RESP pkt for ECG APP freq from M2M2_ADDR_MED_ECG
  * @param  status A pointer, value of which tells if the command was
                   M2M2_APP_COMMON_STATUS_OK or not

  * @retval Return value from the func which could be either of the enum:
            M2M2_SUCCESS = 0
            M2M2_WAIT_TIMEOUT = 1
            M2M2_STATUS_ERROR = 2
            M2M2_STATUS_DISABLE = 3
            M2M2_NO_MEMORY = 4
            M2M2_SEND_ERROR = 5
            M2M2_VALUE_OVERFLOW = 6
  */
uint32_t ble_services_sensor_m2m2_ecg_write_lcfg(uint8_t *status)
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
    p_m2m2_t->src = M2M2_ADDR_BLE_SERVICES_SENSOR;
    p_m2m2_t->dest = M2M2_ADDR_MED_ECG;
    p_m2m2_t->length = M2M2_HEADER_SZ+sizeof(ecg_app_lcfg_op_hdr_t);

    data_t = (ecg_app_lcfg_op_hdr_t *)p_m2m2_t->data;
    data_t->command = M2M2_APP_COMMON_CMD_WRITE_LCFG_REQ;
    data_t->num_ops = 1;
    data_t->ops[0].field = 0;
    data_t->ops[0].value = ECG_APP_FREQUENCY; //Start ECG at 500Hz
    post_office_send(p_m2m2_t,&err);
    if(ADI_OSAL_SUCCESS != err)
    {
        return M2M2_SEND_ERROR;
    }

    while(1)
    {
        p_m2m2_r = post_office_get(M2M2_TIMEOUT_PERIOD, APP_OS_CFG_BLE_SERVICES_SENSOR_TASK_INDEX);//wait 10 ms
        if((p_m2m2_r == NULL)||(p_m2m2_r->data[0] == NULL))
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
#endif

/**
  * @brief  Function for sending M2M2_SENSOR_ADPD_COMMAND_LOAD_CFG_REQ m2m2 cmd REQ
            and get RESP pkt for device id M2M2_SENSOR_ADPD4000_DEVICE_4000_G
            from M2M2_ADDR_SENSOR_ADPD4000
  * @param  status A pointer, value of which tells if the command was
                   M2M2_APP_COMMON_STATUS_OK or not

  * @retval Return value from the func which could be either of the enum:
            M2M2_SUCCESS = 0
            M2M2_WAIT_TIMEOUT = 1
            M2M2_STATUS_ERROR = 2
            M2M2_STATUS_DISABLE = 3
            M2M2_NO_MEMORY = 4
            M2M2_SEND_ERROR = 5
            M2M2_VALUE_OVERFLOW = 6
  */
uint32_t ble_services_sensor_m2m2_ppg_load_adpd_cfg(uint8_t *status)
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
    p_m2m2_t->src = M2M2_ADDR_BLE_SERVICES_SENSOR;
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
        p_m2m2_r = post_office_get(M2M2_TIMEOUT_PERIOD, APP_OS_CFG_BLE_SERVICES_SENSOR_TASK_INDEX);//wait 10 ms
        if((p_m2m2_r == NULL)||(p_m2m2_r->data[0] == NULL))
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

/**
  * @brief  Function for sending M2M2_SENSOR_ADPD_COMMAND_CLOCK_CAL_REQ m2m2 cmd REQ
            and get RESP pkt from M2M2_ADDR_SENSOR_ADPD4000
  * @param  status A pointer, value of which tells if the command was
                   M2M2_APP_COMMON_STATUS_OK or not

  * @retval Return value from the func which could be either of the enum:
            M2M2_SUCCESS = 0
            M2M2_WAIT_TIMEOUT = 1
            M2M2_STATUS_ERROR = 2
            M2M2_STATUS_DISABLE = 3
            M2M2_NO_MEMORY = 4
            M2M2_SEND_ERROR = 5
            M2M2_VALUE_OVERFLOW = 6
  */
uint32_t ble_services_sensor_m2m2_ppg_clock_calibration(uint8_t *status)
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

    p_m2m2_t->src = M2M2_ADDR_BLE_SERVICES_SENSOR;
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
        p_m2m2_r = post_office_get(M2M2_TIMEOUT_PERIOD, APP_OS_CFG_BLE_SERVICES_SENSOR_TASK_INDEX);//wait 10 ms
        if((p_m2m2_r == NULL)||(p_m2m2_r->data[0] == NULL))
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

/**
  * @brief  Function for sending M2M2_APP_COMMON_CMD_SET_LCFG_REQ m2m2 cmd REQ
            and get RESP pkt for lcfg id M2M2_SENSOR_PPG_LCFG_ID_ADPD4000
            from M2M2_ADDR_MED_PPG
  * @param  status A pointer, value of which tells if the command was
                   M2M2_APP_COMMON_STATUS_OK or not

  * @retval Return value from the func which could be either of the enum:
            M2M2_SUCCESS = 0
            M2M2_WAIT_TIMEOUT = 1
            M2M2_STATUS_ERROR = 2
            M2M2_STATUS_DISABLE = 3
            M2M2_NO_MEMORY = 4
            M2M2_SEND_ERROR = 5
            M2M2_VALUE_OVERFLOW = 6
  */
uint32_t ble_services_sensor_m2m2_set_ppg_configuration(uint8_t *status)
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

    p_m2m2_t->src = M2M2_ADDR_BLE_SERVICES_SENSOR;
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
        p_m2m2_r = post_office_get(M2M2_TIMEOUT_PERIOD, APP_OS_CFG_BLE_SERVICES_SENSOR_TASK_INDEX);//wait 10 ms
        if((p_m2m2_r == NULL)||(p_m2m2_r->data[0] == NULL))
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

/**
  * @brief  Function for sending M2M2_APP_COMMON_CMD_SENSOR_STATUS_QUERY_REQ m2m2 cmd REQ
            and get RESP pkt for lcfg id M2M2_SENSOR_PPG_LCFG_ID_ADPD4000
            from M2M2_ADDR_MED_PPG
  * @param  dest_addr m2m2 address from which M2M2_APP_COMMON_CMD_SENSOR_STATUS_QUERY_REQ
            needs to be obtained
  * @param  status A pointer, value of which tells if the command was
                   M2M2_APP_COMMON_STATUS_OK or not
  * @param  subs_num A pointer, value of which tells the number of subscribers
                     for the sensor
  * @param  starts_num A pointer, value of which tells the number of starts
                   for the sensor

  * @retval Return value from the func which could be either of the enum:
            M2M2_SUCCESS = 0
            M2M2_WAIT_TIMEOUT = 1
            M2M2_STATUS_ERROR = 2
            M2M2_STATUS_DISABLE = 3
            M2M2_NO_MEMORY = 4
            M2M2_SEND_ERROR = 5
            M2M2_VALUE_OVERFLOW = 6
  */
uint32_t ble_services_sensor_m2m2_check_sensor_status(M2M2_ADDR_ENUM_t dest_addr,uint8_t *status,uint8_t *subs_num,uint8_t *starts_num)
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

    p_m2m2_t->src = M2M2_ADDR_BLE_SERVICES_SENSOR;
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
        p_m2m2_r = post_office_get(M2M2_TIMEOUT_PERIOD, APP_OS_CFG_BLE_SERVICES_SENSOR_TASK_INDEX);//wait 10 ms
        if((p_m2m2_r == NULL)||(p_m2m2_r->data[0] == NULL))
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

/**
  * @brief  Function to start a sensor or stop a sensor
  * @param  sensor specify the sensor application to be started/stopped
                   SENSOR_PPG -> PPG sensor
                   SENSOR_ECG -> ECG sensor
  * @param  en     1 -> to enable(sensor start)
                   0 -> to disable(sensor stop)
  *
  * @retval Return value from the func which could be either of the enum:
            M2M2_SUCCESS = 0
            M2M2_WAIT_TIMEOUT = 1
            M2M2_STATUS_ERROR = 2
            M2M2_STATUS_DISABLE = 3
            M2M2_NO_MEMORY = 4
            M2M2_SEND_ERROR = 5
            M2M2_VALUE_OVERFLOW = 6
  */
uint32_t ble_services_sensor_m2m2_set_stream_status(uint8_t sensor,uint8_t en)
{
    uint8_t status;
    uint8_t subs_num;
    uint32_t err;
    switch(sensor)
    {
        case SENSOR_PPG:
        {
            err = ble_services_sensor_m2m2_check_sensor_status(M2M2_ADDR_MED_PPG,&status,&subs_num,NULL);
            APP_ERROR_CHECK(err);
            NRF_LOG_INFO("check stream,status = %d,subs_num = %d",status,subs_num);
            if(0 != en)
            {
                if((M2M2_APP_COMMON_STATUS_STREAM_IN_PROGRESS != status)
                &&(M2M2_APP_COMMON_STATUS_STREAM_STARTED != status))
                {
                    err = ble_services_sensor_m2m2_ppg_load_adpd_cfg(&status);
                    APP_ERROR_CHECK(err);
                    if(M2M2_APP_COMMON_STATUS_OK != status)
                    {
                        NRF_LOG_INFO("load adpd configuration fail,status = %d",status);
                        return M2M2_STATUS_ERROR;
                    }
                    err = ble_services_sensor_m2m2_ppg_clock_calibration(&status);
                    APP_ERROR_CHECK(err);
                    if(M2M2_APP_COMMON_STATUS_OK != status)
                    {
                        NRF_LOG_INFO("clock calibration fail,status = %d",status);
                        return M2M2_STATUS_ERROR;
                    }
                    err = ble_services_sensor_m2m2_set_ppg_configuration(&status);
                    APP_ERROR_CHECK(err);
                    if(M2M2_APP_COMMON_STATUS_OK != status)
                    {
                        NRF_LOG_INFO("set ppg configuration fail,status = %d",status);
                        return M2M2_STATUS_ERROR;
                    }

                }
                err = ble_services_sensor_m2m2_start_stream(M2M2_ADDR_MED_PPG,&status);//
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
                    err = ble_services_sensor_m2m2_stop_stream(M2M2_ADDR_MED_PPG,&status);
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
#ifdef BLE_CUSTOM_PROFILE1
        case SENSOR_ECG:
        {
            err = ble_services_sensor_m2m2_check_sensor_status(M2M2_ADDR_MED_ECG,&status,&subs_num,NULL);
            NRF_LOG_INFO("ECG check stream,status = %d,subs_num = %d",status,subs_num);
            APP_ERROR_CHECK(err);

            if(0 != en)
            {
                if((M2M2_APP_COMMON_STATUS_STREAM_IN_PROGRESS != status)
                &&(M2M2_APP_COMMON_STATUS_STREAM_STARTED != status))
                {
                    err = ble_services_sensor_m2m2_ecg_write_lcfg(&status);
                    APP_ERROR_CHECK(err);
                    if(M2M2_APP_COMMON_STATUS_OK != status)
                    {
                        NRF_LOG_INFO("check_sensor_status fail,status = %d",status);
                        return M2M2_STATUS_ERROR;
                    }
                }
                err = ble_services_sensor_m2m2_start_stream(M2M2_ADDR_MED_ECG,&status);//
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
                err = ble_services_sensor_m2m2_stop_stream(M2M2_ADDR_MED_ECG,&status);
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
#endif
        default:
        {
            return M2M2_VALUE_OVERFLOW;
        }
    }
    return M2M2_SUCCESS;
}

/**
  * @brief  Function to subscribe from sensor streams
  * @param  dest_addr m2m2 address from which M2M2_APP_COMMON_CMD_STREAM_SUBSCRIBE_REQ
                      needs to be given
  * @param  sub_addr  Stream Address to subscribe
  * @param  status    A pointer, value of which tells if the command was
                      M2M2_APP_COMMON_STATUS_OK or not
  *
  * @retval Return value from the func which could be either of the enum:
            M2M2_SUCCESS = 0
            M2M2_WAIT_TIMEOUT = 1
            M2M2_STATUS_ERROR = 2
            M2M2_STATUS_DISABLE = 3
            M2M2_NO_MEMORY = 4
            M2M2_SEND_ERROR = 5
            M2M2_VALUE_OVERFLOW = 6
  */
uint32_t ble_services_sensor_m2m2_subscribe_stream(M2M2_ADDR_ENUM_t dest_addr,M2M2_ADDR_ENUM_t sub_addr,uint8_t *status)
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

    p_m2m2_t->src = M2M2_ADDR_BLE_SERVICES_SENSOR;
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
        p_m2m2_r = post_office_get(M2M2_TIMEOUT_PERIOD, APP_OS_CFG_BLE_SERVICES_SENSOR_TASK_INDEX);//wait 10 ms
        if((p_m2m2_r == NULL)||(p_m2m2_r->data[0] == NULL))
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

/**
  * @brief  Function for sending M2M2_APP_COMMON_CMD_STREAM_START_REQ m2m2 cmd REQ
            and get RESP pkt from sensor depending on "dest_addr" argument

  * @param  dest_addr m2m2 address to which M2M2_APP_COMMON_CMD_STREAM_START_REQ
                      needs to be given
  * @param  status    A pointer, value of which tells if the command was
                      M2M2_APP_COMMON_STATUS_OK or not

  * @retval Return value from the func which could be either of the enum:
            M2M2_SUCCESS = 0
            M2M2_WAIT_TIMEOUT = 1
            M2M2_STATUS_ERROR = 2
            M2M2_STATUS_DISABLE = 3
            M2M2_NO_MEMORY = 4
            M2M2_SEND_ERROR = 5
            M2M2_VALUE_OVERFLOW = 6
  */
uint32_t ble_services_sensor_m2m2_start_stream(M2M2_ADDR_ENUM_t dest_addr,uint8_t *status)
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

    p_m2m2_t->src = M2M2_ADDR_BLE_SERVICES_SENSOR;
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
        p_m2m2_r = post_office_get(M2M2_TIMEOUT_PERIOD, APP_OS_CFG_BLE_SERVICES_SENSOR_TASK_INDEX);//wait 10 ms
        if((p_m2m2_r == NULL)||(p_m2m2_r->data[0] == NULL))
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

/**
  * @brief  Function for sending M2M2_APP_COMMON_CMD_STREAM_STOP_REQ m2m2 cmd REQ
            and get RESP pkt from sensor depending on "dest_addr" argument

  * @param  dest_addr m2m2 address to which M2M2_APP_COMMON_CMD_STREAM_STOP_REQ
                      needs to be given
  * @param  status    A pointer, value of which tells if the command was
                      M2M2_APP_COMMON_STATUS_OK or not

  * @retval Return value from the func which could be either of the enum:
            M2M2_SUCCESS = 0
            M2M2_WAIT_TIMEOUT = 1
            M2M2_STATUS_ERROR = 2
            M2M2_STATUS_DISABLE = 3
            M2M2_NO_MEMORY = 4
            M2M2_SEND_ERROR = 5
            M2M2_VALUE_OVERFLOW = 6
  */
uint32_t ble_services_sensor_m2m2_stop_stream(M2M2_ADDR_ENUM_t dest_addr,uint8_t *status)
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

    p_m2m2_t->src = M2M2_ADDR_BLE_SERVICES_SENSOR;
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
        p_m2m2_r = post_office_get(M2M2_TIMEOUT_PERIOD, APP_OS_CFG_BLE_SERVICES_SENSOR_TASK_INDEX);//wait 10 ms
        if((p_m2m2_r == NULL)||(p_m2m2_r->data[0] == NULL))
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

/**
  * @brief  Function to unsubscribe from sensor streams
  * @param  dest_addr m2m2 address from which M2M2_APP_COMMON_CMD_STREAM_UNSUBSCRIBE_REQ
                      needs to be given
  * @param  sub_addr  Stream Address to unsubscribe
  * @param  status    A pointer, value of which tells if the command was
                      M2M2_APP_COMMON_STATUS_OK or not
  *
  * @retval Return value from the func which could be either of the enum:
            M2M2_SUCCESS = 0
            M2M2_WAIT_TIMEOUT = 1
            M2M2_STATUS_ERROR = 2
            M2M2_STATUS_DISABLE = 3
            M2M2_NO_MEMORY = 4
            M2M2_SEND_ERROR = 5
            M2M2_VALUE_OVERFLOW = 6
  */
uint32_t ble_services_sensor_m2m2_unsubscribe_stream(M2M2_ADDR_ENUM_t dest_addr,M2M2_ADDR_ENUM_t sub_addr,uint8_t *status)
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

    p_m2m2_t->src = M2M2_ADDR_BLE_SERVICES_SENSOR;
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
        p_m2m2_r = post_office_get(M2M2_TIMEOUT_PERIOD, APP_OS_CFG_BLE_SERVICES_SENSOR_TASK_INDEX);//wait 10 ms
        if((p_m2m2_r == NULL)||(p_m2m2_r->data[0] == NULL))
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