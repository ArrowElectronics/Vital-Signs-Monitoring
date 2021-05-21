#ifndef _LFS_TESTBENCH_HEADER_
#define _LFS_TESTBENCH_HEADER_


#include "nand_functions.h"
#include <hal/nrf_gpio.h>
#include "FreeRTOS.h"
#include "task.h"
#include "light_fs.h"

//#define CALCMEMVAL(size,position) (uint8_t)((size*position)>>24 | (size*position)>>16 |(size*position)>>8 |(size*position))
#define CALCMEMVAL(size,position) (uint8_t)((size*position)>>24 | (size*position)>>8 | (size*position))

#define FILESIZELIMIT   10000000
#define TEMPBUFFERSIZE  4096

typedef enum {
  LFS_TESTBENCH_ERROR, 
  LFS_TESTBENCH_SUCCESS,
  LFS_TESTBENCH_ERROR_READING_FILE,
  LFS_TESTBENCH_ERROR_OPENING_FILE,
  LFS_TESTBENCH_ERROR_GETING_FILE_LIST,
  LFS_TESTBENCH_ERROR_ENDING_FILE,
  LFS_TESTBENCH_ERROR_UPDATING_FILE,
  LFS_TESTBENCH_ERROR_CORRUPTED_FILE,
  LFS_TESTBENCH_ERROR_MAX_FILE_COUNT_ERROR,
  LFS_TESTBENCH_CONFIG_FILE_POSITION_ERROR,  
  LFS_TESTBENCH_ERROR_CREATING_FILE,
  LFS_TESTBENCH_ERROR_GETING_CONFIG_FILE_STATUS,
} elfs_testbench_result;


elfs_testbench_result check_file(_file_handler *tmpFileHandler,uint32_t * error_counter);
elfs_testbench_result check_files(uint32_t * error_counter);
elfs_testbench_result check_config_files(uint8_t *wr_buffer,
                                          uint32_t * error_counter);
elfs_testbench_result check_config_file(uint8_t *wr_buffer,
                                       _file_handler *tmp_file_handler,
                                        uint32_t *error_counter);
elfs_testbench_result delete_all_files();
elfs_testbench_result createBadBlock(uint32_t badBlockIndex);
elfs_testbench_result create_and_write_file(_file_handler *file_handler,_table_file_handler *tmptablefilehandler,struct _memory_buffer *light_fs_mem,elfs_file_type file_type, uint32_t in_file_size,char *file_name_ext, elfs_op_mode op_mode);
void gen_file_chunk(uint32_t start_pos, uint32_t size, uint32_t file_size, uint8_t * out_buffer);
elfs_testbench_result create_and_write_config_file(uint8_t *wr_buff,_file_handler *file_handler,_table_file_handler *tmp_table_file_handler,struct _memory_buffer *light_fs_mem,uint32_t in_file_size,char *file_name_ext, elfs_op_mode op_mode);
#endif
