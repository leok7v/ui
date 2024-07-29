#pragma once
#include "rt/rt_std.h"

rt_begin_c

// better rt_assert() - augmented with printf format and parameters
// rt_swear() - release configuration rt_assert() in honor of:
// https://github.com/munificent/vigil

#define rt_static_assertion(condition) static_assert(condition, #condition)

typedef struct {
    int32_t (*failed_assertion)(const char* file, int32_t line,
        const char* func, const char* condition, const char* format, ...);
    int32_t (*fatal_termination)(const char* file, int32_t line,
        const char* func, const char* condition, const char* format, ...);
    int32_t (*fatal_if_error)(const char* file, int32_t line, const char* func,
        const char* condition, errno_t r, const char* format, ...);
    void (*test)(void);
} rt_vigil_if;

extern rt_vigil_if rt_vigil;

#if defined(DEBUG)
  #define rt_assert(b, ...) rt_suppress_constant_cond_exp           \
    /* const cond */                                                \
    (void)((!!(b)) || rt_vigil.failed_assertion(__FILE__, __LINE__, \
    __func__, #b, "" __VA_ARGS__))
#else
  #define rt_assert(b, ...) ((void)0)
#endif

// rt_swear() is runtime rt_assert() for both debug and release configurations

#define rt_swear(b, ...) rt_suppress_constant_cond_exp                 \
    /* const cond */                                                \
    (void)((!!(b)) || rt_vigil.failed_assertion(__FILE__, __LINE__, \
    __func__, #b, "" __VA_ARGS__))

#define rt_fatal(...) (void)(rt_vigil.fatal_termination(            \
    __FILE__, __LINE__,  __func__, "",  "" __VA_ARGS__))

#define rt_fatal_if(b, ...) rt_suppress_constant_cond_exp           \
    /* const cond */                                                \
    (void)((!(b)) || rt_vigil.fatal_termination(__FILE__, __LINE__, \
    __func__, #b, "" __VA_ARGS__))

#define rt_fatal_if_not(b, ...) rt_suppress_constant_cond_exp        \
    /* const cond */                                                 \
    (void)((!!(b)) || rt_vigil.fatal_termination(__FILE__, __LINE__, \
    __func__, #b, "" __VA_ARGS__))

#define rt_not_null(e, ...) rt_fatal_if((e) == null, "" __VA_ARGS__)

#define rt_fatal_if_error(r, ...) rt_suppress_constant_cond_exp      \
    /* const cond */                                                 \
    (void)(rt_vigil.fatal_if_error(__FILE__, __LINE__, __func__,     \
                                   #r, r, "" __VA_ARGS__))

#define rt_fatal_win32err(c, ...) rt_suppress_constant_cond_exp      \
    /* const cond */                                                 \
    (void)(rt_vigil.fatal_if_error(__FILE__, __LINE__, __func__,     \
                                   #c, rt_b2e(c), "" __VA_ARGS__))

rt_end_c
