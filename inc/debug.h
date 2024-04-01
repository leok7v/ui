#pragma once
#include "manifest.h"
#include <stdarg.h>

// trace interface essentially is:
// vfprintf(stderr, format, vl)
// fprintf(stderr, format, ...)
// with the additional convience:
// 1. writing to system log (e.g. OutputDebugString() on Windows)
// 2. always appending \n at the end of the line and thus flushing buffer
// Warning: on Windows it is not real-time and subject to 30ms delays
//          that may or may not happen on some calls

typedef struct {
    int32_t verbosity;   // global verbosity (interpretation vary)
    const int32_t info;  // 1
    const int32_t warn;  // 2
    const int32_t error; // 3
    const int32_t fatal; // 4
    const int32_t debug; // 5
    const int32_t trace; // 6
    void (*vprintf)(const char* file, int line, const char* func,
        const char* format, va_list vl);
    void (*printf)(const char* file, int line, const char* func,
        const char* format, ...);
    void (*perrno)(const char* file, int32_t line,
        const char* func, int32_t err_no, const char* format, ...);
    void (*perror)(const char* file, int32_t line,
        const char* func, int32_t error, const char* format, ...);
    void (*breakpoint)(void);
} debug_if;

#define traceln(...) debug.printf(__FILE__, __LINE__, __func__, "" __VA_ARGS__)

begin_c

extern debug_if debug;

end_c
