#include "ut/ut.h"

// for more details run with
// --verbosity info (aka -v)
// or
// --verbosity trace

static int run(void) {
    const char* v = args.option_str("--verbosity");
    if (v != null) {
        debug.verbosity.level = debug.verbosity_from_string(v);
    } else if (args.option_bool("-v") || args.option_bool("--verbose")) {
        debug.verbosity.level = debug.verbosity.info;
    }
    runtime.test();
    traceln("all tests passed\n");
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
