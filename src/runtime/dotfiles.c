#include "runtime/runtime.h"
#include "runtime/win32.h"

static HKEY dotfiles_get_reg_key(const char* name) {
    char path[MAX_PATH];
    strprintf(path, "Software\\app\\%s", name);
    HKEY key = null;
    if (RegOpenKeyA(HKEY_CURRENT_USER, path, &key) != 0) {
        RegCreateKeyA(HKEY_CURRENT_USER, path, &key);
    }
    not_null(key);
    return key;
}

static void dotfiles_save(const char* name,
        const char* key, const void* data, int32_t bytes) {
    HKEY k = dotfiles_get_reg_key(name);
    if (k != null) {
        fatal_if_not_zero(RegSetValueExA(k, key, 0, REG_BINARY,
            (byte*)data, bytes));
        fatal_if_not_zero(RegCloseKey(k));
    }
}

static int32_t dotfiles_size(const char* name, const char* key) {
    int32_t bytes = -1;
    HKEY k = dotfiles_get_reg_key(name);
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

static int32_t dotfiles_load(const char* name,
        const char* key, void* data, int32_t bytes) {
    int32_t read = -1;
    HKEY k = dotfiles_get_reg_key(name);
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

static void dotfiles_test(void) {
    #ifdef RUNTIME_TESTS
    traceln("TODO");
    if (debug.verbosity.level > debug.verbosity.quiet) { traceln("done"); }
    #endif
}

dotfiles_if dotfiles = {
    .save = dotfiles_save,
    .size = dotfiles_size,
    .load = dotfiles_load,
    .test = dotfiles_test
};

