#pragma once
#include "ut/ut_std.h"

// "bt" stands for Stack Back Trace (not British Telecom)

rt_begin_c

enum { rt_backtrace_max_depth = 32 };    // increase if not enough
enum { rt_backtrace_max_symbol = 1024 }; // MSFT symbol size limit

typedef struct thread_s* rt_thread_t;

typedef char rt_backtrace_symbol_t[rt_backtrace_max_symbol];
typedef char rt_backtrace_file_t[512];

typedef struct rt_backtrace_s {
    int32_t frames; // 0 if capture() failed
    uint32_t hash;
    errno_t  error; // last error set by capture() or symbolize()
    void* stack[rt_backtrace_max_depth];
    rt_backtrace_symbol_t symbol[rt_backtrace_max_depth];
    rt_backtrace_file_t file[rt_backtrace_max_depth];
    int32_t line[rt_backtrace_max_depth];
    bool symbolized;
} rt_backtrace_t;

//  calling .trace(bt, /*stop:*/"*")
//  stops backtrace dumping at any of the known Microsoft runtime
//  symbols:
//    "main",
//    "WinMain",
//    "BaseThreadInitThunk",
//    "RtlUserThreadStart",
//    "mainCRTStartup",
//    "WinMainCRTStartup",
//    "invoke_main"
// .trace(bt, null)
// provides complete backtrace to the bottom of stack

typedef struct {
    void (*capture)(rt_backtrace_t *bt, int32_t skip); // number of frames to skip
    void (*context)(rt_thread_t thread, const void* context, rt_backtrace_t *bt);
    void (*symbolize)(rt_backtrace_t *bt);
    // dump backtrace into rt_println():
    void (*trace)(const rt_backtrace_t* bt, const char* stop);
    void (*trace_self)(const char* stop);
    void (*trace_all_but_self)(void); // trace all threads
    const char* (*string)(const rt_backtrace_t* bt, char* text, int32_t count);
    void (*test)(void);
} rt_backtrace_if;

extern rt_backtrace_if rt_backtrace;

#define rt_backtrace_here() do {   \
    rt_backtrace_t bt_ = {0};      \
    rt_backtrace.capture(&bt_, 0); \
    rt_backtrace.symbolize(&bt_);  \
    rt_backtrace.trace(&bt_, "*"); \
} while (0)

rt_end_c
