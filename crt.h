#ifndef crt_defintion
#define crt_defintion

#define crt_version 20221129 // YYYYMMDD

#pragma warning(error  : 4013) // ERROR: 'foo' undefined; assuming extern returning int
#pragma warning(disable: 4191) // 'type cast': unsafe conversion from 'FARPROC'
#pragma warning(disable: 4255) // no function prototype given: converting '()' to '(void)'
#pragma warning(disable: 4820) // 'n' bytes padding added after data member
#pragma warning(disable: 5045) // Compiler will insert Spectre mitigation for memory load
#pragma warning(disable: 4710) // function not inlined
#pragma warning(disable: 4711) // function selected for automatic inline expansion
#pragma warning(disable: 5039) // potentially throwing extern "C" function
#pragma warning(disable: 4668) // is not defined as a preprocessor macro, replacing with '0'
#pragma warning(disable: 4514) // unreferenced inline function has been removed
// local compiler version and github msbuild compiler differ. github falling behind:
#pragma warning(disable: 4619) // #pragma warning: there is no warning number ...

#ifdef __cplusplus
// [[fallthrough]] annotation is ignored by compiler (bug) as of 2022-11-11
#pragma warning(disable: 5262) // implicit fall-through occurs here; Use [[fallthrough]] when a break statement is intentionally omitted between cases
#pragma warning(disable: 5264) // 'const' variable is not used
#endif

#ifndef _CRT_SECURE_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS // shutup MSVC _s() function suggestions
#endif

#ifndef _CRT_NONSTDC_NO_WARNINGS
#define _CRT_NONSTDC_NO_WARNINGS
#endif

#ifndef _SCL_SECURE_NO_WARNINGS
#define _SCL_SECURE_NO_WARNINGS
#endif

#include <assert.h>
#undef assert

#if defined(_DEBUG) || defined(DEBUG)
#define _CRTDBG_MAP_ALLOC
#include <crtdbg.h> // _malloca()
#endif

#include <io.h>
#include <malloc.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef __cplusplus
#define begin_c extern "C" {
#define end_c } // extern "C"
#else
#define begin_c
#define end_c
#endif

#ifdef WIN32 // definded for all windows platforms
#define WINDOWS // less obscure name
#endif

#if defined(WIN32) && !defined(WINDOWS)
#define WINDOWS
#endif
#if defined(_DEBUG) && !defined(DEBUG)
#define DEBUG _DEBUG
#endif
#if defined(DEBUG) && !defined(_DEBUG)
#define _DEBUG DEBUG
#endif

begin_c

typedef unsigned char byte;

#ifndef __cplusplus
// C99: _Thread_local posix: __thread
#define thread_local __declspec(thread)
#define null NULL
#else
#define null nullptr
#endif

// see: https://docs.microsoft.com/en-us/cpp/cpp/inline-functions-cpp

#define force_inline __forceinline
#define inline_c __inline // inline is only in C++ __inline in both C and C++

#ifndef countof
#define countof(a) ((int)(sizeof(a) / sizeof((a)[0])))
#endif

static inline_c int indexof(const int a[], int n, int v) {
    for (int i = 0; i < n; i++) { if (a[i] == v) { return i; } }
    return -1;
}

#ifndef max
#define max(a,b)     (((a) > (b)) ? (a) : (b))
#endif
#define maximum(a,b) (((a) > (b)) ? (a) : (b)) // prefered

#ifndef min
#define min(a,b)     (((a) < (b)) ? (a) : (b))
#endif
#define minimum(a,b) (((a) < (b)) ? (a) : (b)) // prefered

#define static_assertion(condition) static_assert(condition, #condition)

#if defined(DEBUG)
  #define assert(b, ...)  _Pragma("warning(suppress: 4127)") /* const cond */ \
    (void)((!!(b)) || crt.assertion_failed(__FILE__, __LINE__, __func__, #b, \
    "" __VA_ARGS__))
#else
  #define assert(b, ...) (void)(0)
#endif

#define traceln(...) crt.traceline(__FILE__, __LINE__, __func__, \
    "" __VA_ARGS__)

// https://stackoverflow.com/questions/35087781/using-line-in-a-macro-definition

#define fatal_if_at_line(condition, line, call, err2str, err, ...) \
    do { \
        bool _b_##line = (condition); \
        if (_b_##line) { \
            char va[256]; \
            crt.sformat(va, sizeof(va), "" __VA_ARGS__); \
            crt.fatal(__FILE__, line, __func__, err2str, (int32_t)err, call, va); \
        } \
    } while (0)

#define fatal_if_(condition, line, call, err2str, err, ...) \
    fatal_if_at_line(condition, line, call, err2str, err, __VA_ARGS__)

#define fatal_if(call, ...) \
    fatal_if_(call, __LINE__, #call, crt.error, crt.err(), __VA_ARGS__)

#define fatal_if_false(call, ...) \
    fatal_if_(!(call), __LINE__, #call, crt.error, crt.err(), __VA_ARGS__)

#define fatal_if_null(call, ...) \
    fatal_if_((call) == null, __LINE__, #call, crt.error, crt.err(), __VA_ARGS__)

#define fatal_if_not_zero2_(call, line, ...) \
    do { \
        int32_t _r_##line = (call); \
        fatal_if_(_r_##line != 0, line, #call, crt.error, _r_##line, __VA_ARGS__); \
    } while (0)

#define fatal_if_not_zero1_(call, line, ...) \
        fatal_if_not_zero2_(call, line, __VA_ARGS__)

#define fatal_if_not_zero(call, ...) \
        fatal_if_not_zero1_(call, __LINE__, ## __VA_ARGS__)

// In callbacks on application level the formal parameters are
// frequently unused because the application global state is
// more convenient to work with. Also sometimes parameters
// are used in Debug build only (e.g. assert() checks) not in Release.
// To de-uglyfy
//      return_type_t foo(param_type_t param) { (void)param; / *unused */ }
// use this:
//      return_type_t foo(param_type_t unused(param)) { }

#define unused(name) _Pragma("warning(suppress:  4100)") name

// not_null() is a solid fail fast form of null check that works
// for both Debug and Release builds and guarantee to crash
// the code if a value null is not expected.
// Usage includes preconditions in the code as well as post-conditions
//
// return_type_t* foo(input_param_type_t* input, output_param_type_t* output) {
//     // precondition:
//     not_null(input, "foo() does not accept null input parameter");
//     // output parameter storage must be specified:
//     not_null(output, "foo() always expects not-null output pointer");
//
//     // alternatively the function may allow output pointer to be null if
//     // output parameter is optional but this is not the case in this sample
//
//     *output = ...
//     // post-condition:
//     not_null(*output, "foo() guarantees *output is never null");
//
//     return_type_t r = ...
//     // post-condition:
//     not_null(r, "foo() guarantees not to returns null ever");
//     return r;
// }

#define not_null(p, ...) fatal_if_null((p), __VA_ARGS__)

// Frequent Code Analysis false positive warnings
// Mainly because for stackalloc() and casted void* pointers
// compiler cannot infere the size of accessible memory.

#define __suppress_alloca_warnings__ _Pragma("warning(suppress: 6255 6263)")
#define __suppress_buffer_overrun__ _Pragma("warning(suppress: 6386)")
#define __suppress_reading_invalid_data__ _Pragma("warning(suppress: 6385)")

#define stackalloc(bytes) (__suppress_alloca_warnings__ alloca(bytes))
#define zero_initialized_stackalloc(bytes) memset(stackalloc(bytes), 0, (bytes))

#define strempty(s) ((s) == null || (s)[0] == 0)

#define strconcat(a, b) __suppress_buffer_overrun__ \
    (strcat(strcpy((char*)stackalloc(strlen(a) + strlen(b) + 1), (a)), (b)))

#define strequ(s1, s2)  (((void*)(s1) == (void*)(s2)) || \
    (((void*)(s1) != null && (void*)(s2) != null) && strcmp((s1), (s2)) == 0))

#define striequ(s1, s2)  (((void*)(s1) == (void*)(s2)) || \
    (((void*)(s1) != null && (void*)(s2) != null) && stricmp((s1), (s2)) == 0))

#define strendswith(s1, s2) (strlen(s2) <= strlen(s1) && strequ(s1 + strlen(s2), s2))
// strends(s1, s2) (strstr(s1, s2) == s1 + strlen(s1) - strlen(s2))
// won't always work strends("ab", "b") unless compiler coalesce
// two copies "ab" in one

#define strlength(s) ((int)strlen(s)) // avoitd code analysis noise
// a lot of posix like API consumes "int" instead of size_t which
// is acceptable for majority of char* zero terminated strings usage
// since most of them work with filepath that are relatively short
// and on Windows are limited to 260 chars or 32KB - 1 chars.

#define strcopy(s1, s2) /* use with exteme caution */                      \
    do {                                                                   \
        strncpy((s1), (s2), countof((s1)) - 1); s1[countof((s1)) - 1] = 0; \
} while (0)

const char* _strtolc_(char* d, const char* s);

#define strtolowercase(s) _strtolc_((char*)stackalloc(strlen(s) + 1), s)

#define utf16to8(utf16) crt.utf16to8((char*) \
    stackalloc((size_t)crt.utf8_bytes(utf16) + 1), utf16)

#define utf8to16(s) crt.utf8to16((wchar_t*)stackalloc((crt.utf16_bytes(s) + 1) * \
    sizeof(wchar_t)), s)

#define strprintf(s, ...) crt.sformat((s), countof(s), "" __VA_ARGS__)

enum {
    NSEC_IN_USEC = 1000,
    NSEC_IN_MSEC = NSEC_IN_USEC * 1000,
    NSEC_IN_SEC  = NSEC_IN_MSEC * 1000
};

typedef struct {
    int32_t (*err)(); // errno or GetLastError()
    void (*seterr)(int32_t err); // errno = err or SetLastError()
    // non-crypro strong pseudo-random number generators (thread safe)
    uint32_t (*random32)(uint32_t *state); // "Mulberry32"
    uint64_t (*random64)(uint64_t *state); // "Trust"
    int (*memmap_read)(const char* filename, void** data, int64_t* bytes);
    int (*memmap_rw)(const char* filename, void** data, int64_t* bytes);
    void (*memunmap)(void* data, int64_t bytes);
    // memmap_res() maps data from resources, do NOT unmap!
    int (*memmap_res)(const char* label, void** data, int64_t* bytes);
    void (*sleep)(double seconds);
    double (*seconds)(); // since boot
    int64_t (*nanoseconds)(); // since boot
    uint64_t (*microseconds)(); // NOT monotonic(!) UTC since epoch January 1, 1601
    uint64_t (*localtime)();    // local time microseconds since epoch
    void (*time_utc)(uint64_t microseconds, int* year, int* month,
        int* day, int* hh, int* mm, int* ss, int* ms, int* mc);
    void (*time_local)(uint64_t microseconds, int* year, int* month,
        int* day, int* hh, int* mm, int* ss, int* ms, int* mc);
    void (*vformat)(char* utf8, int count, const char* format, va_list vl);
    void (*sformat)(char* utf8, int count, const char* format, ...);
    const char* (*error)(int32_t error); // en-US
    const char* (*error_nls)(int32_t error); // national locale string
    // do not call directly used by macros above
    int (*utf8_bytes)(const wchar_t* utf16);
    int (*utf16_bytes)(const char* s);
    char* (*utf16to8)(char* s, const wchar_t* utf16);
    wchar_t* (*utf8to16)(wchar_t* utf16, const char* s);
    void (*ods)(const char* file, int line, const char* function,
        const char* format, ...); // Output Debug String
    void (*traceline)(const char* file, int line, const char* function,
        const char* format, ...);
    void (*vtraceline)(const char* file, int line, const char* function,
        const char* format, va_list vl);
    void (*breakpoint)();
    int (*gettid)();
    int (*assertion_failed)(const char* file, int line, const char* function,
                         const char* condition, const char* format, ...);
    void (*fatal)(const char* file, int line, const char* function,
        const char* (*err2str)(int32_t error), int32_t error,
        const char* call, const char* extra);
} crt_if;

extern crt_if crt;

typedef void* thread_t;

typedef struct {
    thread_t (*start)(void (*func)(void*), void* p); // never returns null
    bool (*try_join)(thread_t thread, double timeout); // seconds
    void (*join)(thread_t thread);
    void (*name)(const char* name); // names the thread
    void (*realtime)(); // bumps calling thread priority
} threads_if;

extern threads_if threads;

typedef void* event_t;

typedef struct {
    event_t (*create)(); // never returns null
    event_t (*create_manual)(); // never returns null
    void (*set)(event_t e);
    void (*reset)(event_t e);
    void (*wait)(event_t e);
    // returns 0 or -1 on timeout
    int (*wait_or_timeout)(event_t e, double seconds); // seconds < 0 forever
    // returns event index or -1 on timeout or abandon
    int (*wait_any)(int n, event_t events[]); // -1 on abandon
    int (*wait_any_or_timeout)(int n, event_t e[], double milliseconds);
    void (*dispose)(event_t e);
} events_if;

extern events_if events;

typedef struct { byte content[40]; } mutex_t;

typedef struct {
    void (*init)(mutex_t* m);
    void (*lock)(mutex_t* m);
    void (*unlock)(mutex_t* m);
    void (*dispose)(mutex_t* m);
} mutex_if;

extern mutex_if mutexes;

typedef struct {
    void* (*exchange_ptr)(volatile void** a, void* v);
    int32_t (*increment_int32)(volatile int32_t* a); // returns incremented
    int32_t (*decrement_int32)(volatile int32_t* a); // returns decremented
    int64_t (*increment_int64)(volatile int64_t* a); // returns incremented
    int64_t (*decrement_int64)(volatile int64_t* a); // returns decremented
    int64_t (*add_int64)(volatile int64_t* a, int64_t v); // returns result of add
    // returns the value held previously by "a" address:
    int32_t (*exchange_int32)(volatile int32_t* a, int32_t v);
    int64_t (*exchange_int64)(volatile int64_t* a, int64_t v);
    // compare_exchange functions compare the *a value with the comparand value.
    // If the *a is equal to the comparand value, the "v" value is stored in the address
    // specified by "a" otherwise, no operation is performed.
    // returns true is previous value if *a was the same as "comarand"
    bool (*compare_exchange_int64)(volatile int64_t* a, int64_t comparand, int64_t v);
    bool (*compare_exchange_int32)(volatile int32_t* a, int32_t comparand, int32_t v);
    void (*spinlock_acquire)(volatile int64_t* spinlock);
    void (*spinlock_release)(volatile int64_t* spinlock);
    void (*memory_fence)();
} atomics_if;

extern atomics_if atomics;

typedef struct {
    int (*option_index)(int argc, const char* argv[], const char* option);
    int (*remove_at)(int ix, int argc, const char* argv[]);
    /* argc=3 argv={"foo", "--verbose"} -> returns true; argc=1 argv={"foo"} */
    bool (*option_bool)(int *argc, const char* argv[], const char* option);
    /* argc=3 argv={"foo", "--n", "153"} -> value==153, true; argc=1 argv={"foo"}
       also handles negative values (e.g. "-153") and hex (e.g. 0xBADF00D)
    */
    bool (*option_int)(int *argc, const char* argv[], const char* option, int64_t *value);
    /* argc=3 argv={"foo", "--path", "bar"} -> returns "bar" argc=1 argv={"foo"} */
    const char* (*option_str)(int *argc, const char* argv[], const char* option);
    int (*parse)(const char* cl, const char** argv, char* buff);
} args_if;

extern args_if args;

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

#if defined(__GNUC__) || defined(__clang__)
#define attribute_packed __attribute__((packed))
#define begin_packed
#define end_packed attribute_packed
#else
#define begin_packed __pragma( pack(push, 1) )
#define end_packed __pragma( pack(pop) )
#define attribute_packed
#endif

#ifdef _MSC_VER

#if defined(_WIN64) || defined(_M_X64)
#define msvc_symbol_prefix ""
#else
#define msvc_symbol_prefix "_"
#endif
#ifdef __cplusplus
#define msvc_extern_c extern "C"
#else
#define msvc_extern_c extern
#endif

void* _force_symbol_reference_(void* symbol);

#define msvc_ctor(_sym_prefix, func) \
  msvc_extern_c void func(void); \
  msvc_extern_c int (* _array ## func)(void); \
  msvc_extern_c int func ## _wrapper(void); \
  msvc_extern_c int func ## _wrapper(void) { func(); \
    _force_symbol_reference_((void*)_array ## func); \
    _force_symbol_reference_((void*)func ## _wrapper); return 0; } \
  __pragma(comment(linker, "/include:" _sym_prefix # func "_wrapper")) \
  __pragma(section(".CRT$XCU", read)) \
  __declspec(allocate(".CRT$XCU")) \
    int (* _array ## func)(void) = func ## _wrapper;

#define _static_init2_(func, line) msvc_ctor(msvc_symbol_prefix, func ## _constructor_##line) \
    msvc_extern_c void func ## _constructor_##line(void)

#define _static_init1_(func, line) _static_init2_(func, line)

#define static_init(func) _static_init1_(func, __LINE__)

#else
#define static_init(n) __attribute__((constructor)) static void _init_ ## n ##_ctor(void)
#endif

/*
    same as in Java:
        static { System.out.println("called before main()\n"); }
    sample usage: order is not guaranteed
        static void after_main1(void) { printf("called after main() 1\n"); }
        static void after_main2(void) { printf("called after main() 2\n"); }
        static_init(main_init1)       { printf("called before main 1\n"); atexit(after_main1); }
        static_init(main_init2)       { printf("called before main 2\n"); atexit(after_main2); }
        int main() { printf("main()\n"); }
*/

end_c

#endif crt_defintion

#ifdef crt_implementation
#undef crt_implementation

#if !defined(STRICT)
#define STRICT
#endif

#if !defined(WIN32_LEAN_AND_MEAN)
#define WIN32_LEAN_AND_MEAN
#endif

#if !defined(WIN32_LEAN_AND_MEAN)
#define WIN32_LEAN_AND_MEAN
#endif

#include <Windows.h>
#include <WindowsX.h>
#include <timeapi.h>
#include <sysinfoapi.h>
#include <winternl.h>

#include <intrin.h>
#include <immintrin.h>
#include <ammintrin.h>

#include <io.h>
#include <errno.h>
#include <fcntl.h>
#include <float.h>
#ifndef _USE_MATH_DEFINES
#define _USE_MATH_DEFINES
#endif
#include <math.h>
#include <process.h>
#include <sys/stat.h>
#ifndef _CRT_SILENCE_NONCONFORMING_TGMATH_H
#define _CRT_SILENCE_NONCONFORMING_TGMATH_H
#endif
#include <tgmath.h>

begin_c

const char* _strtolc_(char* d, const char* s) {
    char* r = d;
    while (*s != 0) { *d++ = (char)tolower(*s++); }
    return r;
}

static void* symbol_reference[1024];
static int symbol_reference_count;

void* _force_symbol_reference_(void* symbol) {
    assert(symbol_reference_count <= countof(symbol_reference),
        "increase size of symbol_reference[%d] to at least %d",
        countof(symbol_reference), symbol_reference);
    if (symbol_reference_count < countof(symbol_reference)) {
        symbol_reference[symbol_reference_count] = symbol;
//      traceln("symbol_reference[%d] = %p", symbol_reference_count, symbol_reference[symbol_reference_count]);
        symbol_reference_count++;
    }
    return symbol;
}

static int args_option_index(int argc, const char* argv[], const char* option) {
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--") == 0) { break; } // no options after '--'
        if (strcmp(argv[i], option) == 0) { return i; }
    }
    return -1;
}

static int args_remove_at(int ix, int argc, const char* argv[]) { // returns new argc
    assert(0 < argc);
    assert(0 < ix && ix < argc); // cannot remove argv[0]
    for (int i = ix; i < argc; i++) {
        argv[i] = argv[i+1];
    }
    argv[argc - 1] = "";
    return argc - 1;
}

static bool args_option_bool(int *argc, const char* argv[], const char* option) {
    int ix = args_option_index(*argc, argv, option);
    if (ix > 0) {
        *argc = args_remove_at(ix, *argc, argv);
    }
    return ix > 0;
}

static bool args_option_int(int *argc, const char* argv[], const char* option, int64_t *value) {
    int ix = args_option_index(*argc, argv, option);
    if (ix > 0 && ix < *argc - 1) {
        const char* s = argv[ix + 1];
        int base = (strstr(s, "0x") == s || strstr(s, "0X") == s) ? 16 : 10;
        const char* b = s + (base == 10 ? 0 : 2);
        char* e = null;
        errno = 0;
        int64_t v = strtoll(b, &e, base);
        if (errno == 0 && e > b && *e == 0) {
            *value = v;
        } else {
            ix = -1;
        }
    } else {
        ix = -1;
    }
    if (ix > 0) {
        *argc = args_remove_at(ix, *argc, argv); // remove option
        *argc = args_remove_at(ix, *argc, argv); // remove following number
    }
    return ix > 0;
}

static const char* args_option_str(int *argc, const char* argv[], const char* option) {
    int ix = args_option_index(*argc, argv, option);
    const char* s = null;
    if (ix > 0 && ix < *argc - 1) {
        s = argv[ix + 1];
    } else {
        ix = -1;
    }
    if (ix > 0) {
        *argc = args_remove_at(ix, *argc, argv); // remove option
        *argc = args_remove_at(ix, *argc, argv); // remove following string
    }
    return ix > 0 ? s : null;
}

static const char BACKSLASH = '\\';
static const char QUOTE = '\"';

static char next_char(const char** cl, int* escaped) {
    char ch = **cl;
    (*cl)++;
    *escaped = false;
    if (ch == BACKSLASH) {
        if (**cl == BACKSLASH) {
            (*cl)++;
            *escaped = true;
        } else if (**cl == QUOTE) {
            ch = QUOTE;
            (*cl)++;
            *escaped = true;
        } else { /* keep the backslash and copy it into the resulting argument */ }
    }
    return ch;
}

static int args_parse(const char* cl, const char** argv, char* buff) {
    int escaped = 0;
    int argc = 0;
    int j = 0;
    char ch = next_char(&cl, &escaped);
    while (ch != 0) {
        while (isspace(ch)) { ch = next_char(&cl, &escaped); }
        if (ch == 0) { break; }
        argv[argc++] = buff + j;
        if (ch == QUOTE) {
            ch = next_char(&cl, &escaped);
            while (ch != 0) {
                if (ch == QUOTE && !escaped) { break; }
                buff[j++] = ch;
                ch = next_char(&cl, &escaped);
            }
            buff[j++] = 0;
            if (ch == 0) { break; }
            ch = next_char(&cl, &escaped); // skip closing quote maerk
        } else {
            while (ch != 0 && !isspace(ch)) {
                buff[j++] = ch;
                ch = next_char(&cl, &escaped);
            }
            buff[j++] = 0;
        }
    }
    return argc;
}

args_if args = {
    args_option_index,
    args_remove_at,
    args_option_bool,
    args_option_int,
    args_option_str,
    args_parse
};

static void crt_vformat(char* utf8, int count, const char* format, va_list vl) {
    vsnprintf(utf8, count, format, vl);
    utf8[count - 1] = 0;
}

static void crt_sformat(char* utf8, int count, const char* format, ...) {
    va_list vl;
    va_start(vl, format);
    crt.vformat(utf8, count, format, vl);
    va_end(vl);
}

static const char* crt_error_for_language(int32_t error, LANGID language) {
    static thread_local char text[256];
    const DWORD format = FORMAT_MESSAGE_FROM_SYSTEM|
        FORMAT_MESSAGE_IGNORE_INSERTS;
    wchar_t s[256];
    HRESULT hr = 0 <= error && error <= 0xFFFF ?
        HRESULT_FROM_WIN32(error) : error;
    if (FormatMessageW(format, null, hr, language, s, countof(s) - 1,
            (va_list*)null) > 0) {
        s[countof(s) - 1] = 0;
        // remove trailing '\r\n'
        int k = (int)wcslen(s);
        if (k > 0 && s[k - 1] == '\n') { s[k - 1] = 0; }
        k = (int)wcslen(s);
        if (k > 0 && s[k - 1] == '\r') { s[k - 1] = 0; }
        strprintf(text, "0x%08X(%d) \"%s\"", error, error, utf16to8(s));
    } else {
        strprintf(text, "0x%08X(%d)", error, error);
    }
    return text;
}


static const char* crt_error(int32_t error) {
    const LANGID lang = MAKELANGID(LANG_ENGLISH, SUBLANG_DEFAULT);
    return crt_error_for_language(error, lang);
}

static const char* crt_error_nls(int32_t error) {
    const LANGID lang = MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT);
    return crt_error_for_language(error, lang);
}

static void crt_sleep(double seconds) {
    assert(seconds >= 0);
    if (seconds < 0) { seconds = 0; }
    int64_t ns100 = (int64_t)(seconds * 1.0e+7); // in 0.1 us aka 100ns
    typedef int (__stdcall *nt_delay_execution_t)(BOOLEAN Alertable,
        PLARGE_INTEGER DelayInterval);
    static nt_delay_execution_t NtDelayExecution;
    // delay in 100-ns units. negative value means delay relative to current.
    LARGE_INTEGER delay = {0}; // delay in 100-ns units.
    delay.QuadPart = -ns100; // negative value means delay relative to current.
    if (NtDelayExecution == null) {
        HMODULE ntdll = LoadLibraryA("ntdll.dll");
        not_null(ntdll);
        NtDelayExecution = (nt_delay_execution_t)GetProcAddress(ntdll,
            "NtDelayExecution");
        not_null(NtDelayExecution);
    }
    //  If "alertable" is set, execution can break in a result of NtAlertThread call.
    NtDelayExecution(false, &delay);
}

static uint64_t crt_microseconds_since_epoch() { // NOT monotonic
    FILETIME ft; // time in 100ns interval (tenth of microsecond)
    // since 12:00 A.M. January 1, 1601 Coordinated Universal Time (UTC)
    GetSystemTimePreciseAsFileTime(&ft);
    uint64_t microseconds =
        (((uint64_t)ft.dwHighDateTime) << 32 | ft.dwLowDateTime) / 10;
    assert(microseconds > 0);
    return microseconds;
}

static uint64_t crt_localtime() {
    TIME_ZONE_INFORMATION tzi; // UTC = local time + bias
    GetTimeZoneInformation(&tzi);
    uint64_t bias = (uint64_t)tzi.Bias * 60LL * 1000 * 1000; // in microseconds
    return crt_microseconds_since_epoch() - bias;
}

static void crt_time_utc(uint64_t microseconds, int* year, int* month,
        int* day, int* hh, int* mm, int* ss, int* ms, int* mc) {
    uint64_t time_in_100ns = microseconds * 10;
    FILETIME mst = { (DWORD)(time_in_100ns & 0xFFFFFFFF), (DWORD)(time_in_100ns >> 32) };
    SYSTEMTIME utc;
    FileTimeToSystemTime(&mst, &utc);
    *year = utc.wYear;
    *month = utc.wMonth;
    *day = utc.wDay;
    *hh = utc.wHour;
    *mm = utc.wMinute;
    *ss = utc.wSecond;
    *ms = utc.wMilliseconds;
    *mc = microseconds % 1000;
}

static void crt_time_local(uint64_t microseconds, int* year, int* month,
        int* day, int* hh, int* mm, int* ss, int* ms, int* mc) {
    uint64_t time_in_100ns = microseconds * 10;
    FILETIME mst = { (DWORD)(time_in_100ns & 0xFFFFFFFF), (DWORD)(time_in_100ns >> 32) };
    SYSTEMTIME utc;
    FileTimeToSystemTime(&mst, &utc);
    DYNAMIC_TIME_ZONE_INFORMATION tzi;
    GetDynamicTimeZoneInformation(&tzi);
    SYSTEMTIME lt = {0};
    SystemTimeToTzSpecificLocalTimeEx(&tzi, &utc, &lt);
    *year = lt.wYear;
    *month = lt.wMonth;
    *day = lt.wDay;
    *hh = lt.wHour;
    *mm = lt.wMinute;
    *ss = lt.wSecond;
    *ms = lt.wMilliseconds;
    *mc = microseconds % 1000;
}

static double crt_seconds() { // since_boot
    LARGE_INTEGER qpc;
    QueryPerformanceCounter(&qpc);
    static double one_over_freq;
    if (one_over_freq == 0) {
        LARGE_INTEGER frequency;
        QueryPerformanceFrequency(&frequency);
        one_over_freq = 1.0 / frequency.QuadPart;
    }
    return (double)qpc.QuadPart * one_over_freq;
}

#define ctz(x) _tzcnt_u32(x)

static uint32_t crt_gcd(uint32_t u, uint32_t v) {
    uint32_t t = u | v;
    if (u == 0 || v == 0) { return t; }
    int g = ctz(t);
    while (u != 0) {
        u >>= ctz(u);
        v >>= ctz(v);
        if (u >= v) {
            u = (u - v) / 2;
        } else {
            v = (v - u) / 2;
        }
    }
    return v << g;
}

static int64_t crt_nanoseconds() {
    LARGE_INTEGER qpc;
    QueryPerformanceCounter(&qpc);
    static uint32_t freq;
    static uint32_t mul = NSEC_IN_SEC;
    if (freq == 0) {
        LARGE_INTEGER frequency;
        QueryPerformanceFrequency(&frequency);
        assert(frequency.HighPart == 0);
        // known values: 10,000,000 and 3,000,000
        // even 1GHz frequency should fit into 32 bit unsigned
        freq = frequency.LowPart;
        assert(freq != 0);
        uint32_t divider = crt_gcd(NSEC_IN_SEC, freq); // avoiding MulDiv64
        freq /= divider;
        mul /= divider;
    }
    uint64_t ns_mul_freq = (uint64_t)qpc.QuadPart * mul;
    return freq == 1 ? ns_mul_freq : ns_mul_freq / freq;
}

static int64_t crt_ns2ms(int64_t ns) { return (ns + NSEC_IN_MSEC - 1) / NSEC_IN_MSEC; }

typedef int (*gettimerresolution_t)(ULONG* minimum_resolution,
    ULONG* maximum_resolution, ULONG* actual_resolution);
typedef int (*settimerresolution_t)(ULONG requested_resolution,
    BOOLEAN Set, ULONG* actual_resolution);

static int crt_scheduler_set_timer_resolution(int64_t ns) { // nanoseconds
    const int ns100 = (int)(ns / 100);
    HMODULE ntdll = GetModuleHandleA("ntdll.dll");
    if (ntdll == null) { ntdll = LoadLibraryA("ntdll.dll"); }
    not_null(ntdll);
    gettimerresolution_t NtQueryTimerResolution =  (gettimerresolution_t)
        GetProcAddress(ntdll, "NtQueryTimerResolution");
    settimerresolution_t NtSetTimerResolution = (settimerresolution_t)
        GetProcAddress(ntdll, "NtSetTimerResolution");
    // it is resolution not frequency this is why it is in reverse
    // to common sense and what is not on Windows?
    unsigned long min_100ns = 16 * 10 * 1000;
    unsigned long max_100ns = 1 * 10 * 1000;
    unsigned long actual_100ns = 0;
    int r = 0;
    if (NtQueryTimerResolution != null &&
        NtQueryTimerResolution(&min_100ns, &max_100ns, &actual_100ns) == 0) {
        int64_t minimum_ns = min_100ns * 100LL;
        int64_t maximum_ns = max_100ns * 100LL;
//      int64_t actual_ns  = actual_100ns  * 100LL;
        // note that maximum resolution is actually < minimum
        if (NtSetTimerResolution == null) {
            const int milliseconds = (int)(crt_ns2ms(ns) + 0.5);
            r = (int)maximum_ns <= ns && ns <= (int)minimum_ns ?
                timeBeginPeriod(milliseconds) : ERROR_INVALID_PARAMETER;
        } else {
            r = (int)maximum_ns <= ns && ns <= (int)minimum_ns ?
                NtSetTimerResolution(ns100, true, &actual_100ns) :
                ERROR_INVALID_PARAMETER;
        }
        NtQueryTimerResolution(&min_100ns, &max_100ns, &actual_100ns);
    } else {
        const int milliseconds = (int)(crt_ns2ms(ns) + 0.5);
        r = 1 <= milliseconds && milliseconds <= 16 ?
            timeBeginPeriod(milliseconds) : ERROR_INVALID_PARAMETER;
    }
    return r;
}

static void crt_power_throttling_disable_for_process() {
    static bool disabled_for_the_process;
    if (!disabled_for_the_process) {
        PROCESS_POWER_THROTTLING_STATE pt = { 0 };
        pt.Version = PROCESS_POWER_THROTTLING_CURRENT_VERSION;
        pt.ControlMask = PROCESS_POWER_THROTTLING_EXECUTION_SPEED |
            PROCESS_POWER_THROTTLING_IGNORE_TIMER_RESOLUTION;
        pt.StateMask = 0;
        fatal_if_false(SetProcessInformation(GetCurrentProcess(),
            ProcessPowerThrottling, &pt, sizeof(pt)));
        disabled_for_the_process = true;
    }
}

static void crt_power_throttling_disable_for_thread(HANDLE thread) {
    THREAD_POWER_THROTTLING_STATE pt = { 0 };
    pt.Version = THREAD_POWER_THROTTLING_CURRENT_VERSION;
    pt.ControlMask = THREAD_POWER_THROTTLING_EXECUTION_SPEED;
    pt.StateMask = 0;
    fatal_if_false(SetThreadInformation(thread, ThreadPowerThrottling, &pt, sizeof(pt)));
}

static void crt_disable_power_throttling() {
    crt_power_throttling_disable_for_process();
    crt_power_throttling_disable_for_thread(GetCurrentThread());
}

static void threads_realtime() {
    fatal_if_false(SetPriorityClass(GetCurrentProcess(),
        REALTIME_PRIORITY_CLASS));
    fatal_if_false(SetThreadPriority(GetCurrentThread(),
        THREAD_PRIORITY_TIME_CRITICAL));
    fatal_if_false(SetThreadPriorityBoost(GetCurrentThread(),
        /* bDisablePriorityBoost = */ false));
    fatal_if_not_zero(
        crt_scheduler_set_timer_resolution(NSEC_IN_MSEC));
    crt_disable_power_throttling();
}

static thread_t threads_start(void (*func)(void*), void* p) {
    thread_t t = (thread_t)CreateThread(null, 0,
        (LPTHREAD_START_ROUTINE)func, p, 0, null);
    not_null(t);
    return t;
}

static bool is_handle_valid(void* h) {
    DWORD flags = 0;
    return GetHandleInformation(h, &flags);
}

static bool threads_try_join(thread_t t, double timeout) {
    not_null(t);
    fatal_if_false(is_handle_valid(t));
    int32_t timeout_ms = timeout <= 0 ? 0 : (int)(timeout * 1000.0 + 0.5);
    int r = WaitForSingleObject(t, timeout_ms);
    if (r != 0) {
        traceln("failed to join thread %p %s", t, crt.error(r));
    } else {
        r = CloseHandle(t) ? 0 : GetLastError();
        if (r != 0) { traceln("CloseHandle(%p) failed %s", t, crt.error(r)); }
    }
    return r == 0;
}

static void threads_join(thread_t t) {
    // longer timeout for super slow instrumented code
    #ifdef DEBUG
    const double timeout = 3.0; // 30 seconds
    #else
    const double timeout = 1.0; // 1 second
    #endif
    if (!threads.try_join(t, timeout)) {
        traceln("failed to join thread %p", t);
    }
}

static void threads_name(const char* name) {
    HRESULT r = SetThreadDescription(GetCurrentThread(), utf8to16(name));
    // notoriously returns 0x10000000 for no good reason whatsoever
    if (!SUCCEEDED(r)) { fatal_if_not_zero(r); }
}

threads_if threads = {
    .start = threads_start,
    .try_join = threads_try_join,
    .join = threads_join,
    .name = threads_name,
    .realtime = threads_realtime
};

static event_t events_create() {
    HANDLE e = CreateEvent(null, false, false, null);
    not_null(e);
    return (event_t)e;
}

static event_t events_create_manual() {
    HANDLE e = CreateEvent(null, true, false, null);
    not_null(e);
    return (event_t)e;
}

static void events_set(event_t e) {
    fatal_if_false(SetEvent((HANDLE)e));
}

static void events_reset(event_t e) {
    fatal_if_false(ResetEvent((HANDLE)e));
}

static int events_wait_or_timeout(event_t e, double seconds) {
    uint32_t ms = seconds < 0 ? INFINITE : (int32_t)(seconds * 1000.0 + 0.5);
    uint32_t r = 0;
    fatal_if_false((r = WaitForSingleObject(e, ms)) != WAIT_FAILED);
    return r == WAIT_OBJECT_0 ? 0 : -1; // all WAIT_ABANDONED as -1
}

static void events_wait(event_t e) { events_wait_or_timeout(e, -1); }

static int events_wait_any_or_timeout(int n, event_t events_[], double s) {
    uint32_t ms = s < 0 ? INFINITE : (int32_t)(s * 1000.0 + 0.5);
    uint32_t r = 0;
    fatal_if_false((r = WaitForMultipleObjects(n, events_, false, ms)) != WAIT_FAILED);
    // all WAIT_ABANDONED_0 and WAIT_IO_COMPLETION 0xC0 as -1
    return r < WAIT_OBJECT_0 + n ? r - WAIT_OBJECT_0 : -1;
}

static int events_wait_any(int n, event_t e[]) {
    return events_wait_any_or_timeout(n, e, -1);
}

static void events_dispose(event_t handle) {
    fatal_if_false(CloseHandle(handle));
}

events_if events = {
    events_create,
    events_create_manual,
    events_set,
    events_reset,
    events_wait,
    events_wait_or_timeout,
    events_wait_any,
    events_wait_any_or_timeout,
    events_dispose
};

static_assertion(sizeof(CRITICAL_SECTION) == sizeof(mutex_t));

static void mutex_init(mutex_t* m) {
    fatal_if_false(InitializeCriticalSectionAndSpinCount((LPCRITICAL_SECTION)m, 4096));
}

static void mutex_lock(mutex_t* m) { EnterCriticalSection((LPCRITICAL_SECTION)m); }

static void mutex_unlock(mutex_t* m) { LeaveCriticalSection((LPCRITICAL_SECTION)m); }

static void mutex_dispose(mutex_t* m) { DeleteCriticalSection((LPCRITICAL_SECTION)m); }

mutex_if mutexes = {
    .init = mutex_init,
    .lock = mutex_lock,
    .unlock = mutex_unlock,
    .dispose = mutex_dispose
};

// #include <stdatomic.h> still is not implemented

static void* atomics_exchange_ptr(volatile void** a, void* v) {
    size_t s = sizeof(void*); (void)s;
//  assert(sizeof(void*) == 8 && sizeof(uintptr_t) == 8, "expected 64 bit architecture");
    assert(s == sizeof(uint64_t), "expected 64 bit architecture");
    return (void*)atomics.exchange_int64((int64_t*)a, (int64_t)v);
}

static int32_t atomics_increment_int32(volatile int32_t* a) {
    return InterlockedIncrement((volatile LONG*)a);
}

static int32_t atomics_decrement_int32(volatile int32_t* a) {
    return InterlockedDecrement((volatile LONG*)a);
}

static int64_t atomics_increment_int64(volatile int64_t* a) {
    return InterlockedIncrement64((__int64 volatile *)a);
}

static int64_t atomics_decrement_int64(volatile int64_t* a) {
    return InterlockedDecrement64((__int64 volatile *)a);
}

static int64_t atomics_add_int64(volatile int64_t* a, int64_t v) {
    return InterlockedAdd64((__int64 volatile *)a, v);
}

static int64_t atomics_exchange_int64(volatile int64_t* a, int64_t v) {
    return (int64_t)InterlockedExchange64((LONGLONG*)a, (LONGLONG)v);
}

static int32_t atomics_exchange_int32(volatile int32_t* a, int32_t v) {
    assert(sizeof(int32_t) == sizeof(unsigned long));
    return (int32_t)InterlockedExchange((volatile LONG*)a, (unsigned long)v);
}

static bool atomics_compare_exchange_int64(volatile int64_t* a, int64_t comparand, int64_t v) {
    return (int64_t)InterlockedCompareExchange64((LONGLONG*)a, (LONGLONG)v, (LONGLONG)comparand) == comparand;
}

static bool atomics_compare_exchange_int32(volatile int32_t* a, int32_t comparand, int32_t v) {
    return (int64_t)InterlockedCompareExchange((LONG*)a, (LONG)v, (LONG)comparand) == comparand;
}

// https://en.wikipedia.org/wiki/Spinlock

#define __sync_bool_compare_and_swap(p, old_val, new_val) \
    _InterlockedCompareExchange64(p, new_val, old_val) == old_val

#define __builtin_cpu_pause() YieldProcessor()

static void spinlock_acquire(volatile int64_t* spinlock) {
    // Very basic implementation of a spinlock. This is currently
    // only used to guarantee thread-safety during context initialization
    // and shutdown (which are both executed very infrequently and
    // have minimal thread contention).
    // Not a performance champion (because of mem_fence()) but serves
    // the purpose. mem_fence() can be reduced to mem_sfence()... sigh
    while (!__sync_bool_compare_and_swap(spinlock, 0, 1)) {
        while (*spinlock) {
            __builtin_cpu_pause();
        }
    }
    atomics.memory_fence(); // not strcitly necessary on strong mem model Intel/AMD but
    // see: https://cfsamsonbooks.gitbook.io/explaining-atomics-in-rust/
    //      Fig 2 Inconsistent C11 execution of SB and 2+2W
    assert(*spinlock == 1);
}

static void spinlock_release(volatile int64_t* spinlock) {
    assert(*spinlock == 1);
    *spinlock = 0;
    atomics.memory_fence(); // tribute to lengthy Linus discussion going since 2006
}

static void memory_fence() { _mm_mfence(); }

atomics_if atomics = {
    .exchange_ptr    = atomics_exchange_ptr,
    .increment_int32 = atomics_increment_int32,
    .decrement_int32 = atomics_decrement_int32,
    .increment_int64 = atomics_increment_int64,
    .decrement_int64 = atomics_decrement_int64,
    .add_int64 = atomics_add_int64,
    .exchange_int32  = atomics_exchange_int32,
    .exchange_int64  = atomics_exchange_int64,
    .compare_exchange_int64 = atomics_compare_exchange_int64,
    .compare_exchange_int32 = atomics_compare_exchange_int32,
    .spinlock_acquire = spinlock_acquire,
    .spinlock_release = spinlock_release,
    .memory_fence = memory_fence
};

static void utf8_ods(const char* s) { OutputDebugStringW(utf8to16(s)); }

enum { NO_LINEFEED = -2 };

static int vformat_(char* s, int count, const char* file, int line,
    const char* func, const char* format, va_list vl) {
    s[0] = 0;
    char* sb = s;
    int left = count - 1;
    const char* foo = func != null ? func : "";
    const char* fn = file != null ? file : "";
    if (fn[0] != 0) {
        crt.sformat(sb, left, "%s(%d): [%05d] %s ", file, line, crt.gettid(), foo);
        int n = (int)strlen(sb);
        sb += n;
        left -= n;
    } else  if (foo[0] != 0) {
        crt.sformat(sb, left, "[%05d] %s ", crt.gettid(), foo);
        int n = (int)strlen(sb);
        sb += n;
        left -= n;
    }
    crt.vformat(sb, left, format, vl);
    int k = (int)strlen(sb);
    sb += k;
    if (k == 0 || sb[-1] != '\n') {
        *sb = '\n';
        sb++;
        *sb = 0;
    }
    return (int)(sb - s);
}

// traceln() both OutputDebugStringA() and vfprintf() are subject
// of synchronizarion/seralization and may enter wait state which
// in combination with Windows scheduler can take milliseconds to
// complete. In time critical threads use traceln() instead which
// does NOT lock at all and completes in microseconds

static void crt_ods_and_printf_vl(bool print, const char* file, int line,
        const char* func, const char* format, va_list vl) {
    print = print && GetStdHandle(STD_ERROR_HANDLE) != null;
    enum { max_ods_count = 32 * 1024 - 1 };
    char s[max_ods_count * 2 + 1];
    const char* foo = func != null ? func : "";
    const char* fn = file != null ? file : "";
    if (fn[0] != 0 || foo[0] != 0) {
        const char* basename = strrchr(fn, '/');
        if (basename == null) { basename = strrchr(fn, '\\'); }
        basename = basename != null ? basename + 1 : file;
        (void)vformat_(s, (int)countof(s), fn, line, foo, format, vl);
        utf8_ods(s);
        if (print) {
            if (fn[0] != 0) {
                const char* bn = (file + (int)(basename - file));
                fprintf(stderr, "%s(%d): [%05d] %s ", bn, line, crt.gettid(), foo);
            } else {
                fprintf(stderr, "[%05d] %s ", crt.gettid(), foo);
            }
            vfprintf(stderr, format, vl);
            fprintf(stderr, "\n");
        }
    } else {
        crt.vformat(s, (int)countof(s), format, vl);
        utf8_ods(s);
        if (print) { vfprintf(stderr, format, vl); }
    }
}

static void crt_traceline_vl(const char* file, int line, const char* func,
        const char* format, va_list vl) {
    crt_ods_and_printf_vl(true, file, line, func, format, vl);
}

static void crt_traceline(const char* file, int line, const char* func,
        const char* format, ...) {
    va_list vl;
    va_start(vl, format);
    crt_ods_and_printf_vl(true, file, line, func, format, vl);
    va_end(vl);
}

static void crt_ods(const char* file, int line, const char* func,
        const char* format, ...) {
    va_list vl;
    va_start(vl, format);
    crt_ods_and_printf_vl(false, file, line, func, format, vl);
    va_end(vl);
}

static int crt_utf8_bytes(const wchar_t* utf16) {
    int required_bytes_count = WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS,
        utf16, -1, null, 0, null, null);
    assert(required_bytes_count > 0);
    return required_bytes_count;
}

static int crt_utf16_bytes(const char* utf8) {
    int required_bytes_count = MultiByteToWideChar(CP_UTF8, 0, utf8, -1, null, 0);
    assert(required_bytes_count > 0);
    return required_bytes_count;
}

static char* crt_utf16to8(char* s, const wchar_t* utf16) {
    int r = WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS, utf16, -1, s,
        crt.utf8_bytes(utf16), null, null);
    if (r == 0) {
        traceln("WideCharToMultiByte() failed %s", crt.error(crt.err()));
    }
    return s;
}

static wchar_t* crt_utf8to16(wchar_t* utf16, const char* s) {
    int r = MultiByteToWideChar(CP_UTF8, 0, s, -1, utf16, crt.utf16_bytes(s));
    if (r == 0) {
        traceln("WideCharToMultiByte() failed %d", crt.err());
    }
    return utf16;
}

static void crt_breakpoint() { if (IsDebuggerPresent()) { DebugBreak(); } }

static int crt_gettid() { return (int)GetCurrentThreadId(); }

static void crt_fatal(const char* file, int line, const char* func,
        const char* (*err2str)(int32_t err), int32_t error,
        const char* call, const char* extra) {
    crt.traceline(file, line, func, "FATAL: %s failed 0x%08X (%d) %s %s",
        call, error, error, err2str(error), extra);
    crt.breakpoint();
    if (file != null) { ExitProcess(error); }
}

static int crt_assertion_failed(const char* file, int line, const char* func,
                      const char* condition, const char* format, ...) {
    const int n = (int)strlen(format);
    if (n == 0) {
        crt.traceline(file, line, func, "assertion failed: \"%s\"", condition);
    } else {
        crt.traceline(file, line, func, "assertion failed: \"%s\"", condition);
        va_list va;
        va_start(va, format);
        crt_traceline_vl(file, line, func, format, va);
        va_end(va);
    }
    crt.sleep(0.25); // give other threads some time to flush buffers
    crt.breakpoint();
    // pretty much always true but avoids "warning: unreachable code"
    if (file != null || line != 0 || func != null) { exit(1); }
    return 0;
}

static int32_t crt_err() { return GetLastError(); }

static void crt_seterr(int32_t err) { SetLastError(err); }

static uint32_t crt_random32(uint32_t* state) {
    // https://gist.github.com/tommyettinger/46a874533244883189143505d203312c
    static thread_local bool started; // first seed must be odd
    if (!started) { started = true; *state |= 1; }
    uint32_t z = (*state += 0x6D2B79F5UL);
    z = (z ^ (z >> 15)) * (z | 1UL);
    z ^= z + (z ^ (z >> 7)) * (z | 61UL);
    return z ^ (z >> 14);
}

static uint64_t crt_random64(uint64_t *state) {
    // https://gist.github.com/tommyettinger/e6d3e8816da79b45bfe582384c2fe14a
    static thread_local bool started; // first seed must be odd
    if (!started) { started = true; *state |= 1; }
	const uint64_t s = *state;
	const uint64_t z = (s ^ s >> 25) * (*state += 0x6A5D39EAE12657AAULL);
	return z ^ (z >> 22);
}

static int crt_memmap_file(HANDLE file, void* *data, int64_t *bytes, bool rw) {
    int r = 0;
    void* address = null;
    LARGE_INTEGER size = {{0, 0}};
    if (GetFileSizeEx(file, &size)) {
        HANDLE mapping = CreateFileMapping(file, null,
            rw ? PAGE_READWRITE : PAGE_READONLY,
            0, (DWORD)size.QuadPart, null);
        if (mapping == null) {
            r = GetLastError();
        } else {
            address = MapViewOfFile(mapping, FILE_MAP_READ,
                0, 0, (int64_t)size.QuadPart);
            if (address != null) {
                *bytes = (int64_t)size.QuadPart;
            } else {
                r = GetLastError();
            }
            fatal_if_false(CloseHandle(mapping));
        }
    } else {
        r = GetLastError();
    }
    if (r == 0) { *data = address; }
    return r;
}

static int crt_memmap(const char* filename, void* *data,
        int64_t *bytes, bool rw) {
    *bytes = 0;
    int r = 0;
    const DWORD flags = GENERIC_READ | (rw ? GENERIC_WRITE : 0);
    const DWORD share = FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE;
    HANDLE file = CreateFileA(filename, flags, share, null,
        OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, null);
    if (file == INVALID_HANDLE_VALUE) {
        r = GetLastError();
    } else {
        r = crt_memmap_file(file, data, bytes, rw);
        fatal_if_false(CloseHandle(file));
    }
    return r;
}

static int crt_memmap_read(const char* filename, void* *data, int64_t *bytes) {
    return crt_memmap(filename, data, bytes, false);
}

static int crt_memmap_rw(const char* filename, void* *data, int64_t *bytes) {
    return crt_memmap(filename, data, bytes, true);
}

static void crt_memunmap(void* data, int64_t bytes) {
    assert(data != null && bytes > 0);
    (void)bytes; /* unused only need for posix version */
    if (data != null && bytes > 0) {
        fatal_if_false(UnmapViewOfFile(data));
    }
}

static int crt_memmap_res(const char* label, void* *data, int64_t *bytes) {
    HRSRC res = FindResourceA(null, label, (const char*)RT_RCDATA);
    // "LockResource does not actually lock memory; it is just used to
    // obtain a pointer to the memory containing the resource data.
    // The name of the function comes from versions prior to Windows XP,
    // when it was used to lock a global memory block allocated by LoadResource."
    if (res != null) { *bytes = SizeofResource(null, res); }
    HGLOBAL g = res != null ? LoadResource(null, res) : null;
    *data = g != null ? LockResource(g) : null;
    return *data != null ? 0 : GetLastError();
}

crt_if crt = {
    .err = crt_err,
    .seterr = crt_seterr,
    .sleep = crt_sleep,
    .random32 = crt_random32,
    .random64 = crt_random64,
    .memmap_read = crt_memmap_read,
    .memmap_rw = crt_memmap_rw,
    .memunmap = crt_memunmap,
    .memmap_res = crt_memmap_res,
    .seconds = crt_seconds,
    .nanoseconds = crt_nanoseconds,
    .microseconds = crt_microseconds_since_epoch,
    .localtime = crt_localtime,
    .time_utc = crt_time_utc,
    .time_local = crt_time_local,
    .vformat = crt_vformat,
    .sformat = crt_sformat,
    .error = crt_error,
    .error_nls = crt_error_nls,
    .utf8_bytes = crt_utf8_bytes,
    .utf16_bytes = crt_utf16_bytes,
    .utf16to8 = crt_utf16to8,
    .utf8to16 = crt_utf8to16,
    .ods = crt_ods,
    .traceline = crt_traceline,
    .vtraceline = crt_traceline_vl,
    .breakpoint = crt_breakpoint,
    .gettid = crt_gettid,
    .assertion_failed = crt_assertion_failed,
    .fatal = crt_fatal
};

#pragma comment(lib, "advapi32")
#pragma comment(lib, "cabinet")
#pragma comment(lib, "comctl32")
#pragma comment(lib, "comdlg32")
#pragma comment(lib, "cfgmgr32")
#pragma comment(lib, "dxgi")
#pragma comment(lib, "dxva2")
#pragma comment(lib, "dwmapi")
#pragma comment(lib, "gdi32")
#pragma comment(lib, "msimg32")
#pragma comment(lib, "ntdll")
#pragma comment(lib, "ole32")
#pragma comment(lib, "OneCoreUAP")
#pragma comment(lib, "powrprof")
#pragma comment(lib, "rpcrt4")
#pragma comment(lib, "setupapi")
#pragma comment(lib, "shcore")
#pragma comment(lib, "shell32")
#pragma comment(lib, "shlwapi")
#pragma comment(lib, "winmm")
#pragma comment(lib, "winusb")
#pragma comment(lib, "user32")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "dbghelp")
#pragma comment(lib, "imagehlp")
#pragma comment(lib, "opengl32")
#pragma comment(lib, "glu32")

end_c

#endif crt_implementation

