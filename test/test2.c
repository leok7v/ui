#define ut_implementation
#include "single_file_lib/ut.h"

int main(int argc, char* argv[], char *envp[]) {
    args.main(argc, argv, envp);
    const char* v = args.option_str("--verbosity");
    if (v != null) {
        debug.verbosity.level = debug.verbosity_from_string(v);
    } else if (args.option_bool("-v") || args.option_bool("--verbose")) {
        debug.verbosity.level = debug.verbosity.info;
    }
    runtime.test();
    traceln("all tests passed\n");
    args.fini();
    return 0;
}
