#pragma once

#include "manifest.h"

begin_c

// see:
// https://pubs.opengroup.org/onlinepubs/7908799/xsh/dlfcn.h.html

typedef struct {
    const int local;
    const int lazy;
    const int now;
    const int global;
    // "If the value of file is null, dlopen() provides a handle on a global
    //  symbol object." posix
    void* (*open)(const char* filename, int32_t mode);
    void* (*sym)(void* handle, const char* name);
    void  (*close)(void* handle);
    void (*test)(int32_t verbosity);
} loader_if;

extern loader_if loader;

end_c
