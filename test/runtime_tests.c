#include "runtime.h"

// for more details run with
// --verbosity info (aka -v)
// or
// --verbosity trace

int main(int argc, const char* argv[]) {
    const char* v = args.option_str(&argc, argv, "--verbosity");
    if (v != null) {
        debug.verbosity.level = debug.verbosity_from_string(v);
    } else if (args.option_bool(&argc, argv, "-v") ||
               args.option_bool(&argc, argv, "--verbose")) {
        debug.verbosity.level = debug.verbosity.info;
    }
    runtime.test();
    if (debug.verbosity.level > 0) { printf("all tests complete\n"); }
    return 0;
}
