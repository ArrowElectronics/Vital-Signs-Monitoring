/**
***************************************************************************
* @addtogroup Tasks
* @{
* @file         light_fs.h
* @author       ADI
* @version      V1.0.1
* @date         16-June-2020
* @brief        Header file contains major functionalities/modules
                required for file operations.
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
* Copyright (c) 2020 Analog Devices Inc.
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

#ifndef LIGHT_FS
#define LIGHT_FS

#include "nand_functions.h"

/* Macros */
#define MAX_NO_TIME_STAMPS_FOR_FILES_TO_RECORD  10

#define RESERVEDBLOCK   0
#define TOCBLOCK        1
#define CFGBLOCK        (TOCBLOCK+1)//2
#define TMPBLOCK        (TOCBLOCK+2)
#define FILEBLOCK       (TOCBLOCK+3)

#define DATA_OFFSET     0
#define SPARE_OFFSET    0x1010
#define FILE_NAME_LEN   16
#define FOOT_PRINT_LEN  16


#define CONFIG_FILE_POSITION  0

#define FSFOOTPRINT     "LFSFOOTPRINTKEY"
#define FSVERSION       2
#define FSREVISION      0

#define MAX_NUM_OF_BYTES_FOR_PAGE_READ_TEST 100
#define TOC_REPEATED_UPDATE_SIZE 16384


#define MAXFILENUMBER                                     63
#define MAX_NO_OF_WORDS                                   64 //change it to 64 for 2048 blocks
#define MAX_NO_OF_BITS_IN_WORD                            32
#define TABLE_PAGE_INDEX_IN_TOC                           63
#define MAX_NO_LFS_STATUS_REGISTERS                        3
#define NUM_OF_TOC_PAGES_TO_DEL_FOR_HEADER_UPD             2
#define MAX_NUM_OF_VARIABLES_IN_TABLE_PAGE                 4
#define TOC_BLOCK_FIRST_PAGE 64
#define TOC_BLOCK_LAST_PAGE(x, y) (x + y)


// macros
#define DATA_FLASH_SIZE (MEM_SIZE-(FILEBLOCK*PAGES_PER_BLOCK*PAGE_SIZE))

typedef enum {
   LFS_SUCCESS,
   LFS_ERROR,
   LFS_NUM_BLOCKS_OUT_OF_BOUNDS_ERROR,
   LFS_UPDATE_TABLE_FILE_ERROR,
   LFS_MEMORY_FULL_ERROR,
   LFS_READ_RESERVED_INFO_ERROR,
   LFS_WRITE_FAILED_RESERVED_INFO_ERROR,
   LFS_FILE_NOT_FOUND_ERROR,
   LFS_UPDATE_HEADER_ERROR,
   LFS_FILE_CLOSE_ERROR,
   LFS_INVALID_NEXT_WRITABLE_CONFIG_PAGE_ERROR,
   LFS_INVALID_NEXT_WRITABLE_DATA_PAGE_ERROR,
   LFS_NOT_VALID_FORMAT_ERROR,
   LFS_FORMAT_ERROR,
   LFS_TOC_FORMAT_ERROR,
   LFS_BAD_BLOCK_HEADER_CHECK_ERROR,
   LFS_SET_FILE_READ_POSITION_ERROR,
   LFS_FILE_READ_ERROR,
   LFS_FILE_WRITE_ERROR,
   LFS_FILE_PARTIALLY_WRITTEN_ERROR,
   LFS_MAX_FILE_COUNT_ERROR,
   LFS_CONFIG_FILE_POSITION_ERROR,
   LFS_OOB_ERROR,
} elfs_result;

typedef enum{
  LFS_BAD_BLOCK_MARKER_UPDATE=1,
  LFS_HEAD_POINTER_UPDATE=2,
  LFS_TAIL_POINTER_UPDATE=3,
  LFS_INITIALIZE_BUFFER_FLAG_UPDATE=4,
  LFS_MEM_FULL_FLAG_UPDATE=5,
  LFS_OFFSET_UPDATE=6,
  LFS_CONFIG_FILE_POS_OCCUPIED_UPDATE=7,
}elfs_update_type;

#define MAX_UPDATE_TYPE 7

typedef enum {
  LFS_CONFIG_FILE,
  LFS_DATA_FILE,
  LFS_INVALID_FILE,
} elfs_file_type;
typedef enum {
  LFS_MODE_FAST,
  LFS_MODE_SECURE,
  LFS_MODE_MANUAL,
  LFS_MODE_NUMBER,
} elfs_op_mode;
struct _page_header {
  uint32_t next_page;
  uint8_t occupied;
};
struct _memory_buffer {
  struct _page_header next_page_data;
  uint8_t data[PAGE_SIZE];
};
typedef struct _fs_info { 
  char foot_print[FOOT_PRINT_LEN];
  uint32_t version;
  uint32_t revision;
} _fs_info;

typedef struct _file_header{
  uint8_t header_number;
  elfs_file_type file_type;
  uint8_t file_name[FILE_NAME_LEN];
  uint32_t mem_loc;
  uint32_t file_size;
  uint32_t last_used_page;
  uint32_t time_stamp;  
  struct _memory_buffer *tmp_write_mem;
} _file_header;

typedef struct _table_file_header{
  uint32_t bad_block_marker[MAX_NO_OF_WORDS];
  uint32_t head_pointer; // For circular buffer this has page precison
  uint32_t tail_pointer;// this has block precision
  uint8_t initialized_circular_buffer;// this for first time initialization and first time check before first time write
  struct _memory_buffer *tmp_write_mem;
  uint16_t mem_full_flag;
  uint16_t offset;
  uint16_t config_low_touch_occupied;
}_table_file_header;

typedef struct _table_file_handler{
  _table_file_header table_file_info;
  //struct MemoryBuffer * tmp_write_mem;
}_table_file_handler;


typedef struct _fs_format_debug_info{
  uint8_t erase_failed_due_bad_block_check;
  uint8_t wrap_around_cond;
  uint8_t nothing_is_written_to_erase_error;
  uint8_t mem_full_in_partial_erase;
  uint8_t toc_mem_erased_flag;
  uint8_t succesfull_erase_flag;
  uint16_t num_blocks_erased_in_mem_full_partial_erase;
  uint16_t num_blocks_erased_in_partial_erase_1;
  uint16_t num_blocks_erased_in_partial_erase_2;
  uint16_t num_times_format_failed_due_bad_blocks_1;
  uint16_t num_times_format_failed_due_bad_blocks_2;
  uint32_t format_src_blk_ind;
  uint32_t format_dest_blk_ind_1;
  uint32_t format_dest_blk_ind_2;
}fs_format_debug_info;
  

typedef struct _file_handler {
  _file_header head;
  uint32_t current_read_pos;
  uint32_t current_offset;
  uint16_t tmp_write_mem_loc;
  uint32_t curr_write_mem_loc;
  elfs_op_mode op_mode;
} _file_handler;

elfs_result lfs_openfs(_memory_properties * mem_prop);
elfs_result lfs_get_number_of_files(uint8_t * number_of_files);
elfs_result lfs_get_file_size(_file_handler *file_handler,
                              uint32_t * fileSize);
elfs_result get_bad_block_number(uint32_t *bad_block_num,
                                uint32_t src_ind,
                                uint32_t end_ind,
                                _table_file_handler *table_file_handler);
elfs_result lfs_get_remaining_space(uint32_t * remaining_bytes,
                                  elfs_file_type file_type,
                                  _table_file_handler *table_file_handler);
elfs_result lfs_open_file_by_number(uint8_t file_no,
                                  _file_handler *file_handler);
elfs_result lfs_get_config_file_status(bool *out_file_status);
elfs_result lfs_get_file_indexes_list(uint8_t * out_file_indexes,
                                     uint8_t * out_file_no);
elfs_result lfs_create_file(uint8_t * file_name, _file_handler *file_handler,
                           _table_file_handler *table_file_handler,
                           struct _memory_buffer * memory_location,
                           elfs_file_type type);
elfs_result lfs_open_file_by_name(char * file_name,
                                 _file_handler *file_handler);
elfs_result lfs_read_file(uint8_t * outBuffer,
                         uint32_t offset,
                         uint32_t size,
                         _file_handler *file_handler);
elfs_result lfs_delete_config_file(_file_handler *file_handler);
elfs_result lfs_update_file(uint8_t *inBuffer,
                           uint16_t size,
                           _file_handler *file_handler,
                           _table_file_handler *table_file_handler);
elfs_result lfs_end_file(_file_handler *file_handler,
                        _table_file_handler *table_file_handler);
elfs_result lfs_erase_memory(bool forze);
elfs_result erase_toc_memory(bool forze);
elfs_result lfs_check_comp_version(bool * is_compatible);
elfs_result lfs_format(bool bfmt_config_blk);
elfs_result lfs_set_time_stamp(_file_handler *file_handler,
                              uint32_t time_stamp);
elfs_result lfs_set_operating_mode(_file_handler *file_handler,
                                  elfs_op_mode op_mode);
elfs_result lfs_refresh_header(_file_handler *file_handler,
                              _table_file_handler *table_file_handler);
elfs_result lfs_mark_bad(uint32_t block_index);
elfs_result lfs_mark_good(uint32_t block_index);
elfs_result initialize_circular_buffer();
elfs_result lfs_update_config_file(uint8_t *in_buffer,
                          uint16_t size,
                          _file_handler *file_handler);
elfs_result lfs_refresh_config_header(_file_handler *file_handler);
elfs_result lfs_end_config_file(_file_handler *file_handler);
elfs_result lfs_read_config_file(uint8_t *out_buffer,
     _file_handler *file_handler);
elfs_result get_bad_block(uint32_t *bad_block_num);
elfs_result lfs_flash_reset();
elfs_result lfs_reset_reserved_block();
elfs_result lfs_reset_toc_block_info();
elfs_result lfs_update_pattern_file(uint8_t *in_buffer,
                          uint16_t start_block_num,
                          uint16_t size,
                          _file_handler *file_handler,
                          _table_file_handler *table_file_handler,
                          uint8_t first_time_write);
int get_pointers_info(uint32_t *head_pointer,uint32_t *tail_pointer,uint16_t table_page_flags[]) ;
int read_tmp_blk(uint32_t page_number,uint32_t *pdata_memory,uint8_t file_type);
int read_page_data(uint32_t page_num, uint8_t *pdata_mem, uint16_t page_size);
int read_page_ecc_zone(uint32_t page_num, uint32_t *pnext_page, uint8_t *poccupied);
elfs_result lfs_get_file_count(uint8_t * gn_file_count);
elfs_result read_table_file_in_toc(_table_file_handler *table_file_handler);
#endif
