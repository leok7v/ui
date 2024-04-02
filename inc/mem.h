#pragma once
#include "manifest.h"

begin_c

typedef struct {
    int (*map_read)(const char* filename, void** data, int64_t* bytes);
    int (*map_rw)(const char* filename, void** data, int64_t* bytes);
    void (*unmap)(void* data, int64_t bytes);
    // map_resource() maps data from resources, do NOT unmap!
    int  (*map_resource)(const char* label, void** data, int64_t* bytes);
    void (*test)(void);
} mem_if;

extern mem_if mem;

end_c

