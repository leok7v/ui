#pragma once

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

#pragma comment(linker, "/include:_static_force_symbol_reference_")

void* _static_force_symbol_reference_(void* symbol);

#define _msvc_ctor_(_sym_prefix, func)                                    \
  _msvc_extern_c_ void func(void);                                        \
  _msvc_extern_c_ int32_t (* _array ## func)(void);                       \
  _msvc_extern_c_ int32_t func ## _wrapper(void);                         \
  _msvc_extern_c_ int32_t func ## _wrapper(void) { func();                \
    _static_force_symbol_reference_((void*)_array ## func);               \
    _static_force_symbol_reference_((void*)func ## _wrapper); return 0; } \
  __pragma(comment(linker, "/include:" _sym_prefix # func "_wrapper"))    \
  __pragma(section(".CRT$XCU", read))                                     \
  __declspec(allocate(".CRT$XCU"))                                        \
    int32_t (* _array ## func)(void) = func ## _wrapper;

#define _static_init2_(func, line) _msvc_ctor_(_msvc_symbol_prefix_, \
    func ## _constructor_##line)                                     \
    _msvc_extern_c_ void func ## _constructor_##line(void)

#define _static_init1_(func, line) _static_init2_(func, line)

#define static_init(func) _static_init1_(func, __LINE__)

#else
#define static_init(n) __attribute__((constructor)) \
        static void _init_ ## n ## __LINE__ ## _ctor(void)
#endif

void static_init_test(void);

