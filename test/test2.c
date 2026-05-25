#include "posix.h"

int main(int argc, char* argv[], char *envp[]) {
    posix_args.main(argc, argv, envp);
    const char* v = posix_args.option_str("--verbosity");
    if (v != null) {
        posix_debug.verbosity.level = posix_debug.verbosity_from_string(v);
    } else if (posix_args.option_bool("-v") || posix_args.option_bool("--verbose")) {
        posix_debug.verbosity.level = posix_debug.verbosity.info;
    }
    posix_core.test();
    posix_println("all tests passed\n");
    posix_args.fini();
    return 0;
}
