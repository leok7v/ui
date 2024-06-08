#include "ut/ut.h"
#include "ut/ut_win32.h"

static const char* ut_debug_abbreviate(const char* file) {
    const char* fn = strrchr(file, '\\');
    if (fn == null) { fn = strrchr(file, '/'); }
    return fn != null ? fn + 1 : file;
}

#ifdef WINDOWS

static int32_t ut_debug_max_file_line;
static int32_t ut_debug_max_function;

static void ut_debug_println_va(const char* file, int32_t line, const char* func,
        const char* format, va_list vl) {
    // full path is useful in MSVC debugger output pane (clickable)
    // for all other scenarios short filename without path is preferable:
    const char* name = IsDebuggerPresent() ? file : ut_files.basename(file);
    char file_line[1024];
    snprintf(file_line, countof(file_line) - 1, "%s(%d):", name, line);
    file_line[countof(file_line) - 1] = 0;
    ut_debug_max_file_line = ut_max(ut_debug_max_file_line,
                                    (int32_t)strlen(file_line));
    ut_debug_max_function  = ut_max(ut_debug_max_function,
                                    (int32_t)strlen(func));
    char prefix[2 * 1024];
    // snprintf() does not guarantee zero termination on truncation
    snprintf(prefix, countof(prefix) - 1, "%-*s %-*s",
            ut_debug_max_file_line, file_line,
            ut_debug_max_function,  func);
    prefix[countof(prefix) - 1] = 0; // zero terminated
    char text[2 * 1024];
    if (format != null && format[0] != 0) {
        #if defined(__GNUC__) || defined(__clang__)
        #pragma GCC diagnostic push
        #pragma GCC diagnostic ignored "-Wformat-nonliteral"
        #endif
        vsnprintf(text, countof(text) - 1, format, vl);
        text[countof(text) - 1] = 0;
        #if defined(__GNUC__) || defined(__clang__)
        #pragma GCC diagnostic pop
        #endif
    } else {
        text[0] = 0;
    }
    char output[4 * 1024];
    snprintf(output, countof(output) - 1, "%s %s", prefix, text);
    output[countof(output) - 2] = 0;
    // strip trailing \n which can be remnant of fprintf("...\n")
    int32_t n = (int32_t)strlen(output);
    while (n > 0 && (output[n - 1] == '\n' || output[n - 1] == '\r')) {
        output[n - 1] = 0;
        n--;
    }
    // For link.exe /Subsystem:Windows code stdout/stderr are often closed
    if (stderr != null && fileno(stderr) >= 0) {
        fprintf(stderr, "%s\n", output);
    }
    assert(n + 1 < countof(output));
    // OutputDebugString() needs \n
    output[n + 0] = '\n';
    output[n + 1] = 0;
    // SetConsoleCP(CP_UTF8) is not guaranteed to be called
    uint16_t wide[countof(output)];
    ut_str.utf8to16(wide, countof(wide), output);
    OutputDebugStringW(wide);
}

#else // posix version:

static void ut_debug_vprintf(const char* file, int32_t line, const char* func,
        const char* format, va_list vl) {
    fprintf(stderr, "%s(%d): %s ", file, line, func);
    vfprintf(stderr, format, vl);
    fprintf(stderr, "\n");
}

#endif

static void ut_debug_perrno(const char* file, int32_t line,
    const char* func, int32_t err_no, const char* format, ...) {
    if (err_no != 0) {
        if (format != null && format[0] != 0) {
            va_list vl;
            va_start(vl, format);
            ut_debug.println_va(file, line, func, format, vl);
            va_end(vl);
        }
        ut_debug.println(file, line, func, "errno: %d %s", err_no, strerror(err_no));
    }
}

static void ut_debug_perror(const char* file, int32_t line,
    const char* func, int32_t error, const char* format, ...) {
    if (error != 0) {
        if (format != null && format[0] != 0) {
            va_list vl;
            va_start(vl, format);
            ut_debug.println_va(file, line, func, format, vl);
            va_end(vl);
        }
        ut_debug.println(file, line, func, "error: %s", strerr(error));
    }
}

static void ut_debug_println(const char* file, int32_t line, const char* func,
        const char* format, ...) {
    va_list vl;
    va_start(vl, format);
    ut_debug.println_va(file, line, func, format, vl);
    va_end(vl);
}

static bool ut_debug_is_debugger_present(void) { return IsDebuggerPresent(); }

static void ut_debug_breakpoint(void) {
    if (ut_debug.is_debugger_present()) { DebugBreak(); }
}

static int32_t ut_debug_verbosity_from_string(const char* s) {
    char* n = null;
    long v = strtol(s, &n, 10);
    if (stricmp(s, "quiet") == 0) {
        return ut_debug.verbosity.quiet;
    } else if (stricmp(s, "info") == 0) {
        return ut_debug.verbosity.info;
    } else if (stricmp(s, "verbose") == 0) {
        return ut_debug.verbosity.verbose;
    } else if (stricmp(s, "debug") == 0) {
        return ut_debug.verbosity.debug;
    } else if (stricmp(s, "trace") == 0) {
        return ut_debug.verbosity.trace;
    } else if (n > s && ut_debug.verbosity.quiet <= v &&
               v <= ut_debug.verbosity.trace) {
        return v;
    } else {
        fatal("invalid verbosity: %s", s);
        return ut_debug.verbosity.quiet;
    }
}

static void ut_debug_test(void) {
    #ifdef UT_TESTS
    // not clear what can be tested here
    if (ut_debug.verbosity.level > ut_debug.verbosity.quiet) { traceln("done"); }
    #endif
}

ut_debug_if ut_debug = {
    .verbosity = {
        .level   =  0,
        .quiet   =  0,
        .info    =  1,
        .verbose =  2,
        .debug   =  3,
        .trace   =  4,
    },
    .verbosity_from_string = ut_debug_verbosity_from_string,
    .println               = ut_debug_println,
    .println_va            = ut_debug_println_va,
    .perrno                = ut_debug_perrno,
    .perror                = ut_debug_perror,
    .is_debugger_present   = ut_debug_is_debugger_present,
    .breakpoint            = ut_debug_breakpoint,
    .test                  = ut_debug_test
};
