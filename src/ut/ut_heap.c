#include "ut/ut.h"
#include "ut/ut_win32.h"


static errno_t ut_heap_alloc(void* *a, int64_t bytes) {
    return ut_heap.allocate(null, a, bytes, false);
}

static errno_t ut_heap_alloc_zero(void* *a, int64_t bytes) {
    return ut_heap.allocate(null, a, bytes, true);
}

static errno_t ut_heap_realloc(void* *a, int64_t bytes) {
    return ut_heap.reallocate(null, a, bytes, false);
}

static errno_t ut_heap_realloc_zero(void* *a, int64_t bytes) {
    return ut_heap.reallocate(null, a, bytes, true);
}

static void ut_heap_free(void* a) {
    ut_heap.deallocate(null, a);
}

static ut_heap_t* ut_heap_create(bool serialized) {
    const DWORD options = serialized ? 0 : HEAP_NO_SERIALIZE;
    return (ut_heap_t*)HeapCreate(options, 0, 0);
}

static void ut_heap_dispose(ut_heap_t* h) {
    ut_fatal_win32err(HeapDestroy((HANDLE)h));
}

static inline HANDLE ut_heap_or_process_heap(ut_heap_t* h) {
    static HANDLE process_heap;
    if (process_heap == null) { process_heap = GetProcessHeap(); }
    return h != null ? (HANDLE)h : process_heap;
}

static errno_t ut_heap_allocate(ut_heap_t* h, void* *p, int64_t bytes, bool zero) {
    swear(bytes > 0);
    #ifdef DEBUG
        static bool enabled;
        if (!enabled) {
            enabled = true;
            HeapSetInformation(null, HeapEnableTerminationOnCorruption, null, 0);
        }
    #endif
    const DWORD flags = zero ? HEAP_ZERO_MEMORY : 0;
    *p = HeapAlloc(ut_heap_or_process_heap(h), flags, (SIZE_T)bytes);
    return *p == null ? ERROR_OUTOFMEMORY : 0;
}

static errno_t ut_heap_reallocate(ut_heap_t* h, void* *p, int64_t bytes,
        bool zero) {
    swear(bytes > 0);
    const DWORD flags = zero ? HEAP_ZERO_MEMORY : 0;
    void* a = *p == null ? // HeapReAlloc(..., null, bytes) may not work
        HeapAlloc(ut_heap_or_process_heap(h), flags, (SIZE_T)bytes) :
        HeapReAlloc(ut_heap_or_process_heap(h), flags, *p, (SIZE_T)bytes);
    if (a != null) { *p = a; }
    return a == null ? ERROR_OUTOFMEMORY : 0;
}

static void ut_heap_deallocate(ut_heap_t* h, void* a) {
    ut_fatal_win32err(HeapFree(ut_heap_or_process_heap(h), 0, a));
}

static int64_t ut_heap_bytes(ut_heap_t* h, void* a) {
    SIZE_T bytes = HeapSize(ut_heap_or_process_heap(h), 0, a);
    ut_fatal_if(bytes == (SIZE_T)-1);
    return (int64_t)bytes;
}

static void ut_heap_test(void) {
    #ifdef UT_TESTS
    // TODO: allocate, reallocate deallocate, create, dispose
    void*   a[1024]; // addresses
    int32_t b[1024]; // bytes
    uint32_t seed = 0x1;
    for (int i = 0; i < 1024; i++) {
        b[i] = (int32_t)(ut_num.random32(&seed) % 1024) + 1;
        errno_t r = ut_heap.alloc(&a[i], b[i]);
        swear(r == 0);
    }
    for (int i = 0; i < 1024; i++) {
        ut_heap.free(a[i]);
    }
    HeapCompact(ut_heap_or_process_heap(null), 0);
    // "There is no extended error information for HeapValidate;
    //  do not call GetLastError."
    swear(HeapValidate(ut_heap_or_process_heap(null), 0, null));
    if (ut_debug.verbosity.level > ut_debug.verbosity.quiet) { ut_println("done"); }
    #endif
}

ut_heap_if ut_heap = {
    .alloc        = ut_heap_alloc,
    .alloc_zero   = ut_heap_alloc_zero,
    .realloc      = ut_heap_realloc,
    .realloc_zero = ut_heap_realloc_zero,
    .free         = ut_heap_free,
    .create       = ut_heap_create,
    .allocate     = ut_heap_allocate,
    .reallocate   = ut_heap_reallocate,
    .deallocate   = ut_heap_deallocate,
    .bytes        = ut_heap_bytes,
    .dispose      = ut_heap_dispose,
    .test         = ut_heap_test
};
