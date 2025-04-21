#ifndef __FS_INODE_H
#define __FS_INODE_H

#include "stdint.h"

/*the structure of inode*/
struct inode {
    uint32_t i_no;  //the number of inode
    /*
    When the inode indicated the file, the i_size is indicated the size of file,
    when the inode indicated the directory, the i_size is indicated the title size of directory entry.
    */
    uint32_t i_size;  //the unite is Byte

    uint32_t i_open_cnts;  //recode the number of times of file be opened
    bool write_deny;  //When the thread want to write this file, it must check the flag to make sure mutual

    /*i_sectors[0~11] is the direct block's pointer, the i_secotrs[12] be used to store one-level in-direct block's pointer*/
    uint32_t i_sectors[13];  //Our data block's size is euqal with sector, so i_sectors is same to i_block;
    struct list_elem inode_tag;


};

#endif