#ifndef __LIB_SYSCALL_H
#define __LIB_SYSCALL_H
#include "stdint.h"

enum SYSCALL_NR {  //The structure is used to define the syscall's sub-number.
    SYS_GETPID  //Default value is 0.
};

uint32_t getpid(void);

#endif