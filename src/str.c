#include "rt.h"

char* strnchr(const char* s, int n, char ch) {
    while (n > 0 && *s != 0) {
        if (*s == ch) { return (char*)s; }
        s++; n--;
    }
    return null;
}

static void str_vformat(char* utf8, int count, const char* format, va_list vl) {
    vsnprintf(utf8, count, format, vl);
    utf8[count - 1] = 0;
}

static void str_sformat(char* utf8, int count, const char* format, ...) {
    va_list vl;
    va_start(vl, format);
    str.vformat(utf8, count, format, vl);
    va_end(vl);
}

static const char* str_error_for_language(int32_t error, LANGID language) {
    static thread_local char text[256];
    const DWORD format = FORMAT_MESSAGE_FROM_SYSTEM|
        FORMAT_MESSAGE_IGNORE_INSERTS;
    wchar_t s[256];
    HRESULT hr = 0 <= error && error <= 0xFFFF ?
        HRESULT_FROM_WIN32(error) : error;
    if (FormatMessageW(format, null, hr, language, s, countof(s) - 1,
            (va_list*)null) > 0) {
        s[countof(s) - 1] = 0;
        // remove trailing '\r\n'
        int k = (int)wcslen(s);
        if (k > 0 && s[k - 1] == '\n') { s[k - 1] = 0; }
        k = (int)wcslen(s);
        if (k > 0 && s[k - 1] == '\r') { s[k - 1] = 0; }
        strprintf(text, "0x%08X(%d) \"%s\"", error, error, utf16to8(s));
    } else {
        strprintf(text, "0x%08X(%d)", error, error);
    }
    return text;
}

static const char* str_error(int32_t error) {
    const LANGID lang = MAKELANGID(LANG_ENGLISH, SUBLANG_DEFAULT);
    return str_error_for_language(error, lang);
}

static const char* str_error_nls(int32_t error) {
    const LANGID lang = MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT);
    return str_error_for_language(error, lang);
}

static int str_utf8_bytes(const wchar_t* utf16) {
    int required_bytes_count =
        WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS,
        utf16, -1, null, 0, null, null);
    swear(required_bytes_count > 0);
    return required_bytes_count;
}

static int str_utf16_chars(const char* utf8) {
    int required_wide_chars_count =
        MultiByteToWideChar(CP_UTF8, 0, utf8, -1, null, 0);
    swear(required_wide_chars_count > 0);
    return required_wide_chars_count;
}

static char* str_utf16to8(char* s, const wchar_t* utf16) {
    int r = WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS, utf16, -1, s,
        str.utf8_bytes(utf16), null, null);
    if (r == 0) {
        traceln("WideCharToMultiByte() failed %s", str.error(crt.err()));
    }
    return s;
}

static wchar_t* str_utf8to16(wchar_t* utf16, const char* s) {
    int r = MultiByteToWideChar(CP_UTF8, 0, s, -1, utf16,
                                str.utf16_chars(s));
    if (r == 0) {
        traceln("WideCharToMultiByte() failed %s", str.error(crt.err()));
    }
    return utf16;
}

static bool str_is_empty(const char* s) {
    return strempty(s);
}

static bool str_equal(const char* s1, const char* s2) {
    return strequ(s1, s2);
}

static bool str_starts_with(const char* s1, const char* s2) {
    return strstartswith(s1, s2);
}

static bool str_ends_with(const char* s1, const char* s2) {
    return strendswith(s1, s2);
}

static int32_t str_length(const char* s) {
    return strlength(s);
}

static bool str_copy(char* d, int32_t capacity,
                     const char* s, int32_t bytes) {
    not_null(d);
    not_null(s);
    swear(capacity > 0, "capacity: %d", capacity);
    if (bytes < 0) {
        while (capacity > 1 && *s != 0) {
            *d++ = *s++;
            capacity--;
        }
        *d = 0;
        return *s == 0;
    } else {
        while (capacity > 1 && *s != 0 && bytes > 0) {
            *d++ = *s++;
            capacity--;
            bytes--;
        }
        *d = 0;
        return *s == 0 || bytes == 0;
    }
}

static char* str_first_char(const char* s, int32_t bytes, char ch) {
    return bytes > 0 ? strnchr(s, bytes, ch) : strchr(s, ch);
}

static char* str_last_char(const char* s1, char ch) {
    return strrchr(s1, ch);
}

static char* str_first(const char* s1, const char* s2) {
    return strstr(s1, s2);
}

static bool str_to_lowercase(char* d, int32_t capacity, const char* s) {
    swear(capacity > 0, "capacity: %d", capacity);
    while (*s != 0 && capacity > 0) { *d++ = (char)tolower(*s++); }
    *d = 0;
    return *s == 0;
}

static int str_compare(const char* s1, int32_t bytes, const char* s2) {
    return bytes > 0 ? strncmp(s1, s2, bytes) : strcmp(s1, s2);
}

static int str_compare_ignore_case(const char* s1, int32_t bytes,
                                   const char* s2) {
    return bytes > 0 ? strncasecmp(s1, s2, bytes) : strcasecmp(s1, s2);
}

// test:

#pragma push_macro("glyph_chinese_one")
#pragma push_macro("glyph_chinese_two")
#pragma push_macro("glyph_teddy_bear")
#pragma push_macro("glyph_ice_cube")

#define glyph_chinese_one "\xE5\xA3\xB9"
#define glyph_chinese_two "\xE8\xB4\xB0"
#define glyph_teddy_bear  "\xF0\x9F\xA7\xB8"
#define glyph_ice_cube    "\xF0\x9F\xA7\x8A"

static void str_test(int32_t verbosity) {
    // TODO: this is very minimalistic test. Needs to be extended.
    swear(str.is_empty(""));
    swear(!str.is_empty("hello"));
    swear(str.equal("hello", "hello"));
    swear(!str.equal("hello", "world"));
    // --- starts_with, ends_with ---
    swear(str.starts_with("hello world", "hello"));
    swear(!str.starts_with("hello world", "world"));
    swear(str.ends_with("hello world", "world"));
    swear(!str.ends_with("hello world", "hello"));
    // --- length, copy ---
    swear(str.length("hello") == 5);
    char buf[10];
    swear(str.copy(buf, sizeof(buf), "hello", -1));  // Copy whole string
    swear(str.equal(buf, "hello"));
    swear(!str.copy(buf, 3, "hello", -1)); // Buffer too small
    // --- to_lowercase ---
    char lower[20];
    swear(str.to_lowercase(lower, sizeof(lower), "HeLlO WoRlD"));
    swear(str.equal(lower, "hello world"));
    // --- UTF-8 / UTF-16 conversion ---
    const char* utf8_str =  glyph_teddy_bear
        "0" glyph_chinese_one glyph_chinese_two "3456789 "
        glyph_ice_cube;
    const wchar_t* wide_str = utf8to16(utf8_str); // stack allocated
    char utf8_buf[100];
    swear(str.utf16_utf8(utf8_buf, wide_str));
    wchar_t wide_buf[100];
    swear(str.utf8_utf16(wide_buf, utf8_buf));
    swear(str.equal(utf16to8(wide_buf), utf8_str)); // Verify round-trip conversion
    // --- strprintf ---
    char formatted[100];
    str.sformat(formatted, sizeof(formatted), "Number: %d, String: %s", 42, "test");
    swear(str.equal(formatted, "Number: 42, String: test"));
    if (verbosity > 0) { traceln("done"); }
}

#pragma pop_macro("glyph_ice_cube")
#pragma pop_macro("glyph_teddy_bear")
#pragma pop_macro("glyph_chinese_two")
#pragma pop_macro("glyph_chinese_one")

str_if str = {
    .vformat = str_vformat,
    .sformat = str_sformat,
    .error = str_error,
    .error_nls = str_error_nls,
    .utf8_bytes = str_utf8_bytes,
    .utf16_chars = str_utf16_chars,
    .utf16_utf8 = str_utf16to8,
    .utf8_utf16 = str_utf8to16,
    .is_empty = str_is_empty,
    .equal = str_equal,
    .starts_with = str_starts_with,
    .ends_with = str_ends_with,
    .length = str_length,
    .copy = str_copy,
    .first_char = str_first_char,
    .last_char = str_last_char,
    .first = str_first,
    .to_lowercase = str_to_lowercase,
    .compare = str_compare,
    .compare_ignore_case = str_compare_ignore_case,
    .test = str_test
};
