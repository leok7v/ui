#pragma once
// better assert() - augmented with printf format and parameters
// in honor of:
// https://github.com/munificent/vigil
#include <assert.h>
#undef assert

#include "manifest.h"

#define static_assertion(condition) static_assert(condition, #condition)

begin_c

typedef struct {
    int (*failed_assertion)(const char* file, int line,
        const char* func, const char* condition, const char* format, ...);
    int (*fatal_termination)(const char* file, int line,
        const char* func, const char* condition, const char* format, ...);
} vigil_if;

extern vigil_if vigil;

end_c

#if defined(DEBUG)
  #define assert(b, ...) _Pragma("warning(suppress: 4127)")      \
    /* const cond */                                             \
    (void)((!!(b)) || vigil.failed_assertion(__FILE__, __LINE__, \
    __func__, #b, "" __VA_ARGS__))
#else
  #define assert(b, ...) ((void)0)
#endif

// swear() is both debug and release configuration assert

#define swear(b, ...) _Pragma("warning(suppress: 4127)")         \
    /* const cond */                                             \
    (void)((!!(b)) || vigil.failed_assertion(__FILE__, __LINE__, \
    __func__, #b, "" __VA_ARGS__))

#define fatal(...) (void)(vigil.fatal_termination(__FILE__, __LINE__, \
    __func__, "",  "" __VA_ARGS__))

#define fatal_if(b, ...) _Pragma("warning(suppress: 4127)")      \
    /* const cond */                                             \
    (void)((!(b)) || vigil.fatal_termination(__FILE__, __LINE__, \
    __func__, #b, "" __VA_ARGS__))

#define fatal_if_not(b, ...) _Pragma("warning(suppress: 4127)")   \
    /* const cond */                                              \
    (void)((!!(b)) || vigil.fatal_termination(__FILE__, __LINE__, \
    __func__, #b, "" __VA_ARGS__))

#define fatal_if_false fatal_if_not
#define fatal_if_not_zero(e, ...) fatal_if((e) != 0, "" __VA_ARGS__)
#define fatal_if_null(e, ...) fatal_if((e) == null, "" __VA_ARGS__)
#define not_null(e, ...) fatal_if_null(e, "" __VA_ARGS__)
