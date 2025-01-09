#include "memory.h"
#include "stdint.h"
#include "print.h"

#define PG_SIZE 4096  //4kB = 4 * 1024 = 4096 Byte
/*
The kernel stack's bottom is 0xc009f000， 0xc009e000 is kernel main thread's pcb.
one page can present 128MB memory(4KB * 1024 * 8 * 4KB),so, we put bitmap to address of 0xc009a000,
so,our system can offer 4(0x9b000-0x9a000 = 2^12 = 4KB) page of bitmap,these can indicate 512MB memory.
*/
#define MEM_BITMAP_BASE 0xc009a000
/*-------------------------------------------------*/
#define K_HEAP_START 0xc0100000  //pay attention: PAGE_DIR_TABLE_POS equ 0x100000 is an physic address and this is virtual address.

/*The struct of memory pool, it will be used to create two object to manage kernel memory pool and user memory pool.*/
struct pool {   //used to manage physic memory.
    struct bitmap pool_bitmap;
    uint32_t phy_addr_start;
    uint32_t pool_size;
};

struct pool kernel_pool, user_pool; //manage physic
struct virtual_addr kernel_vaddr;   //manage virtual

/*initialize memory pool and ralated structures*/
static void mem_pool_init(uint32_t all_mem) {
    put_str("mem_pool_init start\n");
    uint32_t page_table_size = PG_SIZE * 256;  //used to record the Byte's quantity of (PDT add PT). The PDT's 769~1022 totally have 254 PDE,these point to 254 page,0 and 769 point to same one page(this page point to low 1MB physic memory),1023 point to PDT,so,totally have 256 page,these page have be occupied.
    uint32_t used_mem = page_table_size + 0x100000;  //0x100000 is low 1MB physic memory.
    uint32_t free_mem = all_mem - used_mem;
    uint16_t all_free_page = free_mem / PG_SIZE;
    uint16_t kernel_free_pages = all_free_page / 2;
    uint16_t user_free_pages = all_free_page - kernel_free_pages;

    uint32_t kbm_length = kernel_free_pages / 8;   //length of kernel Bitmap.
    uint32_t ubm_length = user_free_pages / 8;

    uint32_t kp_start = used_mem;  //The low 1MB + PDT + PT are continuous in memory,so,this is kernel pool start address.
    uint32_t up_start = kp_start + kernel_free_pages * PG_SIZE;

    kernel_pool.phy_addr_start = kp_start;
    user_pool.phy_addr_start = up_start;

    kernel_pool.pool_size = kernel_free_pages * PG_SIZE;
    user_pool.pool_size = user_free_pages * PG_SIZE;

    kernel_pool.pool_bitmap.btmp_bytes_len = kbm_length;
    user_pool.pool_bitmap.btmp_bytes_len = ubm_length;

    kernel_pool.pool_bitmap.bits = (void*)MEM_BITMAP_BASE;
    user_pool.pool_bitmap.bits = (void*)(MEM_BITMAP_BASE + kbm_length);

    put_str("kernel_pool_bitmap_start: ");
    put_int((int)kernel_pool.pool_bitmap.bits);
    put_str("  kernel_pool_phy_addr_start: ");
    put_int(kernel_pool.phy_addr_start);
    put_str("\n");
    put_str("user_pool_bitmap_start: ");
    put_int((int)user_pool.pool_bitmap.bits);
    put_str("  user_pool_phy_addr_start: ");
    put_int(user_pool.phy_addr_start);
    put_str("\n");

    /*set bitmap to 0*/
    bitmap_init(&kernel_pool.pool_bitmap);
    bitmap_init(&user_pool.pool_bitmap);

    /*initialize kernel virtual address's bitmap,because this is used to contrlo physic-kernel-heap,so, it's size is same to kernel memory pool.*/
    kernel_vaddr.vaddr_bitmap.btmp_bytes_len = kbm_length;
    kernel_vaddr.vaddr_bitmap.bits = (void*)(MEM_BITMAP_BASE + kbm_length + ubm_length);

    kernel_vaddr.vaddr_start = K_HEAP_START;
    bitmap_init(&kernel_vaddr.vaddr_bitmap);
    put_str("mem_pool_init done\n");
}

/*memory manage port's initial entry*/
void mem_init() {
    put_str("mem_init start\n");
    uint32_t mem_bytes_total = (*(uint32_t*)(0xb00));  //0xb00: total_mem_bytes's address ,this value's defination can see loader.S
    mem_pool_init(mem_bytes_total);  //initialize memory pool.
    put_str("mem_init done\n");
}
