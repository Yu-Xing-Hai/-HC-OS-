#include "string.h"
#include "stdint.h"
#include "global.h"
#include "debug.h"

void memset(void* dst_, uint8_t value, uint32_t size) {
    ASSERT(dst_ != NULL);  //if dst_ is NULL, it will trigger error.
    uint8_t* dst = (uint8_t*)dst_;
    while(size-- > 0) {
        *dst++ = value;
    }
}

void memcpy(void* dst_, const void* src_, uint32_t size) {
    ASSERT(dst_ != NULL && src_ != NULL);
    uint8_t* dst = dst_;
    const uint8_t* src = src_;
    while(size-- > 0)
        *dst++ = *src++;
}

int memcmp(const void* a_, const void* b_, uint32_t size) {
    const char* a = a_;
    const char* b = b_;
    ASSERT(a != NULL && b != NULL);
    while(size-- > 0) {
        if(*a != *b) {
            return *a > *b ? 1 : -1;
        }
        a++;
        b++;
    }
    return 0;
}

char* strcpy(char* dst_, const char* src_) {
    ASSERT(dst_ != NULL && src_ != NULL);
    char* r = dst_;  //aim to return the destination string's start address.
    while((*dst_++ = *src_++));  //this function is same to memcpy,but this function use the flag of '0' which is the end of string.
    return r;
}

uint32_t strlen(const char* str) {
    ASSERT(str != NULL);
    const char* p = str;
    while(*p++);
    return (p - str - 1);  //use pointer's address to get string's len.
}

int8_t strcmp(const char* a, const char* b) {
    ASSERT(a != NULL && b != NULL);
    while(*a != 0 && *a == *b) {
        a++;
        b++;
    }
    return *a < *b ? -1 : *a > *b; //if *a == *b,return 0.
}

/*search the first address from left to right which is the character "ch" first times appear in "str" string.*/
char* strchr(const char* str, const uint8_t ch) {
    ASSERT(str != NULL);
    while(*str != 0) {
        if(*str == ch) {
            return (char*)str;
        }
        str++;
    }
    return NULL;
}

/*search the first address from right to left which is the character "ch" first times appear in "str" string.*/
char* strrchr(const char* str, const uint8_t ch) {
    ASSERT(str != NULL);
    const char* last_char = NULL;  //const: the pointer can be changed,but the vlaue pointed by pointer can't be changed.
    while(*str != 0) {
        if(*str == ch) {
            last_char = str;
        }
        str++;
    }
    return (char*)last_char;
}

/*splice src_ behind dst_,return new string's address*/
char* strcat(char* dst_, const char* src_) {
    ASSERT(dst_ != NULL && src_ != NULL);
    char* str = dst_;
    while(*str++); //when str point to '0',break.

    --str;  //str point to last char in string.
    while((*str++ = *src_++));
    return dst_;
}

/*Sum total quantitys the "ch" appeared in "str"*/
uint32_t strchrs(const char* str, uint8_t ch) {
    ASSERT(str != NULL);
    uint32_t ch_cnt = 0; //cnt: count
    const char* p = str;
    while(*p != 0) {
        if(*p == ch) {
            ch_cnt++;
        }
        p++;
    }
    return ch_cnt;
}