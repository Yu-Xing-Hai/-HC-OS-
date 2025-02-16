#ifndef __KERNEL_MEMORY_H
#define __KERNEL_MEMORY_H
#include "stdint.h"
#include "bitmap.h"
#include "list.h"

#define NULL ((void*)0)

#define PG_SIZE 4096  //4kB = 4 * 1024 = 4096 Byte

#define PG_P_1  1
#define PG_P_0  0
#define PG_RW_R 0
#define PG_RW_W 2
#define PG_US_S 0
#define PG_US_U 4

#define DESC_CNT 7

extern struct pool kernel_pool, user_pool;

/*virtual address pool,it will be used to manage virutal address.*/
struct virtual_addr{  //virtual address has 4GB space,it is different with physic address(only has 32MB memory),so we define two address pool.
    struct bitmap vaddr_bitmap;  //used to manage distribution condition of virtual address by unit of page.
    uint32_t vaddr_start;  //the beginning of virtual address.
};

/*It will be used to judge which pool we will use.*/
enum pool_flags {
    PF_KERNEL = 1,  //The pool flag for kernel.
    PF_USER = 2   //The pool flag for user.
};

/*Memory block*/
struct mem_block {
    struct list_elem free_elem;
};

/*Memory block descriptor*/
struct mem_block_desc {
    uint32_t block_size;  //size of memory block
    uint32_t blocks_per_arena;  //number of memory block in ONE arena
    struct list free_list;  //free list of memory block
};

/*Get memory*/
void mem_init(void);
uint32_t* pte_ptr(uint32_t vaddr);
uint32_t* pde_ptr(uint32_t vaddr);
void* malloc_page(enum pool_flags pf, uint32_t pg_cnt);
void* get_kernel_pages(uint32_t pg_cnt);
void* get_user_pages(uint32_t pg_cnt);
void* get_a_page(enum pool_flags pf, uint32_t vaddr);
uint32_t addr_v2p(uint32_t vaddr);
void block_desc_init(struct mem_block_desc* desc_array);
void* sys_malloc(uint32_t size);

/*Release memory*/
void pfree(uint32_t pg_phy_addr);
void mfree_page(enum pool_flags pf, void* _vaddr, uint32_t pg_cnt);
void sys_free(void* ptr);

#endif