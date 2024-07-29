#include "ut/ut.h"
#include "ut/ut_win32.h"


static errno_t rt_heap_alloc(void* *a, int64_t bytes) {
    return rt_heap.allocate(null, a, bytes, false);
}

static errno_t rt_heap_alloc_zero(void* *a, int64_t bytes) {
    return rt_heap.allocate(null, a, bytes, true);
}

static errno_t rt_heap_realloc(void* *a, int64_t bytes) {
    return rt_heap.reallocate(null, a, bytes, false);
}

static errno_t rt_heap_realloc_zero(void* *a, int64_t bytes) {
    return rt_heap.reallocate(null, a, bytes, true);
}

static void rt_heap_free(void* a) {
    rt_heap.deallocate(null, a);
}

static rt_heap_t* rt_heap_create(bool serialized) {
    const DWORD options = serialized ? 0 : HEAP_NO_SERIALIZE;
    return (rt_heap_t*)HeapCreate(options, 0, 0);
}

static void rt_heap_dispose(rt_heap_t* h) {
    ut_fatal_win32err(HeapDestroy((HANDLE)h));
}

static inline HANDLE rt_heap_or_process_heap(rt_heap_t* h) {
    static HANDLE process_heap;
    if (process_heap == null) { process_heap = GetProcessHeap(); }
    return h != null ? (HANDLE)h : process_heap;
}

static errno_t rt_heap_allocate(rt_heap_t* h, void* *p, int64_t bytes, bool zero) {
    rt_swear(bytes > 0);
    #ifdef DEBUG
        static bool enabled;
        if (!enabled) {
            enabled = true;
            HeapSetInformation(null, HeapEnableTerminationOnCorruption, null, 0);
        }
    #endif
    const DWORD flags = zero ? HEAP_ZERO_MEMORY : 0;
    *p = HeapAlloc(rt_heap_or_process_heap(h), flags, (SIZE_T)bytes);
    return *p == null ? ERROR_OUTOFMEMORY : 0;
}

static errno_t rt_heap_reallocate(rt_heap_t* h, void* *p, int64_t bytes,
        bool zero) {
    rt_swear(bytes > 0);
    const DWORD flags = zero ? HEAP_ZERO_MEMORY : 0;
    void* a = *p == null ? // HeapReAlloc(..., null, bytes) may not work
        HeapAlloc(rt_heap_or_process_heap(h), flags, (SIZE_T)bytes) :
        HeapReAlloc(rt_heap_or_process_heap(h), flags, *p, (SIZE_T)bytes);
    if (a != null) { *p = a; }
    return a == null ? ERROR_OUTOFMEMORY : 0;
}

static void rt_heap_deallocate(rt_heap_t* h, void* a) {
    ut_fatal_win32err(HeapFree(rt_heap_or_process_heap(h), 0, a));
}

static int64_t rt_heap_bytes(rt_heap_t* h, void* a) {
    SIZE_T bytes = HeapSize(rt_heap_or_process_heap(h), 0, a);
    rt_fatal_if(bytes == (SIZE_T)-1);
    return (int64_t)bytes;
}

static void rt_heap_test(void) {
    #ifdef UT_TESTS
    // TODO: allocate, reallocate deallocate, create, dispose
    void*   a[1024]; // addresses
    int32_t b[1024]; // bytes
    uint32_t seed = 0x1;
    for (int i = 0; i < 1024; i++) {
        b[i] = (int32_t)(ut_num.random32(&seed) % 1024) + 1;
        errno_t r = rt_heap.alloc(&a[i], b[i]);
        rt_swear(r == 0);
    }
    for (int i = 0; i < 1024; i++) {
        rt_heap.free(a[i]);
    }
    HeapCompact(rt_heap_or_process_heap(null), 0);
    // "There is no extended error information for HeapValidate;
    //  do not call GetLastError."
    rt_swear(HeapValidate(rt_heap_or_process_heap(null), 0, null));
    if (rt_debug.verbosity.level > rt_debug.verbosity.quiet) { ut_println("done"); }
    #endif
}

rt_heap_if rt_heap = {
    .alloc        = rt_heap_alloc,
    .alloc_zero   = rt_heap_alloc_zero,
    .realloc      = rt_heap_realloc,
    .realloc_zero = rt_heap_realloc_zero,
    .free         = rt_heap_free,
    .create       = rt_heap_create,
    .allocate     = rt_heap_allocate,
    .reallocate   = rt_heap_reallocate,
    .deallocate   = rt_heap_deallocate,
    .bytes        = rt_heap_bytes,
    .dispose      = rt_heap_dispose,
    .test         = rt_heap_test
};
