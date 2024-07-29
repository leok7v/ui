#pragma once
#include "rt/rt_std.h"

rt_begin_c

// debug interface essentially is:
// vfprintf(stderr, format, va)
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
} rt_verbosity_if;

typedef struct {
    rt_verbosity_if verbosity;
    int32_t (*verbosity_from_string)(const char* s);
    // "T" connector for outside write; return false to proceed with default
    bool (*tee)(const char* s, int32_t count); // return true to intercept
    void (*output)(const char* s, int32_t count);
    void (*println_va)(const char* file, int32_t line, const char* func,
        const char* format, va_list va);
    void (*println)(const char* file, int32_t line, const char* func,
        const char* format, ...);
    void (*perrno)(const char* file, int32_t line,
        const char* func, int32_t err_no, const char* format, ...);
    void (*perror)(const char* file, int32_t line,
        const char* func, int32_t error, const char* format, ...);
    bool (*is_debugger_present)(void);
    void (*breakpoint)(void); // no-op if debugger is not present
    errno_t (*raise)(uint32_t exception);
    struct  {
        uint32_t const access_violation;
        uint32_t const datatype_misalignment;
        uint32_t const breakpoint;
        uint32_t const single_step;
        uint32_t const array_bounds;
        uint32_t const float_denormal_operand;
        uint32_t const float_divide_by_zero;
        uint32_t const float_inexact_result;
        uint32_t const float_invalid_operation;
        uint32_t const float_overflow;
        uint32_t const float_stack_check;
        uint32_t const float_underflow;
        uint32_t const int_divide_by_zero;
        uint32_t const int_overflow;
        uint32_t const priv_instruction;
        uint32_t const in_page_error;
        uint32_t const illegal_instruction;
        uint32_t const noncontinuable;
        uint32_t const stack_overflow;
        uint32_t const invalid_disposition;
        uint32_t const guard_page;
        uint32_t const invalid_handle;
        uint32_t const possible_deadlock;
    } exception;
    void (*test)(void);
} rt_debug_if;

#define rt_println(...) rt_debug.println(__FILE__, __LINE__, __func__, "" __VA_ARGS__)

extern rt_debug_if rt_debug;

rt_end_c
