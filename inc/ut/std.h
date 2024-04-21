#pragma once
// C runtime include files that are present on most of the platforms:
#include <assert.h>
#include <ctype.h>
#include <fcntl.h>
#include <io.h>
#include <malloc.h>
#define _USE_MATH_DEFINES
#include <math.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>

#undef assert // will be redefined

#ifdef __cplusplus
    #define begin_c extern "C" {
    #define end_c } // extern "C"
#else
    #define begin_c // C headers compiled as C++
    #define end_c
#endif

#ifndef countof
    #define countof(a) ((int)(sizeof(a) / sizeof((a)[0])))
#endif

#ifndef max // min/max is convoluted story use minimum/maximum
#define max(a,b)     (((a) > (b)) ? (a) : (b))
#endif
#define maximum(a,b) (((a) > (b)) ? (a) : (b)) // preferred

#ifndef min
#define min(a,b)     (((a) < (b)) ? (a) : (b))
#endif
#define minimum(a,b) (((a) < (b)) ? (a) : (b)) // preferred

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

typedef unsigned char byte; // legacy, deprecated: use uint8_t instead

#if defined(_MSC_VER)
    #define thread_local __declspec(thread)
#else
    #ifndef __cplusplus
        #define thread_local _Thread_local // C99
    #else
        // C++ supports thread_local keyword
    #endif
#endif

// begin_packed end_packed
// usage: typedef begin_packed struct foo_s { ... } end_packed foo_t;

#if defined(__GNUC__) || defined(__clang__)
#define attribute_packed __attribute__((packed))
#define begin_packed
#define end_packed attribute_packed
#else
#define begin_packed __pragma( pack(push, 1) )
#define end_packed __pragma( pack(pop) )
#define attribute_packed
#endif

// In callbacks on application level the formal parameters are
// frequently unused because the application global state is
// more convenient to work with. Also sometimes parameters
// are used in Debug build only (e.g. assert() checks) not in Release.
// To de-uglyfy
//      return_type_t foo(param_type_t param) { (void)param; / *unused */ }
// use this:
//      return_type_t foo(param_type_t unused(param)) { }

#define unused(name) _Pragma("warning(suppress:  4100)") name

