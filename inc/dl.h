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
    void* (*open)(const char* filename, int32_t mode);
    void* (*sym)(void* handle, const char* name);
    void  (*close)(void* handle);
    void (*test)(int32_t verbosity);
} dl_if;

extern dl_if dl;

end_c
