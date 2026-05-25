#ifndef posix_definition
#define posix_definition

// _________________________________ posix.h __________________________________

#ifndef POSIX_H
#define POSIX_H

#include "sfh_core.h"

#include <ctype.h>
#include <errno.h>
#include <float.h>
#include <limits.h>
#include <locale.h>
#include <signal.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#if defined(_WIN32)
#include <malloc.h> // _alloca, _msize
#else
#include <unistd.h>
#endif

#define posix_stringify(x) #x
#define posix_tostring(x) posix_stringify(x)
#define posix_pragma(x) _Pragma(posix_tostring(x))

#if defined(__GNUC__) || defined(__clang__)
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
#pragma GCC diagnostic ignored "-Wused-but-marked-unused"
#endif

// Type aliases for floating-point types similar to <stdint.h>
typedef float  fp32_t;
typedef double fp64_t;

#ifdef __cplusplus
    #define posix_begin_c extern "C" {
    #define posix_end_c }
#else
    #define posix_begin_c
    #define posix_end_c
#endif

#define posix_countof(a) ((int32_t)((int)(sizeof(a) / sizeof((a)[0]))))

#if defined(__GNUC__) || defined(__clang__)
    #define posix_force_inline __attribute__((always_inline))
#else
    #define posix_force_inline inline
#endif

#ifndef __cplusplus
    #define null ((void*)0)
#else
    #define null nullptr
#endif

#if defined(_MSC_VER)
    #define posix_thread_local __declspec(thread)
#elif !defined(__cplusplus)
    #define posix_thread_local _Thread_local // C11
#endif

#if defined(_MSC_VER)
    #define posix_suppress_constant_cond_exp _Pragma("warning(suppress: 4127)")
#else
    #define posix_suppress_constant_cond_exp
#endif

#if defined(__GNUC__) || defined(__clang__)
#define posix_attribute_packed __attribute__((packed))
#define posix_begin_packed
#define posix_end_packed posix_attribute_packed
#define posix_aligned_8 __attribute__((aligned(8)))
#define posix_unused(name) name __attribute__((unused))
#elif defined(_MSC_VER)
#define posix_attribute_packed
#define posix_begin_packed posix_pragma( pack(push, 1) )
#define posix_end_packed posix_pragma( pack(pop) )
#define posix_aligned_8 __declspec(align(8))
#define posix_unused(name) _Pragma("warning(suppress: 4100)") name
#else
#define posix_attribute_packed
#define posix_begin_packed
#define posix_end_packed
#define posix_aligned_8
#define posix_unused(name) name
#endif

// Dynamic stack allocation (VLAs are standard in C99, optional in C11)
// Avoiding alloca() when possible.
#if defined(_MSC_VER)
#define posix_stackalloc(n) (_Pragma("warning(suppress: 6255 6263)") _alloca(n))
#else
#define posix_stackalloc(n) (__builtin_alloca(n))
#endif

// _______________________________ posix_win32.h _________________________________

// Win32-only helpers used by the Windows backends and by Windows consumers
// (e.g. ui.*). On other platforms these are not defined. The macros only
// reference Win32 APIs at the expansion site, so posix.h itself stays free
// of <Windows.h>; the implementation (posix.c) and Windows-only consumers
// include the SDK headers they need.

#if defined(_WIN32)

posix_begin_c

// Win32 BOOL -> errno_t (int) translation: 0 on success, GetLastError() on fail.
#define posix_b2e(call) ((int)((call) ? 0 : GetLastError()))

void posix_win32_close_handle(void* h);
int  posix_wait_ix2e(uint32_t r); // translate WaitForSingleObject ix to error

posix_end_c

#endif // _WIN32

// _________________________________ posix_str.h _________________________________

posix_begin_c

struct posix_str64 { char s[64]; };
struct posix_str128 { char s[128]; };
struct posix_str1024 { char s[1024]; };
struct posix_str32K { char s[32 * 1024]; };

#define posix_str_printf(s, ...) posix_str.format((s), posix_countof(s), "" __VA_ARGS__)
#define posix_strerr(r) (posix_str.error((r)).s)

struct posix_str_if {
    char* (*drop_const)(const char* s);
    int32_t (*len)(const char* s);
    int32_t (*len16)(const uint16_t* utf16);
    int32_t (*utf8bytes)(const char* utf8, int32_t bytes);
    int32_t (*glyphs)(const char* utf8, int32_t bytes);
    bool (*starts)(const char* s1, const char* s2);
    bool (*ends)(const char* s1, const char* s2);
    bool (*istarts)(const char* s1, const char* s2);
    bool (*iends)(const char* s1, const char* s2);
    void (*lower)(char* d, int32_t capacity, const char* s);
    void (*upper)(char* d, int32_t capacity, const char* s);
    int32_t (*utf8_bytes)(const uint16_t* utf16, int32_t bytes);
    int32_t (*utf16_chars)(const char* utf8, int32_t bytes);
    int (*utf16to8)(char* utf8, int32_t capacity, const uint16_t* utf16, int32_t chars);
    int (*utf8to16)(uint16_t* utf16, int32_t capacity, const char* utf8, int32_t bytes);
    bool (*utf16_is_low_surrogate)(uint16_t utf16char);
    bool (*utf16_is_high_surrogate)(uint16_t utf16char);
    uint32_t (*utf32)(const char* utf8, int32_t bytes);
    void (*format_va)(char* utf8, int32_t count, const char* format, va_list va);
    void (*format)(char* utf8, int32_t count, const char* format, ...);
    const char* (*grouping_separator)(void);
    struct posix_str64 (*int64_dg)(int64_t v, bool uint, const char* gs);
    struct posix_str64 (*int64)(int64_t v);
    struct posix_str64 (*uint64)(uint64_t v);
    struct posix_str64 (*int64_lc)(int64_t v);
    struct posix_str64 (*uint64_lc)(uint64_t v);
    struct posix_str128 (*fp)(const char* format, fp64_t v);
    struct posix_str1024 (*error)(int32_t error);
    struct posix_str1024 (*error_nls)(int32_t error);
    void (*test)(void);
};

extern struct posix_str_if posix_str;

posix_end_c

// ________________________________ posix_args.h _________________________________

posix_begin_c

struct posix_args_if {
    int32_t c;
    const char** v;
    const char** env;
    void    (*main)(int32_t argc, const char* argv[], const char** env);
    void    (*WinMain)(void); // Windows only; null on other platforms
    int32_t (*option_index)(const char* option);
    void    (*remove_at)(int32_t ix);
    bool    (*option_bool)(const char* option);
    bool    (*option_int)(const char* option, int64_t *value);
    const char* (*option_str)(const char* option);
    const char* (*basename)(void);
    void (*fini)(void);
    void (*test)(void);
};

extern struct posix_args_if posix_args;

posix_end_c

// ______________________________ posix_backtrace.h ______________________________

posix_begin_c

enum { posix_backtrace_max_depth = 64 };
enum { posix_backtrace_max_symbol = 1024 };

typedef struct thread_s* posix_thread_t;
typedef char posix_backtrace_symbol_t[posix_backtrace_max_symbol];
typedef char posix_backtrace_file_t[512];

struct posix_backtrace {
    int32_t frames;
    uint32_t hash;
    int  error;
    void* stack[posix_backtrace_max_depth];
    posix_backtrace_symbol_t symbol[posix_backtrace_max_depth];
    posix_backtrace_file_t file[posix_backtrace_max_depth];
    int32_t line[posix_backtrace_max_depth];
    bool symbolized;
};

struct posix_backtrace_if {
    void (*capture)(struct posix_backtrace *bt, int32_t skip);
    void (*context)(posix_thread_t thread, const void* context, struct posix_backtrace *bt);
    void (*symbolize)(struct posix_backtrace *bt);
    void (*trace)(const struct posix_backtrace* bt, const char* stop);
    void (*trace_self)(const char* stop);
    void (*trace_all_but_self)(void);
    const char* (*string)(const struct posix_backtrace* bt, char* text, int32_t count);
    void (*test)(void);
};

extern struct posix_backtrace_if posix_backtrace;

#define posix_backtrace_here() do {  \
    struct posix_backtrace bt_ = {0};     \
    posix_backtrace.capture(&bt_, 0);\
    posix_backtrace.symbolize(&bt_); \
    posix_backtrace.trace(&bt_, "*");\
} while (0)

posix_end_c

// _______________________________ posix_atomics.h _______________________________

posix_begin_c

struct posix_atomics_if {
    void* (*exchange_ptr)(volatile void** a, void* v);
    int32_t (*increment_int32)(volatile int32_t* a);
    int32_t (*decrement_int32)(volatile int32_t* a);
    int64_t (*increment_int64)(volatile int64_t* a);
    int64_t (*decrement_int64)(volatile int64_t* a);
    int32_t (*add_int32)(volatile int32_t* a, int32_t v);
    int64_t (*add_int64)(volatile int64_t* a, int64_t v);
    int32_t (*exchange_int32)(volatile int32_t* a, int32_t v);
    int64_t (*exchange_int64)(volatile int64_t* a, int64_t v);
    bool (*compare_exchange_int64)(volatile int64_t* a, int64_t comparand, int64_t v);
    bool (*compare_exchange_int32)(volatile int32_t* a, int32_t comparand, int32_t v);
    bool (*compare_exchange_ptr)(volatile void** a, void* comparand, void* v);
    void (*spinlock_acquire)(volatile int64_t* spinlock);
    void (*spinlock_release)(volatile int64_t* spinlock);
    int32_t (*load32)(volatile int32_t* a);
    int64_t (*load64)(volatile int64_t* a);
    void (*memory_fence)(void);
    void (*test)(void);
};

extern struct posix_atomics_if posix_atomics;

posix_end_c

// ______________________________ posix_clipboard.h ______________________________

posix_begin_c

typedef struct ui_bitmap ui_bitmap_t;

struct posix_clipboard_if {
    int (*put_text)(const char* s);
    int (*get_text)(char* text, int32_t* bytes);
    int (*put_image)(ui_bitmap_t* image);
    void (*test)(void);
};

extern struct posix_clipboard_if posix_clipboard;

posix_end_c

// ________________________________ posix_clock.h ________________________________

posix_begin_c

struct posix_clock_if {
    int32_t const nsec_in_usec;
    int32_t const nsec_in_msec;
    int32_t const nsec_in_sec;
    int32_t const usec_in_msec;
    int32_t const msec_in_sec;
    int32_t const usec_in_sec;
    fp64_t   (*seconds)(void);
    uint64_t (*nanoseconds)(void);
    uint64_t (*unix_microseconds)(void);
    uint64_t (*unix_seconds)(void);
    uint64_t (*microseconds)(void);
    uint64_t (*localtime)(void);
    void (*utc)(uint64_t microseconds, int32_t* year, int32_t* month,
        int32_t* day, int32_t* hh, int32_t* mm, int32_t* ss, int32_t* ms,
        int32_t* mc);
    void (*local)(uint64_t microseconds, int32_t* year, int32_t* month,
        int32_t* day, int32_t* hh, int32_t* mm, int32_t* ss, int32_t* ms,
        int32_t* mc);
    void (*test)(void);
};

extern struct posix_clock_if posix_clock;

posix_end_c

// _______________________________ posix_config.h ________________________________

posix_begin_c

struct posix_config_if {
    int (*save)(const char* name, const char* key, const void* data, int32_t bytes);
    int32_t (*size)(const char* name, const char* key);
    int32_t (*load)(const char* name, const char* key, void* data, int32_t bytes);
    int (*remove)(const char* name, const char* key);
    int (*clean)(const char* name);
    void (*test)(void);
};

extern struct posix_config_if posix_config;

posix_end_c

// ________________________________ posix_core.h _________________________________

posix_begin_c

struct posix_core_if {
    int (*err)(void);
    void (*set_err)(int err);
    void (*abort)(void);
    void (*exit)(int32_t exit_code);
    void (*test)(void);
    struct {
        int const access_denied;
        int const bad_file;
        int const broken_pipe;
        int const device_not_ready;
        int const directory_not_empty;
        int const disk_full;
        int const file_exists;
        int const file_not_found;
        int const insufficient_buffer;
        int const interrupted;
        int const invalid_data;
        int const invalid_handle;
        int const invalid_parameter;
        int const io_error;
        int const more_data;
        int const name_too_long;
        int const no_child_process;
        int const not_a_directory;
        int const not_empty;
        int const out_of_memory;
        int const path_not_found;
        int const pipe_not_connected;
        int const read_only_file;
        int const resource_deadlock;
        int const too_many_open_files;
    } const error;
};

extern struct posix_core_if posix_core;

posix_end_c

// ________________________________ posix_debug.h ________________________________

posix_begin_c

struct posix_verbosity_if {
    int32_t level;
    int32_t const quiet;
    int32_t const info;
    int32_t const verbose;
    int32_t const debug;
    int32_t const trace;
};

struct posix_debug_if {
    struct posix_verbosity_if verbosity;
    int32_t (*verbosity_from_string)(const char* s);
    bool (*tee)(const char* s, int32_t count);
    void (*output)(const char* s, int32_t count);
    void (*println_va)(const char* file, int32_t line, const char* func,
        const char* format, va_list va);
    void (*println)(const char* file, int32_t line, const char* func,
        const char* format, ...);
    void (*perrno)(const char* file, int32_t line,
        const char* func, int32_t err_no, const char* format, ...);
    void (*perror)(const char* file, int32_t line,
        const char* func, int32_t error, const char* format, ...);
    bool (*is_debugger_present)(void);
    void (*breakpoint)(void);
    int (*raise)(uint32_t exception);
    struct {
        uint32_t const sig_segv;
        uint32_t const sig_ill;
        uint32_t const sig_trap;
        uint32_t const sig_fpe;
        uint32_t const sig_bus;
        uint32_t const sig_abrt;
    } exception;
    void (*test)(void);
};

#define posix_println(...) posix_debug.println(__FILE__, __LINE__, __func__, "" __VA_ARGS__)

extern struct posix_debug_if posix_debug;

posix_end_c

// ________________________________ posix_files.h ________________________________

posix_begin_c

struct posix_file_name { char s[1024]; };

enum { posix_files_max_path = (int32_t)sizeof(struct posix_file_name) };

struct posix_file;

struct posix_files_stat {
    uint64_t created;
    uint64_t accessed;
    uint64_t updated;
    int64_t  size;
    int64_t  type;
};

struct posix_folder {
    uint8_t data[512];
};

struct posix_files_if {
    struct posix_file* const invalid;
    int32_t const type_folder;
    int32_t const type_symlink;
    int32_t const type_device;
    int32_t const seek_set;
    int32_t const seek_cur;
    int32_t const seek_end;
    int32_t const o_rd;
    int32_t const o_wr;
    int32_t const o_rw;
    int32_t const o_append;
    int32_t const o_create;
    int32_t const o_excl;
    int32_t const o_trunc;
    int32_t const o_sync;
    struct {
        int32_t const home;
        int32_t const desktop;
        int32_t const documents;
        int32_t const downloads;
        int32_t const music;
        int32_t const pictures;
        int32_t const videos;
        int32_t const shared;
        int32_t const bin;
        int32_t const data;
    } const folder;
    int (*open)(struct posix_file* *file, const char* filename, int32_t flags);
    bool    (*is_valid)(struct posix_file* file);
    int (*seek)(struct posix_file* file, int64_t *position, int32_t method);
    int (*stat)(struct posix_file* file, struct posix_files_stat* stat, bool follow_symlink);
    int (*read)(struct posix_file* file, void* data, int64_t bytes, int64_t *transferred);
    int (*write)(struct posix_file* file, const void* data, int64_t bytes, int64_t *transferred);
    int (*flush)(struct posix_file* file);
    void    (*close)(struct posix_file* file);
    int (*write_fully)(const char* filename, const void* data,
                           int64_t bytes, int64_t *transferred);
    bool (*exists)(const char* pathname);
    bool (*is_folder)(const char* pathname);
    bool (*is_symlink)(const char* pathname);
    const char* (*basename)(const char* pathname);
    int (*mkdirs)(const char* pathname);
    int (*rmdirs)(const char* pathname);
    int (*create_tmp)(char* file, int32_t count);
    int (*chmod777)(const char* pathname);
    int (*symlink)(const char* from, const char* to);
    int (*link)(const char* from, const char* to);
    int (*unlink)(const char* pathname);
    int (*copy)(const char* from, const char* to);
    int (*move)(const char* from, const char* to);
    int (*cwd)(char* folder, int32_t count);
    int (*chdir)(const char* folder);
    const char* (*known_folder)(int32_t kf_id);
    const char* (*bin)(void);
    const char* (*data)(void);
    const char* (*tmp)(void);
    int (*opendir)(struct posix_folder* folder, const char* folder_name);
    const char* (*readdir)(struct posix_folder* folder, struct posix_files_stat* optional);
    void (*closedir)(struct posix_folder* folder);
    void (*test)(void);
};

extern struct posix_files_if posix_files;

posix_end_c

// ______________________________ posix_generics.h _______________________________

posix_begin_c

static inline int8_t   posix_max_int8(int8_t x, int8_t y)       { return x > y ? x : y; }
static inline int16_t  posix_max_int16(int16_t x, int16_t y)    { return x > y ? x : y; }
static inline int32_t  posix_max_int32(int32_t x, int32_t y)    { return x > y ? x : y; }
static inline int64_t  posix_max_int64(int64_t x, int64_t y)    { return x > y ? x : y; }
static inline uint8_t  posix_max_uint8(uint8_t x, uint8_t y)    { return x > y ? x : y; }
static inline uint16_t posix_max_uint16(uint16_t x, uint16_t y) { return x > y ? x : y; }
static inline uint32_t posix_max_uint32(uint32_t x, uint32_t y) { return x > y ? x : y; }
static inline uint64_t posix_max_uint64(uint64_t x, uint64_t y) { return x > y ? x : y; }
static inline fp32_t   posix_max_fp32(fp32_t x, fp32_t y)       { return x > y ? x : y; }
static inline fp64_t   posix_max_fp64(fp64_t x, fp64_t y)       { return x > y ? x : y; }

static inline int8_t   posix_min_int8(int8_t x, int8_t y)       { return x < y ? x : y; }
static inline int16_t  posix_min_int16(int16_t x, int16_t y)    { return x < y ? x : y; }
static inline int32_t  posix_min_int32(int32_t x, int32_t y)    { return x < y ? x : y; }
static inline int64_t  posix_min_int64(int64_t x, int64_t y)    { return x < y ? x : y; }
static inline uint8_t  posix_min_uint8(uint8_t x, uint8_t y)    { return x < y ? x : y; }
static inline uint16_t posix_min_uint16(uint16_t x, uint16_t y) { return x < y ? x : y; }
static inline uint32_t posix_min_uint32(uint32_t x, uint32_t y) { return x < y ? x : y; }
static inline uint64_t posix_min_uint64(uint64_t x, uint64_t y) { return x < y ? x : y; }
static inline fp32_t   posix_min_fp32(fp32_t x, fp32_t y)       { return x < y ? x : y; }
static inline fp64_t   posix_min_fp64(fp64_t x, fp64_t y)       { return x < y ? x : y; }

static inline void posix_min_undefined(void) { }
static inline void posix_max_undefined(void) { }

#define posix_max(X, Y) _Generic((X) + (Y), \
    int8_t:   posix_max_int8,   \
    int16_t:  posix_max_int16,  \
    int32_t:  posix_max_int32,  \
    int64_t:  posix_max_int64,  \
    uint8_t:  posix_max_uint8,  \
    uint16_t: posix_max_uint16, \
    uint32_t: posix_max_uint32, \
    uint64_t: posix_max_uint64, \
    fp32_t:   posix_max_fp32,   \
    fp64_t:   posix_max_fp64,   \
    default:  posix_max_undefined)(X, Y)

#define posix_min(X, Y) _Generic((X) + (Y), \
    int8_t:   posix_min_int8,   \
    int16_t:  posix_min_int16,  \
    int32_t:  posix_min_int32,  \
    int64_t:  posix_min_int64,  \
    uint8_t:  posix_min_uint8,  \
    uint16_t: posix_min_uint16, \
    uint32_t: posix_min_uint32, \
    uint64_t: posix_min_uint64, \
    fp32_t:   posix_min_fp32,   \
    fp64_t:   posix_min_fp64,   \
    default:  posix_min_undefined)(X, Y)

struct posix_generics_if {
    void (*test)(void);
};

extern struct posix_generics_if posix_generics;

posix_end_c

// ________________________________ posix_heap.h _________________________________

posix_begin_c

struct posix_heap;

struct posix_heap_if {
    int (*alloc)(void* *a, int64_t bytes);
    int (*alloc_zero)(void* *a, int64_t bytes);
    int (*realloc)(void* *a, int64_t bytes);
    int (*realloc_zero)(void* *a, int64_t bytes);
    void    (*free)(void* a);
    struct posix_heap* (*create)(bool serialized);
    int (*allocate)(struct posix_heap* heap, void* *a, int64_t bytes, bool zero);
    int (*reallocate)(struct posix_heap* heap, void* *a, int64_t bytes, bool zero);
    void    (*deallocate)(struct posix_heap* heap, void* a);
    int64_t (*bytes)(struct posix_heap* heap, void* a);
    void    (*dispose)(struct posix_heap* heap);
    void    (*test)(void);
};

extern struct posix_heap_if posix_heap;

posix_end_c

// _______________________________ posix_loader.h ________________________________

posix_begin_c

struct posix_loader_if {
    int32_t const local;
    int32_t const lazy;
    int32_t const now;
    int32_t const global;
    void* (*open)(const char* filename, int32_t mode);
    void* (*sym)(void* handle, const char* name);
    void  (*close)(void* handle);
    void (*test)(void);
};

extern struct posix_loader_if posix_loader;

posix_end_c

// _________________________________ posix_mem.h _________________________________

posix_begin_c

struct posix_mem_if {
    int (*map_ro)(const char* filename, void** data, int64_t* bytes);
    int (*map_rw)(const char* filename, void** data, int64_t* bytes);
    void (*unmap)(void* data, int64_t bytes);
    int  (*map_resource)(const char* label, void** data, int64_t* bytes);
    int32_t (*page_size)(void);
    int32_t (*large_page_size)(void);
    void* (*allocate)(int64_t bytes_multiple_of_page_size);
    void  (*deallocate)(void* a, int64_t bytes_multiple_of_page_size);
    void  (*test)(void);
};

extern struct posix_mem_if posix_mem;

posix_end_c

// _________________________________ posix_nls.h _________________________________

posix_begin_c

struct posix_nls_if {
    void (*init)(void);
    const char* (*locale)(void);
    int (*set_locale)(const char* locale);
    const char* (*str)(const char* defau1t);
    int32_t (*strid)(const char* s);
    const char* (*string)(int32_t strid, const char* defau1t);
};

extern struct posix_nls_if posix_nls;

posix_end_c

// _________________________________ posix_num.h _________________________________

posix_begin_c

struct posix_num128 {
    uint64_t lo;
    uint64_t hi;
};

struct posix_num_if {
    struct posix_num128 (*add128)(const struct posix_num128 a, const struct posix_num128 b);
    struct posix_num128 (*sub128)(const struct posix_num128 a, const struct posix_num128 b);
    struct posix_num128 (*mul64x64)(uint64_t a, uint64_t b);
    uint64_t (*muldiv128)(uint64_t a, uint64_t b, uint64_t d);
    uint32_t (*gcd32)(uint32_t u, uint32_t v);
    uint32_t (*random32)(uint32_t *state);
    uint64_t (*random64)(uint64_t *state);
    uint32_t (*hash32)(const char* s, int64_t bytes);
    uint64_t (*hash64)(const char* s, int64_t bytes);
    void     (*test)(void);
};

extern struct posix_num_if posix_num;

posix_end_c

// _______________________________ posix_random.h ________________________________

posix_begin_c

// Process-global PRNG backed by core's universal `struct rng` (rng_next).
// The bespoke per-state posix_num.random32/64 remain for rt parity; this is
// the convenience generator. Lazily seeded from the monotonic clock on first
// use; call posix_random_seed() for reproducibility.
void     posix_random_seed(uint64_t seed);
uint64_t posix_random(void);          // 64 random bits
fp64_t   posix_random_uniform(void);  // uniform in [0, 1)

posix_end_c

// _______________________________ posix_static.h ________________________________

posix_begin_c

// posix_static_init(unique_name) { code_to_execute_before_main }

#if defined(_MSC_VER)

#if defined(_WIN64) || defined(_M_X64)
#define _posix_msvc_symbol_prefix_ ""
#else
#define _posix_msvc_symbol_prefix_ "_"
#endif

#pragma comment(linker, "/include:posix_force_symbol_reference")

void* posix_force_symbol_reference(void* symbol);

#define _posix_msvc_ctor_(sym_prefix, func)                              \
  void func(void);                                                       \
  int32_t (* posix_array ## func)(void);                                 \
  int32_t func ## _wrapper(void);                                        \
  int32_t func ## _wrapper(void) { func();                               \
  posix_force_symbol_reference((void*)posix_array ## func);              \
  posix_force_symbol_reference((void*)func ## _wrapper); return 0; }      \
  extern int32_t (* posix_array ## func)(void);                          \
  __pragma(comment(linker, "/include:" sym_prefix # func "_wrapper"))    \
  __pragma(section(".CRT$XCU", read))                                    \
  __declspec(allocate(".CRT$XCU"))                                       \
    int32_t (* posix_array ## func)(void) = func ## _wrapper;

#define posix_static_init2_(func, line) _posix_msvc_ctor_(_posix_msvc_symbol_prefix_, \
    func ## _constructor_##line)                                         \
    void func ## _constructor_##line(void)

#define posix_static_init1_(func, line) posix_static_init2_(func, line)

#define posix_static_init(func) posix_static_init1_(func, __LINE__)

#else

#define posix_static_init(n) __attribute__((constructor)) \
        static void _init_ ## n ## __LINE__ ## _ctor(void)

#endif

void posix_static_init_test(void);

posix_end_c

// _______________________________ posix_streams.h _______________________________

posix_begin_c

struct posix_stream_if;

struct posix_stream_if {
    int (*read)(struct posix_stream_if* s, void* data, int64_t bytes,
                    int64_t *transferred);
    int (*write)(struct posix_stream_if* s, const void* data, int64_t bytes,
                     int64_t *transferred);
    void    (*close)(struct posix_stream_if* s);
};

struct posix_stream_memory_if {
    struct posix_stream_if  stream;
    const void* data_read;
    int64_t     bytes_read;
    int64_t     pos_read;
    void* data_write;
    int64_t     bytes_write;
    int64_t     pos_write;
};

struct posix_streams_if {
    void (*read_only)(struct posix_stream_memory_if* s,  const void* data, int64_t bytes);
    void (*write_only)(struct posix_stream_memory_if* s, void* data, int64_t bytes);
    void (*read_write)(struct posix_stream_memory_if* s, const void* read, int64_t read_bytes,
                                                  void* write, int64_t write_bytes);
    void (*test)(void);
};

extern struct posix_streams_if posix_streams;

posix_end_c

// ______________________________ posix_processes.h ______________________________

posix_begin_c

struct posix_processes_child {
    const char* command;
    struct posix_stream_if* in;
    struct posix_stream_if* out;
    struct posix_stream_if* err;
    uint32_t exit_code;
    fp64_t   timeout;
};

struct posix_processes_if {
    const char* (*name)(void);
    uint64_t  (*pid)(const char* name);
    int   (*pids)(const char* name, uint64_t* pids, int32_t size,
                      int32_t *count);
    int   (*nameof)(uint64_t pid, char* name, int32_t count);
    bool      (*present)(uint64_t pid);
    int   (*kill)(uint64_t pid, fp64_t timeout_seconds);
    int   (*kill_all)(const char* name, fp64_t timeout_seconds);
    bool      (*is_elevated)(void);
    int   (*restart_elevated)(void);
    int   (*run)(struct posix_processes_child* child);
    int   (*popen)(const char* command, int32_t *xc, struct posix_stream_if* output,
                       fp64_t timeout_seconds);
    int  (*spawn)(const char* command);
    void (*test)(void);
};

extern struct posix_processes_if posix_processes;

posix_end_c

// _______________________________ posix_threads.h _______________________________

posix_begin_c

typedef struct posix_event_s* posix_event_t;

struct posix_event_if {
    posix_event_t (*create)(void);
    posix_event_t (*create_manual)(void);
    void (*set)(posix_event_t e);
    void (*reset)(posix_event_t e);
    void (*wait)(posix_event_t e);
    int32_t (*wait_or_timeout)(posix_event_t e, fp64_t seconds);
    int32_t (*wait_any)(int32_t n, posix_event_t events[]);
    int32_t (*wait_any_or_timeout)(int32_t n, posix_event_t e[], fp64_t seconds);
    void (*dispose)(posix_event_t e);
    void (*test)(void);
};

extern struct posix_event_if posix_event;

struct posix_aligned_8 posix_mutex { uint8_t content[64]; };

struct posix_mutex_if {
    void (*init)(struct posix_mutex* m);
    void (*lock)(struct posix_mutex* m);
    void (*unlock)(struct posix_mutex* m);
    void (*dispose)(struct posix_mutex* m);
    void (*test)(void);
};

extern struct posix_mutex_if posix_mutex;

struct posix_thread_if {
    posix_thread_t (*start)(void (*func)(void*), void* p);
    int     (*join)(posix_thread_t thread, fp64_t timeout_seconds);
    void        (*detach)(posix_thread_t thread);
    void        (*name)(const char* name);
    void        (*realtime)(void);
    void        (*yield)(void);
    void        (*sleep_for)(fp64_t seconds);
    uint64_t    (*id_of)(posix_thread_t t);
    uint64_t    (*id)(void);
    posix_thread_t (*self)(void);
    int     (*open)(posix_thread_t* t, uint64_t id);
    void        (*close)(posix_thread_t t);
    void        (*test)(void);
};

extern struct posix_thread_if posix_thread;

posix_end_c

// ________________________________ posix_vigil.h ________________________________

posix_begin_c

#define posix_static_assertion(condition) _Static_assert(condition, #condition)

struct posix_vigil_if {
    int32_t (*failed_assertion)(const char* file, int32_t line,
        const char* func, const char* condition, const char* format, ...);
    int32_t (*fatal_termination)(const char* file, int32_t line,
        const char* func, const char* condition, const char* format, ...);
    int32_t (*fatal_if_error)(const char* file, int32_t line, const char* func,
        const char* condition, int r, const char* format, ...);
    void (*test)(void);
};

extern struct posix_vigil_if posix_vigil;

#if defined(DEBUG)
  #define posix_assert(b, ...) \
    (void)((!!(b)) || posix_vigil.failed_assertion(__FILE__, __LINE__, \
    __func__, #b, "" __VA_ARGS__))
#else
  #define posix_assert(b, ...) ((void)0)
#endif

#define posix_swear(b, ...) \
    (void)((!!(b)) || posix_vigil.failed_assertion(__FILE__, __LINE__, \
    __func__, #b, "" __VA_ARGS__))

#define posix_fatal(...) (void)(posix_vigil.fatal_termination(          \
    __FILE__, __LINE__,  __func__, "",  "" __VA_ARGS__))

#define posix_fatal_if(b, ...) \
    (void)((!(b)) || posix_vigil.fatal_termination(__FILE__, __LINE__, \
    __func__, #b, "" __VA_ARGS__))

#define posix_fatal_if_not(b, ...) \
    (void)((!!(b)) || posix_vigil.fatal_termination(__FILE__, __LINE__, \
    __func__, #b, "" __VA_ARGS__))

#define posix_not_null(e, ...) posix_fatal_if((e) == null, "" __VA_ARGS__)

#define posix_fatal_if_error(r, ...) \
    (void)(posix_vigil.fatal_if_error(__FILE__, __LINE__, __func__,      \
                                   #r, r, "" __VA_ARGS__))

#if defined(_WIN32) // posix_b2e() defined above; only valid in Win32 code
#define posix_fatal_win32err(c, ...) \
    (void)(posix_vigil.fatal_if_error(__FILE__, __LINE__, __func__,      \
                                   #c, posix_b2e(c), "" __VA_ARGS__))
#endif

posix_end_c

// ________________________________ posix_work.h _________________________________

posix_begin_c

struct posix_work;
struct posix_work_queue;

struct posix_work {
    struct posix_work_queue* queue;
    fp64_t when;
    void (*work)(struct posix_work* c);
    void* data;
    posix_event_t  done;
    struct posix_work* next;
    bool   canceled;
};

struct posix_work_queue {
    struct posix_work* head;
    int64_t    lock;
    posix_event_t changed;
};

struct posix_work_queue_if {
    void (*post)(struct posix_work* c);
    bool (*get)(struct posix_work_queue*, struct posix_work* *c);
    void (*call)(struct posix_work* c);
    void (*dispatch)(struct posix_work_queue* q);
    void (*cancel)(struct posix_work* c);
    void (*flush)(struct posix_work_queue* q);
};

extern struct posix_work_queue_if  posix_work_queue;

struct posix_worker {
    struct posix_work_queue queue;
    posix_thread_t     thread;
    posix_event_t      wake;
    volatile bool  quit;
};

struct posix_worker_if {
    void    (*start)(struct posix_worker* tq);
    void    (*post)(struct posix_worker* tq, struct posix_work* w);
    int (*join)(struct posix_worker* tq, fp64_t timeout);
    void    (*test)(void);
};

extern struct posix_worker_if posix_worker;

posix_end_c

#endif // POSIX_H

#endif // posix_definition

#if defined(posix_implementation) && !defined(posix_implementation_included)
#define posix_implementation_included
// _________________________________ posix.c __________________________________

#if !defined(_WIN32)
#define _GNU_SOURCE 1
#else
#ifndef _CRT_SECURE_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS   // strncpy/strerror/snprintf are used deliberately
#endif
#ifndef _CRT_NONSTDC_NO_WARNINGS
#define _CRT_NONSTDC_NO_WARNINGS  // fileno/_environ POSIX names
#endif
#endif

#if defined(_WIN32)

#pragma warning(push)
#pragma warning(disable: 4255) // no function prototype: '()' to '(void)'
#pragma warning(disable: 4459) // declaration hides global declaration
#pragma warning(disable: 4668) // SDK headers test version macros older SDKs lack

#pragma push_macro("UNICODE")
#define UNICODE // always because otherwise IME does not work

#include <Windows.h>
#include <Psapi.h>        // posix_loader, posix_processes
#include <shellapi.h>     // posix_processes
#include <winternl.h>     // posix_processes
#include <initguid.h>     // for known folders
#include <KnownFolders.h> // posix_files
#include <AclAPI.h>       // posix_files
#include <ShlObj_core.h>  // posix_files
#include <Shlwapi.h>      // posix_files
#include <dbghelp.h>      // posix_backtrace
#include <tlhelp32.h>     // posix_backtrace

#pragma pop_macro("UNICODE")
#pragma warning(pop)

#include <fcntl.h>
#include <intrin.h>

// POSIX names used by the portable code, mapped to MSVC equivalents:
#define strcasecmp  _stricmp
#define strncasecmp _strnicmp

// signals MSVC <signal.h> lacks (values are nominal; only stored, not raised):
#ifndef SIGTRAP
#define SIGTRAP 5
#endif
#ifndef SIGBUS
#define SIGBUS 10
#endif

#pragma comment(lib, "advapi32")
#pragma comment(lib, "ntdll")
#pragma comment(lib, "psapi")
#pragma comment(lib, "shell32")
#pragma comment(lib, "shlwapi")
#pragma comment(lib, "kernel32")
#pragma comment(lib, "user32")  // clipboard
#pragma comment(lib, "ole32")   // posix_files.known_folder CoTaskMemFree
#pragma comment(lib, "dbghelp")
#pragma comment(lib, "imagehlp")

#else

#include <stddef.h>
#include <unistd.h>
#include <locale.h>
#include <signal.h>
#include <execinfo.h>

#endif

// ________________________________ posix_args.c _________________________________

static void * posix_args_memory = null;

static void posix_args_main(int32_t argc, const char * argv[], const char ** env) {
    posix_swear(posix_args.c == 0 && posix_args.v == null && posix_args.env == null);
    posix_swear(posix_args_memory == null);
    posix_args.c = argc;
    posix_args.v = argv;
    posix_args.env = env;
}

static int32_t posix_args_option_index(const char * option) {
    int32_t ix = -1;
    for (int32_t i = 1; i < posix_args.c; i++) {
        if (strcmp(posix_args.v[i], "--") == 0) { break; }
        if (strcmp(posix_args.v[i], option) == 0) {
            ix = i;
            break;
        }
    }
    return ix;
}

static void posix_args_remove_at(int32_t ix) {
    posix_assert(0 < posix_args.c);
    posix_assert(0 < ix && ix < posix_args.c);
    for (int32_t i = ix; i < posix_args.c - 1; i++) {
        posix_args.v[i] = posix_args.v[i + 1];
    }
    posix_args.v[posix_args.c - 1] = "";
    posix_args.c--;
}

static bool posix_args_option_bool(const char * option) {
    int32_t ix = posix_args_option_index(option);
    if (ix > 0) { posix_args_remove_at(ix); }
    return ix > 0;
}

static bool posix_args_option_int(const char * option, int64_t * value) {
    int32_t ix = posix_args_option_index(option);
    if (ix > 0 && ix < posix_args.c - 1) {
        const char * s = posix_args.v[ix + 1];
        int32_t base = (strstr(s, "0x") == s || strstr(s, "0X") == s) ? 16 : 10;
        const char * b = s + (base == 10 ? 0 : 2);
        char * e = null;
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
        posix_args_remove_at(ix);
        posix_args_remove_at(ix);
    }
    return ix > 0;
}

static const char * posix_args_option_str(const char * option) {
    int32_t ix = posix_args_option_index(option);
    const char * s = null;
    if (ix > 0 && ix < posix_args.c - 1) {
        s = posix_args.v[ix + 1];
    } else {
        ix = -1;
    }
    if (ix > 0) {
        posix_args_remove_at(ix);
        posix_args_remove_at(ix);
    }
    return ix > 0 ? s : null;
}

static const char * posix_args_basename(void) {
    static char basename[256] = {0};
    posix_swear(posix_args.c > 0);
    if (basename[0] == 0) {
        const char * s = posix_args.v[0];
        const char * b = s;
        while (*s != 0) {
            if (*s == '/' || *s == '\\') { b = s + 1; }
            s++;
        }
        int32_t n = posix_str.len(b);
        posix_swear(n < posix_countof(basename));
        memcpy(basename, b, (size_t)n);
        basename[n] = 0x00;
        char * d = basename + n - 1;
        while (d > basename && *d != '.') { d--; }
        if (*d == '.') { *d = 0x00; }
    }
    return basename;
}

#if defined(_WIN32)

// Microsoft command line parsing (see CommandLineToArgvW rules):
// https://learn.microsoft.com/en-us/cpp/c-language/parsing-c-command-line-arguments

struct posix_args_pair { const char* s; char* d; const char* e; };

static struct posix_args_pair posix_args_parse_backslashes(struct posix_args_pair p) {
    enum { quote = '"', backslash = '\\' };
    const char* s = p.s;
    char* d = p.d;
    posix_swear(*s == backslash);
    int32_t bsc = 0; // number of backslashes
    while (*s == backslash) { s++; bsc++; }
    if (*s == quote) {
        while (bsc > 1 && d < p.e) { *d++ = backslash; bsc -= 2; }
        if (bsc == 1 && d < p.e) { *d++ = *s++; }
    } else {
        while (bsc > 0 && d < p.e) { *d++ = backslash; bsc--; }
    }
    return (struct posix_args_pair){ .s = s, .d = d, .e = p.e };
}

static struct posix_args_pair posix_args_parse_quoted(struct posix_args_pair p) {
    enum { quote = '"', backslash = '\\' };
    const char* s = p.s;
    char* d = p.d;
    posix_swear(*s == quote);
    s++; // opening quote (skip)
    while (*s != 0x00) {
        if (*s == backslash) {
            p = posix_args_parse_backslashes((struct posix_args_pair){
                        .s = s, .d = d, .e = p.e });
            s = p.s; d = p.d;
        } else if (*s == quote && s[1] == quote) {
            if (d < p.e) { *d++ = *s++; }
            s++; // 1 for 2 quotes
        } else if (*s == quote) {
            s++; // closing quote (skip)
            break;
        } else if (d < p.e) {
            *d++ = *s++;
        }
    }
    return (struct posix_args_pair){ .s = s, .d = d, .e = p.e };
}

static void posix_args_parse(const char* s) {
    posix_swear(s[0] != 0, "cannot parse empty string");
    posix_swear(posix_args.c == 0);
    posix_swear(posix_args.v == null);
    posix_swear(posix_args_memory == null);
    enum { quote = '"', backslash = '\\', tab = '\t', space = 0x20 };
    const int32_t len = (int32_t)strlen(s);
    const int32_t k = ((len + 2) / 2 + 1) * (int32_t)sizeof(void*) + (int32_t)sizeof(void*);
    const int32_t n = k + (len + 2) * (int32_t)sizeof(char);
    const int32_t max_argv = k / (int32_t)sizeof(void*) - 1;
    posix_fatal_if_error(posix_heap.allocate(null, &posix_args_memory, n, true));
    posix_args.c = 0;
    posix_args.v = (const char**)posix_args_memory;
    char* d = (char*)(((char*)posix_args.v) + k);
    char* e = d + n; // end of memory
    if (posix_args.c < max_argv) { posix_args.v[posix_args.c++] = d; }
    if (*s == quote) {
        s++;
        while (*s != 0x00 && *s != quote && d < e) { *d++ = *s++; }
        if (*s == quote) {
            s++;
            *d++ = 0x00;
        } else {
            while (*s != 0x00) { s++; }
        }
    } else {
        while (*s != 0x00 && *s != space && *s != tab && d < e) {
            *d++ = *s++;
        }
    }
    if (d < e) { *d++ = 0; }
    while (d < e) {
        while (*s == space || *s == tab) { s++; }
        if (*s == 0) { break; }
        if (*s == quote && s[1] == 0 && d < e) {
            if (posix_args.c < max_argv) { posix_args.v[posix_args.c++] = d; }
            *d++ = *s++;
        } else if (*s == quote) {
            if (posix_args.c < max_argv) { posix_args.v[posix_args.c++] = d; }
            struct posix_args_pair p = posix_args_parse_quoted(
                    (struct posix_args_pair){ .s = s, .d = d, .e = e });
            s = p.s; d = p.d;
        } else {
            if (posix_args.c < max_argv) { posix_args.v[posix_args.c++] = d; }
            while (*s != 0) {
                if (*s == backslash) {
                    struct posix_args_pair p = posix_args_parse_backslashes(
                            (struct posix_args_pair){ .s = s, .d = d, .e = e });
                    s = p.s; d = p.d;
                } else if (*s == quote) {
                    struct posix_args_pair p = posix_args_parse_quoted(
                            (struct posix_args_pair){ .s = s, .d = d, .e = e });
                    s = p.s; d = p.d;
                } else if (*s == tab || *s == space) {
                    break;
                } else if (d < e) {
                    *d++ = *s++;
                }
            }
        }
        if (d < e) { *d++ = 0; }
    }
    if (posix_args.c < max_argv) {
        posix_args.v[posix_args.c] = null;
    }
    posix_swear(posix_args.c < max_argv, "not enough memory - adjust guestimates");
    posix_swear(d <= e, "not enough memory - adjust guestimates");
}

static void posix_args_WinMain(void) {
    posix_swear(posix_args.c == 0 && posix_args.v == null && posix_args.env == null);
    posix_swear(posix_args_memory == null);
    const uint16_t* wcl = GetCommandLineW();
    int32_t n = (int32_t)posix_str.len16(wcl);
    char* cl = null;
    posix_fatal_if_error(posix_heap.allocate(null, (void**)&cl, n * 2 + 1, false));
    posix_str.utf16to8(cl, n * 2 + 1, wcl, -1);
    posix_args_parse(cl);
    posix_heap.deallocate(null, cl);
    posix_args.env = (const char**)(void*)_environ;
}

#endif // _WIN32

static void posix_args_fini(void) {
    if (posix_args_memory != null) { // set by posix_args_WinMain()/parse()
        posix_heap.free(posix_args_memory);
    }
    posix_args_memory = null;
    posix_args.c = 0;
    posix_args.v = null;
}

static void posix_args_test(void) {}

struct posix_args_if posix_args = {
    .main         = posix_args_main,
#if defined(_WIN32)
    .WinMain      = posix_args_WinMain,
#endif
    .option_index = posix_args_option_index,
    .remove_at    = posix_args_remove_at,
    .option_bool  = posix_args_option_bool,
    .option_int   = posix_args_option_int,
    .option_str   = posix_args_option_str,
    .basename     = posix_args_basename,
    .fini         = posix_args_fini,
    .test         = posix_args_test
};

// ________________________________ posix_core.c _________________________________

#if defined(_WIN32)

// abort does NOT call atexit() and does NOT flush streams. Also Win32 runtime
// abort() attempts to show Abort/Retry/Ignore MessageBox - thus ExitProcess()
static void posix_core_abort(void) { ExitProcess(ERROR_FATAL_APP_EXIT); }

static int  posix_core_err(void) { return (int)GetLastError(); }

static void posix_core_seterr(int err) { SetLastError((DWORD)err); }

posix_static_init(runtime) {
    SetErrorMode(SEM_FAILCRITICALERRORS | SEM_NOALIGNMENTFAULTEXCEPT |
                 SEM_NOGPFAULTERRORBOX  | SEM_NOOPENFILEERRORBOX);
}

#else

static void posix_core_abort(void) { abort(); }

static int posix_core_err(void) { return errno; }

static void posix_core_seterr(int err) { errno = err; }

#endif

static void posix_core_exit(int32_t exit_code) { exit(exit_code); }

static void posix_core_test(void) {}

struct posix_core_if posix_core = {
    .err     = posix_core_err,
    .set_err = posix_core_seterr,
    .abort   = posix_core_abort,
    .exit    = posix_core_exit,
    .test    = posix_core_test,
#if defined(_WIN32)
    .error   = {                                              // posix
        .access_denied          = ERROR_ACCESS_DENIED,        // EACCES
        .bad_file               = ERROR_BAD_FILE_TYPE,        // EBADF
        .broken_pipe            = ERROR_BROKEN_PIPE,          // EPIPE
        .device_not_ready       = ERROR_NOT_READY,            // ENXIO
        .directory_not_empty    = ERROR_DIR_NOT_EMPTY,        // ENOTEMPTY
        .disk_full              = ERROR_DISK_FULL,            // ENOSPC
        .file_exists            = ERROR_FILE_EXISTS,          // EEXIST
        .file_not_found         = ERROR_FILE_NOT_FOUND,       // ENOENT
        .insufficient_buffer    = ERROR_INSUFFICIENT_BUFFER,  // E2BIG
        .interrupted            = ERROR_OPERATION_ABORTED,    // EINTR
        .invalid_data           = ERROR_INVALID_DATA,         // EINVAL
        .invalid_handle         = ERROR_INVALID_HANDLE,       // EBADF
        .invalid_parameter      = ERROR_INVALID_PARAMETER,    // EINVAL
        .io_error               = ERROR_IO_DEVICE,            // EIO
        .more_data              = ERROR_MORE_DATA,            // ENOBUFS
        .name_too_long          = ERROR_FILENAME_EXCED_RANGE, // ENAMETOOLONG
        .no_child_process       = ERROR_NO_PROC_SLOTS,        // ECHILD
        .not_a_directory        = ERROR_DIRECTORY,            // ENOTDIR
        .not_empty              = ERROR_DIR_NOT_EMPTY,        // ENOTEMPTY
        .out_of_memory          = ERROR_OUTOFMEMORY,          // ENOMEM
        .path_not_found         = ERROR_PATH_NOT_FOUND,       // ENOENT
        .pipe_not_connected     = ERROR_PIPE_NOT_CONNECTED,   // EPIPE
        .read_only_file         = ERROR_WRITE_PROTECT,        // EROFS
        .resource_deadlock      = ERROR_LOCK_VIOLATION,       // EDEADLK
        .too_many_open_files    = ERROR_TOO_MANY_OPEN_FILES,  // EMFILE
    }
#else
    .error   = {
        .access_denied       = EACCES,
        .bad_file            = EBADF,
        .broken_pipe         = EPIPE,
        .device_not_ready    = ENXIO,
        .directory_not_empty = ENOTEMPTY,
        .disk_full           = ENOSPC,
        .file_exists         = EEXIST,
        .file_not_found      = ENOENT,
        .insufficient_buffer = E2BIG,
        .interrupted         = EINTR,
        .invalid_data        = EINVAL,
        .invalid_handle      = EBADF,
        .invalid_parameter   = EINVAL,
        .io_error            = EIO,
        .more_data           = ENOBUFS,
        .name_too_long       = ENAMETOOLONG,
        .no_child_process    = ECHILD,
        .not_a_directory     = ENOTDIR,
        .not_empty           = ENOTEMPTY,
        .out_of_memory       = ENOMEM,
        .path_not_found      = ENOENT,
        .pipe_not_connected  = EPIPE,
        .read_only_file      = EROFS,
        .resource_deadlock   = EDEADLK,
        .too_many_open_files = EMFILE,
    }
#endif
};

// ________________________________ posix_debug.c ________________________________

static int32_t posix_debug_max_file_line = 0;
static int32_t posix_debug_max_function = 0;

static void posix_debug_output(const char * s, int32_t count) {
    bool intercepted = false;
    if (posix_debug.tee != null) { intercepted = posix_debug.tee(s, count); }
    if (!intercepted) {
        // For link.exe /Subsystem:Windows stdout/stderr are often closed
        if (stderr != null && fileno(stderr) >= 0) {
            fprintf(stderr, "%s", s);
        }
#if defined(_WIN32)
        uint16_t* wide = posix_stackalloc((count + 1) * (int32_t)sizeof(uint16_t));
        posix_str.utf8to16(wide, count + 1, s, -1);
        OutputDebugStringW(wide);
#endif
    }
}

static void posix_debug_println_va(const char * file, int32_t line, const char * func,
        const char * format, va_list va) {
    if (func == null) { func = ""; }
    char file_line[1024];
    if (line == 0 && (file == null || file[0] == 0x00)) {
        file_line[0] = 0x00;
    } else {
        if (file == null) { file = ""; }
        const char * name = posix_files.basename(file);
        snprintf(file_line, posix_countof(file_line) - 1, "%s(%d):", name, line);
    }
    file_line[posix_countof(file_line) - 1] = 0x00;
    posix_debug_max_file_line = posix_max(posix_debug_max_file_line, (int32_t)strlen(file_line));
    posix_debug_max_function  = posix_max(posix_debug_max_function, (int32_t)strlen(func));
    char prefix[2048];
    snprintf(prefix, posix_countof(prefix) - 1, "%-*s %-*s",
             posix_debug_max_file_line, file_line,
             posix_debug_max_function,  func);
    prefix[posix_countof(prefix) - 1] = 0;
    char text[2048];
    if (format != null && format[0] != 0) {
        vsnprintf(text, posix_countof(text) - 1, format, va);
        text[posix_countof(text) - 1] = 0;
    } else {
        text[0] = 0;
    }
    char output[4096];
    snprintf(output, posix_countof(output) - 1, "%s %s\n", prefix, text);
    output[posix_countof(output) - 1] = 0;
    posix_debug.output(output, (int32_t)strlen(output));
}

static void posix_debug_perrno(const char * file, int32_t line,
    const char * func, int32_t err_no, const char * format, ...) {
    if (err_no != 0) {
        if (format != null && format[0] != 0) {
            va_list va;
            va_start(va, format);
            posix_debug.println_va(file, line, func, format, va);
            va_end(va);
        }
        posix_debug.println(file, line, func, "errno: %d %s", err_no, strerror(err_no));
    }
}

static void posix_debug_perror(const char * file, int32_t line,
    const char * func, int32_t error, const char * format, ...) {
    posix_debug_perrno(file, line, func, error, format);
}

static void posix_debug_println(const char * file, int32_t line, const char * func,
        const char * format, ...) {
    va_list va;
    va_start(va, format);
    posix_debug.println_va(file, line, func, format, va);
    va_end(va);
}

#if defined(_WIN32)

static bool posix_debug_is_debugger_present(void) { return IsDebuggerPresent(); }

static void posix_debug_breakpoint(void) {
    if (posix_debug.is_debugger_present()) { DebugBreak(); }
}

static int posix_debug_raise(uint32_t exception) {
    posix_core.set_err(0);
    RaiseException(exception, EXCEPTION_NONCONTINUABLE, 0, null);
    return posix_core.err();
}

#else

static bool posix_debug_is_debugger_present(void) {
    // Requires parsing /proc/self/status TracerPid on Linux, or sysctl on Darwin.
    // Stubbed to false to avoid early complexity overhead.
    return false;
}

static void posix_debug_breakpoint(void) {
    raise(SIGTRAP);
}

static int posix_debug_raise(uint32_t exception) {
    raise(exception);
    return posix_core.err();
}

#endif

static int32_t posix_debug_verbosity_from_string(const char * s) {
    char * n = null;
    long v = strtol(s, &n, 10);
    int32_t r = posix_debug.verbosity.quiet;
    if (strcasecmp(s, "quiet") == 0) {
        r = posix_debug.verbosity.quiet;
    } else if (strcasecmp(s, "info") == 0) {
        r = posix_debug.verbosity.info;
    } else if (strcasecmp(s, "verbose") == 0) {
        r = posix_debug.verbosity.verbose;
    } else if (strcasecmp(s, "debug") == 0) {
        r = posix_debug.verbosity.debug;
    } else if (strcasecmp(s, "trace") == 0) {
        r = posix_debug.verbosity.trace;
    } else if (n > s && posix_debug.verbosity.quiet <= v && v <= posix_debug.verbosity.trace) {
        r = (int32_t)v;
    } else {
        posix_fatal("invalid verbosity: %s", s);
    }
    return r;
}

static void posix_debug_test(void) {}

struct posix_debug_if posix_debug = {
    .verbosity = {
        .level   =  0,
        .quiet   =  0,
        .info    =  1,
        .verbose =  2,
        .debug   =  3,
        .trace   =  4,
    },
    .verbosity_from_string = posix_debug_verbosity_from_string,
    .tee                   = null,
    .output                = posix_debug_output,
    .println               = posix_debug_println,
    .println_va            = posix_debug_println_va,
    .perrno                = posix_debug_perrno,
    .perror                = posix_debug_perror,
    .is_debugger_present   = posix_debug_is_debugger_present,
    .breakpoint            = posix_debug_breakpoint,
    .raise                 = posix_debug_raise,
    .exception             = {
        .sig_segv = SIGSEGV,
        .sig_ill  = SIGILL,
        .sig_trap = SIGTRAP,
        .sig_fpe  = SIGFPE,
        .sig_bus  = SIGBUS,
        .sig_abrt = SIGABRT,
    },
    .test                  = posix_debug_test
};

// _________________________________ posix_str.c _________________________________

static char * posix_str_drop_const(const char * s) {
    return (char *)s;
}

static int32_t posix_str_len(const char * s) { return (int32_t)strlen(s); }

static int32_t posix_str_utf16len(const uint16_t * utf16) {
    int32_t count = 0;
    while (utf16[count] != 0) { count++; }
    return count;
}

static int32_t posix_str_utf8bytes(const char * s, int32_t b) {
    posix_assert(b >= 1);
    const uint8_t * const u = (const uint8_t *)s;
    if (b >= 1 && (u[0] & 0x80u) == 0x00u) {
        return 1;
    } else if (b > 1) {
        uint32_t c = (u[0] << 8) | u[1];
        if (0xC280 <= c && c <= 0xDFBF && (c & 0xE0C0) == 0xC080) { return 2; }
        if (b > 2) {
            c = (c << 8) | u[2];
            if (0xEDA080 <= c && c <= 0xEDBFBF) { return 0; }
            if (0xE0A080 <= c && c <= 0xEFBFBF && (c & 0xF0C0C0) == 0xE08080) { return 3; }
            if (b > 3) {
                c = (c << 8) | u[3];
                if (0xF0908080 <= c && c <= 0xF48FBFBF && (c & 0xF8C0C0C0) == 0xF0808080) { return 4; }
            }
        }
    }
    return 0;
}

static int32_t posix_str_glyphs(const char * utf8, int32_t bytes) {
    posix_swear(bytes >= 0);
    bool ok = true;
    int32_t i = 0;
    int32_t k = 1;
    while (i < bytes && ok) {
        const int32_t b = posix_str.utf8bytes(utf8 + i, bytes - i);
        ok = 0 < b && i + b <= bytes;
        if (ok) { i += b; k++; }
    }
    return ok ? k - 1 : -1;
}

static void posix_str_lower(char * d, int32_t capacity, const char * s) {
    int32_t n = posix_str.len(s);
    posix_swear(capacity > n);
    for (int32_t i = 0; i < n; i++) { d[i] = (char)tolower(s[i]); }
    d[n] = 0;
}

static void posix_str_upper(char * d, int32_t capacity, const char * s) {
    int32_t n = posix_str.len(s);
    posix_swear(capacity > n);
    for (int32_t i = 0; i < n; i++) { d[i] = (char)toupper(s[i]); }
    d[n] = 0;
}

static bool posix_str_starts(const char * s1, const char * s2) {
    int32_t n1 = (int32_t)strlen(s1);
    int32_t n2 = (int32_t)strlen(s2);
    return n1 >= n2 && memcmp(s1, s2, n2) == 0;
}

static bool posix_str_ends(const char * s1, const char * s2) {
    int32_t n1 = (int32_t)strlen(s1);
    int32_t n2 = (int32_t)strlen(s2);
    return n1 >= n2 && memcmp(s1 + n1 - n2, s2, n2) == 0;
}

static bool posix_str_i_starts(const char * s1, const char * s2) {
    int32_t n1 = (int32_t)strlen(s1);
    int32_t n2 = (int32_t)strlen(s2);
    return n1 >= n2 && strncasecmp(s1, s2, n2) == 0;
}

static bool posix_str_i_ends(const char * s1, const char * s2) {
    int32_t n1 = (int32_t)strlen(s1);
    int32_t n2 = (int32_t)strlen(s2);
    return n1 >= n2 && strncasecmp(s1 + n1 - n2, s2, n2) == 0;
}

static uint32_t posix_str_utf32(const char * utf8, int32_t bytes) {
    uint32_t utf32 = 0xFFFD;
    if (bytes >= 1 && (utf8[0] & 0x80) == 0) {
        utf32 = (uint8_t)utf8[0];
    } else if (bytes >= 2 && (utf8[0] & 0xE0) == 0xC0 && (utf8[1] & 0xC0) == 0x80) {
        utf32  = (uint32_t)(utf8[0] & 0x1F) << 6;
        utf32 |= (uint32_t)(utf8[1] & 0x3F);
    } else if (bytes >= 3 && (utf8[0] & 0xF0) == 0xE0 && (utf8[1] & 0xC0) == 0x80 && (utf8[2] & 0xC0) == 0x80) {
        utf32  = (uint32_t)(utf8[0] & 0x0F) << 12;
        utf32 |= (uint32_t)(utf8[1] & 0x3F) <<  6;
        utf32 |= (uint32_t)(utf8[2] & 0x3F);
    } else if (bytes >= 4 && (utf8[0] & 0xF8) == 0xF0 && (utf8[1] & 0xC0) == 0x80 && (utf8[2] & 0xC0) == 0x80 && (utf8[3] & 0xC0) == 0x80) {
        utf32  = (uint32_t)(utf8[0] & 0x07) << 18;
        utf32 |= (uint32_t)(utf8[1] & 0x3F) << 12;
        utf32 |= (uint32_t)(utf8[2] & 0x3F) <<  6;
        utf32 |= (uint32_t)(utf8[3] & 0x3F);
    }
    return utf32;
}

static int32_t posix_str_utf8_bytes(const uint16_t * utf16, int32_t chars) {
    if (chars == 0) { return 0; }
    if (chars < 0 && utf16[0] == 0x0000) { return 1; }
    int32_t req = 0;
    int32_t limit = chars < 0 ? INT32_MAX : chars;
    for (int32_t i = 0; i < limit; i++) {
        if (chars < 0 && utf16[i] == 0) { req++; break; }
        uint16_t c = utf16[i];
        if (c < 0x0080) {
            req += 1;
        } else if (c < 0x0800) {
            req += 2;
        } else if ((c & 0xFC00) == 0xD800) {
            i++;
            if (i >= limit || (utf16[i] & 0xFC00) != 0xDC00) { return -1; }
            req += 4;
        } else if ((c & 0xFC00) == 0xDC00) {
            return -1;
        } else {
            req += 3;
        }
    }
    return req;
}

static int32_t posix_str_utf16_chars(const char * utf8, int32_t bytes) {
    if (bytes == 0) { return 0; }
    if (bytes < 0 && utf8[0] == 0x00) { return 1; }
    int32_t req = 0;
    int32_t i = 0;
    int32_t limit = bytes < 0 ? INT32_MAX : bytes;
    while (i < limit) {
        if (bytes < 0 && utf8[i] == 0) { req++; break; }
        int32_t b = posix_str_utf8bytes(utf8 + i, limit - i);
        if (b <= 0) { return -1; }
        uint32_t cp = posix_str_utf32(utf8 + i, b);
        req += (cp > 0xFFFF) ? 2 : 1;
        i += b;
    }
    return req;
}

static int posix_str_utf16to8(char * utf8, int32_t capacity, const uint16_t * utf16, int32_t chars) {
    if (chars == 0) { return 0; }
    if (chars < 0 && utf16[0] == 0x0000) {
        posix_swear(capacity >= 1);
        utf8[0] = 0x00;
        return 0;
    }
    int32_t limit = chars < 0 ? INT32_MAX : chars;
    int32_t o = 0;
    for (int32_t i = 0; i < limit; i++) {
        if (chars < 0 && utf16[i] == 0) {
            if (o < capacity) { utf8[o++] = 0; }
            break;
        }
        uint16_t c = utf16[i];
        if (c < 0x0080) {
            if (o < capacity) { utf8[o++] = (char)c; }
        } else if (c < 0x0800) {
            if (o + 1 < capacity) {
                utf8[o++] = (char)(0xC0 | (c >> 6));
                utf8[o++] = (char)(0x80 | (c & 0x3F));
            }
        } else if ((c & 0xFC00) == 0xD800) {
            i++;
            if (i < limit) {
                uint32_t cp = 0x10000 + (((c & 0x3FF) << 10) | (utf16[i] & 0x3FF));
                if (o + 3 < capacity) {
                    utf8[o++] = (char)(0xF0 | (cp >> 18));
                    utf8[o++] = (char)(0x80 | ((cp >> 12) & 0x3F));
                    utf8[o++] = (char)(0x80 | ((cp >> 6) & 0x3F));
                    utf8[o++] = (char)(0x80 | (cp & 0x3F));
                }
            }
        } else {
            if (o + 2 < capacity) {
                utf8[o++] = (char)(0xE0 | (c >> 12));
                utf8[o++] = (char)(0x80 | ((c >> 6) & 0x3F));
                utf8[o++] = (char)(0x80 | (c & 0x3F));
            }
        }
    }
    return 0;
}

static int posix_str_utf8to16(uint16_t * utf16, int32_t capacity, const char * utf8, int32_t bytes) {
    if (bytes == 0) { return 0; }
    if (bytes < 0 && utf8[0] == 0x00) {
        posix_swear(capacity >= 1);
        utf16[0] = 0x0000;
        return 0;
    }
    int32_t i = 0;
    int32_t o = 0;
    int32_t limit = bytes < 0 ? INT32_MAX : bytes;
    while (i < limit) {
        if (bytes < 0 && utf8[i] == 0) {
            if (o < capacity) { utf16[o++] = 0; }
            break;
        }
        int32_t b = posix_str_utf8bytes(utf8 + i, limit - i);
        if (b <= 0) { return EINVAL; }
        uint32_t cp = posix_str_utf32(utf8 + i, b);
        if (cp > 0xFFFF) {
            cp -= 0x10000;
            if (o + 1 < capacity) {
                utf16[o++] = (uint16_t)(0xD800 | (cp >> 10));
                utf16[o++] = (uint16_t)(0xDC00 | (cp & 0x3FF));
            }
        } else {
            if (o < capacity) {
                utf16[o++] = (uint16_t)cp;
            }
        }
        i += b;
    }
    return 0;
}

static bool posix_str_utf16_is_low_surrogate(uint16_t utf16char) {
    return 0xDC00 <= utf16char && utf16char <= 0xDFFF;
}

static bool posix_str_utf16_is_high_surrogate(uint16_t utf16char) {
    return 0xD800 <= utf16char && utf16char <= 0xDBFF;
}

static void posix_str_format_va(char * utf8, int32_t count, const char * format, va_list va) {
    vsnprintf(utf8, (size_t)count, format, va);
    utf8[count - 1] = 0;
}

static void posix_str_format(char * utf8, int32_t count, const char * format, ...) {
    va_list va;
    va_start(va, format);
    posix_str.format_va(utf8, count, format, va);
    va_end(va);
}

#if defined(_WIN32)

static struct posix_str1024 posix_str_error_for_language(int32_t error, uint16_t language) {
    DWORD flags = FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS;
    HMODULE module = null;
    HRESULT hr = 0 <= error && error <= 0xFFFF ?
        HRESULT_FROM_WIN32((uint32_t)error) : (HRESULT)error;
    if ((error & 0xC0000000U) == 0xC0000000U) {
        static HMODULE ntdll; // RtlNtStatusToDosError implies linking to ntdll
        if (ntdll == null) { ntdll = GetModuleHandleA("ntdll.dll"); }
        if (ntdll == null) { ntdll = LoadLibraryA("ntdll.dll"); }
        module = ntdll;
        hr = HRESULT_FROM_WIN32(RtlNtStatusToDosError((NTSTATUS)error));
        flags |= FORMAT_MESSAGE_FROM_HMODULE;
    }
    struct posix_str1024 text;
    uint16_t utf16[posix_countof(text.s)];
    DWORD count = FormatMessageW(flags, module, hr, language,
            utf16, posix_countof(utf16) - 1, (va_list*)null);
    utf16[posix_countof(utf16) - 1] = 0; // always
    if (count > 0) {
        posix_swear(count < posix_countof(utf16));
        utf16[count] = 0;
        int32_t k = (int32_t)count;
        if (k > 0 && utf16[k - 1] == '\n') { utf16[k - 1] = 0; }
        k = (int32_t)posix_str.len16(utf16);
        if (k > 0 && utf16[k - 1] == '\r') { utf16[k - 1] = 0; }
        char message[posix_countof(text.s)];
        const int32_t bytes = posix_str.utf8_bytes(utf16, -1);
        if (bytes >= posix_countof(message)) {
            posix_str_printf(message, "error message is too long: %d bytes", bytes);
        } else {
            posix_str.utf16to8(message, posix_countof(message), utf16, -1);
        }
        posix_str_printf(text.s, "0x%08X(%d) \"%s\"", error, error, message);
    } else {
        posix_str_printf(text.s, "0x%08X(%d)", error, error);
    }
    return text;
}

static struct posix_str1024 posix_str_error(int32_t error) {
    const uint16_t language = MAKELANGID(LANG_ENGLISH, SUBLANG_DEFAULT);
    return posix_str_error_for_language(error, language);
}

static struct posix_str1024 posix_str_error_nls(int32_t error) {
    const uint16_t language = MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT);
    return posix_str_error_for_language(error, language);
}

#else

static struct posix_str1024 posix_str_error(int32_t error) {
    struct posix_str1024 text;
    snprintf(text.s, posix_countof(text.s), "0x%08X(%d) \"%s\"", error, error, strerror(error));
    return text;
}

static struct posix_str1024 posix_str_error_nls(int32_t error) {
    return posix_str_error(error);
}

#endif

static const char * posix_str_grouping_separator(void) {
    struct lconv * locale_info = localeconv();
    const char * sep = locale_info->thousands_sep;
    return (sep != null && sep[0] != 0) ? sep : "";
}

static struct posix_str64 posix_str_int64_dg(int64_t v, bool uint, const char * gs) {
    const int32_t m = (int32_t)strlen(gs);
    posix_swear(m < 5);
    struct posix_str64 text;
    enum { max_text_bytes = posix_countof(text.s) };
    int64_t abs64 = v < 0 ? -v : v;
    uint64_t n = uint ? (uint64_t)v : (v != INT64_MIN ? (uint64_t)abs64 : (uint64_t)INT64_MIN);
    int32_t i = 0;
    int32_t groups[8];
    do {
        groups[i] = n % 1000;
        n = n / 1000;
        i++;
    } while (n > 0);
    const int32_t gc = i - 1;
    char * s = text.s;
    if (v < 0 && !uint) { *s++ = '-'; }
    int32_t r = max_text_bytes - 1;
    while (i > 0) {
        i--;
        posix_assert(r > 3 + m);
        if (i == gc) {
            posix_str.format(s, r, "%d%s", groups[i], gc > 0 ? gs : "");
        } else {
            posix_str.format(s, r, "%03d%s", groups[i], i > 0 ? gs : "");
        }
        int32_t k = (int32_t)strlen(s);
        r -= k;
        s += k;
    }
    *s = 0;
    return text;
}

static struct posix_str64 posix_str_int64(int64_t v) {
    return posix_str_int64_dg(v, false, "\xE2\x80\x89"); // Thin space
}

static struct posix_str64 posix_str_uint64(uint64_t v) {
    return posix_str_int64_dg(v, true, "\xE2\x80\x89"); // Thin space
}

static struct posix_str64 posix_str_int64_lc(int64_t v) {
    return posix_str_int64_dg(v, false, posix_str_grouping_separator());
}

static struct posix_str64 posix_str_uint64_lc(uint64_t v) {
    return posix_str_int64_dg(v, true, posix_str_grouping_separator());
}

static struct posix_str128 posix_str_fp(const char * format, fp64_t v) {
    struct lconv * locale_info = localeconv();
    const char * decimal_separator = locale_info->decimal_point;
    if (decimal_separator == null || decimal_separator[0] == 0) { decimal_separator = "."; }
    struct posix_str128 f;
    f.s[0] = 0x00;
    posix_str.format(f.s, posix_countof(f.s), format, v);
    f.s[posix_countof(f.s) - 1] = 0x00;
    struct posix_str128 text;
    char * s = f.s;
    char * d = text.s;
    while (*s != 0x00) {
        if (*s == '.') {
            const char * sep = decimal_separator;
            while (*sep != 0x00) { *d++ = *sep++; }
            s++;
        } else {
            *d++ = *s++;
        }
    }
    *d = 0x00;
    return text;
}

static void posix_str_test(void) {}

struct posix_str_if posix_str = {
    .drop_const              = posix_str_drop_const,
    .len                     = posix_str_len,
    .len16                   = posix_str_utf16len,
    .utf8bytes               = posix_str_utf8bytes,
    .glyphs                  = posix_str_glyphs,
    .lower                   = posix_str_lower,
    .upper                   = posix_str_upper,
    .starts                  = posix_str_starts,
    .ends                    = posix_str_ends,
    .istarts                 = posix_str_i_starts,
    .iends                   = posix_str_i_ends,
    .utf8_bytes              = posix_str_utf8_bytes,
    .utf16_chars             = posix_str_utf16_chars,
    .utf16to8                = posix_str_utf16to8,
    .utf8to16                = posix_str_utf8to16,
    .utf16_is_low_surrogate  = posix_str_utf16_is_low_surrogate,
    .utf16_is_high_surrogate = posix_str_utf16_is_high_surrogate,
    .utf32                   = posix_str_utf32,
    .format                  = posix_str_format,
    .format_va               = posix_str_format_va,
    .error                   = posix_str_error,
    .error_nls               = posix_str_error_nls,
    .grouping_separator      = posix_str_grouping_separator,
    .int64_dg                = posix_str_int64_dg,
    .int64                   = posix_str_int64,
    .uint64                  = posix_str_uint64,
    .int64_lc                = posix_str_int64_lc,
    .uint64_lc               = posix_str_uint64_lc,
    .fp                      = posix_str_fp,
    .test                    = posix_str_test
};

// ________________________________ posix_vigil.c ________________________________

static void posix_vigil_breakpoint_and_abort(void) {
    posix_debug.breakpoint();
    posix_core.abort();
}

static int32_t posix_vigil_failed_assertion(const char * file, int32_t line,
        const char * func, const char * condition, const char * format, ...) {
    va_list va;
    va_start(va, format);
    posix_debug.println_va(file, line, func, format, va);
    va_end(va);
    posix_debug.println(file, line, func, "assertion failed: %s\n", condition);
    posix_vigil_breakpoint_and_abort();
    return 0;
}

static int32_t posix_vigil_fatal_termination_va(const char * file, int32_t line,
        const char * func, const char * condition, int r,
        const char * format, va_list va) {
    const int32_t er = posix_core.err();
    posix_debug.println_va(file, line, func, format, va);
    if (r != er && r != 0) {
        posix_debug.perror(file, line, func, r, "");
    }
    if (er != 0) { posix_debug.perror(file, line, func, er, ""); }
    if (condition != null && condition[0] != 0) {
        posix_debug.println(file, line, func, "FATAL: %s\n", condition);
    } else {
        posix_debug.println(file, line, func, "FATAL\n");
    }
    posix_vigil_breakpoint_and_abort();
    return 0;
}

static int32_t posix_vigil_fatal_termination(const char * file, int32_t line,
        const char * func, const char * condition, const char * format, ...) {
    va_list va;
    va_start(va, format);
    posix_vigil_fatal_termination_va(file, line, func, condition, 0, format, va);
    va_end(va);
    return 0;
}

static int32_t posix_vigil_fatal_if_error(const char * file, int32_t line,
    const char * func, const char * condition, int r,
    const char * format, ...) {
    if (r != 0) {
        va_list va;
        va_start(va, format);
        posix_vigil_fatal_termination_va(file, line, func, condition, r, format, va);
        va_end(va);
    }
    return 0;
}

static void posix_vigil_test(void) {}

struct posix_vigil_if posix_vigil = {
    .failed_assertion  = posix_vigil_failed_assertion,
    .fatal_termination = posix_vigil_fatal_termination,
    .fatal_if_error    = posix_vigil_fatal_if_error,
    .test              = posix_vigil_test
};




#if !defined(_WIN32)
#include <stdatomic.h>
#include <pthread.h>
#include <time.h>
#include <sched.h>
#include <sys/time.h>
#include <errno.h>
#endif

// _______________________________ posix_atomics.c _______________________________

#if defined(_WIN32)

static int32_t posix_atomics_increment_int32(volatile int32_t* a) {
    return InterlockedIncrement((volatile LONG*)a);
}

static int32_t posix_atomics_decrement_int32(volatile int32_t* a) {
    return InterlockedDecrement((volatile LONG*)a);
}

static int64_t posix_atomics_increment_int64(volatile int64_t* a) {
    return InterlockedIncrement64((__int64 volatile*)a);
}

static int64_t posix_atomics_decrement_int64(volatile int64_t* a) {
    return InterlockedDecrement64((__int64 volatile*)a);
}

static int32_t posix_atomics_add_int32(volatile int32_t* a, int32_t v) {
    return InterlockedAdd((LONG volatile*)a, v);
}

static int64_t posix_atomics_add_int64(volatile int64_t* a, int64_t v) {
    return InterlockedAdd64((__int64 volatile*)a, v);
}

static int64_t posix_atomics_exchange_int64(volatile int64_t* a, int64_t v) {
    return (int64_t)InterlockedExchange64((LONGLONG*)a, (LONGLONG)v);
}

static int32_t posix_atomics_exchange_int32(volatile int32_t* a, int32_t v) {
    return (int32_t)InterlockedExchange((volatile LONG*)a, (unsigned long)v);
}

static bool posix_atomics_compare_exchange_int64(volatile int64_t* a,
        int64_t comparand, int64_t v) {
    return (int64_t)InterlockedCompareExchange64((LONGLONG*)a,
        (LONGLONG)v, (LONGLONG)comparand) == comparand;
}

static bool posix_atomics_compare_exchange_int32(volatile int32_t* a,
        int32_t comparand, int32_t v) {
    return (int64_t)InterlockedCompareExchange((LONG*)a,
        (LONG)v, (LONG)comparand) == comparand;
}

static void posix_atomics_memory_fence(void) { MemoryBarrier(); }

static void* posix_atomics_exchange_ptr(volatile void** a, void* v) {
    posix_static_assertion(sizeof(void*) == sizeof(int64_t));
    return (void*)(intptr_t)posix_atomics_exchange_int64((int64_t*)a, (int64_t)v);
}

static bool posix_atomics_compare_exchange_ptr(volatile void** a, void* comparand, void* v) {
    posix_static_assertion(sizeof(void*) == sizeof(int64_t));
    return posix_atomics_compare_exchange_int64((int64_t*)a,
        (int64_t)comparand, (int64_t)v);
}

static void posix_atomics_spinlock_acquire(volatile int64_t* spinlock) {
    while (_InterlockedCompareExchange64(spinlock, 1, 0) != 0) {
        while (*spinlock) { YieldProcessor(); }
    }
    posix_atomics_memory_fence();
    posix_assert(*spinlock == 1);
}

static void posix_atomics_spinlock_release(volatile int64_t* spinlock) {
    posix_assert(*spinlock == 1);
    *spinlock = 0;
    posix_atomics_memory_fence();
}

static int32_t posix_atomics_load_int32(volatile int32_t* a) { return *a; }

static int64_t posix_atomics_load_int64(volatile int64_t* a) { return *a; }

#else

static void* posix_atomics_exchange_ptr(volatile void** a, void* v) {
    return atomic_exchange((volatile _Atomic(void*)*)a, v);
}

static int32_t posix_atomics_increment_int32(volatile int32_t* a) {
    return atomic_fetch_add((volatile _Atomic int32_t*)a, 1) + 1;
}

static int32_t posix_atomics_decrement_int32(volatile int32_t* a) {
    return atomic_fetch_sub((volatile _Atomic int32_t*)a, 1) - 1;
}

static int64_t posix_atomics_increment_int64(volatile int64_t* a) {
    return atomic_fetch_add((volatile _Atomic int64_t*)a, 1) + 1;
}

static int64_t posix_atomics_decrement_int64(volatile int64_t* a) {
    return atomic_fetch_sub((volatile _Atomic int64_t*)a, 1) - 1;
}

static int32_t posix_atomics_add_int32(volatile int32_t* a, int32_t v) {
    return atomic_fetch_add((volatile _Atomic int32_t*)a, v) + v;
}

static int64_t posix_atomics_add_int64(volatile int64_t* a, int64_t v) {
    return atomic_fetch_add((volatile _Atomic int64_t*)a, v) + v;
}

static int32_t posix_atomics_exchange_int32(volatile int32_t* a, int32_t v) {
    return atomic_exchange((volatile _Atomic int32_t*)a, v);
}

static int64_t posix_atomics_exchange_int64(volatile int64_t* a, int64_t v) {
    return atomic_exchange((volatile _Atomic int64_t*)a, v);
}

static bool posix_atomics_compare_exchange_int64(volatile int64_t* a, int64_t comparand, int64_t v) {
    return atomic_compare_exchange_strong((volatile _Atomic int64_t*)a, &comparand, v);
}

static bool posix_atomics_compare_exchange_int32(volatile int32_t* a, int32_t comparand, int32_t v) {
    return atomic_compare_exchange_strong((volatile _Atomic int32_t*)a, &comparand, v);
}

static bool posix_atomics_compare_exchange_ptr(volatile void** a, void* comparand, void* v) {
    return atomic_compare_exchange_strong((volatile _Atomic(void*)*)a, &comparand, v);
}

static void posix_atomics_memory_fence(void) {
    atomic_thread_fence(memory_order_seq_cst);
}

static void posix_atomics_spinlock_acquire(volatile int64_t* spinlock) {
    while (!posix_atomics_compare_exchange_int64(spinlock, 0, 1)) {
        while (*spinlock) {
#if defined(__aarch64__) || defined(__arm__)
            __asm__ volatile("yield" ::: "memory");
#elif defined(__x86_64__) || defined(__i386__)
            __asm__ volatile("pause" ::: "memory");
#endif
        }
    }
    posix_atomics_memory_fence();
}

static void posix_atomics_spinlock_release(volatile int64_t* spinlock) {
    posix_assert(*spinlock == 1);
    *spinlock = 0;
    posix_atomics_memory_fence();
}

static int32_t posix_atomics_load_int32(volatile int32_t* a) {
    return atomic_load_explicit((volatile _Atomic int32_t*)a, memory_order_acquire);
}

static int64_t posix_atomics_load_int64(volatile int64_t* a) {
    return atomic_load_explicit((volatile _Atomic int64_t*)a, memory_order_acquire);
}

#endif // _WIN32

static void posix_atomics_test(void) {}

struct posix_atomics_if posix_atomics = {
    .exchange_ptr           = posix_atomics_exchange_ptr,
    .increment_int32        = posix_atomics_increment_int32,
    .decrement_int32        = posix_atomics_decrement_int32,
    .increment_int64        = posix_atomics_increment_int64,
    .decrement_int64        = posix_atomics_decrement_int64,
    .add_int32              = posix_atomics_add_int32,
    .add_int64              = posix_atomics_add_int64,
    .exchange_int32         = posix_atomics_exchange_int32,
    .exchange_int64         = posix_atomics_exchange_int64,
    .compare_exchange_int64 = posix_atomics_compare_exchange_int64,
    .compare_exchange_int32 = posix_atomics_compare_exchange_int32,
    .compare_exchange_ptr   = posix_atomics_compare_exchange_ptr,
    .spinlock_acquire       = posix_atomics_spinlock_acquire,
    .spinlock_release       = posix_atomics_spinlock_release,
    .load32                 = posix_atomics_load_int32,
    .load64                 = posix_atomics_load_int64,
    .memory_fence           = posix_atomics_memory_fence,
    .test                   = posix_atomics_test
};

// _______________________________ posix_win32.c _________________________________

#if defined(_WIN32)

void posix_win32_close_handle(void* h) {
    #pragma warning(suppress: 6001) // shut up overzealous IntelliSense
    posix_fatal_win32err(CloseHandle((HANDLE)h));
}

// WAIT_ABANDONED only reported for mutexes not events.
// WAIT_FAILED means event was invalid handle or disposed by another thread
// while the calling thread was waiting for it.
int posix_wait_ix2e(uint32_t r) {
    const int32_t ix = (int32_t)r;
    return (int)(
          (int32_t)WAIT_OBJECT_0 <= ix && ix <= WAIT_OBJECT_0 + 63 ? 0 :
          (ix == WAIT_ABANDONED ? ERROR_REQUEST_ABORTED :
            (ix == WAIT_TIMEOUT ? ERROR_TIMEOUT :
              (ix == WAIT_FAILED) ? posix_core.err() : ERROR_INVALID_HANDLE
            )
          )
    );
}

#endif // _WIN32

// _______________________________ posix_threads.c _______________________________

// --- Mutexes ---

#if defined(_WIN32)

posix_static_assertion(sizeof(CRITICAL_SECTION) <= sizeof(struct posix_mutex));

static void posix_mutex_init(struct posix_mutex* m) {
    CRITICAL_SECTION* cs = (CRITICAL_SECTION*)m;
    posix_fatal_win32err(InitializeCriticalSectionAndSpinCount(cs, 4096));
}

static void posix_mutex_lock(struct posix_mutex* m) {
    EnterCriticalSection((CRITICAL_SECTION*)m);
}

static void posix_mutex_unlock(struct posix_mutex* m) {
    LeaveCriticalSection((CRITICAL_SECTION*)m);
}

static void posix_mutex_dispose(struct posix_mutex* m) {
    DeleteCriticalSection((CRITICAL_SECTION*)m);
}

#else

static void posix_mutex_init(struct posix_mutex* m) {
    pthread_mutexattr_t attr;
    pthread_mutexattr_init(&attr);
    pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);
    int r = pthread_mutex_init((pthread_mutex_t*)m->content, &attr);
    posix_fatal_if_error(r);
    pthread_mutexattr_destroy(&attr);
}

static void posix_mutex_lock(struct posix_mutex* m) {
    pthread_mutex_lock((pthread_mutex_t*)m->content);
}

static void posix_mutex_unlock(struct posix_mutex* m) {
    pthread_mutex_unlock((pthread_mutex_t*)m->content);
}

static void posix_mutex_dispose(struct posix_mutex* m) {
    pthread_mutex_destroy((pthread_mutex_t*)m->content);
}

#endif // _WIN32

static void posix_mutex_test(void) {}

struct posix_mutex_if posix_mutex = {
    .init    = posix_mutex_init,
    .lock    = posix_mutex_lock,
    .unlock  = posix_mutex_unlock,
    .dispose = posix_mutex_dispose,
    .test    = posix_mutex_test
};

// --- Events ---

#if defined(_WIN32)

static posix_event_t posix_event_create(void) {
    HANDLE e = CreateEvent(null, false, false, null);
    posix_not_null(e);
    return (posix_event_t)e;
}

static posix_event_t posix_event_create_manual(void) {
    HANDLE e = CreateEvent(null, true, false, null);
    posix_not_null(e);
    return (posix_event_t)e;
}

static void posix_event_set(posix_event_t e) {
    posix_fatal_win32err(SetEvent((HANDLE)e));
}

static void posix_event_reset(posix_event_t e) {
    posix_fatal_win32err(ResetEvent((HANDLE)e));
}

static int32_t posix_event_wait_or_timeout(posix_event_t e, fp64_t seconds) {
    uint32_t ms = seconds < 0 ? INFINITE : (uint32_t)(seconds * 1000.0 + 0.5);
    DWORD i = WaitForSingleObject((HANDLE)e, ms);
    posix_swear(i != WAIT_FAILED, "i: %d", i);
    int r = posix_wait_ix2e(i);
    if (r != 0) { posix_swear(i == WAIT_TIMEOUT || i == WAIT_ABANDONED); }
    return i == WAIT_TIMEOUT ? -1 : (i == WAIT_ABANDONED ? -2 : (int32_t)i);
}

static void posix_event_wait(posix_event_t e) { posix_event_wait_or_timeout(e, -1); }

static int32_t posix_event_wait_any_or_timeout(int32_t n,
        posix_event_t events[], fp64_t s) {
    posix_swear(n < 64); // Win32 API limit
    const uint32_t ms = s < 0 ? INFINITE : (uint32_t)(s * 1000.0 + 0.5);
    const HANDLE* es = (const HANDLE*)events;
    DWORD i = WaitForMultipleObjects((DWORD)n, es, false, ms);
    posix_swear(i != WAIT_FAILED, "i: %d", i);
    int r = posix_wait_ix2e(i);
    if (r != 0) { posix_swear(i == WAIT_TIMEOUT || i == WAIT_ABANDONED); }
    return i == WAIT_TIMEOUT ? -1 : (i == WAIT_ABANDONED ? -2 : (int32_t)i);
}

static int32_t posix_event_wait_any(int32_t n, posix_event_t e[]) {
    return posix_event_wait_any_or_timeout(n, e, -1);
}

static void posix_event_dispose(posix_event_t h) {
    posix_win32_close_handle((HANDLE)h);
}

#else

struct posix_event_impl {
    pthread_mutex_t mutex;
    pthread_cond_t cond;
    bool signaled;
    bool manual_reset;
};

static posix_event_t posix_event_create_base(bool manual) {
    struct posix_event_impl* e = (struct posix_event_impl*)malloc(sizeof(struct posix_event_impl));
    posix_not_null(e);
    pthread_mutex_init(&e->mutex, null);
    pthread_cond_init(&e->cond, null);
    e->signaled = false;
    e->manual_reset = manual;
    return (posix_event_t)e;
}

static posix_event_t posix_event_create(void) { return posix_event_create_base(false); }
static posix_event_t posix_event_create_manual(void) { return posix_event_create_base(true); }

static void posix_event_set(posix_event_t e_handle) {
    struct posix_event_impl* e = (struct posix_event_impl*)e_handle;
    pthread_mutex_lock(&e->mutex);
    e->signaled = true;
    pthread_cond_broadcast(&e->cond);
    pthread_mutex_unlock(&e->mutex);
}

static void posix_event_reset(posix_event_t e_handle) {
    struct posix_event_impl* e = (struct posix_event_impl*)e_handle;
    pthread_mutex_lock(&e->mutex);
    e->signaled = false;
    pthread_mutex_unlock(&e->mutex);
}

static int32_t posix_event_wait_or_timeout(posix_event_t e_handle, fp64_t seconds) {
    struct posix_event_impl* e = (struct posix_event_impl*)e_handle;
    pthread_mutex_lock(&e->mutex);
    int32_t result = 0;
    
    if (seconds < 0) {
        while (!e->signaled) {
            pthread_cond_wait(&e->cond, &e->mutex);
        }
    } else {
        struct timespec ts;
        clock_gettime(CLOCK_REALTIME, &ts);
        uint64_t nsec = ts.tv_nsec + (uint64_t)(seconds * 1e9);
        ts.tv_sec += nsec / 1000000000ULL;
        ts.tv_nsec = nsec % 1000000000ULL;
        
        while (!e->signaled) {
            int r = pthread_cond_timedwait(&e->cond, &e->mutex, &ts);
            if (r == ETIMEDOUT && !e->signaled) {
                result = -1;
                break;
            }
        }
    }
    
    if (result == 0 && !e->manual_reset) {
        e->signaled = false;
    }
    pthread_mutex_unlock(&e->mutex);
    return result;
}

static void posix_event_wait(posix_event_t e) { posix_event_wait_or_timeout(e, -1.0); }

static int32_t posix_event_wait_any_or_timeout(int32_t n, posix_event_t events[], fp64_t seconds) {
    // POSIX lacks WaitForMultipleObjects. We use a high-frequency polling fallback 
    // to keep dependencies zero. Fine for UI/Work threads.
    struct timespec req = { .tv_sec = 0, .tv_nsec = 1000000 }; // 1ms
    fp64_t start = -1;
    
    if (seconds >= 0) {
        struct timeval tv;
        gettimeofday(&tv, null);
        start = tv.tv_sec + (tv.tv_usec / 1000000.0);
    }

    while (true) {
        for (int32_t i = 0; i < n; i++) {
            struct posix_event_impl* e = (struct posix_event_impl*)events[i];
            pthread_mutex_lock(&e->mutex);
            if (e->signaled) {
                if (!e->manual_reset) { e->signaled = false; }
                pthread_mutex_unlock(&e->mutex);
                return i;
            }
            pthread_mutex_unlock(&e->mutex);
        }

        if (seconds >= 0) {
            struct timeval tv;
            gettimeofday(&tv, null);
            fp64_t now = tv.tv_sec + (tv.tv_usec / 1000000.0);
            if (now - start >= seconds) { return -1; }
        }
        nanosleep(&req, null);
    }
}

static int32_t posix_event_wait_any(int32_t n, posix_event_t events[]) {
    return posix_event_wait_any_or_timeout(n, events, -1.0);
}

static void posix_event_dispose(posix_event_t e_handle) {
    struct posix_event_impl* e = (struct posix_event_impl*)e_handle;
    pthread_cond_destroy(&e->cond);
    pthread_mutex_destroy(&e->mutex);
    free(e);
}

#endif // _WIN32

static void posix_event_test(void) {}

struct posix_event_if posix_event = {
    .create              = posix_event_create,
    .create_manual       = posix_event_create_manual,
    .set                 = posix_event_set,
    .reset               = posix_event_reset,
    .wait                = posix_event_wait,
    .wait_or_timeout     = posix_event_wait_or_timeout,
    .wait_any            = posix_event_wait_any,
    .wait_any_or_timeout = posix_event_wait_any_or_timeout,
    .dispose             = posix_event_dispose,
    .test                = posix_event_test
};

// --- Threads ---

#if defined(_WIN32)

static posix_thread_t posix_thread_start(void (*func)(void*), void* p) {
    posix_thread_t t = (posix_thread_t)CreateThread(null, 0,
        (LPTHREAD_START_ROUTINE)(void*)func, p, 0, null);
    posix_not_null(t);
    return t;
}

static bool posix_thread_handle_valid(void* h) {
    DWORD flags = 0;
    return GetHandleInformation(h, &flags);
}

static int posix_thread_join(posix_thread_t t, fp64_t timeout) {
    posix_not_null(t);
    posix_fatal_if(!posix_thread_handle_valid((HANDLE)t));
    const uint32_t ms = timeout < 0 ? INFINITE : (uint32_t)(timeout * 1000.0 + 0.5);
    DWORD ix = WaitForSingleObject((HANDLE)t, (DWORD)ms);
    int r = posix_wait_ix2e(ix);
    if (r == 0) {
        posix_win32_close_handle((HANDLE)t);
    } else {
        posix_println("failed to join thread %p %s", t, posix_strerr(r));
    }
    return r;
}

static void posix_thread_detach(posix_thread_t t) {
    posix_not_null(t);
    posix_fatal_if(!posix_thread_handle_valid((HANDLE)t));
    posix_win32_close_handle((HANDLE)t);
}

static void posix_thread_name(const char* name) {
    uint16_t stack[128];
    posix_fatal_if(posix_str.len(name) >= posix_countof(stack), "name too long: %s", name);
    posix_str.utf8to16(stack, posix_countof(stack), name, -1);
    HRESULT r = SetThreadDescription(GetCurrentThread(), stack);
    posix_fatal_if(!SUCCEEDED(r));
}

static void posix_thread_realtime(void) {
    posix_fatal_win32err(SetPriorityClass(GetCurrentProcess(),
        REALTIME_PRIORITY_CLASS));
    posix_fatal_win32err(SetThreadPriority(GetCurrentThread(),
        THREAD_PRIORITY_TIME_CRITICAL));
    posix_fatal_win32err(SetThreadPriorityBoost(GetCurrentThread(), false));
}

static void posix_thread_yield(void) { SwitchToThread(); }

static void posix_thread_sleep_for(fp64_t seconds) {
    if (seconds < 0) { seconds = 0; }
    Sleep((DWORD)(seconds * 1000.0 + 0.5));
}

static uint64_t posix_thread_id_of(posix_thread_t t) {
    return (uint64_t)GetThreadId((HANDLE)t);
}

static uint64_t posix_thread_id(void) {
    return (uint64_t)GetThreadId(GetCurrentThread());
}

static posix_thread_t posix_thread_self(void) {
    return (posix_thread_t)GetCurrentThread(); // pseudo handle
}

static int posix_thread_open(posix_thread_t* t, uint64_t id) {
    *t = (posix_thread_t)OpenThread(THREAD_ALL_ACCESS, false, (DWORD)id);
    return *t == null ? posix_core.err() : 0;
}

static void posix_thread_close(posix_thread_t t) {
    posix_not_null(t);
    posix_win32_close_handle((HANDLE)t);
}

#else

struct posix_thread_args {
    void (*func)(void*);
    void* arg;
};

static void* posix_thread_wrapper(void* p) {
    struct posix_thread_args* args = (struct posix_thread_args*)p;
    void (*func)(void*) = args->func;
    void* arg = args->arg;
    free(args);
    func(arg);
    return null;
}

static posix_thread_t posix_thread_start(void (*func)(void*), void* p) {
    pthread_t* t = (pthread_t*)malloc(sizeof(pthread_t));
    struct posix_thread_args* args = (struct posix_thread_args*)malloc(sizeof(struct posix_thread_args));
    args->func = func;
    args->arg = p;
    posix_fatal_if_error(pthread_create(t, null, posix_thread_wrapper, args));
    return (posix_thread_t)t;
}

static int posix_thread_join(posix_thread_t thread, fp64_t timeout_seconds) {
    posix_not_null(thread);
    pthread_t* t = (pthread_t*)thread;
    
    // POSIX lacks pthread_timedjoin_np on macOS. We'll do standard blocking join
    // unless timeout is utilized (warn via trace).
    if (timeout_seconds >= 0) {
        posix_debug.println(__FILE__, __LINE__, __func__, "Warning: Timed join fallback to blocking join on POSIX");
    }
    
    int r = pthread_join(*t, null);
    free(t);
    return r;
}

static void posix_thread_detach(posix_thread_t thread) {
    posix_not_null(thread);
    pthread_t* t = (pthread_t*)thread;
    pthread_detach(*t);
    free(t);
}

static void posix_thread_name(const char* name) {
#if defined(__APPLE__)
    pthread_setname_np(name);
#elif defined(__linux__)
    pthread_setname_np(pthread_self(), name);
#endif
}

static void posix_thread_realtime(void) {
    struct sched_param param;
    param.sched_priority = sched_get_priority_max(SCHED_RR);
    // Ignore error, as this usually requires root/sudo capabilities on Linux
    pthread_setschedparam(pthread_self(), SCHED_RR, &param);
}

static void posix_thread_yield(void) { sched_yield(); }

static void posix_thread_sleep_for(fp64_t seconds) {
    if (seconds <= 0) return;
    struct timespec ts;
    ts.tv_sec = (time_t)seconds;
    ts.tv_nsec = (long)((seconds - ts.tv_sec) * 1e9);
    nanosleep(&ts, null);
}

static uint64_t posix_thread_id_of(posix_thread_t t) {
    return (uint64_t)(*(pthread_t*)t);
}

static uint64_t posix_thread_id(void) {
    return (uint64_t)pthread_self();
}

static posix_thread_t posix_thread_self(void) {
    pthread_t* t = (pthread_t*)malloc(sizeof(pthread_t));
    *t = pthread_self();
    return (posix_thread_t)t;
}

static int posix_thread_open(posix_thread_t* t, uint64_t id) {
    pthread_t* th = (pthread_t*)malloc(sizeof(pthread_t));
    *th = (pthread_t)id;
    *t = (posix_thread_t)th;
    return 0;
}

static void posix_thread_close(posix_thread_t t) {
    free((pthread_t*)t);
}

#endif // _WIN32

static void posix_thread_test(void) {}

struct posix_thread_if posix_thread = {
    .start     = posix_thread_start,
    .join      = posix_thread_join,
    .detach    = posix_thread_detach,
    .name      = posix_thread_name,
    .realtime  = posix_thread_realtime,
    .yield     = posix_thread_yield,
    .sleep_for = posix_thread_sleep_for,
    .id_of     = posix_thread_id_of,
    .id        = posix_thread_id,
    .self      = posix_thread_self,
    .open      = posix_thread_open,
    .close     = posix_thread_close,
    .test      = posix_thread_test
};

// ________________________________ posix_work.c _________________________________

static void posix_work_queue_no_duplicates(struct posix_work* w) {
    struct posix_work* e = w->queue->head;
    bool found = false;
    while (e != null && !found) {
        found = (e == w);
        if (!found) { e = e->next; }
    }
    posix_swear(!found);
}

static void posix_work_queue_post(struct posix_work* w) {
    posix_assert(w->queue != null && w != null && w->when >= 0.0);
    struct posix_work_queue* q = w->queue;
    posix_atomics.spinlock_acquire(&q->lock);
    posix_work_queue_no_duplicates(w); 
    struct posix_work* p = null;
    struct posix_work* e = q->head;
    while (e != null && e->when <= w->when) {
        p = e;
        e = e->next;
    }
    w->next = e;
    bool head = (p == null);
    if (head) {
        q->head = w;
    } else {
        p->next = w;
    }
    posix_atomics.spinlock_release(&q->lock);
    if (head && q->changed != null) { posix_event.set(q->changed); }
}

static void posix_work_queue_cancel(struct posix_work* w) {
    if (w != null && w->queue != null && !w->canceled) {
        struct posix_work_queue* q = w->queue;
        posix_atomics.spinlock_acquire(&q->lock);
        struct posix_work* p = null;
        struct posix_work* e = q->head;
        bool changed = false; 
        while (e != null && !w->canceled) {
            if (e == w) {
                changed = (p == null);
                if (changed) {
                    q->head = e->next;
                } else {
                    p->next = e->next;
                }
                e->next = null;
                e->canceled = true;
            } else {
                p = e;
                e = e->next;
            }
        }
        posix_atomics.spinlock_release(&q->lock);
        if (w->canceled && w->done != null) { posix_event.set(w->done); }
        if (changed && q->changed != null) { posix_event.set(q->changed); }
    }
}

static void posix_work_queue_flush(struct posix_work_queue* q) {
    while (q->head != null) { posix_work_queue.cancel(q->head); }
}

static bool posix_work_queue_get(struct posix_work_queue* q, struct posix_work* *r) {
    struct posix_work* w = null;
    posix_atomics.spinlock_acquire(&q->lock);
    fp64_t now = posix_clock.seconds();
    bool changed = (q->head != null && q->head->when <= now);
    if (changed) {
        w = q->head;
        q->head = w->next;
        w->next = null;
    }
    posix_atomics.spinlock_release(&q->lock);
    *r = w;
    if (changed && q->changed != null) { posix_event.set(q->changed); }
    return w != null;
}

static void posix_work_queue_call(struct posix_work* w) {
    if (w->work != null) { w->work(w); }
    if (w->done != null) { posix_event.set(w->done); }
}

static void posix_work_queue_dispatch(struct posix_work_queue* q) {
    struct posix_work* w = null;
    while (posix_work_queue.get(q, &w)) { posix_work_queue.call(w); }
}

struct posix_work_queue_if posix_work_queue = {
    .post     = posix_work_queue_post,
    .get      = posix_work_queue_get,
    .call     = posix_work_queue_call,
    .dispatch = posix_work_queue_dispatch,
    .cancel   = posix_work_queue_cancel,
    .flush    = posix_work_queue_flush
};

static void posix_worker_thread(void* p) {
    posix_thread.name("worker");
    struct posix_worker* worker = (struct posix_worker*)p;
    struct posix_work_queue* q = &worker->queue;
    while (!worker->quit) {
        posix_work_queue.dispatch(q);
        fp64_t timeout = -1.0;
        posix_atomics.spinlock_acquire(&q->lock);
        
        if (q->head != null) {
            fp64_t now = posix_clock.seconds();
            timeout = posix_max(0.0, q->head->when - now);
        }
        posix_atomics.spinlock_release(&q->lock);
        
        if (!worker->quit && timeout != 0) {
            posix_event.wait_or_timeout(worker->wake, timeout);
        }
    }
    posix_work_queue.dispatch(q);
}

static void posix_worker_start(struct posix_worker* worker) {
    posix_assert(worker->wake == null && !worker->quit);
    worker->wake  = posix_event.create();
    worker->queue = (struct posix_work_queue){
        .head = null, .lock = 0, .changed = worker->wake
    };
    worker->thread = posix_thread.start(posix_worker_thread, worker);
}

static int posix_worker_join(struct posix_worker* worker, fp64_t to) {
    worker->quit = true;
    posix_event.set(worker->wake);
    int r = posix_thread.join(worker->thread, to);
    if (r == 0) {
        posix_event.dispose(worker->wake);
        worker->wake = null;
        worker->thread = null;
        worker->quit = false;
        posix_swear(worker->queue.head == null);
    }
    return r;
}

static void posix_worker_post(struct posix_worker* worker, struct posix_work* w) {
    posix_assert(!worker->quit && worker->wake != null && worker->thread != null);
    w->queue = &worker->queue;
    posix_work_queue.post(w);
}

static void posix_worker_test(void) {}

struct posix_worker_if posix_worker = {
    .start = posix_worker_start,
    .post  = posix_worker_post,
    .join  = posix_worker_join,
    .test  = posix_worker_test
};



#if !defined(_WIN32)
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>
#include <dirent.h>
#include <pwd.h>
#include <limits.h>

#if defined(__APPLE__)
#include <malloc/malloc.h>
#elif defined(__linux__)
#include <malloc.h>
#endif
#endif // !_WIN32

// ________________________________ posix_heap.c _________________________________

#if defined(_WIN32)

// Win32 heap: HEAP_ZERO_MEMORY makes HeapReAlloc zero exactly the grown
// region (the heap tracks the old size), so realloc_zero grows correctly.

static HANDLE posix_heap_or_process_heap(struct posix_heap* h) {
    static HANDLE process_heap;
    if (process_heap == null) { process_heap = GetProcessHeap(); }
    return h != null ? (HANDLE)h : process_heap;
}

static int posix_heap_allocate(struct posix_heap* h, void* *p, int64_t bytes, bool zero) {
    posix_swear(bytes > 0);
    #ifdef DEBUG
        static bool enabled;
        if (!enabled) {
            enabled = true;
            HeapSetInformation(null, HeapEnableTerminationOnCorruption, null, 0);
        }
    #endif
    const DWORD flags = zero ? HEAP_ZERO_MEMORY : 0;
    *p = HeapAlloc(posix_heap_or_process_heap(h), flags, (SIZE_T)bytes);
    return *p == null ? ENOMEM : 0;
}

static int posix_heap_reallocate(struct posix_heap* h, void* *p, int64_t bytes, bool zero) {
    posix_swear(bytes > 0);
    const DWORD flags = zero ? HEAP_ZERO_MEMORY : 0;
    void* a = *p == null ? // HeapReAlloc(..., null, bytes) may not work
        HeapAlloc(posix_heap_or_process_heap(h), flags, (SIZE_T)bytes) :
        HeapReAlloc(posix_heap_or_process_heap(h), flags, *p, (SIZE_T)bytes);
    if (a != null) { *p = a; }
    return a == null ? ENOMEM : 0;
}

static void posix_heap_deallocate(struct posix_heap* h, void* a) {
    posix_fatal_win32err(HeapFree(posix_heap_or_process_heap(h), 0, a));
}

static int64_t posix_heap_bytes(struct posix_heap* h, void* a) {
    SIZE_T bytes = HeapSize(posix_heap_or_process_heap(h), 0, a);
    posix_fatal_if(bytes == (SIZE_T)-1);
    return (int64_t)bytes;
}

static struct posix_heap* posix_heap_create(bool serialized) {
    const DWORD options = serialized ? 0 : HEAP_NO_SERIALIZE;
    return (struct posix_heap*)HeapCreate(options, 0, 0);
}

static void posix_heap_dispose(struct posix_heap* h) {
    posix_fatal_win32err(HeapDestroy((HANDLE)h));
}

#else // POSIX

static int64_t posix_heap_bytes(struct posix_heap* heap, void* a) {
    (void)heap;
    if (a == null) { return 0; }
#if defined(__APPLE__)
    return (int64_t)malloc_size(a);
#elif defined(__linux__)
    return (int64_t)malloc_usable_size(a);
#else
    return 0; // Unsupported platform
#endif
}

static int posix_heap_allocate(struct posix_heap* heap, void* *p, int64_t bytes, bool zero) {
    (void)heap;
    *p = zero ? calloc(1, (size_t)bytes) : malloc((size_t)bytes);
    return *p == null ? ENOMEM : 0;
}

static int posix_heap_reallocate(struct posix_heap* heap, void* *p, int64_t bytes, bool zero) {
    (void)heap;
    // realloc() does not zero grown memory; zero [old_usable, bytes) ourselves.
    int64_t old = (zero && *p != null) ? posix_heap_bytes(null, *p) : 0;
    void* a = realloc(*p, (size_t)bytes);
    if (a == null) { return ENOMEM; }
    if (zero && bytes > old) {
        memset((uint8_t*)a + old, 0x00, (size_t)(bytes - old));
    }
    *p = a;
    return 0;
}

static void posix_heap_deallocate(struct posix_heap* heap, void* a) {
    (void)heap;
    free(a);
}

static struct posix_heap* posix_heap_create(bool serialized) {
    (void)serialized;
    return (struct posix_heap*)1; // dummy handle; malloc is thread-safe
}

static void posix_heap_dispose(struct posix_heap* heap) {
    (void)heap;
}

#endif // _WIN32

// thin wrappers over the process heap, shared by both platforms:

static int posix_heap_alloc(void* *a, int64_t bytes) {
    return posix_heap_allocate(null, a, bytes, false);
}

static int posix_heap_alloc_zero(void* *a, int64_t bytes) {
    return posix_heap_allocate(null, a, bytes, true);
}

static int posix_heap_realloc(void* *a, int64_t bytes) {
    return posix_heap_reallocate(null, a, bytes, false);
}

static int posix_heap_realloc_zero(void* *a, int64_t bytes) {
    return posix_heap_reallocate(null, a, bytes, true);
}

static void posix_heap_free(void* a) {
    posix_heap_deallocate(null, a);
}

static void posix_heap_test(void) {}

struct posix_heap_if posix_heap = {
    .alloc        = posix_heap_alloc,
    .alloc_zero   = posix_heap_alloc_zero,
    .realloc      = posix_heap_realloc,
    .realloc_zero = posix_heap_realloc_zero,
    .free         = posix_heap_free,
    .create       = posix_heap_create,
    .allocate     = posix_heap_allocate,
    .reallocate   = posix_heap_reallocate,
    .deallocate   = posix_heap_deallocate,
    .bytes        = posix_heap_bytes,
    .dispose      = posix_heap_dispose,
    .test         = posix_heap_test
};

// _________________________________ posix_mem.c _________________________________

#if defined(_WIN32)

static int posix_mem_map_view_of_file(HANDLE file, void** data, int64_t* bytes, bool rw) {
    int r = 0;
    void* address = null;
    HANDLE mapping = CreateFileMapping(file, null,
        rw ? PAGE_READWRITE : PAGE_READONLY,
        (uint32_t)(*bytes >> 32), (uint32_t)*bytes, null);
    if (mapping == null) {
        r = posix_core.err();
    } else {
        DWORD access = rw ? FILE_MAP_ALL_ACCESS : FILE_MAP_READ;
        address = MapViewOfFile(mapping, access, 0, 0, (SIZE_T)*bytes);
        if (address == null) { r = posix_core.err(); }
        posix_win32_close_handle(mapping);
    }
    if (r == 0) { *data = address; } else { *data = null; *bytes = 0; }
    return r;
}

static int posix_mem_set_token_privilege(void* token, const char* name, bool e) {
    TOKEN_PRIVILEGES tp = { .PrivilegeCount = 1 };
    tp.Privileges[0].Attributes = e ? SE_PRIVILEGE_ENABLED : 0;
    posix_fatal_win32err(LookupPrivilegeValueA(null, name, &tp.Privileges[0].Luid));
    return posix_b2e(AdjustTokenPrivileges(token, false, &tp,
               sizeof(TOKEN_PRIVILEGES), null, null));
}

static int posix_mem_adjust_process_privilege_manage_volume_name(void) {
    const uint32_t access = TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY;
    const HANDLE process = GetCurrentProcess();
    HANDLE token = null;
    int r = posix_b2e(OpenProcessToken(process, access, &token));
    if (r == 0) {
        r = posix_mem_set_token_privilege(token, "SeManageVolumePrivilege", true);
        posix_win32_close_handle(token);
    }
    return r;
}

static int posix_mem_map_file(const char* filename, void** data,
        int64_t* bytes, bool rw) {
    if (rw) { (void)posix_mem_adjust_process_privilege_manage_volume_name(); }
    int r = 0;
    const DWORD access = GENERIC_READ | (rw ? GENERIC_WRITE : 0);
    const DWORD share = FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE;
    const DWORD disposition = rw ? OPEN_ALWAYS : OPEN_EXISTING;
    const DWORD flags = FILE_ATTRIBUTE_NORMAL;
    HANDLE file = CreateFileA(filename, access, share, null, disposition, flags, null);
    if (file == INVALID_HANDLE_VALUE) {
        r = posix_core.err();
    } else {
        LARGE_INTEGER eof = { .QuadPart = 0 };
        posix_fatal_win32err(GetFileSizeEx(file, &eof));
        if (rw && *bytes > eof.QuadPart) {
            const LARGE_INTEGER size = { .QuadPart = *bytes };
            r = r != 0 ? r : posix_b2e(SetFilePointerEx(file, size, null, FILE_BEGIN));
            r = r != 0 ? r : posix_b2e(SetEndOfFile(file));
            r = r != 0 ? r : posix_b2e(SetFileValidData(file, *bytes));
            if (r == ERROR_PRIVILEGE_NOT_HELD) { r = 0; } // ignore
            const LARGE_INTEGER zero = { .QuadPart = 0 };
            r = r != 0 ? r : posix_b2e(SetFilePointerEx(file, zero, null, FILE_BEGIN));
        } else {
            *bytes = eof.QuadPart;
        }
        r = r != 0 ? r : posix_mem_map_view_of_file(file, data, bytes, rw);
        posix_win32_close_handle(file);
    }
    return r;
}

static int posix_mem_map_ro(const char* filename, void** data, int64_t* bytes) {
    return posix_mem_map_file(filename, data, bytes, false);
}

static int posix_mem_map_rw(const char* filename, void** data, int64_t* bytes) {
    return posix_mem_map_file(filename, data, bytes, true);
}

static void posix_mem_unmap(void* data, int64_t bytes) {
    posix_assert(data != null && bytes > 0);
    (void)bytes;
    if (data != null && bytes > 0) { posix_fatal_win32err(UnmapViewOfFile(data)); }
}

static int posix_mem_map_resource(const char* label, void** data, int64_t* bytes) {
    HRSRC res = FindResourceA(null, label, (const char*)RT_RCDATA);
    if (res != null) { *bytes = SizeofResource(null, res); }
    HGLOBAL g = res != null ? LoadResource(null, res) : null;
    *data = g != null ? LockResource(g) : null;
    return *data != null ? 0 : posix_core.err();
}

static int32_t posix_mem_page_size(void) {
    static SYSTEM_INFO system_info;
    if (system_info.dwPageSize == 0) { GetSystemInfo(&system_info); }
    return (int32_t)system_info.dwPageSize;
}

static int32_t posix_mem_large_page_size(void) {
    static SIZE_T large_page_minimum = 0;
    if (large_page_minimum == 0) { large_page_minimum = GetLargePageMinimum(); }
    return (int32_t)large_page_minimum;
}

static void* posix_mem_allocate(int64_t bytes_multiple_of_page_size) {
    posix_assert(bytes_multiple_of_page_size > 0);
    SIZE_T bytes = (SIZE_T)bytes_multiple_of_page_size;
    SIZE_T page_size = (SIZE_T)posix_mem_page_size();
    posix_assert(bytes % page_size == 0);
    int r = 0;
    void* a = null;
    if (bytes_multiple_of_page_size < 0 || bytes % page_size != 0) {
        SetLastError(ERROR_INVALID_PARAMETER);
        r = ERROR_INVALID_PARAMETER;
    } else {
        const DWORD type = MEM_COMMIT | MEM_RESERVE;
        a = VirtualAlloc(null, bytes, type | MEM_PHYSICAL, PAGE_READWRITE);
        if (a == null) { a = VirtualAlloc(null, bytes, type, PAGE_READWRITE); }
        if (a == null) {
            r = posix_core.err();
            posix_println("VirtualAlloc(%lld) failed %s", (int64_t)bytes, posix_strerr(r));
        } else {
            r = VirtualLock(a, bytes) ? 0 : posix_core.err();
            if (r == ERROR_WORKING_SET_QUOTA) {
                SIZE_T min_mem = 0, max_mem = 0;
                r = posix_b2e(GetProcessWorkingSetSize(GetCurrentProcess(), &min_mem, &max_mem));
                if (r == 0) {
                    max_mem = max_mem + bytes * 2;
                    max_mem = (max_mem + page_size - 1) / page_size * page_size +
                               page_size * 16;
                    if (min_mem < max_mem) { min_mem = max_mem; }
                    r = posix_b2e(SetProcessWorkingSetSize(GetCurrentProcess(),
                            min_mem, max_mem));
                    if (r == 0) { r = posix_b2e(VirtualLock(a, bytes)); }
                }
            }
            if (r != 0) { // locking failed but the allocation is usable
                posix_println("VirtualLock(%lld) failed %s", (int64_t)bytes, posix_strerr(r));
            }
        }
    }
    return a;
}

static void posix_mem_deallocate(void* a, int64_t bytes_multiple_of_page_size) {
    posix_assert(bytes_multiple_of_page_size > 0);
    SIZE_T bytes = (SIZE_T)bytes_multiple_of_page_size;
    SIZE_T page_size = (SIZE_T)posix_mem_page_size();
    if (bytes_multiple_of_page_size < 0 || bytes % page_size != 0) {
        posix_println("failed %s", posix_strerr(ERROR_INVALID_PARAMETER));
    } else if (a != null) {
        (void)posix_b2e(VirtualUnlock(a, bytes));
        int r = posix_b2e(VirtualFree(a, 0, MEM_RELEASE));
        if (r != 0) { posix_println("VirtualFree() failed %s", posix_strerr(r)); }
    }
}

#else

static int posix_mem_map_ro(const char* filename, void** data, int64_t* bytes) {
    int fd = open(filename, O_RDONLY);
    if (fd < 0) { return errno; }
    struct stat st;
    if (fstat(fd, &st) < 0) {
        close(fd);
        return errno;
    }
    *bytes = (int64_t)st.st_size;
    *data = mmap(null, (size_t)*bytes, PROT_READ, MAP_PRIVATE, fd, 0);
    close(fd);
    if (*data == MAP_FAILED) {
        *data = null;
        *bytes = 0;
        return errno;
    }
    return 0;
}

static int posix_mem_map_rw(const char* filename, void** data, int64_t* bytes) {
    int fd = open(filename, O_RDWR | O_CREAT, 0666);
    if (fd < 0) { return errno; }
    struct stat st;
    if (fstat(fd, &st) < 0) {
        close(fd);
        return errno;
    }
    if (st.st_size < *bytes) {
        if (ftruncate(fd, (off_t)*bytes) < 0) {
            close(fd);
            return errno;
        }
    } else {
        *bytes = (int64_t)st.st_size;
    }
    *data = mmap(null, (size_t)*bytes, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    close(fd);
    if (*data == MAP_FAILED) {
        *data = null;
        *bytes = 0;
        return errno;
    }
    return 0;
}

static void posix_mem_unmap(void* data, int64_t bytes) {
    if (data != null && data != MAP_FAILED && bytes > 0) {
        munmap(data, (size_t)bytes);
    }
}

static int posix_mem_map_resource(const char* label, void** data, int64_t* bytes) {
    (void)label;
    (void)data;
    (void)bytes;
    return ENOSYS; // No direct POSIX equivalent to Win32 RT_RCDATA
}

static int32_t posix_mem_page_size(void) {
    return (int32_t)sysconf(_SC_PAGESIZE);
}

static int32_t posix_mem_large_page_size(void) {
    // Varies by architecture (typically 2MB on x86_64).
    return 2 * 1024 * 1024;
}

static void* posix_mem_allocate(int64_t bytes_multiple_of_page_size) {
    posix_assert(bytes_multiple_of_page_size > 0);
    void* a = mmap(null, (size_t)bytes_multiple_of_page_size, 
                   PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    return a == MAP_FAILED ? null : a;
}

static void posix_mem_deallocate(void* a, int64_t bytes_multiple_of_page_size) {
    if (a != null && a != MAP_FAILED) {
        munmap(a, (size_t)bytes_multiple_of_page_size);
    }
}

#endif // _WIN32

static void posix_mem_test(void) {}

struct posix_mem_if posix_mem = {
    .map_ro          = posix_mem_map_ro,
    .map_rw          = posix_mem_map_rw,
    .unmap           = posix_mem_unmap,
    .map_resource    = posix_mem_map_resource,
    .page_size       = posix_mem_page_size,
    .large_page_size = posix_mem_large_page_size,
    .allocate        = posix_mem_allocate,
    .deallocate      = posix_mem_deallocate,
    .test            = posix_mem_test
};

// ________________________________ posix_files.c ________________________________

#if defined(_WIN32)

posix_static_assertion(SEEK_SET == FILE_BEGIN);
posix_static_assertion(SEEK_CUR == FILE_CURRENT);
posix_static_assertion(SEEK_END == FILE_END);

#ifndef O_SYNC
#define O_SYNC (0x10000)
#endif

static int posix_files_open(struct posix_file* *file, const char* fn, int32_t f) {
    DWORD access = (f & posix_files.o_wr) ? GENERIC_WRITE :
                   (f & posix_files.o_rw) ? GENERIC_READ | GENERIC_WRITE :
                                            GENERIC_READ;
    access |= (f & posix_files.o_append) ? FILE_APPEND_DATA : 0;
    DWORD disposition =
        (f & posix_files.o_create) ? ((f & posix_files.o_excl)  ? CREATE_NEW :
                                      (f & posix_files.o_trunc) ? CREATE_ALWAYS :
                                                                  OPEN_ALWAYS) :
            (f & posix_files.o_trunc) ? TRUNCATE_EXISTING : OPEN_EXISTING;
    const DWORD share = FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE;
    DWORD attr = FILE_ATTRIBUTE_NORMAL;
    attr |= (f & O_SYNC) ? FILE_FLAG_WRITE_THROUGH : 0;
    *file = (struct posix_file*)CreateFileA(fn, access, share, null, disposition, attr, null);
    return *file != (struct posix_file*)INVALID_HANDLE_VALUE ? 0 : posix_core.err();
}

static bool posix_files_is_valid(struct posix_file* file) {
    return file != posix_files.invalid && file != null;
}

static int posix_files_seek(struct posix_file* file, int64_t *position, int32_t method) {
    LARGE_INTEGER distance_to_move = { .QuadPart = *position };
    LARGE_INTEGER p = { 0 };
    int r = posix_b2e(SetFilePointerEx((HANDLE)file, distance_to_move, &p, (DWORD)method));
    if (r == 0) { *position = p.QuadPart; }
    return r;
}

static inline uint64_t posix_files_ft_to_us(FILETIME ft) { // microseconds
    return (ft.dwLowDateTime | (((uint64_t)ft.dwHighDateTime) << 32)) / 10;
}

static int64_t posix_files_a2t(DWORD a) {
    int64_t type = 0;
    if (a & FILE_ATTRIBUTE_REPARSE_POINT) { type |= posix_files.type_symlink; }
    if (a & FILE_ATTRIBUTE_DIRECTORY)     { type |= posix_files.type_folder; }
    if (a & FILE_ATTRIBUTE_DEVICE)        { type |= posix_files.type_device; }
    return type;
}

static posix_thread_local int32_t posix_files_stat_depth;

static int posix_files_stat(struct posix_file* file, struct posix_files_stat* s,
                            bool follow_symlink) {
    enum { max_symlink_depth = 32 };
    int r = 0;
    BY_HANDLE_FILE_INFORMATION fi;
    posix_fatal_win32err(GetFileInformationByHandle((HANDLE)file, &fi));
    const bool symlink = (fi.dwFileAttributes & FILE_ATTRIBUTE_REPARSE_POINT) != 0;
    if (follow_symlink && symlink && posix_files_stat_depth >= max_symlink_depth) {
        r = (int)ERROR_TOO_MANY_LINKS;
    } else if (follow_symlink && symlink) {
        const DWORD flags = FILE_NAME_NORMALIZED | VOLUME_NAME_DOS;
        DWORD n = GetFinalPathNameByHandleA((HANDLE)file, null, 0, flags);
        if (n == 0) {
            r = posix_core.err();
        } else {
            char* name = null;
            r = posix_heap.allocate(null, (void**)&name, (int64_t)n + 2, false);
            if (r == 0) {
                n = GetFinalPathNameByHandleA((HANDLE)file, name, n + 1, flags);
                if (n == 0) {
                    r = posix_core.err();
                } else {
                    struct posix_file* f = posix_files.invalid;
                    r = posix_files.open(&f, name, posix_files.o_rd);
                    if (r == 0) {
                        posix_files_stat_depth++;
                        r = posix_files.stat(f, s, follow_symlink);
                        posix_files_stat_depth--;
                        posix_files.close(f);
                    }
                }
                posix_heap.deallocate(null, name);
            }
        }
    } else {
        s->size = (int64_t)((uint64_t)fi.nFileSizeLow |
                          (((uint64_t)fi.nFileSizeHigh) << 32));
        s->created  = posix_files_ft_to_us(fi.ftCreationTime);
        s->accessed = posix_files_ft_to_us(fi.ftLastAccessTime);
        s->updated  = posix_files_ft_to_us(fi.ftLastWriteTime);
        s->type = posix_files_a2t(fi.dwFileAttributes);
    }
    return r;
}

static int posix_files_read(struct posix_file* file, void* data, int64_t bytes, int64_t *transferred) {
    int r = 0;
    *transferred = 0;
    while (bytes > 0 && r == 0) {
        DWORD chunk_size = (DWORD)(bytes > UINT32_MAX ? UINT32_MAX : bytes);
        DWORD bytes_read = 0;
        r = posix_b2e(ReadFile((HANDLE)file, data, chunk_size, &bytes_read, null));
        if (r == 0) {
            *transferred += bytes_read;
            bytes -= bytes_read;
            data = (uint8_t*)data + bytes_read;
            if (bytes_read == 0) { break; } // EOF
        }
    }
    return r;
}

static int posix_files_write(struct posix_file* file, const void* data, int64_t bytes, int64_t *transferred) {
    int r = 0;
    *transferred = 0;
    while (bytes > 0 && r == 0) {
        DWORD chunk_size = (DWORD)(bytes > UINT32_MAX ? UINT32_MAX : bytes);
        DWORD bytes_written = 0;
        r = posix_b2e(WriteFile((HANDLE)file, data, chunk_size, &bytes_written, null));
        if (r == 0) {
            *transferred += bytes_written;
            bytes -= bytes_written;
            data = (const uint8_t*)data + bytes_written;
        }
    }
    return r;
}

static int posix_files_flush(struct posix_file* file) {
    return posix_b2e(FlushFileBuffers((HANDLE)file));
}

static void posix_files_close(struct posix_file* file) {
    posix_win32_close_handle((HANDLE)file);
}

static int posix_files_write_fully(const char* filename, const void* data,
                                 int64_t bytes, int64_t *transferred) {
    if (transferred != null) { *transferred = 0; }
    int r = 0;
    const DWORD access = GENERIC_WRITE;
    const DWORD share = FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE;
    const DWORD flags = FILE_ATTRIBUTE_NORMAL | FILE_FLAG_WRITE_THROUGH;
    HANDLE file = CreateFileA(filename, access, share, null, CREATE_ALWAYS, flags, null);
    if (file == INVALID_HANDLE_VALUE) {
        r = posix_core.err();
    } else {
        int64_t written = 0;
        const uint8_t* p = (const uint8_t*)data;
        while (r == 0 && bytes > 0) {
            uint64_t write = bytes >= UINT32_MAX ?
                (uint64_t)(UINT32_MAX) - 0xFFFFuLL : (uint64_t)bytes;
            DWORD chunk = 0;
            r = posix_b2e(WriteFile(file, p, (DWORD)write, &chunk, null));
            written += chunk;
            bytes -= chunk;
            p += chunk;
        }
        if (transferred != null) { *transferred = written; }
        int rc = posix_b2e(FlushFileBuffers(file));
        if (r == 0) { r = rc; }
        posix_win32_close_handle(file);
    }
    return r;
}

static bool posix_files_exists(const char* path) { return PathFileExistsA(path); }

static bool posix_files_is_folder(const char* path) { return PathIsDirectoryA(path); }

static bool posix_files_is_symlink(const char* filename) {
    DWORD attributes = GetFileAttributesA(filename);
    return attributes != INVALID_FILE_ATTRIBUTES &&
          (attributes & FILE_ATTRIBUTE_REPARSE_POINT) != 0;
}

static const char* posix_files_basename(const char* pathname) {
    const char* bn = strrchr(pathname, '\\');
    if (bn == null) { bn = strrchr(pathname, '/'); }
    return bn != null ? bn + 1 : pathname;
}

static int posix_files_unlink(const char* pathname) {
    if (posix_files.is_folder(pathname)) {
        return posix_b2e(RemoveDirectoryA(pathname));
    } else {
        return posix_b2e(DeleteFileA(pathname));
    }
}

static int posix_files_create_tmp(char* fn, int32_t count) {
    posix_swear(fn != null && count > 0);
    const char* tmp = posix_files.tmp();
    int r = 0;
    if (count < (int32_t)strlen(tmp) + 8) {
        r = (int)ERROR_BUFFER_OVERFLOW;
    } else {
        char prefix[4] = { 0 };
        r = GetTempFileNameA(tmp, prefix, 0, fn) == 0 ? posix_core.err() : 0;
    }
    return r;
}

static int posix_files_chmod777(const char* pathname) {
    SID_IDENTIFIER_AUTHORITY SIDAuthWorld = SECURITY_WORLD_SID_AUTHORITY;
    PSID everyone = null;
    posix_fatal_win32err(AllocateAndInitializeSid(&SIDAuthWorld, 1,
             SECURITY_WORLD_RID, 0, 0, 0, 0, 0, 0, 0, &everyone));
    EXPLICIT_ACCESSA ea[1] = { { 0 } };
    ea[0].grfAccessPermissions = 0xFFFFFFFF;
    ea[0].grfAccessMode  = GRANT_ACCESS;
    ea[0].grfInheritance = SUB_CONTAINERS_AND_OBJECTS_INHERIT;
    ea[0].Trustee.TrusteeForm = TRUSTEE_IS_SID;
    ea[0].Trustee.TrusteeType = TRUSTEE_IS_WELL_KNOWN_GROUP;
    ea[0].Trustee.ptstrName  = (LPSTR)everyone;
    ACL* acl = null;
    posix_fatal_if_error(SetEntriesInAclA(1, ea, null, &acl));
    uint8_t stack[SECURITY_DESCRIPTOR_MIN_LENGTH] = {0};
    SECURITY_DESCRIPTOR* sd = (SECURITY_DESCRIPTOR*)stack;
    posix_fatal_win32err(InitializeSecurityDescriptor(sd, SECURITY_DESCRIPTOR_REVISION));
    posix_fatal_win32err(SetSecurityDescriptorDacl(sd, true, acl, false));
    int r = posix_b2e(SetFileSecurityA(pathname, DACL_SECURITY_INFORMATION, sd));
    if (everyone != null) { FreeSid(everyone); }
    if (acl != null) { LocalFree(acl); }
    return r;
}

static int posix_files_mkdirs(const char* dir) {
    const int32_t n = (int32_t)strlen(dir) + 1;
    char* s = null;
    int r = posix_heap.allocate(null, (void**)&s, n, true);
    const char* next = strchr(dir, '\\');
    if (next == null) { next = strchr(dir, '/'); }
    while (r == 0 && next != null) {
        if (next > dir && *(next - 1) != ':') {
            memcpy(s, dir, (size_t)(next - dir));
            r = posix_b2e(CreateDirectoryA(s, null));
            if (r == ERROR_ALREADY_EXISTS) { r = 0; }
        }
        if (r == 0) {
            const char* prev = ++next;
            next = strchr(prev, '\\');
            if (next == null) { next = strchr(prev, '/'); }
        }
    }
    if (r == 0) { r = posix_b2e(CreateDirectoryA(dir, null)); }
    posix_heap.deallocate(null, s);
    return r == ERROR_ALREADY_EXISTS ? 0 : r;
}

#pragma push_macro("posix_files_realloc_path")
#pragma push_macro("posix_files_append_name")

#define posix_files_realloc_path(r, pn, pnc, fn, name) do {             \
    const int32_t bytes = (int32_t)(strlen(fn) + strlen(name) + 3);     \
    if (bytes > pnc) {                                                  \
        r = posix_heap.reallocate(null, (void**)&pn, bytes, false);     \
        if (r == 0) { pnc = bytes; }                                    \
        else { posix_heap.deallocate(null, pn); pn = null; }            \
    }                                                                   \
} while (0)

#define posix_files_append_name(pn, pnc, fn, name) do {  \
    if (strcmp(fn, "\\") == 0 || strcmp(fn, "/") == 0) { \
        posix_str.format(pn, pnc, "\\%s", name);         \
    } else {                                             \
        posix_str.format(pn, pnc, "%.*s\\%s", k, fn, name); \
    }                                                    \
} while (0)

static int posix_files_rmdirs(const char* fn) {
    struct posix_files_stat st;
    struct posix_folder folder;
    int r = posix_files.opendir(&folder, fn);
    if (r == 0) {
        int32_t k = (int32_t)strlen(fn);
        if (k > 1 && (fn[k - 1] == '/' || fn[k - 1] == '\\')) { k--; }
        int32_t pnc = 64 * 1024;
        char* pn = null;
        r = posix_heap.allocate(null, (void**)&pn, pnc, false);
        while (r == 0) {
            const char* name = posix_files.readdir(&folder, &st);
            if (name == null) { break; }
            if (strcmp(name, ".") != 0 && strcmp(name, "..") != 0 &&
                (st.type & posix_files.type_symlink) == 0 &&
                (st.type & posix_files.type_folder) != 0) {
                posix_files_realloc_path(r, pn, pnc, fn, name);
                if (r == 0) {
                    posix_files_append_name(pn, pnc, fn, name);
                    r = posix_files.rmdirs(pn);
                }
            }
        }
        posix_files.closedir(&folder);
        r = posix_files.opendir(&folder, fn);
        while (r == 0) {
            const char* name = posix_files.readdir(&folder, &st);
            if (name == null) { break; }
            if (strcmp(name, ".") != 0 && strcmp(name, "..") != 0 &&
                (st.type & posix_files.type_folder) == 0) {
                posix_files_realloc_path(r, pn, pnc, fn, name);
                if (r == 0) {
                    posix_files_append_name(pn, pnc, fn, name);
                    r = posix_files.unlink(pn);
                }
            }
        }
        posix_heap.deallocate(null, pn);
        posix_files.closedir(&folder);
    }
    if (r == 0) { r = posix_files.unlink(fn); }
    return r;
}

#pragma pop_macro("posix_files_append_name")
#pragma pop_macro("posix_files_realloc_path")

static int posix_files_copy(const char* s, const char* d) {
    return posix_b2e(CopyFileA(s, d, false));
}

static int posix_files_move(const char* s, const char* d) {
    static const DWORD flags =
        MOVEFILE_REPLACE_EXISTING | MOVEFILE_COPY_ALLOWED | MOVEFILE_WRITE_THROUGH;
    return posix_b2e(MoveFileExA(s, d, flags));
}

static int posix_files_link(const char* from, const char* to) {
    return posix_b2e(CreateHardLinkA(to, from, null));
}

static int posix_files_symlink(const char* from, const char* to) {
    DWORD flags = posix_files.is_folder(from) ? SYMBOLIC_LINK_FLAG_DIRECTORY : 0;
    return posix_b2e(CreateSymbolicLinkA(to, from, flags));
}

static const char* posix_files_known_folder(int32_t kf) {
    static const GUID* kf_ids[] = {
        &FOLDERID_Profile,  &FOLDERID_Desktop,   &FOLDERID_Documents,
        &FOLDERID_Downloads, &FOLDERID_Music,    &FOLDERID_Pictures,
        &FOLDERID_Videos,   &FOLDERID_Public,    &FOLDERID_ProgramFiles,
        &FOLDERID_ProgramData
    };
    static struct posix_file_name known_folders[posix_countof(kf_ids)];
    posix_fatal_if(!(0 <= kf && kf < posix_countof(kf_ids)), "invalid kf=%d", kf);
    if (known_folders[kf].s[0] == 0) {
        uint16_t* path = null;
        if (SHGetKnownFolderPath(kf_ids[kf], 0, null, &path) == S_OK && path != null) {
            const int32_t n = posix_countof(known_folders[kf].s);
            posix_str.utf16to8(known_folders[kf].s, n, path, -1);
            CoTaskMemFree(path);
        }
    }
    return known_folders[kf].s;
}

static const char* posix_files_bin(void) {
    return posix_files_known_folder(posix_files.folder.bin);
}

static const char* posix_files_data(void) {
    return posix_files_known_folder(posix_files.folder.data);
}

static const char* posix_files_tmp(void) {
    static char tmp[posix_files_max_path];
    if (tmp[0] == 0) {
        int r = GetTempPathA(posix_countof(tmp), tmp) == 0 ? posix_core.err() : 0;
        posix_fatal_if(r != 0, "GetTempPathA() failed %s", posix_strerr(r));
    }
    return tmp;
}

static int posix_files_cwd(char* fn, int32_t count) {
    posix_swear(count > 1);
    DWORD bytes = (DWORD)(count - 1);
    int r = posix_b2e(GetCurrentDirectoryA(bytes, fn));
    fn[count - 1] = 0;
    return r;
}

static int posix_files_chdir(const char* fn) {
    return posix_b2e(SetCurrentDirectoryA(fn));
}

struct posix_files_dir {
    HANDLE handle;
    WIN32_FIND_DATAA find;
};

posix_static_assertion(sizeof(struct posix_files_dir) <= sizeof(struct posix_folder));

static int posix_files_opendir(struct posix_folder* folder, const char* folder_name) {
    struct posix_files_dir* d = (struct posix_files_dir*)(void*)folder;
    int32_t n = (int32_t)strlen(folder_name);
    char* fn = null;
    int r = posix_heap.allocate(null, (void**)&fn, (int64_t)n + 3, false);
    if (r == 0) {
        posix_str.format(fn, n + 3, "%s\\*", folder_name);
        fn[n + 2] = 0;
        d->handle = FindFirstFileA(fn, &d->find);
        if (d->handle == INVALID_HANDLE_VALUE) { r = posix_core.err(); }
        posix_heap.deallocate(null, fn);
    }
    return r;
}

static uint64_t posix_files_ft2us(FILETIME* ft) {
    return (((uint64_t)ft->dwHighDateTime) << 32 | ft->dwLowDateTime) / 10;
}

static const char* posix_files_readdir(struct posix_folder* folder, struct posix_files_stat* s) {
    const char* fn = null;
    struct posix_files_dir* d = (struct posix_files_dir*)(void*)folder;
    if (FindNextFileA(d->handle, &d->find)) {
        fn = d->find.cFileName;
        d->find.cFileName[posix_countof(d->find.cFileName) - 1] = 0x00;
        if (s != null) {
            s->accessed = posix_files_ft2us(&d->find.ftLastAccessTime);
            s->created  = posix_files_ft2us(&d->find.ftCreationTime);
            s->updated  = posix_files_ft2us(&d->find.ftLastWriteTime);
            s->type = posix_files_a2t(d->find.dwFileAttributes);
            s->size = (int64_t)((((uint64_t)d->find.nFileSizeHigh) << 32) |
                                  (uint64_t)d->find.nFileSizeLow);
        }
    }
    return fn;
}

static void posix_files_closedir(struct posix_folder* folder) {
    struct posix_files_dir* d = (struct posix_files_dir*)(void*)folder;
    posix_fatal_win32err(FindClose(d->handle));
}

#else

static int posix_files_open(struct posix_file* *file, const char* filename, int32_t flags) {
    int f = 0;
    if ((flags & posix_files.o_rw) == posix_files.o_rw) {
        f |= O_RDWR;
    } else if (flags & posix_files.o_wr) {
        f |= O_WRONLY;
    } else {
        f |= O_RDONLY;
    }
    if (flags & posix_files.o_append) { f |= O_APPEND; }
    if (flags & posix_files.o_create) { f |= O_CREAT; }
    if (flags & posix_files.o_excl)   { f |= O_EXCL; }
    if (flags & posix_files.o_trunc)  { f |= O_TRUNC; }
    if (flags & posix_files.o_sync)   { f |= O_SYNC; }
    
    int fd = open(filename, f, 0666);
    if (fd < 0) {
        *file = posix_files.invalid;
        return errno;
    }
    *file = (struct posix_file*)(intptr_t)fd;
    return 0;
}

static bool posix_files_is_valid(struct posix_file* file) {
    return file != posix_files.invalid && file != null && (intptr_t)file >= 0;
}

static int posix_files_seek(struct posix_file* file, int64_t *position, int32_t method) {
    int fd = (int)(intptr_t)file;
    off_t res = lseek(fd, (off_t)*position, method);
    if (res == (off_t)-1) { return errno; }
    *position = (int64_t)res;
    return 0;
}

static int posix_files_stat(struct posix_file* file, struct posix_files_stat* s, bool follow_symlink) {
    (void)follow_symlink; // fstat inherently targets the open file
    int fd = (int)(intptr_t)file;
    struct stat st;
    if (fstat(fd, &st) < 0) { return errno; }
    s->size = (int64_t)st.st_size;
    s->created = (uint64_t)st.st_ctime * 1000000ULL;
    s->accessed = (uint64_t)st.st_atime * 1000000ULL;
    s->updated = (uint64_t)st.st_mtime * 1000000ULL;
    s->type = 0;
    if (S_ISDIR(st.st_mode)) { s->type |= posix_files.type_folder; }
    if (S_ISLNK(st.st_mode)) { s->type |= posix_files.type_symlink; }
    if (S_ISCHR(st.st_mode) || S_ISBLK(st.st_mode)) { s->type |= posix_files.type_device; }
    return 0;
}

static int posix_files_read(struct posix_file* file, void* data, int64_t bytes, int64_t *transferred) {
    int fd = (int)(intptr_t)file;
    ssize_t res = read(fd, data, (size_t)bytes);
    if (res < 0) {
        if (transferred) { *transferred = 0; }
        return errno;
    }
    if (transferred) { *transferred = (int64_t)res; }
    return 0;
}

static int posix_files_write(struct posix_file* file, const void* data, int64_t bytes, int64_t *transferred) {
    int fd = (int)(intptr_t)file;
    ssize_t res = write(fd, data, (size_t)bytes);
    if (res < 0) {
        if (transferred) { *transferred = 0; }
        return errno;
    }
    if (transferred) { *transferred = (int64_t)res; }
    return 0;
}

static int posix_files_flush(struct posix_file* file) {
    int fd = (int)(intptr_t)file;
    return fsync(fd) == 0 ? 0 : errno;
}

static void posix_files_close(struct posix_file* file) {
    int fd = (int)(intptr_t)file;
    if (fd >= 0) { close(fd); }
}

static int posix_files_write_fully(const char* filename, const void* data, int64_t bytes, int64_t *transferred) {
    if (transferred) { *transferred = 0; }
    int fd = open(filename, O_WRONLY | O_CREAT | O_TRUNC, 0666);
    if (fd < 0) { return errno; }
    
    int64_t written = 0;
    const uint8_t* p = (const uint8_t*)data;
    int r = 0;
    
    while (bytes > 0) {
        ssize_t chunk = write(fd, p, (size_t)bytes);
        if (chunk < 0) {
            r = errno;
            break;
        }
        written += chunk;
        p += chunk;
        bytes -= chunk;
    }
    
    if (transferred) { *transferred = written; }
    if (fsync(fd) < 0 && r == 0) { r = errno; }
    close(fd);
    return r;
}

static bool posix_files_exists(const char* pathname) {
    return access(pathname, F_OK) == 0;
}

static bool posix_files_is_folder(const char* pathname) {
    struct stat st;
    if (stat(pathname, &st) == 0) {
        return S_ISDIR(st.st_mode);
    }
    return false;
}

static bool posix_files_is_symlink(const char* pathname) {
    struct stat st;
    if (lstat(pathname, &st) == 0) {
        return S_ISLNK(st.st_mode);
    }
    return false;
}

static const char* posix_files_basename(const char* pathname) {
    const char* bn = strrchr(pathname, '/');
    return bn != null ? bn + 1 : pathname;
}

static int posix_files_mkdirs(const char* pathname) {
    // struct chars grows for paths deeper than a fixed buffer would hold.
    int r = 0;
    struct chars tmp = {0};
    chars_puts(&tmp, pathname);
    if (tmp.count > 0 && tmp.data[tmp.count - 1] == '/') {
        tmp.data[--tmp.count] = 0;
    }
    char* p = tmp.data;
    if (*p != 0) { p++; } // never try to mkdir the leading '/' (root)
    for (; r == 0 && *p != 0; p++) {
        if (*p == '/') {
            *p = 0;
            if (mkdir(tmp.data, 0755) != 0 && errno != EEXIST) { r = errno; }
            *p = '/';
        }
    }
    if (r == 0 && tmp.count > 0 &&
        mkdir(tmp.data, 0755) != 0 && errno != EEXIST) {
        r = errno;
    }
    chars_free(&tmp);
    return r;
}

static int posix_files_rmdirs(const char* pathname) {
    // Basic stub for recursion: relies on system rm -rf for simplicity in shim
    // A robust C implementation requires nftw or recursive opendir.
    char cmd[1024];
    snprintf(cmd, sizeof(cmd), "rm -rf \"%s\"", pathname);
    return system(cmd) == 0 ? 0 : errno;
}

static int posix_files_create_tmp(char* file, int32_t count) {
    (void)count;
    snprintf(file, (size_t)count, "/tmp/rt_XXXXXX");
    int fd = mkstemp(file);
    if (fd < 0) { return errno; }
    close(fd);
    return 0;
}

static int posix_files_chmod777(const char* pathname) {
    return chmod(pathname, 0777) == 0 ? 0 : errno;
}

static int posix_files_symlink(const char* from, const char* to) {
    return symlink(from, to) == 0 ? 0 : errno;
}

static int posix_files_link(const char* from, const char* to) {
    return link(from, to) == 0 ? 0 : errno;
}

static int posix_files_unlink(const char* pathname) {
    return unlink(pathname) == 0 ? 0 : errno;
}

static int posix_files_copy(const char* from, const char* to) {
    char buf[8192];
    int fd_from = open(from, O_RDONLY);
    if (fd_from < 0) { return errno; }
    int fd_to = open(to, O_WRONLY | O_CREAT | O_TRUNC, 0666);
    if (fd_to < 0) { close(fd_from); return errno; }
    
    ssize_t nread;
    int r = 0;
    while ((nread = read(fd_from, buf, sizeof(buf))) > 0) {
        char *out_ptr = buf;
        ssize_t nwritten;
        do {
            nwritten = write(fd_to, out_ptr, (size_t)nread);
            if (nwritten >= 0) {
                nread -= nwritten;
                out_ptr += nwritten;
            } else if (errno != EINTR) {
                r = errno;
                break;
            }
        } while (nread > 0);
        if (r != 0) break;
    }
    
    close(fd_from);
    close(fd_to);
    return r;
}

static int posix_files_move(const char* from, const char* to) {
    return rename(from, to) == 0 ? 0 : errno;
}

static int posix_files_cwd(char* folder, int32_t count) {
    return getcwd(folder, (size_t)count) != null ? 0 : errno;
}

static int posix_files_chdir(const char* folder) {
    return chdir(folder) == 0 ? 0 : errno;
}

static const char* posix_files_known_folder(int32_t kf_id) {
    static char path[1024] = {0};
    const char* home = getenv("HOME");
    if (home == null) { home = "/"; }
    
    if (kf_id == posix_files.folder.home) {
        snprintf(path, sizeof(path), "%s", home);
    } else if (kf_id == posix_files.folder.desktop) {
        snprintf(path, sizeof(path), "%s/Desktop", home);
    } else if (kf_id == posix_files.folder.documents) {
        snprintf(path, sizeof(path), "%s/Documents", home);
    } else if (kf_id == posix_files.folder.downloads) {
        snprintf(path, sizeof(path), "%s/Downloads", home);
    } else if (kf_id == posix_files.folder.bin) {
        snprintf(path, sizeof(path), "/usr/local/bin");
    } else if (kf_id == posix_files.folder.data) {
        snprintf(path, sizeof(path), "%s/.local/share", home);
    } else {
        snprintf(path, sizeof(path), "%s", home);
    }
    return path;
}

static const char* posix_files_bin(void) { return posix_files_known_folder(posix_files.folder.bin); }
static const char* posix_files_data(void) { return posix_files_known_folder(posix_files.folder.data); }

static const char* posix_files_tmp(void) {
    const char* t = getenv("TMPDIR");
    return t != null ? t : "/tmp";
}

struct posix_dir {
    DIR* dir;
    struct dirent* entry;
};

static int posix_files_opendir(struct posix_folder* folder, const char* folder_name) {
    struct posix_dir* d = (struct posix_dir*)folder;
    d->dir = opendir(folder_name);
    return d->dir != null ? 0 : errno;
}

static const char* posix_files_readdir(struct posix_folder* folder, struct posix_files_stat* optional) {
    struct posix_dir* d = (struct posix_dir*)folder;
    d->entry = readdir(d->dir);
    if (d->entry == null) { return null; }
    if (optional != null) {
        optional->type = 0;
        if (d->entry->d_type == DT_DIR) { optional->type |= posix_files.type_folder; }
        if (d->entry->d_type == DT_LNK) { optional->type |= posix_files.type_symlink; }
        optional->size = 0;
    }
    return d->entry->d_name;
}

static void posix_files_closedir(struct posix_folder* folder) {
    struct posix_dir* d = (struct posix_dir*)folder;
    if (d->dir != null) { closedir(d->dir); }
}

#endif // _WIN32

static void posix_files_test(void) {}

struct posix_files_if posix_files = {
    .invalid      = (struct posix_file*)(intptr_t)-1,
    .type_folder  = 0x00000010,
    .type_symlink = 0x00000400,
    .type_device  = 0x00000040,
    .seek_set     = SEEK_SET,
    .seek_cur     = SEEK_CUR,
    .seek_end     = SEEK_END,
    .o_rd         = O_RDONLY,
    .o_wr         = O_WRONLY,
    .o_rw         = O_RDWR,
    .o_append     = O_APPEND,
    .o_create     = O_CREAT,
    .o_excl       = O_EXCL,
    .o_trunc      = O_TRUNC,
    .o_sync       = O_SYNC,
    .folder       = {
        .home      = 0,
        .desktop   = 1,
        .documents = 2,
        .downloads = 3,
        .music     = 4,
        .pictures  = 5,
        .videos    = 6,
        .shared    = 7,
        .bin       = 8,
        .data      = 9
    },
    .open         = posix_files_open,
    .is_valid     = posix_files_is_valid,
    .seek         = posix_files_seek,
    .stat         = posix_files_stat,
    .read         = posix_files_read,
    .write        = posix_files_write,
    .flush        = posix_files_flush,
    .close        = posix_files_close,
    .write_fully  = posix_files_write_fully,
    .exists       = posix_files_exists,
    .is_folder    = posix_files_is_folder,
    .is_symlink   = posix_files_is_symlink,
    .basename     = posix_files_basename,
    .mkdirs       = posix_files_mkdirs,
    .rmdirs       = posix_files_rmdirs,
    .create_tmp   = posix_files_create_tmp,
    .chmod777     = posix_files_chmod777,
    .unlink       = posix_files_unlink,
    .link         = posix_files_link,
    .symlink      = posix_files_symlink,
    .copy         = posix_files_copy,
    .move         = posix_files_move,
    .cwd          = posix_files_cwd,
    .chdir        = posix_files_chdir,
    .known_folder = posix_files_known_folder,
    .bin          = posix_files_bin,
    .data         = posix_files_data,
    .tmp          = posix_files_tmp,
    .opendir      = posix_files_opendir,
    .readdir      = posix_files_readdir,
    .closedir     = posix_files_closedir,
    .test         = posix_files_test
};

// _______________________________ posix_config.c ________________________________

static void posix_config_get_path(const char* name, const char* key,
                                  struct chars* out) {
    const char* home = getenv("HOME");
    if (home == null) { home = "/tmp"; }
    if (key != null) {
        chars_printf(out, "%s/.config/%s/%s", home, name, key);
    } else {
        chars_printf(out, "%s/.config/%s", home, name);
    }
}

static int posix_config_save(const char* name, const char* key, const void* data, int32_t bytes) {
    struct chars dir_path = {0};
    posix_config_get_path(name, null, &dir_path);
    posix_files.mkdirs(dir_path.data);
    chars_free(&dir_path);
    struct chars file_path = {0};
    posix_config_get_path(name, key, &file_path);
    int r = posix_files.write_fully(file_path.data, data, (int64_t)bytes, null);
    chars_free(&file_path);
    return r;
}

static int32_t posix_config_size(const char* name, const char* key) {
    struct chars file_path = {0};
    posix_config_get_path(name, key, &file_path);
    int32_t r = -1;
    struct posix_file* f = posix_files.invalid;
    if (posix_files.open(&f, file_path.data, posix_files.o_rd) == 0) {
        struct posix_files_stat st;
        posix_files.stat(f, &st, false);
        posix_files.close(f);
        r = (int32_t)st.size;
    }
    chars_free(&file_path);
    return r;
}

static int32_t posix_config_load(const char* name, const char* key, void* data, int32_t bytes) {
    struct chars file_path = {0};
    posix_config_get_path(name, key, &file_path);
    int32_t r = -1;
    struct posix_file* f = posix_files.invalid;
    if (posix_files.open(&f, file_path.data, posix_files.o_rd) == 0) {
        int64_t transferred = 0;
        posix_files.read(f, data, (int64_t)bytes, &transferred);
        posix_files.close(f);
        r = (int32_t)transferred;
    }
    chars_free(&file_path);
    return r;
}

static int posix_config_remove(const char* name, const char* key) {
    struct chars file_path = {0};
    posix_config_get_path(name, key, &file_path);
    int r = posix_files.unlink(file_path.data);
    chars_free(&file_path);
    return r;
}

static int posix_config_clean(const char* name) {
    struct chars dir_path = {0};
    posix_config_get_path(name, null, &dir_path);
    int r = posix_files.rmdirs(dir_path.data);
    chars_free(&dir_path);
    return r;
}

static void posix_config_test(void) {}

struct posix_config_if posix_config = {
    .save   = posix_config_save,
    .size   = posix_config_size,
    .load   = posix_config_load,
    .remove = posix_config_remove,
    .clean  = posix_config_clean,
    .test   = posix_config_test
};

// _______________________________ posix_streams.c _______________________________

static int posix_streams_memory_read(struct posix_stream_if* stream, void* data, int64_t bytes, int64_t *transferred) {
    posix_swear(bytes > 0);
    struct posix_stream_memory_if* s = (struct posix_stream_memory_if*)stream;
    posix_swear(0 <= s->pos_read && s->pos_read <= s->bytes_read);
    int64_t transfer = posix_min(bytes, s->bytes_read - s->pos_read);
    memcpy(data, (const uint8_t*)s->data_read + s->pos_read, (size_t)transfer);
    s->pos_read += transfer;
    if (transferred != null) { *transferred = transfer; }
    return 0;
}

static int posix_streams_memory_write(struct posix_stream_if* stream, const void* data, int64_t bytes, int64_t *transferred) {
    posix_swear(bytes > 0);
    struct posix_stream_memory_if* s = (struct posix_stream_memory_if*)stream;
    posix_swear(0 <= s->pos_write && s->pos_write <= s->bytes_write);
    bool overflow = s->bytes_write - s->pos_write <= 0;
    int64_t transfer = posix_min(bytes, s->bytes_write - s->pos_write);
    memcpy((uint8_t*)s->data_write + s->pos_write, data, (size_t)transfer);
    s->pos_write += transfer;
    if (transferred != null) { *transferred = transfer; }
    return overflow ? ENOBUFS : 0;
}

static void posix_streams_read_only(struct posix_stream_memory_if* s, const void* data, int64_t bytes) {
    s->stream.read = posix_streams_memory_read;
    s->stream.write = null;
    s->data_read = data;
    s->bytes_read = bytes;
    s->pos_read = 0;
    s->data_write = null;
    s->bytes_write = 0;
    s->pos_write = 0;
}

static void posix_streams_write_only(struct posix_stream_memory_if* s, void* data, int64_t bytes) {
    s->stream.read = null;
    s->stream.write = posix_streams_memory_write;
    s->data_read = null;
    s->bytes_read = 0;
    s->pos_read = 0;
    s->data_write = data;
    s->bytes_write = bytes;
    s->pos_write = 0;
}

static void posix_streams_read_write(struct posix_stream_memory_if* s, const void* read, int64_t read_bytes, void* write, int64_t write_bytes) {
    s->stream.read = posix_streams_memory_read;
    s->stream.write = posix_streams_memory_write;
    s->data_read = read;
    s->bytes_read = read_bytes;
    s->pos_read = 0;
    s->data_write = write;
    s->bytes_write = write_bytes;
    s->pos_write = 0;
}

static void posix_streams_test(void) {}

struct posix_streams_if posix_streams = {
    .read_only  = posix_streams_read_only,
    .write_only = posix_streams_write_only,
    .read_write = posix_streams_read_write,
    .test       = posix_streams_test
};




#if !defined(_WIN32)
#include <time.h>
#include <sys/time.h>
#include <sys/wait.h>
#include <dlfcn.h>
#include <execinfo.h>
#include <spawn.h>

extern char **environ;
#endif

// ________________________________ posix_clock.c ________________________________

#if defined(_WIN32)

static uint64_t posix_clock_microseconds(void) { // since 1601, NOT monotonic
    FILETIME ft; // 100ns intervals since 1601 UTC
    GetSystemTimePreciseAsFileTime(&ft);
    return (((uint64_t)ft.dwHighDateTime) << 32 | ft.dwLowDateTime) / 10;
}

static uint64_t posix_clock_localtime(void) {
    TIME_ZONE_INFORMATION tzi; // UTC = local time + bias
    GetTimeZoneInformation(&tzi);
    uint64_t bias = (uint64_t)tzi.Bias * 60LL * 1000 * 1000; // microseconds
    return posix_clock_microseconds() - bias;
}

static void posix_clock_utc(uint64_t microseconds, int32_t* year, int32_t* month,
        int32_t* day, int32_t* hh, int32_t* mm, int32_t* ss, int32_t* ms, int32_t* mc) {
    uint64_t time_in_100ns = microseconds * 10;
    FILETIME mst = { (DWORD)(time_in_100ns & 0xFFFFFFFF), (DWORD)(time_in_100ns >> 32) };
    SYSTEMTIME utc;
    FileTimeToSystemTime(&mst, &utc);
    *year = utc.wYear; *month = utc.wMonth; *day = utc.wDay;
    *hh = utc.wHour; *mm = utc.wMinute; *ss = utc.wSecond;
    *ms = utc.wMilliseconds;
    *mc = microseconds % 1000;
}

static void posix_clock_local(uint64_t microseconds, int32_t* year, int32_t* month,
        int32_t* day, int32_t* hh, int32_t* mm, int32_t* ss, int32_t* ms, int32_t* mc) {
    uint64_t time_in_100ns = microseconds * 10;
    FILETIME mst = { (DWORD)(time_in_100ns & 0xFFFFFFFF), (DWORD)(time_in_100ns >> 32) };
    SYSTEMTIME utc;
    FileTimeToSystemTime(&mst, &utc);
    DYNAMIC_TIME_ZONE_INFORMATION tzi;
    GetDynamicTimeZoneInformation(&tzi);
    SYSTEMTIME lt = {0};
    SystemTimeToTzSpecificLocalTimeEx(&tzi, &utc, &lt);
    *year = lt.wYear; *month = lt.wMonth; *day = lt.wDay;
    *hh = lt.wHour; *mm = lt.wMinute; *ss = lt.wSecond;
    *ms = lt.wMilliseconds;
    *mc = microseconds % 1000;
}

static fp64_t posix_clock_seconds(void) { // since boot
    LARGE_INTEGER qpc;
    QueryPerformanceCounter(&qpc);
    static fp64_t one_over_freq;
    if (one_over_freq == 0) {
        LARGE_INTEGER frequency;
        QueryPerformanceFrequency(&frequency);
        one_over_freq = 1.0 / (fp64_t)frequency.QuadPart;
    }
    return (fp64_t)qpc.QuadPart * one_over_freq;
}

static uint64_t posix_clock_nanoseconds(void) {
    LARGE_INTEGER qpc;
    QueryPerformanceCounter(&qpc);
    static uint32_t freq;
    static uint32_t mul = 1000000000; // nsec_in_sec
    if (freq == 0) {
        LARGE_INTEGER frequency;
        QueryPerformanceFrequency(&frequency);
        freq = frequency.LowPart;
        uint32_t divider = posix_num.gcd32(1000000000u, freq);
        freq /= divider;
        mul  /= divider;
    }
    uint64_t ns_mul_freq = (uint64_t)qpc.QuadPart * mul;
    return freq == 1 ? ns_mul_freq : ns_mul_freq / freq;
}

static const uint64_t posix_clock_epoch_diff_usec = 11644473600000000ULL;

static uint64_t posix_clock_unix_microseconds(void) {
    return posix_clock_microseconds() - posix_clock_epoch_diff_usec;
}

static uint64_t posix_clock_unix_seconds(void) {
    return posix_clock_unix_microseconds() / 1000000ULL;
}

#else

static fp64_t posix_clock_seconds(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (fp64_t)ts.tv_sec + (fp64_t)ts.tv_nsec / 1e9;
}

static uint64_t posix_clock_nanoseconds(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)ts.tv_sec * 1000000000ULL + (uint64_t)ts.tv_nsec;
}

static uint64_t posix_clock_microseconds(void) {
    struct timeval tv;
    gettimeofday(&tv, null);
    // Adjust to Windows epoch (Jan 1, 1601) for compatibility if needed, 
    // or keep as POSIX epoch (Jan 1, 1970). The shim keeps POSIX epoch here.
    return (uint64_t)tv.tv_sec * 1000000ULL + (uint64_t)tv.tv_usec;
}

static uint64_t posix_clock_unix_microseconds(void) {
    return posix_clock_microseconds();
}

static uint64_t posix_clock_unix_seconds(void) {
    return posix_clock_microseconds() / 1000000ULL;
}

static uint64_t posix_clock_localtime(void) {
    return posix_clock_microseconds(); // gettimeofday is UTC, timezone adj needed
}

static void posix_clock_utc(uint64_t microseconds, int32_t* year, int32_t* month,
        int32_t* day, int32_t* hh, int32_t* mm, int32_t* ss, int32_t* ms, int32_t* mc) {
    time_t sec = (time_t)(microseconds / 1000000ULL);
    struct tm tm_info;
    gmtime_r(&sec, &tm_info);
    *year = tm_info.tm_year + 1900;
    *month = tm_info.tm_mon + 1;
    *day = tm_info.tm_mday;
    *hh = tm_info.tm_hour;
    *mm = tm_info.tm_min;
    *ss = tm_info.tm_sec;
    *ms = (int32_t)((microseconds % 1000000ULL) / 1000);
    *mc = (int32_t)(microseconds % 1000);
}

static void posix_clock_local(uint64_t microseconds, int32_t* year, int32_t* month,
        int32_t* day, int32_t* hh, int32_t* mm, int32_t* ss, int32_t* ms, int32_t* mc) {
    time_t sec = (time_t)(microseconds / 1000000ULL);
    struct tm tm_info;
    localtime_r(&sec, &tm_info);
    *year = tm_info.tm_year + 1900;
    *month = tm_info.tm_mon + 1;
    *day = tm_info.tm_mday;
    *hh = tm_info.tm_hour;
    *mm = tm_info.tm_min;
    *ss = tm_info.tm_sec;
    *ms = (int32_t)((microseconds % 1000000ULL) / 1000);
    *mc = (int32_t)(microseconds % 1000);
}

#endif // _WIN32

static void posix_clock_test(void) {}

struct posix_clock_if posix_clock = {
    .nsec_in_usec      = 1000,
    .nsec_in_msec      = 1000000,
    .nsec_in_sec       = 1000000000,
    .usec_in_msec      = 1000,
    .msec_in_sec       = 1000,
    .usec_in_sec       = 1000000,
    .seconds           = posix_clock_seconds,
    .nanoseconds       = posix_clock_nanoseconds,
    .unix_microseconds = posix_clock_unix_microseconds,
    .unix_seconds      = posix_clock_unix_seconds,
    .microseconds      = posix_clock_microseconds,
    .localtime         = posix_clock_localtime,
    .utc               = posix_clock_utc,
    .local             = posix_clock_local,
    .test              = posix_clock_test
};

// ______________________________ posix_processes.c ______________________________

#if defined(_WIN32)

struct posix_processes_pidof_lambda {
    bool (*each)(struct posix_processes_pidof_lambda* p, uint64_t pid);
    uint64_t* pids;
    size_t size;
    size_t count;
    fp64_t timeout;
    int error;
};

static int32_t posix_processes_for_each_pidof(const char* pname,
        struct posix_processes_pidof_lambda* la) {
    char stack[1024];
    int32_t n = posix_str.len(pname);
    posix_fatal_if(n + 5 >= posix_countof(stack), "name is too long: %s", pname);
    const char* name = pname;
    if (!posix_str.iends(pname, ".exe")) {
        int32_t k = (int32_t)strlen(pname) + 5;
        char* exe = stack;
        posix_str.format(exe, k, "%s.exe", pname);
        name = exe;
    }
    const char* base = strrchr(name, '\\');
    base = base != null ? base + 1 : name;
    uint16_t wn[1024];
    posix_fatal_if(strlen(base) >= posix_countof(wn), "name too long: %s", base);
    posix_str.utf8to16(wn, posix_countof(wn), base, -1);
    size_t count = 0;
    uint64_t pid = 0;
    uint8_t* data = null;
    ULONG bytes = 0;
    int r = (int)NtQuerySystemInformation(SystemProcessInformation, data, 0, &bytes);
    #pragma push_macro("STATUS_INFO_LENGTH_MISMATCH")
    #define STATUS_INFO_LENGTH_MISMATCH 0xC0000004
    while (r == (int)STATUS_INFO_LENGTH_MISMATCH) {
        bytes += sizeof(SYSTEM_PROCESS_INFORMATION) * 32;
        r = posix_heap.reallocate(null, (void**)&data, bytes, false);
        if (r == 0) {
            r = (int)NtQuerySystemInformation(SystemProcessInformation, data, bytes, &bytes);
        }
    }
    #pragma pop_macro("STATUS_INFO_LENGTH_MISMATCH")
    if (r == 0 && data != null) {
        SYSTEM_PROCESS_INFORMATION* proc = (SYSTEM_PROCESS_INFORMATION*)data;
        while (proc != null) {
            uint16_t* img = proc->ImageName.Buffer;
            bool match = img != null && _wcsicmp(img, wn) == 0;
            if (match) {
                pid = (uint64_t)(uintptr_t)proc->UniqueProcessId;
                if (base != name) {
                    char path[posix_files_max_path];
                    match = posix_processes.nameof(pid, path, posix_countof(path)) == 0 &&
                            posix_str.iends(path, name);
                }
            }
            if (match) {
                if (la != null && count < la->size && la->pids != null) {
                    la->pids[count] = pid;
                }
                count++;
                if (la != null && la->each != null && !la->each(la, pid)) { break; }
            }
            proc = proc->NextEntryOffset != 0 ? (SYSTEM_PROCESS_INFORMATION*)
                ((uint8_t*)proc + proc->NextEntryOffset) : null;
        }
    }
    if (data != null) { posix_heap.deallocate(null, data); }
    return (int32_t)count;
}

static int posix_processes_nameof(uint64_t pid, char* name, int32_t count) {
    posix_assert(name != null && count > 0);
    int r = 0;
    name[0] = 0;
    HANDLE p = OpenProcess(PROCESS_ALL_ACCESS, false, (DWORD)pid);
    if (p != null) {
        r = posix_b2e(GetModuleFileNameExA(p, null, name, count));
        name[count - 1] = 0;
        posix_win32_close_handle(p);
    } else {
        r = (int)ERROR_NOT_FOUND;
    }
    return r;
}

static bool posix_processes_present(uint64_t pid) {
    void* h = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, 0, (DWORD)pid);
    bool b = h != null;
    if (h != null) { posix_win32_close_handle(h); }
    return b;
}

static bool posix_processes_first_pid(struct posix_processes_pidof_lambda* lambda, uint64_t pid) {
    lambda->pids[0] = pid;
    return false;
}

static uint64_t posix_processes_pid(const char* pname) {
    uint64_t first[1] = {0};
    struct posix_processes_pidof_lambda lambda = {
        .each = posix_processes_first_pid, .pids = first, .size = 1
    };
    posix_processes_for_each_pidof(pname, &lambda);
    return first[0];
}

static bool posix_processes_store_pid(struct posix_processes_pidof_lambda* lambda, uint64_t pid) {
    if (lambda->pids != null && lambda->count < lambda->size) {
        lambda->pids[lambda->count++] = pid;
    }
    return true;
}

static int posix_processes_pids(const char* pname, uint64_t* pids,
        int32_t size, int32_t *count) {
    *count = 0;
    struct posix_processes_pidof_lambda lambda = {
        .each = posix_processes_store_pid, .pids = pids, .size = (size_t)size
    };
    *count = posix_processes_for_each_pidof(pname, &lambda);
    return (int32_t)lambda.count == *count ? 0 : (int)ERROR_MORE_DATA;
}

static int posix_processes_kill(uint64_t pid, fp64_t timeout) {
    DWORD milliseconds = timeout < 0 ? INFINITE : (DWORD)(timeout * 1000);
    enum { access = PROCESS_QUERY_LIMITED_INFORMATION |
                    PROCESS_TERMINATE | SYNCHRONIZE };
    int r = (int)ERROR_NOT_FOUND;
    HANDLE h = OpenProcess(access, 0, (DWORD)pid);
    if (h != null) {
        r = posix_b2e(TerminateProcess(h, ERROR_PROCESS_ABORTED));
        if (r == 0) {
            DWORD ix = WaitForSingleObject(h, milliseconds);
            r = posix_wait_ix2e(ix);
        }
        posix_win32_close_handle(h);
        if (r == ERROR_ACCESS_DENIED) {
            posix_thread.sleep_for(0.015);
            HANDLE retry = OpenProcess(access, 0, (DWORD)pid);
            if (retry == null) { r = 0; } else { posix_win32_close_handle(retry); }
        }
    }
    if (r != 0) { posix_core.set_err(r); }
    return r;
}

static bool posix_processes_kill_one(struct posix_processes_pidof_lambda* lambda, uint64_t pid) {
    int r = posix_processes_kill(pid, lambda->timeout);
    if (r != 0) { lambda->error = r; }
    return true;
}

static int posix_processes_kill_all(const char* name, fp64_t timeout) {
    struct posix_processes_pidof_lambda lambda = {
        .each = posix_processes_kill_one, .timeout = timeout
    };
    int32_t c = posix_processes_for_each_pidof(name, &lambda);
    return c == 0 ? (int)ERROR_NOT_FOUND : lambda.error;
}

static bool posix_processes_is_elevated(void) {
    BOOL elevated = false;
    PSID administrators_group = null;
    SID_IDENTIFIER_AUTHORITY nt_authority = SECURITY_NT_AUTHORITY;
    int r = posix_b2e(AllocateAndInitializeSid(&nt_authority, 2,
                SECURITY_BUILTIN_DOMAIN_RID, DOMAIN_ALIAS_RID_ADMINS,
                0, 0, 0, 0, 0, 0, &administrators_group));
    if (r == 0 && administrators_group != null) {
        r = posix_b2e(CheckTokenMembership(null, administrators_group, &elevated));
        FreeSid(administrators_group);
    }
    return elevated;
}

static int posix_processes_restart_elevated(void) {
    int r = 0;
    if (!posix_processes_is_elevated()) {
        SHELLEXECUTEINFOA sei = { sizeof(sei) };
        sei.lpVerb = "runas";
        sei.lpFile = posix_processes.name();
        sei.nShow = SW_NORMAL;
        r = posix_b2e(ShellExecuteExA(&sei));
        if (r == 0) { posix_core.exit(0); } // second elevated copy launched
    }
    return r;
}

static void posix_processes_close_pipes(STARTUPINFOA* si,
        HANDLE *read_out, HANDLE *read_err, HANDLE *write_in) {
    if (si->hStdOutput != INVALID_HANDLE_VALUE) { posix_win32_close_handle(si->hStdOutput); }
    if (si->hStdError  != INVALID_HANDLE_VALUE) { posix_win32_close_handle(si->hStdError);  }
    if (si->hStdInput  != INVALID_HANDLE_VALUE) { posix_win32_close_handle(si->hStdInput);  }
    if (*read_out != INVALID_HANDLE_VALUE) { posix_win32_close_handle(*read_out); }
    if (*read_err != INVALID_HANDLE_VALUE) { posix_win32_close_handle(*read_err); }
    if (*write_in != INVALID_HANDLE_VALUE) { posix_win32_close_handle(*write_in); }
}

static int posix_processes_child_read(struct posix_stream_if* out, HANDLE pipe) {
    char data[32 * 1024];
    DWORD available = 0;
    int r = posix_b2e(PeekNamedPipe(pipe, null, sizeof(data), null, &available, null));
    if (r != 0) {
        // process exited and closed the pipe
    } else if (available > 0) {
        DWORD bytes_read = 0;
        r = posix_b2e(ReadFile(pipe, data, sizeof(data), &bytes_read, null));
        if (out != null && r == 0) { r = out->write(out, data, bytes_read, null); }
    }
    return r;
}

static int posix_processes_child_write(struct posix_stream_if* in, HANDLE pipe) {
    int r = 0;
    if (in != null) {
        uint8_t memory[32 * 1024];
        uint8_t* data = memory;
        int64_t bytes_read = 0;
        in->read(in, data, sizeof(data), &bytes_read);
        while (r == 0 && bytes_read > 0) {
            DWORD bytes_written = 0;
            r = posix_b2e(WriteFile(pipe, data, (DWORD)bytes_read, &bytes_written, null));
            data += bytes_written;
            bytes_read -= bytes_written;
        }
    }
    return r;
}

static int posix_processes_run(struct posix_processes_child* child) {
    const fp64_t deadline = posix_clock.seconds() + child->timeout;
    int r = 0;
    STARTUPINFOA si = {
        .cb = sizeof(STARTUPINFOA),
        .hStdInput  = INVALID_HANDLE_VALUE,
        .hStdOutput = INVALID_HANDLE_VALUE,
        .hStdError  = INVALID_HANDLE_VALUE,
        .dwFlags     = STARTF_USESHOWWINDOW | STARTF_USESTDHANDLES,
        .wShowWindow = SW_HIDE
    };
    SECURITY_ATTRIBUTES sa = { sizeof(sa), null, true };
    PROCESS_INFORMATION pi = {0};
    HANDLE read_out = INVALID_HANDLE_VALUE;
    HANDLE read_err = INVALID_HANDLE_VALUE;
    HANDLE write_in = INVALID_HANDLE_VALUE;
    int ro = posix_b2e(CreatePipe(&read_out, &si.hStdOutput, &sa, 0));
    int re = posix_b2e(CreatePipe(&read_err, &si.hStdError,  &sa, 0));
    int ri = posix_b2e(CreatePipe(&si.hStdInput, &write_in,  &sa, 0));
    if (ro != 0 || re != 0 || ri != 0) {
        posix_processes_close_pipes(&si, &read_out, &read_err, &write_in);
        r = ro != 0 ? ro : (re != 0 ? re : ri);
    }
    if (r == 0) {
        r = posix_b2e(CreateProcessA(null, posix_str.drop_const(child->command),
                null, null, true, CREATE_NO_WINDOW, null, null, &si, &pi));
        if (r != 0) {
            posix_processes_close_pipes(&si, &read_out, &read_err, &write_in);
        }
    }
    if (r == 0) {
        posix_win32_close_handle(pi.hThread);
        pi.hThread = null;
        posix_win32_close_handle(si.hStdOutput);
        posix_win32_close_handle(si.hStdError);
        posix_win32_close_handle(si.hStdInput);
        si.hStdOutput = INVALID_HANDLE_VALUE;
        si.hStdError  = INVALID_HANDLE_VALUE;
        si.hStdInput  = INVALID_HANDLE_VALUE;
        bool done = false;
        while (!done && r == 0) {
            if (child->timeout > 0 && posix_clock.seconds() > deadline) {
                r = posix_b2e(TerminateProcess(pi.hProcess, ERROR_SEM_TIMEOUT));
                if (r == 0) { done = true; }
            }
            if (r == 0) { r = posix_processes_child_write(child->in, write_in); }
            if (r == 0) { r = posix_processes_child_read(child->out, read_out); }
            if (r == 0) { r = posix_processes_child_read(child->err, read_err); }
            if (!done) {
                DWORD ix = WaitForSingleObject(pi.hProcess, 0);
                done = ix == WAIT_OBJECT_0 || r == ERROR_BROKEN_PIPE;
            }
            if (!done) { posix_thread.yield(); }
        }
        if (r == ERROR_BROKEN_PIPE) { r = 0; } // EOF
        DWORD xc = 0;
        int rx = posix_b2e(GetExitCodeProcess(pi.hProcess, &xc));
        if (rx == 0) { child->exit_code = xc; } else if (r == 0) { r = rx; }
        posix_processes_close_pipes(&si, &read_out, &read_err, &write_in);
        posix_win32_close_handle(pi.hProcess);
    }
    return r;
}

struct posix_processes_io_merge {
    struct posix_stream_if stream;
    struct posix_stream_if* output;
    int error;
};

static int posix_processes_merge_write(struct posix_stream_if* stream, const void* data,
        int64_t bytes, int64_t* transferred) {
    if (transferred != null) { *transferred = 0; }
    struct posix_processes_io_merge* s = (struct posix_processes_io_merge*)stream;
    if (s->output != null && bytes > 0) {
        s->error = s->output->write(s->output, data, bytes, transferred);
    }
    return s->error;
}

static int posix_processes_popen(const char* command, int32_t *exit_code,
        struct posix_stream_if* output, fp64_t timeout) {
    posix_not_null(output);
    struct posix_processes_io_merge merge = {
        .stream = { .write = posix_processes_merge_write },
        .output = output, .error = 0
    };
    struct posix_processes_child child = {
        .command = command, .in = null,
        .out = &merge.stream, .err = &merge.stream,
        .exit_code = 0, .timeout = timeout
    };
    int r = posix_processes_run(&child);
    if (exit_code != null) { *exit_code = (int32_t)child.exit_code; }
    uint8_t zero = 0;
    merge.stream.write(&merge.stream, &zero, 1, null);
    if (r == 0 && merge.error != 0) { r = merge.error; }
    return r;
}

static int posix_processes_spawn(const char* command) {
    int r = 0;
    STARTUPINFOA si = {
        .cb = sizeof(STARTUPINFOA),
        .dwFlags = STARTF_USESHOWWINDOW,
        .wShowWindow = SW_HIDE,
        .hStdInput  = INVALID_HANDLE_VALUE,
        .hStdOutput = INVALID_HANDLE_VALUE,
        .hStdError  = INVALID_HANDLE_VALUE
    };
    const DWORD flags = CREATE_BREAKAWAY_FROM_JOB | CREATE_NO_WINDOW |
                        CREATE_NEW_PROCESS_GROUP | DETACHED_PROCESS;
    PROCESS_INFORMATION pi = { .hProcess = null, .hThread = null };
    r = posix_b2e(CreateProcessA(null, posix_str.drop_const(command), null, null,
            false, flags, null, null, &si, &pi));
    if (r == 0) {
        posix_win32_close_handle(pi.hProcess);
        posix_win32_close_handle(pi.hThread);
    }
    return r;
}

static const char* posix_processes_name(void) {
    static char mn[posix_files_max_path];
    if (mn[0] == 0) {
        posix_fatal_win32err(GetModuleFileNameA(null, mn, posix_countof(mn)));
    }
    return mn;
}

#else

static const char* posix_processes_name(void) {
    static char path[1024] = {0};
    if (path[0] == 0) {
#if defined(__APPLE__)
        uint32_t size = sizeof(path);
        _NSGetExecutablePath(path, &size);
#elif defined(__linux__)
        ssize_t len = readlink("/proc/self/exe", path, sizeof(path) - 1);
        if (len != -1) { path[len] = '\0'; }
#endif
    }
    return path;
}

static int posix_processes_pids(const char* name, uint64_t* pids, int32_t size, int32_t *count) {
    *count = 0;
    char cmd[1024];
    snprintf(cmd, sizeof(cmd), "pgrep -f \"%s\"", name);
    FILE* fp = popen(cmd, "r");
    if (fp == null) return errno;
    
    char line[256];
    while (fgets(line, sizeof(line), fp) != null) {
        if (*count < size && pids != null) {
            pids[*count] = (uint64_t)strtoull(line, null, 10);
        }
        (*count)++;
    }
    pclose(fp);
    return *count > size ? ENOBUFS : 0;
}

static uint64_t posix_processes_pid(const char* name) {
    uint64_t pids[1] = {0};
    int32_t count = 0;
    posix_processes_pids(name, pids, 1, &count);
    return count > 0 ? pids[0] : 0;
}

static int posix_processes_nameof(uint64_t pid, char* name, int32_t count) {
#if defined(__linux__)
    char path[256];
    snprintf(path, sizeof(path), "/proc/%llu/exe", (unsigned long long)pid);
    ssize_t len = readlink(path, name, (size_t)(count - 1));
    if (len == -1) return errno;
    name[len] = '\0';
    return 0;
#else
    // macOS requires proc_pidinfo which is complex. Stubbing for brevity.
    snprintf(name, count, "unknown_macos_pid_%llu", (unsigned long long)pid);
    return 0;
#endif
}

static bool posix_processes_present(uint64_t pid) {
    return kill((pid_t)pid, 0) == 0;
}

static int posix_processes_kill(uint64_t pid, fp64_t timeout_seconds) {
    (void)timeout_seconds; // POSIX kill is immediate signal
    return kill((pid_t)pid, SIGKILL) == 0 ? 0 : errno;
}

static int posix_processes_kill_all(const char* name, fp64_t timeout_seconds) {
    (void)timeout_seconds;
    char cmd[1024];
    snprintf(cmd, sizeof(cmd), "pkill -9 -f \"%s\"", name);
    return system(cmd) == 0 ? 0 : errno;
}

static bool posix_processes_is_elevated(void) {
    return geteuid() == 0;
}

static int posix_processes_restart_elevated(void) {
    if (!posix_processes_is_elevated()) {
        char cmd[1024];
        snprintf(cmd, sizeof(cmd), "sudo \"%s\"", posix_processes_name());
        return system(cmd) == 0 ? 0 : errno;
    }
    return 0;
}

static int posix_processes_run(struct posix_processes_child* child) {
    int stdout_pipe[2], stderr_pipe[2], stdin_pipe[2];
    if (pipe(stdout_pipe) < 0 || pipe(stderr_pipe) < 0 || pipe(stdin_pipe) < 0) return errno;

    pid_t pid = fork();
    if (pid < 0) return errno;

    if (pid == 0) { // Child
        dup2(stdin_pipe[0], STDIN_FILENO);
        dup2(stdout_pipe[1], STDOUT_FILENO);
        dup2(stderr_pipe[1], STDERR_FILENO);
        close(stdin_pipe[1]); close(stdout_pipe[0]); close(stderr_pipe[0]);
        
        execl("/bin/sh", "sh", "-c", child->command, null);
        exit(127);
    } else { // Parent
        close(stdin_pipe[0]); close(stdout_pipe[1]); close(stderr_pipe[1]);
        
        char buf[4096];
        ssize_t n;
        if (child->out != null) {
            while ((n = read(stdout_pipe[0], buf, sizeof(buf))) > 0) {
                child->out->write(child->out, buf, n, null);
            }
        }
        if (child->err != null) {
            while ((n = read(stderr_pipe[0], buf, sizeof(buf))) > 0) {
                child->err->write(child->err, buf, n, null);
            }
        }
        
        int status;
        waitpid(pid, &status, 0);
        if (WIFEXITED(status)) {
            child->exit_code = WEXITSTATUS(status);
        }
        
        close(stdin_pipe[1]); close(stdout_pipe[0]); close(stderr_pipe[0]);
    }
    return 0;
}

static int posix_processes_popen(const char* command, int32_t *xc, struct posix_stream_if* output, fp64_t timeout_seconds) {
    struct posix_processes_child child = {
        .command = command,
        .in = null, .out = output, .err = output,
        .exit_code = 0, .timeout = timeout_seconds
    };
    int r = posix_processes_run(&child);
    if (xc) *xc = (int32_t)child.exit_code;
    return r;
}

static int posix_processes_spawn(const char* command) {
    pid_t pid = fork();
    if (pid == 0) {
        setsid(); // Detach from terminal
        execl("/bin/sh", "sh", "-c", command, null);
        exit(127);
    }
    return pid < 0 ? errno : 0;
}

#endif // _WIN32

static void posix_processes_test(void) {}

struct posix_processes_if posix_processes = {
    .name             = posix_processes_name,
    .pid              = posix_processes_pid,
    .pids             = posix_processes_pids,
    .nameof           = posix_processes_nameof,
    .present          = posix_processes_present,
    .kill             = posix_processes_kill,
    .kill_all         = posix_processes_kill_all,
    .is_elevated      = posix_processes_is_elevated,
    .restart_elevated = posix_processes_restart_elevated,
    .run              = posix_processes_run,
    .popen            = posix_processes_popen,
    .spawn            = posix_processes_spawn,
    .test             = posix_processes_test
};

// _______________________________ posix_loader.c ________________________________

#if defined(_WIN32)

static void* posix_loader_all;

static void* posix_loader_sym_all(const char* name) {
    void* sym = null;
    DWORD bytes = 0;
    posix_fatal_win32err(EnumProcessModules(GetCurrentProcess(), null, 0, &bytes));
    HMODULE* modules = null;
    posix_fatal_if_error(posix_heap.allocate(null, (void**)&modules, bytes, false));
    posix_fatal_win32err(EnumProcessModules(GetCurrentProcess(), modules, bytes, &bytes));
    const int32_t n = bytes / (int32_t)sizeof(HMODULE);
    for (int32_t i = 0; i < n && sym == null; i++) {
        sym = (void*)GetProcAddress(modules[i], name);
    }
    if (sym == null) { sym = (void*)GetProcAddress(GetModuleHandleA(null), name); }
    posix_heap.deallocate(null, modules);
    return sym;
}

static void* posix_loader_open(const char* filename, int32_t mode) {
    (void)mode;
    return filename == null ? &posix_loader_all : (void*)LoadLibraryA(filename);
}

static void* posix_loader_sym(void* handle, const char* name) {
    return handle == &posix_loader_all ?
            posix_loader_sym_all(name) :
            (void*)GetProcAddress((HMODULE)handle, name);
}

static void posix_loader_close(void* handle) {
    if (handle != &posix_loader_all) { posix_fatal_win32err(FreeLibrary(handle)); }
}

#else

static void* posix_loader_open(const char* filename, int32_t mode) {
    int m = 0;
    if (mode == posix_loader.local) m = RTLD_LOCAL | RTLD_LAZY;
    if (mode == posix_loader.lazy) m = RTLD_LAZY;
    if (mode == posix_loader.now) m = RTLD_NOW;
    if (mode == posix_loader.global) m = RTLD_GLOBAL | RTLD_LAZY;
    return dlopen(filename, m);
}

static void* posix_loader_sym(void* handle, const char* name) {
    return dlsym(handle != null ? handle : RTLD_DEFAULT, name);
}

static void posix_loader_close(void* handle) {
    if (handle != null) { dlclose(handle); }
}

#endif // _WIN32

static void posix_loader_test(void) {}

struct posix_loader_if posix_loader = {
    .local  = 0,
    .lazy   = 1,
    .now    = 2,
    .global = 256,
    .open   = posix_loader_open,
    .sym    = posix_loader_sym,
    .close  = posix_loader_close,
    .test   = posix_loader_test
};

// _________________________________ posix_num.c _________________________________

static struct posix_num128 posix_num_add128(const struct posix_num128 a, const struct posix_num128 b) {
    struct posix_num128 r = a;
    r.hi += b.hi;
    r.lo += b.lo;
    if (r.lo < b.lo) { r.hi++; }
    return r;
}

static struct posix_num128 posix_num_sub128(const struct posix_num128 a, const struct posix_num128 b) {
    struct posix_num128 r = a;
    r.hi -= b.hi;
    if (r.lo < b.lo) { r.hi--; }
    r.lo -= b.lo;
    return r;
}

static struct posix_num128 posix_num_mul64x64(uint64_t a, uint64_t b) {
    uint64_t a_lo = (uint32_t)a;
    uint64_t a_hi = a >> 32;
    uint64_t b_lo = (uint32_t)b;
    uint64_t b_hi = b >> 32;
    uint64_t low = a_lo * b_lo;
    uint64_t cross1 = a_hi * b_lo;
    uint64_t cross2 = a_lo * b_hi;
    uint64_t high = a_hi * b_hi;
    cross1 += low >> 32;
    cross1 += cross2;
    high += ((uint64_t)(cross1 < cross2 != 0)) << 32;
    high = high + (cross1 >> 32);
    low = ((cross1 & 0xFFFFFFFF) << 32) + (low & 0xFFFFFFFF);
    return (struct posix_num128){.lo = low, .hi = high };
}

#if defined(_MSC_VER)
static inline int32_t posix_num_ctz32(uint32_t x) {
    unsigned long i = 0; _BitScanForward(&i, x); return (int32_t)i;
}
#else
static inline int32_t posix_num_ctz32(uint32_t x) { return (int32_t)__builtin_ctz(x); }
#endif

static inline void posix_num_shift128_left(struct posix_num128* n) {
    const uint64_t top = (1ULL << 63);
    n->hi = (n->hi << 1) | ((n->lo & top) ? 1 : 0);
    n->lo = (n->lo << 1);
}

static inline void posix_num_shift128_right(struct posix_num128* n) {
    const uint64_t top = (1ULL << 63);
    n->lo = (n->lo >> 1) | ((n->hi & 0x1) ? top : 0);
    n->hi = (n->hi >> 1);
}

static inline bool posix_num_less128(const struct posix_num128 a, const struct posix_num128 b) {
    return a.hi < b.hi || (a.hi == b.hi && a.lo < b.lo);
}

static inline bool posix_num_high_bit128(const struct posix_num128 a) {
    return (int64_t)a.hi < 0;
}

// Portable shift-and-subtract long division of (a*b) by divisor; no __int128.
static uint64_t posix_num_muldiv128(uint64_t a, uint64_t b, uint64_t divisor) {
    posix_swear(divisor > 0, "divisor: %lld", divisor);
    struct posix_num128 r = posix_num_mul64x64(a, b);
    uint64_t q = 0;
    if (r.hi >= divisor) {
        q = UINT64_MAX; // overflow
    } else {
        int32_t shift = 0;
        struct posix_num128 d = { .hi = 0, .lo = divisor };
        while (!posix_num_high_bit128(d) && posix_num_less128(d, r)) {
            posix_num_shift128_left(&d);
            shift++;
        }
        while (shift >= 0 && (d.hi != 0 || d.lo != 0)) {
            if (!posix_num_less128(r, d)) {
                r = posix_num_sub128(r, d);
                q |= (1ULL << shift);
            }
            posix_num_shift128_right(&d);
            shift--;
        }
    }
    return q;
}

static uint32_t posix_num_gcd32(uint32_t u, uint32_t v) {
    if (u == 0) return v;
    if (v == 0) return u;
    uint32_t shift = posix_num_ctz32(u | v);
    u >>= posix_num_ctz32(u);
    do {
        v >>= posix_num_ctz32(v);
        if (u > v) { uint32_t t = v; v = u; u = t; }
        v = v - u;
    } while (v != 0);
    return u << shift;
}

static uint32_t posix_num_random32(uint32_t *state) {
    uint32_t z = (*state += 0x6D2B79F5UL);
    z = (z ^ (z >> 15)) * (z | 1UL);
    z ^= z + (z ^ (z >> 7)) * (z | 61UL);
    return z ^ (z >> 14);
}

static uint64_t posix_num_random64(uint64_t *state) {
    const uint64_t s = *state;
    const uint64_t z = (s ^ s >> 25) * (*state += 0x6A5D39EAE12657AAULL);
    return z ^ (z >> 22);
}

static uint32_t posix_num_hash32(const char *data, int64_t len) {
    uint32_t hash  = 0x811c9dc5;
    uint32_t prime = 0x01000193;
    if (len > 0) {
        for (int64_t i = 0; i < len; i++) { hash ^= (uint32_t)data[i]; hash *= prime; }
    } else {
        for (int64_t i = 0; data[i] != 0; i++) { hash ^= (uint32_t)data[i]; hash *= prime; }
    }
    return hash;
}

static uint64_t posix_num_hash64(const char *data, int64_t len) {
    uint64_t hash  = 0xcbf29ce484222325;
    uint64_t prime = 0x100000001b3;
    if (len > 0) {
        for (int64_t i = 0; i < len; i++) { hash ^= (uint64_t)data[i]; hash *= prime; }
    } else {
        for (int64_t i = 0; data[i] != 0; i++) { hash ^= (uint64_t)data[i]; hash *= prime; }
    }
    return hash;
}

static void posix_num_test(void) {}

struct posix_num_if posix_num = {
    .add128    = posix_num_add128,
    .sub128    = posix_num_sub128,
    .mul64x64  = posix_num_mul64x64,
    .muldiv128 = posix_num_muldiv128,
    .gcd32     = posix_num_gcd32,
    .random32  = posix_num_random32,
    .random64  = posix_num_random64,
    .hash32    = posix_num_hash32,
    .hash64    = posix_num_hash64,
    .test      = posix_num_test
};

// ______________________________ posix_random.c _________________________________

// The universal generator lives in core (`struct rng` + rng_next); posix just
// piggybacks on it via a process-global state.
static struct rng posix_random_rng;
static bool       posix_random_seeded;

void posix_random_seed(uint64_t seed) {
    rng_seed(&posix_random_rng, seed);
    posix_random_seeded = true;
}

static void posix_random_ensure_seeded(void) {
    if (!posix_random_seeded) {
        posix_random_seed(posix_clock.nanoseconds());
    }
}

uint64_t posix_random(void) {
    posix_random_ensure_seeded();
    return rng_next(&posix_random_rng);
}

fp64_t posix_random_uniform(void) {
    posix_random_ensure_seeded();
    return (fp64_t)rng_uniform(&posix_random_rng);
}

// ______________________________ posix_backtrace.c ______________________________

#if defined(_WIN32)

// "called from" glyph: North West Arrow with Hook (U+2923), inlined literal so
// posix.* stays free of the ui glyph table.
#define posix_backtrace_glyph_called_from "\xE2\xA4\xA3"

static void*  posix_backtrace_process;
static DWORD  posix_backtrace_pid;

typedef posix_begin_packed struct posix_symbol_info {
    SYMBOL_INFO info; char name[posix_backtrace_max_symbol];
} posix_end_packed posix_symbol_info_t;

static void posix_backtrace_init(void) {
    if (posix_backtrace_process == null) {
        if (GetModuleHandleA("dbghelp.dll") == null) { posix_fatal_win32err(LoadLibraryA("dbghelp.dll")); }
        if (GetModuleHandleA("imagehlp.dll") == null) { posix_fatal_win32err(LoadLibraryA("imagehlp.dll")); }
        DWORD options = SymGetOptions();
        options |= SYMOPT_NO_PROMPTS | SYMOPT_LOAD_LINES | SYMOPT_UNDNAME |
                   SYMOPT_LOAD_ANYTHING;
        posix_swear(SymSetOptions(options));
        posix_backtrace_pid = GetProcessId(GetCurrentProcess());
        posix_swear(posix_backtrace_pid != 0);
        posix_backtrace_process = OpenProcess(PROCESS_ALL_ACCESS, false, posix_backtrace_pid);
        posix_swear(posix_backtrace_process != null);
        posix_swear(SymInitialize(posix_backtrace_process, null, true), "%s",
                    posix_str.error(posix_core.err()).s);
    }
}

static void posix_backtrace_capture(struct posix_backtrace* bt, int32_t skip) {
    posix_backtrace_init();
    SetLastError(0);
    bt->frames = CaptureStackBackTrace(1 + skip, posix_countof(bt->stack),
        bt->stack, (DWORD*)&bt->hash);
    bt->error = posix_core.err();
}

static bool posix_backtrace_function(DWORD64 pc, SYMBOL_INFO* si) {
    bool found = false;
    const DWORD64 module_base = SymGetModuleBase64(posix_backtrace_process, pc);
    if (module_base != 0) {
        const DWORD flags = GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT |
                            GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS;
        HMODULE module_handle = null;
        if (GetModuleHandleExA(flags, (const char*)pc, &module_handle)) {
            DWORD bytes = 0;
            IMAGE_EXPORT_DIRECTORY* dir = (IMAGE_EXPORT_DIRECTORY*)
                    ImageDirectoryEntryToDataEx(module_handle, true,
                            IMAGE_DIRECTORY_ENTRY_EXPORT, &bytes, null);
            if (dir) {
                uint8_t* m = (uint8_t*)module_handle;
                DWORD* functions = (DWORD*)(m + dir->AddressOfFunctions);
                DWORD* names = (DWORD*)(m + dir->AddressOfNames);
                WORD* ordinals = (WORD*)(m + dir->AddressOfNameOrdinals);
                DWORD64 address = 0;
                DWORD64 min_distance = (DWORD64)-1;
                const char* function = null;
                DWORD n_names = dir->NumberOfNames;
                DWORD max_names = bytes / (DWORD)sizeof(DWORD);
                if (n_names > max_names) { n_names = max_names; }
                for (DWORD i = 0; i < n_names; i++) {
                    WORD ord = ordinals[i];
                    if (ord < dir->NumberOfFunctions) {
                        DWORD64 fa = (DWORD64)(m + functions[ord]);
                        if (fa <= pc) {
                            DWORD64 distance = pc - fa;
                            if (distance < min_distance) {
                                min_distance = distance;
                                address = fa;
                                function = (const char*)(m + names[i]);
                            }
                        }
                    }
                }
                if (function != null) {
                    si->ModBase = (uint64_t)m;
                    snprintf(si->Name, si->MaxNameLen - 1, "%s", function);
                    si->Name[si->MaxNameLen - 1] = 0x00;
                    si->NameLen = (DWORD)strlen(si->Name);
                    si->Address = address;
                    found = true;
                }
            }
        }
    }
    return found;
}

static void posix_backtrace_symbolize_inline_frame(struct posix_backtrace* bt,
        int32_t i, DWORD64 pc, DWORD inline_context, posix_symbol_info_t* si) {
    si->info.Name[0] = 0;
    si->info.NameLen = 0;
    bt->file[i][0] = 0;
    bt->line[i] = 0;
    bt->symbol[i][0] = 0;
    DWORD64 displacement = 0;
    if (SymFromInlineContext(posix_backtrace_process, pc, inline_context,
                            &displacement, &si->info)) {
        posix_str_printf(bt->symbol[i], "%s", si->info.Name);
    } else {
        bt->error = posix_core.err();
    }
    IMAGEHLP_LINE64 li = { .SizeOfStruct = sizeof(IMAGEHLP_LINE64) };
    DWORD offset = 0;
    if (SymGetLineFromInlineContext(posix_backtrace_process, pc, inline_context, 0,
                                    &offset, &li)) {
        posix_str_printf(bt->file[i], "%s", li.FileName);
        bt->line[i] = li.LineNumber;
    }
}

static int32_t posix_backtrace_symbolize_frame(struct posix_backtrace* bt, int32_t i) {
    const DWORD64 pc = (DWORD64)bt->stack[i];
    posix_symbol_info_t si = {
        .info = { .SizeOfStruct = sizeof(SYMBOL_INFO),
                  .MaxNameLen = posix_countof(si.name) }
    };
    bt->file[i][0] = 0;
    bt->line[i] = 0;
    bt->symbol[i][0] = 0;
    DWORD64 offsetFromSymbol = 0;
    const DWORD inline_count = SymAddrIncludeInlineTrace(posix_backtrace_process, pc);
    if (inline_count > 0) {
        DWORD ic = 0;
        DWORD fi = 0;
        if (SymQueryInlineTrace(posix_backtrace_process, pc, 0, pc, pc, &ic, &fi)) {
            for (DWORD k = 0; k < inline_count; k++, ic++) {
                posix_backtrace_symbolize_inline_frame(bt, i, pc, ic, &si);
                i++;
            }
        }
    } else {
        if (SymFromAddr(posix_backtrace_process, pc, &offsetFromSymbol, &si.info)) {
            posix_str_printf(bt->symbol[i], "%s", si.info.Name);
            DWORD d = 0;
            IMAGEHLP_LINE64 ln = { .SizeOfStruct = sizeof(IMAGEHLP_LINE64) };
            if (SymGetLineFromAddr64(posix_backtrace_process, pc, &d, &ln)) {
                bt->line[i] = ln.LineNumber;
                posix_str_printf(bt->file[i], "%s", ln.FileName);
            } else {
                bt->error = posix_core.err();
                if (posix_backtrace_function(pc, &si.info)) {
                    GetModuleFileNameA((HANDLE)si.info.ModBase, bt->file[i],
                        posix_countof(bt->file[i]) - 1);
                    bt->file[i][posix_countof(bt->file[i]) - 1] = 0;
                    bt->line[i] = 0;
                } else {
                    bt->file[i][0] = 0x00;
                    bt->line[i] = 0;
                }
            }
            i++;
        } else {
            bt->error = posix_core.err();
            if (posix_backtrace_function(pc, &si.info)) {
                posix_str_printf(bt->symbol[i], "%s", si.info.Name);
                GetModuleFileNameA((HANDLE)si.info.ModBase, bt->file[i],
                    posix_countof(bt->file[i]) - 1);
                bt->file[i][posix_countof(bt->file[i]) - 1] = 0;
                bt->error = 0;
                i++;
            }
        }
    }
    return i;
}

static void posix_backtrace_symbolize_backtrace(struct posix_backtrace* bt) {
    posix_assert(!bt->symbolized);
    bt->error = 0;
    posix_backtrace_init();
    int32_t n = bt->frames;
    void* stack[posix_countof(bt->stack)];
    memcpy(stack, bt->stack, n * sizeof(stack[0]));
    bt->frames = 0;
    for (int32_t i = 0; i < n && bt->frames < posix_countof(bt->stack); i++) {
        bt->stack[bt->frames] = stack[i];
        bt->frames = posix_backtrace_symbolize_frame(bt, i);
    }
    bt->symbolized = true;
}

static void posix_backtrace_symbolize(struct posix_backtrace* bt) {
    if (!bt->symbolized) { posix_backtrace_symbolize_backtrace(bt); }
}

static const char* posix_backtrace_stops[] = {
    "main", "WinMain", "BaseThreadInitThunk", "RtlUserThreadStart",
    "mainCRTStartup", "WinMainCRTStartup", "invoke_main",
    "NdrInterfacePointerMemorySize", null
};

static void posix_backtrace_trace(const struct posix_backtrace* bt, const char* stop) {
    posix_assert(bt->symbolized, "need posix_backtrace.symbolize(bt)");
    const char** alt = stop != null && strcmp(stop, "*") == 0 ?
                       posix_backtrace_stops : null;
    for (int32_t i = 0; i < bt->frames; i++) {
        posix_debug.println(bt->file[i], bt->line[i], bt->symbol[i],
            posix_backtrace_glyph_called_from "%s",
            i == i < bt->frames - 1 ? "\n" : "");
        if (stop != null && strcmp(bt->symbol[i], stop) == 0) { break; }
        const char** s = alt;
        while (s != null && *s != null && strcmp(bt->symbol[i], *s) != 0) { s++; }
        if (s != null && *s != null) { break; }
    }
}

static const char* posix_backtrace_string(const struct posix_backtrace* bt,
        char* text, int32_t count) {
    posix_assert(bt->symbolized, "need posix_backtrace.symbolize(bt)");
    char s[1024];
    char* p = text;
    int32_t n = count;
    for (int32_t i = 0; i < bt->frames && n > 128; i++) {
        int32_t line = bt->line[i];
        const char* file = bt->file[i];
        const char* name = bt->symbol[i];
        if (file[0] != 0 && name[0] != 0) {
            posix_str_printf(s, "%s(%d): %s\n", file, line, name);
        } else if (file[0] == 0 && name[0] != 0) {
            posix_str_printf(s, "%s\n", name);
        }
        s[posix_countof(s) - 1] = 0;
        int32_t k = (int32_t)strlen(s);
        if (k < n) { memcpy(p, s, (size_t)k + 1); p += k; n -= k; }
    }
    return text;
}

static void posix_backtrace_context(posix_thread_t thread, const void* ctx,
        struct posix_backtrace* bt) {
    CONTEXT* context = (CONTEXT*)ctx;
    STACKFRAME64 stack_frame = { 0 };
    int machine_type = IMAGE_FILE_MACHINE_UNKNOWN;
    #if defined(_M_ARM64)
        machine_type = IMAGE_FILE_MACHINE_ARM64;
        stack_frame = (STACKFRAME64){
            .AddrPC    = {.Offset = context->Pc, .Mode = AddrModeFlat},
            .AddrFrame = {.Offset = context->Fp, .Mode = AddrModeFlat},
            .AddrStack = {.Offset = context->Sp, .Mode = AddrModeFlat}
        };
    #elif defined(_M_X64)
        machine_type = IMAGE_FILE_MACHINE_AMD64;
        stack_frame = (STACKFRAME64){
            .AddrPC    = {.Offset = context->Rip, .Mode = AddrModeFlat},
            .AddrFrame = {.Offset = context->Rbp, .Mode = AddrModeFlat},
            .AddrStack = {.Offset = context->Rsp, .Mode = AddrModeFlat}
        };
    #else
        #error "Unsupported platform"
    #endif
    posix_backtrace_init();
    while (StackWalk64(machine_type, posix_backtrace_process, (HANDLE)thread,
            &stack_frame, context, null, SymFunctionTableAccess64,
            SymGetModuleBase64, null)) {
        DWORD64 pc = stack_frame.AddrPC.Offset;
        if (pc == 0) { break; }
        if (bt->frames < posix_countof(bt->stack)) {
            bt->stack[bt->frames] = (void*)pc;
            bt->frames = posix_backtrace_symbolize_frame(bt, bt->frames);
        }
    }
    bt->symbolized = true;
}

typedef struct { char name[32]; } posix_backtrace_thread_name_t;

static posix_backtrace_thread_name_t posix_backtrace_thread_name(HANDLE thread) {
    posix_backtrace_thread_name_t tn;
    tn.name[0] = 0;
    wchar_t* thread_name = null;
    if (SUCCEEDED(GetThreadDescription(thread, &thread_name))) {
        posix_str.utf16to8(tn.name, posix_countof(tn.name), thread_name, -1);
        LocalFree(thread_name);
    }
    return tn;
}

static void posix_backtrace_capture_thread(HANDLE thread, struct posix_backtrace* bt) {
    bt->frames = 0;
    posix_swear(posix_thread.id_of(thread) != posix_thread.id());
    if (SuspendThread(thread) != (DWORD)-1) {
        CONTEXT context = { .ContextFlags = CONTEXT_FULL };
        GetThreadContext(thread, &context);
        posix_backtrace.context(thread, &context, bt);
        if (ResumeThread(thread) == (DWORD)-1) { ExitProcess(0xBD); }
    }
}

static void posix_backtrace_trace_self(const char* stop) {
    struct posix_backtrace bt = {{0}};
    posix_backtrace.capture(&bt, 2);
    posix_backtrace.symbolize(&bt);
    posix_backtrace.trace(&bt, stop);
}

static void posix_backtrace_trace_all_but_self(void) {
    posix_backtrace_init();
    posix_assert(posix_backtrace_process != null && posix_backtrace_pid != 0);
    HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, 0);
    if (snapshot != INVALID_HANDLE_VALUE) {
        THREADENTRY32 te = { .dwSize = sizeof(THREADENTRY32) };
        if (Thread32First(snapshot, &te)) {
            do {
                if (te.th32OwnerProcessID == posix_backtrace_pid) {
                    static const DWORD flags = THREAD_ALL_ACCESS |
                       THREAD_SUSPEND_RESUME | THREAD_GET_CONTEXT;
                    uint32_t tid = te.th32ThreadID;
                    if (tid != (uint32_t)posix_thread.id()) {
                        HANDLE thread = OpenThread(flags, false, tid);
                        if (thread != null) {
                            struct posix_backtrace bt = {0};
                            posix_backtrace_capture_thread(thread, &bt);
                            posix_backtrace_thread_name_t tn = posix_backtrace_thread_name(thread);
                            posix_debug.println(">Thread", tid, tn.name,
                                "id 0x%08X (%d)", tid, tid);
                            if (bt.frames > 0) { posix_backtrace.trace(&bt, "*"); }
                            posix_debug.println("<Thread", tid, tn.name, "");
                            posix_win32_close_handle(thread);
                        }
                    }
                }
            } while (Thread32Next(snapshot, &te));
        }
        posix_win32_close_handle(snapshot);
    }
}

#else

static void posix_backtrace_capture(struct posix_backtrace *bt, int32_t skip) {
    void* buffer[posix_backtrace_max_depth];
    int nptrs = backtrace(buffer, posix_backtrace_max_depth);
    bt->frames = posix_min(nptrs - skip, posix_backtrace_max_depth);
    for (int i = 0; i < bt->frames; i++) {
        bt->stack[i] = buffer[i + skip];
    }
    bt->symbolized = false;
    bt->error = 0;
}

static void posix_backtrace_context(posix_thread_t thread, const void* context, struct posix_backtrace *bt) {
    (void)thread; (void)context;
    bt->frames = 0; // Requires complex arch-specific ucontext unwinding.
}

static void posix_backtrace_symbolize(struct posix_backtrace *bt) {
    if (bt->symbolized) return;
    char** strings = backtrace_symbols(bt->stack, bt->frames);
    if (strings == null) {
        bt->error = errno;
        return;
    }
    for (int i = 0; i < bt->frames; i++) {
        snprintf(bt->symbol[i], posix_backtrace_max_symbol, "%s", strings[i]);
        bt->file[i][0] = '\0';
        bt->line[i] = 0;
    }
    free(strings);
    bt->symbolized = true;
}

static void posix_backtrace_trace(const struct posix_backtrace* bt, const char* stop) {
    (void)stop;
    for (int i = 0; i < bt->frames; i++) {
        posix_debug.println("", 0, bt->symbol[i], "");
    }
}

static void posix_backtrace_trace_self(const char* stop) {
    struct posix_backtrace bt = {0};
    posix_backtrace_capture(&bt, 1);
    posix_backtrace_symbolize(&bt);
    posix_backtrace_trace(&bt, stop);
}

static void posix_backtrace_trace_all_but_self(void) {
    // POSIX lacks standard cross-thread unwinding without ptrace/signals.
}

static const char* posix_backtrace_string(const struct posix_backtrace* bt, char* text, int32_t count) {
    text[0] = '\0';
    for (int i = 0; i < bt->frames; i++) {
        strncat(text, bt->symbol[i], count - strlen(text) - 1);
        strncat(text, "\n", count - strlen(text) - 1);
    }
    return text;
}

#endif // _WIN32

static void posix_backtrace_test(void) {}

struct posix_backtrace_if posix_backtrace = {
    .capture            = posix_backtrace_capture,
    .context            = posix_backtrace_context,
    .symbolize          = posix_backtrace_symbolize,
    .trace              = posix_backtrace_trace,
    .trace_self         = posix_backtrace_trace_self,
    .trace_all_but_self = posix_backtrace_trace_all_but_self,
    .string             = posix_backtrace_string,
    .test               = posix_backtrace_test
};

// _________________________________ posix_nls.c _________________________________

static void posix_nls_init(void) {
    setlocale(LC_ALL, "");
}

static const char* posix_nls_locale(void) {
    const char* l = setlocale(LC_CTYPE, null);
    return l ? l : "en-US";
}

static int posix_nls_set_locale(const char* locale) {
    return setlocale(LC_ALL, locale) != null ? 0 : EINVAL;
}

static const char* posix_nls_str(const char* defau1t) {
    return defau1t; // Stub: wire to gettext if resource bundles are used
}

static int32_t posix_nls_strid(const char* s) {
    (void)s;
    return -1;
}

static const char* posix_nls_string(int32_t strid, const char* defau1t) {
    (void)strid;
    return defau1t;
}

struct posix_nls_if posix_nls = {
    .init       = posix_nls_init,
    .locale     = posix_nls_locale,
    .set_locale = posix_nls_set_locale,
    .str        = posix_nls_str,
    .strid      = posix_nls_strid,
    .string     = posix_nls_string
};

// ______________________________ posix_clipboard.c ______________________________

#if defined(_WIN32)

static int posix_clipboard_put_text(const char* utf8) {
    int32_t chars = posix_str.utf16_chars(utf8, -1);
    int32_t bytes = (chars + 1) * 2;
    uint16_t* utf16 = null;
    int r = posix_heap.alloc((void**)&utf16, (size_t)bytes);
    if (utf16 != null) {
        posix_str.utf8to16(utf16, bytes, utf8, -1);
        const int32_t n = (int32_t)posix_str.len16(utf16) + 1;
        r = OpenClipboard(GetDesktopWindow()) ? 0 : posix_core.err();
        if (r == 0) { r = EmptyClipboard() ? 0 : posix_core.err(); }
        void* global = null;
        if (r == 0) {
            global = GlobalAlloc(GMEM_MOVEABLE, (size_t)n * 2);
            r = global != null ? 0 : posix_core.err();
        }
        if (r == 0) {
            char* d = (char*)GlobalLock(global);
            posix_not_null(d);
            memcpy(d, utf16, (size_t)n * 2);
            r = posix_b2e(SetClipboardData(CF_UNICODETEXT, global));
            GlobalUnlock(global);
            if (r != 0) { GlobalFree(global); } // else owned by clipboard now
        }
        if (r == 0) { r = posix_b2e(CloseClipboard()); }
        posix_heap.free(utf16);
    }
    return r;
}

static int posix_clipboard_get_text(char* utf8, int32_t* bytes) {
    posix_not_null(bytes);
    int r = posix_b2e(OpenClipboard(GetDesktopWindow()));
    if (r == 0) {
        HANDLE global = GetClipboardData(CF_UNICODETEXT);
        if (global == null) {
            r = posix_core.err();
        } else {
            uint16_t* utf16 = (uint16_t*)GlobalLock(global);
            if (utf16 != null) {
                int32_t utf8_bytes = posix_str.utf8_bytes(utf16, -1);
                if (utf8 != null) {
                    char* decoded = (char*)malloc((size_t)utf8_bytes);
                    if (decoded == null) {
                        r = (int)ERROR_OUTOFMEMORY;
                    } else {
                        posix_str.utf16to8(decoded, utf8_bytes, utf16, -1);
                        int32_t nn = *bytes < utf8_bytes ? *bytes : utf8_bytes;
                        memcpy(utf8, decoded, (size_t)nn);
                        free(decoded);
                        if (nn < utf8_bytes) { r = (int)ERROR_INSUFFICIENT_BUFFER; }
                    }
                }
                *bytes = utf8_bytes;
                GlobalUnlock(global);
            }
        }
        r = posix_b2e(CloseClipboard());
    }
    return r;
}

#else

static int posix_clipboard_put_text(const char* s) {
    char cmd[1024];
#if defined(__APPLE__)
    snprintf(cmd, sizeof(cmd), "echo \"%s\" | pbcopy", s);
#else
    snprintf(cmd, sizeof(cmd), "echo \"%s\" | xclip -selection clipboard", s);
#endif
    return system(cmd) == 0 ? 0 : errno;
}

static int posix_clipboard_get_text(char* text, int32_t* bytes) {
#if defined(__APPLE__)
    FILE* fp = popen("pbpaste", "r");
#else
    FILE* fp = popen("xclip -selection clipboard -o", "r");
#endif
    if (fp == null) return errno;
    size_t n = fread(text, 1, *bytes - 1, fp);
    text[n] = '\0';
    *bytes = (int32_t)n;
    pclose(fp);
    return 0;
}

#endif // _WIN32

static int posix_clipboard_put_image(ui_bitmap_t* image) {
    (void)image;
    return ENOSYS;
}

static void posix_clipboard_test(void) {}

struct posix_clipboard_if posix_clipboard = {
    .put_text  = posix_clipboard_put_text,
    .get_text  = posix_clipboard_get_text,
    .put_image = posix_clipboard_put_image,
    .test      = posix_clipboard_test
};

// _______________________________ posix_static.c ________________________________

#if defined(_MSC_VER)

// Referenced by the posix_static_init() .CRT$XCU constructor machinery so the
// MSVC linker keeps the constructor entries (see posix.h).
static void*   posix_static_symbol_reference[1024];
static int32_t posix_static_symbol_reference_count;

void* posix_force_symbol_reference(void* symbol) {
    if (posix_static_symbol_reference_count < posix_countof(posix_static_symbol_reference)) {
        posix_static_symbol_reference[posix_static_symbol_reference_count] = symbol;
        posix_static_symbol_reference_count++;
    }
    return symbol;
}

#endif

void posix_static_init_test(void) {}



#endif // posix_implementation

