#pragma once
#include "manifest.h"

begin_c

typedef struct heap_s heap_t;

typedef struct { // heap == null uses process serialized LFH
    heap_t* (*create)(bool serialized);
    void*   (*allocate)(heap_t* heap, int64_t bytes, bool zero);
    // reallocate may return ERROR_OUTOFMEMORY w/o changing 'a' *)
    errno_t (*reallocate)(heap_t* heap, void* *a, int64_t bytes, bool zero);
    void    (*deallocate)(heap_t* heap, void* a);
    int64_t (*bytes)(heap_t* heap, void* a); // actual allocated size
    void    (*dispose)(heap_t* heap);
} heap_if;

// *) zero in reallocate applies to the newly appended bytes

typedef struct {
    heap_if heap; // process heap (see notes below)
    // whole file read only
    errno_t (*map_ro)(const char* filename, void** data, int64_t* bytes);
    // whole file read-write
    errno_t (*map_rw)(const char* filename, void** data, int64_t* bytes);
    void (*unmap)(void* data, int64_t bytes);
    // map_resource() maps data from resources, do NOT unmap!
    errno_t  (*map_resource)(const char* label, void** data, int64_t* bytes);
    int32_t (*page_size)(void); // 4KB or 64KB on Windows
    int32_t (*large_page_size)(void);  // 2MB on Windows
    /* allocate contiguous reserved virtual address range,
*      if possible committed to physical memory.
       Memory guaranteed to be aligned to page boundary.
       Memory is guaranteed to be initialized to zero on access.
    */
    void* (*alloc_pages)(int64_t bytes_multiple_of_page_size);
    void  (*free_pages)(void* a, int64_t bytes_multiple_of_page_size);
    void  (*test)(void);
} mem_if;

extern mem_if mem;

// On Windows mem.heap is based on serialized LFH returned by GetProcessHeap()
// https://learn.microsoft.com/en-us/windows/win32/memory/low-fragmentation-heap
// threads can benefit from not serialized, not LFH if they allocate and free
// memory in time critical loops.

end_c

