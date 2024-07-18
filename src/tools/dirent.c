#include "dirent.h"
#include <stdio.h>
#include <malloc.h>
#include <Windows.h>

#if defined(__GNUC__) || defined(__clang__) // TODO: remove and fix code
#pragma GCC diagnostic ignored "-Wdeclaration-after-statement"
#pragma GCC diagnostic ignored "-Wunsafe-buffer-usage"
#endif

#define null ((void*)0)

#ifndef countof
    #define ut_countof(a) ((int)(sizeof(a) / sizeof((a)[0])))
#endif

typedef struct dir_s {
    HANDLE handle;
    WIN32_FIND_DATAA find; // 320 bytes
    struct dirent entry;
} dir_t;

DIR *opendir(const char *dirname) {
    dir_t *d = calloc(1, sizeof(dir_t));
    if (d != null) {
        char spec[NAME_MAX + 2]; // extra room for "\*" suffix
        snprintf(spec, ut_countof(spec) - 1, "%s\\*", dirname);
        spec[ut_countof(spec) - 1] = 0;
        d->handle = FindFirstFileA(spec, &d->find);
        if (d->handle == INVALID_HANDLE_VALUE) {
            free(d);
            d = null;
        }
    }
    return (DIR*)d;
}

struct dirent* readdir(DIR* dir) {
    dir_t* d = (dir_t*)dir;
    struct dirent* de = null;
    if (d->handle != INVALID_HANDLE_VALUE &&
        FindNextFileA(d->handle, &d->find)) {
        enum { n = ut_countof(d->entry.d_name) };
        strncpy(d->entry.d_name, d->find.cFileName, n - 1);
        d->entry.d_name[n - 1] = 0x00; // Ensure zero termination
        de = &d->entry;
    }
    return de;
}

int closedir(DIR* dir) {
    errno_t e = 0;
    dir_t *d = (dir_t*)dir;
    if (d->handle != INVALID_HANDLE_VALUE) {
        if (!FindClose(d->handle)) { e = EINVAL; }
    }
    if (e == 0) { free(d); }
    return e;
}
