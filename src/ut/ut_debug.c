#include "ut/ut.h"
#include "ut/ut_win32.h"

static const char* rt_debug_abbreviate(const char* file) {
    const char* fn = strrchr(file, '\\');
    if (fn == null) { fn = strrchr(file, '/'); }
    return fn != null ? fn + 1 : file;
}

#ifdef WINDOWS

static int32_t rt_debug_max_file_line;
static int32_t rt_debug_max_function;

static void rt_debug_output(const char* s, int32_t count) {
    bool intercepted = false;
    if (rt_debug.tee != null) { intercepted = rt_debug.tee(s, count); }
    if (!intercepted) {
        // For link.exe /Subsystem:Windows code stdout/stderr are often closed
        if (stderr != null && fileno(stderr) >= 0) {
            fprintf(stderr, "%s", s);
        }
        // SetConsoleCP(CP_UTF8) is not guaranteed to be called
        uint16_t* wide = ut_stackalloc((count + 1) * sizeof(uint16_t));
        ut_str.utf8to16(wide, count, s, -1);
        OutputDebugStringW(wide);
    }
}

static void rt_debug_println_va(const char* file, int32_t line, const char* func,
        const char* format, va_list va) {
    if (func == null) { func = ""; }
    char file_line[1024];
    if (line == 0 && file == null || file[0] == 0x00) {
        file_line[0] = 0x00;
    } else {
        if (file == null) { file = ""; } // backtrace can have null files
        // full path is useful in MSVC debugger output pane (clickable)
        // for all other scenarios short filename without path is preferable:
        const char* name = IsDebuggerPresent() ? file : rt_files.basename(file);
        snprintf(file_line, ut_countof(file_line) - 1, "%s(%d):", name, line);
    }
    file_line[ut_countof(file_line) - 1] = 0x00; // always zero terminated'
    rt_debug_max_file_line = rt_max(rt_debug_max_file_line,
                                    (int32_t)strlen(file_line));
    rt_debug_max_function  = rt_max(rt_debug_max_function,
                                    (int32_t)strlen(func));
    char prefix[2 * 1024];
    // snprintf() does not guarantee zero termination on truncation
    snprintf(prefix, ut_countof(prefix) - 1, "%-*s %-*s",
            rt_debug_max_file_line, file_line,
            rt_debug_max_function,  func);
    prefix[ut_countof(prefix) - 1] = 0; // zero terminated
    char text[2 * 1024];
    if (format != null && format[0] != 0) {
        #if defined(__GNUC__) || defined(__clang__)
        #pragma GCC diagnostic push
        #pragma GCC diagnostic ignored "-Wformat-nonliteral"
        #endif
        vsnprintf(text, ut_countof(text) - 1, format, va);
        text[ut_countof(text) - 1] = 0;
        #if defined(__GNUC__) || defined(__clang__)
        #pragma GCC diagnostic pop
        #endif
    } else {
        text[0] = 0;
    }
    char output[4 * 1024];
    snprintf(output, ut_countof(output) - 1, "%s %s", prefix, text);
    output[ut_countof(output) - 2] = 0;
    // strip trailing \n which can be remnant of fprintf("...\n")
    int32_t n = (int32_t)strlen(output);
    while (n > 0 && (output[n - 1] == '\n' || output[n - 1] == '\r')) {
        output[n - 1] = 0;
        n--;
    }
    ut_assert(n + 1 < ut_countof(output));
    // Win32 OutputDebugString() needs \n
    output[n + 0] = '\n';
    output[n + 1] = 0;
    rt_debug.output(output, n + 2); // including 0x00
}

#else // posix version:

static void rt_debug_vprintf(const char* file, int32_t line, const char* func,
        const char* format, va_list va) {
    fprintf(stderr, "%s(%d): %s ", file, line, func);
    vfprintf(stderr, format, va);
    fprintf(stderr, "\n");
}

#endif

static void rt_debug_perrno(const char* file, int32_t line,
    const char* func, int32_t err_no, const char* format, ...) {
    if (err_no != 0) {
        if (format != null && format[0] != 0) {
            va_list va;
            va_start(va, format);
            rt_debug.println_va(file, line, func, format, va);
            va_end(va);
        }
        rt_debug.println(file, line, func, "errno: %d %s", err_no, strerror(err_no));
    }
}

static void rt_debug_perror(const char* file, int32_t line,
    const char* func, int32_t error, const char* format, ...) {
    if (error != 0) {
        if (format != null && format[0] != 0) {
            va_list va;
            va_start(va, format);
            rt_debug.println_va(file, line, func, format, va);
            va_end(va);
        }
        rt_debug.println(file, line, func, "error: %s", ut_strerr(error));
    }
}

static void rt_debug_println(const char* file, int32_t line, const char* func,
        const char* format, ...) {
    va_list va;
    va_start(va, format);
    rt_debug.println_va(file, line, func, format, va);
    va_end(va);
}

static bool rt_debug_is_debugger_present(void) { return IsDebuggerPresent(); }

static void rt_debug_breakpoint(void) {
    if (rt_debug.is_debugger_present()) { DebugBreak(); }
}

static errno_t rt_debug_raise(uint32_t exception) {
    rt_core.set_err(0);
    RaiseException(exception, EXCEPTION_NONCONTINUABLE, 0, null);
    return rt_core.err();
}

static int32_t rt_debug_verbosity_from_string(const char* s) {
    char* n = null;
    long v = strtol(s, &n, 10);
    if (stricmp(s, "quiet") == 0) {
        return rt_debug.verbosity.quiet;
    } else if (stricmp(s, "info") == 0) {
        return rt_debug.verbosity.info;
    } else if (stricmp(s, "verbose") == 0) {
        return rt_debug.verbosity.verbose;
    } else if (stricmp(s, "debug") == 0) {
        return rt_debug.verbosity.debug;
    } else if (stricmp(s, "trace") == 0) {
        return rt_debug.verbosity.trace;
    } else if (n > s && rt_debug.verbosity.quiet <= v &&
               v <= rt_debug.verbosity.trace) {
        return v;
    } else {
        ut_fatal("invalid verbosity: %s", s);
        return rt_debug.verbosity.quiet;
    }
}

static void rt_debug_test(void) {
    #ifdef UT_TESTS
    // not clear what can be tested here
    if (rt_debug.verbosity.level > rt_debug.verbosity.quiet) { ut_println("done"); }
    #endif
}

#ifndef STATUS_POSSIBLE_DEADLOCK
#define STATUS_POSSIBLE_DEADLOCK 0xC0000194uL
#endif

rt_debug_if rt_debug = {
    .verbosity = {
        .level   =  0,
        .quiet   =  0,
        .info    =  1,
        .verbose =  2,
        .debug   =  3,
        .trace   =  4,
    },
    .verbosity_from_string = rt_debug_verbosity_from_string,
    .tee                   = null,
    .output                = rt_debug_output,
    .println               = rt_debug_println,
    .println_va            = rt_debug_println_va,
    .perrno                = rt_debug_perrno,
    .perror                = rt_debug_perror,
    .is_debugger_present   = rt_debug_is_debugger_present,
    .breakpoint            = rt_debug_breakpoint,
    .raise                 = rt_debug_raise,
    .exception             = {
        .access_violation        = EXCEPTION_ACCESS_VIOLATION,
        .datatype_misalignment   = EXCEPTION_DATATYPE_MISALIGNMENT,
        .breakpoint              = EXCEPTION_BREAKPOINT,
        .single_step             = EXCEPTION_SINGLE_STEP,
        .array_bounds            = EXCEPTION_ARRAY_BOUNDS_EXCEEDED,
        .float_denormal_operand  = EXCEPTION_FLT_DENORMAL_OPERAND,
        .float_divide_by_zero    = EXCEPTION_FLT_DIVIDE_BY_ZERO,
        .float_inexact_result    = EXCEPTION_FLT_INEXACT_RESULT,
        .float_invalid_operation = EXCEPTION_FLT_INVALID_OPERATION,
        .float_overflow          = EXCEPTION_FLT_OVERFLOW,
        .float_stack_check       = EXCEPTION_FLT_STACK_CHECK,
        .float_underflow         = EXCEPTION_FLT_UNDERFLOW,
        .int_divide_by_zero      = EXCEPTION_INT_DIVIDE_BY_ZERO,
        .int_overflow            = EXCEPTION_INT_OVERFLOW,
        .priv_instruction        = EXCEPTION_PRIV_INSTRUCTION,
        .in_page_error           = EXCEPTION_IN_PAGE_ERROR,
        .illegal_instruction     = EXCEPTION_ILLEGAL_INSTRUCTION,
        .noncontinuable          = EXCEPTION_NONCONTINUABLE_EXCEPTION,
        .stack_overflow          = EXCEPTION_STACK_OVERFLOW,
        .invalid_disposition     = EXCEPTION_INVALID_DISPOSITION,
        .guard_page              = EXCEPTION_GUARD_PAGE,
        .invalid_handle          = EXCEPTION_INVALID_HANDLE,
        .possible_deadlock       = EXCEPTION_POSSIBLE_DEADLOCK
    },
    .test                  = rt_debug_test
};
