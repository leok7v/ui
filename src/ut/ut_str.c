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

static int32_t ut_str_len(const char* s) { return (int32_t)strlen(s); }

static int32_t ut_str_utf16len(const uint16_t* ws) { return (int32_t)wcslen(ws); }


static int32_t ut_str_cmp(const char* s1, const char* s2) {
    return strcmp(s1, s2);
}

static int32_t ut_str_i_cmp(const char* s1, const char* s2) {
    return stricmp(s1, s2);
}

static bool ut_str_equ(const char* s1, const char* s2) {
    return strcmp(s1, s2) == 0;
}

static bool ut_str_i_equ(const char* s1, const char* s2) {
    return stricmp(s1, s2) == 0;
}

static char* ut_str_chr(const char* s, char ch) {
    return ut_str_drop_const(strchr(s, ch));
}

static char* ut_str_r_chr(const char* s, char ch) {
    return ut_str_drop_const(strrchr(s, ch));
}

static char* ut_str_str(const char* s1, const char* s2) {
    swear(s2[0] != 0);
    return ut_str_drop_const(strstr(s1, s2));
}

static char* ut_str_i_str(const char* s1, const char* s2) {
    int32_t n1 = ut_str.len(s1);
    int32_t n2 = ut_str.len(s2);
    if (n2 <= n1) {
        const char* s = s1;
        while (s + n2 <= s1 + n1) {
            if (strnicmp(s, s2, n2) == 0) { return ut_str_drop_const(s); }
            s++;
        }
    }
    return null;
}

static char* ut_str_r_str(const char* s1, const char* s2) {
    int32_t n1 = ut_str.len(s1);
    int32_t n2 = ut_str.len(s2);
    swear(n2 > 0);
    if (n2 <= n1) {
        const char* s = s1 + n1 - n2;
        while (s >= s1) {
            if (memcmp(s, s2, n2) == 0) { return ut_str_drop_const(s); }
            s--;
        }
    }
    return null;
}

static char* ut_str_i_r_str(const char* s1, const char* s2) {
    int32_t n1 = ut_str.len(s1);
    int32_t n2 = ut_str.len(s2);
    swear(n2 > 0);
    if (n2 <= n1) {
        const char* s = s1 + n1 - n2;
        while (s >= s1) {
            if (strnicmp(s, s2, n2) == 0) { return ut_str_drop_const(s); }
            s--;
        }
    }
    return null;
}

static void ut_str_cpy(char* d, int32_t capacity, const char* s) {
    int32_t n = ut_str.len(s);
    swear(capacity > n);
    memcpy(d, s, n + 1);
}

static void ut_str_cat(char* d, int32_t capacity, const char* s) {
    int32_t n = ut_str.len(s);
    swear(capacity > n);
    int32_t k = ut_str.len(d);
    memcpy(d + k, s, n + 1);
}

static void ut_str_lower(char* d, int32_t capacity, const char* s) {
    int32_t n = ut_str.len(s);
    swear(capacity > n);
    for (int32_t i = 0; i < n; i++) { d[i] = (char)tolower(s[i]); }
    d[n] = 0;
}

static void ut_str_upper(char* d, int32_t capacity, const char* s) {
    int32_t n = ut_str.len(s);
    swear(capacity > n);
    for (int32_t i = 0; i < n; i++) { d[i] = (char)toupper(s[i]); }
    d[n] = 0;
}

static bool ut_str_starts(const char* s1, const char* s2) {
    return ut_str.str(s1, s2) == s1;
}

static bool ut_str_ends(const char* s1, const char* s2) {
    return ut_str.r_str(s1, s2) == s1 + ut_str.len(s1) - ut_str.len(s2);
}

static bool ut_str_i_starts(const char* s1, const char* s2) {
    return ut_str.i_str(s1, s2) == s1;
}

static bool ut_str_i_ends(const char* s1, const char* s2) {
    return ut_str.i_r_str(s1, s2) == s1 + ut_str.len(s1) - ut_str.len(s2);
}

static int32_t ut_str_utf8_bytes(const uint16_t* utf16) {
    // If cchWideChar argument is -1, the function WideCharToMultiByte
    // includes the zero-terminating character in the conversion and
    // the returned byte count.
    int32_t required_bytes_count =
        WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS,
        utf16, -1, null, 0, null, null);
    swear(required_bytes_count < 0 || required_bytes_count > 0);
    return required_bytes_count;
}

static int32_t ut_str_utf16_chars(const char* utf8) {
    // If cbMultiByte argument is -1, the function MultiByteToWideChar
    // includes the zero-terminating character in the conversion and
    // the returned byte count.
    int32_t required_wide_chars_count =
        MultiByteToWideChar(CP_UTF8, 0, utf8, -1, null, 0);
    swear(required_wide_chars_count < 0 || required_wide_chars_count > 0);
    return required_wide_chars_count;
}

static void ut_str_utf16to8(char* d, int32_t capacity, const uint16_t* utf16) {
    const int32_t required = ut_str.utf8_bytes(utf16);
    swear(required > 0 && capacity >= required);
    int32_t bytes = WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS,
                        utf16, -1, d, capacity, null, null);
    swear(required == bytes);
}

static void ut_str_utf8to16(uint16_t* d, int32_t capacity, const char* utf8) {
    const int32_t required = ut_str.utf16_chars(utf8);
    swear(required > 0 && capacity >= required);
    int32_t count = MultiByteToWideChar(CP_UTF8, 0, utf8, -1, d, capacity);
    swear(required == count);
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
    static thread_local char text[1024];
    const DWORD format = FORMAT_MESSAGE_FROM_SYSTEM|
        FORMAT_MESSAGE_IGNORE_INSERTS;
    uint16_t s[256];
    HRESULT hr = 0 <= error && error <= 0xFFFF ?
        HRESULT_FROM_WIN32((uint32_t)error) : (HRESULT)error;
    if (FormatMessageW(format, null, (DWORD)hr, language, s, countof(s) - 1,
            (va_list*)null) > 0) {
        s[countof(s) - 1] = 0;
        // remove trailing '\r\n'
        int32_t k = (int32_t)ut_str.utf16len(s);
        if (k > 0 && s[k - 1] == '\n') { s[k - 1] = 0; }
        k = (int32_t)ut_str.utf16len(s);
        if (k > 0 && s[k - 1] == '\r') { s[k - 1] = 0; }
        char message[512];
        fatal_if(k >= countof(message), "error message too long");
        ut_str.utf16to8(message, countof(message), s);
        ut_str_printf(text, "0x%08X(%d) \"%s\"", error, error, message);
    } else {
        ut_str_printf(text, "0x%08X(%d)", error, error);
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
    // sprintf format %`lld may not be implemented or
    // does not respect locale or UI separators...
    // Do it hard way:
    const int32_t m = (int32_t)strlen(gs);
    swear(m < 5); // utf-8 4 bytes max
    // 64 calls per thread 32 or less bytes each because:
    // "18446744073709551615" 21 characters + 6x4 groups:
    // "18'446'744'073'709'551'615" 27 characters
    enum { max_text_bytes = 64 };
    ut_thread_local_str_pool(text, count, 64, max_text_bytes); // 4KB
    int64_t abs64 = v < 0 ? -v : v; // incorrect for INT64_MIN
    uint64_t n = uint ? (uint64_t)v :
        (v != INT64_MIN ? (uint64_t)abs64 : (uint64_t)INT64_MIN);
    int32_t i = 0;
    int32_t groups[8]; // 2^63 - 1 ~= 9 x 10^19 upto 7 groups of 3 digits
    do {
        groups[i] = n % 1000;
        n = n / 1000;
        i++;
    } while (n > 0);
    const int32_t gc = i - 1; // group count
    char* s = text;
    if (v < 0 && !uint) { *s++ = '-'; } // sign
    int32_t r = max_text_bytes - 1;
    while (i > 0) {
        i--;
        assert(r > 3 + m);
        if (i == gc) {
            ut_str.format(s, r, "%d%s", groups[i], gc > 0 ? gs : "");
        } else {
            ut_str.format(s, r, "%03d%s", groups[i], i > 0 ? gs : "");
        }
        int32_t k = (int32_t)strlen(s);
        r -= k;
        s += k;
    }
    *s = 0;
    return text;
}

#pragma push_macro("ut_glyph_thin_space")
#pragma push_macro("ut_glyph_mmsp")
#pragma push_macro("ut_glyph_three_per_em")
#pragma push_macro("ut_glyph_six_per_em")
#pragma push_macro("ut_glyph_punctuation")
#pragma push_macro("ut_glyph_hair_space")

// Thin Space U+2009
#define ut_glyph_thin_space     "\xE2\x80\x89"

// Medium Mathematical Space (MMSP) U+205F
#define ut_glyph_mmsp           "\xE2\x81\x9F"

// Three-Per-Em Space U+2004
#define ut_glyph_three_per_em   "\xE2\x80\x84"

// Six-Per-Em Space U+2006
#define ut_glyph_six_per_em     "\xE2\x80\x86"

// Punctuation Space U+2008
#define ut_glyph_punctuation    "\xE2\x80\x88"

// Hair Space U+200A
#define ut_glyph_hair_space     "\xE2\x80\x8A" // winner all other too wide

static const char* ut_str_int64(int64_t v) {
    return ut_str_int64_dg(v, false, ut_glyph_hair_space);
}

const char* ut_str_uint64(uint64_t v) {
    return ut_str_int64_dg(v, true, ut_glyph_hair_space);
}

#pragma pop_macro("ut_glyph_thin_space")
#pragma pop_macro("ut_glyph_mmsp")
#pragma pop_macro("ut_glyph_three_per_em")
#pragma pop_macro("ut_glyph_six_per_em")
#pragma pop_macro("ut_glyph_punctuation")
#pragma pop_macro("ut_glyph_hair_space")

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
    swear(ut_str.len("hello") == 5);
    swear(ut_str.equ("hello", "hello"));
    swear(!ut_str.equ("hello", "world"));
    swear(ut_str.i_cmp("hello", "HeLLo") == 0);
    char concat[32];
    ut_str_printf(concat, "%s", "hello_");
    ut_str.cat(concat, countof(concat), "_world");
    swear(ut_str.equ(concat, "hello__world"));
    swear(ut_str.equ("hello", "hello"));
    swear(!ut_str.equ("hello", "world"));
    swear(ut_str.i_equ("hello", "HeLlO"));
    swear(ut_str.starts("hello world", "hello"));
    swear(ut_str.ends("hello world", "world"));
    swear(ut_str.i_starts("hello world", "HeLlO"));
    swear(ut_str.i_ends("hello world", "WoRlD"));
    char text[10] = {0};
    ut_str.cpy(text, countof(text), "hello");
    swear(ut_str.equ(text, "hello"));
    char ls[20] = {0};
    ut_str.lower(ls, countof(ls), "HeLlO WoRlD");
    swear(ut_str.equ(ls, "hello world"));
    #pragma push_macro("glyph_chinese_one")
    #pragma push_macro("glyph_chinese_two")
    #pragma push_macro("glyph_teddy_bear")
    #pragma push_macro("glyph_ice_cube")
    #define glyph_chinese_one "\xE5\xA3\xB9"
    #define glyph_chinese_two "\xE8\xB4\xB0"
    #define glyph_teddy_bear  "\xF0\x9F\xA7\xB8"
    #define glyph_ice_cube    "\xF0\x9F\xA7\x8A"
    const char* utf8_str =
            glyph_teddy_bear
            "0"
            glyph_chinese_one glyph_chinese_two
            "3456789 "
            glyph_ice_cube;
    #pragma pop_macro("glyph_ice_cube")
    #pragma pop_macro("glyph_teddy_bear")
    #pragma pop_macro("glyph_chinese_two")
    #pragma pop_macro("glyph_chinese_one")
    uint16_t wide_str[100] = {0};
    ut_str.utf8to16(wide_str, countof(wide_str), utf8_str);
    char utf8[100] = {0};
    ut_str.utf16to8(utf8, countof(utf8), wide_str);
    uint16_t utf16[100];
    ut_str.utf8to16(utf16, countof(utf16), utf8);
    char narrow_str[100] = {0};
    ut_str.utf16to8(narrow_str, countof(narrow_str), utf16);
    swear(ut_str.equ(narrow_str, utf8_str));
    char formatted[100];
    ut_str.format(formatted, countof(formatted), "n: %d, s: %s", 42, "test");
    swear(ut_str.equ(formatted, "n: 42, s: test"));
    char copy[11] = {0};
    copy[10] = 0xFF;
    ut_str.cpy(copy, countof(copy), "0123456789");
    swear(copy[10] == 0);
    char lower[11] = {0};
    lower[10] = 0xFF;
    ut_str.lower(lower, countof(lower), "HELLO12345");
    swear(lower[10] == 0);
    swear(ut_str.equ(lower, "hello12345"));
    // numeric values digit grouping format:
    swear(ut_str.equ("0", ut_str.int64_dg(0, true, ",")));
    swear(ut_str.equ("-1", ut_str.int64_dg(-1, false, ",")));
    swear(ut_str.equ("999", ut_str.int64_dg(999, true, ",")));
    swear(ut_str.equ("-999", ut_str.int64_dg(-999, false, ",")));
    swear(ut_str.equ("1,001", ut_str.int64_dg(1001, true, ",")));
    swear(ut_str.equ("-1,001", ut_str.int64_dg(-1001, false, ",")));
    swear(ut_str.equ("18,446,744,073,709,551,615",
        ut_str.int64_dg(UINT64_MAX, true, ",")
    ));
    swear(ut_str.equ("9,223,372,036,854,775,807",
        ut_str.int64_dg(INT64_MAX, false, ",")
    ));
    swear(ut_str.equ("-9,223,372,036,854,775,808",
        ut_str.int64_dg(INT64_MIN, false, ",")
    ));
    //  see:
    // https://en.wikipedia.org/wiki/Single-precision_floating-point_format
    uint32_t pi_fp32 = 0x40490FDBULL; // 3.14159274101257324
    swear(ut_str.equ("3.141592741",
                ut_str.fp("%.9f", *(fp32_t*)&pi_fp32)),
          "%s", ut_str.fp("%.9f", *(fp32_t*)&pi_fp32)
    );
    //  3.141592741
    //  ********^ (*** true digits ^ first rounded digit)
    //    123456 (%.6f)
    //
    //  https://en.wikipedia.org/wiki/Double-precision_floating-point_format
    uint64_t pi_fp64 = 0x400921FB54442D18ULL;
    swear(ut_str.equ("3.141592653589793116",
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
}

#else

static void ut_str_test(void) {}

#endif

ut_str_if ut_str = {
    .drop_const         = ut_str_drop_const,
    .len                = ut_str_len,
    .utf16len           = ut_str_utf16len,
    .cmp                = ut_str_cmp,
    .i_cmp              = ut_str_i_cmp,
    .equ                = ut_str_equ,
    .i_equ              = ut_str_i_equ,
    .chr                = ut_str_chr,
    .r_chr              = ut_str_r_chr,
    .str                = ut_str_str,
    .i_str              = ut_str_i_str,
    .r_str              = ut_str_r_str,
    .i_r_str            = ut_str_i_r_str,
    .cpy                = ut_str_cpy,
    .cat                = ut_str_cat,
    .lower              = ut_str_lower,
    .upper              = ut_str_upper,
    .starts             = ut_str_starts,
    .ends               = ut_str_ends,
    .i_starts           = ut_str_i_starts,
    .i_ends             = ut_str_i_ends,
    .utf8_bytes         = ut_str_utf8_bytes,
    .utf16_chars        = ut_str_utf16_chars,
    .utf16to8           = ut_str_utf16to8,
    .utf8to16           = ut_str_utf8to16,
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
