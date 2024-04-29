#include "ut/ut.h"
#include "ut/ut_win32.h"

// On Unix the implementation should keep KV pairs in
// key-named files inside .name/ folder

static const char* ut_config_app = "Software\\apps";

const DWORD ut_config_access = KEY_READ|KEY_WRITE|KEY_SET_VALUE|KEY_QUERY_VALUE|
                            KEY_ENUMERATE_SUB_KEYS|DELETE;

static errno_t ut_config_get_reg_key(const char* name, HKEY *key) {
    errno_t r = 0;
    char path[256];
    strprintf(path, "%s\\%s", ut_config_app, name);
    if (RegOpenKeyExA(HKEY_CURRENT_USER, path, 0, ut_config_access, key) != 0) {
        const DWORD option = REG_OPTION_NON_VOLATILE;
        r = RegCreateKeyExA(HKEY_CURRENT_USER, path, 0, null, option,
                            ut_config_access, null, key, null);
    }
    return r;
}

static errno_t ut_config_save(const char* name,
        const char* key, const void* data, int32_t bytes) {
    errno_t r = 0;
    HKEY k = null;
    r = ut_config_get_reg_key(name, &k);
    if (k != null) {
        r = RegSetValueExA(k, key, 0, REG_BINARY,
            (byte*)data, bytes);
        fatal_if_not_zero(RegCloseKey(k));
    }
    return r;
}

static errno_t ut_config_remove(const char* name, const char* key) {
    errno_t r = 0;
    HKEY k = null;
    r = ut_config_get_reg_key(name, &k);
    if (k != null) {
        r = RegDeleteValueA(k, key);
        fatal_if_not_zero(RegCloseKey(k));
    }
    return r;
}

static errno_t ut_config_clean(const char* name) {
    errno_t r = 0;
    HKEY k = null;
    if (RegOpenKeyExA(HKEY_CURRENT_USER, ut_config_app,
                                      0, ut_config_access, &k) == 0) {
       r = RegDeleteTreeA(k, name);
       fatal_if_not_zero(RegCloseKey(k));
    }
    return r;
}

static int32_t ut_config_size(const char* name, const char* key) {
    int32_t bytes = -1;
    HKEY k = null;
    errno_t r = ut_config_get_reg_key(name, &k);
    if (k != null) {
        DWORD type = REG_BINARY;
        DWORD cb = 0;
        r = RegQueryValueExA(k, key, null, &type, null, &cb);
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

static int32_t ut_config_load(const char* name,
        const char* key, void* data, int32_t bytes) {
    int32_t read = -1;
    HKEY k = null;
    errno_t r = ut_config_get_reg_key(name, &k);
    if (k != null) {
        DWORD type = REG_BINARY;
        DWORD cb = (DWORD)bytes;
        r = RegQueryValueExA(k, key, null, &type, (byte*)data, &cb);
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

#ifdef UT_TESTS

static void ut_config_test(void) {
    const char* name = strrchr(ut_args.v[0], '\\');
    if (name == null) { name = strrchr(ut_args.v[0], '/'); }
    name = name != null ? name + 1 : ut_args.v[0];
    swear(name != null);
    const char* key = "test";
    const char data[] = "data";
    int32_t bytes = sizeof(data);
    swear(ut_config.save(name, key, data, bytes) == 0);
    char read[256];
    swear(ut_config.load(name, key, read, bytes) == bytes);
    int32_t size = ut_config.size(name, key);
    swear(size == bytes);
    swear(ut_config.remove(name, key) == 0);
    swear(ut_config.clean(name) == 0);
    if (ut_debug.verbosity.level > ut_debug.verbosity.quiet) { traceln("done"); }
}

#else

static void ut_config_test(void) { }

#endif

ut_config_if ut_config = {
    .save   = ut_config_save,
    .size   = ut_config_size,
    .load   = ut_config_load,
    .remove = ut_config_remove,
    .clean  = ut_config_clean,
    .test   = ut_config_test
};
