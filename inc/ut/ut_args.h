#pragma once
#include "ut/ut_std.h"

ut_begin_c

typedef struct {
    // On Unix it is responsibility of the main() to assign these values
    int32_t c;      // argc
    const char** v; // argv[argc]
    const char** env; // ut_args.env[] is null-terminated
    void    (*main)(int32_t argc, const char* argv[], const char** env);
    void    (*WinMain)(void); // windows specific
    int32_t (*option_index)(const char* option); // e.g. option: "--verbosity" or "-v"
    void    (*remove_at)(int32_t ix);
    /* argc=3 argv={"foo", "--verbose"} -> returns true; argc=1 argv={"foo"} */
    bool    (*option_bool)(const char* option);
    /* argc=3 argv={"foo", "--n", "153"} -> value==153, true; argc=1 argv={"foo"}
       also handles negative values (e.g. "-153") and hex (e.g. 0xBADF00D)
    */
    bool    (*option_int)(const char* option, int64_t *value);
    // for argc=3 argv={"foo", "--path", "bar"}
    //     option_str("--path", option)
    // returns option: "bar" and argc=1 argv={"foo"} */
    const char* (*option_str)(const char* option);
    // basename() for argc=3 argv={"/bin/foo.exe", ...} returns "foo":
    const char* (*basename)(void);
    void (*fini)(void);
    void (*test)(void);
} ut_args_if;

extern ut_args_if ut_args;

/* Usage:

    (both main() and WinMain() could be compiled at the same time on Windows):

    static int run(void);

    int main(int argc, char* argv[], char* envp[]) { // link.exe /SUBSYSTEM:CONSOLE
        ut_args.main(argc, argv, envp); // Initialize args with command-line parameters
        int r = run();
        ut_args.fini();
        return r;
    }

    #include "ut/ut_win32.h"

    int APIENTRY WinMain(HINSTANCE inst, HINSTANCE prev, char* command, int show) {
        // link.exe /SUBSYSTEM:WINDOWS
        ut_args.WinMain();
        int r = run();
        ut_args.fini();
        return 0;
    }

    static int run(void) {
        if (ut_args.option_bool("-v")) {
            ut_debug.verbosity.level = ut_debug.verbosity.verbose;
        }
        int64_t num = 0;
        if (ut_args.option_int("--number", &num)) {
            printf("--number: %ld\n", num);
        }
        const char* path = ut_args.option_str("--path");
        if (path != null) {
            printf("--path: %s\n", path);
        }
        printf("ut_args.basename(): %s\n", ut_args.basename());
        printf("ut_args.v[0]: %s\n", ut_args.v[0]);
        for (int i = 1; i < ut_args.c; i++) {
            printf("ut_args.v[%d]: %s\n", i, ut_args.v[i]);
        }
        return 0;
    }

    // Also see: https://github.com/leok7v/ut/blob/main/test/test1.c

*/

ut_end_c
