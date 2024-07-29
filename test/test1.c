#include "ut/ut.h"
#include <stdio.h>

static int usage(void) {
    fprintf(stderr, "Usage: %s [options]\n", rt_args.basename());
    fprintf(stderr, "Options:\n");
    fprintf(stderr, "  --help, -h     - this help\n");
    fprintf(stderr, "  --verbosity    - set verbosity level "
                                "(quiet, info, verbose, debug, trace)\n");
    fprintf(stderr, "  --verbose, -v  - set verbosity level to verbose\n");
    return 0;
}

static int run(void) {
    if (rt_args.option_bool("--help") || rt_args.option_bool("-?")) {
        return usage();
    }
    const char* v = rt_args.option_str("--verbosity");
    if (v != null) {
        rt_debug.verbosity.level = rt_debug.verbosity_from_string(v);
    } else if (rt_args.option_bool("-v") || rt_args.option_bool("--verbose")) {
        rt_debug.verbosity.level = rt_debug.verbosity.verbose;
    }
    rt_core.test();
    rt_println("all tests passed\n\n");
//  rt_println("rt_args.basename(): %s", rt_args.basename());
//  rt_println("rt_args.v[0]: %s", rt_args.v[0]);
//  for (int i = 1; i < rt_args.c; i++) {
//      rt_println("rt_args.v[%d]: %s", i, rt_args.v[i]);
//  }
    //  $ .\bin\debug\test1.exe "Hello World" Hello World
    //  rt_args.v[0]: .\bin\debug\test1.exe
    //  rt_args.basename(): test1
    //  rt_args.v[1]: Hello World
    //  rt_args.v[2]: Hello
    //  rt_args.v[3]: World
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

int main(int argc, const char* argv[], const char *envp[]) {
    rt_args.main(argc, argv, envp);
    int r = run();
    rt_args.fini();
    return r;
}

#include "ut/ut_win32.h"

#pragma warning(suppress: 28251) // no annotations

int APIENTRY WinMain(HINSTANCE rt_unused(inst), HINSTANCE rt_unused(prev),
                     char* rt_unused(command), int rt_unused(show)) {
    rt_args.WinMain(); // Uses GetCommandLineW() which has full pathname
    int r = run();
    rt_args.fini();
    return r;
}
