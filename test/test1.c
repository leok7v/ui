#include "ut/ut.h"
#include <stdio.h>

static int usage(void) {
    fprintf(stderr, "Usage: %s [options]\n", ut_args.basename());
    fprintf(stderr, "Options:\n");
    fprintf(stderr, "  --help, -h     - this help\n");
    fprintf(stderr, "  --verbosity    - set verbosity level "
                                "(quiet, info, verbose, debug, trace)\n");
    fprintf(stderr, "  --verbose, -v  - set verbosity level to verbose\n");
    return 0;
}

static int run(void) {
    if (ut_args.option_bool("--help") || ut_args.option_bool("-?")) {
        return usage();
    }
    const char* v = ut_args.option_str("--verbosity");
    if (v != null) {
        ut_debug.verbosity.level = ut_debug.verbosity_from_string(v);
    } else if (ut_args.option_bool("-v") || ut_args.option_bool("--verbose")) {
        ut_debug.verbosity.level = ut_debug.verbosity.verbose;
    }
    ut_runtime.test();
    traceln("all tests passed\n");
    traceln("ut_args.v[0]: %s", ut_args.v[0]);
    traceln("ut_args.basename(): %s", ut_args.basename());
    for (int i = 1; i < ut_args.c; i++) {
        traceln("ut_args.v[%d]: %s", i, ut_args.v[i]);
    }
    //  $ .\bin\debug\test1.exe "Hello World" Hello World
    //  ut_args.v[0]: .\bin\debug\test1.exe
    //  ut_args.basename(): test1
    //  ut_args.v[1]: Hello World
    //  ut_args.v[2]: Hello
    //  ut_args.v[3]: World
    return 0;
}

// both main() wand WinMain() can be present and compiled.
// Runtime does something along the lines:
// #include <winnt.h>
// #include <dbghelp.h>
//   IMAGE_NT_HEADERS32* h = ImageNtHeader(GetModuleHandle(null));
//   h->OptionalHeader.Subsystem == IMAGE_SUBSYSTEM_WINDOWS_CUI
//   h->OptionalHeader.Subsystem == IMAGE_SUBSYSTEM_WINDOWS_GUI
// to select and call appropriate function:

int main(int argc, char* argv[], char *envp[]) {
    ut_args.main(argc, argv, envp);
    int r = run();
    ut_args.fini();
    return r;
}

#include "ut/ut_win32.h"

#pragma warning(suppress: 28251) // no annotations

int APIENTRY WinMain(HINSTANCE unused(inst), HINSTANCE unused(prev),
                     char* unused(command), int unused(show)) {
    ut_args.WinMain(); // Uses GetCommandLineW() which has full pathname
    int r = run();
    ut_args.fini();
    return r;
}
