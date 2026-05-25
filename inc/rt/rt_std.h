#pragma once
#include <ctype.h>
#include <errno.h>
#include <float.h>
#include <limits.h>
#include <locale.h>
#include <malloc.h>
#include <signal.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define rt_stringify(x) #x
#define rt_tostring(x) rt_stringify(x)
#define rt_pragma(x) _Pragma(rt_tostring(x))

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
#define rt_msvc_pragma(x)
#define rt_gcc_pragma(x) rt_pragma(x)
#else
#define rt_gcc_pragma(x)
#define rt_msvc_pragma(x) rt_pragma(x)
#endif

#ifdef _MSC_VER
    #define rt_suppress_constant_cond_exp _Pragma("warning(suppress: 4127)")
#else
    #define rt_suppress_constant_cond_exp
#endif

// Type aliases for floating-point types similar to <stdint.h>
typedef float  fp32_t;
typedef double fp64_t;
// "long fp64_t" is required by C standard but the bitness
// of it is not specified.

#ifdef __cplusplus
    #define rt_begin_c extern "C" {
    #define rt_end_c } // extern "C"
#else
    #define rt_begin_c // C headers compiled as C++
    #define rt_end_c
#endif

// rt_countof() and rt_countof() are suitable for
// small < 2^31 element arrays

#define rt_countof(a) ((int32_t)((int)(sizeof(a) / sizeof((a)[0]))))

#if defined(__GNUC__) || defined(__clang__)
    #define rt_force_inline __attribute__((always_inline))
#elif defined(_MSC_VER)
    #define rt_force_inline __forceinline
#endif

#ifndef __cplusplus
    #define null ((void*)0) // better than NULL which is zero
#else
    #define null nullptr
#endif

#if defined(_MSC_VER)
    #define rt_thread_local __declspec(thread)
#else
    #ifndef __cplusplus
        #define rt_thread_local _Thread_local // C99
    #else
        // C++ supports rt_thread_local keyword
    #endif
#endif

// rt_begin_packed rt_end_packed
// usage: typedef rt_begin_packed struct foo_s { ... } rt_end_packed foo_t;

#if defined(__GNUC__) || defined(__clang__)
#define rt_attribute_packed __attribute__((packed))
#define rt_begin_packed
#define rt_end_packed rt_attribute_packed
#else
#define rt_begin_packed rt_pragma( pack(push, 1) )
#define rt_end_packed rt_pragma( pack(pop) )
#define rt_attribute_packed
#endif

// usage: typedef struct rt_aligned_8 foo_s { ... } foo_t;

#if defined(__GNUC__) || defined(__clang__)
#define rt_aligned_8 __attribute__((aligned(8)))
#elif defined(_MSC_VER)
#define rt_aligned_8 __declspec(align(8))
#else
#define rt_aligned_8
#endif


// In callbacks the formal parameters are
// frequently unused. Also sometimes parameters
// are used in debug configuration only (e.g. rt_assert() checks)
// but not in release.
// C does not have anonymous parameters like C++
// Instead of:
//      void foo(param_type_t param) { (void)param; / *unused */ }
// use:
//      vod foo(param_type_t rt_unused(param)) { }

#if defined(__GNUC__) || defined(__clang__)
#define rt_unused(name) name __attribute__((unused))
#elif defined(_MSC_VER)
#define rt_unused(name) _Pragma("warning(suppress:  4100)") name
#else
#define rt_unused(name) name
#endif

// Because MS C compiler is unhappy about alloca() and
// does not implement (C99 optional) dynamic arrays on the stack:

#define rt_stackalloc(n) (_Pragma("warning(suppress: 6255 6263)") alloca(n))

// alloca() is messy and in general is a not a good idea.
// try to avoid if possible. Stack sizes vary from 64KB to 8MB in 2024.