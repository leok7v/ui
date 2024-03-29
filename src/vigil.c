#include "rt.h"
#include <stdbool.h>
#include <stdlib.h>

begin_c

static int32_t vigil_failed_assertion(const char* file, int32_t line,
        const char* func, const char* condition, const char* format, ...) {
    va_list vl;
    va_start(vl, format);
    debug.vprintf(file, line, func, format, vl);
    va_end(vl);
    debug.printf(file, line, func, "assertion failed: %s\n", condition);
    // avoid warnings: conditional expression always true and unreachable code
    const bool always_true = crt.abort != null;
    if (always_true) { crt.abort(); }
    return 0;
}
static int32_t vigil_fatal_termination(const char* file, int32_t line,
        const char* func, const char* condition, const char* format, ...) {
    const int32_t er = crt.err();
    const int32_t en = errno;
    va_list vl;
    va_start(vl, format);
    debug.vprintf(file, line, func, format, vl);
    va_end(vl);
    // report last errors:
    if (er != 0) { debug.perror(file, line, func, er, ""); }
    if (en != 0) { debug.perrno(file, line, func, en, ""); }
    if (condition != null && condition[0] != 0) {
        debug.printf(file, line, func, "FATAL: %s\n", condition);
    } else {
        debug.printf(file, line, func, "FATAL\n");
    }
    const bool always_true = debug.printf != null;
    if (always_true) { abort(); }
    return 0;
}

// test

static vigil_if vigil_test_saved;
static int32_t vigil_test_failed_assertion_count;
static int32_t vigil_test_verbosity;

#pragma push_macro("vigil")
// intimate knowledge of vigil.*() functions used in macro definitions
#define vigil vigil_test_saved

static int32_t vigil_test_failed_assertion(const char* file, int line,
        const char* func, const char* condition, const char* format, ...) {
    fatal_if_not(strequ(file,  __FILE__), "file: %s", file);
    fatal_if_not(line > __LINE__, "line: %s", line);
    assert(strequ(func, "vigil_test"), "func: %s", func);
    fatal_if(condition == null || condition[0] == 0);
    fatal_if(format == null || format[0] == 0);
    vigil_test_failed_assertion_count++;
    if (vigil_test_verbosity > 0) {
        va_list vl;
        va_start(vl, format);
        debug.vprintf(file, line, func, format, vl);
        va_end(vl);
        debug.printf(file, line, func, "assertion failed: %s\n", condition);
    }
    return 0;
}

static int32_t vigil_test_fatal_calls_count;

static int32_t vigil_test_fatal_termination(const char* file, int line,
        const char* func, const char* condition, const char* format, ...) {
    const int32_t er = crt.err();
    const int32_t en = errno;
    assert(er == 2, "crt.err: %d expected 2", er);
    assert(en == 2, "errno: %d expected 2", en);
    fatal_if_not(strequ(file,  __FILE__), "file: %s", file);
    fatal_if_not(line > __LINE__, "line: %s", line);
    assert(strequ(func, "vigil_test"), "func: %s", func);
    assert(strequ(condition, "")); // not yet used expected to be ""
    assert(format != null && format[0] != 0);
    vigil_test_fatal_calls_count++;
    if (vigil_test_verbosity > 0) {
        va_list vl;
        va_start(vl, format);
        debug.vprintf(file, line, func, format, vl);
        va_end(vl);
        if (er != 0) { debug.perror(file, line, func, er, ""); }
        if (en != 0) { debug.perrno(file, line, func, en, ""); }
        if (condition != null && condition[0] != 0) {
            debug.printf(file, line, func, "FATAL: %s\n", condition);
        } else {
            debug.printf(file, line, func, "FATAL\n");
        }
    }
    return 0;
}

#pragma pop_macro("vigil")

static void vigil_test(int32_t verbosity) {
    vigil_test_verbosity = verbosity;
    vigil_test_saved = vigil;
    int32_t en = errno;
    int32_t er = crt.err();
    errno = 2; // ENOENT
    crt.seterr(2); // ERROR_FILE_NOT_FOUND
    vigil.failed_assertion  = vigil_test_failed_assertion;
    vigil.fatal_termination = vigil_test_fatal_termination;
    int32_t count = vigil_test_fatal_calls_count;
    fatal("testing: %s call", "fatal()");
    assert(vigil_test_fatal_calls_count == count + 1);
    count = vigil_test_failed_assertion_count;
    assert(false, "testing: assert(%s)", "false");
    #ifdef DEBUG // verify that assert() is only compiled in in DEBUG:
        fatal_if_not(vigil_test_failed_assertion_count == count + 1);
    #else // not RELEASE buid:
        fatal_if_not(vigil_test_failed_assertion_count == count);
    #endif
    count = vigil_test_failed_assertion_count;
    swear(false, "testing: swear(%s)", "false");
    // swear() is triggered in both debug and release configurations:
    fatal_if_not(vigil_test_failed_assertion_count == count + 1);
    errno = en;
    crt.seterr(er);
    vigil = vigil_test_saved;
}


vigil_if vigil = {
    .failed_assertion  = vigil_failed_assertion,
    .fatal_termination = vigil_fatal_termination,
    .test = vigil_test
};

end_c

