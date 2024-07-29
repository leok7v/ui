#define ut_implementation
#include "single_file_lib/ut/ut.h"

int main(int argc, char* argv[], char *envp[]) {
    rt_args.main(argc, argv, envp);
    const char* v = rt_args.option_str("--verbosity");
    if (v != null) {
        rt_debug.verbosity.level = rt_debug.verbosity_from_string(v);
    } else if (rt_args.option_bool("-v") || rt_args.option_bool("--verbose")) {
        rt_debug.verbosity.level = rt_debug.verbosity.info;
    }
    rt_core.test();
    ut_println("all tests passed\n");
    rt_args.fini();
    return 0;
}
