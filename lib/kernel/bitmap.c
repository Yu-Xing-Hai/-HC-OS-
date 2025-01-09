#include "bitmap.h"
#include "stdint.h"
#include "stdbool.h"
#include "print.h"
#include "interrupt.h"
#include "debug.h"

void bitmap_init(struct bitmap* btmp) {
    memset(btmp->bits, 0, btmp->btmp_bytes_len);
    /*bits is a pointer and is's type is int8_t,it means bits++, the pointer's address will add one byte.*/
}

/*judge the bit which index is bit_index in bitmap whether is 1 or 0*/
bool bitmap_scan_test(struct bitmap* btmp, uint32_t bit_idx) {  //pay attention to bit_idx and byte_idx.
    uint32_t byte_idx = bit_idx / 8;
    uint32_t bit_odd = bit_idx % 8;
    return (btmp->bits[byte_idx] & (uint8_t)(BITMAP_MASK << bit_odd));
}

/*used to find empty bit's address in bitmap,and desire to find the empty bit's quantities is cnt */
int bitmap_scan(struct bitmap* btmp, uint32_t cnt) {
    uint32_t idx_byte = 0; //used to record the byte where empty bit is in. 
    while((0xff == btmp->bits[idx_byte]) && (idx_byte < btmp->btmp_bytes_len)) {
        idx_byte++;
    }
    ASSERT(idx_byte < btmp->btmp_bytes_len);
    if(idx_byte == btmp->btmp_bytes_len) {
        return -1;  //can't find an empty space in memory pool.
    }
    int idx_bit  = 0;
    while((uint8_t)(BITMAP_MASK << idx_bit) & btmp->bits[idx_byte]) {
        idx_bit++;
    }
    int bit_idx_start = idx_byte * 8 + idx_bit;  //the index of empty bit in bitmap.
    if(cnt == 1) {
        return bit_idx_start;
    }
    uint32_t bit_left = (btmp->btmp_bytes_len * 8 - bit_idx_start);  //to record how many bits we can judge.
    uint32_t next_bit = bit_idx_start + 1;
    uint32_t count = 1;

    bit_idx_start = -1;  //set to -1 and if can not find continuous bit, then return it.
    while(bit_left-- > 0) {
        if(!(bitmap_scan_test(btmp, next_bit))) {
            count++;
        }
        else {
            count = 0;
        }
        if(count == cnt) {
            bit_idx_start = next_bit - cnt + 1;
            break;
        }
        next_bit++;
    }
    return bit_idx_start;
}

/*set the bit of bit_idx in btmp to value.*/
void bitmap_set(struct bitmap* btmp, uint32_t bit_idx, uint8_t value) {
    ASSERT((value == 0) || (value == 1));
    uint32_t byte_idx = bit_idx / 8;
    uint32_t bit_odd = bit_idx % 8;

    if(value) {
        btmp->bits[byte_idx] != (uint8_t)(BITMAP_MASK << bit_odd);
    }
    else {
        btmp->bits[byte_idx] &= ~(uint8_t)(BITMAP_MASK << bit_odd);
    }
}