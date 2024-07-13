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
    va_list va;
    va_start(va, format);
    ut_debug.println_va(file, line, func, format, va);
    va_end(va);
    ut_debug.println(file, line, func, "assertion failed: %s\n", condition);
    // avoid warnings: conditional expression always true and unreachable code
    const bool always_true = ut_runtime.abort != null;
    if (always_true) { vigil_breakpoint_and_abort(); }
    return 0;
}

static int32_t vigil_fatal_termination_va(const char* file, int32_t line,
        const char* func, const char* condition, errno_t r,
        const char* format, va_list va) {
    const int32_t er = ut_runtime.err();
    const int32_t en = errno;
    ut_debug.println_va(file, line, func, format, va);
    if (r != er && r != 0) {
        ut_debug.perror(file, line, func, r, "");
    }
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


static int32_t vigil_fatal_termination(const char* file, int32_t line,
        const char* func, const char* condition, const char* format, ...) {
    va_list va;
    va_start(va, format);
    vigil_fatal_termination_va(file, line, func, condition, 0, format, va);
    va_end(va);
    return 0;
}

static int32_t vigil_fatal_if_error(const char* file, int32_t line,
    const char* func, const char* condition, errno_t r,
    const char* format, ...) {
    if (r != 0) {
        va_list va;
        va_start(va, format);
        vigil_fatal_termination_va(file, line, func, condition, r, format, va);
        va_end(va);
    }
    return 0;
}


#ifdef UT_TESTS

static ut_vigil_if  ut_vigil_test_saved;
static int32_t      ut_vigil_test_failed_assertion_count;

#pragma push_macro("ut_vigil")
// intimate knowledge of vigil.*() functions used in macro definitions
#define ut_vigil ut_vigil_test_saved

static int32_t vigil_test_failed_assertion(const char* file, int32_t line,
        const char* func, const char* condition, const char* format, ...) {
    ut_fatal_if_not(strcmp(file,  __FILE__) == 0, "file: %s", file);
    ut_fatal_if_not(line > __LINE__, "line: %s", line);
    assert(strcmp(func, "vigil_test") == 0, "func: %s", func);
    ut_fatal_if(condition == null || condition[0] == 0);
    ut_fatal_if(format == null || format[0] == 0);
    ut_vigil_test_failed_assertion_count++;
    if (ut_debug.verbosity.level >= ut_debug.verbosity.trace) {
        va_list va;
        va_start(va, format);
        ut_debug.println_va(file, line, func, format, va);
        va_end(va);
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
    ut_fatal_if_not(strcmp(file,  __FILE__) == 0, "file: %s", file);
    ut_fatal_if_not(line > __LINE__, "line: %s", line);
    assert(strcmp(func, "vigil_test") == 0, "func: %s", func);
    assert(strcmp(condition, "") == 0); // not yet used expected to be ""
    assert(format != null && format[0] != 0);
    vigil_test_fatal_calls_count++;
    if (ut_debug.verbosity.level > ut_debug.verbosity.trace) {
        va_list va;
        va_start(va, format);
        ut_debug.println_va(file, line, func, format, va);
        va_end(va);
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

#pragma pop_macro("ut_vigil")

static void vigil_test(void) {
    ut_vigil_test_saved = ut_vigil;
    int32_t en = errno;
    int32_t er = ut_runtime.err();
    errno = 2; // ENOENT
    ut_runtime.set_err(2); // ERROR_FILE_NOT_FOUND
    ut_vigil.failed_assertion  = vigil_test_failed_assertion;
    ut_vigil.fatal_termination = vigil_test_fatal_termination;
    int32_t count = vigil_test_fatal_calls_count;
    ut_fatal("testing: %s call", "fatal()");
    assert(vigil_test_fatal_calls_count == count + 1);
    count = ut_vigil_test_failed_assertion_count;
    assert(false, "testing: assert(%s)", "false");
    #ifdef DEBUG // verify that assert() is only compiled in DEBUG:
        ut_fatal_if_not(ut_vigil_test_failed_assertion_count == count + 1);
    #else // not RELEASE buid:
        ut_fatal_if_not(ut_vigil_test_failed_assertion_count == count);
    #endif
    count = ut_vigil_test_failed_assertion_count;
    swear(false, "testing: swear(%s)", "false");
    // swear() is triggered in both debug and release configurations:
    ut_fatal_if_not(ut_vigil_test_failed_assertion_count == count + 1);
    errno = en;
    ut_runtime.set_err(er);
    ut_vigil = ut_vigil_test_saved;
    if (ut_debug.verbosity.level > ut_debug.verbosity.quiet) { ut_traceln("done"); }
}

#else

static void vigil_test(void) { }

#endif

ut_vigil_if ut_vigil = {
    .failed_assertion  = vigil_failed_assertion,
    .fatal_termination = vigil_fatal_termination,
    .fatal_if_error    = vigil_fatal_if_error,
    .test = vigil_test
};
