#pragma once

#include "manifest.h"
#include "args.h"
#include "atomics.h"
#include "clock.h"
#include "debug.h"
#include "events.h"
#include "mem.h"
#include "mutexes.h"
#include "num.h"
#include "static.h"
#include "str.h"
#include "threads.h"
#include "va_args.h"
#include "vigil.h"

#define crt_version 20240318 // YYYYMMDD

begin_c

typedef struct {
    int32_t (*err)(void); // errno or GetLastError()
    void (*seterr)(int32_t err); // errno = err or SetLastError()
    void (*abort)(void);
    void (*exit)(int32_t exit_code); // only 8 bits on posix
    void* (*dlopen)(const char* filename, int32_t mode); // RTLD_LOCAL == 0
    void* (*dlsym)(void* handle, const char* name);
    void  (*dlclose)(void* handle);
    // persistent storage interface:
    void (*data_save)(const char* name, const char* key, const void* data, int32_t bytes);
    int32_t (*data_size)(const char* name, const char* key);
    int  (*data_load)(const char* name, const char* key, void* data, int32_t bytes);
    void (*test)(int32_t verbosity);
} crt_if;

extern crt_if crt;

end_c

