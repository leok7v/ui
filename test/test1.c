#include "posix.h"
#include <stdio.h>

static int usage(void) {
    fprintf(stderr, "Usage: %s [options]\n", posix_args.basename());
    fprintf(stderr, "Options:\n");
    fprintf(stderr, "  --help, -h     - this help\n");
    fprintf(stderr, "  --verbosity    - set verbosity level "
                                "(quiet, info, verbose, debug, trace)\n");
    fprintf(stderr, "  --verbose, -v  - set verbosity level to verbose\n");
    return 0;
}

static int run(void) {
    if (posix_args.option_bool("--help") || posix_args.option_bool("-?")) {
        return usage();
    }
    const char* v = posix_args.option_str("--verbosity");
    if (v != null) {
        posix_debug.verbosity.level = posix_debug.verbosity_from_string(v);
    } else if (posix_args.option_bool("-v") || posix_args.option_bool("--verbose")) {
        posix_debug.verbosity.level = posix_debug.verbosity.verbose;
    }
    posix_core.test();
    posix_println("all tests passed\n\n");
//  posix_println("posix_args.basename(): %s", posix_args.basename());
//  posix_println("posix_args.v[0]: %s", posix_args.v[0]);
//  for (int i = 1; i < posix_args.c; i++) {
//      posix_println("posix_args.v[%d]: %s", i, posix_args.v[i]);
//  }
    //  $ .\bin\debug\test1.exe "Hello World" Hello World
    //  posix_args.v[0]: .\bin\debug\test1.exe
    //  posix_args.basename(): test1
    //  posix_args.v[1]: Hello World
    //  posix_args.v[2]: Hello
    //  posix_args.v[3]: World
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
    posix_args.main(argc, argv, envp);
    int r = run();
    posix_args.fini();
    return r;
}

#include "ui/ui_win32.h"

#pragma warning(suppress: 28251) // no annotations

int APIENTRY WinMain(HINSTANCE posix_unused(inst), HINSTANCE posix_unused(prev),
                     char* posix_unused(command), int posix_unused(show)) {
    posix_args.WinMain(); // Uses GetCommandLineW() which has full pathname
    int r = run();
    posix_args.fini();
    return r;
}
