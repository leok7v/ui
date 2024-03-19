#include "rt.h"
#include "trace.h"
#include <stdint.h>
#include <stdio.h>
#include <string.h>

begin_c

#pragma push_macro("ns") // namespace
#pragma push_macro("fn") // function

#define ns(name) trace_ ## name
#define fn(type, name) static type ns(name)

#ifdef WINDOWS

#ifndef OutputDebugString
__declspec(dllimport) void __stdcall OutputDebugStringW(const uint16_t* s);
#pragma comment(lib, "Kernel32")
#endif

fn(void, vtraceline)(const char* file, int line, const char* func,
        const char* format, va_list vl) {
    // snprintf() does not guarantee zero termination on truncation
    char prefix[2 * 1024];
    snprintf(prefix, countof(prefix) - 1, "%s(%d): %s", file, line, func);
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
    // strip trailing \n which can remain present in dirty lazy
    // manual translation of fprintf() to traceln() debugging
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
    OutputDebugStringW(utf8to16(output));
}

#else // posix version:

fn(void, vtraceline)(const char* file, int line, const char* func,
        const char* format, va_list vl) {
    fprintf(stderr, "%s(%d): %s ", file, line, func);
    vfprintf(stderr, format, vl);
    fprintf(stderr, "\n");
}

#endif

fn(void, traceline)(const char* file, int line, const char* func,
        const char* format, ...) {
    va_list vl;
    va_start(vl, format);
    ns(vtraceline)(file, line, func, format, vl);
    va_end(vl);
}

trace_if trace = {
    .traceline  = ns(traceline),
    .vtraceline = ns(vtraceline)
};

#pragma pop_macro("fn") // function
#pragma pop_macro("ns") // namespace

end_c
