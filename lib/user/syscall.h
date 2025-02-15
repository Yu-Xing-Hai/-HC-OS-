#ifndef __LIB_SYSCALL_H
#define __LIB_SYSCALL_H
#include "stdint.h"

enum SYSCALL_NR {  //The structure is used to define the syscall's sub-number.
    SYS_GETPID,  //Default value is 0.
    SYS_WRITE   //Default value is 1.
};

uint32_t getpid(void);
uint32_t write(char* str);

#endif