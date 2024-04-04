#include "runtime/runtime.h"
#include "runtime/win32.h"

// abort does NOT call atexit() functions and
// does NOT flush streams. Also Win32 runtime
// abort() attempt to show Abort/Retry/Ignore
// MessageBox - thus ExitProcess()

static void runtime_abort(void) { ExitProcess(ERROR_FATAL_APP_EXIT); }

static void runtime_exit(int32_t exit_code) { exit(exit_code); }

static int32_t runtime_err(void) { return GetLastError(); }

static void runtime_seterr(int32_t err) { SetLastError(err); }

static_init(runtime) {
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

static void rt_test(void) {
    #ifdef RUNTIME_TESTS // in alphabetical order
    args.test();
    atomics.test();
    clock.test();
    config.test();
    debug.test();
    events.test();
    files.test();
    folders.test();
    heap.test();
    loader.test();
    mem.test();
    mutexes.test();
    num.test();
    processes.test();
    static_init_test();
    str.test();
    streams.test();
    threads.test();
    vigil.test();
    #endif
}

runtime_if runtime = {
    .err    = runtime_err,
    .seterr = runtime_seterr,
    .abort  = runtime_abort,
    .exit   = runtime_exit,
    .test   = rt_test
};

#pragma comment(lib, "advapi32")
#pragma comment(lib, "cabinet")
#pragma comment(lib, "cfgmgr32")
#pragma comment(lib, "comctl32")
#pragma comment(lib, "comdlg32")
#pragma comment(lib, "dbghelp")
#pragma comment(lib, "dwmapi")
#pragma comment(lib, "dxgi")
#pragma comment(lib, "dxva2")
#pragma comment(lib, "gdi32")
#pragma comment(lib, "glu32")
#pragma comment(lib, "imagehlp")
#pragma comment(lib, "imm32")
#pragma comment(lib, "msimg32")
#pragma comment(lib, "ntdll")
#pragma comment(lib, "ole32")
#pragma comment(lib, "OneCoreUAP")
#pragma comment(lib, "opengl32")
#pragma comment(lib, "powrprof")
#pragma comment(lib, "psapi")
#pragma comment(lib, "rpcrt4")
#pragma comment(lib, "setupapi")
#pragma comment(lib, "shcore")
#pragma comment(lib, "shell32")
#pragma comment(lib, "shlwapi")
#pragma comment(lib, "user32")
#pragma comment(lib, "winusb")
