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

#define posix_static_init(n) __attribute__((constructor)) \
        static void _init_ ## n ## __LINE__ ## _ctor(void)

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
