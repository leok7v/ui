#pragma once
#include <ctype.h>
#include <errno.h>
#include <locale.h>
#include <malloc.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define ut_stringify(x) #x
#define ut_tostring(x) ut_stringify(x)
#define ut_pragma(x) _Pragma(ut_tostring(x))

#if defined(__GNUC__) || defined(__clang__) // TODO: remove and fix code
#pragma GCC diagnostic ignored "-Wgnu-zero-variadic-macro-arguments"
#pragma GCC diagnostic ignored "-Wdeclaration-after-statement"
#pragma GCC diagnostic ignored "-Wfour-char-constants"
#pragma GCC diagnostic ignored "-Wmissing-field-initializers"
#pragma GCC diagnostic ignored "-Wunsafe-buffer-usage"
#pragma GCC diagnostic ignored "-Wunused-function"
#pragma GCC diagnostic ignored "-Wfloat-equal"
#pragma GCC diagnostic ignored "-Wmissing-noreturn"
#pragma GCC diagnostic ignored "-Wdouble-promotion"
#pragma GCC diagnostic ignored "-Wcast-align"
#pragma GCC diagnostic ignored "-Waddress-of-packed-member"
#pragma GCC diagnostic ignored "-Wused-but-marked-unused" // because in debug only
#define ut_msvc_pragma(x)
#define ut_gcc_pragma(x) ut_pragma(x)
#else
#define ut_gcc_pragma(x)
#define ut_msvc_pragma(x) ut_pragma(x)
#endif

// Type aliases for floating-point types similar to <stdint.h>
typedef float  fp32_t;
typedef double fp64_t;
// "long fp64_t" is required by C standard but the bitness
// of it is not specified.

#ifdef __cplusplus
    #define begin_c extern "C" {
    #define end_c } // extern "C"
#else
    #define begin_c // C headers compiled as C++
    #define end_c
#endif

// ut_countof() and ut_count_of() are suitable for
// small < 2^31 element arrays

// constexpr
#define ut_countof(a) ((int32_t)((int)(sizeof(a) / sizeof((a)[0]))))

// ut_count_of() generates "Integer division by zero" exception
// at runtime because ((void*)&(a) == &(a)[0]) does NOT evaluate
// to constant expression in cl.exe version 19.40.33811
#define ut_count_of(a) ((int32_t)(sizeof(a) / sizeof(a[1]) + \
                        (1 - 1 / ((void*)&(a) == &(a)[0]))))
// int a[5];
// int *b = a;
// printf("%d\n", ut_count_of(a));
// printf("%d\n", ut_count_of(b)); // "Integer division by zero"

#if defined(__GNUC__) || defined(__clang__)
    #define force_inline __attribute__((always_inline))
#elif defined(_MSC_VER)
    #define force_inline __forceinline
#endif

#ifndef __cplusplus
    #define null ((void*)0) // better than NULL which is zero
#else
    #define null nullptr
#endif

#if defined(_MSC_VER)
    #define thread_local __declspec(thread)
#else
    #ifndef __cplusplus
        #define thread_local _Thread_local // C99
    #else
        // C++ supports thread_local keyword
    #endif
#endif

// ut_begin_packed ut_end_packed
// usage: typedef ut_begin_packed struct foo_s { ... } ut_end_packed foo_t;

#if defined(__GNUC__) || defined(__clang__)
#define attribute_packed __attribute__((packed))
#define ut_begin_packed
#define ut_end_packed attribute_packed
#else
#define ut_begin_packed ut_pragma( pack(push, 1) )
#define ut_end_packed ut_pragma( pack(pop) )
#define attribute_packed
#endif

// usage: typedef struct ut_aligned_8 foo_s { ... } foo_t;

#if defined(__GNUC__) || defined(__clang__)
#define ut_aligned_8 __attribute__((aligned(8)))
#elif defined(_MSC_VER)
#define ut_aligned_8 __declspec(align(8))
#else
#define ut_aligned_8
#endif


// In callbacks the formal parameters are
// frequently unused. Also sometimes parameters
// are used in debug configuration only (e.g. assert() checks)
// but not in release.
// C does not have anonymous parameters like C++
// Instead of:
//      void foo(param_type_t param) { (void)param; / *unused */ }
// use:
//      vod foo(param_type_t unused(param)) { }

#if defined(__GNUC__) || defined(__clang__)
#define unused(name) name __attribute__((unused))
#elif defined(_MSC_VER)
#define unused(name) _Pragma("warning(suppress:  4100)") name
#else
#define unused(name) name
#endif

// Because MS C compiler is unhappy about alloca() and
// does not implement (C99 optional) dynamic arrays on the stack:

#define ut_stackalloc(n) (_Pragma("warning(suppress: 6255 6263)") alloca(n))

// alloca() is messy and in general is a not a good idea.
// try to avoid if possible. Stack sizes vary from 64KB to 8MB in 2024.