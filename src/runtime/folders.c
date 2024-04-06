#include "runtime/runtime.h"
#include "runtime/win32.h"

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

static const char* folders_bin(void) {
    static char program_files[1024];
    if (program_files[0] == 0) {
        wchar_t* program_files_w = null;
        fatal_if(SHGetKnownFolderPath(&FOLDERID_ProgramFilesX64, 0,
            null, &program_files_w) != 0);
        int32_t len = (int32_t)wcslen(program_files_w);
        assert(len < countof(program_files));
        fatal_if(len >= countof(program_files), "len=%d", len);
        for (int32_t i = 0; i <= len; i++) { // including zero terminator
            assert(program_files_w[i] < 128); // pure ascii
            program_files[i] = (char)program_files_w[i];
        }
    }
    return program_files;
}

static const char* folders_tmp(void) {
    static char tmp[1024];
    if (tmp[0] == 0) {
        // If GetTempPathA() succeeds, the return value is the length,
        // in chars, of the string copied to lpBuffer, not including
        // the terminating null character. If the function fails, the
        // return value is zero.
        errno_t r = GetTempPathA(countof(tmp), tmp) == 0 ? runtime.err() : 0;
        fatal_if(r != 0, "GetTempPathA() failed %s", str.error(r));
    }
    return tmp;
}

static const char* folders_data(void) {
    static char program_data[1024];
    if (program_data[0] == 0) {
        wchar_t* program_data_w = null;
        fatal_if(SHGetKnownFolderPath(&FOLDERID_ProgramData, 0,
            null, &program_data_w) != 0);
        int32_t len = (int32_t)wcslen(program_data_w);
        assert(len < countof(program_data));
        fatal_if(len >= countof(program_data), "len=%d", len);
        for (int32_t i = 0; i <= len; i++) { // including zero terminator
            assert(program_data_w[i] < 128); // pure ascii
            program_data[i] = (char)program_data_w[i];
        }
    }
    return program_data;
}

void folders_close(folders_t* fs) {
    folders_t_* d = (folders_t_*)fs;
    if (d != null) {
        heap.deallocate(null, d->data);   d->data = null;
        heap.deallocate(null, d->folder); d->folder = null;
    }
    heap.deallocate(null, d);
}

const char* folders_folder(folders_t* fs) {
    folders_t_* d = (folders_t_*)fs;
    return d->folder;
}

int32_t folders_count(folders_t* fs) {
    folders_t_* d = (folders_t_*)fs;
    return d->n;
}

#define folders_return_time_field(field) \
    folders_t_* d = (folders_t_*)fs; \
    assert(0 <= i && i < d->n, "assertion %d out of range [0..%d[", i, d->n); \
    return 0 <= i && i < d->n ? \
        (((uint64_t)d->data[i].ffd.field.dwHighDateTime) << 32 | \
                    d->data[i].ffd.field.dwLowDateTime) / 10 : 0

#define folders_return_bool_field(field, bit) \
    folders_t_* d = (folders_t_*)fs; \
    assert(0 <= i && i < d->n, "assertion %d out of range [0..%d[", i, d->n); \
    return 0 <= i && i < d->n ? (d->data[i].ffd.field & bit) != 0 : false

#define folders_return_int64_file_size() \
    folders_t_* d = (folders_t_*)fs; \
    assert(0 <= i && i < d->n, "assertion %d out of range [0..%d[", i, d->n); \
    return 0 <= i && i < d->n ? \
        (int64_t)(((uint64_t)d->data[i].ffd.nFileSizeHigh) << 32 | \
        d->data[i].ffd.nFileSizeLow) : -1

const char* folders_filename(folders_t* fs, int32_t i) {
    folders_t_* d = (folders_t_*)fs;
    assert(0 <= i && i < d->n, "assertion %d out of range [0..%d[", i, d->n);
    return 0 <= i && i < d->n ? d->data[i].ffd.cFileName : null;
}

bool folders_is_folder(folders_t* fs, int32_t i) {
    folders_return_bool_field(dwFileAttributes, FILE_ATTRIBUTE_DIRECTORY);
}

bool folders_is_symlink(folders_t* fs, int32_t i) {
    folders_return_bool_field(dwFileAttributes, FILE_ATTRIBUTE_REPARSE_POINT);
}

int64_t folders_bytes(folders_t* fs, int32_t i) {
    folders_return_int64_file_size();
}

// functions folders_time_*() return time in absolute nanoseconds since
// start of OS epoch or 0 if failed or not available

uint64_t folders_time_created(folders_t* fs, int32_t i) {
    folders_return_time_field(ftCreationTime);
}

uint64_t folders_time_updated(folders_t* fs, int32_t i) {
    folders_return_time_field(ftLastWriteTime);
}

uint64_t folders_time_accessed(folders_t* fs, int32_t i) {
    folders_return_time_field(ftLastAccessTime);
}

errno_t folders_enumerate(folders_t* d, const char* fn) {
    errno_t r = 0;
    WIN32_FIND_DATAA ffd = {0};
    int32_t k = (int32_t)strlen(fn);
    // remove trailing backslash (except if it is root: "/" or "\\")
    if (k > 1 && (fn[k - 1] == '/' || fn[k - 1] == '\\')) {
        k--;
    }
    int32_t pattern_length = k + 3;
    char* pattern = (char*)stackalloc(pattern_length);
    if (!strequ(fn, "\\") && strequ(fn, "/")) {
        str.sformat(pattern, pattern_length, "\\*");
    } else {
        str.sformat(pattern, pattern_length, "%-*.*s\\*", k, k, fn);
    }
    d->folder = (char*)heap.allocate(null, k + 1, true);
    if (d->folder != null) {
        str.sformat(d->folder, k + 1, "%.*s", k, fn);
        d->capacity = 128;
        d->n = 0;
        const int64_t bytes = sizeof(folders_data_t) * d->capacity;
        d->data = (folders_data_t*)heap.allocate(null, bytes, true);
        if (d->data == null) {
            heap.deallocate(null, d->data);
            d->capacity = 0;
            d->data = null;
            r = ERROR_OUTOFMEMORY;
        } else {
            assert(d->capacity > 0 && d->n <= d->capacity && d->data != null,
                "inconsistent values of n=%d allocated=%d", d->n, d->capacity);
            d->n = 0;
            if (d->capacity > 0 && d->n <= d->capacity && d->data != null) {
                HANDLE h = FindFirstFileA(pattern, &ffd);
                if (h != INVALID_HANDLE_VALUE) {
                    do {
                        if (!strequ(".", ffd.cFileName) &&
                            !strequ("..", ffd.cFileName)) {
                            if (d->n >= d->capacity) {
                                const int64_t bytes_x_2 =
                                    sizeof(folders_data_t) * d->capacity * 2;
                                r = heap.reallocate(null, &d->data,
                                    bytes_x_2, false);
                                if (r == 0) {
                                    d->capacity = d->capacity * 2;
                                }
                            }
                            if (r == 0) {
                                d->data[d->n].ffd = ffd;
                                d->n++;
                            } else {
                                r = ERROR_OUTOFMEMORY;
                            }
                        }
                    } while (FindNextFileA(h, &ffd));
                    fatal_if_false(runtime.err() == ERROR_NO_MORE_FILES);
                    fatal_if_false(FindClose(h));
                }
            }
        }
    }
    return r;
}

static errno_t folders_open(folders_t* *fs, const char* pathname) {
    folders_t_* d = (folders_t_*)heap.allocate(null,
        sizeof(folders_t_), true);
    *fs = d;
    return d != null ? folders_enumerate(d, pathname) : ERROR_OUTOFMEMORY;
}

#ifdef RUNTIME_TESTS

#pragma push_macro("verbose") // --verbosity trace

#define verbose(...) do {                                 \
    if (debug.verbosity.level >= debug.verbosity.trace) { \
        traceln(__VA_ARGS__);                             \
    }                                                     \
} while (0)

static void folders_test(void) {
    uint64_t now = clock.microseconds(); // microseconds since epoch
    uint64_t before = now - 1 * clock.usec_in_sec; // one second earlier
    uint64_t after  = now + 2 * clock.usec_in_sec; // two seconds later
    int32_t year = 0;
    int32_t month = 0;
    int32_t day = 0;
    int32_t hh = 0;
    int32_t mm = 0;
    int32_t ss = 0;
    int32_t ms = 0;
    int32_t mc = 0;
    clock.local(now, &year, &month, &day, &hh, &mm, &ss, &ms, &mc);
    verbose("now: %04d-%02d-%02d %02d:%02d:%02d.%3d:%3d",
             year, month, day, hh, mm, ss, ms, mc);
    // there is no racing free way to create temporary folder
    // without having a temporary file for the duration of folder usage:
    char tmp_file[1024]; // create_tmp() is thread safe race free:
    errno_t r = files.create_tmp(tmp_file, countof(tmp_file));
    fatal_if(r != 0, "files.create_tmp() failed %s", str.error(r));
    char tmp[1024];
    strprintf(tmp, "%s.dir", tmp_file);
    r = files.mkdirs(tmp);
    fatal_if(r != 0, "files.mkdirs(%s) failed %s", tmp, str.error(r));
    verbose("%s", tmp);
    folders_t* fs = null;
    char pn[1024] = {0};
    strprintf(pn, "%s/file", tmp);
    // cannot test symlinks because they are only
    // available to Administrators and in Developer mode
//  char sym[1024] = {0};
    char hard[1024] = {0};
    char sub[1024] = {0};
    strprintf(hard, "%s/hard", tmp);
//  strprintf(sym, "%s/sym", tmp);
    strprintf(sub, "%s/subd", tmp);
    const char* content = "content";
    int64_t transferred = 0;
    r = files.write_fully(pn, content, strlen(content), &transferred);
    fatal_if(r != 0, "files.write_fully(\"%s\") failed %s", pn, str.error(r));
    swear(transferred == (int64_t)strlen(content));
    r = files.link(pn, hard);
    fatal_if(r != 0, "files.link(\"%s\", \"%s\") failed %s",
                      pn, hard, str.error(r));
//  r = files.symlink(pn, sym);
//  fatal_if(r != 0, "files.link(\"%s\", \"%s\") failed %s",
//                    pn, sym, str.error(r));
    r = files.mkdirs(sub);
    fatal_if(r != 0, "files.mkdirs(\"%s\") failed %s", sub, str.error(r));
    r = folders.open(&fs, tmp);
    fatal_if(r != 0, "folders.open(\"%s\") failed %s",
                        tmp, str.error(r));
    fatal_if(!str.equal(folders.folder(fs), tmp),
            "folders.folder(fs): %s tmp: %s", folders.folder(fs), tmp);
    int32_t count = folders.count(fs);
    fatal_if(count != 3, "count: %d expected 4", count);
    for (int32_t i = 0; i < count; ++i) {
        const char* name = folders.name(fs, i);
        uint64_t at = folders.accessed(fs, i);
        uint64_t ct = folders.created(fs, i);
        uint64_t ut = folders.updated(fs, i);
        swear(ct <= at && ct <= ut);
        clock.local(ct, &year, &month, &day, &hh, &mm, &ss, &ms, &mc);
        bool is_folder = folders.is_folder(fs, i);
        bool is_symlink = folders.is_symlink(fs, i);
        int64_t bytes = folders.bytes(fs, i);
        verbose("%s: %04d-%02d-%02d %02d:%02d:%02d.%3d:%3d %lld bytes %s%s",
                name, year, month, day, hh, mm, ss, ms, mc,
                bytes, is_folder ? "[folder]" : "", is_symlink ? "[symlink]" : "");
        if (str.equal(name, "file") || str.equal(name, "hard")) {
            swear(bytes == (int64_t)strlen(content),
                    "size of \"%s\": %lld is incorrect expected: %d",
                    name, bytes, transferred);
        }
        swear(str.equal(name, "subd") == is_folder,
              "\"%s\" is_folder: %d", name, is_folder);
        // empirically timestamps are imprecise on NTFS
        swear(at >= before, "access: %lld  >= %lld", at, before);
        swear(ct >= before, "create: %lld  >= %lld", ct, before);
        swear(ut >= before, "update: %lld  >= %lld", ut, before);
        // and no later than 2 seconds since folders_test()
        swear(at < after, "access: %lld  < %lld", at, after);
        swear(ct < after, "create: %lld  < %lld", ct, after);
        swear(at < after, "update: %lld  < %lld", ut, after);
    }
    folders.close(fs);
    r = files.rmdirs(tmp);
    fatal_if(r != 0, "files.rmdirs(\"%s\") failed %s",
                     tmp, str.error(r));
    r = files.unlink(tmp_file);
    fatal_if(r != 0, "files.unlink(\"%s\") failed %s",
                     tmp_file, str.error(r));
    if (debug.verbosity.level > debug.verbosity.quiet) { traceln("done"); }
}

#pragma pop_macro("verbose")

#else

static void folders_test(void) { }

#endif

folders_if folders = {
    .bin         = folders_bin,
    .tmp         = folders_tmp,
    .data        = folders_data,
    .open        = folders_open,
    .folder      = folders_folder,
    .count       = folders_count,
    .name        = folders_filename,
    .is_folder   = folders_is_folder,
    .is_symlink  = folders_is_symlink,
    .bytes       = folders_bytes,
    .created     = folders_time_created,
    .updated     = folders_time_updated,
    .accessed    = folders_time_accessed,
    .close       = folders_close,
    .test        = folders_test
};

end_c

