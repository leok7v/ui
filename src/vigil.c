#include "vigil.h"
#include "trace.h"
#include "rt.h"
#include <stdbool.h>
#include <stdlib.h>

begin_c

static int32_t vigil_failed_assertion(const char* file, int32_t line,
        const char* func, const char* condition, const char* format, ...) {
    va_list vl;
    va_start(vl, format);
    trace.vprintf(file, line, func, format, vl);
    va_end(vl);
    trace.printf(file, line, func, "assertion failed: %s\n", condition);
    // avoid warnings: conditional expression always true and unreachable code
    const bool always_true = trace.printf != null;
    if (always_true) { abort(); }
    return 0;
}
static int32_t vigil_fatal_termination(const char* file, int32_t line,
        const char* func, const char* condition, const char* format, ...) {
    const int32_t er = crt.err();
    const int32_t en = errno;
    va_list vl;
    va_start(vl, format);
    trace.vprintf(file, line, func, format, vl);
    va_end(vl);
    // report last errors:
    if (er != 0) { trace.perror(file, line, func, er, ""); }
    if (en != 0) { trace.perrno(file, line, func, en, ""); }
    if (condition != null && condition[0] != 0) {
        trace.printf(file, line, func, "FATAL: %s\n", condition);
    } else {
        trace.printf(file, line, func, "FATAL\n");
    }
    const bool always_true = trace.printf != null;
    if (always_true) { abort(); }
    return 0;
}

vigil_if vigil = {
    .failed_assertion  = vigil_failed_assertion,
    .fatal_termination = vigil_fatal_termination
};

end_c

