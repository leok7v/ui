#pragma once
#include "ut/ut_std.h"

// "bt" stands for Stack Back Trace (not British Telecom)

ut_begin_c

enum { ut_bt_max_depth = 32 };    // increase if not enough
enum { ut_bt_max_symbol = 1024 }; // MSFT symbol size limit

typedef struct thread_s* ut_thread_t;

typedef char ut_bt_symbol_t[ut_bt_max_symbol];
typedef char ut_bt_file_t[512];

typedef struct ut_bt_s {
    int32_t frames; // 0 if capture() failed
    uint32_t hash;
    errno_t  error; // last error set by capture() or symbolize()
    void* stack[ut_bt_max_depth];
    ut_bt_symbol_t symbol[ut_bt_max_depth];
    ut_bt_file_t file[ut_bt_max_depth];
    int32_t line[ut_bt_max_depth];
    bool symbolized;
} ut_bt_t;

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
    void (*capture)(ut_bt_t *bt, int32_t skip); // number of frames to skip
    void (*context)(ut_thread_t thread, const void* context, ut_bt_t *bt);
    void (*symbolize)(ut_bt_t *bt);
    // dump backtrace into ut_println():
    void (*trace)(const ut_bt_t* bt, const char* stop);
    void (*trace_self)(const char* stop);
    void (*trace_all_but_self)(void); // trace all threads
    const char* (*string)(const ut_bt_t* bt, char* text, int32_t count);
    void (*test)(void);
} ut_bt_if;

extern ut_bt_if ut_bt;

#define ut_bt_here() do {   \
    ut_bt_t bt_ = {0};      \
    ut_bt.capture(&bt_, 0); \
    ut_bt.symbolize(&bt_);  \
    ut_bt.trace(&bt_, "*"); \
} while (0)

ut_end_c
