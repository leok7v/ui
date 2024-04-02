#include "runtime.h"

static int32_t string_to_verbosity(const char* s) {
    const char* n = null;
    long v = strtol(s, &n, 10);
    if (str.equal_nc(s, "info")) {
        return debug.info;
    } else if (str.equal_nc(s, "warn")) {
        return debug.warn;
    } else if (str.equal_nc(s, "error")) {
        return debug.error;
    } else if (str.equal_nc(s, "fatal")) {
        return debug.fatal;
    } else if (str.equal_nc(s, "debug")) {
        return debug.debug;
    } else if (str.equal_nc(s, "trace")) {
        return debug.trace;
    } else if (0 <= v && v <= 6 && n > s) {
        return v;
    } else {
        fatal("invalid verbosity: %s", s);
        return 0;
    }
}

// for more details run with
// --verbosity info (aka -v)
// or
// --verbosity trace

int main(int argc, const char* argv[]) {
    const char* vs = args.option_str(&argc, argv, "--verbosity");
    if (vs != null) {
        debug.verbosity = string_to_verbosity(vs);
    } else if (args.option_bool(&argc, argv, "--verbose") ||
              args.option_bool(&argc, argv, "-v")) {
        debug.verbosity = debug.info;
    }
    runtime.test(debug.verbosity);
    if (debug.verbosity > 0) { printf("all tests complete\n"); }
    return 0;
}

