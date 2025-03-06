#ifndef __LIB_STDIO_H__
#define __LIB_STDIO_H__

#include "stdint.h"
#include "global.h"

#define va_start(ap, v) ap = (va_list)&v  //Initialize the pointer of ap, make ap point to the first unnamed arg "v".
#define va_arg(ap, t) *((t*)(ap += 4))  //return the value of the next argument, in 32bit system, the unit of stack is 4Byte.
#define va_end(ap) ap = NULL  //clean up ap.

typedef char* va_list;

uint32_t vsprintf(char* str, const char* format, va_list ap);
uint32_t printf(const char* format, ...);
uint32_t sprintf(char* buf, const char* format, ...);

#endif