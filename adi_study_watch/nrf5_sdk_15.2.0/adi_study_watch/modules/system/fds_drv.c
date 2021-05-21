/*!
 *  \copyright Analog Devices
 * ****************************************************************************
 *
 * License Agreement
 *
 * Copyright (c) 2019 Analog Devices Inc.
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

#include "nrf_log_ctrl.h"
#include "nrfx.h"
#ifdef PROFILE_TIME_ENABLED
#include "us_tick.h"
#endif
/* FDS Driver Module Log settings */
#define NRF_LOG_MODULE_NAME ADI_FDS


#ifdef NRF_LOG_DEFAULT_LEVEL
#undef NRF_LOG_DEFAULT_LEVEL
#define NRF_LOG_DEFAULT_LEVEL 3
#endif
#define NRF_LOG_INFO_COLOR  2
#define NRF_LOG_DEBUG_COLOR 3
#include "nrf_log.h"
NRF_LOG_MODULE_REGISTER();

#include "fds.h"
#include "fds_drv.h"
//#include "nrf_delay.h"
#define COLOR_GREEN     "\033[1;32m"
#define COLOR_YELLOW    "\033[1;33m"
#define COLOR_CYAN      "\033[1;36m"

/* Macros to print on the console using colors. */

#define NRF_LOG_CYAN(...)   NRF_LOG_INFO(COLOR_CYAN   __VA_ARGS__)
#define NRF_LOG_YELLOW(...) NRF_LOG_INFO(COLOR_YELLOW __VA_ARGS__)
#define NRF_LOG_GREEN(...)  NRF_LOG_INFO(COLOR_GREEN  __VA_ARGS__)

/* Array to map FDS return values to strings. */
char const * fds_err_str[] =
{
    "FDS_SUCCESS",
    "FDS_ERR_OPERATION_TIMEOUT",
    "FDS_ERR_NOT_INITIALIZED",
    "FDS_ERR_UNALIGNED_ADDR",
    "FDS_ERR_INVALID_ARG",
    "FDS_ERR_NULL_ARG",
    "FDS_ERR_NO_OPEN_RECORDS",
    "FDS_ERR_NO_SPACE_IN_FLASH",
    "FDS_ERR_NO_SPACE_IN_QUEUES",
    "FDS_ERR_RECORD_TOO_LARGE",
    "FDS_ERR_NOT_FOUND",
    "FDS_ERR_NO_PAGES",
    "FDS_ERR_USER_LIMIT_REACHED",
    "FDS_ERR_CRC_CHECK_FAILED",
    "FDS_ERR_BUSY",
    "FDS_ERR_INTERNAL",
};

/* Array to map FDS events to strings. */
static char const * fds_evt_str[] =
{
    "FDS_EVT_INIT",
    "FDS_EVT_WRITE",
    "FDS_EVT_UPDATE",
    "FDS_EVT_DEL_RECORD",
    "FDS_EVT_DEL_FILE",
    "FDS_EVT_GC",
};

/* Flag to check fds initialization. */
static bool volatile m_fds_initialized;

#ifdef FREERTOS
#include "adi_osal.h"

static ADI_OSAL_SEM_HANDLE xfdsSem = NULL;
static void _adi_fds_param_init(void)
{
    adi_osal_SemCreate(&xfdsSem,0);
}
/**@brief   Wait for fds to initialize. */
static void _adi_fds_wait_for_fds_ready(void)
{
    adi_osal_SemPend(xfdsSem, ADI_OSAL_TIMEOUT_FOREVER);
}
static void _adi_fds_is_ready(void)
{
    adi_osal_SemPost( xfdsSem);
}
#else
static bool volatile m_fds_is_ready = 0;
static void _adi_fds_param_init(void)
{
    m_fds_is_ready = 0;
}
/**@brief   Wait for fds to initialize. */
static void _adi_fds_wait_for_fds_ready(void)
{
    while (!m_fds_is_ready)
    {
#ifdef SOFTDEVICE_PRESENT
    (void) sd_app_evt_wait();
#else
    __WFE();
#endif
    }
    m_fds_is_ready = 0;
}
static void _adi_fds_is_ready(void)
{
    m_fds_is_ready = 1;
}
#endif
/**@brief   adi FDS event handler. */
static void _adi_fds_evt_handler(fds_evt_t const * p_evt)
{
    NRF_LOG_GREEN("Event: %s received (%s)",
                  fds_evt_str[p_evt->id],
                  fds_err_str[p_evt->result]);

    switch (p_evt->id)
    {
        case FDS_EVT_INIT:
        {
            NRF_LOG_INFO("FDS init done");
            if (p_evt->result == FDS_SUCCESS)
            {
                _adi_fds_is_ready();
            }
        } break;

        case FDS_EVT_WRITE:
        case FDS_EVT_UPDATE:
        {
            if (p_evt->result == FDS_SUCCESS)
            {
                NRF_LOG_INFO("Record ID:\t0x%04x",  p_evt->write.record_id);
                NRF_LOG_INFO("File ID:\t0x%04x",    p_evt->write.file_id);
                NRF_LOG_INFO("Record key:\t0x%04x", p_evt->write.record_key);
                _adi_fds_is_ready();
            }
        } break;

        case FDS_EVT_DEL_RECORD:
        {
#ifdef BLE_PEER_ENABLE
            if(0xC000 > p_evt->write.file_id)//different with PM
            {
#endif
            if (p_evt->result == FDS_SUCCESS)
            {
                NRF_LOG_INFO("Record ID:\t0x%04x",  p_evt->del.record_id);
                NRF_LOG_INFO("File ID:\t0x%04x",    p_evt->del.file_id);
                NRF_LOG_INFO("Record key:\t0x%04x", p_evt->del.record_key);
                _adi_fds_is_ready();  /*sem post during delete record completion event*/
            }
#ifdef BLE_PEER_ENABLE
            }
#endif
        } break;

        case FDS_EVT_GC:
        {
            if (p_evt->result == FDS_SUCCESS)
            {
                _adi_fds_is_ready();
            }
        } break;

        default:
        break;
    }
}


/**@brief   Function for initializing FDS.
 *
 * @param None
 * @return None
 */
ret_code_t adi_fds_init(void)
{
    ret_code_t rc;
    _adi_fds_param_init();
    /* Register first to receive an event when initialization is complete. */
    (void)fds_register(_adi_fds_evt_handler);

    rc = fds_init();

    if(NRF_SUCCESS != rc)
    {
        NRF_LOG_INFO("fds_init fail. rc = %d",rc);
        return rc;
    }

    /* Wait for fds to initialize. */
    _adi_fds_wait_for_fds_ready();

    fds_stat_t stat = {0};

    rc = fds_stat(&stat);
    APP_ERROR_CHECK(rc);

    NRF_LOG_INFO("Found %d dirty records (ready to be garbage collected).", stat.dirty_records);
    NRF_LOG_INFO("Found %d valid records.", stat.valid_records);

    if (stat.dirty_records > FDS_DIRTY_REC_CNT)
    {
      NRF_LOG_INFO("Dirty records found - doing garbage collection");
      rc = fds_gc();
      APP_ERROR_CHECK(rc);
      _adi_fds_wait_for_fds_ready();
    }

    m_fds_initialized = true; //Update FDS driver initialisation to true
    return rc;
}

/**@brief   Function for initializing FDS.
 *
 * @param file_id: File name within FDS; File name within FDS
 * @param fds_index: Record key within file name to be deleted
 * @return return value of type ret_code_t
 */
ret_code_t adi_fds_delete_record(const uint16_t file_id, const uint16_t fds_index)
{
    ret_code_t rc;
    fds_record_desc_t   record_desc;
    fds_find_token_t    ftok;

    memset(&ftok, 0x00, sizeof(fds_find_token_t));
    rc = fds_record_find(file_id, fds_index, &record_desc, &ftok);
    if (rc == FDS_SUCCESS)
    {
        NRF_LOG_INFO("Records to Delete Found!!!.");
        //NRFX_CRITICAL_SECTION_ENTER();
        uint8_t dummy = 0;
        uint32_t err_code = sd_nvic_critical_region_enter(&dummy);
        APP_ERROR_CHECK(err_code);
        rc = fds_record_delete(&record_desc);
        //NRFX_CRITICAL_SECTION_EXIT();
        //adi_osal_ExitCriticalRegion();
        err_code = sd_nvic_critical_region_exit(0);
        APP_ERROR_CHECK(err_code);
        APP_ERROR_CHECK(rc);
        _adi_fds_wait_for_fds_ready();
        if (rc == FDS_SUCCESS)
        {
            fds_stat_t stat = {0};

            rc = fds_stat(&stat);
            APP_ERROR_CHECK(rc);

            NRF_LOG_INFO("Found %d dirty records (ready to be garbage collected).", stat.dirty_records);
            NRF_LOG_INFO("Found %d valid records.", stat.valid_records);

            if (stat.dirty_records > FDS_DIRTY_REC_CNT)
            {
                NRF_LOG_INFO("Dirty records found - doing garbage collection");

                rc = fds_gc();        //Minimising frequency of garbage colection
                APP_ERROR_CHECK(rc);
                _adi_fds_wait_for_fds_ready();
                NRF_LOG_INFO("Deleting Records Successful.");
            }
        }
        else
        {
            NRF_LOG_INFO("Deleting Records Fail.");
        }
    }

    return rc;
}

/**@brief   Function for deleting FDS file
 *
 * @param fds_file: either of ADI_RTC_FILE or ADI_DCB_FILE; File name within FDS
 * @return return value of type ret_code_t
 */
ret_code_t adi_fds_clear_entries(uint16_t fds_file)
{
    ret_code_t rc;
    rc = fds_file_delete((uint16_t)fds_file);
    APP_ERROR_CHECK(rc);
    //if (rc != FDS_SUCCESS)
    return rc;
}

/**@brief   Internal function which will differentiate between write/update to FDS file at particular record key
 *
 * @param file_id: either of ADI_RTC_FILE or ADI_DCB_FILE; File name within FDS
 * @param fds_index: Record key within file name to be updated
 * @param data: pointer to byte array which needs to be written
 * @param len: length in bytes to write
 * @return return value of type ret_code_t
 */
static ret_code_t _adi_fds_save(const uint16_t file_id, const uint16_t fds_index,
                                const void *data,
                                const uint16_t len)
{
    ret_code_t rc;
    fds_record_t        record;
    fds_record_desc_t   record_desc;
    fds_find_token_t    ftok;

    // Set up record.
    record.file_id           = (uint16_t)file_id;
    record.key               = (uint16_t)fds_index;
    record.data.p_data       = (const void *)data;
    record.data.length_words = len / 4;   /* one word is four bytes. */

    memset(&ftok, 0x00, sizeof(fds_find_token_t));
    rc = fds_record_find(file_id, fds_index, &record_desc, &ftok);
    if (rc == FDS_ERR_NOT_FOUND)
    {
        // Do 'write' operation
        NRF_LOG_INFO("Doing write operation");
        //NRFX_CRITICAL_SECTION_ENTER();
        uint8_t dummy = 0;
        uint32_t err_code = sd_nvic_critical_region_enter(&dummy);
        APP_ERROR_CHECK(err_code);
        rc = fds_record_write(&record_desc, &record);
        //NRFX_CRITICAL_SECTION_EXIT();
        err_code = sd_nvic_critical_region_exit(0);
        APP_ERROR_CHECK(err_code);
        APP_ERROR_CHECK(rc);
    }
    else if (rc == FDS_SUCCESS)
    {
        // Do 'update' operation
        NRF_LOG_INFO("Doing update operation");
        //NRFX_CRITICAL_SECTION_ENTER();
        uint8_t dummy = 0;
        uint32_t err_code = sd_nvic_critical_region_enter(&dummy);
        APP_ERROR_CHECK(err_code);
        rc = fds_record_update(&record_desc, &record);
        //NRFX_CRITICAL_SECTION_EXIT();
        err_code = sd_nvic_critical_region_exit(0);
        APP_ERROR_CHECK(err_code);
        APP_ERROR_CHECK(rc);
    }
    else
    {
        NRF_LOG_INFO("fds record find error!");//can't to here.
    }
    return rc;
}

/**@brief   Function for writing/updating to FDS file at particular record key
 *
 * @param file_id: either of ADI_RTC_FILE or ADI_DCB_FILE;File name within FDS
 * @param fds_index: Record key within file name to be updated
 * @param data: pointer to byte array which needs to be written
 * @param len: length in bytes to write
 * @return return value of type ret_code_t
 */
ret_code_t adi_fds_update_entry(const uint16_t file_id, const uint16_t fds_index,
                                const void *data,
                                const uint16_t len)
{
#ifdef PROFILE_TIME_ENABLED
  uint32_t fds_gc_t1,fds_gc_time,rec_save_time,rec_wait_till_ready_time;
#endif
    ret_code_t rc;

#ifdef PROFILE_TIME_ENABLED
    uint32_t rec_save_time_t1 = get_micro_sec();
#endif
    rc = _adi_fds_save(file_id,fds_index,data,len);
#ifdef PROFILE_TIME_ENABLED
    rec_save_time = get_micro_sec() - rec_save_time_t1;
#endif

    if (rc == FDS_SUCCESS)
    {

#ifdef PROFILE_TIME_ENABLED
        uint32_t rec_wait_till_ready_time_t1 = get_micro_sec();
#endif
        _adi_fds_wait_for_fds_ready();
#ifdef PROFILE_TIME_ENABLED
        rec_wait_till_ready_time = get_micro_sec() -  rec_wait_till_ready_time_t1;
#endif
    }
    else if(rc == FDS_ERR_NO_SPACE_IN_FLASH)
    {
        NRF_LOG_INFO("FDS_ERR_NO_SPACE_IN_FLASH,need execute GC!");
#ifdef PROFILE_TIME_ENABLED
        fds_gc_t1 = get_micro_sec();
#endif
        rc = fds_gc();
#ifdef PROFILE_TIME_ENABLED
        fds_gc_time = get_micro_sec() -  fds_gc_t1;
#endif
        APP_ERROR_CHECK(rc);

        _adi_fds_wait_for_fds_ready();

        rc = _adi_fds_save(file_id,fds_index,data,len);
        if (rc == FDS_SUCCESS)
        {
            _adi_fds_wait_for_fds_ready();
        }
        else
        {
            NRF_LOG_INFO("fds no space in flash!");//can't to here
        }
    }
    else
    {
        NRF_LOG_INFO("fds update or write error!");//can't to here
    }
#if 0 //reserved for test
    fds_stat_t stat = {0};

    rc = fds_stat(&stat);
    APP_ERROR_CHECK(rc);

    NRF_LOG_INFO("Found %d dirty records (ready to be garbage collected).", stat.dirty_records);
    NRF_LOG_INFO("Found %d valid records.", stat.valid_records);
#endif
    return rc;
}

/**@brief   Function for writing/updating to FDS file at particular record key
 *
 * @param file_id: either of ADI_RTC_FILE or ADI_DCB_FILE; File name within FDS
 * @param fds_index: Record key within file name to be updated
 * @param data: pointer to byte array which needs to be written
 * @param len: length in bytes to be read
 * @return return value of type ret_code_t
 */
ret_code_t adi_fds_read_entry(const uint16_t file_id, const uint16_t fds_index,
                              void *data,
                              uint16_t *len)
{
#ifdef PROFILE_TIME_ENABLED
uint32_t rec_close_time,rec_open_time;
#endif
    ret_code_t rc = NRF_SUCCESS;
    static fds_flash_record_t  flash_record;
    fds_record_desc_t   record_desc;
    fds_find_token_t    ftok;

    /* Error check on input arguments */
    if(data == NULL || len == NULL)
      rc = NRF_ERROR_NULL;
    APP_ERROR_CHECK(rc);
    if(*len == 0)
      rc = NRF_ERROR_INVALID_PARAM;
    APP_ERROR_CHECK(rc);

    /* It is required to zero the token before first use. */
    memset(&ftok, 0x00, sizeof(fds_find_token_t));

#ifdef PROFILE_TIME_ENABLED
    uint16_t rec_find_time_t1 = get_micro_sec();
#endif
    rc = fds_record_find(file_id, fds_index, &record_desc, &ftok);
#ifdef PROFILE_TIME_ENABLED
    uint32_t rec_find_time = get_micro_sec() - rec_find_time_t1;
#endif
    if (rc != FDS_SUCCESS)
    {
        return rc;
    }

#ifdef PROFILE_TIME_ENABLED
    uint16_t rec_open_time_t1 = get_micro_sec();
#endif

    rc = fds_record_open(&record_desc, &flash_record);

#ifdef PROFILE_TIME_ENABLED
    rec_open_time = get_micro_sec() - rec_open_time_t1;
#endif

    APP_ERROR_CHECK(rc);
    uint16_t r_len = ((flash_record.p_header->length_words * 4) < *len) ? (flash_record.p_header->length_words * 4):(*len);

    /* Access the record through the flash_record structure. */
    //NRFX_CRITICAL_SECTION_ENTER();
    uint8_t dummy = 0;
    uint32_t err_code = sd_nvic_critical_region_enter(&dummy);
    APP_ERROR_CHECK(err_code);
    memcpy(data, flash_record.p_data, r_len);
    //NRFX_CRITICAL_SECTION_EXIT();
    err_code = sd_nvic_critical_region_exit(0);
    APP_ERROR_CHECK(err_code);
    *len = r_len;

#ifdef PROFILE_TIME_ENABLED
    uint32_t rec_close_time_t1 = get_micro_sec();
#endif
    /* Close the record when done. */
    rc = fds_record_close(&record_desc);

#ifdef PROFILE_TIME_ENABLED
    rec_close_time = get_micro_sec() - rec_close_time_t1;
#endif

    APP_ERROR_CHECK(rc);
    return NRF_SUCCESS;
}

/**@brief   Function to find if particular record key is created within a file in FDS
 *
 * @param file_id: either of ADI_RTC_FILE or ADI_DCB_FILE; File name within FDS
 * @param fds_record_Key: Record key within file name to be searched/checked
 * @return return value of type ret_code_t
 */
ret_code_t adi_fds_check_entry(const uint16_t file_id, const uint16_t fds_record_Key)
{
#ifdef PROFILE_TIME_ENABLED
uint32_t rec_find_time ;
#endif
    ret_code_t rc;
    fds_record_desc_t   record_desc;
    fds_find_token_t    ftok;

    memset(&ftok, 0x00, sizeof(fds_find_token_t));
#ifdef PROFILE_TIME_ENABLED
    uint32_t rec_find_t1 =  get_micro_sec();
#endif

    rc = fds_record_find(file_id, fds_record_Key, &record_desc, &ftok);
    if (rc == FDS_ERR_NOT_FOUND)
    {
        NRF_LOG_INFO("Record Not Found!!!");
    }
    else if (rc == FDS_SUCCESS)
    {
        NRF_LOG_INFO("Record Found!!!");
    }

#ifdef PROFILE_TIME_ENABLED
  rec_find_time =  get_micro_sec() - rec_find_t1;
#endif
    return rc;
}
