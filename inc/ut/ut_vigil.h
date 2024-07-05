#pragma once
#include "ut/ut_std.h"
#include <assert.h>
// ^^^ ensures that <assert.h> will not be included and redefined again
#undef assert // because better assert(b, ...) is be defined below

begin_c

// better assert() - augmented with printf format and parameters
// swear() - release configuration assert() in honor of:
// https://github.com/munificent/vigil

#define ut_static_assertion(condition) static_assert(condition, #condition)

typedef struct {
    int32_t (*failed_assertion)(const char* file, int32_t line,
        const char* func, const char* condition, const char* format, ...);
    int32_t (*fatal_termination)(const char* file, int32_t line,
        const char* func, const char* condition, const char* format, ...);
    int32_t (*fatal_if_error)(const char* file, int32_t line, const char* func,
        const char* condition, errno_t r, const char* format, ...);
    void (*test)(void);
} ut_vigil_if;

extern ut_vigil_if ut_vigil;

#ifdef _MSC_VER
    #define ut_suppress_constant_cond_exp _Pragma("warning(suppress: 4127)")
#else
    #define ut_suppress_constant_cond_exp
#endif

#if defined(DEBUG)
  #define ut_assert(b, ...) ut_suppress_constant_cond_exp           \
    /* const cond */                                                \
    (void)((!!(b)) || ut_vigil.failed_assertion(__FILE__, __LINE__, \
    __func__, #b, "" __VA_ARGS__))
#else
  #define ut_assert(b, ...) ((void)0)
#endif

// Microsoft runtime shows Abort/Retry/Ignore message box
// on assert - really annoying
#define assert(b, ...) ut_assert(b, __VA_ARGS__)

// swear() is runtime assert() for both debug and release configurations

#define swear(b, ...) ut_suppress_constant_cond_exp                 \
    /* const cond */                                                \
    (void)((!!(b)) || ut_vigil.failed_assertion(__FILE__, __LINE__, \
    __func__, #b, "" __VA_ARGS__))

#define ut_fatal(...) (void)(ut_vigil.fatal_termination(            \
    __FILE__, __LINE__,  __func__, "",  "" __VA_ARGS__))

#define ut_fatal_if(b, ...) ut_suppress_constant_cond_exp           \
    /* const cond */                                                \
    (void)((!(b)) || ut_vigil.fatal_termination(__FILE__, __LINE__, \
    __func__, #b, "" __VA_ARGS__))

#define ut_fatal_if_not(b, ...) ut_suppress_constant_cond_exp        \
    /* const cond */                                                 \
    (void)((!!(b)) || ut_vigil.fatal_termination(__FILE__, __LINE__, \
    __func__, #b, "" __VA_ARGS__))

#define ut_not_null(e, ...) ut_fatal_if((e) == null, "" __VA_ARGS__)

#define ut_fatal_if_error(r, ...) ut_suppress_constant_cond_exp      \
    /* const cond */                                                 \
    (void)(ut_vigil.fatal_if_error(__FILE__, __LINE__, __func__,     \
                                   #r, r, "" __VA_ARGS__))

end_c
