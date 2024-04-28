#include "ut/ut.h"
#include "ut/win32.h"

static const char* debug_abbreviate(const char* file) {
    const char* fn = strrchr(file, '\\');
    if (fn == null) { fn = strrchr(file, '/'); }
    return fn != null ? fn + 1 : file;
}

#ifdef WINDOWS

static void debug_println_va(const char* file, int32_t line, const char* func,
        const char* format, va_list vl) {
    char prefix[2 * 1024];
    // full path is useful in MSVC debugger output pane (clickable)
    // for all other scenarios short filename without path is preferable:
    const char* name = IsDebuggerPresent() ? file : files.basename(file);
    // snprintf() does not guarantee zero termination on truncation
    snprintf(prefix, countof(prefix) - 1, "%s(%d): %s", name, line, func);
    prefix[countof(prefix) - 1] = 0; // zero terminated
    char text[2 * 1024];
    if (format != null && !strequ(format, "")) {
        vsnprintf(text, countof(text) - 1, format, vl);
        text[countof(text) - 1] = 0;
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
    OutputDebugStringW(str.utf8_utf16(wide, output));
}

#else // posix version:

static void debug_vprintf(const char* file, int32_t line, const char* func,
        const char* format, va_list vl) {
    fprintf(stderr, "%s(%d): %s ", file, line, func);
    vfprintf(stderr, format, vl);
    fprintf(stderr, "\n");
}

#endif

static void debug_perrno(const char* file, int32_t line,
    const char* func, int32_t err_no, const char* format, ...) {
    if (err_no != 0) {
        if (format != null && !strequ(format, "")) {
            va_list vl;
            va_start(vl, format);
            debug.println_va(file, line, func, format, vl);
            va_end(vl);
        }
        debug.println(file, line, func, "errno: %d %s", err_no, strerror(err_no));
    }
}

static void debug_perror(const char* file, int32_t line,
    const char* func, int32_t error, const char* format, ...) {
    if (error != 0) {
        if (format != null && !strequ(format, "")) {
            va_list vl;
            va_start(vl, format);
            debug.println_va(file, line, func, format, vl);
            va_end(vl);
        }
        debug.println(file, line, func, "error: %s", str.error(error));
    }
}

static void debug_println(const char* file, int32_t line, const char* func,
        const char* format, ...) {
    va_list vl;
    va_start(vl, format);
    debug.println_va(file, line, func, format, vl);
    va_end(vl);
}

static bool debug_is_debugger_present(void) { return IsDebuggerPresent(); }

static void debug_breakpoint(void) {
    if (debug.is_debugger_present()) { DebugBreak(); }
}

static int32_t debug_verbosity_from_string(const char* s) {
    const char* n = null;
    long v = strtol(s, &n, 10);
    if (striequ(s, "quiet")) {
        return debug.verbosity.quiet;
    } else if (striequ(s, "info")) {
        return debug.verbosity.info;
    } else if (striequ(s, "verbose")) {
        return debug.verbosity.verbose;
    } else if (striequ(s, "debug")) {
        return debug.verbosity.debug;
    } else if (striequ(s, "trace")) {
        return debug.verbosity.trace;
    } else if (n > s && debug.verbosity.quiet <= v &&
               v <= debug.verbosity.trace) {
        return v;
    } else {
        fatal("invalid verbosity: %s", s);
        return debug.verbosity.quiet;
    }
}

static void debug_test(void) {
    #ifdef UT_TESTS
    // not clear what can be tested here
    if (debug.verbosity.level > debug.verbosity.quiet) { traceln("done"); }
    #endif
}

debug_if debug = {
    .verbosity = {
        .level   =  0,
        .quiet   =  0,
        .info    =  1,
        .verbose =  2,
        .debug   =  3,
        .trace   =  4,
    },
    .verbosity_from_string = debug_verbosity_from_string,
    .println               = debug_println,
    .println_va            = debug_println_va,
    .perrno                = debug_perrno,
    .perror                = debug_perror,
    .is_debugger_present   = debug_is_debugger_present,
    .breakpoint            = debug_breakpoint,
    .test                  = debug_test
};
