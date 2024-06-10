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
    .err    = ut_runtime_err,
    .seterr = ut_runtime_seterr,
    .abort  = ut_runtime_abort,
    .exit   = ut_runtime_exit,
    .test   = ut_runtime_test
};

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


