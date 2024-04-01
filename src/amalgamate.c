#include "runtime.h"
// #include <stdarg.h>
// #include <stdint.h>
// #include <stdio.h>
// #include <stdlib.h>
// #include <string.h>
#include <direct.h>

// amalgamate \uh-MAL-guh-mayt\ verb. : to unite in or as if in a
// mixture of elements; especially : to merge into a single body.

static char sys_includes[1024 * 1024];
static char usr_includes[1024 * 1024];

static void append_to_includes(const char* include) {
    if (strstartswith(include, "#include \"")) {
        if (strstr(usr_includes, include) == null) {
            fatal_if_not_zero(
                strcat_s(usr_includes, countof(usr_includes), include)
            );
        }
    } else if (strstartswith(include, "#include <")) {
        if (strstr(sys_includes, include) == null) {
            fatal_if_not_zero(
                strcat_s(sys_includes, countof(usr_includes), include)
            );
        }
    } else {
        fatal("expected #include: %s", include);
    }
}

static char* trim_cr_lf(char* s) {
    int32_t n = strlength(s);
    while (n > 0 && s[n] < 0x20) { s[n] = 0; n--; }
    return s;
}

static void inc(const char* fn) {
    traceln("%s", fn);
    FILE* f = fopen(fn, "r");
    if (f == null) {
        fatal("file not found: %s", fn);
    } else {
        fclose(f);
    }
}

static void src(const char* fn) {
    traceln("%s", fn);
    FILE* f = fopen(fn, "r");
    if (f == null) {
        fatal("file not found: %s", fn);
    } else {
        fclose(f);
    }
}

static char folder[1024];

static void msvc_folder_up2(const char* argv0) {
    // On github CI the bin/release/foo.exe is usually
    // started at the root of the repository (unless some
    // cd or pushd was invoked)
    // In MSVC by default executables start at projects
    // folder which is one or two level deep from the root
    strcpy(folder, argv0);
    char* last = strrchr(folder, '\\');
    fatal_if_null(last);
    *last = 0;
    if (strendswith(folder, "bin\\release") ||
        strendswith(folder, "bin\\Release") ||
        strendswith(folder, "bin\\debug") ||
        strendswith(folder, "bin\\Debug")) {
        char cd[4 * 1024];
        strprintf(cd, "%s\\..\\..", folder);
        fatal_if_not_zero(_chdir(cd));
    }
}

int main(int unused(argc), const char* argv[]) {
    msvc_folder_up2(argv[0]); // msvc debugging convenience
    fatal_if_null(getcwd(folder, countof(folder)));
    traceln("%s", folder);
    static const char* filename = "amalgamate.txt";
    FILE* f = fopen(filename, "r");
    if (f == null) {
        fatal("file not found: %s", filename);
    } else {
        static char s[16 * 1024];
        while (fgets(s, countof(s), f) != null) {
//          printf("%s", s);
            if (s[0] == '#') {
                // skip comment
            } else if (strstartswith(s, "inc")) {
                inc(trim_cr_lf(s));
            } else if (strstartswith(s, "src")) {
                src(trim_cr_lf(s));
            }
        }
        fclose(f);
    }
    return 0;
}
