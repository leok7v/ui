#pragma once
#include "ut/std.h"

begin_c

// better assert() - augmented with printf format and parameters
// swear() - release configuration assert() in honor of:
// https://github.com/munificent/vigil


#define static_assertion(condition) static_assert(condition, #condition)


typedef struct {
    int32_t (*failed_assertion)(const char* file, int32_t line,
        const char* func, const char* condition, const char* format, ...);
    int32_t (*fatal_termination)(const char* file, int32_t line,
        const char* func, const char* condition, const char* format, ...);
    void (*test)(void);
} vigil_if;

extern vigil_if vigil;

#ifdef _MSC_VER
    #define __suppress_constant_cond_exp__ _Pragma("warning(suppress: 4127)")
#else
    #define __suppress_constant_cond_exp__
#endif

#if defined(DEBUG)
  #define assert(b, ...) __suppress_constant_cond_exp__          \
    /* const cond */                                             \
    (void)((!!(b)) || vigil.failed_assertion(__FILE__, __LINE__, \
    __func__, #b, "" __VA_ARGS__))
#else
  #define assert(b, ...) ((void)0)
#endif

// swear() is both debug and release configuration assert

#define swear(b, ...) __suppress_constant_cond_exp__             \
    /* const cond */                                             \
    (void)((!!(b)) || vigil.failed_assertion(__FILE__, __LINE__, \
    __func__, #b, "" __VA_ARGS__))

#define fatal(...) (void)(vigil.fatal_termination(               \
    __FILE__, __LINE__,  __func__, "",  "" __VA_ARGS__))

#define fatal_if(b, ...) __suppress_constant_cond_exp__          \
    /* const cond */                                             \
    (void)((!(b)) || vigil.fatal_termination(__FILE__, __LINE__, \
    __func__, #b, "" __VA_ARGS__))

#define fatal_if_not(b, ...) __suppress_constant_cond_exp__       \
    /* const cond */                                              \
    (void)((!!(b)) || vigil.fatal_termination(__FILE__, __LINE__, \
    __func__, #b, "" __VA_ARGS__))

#define fatal_if_false fatal_if_not
#define fatal_if_not_zero(e, ...) fatal_if((e) != 0, "" __VA_ARGS__)
#define fatal_if_null(e, ...) fatal_if((e) == null, "" __VA_ARGS__)
#define not_null(e, ...) fatal_if_null(e, "" __VA_ARGS__)

end_c
