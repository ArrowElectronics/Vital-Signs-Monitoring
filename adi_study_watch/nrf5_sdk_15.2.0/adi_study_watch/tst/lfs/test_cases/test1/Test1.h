#include "light_fs.h"
#include "light_fs_test_bench.h"

#define ITERATIONNUMBERT1               1
#define MAXNUMBEROFFILESPERITERATION    1
#define NUMBEROFBADBLOCKS               10
#define BADBLOCKINDEXLIMIT              1000

void Test1(_file_handler * file_handler, _table_file_handler *table_file_handler, struct _memory_buffer * light_fs_mem);