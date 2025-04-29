#ifndef __FS_DIR_H
#define __FS_DIR_H

#include "stdint.h"
#include "fs.h"
#include "inode.h"

#define MAX_FILE_NAME_LEN  16

/*
The structure of directory
It note be stored in disk, just be created in memory when we operate directory
*/
struct dir {
    struct inode* ionde;
    uint32_t dir_pos;
    uint32_t dir_buf[512];
}

/*The structure of directory entry*/
struct dir_entry {
    char filename[MAX_FILE_NAME_LEN];
    uint32_t i_no;  //The index of inode in inode table
    enum file_types f_type;  //the file's type
};

#endif