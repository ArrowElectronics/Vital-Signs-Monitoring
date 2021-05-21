
#ifndef TEST0_H
#define TEST0_H

#include "nand_functions.h"
#include <hal/nrf_gpio.h>
#include "FreeRTOS.h"
#include "task.h"
#include "light_fs.h"

void Test0(_file_handler * file_handler,_table_file_handler *table_file_handler,struct _memory_buffer *light_fs_mem);


#endif
