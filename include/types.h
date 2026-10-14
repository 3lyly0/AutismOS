#ifndef TYPES_H
#define TYPES_H

#include <stdint.h>
#include <stddef.h>

typedef uint8_t  uint8;
typedef uint16_t uint16;
typedef uint32_t uint32;
typedef uint64_t uint64;
typedef int8_t   sint8;
typedef int16_t  sint16;
typedef int32_t  sint32;
typedef int64_t  sint64;

typedef uint8 byte;
typedef uint16 word;
typedef uint32 dword;

typedef enum {
    FALSE = 0,
    TRUE = 1
} BOOL;

#ifndef NULL
#define NULL ((void*)0)
#endif

#endif
