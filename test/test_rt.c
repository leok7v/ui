#include "rt.h"

static bool verbose;

// testing static_init() { code } that will be executed in random
// order but before main()

static int32_t static_init_function_called;

static void force_inline static_init_function(void) {
    static_init_function_called = 1;
}

static_init(static_init_test) { static_init_function(); }

static void test_static_init(void) {
    fatal_if(static_init_function_called != 1,
    "static_init_function() expected to be called before main()");
    if (verbose) { traceln("static_init_function() called before main()"); }
}

// vigil: assert(), swear() and fatal()

static int32_t failed_assertion_count;

static int failed_assertion(const char* file, int line,
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
        trace.vprintf(file, line, func, format, vl);
        va_end(vl);
        trace.printf(file, line, func, "assertion failed: %s\n", condition);
    }
    return 0;
}

static int32_t fatal_calls_count;

static int fatal_termination(const char* file, int line, const char* func,
        const char* condition, const char* format, ...) {
    const int32_t er = crt.err();
    const int32_t en = errno;
    assert(er == 2);
    assert(en == 2);
    assert(strequ(file,  __FILE__));
    assert(line > __LINE__);
    assert(strequ(func, "test_vigil"));
    assert(strequ(condition, "")); // not yet used expected to be ""
    assert(format != null && format[0] != 0);
    fatal_calls_count++;
    if (verbose) {
        va_list vl;
        va_start(vl, format);
        trace.vprintf(file, line, func, format, vl);
        va_end(vl);
        if (er != 0) { trace.perror(file, line, func, er, ""); }
        if (en != 0) { trace.perrno(file, line, func, en, ""); }
        if (condition != null && condition[0] != 0) {
            trace.printf(file, line, func, "FATAL: %s\n", condition);
        } else {
            trace.printf(file, line, func, "FATAL\n");
        }
    }
    return 0;
}

static void test_vigil(void) {
    vigil_if saved = vigil;
    int32_t en = errno;
    int32_t er = crt.err();
    errno = 2; // ENOENT
    crt.seterr(2); // ERROR_FILE_NOT_FOUND
    vigil.failed_assertion  = failed_assertion;
    vigil.fatal_termination = fatal_termination;
    int32_t count = fatal_calls_count;
    fatal("testing: %s call", "fatal()");
    assert(fatal_calls_count == count + 1);
    count = failed_assertion_count;
    assert(false, "testing: assert(%s)", "false");
    #ifdef DEBUG // verify that assert() is only compiled in in DEBUG:
        fatal_if_not(failed_assertion_count == count + 1);
    #else // not RELEASE buid:
        fatal_if_not(failed_assertion_count == count);
    #endif
    count = failed_assertion_count;
    swear(false, "testing: swear(%s)", "false");
    // swear() is triggered in both debug and release configurations:
    fatal_if_not(failed_assertion_count == count + 1);
    errno = en;
    crt.seterr(er);
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

