#include "rt/rt.h"
#include "rt/rt_win32.h"

static char* rt_str_drop_const(const char* s) {
    #if defined(__GNUC__) || defined(__clang__)
    #pragma GCC diagnostic push
    #pragma GCC diagnostic ignored "-Wcast-qual"
    #endif
    return (char*)s;
    #if defined(__GNUC__) || defined(__clang__)
    #pragma GCC diagnostic pop
    #endif
}

static int32_t rt_str_len(const char* s) { return (int32_t)strlen(s); }

static int32_t rt_str_utf16len(const uint16_t* utf16) {
    return (int32_t)wcslen(utf16);
}

static int32_t rt_str_utf8bytes(const char* s, int32_t b) {
    rt_assert(b >= 1, "should not be called with bytes < 1");
    const uint8_t* const u = (const uint8_t*)s;
    // based on:
    // https://stackoverflow.com/questions/66715611/check-for-valid-utf-8-encoding-in-c
    if (b >= 1 && (u[0] & 0x80u) == 0x00u) {
        return 1;
    } else if (b > 1) {
        uint32_t c = (u[0] << 8) | u[1];
        // TODO: 0xC080 is a hack - consider removing
        if (c == 0xC080) { return 2; } // 0xC080 as not zero terminating '\0'
        if (0xC280 <= c && c <= 0xDFBF && (c & 0xE0C0) == 0xC080) { return 2; }
        if (b > 2) {
            c = (c << 8) | u[2];
            // reject utf16 surrogates:
            if (0xEDA080 <= c && c <= 0xEDBFBF) { return 0; }
            if (0xE0A080 <= c && c <= 0xEFBFBF && (c & 0xF0C0C0) == 0xE08080) {
                return 3;
            }
            if (b > 3) {
                c = (c << 8) | u[3];
                if (0xF0908080 <= c && c <= 0xF48FBFBF &&
                    (c & 0xF8C0C0C0) == 0xF0808080) {
                    return 4;
                }
            }
        }
    }
    return 0; // invalid utf8 sequence
}

static int32_t rt_str_glyphs(const char* utf8, int32_t bytes) {
    rt_swear(bytes >= 0);
    bool ok = true;
    int32_t i = 0;
    int32_t k = 1;
    while (i < bytes && ok) {
        const int32_t b = rt_str.utf8bytes(utf8 + i, bytes - i);
        ok = 0 < b && i + b <= bytes;
        if (ok) { i += b; k++; }
    }
    return ok ? k - 1 : -1;
}

static void rt_str_lower(char* d, int32_t capacity, const char* s) {
    int32_t n = rt_str.len(s);
    rt_swear(capacity > n);
    for (int32_t i = 0; i < n; i++) { d[i] = (char)tolower(s[i]); }
    d[n] = 0;
}

static void rt_str_upper(char* d, int32_t capacity, const char* s) {
    int32_t n = rt_str.len(s);
    rt_swear(capacity > n);
    for (int32_t i = 0; i < n; i++) { d[i] = (char)toupper(s[i]); }
    d[n] = 0;
}

static bool rt_str_starts(const char* s1, const char* s2) {
    int32_t n1 = (int32_t)strlen(s1);
    int32_t n2 = (int32_t)strlen(s2);
    return n1 >= n2 && memcmp(s1, s2, n2) == 0;
}

static bool rt_str_ends(const char* s1, const char* s2) {
    int32_t n1 = (int32_t)strlen(s1);
    int32_t n2 = (int32_t)strlen(s2);
    return n1 >= n2 && memcmp(s1 + n1 - n2, s2, n2) == 0;
}

static bool rt_str_i_starts(const char* s1, const char* s2) {
    int32_t n1 = (int32_t)strlen(s1);
    int32_t n2 = (int32_t)strlen(s2);
    return n1 >= n2 && strnicmp(s1, s2, n2) == 0;

}

static bool rt_str_i_ends(const char* s1, const char* s2) {
    int32_t n1 = (int32_t)strlen(s1);
    int32_t n2 = (int32_t)strlen(s2);
    return n1 >= n2 && strnicmp(s1 + n1 - n2, s2, n2) == 0;
}

static int32_t rt_str_utf8_bytes(const uint16_t* utf16, int32_t chars) {
    // If `chars` argument is -1, the function utf8_bytes includes the zero
    // terminating character in the conversion and the returned byte count.
    // Function will fail (return 0) on incomplete surrogate pairs like
    // 0xD83D without following 0xDC1E https://compart.com/en/unicode/U+1F41E
    if (chars == 0) { return 0; }
    if (chars < 0 && utf16[0] == 0x0000) { return 1; }
    const int32_t required_bytes_count =
        WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS,
        utf16, chars, null, 0, null, null);
    if (required_bytes_count == 0) {
        errno_t r = rt_core.err();
        rt_println("WideCharToMultiByte() failed %s", rt_strerr(r));
        rt_core.set_err(r);
    }
    return required_bytes_count == 0 ? -1 : required_bytes_count;
}

static int32_t rt_str_utf16_chars(const char* utf8, int32_t bytes) {
    // If `bytes` argument is -1, the function utf16_chars() includes the zero
    // terminating character in the conversion and the returned character count.
    if (bytes == 0) { return 0; }
    if (bytes < 0 && utf8[0] == 0x00) { return 1; }
    const int32_t required_wide_chars_count =
        MultiByteToWideChar(CP_UTF8, 0, utf8, bytes, null, 0);
    if (required_wide_chars_count == 0) {
        errno_t r = rt_core.err();
        rt_println("MultiByteToWideChar() failed %s", rt_strerr(r));
        rt_core.set_err(r);
    }
    return required_wide_chars_count == 0 ? -1 : required_wide_chars_count;
}

static errno_t rt_str_utf16to8(char* utf8, int32_t capacity,
        const uint16_t* utf16, int32_t chars) {
    if (chars == 0) { return 0; }
    if (chars < 0 && utf16[0] == 0x0000) {
        rt_swear(capacity >= 1);
        utf8[0] = 0x00;
        return 0;
    }
    const int32_t required = rt_str.utf8_bytes(utf16, chars);
    errno_t r = required < 0 ? rt_core.err() : 0;
    if (r == 0) {
        rt_swear(required > 0 && capacity >= required);
        int32_t bytes = WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS,
                            utf16, chars, utf8, capacity, null, null);
        rt_swear(required == bytes);
    }
    return r;
}

static errno_t rt_str_utf8to16(uint16_t* utf16, int32_t capacity,
        const char* utf8, int32_t bytes) {
    const int32_t required = rt_str.utf16_chars(utf8, bytes);
    errno_t r = required < 0 ? rt_core.err() : 0;
    if (r == 0) {
        rt_swear(required >= 0 && capacity >= required);
        int32_t count = MultiByteToWideChar(CP_UTF8, 0, utf8, bytes,
                                            utf16, capacity);
        rt_swear(required == count);
#if 0 // TODO: incorrect need output != input
        if (count > 0 && !IsNormalizedString(NormalizationC, utf16, count)) {
            rt_core.set_err(0);
            int32_t n = NormalizeString(NormalizationC, utf16, count, utf16, count);
            if (n <= 0) {
                r = rt_core.err();
                rt_println("NormalizeString() failed %s", rt_strerr(r));
            }
        }
#endif 
    }
    return r;
}

static bool rt_str_utf16_is_low_surrogate(uint16_t utf16char) {
    return 0xDC00 <= utf16char && utf16char <= 0xDFFF;
}

static bool rt_str_utf16_is_high_surrogate(uint16_t utf16char) {
    return 0xD800 <= utf16char && utf16char <= 0xDBFF;
}

static uint32_t rt_str_utf32(const char* utf8, int32_t bytes) {
    uint32_t utf32 = 0;
    if ((utf8[0] & 0x80) == 0) {
        utf32 = utf8[0];
        rt_swear(bytes == 1);
    } else if ((utf8[0] & 0xE0) == 0xC0) {
        utf32  = (utf8[0] & 0x1F) << 6;
        utf32 |= (utf8[1] & 0x3F);
        rt_swear(bytes == 2);
    } else if ((utf8[0] & 0xF0) == 0xE0) {
        utf32  = (utf8[0] & 0x0F) << 12;
        utf32 |= (utf8[1] & 0x3F) <<  6;
        utf32 |= (utf8[2] & 0x3F);
        rt_swear(bytes == 3);
    } else if ((utf8[0] & 0xF8) == 0xF0) {
        utf32  = (utf8[0] & 0x07) << 18;
        utf32 |= (utf8[1] & 0x3F) << 12;
        utf32 |= (utf8[2] & 0x3F) <<  6;
        utf32 |= (utf8[3] & 0x3F);
        rt_swear(bytes == 4);
    } else {
        rt_swear(false);
    }
    return utf32;
}

static void rt_str_format_va(char* utf8, int32_t count, const char* format,
        va_list va) {
    #if defined(__GNUC__) || defined(__clang__)
    #pragma GCC diagnostic push
    #pragma GCC diagnostic ignored "-Wformat-nonliteral"
    #endif
    vsnprintf(utf8, (size_t)count, format, va);
    utf8[count - 1] = 0;
    #if defined(__GNUC__) || defined(__clang__)
    #pragma GCC diagnostic pop
    #endif
}

static void rt_str_format(char* utf8, int32_t count, const char* format, ...) {
    va_list va;
    va_start(va, format);
    rt_str.format_va(utf8, count, format, va);
    va_end(va);
}

static rt_str1024_t rt_str_error_for_language(int32_t error, LANGID language) {
    DWORD flags = FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS;
    HMODULE module = null;
    HRESULT hr = 0 <= error && error <= 0xFFFF ?
        HRESULT_FROM_WIN32((uint32_t)error) : (HRESULT)error;
    if ((error & 0xC0000000U) == 0xC0000000U) {
        // https://stackoverflow.com/questions/25566234/how-to-convert-specific-ntstatus-value-to-the-hresult
        static HMODULE ntdll; // RtlNtStatusToDosError implies linking to ntdll
        if (ntdll == null) { ntdll = GetModuleHandleA("ntdll.dll"); }
        if (ntdll == null) { ntdll = LoadLibraryA("ntdll.dll"); }
        module = ntdll;
        hr = HRESULT_FROM_WIN32(RtlNtStatusToDosError((NTSTATUS)error));
        flags |= FORMAT_MESSAGE_FROM_HMODULE;
    }
    rt_str1024_t text;
    uint16_t utf16[rt_countof(text.s)];
    DWORD count = FormatMessageW(flags, module, hr, language,
            utf16, rt_countof(utf16) - 1, (va_list*)null);
    utf16[rt_countof(utf16) - 1] = 0; // always
    // If FormatMessageW() succeeds, the return value is the number of utf16
    // characters stored in the output buffer, excluding the terminating zero.
    if (count > 0) {
        rt_swear(count < rt_countof(utf16));
        utf16[count] = 0;
        // remove trailing '\r\n'
        int32_t k = count;
        if (k > 0 && utf16[k - 1] == '\n') { utf16[k - 1] = 0; }
        k = (int32_t)rt_str.len16(utf16);
        if (k > 0 && utf16[k - 1] == '\r') { utf16[k - 1] = 0; }
        char message[rt_countof(text.s)];
        const int32_t bytes = rt_str.utf8_bytes(utf16, -1);
        if (bytes >= rt_countof(message)) {
            rt_str_printf(message, "error message is too long: %d bytes", bytes);
        } else {
            rt_str.utf16to8(message, rt_countof(message), utf16, -1);
        }
        // truncating printf to string:
        rt_str_printf(text.s, "0x%08X(%d) \"%s\"", error, error, message);
    } else {
        rt_str_printf(text.s, "0x%08X(%d)", error, error);
    }
    return text;
}

static rt_str1024_t rt_str_error(int32_t error) {
    const LANGID language = MAKELANGID(LANG_ENGLISH, SUBLANG_DEFAULT);
    return rt_str_error_for_language(error, language);
}

static rt_str1024_t rt_str_error_nls(int32_t error) {
    const LANGID language = MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT);
    return rt_str_error_for_language(error, language);
}

static const char* rt_str_grouping_separator(void) {
    #ifdef WINDOWS
        // en-US Windows 10/11:
        // grouping_separator == ","
        // decimal_separator  == "."
        static char grouping_separator[8];
        if (grouping_separator[0] == 0x00) {
            errno_t r = rt_b2e(GetLocaleInfoA(LOCALE_USER_DEFAULT, LOCALE_STHOUSAND,
                grouping_separator, sizeof(grouping_separator)));
            rt_swear(r == 0 && grouping_separator[0] != 0);
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
//   rt_b2e(GetLocaleInfo(LOCALE_USER_DEFAULT, LOCALE_STHOUSAND,
//       grouping_separator, sizeof(grouping_separator)));
//   rt_b2e(GetLocaleInfo(LOCALE_USER_DEFAULT, LOCALE_SDECIMAL,
//       decimal_separator, sizeof(decimal_separator)));
// en-US Windows 1x:
// grouping_separator == ","
// decimal_separator  == "."

static rt_str64_t rt_str_int64_dg(int64_t v, // digit_grouped
        bool uint, const char* gs) { // grouping separator: gs
    // sprintf format %`lld may not be implemented or
    // does not respect locale or UI separators...
    // Do it hard way:
    const int32_t m = (int32_t)strlen(gs);
    rt_swear(m < 5); // utf-8 4 bytes max
    // 64 calls per thread 32 or less bytes each because:
    // "18446744073709551615" 21 characters + 6x4 groups:
    // "18'446'744'073'709'551'615" 27 characters
    rt_str64_t text;
    enum { max_text_bytes = rt_countof(text.s) };
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
    char* s = text.s;
    if (v < 0 && !uint) { *s++ = '-'; } // sign
    int32_t r = max_text_bytes - 1;
    while (i > 0) {
        i--;
        rt_assert(r > 3 + m);
        if (i == gc) {
            rt_str.format(s, r, "%d%s", groups[i], gc > 0 ? gs : "");
        } else {
            rt_str.format(s, r, "%03d%s", groups[i], i > 0 ? gs : "");
        }
        int32_t k = (int32_t)strlen(s);
        r -= k;
        s += k;
    }
    *s = 0;
    return text;
}

static rt_str64_t rt_str_int64(int64_t v) {
    return rt_str_int64_dg(v, false, rt_glyph_hair_space);
}

static rt_str64_t rt_str_uint64(uint64_t v) {
    return rt_str_int64_dg(v, true, rt_glyph_hair_space);
}

static rt_str64_t rt_str_int64_lc(int64_t v) {
    return rt_str_int64_dg(v, false, rt_str_grouping_separator());
}

static rt_str64_t rt_str_uint64_lc(uint64_t v) {
    return rt_str_int64_dg(v, true, rt_str_grouping_separator());
}

static rt_str128_t rt_str_fp(const char* format, fp64_t v) {
    static char decimal_separator[8];
    if (decimal_separator[0] == 0) {
        errno_t r = rt_b2e(GetLocaleInfoA(LOCALE_USER_DEFAULT, LOCALE_SDECIMAL,
            decimal_separator, sizeof(decimal_separator)));
        rt_swear(r == 0 && decimal_separator[0] != 0);
    }
    rt_swear(strlen(decimal_separator) <= 4);
    rt_str128_t f; // formatted float point
    // snprintf format does not handle thousands separators on all know runtimes
    // and respects setlocale() on Un*x systems but in MS runtime only when
    // _snprintf_l() is used.
    f.s[0] = 0x00;
    rt_str.format(f.s, rt_countof(f.s), format, v);
    f.s[rt_countof(f.s) - 1] = 0x00;
    rt_str128_t text;
    char* s = f.s;
    char* d = text.s;
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

#ifdef RT_TESTS

static void rt_str_test(void) {
    rt_swear(rt_str.len("hello") == 5);
    rt_swear(rt_str.starts("hello world", "hello"));
    rt_swear(rt_str.ends("hello world", "world"));
    rt_swear(rt_str.istarts("hello world", "HeLlO"));
    rt_swear(rt_str.iends("hello world", "WoRlD"));
    char ls[20] = {0};
    rt_str.lower(ls, rt_countof(ls), "HeLlO WoRlD");
    rt_swear(strcmp(ls, "hello world") == 0);
    char upper[11] = {0};
    rt_str.upper(upper, rt_countof(upper), "hello12345");
    rt_swear(strcmp(upper,  "HELLO12345") == 0);
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
            rt_glyph_chinese_jin4 rt_glyph_chinese_gong
            "3456789 "
            glyph_ice_cube;
    rt_swear(rt_str.utf8bytes("\x01", 1) == 1);
    rt_swear(rt_str.utf8bytes("\x7F", 1) == 1);
    rt_swear(rt_str.utf8bytes("\x80", 1) == 0);
//  swear(rt_str.utf8bytes(glyph_chinese_one, 0) == 0);
    rt_swear(rt_str.utf8bytes(glyph_chinese_one, 1) == 0);
    rt_swear(rt_str.utf8bytes(glyph_chinese_one, 2) == 0);
    rt_swear(rt_str.utf8bytes(glyph_chinese_one, 3) == 3);
    rt_swear(rt_str.utf8bytes(glyph_teddy_bear,  4) == 4);
    #pragma pop_macro("glyph_ice_cube")
    #pragma pop_macro("glyph_teddy_bear")
    #pragma pop_macro("glyph_chinese_two")
    #pragma pop_macro("glyph_chinese_one")
    uint16_t wide_str[100] = {0};
    rt_str.utf8to16(wide_str, rt_countof(wide_str), utf8_str, -1);
    char utf8[100] = {0};
    rt_str.utf16to8(utf8, rt_countof(utf8), wide_str, -1);
    uint16_t utf16[100];
    rt_str.utf8to16(utf16, rt_countof(utf16), utf8, -1);
    char narrow_str[100] = {0};
    rt_str.utf16to8(narrow_str, rt_countof(narrow_str), utf16, -1);
    rt_swear(strcmp(narrow_str, utf8_str) == 0);
    char formatted[100];
    rt_str.format(formatted, rt_countof(formatted), "n: %d, s: %s", 42, "test");
    rt_swear(strcmp(formatted, "n: 42, s: test") == 0);
    // numeric values digit grouping format:
    rt_swear(strcmp("0", rt_str.int64_dg(0, true, ",").s) == 0);
    rt_swear(strcmp("-1", rt_str.int64_dg(-1, false, ",").s) == 0);
    rt_swear(strcmp("999", rt_str.int64_dg(999, true, ",").s) == 0);
    rt_swear(strcmp("-999", rt_str.int64_dg(-999, false, ",").s) == 0);
    rt_swear(strcmp("1,001", rt_str.int64_dg(1001, true, ",").s) == 0);
    rt_swear(strcmp("-1,001", rt_str.int64_dg(-1001, false, ",").s) == 0);
    rt_swear(strcmp("18,446,744,073,709,551,615",
        rt_str.int64_dg(UINT64_MAX, true, ",").s) == 0
    );
    rt_swear(strcmp("9,223,372,036,854,775,807",
        rt_str.int64_dg(INT64_MAX, false, ",").s) == 0
    );
    rt_swear(strcmp("-9,223,372,036,854,775,808",
        rt_str.int64_dg(INT64_MIN, false, ",").s) == 0
    );
    //  see:
    // https://en.wikipedia.org/wiki/Single-precision_floating-point_format
    uint32_t pi_fp32 = 0x40490FDBULL; // 3.14159274101257324
    rt_swear(strcmp("3.141592741",
                rt_str.fp("%.9f", *(fp32_t*)&pi_fp32).s) == 0,
          "%s", rt_str.fp("%.9f", *(fp32_t*)&pi_fp32).s
    );
    //  3.141592741
    //  ********^ (*** true digits ^ first rounded digit)
    //    123456 (%.6f)
    //
    //  https://en.wikipedia.org/wiki/Double-precision_floating-point_format
    uint64_t pi_fp64 = 0x400921FB54442D18ULL;
    rt_swear(strcmp("3.141592653589793116",
                rt_str.fp("%.18f", *(fp64_t*)&pi_fp64).s) == 0,
          "%s", rt_str.fp("%.18f", *(fp64_t*)&pi_fp64).s
    );
    //  3.141592653589793116
    //  *****************^ (*** true digits ^ first rounded digit)
    //    123456789012345 (%.15f)
    //  https://en.wikipedia.org/wiki/Double-precision_floating-point_format
    //
    //  actual "pi" first 64 digits:
    //  3.1415926535897932384626433832795028841971693993751058209749445923
    if (rt_debug.verbosity.level > rt_debug.verbosity.quiet) { rt_println("done"); }
}

#else

static void rt_str_test(void) {}

#endif

rt_str_if rt_str = {
    .drop_const              = rt_str_drop_const,
    .len                     = rt_str_len,
    .len16                   = rt_str_utf16len,
    .utf8bytes               = rt_str_utf8bytes,
    .glyphs                  = rt_str_glyphs,
    .lower                   = rt_str_lower,
    .upper                   = rt_str_upper,
    .starts                  = rt_str_starts,
    .ends                    = rt_str_ends,
    .istarts                 = rt_str_i_starts,
    .iends                   = rt_str_i_ends,
    .utf8_bytes              = rt_str_utf8_bytes,
    .utf16_chars             = rt_str_utf16_chars,
    .utf16to8                = rt_str_utf16to8,
    .utf8to16                = rt_str_utf8to16,
    .utf16_is_low_surrogate  = rt_str_utf16_is_low_surrogate,
    .utf16_is_high_surrogate = rt_str_utf16_is_high_surrogate,
    .utf32                   = rt_str_utf32,
    .format                  = rt_str_format,
    .format_va               = rt_str_format_va,
    .error                   = rt_str_error,
    .error_nls               = rt_str_error_nls,
    .grouping_separator      = rt_str_grouping_separator,
    .int64_dg                = rt_str_int64_dg,
    .int64                   = rt_str_int64,
    .uint64                  = rt_str_uint64,
    .int64_lc                = rt_str_int64,
    .uint64_lc               = rt_str_uint64,
    .fp                      = rt_str_fp,
    .test                    = rt_str_test
};
