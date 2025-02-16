#ifndef __LIB_SYSCALL_H
#define __LIB_SYSCALL_H
#include "stdint.h"

uint32_t getpid(void);
uint32_t write(char* str);
void* malloc(uint32_t size);
void free(void* ptr);

#endif