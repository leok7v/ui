#include "runtime/runtime.h"
#include "runtime/win32.h"

// abort does NOT call atexit() functions and
// does NOT flush streams. Also Win32 runtime
// abort() attempt to show Abort/Retry/Ignore
// MessageBox - thus ExitProcess()

static void runtime_abort(void) { ExitProcess(ERROR_FATAL_APP_EXIT); }

static void runtime_exit(int32_t exit_code) { exit(exit_code); }

static HKEY runtime_get_reg_key(const char* name) {
    char path[MAX_PATH];
    strprintf(path, "Software\\app\\%s", name);
    HKEY key = null;
    if (RegOpenKeyA(HKEY_CURRENT_USER, path, &key) != 0) {
        RegCreateKeyA(HKEY_CURRENT_USER, path, &key);
    }
    not_null(key);
    return key;
}

static void runtime_data_save(const char* name,
        const char* key, const void* data, int32_t bytes) {
    HKEY k = runtime_get_reg_key(name);
    if (k != null) {
        fatal_if_not_zero(RegSetValueExA(k, key, 0, REG_BINARY,
            (byte*)data, bytes));
        fatal_if_not_zero(RegCloseKey(k));
    }
}

static int32_t runtime_data_size(const char* name, const char* key) {
    int32_t bytes = -1;
    HKEY k = runtime_get_reg_key(name);
    if (k != null) {
        DWORD type = REG_BINARY;
        DWORD cb = 0;
        errno_t r = RegQueryValueExA(k, key, null, &type, null, &cb);
        if (r == ERROR_FILE_NOT_FOUND) {
            bytes = 0; // do not report data_size() often used this way
        } else if (r != 0) {
            traceln("%s.RegQueryValueExA(\"%s\") failed %s",
                name, key, str.error(r));
            bytes = 0; // on any error behave as empty data
        } else {
            bytes = (int)cb;
        }
        fatal_if_not_zero(RegCloseKey(k));
    }
    return bytes;
}

static int32_t runtime_data_load(const char* name,
        const char* key, void* data, int32_t bytes) {
    int32_t read = -1;
    HKEY k = runtime_get_reg_key(name);
    if (k != null) {
        DWORD type = REG_BINARY;
        DWORD cb = (DWORD)bytes;
        errno_t r = RegQueryValueExA(k, key, null, &type, (byte*)data, &cb);
        if (r == ERROR_MORE_DATA) {
            // returns -1 app.data_size() should be used
        } else if (r != 0) {
            if (r != ERROR_FILE_NOT_FOUND) {
                traceln("%s.RegQueryValueExA(\"%s\") failed %s",
                    name, key, str.error(r));
            }
            read = 0; // on any error behave as empty data
        } else {
            read = (int)cb;
        }
        fatal_if_not_zero(RegCloseKey(k));
    }
    return read;
}

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
    .err = runtime_err,
    .seterr = runtime_seterr,
    .abort = runtime_abort,
    .exit = runtime_exit,
    .data_save = runtime_data_save,
    .data_size = runtime_data_size,
    .data_load = runtime_data_load,
    .test = rt_test
};

#pragma comment(lib, "advapi32")
#pragma comment(lib, "cabinet")
#pragma comment(lib, "comctl32")
#pragma comment(lib, "comdlg32")
#pragma comment(lib, "cfgmgr32")
#pragma comment(lib, "dxgi")
#pragma comment(lib, "dxva2")
#pragma comment(lib, "dwmapi")
#pragma comment(lib, "gdi32")
#pragma comment(lib, "glu32")
#pragma comment(lib, "imm32")
#pragma comment(lib, "msimg32")
#pragma comment(lib, "ntdll")
#pragma comment(lib, "ole32")
#pragma comment(lib, "OneCoreUAP")
#pragma comment(lib, "powrprof")
#pragma comment(lib, "psapi")
#pragma comment(lib, "rpcrt4")
#pragma comment(lib, "setupapi")
#pragma comment(lib, "shcore")
#pragma comment(lib, "shell32")
#pragma comment(lib, "shlwapi")
#pragma comment(lib, "winmm")
#pragma comment(lib, "winusb")
#pragma comment(lib, "user32")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "dbghelp")
#pragma comment(lib, "imagehlp")
#pragma comment(lib, "opengl32")
#pragma comment(lib, "winmm")


