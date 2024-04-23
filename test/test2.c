#include <assert.h>
#include <ctype.h>
#include <fcntl.h>
#include <io.h>
#include <malloc.h>
#define _USE_MATH_DEFINES
#include <math.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>

#define ut_implementation
#include "single_file_lib/ut/ut.h"

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
