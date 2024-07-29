#pragma once
#include "rt/rt_std.h"

rt_begin_c

// rt_static_init(unique_name) { code_to_execute_before_main }

#if defined(_MSC_VER)

#if defined(_WIN64) || defined(_M_X64)
#define _msvc_symbol_prefix_ ""
#else
#define _msvc_symbol_prefix_ "_"
#endif

#pragma comment(linker, "/include:rt_force_symbol_reference")

void* rt_force_symbol_reference(void* symbol);

#define _msvc_ctor_(sym_prefix, func)                                    \
  void func(void);                                                        \
  int32_t (* rt_array ## func)(void);                                     \
  int32_t func ## _wrapper(void);                                         \
  int32_t func ## _wrapper(void) { func();                                \
  rt_force_symbol_reference((void*)rt_array ## func);                     \
  rt_force_symbol_reference((void*)func ## _wrapper); return 0; }         \
  extern int32_t (* rt_array ## func)(void);                              \
  __pragma(comment(linker, "/include:" sym_prefix # func "_wrapper"))     \
  __pragma(section(".CRT$XCU", read))                                     \
  __declspec(allocate(".CRT$XCU"))                                        \
    int32_t (* rt_array ## func)(void) = func ## _wrapper;

#define rt_static_init2_(func, line) _msvc_ctor_(_msvc_symbol_prefix_, \
    func ## _constructor_##line)                                       \
    void func ## _constructor_##line(void)

#define rt_static_init1_(func, line) rt_static_init2_(func, line)

#define rt_static_init(func) rt_static_init1_(func, __LINE__)

#else
#define rt_static_init(n) __attribute__((constructor)) \
        static void _init_ ## n ## __LINE__ ## _ctor(void)
#endif

void rt_static_init_test(void);

rt_end_c
