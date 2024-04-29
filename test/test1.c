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
        traceln("args.v[%d]: %s", i, args.v[i]);
    }
    //  $ .\bin\debug\test1.exe "Hello World" Hello World
    //  args.v[0]: .\bin\debug\test1.exe
    //  args.basename(): test1
    //  args.v[1]: Hello World
    //  args.v[2]: Hello
    //  args.v[3]: World
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
    args.main(argc, argv, envp);
    int r = run();
    args.fini();
    return r;
}

#include "ut/ut_win32.h"

#pragma warning(suppress: 28251) // no annotations

int APIENTRY WinMain(HINSTANCE unused(inst), HINSTANCE unused(prev),
                     char* unused(command), int unused(show)) {
    args.WinMain(); // Uses GetCommandLineW() which has full pathname
    int r = run();
    args.fini();
    return r;
}
