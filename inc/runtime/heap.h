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
    void    (*test)(void);
} heap_if;

extern heap_if heap;

// *) zero in reallocate applies to the newly appended bytes

// On Windows mem.heap is based on serialized LFH returned by GetProcessHeap()
// https://learn.microsoft.com/en-us/windows/win32/memory/low-fragmentation-heap
// threads can benefit from not serialized, not LFH if they allocate and free
// memory in time critical loops.

end_c

