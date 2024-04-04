#pragma once
#include "manifest.h"

begin_c

// Persistent storage for configuration files or other small data
// related to specific application.
// on Unix-like system ~/.app/name files are used.
// On Window User registry (could be .dot files/folders).
// "app" is customary basename of "args.v[0]"

typedef struct {
    void (*save)(const char* name, const char* key,
                 const void* data, int32_t bytes);
    int32_t (*size)(const char* name, const char* key);
    int32_t (*load)(const char* name, const char* key,
                    void* data, int32_t bytes);
    void (*test)(void);
} dotfiles_if;

extern dotfiles_if dotfiles;

end_c

