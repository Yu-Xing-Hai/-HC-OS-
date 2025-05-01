#ifndef __FS_H
#define __FS_H

#include "stdint.h"

#define MAX_FILES_PER_PART  4096  //The max mumber of create-files each part support
#define BITS_PER_SECTOR  4096  //Each sector's bit mumber
#define SECTOR_SIZE  512  //The Byte size of one sector
#define BLOCK_SIZE  SECTOR_SIZE

/*The type of file's type*/
enum file_types {
    FT_UNKNOWN,  //The file type which is not supported, it's value is 0
    FT_REGULAR,  //The regular file type, it's value is 1
    FT_DIRECTORY  //The type of directory, it's value is 2
};

void filesys_init(void);

#endif
