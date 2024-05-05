#ifndef ut_definition
#define ut_definition

// _________________________________ ut_std.h _________________________________

#include <ctype.h>
#include <errno.h>
#include <malloc.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Type aliases for floating-point types similar to <stdint.h>
typedef float  fp32_t;
typedef double fp64_t;
// "long fp64_t" is required by C standard but the bitness
// of it is not specified.

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

// In callbacks the formal parameters are
// frequently unused. Also sometimes parameters
// are used in debug configuration only (e.g. assert() checks)
// but not in release.
// C does not have anonymous parameters like C++
// Instead of:
//      void foo(param_type_t param) { (void)param; / *unused */ }
// use:
//      vod foo(param_type_t unused(param)) { }

#define unused(name) _Pragma("warning(suppress:  4100)") name

// Because MS C compiler is unhappy about alloca() and
// does not implement (C99 optional) dynamic arrays on the stack:

#define stackalloc(n) (_Pragma("warning(suppress: 6255 6263)") alloca(n))


// ________________________________ ut_args.h _________________________________

typedef struct {
    // On Unix it is responsibility of the main() to assign these values
    int32_t c;      // argc
    const char** v; // argv[argc]
    const char** env; // ut_args.env[] is null-terminated
    void    (*main)(int32_t argc, const char* argv[], const char** env);
    void    (*WinMain)(void); // windows specific
    int32_t (*option_index)(const char* option); // e.g. option: "--verbosity" or "-v"
    void    (*remove_at)(int32_t ix);
    /* argc=3 argv={"foo", "--verbose"} -> returns true; argc=1 argv={"foo"} */
    bool    (*option_bool)(const char* option);
    /* argc=3 argv={"foo", "--n", "153"} -> value==153, true; argc=1 argv={"foo"}
       also handles negative values (e.g. "-153") and hex (e.g. 0xBADF00D)
    */
    bool    (*option_int)(const char* option, int64_t *value);
    // for argc=3 argv={"foo", "--path", "bar"}
    //     option_str("--path", option)
    // returns option: "bar" and argc=1 argv={"foo"} */
    const char* (*option_str)(const char* option);
    // basename() for argc=3 argv={"/bin/foo.exe", ...} returns "foo":
    const char* (*basename)(void);
    void (*fini)(void);
    void (*test)(void);
} ut_args_if;

extern ut_args_if ut_args;

/* Usage:

    (both main() and WinMain() could be compiled at the same time on Windows):

    static int run(void);

    int main(int argc, char* argv[], char* envp[]) { // link.exe /SUBSYSTEM:CONSOLE
        ut_args.main(argc, argv, envp); // Initialize args with command-line parameters
        int r = run();
        ut_args.fini();
        return r;
    }

    #include "ut/ut_win32.h"

    int APIENTRY WinMain(HINSTANCE inst, HINSTANCE prev, char* command, int show) {
        // link.exe /SUBSYSTEM:WINDOWS
        ut_args.WinMain();
        int r = run();
        ut_args.fini();
        return 0;
    }

    static int run(void) {
        if (ut_args.option_bool("-v")) {
            ut_debug.verbosity.level = ut_debug.verbosity.verbose;
        }
        int64_t num = 0;
        if (ut_args.option_int("--number", &num)) {
            printf("--number: %ld\n", num);
        }
        const char* path = ut_args.option_str("--path");
        if (path != null) {
            printf("--path: %s\n", path);
        }
        printf("ut_args.basename(): %s\n", ut_args.basename());
        printf("ut_args.v[0]: %s\n", ut_args.v[0]);
        for (int i = 1; i < ut_args.c; i++) {
            printf("ut_args.v[%d]: %s\n", i, ut_args.v[i]);
        }
        return 0;
    }

    // Also see: https://github.com/leok7v/ut/blob/main/test/test1.c

*/


// _______________________________ ut_atomics.h _______________________________

// Will be deprecated soon after Microsoft fully supports <stdatomic.h>


typedef struct {
    void* (*exchange_ptr)(volatile void** a, void* v); // retuns previous value
    int32_t (*increment_int32)(volatile int32_t* a); // returns incremented
    int32_t (*decrement_int32)(volatile int32_t* a); // returns decremented
    int64_t (*increment_int64)(volatile int64_t* a); // returns incremented
    int64_t (*decrement_int64)(volatile int64_t* a); // returns decremented
    int32_t (*add_int32)(volatile int32_t* a, int32_t v); // returns result of add
    int64_t (*add_int64)(volatile int64_t* a, int64_t v); // returns result of add
    // returns the value held previously by "a" address:
    int32_t (*exchange_int32)(volatile int32_t* a, int32_t v);
    int64_t (*exchange_int64)(volatile int64_t* a, int64_t v);
    // compare_exchange functions compare the *a value with the comparand value.
    // If the *a is equal to the comparand value, the "v" value is stored in the address
    // specified by "a" otherwise, no operation is performed.
    // returns true if previous value *a was the same as "comparand"
    // false if *a was different from "comparand" and "a" was NOT modified.
    bool (*compare_exchange_int64)(volatile int64_t* a, int64_t comparand, int64_t v);
    bool (*compare_exchange_int32)(volatile int32_t* a, int32_t comparand, int32_t v);
    bool (*compare_exchange_ptr)(volatile void** a, void* comparand, void* v);
    void (*spinlock_acquire)(volatile int64_t* spinlock);
    void (*spinlock_release)(volatile int64_t* spinlock);
    int32_t (*load32)(volatile int32_t* a);
    int64_t (*load64)(volatile int64_t* a);
    void (*memory_fence)(void);
    void (*test)(void);
} ut_atomics_if;

extern ut_atomics_if ut_atomics;



// ______________________________ ut_clipboard.h ______________________________

typedef struct ui_image_s ui_image_t;

typedef struct {
    errno_t (*put_text)(const char* s);
    errno_t (*get_text)(char* text, int32_t* bytes);
    errno_t (*put_image)(ui_image_t* image); // only for Windows apps
    void (*test)(void);
} ut_clipboard_if;

extern ut_clipboard_if ut_clipboard;



// ________________________________ ut_clock.h ________________________________

typedef struct {
    int32_t const nsec_in_usec; // nano in micro second
    int32_t const nsec_in_msec; // nano in milli
    int32_t const nsec_in_sec;
    int32_t const usec_in_msec; // micro in milli
    int32_t const msec_in_sec;  // milli in sec
    int32_t const usec_in_sec;  // micro in sec
    fp64_t   (*seconds)(void);      // since boot
    uint64_t (*nanoseconds)(void);  // since boot overflows in about 584.5 years
    uint64_t (*unix_microseconds)(void); // since January 1, 1970
    uint64_t (*unix_seconds)(void);      // since January 1, 1970
    uint64_t (*microseconds)(void); // NOT monotonic(!) UTC since epoch January 1, 1601
    uint64_t (*localtime)(void);    // local time microseconds since epoch
    void (*utc)(uint64_t microseconds, int32_t* year, int32_t* month,
        int32_t* day, int32_t* hh, int32_t* mm, int32_t* ss, int32_t* ms,
        int32_t* mc);
    void (*local)(uint64_t microseconds, int32_t* year, int32_t* month,
        int32_t* day, int32_t* hh, int32_t* mm, int32_t* ss, int32_t* ms,
        int32_t* mc);
    void (*test)(void);
} ut_clock_if;

extern ut_clock_if ut_clock;




// _______________________________ ut_config.h ________________________________

// Persistent storage for configuration and other small data
// related to specific application.
// on Unix-like system ~/.name/key files are used.
// On Window User registry (could be .dot files/folders).
// "name" is customary basename of "ut_args.v[0]"

typedef struct {
    errno_t (*save)(const char* name, const char* key,
                    const void* data, int32_t bytes);
    int32_t (*size)(const char* name, const char* key);
    // load() returns number of actual loaded bytes:
    int32_t (*load)(const char* name, const char* key,
                    void* data, int32_t bytes);
    errno_t (*remove)(const char* name, const char* key);
    errno_t (*clean)(const char* name); // remove all subkeys
    void (*test)(void);
} ut_config_if;

extern ut_config_if ut_config;




// ________________________________ ut_debug.h ________________________________

// debug interface essentially is:
// vfprintf(stderr, format, vl)
// fprintf(stderr, format, ...)
// with the additional convience:
// 1. writing to system log (e.g. OutputDebugString() on Windows)
// 2. always appending \n at the end of the line and thus flushing buffer
// Warning: on Windows it is not real-time and subject to 30ms delays
//          that may or may not happen on some calls

typedef struct {
    int32_t level; // global verbosity (interpretation may vary)
    int32_t const quiet;    // 0
    int32_t const info;     // 1 basic information (errors and warnings)
    int32_t const verbose;  // 2 detailed diagnostic
    int32_t const debug;    // 3 including debug messages
    int32_t const trace;    // 4 everything (may include nested calls)
} ut_verbosity_if;

typedef struct {
    ut_verbosity_if verbosity;
    int32_t (*verbosity_from_string)(const char* s);
    void (*println_va)(const char* file, int32_t line, const char* func,
        const char* format, va_list vl);
    void (*println)(const char* file, int32_t line, const char* func,
        const char* format, ...);
    void (*perrno)(const char* file, int32_t line,
        const char* func, int32_t err_no, const char* format, ...);
    void (*perror)(const char* file, int32_t line,
        const char* func, int32_t error, const char* format, ...);
    bool (*is_debugger_present)(void);
    void (*breakpoint)(void);
    void (*test)(void);
} ut_debug_if;

#define traceln(...) ut_debug.println(__FILE__, __LINE__, __func__, "" __VA_ARGS__)

extern ut_debug_if ut_debug;



// ________________________________ ut_files.h ________________________________

enum { ut_files_max_path = 4 * 1024 }; // *)

typedef struct ut_file_s ut_file_t;

typedef struct ut_files_stat_s {
    uint64_t created;
    uint64_t accessed;
    uint64_t updated;
    int64_t  size; // bytes
    int64_t  type; // device / folder / symlink
} ut_files_stat_t;

typedef struct ut_folder_s {
    uint8_t data[512]; // implementation specific
} ut_folder_t;

typedef struct {
    ut_file_t* const invalid; // (ut_file_t*)-1
    // ut_files_stat_t.type:
    int32_t const type_folder;
    int32_t const type_symlink;
    int32_t const type_device;
    // seek() methods:
    int32_t const seek_set;
    int32_t const seek_cur;
    int32_t const seek_end;
    // open() flags
    int32_t const o_rd; // read only
    int32_t const o_wr; // write only
    int32_t const o_rw; // != (o_rd | o_wr)
    int32_t const o_append;
    int32_t const o_create; // opens existing or creates new
    int32_t const o_excl;   // create fails if file exists
    int32_t const o_trunc;  // open always truncates to empty
    int32_t const o_sync;
    errno_t (*open)(ut_file_t* *file, const char* filename, int32_t flags);
    bool    (*is_valid)(ut_file_t* file); // checks both null and invalid
    errno_t (*seek)(ut_file_t* file, int64_t *position, int32_t method);
    errno_t (*stat)(ut_file_t* file, ut_files_stat_t* stat, bool follow_symlink);
    errno_t (*read)(ut_file_t* file, void* data, int64_t bytes, int64_t *transferred);
    errno_t (*write)(ut_file_t* file, const void* data, int64_t bytes, int64_t *transferred);
    errno_t (*flush)(ut_file_t* file);
    void    (*close)(ut_file_t* file);
    errno_t (*write_fully)(const char* filename, const void* data,
                           int64_t bytes, int64_t *transferred);
    bool (*exists)(const char* pathname); // does not guarantee any access writes
    bool (*is_folder)(const char* pathname);
    bool (*is_symlink)(const char* pathname);
    const char* (*basename)(const char* pathname); // c:\foo\bar.ext -> bar.ext
    errno_t (*mkdirs)(const char* pathname); // tries to deep create all folders in pathname
    errno_t (*rmdirs)(const char* pathname); // tries to remove folder and its subtree
    errno_t (*create_tmp)(char* file, int32_t count); // create temporary file
    errno_t (*chmod777)(const char* pathname); // and whole subtree new files and folders
    errno_t (*symlink)(const char* from, const char* to); // sym link "ln -s" **)
    errno_t (*link)(const char* from, const char* to); // hard link like "ln"
    errno_t (*unlink)(const char* pathname); // delete file or empty folder
    errno_t (*copy)(const char* from, const char* to); // allows overwriting
    errno_t (*move)(const char* from, const char* to); // allows overwriting
    errno_t (*cwd)(char* folder, int32_t count); // get current working dir
    errno_t (*chdir)(const char* folder); // set working directory
    const char* (*bin)(void);  // Windows: "c:\Program Files" Un*x: "/bin"
    const char* (*data)(void); // Windows: "c:\ProgramData" Un*x: /data or /var
    const char* (*tmp)(void);  // temporary folder (system or user)
    // There are better, native, higher performance ways to iterate thru
    // folders in Posix, Linux and Windows. The following is minimalistic
    // approach to folder content reading:
    errno_t (*opendir)(ut_folder_t* folder, const char* folder_name);
    const char* (*readdir)(ut_folder_t* folder, ut_files_stat_t* optional);
    void (*closedir)(ut_folder_t* folder);
    void (*test)(void);
} ut_files_if;

// *) ut_files_max_path is a compromise - way longer than Windows MAX_PATH of 260
// and somewhat shorter than 32 * 1024 Windows long path.
// Use with caution understanding that it is a limitation and where it is
// important heap may and should be used. Do not to rely on thread stack size
// (default: 1MB on Windows, Android Linux 64KB, 512 KB  on MacOS, Ubuntu 8MB)
//
// **) symlink on Win32 is only allowed in Admin elevated
//     processes and in Developer mode.

extern ut_files_if ut_files;



// ______________________________ ut_generics.h _______________________________

// Most of ut/ui code is written the way of min(a,b) max(a,b)
// not having side effects on the arguments and thus evaluating
// them twice ain't a big deal. However, out of curiosity of
// usefulness of Generic() in C11 standard here is type-safe
// single evaluation of the arguments version of what could
// have been simple minimum and maximum macro definitions.
// Type safety comes with the cost of complexity in puritan
// or stoic language like C:

static inline int8_t   ut_max_int8(int8_t x, int8_t y)       { return x > y ? x : y; }
static inline int16_t  ut_max_int16(int16_t x, int16_t y)    { return x > y ? x : y; }
static inline int32_t  ut_max_int32(int32_t x, int32_t y)    { return x > y ? x : y; }
static inline int64_t  ut_max_int64(int64_t x, int64_t y)    { return x > y ? x : y; }
static inline uint8_t  ut_max_uint8(uint8_t x, uint8_t y)    { return x > y ? x : y; }
static inline uint16_t ut_max_uint16(uint16_t x, uint16_t y) { return x > y ? x : y; }
static inline uint32_t ut_max_uint32(uint32_t x, uint32_t y) { return x > y ? x : y; }
static inline uint64_t ut_max_uint64(uint64_t x, uint64_t y) { return x > y ? x : y; }
static inline fp32_t   ut_max_fp32(fp32_t x, fp32_t y)       { return x > y ? x : y; }
static inline fp64_t   ut_max_fp64(fp64_t x, fp64_t y)       { return x > y ? x : y; }

// MS cl.exe version 19.39.33523 has issues with "long":
// does not pick up int32_t/uint32_t types for "long" and "unsigned long"
// need to handle long / unsigned long separately:

static inline long          ut_max_long(long x, long y)                    { return x > y ? x : y; }
static inline unsigned long ut_max_ulong(unsigned long x, unsigned long y) { return x > y ? x : y; }

static inline int8_t   ut_min_int8(int8_t x, int8_t y)       { return x < y ? x : y; }
static inline int16_t  ut_min_int16(int16_t x, int16_t y)    { return x < y ? x : y; }
static inline int32_t  ut_min_int32(int32_t x, int32_t y)    { return x < y ? x : y; }
static inline int64_t  ut_min_int64(int64_t x, int64_t y)    { return x < y ? x : y; }
static inline uint8_t  ut_min_uint8(uint8_t x, uint8_t y)    { return x < y ? x : y; }
static inline uint16_t ut_min_uint16(uint16_t x, uint16_t y) { return x < y ? x : y; }
static inline uint32_t ut_min_uint32(uint32_t x, uint32_t y) { return x < y ? x : y; }
static inline uint64_t ut_min_uint64(uint64_t x, uint64_t y) { return x < y ? x : y; }
static inline fp32_t   ut_min_fp32(fp32_t x, fp32_t y)       { return x < y ? x : y; }
static inline fp64_t   ut_min_fp64(fp64_t x, fp64_t y)       { return x < y ? x : y; }

static inline long          ut_min_long(long x, long y)                    { return x < y ? x : y; }
static inline unsigned long ut_min_ulong(unsigned long x, unsigned long y) { return x < y ? x : y; }


static inline void ut_min_undefined(void) { }
static inline void ut_max_undefined(void) { }

#define ut_max(X, Y) _Generic((X) + (Y), \
    int8_t:   ut_max_int8,   \
    int16_t:  ut_max_int16,  \
    int32_t:  ut_max_int32,  \
    int64_t:  ut_max_int64,  \
    uint8_t:  ut_max_uint8,  \
    uint16_t: ut_max_uint16, \
    uint32_t: ut_max_uint32, \
    uint64_t: ut_max_uint64, \
    fp32_t:   ut_max_fp32,   \
    fp64_t:   ut_max_fp64,   \
    long:     ut_max_long,   \
    unsigned long: ut_max_ulong, \
    default:  ut_max_undefined)(X, Y)

#define ut_min(X, Y) _Generic((X) + (Y), \
    int8_t:   ut_min_int8,   \
    int16_t:  ut_min_int16,  \
    int32_t:  ut_min_int32,  \
    int64_t:  ut_min_int64,  \
    uint8_t:  ut_min_uint8,  \
    uint16_t: ut_min_uint16, \
    uint32_t: ut_min_uint32, \
    uint64_t: ut_min_uint64, \
    fp32_t:   ut_min_fp32,   \
    fp64_t:   ut_min_fp64,   \
    long:     ut_min_long,   \
    unsigned long: ut_min_ulong, \
    default:  ut_min_undefined)(X, Y)


// The expression (X) + (Y) is used in _Generic primarily for type promotion
// and compatibility between different types of the two operands. In C, when
// you perform an arithmetic operation like addition between two variables,
// the types of these variables undergo implicit conversions to a common type
// according to the usual arithmetic conversions defined in the C standard.
// This helps ensure that:
//
// Type Compatibility: The macro works correctly even if X and Y are of
// different types. By using (X) + (Y), both X and Y are promoted to a common
// type, which ensures that the macro selects the appropriate function
// that can handle this common type.
//
// Type Safety: It ensures that the selected function can handle the type
// resulting from the operation, thereby preventing type mismatches that
// could lead to undefined behavior or compilation errors.

typedef struct {
    void (*test)(void);
} ut_generics_if;

extern ut_generics_if ut_generics;



// ________________________________ ut_heap.h _________________________________

// It is absolutely OK to use posix compliant
// malloc()/calloc()/realloc()/free() function calls with understanding
// that they introduce serialization points in multi-threaded applications
// and may be induce wait states that under pressure (all cores busy) may
// result in prolonged wait states which may not be acceptable for real time
// processing.
//
// heap_if.functions may or may not be faster than malloc()/free() ...
//
// Some callers may find realloc parameters more convenient to avoid
// anti-pattern
//      void* reallocated = realloc(p, new_size);
//      if (reallocated != null) { p = reallocated; }
// and avoid never ending discussion of legality and implementation
// compliance of the situation:
//      realloc(p /* when p == null */, ...)
//
// zero: true initializes allocated or reallocated tail memory to 0x00

typedef struct ut_heap_s ut_heap_t;

typedef struct { // heap == null uses process serialized LFH
    ut_heap_t* (*create)(bool serialized);
    errno_t (*allocate)(ut_heap_t* heap, void* *a, int64_t bytes, bool zero);
    // reallocate may return ERROR_OUTOFMEMORY w/o changing 'a' *)
    errno_t (*reallocate)(ut_heap_t* heap, void* *a, int64_t bytes, bool zero);
    void    (*deallocate)(ut_heap_t* heap, void* a);
    int64_t (*bytes)(ut_heap_t* heap, void* a); // actual allocated size
    void    (*dispose)(ut_heap_t* heap);
    void    (*test)(void);
} ut_heap_if;

extern ut_heap_if ut_heap;

// *) zero in reallocate applies to the newly appended bytes

// On Windows ut_mem.heap is based on serialized LFH returned by GetProcessHeap()
// https://learn.microsoft.com/en-us/windows/win32/memory/low-fragmentation-heap
// threads can benefit from not serialized, not LFH if they allocate and free
// memory in time critical loops.




// _______________________________ ut_loader.h ________________________________

// see:
// https://pubs.opengroup.org/onlinepubs/7908799/xsh/dlfcn.h.html

typedef struct {
    // mode:
    int32_t const local;
    int32_t const lazy;
    int32_t const now;
    int32_t const global;
    // "If the value of file is null, dlopen() provides a handle on a global
    //  symbol object." posix
    void* (*open)(const char* filename, int32_t mode);
    void* (*sym)(void* handle, const char* name);
    void  (*close)(void* handle);
    void (*test)(void);
} ut_loader_if;

extern ut_loader_if ut_loader;



// _________________________________ ut_mem.h _________________________________

typedef struct {
    // whole file read only
    errno_t (*map_ro)(const char* filename, void** data, int64_t* bytes);
    // whole file read-write
    errno_t (*map_rw)(const char* filename, void** data, int64_t* bytes);
    void (*unmap)(void* data, int64_t bytes);
    // map_resource() maps data from resources, do NOT unmap!
    errno_t  (*map_resource)(const char* label, void** data, int64_t* bytes);
    int32_t (*page_size)(void); // 4KB or 64KB on Windows
    int32_t (*large_page_size)(void);  // 2MB on Windows
    // allocate() contiguous reserved virtual address range,
    // if possible committed to physical memory.
    // Memory guaranteed to be aligned to page boundary.
    // Memory is guaranteed to be initialized to zero on access.
    void* (*allocate)(int64_t bytes_multiple_of_page_size);
    void  (*deallocate)(void* a, int64_t bytes_multiple_of_page_size);
    void  (*test)(void);
} ut_mem_if;

extern ut_mem_if ut_mem;




// _________________________________ ut_num.h _________________________________

typedef struct {
    uint64_t lo;
    uint64_t hi;
} ut_num128_t; // uint128_t may be supported by compiler

typedef struct {
    ut_num128_t (*add128)(const ut_num128_t a, const ut_num128_t b);
    ut_num128_t (*sub128)(const ut_num128_t a, const ut_num128_t b);
    ut_num128_t (*mul64x64)(uint64_t a, uint64_t b);
    uint64_t (*muldiv128)(uint64_t a, uint64_t b, uint64_t d);
    uint32_t (*gcd32)(uint32_t u, uint32_t v); // greatest common denominator
    // non-crypto strong pseudo-random number generators (thread safe)
    uint32_t (*random32)(uint32_t *state); // "Mulberry32"
    uint64_t (*random64)(uint64_t *state); // "Trust"
    // "FNV-1a" hash functions (if bytes == 0 expects zero terminated string)
    uint32_t (*hash32)(const char* s, int64_t bytes);
    uint64_t (*hash64)(const char* s, int64_t bytes);
    void     (*test)(void);
} ut_num_if;

extern ut_num_if ut_num;




// _______________________________ ut_static.h ________________________________

// ut_static_init(unique_name) { code_to_execute_before_main }

#if defined(_MSC_VER)

#if defined(_WIN64) || defined(_M_X64)
#define _msvc_symbol_prefix_ ""
#else
#define _msvc_symbol_prefix_ "_"
#endif

#pragma comment(linker, "/include:_static_force_symbol_reference_")

void* _static_force_symbol_reference_(void* symbol);

#define _msvc_ctor_(_sym_prefix, func)                                    \
  void func(void);                                                        \
  int32_t (* _array ## func)(void);                                       \
  int32_t func ## _wrapper(void);                                         \
  int32_t func ## _wrapper(void) { func();                                \
    _static_force_symbol_reference_((void*)_array ## func);               \
    _static_force_symbol_reference_((void*)func ## _wrapper); return 0; } \
  __pragma(comment(linker, "/include:" _sym_prefix # func "_wrapper"))    \
  __pragma(section(".CRT$XCU", read))                                     \
  __declspec(allocate(".CRT$XCU"))                                        \
    int32_t (* _array ## func)(void) = func ## _wrapper;

#define ut_static_init2_(func, line) _msvc_ctor_(_msvc_symbol_prefix_, \
    func ## _constructor_##line)                                     \
    void func ## _constructor_##line(void)

#define ut_static_init1_(func, line) ut_static_init2_(func, line)

#define ut_static_init(func) ut_static_init1_(func, __LINE__)

#else
#define ut_static_init(n) __attribute__((constructor)) \
        static void _init_ ## n ## __LINE__ ## _ctor(void)
#endif

void ut_static_init_test(void);



// _________________________________ ut_str.h _________________________________

// Since a lot of str*() operations are preprocessor defines
// care should be exercised that arguments of macro invocations
// do not have side effects or not computationally expensive.
// None of the definitions are performance champions - if the
// code needs extreme cpu cycle savings working with utf8 strings.
//
// str* macros and functions assume zero terminated UTF-8 strings
// in C tradition. Use ut_str.* functions for non-zero terminated
// strings.

#define strempty(s) ((void*)(s) == null || (s)[0] == 0)

#define strconcat(a, b) _Pragma("warning(suppress: 6386)") \
    (strcat(strcpy((char*)stackalloc(strlen(a) + strlen(b) + 1), (a)), (b)))

#define strequ(s1, s2)  (((void*)(s1) == (void*)(s2)) || \
    (((void*)(s1) != null && (void*)(s2) != null) && strcmp((s1), (s2)) == 0))

#define striequ(s1, s2)  (((void*)(s1) == (void*)(s2)) || \
    (((void*)(s1) != null && (void*)(s2) != null) && stricmp((s1), (s2)) == 0))

#define strstartswith(a, b) \
    (strlen(a) >= strlen(b) && memcmp((a), (b), strlen(b)) == 0)

#define strendswith(s1, s2) \
    (strlen(s1) >= strlen(s2) && strcmp((s1) + strlen(s1) - strlen(s2), (s2)) == 0)

#define strlength(s) ((int32_t)strlen(s)) // avoid code analysis noise
// a lot of posix like API consumes "int" instead of size_t which
// is acceptable for majority of char* zero terminated strings usage
// since most of them work with filepath that are relatively short
// and on Windows are limited to 260 chars or 32KB - 1 chars.

#define strcopy(s1, s2) /* use with extreme caution */                      \
    do {                                                                    \
        static_assertion(countof(s1) > sizeof(void*));                      \
        strncpy((s1), (s2), countof((s1)) - 1); s1[countof((s1)) - 1] = 0;  \
} while (0)

char* strnchr(const char* s, int32_t n, char ch);

#define strtolowercase(s) \
    ut_str.to_lowercase((char*)stackalloc(strlen(s) + 1), strlen(s) + 1, s)

#define utf16to8(utf16) ut_str.utf16_utf8((char*) \
    stackalloc((size_t)ut_str.utf8_bytes(utf16) + 1), utf16)

#define utf8to16(s) ut_str.utf8_utf16((uint16_t*) \
    stackalloc((ut_str.utf16_chars(s) + 1) * sizeof(uint16_t)), s)

#define strprintf(s, ...) ut_str.format((s), countof(s), "" __VA_ARGS__)

#define strncasecmp _strnicmp
#define strcasecmp _stricmp

// ut_str.functions are capable of working with both 0x00 terminated
// strings and UTF-8 strings of fixed length denoted by n1 and n2.
// case insensitive functions with postfix _nc
// operate only on ASCII characters. No ANSI no UTF-8

typedef struct {
    const char* (*error)(int32_t error);     // en-US
    const char* (*error_nls)(int32_t error); // national locale string
    int32_t (*utf8_bytes)(const uint16_t* utf16);
    int32_t (*utf16_chars)(const char* s);
    char* (*utf16_utf8)(char* destination, const uint16_t* utf16);
    uint16_t* (*utf8_utf16)(uint16_t* destination, const char* utf8);
    // string formatting printf style:
    void (*format_va)(char* utf8, int32_t count, const char* format, va_list vl);
    void (*format)(char* utf8, int32_t count, const char* format, ...);
    bool (*is_empty)(const char* s); // null or empty string
    bool (*equal)(const char* s1, int32_t n1, const char* s2, int32_t n2);
    bool (*equal_nc)(const char* s1, int32_t n1, const char* s2, int32_t n2);
    int32_t (*length)(const char* s);
    // copy(s1, countof(s1), s2, /*bytes*/-1) means zero terminated
    bool  (*copy)(char* d, int32_t capacity,
                 const char* s, int32_t bytes); // false on overflow
    char* (*first_char)(const char* s, int32_t bytes, char ch);
    char* (*last_char)(const char* s, int32_t n, char ch); // strrchr
    char* (*first)(const char* s1, int32_t n1, const char* s2, int32_t n2);
    bool  (*to_lower)(char* d, int32_t capacity, const char* s, int32_t n);
    bool  (*to_upper)(char* d, int32_t capacity, const char* s, int32_t n);
    int32_t (*compare)(const char* s1, int32_t n1, const char* s2, int32_t n2);
    int32_t (*compare_nc)(const char* s1, int32_t n1,
                        const char* s2, int32_t n2);
    bool (*starts_with)(const char* s1, int32_t n1, const char* s2, int32_t n2);
    bool (*ends_with)(const char* s1, int32_t n1, const char* s2, int32_t n2);
    bool (*ends_with_nc)(const char* s1, int32_t n1, const char* s2, int32_t n2);
    bool (*starts_with_nc)(const char* s1, int32_t n1, const char* s2, int32_t n2);
    // removes quotes from a head and tail of the string `s` if present
    const char* (*unquote)(char* *s, int32_t n); // modifies `s` in place
    void (*test)(void);
} ut_str_if;

extern ut_str_if ut_str;




// _______________________________ ut_streams.h _______________________________

typedef struct ut_stream_if ut_stream_if;

typedef struct ut_stream_if {
    errno_t (*read)(ut_stream_if* s, void* data, int64_t bytes,
                    int64_t *transferred);
    errno_t (*write)(ut_stream_if* s, const void* data, int64_t bytes,
                     int64_t *transferred);
    void    (*close)(ut_stream_if* s); // optional
} ut_stream_if;

typedef struct {
    ut_stream_if   stream;
    const void* data_read;
    int64_t     bytes_read;
    int64_t     pos_read;
    void*       data_write;
    int64_t     bytes_write;
    int64_t     pos_write;
} ut_stream_memory_if;

typedef struct {
    void (*read_only)(ut_stream_memory_if* s,  const void* data, int64_t bytes);
    void (*write_only)(ut_stream_memory_if* s, void* data, int64_t bytes);
    void (*read_write)(ut_stream_memory_if* s, const void* read, int64_t read_bytes,
                                               void* write, int64_t write_bytes);
    void (*test)(void);
} ut_streams_if;

extern ut_streams_if ut_streams;



// ______________________________ ut_processes.h ______________________________

typedef struct {
    const char* command;
    ut_stream_if* in;
    ut_stream_if* out;
    ut_stream_if* err;
    uint32_t exit_code;
    fp64_t   timeout; // seconds
} ut_processes_child_t;

// Process name may be an the executable filename with
// full, partial or absent pathname.
// Case insensitive on Windows.

typedef struct {
    const char* (*name)(void); // argv[0] like but full path
    uint64_t  (*pid)(const char* name); // 0 if process not found
    errno_t   (*pids)(const char* name, uint64_t* pids/*[size]*/, int32_t size,
                      int32_t *count); // return 0, error or ERROR_MORE_DATA
    errno_t   (*nameof)(uint64_t pid, char* name, int32_t count); // pathname
    bool      (*present)(uint64_t pid);
    errno_t   (*kill)(uint64_t pid, fp64_t timeout_seconds);
    errno_t   (*kill_all)(const char* name, fp64_t timeout_seconds);
    bool      (*is_elevated)(void); // Is process running as root/ Admin / System?
    errno_t   (*restart_elevated)(void); // retuns error or exits on success
    errno_t   (*run)(ut_processes_child_t* child);
    errno_t   (*popen)(const char* command, int32_t *xc, ut_stream_if* output,
                       fp64_t timeout_seconds); // <= 0 infinite
    // popen() does NOT guarantee stream zero termination on errors
    errno_t  (*spawn)(const char* command); // spawn fully detached process
    void (*test)(void);
} ut_processes_if;

extern ut_processes_if ut_processes;



// _______________________________ ut_runtime.h _______________________________

typedef struct {
    int32_t (*err)(void); // errno or GetLastError()
    void (*seterr)(int32_t err); // errno = err or SetLastError()
    void (*abort)(void);
    void (*exit)(int32_t exit_code); // only 8 bits on posix
    void (*test)(void);
} ut_runtime_if;

extern ut_runtime_if ut_runtime;




// _______________________________ ut_threads.h _______________________________

typedef struct event_s * event_t;

typedef struct {
    event_t (*create)(void); // never returns null
    event_t (*create_manual)(void); // never returns null
    void (*set)(event_t e);
    void (*reset)(event_t e);
    void (*wait)(event_t e);
    // returns 0 or -1 on timeout
    int32_t (*wait_or_timeout)(event_t e, fp64_t seconds); // seconds < 0 forever
    // returns event index or -1 on timeout or abandon
    int32_t (*wait_any)(int32_t n, event_t events[]); // -1 on abandon
    int32_t (*wait_any_or_timeout)(int32_t n, event_t e[], fp64_t seconds);
    void (*dispose)(event_t e);
    void (*test)(void);
} ut_event_if;

extern ut_event_if ut_event;

typedef struct { uint8_t content[40]; } mutex_t;

typedef struct {
    void (*init)(mutex_t* m);
    void (*lock)(mutex_t* m);
    void (*unlock)(mutex_t* m);
    void (*dispose)(mutex_t* m);
    void (*test)(void);
} ut_mutex_if;

extern ut_mutex_if ut_mutex;

typedef struct thread_s * thread_t;

typedef struct {
    thread_t (*start)(void (*func)(void*), void* p); // never returns null
    errno_t (*join)(thread_t thread, fp64_t timeout_seconds); // < 0 forever
    void (*detach)(thread_t thread); // closes handle. thread is not joinable
    void (*name)(const char* name); // names the thread
    void (*realtime)(void); // bumps calling thread priority
    void (*yield)(void);    // pthread_yield() / Win32: SwitchToThread()
    void (*sleep_for)(fp64_t seconds);
    int32_t (*id)(void);    // gettid()
    void (*test)(void);
} ut_thread_if;

extern ut_thread_if ut_thread;

// ________________________________ ut_vigil.h ________________________________

#include <assert.h> // unsures that it will not be included again
#undef assert       // because better assert(b, ...) will be defined here


// better assert() - augmented with printf format and parameters
// swear() - release configuration assert() in honor of:
// https://github.com/munificent/vigil


#define static_assertion(condition) static_assert(condition, #condition)


typedef struct {
    int32_t (*failed_assertion)(const char* file, int32_t line,
        const char* func, const char* condition, const char* format, ...);
    int32_t (*fatal_termination)(const char* file, int32_t line,
        const char* func, const char* condition, const char* format, ...);
    void (*test)(void);
} vigil_if;

extern vigil_if vigil;

#ifdef _MSC_VER
    #define __suppress_constant_cond_exp__ _Pragma("warning(suppress: 4127)")
#else
    #define __suppress_constant_cond_exp__
#endif

#if defined(DEBUG)
  #define assert(b, ...) __suppress_constant_cond_exp__          \
    /* const cond */                                             \
    (void)((!!(b)) || vigil.failed_assertion(__FILE__, __LINE__, \
    __func__, #b, "" __VA_ARGS__))
#else
  #define assert(b, ...) ((void)0)
#endif

// swear() is both debug and release configuration assert

#define swear(b, ...) __suppress_constant_cond_exp__             \
    /* const cond */                                             \
    (void)((!!(b)) || vigil.failed_assertion(__FILE__, __LINE__, \
    __func__, #b, "" __VA_ARGS__))

#define fatal(...) (void)(vigil.fatal_termination(               \
    __FILE__, __LINE__,  __func__, "",  "" __VA_ARGS__))

#define fatal_if(b, ...) __suppress_constant_cond_exp__          \
    /* const cond */                                             \
    (void)((!(b)) || vigil.fatal_termination(__FILE__, __LINE__, \
    __func__, #b, "" __VA_ARGS__))

#define fatal_if_not(b, ...) __suppress_constant_cond_exp__       \
    /* const cond */                                              \
    (void)((!!(b)) || vigil.fatal_termination(__FILE__, __LINE__, \
    __func__, #b, "" __VA_ARGS__))

#define fatal_if_false fatal_if_not
#define fatal_if_not_zero(e, ...) fatal_if((e) != 0, "" __VA_ARGS__)
#define fatal_if_null(e, ...) fatal_if((e) == null, "" __VA_ARGS__)
#define not_null(e, ...) fatal_if_null(e, "" __VA_ARGS__)


end_c

#endif // ut_definition

#ifdef ut_implementation
// ________________________________ ut_win32.h ________________________________

#ifdef WIN32

#pragma warning(push)
#pragma warning(disable: 4255) // no function prototype: '()' to '(void)'
#pragma warning(disable: 4459) // declaration of '...' hides global declaration

// ut:
#include <Windows.h>  // used by:
#include <psapi.h>    // both ut_loader.c and ut_processes.c
#include <shellapi.h> // ut_processes.c
#include <winternl.h> // ut_processes.c
#include <initguid.h>     // for knownfolders
#include <knownfolders.h> // ut_files.c
#include <aclapi.h>       // ut_files.c
#include <shlobj_core.h>  // ut_files.c
#include <shlwapi.h>      // ut_files.c
// ui:
#include <windowsx.h>
#include <commdlg.h>
#include <dwmapi.h>
#include <ShellScalingApi.h>
#include <VersionHelpers.h>

#pragma warning(pop)

#include <fcntl.h>

#define export __declspec(dllexport)

#define b2e(call) (call ? 0 : GetLastError()) // BOOL -> errno_t

#define wait2e(ix) (errno_t)                                                     \
    ((int32_t)WAIT_OBJECT_0 <= (int32_t)(ix) && (ix) <= WAIT_OBJECT_0 + 63 ? 0 : \
      ((ix) == WAIT_ABANDONED ? ERROR_REQUEST_ABORTED :                          \
        ((ix) == WAIT_TIMEOUT ? ERROR_TIMEOUT :                                  \
          ((ix) == WAIT_FAILED) ? (errno_t)GetLastError() : ERROR_INVALID_HANDLE \
        )                                                                        \
      )                                                                          \
    )


#endif // WIN32
// ___________________________________ ut.c ___________________________________

// #include "ut/macos.h" // TODO
// #include "ut/linux.h" // TODO


// ________________________________ ut_args.c _________________________________

static void* ut_args_memory;

static void ut_args_main(int32_t argc, const char* argv[], const char** env) {
    swear(ut_args.c == 0 && ut_args.v == null && ut_args.env == null);
    swear(ut_args_memory == null);
    ut_args.c = argc;
    ut_args.v = argv;
    ut_args.env = env;
}

static int32_t ut_args_option_index(const char* option) {
    for (int32_t i = 1; i < ut_args.c; i++) {
        if (strcmp(ut_args.v[i], "--") == 0) { break; } // no options after '--'
        if (strcmp(ut_args.v[i], option) == 0) { return i; }
    }
    return -1;
}

static void ut_args_remove_at(int32_t ix) {
    // returns new argc
    assert(0 < ut_args.c);
    assert(0 < ix && ix < ut_args.c); // cannot remove ut_args.v[0]
    for (int32_t i = ix; i < ut_args.c; i++) {
        ut_args.v[i] = ut_args.v[i + 1];
    }
    ut_args.v[ut_args.c - 1] = "";
    ut_args.c--;
}

static bool ut_args_option_bool(const char* option) {
    int32_t ix = ut_args_option_index(option);
    if (ix > 0) { ut_args_remove_at(ix); }
    return ix > 0;
}

static bool ut_args_option_int(const char* option, int64_t *value) {
    int32_t ix = ut_args_option_index(option);
    if (ix > 0 && ix < ut_args.c - 1) {
        const char* s = ut_args.v[ix + 1];
        int32_t base = (strstr(s, "0x") == s || strstr(s, "0X") == s) ? 16 : 10;
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
        ut_args_remove_at(ix); // remove option
        ut_args_remove_at(ix); // remove following number
    }
    return ix > 0;
}

static const char* ut_args_option_str(const char* option) {
    int32_t ix = ut_args_option_index(option);
    const char* s = null;
    if (ix > 0 && ix < ut_args.c - 1) {
        s = ut_args.v[ix + 1];
    } else {
        ix = -1;
    }
    if (ix > 0) {
        ut_args_remove_at(ix); // remove option
        ut_args_remove_at(ix); // remove following string
    }
    return ix > 0 ? s : null;
}

// Terminology: "quote" in the code and comments below
// actually refers to "fp64_t quote mark" and used for brevity.

// TODO: posix like systems
// Looks like all shells support quote marks but
// AFAIK MacOS bash and zsh also allow (and prefer) backslash escaped
// space character. Unclear what other escaping shell and posix compliant
// parser should support.
// Lengthy discussion here:
// https://stackoverflow.com/questions/1706551/parse-string-into-argv-argc

// Microsoft specific argument parsing:
// https://web.archive.org/web/20231115181633/http://learn.microsoft.com/en-us/cpp/c-language/parsing-c-command-line-arguments?view=msvc-170
// Alternative: just use CommandLineToArgvW()

typedef struct { const char* s; char* d; const char* e; } ut_args_pair_t;

static ut_args_pair_t ut_args_parse_backslashes(ut_args_pair_t p) {
    enum { quote = '"', backslash = '\\' };
    const char* s = p.s;
    char* d = p.d;
    swear(*s == backslash);
    int32_t bsc = 0; // number of backslashes
    while (*s == backslash) { s++; bsc++; }
    if (*s == quote) {
        while (bsc > 1 && d < p.e) { *d++ = backslash; bsc -= 2; }
        if (bsc == 1 && d < p.e) { *d++ = *s++; }
    } else {
        // Backslashes are interpreted literally,
        // unless they immediately precede a quote:
        while (bsc > 0 && d < p.e) { *d++ = backslash; bsc--; }
    }
    return (ut_args_pair_t){ .s = s, .d = d, .e = p.e };
}

static ut_args_pair_t ut_args_parse_quoted(ut_args_pair_t p) {
    enum { quote = '"', backslash = '\\' };
    const char* s = p.s;
    char* d = p.d;
    swear(*s == quote);
    s++; // opening quote (skip)
    while (*s != 0x00) {
        if (*s == backslash) {
            p = ut_args_parse_backslashes((ut_args_pair_t){
                        .s = s, .d = d, .e = p.e });
            s = p.s; d = p.d;
        } else if (*s == quote && s[1] == quote) {
            // Within a quoted string, a pair of quote is
            // interpreted as a single escaped quote.
            if (d < p.e) { *d++ = *s++; }
            s++; // 1 for 2 quotes
        } else if (*s == quote) {
            s++; // closing quote (skip)
            break;
        } else if (d < p.e) {
            *d++ = *s++;
        }
    }
    return (ut_args_pair_t){ .s = s, .d = d, .e = p.e };
}

static void ut_args_parse(const char* s) {
    swear(s[0] != 0, "cannot parse empty string");
    swear(ut_args.c == 0);
    swear(ut_args.v == null);
    swear(ut_args_memory == null);
    enum { quote = '"', backslash = '\\', tab = '\t', space = 0x20 };
    const int32_t len = (int)strlen(s);
    // Worst-case scenario (possible to optimize with dry run of parse)
    // at least 2 characters per token in "a b c d e" plush null at the end:
    const int32_t k = ((len + 2) / 2 + 1) * (int)sizeof(void*) + (int)sizeof(void*);
    const int32_t n = k + (len + 2) * (int)sizeof(char);
    fatal_if_not_zero(ut_heap.allocate(null, &ut_args_memory, n, true));
    ut_args.c = 0;
    ut_args.v = (const char**)ut_args_memory;
    char* d = (char*)(((char*)ut_args.v) + k);
    char* e = d + n; // end of memory
    // special rules for 1st argument:
    if (ut_args.c < n) { ut_args.v[ut_args.c++] = d; }
    if (*s == quote) {
        s++;
        while (*s != 0x00 && *s != quote && d < e) { *d++ = *s++; }
        while (*s != 0x00) { s++; }
    } else {
        while (*s != 0x00 && *s != space && *s != tab && d < e) {
            *d++ = *s++;
        }
    }
    if (d < e) { *d++ = 0; }
    while (d < e) {
        while (*s == space || *s == tab) { s++; }
        if (*s == 0) { break; }
        if (*s == quote && s[1] == 0 && d < e) { // unbalanced single quote
            if (ut_args.c < n) { ut_args.v[ut_args.c++] = d; } // spec does not say what to do
            *d++ = *s++;
        } else if (*s == quote) { // quoted arg
            if (ut_args.c < n) { ut_args.v[ut_args.c++] = d; }
            ut_args_pair_t p = ut_args_parse_quoted(
                    (ut_args_pair_t){ .s = s, .d = d, .e = e });
            s = p.s; d = p.d;
        } else { // non-quoted arg (that can have quoted strings inside)
            if (ut_args.c < n) { ut_args.v[ut_args.c++] = d; }
            while (*s != 0) {
                if (*s == backslash) {
                    ut_args_pair_t p = ut_args_parse_backslashes(
                            (ut_args_pair_t){ .s = s, .d = d, .e = e });
                    s = p.s; d = p.d;
                } else if (*s == quote) {
                    ut_args_pair_t p = ut_args_parse_quoted(
                            (ut_args_pair_t){ .s = s, .d = d, .e = e });
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
    if (ut_args.c < n) {
        ut_args.v[ut_args.c] = null;
    }
    swear(ut_args.c < n, "not enough memory - adjust guestimates");
    swear(d <= e, "not enough memory - adjust guestimates");
}

const char* ut_args_basename(void) {
    static char basename[260];
    swear(ut_args.c > 0);
    if (basename[0] == 0) {
        const char* s = ut_args.v[0];
        const char* b = s;
        while (*s != 0) {
            if (*s == '\\' || *s == '/') { b = s + 1; }
            s++;
        }
        int32_t n = ut_str.length(b);
        swear(n < countof(basename));
        strncpy(basename, b, countof(basename) - 1);
        char* d = basename + n - 1;
        while (d > basename && *d != '.') { d--; }
        if (*d == '.') { *d = 0x00; }
    }
    return basename;
}

static void ut_args_fini(void) {
    ut_heap.deallocate(null, ut_args_memory); // can be null is parse() was not called
    ut_args_memory = null;
    ut_args.c = 0;
    ut_args.v = null;
}

static void ut_args_WinMain(void) {
    swear(ut_args.c == 0 && ut_args.v == null && ut_args.env == null);
    swear(ut_args_memory == null);
    const uint16_t* wcl = GetCommandLineW();
    int32_t n = (int32_t)wcslen(wcl);
    char* cl = null;
    fatal_if_not_zero(ut_heap.allocate(null, &cl, n * 2 + 1, false));
    ut_args_parse(ut_str.utf16_utf8(cl, wcl));
    ut_heap.deallocate(null, cl);
    ut_args.env = _environ;
}

#ifdef UT_TESTS

// https://learn.microsoft.com/en-us/cpp/c-language/parsing-c-command-line-arguments
// Command-line input       argv[1]     argv[2]	    argv[3]
// "a b c" d e	            a b c       d           e
// "ab\"c" "\\" d           ab"c        \           d
// a\\\b d"e f"g h          a\\\b       de fg       h
// a\\\"b c d               a\"b        c           d
// a\\\\"b c" d e           a\\b c      d           e
// a"b"" c d                ab" c d

#ifndef __INTELLISENSE__ // confused data analysis

static void ut_args_test_verify(const char* cl, int32_t expected, ...) {
    if (ut_debug.verbosity.level >= ut_debug.verbosity.trace) {
        traceln("cl: `%s`", cl);
    }
    int32_t argc = ut_args.c;
    const char** argv = ut_args.v;
    void* memory = ut_args_memory;
    ut_args.c = 0;
    ut_args.v = null;
    ut_args_memory = null;
    ut_args_parse(cl);
    va_list vl;
    va_start(vl, expected);
    for (int32_t i = 0; i < expected; i++) {
        const char* s = va_arg(vl, const char*);
//      if (ut_debug.verbosity.level >= ut_debug.verbosity.trace) {
//          traceln("ut_args.v[%d]: `%s` expected: `%s`", i, ut_args.v[i], s);
//      }
        // Warning 6385: reading data outside of array
        const char* ai = _Pragma("warning(suppress:  6385)")ut_args.v[i];
        swear(strcmp(ai, s) == 0, "ut_args.v[%d]: `%s` expected: `%s`",
              i, ai, s);
    }
    va_end(vl);
    ut_args.fini();
    // restore command line arguments:
    ut_args.c = argc;
    ut_args.v = argv;
    ut_args_memory = memory;
}

#endif // __INTELLISENSE__

static void ut_args_test(void) {
    // The first argument (ut_args.v[0]) is treated specially.
    // It represents the program name. Because it must be a valid pathname,
    // parts surrounded by quote (") are allowed. The quote aren't included
    // in the ut_args.v[0] output. The parts surrounded by quote prevent
    // interpretation of a space or tab character as the end of the argument.
    // The escaping rules don't apply.
    ut_args_test_verify("\"c:\\foo\\bar\\snafu.exe\"", 1,
                     "c:\\foo\\bar\\snafu.exe");
    ut_args_test_verify("c:\\foo\\bar\\snafu.exe", 1,
                     "c:\\foo\\bar\\snafu.exe");
    ut_args_test_verify("foo.exe \"a b c\" d e", 4,
                     "foo.exe", "a b c", "d", "e");
    ut_args_test_verify("foo.exe \"ab\\\"c\" \"\\\\\" d", 4,
                     "foo.exe", "ab\"c", "\\", "d");
    ut_args_test_verify("foo.exe a\\\\\\b d\"e f\"g h", 4,
                     "foo.exe", "a\\\\\\b", "de fg", "h");
    ut_args_test_verify("foo.exe a\\\\\\b d\"e f\"g h", 4,
                     "foo.exe", "a\\\\\\b", "de fg", "h");
    ut_args_test_verify("foo.exe a\"b\"\" c d", 2, // unmatched quote
                     "foo.exe", "ab\" c d");
    // unbalanced quote and backslash:
    ut_args_test_verify("foo.exe \"",     2, "foo.exe", "\"");
    ut_args_test_verify("foo.exe \\",     2, "foo.exe", "\\");
    ut_args_test_verify("foo.exe \\\\",   2, "foo.exe", "\\\\");
    ut_args_test_verify("foo.exe \\\\\\", 2, "foo.exe", "\\\\\\");
    if (ut_debug.verbosity.level > ut_debug.verbosity.quiet) { traceln("done"); }
}

#else

static void ut_args_test(void) {}

#endif

ut_args_if ut_args = {
    .main         = ut_args_main,
    .WinMain      = ut_args_WinMain,
    .option_index = ut_args_option_index,
    .remove_at    = ut_args_remove_at,
    .option_bool  = ut_args_option_bool,
    .option_int   = ut_args_option_int,
    .option_str   = ut_args_option_str,
    .basename     = ut_args_basename,
    .fini         = ut_args_fini,
    .test         = ut_args_test
};
// _______________________________ ut_atomics.c _______________________________

#include <stdatomic.h> // needs cl.exe /experimental:c11atomics command line

// see: https://developercommunity.visualstudio.com/t/C11--C17-include-stdatomich-issue/10620622

#define ATOMICS_HAS_STDATOMIC_H

#ifndef ATOMICS_HAS_STDATOMIC_H

static int32_t ut_atomics_increment_int32(volatile int32_t* a) {
    return InterlockedIncrement((volatile LONG*)a);
}

static int32_t ut_atomics_decrement_int32(volatile int32_t* a) {
    return InterlockedDecrement((volatile LONG*)a);
}

static int64_t ut_atomics_increment_int64(volatile int64_t* a) {
    return InterlockedIncrement64((__int64 volatile *)a);
}

static int64_t ut_atomics_decrement_int64(volatile int64_t* a) {
    return InterlockedDecrement64((__int64 volatile *)a);
}

static int32_t ut_atomics_add_int32(volatile int32_t* a, int32_t v) {
    return InterlockedAdd((LONG volatile *)a, v);
}

static int64_t ut_atomics_add_int64(volatile int64_t* a, int64_t v) {
    return InterlockedAdd64((__int64 volatile *)a, v);
}

static int64_t ut_atomics_exchange_int64(volatile int64_t* a, int64_t v) {
    return (int64_t)InterlockedExchange64((LONGLONG*)a, (LONGLONG)v);
}

static int32_t ut_atomics_exchange_int32(volatile int32_t* a, int32_t v) {
    assert(sizeof(int32_t) == sizeof(unsigned long));
    return (int32_t)InterlockedExchange((volatile LONG*)a, (unsigned long)v);
}

static bool ut_atomics_compare_exchange_int64(volatile int64_t* a,
        int64_t comparand, int64_t v) {
    return (int64_t)InterlockedCompareExchange64((LONGLONG*)a,
        (LONGLONG)v, (LONGLONG)comparand) == comparand;
}

static bool ut_atomics_compare_exchange_int32(volatile int32_t* a,
        int32_t comparand, int32_t v) {
    return (int64_t)InterlockedCompareExchange((LONG*)a,
        (LONG)v, (LONG)comparand) == comparand;
}

static void memory_fence(void) { _mm_mfence(); }

#else

// stdatomic.h version:

#ifndef __INTELLISENSE__ // IntelliSense chokes on _Atomic(_Type)
// __INTELLISENSE__ Defined as 1 during an IntelliSense compiler pass
// in the Visual Studio IDE. Otherwise, undefined. You can use this macro
// to guard code the IntelliSense compiler doesn't understand,
// or use it to toggle between the build and IntelliSense compiler.


// _strong() operations are the same as _explicit(..., memory_order_seq_cst)
// memory_order_seq_cst stands for Sequentially Consistent Ordering
//
// This is the strongest memory order, providing the guarantee that
// all sequentially consistent operations appear to be executed in
// the same order on all threads (cores)
//
// int_fast32_t: Fastest integer type with at least 32 bits.
// int_least32_t: Smallest integer type with at least 32 bits.

static_assertion(sizeof(int32_t) == sizeof(int_fast32_t));
static_assertion(sizeof(int32_t) == sizeof(int_least32_t));

static int32_t ut_atomics_increment_int32(volatile int32_t* a) {
    return atomic_fetch_add((atomic_int_fast32_t*)a, 1) + 1;
}

static int32_t ut_atomics_decrement_int32(volatile int32_t* a) {
    return atomic_fetch_sub((atomic_int_fast32_t*)a, 1) - 1;
}

static int64_t ut_atomics_increment_int64(volatile int64_t* a) {
    return atomic_fetch_add((atomic_int_fast64_t*)a, 1) + 1;
}

static int64_t ut_atomics_decrement_int64(volatile int64_t* a) {
    return atomic_fetch_sub((atomic_int_fast64_t*)a, 1) - 1;
}

static int32_t ut_atomics_add_int32(volatile int32_t* a, int32_t v) {
    return atomic_fetch_add((atomic_int_fast32_t*)a, v) + v;
}

static int64_t ut_atomics_add_int64(volatile int64_t* a, int64_t v) {
    return atomic_fetch_add((atomic_int_fast64_t*)a, v) + v;
}

static int64_t ut_atomics_exchange_int64(volatile int64_t* a, int64_t v) {
    return atomic_exchange((atomic_int_fast64_t*)a, v);
}

static int32_t ut_atomics_exchange_int32(volatile int32_t* a, int32_t v) {
    return atomic_exchange((atomic_int_fast32_t*)a, v);
}

static bool ut_atomics_compare_exchange_int64(volatile int64_t* a,
    int64_t comparand, int64_t v) {
    return atomic_compare_exchange_strong((atomic_int_fast64_t*)a,
        &comparand, v);
}

// Code here is not "seen" by IntelliSense but is compiled normally.
static bool ut_atomics_compare_exchange_int32(volatile int32_t* a,
    int32_t comparand, int32_t v) {
    return atomic_compare_exchange_strong((atomic_int_fast32_t*)a,
        &comparand, v);
}

static void memory_fence(void) { atomic_thread_fence(memory_order_seq_cst); }

#endif // __INTELLISENSE__

#endif // ATOMICS_HAS_STDATOMIC_H

static int32_t ut_atomics_load_int32(volatile int32_t* a) {
    return ut_atomics.add_int32(a, 0);
}

static int64_t ut_atomics_load_int64(volatile int64_t* a) {
    return ut_atomics.add_int64(a, 0);
}

static void* ut_atomics_exchange_ptr(volatile void* *a, void* v) {
    static_assertion(sizeof(void*) == sizeof(uint64_t));
    return (void*)ut_atomics.exchange_int64((int64_t*)a, (int64_t)v);
}

static bool ut_atomics_compare_exchange_ptr(volatile void* *a, void* comparand, void* v) {
    static_assertion(sizeof(void*) == sizeof(int64_t));
    return ut_atomics.compare_exchange_int64((int64_t*)a,
        (int64_t)comparand, (int64_t)v);
}

// https://en.wikipedia.org/wiki/Spinlock

#define __sync_bool_compare_and_swap(p, old_val, new_val) \
    _InterlockedCompareExchange64(p, new_val, old_val) == old_val

// https://stackoverflow.com/questions/37063700/mm-pause-usage-in-gcc-on-intel
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
    ut_atomics.memory_fence();
    // not strcitly necessary on strong mem model Intel/AMD but
    // see: https://cfsamsonbooks.gitbook.io/explaining-atomics-in-rust/
    //      Fig 2 Inconsistent C11 execution of SB and 2+2W
    assert(*spinlock == 1);
}

static void spinlock_release(volatile int64_t* spinlock) {
    assert(*spinlock == 1);
    *spinlock = 0;
    // tribute to lengthy Linus discussion going since 2006:
    ut_atomics.memory_fence();
}

static void ut_atomics_test(void) {
    #ifdef UT_TESTS
    volatile int32_t int32_var = 0;
    volatile int64_t int64_var = 0;
    volatile void* ptr_var = null;
    int64_t spinlock = 0;
    void* old_ptr = ut_atomics.exchange_ptr(&ptr_var, (void*)123);
    swear(old_ptr == null);
    swear(ptr_var == (void*)123);
    int32_t incremented_int32 = ut_atomics.increment_int32(&int32_var);
    swear(incremented_int32 == 1);
    swear(int32_var == 1);
    int32_t decremented_int32 = ut_atomics.decrement_int32(&int32_var);
    swear(decremented_int32 == 0);
    swear(int32_var == 0);
    int64_t incremented_int64 = ut_atomics.increment_int64(&int64_var);
    swear(incremented_int64 == 1);
    swear(int64_var == 1);
    int64_t decremented_int64 = ut_atomics.decrement_int64(&int64_var);
    swear(decremented_int64 == 0);
    swear(int64_var == 0);
    int32_t added_int32 = ut_atomics.add_int32(&int32_var, 5);
    swear(added_int32 == 5);
    swear(int32_var == 5);
    int64_t added_int64 = ut_atomics.add_int64(&int64_var, 10);
    swear(added_int64 == 10);
    swear(int64_var == 10);
    int32_t old_int32 = ut_atomics.exchange_int32(&int32_var, 3);
    swear(old_int32 == 5);
    swear(int32_var == 3);
    int64_t old_int64 = ut_atomics.exchange_int64(&int64_var, 6);
    swear(old_int64 == 10);
    swear(int64_var == 6);
    bool int32_exchanged = ut_atomics.compare_exchange_int32(&int32_var, 3, 4);
    swear(int32_exchanged);
    swear(int32_var == 4);
    bool int64_exchanged = ut_atomics.compare_exchange_int64(&int64_var, 6, 7);
    swear(int64_exchanged);
    swear(int64_var == 7);
    ptr_var = (void*)0x123;
    bool ptr_exchanged = ut_atomics.compare_exchange_ptr(&ptr_var,
        (void*)0x123, (void*)0x456);
    swear(ptr_exchanged);
    swear(ptr_var == (void*)0x456);
    ut_atomics.spinlock_acquire(&spinlock);
    swear(spinlock == 1);
    ut_atomics.spinlock_release(&spinlock);
    swear(spinlock == 0);
    int32_t loaded_int32 = ut_atomics.load32(&int32_var);
    swear(loaded_int32 == int32_var);
    int64_t loaded_int64 = ut_atomics.load64(&int64_var);
    swear(loaded_int64 == int64_var);
    ut_atomics.memory_fence();
    if (ut_debug.verbosity.level > ut_debug.verbosity.quiet) { traceln("done"); }
    #endif
}

#ifndef __INTELLISENSE__ // IntelliSense chokes on _Atomic(_Type)

static_assertion(sizeof(void*) == sizeof(int64_t));
static_assertion(sizeof(void*) == sizeof(uintptr_t));

ut_atomics_if ut_atomics = {
    .exchange_ptr    = ut_atomics_exchange_ptr,
    .increment_int32 = ut_atomics_increment_int32,
    .decrement_int32 = ut_atomics_decrement_int32,
    .increment_int64 = ut_atomics_increment_int64,
    .decrement_int64 = ut_atomics_decrement_int64,
    .add_int32 = ut_atomics_add_int32,
    .add_int64 = ut_atomics_add_int64,
    .exchange_int32  = ut_atomics_exchange_int32,
    .exchange_int64  = ut_atomics_exchange_int64,
    .compare_exchange_int64 = ut_atomics_compare_exchange_int64,
    .compare_exchange_int32 = ut_atomics_compare_exchange_int32,
    .compare_exchange_ptr = ut_atomics_compare_exchange_ptr,
    .load32 = ut_atomics_load_int32,
    .load64 = ut_atomics_load_int64,
    .spinlock_acquire = spinlock_acquire,
    .spinlock_release = spinlock_release,
    .memory_fence = memory_fence,
    .test = ut_atomics_test
};

#endif // __INTELLISENSE__

// 2024-03-20 latest windows runtime and toolchain cl.exe
// ... VC\Tools\MSVC\14.39.33519\include
// see:
//     vcruntime_c11_atomic_support.h
//     vcruntime_c11_stdatomic.h
//     stdatomic.h
// https://developercommunity.visualstudio.com/t/C11--C17-include--issue/10620622
// cl.exe /std:c11 /experimental:c11atomics
// command line option are required
// even in C17 mode in spring of 2024

// ______________________________ ut_clipboard.c ______________________________

static errno_t ut_clipboard_put_text(const char* utf8) {
    errno_t r = 0;
    int32_t chars = ut_str.utf16_chars(utf8);
    int32_t bytes = (chars + 1) * 2;
    wchar_t* utf16 = (wchar_t*)malloc(bytes);
    if (utf16 == null) {
        r = ERROR_OUTOFMEMORY;
    } else {
        ut_str.utf8_utf16(utf16, utf8);
        assert(utf16[chars - 1] == 0);
        const int32_t n = (int)wcslen(utf16) + 1;
        r = OpenClipboard(GetDesktopWindow()) ? 0 : GetLastError();
        if (r != 0) { traceln("OpenClipboard() failed %s", ut_str.error(r)); }
        if (r == 0) {
            r = EmptyClipboard() ? 0 : GetLastError();
            if (r != 0) { traceln("EmptyClipboard() failed %s", ut_str.error(r)); }
        }
        void* global = null;
        if (r == 0) {
            global = GlobalAlloc(GMEM_MOVEABLE, n * 2);
            r = global != null ? 0 : GetLastError();
            if (r != 0) { traceln("GlobalAlloc() failed %s", ut_str.error(r)); }
        }
        if (r == 0) {
            char* d = (char*)GlobalLock(global);
            not_null(d);
            memcpy(d, utf16, n * 2);
            r = SetClipboardData(CF_UNICODETEXT, global) ? 0 : GetLastError();
            GlobalUnlock(global);
            if (r != 0) {
                traceln("SetClipboardData() failed %s", ut_str.error(r));
                GlobalFree(global);
            } else {
                // do not free global memory. It's owned by system clipboard now
            }
        }
        if (r == 0) {
            r = CloseClipboard() ? 0 : GetLastError();
            if (r != 0) {
                traceln("CloseClipboard() failed %s", ut_str.error(r));
            }
        }
        free(utf16);
    }
    return r;
}

static errno_t ut_clipboard_get_text(char* utf8, int32_t* bytes) {
    not_null(bytes);
    int r = OpenClipboard(GetDesktopWindow()) ? 0 : GetLastError();
    if (r != 0) { traceln("OpenClipboard() failed %s", ut_str.error(r)); }
    if (r == 0) {
        HANDLE global = GetClipboardData(CF_UNICODETEXT);
        if (global == null) {
            r = GetLastError();
        } else {
            wchar_t* utf16 = (wchar_t*)GlobalLock(global);
            if (utf16 != null) {
                int32_t utf8_bytes = ut_str.utf8_bytes(utf16);
                if (utf8 != null) {
                    char* decoded = (char*)malloc(utf8_bytes);
                    if (decoded == null) {
                        r = ERROR_OUTOFMEMORY;
                    } else {
                        ut_str.utf16_utf8(decoded, utf16);
                        int32_t n = ut_min(*bytes, utf8_bytes);
                        memcpy(utf8, decoded, n);
                        free(decoded);
                        if (n < utf8_bytes) {
                            r = ERROR_INSUFFICIENT_BUFFER;
                        }
                    }
                }
                *bytes = utf8_bytes;
                GlobalUnlock(global);
            }
        }
        r = CloseClipboard() ? 0 : GetLastError();
    }
    return r;
}

#ifdef UT_TESTS

static void ut_clipboard_test(void) {
    fatal_if_not_zero(ut_clipboard.put_text("Hello Clipboard"));
    char text[256];
    int32_t bytes = countof(text);
    fatal_if_not_zero(ut_clipboard.get_text(text, &bytes));
    swear(strequ(text, "Hello Clipboard"));
    if (ut_debug.verbosity.level > ut_debug.verbosity.quiet) { traceln("done"); }
}

#else

static void ut_clipboard_test(void) {
}

#endif

ut_clipboard_if ut_clipboard = {
    .put_text   = ut_clipboard_put_text,
    .get_text   = ut_clipboard_get_text,
    .put_image  = null, // implemented in ui.app
    .test       = ut_clipboard_test
};

// ________________________________ ut_clock.c ________________________________

enum {
    ut_clock_nsec_in_usec = 1000, // nano in micro
    ut_clock_nsec_in_msec = ut_clock_nsec_in_usec * 1000, // nano in milli
    ut_clock_nsec_in_sec  = ut_clock_nsec_in_msec * 1000,
    ut_clock_usec_in_msec = 1000, // micro in mill
    ut_clock_msec_in_sec  = 1000, // milli in sec
    ut_clock_usec_in_sec  = ut_clock_usec_in_msec * ut_clock_msec_in_sec // micro in sec
};

static uint64_t ut_clock_microseconds_since_epoch(void) { // NOT monotonic
    FILETIME ft; // time in 100ns interval (tenth of microsecond)
    // since 12:00 A.M. January 1, 1601 Coordinated Universal Time (UTC)
    GetSystemTimePreciseAsFileTime(&ft);
    uint64_t microseconds =
        (((uint64_t)ft.dwHighDateTime) << 32 | ft.dwLowDateTime) / 10;
    assert(microseconds > 0);
    return microseconds;
}

static uint64_t ut_clock_localtime(void) {
    TIME_ZONE_INFORMATION tzi; // UTC = local time + bias
    GetTimeZoneInformation(&tzi);
    uint64_t bias = (uint64_t)tzi.Bias * 60LL * 1000 * 1000; // in microseconds
    return ut_clock_microseconds_since_epoch() - bias;
}

static void ut_clock_utc(uint64_t microseconds,
        int32_t* year, int32_t* month, int32_t* day,
        int32_t* hh, int32_t* mm, int32_t* ss, int32_t* ms, int32_t* mc) {
    uint64_t time_in_100ns = microseconds * 10;
    FILETIME mst = { (DWORD)(time_in_100ns & 0xFFFFFFFF),
                     (DWORD)(time_in_100ns >> 32) };
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

static void ut_clock_local(uint64_t microseconds,
        int32_t* year, int32_t* month, int32_t* day,
        int32_t* hh, int32_t* mm, int32_t* ss, int32_t* ms, int32_t* mc) {
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

static fp64_t ut_clock_seconds(void) { // since_boot
    LARGE_INTEGER qpc;
    QueryPerformanceCounter(&qpc);
    static fp64_t one_over_freq;
    if (one_over_freq == 0) {
        LARGE_INTEGER frequency;
        QueryPerformanceFrequency(&frequency);
        one_over_freq = 1.0 / frequency.QuadPart;
    }
    return (fp64_t)qpc.QuadPart * one_over_freq;
}

// Max duration in nanoseconds=2^64 - 1 nanoseconds
//                          2^64 - 1 ns        1 sec          1 min
// Max Duration in Hours =  ----------- x  ------------ x -------------
//                          10^9 ns / s    60 sec / min   60 min / hour
//
//                              1 hour
// Max Duration in Days =  ---------------
//                          24 hours / day
//
// it would take approximately 213,503 days (or about 584.5 years)
// for ut_clock.nanoseconds() to overflow
//
// for divider = ut_num.gcd32(nsec_in_sec, freq) below and 10MHz timer
// the actual duration is shorter because of (mul == 100)
//    (uint64_t)qpc.QuadPart * mul
// 64 bit overflow and is about 5.8 years.
//
// In a long running code like services is advisable to use
// ut_clock.nanoseconds() to measure only deltas and pay close attention
// to the wrap around despite of 5 years monotony

static uint64_t ut_clock_nanoseconds(void) {
    LARGE_INTEGER qpc;
    QueryPerformanceCounter(&qpc);
    static uint32_t freq;
    static uint32_t mul = ut_clock_nsec_in_sec;
    if (freq == 0) {
        LARGE_INTEGER frequency;
        QueryPerformanceFrequency(&frequency);
        assert(frequency.HighPart == 0);
        // even 1GHz frequency should fit into 32 bit unsigned
        assert(frequency.HighPart == 0, "%08lX%%08lX",
               frequency.HighPart, frequency.LowPart);
        // known values: 10,000,000 and 3,000,000 10MHz, 3MHz
        assert(frequency.LowPart % (1000 * 1000) == 0);
        // if we start getting weird frequencies not
        // multiples of MHz ut_num.gcd() approach may need
        // to be revised in favor of ut_num.muldiv64x64()
        freq = frequency.LowPart;
        assert(freq != 0 && freq < (uint32_t)ut_clock.nsec_in_sec);
        // to avoid ut_num.muldiv128:
        uint32_t divider = ut_num.gcd32(ut_clock.nsec_in_sec, freq);
        freq /= divider;
        mul  /= divider;
    }
    uint64_t ns_mul_freq = (uint64_t)qpc.QuadPart * mul;
    return freq == 1 ? ns_mul_freq : ns_mul_freq / freq;
}

// Difference between 1601 and 1970 in microseconds:

const uint64_t ut_clock_epoch_diff_usec = 11644473600000000ULL;

static uint64_t ut_clock_unix_microseconds(void) {
    return ut_clock.microseconds() - ut_clock_epoch_diff_usec;
}

static uint64_t ut_clock_unix_seconds(void) {
    return ut_clock.unix_microseconds() / ut_clock.usec_in_sec;
}

static void ut_clock_test(void) {
    #ifdef UT_TESTS
    // TODO: implement more tests
    uint64_t t0 = ut_clock.nanoseconds();
    uint64_t t1 = ut_clock.nanoseconds();
    int32_t count = 0;
    while (t0 == t1 && count < 1024) {
        t1 = ut_clock.nanoseconds();
        count++;
    }
    swear(t0 != t1, "count: %d t0: %lld t1: %lld", count, t0, t1);
    if (ut_debug.verbosity.level > ut_debug.verbosity.quiet) { traceln("done"); }
    #endif
}

ut_clock_if ut_clock = {
    .nsec_in_usec      = ut_clock_nsec_in_usec,
    .nsec_in_msec      = ut_clock_nsec_in_msec,
    .nsec_in_sec       = ut_clock_nsec_in_sec,
    .usec_in_msec      = ut_clock_usec_in_msec,
    .msec_in_sec       = ut_clock_msec_in_sec,
    .usec_in_sec       = ut_clock_usec_in_sec,
    .seconds           = ut_clock_seconds,
    .nanoseconds       = ut_clock_nanoseconds,
    .unix_microseconds = ut_clock_unix_microseconds,
    .unix_seconds      = ut_clock_unix_seconds,
    .microseconds      = ut_clock_microseconds_since_epoch,
    .localtime         = ut_clock_localtime,
    .utc               = ut_clock_utc,
    .local             = ut_clock_local,
    .test              = ut_clock_test
};

// _______________________________ ut_config.c ________________________________

// On Unix the implementation should keep KV pairs in
// key-named files inside .name/ folder

static const char* ut_config_app = "Software\\apps";

static const DWORD ut_config_access =
    KEY_READ|KEY_WRITE|KEY_SET_VALUE|KEY_QUERY_VALUE|
    KEY_ENUMERATE_SUB_KEYS|DELETE;

static errno_t ut_config_get_reg_key(const char* name, HKEY *key) {
    errno_t r = 0;
    char path[256] = {0};
    strprintf(path, "%s\\%s", ut_config_app, name);
    if (RegOpenKeyExA(HKEY_CURRENT_USER, path, 0, ut_config_access, key) != 0) {
        const DWORD option = REG_OPTION_NON_VOLATILE;
        r = RegCreateKeyExA(HKEY_CURRENT_USER, path, 0, null, option,
                            ut_config_access, null, key, null);
    }
    return r;
}

static errno_t ut_config_save(const char* name,
        const char* key, const void* data, int32_t bytes) {
    errno_t r = 0;
    HKEY k = null;
    r = ut_config_get_reg_key(name, &k);
    if (k != null) {
        r = RegSetValueExA(k, key, 0, REG_BINARY,
            (byte*)data, bytes);
        fatal_if_not_zero(RegCloseKey(k));
    }
    return r;
}

static errno_t ut_config_remove(const char* name, const char* key) {
    errno_t r = 0;
    HKEY k = null;
    r = ut_config_get_reg_key(name, &k);
    if (k != null) {
        r = RegDeleteValueA(k, key);
        fatal_if_not_zero(RegCloseKey(k));
    }
    return r;
}

static errno_t ut_config_clean(const char* name) {
    errno_t r = 0;
    HKEY k = null;
    if (RegOpenKeyExA(HKEY_CURRENT_USER, ut_config_app,
                                      0, ut_config_access, &k) == 0) {
       r = RegDeleteTreeA(k, name);
       fatal_if_not_zero(RegCloseKey(k));
    }
    return r;
}

static int32_t ut_config_size(const char* name, const char* key) {
    int32_t bytes = -1;
    HKEY k = null;
    errno_t r = ut_config_get_reg_key(name, &k);
    if (k != null) {
        DWORD type = REG_BINARY;
        DWORD cb = 0;
        r = RegQueryValueExA(k, key, null, &type, null, &cb);
        if (r == ERROR_FILE_NOT_FOUND) {
            bytes = 0; // do not report data_size() often used this way
        } else if (r != 0) {
            traceln("%s.RegQueryValueExA(\"%s\") failed %s",
                name, key, ut_str.error(r));
            bytes = 0; // on any error behave as empty data
        } else {
            bytes = (int)cb;
        }
        fatal_if_not_zero(RegCloseKey(k));
    }
    return bytes;
}

static int32_t ut_config_load(const char* name,
        const char* key, void* data, int32_t bytes) {
    int32_t read = -1;
    HKEY k = null;
    errno_t r = ut_config_get_reg_key(name, &k);
    if (k != null) {
        DWORD type = REG_BINARY;
        DWORD cb = (DWORD)bytes;
        r = RegQueryValueExA(k, key, null, &type, (byte*)data, &cb);
        if (r == ERROR_MORE_DATA) {
            // returns -1 app.data_size() should be used
        } else if (r != 0) {
            if (r != ERROR_FILE_NOT_FOUND) {
                traceln("%s.RegQueryValueExA(\"%s\") failed %s",
                    name, key, ut_str.error(r));
            }
            read = 0; // on any error behave as empty data
        } else {
            read = (int)cb;
        }
        fatal_if_not_zero(RegCloseKey(k));
    }
    return read;
}

#ifdef UT_TESTS

static void ut_config_test(void) {
    const char* name = strrchr(ut_args.v[0], '\\');
    if (name == null) { name = strrchr(ut_args.v[0], '/'); }
    name = name != null ? name + 1 : ut_args.v[0];
    swear(name != null);
    const char* key = "test";
    const char data[] = "data";
    int32_t bytes = sizeof(data);
    swear(ut_config.save(name, key, data, bytes) == 0);
    char read[256];
    swear(ut_config.load(name, key, read, bytes) == bytes);
    int32_t size = ut_config.size(name, key);
    swear(size == bytes);
    swear(ut_config.remove(name, key) == 0);
    swear(ut_config.clean(name) == 0);
    if (ut_debug.verbosity.level > ut_debug.verbosity.quiet) { traceln("done"); }
}

#else

static void ut_config_test(void) { }

#endif

ut_config_if ut_config = {
    .save   = ut_config_save,
    .size   = ut_config_size,
    .load   = ut_config_load,
    .remove = ut_config_remove,
    .clean  = ut_config_clean,
    .test   = ut_config_test
};

// ________________________________ ut_debug.c ________________________________

static const char* ut_debug_abbreviate(const char* file) {
    const char* fn = strrchr(file, '\\');
    if (fn == null) { fn = strrchr(file, '/'); }
    return fn != null ? fn + 1 : file;
}

#ifdef WINDOWS

static void ut_debug_println_va(const char* file, int32_t line, const char* func,
        const char* format, va_list vl) {
    char prefix[2 * 1024];
    // full path is useful in MSVC debugger output pane (clickable)
    // for all other scenarios short filename without path is preferable:
    const char* name = IsDebuggerPresent() ? file : ut_files.basename(file);
    // snprintf() does not guarantee zero termination on truncation
    snprintf(prefix, countof(prefix) - 1, "%s(%d): %s", name, line, func);
    prefix[countof(prefix) - 1] = 0; // zero terminated
    char text[2 * 1024];
    if (format != null && !strequ(format, "")) {
        vsnprintf(text, countof(text) - 1, format, vl);
        text[countof(text) - 1] = 0;
    } else {
        text[0] = 0;
    }
    char output[4 * 1024];
    snprintf(output, countof(output) - 1, "%s %s", prefix, text);
    output[countof(output) - 2] = 0;
    // strip trailing \n which can be remnant of fprintf("...\n")
    int32_t n = (int32_t)strlen(output);
    while (n > 0 && (output[n - 1] == '\n' || output[n - 1] == '\r')) {
        output[n - 1] = 0;
        n--;
    }
    // For link.exe /Subsystem:Windows code stdout/stderr are often closed
    if (stderr != null && fileno(stderr) >= 0) {
        fprintf(stderr, "%s\n", output);
    }
    assert(n + 1 < countof(output));
    // OutputDebugString() needs \n
    output[n + 0] = '\n';
    output[n + 1] = 0;
    // SetConsoleCP(CP_UTF8) is not guaranteed to be called
    uint16_t wide[countof(output)];
    OutputDebugStringW(ut_str.utf8_utf16(wide, output));
}

#else // posix version:

static void ut_debug_vprintf(const char* file, int32_t line, const char* func,
        const char* format, va_list vl) {
    fprintf(stderr, "%s(%d): %s ", file, line, func);
    vfprintf(stderr, format, vl);
    fprintf(stderr, "\n");
}

#endif

static void ut_debug_perrno(const char* file, int32_t line,
    const char* func, int32_t err_no, const char* format, ...) {
    if (err_no != 0) {
        if (format != null && !strequ(format, "")) {
            va_list vl;
            va_start(vl, format);
            ut_debug.println_va(file, line, func, format, vl);
            va_end(vl);
        }
        ut_debug.println(file, line, func, "errno: %d %s", err_no, strerror(err_no));
    }
}

static void ut_debug_perror(const char* file, int32_t line,
    const char* func, int32_t error, const char* format, ...) {
    if (error != 0) {
        if (format != null && !strequ(format, "")) {
            va_list vl;
            va_start(vl, format);
            ut_debug.println_va(file, line, func, format, vl);
            va_end(vl);
        }
        ut_debug.println(file, line, func, "error: %s", ut_str.error(error));
    }
}

static void ut_debug_println(const char* file, int32_t line, const char* func,
        const char* format, ...) {
    va_list vl;
    va_start(vl, format);
    ut_debug.println_va(file, line, func, format, vl);
    va_end(vl);
}

static bool ut_debug_is_debugger_present(void) { return IsDebuggerPresent(); }

static void ut_debug_breakpoint(void) {
    if (ut_debug.is_debugger_present()) { DebugBreak(); }
}

static int32_t ut_debug_verbosity_from_string(const char* s) {
    const char* n = null;
    long v = strtol(s, &n, 10);
    if (striequ(s, "quiet")) {
        return ut_debug.verbosity.quiet;
    } else if (striequ(s, "info")) {
        return ut_debug.verbosity.info;
    } else if (striequ(s, "verbose")) {
        return ut_debug.verbosity.verbose;
    } else if (striequ(s, "debug")) {
        return ut_debug.verbosity.debug;
    } else if (striequ(s, "trace")) {
        return ut_debug.verbosity.trace;
    } else if (n > s && ut_debug.verbosity.quiet <= v &&
               v <= ut_debug.verbosity.trace) {
        return v;
    } else {
        fatal("invalid verbosity: %s", s);
        return ut_debug.verbosity.quiet;
    }
}

static void ut_debug_test(void) {
    #ifdef UT_TESTS
    // not clear what can be tested here
    if (ut_debug.verbosity.level > ut_debug.verbosity.quiet) { traceln("done"); }
    #endif
}

ut_debug_if ut_debug = {
    .verbosity = {
        .level   =  0,
        .quiet   =  0,
        .info    =  1,
        .verbose =  2,
        .debug   =  3,
        .trace   =  4,
    },
    .verbosity_from_string = ut_debug_verbosity_from_string,
    .println               = ut_debug_println,
    .println_va            = ut_debug_println_va,
    .perrno                = ut_debug_perrno,
    .perror                = ut_debug_perror,
    .is_debugger_present   = ut_debug_is_debugger_present,
    .breakpoint            = ut_debug_breakpoint,
    .test                  = ut_debug_test
};

// ________________________________ ut_files.c ________________________________

// TODO: test FILE_APPEND_DATA
// https://learn.microsoft.com/en-us/windows/win32/fileio/appending-one-file-to-another-file?redirectedfrom=MSDN

// are posix and Win32 seek in agreement?
static_assertion(SEEK_SET == FILE_BEGIN);
static_assertion(SEEK_CUR == FILE_CURRENT);
static_assertion(SEEK_END == FILE_END);

#ifndef O_SYNC
#define O_SYNC (0x10000)
#endif

static errno_t ut_files_open(ut_file_t* *file, const char* fn, int32_t f) {
    DWORD access = (f & ut_files.o_wr) ? GENERIC_WRITE :
                   (f & ut_files.o_rw) ? GENERIC_READ | GENERIC_WRITE :
                                      GENERIC_READ;
    access |= (f & ut_files.o_append) ? FILE_APPEND_DATA : 0;
    DWORD disposition =
        (f & ut_files.o_create) ? ((f & ut_files.o_excl)  ? CREATE_NEW :
                                (f & ut_files.o_trunc) ? CREATE_ALWAYS :
                                                      OPEN_ALWAYS) :
            (f & ut_files.o_trunc) ? TRUNCATE_EXISTING : OPEN_EXISTING;
    const DWORD share = FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE;
    DWORD attr = FILE_ATTRIBUTE_NORMAL;
    attr |= (f & O_SYNC) ? FILE_FLAG_WRITE_THROUGH : 0;
    *file = CreateFileA(fn, access, share, null, disposition, attr, null);
    return *file != INVALID_HANDLE_VALUE ? 0 : ut_runtime.err();
}

static bool ut_files_is_valid(ut_file_t* file) { // both null and ut_files.invalid
    return file != ut_files.invalid && file != null;
}

static errno_t ut_files_seek(ut_file_t* file, int64_t *position, int32_t method) {
    LARGE_INTEGER distance_to_move = { .QuadPart = *position };
    LARGE_INTEGER pointer = {0};
    errno_t r = b2e(SetFilePointerEx(file, distance_to_move, &pointer, method));
    if (r == 0) { *position = pointer.QuadPart; };
    return r;
}

static inline uint64_t ut_files_ft_to_us(FILETIME ft) { // us (microseconds)
    return (ft.dwLowDateTime | (((uint64_t)ft.dwHighDateTime) << 32)) / 10;
}

static int64_t ut_files_a2t(DWORD a) {
    int64_t type = 0;
    if (a & FILE_ATTRIBUTE_REPARSE_POINT) {
        type |= ut_files.type_symlink;
    }
    if (a & FILE_ATTRIBUTE_DIRECTORY) {
        type |= ut_files.type_folder;
    }
    if (a & FILE_ATTRIBUTE_DEVICE) {
        type |= ut_files.type_device;
    }
    return type;
}

#ifdef FILES_LINUX_PATH_BY_FD

static int get_final_path_name_by_fd(int fd, char *buffer, int32_t bytes) {
    swear(bytes >= 0);
    char fd_path[16 * 1024];
    // /proc/self/fd/* is a symbolic link
    snprintf(fd_path, sizeof(fd_path), "/proc/self/fd/%d", fd);
    size_t len = readlink(fd_path, buffer, bytes - 1);
    if (len != -1) { buffer[len] = 0x00; } // Null-terminate the result
    return len == -1 ? errno : 0;
}

#endif

static errno_t ut_files_stat(ut_file_t* file, ut_files_stat_t* s, bool follow_symlink) {
    errno_t r = 0;
    BY_HANDLE_FILE_INFORMATION fi;
    fatal_if_false(GetFileInformationByHandle(file, &fi));
    const bool symlink = (fi.dwFileAttributes & FILE_ATTRIBUTE_REPARSE_POINT) != 0;
    if (follow_symlink && symlink) {
        const DWORD flags = FILE_NAME_NORMALIZED | VOLUME_NAME_DOS;
        DWORD n = GetFinalPathNameByHandleA(file, null, 0, flags);
        if (n == 0) {
            r = GetLastError();
        } else {
            char* name = null;
            r = ut_heap.allocate(null, &name, n + 2, false);
            if (r == 0) {
                n = GetFinalPathNameByHandleA(file, name, n + 1, flags);
                if (n == 0) {
                    r = GetLastError();
                } else {
                    ut_file_t* f = ut_files.invalid;
                    r = ut_files.open(&f, name, ut_files.o_rd);
                    if (r == 0) { // keep following:
                        r = ut_files.stat(f, s, follow_symlink);
                        ut_files.close(f);
                    }
                }
                ut_heap.deallocate(null, name);
            }
        }
    } else {
        s->size = fi.nFileSizeLow | (((uint64_t)fi.nFileSizeHigh) << 32);
        s->created  = ut_files_ft_to_us(fi.ftCreationTime); // since epoch
        s->accessed = ut_files_ft_to_us(fi.ftLastAccessTime);
        s->updated  = ut_files_ft_to_us(fi.ftLastWriteTime);
        s->type = ut_files_a2t(fi.dwFileAttributes);
    }
    return r;
}

static errno_t ut_files_read(ut_file_t* file, void* data, int64_t bytes, int64_t *transferred) {
    errno_t r = 0;
    *transferred = 0;
    while (bytes > 0 && r == 0) {
        DWORD chunk_size = (DWORD)(bytes > UINT32_MAX ? UINT32_MAX : bytes);
        DWORD bytes_read = 0;
        r = b2e(ReadFile(file, data, chunk_size, &bytes_read, null));
        if (r == 0) {
            *transferred += bytes_read;
            bytes -= bytes_read;
            data = (uint8_t*)data + bytes_read;
        }
    }
    return r;
}

static errno_t ut_files_write(ut_file_t* file, const void* data, int64_t bytes, int64_t *transferred) {
    errno_t r = 0;
    *transferred = 0;
    while (bytes > 0 && r == 0) {
        DWORD chunk_size = (DWORD)(bytes > UINT32_MAX ? UINT32_MAX : bytes);
        DWORD bytes_read = 0;
        r = b2e(WriteFile(file, data, chunk_size, &bytes_read, null));
        if (r == 0) {
            *transferred += bytes_read;
            bytes -= bytes_read;
            data = (uint8_t*)data + bytes_read;
        }
    }
    return r;
}

static errno_t ut_files_flush(ut_file_t* file) {
    return b2e(FlushFileBuffers(file));
}

static void ut_files_close(ut_file_t* file) {
    fatal_if_false(CloseHandle(file));
}

static errno_t ut_files_write_fully(const char* filename, const void* data,
                                 int64_t bytes, int64_t *transferred) {
    if (transferred != null) { *transferred = 0; }
    errno_t r = 0;
    const DWORD access = GENERIC_WRITE;
    const DWORD share = FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE;
    const DWORD flags = FILE_ATTRIBUTE_NORMAL | FILE_FLAG_WRITE_THROUGH;
    HANDLE file = CreateFileA(filename, access, share, null, CREATE_ALWAYS,
                              flags, null);
    if (file == INVALID_HANDLE_VALUE) {
        r = ut_runtime.err();
    } else {
        int64_t written = 0;
        const uint8_t* p = (const uint8_t*)data;
        while (r == 0 && bytes > 0) {
            uint64_t write = bytes >= UINT32_MAX ?
                (UINT32_MAX) - 0xFFFF : (uint64_t)bytes;
            assert(0 < write && write < UINT32_MAX);
            DWORD chunk = 0;
            r = b2e(WriteFile(file, p, (DWORD)write, &chunk, null));
            written += chunk;
            bytes -= chunk;
        }
        if (transferred != null) { *transferred = written; }
        errno_t rc = b2e(CloseHandle(file));
        if (r == 0) { r = rc; }
    }
    return r;
}

static errno_t ut_files_unlink(const char* pathname) {
    if (ut_files.is_folder(pathname)) {
        return b2e(RemoveDirectoryA(pathname));
    } else {
        return b2e(DeleteFileA(pathname));
    }
}

static errno_t ut_files_create_tmp(char* fn, int32_t count) {
    // create temporary file (not folder!) see folders_test() about racing
    swear(fn != null && count > 0);
    const char* tmp = ut_files.tmp();
    errno_t r = 0;
    if (count < (int)strlen(tmp) + 8) {
        r = ERROR_BUFFER_OVERFLOW;
    } else {
        assert(count > (int)strlen(tmp) + 8);
        // If GetTempFileNameA() succeeds, the return value is the length,
        // in chars, of the string copied to lpBuffer, not including the
        // terminating null character.If the function fails,
        // the return value is zero.
        if (count > (int)strlen(tmp) + 8) {
            char prefix[4] = {0};
            r = GetTempFileNameA(tmp, prefix, 0, fn) == 0 ? ut_runtime.err() : 0;
            if (r == 0) {
                assert(ut_files.exists(fn) && !ut_files.is_folder(fn));
            } else {
                traceln("GetTempFileNameA() failed %s", ut_str.error(r));
            }
        } else {
            r = ERROR_BUFFER_OVERFLOW;
        }
    }
    return r;
}

#pragma push_macro("files_acl_args")
#pragma push_macro("files_get_acl")
#pragma push_macro("files_set_acl")

#define ut_files_acl_args(acl) DACL_SECURITY_INFORMATION, null, null, acl, null

#define ut_files_get_acl(obj, type, acl, sd)                       \
    (type == SE_FILE_OBJECT ? GetNamedSecurityInfoA((char*)obj, \
             SE_FILE_OBJECT, ut_files_acl_args(acl), &sd) :        \
    (type == SE_KERNEL_OBJECT) ? GetSecurityInfo((HANDLE)obj,   \
             SE_KERNEL_OBJECT, ut_files_acl_args(acl), &sd) :      \
    ERROR_INVALID_PARAMETER)

#define ut_files_set_acl(obj, type, acl)                           \
    (type == SE_FILE_OBJECT ? SetNamedSecurityInfoA((char*)obj, \
             SE_FILE_OBJECT, ut_files_acl_args(acl)) :             \
    (type == SE_KERNEL_OBJECT) ? SetSecurityInfo((HANDLE)obj,   \
             SE_KERNEL_OBJECT, ut_files_acl_args(acl)) :           \
    ERROR_INVALID_PARAMETER)

static errno_t ut_files_acl_add_ace(ACL* acl, SID* sid, uint32_t mask,
                                 ACL** free_me, byte flags) {
    ACL_SIZE_INFORMATION info;
    ACL* bigger = null;
    uint32_t bytes_needed = sizeof(ACCESS_ALLOWED_ACE) + GetLengthSid(sid)
                          - sizeof(DWORD);
    errno_t r = b2e(GetAclInformation(acl, &info, sizeof(ACL_SIZE_INFORMATION),
        AclSizeInformation));
    if (r == 0 && info.AclBytesFree < bytes_needed) {
        const int64_t bytes = info.AclBytesInUse + bytes_needed;
        r = ut_heap.allocate(null, &bigger, bytes, true);
        if (r == 0) {
            r = b2e(InitializeAcl((ACL*)bigger,
                    info.AclBytesInUse + bytes_needed, ACL_REVISION));
        }
    }
    if (r == 0 && bigger != null) {
        for (int32_t i = 0; i < (int)info.AceCount; i++) {
            ACCESS_ALLOWED_ACE* ace;
            r = b2e(GetAce(acl, i, (void**)&ace));
            if (r != 0) { break; }
            r = b2e(AddAce(bigger, ACL_REVISION, MAXDWORD, ace,
                           ace->Header.AceSize));
            if (r != 0) { break; }
        }
    }
    if (r == 0) {
        ACCESS_ALLOWED_ACE* ace = null;
        r = ut_heap.allocate(null, &ace, bytes_needed, true);
        if (r == 0) {
            ace->Header.AceFlags = flags;
            ace->Header.AceType = ACCESS_ALLOWED_ACE_TYPE;
            ace->Header.AceSize = (WORD)bytes_needed;
            ace->Mask = mask;
            ace->SidStart = sizeof(ACCESS_ALLOWED_ACE);
            memcpy(&ace->SidStart, sid, GetLengthSid(sid));
            r = b2e(AddAce(bigger != null ? bigger : acl, ACL_REVISION, MAXDWORD,
                           ace, bytes_needed));
            ut_heap.deallocate(null, ace);
        }
    }
    *free_me = bigger;
    return r;
}

static errno_t ut_files_lookup_sid(ACCESS_ALLOWED_ACE* ace) {
    // handy for debugging
    SID* sid = (SID*)&ace->SidStart;
    DWORD l1 = 128, l2 = 128;
    char account[128];
    char group[128];
    SID_NAME_USE use;
    errno_t r = b2e(LookupAccountSidA(null, sid, account,
                                     &l1, group, &l2, &use));
    if (r == 0) {
        traceln("%s/%s: type: %d, mask: 0x%X, flags:%d",
                group, account,
                ace->Header.AceType, ace->Mask, ace->Header.AceFlags);
    } else {
        traceln("LookupAccountSidA() failed %s", ut_str.error(r));
    }
    return r;
}

static errno_t ut_files_add_acl_ace(const void* obj, int32_t obj_type,
                                 int32_t sid_type, uint32_t mask) {
    uint8_t stack[SECURITY_MAX_SID_SIZE];
    DWORD n = countof(stack);
    SID* sid = (SID*)stack;
    errno_t r = b2e(CreateWellKnownSid((WELL_KNOWN_SID_TYPE)sid_type,
                                       null, sid, &n));
    if (r != 0) {
        return ERROR_INVALID_PARAMETER;
    }
    ACL* acl = null;
    void* sd = null;
    r = ut_files_get_acl(obj, obj_type, &acl, sd);
    if (r == 0) {
        ACCESS_ALLOWED_ACE* found = null;
        for (int32_t i = 0; i < acl->AceCount; i++) {
            ACCESS_ALLOWED_ACE* ace;
            r = b2e(GetAce(acl, i, (void**)&ace));
            if (r != 0) { break; }
            if (EqualSid((SID*)&ace->SidStart, sid)) {
                if (ace->Header.AceType == ACCESS_ALLOWED_ACE_TYPE &&
                   (ace->Header.AceFlags & INHERITED_ACE) == 0) {
                    found = ace;
                } else if (ace->Header.AceType !=
                           ACCESS_ALLOWED_ACE_TYPE) {
                    traceln("%d ACE_TYPE is not supported.",
                             ace->Header.AceType);
                    r = ERROR_INVALID_PARAMETER;
                }
                break;
            }
        }
        if (r == 0 && found) {
            if ((found->Mask & mask) != mask) {
//              traceln("updating existing ace");
                found->Mask |= mask;
                r = ut_files_set_acl(obj, obj_type, acl);
            } else {
//              traceln("desired access is already allowed by ace");
            }
        } else if (r == 0) {
//          traceln("inserting new ace");
            ACL* new_acl = null;
            byte flags = obj_type == SE_FILE_OBJECT ?
                CONTAINER_INHERIT_ACE | OBJECT_INHERIT_ACE : 0;
            r = ut_files_acl_add_ace(acl, sid, mask, &new_acl, flags);
            if (r == 0) {
                r = ut_files_set_acl(obj, obj_type, (new_acl != null ? new_acl : acl));
            }
            if (new_acl != null) { ut_heap.deallocate(null, new_acl); }
        }
    }
    if (sd != null) { LocalFree(sd); }
    return r;
}

#pragma pop_macro("files_set_acl")
#pragma pop_macro("files_get_acl")
#pragma pop_macro("files_acl_args")

static errno_t ut_files_chmod777(const char* pathname) {
    SID_IDENTIFIER_AUTHORITY SIDAuthWorld = SECURITY_WORLD_SID_AUTHORITY;
    PSID everyone = null; // Create a well-known SID for the Everyone group.
    fatal_if_false(AllocateAndInitializeSid(&SIDAuthWorld, 1,
             SECURITY_WORLD_RID, 0, 0, 0, 0, 0, 0, 0, &everyone));
    EXPLICIT_ACCESSA ea[1] = {{0}};
    // Initialize an EXPLICIT_ACCESS structure for an ACE.
    ea[0].grfAccessPermissions = 0xFFFFFFFF;
    ea[0].grfAccessMode  = GRANT_ACCESS; // The ACE will allow everyone all access.
    ea[0].grfInheritance = SUB_CONTAINERS_AND_OBJECTS_INHERIT;
    ea[0].Trustee.TrusteeForm = TRUSTEE_IS_SID;
    ea[0].Trustee.TrusteeType = TRUSTEE_IS_WELL_KNOWN_GROUP;
    ea[0].Trustee.ptstrName  = (LPSTR)everyone;
    // Create a new ACL that contains the new ACEs.
    ACL* acl = null;
    fatal_if_not_zero(SetEntriesInAclA(1, ea, null, &acl));
    // Initialize a security descriptor.
    uint8_t stack[SECURITY_DESCRIPTOR_MIN_LENGTH];
    SECURITY_DESCRIPTOR* sd = (SECURITY_DESCRIPTOR*)stack;
    fatal_if_false(InitializeSecurityDescriptor(sd, SECURITY_DESCRIPTOR_REVISION));
    // Add the ACL to the security descriptor.
    fatal_if_false(SetSecurityDescriptorDacl(sd, /* DaclPresent flag: */ true,
                                   acl, /* not a default DACL: */  false));
    // Change the security attributes
    errno_t r = b2e(SetFileSecurityA(pathname, DACL_SECURITY_INFORMATION, sd));
    if (r != 0) {
        traceln("chmod777(%s) failed %s", pathname, ut_str.error(r));
    }
    if (everyone != null) { FreeSid(everyone); }
    if (acl != null) { LocalFree(acl); }
    return r;
}

// https://docs.microsoft.com/en-us/windows/win32/api/fileapi/nf-fileapi-createdirectorya
// "If lpSecurityAttributes is null, the directory gets a default security
//  descriptor. The ACLs in the default security descriptor for a directory
//  are inherited from its parent directory."

static errno_t ut_files_mkdirs(const char* dir) {
    const int32_t n = (int)strlen(dir) + 1;
    char* s = null;
    errno_t r = ut_heap.allocate(null, &s, n, true);
    const char* next = strchr(dir, '\\');
    if (next == null) { next = strchr(dir, '/'); }
    while (r == 0 && next != null) {
        if (next > dir && *(next - 1) != ':') {
            memcpy(s, dir, next - dir);
            r = b2e(CreateDirectoryA(s, null));
            if (r == ERROR_ALREADY_EXISTS) { r = 0; }
        }
        if (r == 0) {
            const char* prev = ++next;
            next = strchr(prev, '\\');
            if (next == null) { next = strchr(prev, '/'); }
        }
    }
    if (r == 0) {
        r = b2e(CreateDirectoryA(dir, null));
    }
    ut_heap.deallocate(null, s);
    return r == ERROR_ALREADY_EXISTS ? 0 : r;
}

#pragma push_macro("files_realloc_path")
#pragma push_macro("files_append_name")

#define ut_files_realloc_path(r, pn, pnc, fn, name) do {                   \
    const int32_t bytes = (int32_t)(strlen(fn) + strlen(name) + 3);     \
    if (bytes > pnc) {                                                  \
        r = ut_heap.reallocate(null, &pn, bytes, false);                   \
        if (r != 0) {                                                   \
            pnc = bytes;                                                \
        } else {                                                        \
            ut_heap.deallocate(null, pn);                                  \
            pn = null;                                                  \
        }                                                               \
    }                                                                   \
} while (0)

#define ut_files_append_name(pn, pnc, fn, name) do {      \
    if (strequ(fn, "\\") || strequ(fn, "/")) {         \
        ut_str.format(pn, pnc, "\\%s", name);            \
    } else {                                           \
        ut_str.format(pn, pnc, "%.*s\\%s", k, fn, name); \
    }                                                  \
} while (0)

static errno_t ut_files_rmdirs(const char* fn) {
    ut_files_stat_t st;
    ut_folder_t folder;
    errno_t r = ut_files.opendir(&folder, fn);
    if (r == 0) {
        int32_t k = (int32_t)strlen(fn);
        // remove trailing backslash (except if it is root: "/" or "\\")
        if (k > 1 && (fn[k - 1] == '/' || fn[k - 1] == '\\')) {
            k--;
        }
        int32_t pnc = 64 * 1024; // pathname "pn" capacity in bytes
        char* pn = null;
        r = ut_heap.allocate(null, &pn, pnc, false);
        while (r == 0) {
            // recurse into sub folders and remove them first
            // do NOT follow symlinks - it could be disastrous
            const char* name = ut_files.readdir(&folder, &st);
            if (name == null) { break; }
            if (!strequ(name, ".") && !strequ(name, "..") &&
                (st.type & ut_files.type_symlink) == 0 &&
                (st.type & ut_files.type_folder) != 0) {
                ut_files_realloc_path(r, pn, pnc, fn, name);
                if (r == 0) {
                    ut_files_append_name(pn, pnc, fn, name);
                    r = ut_files.rmdirs(pn);
                }
            }
        }
        ut_files.closedir(&folder);
        r = ut_files.opendir(&folder, fn);
        while (r == 0) {
            const char* name = ut_files.readdir(&folder, &st);
            if (name == null) { break; }
            // symlinks are already removed as normal files
            if (!strequ(name, ".") && !strequ(name, "..") &&
                (st.type & ut_files.type_folder) == 0) {
                ut_files_realloc_path(r, pn, pnc, fn, name);
                if (r == 0) {
                    ut_files_append_name(pn, pnc, fn, name);
                    r = ut_files.unlink(pn);
                    if (r != 0) {
                        traceln("remove(%s) failed %s", pn, ut_str.error(r));
                    }
                }
            }
        }
        ut_heap.deallocate(null, pn);
        ut_files.closedir(&folder);
    }
    if (r == 0) { r = ut_files.unlink(fn); }
    return r;
}

#pragma pop_macro("files_append_name")
#pragma pop_macro("files_realloc_path")

static bool ut_files_exists(const char* path) {
    return PathFileExistsA(path);
}

static bool ut_files_is_folder(const char* path) {
    return PathIsDirectoryA(path);
}

static bool ut_files_is_symlink(const char* filename) {
    DWORD attributes = GetFileAttributesA(filename);
    return attributes != INVALID_FILE_ATTRIBUTES &&
          (attributes & FILE_ATTRIBUTE_REPARSE_POINT) != 0;
}

const char* ut_files_basename(const char* pathname) {
    const char* bn = strrchr(pathname, '\\');
    if (bn == null) { bn = strrchr(pathname, '/'); }
    return bn != null ? bn + 1 : pathname;
}

static errno_t ut_files_copy(const char* s, const char* d) {
    return b2e(CopyFileA(s, d, false));
}

static errno_t ut_files_move(const char* s, const char* d) {
    static const DWORD flags =
        MOVEFILE_REPLACE_EXISTING |
        MOVEFILE_COPY_ALLOWED |
        MOVEFILE_WRITE_THROUGH;
    return b2e(MoveFileExA(s, d, flags));
}

static errno_t ut_files_link(const char* from, const char* to) {
    // note reverse order of parameters:
    return b2e(CreateHardLinkA(to, from, null));
}

static errno_t ut_files_symlink(const char* from, const char* to) {
    // The correct order of parameters for CreateSymbolicLinkA is:
    // CreateSymbolicLinkA(symlink_to_create, existing_file, flags);
    DWORD flags = ut_files.is_folder(from) ? SYMBOLIC_LINK_FLAG_DIRECTORY : 0;
    return b2e(CreateSymbolicLinkA(to, from, flags));
}

static const char* ut_files_bin(void) {
    static char program_files[ut_files_max_path];
    if (program_files[0] == 0) {
        wchar_t* program_files_w = null;
        fatal_if(SHGetKnownFolderPath(&FOLDERID_ProgramFilesX64, 0,
            null, &program_files_w) != 0);
        int32_t len = (int32_t)wcslen(program_files_w);
        assert(len < countof(program_files));
        fatal_if(len >= countof(program_files), "len=%d", len);
        for (int32_t i = 0; i <= len; i++) { // including zero terminator
            assert(program_files_w[i] < 128); // pure ascii
            program_files[i] = (char)program_files_w[i];
        }
    }
    return program_files;
}

static const char* ut_files_tmp(void) {
    static char tmp[ut_files_max_path];
    if (tmp[0] == 0) {
        // If GetTempPathA() succeeds, the return value is the length,
        // in chars, of the string copied to lpBuffer, not including
        // the terminating null character. If the function fails, the
        // return value is zero.
        errno_t r = GetTempPathA(countof(tmp), tmp) == 0 ? ut_runtime.err() : 0;
        fatal_if(r != 0, "GetTempPathA() failed %s", ut_str.error(r));
    }
    return tmp;
}

static errno_t ut_files_cwd(char* fn, int32_t count) {
    swear(count > 1);
    DWORD bytes = count - 1;
    errno_t r = b2e(GetCurrentDirectoryA(bytes, fn));
    fn[count - 1] = 0; // always
    return r;
}

static errno_t ut_files_chdir(const char* fn) {
    return b2e(SetCurrentDirectoryA(fn));
}

typedef struct ut_files_dir_s {
    HANDLE handle;
    WIN32_FIND_DATAA find; // On Win64: 320 bytes
} ut_files_dir_t;

static_assertion(sizeof(ut_files_dir_t) <= sizeof(ut_folder_t));

errno_t ut_files_opendir(ut_folder_t* folder, const char* folder_name) {
    ut_files_dir_t* d = (ut_files_dir_t*)folder;
    int32_t n = (int32_t)strlen(folder_name);
    char* fn = null;
    errno_t r = ut_heap.allocate(null, &fn, n + 3, false); // extra room for "\*" suffix
    if (r == 0) {
        snprintf(fn, n + 3, "%s\\*", folder_name);
        fn[n + 2] = 0;
        d->handle = FindFirstFileA(fn, &d->find);
        if (d->handle == INVALID_HANDLE_VALUE) { r = GetLastError(); }
        ut_heap.deallocate(null, fn);
    }
    return r;
}

static uint64_t ut_files_ft2us(FILETIME* ft) { // 100ns units to microseconds:
    return (((uint64_t)ft->dwHighDateTime) << 32 | ft->dwLowDateTime) / 10;
}

const char* ut_files_readdir(ut_folder_t* folder, ut_files_stat_t* s) {
    const char* fn = null;
    ut_files_dir_t* d = (ut_files_dir_t*)folder;
    if (FindNextFileA(d->handle, &d->find)) {
        fn = d->find.cFileName;
        // Ensure zero termination
        d->find.cFileName[countof(d->find.cFileName) - 1] = 0x00;
        if (s != null) {
            s->accessed = ut_files_ft2us(&d->find.ftLastAccessTime);
            s->created = ut_files_ft2us(&d->find.ftCreationTime);
            s->updated = ut_files_ft2us(&d->find.ftLastWriteTime);
            s->type = ut_files_a2t(d->find.dwFileAttributes);
            s->size = (((uint64_t)d->find.nFileSizeHigh) << 32) |
                                  d->find.nFileSizeLow;
        }
    }
    return fn;
}

void ut_files_closedir(ut_folder_t* folder) {
    ut_files_dir_t* d = (ut_files_dir_t*)folder;
    fatal_if_false(FindClose(d->handle));
}

#pragma push_macro("files_test_failed")

#ifdef UT_TESTS

#define ut_files_test_failed " failed %s", ut_str.error(ut_runtime.err())

#pragma push_macro("verbose") // --verbosity trace

#define verbose(...) do {                                 \
    if (ut_debug.verbosity.level >= ut_debug.verbosity.trace) { \
        traceln(__VA_ARGS__);                             \
    }                                                     \
} while (0)

static void folders_dump_time(const char* label, uint64_t us) {
    int32_t year = 0;
    int32_t month = 0;
    int32_t day = 0;
    int32_t hh = 0;
    int32_t mm = 0;
    int32_t ss = 0;
    int32_t ms = 0;
    int32_t mc = 0;
    ut_clock.local(us, &year, &month, &day, &hh, &mm, &ss, &ms, &mc);
    traceln("%-7s: %04d-%02d-%02d %02d:%02d:%02d.%03d:%03d",
            label, year, month, day, hh, mm, ss, ms, mc);
}

static void folders_test(void) {
    uint64_t now = ut_clock.microseconds(); // microseconds since epoch
    uint64_t before = now - 1 * ut_clock.usec_in_sec; // one second earlier
    uint64_t after  = now + 2 * ut_clock.usec_in_sec; // two seconds later
    int32_t year = 0;
    int32_t month = 0;
    int32_t day = 0;
    int32_t hh = 0;
    int32_t mm = 0;
    int32_t ss = 0;
    int32_t ms = 0;
    int32_t mc = 0;
    ut_clock.local(now, &year, &month, &day, &hh, &mm, &ss, &ms, &mc);
    verbose("now: %04d-%02d-%02d %02d:%02d:%02d.%03d:%03d",
             year, month, day, hh, mm, ss, ms, mc);
    // Test cwd, setcwd
    const char* tmp = ut_files.tmp();
    char cwd[256] = {0};
    fatal_if(ut_files.cwd(cwd, sizeof(cwd)) != 0, "ut_files.cwd() failed");
    fatal_if(ut_files.chdir(tmp) != 0, "ut_files.chdir(\"%s\") failed %s",
                tmp, ut_str.error(ut_runtime.err()));
    // there is no racing free way to create temporary folder
    // without having a temporary file for the duration of folder usage:
    char tmp_file[ut_files_max_path]; // create_tmp() is thread safe race free:
    errno_t r = ut_files.create_tmp(tmp_file, countof(tmp_file));
    fatal_if(r != 0, "ut_files.create_tmp() failed %s", ut_str.error(r));
    char tmp_dir[ut_files_max_path];
    strprintf(tmp_dir, "%s.dir", tmp_file);
    r = ut_files.mkdirs(tmp_dir);
    fatal_if(r != 0, "ut_files.mkdirs(%s) failed %s", tmp_dir, ut_str.error(r));
    verbose("%s", tmp_dir);
    ut_folder_t folder;
    char pn[ut_files_max_path] = {0};
    strprintf(pn, "%s/file", tmp_dir);
    // cannot test symlinks because they are only
    // available to Administrators and in Developer mode
//  char sym[ut_files_max_path] = {0};
    char hard[ut_files_max_path] = {0};
    char sub[ut_files_max_path] = {0};
    strprintf(hard, "%s/hard", tmp_dir);
    strprintf(sub, "%s/subd", tmp_dir);
    const char* content = "content";
    int64_t transferred = 0;
    r = ut_files.write_fully(pn, content, strlen(content), &transferred);
    fatal_if(r != 0, "ut_files.write_fully(\"%s\") failed %s", pn, ut_str.error(r));
    swear(transferred == (int64_t)strlen(content));
    r = ut_files.link(pn, hard);
    fatal_if(r != 0, "ut_files.link(\"%s\", \"%s\") failed %s",
                      pn, hard, ut_str.error(r));
    r = ut_files.mkdirs(sub);
    fatal_if(r != 0, "ut_files.mkdirs(\"%s\") failed %s", sub, ut_str.error(r));
    r = ut_files.opendir(&folder, tmp_dir);
    fatal_if(r != 0, "ut_files.opendir(\"%s\") failed %s", tmp_dir, ut_str.error(r));
    for (;;) {
        ut_files_stat_t st = {0};
        const char* name = ut_files.readdir(&folder, &st);
        if (name == null) { break; }
        uint64_t at = st.accessed;
        uint64_t ct = st.created;
        uint64_t ut = st.updated;
        swear(ct <= at && ct <= ut);
        ut_clock.local(ct, &year, &month, &day, &hh, &mm, &ss, &ms, &mc);
        bool is_folder = st.type & ut_files.type_folder;
        bool is_symlink = st.type & ut_files.type_symlink;
        int64_t bytes = st.size;
        verbose("%s: %04d-%02d-%02d %02d:%02d:%02d.%03d:%03d %lld bytes %s%s",
                name, year, month, day, hh, mm, ss, ms, mc,
                bytes, is_folder ? "[folder]" : "", is_symlink ? "[symlink]" : "");
        if (strequ(name, "file") || strequ(name, "hard")) {
            swear(bytes == (int64_t)strlen(content),
                    "size of \"%s\": %lld is incorrect expected: %d",
                    name, bytes, transferred);
        }
        if (strequ(name, ".") || strequ(name, "..")) {
            swear(is_folder, "\"%s\" is_folder: %d", name, is_folder);
        } else {
            swear(strequ(name, "subd") == is_folder,
                  "\"%s\" is_folder: %d", name, is_folder);
            // empirically timestamps are imprecise on NTFS
            swear(at >= before, "access: %lld  >= %lld", at, before);
            if (ct < before || ut < before || at >= after || ct >= after || ut >= after) {
                traceln("file: %s", name);
                folders_dump_time("before", before);
                folders_dump_time("create", ct);
                folders_dump_time("update", ut);
                folders_dump_time("access", at);
            }
            swear(ct >= before, "create: %lld  >= %lld", ct, before);
            swear(ut >= before, "update: %lld  >= %lld", ut, before);
            // and no later than 2 seconds since folders_test()
            swear(at < after, "access: %lld  < %lld", at, after);
            swear(ct < after, "create: %lld  < %lld", ct, after);
            swear(at < after, "update: %lld  < %lld", ut, after);
        }
    }
    ut_files.closedir(&folder);
    r = ut_files.rmdirs(tmp_dir);
    fatal_if(r != 0, "ut_files.rmdirs(\"%s\") failed %s",
                     tmp_dir, ut_str.error(r));
    r = ut_files.unlink(tmp_file);
    fatal_if(r != 0, "ut_files.unlink(\"%s\") failed %s",
                     tmp_file, ut_str.error(r));
    fatal_if(ut_files.chdir(cwd) != 0, "ut_files.chdir(\"%s\") failed %s",
             cwd, ut_str.error(ut_runtime.err()));
    if (ut_debug.verbosity.level > ut_debug.verbosity.quiet) { traceln("done"); }
}

#pragma pop_macro("verbose")

static void ut_files_test_append_thread(void* p) {
    ut_file_t* f = (ut_file_t*)p;
    uint8_t data[256];
    for (int i = 0; i < 256; i++) { data[i] = (uint8_t)i; }
    int64_t transferred = 0;
    fatal_if(ut_files.write(f, data, countof(data), &transferred) != 0 ||
             transferred != countof(data), "ut_files.write()" ut_files_test_failed);
}

static void ut_files_test(void) {
    folders_test();
    uint64_t now = ut_clock.microseconds(); // epoch time
    char tf[256]; // temporary file
    fatal_if(ut_files.create_tmp(tf, countof(tf)) != 0,
            "ut_files.create_tmp()" ut_files_test_failed);
    uint8_t data[256];
    int64_t transferred = 0;
    for (int i = 0; i < 256; i++) { data[i] = (uint8_t)i; }
    {
        ut_file_t* f = ut_files.invalid;
        fatal_if(ut_files.open(&f, tf,
                 ut_files.o_wr | ut_files.o_create | ut_files.o_trunc) != 0 ||
                !ut_files.is_valid(f), "ut_files.open()" ut_files_test_failed);
        fatal_if(ut_files.write_fully(tf, data, countof(data), &transferred) != 0 ||
                 transferred != countof(data),
                "ut_files.write_fully()" ut_files_test_failed);
        fatal_if(ut_files.open(&f, tf, ut_files.o_rd) != 0 ||
                !ut_files.is_valid(f), "ut_files.open()" ut_files_test_failed);
        for (int32_t i = 0; i < 256; i++) {
            for (int32_t j = 1; j < 256 - i; j++) {
                uint8_t test[countof(data)] = {0};
                int64_t position = i;
                fatal_if(ut_files.seek(f, &position, ut_files.seek_set) != 0 ||
                         position != i,
                        "ut_files.seek(position: %lld) failed %s",
                         position, ut_str.error(ut_runtime.err()));
                fatal_if(ut_files.read(f, test, j, &transferred) != 0 ||
                         transferred != j,
                        "ut_files.read() transferred: %lld failed %s",
                        transferred, ut_str.error(ut_runtime.err()));
                for (int32_t k = 0; k < j; k++) {
                    swear(test[k] == data[i + k],
                         "Data mismatch at position: %d, length %d"
                         "test[%d]: 0x%02X != data[%d + %d]: 0x%02X ",
                          i, j,
                          k, test[k], i, k, data[i + k]);
                }
            }
        }
        swear((ut_files.o_rd | ut_files.o_wr) != ut_files.o_rw);
        fatal_if(ut_files.open(&f, tf, ut_files.o_rw) != 0 || !ut_files.is_valid(f),
                "ut_files.open()" ut_files_test_failed);
        for (int32_t i = 0; i < 256; i++) {
            uint8_t val = ~data[i];
            int64_t pos = i;
            fatal_if(ut_files.seek(f, &pos, ut_files.seek_set) != 0 || pos != i,
                    "ut_files.seek() failed %s", ut_runtime.err());
            fatal_if(ut_files.write(f, &val, 1, &transferred) != 0 ||
                     transferred != 1, "ut_files.write()" ut_files_test_failed);
            pos = i;
            fatal_if(ut_files.seek(f, &pos, ut_files.seek_set) != 0 || pos != i,
                    "ut_files.seek(pos: %lld i: %d) failed %s", pos, i, ut_runtime.err());
            uint8_t read_val = 0;
            fatal_if(ut_files.read(f, &read_val, 1, &transferred) != 0 ||
                     transferred != 1, "ut_files.read()" ut_files_test_failed);
            swear(read_val == val, "Data mismatch at position %d", i);
        }
        ut_files_stat_t s = {0};
        ut_files.stat(f, &s, false);
        uint64_t before = now - 1 * ut_clock.usec_in_sec; // one second before now
        uint64_t after  = now + 2 * ut_clock.usec_in_sec; // two seconds after
        swear(before <= s.created  && s.created  <= after,
             "before: %lld created: %lld after: %lld", before, s.created, after);
        swear(before <= s.accessed && s.accessed <= after,
             "before: %lld created: %lld accessed: %lld", before, s.accessed, after);
        swear(before <= s.updated  && s.updated  <= after,
             "before: %lld created: %lld updated: %lld", before, s.updated, after);
        ut_files.close(f);
        fatal_if(ut_files.open(&f, tf, ut_files.o_wr | ut_files.o_create | ut_files.o_trunc) != 0 ||
                !ut_files.is_valid(f), "ut_files.open()" ut_files_test_failed);
        ut_files.stat(f, &s, false);
        swear(s.size == 0, "File is not empty after truncation. .size: %lld", s.size);
        ut_files.close(f);
    }
    {  // Append test with threads
        ut_file_t* f = ut_files.invalid;
        fatal_if(ut_files.open(&f, tf, ut_files.o_rw | ut_files.o_append) != 0 ||
                !ut_files.is_valid(f), "ut_files.open()" ut_files_test_failed);
        thread_t thread1 = ut_thread.start(ut_files_test_append_thread, f);
        thread_t thread2 = ut_thread.start(ut_files_test_append_thread, f);
        ut_thread.join(thread1, -1);
        ut_thread.join(thread2, -1);
        ut_files.close(f);
    }
    {   // write_fully, exists, is_folder, mkdirs, rmdirs, create_tmp, chmod777
        fatal_if(ut_files.write_fully(tf, data, countof(data), &transferred) != 0 ||
                 transferred != countof(data),
                "ut_files.write_fully() failed %s", ut_runtime.err());
        fatal_if(!ut_files.exists(tf), "file \"%s\" does not exist", tf);
        fatal_if(ut_files.is_folder(tf), "%s is a folder", tf);
        fatal_if(ut_files.chmod777(tf) != 0, "ut_files.chmod777(\"%s\") failed %s",
                 tf, ut_str.error(ut_runtime.err()));
        char folder[256] = {0};
        strprintf(folder, "%s.folder\\subfolder", tf);
        fatal_if(ut_files.mkdirs(folder) != 0, "ut_files.mkdirs(\"%s\") failed %s",
            folder, ut_str.error(ut_runtime.err()));
        fatal_if(!ut_files.is_folder(folder), "\"%s\" is not a folder", folder);
        fatal_if(ut_files.chmod777(folder) != 0, "ut_files.chmod777(\"%s\") failed %s",
                 folder, ut_str.error(ut_runtime.err()));
        fatal_if(ut_files.rmdirs(folder) != 0, "ut_files.rmdirs(\"%s\") failed %s",
                 folder, ut_str.error(ut_runtime.err()));
        fatal_if(ut_files.exists(folder), "folder \"%s\" still exists", folder);
    }
    {   // getcwd, chdir
        const char* tmp = ut_files.tmp();
        char cwd[256] = {0};
        fatal_if(ut_files.cwd(cwd, sizeof(cwd)) != 0, "ut_files.cwd() failed");
        fatal_if(ut_files.chdir(tmp) != 0, "ut_files.chdir(\"%s\") failed %s",
                 tmp, ut_str.error(ut_runtime.err()));
        // symlink
        if (ut_processes.is_elevated()) {
            char sym_link[ut_files_max_path];
            strprintf(sym_link, "%s.sym_link", tf);
            fatal_if(ut_files.symlink(tf, sym_link) != 0,
                "ut_files.symlink(\"%s\", \"%s\") failed %s",
                tf, sym_link, ut_str.error(ut_runtime.err()));
            fatal_if(!ut_files.is_symlink(sym_link), "\"%s\" is not a sym_link", sym_link);
            fatal_if(ut_files.unlink(sym_link) != 0, "ut_files.unlink(\"%s\") failed %s",
                    sym_link, ut_str.error(ut_runtime.err()));
        } else {
            traceln("Skipping ut_files.symlink test: process is not elevated");
        }
        // hard link
        char hard_link[ut_files_max_path];
        strprintf(hard_link, "%s.hard_link", tf);
        fatal_if(ut_files.link(tf, hard_link) != 0,
            "ut_files.link(\"%s\", \"%s\") failed %s",
            tf, hard_link, ut_str.error(ut_runtime.err()));
        fatal_if(!ut_files.exists(hard_link), "\"%s\" does not exist", hard_link);
        fatal_if(ut_files.unlink(hard_link) != 0, "ut_files.unlink(\"%s\") failed %s",
                 hard_link, ut_str.error(ut_runtime.err()));
        fatal_if(ut_files.exists(hard_link), "\"%s\" still exists", hard_link);
        // copy, move:
        fatal_if(ut_files.copy(tf, "copied_file") != 0,
            "ut_files.copy(\"%s\", 'copied_file') failed %s",
            tf, ut_str.error(ut_runtime.err()));
        fatal_if(!ut_files.exists("copied_file"), "'copied_file' does not exist");
        fatal_if(ut_files.move("copied_file", "moved_file") != 0,
            "ut_files.move('copied_file', 'moved_file') failed %s",
            ut_str.error(ut_runtime.err()));
        fatal_if(ut_files.exists("copied_file"), "'copied_file' still exists");
        fatal_if(!ut_files.exists("moved_file"), "'moved_file' does not exist");
        fatal_if(ut_files.unlink("moved_file") != 0,
                "ut_files.unlink('moved_file') failed %s",
                 ut_str.error(ut_runtime.err()));
        fatal_if(ut_files.chdir(cwd) != 0, "ut_files.chdir(\"%s\") failed %s",
                    cwd, ut_str.error(ut_runtime.err()));
    }
    fatal_if(ut_files.unlink(tf) != 0);
    if (ut_debug.verbosity.level > ut_debug.verbosity.quiet) { traceln("done"); }
}

#else

static void ut_files_test(void) {}

#endif // UT_TESTS

#pragma pop_macro("files_test_failed")

ut_files_if ut_files = {
    .invalid  = (ut_file_t*)INVALID_HANDLE_VALUE,
    // ut_files_stat_t.type:
    .type_folder  = 0x00000010, // FILE_ATTRIBUTE_DIRECTORY
    .type_symlink = 0x00000400, // FILE_ATTRIBUTE_REPARSE_POINT
    .type_device  = 0x00000040, // FILE_ATTRIBUTE_DEVICE
    // seek() methods:
    .seek_set = SEEK_SET,
    .seek_cur = SEEK_CUR,
    .seek_end = SEEK_END,
    // open() flags: missing O_RSYNC, O_DSYNC, O_NONBLOCK, O_NOCTTY
    .o_rd     = O_RDONLY,
    .o_wr     = O_WRONLY,
    .o_rw     = O_RDWR,
    .o_append = O_APPEND,
    .o_create = O_CREAT,
    .o_excl   = O_EXCL,
    .o_trunc  = O_TRUNC,
    .o_sync   = O_SYNC,
    .open               = ut_files_open,
    .is_valid           = ut_files_is_valid,
    .seek               = ut_files_seek,
    .stat               = ut_files_stat,
    .read               = ut_files_read,
    .write              = ut_files_write,
    .flush              = ut_files_flush,
    .close              = ut_files_close,
    .write_fully        = ut_files_write_fully,
    .exists             = ut_files_exists,
    .is_folder          = ut_files_is_folder,
    .is_symlink         = ut_files_is_symlink,
    .mkdirs             = ut_files_mkdirs,
    .rmdirs             = ut_files_rmdirs,
    .create_tmp         = ut_files_create_tmp,
    .chmod777           = ut_files_chmod777,
    .unlink             = ut_files_unlink,
    .link               = ut_files_link,
    .symlink            = ut_files_symlink,
    .basename           = ut_files_basename,
    .copy               = ut_files_copy,
    .move               = ut_files_move,
    .cwd                = ut_files_cwd,
    .chdir              = ut_files_chdir,
    .tmp                = ut_files_tmp,
    .bin                = ut_files_bin,
    .opendir            = ut_files_opendir,
    .readdir            = ut_files_readdir,
    .closedir           = ut_files_closedir,
    .test               = ut_files_test
};

// ______________________________ ut_generics.c _______________________________

#ifdef UT_TESTS

static void ut_generics_test(void) {
    {
        int8_t a = 10, b = 20;
        swear(ut_max(a++, b++) == 20);
        swear(ut_min(a++, b++) == 11);
    }
    {
        int32_t a = 10, b = 20;
        swear(ut_max(a++, b++) == 20);
        swear(ut_min(a++, b++) == 11);
    }
    {
        fp32_t a = 1.1f, b = 2.2f;
        swear(ut_max(a, b) == b);
        swear(ut_min(a, b) == a);
    }
    {
        fp64_t a = 1.1, b = 2.2;
        swear(ut_max(a, b) == b);
        swear(ut_min(a, b) == a);
    }
    {
        fp32_t a = 1.1f, b = 2.2f;
        swear(ut_max(a, b) == b);
        swear(ut_min(a, b) == a);
    }
    {
        fp64_t a = 1.1, b = 2.2;
        swear(ut_max(a, b) == b);
        swear(ut_min(a, b) == a);
    }
    {
        char a = 1, b = 2;
        swear(ut_max(a, b) == b);
        swear(ut_min(a, b) == a);
    }
    {
        unsigned char a = 1, b = 2;
        swear(ut_max(a, b) == b);
        swear(ut_min(a, b) == a);
    }
    // MS cl.exe version 19.39.33523 has issues with "long":
    // does not pick up int32_t/uint32_t types for "long" and "unsigned long"
    {
        long int a = 1, b = 2;
        swear(ut_max(a, b) == b);
        swear(ut_min(a, b) == a);
    }
    {
        unsigned long a = 1, b = 2;
        swear(ut_max(a, b) == b);
        swear(ut_min(a, b) == a);
    }
    {
        long long a = 1, b = 2;
        swear(ut_max(a, b) == b);
        swear(ut_min(a, b) == a);
    }
    if (ut_debug.verbosity.level > ut_debug.verbosity.quiet) { traceln("done"); }
}

#else

static void ut_generics_test(void) { }

#endif

ut_generics_if ut_generics = {
    .test = ut_generics_test
};

// ________________________________ ut_heap.c _________________________________

static ut_heap_t* ut_heap_create(bool serialized) {
    const DWORD options = serialized ? 0 : HEAP_NO_SERIALIZE;
    return (ut_heap_t*)HeapCreate(options, 0, 0);
}

static void ut_heap_dispose(ut_heap_t* h) {
    fatal_if_false(HeapDestroy((HANDLE)h));
}

static inline HANDLE mem_heap(ut_heap_t* h) {
    static HANDLE process_heap;
    if (process_heap == null) { process_heap = GetProcessHeap(); }
    return h != null ? (HANDLE)h : process_heap;
}

static errno_t ut_heap_allocate(ut_heap_t* h, void* *p, int64_t bytes, bool zero) {
    swear(bytes > 0);
    const DWORD flags = zero ? HEAP_ZERO_MEMORY : 0;
    *p = HeapAlloc(mem_heap(h), flags, (SIZE_T)bytes);
    return *p == null ? ERROR_OUTOFMEMORY : 0;
}

static errno_t ut_heap_reallocate(ut_heap_t* h, void* *p, int64_t bytes,
        bool zero) {
    swear(bytes > 0);
    const DWORD flags = zero ? HEAP_ZERO_MEMORY : 0;
    void* a = *p == null ? // HeapReAlloc(..., null, bytes) may not work
        HeapAlloc(mem_heap(h), flags, (SIZE_T)bytes) :
        HeapReAlloc(mem_heap(h), flags, *p, (SIZE_T)bytes);
    if (a != null) { *p = a; }
    return a == null ? ERROR_OUTOFMEMORY : 0;
}

static void ut_heap_deallocate(ut_heap_t* h, void* a) {
    fatal_if_false(HeapFree(mem_heap(h), 0, a));
}

static int64_t ut_heap_bytes(ut_heap_t* h, void* a) {
    SIZE_T bytes = HeapSize(mem_heap(h), 0, a);
    fatal_if(bytes == (SIZE_T)-1);
    return (int64_t)bytes;
}

static void ut_heap_test(void) {
    #ifdef UT_TESTS
    // TODO: allocate, reallocate deallocate, create, dispose
    traceln("TODO");
    if (ut_debug.verbosity.level > ut_debug.verbosity.quiet) { traceln("done"); }
    #endif
}

ut_heap_if ut_heap = {
    .create      = ut_heap_create,
    .allocate    = ut_heap_allocate,
    .reallocate  = ut_heap_reallocate,
    .deallocate  = ut_heap_deallocate,
    .bytes       = ut_heap_bytes,
    .dispose     = ut_heap_dispose,
    .test        = ut_heap_test
};

// _______________________________ ut_loader.c ________________________________

// This is oversimplified Win32 version completely ignoring mode.

// I bit more Posix compliant version is here:
// https://github.com/dlfcn-win32/dlfcn-win32/blob/master/src/dlfcn.c
// POSIX says that if the value of file is NULL, a handle on a global
// symbol object must be provided. That object must be able to access
// all symbols from the original program file, and any objects loaded
// with the RTLD_GLOBAL flag.
// The return value from GetModuleHandle( ) allows us to retrieve
// symbols only from the original program file. EnumProcessModules() is
// used to access symbols from other libraries. For objects loaded
// with the RTLD_LOCAL flag, we create our own list later on. They are
// excluded from EnumProcessModules() iteration.

static void* ut_loader_all;

static void* ut_loader_sym_all(const char* name) {
    void* sym = null;
    DWORD bytes = 0;
    fatal_if_false(EnumProcessModules(GetCurrentProcess(), null, 0, &bytes));
    assert(bytes % sizeof(HMODULE) == 0);
    assert(bytes / sizeof(HMODULE) < 1024); // OK to allocate 8KB on stack
    HMODULE* modules = null;
    fatal_if_not_zero(ut_heap.allocate(null, (void**)&modules, bytes, false));
    fatal_if_false(EnumProcessModules(GetCurrentProcess(), modules, bytes,
                                                                   &bytes));
    const int32_t n = bytes / (int)sizeof(HMODULE);
    for (int32_t i = 0; i < n && sym != null; i++) {
        sym = ut_loader.sym(modules[i], name);
    }
    if (sym == null) {
        sym = ut_loader.sym(GetModuleHandleA(null), name);
    }
    ut_heap.deallocate(null, modules);
    return sym;
}

static void* ut_loader_open(const char* filename, int32_t unused(mode)) {
    return filename == null ? &ut_loader_all : (void*)LoadLibraryA(filename);
}

static void* ut_loader_sym(void* handle, const char* name) {
    return handle == &ut_loader_all ?
            (void*)ut_loader_sym_all(name) :
            (void*)GetProcAddress((HMODULE)handle, name);
}

static void ut_loader_close(void* handle) {
    if (handle != &ut_loader_all) {
        fatal_if_false(FreeLibrary(handle));
    }
}

#ifdef UT_TESTS

static int32_t ut_loader_test_count;

export void ut_loader_test_exported_function(void) { ut_loader_test_count++; }

static void ut_loader_test(void) {
    ut_loader_test_count = 0;
    ut_loader_test_exported_function(); // to make sure it is linked in
    swear(ut_loader_test_count == 1);
    void* global = ut_loader.open(null, ut_loader.local);
    typedef void (*foo_t)(void);
    foo_t foo = (foo_t)ut_loader.sym(global, "ut_loader_test_exported_function");
    foo();
    swear(ut_loader_test_count == 2);
    ut_loader.close(global);
    // NtQueryTimerResolution - http://undocumented.ntinternals.net/
    typedef long (__stdcall *query_timer_resolution_t)(
        long* minimum_resolution,
        long* maximum_resolution,
        long* current_resolution);
    void* nt_dll = ut_loader.open("ntdll", ut_loader.local);
    query_timer_resolution_t query_timer_resolution =
        (query_timer_resolution_t)ut_loader.sym(nt_dll, "NtQueryTimerResolution");
    // in 100ns = 0.1us units
    long min_resolution = 0;
    long max_resolution = 0; // lowest possible delay between timer events
    long cur_resolution = 0;
    fatal_if(query_timer_resolution(
        &min_resolution, &max_resolution, &cur_resolution) != 0);
//  if (ut_debug.verbosity.level >= ut_debug.verbosity.trace) {
//      traceln("timer resolution min: %.3f max: %.3f cur: %.3f millisecond",
//          min_resolution / 10.0 / 1000.0,
//          max_resolution / 10.0 / 1000.0,
//          cur_resolution / 10.0 / 1000.0);
//      // Interesting observation cur_resolution sometimes 15.625ms or 1.0ms
//  }
    ut_loader.close(nt_dll);
    if (ut_debug.verbosity.level > ut_debug.verbosity.quiet) { traceln("done"); }
}

#else

static void ut_loader_test(void) {}

#endif

enum {
    ut_loader_local  = 0,       // RTLD_LOCAL  All symbols are not made available for relocation processing by other modules.
    ut_loader_lazy   = 1,       // RTLD_LAZY   Relocations are performed at an implementation-dependent time.
    ut_loader_now    = 2,       // RTLD_NOW    Relocations are performed when the object is loaded.
    ut_loader_global = 0x00100, // RTLD_GLOBAL All symbols are available for relocation processing of other modules.
};

ut_loader_if ut_loader = {
    .local  = ut_loader_local,
    .lazy   = ut_loader_lazy,
    .now    = ut_loader_now,
    .global = ut_loader_global,
    .open   = ut_loader_open,
    .sym    = ut_loader_sym,
    .close  = ut_loader_close,
    .test   = ut_loader_test
};

// _________________________________ ut_mem.c _________________________________

static errno_t ut_mem_map_view_of_file(HANDLE file,
        void* *data, int64_t *bytes, bool rw) {
    errno_t r = 0;
    void* address = null;
    HANDLE mapping = CreateFileMapping(file, null,
        rw ? PAGE_READWRITE : PAGE_READONLY,
        (uint32_t)(*bytes >> 32), (uint32_t)*bytes, null);
    if (mapping == null) {
        r = ut_runtime.err();
    } else {
        DWORD access = rw ? FILE_MAP_ALL_ACCESS : FILE_MAP_READ;
        address = MapViewOfFile(mapping, access, 0, 0, *bytes);
        if (address == null) { r = ut_runtime.err(); }
        fatal_if_false(CloseHandle(mapping));
    }
    if (r == 0) {
        *data = address;
    } else {
        *data = null;
        *bytes = 0;
    }
    return r;
}

// see: https://learn.microsoft.com/en-us/windows/win32/secauthz/enabling-and-disabling-privileges-in-c--

static errno_t ut_mem_set_token_privilege(void* token,
            const char* name, bool e) {
    TOKEN_PRIVILEGES tp = { .PrivilegeCount = 1 };
    tp.Privileges[0].Attributes = e ? SE_PRIVILEGE_ENABLED : 0;
    fatal_if_false(LookupPrivilegeValueA(null, name, &tp.Privileges[0].Luid));
    return b2e(AdjustTokenPrivileges(token, false, &tp,
               sizeof(TOKEN_PRIVILEGES), null, null));
}

static errno_t ut_mem_adjust_process_privilege_manage_volume_name(void) {
    // see: https://devblogs.microsoft.com/oldnewthing/20160603-00/?p=93565
    const uint32_t access = TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY;
    const HANDLE process = GetCurrentProcess();
    HANDLE token = null;
    errno_t r = b2e(OpenProcessToken(process, access, &token));
    if (r == 0) {
        #ifdef UNICODE
        const char* se_manage_volume_name = utf16to8(SE_MANAGE_VOLUME_NAME);
        #else
        const char* se_manage_volume_name = SE_MANAGE_VOLUME_NAME;
        #endif
        r = ut_mem_set_token_privilege(token, se_manage_volume_name, true);
        fatal_if_false(CloseHandle(token));
    }
    return r;
}

static errno_t ut_mem_map_file(const char* filename, void* *data,
        int64_t *bytes, bool rw) {
    if (rw) { // for SetFileValidData() call:
        (void)ut_mem_adjust_process_privilege_manage_volume_name();
    }
    errno_t r = 0;
    const DWORD access = GENERIC_READ | (rw ? GENERIC_WRITE : 0);
    const DWORD share = FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE;
    const DWORD disposition = rw ? OPEN_ALWAYS : OPEN_EXISTING;
    const DWORD flags = FILE_ATTRIBUTE_NORMAL;
    HANDLE file = CreateFileA(filename, access, share, null, disposition,
                              flags, null);
    if (file == INVALID_HANDLE_VALUE) {
        r = ut_runtime.err();
    } else {
        LARGE_INTEGER eof = { .QuadPart = 0 };
        fatal_if_false(GetFileSizeEx(file, &eof));
        if (rw && *bytes > eof.QuadPart) { // increase file size
            const LARGE_INTEGER size = { .QuadPart = *bytes };
            r = r != 0 ? r : (b2e(SetFilePointerEx(file, size, null, FILE_BEGIN)));
            r = r != 0 ? r : (b2e(SetEndOfFile(file)));
            // the following not guaranteed to work but helps with sparse files
            r = r != 0 ? r : (b2e(SetFileValidData(file, *bytes)));
            // SetFileValidData() only works for Admin (verified) or System accounts
            if (r == ERROR_PRIVILEGE_NOT_HELD) { r = 0; } // ignore
            // SetFileValidData() is also semi-security hole because it allows to read
            // previously not zeroed disk content of other files
            const LARGE_INTEGER zero = { .QuadPart = 0 }; // rewind stream:
            r = r != 0 ? r : (b2e(SetFilePointerEx(file, zero, null, FILE_BEGIN)));
        } else {
            *bytes = eof.QuadPart;
        }
        r = r != 0 ? r : ut_mem_map_view_of_file(file, data, bytes, rw);
        fatal_if_false(CloseHandle(file));
    }
    return r;
}

static errno_t ut_mem_map_ro(const char* filename, void* *data, int64_t *bytes) {
    return ut_mem_map_file(filename, data, bytes, false);
}

static errno_t ut_mem_map_rw(const char* filename, void* *data, int64_t *bytes) {
    return ut_mem_map_file(filename, data, bytes, true);
}

static void ut_mem_unmap(void* data, int64_t bytes) {
    assert(data != null && bytes > 0);
    (void)bytes; /* unused only need for posix version */
    if (data != null && bytes > 0) {
        fatal_if_false(UnmapViewOfFile(data));
    }
}

static errno_t ut_mem_map_resource(const char* label, void* *data, int64_t *bytes) {
    HRSRC res = FindResourceA(null, label, (const char*)RT_RCDATA);
    // "LockResource does not actually lock memory; it is just used to
    // obtain a pointer to the memory containing the resource data.
    // The name of the function comes from versions prior to Windows XP,
    // when it was used to lock a global memory block allocated by LoadResource."
    if (res != null) { *bytes = SizeofResource(null, res); }
    HGLOBAL g = res != null ? LoadResource(null, res) : null;
    *data = g != null ? LockResource(g) : null;
    return *data != null ? 0 : ut_runtime.err();
}

static int32_t ut_mem_page_size(void) {
    static SYSTEM_INFO system_info;
    if (system_info.dwPageSize == 0) {
        GetSystemInfo(&system_info);
    }
    return (int32_t)system_info.dwPageSize;
}

static int ut_mem_large_page_size(void) {
    static SIZE_T large_page_minimum = 0;
    if (large_page_minimum == 0) {
        large_page_minimum = GetLargePageMinimum();
    }
    return (int32_t)large_page_minimum;
}

static void* ut_mem_allocate(int64_t bytes_multiple_of_page_size) {
    assert(bytes_multiple_of_page_size > 0);
    SIZE_T bytes = (SIZE_T)bytes_multiple_of_page_size;
    int page_size = ut_mem_page_size();
    assert(bytes % page_size == 0);
    int r = 0;
    void* a = null;
    if (bytes_multiple_of_page_size < 0 || bytes % page_size != 0) {
        SetLastError(ERROR_INVALID_PARAMETER);
        r = EINVAL;
    } else {
        const DWORD type = MEM_COMMIT | MEM_RESERVE;
        const DWORD physical = type | MEM_PHYSICAL;
        a = VirtualAlloc(null, bytes, physical, PAGE_READWRITE);
        if (a == null) {
            a = VirtualAlloc(null, bytes, type, PAGE_READWRITE);
        }
        if (a == null) {
            r = ut_runtime.err();
            if (r != 0) {
                traceln("VirtualAlloc(%lld) failed %s", bytes, ut_str.error(r));
            }
        } else {
            r = VirtualLock(a, bytes) ? 0 : ut_runtime.err();
            if (r == ERROR_WORKING_SET_QUOTA) {
                // The default size is 345 pages (for example,
                // this is 1,413,120 bytes on systems with a 4K page size).
                SIZE_T min_mem = 0, max_mem = 0;
                r = b2e(GetProcessWorkingSetSize(GetCurrentProcess(), &min_mem, &max_mem));
                if (r != 0) {
                    traceln("GetProcessWorkingSetSize() failed %s", ut_str.error(r));
                } else {
                    max_mem =  max_mem + bytes * 2LL;
                    max_mem = (max_mem + page_size - 1) / page_size * page_size +
                               page_size * 16;
                    if (min_mem < max_mem) { min_mem = max_mem; }
                    r = b2e(SetProcessWorkingSetSize(GetCurrentProcess(),
                            min_mem, max_mem));
                    if (r != 0) {
                        traceln("SetProcessWorkingSetSize(%lld, %lld) failed %s",
                            (uint64_t)min_mem, (uint64_t)max_mem, ut_str.error(r));
                    } else {
                        r = b2e(VirtualLock(a, bytes));
                    }
                }
            }
            if (r != 0) {
                traceln("VirtualLock(%lld) failed %s", bytes, ut_str.error(r));
            }
        }
    }
    if (r != 0) {
        traceln("mem_alloc_pages(%lld) failed %s", bytes, ut_str.error(r));
        assert(a == null);
    }
    return a;
}

static void ut_mem_deallocate(void* a, int64_t bytes_multiple_of_page_size) {
    assert(bytes_multiple_of_page_size > 0);
    SIZE_T bytes = (SIZE_T)bytes_multiple_of_page_size;
    int r = 0;
    int page_size = ut_mem_page_size();
    if (bytes_multiple_of_page_size < 0 || bytes % page_size != 0) {
        r = EINVAL;
        traceln("failed %s", ut_str.error(r));
    } else {
        if (a != null) {
            // in case it was successfully locked
            r = b2e(VirtualUnlock(a, bytes));
            if (r != 0) {
                traceln("VirtualUnlock() failed %s", ut_str.error(r));
            }
            // If the "dwFreeType" parameter is MEM_RELEASE, "dwSize" parameter
            // must be the base address returned by the VirtualAlloc function when
            // the region of pages is reserved.
            r = b2e(VirtualFree(a, 0, MEM_RELEASE));
            if (r != 0) { traceln("VirtuaFree() failed %s", ut_str.error(r)); }
        }
    }
}

static void ut_mem_test(void) {
    #ifdef UT_TESTS
    swear(ut_args.c > 0);
    void* data = null;
    int64_t bytes = 0;
    swear(ut_mem.map_ro(ut_args.v[0], &data, &bytes) == 0);
    swear(data != null && bytes != 0);
    ut_mem.unmap(data, bytes);
    // TODO: page_size large_page_size allocate deallocate
    // TODO: test heap functions
    if (ut_debug.verbosity.level > ut_debug.verbosity.quiet) { traceln("done"); }
    #endif
}

ut_mem_if ut_mem = {
    .map_ro          = ut_mem_map_ro,
    .map_rw          = ut_mem_map_rw,
    .unmap           = ut_mem_unmap,
    .map_resource    = ut_mem_map_resource,
    .page_size       = ut_mem_page_size,
    .large_page_size = ut_mem_large_page_size,
    .allocate        = ut_mem_allocate,
    .deallocate      = ut_mem_deallocate,
    .test            = ut_mem_test
};
// _________________________________ ut_num.c _________________________________

#include <immintrin.h> // _tzcnt_u32

static inline ut_num128_t ut_num_add128_inline(const ut_num128_t a, const ut_num128_t b) {
    ut_num128_t r = a;
    r.hi += b.hi;
    r.lo += b.lo;
    if (r.lo < b.lo) { r.hi++; } // carry
    return r;
}

static inline ut_num128_t ut_num_sub128_inline(const ut_num128_t a, const ut_num128_t b) {
    ut_num128_t r = a;
    r.hi -= b.hi;
    if (r.lo < b.lo) { r.hi--; } // borrow
    r.lo -= b.lo;
    return r;
}

static ut_num128_t ut_num_add128(const ut_num128_t a, const ut_num128_t b) {
    return ut_num_add128_inline(a, b);
}

static ut_num128_t ut_num_sub128(const ut_num128_t a, const ut_num128_t b) {
    return ut_num_sub128_inline(a, b);
}

static ut_num128_t ut_num_mul64x64(uint64_t a, uint64_t b) {
    uint64_t a_lo = (uint32_t)a;
    uint64_t a_hi = a >> 32;
    uint64_t b_lo = (uint32_t)b;
    uint64_t b_hi = b >> 32;
    uint64_t low = a_lo * b_lo;
    uint64_t cross1 = a_hi * b_lo;
    uint64_t cross2 = a_lo * b_hi;
    uint64_t high = a_hi * b_hi;
    // this cannot overflow as (2^32-1)^2 + 2^32-1 < 2^64-1
    cross1 += low >> 32;
    // this one can overflow
    cross1 += cross2;
    // propagate the carry if any
    high += ((uint64_t)(cross1 < cross2 != 0)) << 32;
    high = high + (cross1 >> 32);
    low = ((cross1 & 0xFFFFFFFF) << 32) + (low & 0xFFFFFFFF);
    return (ut_num128_t){.lo = low, .hi = high };
}

static inline void ut_num_shift128_left_inline(ut_num128_t* n) {
    const uint64_t top = (1ULL << 63);
    n->hi = (n->hi << 1) | ((n->lo & top) ? 1 : 0);
    n->lo = (n->lo << 1);
}

static inline void ut_num_shift128_right_inline(ut_num128_t* n) {
    const uint64_t top = (1ULL << 63);
    n->lo = (n->lo >> 1) | ((n->hi & 0x1) ? top : 0);
    n->hi = (n->hi >> 1);
}

static inline bool ut_num_less128_inline(const ut_num128_t a, const ut_num128_t b) {
    return a.hi < b.hi || (a.hi == b.hi && a.lo < b.lo);
}

static inline bool ut_num_uint128_high_bit(const ut_num128_t a) {
    return (int64_t)a.hi < 0;
}

static uint64_t ut_num_muldiv128(uint64_t a, uint64_t b, uint64_t divisor) {
    swear(divisor > 0, "divisor: %lld", divisor);
    ut_num128_t r = ut_num.mul64x64(a, b); // reminder: a * b
    uint64_t q = 0; // quotient
    if (r.hi >= divisor) {
        q = UINT64_MAX; // overflow
    } else {
        int32_t  shift = 0;
        ut_num128_t d = { .hi = 0, .lo = divisor };
        while (!ut_num_uint128_high_bit(d) && ut_num_less128_inline(d, r)) {
            ut_num_shift128_left_inline(&d);
            shift++;
        }
        assert(shift <= 64);
        while (shift >= 0 && (d.hi != 0 || d.lo != 0)) {
            if (!ut_num_less128_inline(r, d)) {
                r = ut_num_sub128_inline(r, d);
                assert(shift < 64);
                q |= (1ULL << shift);
            }
            ut_num_shift128_right_inline(&d);
            shift--;
        }
    }
    return q;
}

#define ctz(x) _tzcnt_u32(x)

static uint32_t ut_num_gcd32(uint32_t u, uint32_t v) {
    uint32_t t = u | v;
    if (u == 0 || v == 0) { return t; }
    int32_t g = ctz(t);
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

static uint32_t ut_num_random32(uint32_t* state) {
    // https://gist.github.com/tommyettinger/46a874533244883189143505d203312c
    static thread_local bool started; // first seed must be odd
    if (!started) { started = true; *state |= 1; }
    uint32_t z = (*state += 0x6D2B79F5UL);
    z = (z ^ (z >> 15)) * (z | 1UL);
    z ^= z + (z ^ (z >> 7)) * (z | 61UL);
    return z ^ (z >> 14);
}

static uint64_t ut_num_random64(uint64_t *state) {
    // https://gist.github.com/tommyettinger/e6d3e8816da79b45bfe582384c2fe14a
    static thread_local bool started; // first seed must be odd
    if (!started) { started = true; *state |= 1; }
	const uint64_t s = *state;
	const uint64_t z = (s ^ s >> 25) * (*state += 0x6A5D39EAE12657AAULL);
	return z ^ (z >> 22);
}

// https://en.wikipedia.org/wiki/Fowler%E2%80%93Noll%E2%80%93Vo_hash_function

static uint32_t ut_num_hash32(const char *data, int64_t len) {
    uint32_t hash  = 0x811c9dc5;  // FNV_offset_basis for 32-bit
    uint32_t prime = 0x01000193; // FNV_prime for 32-bit
    if (len > 0) {
        for (int64_t i = 1; i < len; i++) {
            hash ^= data[i];
            hash *= prime;
        }
    } else {
        for (int64_t i = 0; data[i] != 0; i++) {
            hash ^= data[i];
            hash *= prime;
        }
    }
    return hash;
}

static uint64_t ut_num_hash64(const char *data, int64_t len) {
    uint64_t hash  = 0xcbf29ce484222325; // FNV_offset_basis for 64-bit
    uint64_t prime = 0x100000001b3;      // FNV_prime for 64-bit
    if (len > 0) {
        for (int64_t i = 0; i < len; i++) {
            hash ^= data[i];
            hash *= prime;
        }
    } else {
        for (int64_t i = 0; data[i] != 0; i++) {
            hash ^= data[i];
            hash *= prime;
        }
    }
    return hash;
}

static void ut_num_test(void) {
    #ifdef UT_TESTS
    {
        // https://asecuritysite.com/encryption/nprimes?y=64
        // https://www.rapidtables.com/convert/number/decimal-to-hex.html
        uint64_t p = 15843490434539008357u; // prime
        uint64_t q = 16304766625841520833u; // prime
        // pq: 258324414073910997987910483408576601381
        //     0xC25778F20853A9A1EC0C27C467C45D25
        ut_num128_t pq = {.hi = 0xC25778F20853A9A1uLL,
                       .lo = 0xEC0C27C467C45D25uLL };
        ut_num128_t p_q = ut_num.mul64x64(p, q);
        swear(p_q.hi == pq.hi && pq.lo == pq.lo);
        uint64_t p1 = ut_num.muldiv128(p, q, q);
        uint64_t q1 = ut_num.muldiv128(p, q, p);
        swear(p1 == p);
        swear(q1 == q);
    }
    #ifdef DEBUG
    enum { n = 100 };
    #else
    enum { n = 10000 };
    #endif
    uint64_t seed64 = 1;
    for (int32_t i = 0; i < n; i++) {
        uint64_t p = ut_num.random64(&seed64);
        uint64_t q = ut_num.random64(&seed64);
        uint64_t p1 = ut_num.muldiv128(p, q, q);
        uint64_t q1 = ut_num.muldiv128(p, q, p);
        swear(p == p1, "0%16llx (0%16llu) != 0%16llx (0%16llu)", p, p1);
        swear(q == q1, "0%16llx (0%16llu) != 0%16llx (0%16llu)", p, p1);
    }
    uint32_t seed32 = 1;
    for (int32_t i = 0; i < n; i++) {
        uint64_t p = ut_num.random32(&seed32);
        uint64_t q = ut_num.random32(&seed32);
        uint64_t r = ut_num.muldiv128(p, q, 1);
        swear(r == p * q);
        // division by the maximum uint64_t value:
        r = ut_num.muldiv128(p, q, UINT64_MAX);
        swear(r == 0);
    }
    if (ut_debug.verbosity.level > ut_debug.verbosity.quiet) { traceln("done"); }
    #endif
}

ut_num_if ut_num = {
    .add128    = ut_num_add128,
    .sub128    = ut_num_sub128,
    .mul64x64  = ut_num_mul64x64,
    .muldiv128 = ut_num_muldiv128,
    .gcd32     = ut_num_gcd32,
    .random32  = ut_num_random32,
    .random64  = ut_num_random64,
    .hash32    = ut_num_hash32,
    .hash64    = ut_num_hash64,
    .test      = ut_num_test
};

// ______________________________ ut_processes.c ______________________________

typedef struct ut_processes_pidof_lambda_s ut_processes_pidof_lambda_t;

typedef struct ut_processes_pidof_lambda_s {
    bool (*each)(ut_processes_pidof_lambda_t* p, uint64_t pid); // returns true to continue
    uint64_t* pids;
    size_t size;  // pids[size]
    size_t count; // number of valid pids in the pids
    fp64_t timeout;
    errno_t error;
} ut_processes_pidof_lambda_t;

static int32_t ut_processes_for_each_pidof(const char* pname, ut_processes_pidof_lambda_t* la) {
    char stack[1024]; // avoid alloca()
    int32_t n = ut_str.length(pname);
    fatal_if(n + 5 >= countof(stack), "name is too long: %s", pname);
    const char* name = pname;
    // append ".exe" if not present:
    if (!ut_str.ends_with_nc(pname, -1, ".exe", -1)) {
        int32_t k = (int32_t)strlen(pname) + 5;
        char* exe = stack;
        ut_str.format(exe, k, "%s.exe", pname);
        name = exe;
    }
    const char* base = strrchr(name, '\\');
    if (base != null) {
        base++; // advance past "\\"
    } else {
        base = name;
    }
    uint16_t wide[1024];
    fatal_if(strlen(base) >= countof(wide), "name too long: %s", base);
    uint16_t* wn = ut_str.utf8_utf16(wide, base);
    size_t count = 0;
    uint64_t pid = 0;
    byte* data = null;
    ULONG bytes = 0;
    errno_t r = NtQuerySystemInformation(SystemProcessInformation, data, 0, &bytes);
    #pragma push_macro("STATUS_INFO_LENGTH_MISMATCH")
    #define STATUS_INFO_LENGTH_MISMATCH      0xC0000004
    while (r == STATUS_INFO_LENGTH_MISMATCH) {
        // bytes == 420768 on Windows 11 which may be a bit
        // too much for stack alloca()
        // add little extra if new process is spawned in between calls.
        bytes += sizeof(SYSTEM_PROCESS_INFORMATION) * 32;
        r = ut_heap.reallocate(null, &data, bytes, false);
        if (r == 0) {
            r = NtQuerySystemInformation(SystemProcessInformation, data, bytes, &bytes);
        } else {
            assert(r == ERROR_NOT_ENOUGH_MEMORY);
        }
    }
    #pragma pop_macro("STATUS_INFO_LENGTH_MISMATCH")
    if (r == 0 && data != null) {
        SYSTEM_PROCESS_INFORMATION* proc = (SYSTEM_PROCESS_INFORMATION*)data;
        while (proc != null) {
            wchar_t* img = proc->ImageName.Buffer; // last name only, not a pathname!
            bool match = img != null && wcsicmp(img, wn) == 0;
            if (match) {
                pid = (uint64_t)proc->UniqueProcessId; // HANDLE .UniqueProcessId
                if (base != name) {
                    char path[ut_files_max_path];
                    match = ut_processes.nameof(pid, path, countof(path)) == 0 &&
                            ut_str.ends_with_nc(path, -1, name, -1);
//                  traceln("\"%s\" -> \"%s\" match: %d", name, path, match);
                }
            }
            if (match) {
                if (la != null && count < la->size && la->pids != null) {
                    la->pids[count] = pid;
                }
                count++;
                if (la != null && la->each != null && !la->each(la, pid)) {
                    break;
                }
            }
            proc = proc->NextEntryOffset != 0 ? (SYSTEM_PROCESS_INFORMATION*)
                ((byte*)proc + proc->NextEntryOffset) : null;
        }
    }
    if (data != null) { ut_heap.deallocate(null, data); }
    assert((int32_t)count == count);
    return (int32_t)count;
}

static void ut_processes_close_handle(HANDLE h) {
    fatal_if_false(CloseHandle(h));
}

static errno_t ut_processes_nameof(uint64_t pid, char* name, int32_t count) {
    assert(name != null && count > 0);
    errno_t r = 0;
    name[0] = 0;
    HANDLE p = OpenProcess(PROCESS_ALL_ACCESS, false, (DWORD)pid);
    if (p != null) {
        r = b2e(GetModuleFileNameExA(p, null, name, count));
        name[count - 1] = 0; // ensure zero termination
        ut_processes_close_handle(p);
    } else {
        r = ERROR_NOT_FOUND;
    }
    return r;
}

static bool ut_processes_present(uint64_t pid) {
    void* h = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, 0, (DWORD)pid);
    bool b = h != null;
    if (h != null) { ut_processes_close_handle(h); }
    return b;
}

static bool ut_processes_first_pid(ut_processes_pidof_lambda_t* lambda, uint64_t pid) {
    lambda->pids[0] = pid;
    return false;
}

static uint64_t ut_processes_pid(const char* pname) {
    uint64_t first[1] = {0};
    ut_processes_pidof_lambda_t lambda = {
        .each = ut_processes_first_pid,
        .pids = first,
        .size  = 1,
        .count = 0,
        .timeout = 0,
        .error = 0
    };
    ut_processes_for_each_pidof(pname, &lambda);
    return first[0];
}

static bool ut_processes_store_pid(ut_processes_pidof_lambda_t* lambda, uint64_t pid) {
    if (lambda->pids != null && lambda->count < lambda->size) {
        lambda->pids[lambda->count++] = pid;
    }
    return true; // always - need to count all
}

static errno_t ut_processes_pids(const char* pname, uint64_t* pids/*[size]*/,
        int32_t size, int32_t *count) {
    *count = 0;
    ut_processes_pidof_lambda_t lambda = {
        .each = ut_processes_store_pid,
        .pids = pids,
        .size  = size,
        .count = 0,
        .timeout = 0,
        .error = 0
    };
    *count = ut_processes_for_each_pidof(pname, &lambda);
    return (int32_t)lambda.count == *count ? 0 : ERROR_MORE_DATA;
}

static errno_t ut_processes_kill(uint64_t pid, fp64_t timeout) {
    DWORD timeout_milliseconds = (DWORD)(timeout * 1000);
    enum { access = PROCESS_QUERY_LIMITED_INFORMATION | PROCESS_TERMINATE | SYNCHRONIZE };
    assert((DWORD)pid == pid); // Windows... HANDLE vs DWORD in different APIs
    errno_t r = ERROR_NOT_FOUND;
    HANDLE h = OpenProcess(access, 0, (DWORD)pid);
    if (h != null) {
        char path[ut_files_max_path];
        path[0] = 0;
        r = b2e(TerminateProcess(h, ERROR_PROCESS_ABORTED));
        if (r == 0) {
            DWORD ix = WaitForSingleObject(h, timeout_milliseconds);
            r = wait2e(ix);
        } else {
            DWORD bytes = countof(path);
            errno_t rq = b2e(QueryFullProcessImageNameA(h, 0, path, &bytes));
            if (rq != 0) {
                traceln("QueryFullProcessImageNameA(pid=%d, h=%p) "
                        "failed %s", pid, h, ut_str.error(rq));
            }
        }
        ut_processes_close_handle(h);
        if (r == ERROR_ACCESS_DENIED) { // special case
            ut_thread.sleep_for(0.015); // need to wait a bit
            HANDLE retry = OpenProcess(access, 0, (DWORD)pid);
            // process may have died before we have chance to terminate it:
            if (retry == null) {
                traceln("TerminateProcess(pid=%d, h=%p, im=%s) "
                        "failed but zombie died after: %s",
                        pid, h, path, ut_str.error(r));
                r = 0;
            } else {
                ut_processes_close_handle(retry);
            }
        }
        if (r != 0) {
            traceln("TerminateProcess(pid=%d, h=%p, im=%s) failed %s",
                pid, h, path, ut_str.error(r));
        }
    }
    if (r != 0) { errno = r; }
    return r;
}

static bool ut_processes_kill_one(ut_processes_pidof_lambda_t* lambda, uint64_t pid) {
    errno_t r = ut_processes_kill(pid, lambda->timeout);
    if (r != 0) { lambda->error = r; }
    return true; // keep going
}

static errno_t ut_processes_kill_all(const char* name, fp64_t timeout) {
    ut_processes_pidof_lambda_t lambda = {
        .each = ut_processes_kill_one,
        .pids = null,
        .size  = 0,
        .count = 0,
        .timeout = timeout,
        .error = 0
    };
    int32_t c = ut_processes_for_each_pidof(name, &lambda);
    return c == 0 ? ERROR_NOT_FOUND : lambda.error;
}

static bool ut_processes_is_elevated(void) { // Is process running as Admin / System ?
    BOOL elevated = false;
    PSID administrators_group = null;
    // Allocate and initialize a SID of the administrators group.
    SID_IDENTIFIER_AUTHORITY administrators_group_authority = SECURITY_NT_AUTHORITY;
    errno_t r = b2e(AllocateAndInitializeSid(&administrators_group_authority, 2,
                SECURITY_BUILTIN_DOMAIN_RID, DOMAIN_ALIAS_RID_ADMINS,
                0, 0, 0, 0, 0, 0, &administrators_group));
    if (r != 0) {
        traceln("AllocateAndInitializeSid() failed %s", ut_str.error(r));
    }
    PSID system_ops = null;
    SID_IDENTIFIER_AUTHORITY system_ops_authority = SECURITY_NT_AUTHORITY;
    r = b2e(AllocateAndInitializeSid(&system_ops_authority, 2,
            SECURITY_BUILTIN_DOMAIN_RID, DOMAIN_ALIAS_RID_SYSTEM_OPS,
            0, 0, 0, 0, 0, 0, &system_ops));
    if (r != 0) {
        traceln("AllocateAndInitializeSid() failed %s", ut_str.error(r));
    }
    if (administrators_group != null) {
        r = b2e(CheckTokenMembership(null, administrators_group, &elevated));
    }
    if (system_ops != null && !elevated) {
        r = b2e(CheckTokenMembership(null, administrators_group, &elevated));
    }
    if (administrators_group != null) { FreeSid(administrators_group); }
    if (system_ops != null) { FreeSid(system_ops); }
    if (r != 0) {
        traceln("failed %s", ut_str.error(r));
    }
    return elevated;
}

static errno_t ut_processes_restart_elevated(void) {
    errno_t r = 0;
    if (!ut_processes.is_elevated()) {
        const char* path = ut_processes.name();
        SHELLEXECUTEINFOA sei = { sizeof(sei) };
        sei.lpVerb = "runas";
        sei.lpFile = path;
        sei.hwnd = null;
        sei.nShow = SW_NORMAL;
        r = b2e(ShellExecuteExA(&sei));
        if (r == ERROR_CANCELLED) {
            traceln("The user unable or refused to allow privileges elevation");
        } else if (r == 0) {
            ut_runtime.exit(0); // second copy of the app is running now
        }
    }
    return r;
}

static void ut_processes_close_pipes(STARTUPINFOA* si,
        HANDLE *read_out,
        HANDLE *read_err,
        HANDLE *write_in) {
    if (si->hStdOutput != INVALID_HANDLE_VALUE) { ut_processes_close_handle(si->hStdOutput); }
    if (si->hStdError  != INVALID_HANDLE_VALUE) { ut_processes_close_handle(si->hStdError);  }
    if (si->hStdInput  != INVALID_HANDLE_VALUE) { ut_processes_close_handle(si->hStdInput);  }
    if (*read_out != INVALID_HANDLE_VALUE) { ut_processes_close_handle(*read_out); }
    if (*read_err != INVALID_HANDLE_VALUE) { ut_processes_close_handle(*read_err); }
    if (*write_in != INVALID_HANDLE_VALUE) { ut_processes_close_handle(*write_in); }
}

static errno_t ut_processes_child_read(ut_stream_if* out, HANDLE pipe) {
    char data[32 * 1024]; // Temporary buffer for reading
    DWORD available = 0;
    errno_t r = b2e(PeekNamedPipe(pipe, null, sizeof(data), null,
                                 &available, null));
    if (r != 0) {
        if (r != ERROR_BROKEN_PIPE) { // unexpected!
//          traceln("PeekNamedPipe() failed %s", ut_str.error(r));
        }
        // process has exited and closed the pipe
        assert(r == ERROR_BROKEN_PIPE);
    } else if (available > 0) {
        DWORD bytes_read = 0;
        r = b2e(ReadFile(pipe, data, sizeof(data), &bytes_read, null));
//      traceln("r: %d bytes_read: %d", r, bytes_read);
        if (out != null) {
            if (r == 0) {
                r = out->write(out, data, bytes_read, null);
            }
        } else {
            // no one interested - drop on the floor
        }
    }
    return r;
}

static errno_t ut_processes_child_write(ut_stream_if* in, HANDLE pipe) {
    errno_t r = 0;
    if (in != null) {
        uint8_t  memory[32 * 1024]; // Temporary buffer for reading
        uint8_t* data = memory;
        int64_t bytes_read = 0;
        in->read(in, data, sizeof(data), &bytes_read);
        while (r == 0 && bytes_read > 0) {
            DWORD bytes_written = 0;
            r = b2e(WriteFile(pipe, data, (DWORD)bytes_read,
                             &bytes_written, null));
            traceln("r: %d bytes_written: %d", r, bytes_written);
            assert((int32_t)bytes_written <= bytes_read);
            data += bytes_written;
            bytes_read -= bytes_written;
        }
    }
    return r;
}

static errno_t ut_processes_run(ut_processes_child_t* child) {
    const fp64_t deadline = ut_clock.seconds() + child->timeout;
    errno_t r = 0;
    STARTUPINFOA si = {
        .cb = sizeof(STARTUPINFOA),
        .hStdInput  = INVALID_HANDLE_VALUE,
        .hStdOutput = INVALID_HANDLE_VALUE,
        .hStdError  = INVALID_HANDLE_VALUE,
        .dwFlags     = STARTF_USESHOWWINDOW | STARTF_USESTDHANDLES,
        .wShowWindow = SW_HIDE
    };
    SECURITY_ATTRIBUTES sa = { sizeof(sa), null, true };  // Inheritable handles
    PROCESS_INFORMATION pi = {0};
    HANDLE read_out = INVALID_HANDLE_VALUE;
    HANDLE read_err = INVALID_HANDLE_VALUE;
    HANDLE write_in = INVALID_HANDLE_VALUE;
    errno_t ro = b2e(CreatePipe(&read_out, &si.hStdOutput, &sa, 0));
    errno_t re = b2e(CreatePipe(&read_err, &si.hStdError,  &sa, 0));
    errno_t ri = b2e(CreatePipe(&si.hStdInput, &write_in,  &sa, 0));
    if (ro != 0 || re != 0 || ri != 0) {
        ut_processes_close_pipes(&si, &read_out, &read_err, &write_in);
        if (ro != 0) { traceln("CreatePipe() failed %s", ut_str.error(ro)); r = ro; }
        if (re != 0) { traceln("CreatePipe() failed %s", ut_str.error(re)); r = re; }
        if (ri != 0) { traceln("CreatePipe() failed %s", ut_str.error(ri)); r = ri; }
    }
    if (r == 0) {
        r = b2e(CreateProcessA(null, (char*)child->command, null, null, true,
                CREATE_NO_WINDOW, null, null, &si, &pi));
        if (r != 0) {
            traceln("CreateProcess() failed %s", ut_str.error(r));
            ut_processes_close_pipes(&si, &read_out, &read_err, &write_in);
        }
    }
    if (r == 0) {
        // not relevant: stdout can be written in other threads
        fatal_if_false(CloseHandle(pi.hThread));
        pi.hThread = null;
        // need to close si.hStdO* handles on caller side so,
        // when the process closes handles of the pipes, EOF happens
        // on caller side with io result ERROR_BROKEN_PIPE
        // indicating no more data can be read or written
        fatal_if_false(CloseHandle(si.hStdOutput));
        fatal_if_false(CloseHandle(si.hStdError));
        fatal_if_false(CloseHandle(si.hStdInput));
        si.hStdOutput = INVALID_HANDLE_VALUE;
        si.hStdError  = INVALID_HANDLE_VALUE;
        si.hStdInput  = INVALID_HANDLE_VALUE;
        bool done = false;
        while (!done && r == 0) {
            if (child->timeout > 0 && ut_clock.seconds() > deadline) {
                r = b2e(TerminateProcess(pi.hProcess, ERROR_SEM_TIMEOUT));
                if (r != 0) {
                    traceln("TerminateProcess() failed %s", ut_str.error(r));
                } else {
                    done = true;
                }
            }
            if (r == 0) { r = ut_processes_child_write(child->in, write_in); }
            if (r == 0) { r = ut_processes_child_read(child->out, read_out); }
            if (r == 0) { r = ut_processes_child_read(child->err, read_err); }
            if (!done) {
                DWORD ix = WaitForSingleObject(pi.hProcess, 0);
                // ix == 0 means process has exited (or terminated)
                // r == ERROR_BROKEN_PIPE process closed one of the handles
                done = ix == WAIT_OBJECT_0 || r == ERROR_BROKEN_PIPE;
            }
            // to avoid tight loop 100% cpu utilization:
            if (!done) { ut_thread.yield(); }
        }
        // broken pipe actually signifies EOF on the pipe
        if (r == ERROR_BROKEN_PIPE) { r = 0; } // not an error
//      if (r != 0) { traceln("pipe loop failed %s", ut_str.error(r));}
        DWORD xc = 0;
        errno_t rx = b2e(GetExitCodeProcess(pi.hProcess, &xc));
        if (rx == 0) {
            child->exit_code = xc;
        } else {
            traceln("GetExitCodeProcess() failed %s", ut_str.error(rx));
            if (r != 0) { r = rx; } // report earliest error
        }
        ut_processes_close_pipes(&si, &read_out, &read_err, &write_in);
        fatal_if_false(CloseHandle(pi.hProcess)); // expected never to fail
    }
    return r;
}

typedef struct {
    ut_stream_if stream;
    ut_stream_if* output;
    errno_t error;
} ut_processes_io_merge_out_and_err_if;

static errno_t ut_processes_merge_write(ut_stream_if* stream, const void* data,
        int64_t bytes, int64_t* transferred) {
    if (transferred != null) { *transferred = 0; }
    ut_processes_io_merge_out_and_err_if* s =
        (ut_processes_io_merge_out_and_err_if*)stream;
    if (s->output != null && bytes > 0) {
        s->error = s->output->write(s->output, data, bytes, transferred);
    }
    return s->error;
}

static errno_t ut_processes_open(const char* command, int32_t *exit_code,
        ut_stream_if* output,  fp64_t timeout) {
    not_null(output);
    ut_processes_io_merge_out_and_err_if merge_out_and_err = {
        .stream ={ .write = ut_processes_merge_write },
        .output = output,
        .error = 0
    };
    ut_processes_child_t child = {
        .command = command,
        .in = null,
        .out = &merge_out_and_err.stream,
        .err = &merge_out_and_err.stream,
        .exit_code = 0,
        .timeout = timeout
    };
    errno_t r = ut_processes.run(&child);
    if (exit_code != null) { *exit_code = child.exit_code; }
    uint8_t zero = 0; // zero termination
    merge_out_and_err.stream.write(&merge_out_and_err.stream, &zero, 1, null);
    if (r == 0 && merge_out_and_err.error != 0) {
        r = merge_out_and_err.error; // zero termination is not guaranteed
    }
    return r;
}

static errno_t ut_processes_spawn(const char* command) {
    errno_t r = 0;
    STARTUPINFOA si = {
        .cb = sizeof(STARTUPINFOA),
        .dwFlags     = STARTF_USESHOWWINDOW
                     | CREATE_NEW_PROCESS_GROUP
                     | DETACHED_PROCESS,
        .wShowWindow = SW_HIDE,
        .hStdInput  = INVALID_HANDLE_VALUE,
        .hStdOutput = INVALID_HANDLE_VALUE,
        .hStdError  = INVALID_HANDLE_VALUE
    };
    const DWORD flags = CREATE_BREAKAWAY_FROM_JOB
                | CREATE_NO_WINDOW
                | CREATE_NEW_PROCESS_GROUP
                | DETACHED_PROCESS;
    PROCESS_INFORMATION pi = {0};
    r = b2e(CreateProcessA(null, (char*)command, null, null,
            /*bInheritHandles:*/false, flags, null, null, &si, &pi));
    if (r == 0) { // Close handles immediately
        fatal_if_false(CloseHandle(pi.hProcess));
        fatal_if_false(CloseHandle(pi.hThread));
    } else {
//      traceln("CreateProcess() failed %s", ut_str.error(r));
    }
    return r;
}

static const char* ut_processes_name(void) {
    static char module_name[ut_files_max_path];
    if (module_name[0] == 0) {
        fatal_if_false(GetModuleFileNameA(null, module_name, countof(module_name)));
    }
    return module_name;
}

#pragma push_macro("verbose") // --verbosity trace

#define verbose(...) do {                                 \
    if (ut_debug.verbosity.level >= ut_debug.verbosity.trace) { \
        traceln(__VA_ARGS__);                             \
    }                                                     \
} while (0)

static void ut_processes_test(void) {
    #ifdef UT_TESTS // in alphabetical order
    const char* names[] = { "svchost", "RuntimeBroker", "conhost" };
    for (int32_t j = 0; j < countof(names); j++) {
        int32_t size  = 0;
        int32_t count = 0;
        uint64_t* pids = null;
        errno_t r = ut_processes.pids(names[j], null, size, &count);
        while (r == ERROR_MORE_DATA && count > 0) {
            size = count * 2; // set of processes may change rapidly
            r = ut_heap.reallocate(null, &pids, sizeof(uint64_t) * size, false);
            if (r == 0) {
                r = ut_processes.pids(names[j], pids, size, &count);
            }
        }
        if (r == 0 && count > 0) {
            for (int32_t i = 0; i < count; i++) {
                char path[256] = {0};
                #pragma warning(suppress: 6011) // dereferencing null
                r = ut_processes.nameof(pids[i], path, countof(path));
                if (r != ERROR_NOT_FOUND) {
                    assert(r == 0 && !strempty(path));
                    verbose("%6d %s %s", pids[i], path, r == 0 ? "" : ut_str.error(r));
                }
            }
        }
        ut_heap.deallocate(null, pids);
    }
    // test popen()
    int32_t xc = 0;
    char data[32 * 1024];
    ut_stream_memory_if output;
    ut_streams.write_only(&output, data, countof(data));
    const char* cmd = "cmd /c dir 2>nul >nul";;
    errno_t r = ut_processes.popen(cmd, &xc, &output.stream, 99999.0);
    verbose("r: %d xc: %d output:\n%s", r, xc, data);
    ut_streams.write_only(&output, data, countof(data));
    cmd = "cmd /c dir \"folder that does not exist\\\"";
    r = ut_processes.popen(cmd, &xc, &output.stream, 99999.0);
    verbose("r: %d xc: %d output:\n%s", r, xc, data);
    ut_streams.write_only(&output, data, countof(data));
    cmd = "cmd /c dir";
    r = ut_processes.popen(cmd, &xc, &output.stream, 99999.0);
    verbose("r: %d xc: %d output:\n%s", r, xc, data);
    ut_streams.write_only(&output, data, countof(data));
    cmd = "cmd /c timeout 1";
    r = ut_processes.popen(cmd, &xc, &output.stream, 1.0E-9);
    verbose("r: %d xc: %d output:\n%s", r, xc, data);
    #endif
}

#pragma pop_macro("verbose")

ut_processes_if ut_processes = {
    .pid                 = ut_processes_pid,
    .pids                = ut_processes_pids,
    .nameof              = ut_processes_nameof,
    .present             = ut_processes_present,
    .kill                = ut_processes_kill,
    .kill_all            = ut_processes_kill_all,
    .is_elevated         = ut_processes_is_elevated,
    .restart_elevated    = ut_processes_restart_elevated,
    .run                 = ut_processes_run,
    .popen               = ut_processes_open,
    .spawn               = ut_processes_spawn,
    .name                = ut_processes_name,
    .test                = ut_processes_test
};

// _______________________________ ut_runtime.c _______________________________

// abort does NOT call atexit() functions and
// does NOT flush ut_streams. Also Win32 runtime
// abort() attempt to show Abort/Retry/Ignore
// MessageBox - thus ExitProcess()

static void ut_runtime_abort(void) { ExitProcess(ERROR_FATAL_APP_EXIT); }

static void ut_runtime_exit(int32_t exit_code) { exit(exit_code); }

// TODO: consider r = HRESULT_FROM_WIN32() and r = HRESULT_CODE(hr);
// this separates posix error codes from win32 error codes


static int32_t ut_runtime_err(void) { return GetLastError(); }

static void ut_runtime_seterr(int32_t err) { SetLastError(err); }

ut_static_init(runtime) {
    SetErrorMode(
        // The system does not display the critical-error-handler message box.
        // Instead, the system sends the error to the calling process:
        SEM_FAILCRITICALERRORS|
        // The system automatically fixes memory alignment faults and
        // makes them invisible to the application.
        SEM_NOALIGNMENTFAULTEXCEPT|
        // The system does not display the Windows Error Reporting dialog.
        SEM_NOGPFAULTERRORBOX|
        // The OpenFile function does not display a message box when it fails
        // to find a file. Instead, the error is returned to the caller.
        // This error mode overrides the OF_PROMPT flag.
        SEM_NOOPENFILEERRORBOX);
}

#ifdef UT_TESTS

static void ut_runtime_test(void) { // in alphabetical order
    ut_args.test();
    ut_atomics.test();
    ut_clipboard.test();
    ut_clock.test();
    ut_config.test();
    ut_debug.test();
    ut_event.test();
    ut_files.test();
    ut_generics.test();
    ut_heap.test();
    ut_loader.test();
    ut_mem.test();
    ut_mutex.test();
    ut_num.test();
    ut_processes.test();
    ut_static_init_test();
    ut_str.test();
    ut_streams.test();
    ut_thread.test();
    vigil.test();
}

#else

static void ut_runtime_test(void) { }

#endif

ut_runtime_if ut_runtime = {
    .err    = ut_runtime_err,
    .seterr = ut_runtime_seterr,
    .abort  = ut_runtime_abort,
    .exit   = ut_runtime_exit,
    .test   = ut_runtime_test
};

#pragma comment(lib, "advapi32")
#pragma comment(lib, "ntdll")
#pragma comment(lib, "psapi")
#pragma comment(lib, "shell32")
#pragma comment(lib, "shlwapi")
#pragma comment(lib, "kernel32")
#pragma comment(lib, "user32") // clipboard

// _______________________________ ut_static.c ________________________________

static void*   _static_symbol_reference[1024];
static int32_t _static_symbol_reference_count;

void* _static_force_symbol_reference_(void* symbol) {
    assert(_static_symbol_reference_count <= countof(_static_symbol_reference),
        "increase size of _static_symbol_reference[%d] to at least %d",
        countof(_static_symbol_reference), _static_symbol_reference);
    if (_static_symbol_reference_count < countof(_static_symbol_reference)) {
        _static_symbol_reference[_static_symbol_reference_count] = symbol;
//      traceln("_static_symbol_reference[%d] = %p", _static_symbol_reference_count,
//               _static_symbol_reference[symbol_reference_count]);
        _static_symbol_reference_count++;
    }
    return symbol;
}

// test ut_static_init() { code } that will be executed in random
// order but before main()

#ifdef UT_TESTS

static int32_t ut_static_init_function_called;

static void force_inline ut_static_init_function(void) {
    ut_static_init_function_called = 1;
}

ut_static_init(static_init_test) { ut_static_init_function(); }

void ut_static_init_test(void) {
    fatal_if(ut_static_init_function_called != 1,
        "static_init_function() expected to be called before main()");
    if (ut_debug.verbosity.level > ut_debug.verbosity.quiet) { traceln("done"); }
}

#else

void ut_static_init_test(void) {}

#endif

// _________________________________ ut_str.c _________________________________

char* strnchr(const char* s, int32_t n, char ch) {
    while (n > 0 && *s != 0) {
        if (*s == ch) { return (char*)s; }
        s++; n--;
    }
    return null;
}

static void ut_str_format_va(char* utf8, int32_t count, const char* format,
        va_list vl) {
    vsnprintf(utf8, count, format, vl);
    utf8[count - 1] = 0;
}

static void ut_str_format(char* utf8, int32_t count, const char* format, ...) {
    va_list vl;
    va_start(vl, format);
    ut_str.format_va(utf8, count, format, vl);
    va_end(vl);
}

static const char* ut_str_error_for_language(int32_t error, LANGID language) {
    static thread_local char text[256];
    const DWORD format = FORMAT_MESSAGE_FROM_SYSTEM|
        FORMAT_MESSAGE_IGNORE_INSERTS;
    uint16_t s[256];
    HRESULT hr = 0 <= error && error <= 0xFFFF ?
        HRESULT_FROM_WIN32(error) : error;
    if (FormatMessageW(format, null, hr, language, s, countof(s) - 1,
            (va_list*)null) > 0) {
        s[countof(s) - 1] = 0;
        // remove trailing '\r\n'
        int32_t k = (int32_t)wcslen(s);
        if (k > 0 && s[k - 1] == '\n') { s[k - 1] = 0; }
        k = (int32_t)wcslen(s);
        if (k > 0 && s[k - 1] == '\r') { s[k - 1] = 0; }
        char stack[2048];
        fatal_if(k >= countof(stack), "error message too long");
        strprintf(text, "0x%08X(%d) \"%s\"", error, error,
                  ut_str.utf16_utf8(stack, s));
    } else {
        strprintf(text, "0x%08X(%d)", error, error);
    }
    return text;
}

static const char* ut_str_error(int32_t error) {
    const LANGID lang = MAKELANGID(LANG_ENGLISH, SUBLANG_DEFAULT);
    return ut_str_error_for_language(error, lang);
}

static const char* ut_str_error_nls(int32_t error) {
    const LANGID lang = MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT);
    return ut_str_error_for_language(error, lang);
}

static int32_t ut_str_utf8_bytes(const uint16_t* utf16) {
    int32_t required_bytes_count =
        WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS,
        utf16, -1, null, 0, null, null);
    swear(required_bytes_count > 0);
    return required_bytes_count;
}

static int32_t ut_str_utf16_chars(const char* utf8) {
    int32_t required_wide_chars_count =
        MultiByteToWideChar(CP_UTF8, 0, utf8, -1, null, 0);
    swear(required_wide_chars_count > 0);
    return required_wide_chars_count;
}

static char* ut_str_utf16to8(char* s, const uint16_t* utf16) {
    errno_t r = WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS, utf16, -1, s,
        ut_str.utf8_bytes(utf16), null, null);
    fatal_if(r == 0, "WideCharToMultiByte() failed %s",
            ut_str.error(ut_runtime.err()));
    return s;
}

static uint16_t* ut_str_utf8to16(uint16_t* utf16, const char* s) {
    errno_t r = MultiByteToWideChar(CP_UTF8, 0, s, -1, utf16,
                                ut_str.utf16_chars(s));
    fatal_if(r == 0, "WideCharToMultiByte() failed %s",
            ut_str.error(ut_runtime.err()));
    return utf16;
}

static bool ut_str_is_empty(const char* s) {
    return strempty(s);
}

static bool ut_str_equal(const char* s1, int32_t n1, const char* s2, int32_t n2) {
    if (n1 < 0) { n1 = ut_str.length(s1); }
    if (n2 < 0) { n2 = ut_str.length(s2); }
    return n1 == n2 && memcmp(s1, s2, n1) == 0;
}

static bool ut_str_equal_nc(const char* s1, int32_t n1, const char* s2, int32_t n2) {
    if (n1 < 0 && n2 < 0) {
        return striequ(s1, s2);
    }
    if (n1 < 0) { n1 = ut_str.length(s1); }
    if (n2 < 0) { n2 = ut_str.length(s2); }
    if (n1 != n2) { return false; }
    for (int32_t i = 0; i < n1; i++) {
        if (tolower(s1[i]) != tolower(s2[i])) { return false; }
    }
    return true;
}

static bool ut_str_starts_with(const char* s1, int32_t n1, const char* s2, int32_t n2) {
    if (n1 < 0 && n2 < 0) { return strstartswith(s1, s2); }
    if (n1 <= 0) { n1 = (int32_t)strlen(s1); }
    if (n2 <= 0) { n2 = (int32_t)strlen(s2); }
    return n1 >= n2 && memcmp(s1, s2, n2) == 0;
}

static bool ut_str_ends_with(const char* s1, int32_t n1, const char* s2, int32_t n2) {
    if (n1 < 0 && n2 < 0) {
        return s1 != null && s2 != null && strendswith(s1, s2);
    }
    if (n1 <= 0) { n1 = (int32_t)strlen(s1); }
    if (n2 <= 0) { n2 = (int32_t)strlen(s2); }
    return n1 >= n2 && memcmp(s1 + n1 - n2, s2, n2) == 0;
}

static bool ut_str_starts_with_nc(const char* s1, int32_t n1, const char* s2, int32_t n2) {
    if (n1 < 0 && n2 < 0) { return strstartswith(s1, s2); }
    if (n1 <= 0) { n1 = (int32_t)strlen(s1); }
    if (n2 <= 0) { n2 = (int32_t)strlen(s2); }
    return n1 >= n2 && ut_str.compare_nc(s1, n2, s2, n2) == 0;
}

static bool ut_str_ends_with_nc(const char* s1, int32_t n1, const char* s2, int32_t n2) {
    if (n1 <= 0) { n1 = (int32_t)strlen(s1); }
    if (n2 <= 0) { n2 = (int32_t)strlen(s2); }
    return n1 >= n2 && ut_str.compare_nc(s1 + n1 - n2, n2, s2, n2) == 0;
}

static int32_t ut_str_length(const char* s) {
    return strlength(s);
}

static bool ut_str_copy(char* d, int32_t capacity,
                     const char* s, int32_t bytes) {
    not_null(d);
    not_null(s);
    swear(capacity > 1, "capacity: %d", capacity);
    if (bytes < 0) {
        while (capacity > 1 && *s != 0) {
            *d++ = *s++;
            capacity--;
        }
        *d = 0;
        return *s == 0;
    } else {
        while (capacity > 1 && *s != 0 && bytes > 0) {
            *d++ = *s++;
            capacity--;
            bytes--;
        }
        *d = 0;
        return *s == 0 || bytes == 0;
    }
}

static char* ut_str_first_char(const char* s, int32_t bytes, char ch) {
    return bytes > 0 ? strnchr(s, bytes, ch) : strchr(s, ch);
}

static char* ut_str_last_char(const char* s, int32_t n, char ch) {
    if (n < 0) { return strrchr(s, ch); }
    for (int32_t i = n; i >= 0; i--) {
        if (s[i] == ch) { return (char*)&s[i]; }
    }
    return null;
}

static char* ut_str_first(const char* s1, int32_t n1, const char* s2, int32_t n2) {
    if (n1 < 0 && n2 < 0) { return strstr(s1, s2); }
    if (n1 < 0) { n1 = ut_str.length(s1); }
    if (n2 < 0) { n2 = ut_str.length(s2); }
    for (int32_t i = 0; i <= n1 - n2; i++) {
        if (memcmp(s1 + i, s2, n2) == 0) { return (char*)&s1[i]; }
    }
    return null;
}

static bool ut_str_to_lower(char* d, int32_t capacity, const char* s, int32_t n) {
    swear(capacity > 1, "capacity: %d", capacity);
    if (n < 0) {
        while (*s != 0 && capacity > 1) {
            *d++ = (char)tolower(*s++);
            capacity--;
        }
        *d = 0;
        return *s == 0;
    } else {
        while (capacity > 1 && n > 0) {
            *d++ = (char)tolower(*s++);
            capacity--;
            n--;
        }
        *d = 0;
        return n == 0;
    }
}

static bool ut_str_to_upper(char* d, int32_t capacity, const char* s, int32_t n) {
    swear(capacity > 1, "capacity: %d", capacity);
    if (n < 0) {
        while (*s != 0 && capacity > 1) {
            *d++ = (char)toupper(*s++);
            capacity--;
        }
        *d = 0;
        return *s == 0;
    } else {
        while (capacity > 1 && n > 0) {
            *d++ = (char)toupper(*s++);
            capacity--;
            n--;
        }
        *d = 0;
        return n == 0;
    }
}

static int32_t ut_str_compare(const char* s1, int32_t n1, const char* s2, int32_t n2) {
    if (n2 < 0) {
        return n1 > 0 ? strncmp(s1, s2, n1) : strcmp(s1, s2);
    } else {
        if (n1 < 0) { n1 = ut_str.length(s1); }
        if (n2 < 0) { n2 = ut_str.length(s2); }
        return n1 == n2 ? memcmp(s1, s2, n1) : n1 - n2;
    }
}

static int32_t ut_str_compare_nc(const char* s1, int32_t n1, const char* s2, int32_t n2) {
    if (n2 < 0) {
        return n1 > 0 ? strncasecmp(s1, s2, n1) : strcasecmp(s1, s2);
    } else {
        if (n1 < 0) { n1 = ut_str.length(s1); }
        if (n2 < 0) { n2 = ut_str.length(s2); }
        int32_t n = ut_min(n1, n2);
        for (int32_t i = 0; i < n; i++) {
            int32_t r = tolower(s1[i]) - tolower(s2[i]);
            if (r != 0) { return r; }
        }
        return n1 - n2;
    }
}

static const char* ut_str_unquote(char* *s, int32_t n) {
    if (n < 0) { n = ut_str.length(*s); }
    if (n > 0 && (*s)[0] == '\"' && (*s)[n - 1] == '\"') {
        (*s)[n - 1] = 0x00;
        (*s)++;
    }
    return *s;
}


#ifdef UT_TESTS

static void ut_str_test(void) {
    #pragma push_macro("glyph_chinese_one")
    #pragma push_macro("glyph_chinese_two")
    #pragma push_macro("glyph_teddy_bear")
    #pragma push_macro("glyph_ice_cube")
    #define glyph_chinese_one "\xE5\xA3\xB9"
    #define glyph_chinese_two "\xE8\xB4\xB0"
    #define glyph_teddy_bear  "\xF0\x9F\xA7\xB8"
    #define glyph_ice_cube    "\xF0\x9F\xA7\x8A"
    const char* null_string = null;
    swear(strempty(null_string));
    swear(strempty(""));
    swear(!strempty("hello"));
    swear(strequ("hello", "hello"));
    swear(!strequ("hello", "world"));
    swear(strcasecmp("hello", "HeLLo") == 0);
    swear(strncasecmp("hello", "HeLLo", 4) == 0);
    swear(strcasecmp("hello", "world") != 0);
    swear(strncasecmp("hello", "world", 4) != 0);
    swear(strequ(strconcat("hello_", "_world"), "hello__world"));
    swear(ut_str.is_empty(null_string));
    swear(ut_str.is_empty(""));
    swear(!ut_str.is_empty("hello"));
    swear(ut_str.equal("hello", -1, "hello", -1));
    swear(ut_str.equal_nc("hello", -1, "HeLlO", -1));
    swear(!ut_str.equal("hello", -1, "world", -1));
    swear(!ut_str.equal("hello", -1, "hello", 4));
    swear(!ut_str.equal("hello",  4, "hello", -1));
    swear(ut_str.equal("hellO",  4, "hello", 4));
    swear(ut_str.equal("abc", 1, "a", 1));
    swear(!ut_str.equal("a", 1, "abc", 3));
    swear(ut_str.starts_with("hello world", -1, "hello", -1));
    swear(!ut_str.starts_with("hello world", -1, "world", -1));
    swear(ut_str.ends_with("hello world", -1, "world", -1));
    swear(!ut_str.ends_with("hello world", -1, "hello", -1));
    swear(ut_str.length("hello") == 5);
    char text[10];
    swear(ut_str.copy(text, countof(text), "hello", -1));
    swear(ut_str.equal(text, -1, "hello", -1));
    swear(!ut_str.copy(text, 9, "0123456789", -1));
    char lower[20];
    swear(ut_str.to_lower(lower, countof(lower), "HeLlO WoRlD", -1));
    swear(ut_str.equal(lower, -1, "hello world", -1));
    const char* utf8_str =
            glyph_teddy_bear "0"
            glyph_chinese_one glyph_chinese_two
            "3456789 "
            glyph_ice_cube;
    const uint16_t* wide_str = utf8to16(utf8_str);
    char utf8[100];
    swear(ut_str.utf16_utf8(utf8, wide_str));
    uint16_t utf16[100];
    swear(ut_str.utf8_utf16(utf16, utf8));
    swear(ut_str.equal(utf16to8(utf16), -1, utf8_str, -1));
    char formatted[100];
    ut_str.format(formatted, countof(formatted), "n: %d, s: %s", 42, "test");
    swear(ut_str.equal(formatted, -1, "n: 42, s: test", -1));
    char truncated[9];
    truncated[8] = 0xFF;
    swear(!ut_str.copy(truncated, countof(truncated), "0123456789", -1));
    swear(truncated[8] == 0);
    char truncated_lower[9];
    truncated_lower[8] = 0xFF;
    ut_str.to_lower(truncated_lower, countof(truncated_lower), "HELLO012345", -1);
    swear(truncated_lower[8] == 0);
    char truncated_formatted[9];
    truncated_formatted[8] = 0xFF;
    ut_str.format(truncated_formatted, countof(truncated_formatted), "n: %d, s: %s", 42, "a long test string");
    swear(truncated_formatted[8] == 0);
    char very_short_str[1];
    very_short_str[0] = 0xFF;
    ut_str.format(very_short_str, countof(very_short_str), "%s", "test");
    swear(very_short_str[0] == 0);
    swear(ut_str.starts_with("example text", 7, "exam", 4));
    swear(ut_str.starts_with_nc("example text", 7, "ExAm", 4));
    swear(ut_str.ends_with("testing", 7, "ing", 3));
    swear(ut_str.ends_with_nc("testing", 7, "InG", 3));
    swear(ut_str.compare("hello", 5, "hello", 5) == 0);
    swear(ut_str.compare_nc("Hello", 5, "hello", 5) == 0);
    swear(ut_str.compare("ab", 2, "abc", 3) < 0);
    swear(ut_str.compare_nc("abc", 3, "ABCD", 4) < 0);
    if (ut_debug.verbosity.level > ut_debug.verbosity.quiet) { traceln("done"); }
    #pragma pop_macro("glyph_ice_cube")
    #pragma pop_macro("glyph_teddy_bear")
    #pragma pop_macro("glyph_chinese_two")
    #pragma pop_macro("glyph_chinese_one")
}

#else

static void ut_str_test(void) {}

#endif

ut_str_if ut_str = {
    .error          = ut_str_error,
    .error_nls      = ut_str_error_nls,
    .utf8_bytes     = ut_str_utf8_bytes,
    .utf16_chars    = ut_str_utf16_chars,
    .utf16_utf8     = ut_str_utf16to8,
    .utf8_utf16     = ut_str_utf8to16,
    .format         = ut_str_format,
    .format_va      = ut_str_format_va,
    .is_empty       = ut_str_is_empty,
    .equal          = ut_str_equal,
    .equal_nc       = ut_str_equal_nc,
    .length         = ut_str_length,
    .copy           = ut_str_copy,
    .first_char     = ut_str_first_char,
    .last_char      = ut_str_last_char,
    .first          = ut_str_first,
    .to_lower       = ut_str_to_lower,
    .to_upper       = ut_str_to_upper,
    .compare        = ut_str_compare,
    .compare_nc     = ut_str_compare_nc,
    .starts_with    = ut_str_starts_with,
    .ends_with      = ut_str_ends_with,
    .starts_with_nc = ut_str_starts_with_nc,
    .ends_with_nc   = ut_str_ends_with_nc,
    .unquote        = ut_str_unquote,
    .test           = ut_str_test
};

// _______________________________ ut_streams.c _______________________________

static errno_t ut_streams_memory_read(ut_stream_if* stream, void* data, int64_t bytes,
        int64_t *transferred) {
    swear(bytes > 0);
    ut_stream_memory_if* s = (ut_stream_memory_if*)stream;
    swear(0 <= s->pos_read && s->pos_read <= s->bytes_read,
          "bytes: %lld stream .pos: %lld .bytes: %lld",
          bytes, s->pos_read, s->bytes_read);
    int64_t transfer = ut_min(bytes, s->bytes_read - s->pos_read);
    memcpy(data, (uint8_t*)s->data_read + s->pos_read, transfer);
    s->pos_read += transfer;
    if (transferred != null) { *transferred = transfer; }
    return 0;
}

static errno_t ut_streams_memory_write(ut_stream_if* stream, const void* data, int64_t bytes,
        int64_t *transferred) {
    swear(bytes > 0);
    ut_stream_memory_if* s = (ut_stream_memory_if*)stream;
    swear(0 <= s->pos_write && s->pos_write <= s->bytes_write,
          "bytes: %lld stream .pos: %lld .bytes: %lld",
          bytes, s->pos_write, s->bytes_write);
    bool overflow = s->bytes_write - s->pos_write <= 0;
    int64_t transfer = ut_min(bytes, s->bytes_write - s->pos_write);
    memcpy((uint8_t*)s->data_write + s->pos_write, data, transfer);
    s->pos_write += transfer;
    if (transferred != null) { *transferred = transfer; }
    return overflow ? ERROR_INSUFFICIENT_BUFFER : 0;
}

static void ut_streams_read_only(ut_stream_memory_if* s,
        const void* data, int64_t bytes) {
    s->stream.read = ut_streams_memory_read;
    s->stream.write = null;
    s->data_read = data;
    s->bytes_read = bytes;
    s->pos_read = 0;
    s->data_write = null;
    s->bytes_write = 0;
    s->pos_write = 0;
}

static void ut_streams_write_only(ut_stream_memory_if* s,
        void* data, int64_t bytes) {
    s->stream.read = null;
    s->stream.write = ut_streams_memory_write;
    s->data_read = null;
    s->bytes_read = 0;
    s->pos_read = 0;
    s->data_write = data;
    s->bytes_write = bytes;
    s->pos_write = 0;
}

static void ut_streams_read_write(ut_stream_memory_if* s,
        const void* read, int64_t read_bytes,
        void* write, int64_t write_bytes) {
    s->stream.read = ut_streams_memory_read;
    s->stream.write = ut_streams_memory_write;
    s->data_read = read;
    s->bytes_read = read_bytes;
    s->pos_read = 0;
    s->pos_read = 0;
    s->data_write = write;
    s->bytes_write = write_bytes;
    s->pos_write = 0;
}

#ifdef UT_TESTS

static void ut_streams_test(void) {
    {   // read test
        uint8_t memory[256];
        for (int32_t i = 0; i < countof(memory); i++) { memory[i] = (uint8_t)i; }
        for (int32_t i = 1; i < countof(memory) - 1; i++) {
            ut_stream_memory_if ms; // memory stream
            ut_streams.read_only(&ms, memory, sizeof(memory));
            uint8_t data[256];
            for (int32_t j = 0; j < countof(data); j++) { data[j] = 0xFF; }
            int64_t transferred = 0;
            errno_t r = ms.stream.read(&ms.stream, data, i, &transferred);
            swear(r == 0 && transferred == i);
            for (int32_t j = 0; j < i; j++) { swear(data[j] == memory[j]); }
            for (int32_t j = i; j < countof(data); j++) { swear(data[j] == 0xFF); }
        }
    }
    {   // write test
        // TODO: implement
    }
    {   // read/write test
        // TODO: implement
    }
    if (ut_debug.verbosity.level > ut_debug.verbosity.quiet) { traceln("done"); }
}

#else

static void ut_streams_test(void) { }

#endif

ut_streams_if ut_streams = {
    .read_only  = ut_streams_read_only,
    .write_only = ut_streams_write_only,
    .read_write = ut_streams_read_write,
    .test       = ut_streams_test
};

// _______________________________ ut_threads.c _______________________________

// events:

static event_t ut_event_create(void) {
    HANDLE e = CreateEvent(null, false, false, null);
    not_null(e);
    return (event_t)e;
}

static event_t ut_event_create_manual(void) {
    HANDLE e = CreateEvent(null, true, false, null);
    not_null(e);
    return (event_t)e;
}

static void ut_event_set(event_t e) {
    fatal_if_false(SetEvent((HANDLE)e));
}

static void ut_event_reset(event_t e) {
    fatal_if_false(ResetEvent((HANDLE)e));
}

static int32_t ut_event_wait_or_timeout(event_t e, fp64_t seconds) {
    uint32_t ms = seconds < 0 ? INFINITE : (int32_t)(seconds * 1000.0 + 0.5);
    DWORD ix = WaitForSingleObject(e, ms);
    errno_t r = wait2e(ix);
    return r != 0 ? -1 : 0;
}

static void ut_event_wait(event_t e) { ut_event_wait_or_timeout(e, -1); }

static int32_t ut_event_wait_any_or_timeout(int32_t n, event_t ut_event_[], fp64_t s) {
    uint32_t ms = s < 0 ? INFINITE : (int32_t)(s * 1000.0 + 0.5);
    DWORD ix = WaitForMultipleObjects(n, ut_event_, false, ms);
    errno_t r = wait2e(ix);
    // all WAIT_ABANDONED_0 and WAIT_IO_COMPLETION 0xC0 as -1
    return r != 0 ? -1 : ix;
}

static int32_t ut_event_wait_any(int32_t n, event_t e[]) {
    return ut_event_wait_any_or_timeout(n, e, -1);
}

static void ut_event_dispose(event_t handle) {
    fatal_if_false(CloseHandle(handle));
}

// test:

// check if the elapsed time is within the expected range
static void ut_event_test_check_time(fp64_t start, fp64_t expected) {
    fp64_t elapsed = ut_clock.seconds() - start;
    // Old Windows scheduler is prone to 2x16.6ms ~ 33ms delays
    swear(elapsed >= expected - 0.04 && elapsed <= expected + 0.04,
          "expected: %f elapsed %f seconds", expected, elapsed);
}

static void ut_event_test(void) {
    #ifdef UT_TESTS
    event_t event = ut_event.create();
    fp64_t start = ut_clock.seconds();
    ut_event.set(event);
    ut_event.wait(event);
    ut_event_test_check_time(start, 0); // Event should be immediate
    ut_event.reset(event);
    start = ut_clock.seconds();
    const fp64_t timeout_seconds = 0.01;
    int32_t result = ut_event.wait_or_timeout(event, timeout_seconds);
    ut_event_test_check_time(start, timeout_seconds);
    swear(result == -1); // Timeout expected
    enum { count = 5 };
    event_t event_array[count];
    for (int32_t i = 0; i < countof(event_array); i++) {
        event_array[i] = ut_event.create_manual();
    }
    start = ut_clock.seconds();
    ut_event.set(event_array[2]); // Set the third event
    int32_t index = ut_event.wait_any(countof(event_array), event_array);
    ut_event_test_check_time(start, 0);
    swear(index == 2); // Third event should be triggered
    ut_event.reset(event_array[2]); // Reset the third event
    start = ut_clock.seconds();
    result = ut_event.wait_any_or_timeout(countof(event_array),
        event_array, timeout_seconds);
    ut_event_test_check_time(start, timeout_seconds);
    swear(result == -1); // Timeout expected
    // Clean up
    ut_event.dispose(event);
    for (int32_t i = 0; i < countof(event_array); i++) {
        ut_event.dispose(event_array[i]);
    }
    if (ut_debug.verbosity.level > ut_debug.verbosity.quiet) { traceln("done"); }
    #endif
}

ut_event_if ut_event = {
    .create              = ut_event_create,
    .create_manual       = ut_event_create_manual,
    .set                 = ut_event_set,
    .reset               = ut_event_reset,
    .wait                = ut_event_wait,
    .wait_or_timeout     = ut_event_wait_or_timeout,
    .wait_any            = ut_event_wait_any,
    .wait_any_or_timeout = ut_event_wait_any_or_timeout,
    .dispose             = ut_event_dispose,
    .test                = ut_event_test
};

// mutexes:

static_assertion(sizeof(CRITICAL_SECTION) == sizeof(mutex_t));

static void ut_mutex_init(mutex_t* m) {
    CRITICAL_SECTION* cs = (CRITICAL_SECTION*)m;
    fatal_if_false(
        InitializeCriticalSectionAndSpinCount(cs, 4096)
    );
}

static void ut_mutex_lock(mutex_t* m) { EnterCriticalSection((CRITICAL_SECTION*)m); }

static void ut_mutex_unlock(mutex_t* m) { LeaveCriticalSection((CRITICAL_SECTION*)m); }

static void ut_mutex_dispose(mutex_t* m) { DeleteCriticalSection((CRITICAL_SECTION*)m); }

// test:

// check if the elapsed time is within the expected range
static void ut_mutex_test_check_time(fp64_t start, fp64_t expected) {
    fp64_t elapsed = ut_clock.seconds() - start;
    // Old Windows scheduler is prone to 2x16.6ms ~ 33ms delays
    swear(elapsed >= expected - 0.04 && elapsed <= expected + 0.04,
          "expected: %f elapsed %f seconds", expected, elapsed);
}

static void ut_mutex_test_lock_unlock(void* arg) {
    mutex_t* mutex = (mutex_t*)arg;
    ut_mutex.lock(mutex);
    ut_thread.sleep_for(0.01); // Hold the mutex for 10ms
    ut_mutex.unlock(mutex);
}

static void ut_mutex_test(void) {
    mutex_t mutex;
    ut_mutex.init(&mutex);
    fp64_t start = ut_clock.seconds();
    ut_mutex.lock(&mutex);
    ut_mutex.unlock(&mutex);
    // Lock and unlock should be immediate
    ut_mutex_test_check_time(start, 0);
    enum { count = 5 };
    thread_t ts[count];
    for (int32_t i = 0; i < countof(ts); i++) {
        ts[i] = ut_thread.start(ut_mutex_test_lock_unlock, &mutex);
    }
    // Wait for all threads to finish
    for (int32_t i = 0; i < countof(ts); i++) {
        ut_thread.join(ts[i], -1);
    }
    ut_mutex.dispose(&mutex);
    if (ut_debug.verbosity.level > ut_debug.verbosity.quiet) { traceln("done"); }
}

ut_mutex_if ut_mutex = {
    .init    = ut_mutex_init,
    .lock    = ut_mutex_lock,
    .unlock  = ut_mutex_unlock,
    .dispose = ut_mutex_dispose,
    .test    = ut_mutex_test
};

// threads:

static void* ut_thread_ntdll(void) {
    static HMODULE ntdll;
    if (ntdll == null) {
        ntdll = (void*)GetModuleHandleA("ntdll.dll");
    }
    if (ntdll == null) {
        ntdll = ut_loader.open("ntdll.dll", 0);
    }
    not_null(ntdll);
    return ntdll;
}

static fp64_t ut_thread_ns2ms(int64_t ns) {
    return ns / (fp64_t)ut_clock.nsec_in_msec;
}

static void ut_thread_set_timer_resolution(uint64_t nanoseconds) {
    typedef int32_t (*query_timer_resolution_t)(ULONG* minimum_resolution,
        ULONG* maximum_resolution, ULONG* actual_resolution);
    typedef int32_t (*set_timer_resolution_t)(ULONG requested_resolution,
        BOOLEAN set, ULONG* actual_resolution); // ntdll.dll
    void* nt_dll = ut_thread_ntdll();
    query_timer_resolution_t query_timer_resolution =  (query_timer_resolution_t)
        ut_loader.sym(nt_dll, "NtQueryTimerResolution");
    set_timer_resolution_t set_timer_resolution = (set_timer_resolution_t)
        ut_loader.sym(nt_dll, "NtSetTimerResolution");
    unsigned long min100ns = 16 * 10 * 1000;
    unsigned long max100ns =  1 * 10 * 1000;
    unsigned long cur100ns =  0;
    fatal_if(query_timer_resolution(&min100ns, &max100ns, &cur100ns) != 0);
    uint64_t max_ns = max100ns * 100uLL;
//  uint64_t min_ns = min100ns * 100uLL;
//  uint64_t cur_ns = cur100ns * 100uLL;
    // max resolution is lowest possible delay between timer events
//  if (ut_debug.verbosity.level >= ut_debug.verbosity.trace) {
//      traceln("timer resolution min: %.3f max: %.3f cur: %.3f"
//          " ms (milliseconds)",
//          ut_thread_ns2ms(min_ns),
//          ut_thread_ns2ms(max_ns),
//          ut_thread_ns2ms(cur_ns));
//  }
    // note that maximum resolution is actually < minimum
    nanoseconds = ut_max(max_ns, nanoseconds);
    unsigned long ns = (unsigned long)((nanoseconds + 99) / 100);
    fatal_if(set_timer_resolution(ns, true, &cur100ns) != 0);
    fatal_if(query_timer_resolution(&min100ns, &max100ns, &cur100ns) != 0);
//  if (ut_debug.verbosity.level >= ut_debug.verbosity.trace) {
//      min_ns = min100ns * 100uLL;
//      max_ns = max100ns * 100uLL; // the smallest interval
//      cur_ns = cur100ns * 100uLL;
//      traceln("timer resolution min: %.3f max: %.3f cur: %.3f ms (milliseconds)",
//          ut_thread_ns2ms(min_ns),
//          ut_thread_ns2ms(max_ns),
//          ut_thread_ns2ms(cur_ns));
//  }
}

static void ut_thread_power_throttling_disable_for_process(void) {
    static bool disabled_for_the_process;
    if (!disabled_for_the_process) {
        PROCESS_POWER_THROTTLING_STATE pt = { 0 };
        pt.Version = PROCESS_POWER_THROTTLING_CURRENT_VERSION;
        pt.ControlMask = PROCESS_POWER_THROTTLING_EXECUTION_SPEED;
        pt.StateMask = 0;
        fatal_if_false(SetProcessInformation(GetCurrentProcess(),
            ProcessPowerThrottling, &pt, sizeof(pt)));
        // PROCESS_POWER_THROTTLING_IGNORE_TIMER_RESOLUTION
        // does not work on Win10. There is no easy way to
        // distinguish Windows 11 from 10 (Microsoft great engineering)
        pt.ControlMask = PROCESS_POWER_THROTTLING_IGNORE_TIMER_RESOLUTION;
        pt.StateMask = 0;
        // ignore error on Windows 10:
        (void)SetProcessInformation(GetCurrentProcess(),
            ProcessPowerThrottling, &pt, sizeof(pt));
        disabled_for_the_process = true;
    }
}

static void ut_thread_power_throttling_disable_for_thread(HANDLE thread) {
    THREAD_POWER_THROTTLING_STATE pt = { 0 };
    pt.Version = THREAD_POWER_THROTTLING_CURRENT_VERSION;
    pt.ControlMask = THREAD_POWER_THROTTLING_EXECUTION_SPEED;
    pt.StateMask = 0;
    fatal_if_false(SetThreadInformation(thread, ThreadPowerThrottling,
        &pt, sizeof(pt)));
}

static void ut_thread_disable_power_throttling(void) {
    ut_thread_power_throttling_disable_for_process();
    ut_thread_power_throttling_disable_for_thread(GetCurrentThread());
}

static const char* ut_thread_rel2str(int32_t rel) {
    switch (rel) {
        case RelationProcessorCore   : return "ProcessorCore   ";
        case RelationNumaNode        : return "NumaNode        ";
        case RelationCache           : return "Cache           ";
        case RelationProcessorPackage: return "ProcessorPackage";
        case RelationGroup           : return "Group           ";
        case RelationProcessorDie    : return "ProcessorDie    ";
        case RelationNumaNodeEx      : return "NumaNodeEx      ";
        case RelationProcessorModule : return "ProcessorModule ";
        default: assert(false, "fix me"); return "???";
    }
}

static uint64_t ut_thread_next_physical_processor_affinity_mask(void) {
    static volatile int32_t initialized;
    static int32_t init;
    static int32_t next = 1; // next physical core to use
    static int32_t cores = 0; // number of physical processors (cores)
    static uint64_t any;
    static uint64_t affinity[64]; // mask for each physical processor
    bool set_to_true = ut_atomics.compare_exchange_int32(&init, false, true);
    if (set_to_true) {
        // Concept D: 6 cores, 12 logical processors: 27 lpi entries
        static SYSTEM_LOGICAL_PROCESSOR_INFORMATION lpi[64];
        DWORD bytes = 0;
        GetLogicalProcessorInformation(null, &bytes);
        assert(bytes % sizeof(lpi[0]) == 0);
        // number of lpi entries == 27 on 6 core / 12 logical processors system
        int32_t n = bytes / sizeof(lpi[0]);
        assert(bytes <= sizeof(lpi), "increase lpi[%d]", n);
        fatal_if_false(GetLogicalProcessorInformation(&lpi[0], &bytes));
        for (int32_t i = 0; i < n; i++) {
//          if (ut_debug.verbosity.level >= ut_debug.verbosity.trace) {
//              traceln("[%2d] affinity mask 0x%016llX relationship=%d %s", i,
//                  lpi[i].ProcessorMask, lpi[i].Relationship,
//                  ut_thread_rel2str(lpi[i].Relationship));
//          }
            if (lpi[i].Relationship == RelationProcessorCore) {
                assert(cores < countof(affinity), "increase affinity[%d]", cores);
                if (cores < countof(affinity)) {
                    any |= lpi[i].ProcessorMask;
                    affinity[cores] = lpi[i].ProcessorMask;
                    cores++;
                }
            }
        }
        initialized = true;
    } else {
        while (initialized == 0) { ut_thread.sleep_for(1 / 1024.0); }
        assert(any != 0); // should not ever happen
        if (any == 0) { any = (uint64_t)(-1LL); }
    }
    uint64_t mask = next < cores ? affinity[next] : any;
    assert(mask != 0);
    // assume last physical core is least popular
    if (next < cores) { next++; } // not circular
    return mask;
}

static void ut_thread_realtime(void) {
    fatal_if_false(SetPriorityClass(GetCurrentProcess(),
        REALTIME_PRIORITY_CLASS));
    fatal_if_false(SetThreadPriority(GetCurrentThread(),
        THREAD_PRIORITY_TIME_CRITICAL));
    fatal_if_false(SetThreadPriorityBoost(GetCurrentThread(),
        /* bDisablePriorityBoost = */ false));
    // desired: 0.5ms = 500us (microsecond) = 50,000ns
    ut_thread_set_timer_resolution(ut_clock.nsec_in_usec * 500);
    fatal_if_false(SetThreadAffinityMask(GetCurrentThread(),
        ut_thread_next_physical_processor_affinity_mask()));
    ut_thread_disable_power_throttling();
}

static void ut_thread_yield(void) { SwitchToThread(); }

static thread_t ut_thread_start(void (*func)(void*), void* p) {
    thread_t t = (thread_t)CreateThread(null, 0,
        (LPTHREAD_START_ROUTINE)(void*)func, p, 0, null);
    not_null(t);
    return t;
}

static bool is_handle_valid(void* h) {
    DWORD flags = 0;
    return GetHandleInformation(h, &flags);
}

static errno_t ut_thread_join(thread_t t, fp64_t timeout) {
    not_null(t);
    fatal_if_false(is_handle_valid(t));
    int32_t timeout_ms = timeout < 0 ? INFINITE : (int)(timeout * 1000.0 + 0.5);
    DWORD ix = WaitForSingleObject(t, timeout_ms);
    errno_t r = wait2e(ix);
    assert(r != ERROR_REQUEST_ABORTED, "AFAIK thread can`t be ABANDONED");
    if (r == 0) {
        fatal_if_false(CloseHandle(t));
    } else {
        traceln("failed to join thread %p %s", t, ut_str.error(r));
    }
    return r;
}

static void ut_thread_detach(thread_t t) {
    not_null(t);
    fatal_if_false(is_handle_valid(t));
    fatal_if_false(CloseHandle(t));
}

static void ut_thread_name(const char* name) {
    uint16_t stack[1024];
    fatal_if(ut_str.length(name) >= countof(stack), "name too long: %s", name);
    uint16_t* wide = ut_str.utf8_utf16(stack, name);
    HRESULT r = SetThreadDescription(GetCurrentThread(), wide);
    // notoriously returns 0x10000000 for no good reason whatsoever
    if (!SUCCEEDED(r)) { fatal_if_not_zero(r); }
}

static void ut_thread_sleep_for(fp64_t seconds) {
    assert(seconds >= 0);
    if (seconds < 0) { seconds = 0; }
    int64_t ns100 = (int64_t)(seconds * 1.0e+7); // in 0.1 us aka 100ns
    typedef int32_t (__stdcall *nt_delay_execution_t)(BOOLEAN alertable,
        PLARGE_INTEGER DelayInterval);
    static nt_delay_execution_t NtDelayExecution;
    // delay in 100-ns units. negative value means delay relative to current.
    LARGE_INTEGER delay = {0}; // delay in 100-ns units.
    delay.QuadPart = -ns100; // negative value means delay relative to current.
    if (NtDelayExecution == null) {
        void* ntdll = ut_thread_ntdll();
        NtDelayExecution = (nt_delay_execution_t)
            ut_loader.sym(ntdll, "NtDelayExecution");
        not_null(NtDelayExecution);
    }
    // If "alertable" is set, sleep_for() can break earlier
    // as a result of NtAlertThread call.
    NtDelayExecution(false, &delay);
}

static int32_t ut_thread_id(void) { return GetThreadId(GetCurrentThread()); }

#ifdef UT_TESTS

// test: https://en.wikipedia.org/wiki/Dining_philosophers_problem

typedef struct ut_thread_philosophers_s ut_thread_philosophers_t;

typedef struct {
    ut_thread_philosophers_t* ps;
    mutex_t  fork;
    mutex_t* left_fork;
    mutex_t* right_fork;
    thread_t thread;
    int32_t  id;
} ut_thread_philosopher_t;

typedef struct ut_thread_philosophers_s {
    ut_thread_philosopher_t philosopher[3];
    event_t fed_up[3];
    uint32_t seed;
    volatile bool enough;
} ut_thread_philosophers_t;

#pragma push_macro("verbose") // --verbosity trace

#define verbose(...) do {                                 \
    if (ut_debug.verbosity.level >= ut_debug.verbosity.trace) { \
        traceln(__VA_ARGS__);                             \
    }                                                     \
} while (0)

static void ut_thread_philosopher_think(ut_thread_philosopher_t* p) {
    verbose("philosopher %d is thinking.", p->id);
    // Random think time between .1 and .3 seconds
    fp64_t seconds = (ut_num.random32(&p->ps->seed) % 30 + 1) / 100.0;
    ut_thread.sleep_for(seconds);
}

static void ut_thread_philosopher_eat(ut_thread_philosopher_t* p) {
    verbose("philosopher %d is eating.", p->id);
    // Random eat time between .1 and .2 seconds
    fp64_t seconds = (ut_num.random32(&p->ps->seed) % 20 + 1) / 100.0;
    ut_thread.sleep_for(seconds);
}

// To avoid deadlocks in the Three Philosophers problem, we can implement
// the Tanenbaum's solution, which ensures that one of the philosophers
// (e.g., the last one) tries to pick up the right fork first, while the
// others pick up the left fork first. This breaks the circular wait
// condition and prevents deadlock.

// If the philosopher is the last one (p->id == n - 1) they will try to pick
// up the right fork first and then the left fork. All other philosophers will
// pick up the left fork first and then the right fork, as before. This change
// ensures that at least one philosopher will be able to eat, breaking the
// circular wait condition and preventing deadlock.

static void ut_thread_philosopher_routine(void* arg) {
    ut_thread_philosopher_t* p = (ut_thread_philosopher_t*)arg;
    enum { n = countof(p->ps->philosopher) };
    ut_thread.name("philosopher");
    ut_thread.realtime();
    while (!p->ps->enough) {
        ut_thread_philosopher_think(p);
        if (p->id == n - 1) { // Last philosopher picks up the right fork first
            ut_mutex.lock(p->right_fork);
            verbose("philosopher %d picked up right fork.", p->id);
            ut_mutex.lock(p->left_fork);
            verbose("philosopher %d picked up left fork.", p->id);
        } else { // Other philosophers pick up the left fork first
            ut_mutex.lock(p->left_fork);
            verbose("philosopher %d picked up left fork.", p->id);
            ut_mutex.lock(p->right_fork);
            verbose("philosopher %d picked up right fork.", p->id);
        }
        ut_thread_philosopher_eat(p);
        ut_mutex.unlock(p->right_fork);
        verbose("philosopher %d put down right fork.", p->id);
        ut_mutex.unlock(p->left_fork);
        verbose("philosopher %d put down left fork.", p->id);
        ut_event.set(p->ps->fed_up[p->id]);
    }
}

static void ut_thread_detached_sleep(void* unused(p)) {
    ut_thread.sleep_for(1000.0); // seconds
}

static void ut_thread_detached_loop(void* unused(p)) {
    uint64_t sum = 0;
    for (uint64_t i = 0; i < UINT64_MAX; i++) { sum += i; }
    // making sure that compiler won't get rid of the loop:
    traceln("%lld", sum);
}

static void ut_thread_test(void) {
    ut_thread_philosophers_t ps = { .seed = 1 };
    enum { n = countof(ps.philosopher) };
    // Initialize mutexes for forks
    for (int32_t i = 0; i < n; i++) {
        ut_thread_philosopher_t* p = &ps.philosopher[i];
        p->id = i;
        p->ps = &ps;
        ut_mutex.init(&p->fork);
        p->left_fork = &p->fork;
        ps.fed_up[i] = ut_event.create();
    }
    // Create and start philosopher threads
    for (int32_t i = 0; i < n; i++) {
        ut_thread_philosopher_t* p = &ps.philosopher[i];
        ut_thread_philosopher_t* r = &ps.philosopher[(i + 1) % n];
        p->right_fork = r->left_fork;
        p->thread = ut_thread.start(ut_thread_philosopher_routine, p);
    }
    // wait for all philosophers being fed up:
    for (int32_t i = 0; i < n; i++) { ut_event.wait(ps.fed_up[i]); }
    ps.enough = true;
    // join all philosopher threads
    for (int32_t i = 0; i < n; i++) {
        ut_thread_philosopher_t* p = &ps.philosopher[i];
        ut_thread.join(p->thread, -1);
    }
    // Dispose of mutexes and events
    for (int32_t i = 0; i < n; ++i) {
        ut_thread_philosopher_t* p = &ps.philosopher[i];
        ut_mutex.dispose(&p->fork);
        ut_event.dispose(ps.fed_up[i]);
    }
    // detached threads are hacky and not that swell of an idea
    // but sometimes can be useful for 1. quick hacks 2. threads
    // that execute blocking calls that e.g. write logs to the
    // internet service that hangs.
    // test detached threads
    thread_t detached_sleep = ut_thread.start(ut_thread_detached_sleep, null);
    ut_thread.detach(detached_sleep);
    thread_t detached_loop = ut_thread.start(ut_thread_detached_loop, null);
    ut_thread.detach(detached_loop);
    // leave detached threads sleeping and running till ExitProcess(0)
    // that should NOT hang.
    if (ut_debug.verbosity.level > ut_debug.verbosity.quiet) { traceln("done"); }
}

#pragma pop_macro("verbose")

#else
static void ut_thread_test(void) { }
#endif

ut_thread_if ut_thread = {
    .start     = ut_thread_start,
    .join      = ut_thread_join,
    .detach    = ut_thread_detach,
    .name      = ut_thread_name,
    .realtime  = ut_thread_realtime,
    .yield     = ut_thread_yield,
    .sleep_for = ut_thread_sleep_for,
    .id        = ut_thread_id,
    .test      = ut_thread_test
};
// ________________________________ ut_vigil.c ________________________________

#include <stdio.h>
#include <string.h>

static void vigil_breakpoint_and_abort(void) {
    ut_debug.breakpoint(); // only if debugger is present
    ut_runtime.abort();
}

static int32_t vigil_failed_assertion(const char* file, int32_t line,
        const char* func, const char* condition, const char* format, ...) {
    va_list vl;
    va_start(vl, format);
    ut_debug.println_va(file, line, func, format, vl);
    va_end(vl);
    ut_debug.println(file, line, func, "assertion failed: %s\n", condition);
    // avoid warnings: conditional expression always true and unreachable code
    const bool always_true = ut_runtime.abort != null;
    if (always_true) { vigil_breakpoint_and_abort(); }
    return 0;
}

static int32_t vigil_fatal_termination(const char* file, int32_t line,
        const char* func, const char* condition, const char* format, ...) {
    const int32_t er = ut_runtime.err();
    const int32_t en = errno;
    va_list vl;
    va_start(vl, format);
    ut_debug.println_va(file, line, func, format, vl);
    va_end(vl);
    // report last errors:
    if (er != 0) { ut_debug.perror(file, line, func, er, ""); }
    if (en != 0) { ut_debug.perrno(file, line, func, en, ""); }
    if (condition != null && condition[0] != 0) {
        ut_debug.println(file, line, func, "FATAL: %s\n", condition);
    } else {
        ut_debug.println(file, line, func, "FATAL\n");
    }
    const bool always_true = ut_runtime.abort != null;
    if (always_true) { vigil_breakpoint_and_abort(); }
    return 0;
}

#ifdef UT_TESTS

static vigil_if vigil_test_saved;
static int32_t  vigil_test_failed_assertion_count;

#pragma push_macro("vigil")
// intimate knowledge of vigil.*() functions used in macro definitions
#define vigil vigil_test_saved

static int32_t vigil_test_failed_assertion(const char* file, int32_t line,
        const char* func, const char* condition, const char* format, ...) {
    fatal_if_not(strequ(file,  __FILE__), "file: %s", file);
    fatal_if_not(line > __LINE__, "line: %s", line);
    assert(strequ(func, "vigil_test"), "func: %s", func);
    fatal_if(condition == null || condition[0] == 0);
    fatal_if(format == null || format[0] == 0);
    vigil_test_failed_assertion_count++;
    if (ut_debug.verbosity.level >= ut_debug.verbosity.trace) {
        va_list vl;
        va_start(vl, format);
        ut_debug.println_va(file, line, func, format, vl);
        va_end(vl);
        ut_debug.println(file, line, func, "assertion failed: %s (expected)\n",
                     condition);
    }
    return 0;
}

static int32_t vigil_test_fatal_calls_count;

static int32_t vigil_test_fatal_termination(const char* file, int32_t line,
        const char* func, const char* condition, const char* format, ...) {
    const int32_t er = ut_runtime.err();
    const int32_t en = errno;
    assert(er == 2, "ut_runtime.err: %d expected 2", er);
    assert(en == 2, "errno: %d expected 2", en);
    fatal_if_not(strequ(file,  __FILE__), "file: %s", file);
    fatal_if_not(line > __LINE__, "line: %s", line);
    assert(strequ(func, "vigil_test"), "func: %s", func);
    assert(strequ(condition, "")); // not yet used expected to be ""
    assert(format != null && format[0] != 0);
    vigil_test_fatal_calls_count++;
    if (ut_debug.verbosity.level > ut_debug.verbosity.trace) {
        va_list vl;
        va_start(vl, format);
        ut_debug.println_va(file, line, func, format, vl);
        va_end(vl);
        if (er != 0) { ut_debug.perror(file, line, func, er, ""); }
        if (en != 0) { ut_debug.perrno(file, line, func, en, ""); }
        if (condition != null && condition[0] != 0) {
            ut_debug.println(file, line, func, "FATAL: %s (testing)\n", condition);
        } else {
            ut_debug.println(file, line, func, "FATAL (testing)\n");
        }
    }
    return 0;
}

#pragma pop_macro("vigil")

static void vigil_test(void) {
    vigil_test_saved = vigil;
    int32_t en = errno;
    int32_t er = ut_runtime.err();
    errno = 2; // ENOENT
    ut_runtime.seterr(2); // ERROR_FILE_NOT_FOUND
    vigil.failed_assertion  = vigil_test_failed_assertion;
    vigil.fatal_termination = vigil_test_fatal_termination;
    int32_t count = vigil_test_fatal_calls_count;
    fatal("testing: %s call", "fatal()");
    assert(vigil_test_fatal_calls_count == count + 1);
    count = vigil_test_failed_assertion_count;
    assert(false, "testing: assert(%s)", "false");
    #ifdef DEBUG // verify that assert() is only compiled in DEBUG:
        fatal_if_not(vigil_test_failed_assertion_count == count + 1);
    #else // not RELEASE buid:
        fatal_if_not(vigil_test_failed_assertion_count == count);
    #endif
    count = vigil_test_failed_assertion_count;
    swear(false, "testing: swear(%s)", "false");
    // swear() is triggered in both debug and release configurations:
    fatal_if_not(vigil_test_failed_assertion_count == count + 1);
    errno = en;
    ut_runtime.seterr(er);
    vigil = vigil_test_saved;
    if (ut_debug.verbosity.level > ut_debug.verbosity.quiet) { traceln("done"); }
}

#else

static void vigil_test(void) { }

#endif

vigil_if vigil = {
    .failed_assertion  = vigil_failed_assertion,
    .fatal_termination = vigil_fatal_termination,
    .test = vigil_test
};

#endif // ut_implementation

