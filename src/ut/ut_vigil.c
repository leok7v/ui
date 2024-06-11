#include "ut/ut.h"
#include <stdio.h>
#include <string.h>

static void vigil_breakpoint_and_abort(void) {
    ut_debug.breakpoint(); // only if debugger is present
    ut_debug.raise(ut_debug.exception.noncontinuable);
    ut_runtime.abort();
}

static int32_t vigil_failed_assertion(const char* file, int32_t line,
        const char* func, const char* condition, const char* format, ...) {
    va_list vl;
    va_start(vl, format);
    ut_debug.println_va(file, line, func, format, vl);
    va_end(vl);
    ut_debug.println(file, line, func, "assertion failed: %s\n", condition);
    // avoid warnings: conditional expression always true and unreachable code
    const bool always_true = ut_runtime.abort != null;
    if (always_true) { vigil_breakpoint_and_abort(); }
    return 0;
}

static int32_t vigil_fatal_termination(const char* file, int32_t line,
        const char* func, const char* condition, const char* format, ...) {
    const int32_t er = ut_runtime.err();
    const int32_t en = errno;
    va_list vl;
    va_start(vl, format);
    ut_debug.println_va(file, line, func, format, vl);
    va_end(vl);
    // report last errors:
    if (er != 0) { ut_debug.perror(file, line, func, er, ""); }
    if (en != 0) { ut_debug.perrno(file, line, func, en, ""); }
    if (condition != null && condition[0] != 0) {
        ut_debug.println(file, line, func, "FATAL: %s\n", condition);
    } else {
        ut_debug.println(file, line, func, "FATAL\n");
    }
    const bool always_true = ut_runtime.abort != null;
    if (always_true) { vigil_breakpoint_and_abort(); }
    return 0;
}

#ifdef UT_TESTS

static vigil_if vigil_test_saved;
static int32_t  vigil_test_failed_assertion_count;

#pragma push_macro("vigil")
// intimate knowledge of vigil.*() functions used in macro definitions
#define vigil vigil_test_saved

static int32_t vigil_test_failed_assertion(const char* file, int32_t line,
        const char* func, const char* condition, const char* format, ...) {
    fatal_if_not(strcmp(file,  __FILE__) == 0, "file: %s", file);
    fatal_if_not(line > __LINE__, "line: %s", line);
    assert(strcmp(func, "vigil_test") == 0, "func: %s", func);
    fatal_if(condition == null || condition[0] == 0);
    fatal_if(format == null || format[0] == 0);
    vigil_test_failed_assertion_count++;
    if (ut_debug.verbosity.level >= ut_debug.verbosity.trace) {
        va_list vl;
        va_start(vl, format);
        ut_debug.println_va(file, line, func, format, vl);
        va_end(vl);
        ut_debug.println(file, line, func, "assertion failed: %s (expected)\n",
                     condition);
    }
    return 0;
}

static int32_t vigil_test_fatal_calls_count;

static int32_t vigil_test_fatal_termination(const char* file, int32_t line,
        const char* func, const char* condition, const char* format, ...) {
    const int32_t er = ut_runtime.err();
    const int32_t en = errno;
    assert(er == 2, "ut_runtime.err: %d expected 2", er);
    assert(en == 2, "errno: %d expected 2", en);
    fatal_if_not(strcmp(file,  __FILE__) == 0, "file: %s", file);
    fatal_if_not(line > __LINE__, "line: %s", line);
    assert(strcmp(func, "vigil_test") == 0, "func: %s", func);
    assert(strcmp(condition, "") == 0); // not yet used expected to be ""
    assert(format != null && format[0] != 0);
    vigil_test_fatal_calls_count++;
    if (ut_debug.verbosity.level > ut_debug.verbosity.trace) {
        va_list vl;
        va_start(vl, format);
        ut_debug.println_va(file, line, func, format, vl);
        va_end(vl);
        if (er != 0) { ut_debug.perror(file, line, func, er, ""); }
        if (en != 0) { ut_debug.perrno(file, line, func, en, ""); }
        if (condition != null && condition[0] != 0) {
            ut_debug.println(file, line, func, "FATAL: %s (testing)\n", condition);
        } else {
            ut_debug.println(file, line, func, "FATAL (testing)\n");
        }
    }
    return 0;
}

#pragma pop_macro("vigil")

static void vigil_test(void) {
    vigil_test_saved = vigil;
    int32_t en = errno;
    int32_t er = ut_runtime.err();
    errno = 2; // ENOENT
    ut_runtime.seterr(2); // ERROR_FILE_NOT_FOUND
    vigil.failed_assertion  = vigil_test_failed_assertion;
    vigil.fatal_termination = vigil_test_fatal_termination;
    int32_t count = vigil_test_fatal_calls_count;
    fatal("testing: %s call", "fatal()");
    assert(vigil_test_fatal_calls_count == count + 1);
    count = vigil_test_failed_assertion_count;
    assert(false, "testing: assert(%s)", "false");
    #ifdef DEBUG // verify that assert() is only compiled in DEBUG:
        fatal_if_not(vigil_test_failed_assertion_count == count + 1);
    #else // not RELEASE buid:
        fatal_if_not(vigil_test_failed_assertion_count == count);
    #endif
    count = vigil_test_failed_assertion_count;
    swear(false, "testing: swear(%s)", "false");
    // swear() is triggered in both debug and release configurations:
    fatal_if_not(vigil_test_failed_assertion_count == count + 1);
    errno = en;
    ut_runtime.seterr(er);
    vigil = vigil_test_saved;
    if (ut_debug.verbosity.level > ut_debug.verbosity.quiet) { traceln("done"); }
}

#else

static void vigil_test(void) { }

#endif

vigil_if vigil = {
    .failed_assertion  = vigil_failed_assertion,
    .fatal_termination = vigil_fatal_termination,
    .test = vigil_test
};
