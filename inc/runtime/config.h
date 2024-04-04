#pragma once
#include "manifest.h"

begin_c

// Persistent storage for configuration and other small data
// related to specific application.
// on Unix-like system ~/.name/key files are used.
// On Window User registry (could be .dot files/folders).
// "name" is customary basename of "args.v[0]"

typedef struct {
    void    (*save)(const char* name, const char* key,
                    const void* data, int32_t bytes);
    int32_t (*size)(const char* name, const char* key);
    int32_t (*load)(const char* name, const char* key,
                    void* data, int32_t bytes);
    void (*test)(void);
} config_if;

extern config_if config;

end_c

