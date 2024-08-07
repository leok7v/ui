#pragma once
#include "rt/rt_std.h"

rt_begin_c

// It is absolutely OK to use posix compliant
// malloc()/calloc()/realloc()/free() function calls with understanding
// that they introduce serialization points in multi-threaded applications
// and may be induce wait states that under pressure (all cores busy) may
// result in prolonged wait which may not be acceptable for real time
// processing pipelines.
//
// heap_if.functions may or may not be faster than malloc()/free() ...
//
// Some callers may find realloc() parameters more convenient to avoid
// anti-pattern
//      void* reallocated = realloc(p, new_size);
//      if (reallocated != null) { p = reallocated; }
// and avoid never ending discussion of legality and implementation
// compliance of the situation:
//      realloc(p /* when p == null */, ...)
//
// zero: true initializes allocated or reallocated tail memory to 0x00
// be careful with zeroing heap memory. It will result in virtual
// to physical memory mapping and may be expensive.

typedef struct rt_heap_s rt_heap_t;

typedef struct { // heap == null uses process serialized LFH
    errno_t (*alloc)(void* *a, int64_t bytes);
    errno_t (*alloc_zero)(void* *a, int64_t bytes);
    errno_t (*realloc)(void* *a, int64_t bytes);
    errno_t (*realloc_zero)(void* *a, int64_t bytes);
    void    (*free)(void* a);
    // heaps:
    rt_heap_t* (*create)(bool serialized);
    errno_t (*allocate)(rt_heap_t* heap, void* *a, int64_t bytes, bool zero);
    // reallocate may return ERROR_OUTOFMEMORY w/o changing 'a' *)
    errno_t (*reallocate)(rt_heap_t* heap, void* *a, int64_t bytes, bool zero);
    void    (*deallocate)(rt_heap_t* heap, void* a);
    int64_t (*bytes)(rt_heap_t* heap, void* a); // actual allocated size
    void    (*dispose)(rt_heap_t* heap);
    void    (*test)(void);
} rt_heap_if;

extern rt_heap_if rt_heap;

// *) zero in reallocate applies to the newly appended bytes

// On Windows rt_mem.heap is based on serialized LFH returned by GetProcessHeap()
// https://learn.microsoft.com/en-us/windows/win32/memory/low-fragmentation-heap
// threads can benefit from not serialized, not LFH if they allocate and free
// memory in time critical loops.

rt_end_c

