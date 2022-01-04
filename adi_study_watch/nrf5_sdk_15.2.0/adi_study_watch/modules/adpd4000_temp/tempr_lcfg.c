/**
****************************************************************************
* @file     tempr_lcfg.c
* @author   ADI
* @version  V0.1
* @date     08-Sept-2021
* @brief    Library Configuratin File for Temperature Application 
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
#ifdef ENABLE_TEMPERATURE_APP
#include <tempr_lcfg.h>
#ifdef DCB
static volatile bool g_tempr_dcb_Present = false;
//static bool check_dcb_erase = false;
#endif

/* Default Temperature LCFG structure */
const temperature_lcfg_t tempr_default_lcfg =
      {  
        .sample_period = 1,         /* Sampling period set to 1 sec by default*/
        .slots_selected = 0x018,    /* Slot D and E are enabled by default*/
        .T_I_curve_LUT =            /* Look Up Tables[0:4] have values of thermistor NTC104EF104FTDSX by default*/
                        { 
                           {
                            355600, 271800, 209400, 162500, 127000,
                            100000, 79230,  63180,  50680,  40900,
                            33190,  27090,  22220,  18320,  15180,
                            12640,  10580,  8887,   7500,   6357,
                            5410
                           },
                           {
                            355600, 271800, 209400, 162500, 127000,
                            100000, 79230,  63180,  50680,  40900,
                            33190,  27090,  22220,  18320,  15180,
                            12640,  10580,  8887,   7500,   6357,
                            5410
                           },
                           {
                            355600, 271800, 209400, 162500, 127000,
                            100000, 79230,  63180,  50680,  40900,
                            33190,  27090,  22220,  18320,  15180,
                            12640,  10580,  8887,   7500,   6357,
                            5410
                           },
                           {
                            355600, 271800, 209400, 162500, 127000,
                            100000, 79230,  63180,  50680,  40900,
                            33190,  27090,  22220,  18320,  15180,
                            12640,  10580,  8887,   7500,   6357,
                            5410
                           },
                           {
                            355600, 271800, 209400, 162500, 127000,
                            100000, 79230,  63180,  50680,  40900,
                            33190,  27090,  22220,  18320,  15180,
                            12640,  10580,  8887,   7500,   6357,
                            5410
                           },
                        },
      };

uint16_t g_tempr_lcfg_dcb_size; /*! size of lcfg read from dcb*/
extern temperature_lcfg_t active_temperature_lcfg;
//extern uint8_t gsConfigTmpFile[];
extern uint32_t gsConfigTmpFile[MAXGENBLKDCBSIZE * MAX_GEN_BLK_DCB_PKTS];

/*!
 ****************************************************************************
 * @brief    load the lcfg from the temperature dcb
 * @param    None
 * @retval   None
 *****************************************************************************/
void load_tempr_lcfg_from_dcb(void)
{
    if (tempr_get_dcb_present_flag() == true) {
      uint32_t *dcbdata = (uint32_t *)&gsConfigTmpFile[0];
      memset(gsConfigTmpFile, 0, sizeof(gsConfigTmpFile));
      g_tempr_lcfg_dcb_size = sizeof(active_temperature_lcfg)/sizeof(uint32_t);
      if (read_tempr_dcb(dcbdata, &g_tempr_lcfg_dcb_size) == TEMPERATURE_SUCCESS) {
        /*! clear the lcfg structure*/
        memset(&active_temperature_lcfg,0,sizeof(active_temperature_lcfg));
        /*! load the lcfg values*/
        memcpy(&active_temperature_lcfg,dcbdata,sizeof(active_temperature_lcfg));
      }
    }
}


/*!
 ****************************************************************************
 * @brief         Function to write temperature LCFG parameter
 * @param[in]     n_field: LCFG field that has to be written
 * @param[in]     p_value: pointer to the data to be written
 * @retval        TEMPERATURE_ERROR_CODE_t
 *****************************************************************************/
TEMPERATURE_ERROR_CODE_t temperature_write_lcfg(uint8_t n_field, uint32_t *p_value) {

  if (n_field < TEMPERATUER_LCFG_MAX) {
    switch (n_field) {
    case TEMPERATUER_LCFG_SAMPLE_PERIOD:
      active_temperature_lcfg.sample_period = p_value[0];
      break;
    case TEMPERATUER_LCFG_SLOTS_SELECTED:
      active_temperature_lcfg.slots_selected = p_value[0];
      break;
    case TEMPERATUER_LCFG_LUT_0:
      memcpy(active_temperature_lcfg.T_I_curve_LUT[0],p_value,sizeof(uint32_t)*NUM_ENTRIES_T_I_LUT);
      break;
    case TEMPERATUER_LCFG_LUT_1:
      memcpy(active_temperature_lcfg.T_I_curve_LUT[1],p_value,sizeof(uint32_t)*NUM_ENTRIES_T_I_LUT);
      break;
    case TEMPERATUER_LCFG_LUT_2:
      memcpy(active_temperature_lcfg.T_I_curve_LUT[2],p_value,sizeof(uint32_t)*NUM_ENTRIES_T_I_LUT);
      break;
    case TEMPERATUER_LCFG_LUT_3:
      memcpy(active_temperature_lcfg.T_I_curve_LUT[3],p_value,sizeof(uint32_t)*NUM_ENTRIES_T_I_LUT);
      break;
    case TEMPERATUER_LCFG_LUT_4:
      memcpy(active_temperature_lcfg.T_I_curve_LUT[4],p_value,sizeof(uint32_t)*NUM_ENTRIES_T_I_LUT);
      break;
    }
    return TEMPERATURE_SUCCESS;
  }
  return TEMPERATURE_ERROR;
}

/*!
 ****************************************************************************
 * @brief         Function to read temperature LCFG parameter
 * @param[in]     n_field: LCFG field value to be read
 * @param[in]     p_value: pointer to the buffer to which LCFG values from
 *                         specified field needs to be updated
 * @retval        TEMPERATURE_ERROR_CODE_t
 *****************************************************************************/
TEMPERATURE_ERROR_CODE_t temperature_read_lcfg(uint8_t n_field, uint32_t *p_value) {

  if (n_field < TEMPERATUER_LCFG_MAX) {
    switch (n_field) {
    case TEMPERATUER_LCFG_SAMPLE_PERIOD:
      p_value[0] = active_temperature_lcfg.sample_period;
      break;
    case TEMPERATUER_LCFG_SLOTS_SELECTED:
      p_value[0] = active_temperature_lcfg.slots_selected;
      break;
    case TEMPERATUER_LCFG_LUT_0:
      memcpy(p_value,active_temperature_lcfg.T_I_curve_LUT[0],sizeof(uint32_t)*NUM_ENTRIES_T_I_LUT);
      break;
    case TEMPERATUER_LCFG_LUT_1:
      memcpy(p_value,active_temperature_lcfg.T_I_curve_LUT[1],sizeof(uint32_t)*NUM_ENTRIES_T_I_LUT);
      break;
    case TEMPERATUER_LCFG_LUT_2:
      memcpy(p_value,active_temperature_lcfg.T_I_curve_LUT[2],sizeof(uint32_t)*NUM_ENTRIES_T_I_LUT);
      break;
    case TEMPERATUER_LCFG_LUT_3:
      memcpy(p_value,active_temperature_lcfg.T_I_curve_LUT[3],sizeof(uint32_t)*NUM_ENTRIES_T_I_LUT);
      break;
    case TEMPERATUER_LCFG_LUT_4:
      memcpy(p_value,active_temperature_lcfg.T_I_curve_LUT[4],sizeof(uint32_t)*NUM_ENTRIES_T_I_LUT);
      break;
    }
    return TEMPERATURE_SUCCESS;
  }
  return TEMPERATURE_ERROR;
}


/*!
 ****************************************************************************
 *@brief      Temperature stream Read/write library configuration options
 *@param      pPkt: pointer to the packet structure
 *@return     m2m2_hdr_t
 ******************************************************************************/
m2m2_hdr_t *temperature_app_lcfg_access(m2m2_hdr_t *p_pkt) {
  M2M2_APP_COMMON_STATUS_ENUM_t status = M2M2_APP_COMMON_STATUS_ERROR;
  PYLD_CST(p_pkt, temperature_app_lcfg_t, p_in_payload);
  PKT_MALLOC(p_resp_pkt, temperature_app_lcfg_t, 0);
  if (NULL != p_resp_pkt) {
    PYLD_CST(p_resp_pkt, temperature_app_lcfg_t, p_resp_payload);
    uint16_t reg_data = 0;

    switch (p_in_payload->command) {
    case M2M2_APP_COMMON_CMD_READ_LCFG_REQ:
        if (temperature_read_lcfg(p_in_payload->field, &p_resp_payload->value[0]) == TEMPERATURE_SUCCESS) {
          status = M2M2_APP_COMMON_STATUS_OK;
        } else {
          status = M2M2_APP_COMMON_STATUS_ERROR;
        }
        p_resp_payload->field =  p_in_payload->field;
        p_resp_payload->command =
            (M2M2_APP_COMMON_CMD_ENUM_t)M2M2_APP_COMMON_CMD_READ_LCFG_RESP;
      break;
    case M2M2_APP_COMMON_CMD_WRITE_LCFG_REQ:
        if (temperature_write_lcfg(p_in_payload->field,
              &p_in_payload->value[0]) == TEMPERATURE_SUCCESS) {
          status = M2M2_APP_COMMON_STATUS_OK;
        } else {
          status = M2M2_APP_COMMON_STATUS_ERROR;
        }
        for(uint8_t i = 0; i < NUM_ENTRIES_T_I_LUT; i++) {
          p_resp_payload->value[i] = p_in_payload->value[i];
        }
        p_resp_payload->field =  p_in_payload->field;
        p_resp_payload->command =
            (M2M2_APP_COMMON_CMD_ENUM_t)M2M2_APP_COMMON_CMD_WRITE_LCFG_RESP;
      break;
    default:
      /* Something has gone horribly wrong. */
      post_office_consume_msg(p_resp_pkt);
      return NULL;
    }
    p_resp_pkt->dest = p_pkt->src;
    p_resp_pkt->src = p_pkt->dest;
    p_resp_payload->status = status;
  }
  return p_resp_pkt;
}

#ifdef DCB
/*!
 ****************************************************************************
 * @brief    Gets the entire TEMPERATURE DCB configuration written in flash
 * @param    temp_dcb_data: pointer to dcb struct variable,
 *            in size of data in Double Word (32-bits)
 * @param    read_size: The size of the record to be returned to the user
 * @retval   TEMPERATURE_ERROR_CODE_t: Status
 *****************************************************************************/
TEMPERATURE_ERROR_CODE_t read_tempr_dcb(uint32_t *temp_dcb_data, uint16_t *read_size) {
  TEMPERATURE_ERROR_CODE_t dcb_status = TEMPERATURE_ERROR;

  if (adi_dcb_read_from_fds(ADI_DCB_TEMPERATURE_BLOCK_IDX, temp_dcb_data, read_size) ==
      DEF_OK) {
    dcb_status = TEMPERATURE_SUCCESS;
  }
  return dcb_status;
}

/*!
 ****************************************************************************
 * @brief    Sets the entire TEMPERATURE DCB configuration in flash
 * @param    temp_dcb_data: pointer to dcb struct variable,
 *            in size of data in Double Word (32-bits)
 * @param    write_Size: The size of the record to be written
 * @retval   TEMPERATURE_ERROR_CODE_t: Status
 *****************************************************************************/
TEMPERATURE_ERROR_CODE_t write_tempr_dcb(uint32_t *temp_dcb_data, uint16_t write_Size) {
  TEMPERATURE_ERROR_CODE_t dcb_status = TEMPERATURE_ERROR;

  if (adi_dcb_write_to_fds(ADI_DCB_TEMPERATURE_BLOCK_IDX, temp_dcb_data, write_Size) ==
      DEF_OK) {
    dcb_status = TEMPERATURE_SUCCESS;
  }
  return dcb_status;
}

/*!
 ****************************************************************************
 * @brief    Delete the entire TEMPERATURE DCB configuration in flash
 * @param    None
 * @retval   TEMPERATURE_ERROR_CODE_t: Status
 *****************************************************************************/
TEMPERATURE_ERROR_CODE_t delete_tempr_dcb(void) {
  TEMPERATURE_ERROR_CODE_t dcb_status = TEMPERATURE_ERROR;

  if (adi_dcb_delete_fds_settings(ADI_DCB_TEMPERATURE_BLOCK_IDX) == DEF_OK) {
    dcb_status = TEMPERATURE_SUCCESS;
  }
  return dcb_status;
}

/*!
 ****************************************************************************
 * @brief    Set the dcb present flag
 * @param    set_flag: flag to set presence of dcb in flash
 * @retval   None
 *****************************************************************************/
void tempr_set_dcb_present_flag(bool set_flag) {
  g_tempr_dcb_Present = set_flag;
  NRF_LOG_INFO("TEMPERATURE DCB present set: %s",
      (g_tempr_dcb_Present == true ? "TRUE" : "FALSE"));
}

/*!
 ****************************************************************************
 * @brief    Get whether the dcb is present in flash
 * @param    None
 * @retval   bool: TRUE: dcb present, FALSE: dcb absent
 *****************************************************************************/
bool tempr_get_dcb_present_flag(void) {
  NRF_LOG_INFO(
      "TEMPERATURE DCB present: %s", (g_tempr_dcb_Present == true ? "TRUE" : "FALSE"));
  return g_tempr_dcb_Present;
}

/*!
 ****************************************************************************
 * @brief    Update the global dcb presence flag in flash
 * @param    None
 * @retval   None
 *****************************************************************************/
void tempr_update_dcb_present_flag(void) {
  g_tempr_dcb_Present = adi_dcb_check_fds_entry(ADI_DCB_TEMPERATURE_BLOCK_IDX);
  NRF_LOG_INFO("Updated. TEMPERATURE DCB present: %s",
      (g_tempr_dcb_Present == true ? "TRUE" : "FALSE"));
}

/*!
 ****************************************************************************
 * @brief    Update the temperature lcfg from the dcb
 * @param    p_pkt: pointer to packet
 * @retval   m2m2_hdr_t
 *****************************************************************************/
m2m2_hdr_t *tempr_app_set_dcb_lcfg(m2m2_hdr_t *p_pkt) {
  M2M2_APP_COMMON_STATUS_ENUM_t status = M2M2_APP_COMMON_STATUS_ERROR;
  PKT_MALLOC(p_resp_pkt, temperature_app_dcb_lcfg_t, 0);
  if (NULL != p_resp_pkt) {
    /* Declare a pointer to the response packet payload */
    PYLD_CST(p_resp_pkt, temperature_app_dcb_lcfg_t, p_resp_payload);
    if (tempr_get_dcb_present_flag() == true) {
      uint32_t *dcbdata = (uint32_t *)&gsConfigTmpFile[0];
      memset(gsConfigTmpFile, 0, sizeof(gsConfigTmpFile));
      g_tempr_lcfg_dcb_size = sizeof(active_temperature_lcfg)/sizeof(uint32_t);
      if (read_tempr_dcb(dcbdata, &g_tempr_lcfg_dcb_size) == TEMPERATURE_SUCCESS) {
        status = M2M2_APP_COMMON_STATUS_OK;
        /*! clear the lcfg values*/
        memset(&active_temperature_lcfg,0,sizeof(active_temperature_lcfg));
        memcpy(&active_temperature_lcfg,dcbdata,sizeof(active_temperature_lcfg));
      } else {
        status = M2M2_APP_COMMON_STATUS_ERROR;
      }
    } else {
      status = M2M2_APP_COMMON_STATUS_ERROR;
    }

    p_resp_payload->status = status;
    p_resp_payload->command = M2M2_APP_COMMON_CMD_SET_LCFG_RESP;
    p_resp_pkt->src = p_pkt->dest;
    p_resp_pkt->dest = p_pkt->src;
  }
  return p_resp_pkt;
}

/*!
 ****************************************************************************
 * @brief Read Temperature DCB Configuration
 *
 * @param[in]           p_pkt: input m2m2 packet
 *
 * @return              pointer to reponse m2m2 packet
 *****************************************************************************/
m2m2_hdr_t *tempr_dcb_command_read_config(m2m2_hdr_t *p_pkt)
{
    uint16_t r_size;
    uint16_t i = 0, num_pkts = 0, dcbdata_index = 0;

    /* dcbdata - storage for DCB content during READ DCB block
     * for Gen block DCB; Reusing the RAM buffer from system task,
     * instead of declaring it in the function,
     * saves space on Stack requirement from LT Task
     */
    uint32_t *dcbdata = (uint32_t *)&gsConfigTmpFile[0];
    ADI_OSAL_STATUS  err;
    M2M2_DCB_STATUS_ENUM_t status = M2M2_DCB_STATUS_ERR_NOT_CHKD;

    PKT_MALLOC(p_resp_pkt, m2m2_dcb_temperature_data_t, 0);
    if(p_resp_pkt != NULL )
    {
       // Declare a pointer to the response packet payload
       PYLD_CST(p_resp_pkt, m2m2_dcb_temperature_data_t, p_resp_payload);
       memset(p_resp_payload->dcbdata, 0, sizeof(p_resp_payload->dcbdata));
       memset(gsConfigTmpFile, 0, sizeof(gsConfigTmpFile));
       r_size = (uint16_t)(MAXTEMPRLCFGDCBSIZE*MAX_TEMPRLCFG_DCB_PKTS); //Max words that can be read from FDS
       if(read_tempr_dcb(&dcbdata[0], &r_size) == ADPD4000_DCB_STATUS_OK)
       {
         status = M2M2_DCB_STATUS_OK;
         num_pkts = (r_size / MAXTEMPRLCFGDCBSIZE) + ((r_size % MAXTEMPRLCFGDCBSIZE) ? 1 : 0 );
         dcbdata_index = 0;
         for(uint16_t p = 0; p < num_pkts; p++)
         {
      	   p_resp_payload->size = \
                (p != num_pkts-1) ? MAXTEMPRLCFGDCBSIZE : (r_size % MAXTEMPRLCFGDCBSIZE);
      	   p_resp_payload->num_of_pkts = num_pkts;
           for (i = 0; i < p_resp_payload->size; i++)
           {
      	      p_resp_payload->dcbdata[i] = dcbdata[dcbdata_index++];
           }
           if(p != num_pkts-1)
      	   {
             p_resp_pkt->src = p_pkt->dest;
      	     p_resp_pkt->dest = p_pkt->src;
             p_resp_payload->status = status;
             p_resp_payload->command = M2M2_DCB_COMMAND_READ_CONFIG_RESP;
      	     post_office_send(p_resp_pkt, &err);

      	     /* Delay is kept same as what is there in low touch task */
      	     MCU_HAL_Delay(60);

      	     PKT_MALLOC(p_resp_pkt, m2m2_dcb_temperature_data_t, 0);
      	     if(NULL != p_resp_pkt)
      	     {
               // Declare a pointer to the response packet payload
               PYLD_CST(p_resp_pkt, m2m2_dcb_temperature_data_t, p_resp_payload);
               memset(p_resp_payload->dcbdata, 0, sizeof(p_resp_payload->dcbdata));
      	     }//if(NULL != p_resp_pkt)
      	     else
      	     {
               return NULL;
      	     }
      	   }
         }
        }
        else
        {
            p_resp_payload->size = 0;
            p_resp_payload->num_of_pkts = 0;
            status = M2M2_DCB_STATUS_ERR_ARGS;
        }//if(read_adpd4000_dcb())
        p_resp_payload->status = status;
        p_resp_payload->command = M2M2_DCB_COMMAND_READ_CONFIG_RESP;
        p_resp_pkt->src = p_pkt->dest;
        p_resp_pkt->dest = p_pkt->src;
    }//if(NULL != p_resp_pkt)
  return p_resp_pkt;
}

/*!
 ****************************************************************************
 * @brief Write Temperature LCFG DCB Configuration
 *
 * @param[in]           p_pkt: input m2m2 packet
 *
 * @return              pointer to reponse m2m2 packet
 *****************************************************************************/
m2m2_hdr_t *tempr_dcb_command_write_config(m2m2_hdr_t *p_pkt)
{
    M2M2_DCB_STATUS_ENUM_t status = M2M2_DCB_STATUS_ERR_NOT_CHKD;
    static uint16_t i = 0;
    static uint16_t num_of_pkts = 0;
    uint16_t j;
    /* dcbdata - storage for DCB content during write DCB block
     * for Gen block DCB. Reusing the RAM buffer from system task,
     * instead of declaring a static buffer in the function,
     * saves space on total RAM usage from the watch application
     */
    uint32_t *dcbdata = (uint32_t *)&gsConfigTmpFile[0];

    // Declare a pointer to access the input packet payload
    PYLD_CST(p_pkt, m2m2_dcb_temperature_data_t, p_in_payload);
    PKT_MALLOC(p_resp_pkt, m2m2_dcb_temperature_data_t, 0);
    if(p_resp_pkt != NULL )
    {
        // Declare a pointer to the response packet payload
        PYLD_CST(p_resp_pkt, m2m2_dcb_temperature_data_t, p_resp_payload);

        //Maximum two packets can be written to ADPD4000_DCB
        if(p_in_payload->num_of_pkts >= 1 && p_in_payload->num_of_pkts <= MAX_TEMPRLCFG_DCB_PKTS)
        {
            num_of_pkts += 1;
            for( j=0; j<p_in_payload->size; j++ )
              dcbdata[i++] = p_in_payload->dcbdata[j];
            NRF_LOG_INFO("Wr DCB:pkt sz->%d, arr index=%d",p_in_payload->size,i);
            if(num_of_pkts == p_in_payload->num_of_pkts)
            {
                if(write_tempr_dcb(&dcbdata[0], i) == ADPD4000_DCB_STATUS_OK)
                {
                    tempr_set_dcb_present_flag(true);
                    status = M2M2_DCB_STATUS_OK;
                }
                else
                {
                    status = M2M2_DCB_STATUS_ERR_ARGS;
                }
                num_of_pkts = 0;
                i=0;
            }
            else
              status = M2M2_DCB_STATUS_OK;
        }
        else
        {
            status = M2M2_DCB_STATUS_ERR_ARGS;
        }

        p_resp_payload->status = status;
        p_resp_payload->command = M2M2_DCB_COMMAND_WRITE_CONFIG_RESP;
        p_resp_payload->size = 0;
        p_resp_payload->num_of_pkts = p_in_payload->num_of_pkts;
        for(uint16_t k=0; k < MAXTEMPRLCFGDCBSIZE; k++)
           p_resp_payload->dcbdata[k] = 0;
        p_resp_pkt->src = p_pkt->dest;
        p_resp_pkt->dest = p_pkt->src;
    }
  return p_resp_pkt;
}

/*!
 ****************************************************************************
 * @brief Delete Temperature LCFG DCB Configuration
 *
 * @param[in]           p_pkt: input m2m2 packet
 *
 * @return              pointer to reponse m2m2 packet
 *****************************************************************************/
m2m2_hdr_t *tempr_dcb_command_delete_config(m2m2_hdr_t *p_pkt) {
  M2M2_DCB_STATUS_ENUM_t status = M2M2_DCB_STATUS_ERR_NOT_CHKD;

  PKT_MALLOC(p_resp_pkt, m2m2_dcb_temperature_data_t, 0);
  if (NULL != p_resp_pkt) {
    /* Declare a pointer to the response packet payload */
    PYLD_CST(p_resp_pkt, m2m2_dcb_temperature_data_t, p_resp_payload);

    if (delete_tempr_dcb() == ADPD4000_DCB_STATUS_OK) {
      tempr_set_dcb_present_flag(false);
      /*! dcb got deleted, so load the lcfg structure with default values */
      memcpy((uint8_t *)&active_temperature_lcfg,(uint8_t *)&tempr_default_lcfg,sizeof(active_temperature_lcfg));
      //check_dcb_erase = true;
      status = M2M2_DCB_STATUS_OK;
    } else {
      status = M2M2_DCB_STATUS_ERR_ARGS;
    }

    p_resp_payload->status = status;
    p_resp_payload->command = M2M2_DCB_COMMAND_ERASE_CONFIG_RESP;
    p_resp_payload->size = 0;
    for(uint16_t i=0; i< MAXTEMPRLCFGDCBSIZE; i++)
      p_resp_payload->dcbdata[i] = 0;
    p_resp_pkt->src = p_pkt->dest;
    p_resp_pkt->dest = p_pkt->src;
  }
  return p_resp_pkt;
}
#endif  //DCB
#endif //ENABLE_TEMPERATURE_APP