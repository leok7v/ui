#pragma once

#ifdef __cplusplus
    #define begin_c extern "C" {
    #define end_c } // extern "C"
#else
    #define begin_c
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

// static_init(unique_name) { code_to_execute_before_main }

#if defined(_MSC_VER)

#if defined(_WIN64) || defined(_M_X64)
#define _msvc_symbol_prefix_ ""
#else
#define _msvc_symbol_prefix_ "_"
#endif
#ifdef __cplusplus
#define _msvc_extern_c_ extern "C"
#else
#define _msvc_extern_c_ extern
#endif

#pragma comment(linker, "/include:_force_symbol_reference_")

void* _force_symbol_reference_(void* symbol);

#define _msvc_ctor_(_sym_prefix, func)                                  \
  _msvc_extern_c_ void func(void);                                      \
  _msvc_extern_c_ int (* _array ## func)(void);                         \
  _msvc_extern_c_ int func ## _wrapper(void);                           \
  _msvc_extern_c_ int func ## _wrapper(void) { func();                  \
    _force_symbol_reference_((void*)_array ## func);                    \
    _force_symbol_reference_((void*)func ## _wrapper); return 0; }      \
  __pragma(comment(linker, "/include:" _sym_prefix # func "_wrapper"))  \
  __pragma(section(".CRT$XCU", read))                                   \
  __declspec(allocate(".CRT$XCU"))                                      \
    int (* _array ## func)(void) = func ## _wrapper;

#define _static_init2_(func, line) _msvc_ctor_(_msvc_symbol_prefix_, \
    func ## _constructor_##line)                                     \
    _msvc_extern_c_ void func ## _constructor_##line(void)

#define _static_init1_(func, line) _static_init2_(func, line)

#define static_init(func) _static_init1_(func, __LINE__)

#else
#define static_init(n) __attribute__((constructor)) static void _init_ ## n ##_ctor(void)
#endif

// va_first() va_rest() va_count() magic:
// https://stackoverflow.com/questions/5588855/standard-alternative-to-gccs-va-args-trick
// because
//     #define println(fmt, ...) printf(fmt "\n", ## __VA_ARGS__)
// 1. Is not a C99/C11/C17 standard (though supported by cl.exe)
// 2. Microsoft has it's own syntax
// 3. Intelisense chokes on both
// see: https://learn.microsoft.com/en-us/cpp/preprocessor/variadic-macros?view=msvc-170

#define va_first(...) __va_first_helper(__VA_ARGS__, throwaway)
#define __va_first_helper(first, ...) first
#define va_rest(...) __va_rest_helper(va_count(__VA_ARGS__), __VA_ARGS__)
#define __va_rest_helper(qty, ...) __va_rest_helper2(qty, __VA_ARGS__)
#define __va_rest_helper2(qty, ...) __va_rest_helper_##qty(__VA_ARGS__)
#define __va_rest_helper_one(first)
#define __va_rest_helper_twoormore(first, ...) , __VA_ARGS__
#define va_count(...) \
    __va_select_21th(__VA_ARGS__, \
                twoormore, twoormore, twoormore, twoormore, twoormore, \
                twoormore, twoormore, twoormore, twoormore, twoormore, \
                twoormore, twoormore, twoormore, twoormore, twoormore, \
                twoormore, twoormore, twoormore, twoormore, one, throwaway)
#define __va_select_21th(a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, \
    a11, a12, a13, a14, a15, a16, a17, a18, a19, a20, \
    a21, ...) a21

/*
va_first() va_rest() usage:

    #define println(...) printf(va_first(__VA_ARGS__) "\n----\n" va_rest(__VA_ARGS__))

    int main(int argc, const char* argv[]) {
        println("Hello");
        println("Hello %s", "World");
        println("2-args %d %d", 1, 2);
        println("3-args %d %d %d", 1, 2, 3);
        println("19-args %d %d %d %d %d %d %d %d %d %d  %d %d %d %d %d %d %d %d %d",
            1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19);
    }

produces:

    Hello
    ----
    Hello World
    ----
    2-args 1 2
    ----
    3-args 1 2 3
    ----
    19-args 1 2 3 4 5 6 7 8 9 10  11 12 13 14 15 16 17 18 19
    ----
*/
