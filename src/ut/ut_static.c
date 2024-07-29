#include "ut/ut.h"

static void*   rt_static_symbol_reference[1024];
static int32_t rt_static_symbol_reference_count;

void* rt_force_symbol_reference(void* symbol) {
    rt_assert(rt_static_symbol_reference_count <= rt_countof(rt_static_symbol_reference),
        "increase size of rt_static_symbol_reference[%d] to at least %d",
        rt_countof(rt_static_symbol_reference), rt_static_symbol_reference);
    if (rt_static_symbol_reference_count < rt_countof(rt_static_symbol_reference)) {
        rt_static_symbol_reference[rt_static_symbol_reference_count] = symbol;
//      rt_println("rt_static_symbol_reference[%d] = %p", rt_static_symbol_reference_count,
//               rt_static_symbol_reference[symbol_reference_count]);
        rt_static_symbol_reference_count++;
    }
    return symbol;
}

// test rt_static_init() { code } that will be executed in random
// order but before main()

#ifdef UT_TESTS

static int32_t rt_static_init_function_called;

static void rt_force_inline rt_static_init_function(void) {
    rt_static_init_function_called = 1;
}

rt_static_init(static_init_test) { rt_static_init_function(); }

void rt_static_init_test(void) {
    rt_fatal_if(rt_static_init_function_called != 1,
        "static_init_function() expected to be called before main()");
    if (rt_debug.verbosity.level > rt_debug.verbosity.quiet) { rt_println("done"); }
}

#else

void rt_static_init_test(void) {}

#endif
