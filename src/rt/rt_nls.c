#include "rt/rt.h"
#include "rt/rt_win32.h"

// Simplistic Win32 implementation of national language support.
// Windows NLS family of functions is very complicated and has
// difficult history of LANGID vs LCID etc... See:
// ResolveLocaleName()
// GetThreadLocale()
// SetThreadLocale()
// GetUserDefaultLocaleName()
// WM_SETTINGCHANGE lParam="intl"
// and many others...

enum {
    rt_nls_str_count_max = 1024,
    rt_nls_str_mem_max = 64 * rt_nls_str_count_max
};

static char  rt_nls_strings_memory[rt_nls_str_mem_max]; // increase if overflows
static char* rt_nls_strings_free = rt_nls_strings_memory;

static int32_t rt_nls_strings_count;

static const char* rt_nls_ls[rt_nls_str_count_max]; // localized strings
static const char* rt_nls_ns[rt_nls_str_count_max]; // neutral language strings

static uint16_t* rt_nls_load_string(int32_t strid, LANGID lang_id) {
    rt_assert(0 <= strid && strid < rt_countof(rt_nls_ns));
    uint16_t* r = null;
    int32_t block = strid / 16 + 1;
    int32_t index  = strid % 16;
    HRSRC res = FindResourceExW(((HMODULE)null), RT_STRING,
        MAKEINTRESOURCEW(block), lang_id);
//  rt_println("FindResourceExA(block=%d lang_id=%04X)=%p", block, lang_id, res);
    uint8_t* memory = res == null ? null : (uint8_t*)LoadResource(null, res);
    uint16_t* ws = memory == null ? null : (uint16_t*)LockResource(memory);
//  rt_println("LockResource(block=%d lang_id=%04X)=%p", block, lang_id, ws);
    if (ws != null) {
        for (int32_t i = 0; i < 16 && r == null; i++) {
            if (ws[0] != 0) {
                int32_t count = (int32_t)ws[0];  // String size in characters.
                ws++;
                rt_assert(ws[count - 1] == 0, "use rc.exe /n command line option");
                if (i == index) { // the string has been found
//                  rt_println("%04X found %s", lang_id, utf16to8(ws));
                    r = ws;
                }
                ws += count;
            } else {
                ws++;
            }
        }
    }
    return r;
}

static const char* rt_nls_save_string(uint16_t* utf16) {
    const int32_t bytes = rt_str.utf8_bytes(utf16, -1);
    rt_swear(bytes > 1);
    char* s = rt_nls_strings_free;
    uintptr_t left = (uintptr_t)rt_countof(rt_nls_strings_memory) -
        (uintptr_t)(rt_nls_strings_free - rt_nls_strings_memory);
    rt_fatal_if(left < (uintptr_t)bytes, "string_memory[] overflow");
    rt_str.utf16to8(s, (int32_t)left, utf16, -1);
    rt_assert((int32_t)strlen(s) == bytes - 1, "utf16to8() does not truncate");
    rt_nls_strings_free += bytes;
    return s;
}

static const char* rt_nls_localized_string(int32_t strid) {
    rt_swear(0 < strid && strid < rt_countof(rt_nls_ns));
    const char* s = null;
    if (0 < strid && strid < rt_countof(rt_nls_ns)) {
        if (rt_nls_ls[strid] != null) {
            s = rt_nls_ls[strid];
        } else {
            LCID lc_id = GetThreadLocale();
            LANGID lang_id = LANGIDFROMLCID(lc_id);
            uint16_t* utf16 = rt_nls_load_string(strid, lang_id);
            if (utf16 == null) { // try default dialect:
                LANGID primary = PRIMARYLANGID(lang_id);
                lang_id = MAKELANGID(primary, SUBLANG_NEUTRAL);
                utf16 = rt_nls_load_string(strid, lang_id);
            }
            if (utf16 != null && utf16[0] != 0x0000) {
                s = rt_nls_save_string(utf16);
                rt_nls_ls[strid] = s;
            }
        }
    }
    return s;
}

static int32_t rt_nls_strid(const char* s) {
    int32_t strid = -1;
    for (int32_t i = 1; i < rt_nls_strings_count && strid == -1; i++) {
        if (rt_nls_ns[i] != null && strcmp(s, rt_nls_ns[i]) == 0) {
            strid = i;
            rt_nls_localized_string(strid); // to save it, ignore result
        }
    }
    return strid;
}

static const char* rt_nls_string(int32_t strid, const char* defau1t) {
    const char* r = rt_nls_localized_string(strid);
    return r == null ? defau1t : r;
}

static const char* rt_nls_str(const char* s) {
    int32_t id = rt_nls_strid(s);
    return id < 0 ? s : rt_nls_string(id, s);
}

static const char* rt_nls_locale(void) {
    uint16_t utf16[LOCALE_NAME_MAX_LENGTH + 1];
    LCID lc_id = GetThreadLocale();
    int32_t n = LCIDToLocaleName(lc_id, utf16, rt_countof(utf16),
        LOCALE_ALLOW_NEUTRAL_NAMES);
    static char ln[LOCALE_NAME_MAX_LENGTH * 4 + 1];
    ln[0] = 0;
    if (n == 0) {
        errno_t r = rt_core.err();
        rt_println("LCIDToLocaleName(0x%04X) failed %s", lc_id, rt_str.error(r));
    } else {
        rt_str.utf16to8(ln, rt_countof(ln), utf16, -1);
    }
    return ln;
}

static errno_t rt_nls_set_locale(const char* locale) {
    errno_t r = 0;
    uint16_t utf16[LOCALE_NAME_MAX_LENGTH + 1];
    rt_str.utf8to16(utf16, rt_countof(utf16), locale, -1);
    uint16_t rln[LOCALE_NAME_MAX_LENGTH + 1]; // resolved locale name
    int32_t n = (int32_t)ResolveLocaleName(utf16, rln, (DWORD)rt_countof(rln));
    if (n == 0) {
        r = rt_core.err();
        rt_println("ResolveLocaleName(\"%s\") failed %s", locale, rt_str.error(r));
    } else {
        LCID lc_id = LocaleNameToLCID(rln, LOCALE_ALLOW_NEUTRAL_NAMES);
        if (lc_id == 0) {
            r = rt_core.err();
            rt_println("LocaleNameToLCID(\"%s\") failed %s", locale, rt_str.error(r));
        } else {
            rt_fatal_win32err(SetThreadLocale(lc_id));
            memset((void*)rt_nls_ls, 0, sizeof(rt_nls_ls)); // start all over
        }
    }
    return r;
}

static void rt_nls_init(void) {
    static_assert(rt_countof(rt_nls_ns) % 16 == 0, 
                 "rt_countof(ns) must be multiple of 16");
    LANGID lang_id = MAKELANGID(LANG_ENGLISH, SUBLANG_NEUTRAL);
    for (int32_t strid = 0; strid < rt_countof(rt_nls_ns); strid += 16) {
        int32_t block = strid / 16 + 1;
        HRSRC res = FindResourceExW(((HMODULE)null), RT_STRING,
            MAKEINTRESOURCEW(block), lang_id);
        uint8_t* memory = res == null ? null : (uint8_t*)LoadResource(null, res);
        uint16_t* ws = memory == null ? null : (uint16_t*)LockResource(memory);
        if (ws == null) { break; }
        for (int32_t i = 0; i < 16; i++) {
            int32_t ix = strid + i;
            uint16_t count = ws[0];
            if (count > 0) {
                ws++;
                rt_fatal_if(ws[count - 1] != 0, "use rc.exe /n");
                rt_nls_ns[ix] = rt_nls_save_string(ws);
                rt_nls_strings_count = ix + 1;
//              rt_println("ns[%d] := %d \"%s\"", ix, strlen(rt_nls_ns[ix]), rt_nls_ns[ix]);
                ws += count;
            } else {
                ws++;
            }
        }
    }
}

rt_nls_if rt_nls = {
    .init       = rt_nls_init,
    .strid      = rt_nls_strid,
    .str        = rt_nls_str,
    .string     = rt_nls_string,
    .locale     = rt_nls_locale,
    .set_locale = rt_nls_set_locale,
};
