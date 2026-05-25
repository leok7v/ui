#ifndef POSIX_H
#define POSIX_H

#include "core.h"

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
#include <unistd.h>

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

#ifndef __cplusplus
    #define posix_thread_local _Thread_local
#endif

#if defined(__GNUC__) || defined(__clang__)
#define posix_attribute_packed __attribute__((packed))
#define posix_begin_packed
#define posix_end_packed posix_attribute_packed
#define posix_aligned_8 __attribute__((aligned(8)))
#define posix_unused(name) name __attribute__((unused))
#else
#define posix_attribute_packed
#define posix_begin_packed
#define posix_end_packed
#define posix_aligned_8
#define posix_unused(name) name
#endif

// Dynamic stack allocation (VLAs are standard in C99, optional in C11)
// Avoiding alloca() when possible.
#define posix_stackalloc(n) (__builtin_alloca(n))

// _________________________________ posix_str.h _________________________________

posix_begin_c

typedef struct posix_str64_t { char s[64]; } posix_str64_t;
typedef struct posix_str128_t { char s[128]; } posix_str128_t;
typedef struct posix_str1024_t { char s[1024]; } posix_str1024_t;
typedef struct posix_str32K_t { char s[32 * 1024]; } posix_str32K_t;

#define posix_str_printf(s, ...) posix_str.format((s), posix_countof(s), "" __VA_ARGS__)
#define posix_strerr(r) (posix_str.error((r)).s)

typedef struct {
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
    posix_str64_t (*int64_dg)(int64_t v, bool uint, const char* gs);
    posix_str64_t (*int64)(int64_t v);
    posix_str64_t (*uint64)(uint64_t v);
    posix_str64_t (*int64_lc)(int64_t v);
    posix_str64_t (*uint64_lc)(uint64_t v);
    posix_str128_t (*fp)(const char* format, fp64_t v);
    posix_str1024_t (*error)(int32_t error);
    posix_str1024_t (*error_nls)(int32_t error);
    void (*test)(void);
} posix_str_if;

extern posix_str_if posix_str;

posix_end_c

// ________________________________ posix_args.h _________________________________

posix_begin_c

typedef struct {
    int32_t c;
    const char** v;
    const char** env;
    void    (*main)(int32_t argc, const char* argv[], const char** env);
    int32_t (*option_index)(const char* option);
    void    (*remove_at)(int32_t ix);
    bool    (*option_bool)(const char* option);
    bool    (*option_int)(const char* option, int64_t *value);
    const char* (*option_str)(const char* option);
    const char* (*basename)(void);
    void (*fini)(void);
    void (*test)(void);
} posix_args_if;

extern posix_args_if posix_args;

posix_end_c

// ______________________________ posix_backtrace.h ______________________________

posix_begin_c

enum { posix_backtrace_max_depth = 64 };
enum { posix_backtrace_max_symbol = 1024 };

typedef struct thread_s* posix_thread_t;
typedef char posix_backtrace_symbol_t[posix_backtrace_max_symbol];
typedef char posix_backtrace_file_t[512];

typedef struct posix_backtrace_s {
    int32_t frames;
    uint32_t hash;
    int  error;
    void* stack[posix_backtrace_max_depth];
    posix_backtrace_symbol_t symbol[posix_backtrace_max_depth];
    posix_backtrace_file_t file[posix_backtrace_max_depth];
    int32_t line[posix_backtrace_max_depth];
    bool symbolized;
} posix_backtrace_t;

typedef struct {
    void (*capture)(posix_backtrace_t *bt, int32_t skip);
    void (*context)(posix_thread_t thread, const void* context, posix_backtrace_t *bt);
    void (*symbolize)(posix_backtrace_t *bt);
    void (*trace)(const posix_backtrace_t* bt, const char* stop);
    void (*trace_self)(const char* stop);
    void (*trace_all_but_self)(void);
    const char* (*string)(const posix_backtrace_t* bt, char* text, int32_t count);
    void (*test)(void);
} posix_backtrace_if;

extern posix_backtrace_if posix_backtrace;

#define posix_backtrace_here() do {  \
    posix_backtrace_t bt_ = {0};     \
    posix_backtrace.capture(&bt_, 0);\
    posix_backtrace.symbolize(&bt_); \
    posix_backtrace.trace(&bt_, "*");\
} while (0)

posix_end_c

// _______________________________ posix_atomics.h _______________________________

posix_begin_c

typedef struct {
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
} posix_atomics_if;

extern posix_atomics_if posix_atomics;

posix_end_c

// ______________________________ posix_clipboard.h ______________________________

posix_begin_c

typedef struct ui_bitmap ui_bitmap_t;

typedef struct {
    int (*put_text)(const char* s);
    int (*get_text)(char* text, int32_t* bytes);
    int (*put_image)(ui_bitmap_t* image);
    void (*test)(void);
} posix_clipboard_if;

extern posix_clipboard_if posix_clipboard;

posix_end_c

// ________________________________ posix_clock.h ________________________________

posix_begin_c

typedef struct {
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
} posix_clock_if;

extern posix_clock_if posix_clock;

posix_end_c

// _______________________________ posix_config.h ________________________________

posix_begin_c

typedef struct {
    int (*save)(const char* name, const char* key, const void* data, int32_t bytes);
    int32_t (*size)(const char* name, const char* key);
    int32_t (*load)(const char* name, const char* key, void* data, int32_t bytes);
    int (*remove)(const char* name, const char* key);
    int (*clean)(const char* name);
    void (*test)(void);
} posix_config_if;

extern posix_config_if posix_config;

posix_end_c

// ________________________________ posix_core.h _________________________________

posix_begin_c

typedef struct {
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
} posix_core_if;

extern posix_core_if posix_core;

posix_end_c

// ________________________________ posix_debug.h ________________________________

posix_begin_c

typedef struct {
    int32_t level;
    int32_t const quiet;
    int32_t const info;
    int32_t const verbose;
    int32_t const debug;
    int32_t const trace;
} posix_verbosity_if;

typedef struct {
    posix_verbosity_if verbosity;
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
} posix_debug_if;

#define posix_println(...) posix_debug.println(__FILE__, __LINE__, __func__, "" __VA_ARGS__)

extern posix_debug_if posix_debug;

posix_end_c

// ________________________________ posix_files.h ________________________________

posix_begin_c

typedef struct posix_file_name_s { char s[1024]; } posix_file_name_t;

enum { posix_files_max_path = (int32_t)sizeof(posix_file_name_t) };

typedef struct posix_file_s posix_file_t;

typedef struct posix_files_stat_s {
    uint64_t created;
    uint64_t accessed;
    uint64_t updated;
    int64_t  size;
    int64_t  type;
} posix_files_stat_t;

typedef struct posix_folder_s {
    uint8_t data[512];
} posix_folder_t;

typedef struct {
    posix_file_t* const invalid;
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
    int (*open)(posix_file_t* *file, const char* filename, int32_t flags);
    bool    (*is_valid)(posix_file_t* file);
    int (*seek)(posix_file_t* file, int64_t *position, int32_t method);
    int (*stat)(posix_file_t* file, posix_files_stat_t* stat, bool follow_symlink);
    int (*read)(posix_file_t* file, void* data, int64_t bytes, int64_t *transferred);
    int (*write)(posix_file_t* file, const void* data, int64_t bytes, int64_t *transferred);
    int (*flush)(posix_file_t* file);
    void    (*close)(posix_file_t* file);
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
    int (*opendir)(posix_folder_t* folder, const char* folder_name);
    const char* (*readdir)(posix_folder_t* folder, posix_files_stat_t* optional);
    void (*closedir)(posix_folder_t* folder);
    void (*test)(void);
} posix_files_if;

extern posix_files_if posix_files;

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

typedef struct {
    void (*test)(void);
} posix_generics_if;

extern posix_generics_if posix_generics;

posix_end_c

// ________________________________ posix_heap.h _________________________________

posix_begin_c

typedef struct posix_heap_s posix_heap_t;

typedef struct {
    int (*alloc)(void* *a, int64_t bytes);
    int (*alloc_zero)(void* *a, int64_t bytes);
    int (*realloc)(void* *a, int64_t bytes);
    int (*realloc_zero)(void* *a, int64_t bytes);
    void    (*free)(void* a);
    posix_heap_t* (*create)(bool serialized);
    int (*allocate)(posix_heap_t* heap, void* *a, int64_t bytes, bool zero);
    int (*reallocate)(posix_heap_t* heap, void* *a, int64_t bytes, bool zero);
    void    (*deallocate)(posix_heap_t* heap, void* a);
    int64_t (*bytes)(posix_heap_t* heap, void* a);
    void    (*dispose)(posix_heap_t* heap);
    void    (*test)(void);
} posix_heap_if;

extern posix_heap_if posix_heap;

posix_end_c

// _______________________________ posix_loader.h ________________________________

posix_begin_c

typedef struct {
    int32_t const local;
    int32_t const lazy;
    int32_t const now;
    int32_t const global;
    void* (*open)(const char* filename, int32_t mode);
    void* (*sym)(void* handle, const char* name);
    void  (*close)(void* handle);
    void (*test)(void);
} posix_loader_if;

extern posix_loader_if posix_loader;

posix_end_c

// _________________________________ posix_mem.h _________________________________

posix_begin_c

typedef struct {
    int (*map_ro)(const char* filename, void** data, int64_t* bytes);
    int (*map_rw)(const char* filename, void** data, int64_t* bytes);
    void (*unmap)(void* data, int64_t bytes);
    int  (*map_resource)(const char* label, void** data, int64_t* bytes);
    int32_t (*page_size)(void);
    int32_t (*large_page_size)(void);
    void* (*allocate)(int64_t bytes_multiple_of_page_size);
    void  (*deallocate)(void* a, int64_t bytes_multiple_of_page_size);
    void  (*test)(void);
} posix_mem_if;

extern posix_mem_if posix_mem;

posix_end_c

// _________________________________ posix_nls.h _________________________________

posix_begin_c

typedef struct {
    void (*init)(void);
    const char* (*locale)(void);
    int (*set_locale)(const char* locale);
    const char* (*str)(const char* defau1t);
    int32_t (*strid)(const char* s);
    const char* (*string)(int32_t strid, const char* defau1t);
} posix_nls_if;

extern posix_nls_if posix_nls;

posix_end_c

// _________________________________ posix_num.h _________________________________

posix_begin_c

typedef struct {
    uint64_t lo;
    uint64_t hi;
} posix_num128_t;

typedef struct {
    posix_num128_t (*add128)(const posix_num128_t a, const posix_num128_t b);
    posix_num128_t (*sub128)(const posix_num128_t a, const posix_num128_t b);
    posix_num128_t (*mul64x64)(uint64_t a, uint64_t b);
    uint64_t (*muldiv128)(uint64_t a, uint64_t b, uint64_t d);
    uint32_t (*gcd32)(uint32_t u, uint32_t v);
    uint32_t (*random32)(uint32_t *state);
    uint64_t (*random64)(uint64_t *state);
    uint32_t (*hash32)(const char* s, int64_t bytes);
    uint64_t (*hash64)(const char* s, int64_t bytes);
    void     (*test)(void);
} posix_num_if;

extern posix_num_if posix_num;

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

#define posix_static_init(n) __attribute__((constructor)) \
        static void _init_ ## n ## __LINE__ ## _ctor(void)

void posix_static_init_test(void);

posix_end_c

// _______________________________ posix_streams.h _______________________________

posix_begin_c

typedef struct posix_stream_if posix_stream_if;

typedef struct posix_stream_if {
    int (*read)(posix_stream_if* s, void* data, int64_t bytes,
                    int64_t *transferred);
    int (*write)(posix_stream_if* s, const void* data, int64_t bytes,
                     int64_t *transferred);
    void    (*close)(posix_stream_if* s);
} posix_stream_if;

typedef struct {
    posix_stream_if  stream;
    const void* data_read;
    int64_t     bytes_read;
    int64_t     pos_read;
    void* data_write;
    int64_t     bytes_write;
    int64_t     pos_write;
} posix_stream_memory_if;

typedef struct {
    void (*read_only)(posix_stream_memory_if* s,  const void* data, int64_t bytes);
    void (*write_only)(posix_stream_memory_if* s, void* data, int64_t bytes);
    void (*read_write)(posix_stream_memory_if* s, const void* read, int64_t read_bytes,
                                                  void* write, int64_t write_bytes);
    void (*test)(void);
} posix_streams_if;

extern posix_streams_if posix_streams;

posix_end_c

// ______________________________ posix_processes.h ______________________________

posix_begin_c

typedef struct {
    const char* command;
    posix_stream_if* in;
    posix_stream_if* out;
    posix_stream_if* err;
    uint32_t exit_code;
    fp64_t   timeout;
} posix_processes_child_t;

typedef struct {
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
    int   (*run)(posix_processes_child_t* child);
    int   (*popen)(const char* command, int32_t *xc, posix_stream_if* output,
                       fp64_t timeout_seconds);
    int  (*spawn)(const char* command);
    void (*test)(void);
} posix_processes_if;

extern posix_processes_if posix_processes;

posix_end_c

// _______________________________ posix_threads.h _______________________________

posix_begin_c

typedef struct posix_event_s* posix_event_t;

typedef struct {
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
} posix_event_if;

extern posix_event_if posix_event;

typedef struct posix_aligned_8 posix_mutex_s { uint8_t content[64]; } posix_mutex_t;

typedef struct {
    void (*init)(posix_mutex_t* m);
    void (*lock)(posix_mutex_t* m);
    void (*unlock)(posix_mutex_t* m);
    void (*dispose)(posix_mutex_t* m);
    void (*test)(void);
} posix_mutex_if;

extern posix_mutex_if posix_mutex;

typedef struct {
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
} posix_thread_if;

extern posix_thread_if posix_thread;

posix_end_c

// ________________________________ posix_vigil.h ________________________________

posix_begin_c

#define posix_static_assertion(condition) _Static_assert(condition, #condition)

typedef struct {
    int32_t (*failed_assertion)(const char* file, int32_t line,
        const char* func, const char* condition, const char* format, ...);
    int32_t (*fatal_termination)(const char* file, int32_t line,
        const char* func, const char* condition, const char* format, ...);
    int32_t (*fatal_if_error)(const char* file, int32_t line, const char* func,
        const char* condition, int r, const char* format, ...);
    void (*test)(void);
} posix_vigil_if;

extern posix_vigil_if posix_vigil;

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

posix_end_c

// ________________________________ posix_work.h _________________________________

posix_begin_c

typedef struct posix_work_s      posix_work_t;
typedef struct posix_work_queue_s posix_work_queue_t;

typedef struct posix_work_s {
    posix_work_queue_t* queue;
    fp64_t when;
    void (*work)(posix_work_t* c);
    void* data;
    posix_event_t  done;
    posix_work_t* next;
    bool   canceled;
} posix_work_t;

typedef struct posix_work_queue_s {
    posix_work_t* head;
    int64_t    lock;
    posix_event_t changed;
} posix_work_queue_t;

typedef struct posix_work_queue_if {
    void (*post)(posix_work_t* c);
    bool (*get)(posix_work_queue_t*, posix_work_t* *c);
    void (*call)(posix_work_t* c);
    void (*dispatch)(posix_work_queue_t* q);
    void (*cancel)(posix_work_t* c);
    void (*flush)(posix_work_queue_t* q);
} posix_work_queue_if;

extern posix_work_queue_if  posix_work_queue;

typedef struct posix_worker_s {
    posix_work_queue_t queue;
    posix_thread_t     thread;
    posix_event_t      wake;
    volatile bool  quit;
} posix_worker_t;

typedef struct posix_worker_if {
    void    (*start)(posix_worker_t* tq);
    void    (*post)(posix_worker_t* tq, posix_work_t* w);
    int (*join)(posix_worker_t* tq, fp64_t timeout);
    void    (*test)(void);
} posix_worker_if;

extern posix_worker_if posix_worker;

posix_end_c

#endif // POSIX_H
