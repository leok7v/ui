#pragma once
#include "util/manifest.h"

begin_c

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
} mem_if;

extern mem_if mem;

end_c

