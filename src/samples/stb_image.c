#include "ut/ut.h"

static void* stb_malloc(size_t n) {
    ut_assert(n > 0);
    void* a = null;
    errno_t r = ut_heap.allocate(null, &a, n, false);
    ut_swear(r == 0 && a != null);
//  ut_println("%p : %8lld", a, n);
    return a;
}

static void* stb_realloc(void* p, size_t n) {
    ut_assert(n > 0);
    void* a = p;
    errno_t r = ut_heap.reallocate(null, &a, n, false);
    ut_swear(r == 0 && a != null);
//  ut_println("%p -> %p : %8lld", p, a, n);
    return a;
}

static void* stb_realloc_sized(void* p, size_t ut_unused(s), size_t n) {
    ut_assert(n > 0);
    void* a = p;
    errno_t r = ut_heap.reallocate(null, &a, n, false);
    ut_swear(r == 0 && a != null);
//  ut_println("%p : %8lld -> %p : %8lld", p, s, a, n);
    return a;
}

static void  stb_free(void* p) {
//  ut_println("%p", p);
    ut_heap.deallocate(null, p);
}

#pragma warning(disable: 4459) // declaration of '...' hides global declaration
#define STBI_ASSERT(...) ut_assert(__VA_ARGS__)

#define STBI_MALLOC(sz)           stb_malloc(sz)
#define STBI_REALLOC(p,newsz)     stb_realloc((p), (newsz))
#define STBI_FREE(p)              stb_free(p)
#define STBI_REALLOC_SIZED(p,oldsz,newsz) stb_realloc_sized((p),(oldsz),(newsz))

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
