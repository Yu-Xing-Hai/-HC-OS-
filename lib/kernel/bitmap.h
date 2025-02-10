#ifndef __LIB_KERNEL_BITMAP_H
#define __LIB_KERNEL_BITMAP_H
#include "global.h"
#include "stdbool.h"

#define BITMAP_MASK 1
struct bitmap {
    uint32_t btmp_bytes_len;
    /*bits's type is pointer,it means bits++, the pointer's address will add one byte.*/
    uint8_t* bits;  // other module create bitmap which type is uint8_t, then ,that module translate it's bitmap's head address to bits.
};

void bitmap_init(struct bitmap* btmp);
bool bitmap_scan_test(struct bitmap* btmp, uint32_t bit_idx);
int bitmap_scan(struct bitmap* btmp, uint32_t cnt);
void bitmap_set(struct bitmap* btmp, uint32_t bit_idx, uint8_t value);
#endif