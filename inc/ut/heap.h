#pragma once
#include "ut/ut_std.h"

begin_c

// It is absolutely OK to use posix compliant
// malloc()/calloc()/realloc()/free() function calls with understanding
// that they introduce serialization points in multi-threaded applications
// and may be induce wait states that under pressure (all cores busy) may
// result in prolonged wait states which may not be acceptable for real time
// processing.
//
// heap_if.functions may or may not be faster than malloc()/free() ...
//
// Some callers may find realloc parameters more convenient to avoid
// anti-pattern
//      void* reallocated = realloc(p, new_size);
//      if (reallocated != null) { p = reallocated; }
// and avoid never ending discussion of legality and implementation
// compliance of the situation:
//      realloc(p /* when p == null */, ...)
//
// zero: true initializes allocated or reallocated tail memory to 0x00

typedef struct heap_s heap_t;

typedef struct { // heap == null uses process serialized LFH
    heap_t* (*create)(bool serialized);
    errno_t (*allocate)(heap_t* heap, void* *a, int64_t bytes, bool zero);
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

