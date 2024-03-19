#include "rt.h"

begin_c // allows C99 code to be compiled by C++ compiler (e.g. in #include)

static bool verbose;

// testing static_init() { code } that will be executed in random
// order but before main()

static int32_t static_init_function_called;

static void force_inline static_init_function(void) {
    static_init_function_called = 1;
}

static_init(static_init_test) { static_init_function(); }

static void test_static_init() {
    fatal_if(static_init_function_called != 1,
    "static_init_function() should have been called before main()");
}

// vigil: assert() and fatal() test

static int32_t failed_assertion_count;

static int test_failed_assertion(const char* file, int line,
        const char* func, const char* condition, const char* format, ...) {
    fatal_if_not(strequ(file,  __FILE__));
    fatal_if_not(line > __LINE__);
    fatal_if_not(strequ(func, "test_vigil"));
    fatal_if(condition == null || condition[0] == 0);
    fatal_if(format == null || format[0] == 0);
    failed_assertion_count++;
    if (verbose) {
        va_list vl;
        va_start(vl, format);
        trace.vtraceline(file, line, func, format, vl);
        va_end(vl);
        trace.traceline(file, line, func, "assertion failed: %s\n", condition);
    }
    return 0;
}

static int32_t fatal_calls_count;

static int test_fatal(const char* file, int line, const char* func,
        const char* condition, const char* format, ...) {
    assert(strequ(file,  __FILE__));
    assert(line > __LINE__);
    assert(strequ(func, "test_vigil"));
    assert(strequ(condition, "")); // not yet used expected to be ""
    assert(format != null && format[0] != 0);
    fatal_calls_count++;
    if (verbose) {
        va_list vl;
        va_start(vl, format);
        trace.vtraceline(file, line, func, format, vl);
        va_end(vl);
        if (condition != null && condition[0] != 0) {
            trace.traceline(file, line, func, "FATAL: %s\n", condition);
        } else {
            trace.traceline(file, line, func, "FATAL\n");
        }
    }
    return 0;
}

static void test_vigil(void) {
    // save global functions for failed assertion
    vigil_if saved = vigil;
    // test functions override:
    vigil.failed_assertion = test_failed_assertion;
    vigil.fatal_termination = test_fatal;
    // testing functions:
    int32_t count = fatal_calls_count;
    fatal("testing: %s call", "fatal()");
    assert(fatal_calls_count == count + 1);
    count = failed_assertion_count;
    assert(false, "testing: assert(%s)", "false");
    fatal_if_not(failed_assertion_count == count + 1);
    // restore vigil functions:
    vigil = saved;
}

int main(int unused(argc), const char* unused(argv)[]) {
    verbose = args.option_bool(&argc, argv, "--verbose") ||
              args.option_bool(&argc, argv, "-v");
    test_static_init();
    test_vigil();
    if (verbose) { printf("done\n"); }
    return 0;
}

end_c
