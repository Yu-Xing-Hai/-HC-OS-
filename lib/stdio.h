#ifndef __LIB_STDIO_H__
#define __LIB_STDIO_H__

#include "stdint.h"

typedef char* va_list;

uint32_t printf(const char* format, ...);

#endif