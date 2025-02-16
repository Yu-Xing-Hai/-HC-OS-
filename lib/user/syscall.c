#include "syscall.h"
#include "syscall-init.h"
#include "stdint.h"

/*We will achieve the syscall's interface which will be used in User-process.*/

/*The syscall of No-parameters*/
//In Brace, the last value will become the return value of this Brace.
#define _syscall0(NUMBER) ({ \
    int retval; \
    asm volatile("int $0x80" : "=a"(retval) : "a"(NUMBER) : "memory"); \
    retval; \
})

/*The syscall of One-parameter*/
#define _syscall1(NUMBER, ARG1) ({ \
    int retval; \
    asm volatile("int $0x80" : "=a"(retval) : "a"(NUMBER), "b"(ARG1) : "memory"); \
    retval; \
})

/*The syscall of Two-parameters*/
#define _syscall2(NUMBER, ARG1, ARG2) ({ \
    int retval; \
    asm volatile("int $0x80" : "=a"(retval) : "a"(NUMBER), "b"(ARG1), "c"(ARG2) : "memory"); \
    retval; \
})

/*The syscall of Three-parameters*/
#define _syscall3(NUMBER, ARG1, ARG2, ARG3) ({ \
    int retval; \
    asm volatile("int $0x80" : "=a"(retval) : "a"(NUMBER), "b"(ARG1), "c"(ARG2), "d"(ARG3) : "memory"); \
    retval; \
})

/*Define the User's system-call-interface which will be used in User-process*/

/*
************************ Syscall Interface ****************
First: Return PID of current task
Second: Write the buffer to the console
*/
uint32_t getpid(void) {
    return _syscall0(SYS_GETPID);
}

/*Print the string of str.*/
uint32_t write(char* str) {  //When we access the file system, we will change it again.
    return _syscall1(SYS_WRITE, str);  //This function will return the length of the string.
}

/*Allocate the memory of size.*/
void* malloc(uint32_t size) {
    return (void*)_syscall1(SYS_MALLOC, size);
}

/*Free the memory which is pointed by ptr.*/
void free(void* ptr) {
    _syscall1(SYS_FREE, ptr);
}