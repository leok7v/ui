#include "ut/ut.h"
#include <stdio.h>

static int usage(void) {
    fprintf(stderr, "Usage: %s [options]\n", args.basename());
    fprintf(stderr, "Options:\n");
    fprintf(stderr, "  --help, -h     - this help\n");
    fprintf(stderr, "  --verbosity    - set verbosity level "
                                "(quiet, info, verbose, debug, trace)\n");
    fprintf(stderr, "  --verbose, -v  - set verbosity level to verbose\n");
    return 0;
}

static int run(void) {
    if (args.option_bool("--help") || args.option_bool("-?")) {
        return usage();
    }
    const char* v = args.option_str("--verbosity");
    if (v != null) {
        debug.verbosity.level = debug.verbosity_from_string(v);
    } else if (args.option_bool("-v") || args.option_bool("--verbose")) {
        debug.verbosity.level = debug.verbosity.verbose;
    }
    runtime.test();
    traceln("all tests passed\n");
    traceln("args.v[0]: %s", args.v[0]);
    traceln("args.basename(): %s", args.basename());
    for (int i = 1; i < args.c; i++) {
        const char* ai = args.unquote(&args.v[i]);
        traceln("args.v[%d]: %s", i, ai);
    }
    //  $ .\bin\debug\test1.exe "Hello World" Hello World
    //  args.v[0]: .\bin\debug\test1.exe
    //  args.basename(): test1
    //  args.v[1]: Hello World
    //  args.v[2]: Hello
    //  args.v[3]: World
    return 0;
}

int main(int argc, char* argv[], char *envp[]) {
    args.main(argc, argv, envp);
    int r = run();
    args.fini();
    return r;
}

#include "ut/win32.h"

#pragma warning(suppress: 28251) // no annotations

int APIENTRY WinMain(HINSTANCE unused(inst), HINSTANCE unused(prev),
                     char* cl, int unused(show)) {
    args.WinMain(cl);
    int r = run();
    args.fini();
    return r;
}
