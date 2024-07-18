#include "ut/ut.h"

static void*   ut_static_symbol_reference[1024];
static int32_t ut_static_symbol_reference_count;

void* ut_force_symbol_reference(void* symbol) {
    assert(ut_static_symbol_reference_count <= ut_countof(ut_static_symbol_reference),
        "increase size of ut_static_symbol_reference[%d] to at least %d",
        ut_countof(ut_static_symbol_reference), ut_static_symbol_reference);
    if (ut_static_symbol_reference_count < ut_countof(ut_static_symbol_reference)) {
        ut_static_symbol_reference[ut_static_symbol_reference_count] = symbol;
//      ut_traceln("ut_static_symbol_reference[%d] = %p", ut_static_symbol_reference_count,
//               ut_static_symbol_reference[symbol_reference_count]);
        ut_static_symbol_reference_count++;
    }
    return symbol;
}

// test ut_static_init() { code } that will be executed in random
// order but before main()

#ifdef UT_TESTS

static int32_t ut_static_init_function_called;

static void ut_force_inline ut_static_init_function(void) {
    ut_static_init_function_called = 1;
}

ut_static_init(static_init_test) { ut_static_init_function(); }

void ut_static_init_test(void) {
    ut_fatal_if(ut_static_init_function_called != 1,
        "static_init_function() expected to be called before main()");
    if (ut_debug.verbosity.level > ut_debug.verbosity.quiet) { ut_traceln("done"); }
}

#else

void ut_static_init_test(void) {}

#endif
