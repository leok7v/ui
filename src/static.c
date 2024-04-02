#include "runtime.h"

static void*   _static_symbol_reference[1024];
static int32_t _static_symbol_reference_count;

void* _static_force_symbol_reference_(void* symbol) {
    assert(_static_symbol_reference_count <= countof(_static_symbol_reference),
        "increase size of _static_symbol_reference[%d] to at least %d",
        countof(_static_symbol_reference), _static_symbol_reference);
    if (_static_symbol_reference_count < countof(_static_symbol_reference)) {
        _static_symbol_reference[_static_symbol_reference_count] = symbol;
//      traceln("_static_symbol_reference[%d] = %p", _static_symbol_reference_count,
//               _static_symbol_reference[symbol_reference_count]);
        _static_symbol_reference_count++;
    }
    return symbol;
}

// test static_init() { code } that will be executed in random
// order but before main()

#ifdef RUNTIME_TESTS
static int32_t static_init_function_called;

static void force_inline static_init_function(void) {
    static_init_function_called = 1;
}

static_init(static_init_test) { static_init_function(); }
#endif

void static_init_test(void) {
#ifdef RUNTIME_TESTS
    fatal_if(static_init_function_called != 1,
        "static_init_function() expected to be called before main()");
    if (debug.verbosity.level > debug.verbosity.quiet) { traceln("done"); }
#endif
}
