#ifndef __KERNEL_MEMORY_H
#define __KERNEL_MEMORY_H
#include "stdint.h"
#include "bitmap.h"

/*virtual address pool,it will be used to manage virutal address.*/
struct virtual_addr{  //virtual address has 4GB space,it is different with physic address(only has 32MB memory),so we define two address pool.
    struct bitmap vaddr_bitmap;  //used to manage distribution condition of virtual address by unit of page.
    uint32_t vaddr_start;  //the beginning of virtual address.
};

extern struct pool kernel_pool, user_pool;
void mem_init(void);
#endif