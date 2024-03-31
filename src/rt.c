#include "rt.h"

#include <immintrin.h>
#include <Windows.h>
// #include <timeapi.h>
// #include <sysinfoapi.h>

begin_c

// abort does NOT call atexit() functions and
// does NOT flush streams. Also Win32 runtime
// abort() attempt to show Abort/Retry/Ignore
// MessageBox - thus ExitProcess()

static void crt_abort(void) { ExitProcess(ERROR_FATAL_APP_EXIT); }

static void crt_exit(int32_t exit_code) { exit(exit_code); }

static HKEY crt_get_reg_key(const char* name) {
    char path[MAX_PATH];
    strprintf(path, "Software\\app\\%s", name);
    HKEY key = null;
    if (RegOpenKeyA(HKEY_CURRENT_USER, path, &key) != 0) {
        RegCreateKeyA(HKEY_CURRENT_USER, path, &key);
    }
    not_null(key);
    return key;
}

static void crt_data_save(const char* name,
        const char* key, const void* data, int bytes) {
    HKEY k = crt_get_reg_key(name);
    if (k != null) {
        fatal_if_not_zero(RegSetValueExA(k, key, 0, REG_BINARY,
            (byte*)data, bytes));
        fatal_if_not_zero(RegCloseKey(k));
    }
}

static int crt_data_size(const char* name, const char* key) {
    int bytes = -1;
    HKEY k = crt_get_reg_key(name);
    if (k != null) {
        DWORD type = REG_BINARY;
        DWORD cb = 0;
        int r = RegQueryValueExA(k, key, null, &type, null, &cb);
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

static int crt_data_load(const char* name,
        const char* key, void* data, int bytes) {
    int read = -1;
    HKEY k= crt_get_reg_key(name);
    if (k != null) {
        DWORD type = REG_BINARY;
        DWORD cb = (DWORD)bytes;
        int r = RegQueryValueExA(k, key, null, &type, (byte*)data, &cb);
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

static int32_t crt_err(void) { return GetLastError(); }

static void crt_seterr(int32_t err) { SetLastError(err); }

static void rt_test(int32_t verbosity) {
    static_init_test(verbosity);
    vigil.test(verbosity);
    str.test(verbosity);
    num.test(verbosity);
    dl.test(verbosity);
    atomics.test(verbosity);
    clock.test(verbosity);
    mem.test(verbosity);
    events.test(verbosity);
    mutexes.test(verbosity);
    threads.test(verbosity);
}

crt_if crt = {
    .err = crt_err,
    .seterr = crt_seterr,
    .abort = crt_abort,
    .exit = crt_exit,
    .data_save = crt_data_save,
    .data_size = crt_data_size,
    .data_load = crt_data_load,
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

end_c

