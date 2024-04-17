#include "ut/ut.h"
#include "ut/win32.h"

static heap_t* heap_create(bool serialized) {
    const DWORD options = serialized ? 0 : HEAP_NO_SERIALIZE;
    return (heap_t*)HeapCreate(options, 0, 0);
}

static void heap_dispose(heap_t* h) {
    fatal_if_false(HeapDestroy((HANDLE)h));
}

static inline HANDLE mem_heap(heap_t* h) {
    static HANDLE process_heap;
    if (process_heap == null) { process_heap = GetProcessHeap(); }
    return h != null ? (HANDLE)h : process_heap;
}

static errno_t heap_allocate(heap_t* h, void* *p, int64_t bytes, bool zero) {
    swear(bytes > 0);
    const DWORD flags = zero ? HEAP_ZERO_MEMORY : 0;
    *p = HeapAlloc(mem_heap(h), flags, (SIZE_T)bytes);
    return *p == null ? ERROR_OUTOFMEMORY : 0;
}

static errno_t heap_reallocate(heap_t* h, void* *p, int64_t bytes,
        bool zero) {
    swear(bytes > 0);
    const DWORD flags = zero ? HEAP_ZERO_MEMORY : 0;
    void* a = *p == null ? // HeapReAlloc(..., null, bytes) may not work
        HeapAlloc(mem_heap(h), flags, (SIZE_T)bytes) :
        HeapReAlloc(mem_heap(h), flags, *p, (SIZE_T)bytes);
    if (a != null) { *p = a; }
    return a == null ? ERROR_OUTOFMEMORY : 0;
}

static void heap_deallocate(heap_t* h, void* a) {
    fatal_if_false(HeapFree(mem_heap(h), 0, a));
}

static int64_t heap_bytes(heap_t* h, void* a) {
    SIZE_T bytes = HeapSize(mem_heap(h), 0, a);
    fatal_if(bytes == (SIZE_T)-1);
    return (int64_t)bytes;
}

static void heap_test(void) {
    #ifdef RUNTIME_TESTS
    // TODO: allocate, reallocate deallocate, create, dispose
    traceln("TODO");
    if (debug.verbosity.level > debug.verbosity.quiet) { traceln("done"); }
    #endif
}

heap_if heap = {
    .create      = heap_create,
    .allocate    = heap_allocate,
    .reallocate  = heap_reallocate,
    .deallocate  = heap_deallocate,
    .bytes       = heap_bytes,
    .dispose     = heap_dispose,
    .test        = heap_test
};
