#pragma once
#include "ut/ut_std.h"

begin_c

typedef struct {
    errno_t (*err)(void); // errno or GetLastError()
    void (*set_err)(errno_t err); // errno = err or SetLastError()
    void (*abort)(void);
    void (*exit)(int32_t exit_code); // only 8 bits on posix
    void (*test)(void);
    struct {                                // posix
        errno_t const access_denied;        // EACCES
        errno_t const bad_file;             // EBADF
        errno_t const broken_pipe;          // EPIPE
        errno_t const device_not_ready;     // ENXIO
        errno_t const directory_not_empty;  // ENOTEMPTY
        errno_t const disk_full;            // ENOSPC
        errno_t const file_exists;          // EEXIST
        errno_t const file_not_found;       // ENOENT
        errno_t const insufficient_buffer;  // E2BIG
        errno_t const interrupted;          // EINTR
        errno_t const invalid_data;         // EINVAL
        errno_t const invalid_handle;       // EBADF
        errno_t const invalid_parameter;    // EINVAL
        errno_t const io_error;             // EIO
        errno_t const more_data;            // ENOBUFS
        errno_t const name_too_long;        // ENAMETOOLONG
        errno_t const no_child_process;     // ECHILD
        errno_t const not_a_directory;      // ENOTDIR
        errno_t const not_empty;            // ENOTEMPTY
        errno_t const out_of_memory;        // ENOMEM
        errno_t const path_not_found;       // ENOENT
        errno_t const pipe_not_connected;   // EPIPE
        errno_t const read_only_file;       // EROFS
        errno_t const resource_deadlock;    // EDEADLK
        errno_t const too_many_open_files;  // EMFILE
    } const error;
} ut_runtime_if;

extern ut_runtime_if ut_runtime;

end_c
