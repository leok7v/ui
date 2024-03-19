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
    void (*vtraceline)(const char* file, int line, const char* func,
        const char* format, va_list vl);

    void (*traceline)(const char* file, int line, const char* func,
        const char* format, ...);

} trace_if;

#define traceln(...) trace.traceline(__FILE__, __LINE__, __func__, "" __VA_ARGS__)

begin_c

extern trace_if trace;

end_c
