#pragma once
#include "manifest.h"
#include <stdarg.h>

// debug interface essentially is:
// vfprintf(stderr, format, vl)
// fprintf(stderr, format, ...)
// with the additional convience:
// 1. writing to system log (e.g. OutputDebugString() on Windows)
// 2. always appending \n at the end of the line and thus flushing buffer
// Warning: on Windows it is not real-time and subject to 30ms delays
//          that may or may not happen on some calls

typedef struct {
    int32_t level; // global verbosity (interpretation may vary)
    const int32_t quiet;    // 0
    const int32_t info;     // 1 basic information (errors and warnings)
    const int32_t verbose;  // 2 detailed diagnostic
    const int32_t debug;    // 3 including debug messages
    const int32_t trace;    // 4 everything (may include nested calls)
} verbosity_if;

typedef struct {
    verbosity_if verbosity;
    int32_t (*verbosity_from_string)(const char* s);
    void (*vprintf)(const char* file, int32_t line, const char* func,
        const char* format, va_list vl);
    void (*printf)(const char* file, int32_t line, const char* func,
        const char* format, ...);
    void (*perrno)(const char* file, int32_t line,
        const char* func, int32_t err_no, const char* format, ...);
    void (*perror)(const char* file, int32_t line,
        const char* func, int32_t error, const char* format, ...);
    bool (*is_debugger_present)(void);
    void (*breakpoint)(void);
    void (*test)(void);
} debug_if;

#define traceln(...) debug.printf(__FILE__, __LINE__, __func__, "" __VA_ARGS__)

begin_c

extern debug_if debug;

end_c
