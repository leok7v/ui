#include <errno.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* amalgamate \uh-MAL-guh-mayt\ verb. : to unite in or as if in a
   mixture of elements; especially : to merge into a single body.
*/

#ifndef countof
#define countof(a) (sizeof(a) / sizeof((a)[0]))
#endif

// absence of #include <error.h> on Windows:

static void error(int32_t status, int32_t errnum, const char *format, ...) {
    if (status != 0) {
        va_list args;
        va_start(args, format);
        vfprintf(stderr, format, args);
        va_end(args);
        if (errnum != 0) {
            fprintf(stderr, "error: %d 0x%08X %s\n", errnum, errnum, strerror(errnum));
        }
        exit(status);
    }
}

int main(int argc, const char* argv[]) {
    for (int i = 1; i < argc; i++) {
        char fn[1024];
        snprintf(fn, countof(fn), "%s.h", argv[i]);
        FILE* f = fopen(fn, "r");
        if (f == NULL) {
            error(1, errno, "file not found %s\n", fn);
            return errno;
        } else {
            static char line[16 * 1024];
            while (fgets(line, countof(line), f) != NULL) {
                printf("%s", line);
            }
            fclose(f);
        }
    }
    return 0;
}
