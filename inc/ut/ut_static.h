#pragma once
#include "ut/ut_std.h"

ut_begin_c

// ut_static_init(unique_name) { code_to_execute_before_main }

#if defined(_MSC_VER)

#if defined(_WIN64) || defined(_M_X64)
#define _msvc_symbol_prefix_ ""
#else
#define _msvc_symbol_prefix_ "_"
#endif

#pragma comment(linker, "/include:ut_force_symbol_reference")

void* ut_force_symbol_reference(void* symbol);

#define _msvc_ctor_(sym_prefix, func)                                    \
  void func(void);                                                        \
  int32_t (* ut_array ## func)(void);                                     \
  int32_t func ## _wrapper(void);                                         \
  int32_t func ## _wrapper(void) { func();                                \
  ut_force_symbol_reference((void*)ut_array ## func);                     \
  ut_force_symbol_reference((void*)func ## _wrapper); return 0; }         \
  extern int32_t (* ut_array ## func)(void);                              \
  __pragma(comment(linker, "/include:" sym_prefix # func "_wrapper"))     \
  __pragma(section(".CRT$XCU", read))                                     \
  __declspec(allocate(".CRT$XCU"))                                        \
    int32_t (* ut_array ## func)(void) = func ## _wrapper;

#define ut_static_init2_(func, line) _msvc_ctor_(_msvc_symbol_prefix_, \
    func ## _constructor_##line)                                       \
    void func ## _constructor_##line(void)

#define ut_static_init1_(func, line) ut_static_init2_(func, line)

#define ut_static_init(func) ut_static_init1_(func, __LINE__)

#else
#define ut_static_init(n) __attribute__((constructor)) \
        static void _init_ ## n ## __LINE__ ## _ctor(void)
#endif

void ut_static_init_test(void);

ut_end_c
