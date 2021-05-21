#include "light_fs.h"
#include "light_fs_test_bench.h"

#define ITERATIONNUMBERT2               10
#define MAXNUMBEROFFILESPERITERATIONT2  64
#define NUMBEROFBADBLOCKS               10
#define BADBLOCKINDEXLIMIT              1000
#define DATA_FILE_TO_TEST               1
void Test2(_file_handler * file_handler, _table_file_handler *table_file_handler, struct _memory_buffer *light_fs_mem);