#include "ut/ut.h"
#include "ut/ut_win32.h"

static inline char* ut_inline_str_drop_const(const char* s) {
    #if defined(__GNUC__) || defined(__clang__)
    #pragma GCC diagnostic push
    #pragma GCC diagnostic ignored "-Wcast-qual"
    #endif
    return (char*)s;
    #if defined(__GNUC__) || defined(__clang__)
    #pragma GCC diagnostic pop
    #endif
}

static char* ut_str_drop_const(const char* s) {
    return ut_inline_str_drop_const(s);
}

char* strnchr(const char* s, int32_t n, char ch) {
    while (n > 0 && *s != 0) {
        if (*s == ch) { return ut_inline_str_drop_const(s); }
        s++; n--;
    }
    return null;
}

static void ut_str_format_va(char* utf8, int32_t count, const char* format,
        va_list vl) {
    #if defined(__GNUC__) || defined(__clang__)
    #pragma GCC diagnostic push
    #pragma GCC diagnostic ignored "-Wformat-nonliteral"
    #endif
    vsnprintf(utf8, (size_t)count, format, vl);
    utf8[count - 1] = 0;
    #if defined(__GNUC__) || defined(__clang__)
    #pragma GCC diagnostic pop
    #endif
}

static void ut_str_format(char* utf8, int32_t count, const char* format, ...) {
    va_list vl;
    va_start(vl, format);
    ut_str.format_va(utf8, count, format, vl);
    va_end(vl);
}

static const char* ut_str_error_for_language(int32_t error, LANGID language) {
    static thread_local char text[256];
    const DWORD format = FORMAT_MESSAGE_FROM_SYSTEM|
        FORMAT_MESSAGE_IGNORE_INSERTS;
    uint16_t s[256];
    HRESULT hr = 0 <= error && error <= 0xFFFF ?
        HRESULT_FROM_WIN32((uint32_t)error) : (HRESULT)error;
    if (FormatMessageW(format, null, (DWORD)hr, language, s, countof(s) - 1,
            (va_list*)null) > 0) {
        s[countof(s) - 1] = 0;
        // remove trailing '\r\n'
        int32_t k = (int32_t)wcslen(s);
        if (k > 0 && s[k - 1] == '\n') { s[k - 1] = 0; }
        k = (int32_t)wcslen(s);
        if (k > 0 && s[k - 1] == '\r') { s[k - 1] = 0; }
        char stack[2048];
        fatal_if(k >= countof(stack), "error message too long");
        strprintf(text, "0x%08X(%d) \"%s\"", error, error,
                  ut_str.utf16_utf8(stack, s));
    } else {
        strprintf(text, "0x%08X(%d)", error, error);
    }
    return text;
}

static const char* ut_str_error(int32_t error) {
    const LANGID lang = MAKELANGID(LANG_ENGLISH, SUBLANG_DEFAULT);
    return ut_str_error_for_language(error, lang);
}

static const char* ut_str_error_nls(int32_t error) {
    const LANGID lang = MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT);
    return ut_str_error_for_language(error, lang);
}

static int32_t ut_str_utf8_bytes(const uint16_t* utf16) {
    int32_t required_bytes_count =
        WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS,
        utf16, -1, null, 0, null, null);
    swear(required_bytes_count > 0);
    return required_bytes_count;
}

static int32_t ut_str_utf16_chars(const char* utf8) {
    int32_t required_wide_chars_count =
        MultiByteToWideChar(CP_UTF8, 0, utf8, -1, null, 0);
    swear(required_wide_chars_count > 0);
    return required_wide_chars_count;
}

static char* ut_str_utf16to8(char* s, const uint16_t* utf16) {
    errno_t r = WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS, utf16, -1, s,
        ut_str.utf8_bytes(utf16), null, null);
    fatal_if(r == 0, "WideCharToMultiByte() failed %s",
            ut_str.error(ut_runtime.err()));
    return s;
}

static uint16_t* ut_str_utf8to16(uint16_t* utf16, const char* s) {
    errno_t r = MultiByteToWideChar(CP_UTF8, 0, s, -1, utf16,
                                ut_str.utf16_chars(s));
    fatal_if(r == 0, "WideCharToMultiByte() failed %s",
            ut_str.error(ut_runtime.err()));
    return utf16;
}

static bool ut_str_is_empty(const char* s) {
    return strempty(s);
}

static bool ut_str_equal(const char* s1, int32_t n1, const char* s2, int32_t n2) {
    if (n1 < 0) { n1 = ut_str.length(s1); }
    if (n2 < 0) { n2 = ut_str.length(s2); }
    return n1 == n2 && memcmp(s1, s2, (size_t)n1) == 0;
}

static bool ut_str_equal_nc(const char* s1, int32_t n1, const char* s2, int32_t n2) {
    if (n1 < 0 && n2 < 0) {
        return striequ(s1, s2);
    }
    if (n1 < 0) { n1 = ut_str.length(s1); }
    if (n2 < 0) { n2 = ut_str.length(s2); }
    if (n1 != n2) { return false; }
    for (int32_t i = 0; i < n1; i++) {
        if (tolower(s1[i]) != tolower(s2[i])) { return false; }
    }
    return true;
}

static bool ut_str_starts_with(const char* s1, int32_t n1, const char* s2, int32_t n2) {
    if (n1 < 0 && n2 < 0) { return strstartswith(s1, s2); }
    if (n1 <= 0) { n1 = (int32_t)strlen(s1); }
    if (n2 <= 0) { n2 = (int32_t)strlen(s2); }
    return n1 >= n2 && memcmp(s1, s2, (size_t)n2) == 0;
}

static bool ut_str_ends_with(const char* s1, int32_t n1, const char* s2, int32_t n2) {
    if (n1 < 0 && n2 < 0) {
        return s1 != null && s2 != null && strendswith(s1, s2);
    }
    if (n1 <= 0) { n1 = (int32_t)strlen(s1); }
    if (n2 <= 0) { n2 = (int32_t)strlen(s2); }
    return n1 >= n2 && memcmp(s1 + n1 - n2, s2, (size_t)n2) == 0;
}

static bool ut_str_starts_with_nc(const char* s1, int32_t n1, const char* s2, int32_t n2) {
    if (n1 < 0 && n2 < 0) { return strstartswith(s1, s2); }
    if (n1 <= 0) { n1 = (int32_t)strlen(s1); }
    if (n2 <= 0) { n2 = (int32_t)strlen(s2); }
    return n1 >= n2 && ut_str.compare_nc(s1, n2, s2, n2) == 0;
}

static bool ut_str_ends_with_nc(const char* s1, int32_t n1, const char* s2, int32_t n2) {
    if (n1 <= 0) { n1 = (int32_t)strlen(s1); }
    if (n2 <= 0) { n2 = (int32_t)strlen(s2); }
    return n1 >= n2 && ut_str.compare_nc(s1 + n1 - n2, n2, s2, n2) == 0;
}

static int32_t ut_str_length(const char* s) {
    return strlength(s);
}

static bool ut_str_copy(char* d, int32_t capacity,
                     const char* s, int32_t bytes) {
    not_null(d);
    not_null(s);
    swear(capacity > 1, "capacity: %d", capacity);
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

static char* ut_str_first_char(const char* s, int32_t bytes, char ch) {
    return bytes > 0 ? strnchr(s, bytes, ch) : strchr(s, ch);
}

static char* ut_str_last_char(const char* s, int32_t n, char ch) {
    if (n < 0) { return strrchr(s, ch); }
    for (int32_t i = n; i >= 0; i--) {
        if (s[i] == ch) { return ut_inline_str_drop_const(&s[i]); }
    }
    return null;
}

static char* ut_str_first(const char* s1, int32_t n1, const char* s2,
        int32_t n2) {
    if (n1 < 0 && n2 < 0) { return strstr(s1, s2); }
    if (n1 < 0) { n1 = ut_str.length(s1); }
    if (n2 < 0) { n2 = ut_str.length(s2); }
    for (int32_t i = 0; i <= n1 - n2; i++) {
        if (memcmp(s1 + i, s2, (size_t)n2) == 0) {
            return ut_inline_str_drop_const(&s1[i]);
        }
    }
    return null;
}

static bool ut_str_to_lower(char* d, int32_t capacity, const char* s, int32_t n) {
    swear(capacity > 1, "capacity: %d", capacity);
    if (n < 0) {
        while (*s != 0 && capacity > 1) {
            *d++ = (char)tolower(*s++);
            capacity--;
        }
        *d = 0;
        return *s == 0;
    } else {
        while (capacity > 1 && n > 0) {
            *d++ = (char)tolower(*s++);
            capacity--;
            n--;
        }
        *d = 0;
        return n == 0;
    }
}

static bool ut_str_to_upper(char* d, int32_t capacity, const char* s, int32_t n) {
    swear(capacity > 1, "capacity: %d", capacity);
    if (n < 0) {
        while (*s != 0 && capacity > 1) {
            *d++ = (char)toupper(*s++);
            capacity--;
        }
        *d = 0;
        return *s == 0;
    } else {
        while (capacity > 1 && n > 0) {
            *d++ = (char)toupper(*s++);
            capacity--;
            n--;
        }
        *d = 0;
        return n == 0;
    }
}

static int32_t ut_str_compare(const char* s1, int32_t n1, const char* s2, int32_t n2) {
    if (n2 < 0) {
        return n1 > 0 ? strncmp(s1, s2, (size_t)n1) : strcmp(s1, s2);
    } else {
        if (n1 < 0) { n1 = ut_str.length(s1); }
        if (n2 < 0) { n2 = ut_str.length(s2); }
        return n1 == n2 ? memcmp(s1, s2, (size_t)n1) : n1 - n2;
    }
}

static int32_t ut_str_compare_nc(const char* s1, int32_t n1, const char* s2, int32_t n2) {
    if (n2 < 0) {
        return n1 > 0 ? strncasecmp(s1, s2, (size_t)n1) : strcasecmp(s1, s2);
    } else {
        if (n1 < 0) { n1 = ut_str.length(s1); }
        if (n2 < 0) { n2 = ut_str.length(s2); }
        int32_t n = ut_min(n1, n2);
        for (int32_t i = 0; i < n; i++) {
            int32_t r = tolower(s1[i]) - tolower(s2[i]);
            if (r != 0) { return r; }
        }
        return n1 - n2;
    }
}

static const char* ut_str_unquote(char* *s, int32_t n) {
    if (n < 0) { n = ut_str.length(*s); }
    if (n > 0 && (*s)[0] == '\"' && (*s)[n - 1] == '\"') {
        (*s)[n - 1] = 0x00;
        (*s)++;
    }
    return *s;
}


#pragma push_macro("ut_thread_local_str_pool")

// upto "n" strings of "count = "bytes" per thread
// can be used in one call:

#define ut_thread_local_str_pool(text_, count_, n_, bytes_) \
    enum { count_ = bytes_ };                               \
    static volatile int32_t ix_;                            \
    static char strings_[n_][bytes_];                       \
    char* text = strings_[                                  \
        ut_atomics.increment_int32(&ix_) %                  \
        countof(strings_)];

// Posix and Win32 C runtime:
//   #include <locale.h>
//   struct lconv *locale_info = localeconv();
//   const char* grouping_separator = locale_info->thousands_sep;
//   const char* decimal_separator = locale_info->decimal_point;
// en-US Windows 1x:
// grouping_separator == ""
// decimal_separator  == "."
//
// Win32 API:
//   b2e(GetLocaleInfo(LOCALE_USER_DEFAULT, LOCALE_STHOUSAND,
//       grouping_separator, sizeof(grouping_separator)));
//   b2e(GetLocaleInfo(LOCALE_USER_DEFAULT, LOCALE_SDECIMAL,
//       decimal_separator, sizeof(decimal_separator)));
// en-US Windows 1x:
// grouping_separator == ","
// decimal_separator  == "."

static const char* ut_str_int64_dg(int64_t v, // digit_grouped
        bool uint, const char* gs) { // grouping separator: gs
    // 64 calls per thread 32 or less bytes each because:
    // "18446744073709551615" 21 characters
    // "18'446'744'073'709'551'615" 27 characters
    ut_thread_local_str_pool(text, count, 64, 32); // 2KB
    // When sprintf format %`lld is not implemented do it hard way:
    uint64_t n = uint || v >= 0 ? (uint64_t)v : (uint64_t)INT64_MIN;
    int32_t i = 0;
    int groups[8]; // 2^63 - 1 ~= 9 x 10^19 upto 7 groups of 3 digits
    while (n > 0) {
        groups[i] = n % 1000;
        n = n / 1000;
        i++;
    }
    const int gc = i - 1; // group count
    char* s = text;
    if (v < 0 && !uint) { *s++ = '-'; } // sign
    int k = 0;
    while (i > 0) {
        i--;
        if (i == gc) {
            ut_str.format(s, 5, i > 0 ? "%d%s" : "%d", groups[i], gs);
        } else {
            ut_str.format(s, 5, i > 0 ? "%03d%s" : "%03d", groups[i], gs);
        }
        k = (int32_t)strlen(s);
        s += k;
    }
    *s = 0;
    return text;
}

static const char* ut_str_int64(int64_t v) {
    return ut_str_int64_dg(v, false, "\xE2\x80\x89");
}

const char* ut_str_uint64(uint64_t v) {
    return ut_str_int64_dg(v, true, "\xE2\x80\x89");
}

static const char* ut_str_grouping_separator(void) {
    #ifdef WINDOWS
        // en-US Windows 10/11:
        // grouping_separator == ","
        // decimal_separator  == "."
        static char grouping_separator[8];
        if (grouping_separator[0] == 0x00) {
            errno_t r = b2e(GetLocaleInfo(LOCALE_USER_DEFAULT, LOCALE_STHOUSAND,
                grouping_separator, sizeof(grouping_separator)));
            swear(r == 0 && grouping_separator[0] != 0);
        }
        return grouping_separator;
    #else
        // en-US Windows 10/11:
        // grouping_separator == ""
        // decimal_separator  == "."
        struct lconv *locale_info = localeconv();
        const char* grouping_separator = null;
        if (grouping_separator == null) {
            grouping_separator = locale_info->thousands_sep;
            swear(grouping_separator != null);
        }
        return grouping_separator;
    #endif
}

static const char* ut_str_int64_lc(int64_t v) {
    return ut_str_int64_dg(v, false, ut_str_grouping_separator());
}

const char* ut_str_uint64_lc(uint64_t v) {
    return ut_str_int64_dg(v, true, ut_str_grouping_separator());
}

static const char* ut_str_fp(const char* format, fp64_t v) {
    static char decimal_separator[8];
    if (decimal_separator[0] == 0) {
        errno_t r = b2e(GetLocaleInfo(LOCALE_USER_DEFAULT, LOCALE_SDECIMAL,
            decimal_separator, sizeof(decimal_separator)));
        swear(r == 0 && decimal_separator[0] != 0);
    }
    swear(strlen(decimal_separator) <= 4);
    char f[128]; // formatted float point
    // 32 calls per thread
    ut_thread_local_str_pool(text, count, 32, countof(f) + 5);
    // snprintf format does not handle thousands separators on all know runtimes
    // and respects setlocale() on Un*x systems but in MS runtime only when
    // _snprintf_l() is used.
    f[0] = 0x00;
    ut_str.format(f, countof(f), format, v);
    f[countof(f) - 1] = 0x00;
    char* s = f;
    char* d = text;
    while (*s != 0x00) {
        if (*s == '.') {
            const char* sep = decimal_separator;
            while (*sep != 0x00) { *d++ = *sep++; }
            s++;
        } else {
            *d++ = *s++;
        }
    }
    *d = 0x00;
    // TODO: It's possible to handle mantissa grouping but...
    // Not clear if human expects it in 5 digits or 3 digits chunks
    // and unfortunately locale does not specify how
    return text;
}

#pragma pop_macro("ut_thread_local_str_pool")

#ifdef UT_TESTS

static void ut_str_test(void) {
    #pragma push_macro("glyph_chinese_one")
    #pragma push_macro("glyph_chinese_two")
    #pragma push_macro("glyph_teddy_bear")
    #pragma push_macro("glyph_ice_cube")
    #define glyph_chinese_one "\xE5\xA3\xB9"
    #define glyph_chinese_two "\xE8\xB4\xB0"
    #define glyph_teddy_bear  "\xF0\x9F\xA7\xB8"
    #define glyph_ice_cube    "\xF0\x9F\xA7\x8A"
    const char* null_string = null;
    swear(strempty(null_string));
    swear(strempty(""));
    swear(!strempty("hello"));
    swear(strequ("hello", "hello"));
    swear(!strequ("hello", "world"));
    swear(strcasecmp("hello", "HeLLo") == 0);
    swear(strncasecmp("hello", "HeLLo", 4) == 0);
    swear(strcasecmp("hello", "world") != 0);
    swear(strncasecmp("hello", "world", 4) != 0);
    swear(strequ(strconcat("hello_", "_world"), "hello__world"));
    swear(ut_str.is_empty(null_string));
    swear(ut_str.is_empty(""));
    swear(!ut_str.is_empty("hello"));
    swear(ut_str.equal("hello", -1, "hello", -1));
    swear(ut_str.equal_nc("hello", -1, "HeLlO", -1));
    swear(!ut_str.equal("hello", -1, "world", -1));
    swear(!ut_str.equal("hello", -1, "hello", 4));
    swear(!ut_str.equal("hello",  4, "hello", -1));
    swear(ut_str.equal("hellO",  4, "hello", 4));
    swear(ut_str.equal("abc", 1, "a", 1));
    swear(!ut_str.equal("a", 1, "abc", 3));
    swear(ut_str.starts_with("hello world", -1, "hello", -1));
    swear(!ut_str.starts_with("hello world", -1, "world", -1));
    swear(ut_str.ends_with("hello world", -1, "world", -1));
    swear(!ut_str.ends_with("hello world", -1, "hello", -1));
    swear(ut_str.length("hello") == 5);
    char text[10];
    swear(ut_str.copy(text, countof(text), "hello", -1));
    swear(ut_str.equal(text, -1, "hello", -1));
    swear(!ut_str.copy(text, 9, "0123456789", -1));
    char lower[20];
    swear(ut_str.to_lower(lower, countof(lower), "HeLlO WoRlD", -1));
    swear(ut_str.equal(lower, -1, "hello world", -1));
    const char* utf8_str =
            glyph_teddy_bear "0"
            glyph_chinese_one glyph_chinese_two
            "3456789 "
            glyph_ice_cube;
    const uint16_t* wide_str = utf8to16(utf8_str);
    char utf8[100];
    swear(ut_str.utf16_utf8(utf8, wide_str));
    uint16_t utf16[100];
    swear(ut_str.utf8_utf16(utf16, utf8));
    swear(ut_str.equal(utf16to8(utf16), -1, utf8_str, -1));
    char formatted[100];
    ut_str.format(formatted, countof(formatted), "n: %d, s: %s", 42, "test");
    swear(ut_str.equal(formatted, -1, "n: 42, s: test", -1));
    char truncated[9];
    truncated[8] = 0xFF;
    swear(!ut_str.copy(truncated, countof(truncated), "0123456789", -1));
    swear(truncated[8] == 0);
    char truncated_lower[9];
    truncated_lower[8] = 0xFF;
    ut_str.to_lower(truncated_lower, countof(truncated_lower), "HELLO012345", -1);
    swear(truncated_lower[8] == 0);
    char truncated_formatted[9];
    truncated_formatted[8] = 0xFF;
    ut_str.format(truncated_formatted, countof(truncated_formatted), "n: %d, s: %s", 42, "a long test string");
    swear(truncated_formatted[8] == 0);
    char very_short_str[1];
    very_short_str[0] = 0xFF;
    ut_str.format(very_short_str, countof(very_short_str), "%s", "test");
    swear(very_short_str[0] == 0);
    swear(ut_str.starts_with("example text", 7, "exam", 4));
    swear(ut_str.starts_with_nc("example text", 7, "ExAm", 4));
    swear(ut_str.ends_with("testing", 7, "ing", 3));
    swear(ut_str.ends_with_nc("testing", 7, "InG", 3));
    swear(ut_str.compare("hello", 5, "hello", 5) == 0);
    swear(ut_str.compare_nc("Hello", 5, "hello", 5) == 0);
    swear(ut_str.compare("ab", 2, "abc", 3) < 0);
    swear(ut_str.compare_nc("abc", 3, "ABCD", 4) < 0);
    // numeric values digit grouping format:
    swear(strequ("18,446,744,073,709,551,615",
        ut_str.int64_dg(UINT64_MAX, true, ",")
    ));
    swear(strequ("9,223,372,036,854,775,807",
        ut_str.int64_dg(INT64_MAX, false, ",")
    ));
    swear(strequ("-9,223,372,036,854,775,808",
        ut_str.int64_dg(INT64_MIN, false, ",")
    ));
    //  see:
    // https://en.wikipedia.org/wiki/Single-precision_floating-point_format
    uint32_t pi_fp32 = 0x40490FDBULL; // 3.14159274101257324
    swear(strequ("3.141592741",
                ut_str.fp("%.9f", *(fp32_t*)&pi_fp32)),
          "%s", ut_str.fp("%.9f", *(fp32_t*)&pi_fp32)
    );
    //  3.141592741
    //  ********^ (*** true digits ^ first rounded digit)
    //    123456 (%.6f)
    //
    //  https://en.wikipedia.org/wiki/Double-precision_floating-point_format
    uint64_t pi_fp64 = 0x400921FB54442D18ULL;
    swear(strequ("3.141592653589793116",
                ut_str.fp("%.18f", *(fp64_t*)&pi_fp64)),
          "%s", ut_str.fp("%.18f", *(fp64_t*)&pi_fp64)
    );
    //  3.141592653589793116
    //  *****************^ (*** true digits ^ first rounded digit)
    //    123456789012345 (%.15f)
    //  https://en.wikipedia.org/wiki/Double-precision_floating-point_format
    //
    //  actual "pi" first 64 digits:
    //  3.1415926535897932384626433832795028841971693993751058209749445923
    if (ut_debug.verbosity.level > ut_debug.verbosity.quiet) { traceln("done"); }
    #pragma pop_macro("glyph_ice_cube")
    #pragma pop_macro("glyph_teddy_bear")
    #pragma pop_macro("glyph_chinese_two")
    #pragma pop_macro("glyph_chinese_one")
}

#else

static void ut_str_test(void) {}

#endif

ut_str_if ut_str = {
    .drop_const         = ut_str_drop_const,
    .is_empty           = ut_str_is_empty,
    .equal              = ut_str_equal,
    .equal_nc           = ut_str_equal_nc,
    .length             = ut_str_length,
    .copy               = ut_str_copy,
    .first_char         = ut_str_first_char,
    .last_char          = ut_str_last_char,
    .first              = ut_str_first,
    .to_lower           = ut_str_to_lower,
    .to_upper           = ut_str_to_upper,
    .compare            = ut_str_compare,
    .compare_nc         = ut_str_compare_nc,
    .starts_with        = ut_str_starts_with,
    .ends_with          = ut_str_ends_with,
    .starts_with_nc     = ut_str_starts_with_nc,
    .ends_with_nc       = ut_str_ends_with_nc,
    .unquote            = ut_str_unquote,
    .utf8_bytes         = ut_str_utf8_bytes,
    .utf16_chars        = ut_str_utf16_chars,
    .utf16_utf8         = ut_str_utf16to8,
    .utf8_utf16         = ut_str_utf8to16,
    .format             = ut_str_format,
    .format_va          = ut_str_format_va,
    .error              = ut_str_error,
    .error_nls          = ut_str_error_nls,
    .grouping_separator = ut_str_grouping_separator,
    .int64_dg           = ut_str_int64_dg,
    .int64              = ut_str_int64,
    .uint64             = ut_str_uint64,
    .int64_lc           = ut_str_int64,
    .uint64_lc          = ut_str_uint64,
    .fp                 = ut_str_fp,
    .test               = ut_str_test
};
