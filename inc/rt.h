#pragma once

#include "win32.h"
#include "manifest.h"
#include "args.h"
#include "atomics.h"
#include "events.h"
#include "vigil.h"
#include "mutexes.h"
#include "threads.h"
#include "trace.h"

#include <ctype.h>
#include <io.h>
#include <malloc.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define crt_version 20240318 // YYYYMMDD

begin_c

// Frequent Code Analysis false positive warnings
// Mainly because for stackalloc() and casted void* pointers
// compiler cannot infer the size of accessible memory.

#define __suppress_alloca_warnings__ _Pragma("warning(suppress: 6255 6263)")
#define __suppress_buffer_overrun__  _Pragma("warning(suppress: 6386)")

#define stackalloc(bytes) (__suppress_alloca_warnings__ alloca(bytes))
#define zero_initialized_stackalloc(bytes) memset(stackalloc(bytes), 0, (bytes))

// Since a lot of str*() operations are preprocessor defines
// care should be exercised that arguments of macro invocations
// do not have side effects or not computationally expensive.
// None of the definitions are performance champions - if the
// code needs extreme cpu cycle savings working with utf8 strings

#define strempty(s) ((s) == null || (s)[0] == 0)

#define strconcat(a, b) __suppress_buffer_overrun__ \
    (strcat(strcpy((char*)stackalloc(strlen(a) + strlen(b) + 1), (a)), (b)))

#define strequ(s1, s2)  (((void*)(s1) == (void*)(s2)) || \
    (((void*)(s1) != null && (void*)(s2) != null) && strcmp((s1), (s2)) == 0))

#define striequ(s1, s2)  (((void*)(s1) == (void*)(s2)) || \
    (((void*)(s1) != null && (void*)(s2) != null) && stricmp((s1), (s2)) == 0))

#define strendswith(s1, s2) \
    (strlen(s1) >= strlen(s2) && strcmp((s1) + strlen(s1) - strlen(s2), (s2)) == 0)

#define strlength(s) ((int)strlen(s)) // avoid code analysis noise
// a lot of posix like API consumes "int" instead of size_t which
// is acceptable for majority of char* zero terminated strings usage
// since most of them work with filepath that are relatively short
// and on Windows are limited to 260 chars or 32KB - 1 chars.

#define strcopy(s1, s2) /* use with extreme caution */                      \
    do {                                                                   \
        strncpy((s1), (s2), countof((s1)) - 1); s1[countof((s1)) - 1] = 0; \
} while (0)

const char* _strtolc_(char* d, const char* s);

char* strnchr(const char* s, int n, char ch);

#define strtolowercase(s) _strtolc_((char*)stackalloc(strlen(s) + 1), s)

#define utf16to8(utf16) crt.utf16_utf8((char*) \
    stackalloc((size_t)crt.utf8_bytes(utf16) + 1), utf16)

#define utf8to16(s) crt.utf8_utf16((wchar_t*)stackalloc((crt.utf16_chars(s) + 1) * \
    sizeof(wchar_t)), s)

#define strprintf(s, ...) crt.sformat((s), countof(s), "" __VA_ARGS__)

#define strstartswith(a, b) \
    (strlen(a) >= strlen(b) && memcmp((a), (b), strlen(b)) == 0)

enum {
    NSEC_IN_USEC = 1000,
    NSEC_IN_MSEC = NSEC_IN_USEC * 1000,
    NSEC_IN_SEC  = NSEC_IN_MSEC * 1000
};

typedef struct {
    int32_t (*err)(void); // errno or GetLastError()
    void (*seterr)(int32_t err); // errno = err or SetLastError()
    // non-crypto strong pseudo-random number generators (thread safe)
    uint32_t (*random32)(uint32_t *state); // "Mulberry32"
    uint64_t (*random64)(uint64_t *state); // "Trust"
    // "FNV-1a" hash functions (if bytes == 0 expects zero terminated string)
    uint32_t (*hash32)(const char* s, int64_t bytes);
    uint64_t (*hash64)(const char* s, int64_t bytes);
    int (*memmap_read)(const char* filename, void** data, int64_t* bytes);
    int (*memmap_rw)(const char* filename, void** data, int64_t* bytes);
    void (*memunmap)(void* data, int64_t bytes);
    // memmap_res() maps data from resources, do NOT unmap!
    int (*memmap_res)(const char* label, void** data, int64_t* bytes);
    void (*sleep)(double seconds);
    double (*seconds)(void); // since boot
    int64_t (*nanoseconds)(void); // since boot
    uint64_t (*microseconds)(void); // NOT monotonic(!) UTC since epoch January 1, 1601
    uint64_t (*localtime)(void);    // local time microseconds since epoch
    void (*time_utc)(uint64_t microseconds, int* year, int* month,
        int* day, int* hh, int* mm, int* ss, int* ms, int* mc);
    void (*time_local)(uint64_t microseconds, int* year, int* month,
        int* day, int* hh, int* mm, int* ss, int* ms, int* mc);
    // persistent storage interface:
    void (*data_save)(const char* name, const char* key, const void* data, int bytes);
    int  (*data_size)(const char* name, const char* key);
    int  (*data_load)(const char* name, const char* key, void* data, int bytes);
    // string formatting printf style:
    void (*vformat)(char* utf8, int count, const char* format, va_list vl);
    void (*sformat)(char* utf8, int count, const char* format, ...);
    const char* (*error)(int32_t error); // en-US
    const char* (*error_nls)(int32_t error); // national locale string
    // do not call directly used by macros above
    int (*utf8_bytes)(const wchar_t* utf16);
    int (*utf16_chars)(const char* s);
    char* (*utf16_utf8)(char* destination, const wchar_t* utf16);
    wchar_t* (*utf8_utf16)(wchar_t* destination, const char* utf8);
    void (*ods)(const char* file, int line, const char* function,
        const char* format, ...); // Output Debug String
    void (*traceline)(const char* file, int line, const char* function,
        const char* format, ...);
    void (*vtraceline)(const char* file, int line, const char* function,
        const char* format, va_list vl);
    void (*breakpoint)(void);
    int (*gettid)(void);
    int (*assertion_failed)(const char* file, int line, const char* function,
                         const char* condition, const char* format, ...);
    void (*fatal)(const char* file, int line, const char* function,
        const char* (*err2str)(int32_t error), int32_t error,
        const char* call, const char* extra);
} crt_if;

extern crt_if crt;

end_c

