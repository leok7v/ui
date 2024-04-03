#include "runtime.h"
#include "win32.h"

begin_c

typedef struct folders_data_s {
    WIN32_FIND_DATAA ffd;
} folders_data_t;

typedef struct folders_s {
    int32_t n;
    int32_t capacity; // number of heap allocated bytes
    int32_t fd;
    char* folder;
    folders_data_t* data;
} folders_t_;

static folders_t* folders_open(void) {
    return (folders_t_*)calloc(sizeof(folders_t_), 1);
}

void folders_close(folders_t* dirs) {
    folders_t_* d = (folders_t_*)dirs;
    if (d != null) {
        free(d->data);  d->data = null;
        free(d->folder); d->folder = null;
    }
    free(d);
}

const char* folders_folder_name(folders_t* dirs) {
    folders_t_* d = (folders_t_*)dirs;
    return d->folder;
}

int32_t folders_count(folders_t* dirs) {
    folders_t_* d = (folders_t_*)dirs;
    return d->n;
}

#define return_time_field(field) \
    folders_t_* d = (folders_t_*)dirs; \
    assert(0 <= i && i < d->n, "assertion %d out of range [0..%d[", i, d->n); \
    return 0 <= i && i < d->n ? \
        (((uint64_t)d->data[i].ffd.field.dwHighDateTime) << 32 | \
                    d->data[i].ffd.field.dwLowDateTime) * 100 : 0

#define return_bool_field(field, bit) \
    folders_t_* d = (folders_t_*)dirs; \
    assert(0 <= i && i < d->n, "assertion %d out of range [0..%d[", i, d->n); \
    return 0 <= i && i < d->n ? (d->data[i].ffd.field & bit) != 0 : false

#define return_int64_file_size() \
    folders_t_* d = (folders_t_*)dirs; \
    assert(0 <= i && i < d->n, "assertion %d out of range [0..%d[", i, d->n); \
    return 0 <= i && i < d->n ? \
        (int64_t)(((uint64_t)d->data[i].ffd.nFileSizeHigh) << 32 | \
        d->data[i].ffd.nFileSizeLow) : -1

const char* folders_filename(folders_t* dirs, int32_t i) {
    folders_t_* d = (folders_t_*)dirs;
    assert(0 <= i && i < d->n, "assertion %d out of range [0..%d[", i, d->n);
    return 0 <= i && i < d->n ? d->data[i].ffd.cFileName : null;
}

bool folders_is_folder(folders_t* dirs, int32_t i) {
    return_bool_field(dwFileAttributes, FILE_ATTRIBUTE_DIRECTORY);
}

bool folders_is_symlink(folders_t* dirs, int32_t i) {
    return_bool_field(dwFileAttributes, FILE_ATTRIBUTE_REPARSE_POINT);
}

int64_t folders_file_size(folders_t* dirs, int32_t i) {
    return_int64_file_size();
}

// functions folders_time_*() return time in absolute nanoseconds since
// start of OS epoch or 0 if failed or not available

uint64_t folders_time_created(folders_t* dirs, int32_t i) {
    return_time_field(ftCreationTime);
}

uint64_t folders_time_updated(folders_t* dirs, int32_t i) {
    return_time_field(ftLastWriteTime);
}

uint64_t folders_time_accessed(folders_t* dirs, int32_t i) {
    return_time_field(ftLastAccessTime);
}

int32_t folders_enumerate(folders_t* dirs, const char* folder) {
    folders_t_* d = (folders_t_*)dirs;
    WIN32_FIND_DATAA ffd = {0};
    int32_t k = (int32_t)strlen(folder);
    if (k > 0 &&
       (folder[k - 1] == '/' ||
        folder[k - 1] == '\\')) {
        assert(folder[k - 1] != '/' && folder[k - 1] != '\\',
            "folder name should not contain trailing [back] slash: %s", folder);
        k--;
    }
    if (k == 0) {
        return -1;
    }
    int32_t pattern_length = k + 3;
    char* pattern = (char*)stackalloc(pattern_length);
    str.sformat(pattern, pattern_length, "%-*.*s/*", k, k, folder);
    if (d->folder != null) { free(d->folder); d->folder = null; }
    d->folder = (char*)malloc(k + 1);
    if (d->folder == null) {
        return -1;
    }
    str.sformat(d->folder, k + 1, "%s", folder);
    assert(strequ(d->folder, folder));
    if (d->capacity == 0 && d->n == 0 && d->data == null) {
        d->capacity = 128;
        d->n = 0;
        d->data = (folders_data_t*)malloc(sizeof(folders_data_t) * d->capacity);
        if (d->data == null) {
            free(d->data);
            d->capacity = 0;
            d->data = null;
        }
    }
    assert(d->capacity > 0 && d->n <= d->capacity && d->data != null,
        "inconsistent values of n=%d allocated=%d", d->n, d->capacity);
    d->n = 0;
    if (d->capacity > 0 && d->n <= d->capacity && d->data != null) {
        int32_t n = (int32_t)(strlen(folder) + countof(ffd.cFileName) + 3);
        char* pathname = (char*)stackalloc(n);
        HANDLE h = FindFirstFileA(pattern, &ffd);
        if (h != INVALID_HANDLE_VALUE) {
            do {
                if (strequ(".", ffd.cFileName) || strequ("..", ffd.cFileName)) { continue; }
                if (d->n >= d->capacity) {
                    folders_data_t* r = (folders_data_t*)realloc(d->data,
                        sizeof(folders_data_t) * d->capacity * 2);
                    if (r != null) {
                        // out of memory - do the best we can, leave the rest for next pass
                        d->capacity = d->capacity * 2;
                        d->data = r;
                    }
                }
                if (d->n < d->capacity && d->data != null) {
                    str.sformat(pathname, n, "%s/%s", folder, ffd.cFileName);
 //                 traceln("%s", pathname);
                    d->data[d->n].ffd = ffd;
                    d->n++;
                } else {
                    return -1; // keep the data we have so far intact
                }
            } while (FindNextFileA(h, &ffd));
            FindClose(h);
        }
        return 0;
    }
    return -1;
}

static void folders_test(void) {
    #ifdef RUNTIME_TESTS
    traceln("TODO");
    if (debug.verbosity.level > debug.verbosity.quiet) { traceln("done"); }
    #endif
}

folders_if folders = {
    .open        = folders_open,
    .enumerate   = folders_enumerate,
    .folder_name = folders_folder_name,
    .count       = folders_count,
    .name        = folders_filename,
    .is_folder   = folders_is_folder,
    .is_symlink  = folders_is_symlink,
    .size        = folders_file_size,
    .created     = folders_time_created,
    .updated     = folders_time_updated,
    .accessed    = folders_time_accessed,
    .close       = folders_close,
    .test        = folders_test
};

end_c

