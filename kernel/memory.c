#include "memory.h"
#include "stdint.h"
#include "print.h"
#include "debug.h"
#include "string.h"
#include "bitmap.h"

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

#define PDE_IDX(addr) ((addr & 0xffc00000) >> 22)  //get high 10 bit of 32 address
#define PTE_IDX(addr) ((addr & 0x003ff000) >> 12)  //get mid 10 bit of 32 address

struct pool kernel_pool, user_pool; //manage physic
struct virtual_addr kernel_vaddr;   //manage virtual

/*applicate virtual page in pf(pool flag) by quantity of pg_cnt,if successful ,return the start address of virtual page,of fail,return NULL*/
static void* vaddr_get(enum pool_flags pf, uint32_t pg_cnt) {
    int vaddr_start = 0, bit_idx_start = -1;  //bit_idx is the index in bitmap.
    uint32_t cnt = 0;
    if(pf == PF_KERNEL) {
        bit_idx_start = bitmap_scan(&kernel_vaddr.vaddr_bitmap, pg_cnt);  //pay attention: bitmap_scan()'s parameter is an bitmap structure's address,not an bitmap's address.
        if(bit_idx_start == -1) {
            return NULL;
        }
        while(cnt < pg_cnt) {
            bitmap_set(&kernel_vaddr.vaddr_bitmap, bit_idx_start + cnt++, 1); //indicate these pages have be used.
        }
        vaddr_start = kernel_vaddr.vaddr_start + bit_idx_start * PG_SIZE;
    }
    else {
        // User memory pool, we will add it when we do user processes.
    }
    return (void*)vaddr_start;
}

/*get the pte pointer of virtual address "vaddr"*/
/*remember!! pointer's address is vritual address*/
/*
remember!! pte_ptr() and pde_ptr() don't care whether pte and pde exist, they just caculate virtual address of pte and pde which are related with vaddr
******************************these two function are the key of edit page table.**************************
*/
uint32_t* pte_ptr(uint32_t vaddr) {
    /*0xffc00000: this is virtual address, and it is point to last PDE, last PDE point to PDT's physic address*/
    uint32_t* pte = (uint32_t*)(0xffc00000 + ((vaddr & 0xffc00000) >> 10) + PTE_IDX(vaddr) * 4);
    return pte;
}

/*get the pde pointer of virtual address "vaddr"*/
uint32_t* pde_ptr(uint32_t vaddr) {
    uint32_t* pde = (uint32_t*)((0xfffff000) + PDE_IDX(vaddr) * 4);
    return pde;
}
/******************************************************************************************* */

/*destribute one physic page from physic memory pool which is pointed by m_pool*/
/*if success, return physic address of page,if fail,return NULL.*/
static void* palloc(struct pool* m_pool) {
    int bit_idx = bitmap_scan(&m_pool->pool_bitmap, 1);
    if(bit_idx == -1) {
        return NULL;
    }
    bitmap_set(&m_pool->pool_bitmap, bit_idx, 1);
    uint32_t page_phyaddr = ((bit_idx * PG_SIZE) + m_pool->phy_addr_start);
    return (void*)page_phyaddr;
}

/*add mapping from _vaddr to _page_phyaddr in page table.*/
static void page_table_add(void* _vaddr, void* _page_phyaddr) {
    uint32_t vaddr = (uint32_t)_vaddr, page_phyaddr = (uint32_t)_page_phyaddr;
    uint32_t* pde = pde_ptr(vaddr);
    uint32_t* pte = pte_ptr(vaddr);

/**************************************8pay attention!!*******************************/
/*if we want execute *pte, we must make sure we have finished create pde,if we don's create pde and execute *pte, we will make page_fault*/
    if(*pde & 0x00000001) {  //the attribute of "P",it indicate whether this pte exist.
        //ASSERT(!(*pte & 0x00000001));  //if we want create pte,so,it's "P" must 0.
        if(!(*pte & 0x00000001)) {  //check more times to make sure safety.
            *pte = (page_phyaddr | PG_US_U | PG_RW_W | PG_P_1);  //structure of pte.
        }
        else {
            PANIC("pte repeat");
            *pte = (page_phyaddr | PG_US_U | PG_RW_W | PG_P_1);
        }
    }
    else { //the page space of PT all be dectributed from kernel space.
        uint32_t pde_phyaddr = (uint32_t)palloc(&kernel_pool);  //apply for a page space from kernel pool to create PT.
        *pde = (pde_phyaddr | PG_US_U | PG_RW_W | PG_P_1);
        memset((void*)((int)pte & 0xfffff000), 0, PG_SIZE);   //clean the page space of pde_phyaddr

        ASSERT(!(*pte & 0x00000001));
        *pte = (page_phyaddr | PG_US_U | PG_RW_W | PG_P_1);
    }
}

/*destribute page space and the quantities are pg_cnt.If success,return start virtual address,else return NULL.*/
void* malloc_page(enum pool_flags pf, uint32_t pg_cnt) {
    ASSERT(pg_cnt > 0 && pg_cnt < 3840);  //3840： kernel or user pyhsic space have about 16MB,we use 15MB to limit. pg_cnt < 15 * 1024 * 1024 / 4096(PG_SIZE) = 3840 (page)
/*************************the principle of malloc_page()***************************/
/*      First: use vaddr_get() to apply vaitual address from virtual memory pool.
        Second: use palloc() to apply physic page from physic memory pool.
        Thired: use page_table_add() to finish mapping from virtual address to physic address in PT.
*/
    void* vaddr_start = vaddr_get(pf, pg_cnt);
    if(vaddr_start == NULL) {
        return NULL;
    }

    uint32_t vaddr = (uint32_t)vaddr_start, cnt = pg_cnt;
    struct pool* mem_pool = pf & PF_KERNEL ? &kernel_pool : &user_pool;  //make sure to use which type of pool to destribute physic page,this pointer(mem_pool) point to pool structure.

    /*virtual address is continuous,but physic address could not be continuous,so we divide these three function from one cycle,
    and we can apply more virtual page onetime instead of applying virtual page one by one,
    if we failed to apply virtual page,we can decrease steps to apply physic page.*/
    while(cnt-- > 0) {
        void* page_phyaddr = palloc(mem_pool);
        if(page_phyaddr == NULL) {
            //if failed, we must rollback all address(virtual/physic page) which are applied, we will achieve it in the future.
            return NULL;
        }
        page_table_add((void*)vaddr, page_phyaddr);  //make mapping in PT.
        vaddr += PG_SIZE;    //next virtual page.
    }
    return vaddr_start;
}

/*apply one page memory from pyhsic kernel memory pool, if success, return virtual address,else,return NULL.*/
void* get_kernel_pages(uint32_t pg_cnt) {
    void* vaddr = malloc_page(PF_KERNEL, pg_cnt);
    if(vaddr != NULL) {
        memset(vaddr, 0, pg_cnt * PG_SIZE);  //clean this memory space which you have applied.
    }
    return vaddr;
}

/*initialize memory pool and ralated structures*/
static void mem_pool_init(uint32_t all_mem) {
    put_str("mem_pool_init start\n");
    uint32_t page_table_size = PG_SIZE * 256;  //used to record the Byte's quantity of (PDT add PT). The PDT's 769~1022(kernel space) totally have 254 PDE,these point to 254 page,0 and 769 point to same one page(this page point to low 1MB physic memory),1023 point to PDT,so,totally have 256 page,these page have be occupied.
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

    /*initialize kernel virtual address's bitmap,because this is used to control physic-kernel-heap,so, it's size is same to kernel memory pool.*/
    kernel_vaddr.vaddr_bitmap.btmp_bytes_len = kbm_length;
    kernel_vaddr.vaddr_bitmap.bits = (void*)(MEM_BITMAP_BASE + kbm_length + ubm_length);
    put_str("kernel_vaddr_bitmap_start: ");
    put_int((int)kernel_vaddr.vaddr_bitmap.bits);

    kernel_vaddr.vaddr_start = K_HEAP_START;
    put_str("  kernel_vaddr_start: ");
    put_int(K_HEAP_START);
    put_str("\n");
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
