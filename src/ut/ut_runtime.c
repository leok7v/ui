#include "ut/ut.h"
#include "ut/ut_win32.h"

// abort does NOT call atexit() functions and
// does NOT flush ut_streams. Also Win32 runtime
// abort() attempt to show Abort/Retry/Ignore
// MessageBox - thus ExitProcess()

static void ut_runtime_abort(void) { ExitProcess(ERROR_FATAL_APP_EXIT); }

static void ut_runtime_exit(int32_t exit_code) { exit(exit_code); }

// TODO: consider r = HRESULT_FROM_WIN32() and r = HRESULT_CODE(hr);
// this separates posix error codes from win32 error codes


static errno_t ut_runtime_err(void) { return (errno_t)GetLastError(); }

static void ut_runtime_seterr(errno_t err) { SetLastError((DWORD)err); }

ut_static_init(runtime) {
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

#ifdef UT_TESTS

static void ut_runtime_test(void) { // in alphabetical order
    ut_args.test();
    ut_atomics.test();
    ut_bt.test();
    ut_clipboard.test();
    ut_clock.test();
    ut_config.test();
    ut_debug.test();
    ut_event.test();
    ut_files.test();
    ut_generics.test();
    ut_heap.test();
    ut_loader.test();
    ut_mem.test();
    ut_mutex.test();
    ut_num.test();
    ut_processes.test();
    ut_static_init_test();
    ut_str.test();
    ut_streams.test();
    ut_thread.test();
    vigil.test();
}

#else

static void ut_runtime_test(void) { }

#endif

ut_runtime_if ut_runtime = {
    .err     = ut_runtime_err,
    .set_err = ut_runtime_seterr,
    .abort   = ut_runtime_abort,
    .exit    = ut_runtime_exit,
    .test    = ut_runtime_test,
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

/*


.error = { //      win32             posix
    .out_of_memory    = ERROR_OUTOFMEMORY,     // ENOMEM
    .file_not_found   = ERROR_FILE_NOT_FOUND,   // ENOENT
    .insufficient_buffer = ERROR_INSUFFICIENT_BUFFER, // E2BIG
    .more_data      = ERROR_MORE_DATA,      // ENOBUFS
    .invalid_data    = ERROR_INVALID_DATA,    // EINVAL
    .invalid_parameter  = ERROR_INVALID_PARAMETER,  // EINVAL
    .access_denied    = ERROR_ACCESS_DENIED,    // EACCES
    .invalid_handle    = ERROR_INVALID_HANDLE,   // EBADF
    .no_child_process   = ERROR_NO_PROC_SLOTS,   // ECHILD
    .resource_deadlock  = ERROR_LOCK_VIOLATION,  // EDEADLK
    .disk_full      = ERROR_DISK_FULL,      // ENOSPC
    .broken_pipe     = ERROR_BROKEN_PIPE,    // EPIPE
    .name_too_long    = ERROR_FILENAME_EXCED_RANGE, // ENAMETOOLONG
    .not_empty      = ERROR_DIR_NOT_EMPTY,  // ENOTEMPTY
    .io_error       = ERROR_IO_DEVICE,     // EIO
    .interrupted     = ERROR_OPERATION_ABORTED, // EINTR
    .bad_file       = ERROR_BAD_FILE_TYPE,    // EBADF
    .device_not_ready  = ERROR_NOT_READY,      // ENXIO
    .directory_not_empty = ERROR_DIR_NOT_EMPTY,    // ENOTEMPTY
    .disk_full      = ERROR_DISK_FULL,      // ENOSPC
    .file_exists     = ERROR_FILE_EXISTS,     // EEXIST
    .not_a_directory   = ERROR_DIRECTORY,      // ENOTDIR
    .path_not_found   = ERROR_PATH_NOT_FOUND,   // ENOENT
    .pipe_not_connected = ERROR_PIPE_NOT_CONNECTED, // EPIPE
    .read_only_file   = ERROR_WRITE_PROTECT,    // EROFS
    .too_many_open_files = ERROR_TOO_MANY_OPEN_FILES, // EMFILE
  }




EPERM
ENOENT
ESRCH
EINTR
EIO
ENXIO
E2BIG
ENOEXEC
EBADF
ECHILD
EAGAIN
ENOMEM
EACCES
EFAULT
EBUSY
EEXIST
EXDEV
ENODEV
ENOTDIR
EISDIR
ENFILE
EMFILE
ENOTTY
EFBIG
ENOSPC
ESPIPE
EROFS
EMLINK
EPIPE
EDOM
EDEADLK
ENAMETOOLONG
ENOLCK
ENOSYS
ENOTEMPTY
EINVAL
ERANGE
EILSEQ
STRUNCATE
EADDRINUSE
EADDRNOTAVAIL
EAFNOSUPPORT
EALREADY
EBADMSG
ECANCELED
ECONNABORTED
ECONNREFUSED
ECONNRESET
EDESTADDRREQ
EHOSTUNREACH
EIDRM
EINPROGRESS
EISCONN
ELOOP
EMSGSIZE
ENETDOWN
ENETRESET
ENETUNREACH
ENOBUFS
ENODATA
ENOLINK
ENOMSG
ENOPROTOOPT
ENOSR
ENOSTR
ENOTCONN
ENOTRECOVERABLE
ENOTSOCK
ENOTSUP
EOPNOTSUPP
EOTHER
EOVERFLOW
EOWNERDEAD
EPROTO
EPROTONOSUPPORT
EPROTOTYPE
ETIME
ETIMEDOUT
ETXTBSY
EWOULDBLOCK
EXDEV
EDEADLK
ENAMETOOLONG
ENOLCK
ENOSYS
ENOTEMPTY
ELOOP
EWOULDBLOCK
EAGAIN
ENOMSG
EIDRM
ECHRNG
EL2NSYNC
EL3HLT
EL3RST
ELNRNG
EUNATCH
ENOCSI
EL2HLT
EBADE
EBADR
EXFULL
ENOANO
EBADRQC
EBADSLT
EDEADLOCK
EDEADLK
EBFONT
ENOSTR
ENODATA
ETIME
ENOSR
ENONET
ENOPKG
EREMOTE
ENOLINK
EADV
ESRMNT
ECOMM
EPROTO
EMULTIHOP
EDOTDOT
EBADMSG
EOVERFLOW
ENOTUNIQ
EBADFD
EREMCHG
ELIBACC
ELIBBAD
ELIBSCN
ELIBMAX
ELIBEXEC
EILSEQ
ERESTART
ESTRPIPE
EUSERS
ENOTSOCK
EDESTADDRREQ
EMSGSIZE
EPROTOTYPE
ENOPROTOOPT
EPROTONOSUPPORT
ESOCKTNOSUPPORT
EOPNOTSUPP
EPFNOSUPPORT
EAFNOSUPPORT
EADDRINUSE
EADDRNOTAVAIL
ENETDOWN
ENETUNREACH
ENETRESET
ECONNABORTED
ECONNRESET
ENOBUFS
EISCONN
ENOTCONN
ESHUTDOWN
ETOOMANYREFS
ETIMEDOUT
ECONNREFUSED
EHOSTDOWN
EHOSTUNREACH
EALREADY
EINPROGRESS
ESTALE
EUCLEAN
ENOTNAM
ENAVAIL
EISNAM
EREMOTEIO
EDQUOT
ENOMEDIUM
EMEDIUMTYPE
ECANCELED
ENOKEY
EKEYEXPIRED
EKEYREVOKED
EKEYREJECTED
EOWNERDEAD
ENOTRECOVERABLE
ERFKILL
EHWPOISON


*/


#pragma comment(lib, "advapi32")
#pragma comment(lib, "ntdll")
#pragma comment(lib, "psapi")
#pragma comment(lib, "shell32")
#pragma comment(lib, "shlwapi")
#pragma comment(lib, "kernel32")
#pragma comment(lib, "user32") // clipboard
#pragma comment(lib, "ole32")  // ut_files.known_folder CoMemFree
#pragma comment(lib, "dbghelp")
#pragma comment(lib, "imagehlp")


