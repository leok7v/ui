#include "ut/ut.h"
#include <stdio.h>
#include <string.h>

static void rt_vigil_breakpoint_and_abort(void) {
    rt_debug.breakpoint(); // only if debugger is present
    rt_debug.raise(rt_debug.exception.noncontinuable);
    rt_core.abort();
}

static int32_t rt_vigil_failed_assertion(const char* file, int32_t line,
        const char* func, const char* condition, const char* format, ...) {
    va_list va;
    va_start(va, format);
    rt_debug.println_va(file, line, func, format, va);
    va_end(va);
    rt_debug.println(file, line, func, "assertion failed: %s\n", condition);
    // avoid warnings: conditional expression always true and unreachable code
    const bool always_true = rt_core.abort != null;
    if (always_true) { rt_vigil_breakpoint_and_abort(); }
    return 0;
}

static int32_t rt_vigil_fatal_termination_va(const char* file, int32_t line,
        const char* func, const char* condition, errno_t r,
        const char* format, va_list va) {
    const int32_t er = rt_core.err();
    const int32_t en = errno;
    rt_debug.println_va(file, line, func, format, va);
    if (r != er && r != 0) {
        rt_debug.perror(file, line, func, r, "");
    }
    // report last errors:
    if (er != 0) { rt_debug.perror(file, line, func, er, ""); }
    if (en != 0) { rt_debug.perrno(file, line, func, en, ""); }
    if (condition != null && condition[0] != 0) {
        rt_debug.println(file, line, func, "FATAL: %s\n", condition);
    } else {
        rt_debug.println(file, line, func, "FATAL\n");
    }
    const bool always_true = rt_core.abort != null;
    if (always_true) { rt_vigil_breakpoint_and_abort(); }
    return 0;
}

static int32_t rt_vigil_fatal_termination(const char* file, int32_t line,
        const char* func, const char* condition, const char* format, ...) {
    va_list va;
    va_start(va, format);
    rt_vigil_fatal_termination_va(file, line, func, condition, 0, format, va);
    va_end(va);
    return 0;
}

static int32_t rt_vigil_fatal_if_error(const char* file, int32_t line,
    const char* func, const char* condition, errno_t r,
    const char* format, ...) {
    if (r != 0) {
        va_list va;
        va_start(va, format);
        rt_vigil_fatal_termination_va(file, line, func, condition, r, format, va);
        va_end(va);
    }
    return 0;
}

#ifdef UT_TESTS

static rt_vigil_if  rt_vigil_test_saved;
static int32_t      rt_vigil_test_failed_assertion_count;

#pragma push_macro("rt_vigil")
// intimate knowledge of vigil.*() functions used in macro definitions
#define rt_vigil rt_vigil_test_saved

static int32_t rt_vigil_test_failed_assertion(const char* file, int32_t line,
        const char* func, const char* condition, const char* format, ...) {
    rt_fatal_if_not(strcmp(file,  __FILE__) == 0, "file: %s", file);
    rt_fatal_if_not(line > __LINE__, "line: %s", line);
    rt_assert(strcmp(func, "rt_vigil_test") == 0, "func: %s", func);
    rt_fatal_if(condition == null || condition[0] == 0);
    rt_fatal_if(format == null || format[0] == 0);
    rt_vigil_test_failed_assertion_count++;
    if (rt_debug.verbosity.level >= rt_debug.verbosity.trace) {
        va_list va;
        va_start(va, format);
        rt_debug.println_va(file, line, func, format, va);
        va_end(va);
        rt_debug.println(file, line, func, "assertion failed: %s (expected)\n",
                     condition);
    }
    return 0;
}

static int32_t rt_vigil_test_fatal_calls_count;

static int32_t rt_vigil_test_fatal_termination(const char* file, int32_t line,
        const char* func, const char* condition, const char* format, ...) {
    const int32_t er = rt_core.err();
    const int32_t en = errno;
    rt_assert(er == 2, "rt_core.err: %d expected 2", er);
    rt_assert(en == 2, "errno: %d expected 2", en);
    rt_fatal_if_not(strcmp(file,  __FILE__) == 0, "file: %s", file);
    rt_fatal_if_not(line > __LINE__, "line: %s", line);
    rt_assert(strcmp(func, "rt_vigil_test") == 0, "func: %s", func);
    rt_assert(strcmp(condition, "") == 0); // not yet used expected to be ""
    rt_assert(format != null && format[0] != 0);
    rt_vigil_test_fatal_calls_count++;
    if (rt_debug.verbosity.level > rt_debug.verbosity.trace) {
        va_list va;
        va_start(va, format);
        rt_debug.println_va(file, line, func, format, va);
        va_end(va);
        if (er != 0) { rt_debug.perror(file, line, func, er, ""); }
        if (en != 0) { rt_debug.perrno(file, line, func, en, ""); }
        if (condition != null && condition[0] != 0) {
            rt_debug.println(file, line, func, "FATAL: %s (testing)\n", condition);
        } else {
            rt_debug.println(file, line, func, "FATAL (testing)\n");
        }
    }
    return 0;
}

#pragma pop_macro("rt_vigil")

static void rt_vigil_test(void) {
    rt_vigil_test_saved = rt_vigil;
    int32_t en = errno;
    int32_t er = rt_core.err();
    errno = 2; // ENOENT
    rt_core.set_err(2); // ERROR_FILE_NOT_FOUND
    rt_vigil.failed_assertion  = rt_vigil_test_failed_assertion;
    rt_vigil.fatal_termination = rt_vigil_test_fatal_termination;
    int32_t count = rt_vigil_test_fatal_calls_count;
    rt_fatal("testing: %s call", "fatal()");
    rt_assert(rt_vigil_test_fatal_calls_count == count + 1);
    count = rt_vigil_test_failed_assertion_count;
    rt_assert(false, "testing: rt_assert(%s)", "false");
    #ifdef DEBUG // verify that rt_assert() is only compiled in DEBUG:
        rt_fatal_if_not(rt_vigil_test_failed_assertion_count == count + 1);
    #else // not RELEASE buid:
        rt_fatal_if_not(rt_vigil_test_failed_assertion_count == count);
    #endif
    count = rt_vigil_test_failed_assertion_count;
    rt_swear(false, "testing: swear(%s)", "false");
    // swear() is triggered in both debug and release configurations:
    rt_fatal_if_not(rt_vigil_test_failed_assertion_count == count + 1);
    errno = en;
    rt_core.set_err(er);
    rt_vigil = rt_vigil_test_saved;
    if (rt_debug.verbosity.level > rt_debug.verbosity.quiet) { rt_println("done"); }
}

#else

static void rt_vigil_test(void) { }

#endif

rt_vigil_if rt_vigil = {
    .failed_assertion  = rt_vigil_failed_assertion,
    .fatal_termination = rt_vigil_fatal_termination,
    .fatal_if_error    = rt_vigil_fatal_if_error,
    .test              = rt_vigil_test
};
