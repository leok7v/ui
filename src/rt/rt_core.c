#include "rt/rt.h"
#include "rt/rt_win32.h"

// abort does NOT call atexit() functions and
// does NOT flush rt_streams. Also Win32 runtime
// abort() attempt to show Abort/Retry/Ignore
// MessageBox - thus ExitProcess()

static void rt_core_abort(void) { ExitProcess(ERROR_FATAL_APP_EXIT); }

static void rt_core_exit(int32_t exit_code) { exit(exit_code); }

// TODO: consider r = HRESULT_FROM_WIN32() and r = HRESULT_CODE(hr);
// this separates posix error codes from win32 error codes


static errno_t rt_core_err(void) { return (errno_t)GetLastError(); }

static void rt_core_seterr(errno_t err) { SetLastError((DWORD)err); }

rt_static_init(runtime) {
    SetErrorMode(
        // The system does not display the critical-error-handler message box.
        // Instead, the system sends the error to the calling process:
        SEM_FAILCRITICALERRORS|
        // The system automatically fixes memory alignment faults and
        // makes them invisible to the application.
        SEM_NOALIGNMENTFAULTEXCEPT|
        // The system does not display the Windows Error Reporting dialog.
        SEM_NOGPFAULTERRORBOX|
        // The OpenFile function does not display a message box when it fails
        // to find a file. Instead, the error is returned to the caller.
        // This error mode overrides the OF_PROMPT flag.
        SEM_NOOPENFILEERRORBOX);
}

#ifdef RT_TESTS

static void rt_core_test(void) { // in alphabetical order
    rt_args.test();
    rt_atomics.test();
    rt_backtrace.test();
    rt_clipboard.test();
    rt_clock.test();
    rt_config.test();
    rt_debug.test();
    rt_event.test();
    rt_files.test();
    rt_generics.test();
    rt_heap.test();
    rt_loader.test();
    rt_mem.test();
    rt_mutex.test();
    rt_num.test();
    rt_processes.test();
    rt_static_init_test();
    rt_str.test();
    rt_streams.test();
    rt_thread.test();
    rt_vigil.test();
    rt_worker.test();
}

#else

static void rt_core_test(void) { }

#endif

rt_core_if rt_core = {
    .err     = rt_core_err,
    .set_err = rt_core_seterr,
    .abort   = rt_core_abort,
    .exit    = rt_core_exit,
    .test    = rt_core_test,
    .error   = {                                              // posix
        .access_denied          = ERROR_ACCESS_DENIED,        // EACCES
        .bad_file               = ERROR_BAD_FILE_TYPE,        // EBADF
        .broken_pipe            = ERROR_BROKEN_PIPE,          // EPIPE
        .device_not_ready       = ERROR_NOT_READY,            // ENXIO
        .directory_not_empty    = ERROR_DIR_NOT_EMPTY,        // ENOTEMPTY
        .disk_full              = ERROR_DISK_FULL,            // ENOSPC
        .file_exists            = ERROR_FILE_EXISTS,          // EEXIST
        .file_not_found         = ERROR_FILE_NOT_FOUND,       // ENOENT
        .insufficient_buffer    = ERROR_INSUFFICIENT_BUFFER,  // E2BIG
        .interrupted            = ERROR_OPERATION_ABORTED,    // EINTR
        .invalid_data           = ERROR_INVALID_DATA,         // EINVAL
        .invalid_handle         = ERROR_INVALID_HANDLE,       // EBADF
        .invalid_parameter      = ERROR_INVALID_PARAMETER,    // EINVAL
        .io_error               = ERROR_IO_DEVICE,            // EIO
        .more_data              = ERROR_MORE_DATA,            // ENOBUFS
        .name_too_long          = ERROR_FILENAME_EXCED_RANGE, // ENAMETOOLONG
        .no_child_process       = ERROR_NO_PROC_SLOTS,        // ECHILD
        .not_a_directory        = ERROR_DIRECTORY,            // ENOTDIR
        .not_empty              = ERROR_DIR_NOT_EMPTY,        // ENOTEMPTY
        .out_of_memory          = ERROR_OUTOFMEMORY,          // ENOMEM
        .path_not_found         = ERROR_PATH_NOT_FOUND,       // ENOENT
        .pipe_not_connected     = ERROR_PIPE_NOT_CONNECTED,   // EPIPE
        .read_only_file         = ERROR_WRITE_PROTECT,        // EROFS
        .resource_deadlock      = ERROR_LOCK_VIOLATION,       // EDEADLK
        .too_many_open_files    = ERROR_TOO_MANY_OPEN_FILES,  // EMFILE
    }
};

#pragma comment(lib, "advapi32")
#pragma comment(lib, "ntdll")
#pragma comment(lib, "psapi")
#pragma comment(lib, "shell32")
#pragma comment(lib, "shlwapi")
#pragma comment(lib, "kernel32")
#pragma comment(lib, "user32") // clipboard
#pragma comment(lib, "imm32")  // Internationalization input method
#pragma comment(lib, "ole32")  // rt_files.known_folder CoMemFree
#pragma comment(lib, "dbghelp")
#pragma comment(lib, "imagehlp")


