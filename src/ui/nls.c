#include "ut/ut.h"
#include "ui/ui.h"
#include "ut/win32.h"

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
    nls_str_count_max = 1024,
    nls_str_mem_max = 64 * nls_str_count_max
};

static char nls_strings_memory[nls_str_mem_max]; // increase if overflows
static char* nls_strings_free = nls_strings_memory;
static int32_t nls_strings_count;
static const char* nls_ls[nls_str_count_max]; // localized strings
static const char* nls_ns[nls_str_count_max]; // neutral language strings

wchar_t* nls_load_string(int32_t strid, LANGID langid) {
    assert(0 <= strid && strid < countof(nls_ns));
    wchar_t* r = null;
    int32_t block = strid / 16 + 1;
    int32_t index  = strid % 16;
    HRSRC res = FindResourceExA(((HMODULE)null), RT_STRING,
        MAKEINTRESOURCE(block), langid);
//  traceln("FindResourceExA(block=%d langid=%04X)=%p", block, langid, res);
    uint8_t* memory = res == null ? null : (uint8_t*)LoadResource(null, res);
    wchar_t* ws = memory == null ? null : (wchar_t*)LockResource(memory);
//  traceln("LockResource(block=%d langid=%04X)=%p", block, langid, ws);
    if (ws != null) {
        for (int32_t i = 0; i < 16 && r == null; i++) {
            if (ws[0] != 0) {
                int32_t count = (int)ws[0];  // String size in characters.
                ws++;
                assert(ws[count - 1] == 0, "use rc.exe /n command line option");
                if (i == index) { // the string has been found
//                  traceln("%04X found %s", langid, utf16to8(ws));
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

static const char* nls_save_string(wchar_t* memory) {
    const char* utf8 = utf16to8(memory);
    uintptr_t n = strlen(utf8) + 1;
    assert(n > 1);
    uintptr_t left = countof(nls_strings_memory) - (
        nls_strings_free - nls_strings_memory);
    fatal_if_false(left >= n, "string_memory[] overflow");
    memcpy(nls_strings_free, utf8, n);
    const char* s = nls_strings_free;
    nls_strings_free += n;
    return s;
}

const char* nls_localize_string(int32_t strid) {
    assert(0 < strid && strid < countof(nls_ns));
    const char* r = null;
    if (0 < strid && strid < countof(nls_ns)) {
        if (nls_ls[strid] != null) {
            r = nls_ls[strid];
        } else {
            LCID lcid = GetThreadLocale();
            LANGID langid = LANGIDFROMLCID(lcid);
            wchar_t* ws = nls_load_string(strid, langid);
            if (ws == null) { // try default dialect:
                LANGID primary = PRIMARYLANGID(langid);
                langid = MAKELANGID(primary, SUBLANG_NEUTRAL);
                ws = nls_load_string(strid, langid);
            }
            if (ws != null) {
                r = nls_save_string(ws);
                nls_ls[strid] = r;
            }
        }
    }
    return r;
}

static int32_t nls_strid(const char* s) {
    int32_t strid = 0;
    for (int32_t i = 1; i < nls_strings_count && strid == 0; i++) {
        if (nls_ns[i] != null && strcmp(s, nls_ns[i]) == 0) {
            strid = i;
            nls_localize_string(strid); // to save it, ignore result
        }
    }
    return strid;
}

static const char* nls_string(int32_t strid, const char* defau1t) {
    const char* r = nls_localize_string(strid);
    return r == null ? defau1t : r;
}

const char* nls_str(const char* s) {
    int32_t id = nls_strid(s);
    return id == 0 ? s : nls_string(id, s);
}

static const char* nls_locale(void) {
    wchar_t wln[LOCALE_NAME_MAX_LENGTH + 1];
    LCID lcid = GetThreadLocale();
    int32_t n = LCIDToLocaleName(lcid, wln, countof(wln),
        LOCALE_ALLOW_NEUTRAL_NAMES);
    static char ln[LOCALE_NAME_MAX_LENGTH * 4 + 1];
    ln[0] = 0;
    if (n == 0) {
        // TODO: log error
    } else {
        if (n == 0) {
        } else {
            strprintf(ln, "%s", utf16to8(wln));
        }
    }
    return ln;
}

static void nls_set_locale(const char* locale) {
    wchar_t rln[LOCALE_NAME_MAX_LENGTH + 1];
    int32_t n = ResolveLocaleName(utf8to16(locale), rln, countof(rln));
    if (n == 0) {
        // TODO: log error
    } else {
        LCID lcid = LocaleNameToLCID(rln, LOCALE_ALLOW_NEUTRAL_NAMES);
        if (lcid == 0) {
            // TODO: log error
        } else {
            fatal_if_false(SetThreadLocale(lcid));
            memset((void*)nls_ls, 0, sizeof(nls_ls)); // start all over
        }
    }
}

static void nls_init(void) {
    static_assert(countof(nls_ns) % 16 == 0, "countof(ns) must be multiple of 16");
    LANGID langid = MAKELANGID(LANG_ENGLISH, SUBLANG_NEUTRAL);
    for (int32_t strid = 0; strid < countof(nls_ns); strid += 16) {
        int32_t block = strid / 16 + 1;
        HRSRC res = FindResourceExA(((HMODULE)null), RT_STRING,
            MAKEINTRESOURCE(block), langid);
        uint8_t* memory = res == null ? null : (uint8_t*)LoadResource(null, res);
        wchar_t* ws = memory == null ? null : (wchar_t*)LockResource(memory);
        if (ws == null) { break; }
        for (int32_t i = 0; i < 16; i++) {
            int32_t ix = strid + i;
            uint16_t count = ws[0];
            if (count > 0) {
                ws++;
                fatal_if_false(ws[count - 1] == 0, "use rc.exe /n");
                nls_ns[ix] = nls_save_string(ws);
                nls_strings_count = ix + 1;
//              traceln("ns[%d] := %d \"%s\"", ix, strlen(ns[ix]), ns[ix]);
                ws += count;
            } else {
                ws++;
            }
        }
    }
}

nls_if nls = {
    .init   = nls_init,
    .strid  = nls_strid,
    .str    = nls_str,
    .string = nls_string,
    .locale = nls_locale,
    .set_locale = nls_set_locale,
};

