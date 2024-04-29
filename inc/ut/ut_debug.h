#pragma once
#include "ut/ut_std.h"

begin_c

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
    int32_t const quiet;    // 0
    int32_t const info;     // 1 basic information (errors and warnings)
    int32_t const verbose;  // 2 detailed diagnostic
    int32_t const debug;    // 3 including debug messages
    int32_t const trace;    // 4 everything (may include nested calls)
} verbosity_if;

typedef struct {
    verbosity_if verbosity;
    int32_t (*verbosity_from_string)(const char* s);
    void (*println_va)(const char* file, int32_t line, const char* func,
        const char* format, va_list vl);
    void (*println)(const char* file, int32_t line, const char* func,
        const char* format, ...);
    void (*perrno)(const char* file, int32_t line,
        const char* func, int32_t err_no, const char* format, ...);
    void (*perror)(const char* file, int32_t line,
        const char* func, int32_t error, const char* format, ...);
    bool (*is_debugger_present)(void);
    void (*breakpoint)(void);
    void (*test)(void);
} ut_debug_if;

#define traceln(...) ut_debug.println(__FILE__, __LINE__, __func__, "" __VA_ARGS__)

extern ut_debug_if ut_debug;

end_c
