#ifndef __FILE_H
#define __FILE_H

#include "stdint.h"
#include "ide.h"

/*The structure of file, which is pointered by file descriptor in PCB*/
struct file {
    uint32_t fd_pos;  //Record the offset of curent file-operating
    uint32_t fd_flag;
    struct inode* fd_inode;  //Used to point inode in part->open_inodes(inode queue)
};

/*Standerd file descriptor*/
enum std_fd {
    stdin_no,  // 0 standerd input
    stdout_no,  //1 standerd output
    stderr_no  //2 standard error
};

/*The type of bitmap, we will use this when we want to sync bitmap to disk*/
enum bitmap_type {
    INODE_BITMAP,  //The bitmap of inode
    BLOCK_BITMAP  //The bitmap of block
};

#define MAX_FILE_OPEN 32  //The max file times system can open at the same time

int32_t get_free_slot_in_global(void);
int32_t pcb_fd_install(int32_t globa_fd_idx);
int32_t inode_bitmap_alloc(struct partition* part);
int32_t block_bitmap_alloc(struct partition* part);
void bitmap_sync(struct partition* part, uint32_t bit_idx, uint8_t btmp);

#endif