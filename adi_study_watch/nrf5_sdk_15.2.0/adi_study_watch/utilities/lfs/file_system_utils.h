/*! *****************************************************************************
 * @file:    file_system_utils.h
 * @brief:   file_system utility file.
 * @details: This header file contains the helper functions for the LFS
 * @version: $Revision: 1.0.1
 * @date:    $Date:16-June-2020
 -----------------------------------------------------------------------------
Copyright (c) 2020 Analog Devices, Inc.

All rights reserved.

Redistribution and use in source and binary forms, with or without modification,
are permitted provided that the following conditions are met:
  - Redistributions of source code must retain the above copyright notice,
    this list of conditions and the following disclaimer.
  - Redistributions in binary form must reproduce the above copyright notice,
    this list of conditions and the following disclaimer in the documentation
    and/or other materials provided with the distribution.
  - Modified versions of the software must be conspicuously marked as such.
  - This software is licensed solely and exclusively for use with processors
    manufactured by or for Analog Devices, Inc.
  - This software may not be combined or merged with other code in any manner
    that would cause the software to become subject to terms and conditions
    which differ from those listed here.
  - Neither the name of Analog Devices, Inc. nor the names of its
    contributors may be used to endorse or promote products derived
    from this software without specific prior written permission.
  - The use of this software may or may not infringe the patent rights of one
    or more patent holders.  This license does not release you from the
    requirement that you obtain separate licenses from these patent holders
    to use this software.

THIS SOFTWARE IS PROVIDED BY ANALOG DEVICES, INC. AND CONTRIBUTORS "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, NON-
INFRINGEMENT, TITLE, MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL ANALOG DEVICES, INC. OR CONTRIBUTORS BE LIABLE
FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, PUNITIVE OR
CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, DAMAGES ARISING OUT OF
CLAIMS OF INTELLECTUAL PROPERTY RIGHTS INFRINGEMENT; PROCUREMENT OF
SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
POSSIBILITY OF SUCH DAMAGE.

*****************************************************************************/
#ifndef FILE_SYSTEM_UTILS_H
#define FILE_SYSTEM_UTILS_H
//#include <fs_api.h>
#include "common_application_interface.h"
#include "post_office.h"
#include "us_tick.h"
#include <file_system_interface.h>
#include <light_fs.h>
#ifdef USE_FS

#define ADI_FS_LDO  3

#define MEM_SIZE  536870912 //NUM_OF_BLOCKS x PAGES_PER_BLOCK x PAGE_SIZE bytes
#define BLOCK_SIZE 262144   //PAGES_PER_BLOCK x PAGE_SIZE bytes
#define PAGE_SIZE 4096      //bytes
#define PAGES_PER_BLOCK 64
#define NUM_OF_BLOCKS 2048
#define DELIMITER ','
#define MIN_DATA_FILE_BLOCK_PAGE_NUMBER FILEBLOCK * PAGES_PER_BLOCK
#define MAX_DATA_FILE_BLOCK_PAGE_NUMBER ((NUM_OF_BLOCKS) * PAGES_PER_BLOCK) - 1
#define FILE_PAGE_ROLL_OVER (MAX_DATA_FILE_BLOCK_PAGE_NUMBER - MIN_DATA_FILE_BLOCK_PAGE_NUMBER + 1)
typedef enum {
  FS_STATUS_OK = 0x00,
  FS_STATUS_ERR,
  FS_STATUS_ERR_EOF,
  FS_STATUS_ERR_STDIO,
  FS_STATUS_ERR_EOD,
  FS_STATUS_ERR_INVALID_DIR,
  FS_STATUS_ERR_EMPTY_DIR,
  FS_STATUS_ERR_MEMORY_FULL,
  FS_STATUS_ERR_MAX_FILE_COUNT,
  FS_STATUS_ERR_CONFIG_FILE_POSITION,
  FS_STATUS_ERR_INVALID_FILE,
  FS_STATUS_INIT_DRIVER_ERR,
  FS_STATUS_ERR_CONFIG_FILE_NOT_FOUND,
  FS_STATUS_NO_FILE_TO_APPEND_ERROR,
}FS_STATUS_ENUM_t;

typedef enum {
  FS_FILE_ACCESS_START = 0x00,
  FS_FILE_ACCESS_IN_PROGRESS = 0x01,
} FS_FILE_STATE_ENUM_t;

typedef struct _file_info_t {
  uint8_t  file_name[16];
  uint32_t start_page;
  uint32_t end_page;
  uint32_t file_size;
} file_info_t;

typedef struct _vol_info_buff_t {
   uint32_t tmp_head_pointer;
   uint32_t tmp_tail_pointer;
   uint32_t bad_block_num;
   uint8_t bad_block_updated;
   uint32_t used_memory;
   uint32_t avail_memory;
}vol_info_buff_t;

int fs_hal_vol_name_size( void );
const char *fs_hal_vol_name( void );
FS_STATUS_ENUM_t fs_hal_init( void );
FS_STATUS_ENUM_t fs_hal_mount( void );
FS_STATUS_ENUM_t fs_hal_format( bool_t bfmt_config_blk );
FS_STATUS_ENUM_t fs_hal_list_dir(const char* p_dir_path, char *p_file_path, uint32_t length, uint8_t *filetype, uint32_t *filesize);
FS_STATUS_ENUM_t fs_hal_get_size(char* p_path, uint32_t *p_size);
FS_STATUS_ENUM_t fs_hal_read_file(char* p_file_path, uint8_t *p_buffer, uint32_t *p_length, uint32_t *p_page_number);
FS_STATUS_ENUM_t fs_hal_open_file(char* p_file_path);
FS_STATUS_ENUM_t fs_hal_open_config_file(uint8_t* p_file_path);
FS_STATUS_ENUM_t fs_hal_find_config_file( uint8_t *pfile_name, uint8_t nfile_nameLen, bool_t bdelete_config_file, uint8_t *pconfig_file_found);
FS_STATUS_ENUM_t fs_hal_delete_config_file( uint8_t *pfile_name, uint8_t nfile_nameLen);
FS_STATUS_ENUM_t fs_hal_get_file_count(uint8_t *p_file_count);
FS_STATUS_ENUM_t fs_hal_close_file(_file_handler *file_handler);
FS_STATUS_ENUM_t fs_hal_write_file(uint8_t *p_buffer, uint32_t *nitems, _file_handler *file_handler);
M2M2_APP_COMMON_STATUS_ENUM_t fs_hal_write_packet_stream(m2m2_hdr_t *pkt);
M2M2_APP_COMMON_STATUS_ENUM_t fs_hal_test_pattern_write(uint8_t* pBuff, uint32_t nBuffSize);
FS_FILE_STATE_ENUM_t fs_hal_write_access_state( void );
_file_handler *fs_hal_write_file_pointer( void );
_file_handler *fs_hal_get_read_file_pointer( void );
FS_STATUS_ENUM_t fs_hal_get_vol_info(uint32_t *p_size);
void fs_hal_write_error_resp(m2m2_hdr_t *response_pkt,M2M2_ADDR_ENUM_t *src);
FS_STATUS_ENUM_t fs_hal_unblock_flash(void);
FS_STATUS_ENUM_t fs_flash_power_on (bool benable);
FS_STATUS_ENUM_t fs_hal_find_file(char* filename);
uint32_t fs_hal_get_remaining_memory (void);
FS_STATUS_ENUM_t fs_hal_flash_reset();
FS_STATUS_ENUM_t fs_hal_read_pageoffset(char* p_file_path, uint8_t *p_buffer, uint32_t *p_length,  uint32_t *p_filesize,uint32_t in_page_number,uint32_t *p_out_page_number,uint32_t *next_page_number);
M2M2_APP_COMMON_STATUS_ENUM_t fs_hal_test_pattern_config_write(uint8_t* pbuff, uint32_t nbuff_size);
FS_STATUS_ENUM_t fs_hal_write_config_file(uint8_t *p_buffer, uint32_t *nitems, _file_handler *file_handler);
FS_STATUS_ENUM_t fs_hal_write_blocks( uint8_t *pbuff, uint16_t start_block_num,uint16_t num_blocks_write,uint8_t first_time_write );
FS_STATUS_ENUM_t fs_hal_fixed_pattern_write_file(uint8_t *p_buffer,uint16_t start_block_num, uint16_t *nitems, _file_handler *file_handler,uint8_t first_time_write);
FS_STATUS_ENUM_t fs_hal_get_file_info(uint8_t* pfile_index, file_info_t *pfile_info);
FS_STATUS_ENUM_t fs_hal_page_read_test(uint32_t* ppage_num, m2m2_file_sys_page_read_test_resp_pkt_t *pfile_info,uint8_t num_bytes);
FS_STATUS_ENUM_t fs_write_rsd_block(uint32_t data[], uint16_t size);
FS_STATUS_ENUM_t fs_block_erase(uint16_t block_no);
bool UpdateFileInfo();
FS_STATUS_ENUM_t fs_hal_append_file();
void set_write_handler_mode();
uint32_t fs_get_file_page_number_offset(uint32_t *CurrentPageNumber, uint32_t *StartPageNumber, bool bHandleRollover);
#ifdef TEST_FS_NAND
void fs_hal_test_features(void);
void fs_hal_test_file_write_read(uint32_t inp_num_iter, uint32_t block_size);
#endif //TEST_FS_NAND

#endif // USE_FS

#endif  // FILE_SYSTEM_UTILS_H
