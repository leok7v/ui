#include "util/runtime.h"
#include "util/win32.h"

char* strnchr(const char* s, int32_t n, char ch) {
    while (n > 0 && *s != 0) {
        if (*s == ch) { return (char*)s; }
        s++; n--;
    }
    return null;
}

static void str_vformat(char* utf8, int32_t count, const char* format, va_list vl) {
    vsnprintf(utf8, count, format, vl);
    utf8[count - 1] = 0;
}

static void str_sformat(char* utf8, int32_t count, const char* format, ...) {
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
        int32_t k = (int32_t)wcslen(s);
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

static int32_t str_utf8_bytes(const wchar_t* utf16) {
    int32_t required_bytes_count =
        WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS,
        utf16, -1, null, 0, null, null);
    swear(required_bytes_count > 0);
    return required_bytes_count;
}

static int32_t str_utf16_chars(const char* utf8) {
    int32_t required_wide_chars_count =
        MultiByteToWideChar(CP_UTF8, 0, utf8, -1, null, 0);
    swear(required_wide_chars_count > 0);
    return required_wide_chars_count;
}

static char* str_utf16to8(char* s, const wchar_t* utf16) {
    errno_t r = WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS, utf16, -1, s,
        str.utf8_bytes(utf16), null, null);
    fatal_if(r == 0, "WideCharToMultiByte() failed %s",
            str.error(runtime.err()));
    return s;
}

static wchar_t* str_utf8to16(wchar_t* utf16, const char* s) {
    errno_t r = MultiByteToWideChar(CP_UTF8, 0, s, -1, utf16,
                                str.utf16_chars(s));
    fatal_if(r == 0, "WideCharToMultiByte() failed %s",
            str.error(runtime.err()));
    return utf16;
}

static bool str_is_empty(const char* s) {
    return strempty(s);
}

static bool str_equal(const char* s1, const char* s2) {
    return strequ(s1, s2);
}

static bool str_equal_nc(const char* s1, const char* s2) {
    return striequ(s1, s2);
}

static bool str_starts_with(const char* s1, const char* s2) {
    return s1 != null && s2 != null && strstartswith(s1, s2);
}

static bool str_ends_with(const char* s1, const char* s2) {
    return s1 != null && s2 != null && strendswith(s1, s2);
}

static bool str_ends_with_nc(const char* s1, const char* s2) {
    size_t n1 = s1 == null ? 0 : strlen(s1);
    size_t n2 = s2 == null ? 0 : strlen(s2);
    return s1 != null && s2 != null &&
           n1 >= n2 && stricmp(s1 + n1 - n2, s2) == 0;
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

static bool str_to_lower(char* d, int32_t capacity, const char* s) {
    swear(capacity > 0, "capacity: %d", capacity);
    while (*s != 0 && capacity > 0) { *d++ = (char)tolower(*s++); }
    *d = 0;
    return *s == 0;
}

static bool str_to_upper(char* d, int32_t capacity, const char* s) {
    swear(capacity > 0, "capacity: %d", capacity);
    while (*s != 0 && capacity > 0) { *d++ = (char)toupper(*s++); }
    *d = 0;
    return *s == 0;
}

static int32_t str_compare(const char* s1, int32_t bytes, const char* s2) {
    return bytes > 0 ? strncmp(s1, s2, bytes) : strcmp(s1, s2);
}

static int32_t str_compare_nc(const char* s1, int32_t bytes, const char* s2) {
    return bytes > 0 ? strncasecmp(s1, s2, bytes) : strcasecmp(s1, s2);
}

static void str_test(void) {
#ifdef RUNTIME_TESTS
    #pragma push_macro("glyph_chinese_one")
    #pragma push_macro("glyph_chinese_two")
    #pragma push_macro("glyph_teddy_bear")
    #pragma push_macro("glyph_ice_cube")

    #define glyph_chinese_one "\xE5\xA3\xB9"
    #define glyph_chinese_two "\xE8\xB4\xB0"
    #define glyph_teddy_bear  "\xF0\x9F\xA7\xB8"
    #define glyph_ice_cube    "\xF0\x9F\xA7\x8A"

    swear(str.is_empty(null));
    swear(str.is_empty(""));
    swear(!str.is_empty("hello"));
    swear(str.equal("hello", "hello"));
    swear(str.equal_nc("hello", "HeLlO"));
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
    // --- to_lower ---
    char lower[20];
    swear(str.to_lower(lower, sizeof(lower), "HeLlO WoRlD"));
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
    // Verify round-trip conversion:
    swear(str.equal(utf16to8(wide_buf), utf8_str));
    // --- strprintf ---
    char formatted[100];
    str.sformat(formatted, countof(formatted), "n: %d, s: %s", 42, "test");
    swear(str.equal(formatted, "n: 42, s: test"));
    // str.copy() truncation
    char truncated_buf[5]; // Truncate to fit:
    str.copy(truncated_buf, countof(truncated_buf), "hello", -1);
    swear(truncated_buf[4] == 0, "expected zero termination");
    // str.to_lower() truncation
    char truncated_lower[6]; // Truncate to fit:
    str.to_lower(truncated_lower, countof(truncated_lower), "HELLO");
    swear(truncated_lower[5] == 0, "expected zero termination");
    // str.sformat() truncation
    char truncated_formatted[8]; // Truncate to fit:
    str.sformat(truncated_formatted, countof(truncated_formatted),
                "n: %d, s: %s", 42, "a long test string");
    swear(truncated_formatted[7] == 0, "expected zero termination");
    // str.sformat() truncation
    char very_short_str[1];
    very_short_str[0] = 0xFF; // not zero terminated
    strprintf(very_short_str, "%s", "test");
    swear(very_short_str[0] == 0, "expected zero termination");
    if (debug.verbosity.level > debug.verbosity.quiet) { traceln("done"); }
    #pragma pop_macro("glyph_ice_cube")
    #pragma pop_macro("glyph_teddy_bear")
    #pragma pop_macro("glyph_chinese_two")
    #pragma pop_macro("glyph_chinese_one")
#endif
}

str_if str = {
    .vformat      = str_vformat,
    .sformat      = str_sformat,
    .error        = str_error,
    .error_nls    = str_error_nls,
    .utf8_bytes   = str_utf8_bytes,
    .utf16_chars  = str_utf16_chars,
    .utf16_utf8   = str_utf16to8,
    .utf8_utf16   = str_utf8to16,
    .is_empty     = str_is_empty,
    .equal        = str_equal,
    .equal_nc     = str_equal_nc,
    .length       = str_length,
    .copy         = str_copy,
    .first_char   = str_first_char,
    .last_char    = str_last_char,
    .first        = str_first,
    .to_lower     = str_to_lower,
    .to_upper     = str_to_upper,
    .compare      = str_compare,
    .compare_nc   = str_compare_nc,
    .starts_with  = str_starts_with,
    .ends_with    = str_ends_with,
    .ends_with_nc = str_ends_with_nc,
    .test         = str_test
};
