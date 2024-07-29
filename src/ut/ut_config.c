#include "ut/ut.h"
#include "ut/ut_win32.h"

// On Unix the implementation should keep KV pairs in
// key-named files inside .name/ folder

static const char* rt_config_apps = "Software\\leok7v\\ui\\apps";

static const DWORD rt_config_access =
    KEY_READ|KEY_WRITE|KEY_SET_VALUE|KEY_QUERY_VALUE|
    KEY_ENUMERATE_SUB_KEYS|DELETE;

static errno_t rt_config_get_reg_key(const char* name, HKEY *key) {
    char path[256] = {0};
    rt_str_printf(path, "%s\\%s", rt_config_apps, name);
    errno_t r = RegOpenKeyExA(HKEY_CURRENT_USER, path, 0, rt_config_access, key);
    if (r != 0) {
        const DWORD option = REG_OPTION_NON_VOLATILE;
        r = RegCreateKeyExA(HKEY_CURRENT_USER, path, 0, null, option,
                            rt_config_access, null, key, null);
    }
    return r;
}

static errno_t rt_config_save(const char* name,
        const char* key, const void* data, int32_t bytes) {
    errno_t r = 0;
    HKEY k = null;
    r = rt_config_get_reg_key(name, &k);
    if (k != null) {
        r = RegSetValueExA(k, key, 0, REG_BINARY,
            (const uint8_t*)data, (DWORD)bytes);
        rt_fatal_if_error(RegCloseKey(k));
    }
    return r;
}

static errno_t rt_config_remove(const char* name, const char* key) {
    errno_t r = 0;
    HKEY k = null;
    r = rt_config_get_reg_key(name, &k);
    if (k != null) {
        r = RegDeleteValueA(k, key);
        rt_fatal_if_error(RegCloseKey(k));
    }
    return r;
}

static errno_t rt_config_clean(const char* name) {
    errno_t r = 0;
    HKEY k = null;
    if (RegOpenKeyExA(HKEY_CURRENT_USER, rt_config_apps,
                                      0, rt_config_access, &k) == 0) {
       r = RegDeleteTreeA(k, name);
       rt_fatal_if_error(RegCloseKey(k));
    }
    return r;
}

static int32_t rt_config_size(const char* name, const char* key) {
    int32_t bytes = -1;
    HKEY k = null;
    errno_t r = rt_config_get_reg_key(name, &k);
    if (k != null) {
        DWORD type = REG_BINARY;
        DWORD cb = 0;
        r = RegQueryValueExA(k, key, null, &type, null, &cb);
        if (r == ERROR_FILE_NOT_FOUND) {
            bytes = 0; // do not report data_size() often used this way
        } else if (r != 0) {
            rt_println("%s.RegQueryValueExA(\"%s\") failed %s",
                name, key, rt_strerr(r));
            bytes = 0; // on any error behave as empty data
        } else {
            bytes = (int32_t)cb;
        }
        rt_fatal_if_error(RegCloseKey(k));
    }
    return bytes;
}

static int32_t rt_config_load(const char* name,
        const char* key, void* data, int32_t bytes) {
    int32_t read = -1;
    HKEY k = null;
    errno_t r = rt_config_get_reg_key(name, &k);
    if (k != null) {
        DWORD type = REG_BINARY;
        DWORD cb = (DWORD)bytes;
        r = RegQueryValueExA(k, key, null, &type, (uint8_t*)data, &cb);
        if (r == ERROR_MORE_DATA) {
            // returns -1 ui_app.data_size() should be used
        } else if (r != 0) {
            if (r != ERROR_FILE_NOT_FOUND) {
                rt_println("%s.RegQueryValueExA(\"%s\") failed %s",
                    name, key, rt_strerr(r));
            }
            read = 0; // on any error behave as empty data
        } else {
            read = (int32_t)cb;
        }
        rt_fatal_if_error(RegCloseKey(k));
    }
    return read;
}

#ifdef UT_TESTS

static void rt_config_test(void) {
    const char* name = strrchr(rt_args.v[0], '\\');
    if (name == null) { name = strrchr(rt_args.v[0], '/'); }
    name = name != null ? name + 1 : rt_args.v[0];
    rt_swear(name != null);
    const char* key = "test";
    const char data[] = "data";
    int32_t bytes = sizeof(data);
    rt_swear(rt_config.save(name, key, data, bytes) == 0);
    char read[256];
    rt_swear(rt_config.load(name, key, read, bytes) == bytes);
    int32_t size = rt_config.size(name, key);
    rt_swear(size == bytes);
    rt_swear(rt_config.remove(name, key) == 0);
    rt_swear(rt_config.clean(name) == 0);
    if (rt_debug.verbosity.level > rt_debug.verbosity.quiet) { rt_println("done"); }
}

#else

static void rt_config_test(void) { }

#endif

rt_config_if rt_config = {
    .save   = rt_config_save,
    .size   = rt_config_size,
    .load   = rt_config_load,
    .remove = rt_config_remove,
    .clean  = rt_config_clean,
    .test   = rt_config_test
};
