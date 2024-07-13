#define ut_implementation
#include "single_file_lib/ut/ut.h"

int main(int argc, char* argv[], char *envp[]) {
    ut_args.main(argc, argv, envp);
    const char* v = ut_args.option_str("--verbosity");
    if (v != null) {
        ut_debug.verbosity.level = ut_debug.verbosity_from_string(v);
    } else if (ut_args.option_bool("-v") || ut_args.option_bool("--verbose")) {
        ut_debug.verbosity.level = ut_debug.verbosity.info;
    }
    ut_runtime.test();
    ut_traceln("all tests passed\n");
    ut_args.fini();
    return 0;
}
