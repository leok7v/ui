#include "ut/runtime.h"

// for more details run with
// --verbosity info (aka -v)
// or
// --verbosity trace

int main(int argc, char* argv[], char *envp[]) {
    #ifdef WINDOWS // see static_init(args)
        swear(args.c == argc && args.v == argv && args.env == envp);
    #endif
    args.c = argc; // On Unix it is necessary
    args.v = argv;
    args.env = envp;
    const char* v = args.option_str(&argc, argv, "--verbosity");
    if (v != null) {
        debug.verbosity.level = debug.verbosity_from_string(v);
    } else if (args.option_bool(&argc, argv, "-v") ||
               args.option_bool(&argc, argv, "--verbose")) {
        debug.verbosity.level = debug.verbosity.info;
    }
    runtime.test();
    traceln("all tests passed\n");
    return 0;
}
