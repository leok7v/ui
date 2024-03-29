#pragma once

#include "win32.h"
#include "manifest.h"
#include "args.h"
#include "atomics.h"
#include "debug.h"
#include "events.h"
#include "vigil.h"
#include "mutexes.h"
#include "static.h"
#include "str.h"
#include "threads.h"
#include "va_args.h"

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

enum {
    NSEC_IN_USEC = 1000,
    NSEC_IN_MSEC = NSEC_IN_USEC * 1000,
    NSEC_IN_SEC  = NSEC_IN_MSEC * 1000
};

typedef struct {
    int32_t (*err)(void); // errno or GetLastError()
    void (*seterr)(int32_t err); // errno = err or SetLastError()
    void (*abort)(void);
    void (*exit)(int32_t exit_code); // only 8 bits on posix
    void* (*dlopen)(const char* filename, int32_t mode); // RTLD_LOCAL == 0
    void* (*dlsym)(void* handle, const char* name);
    void  (*dlclose)(void* handle);
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
    void (*sleep)(double seconds);  // deprecated: threads.sleep_for()
    double (*seconds)(void); // since boot
    int64_t (*nanoseconds)(void); // since boot
    uint64_t (*microseconds)(void); // NOT monotonic(!) UTC since epoch January 1, 1601
    uint64_t (*localtime)(void);    // local time microseconds since epoch
    void (*time_utc)(uint64_t microseconds, int* year, int* month,
        int* day, int* hh, int* mm, int* ss, int* ms, int* mc);
    void (*time_local)(uint64_t microseconds, int* year, int* month,
        int* day, int* hh, int* mm, int* ss, int* ms, int* mc);
    // persistent storage interface:
    void (*data_save)(const char* name, const char* key, const void* data, int32_t bytes);
    int32_t (*data_size)(const char* name, const char* key);
    int  (*data_load)(const char* name, const char* key, void* data, int32_t bytes);
} crt_if;

extern crt_if crt;

end_c

