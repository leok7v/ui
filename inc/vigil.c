#include "vigil.h"
#include "trace.h"
#include <stdlib.h>

begin_c

#pragma push_macro("ns") // namespace
#pragma push_macro("fn") // function

#define ns(name) vigil_ ## name
#define fn(type, name) static type ns(name)

fn(int, failed_assertion)(const char* file, int line,
        const char* func, const char* condition, const char* format, ...) {
    va_list vl;
    va_start(vl, format);
    trace.vtraceline(file, line, func, format, vl);
    va_end(vl);
    trace.traceline(file, line, func, "assertion failed: %s\n", condition);
    // to avoid 2 warning: conditional expression always true:
    if (trace.traceline != null) { abort(); }
    // and unreachable code:
    return 0;
}

fn(int, fatal_termination)(const char* file, int line,
        const char* func, const char* condition, const char* format, ...) {
    va_list vl;
    va_start(vl, format);
    trace.vtraceline(file, line, func, format, vl);
    va_end(vl);
    if (condition != null && condition[0] != 0) {
        trace.traceline(file, line, func, "FATAL: %s\n", condition);
    } else {
        trace.traceline(file, line, func, "FATAL\n");
    }
    if (trace.traceline != null) { abort(); }
    return 0;
}

vigil_if vigil = {
    .failed_assertion  = ns(failed_assertion),
    .fatal_termination = ns(fatal_termination)
};

#pragma pop_macro("fn") // function
#pragma pop_macro("ns") // namespace

end_c

