#pragma once
#include "ut/ut_std.h"

ut_begin_c

typedef struct str64_t {
    char s[64];
} str64_t;

typedef struct str128_t {
    char s[128];
} str128_t;

typedef struct str1024_t {
    char s[1024];
} str1024_t;

typedef struct str32K_t {
    char s[32 * 1024];
} str32K_t;

// truncating string printf:
// char s[100]; ut_str_printf(s, "Hello %s", "world");
// do not use with char* and char s[] parameters
// because ut_countof(s) will be sizeof(char*) not the size of the buffer.

#define ut_str_printf(s, ...) ut_str.format((s), ut_countof(s), "" __VA_ARGS__)

#define ut_strerr(r) (ut_str.error((r)).s) // use only as ut_str_printf() parameter

// The strings are expected to be UTF-8 encoded.
// Copy functions fatal fail if the destination buffer is too small.
// It is responsibility of the caller to make sure it won't happen.

typedef struct {
    char* (*drop_const)(const char* s); // because of strstr() and alike
    int32_t (*len)(const char* s);
    int32_t (*len16)(const uint16_t* utf16);
    int32_t (*utf8bytes)(const char* utf8, int32_t bytes); // 0 on error
    int32_t (*glyphs)(const char* utf8, int32_t bytes); // -1 on error
    bool (*starts)(const char* s1, const char* s2);  // s1 starts with s2
    bool (*ends)(const char* s1, const char* s2);    // s1 ends with s2
    bool (*istarts)(const char* s1, const char* s2); // ignore case
    bool (*iends)(const char* s1, const char* s2);   // ignore case
    // string truncation is fatal use strlen() to check at call site
    void (*lower)(char* d, int32_t capacity, const char* s); // ASCII only
    void (*upper)(char* d, int32_t capacity, const char* s); // ASCII only
    // utf8/utf16 conversion
    // If `chars` argument is -1, the function utf8_bytes includes the zero
    // terminating character in the conversion and the returned byte count.
    int32_t (*utf8_bytes)(const uint16_t* utf16, int32_t bytes); // bytes count
    // If `bytes` argument is -1, the function utf16_chars() includes the zero
    // terminating character in the conversion and the returned character count.
    int32_t (*utf16_chars)(const char* utf8, int32_t bytes); // chars count
    // utf8_bytes() and utf16_chars() return -1 on invalid UTF-8/UTF-16
    // utf8_bytes(L"", -1) returns 1 for zero termination
    // utf16_chars("", -1) returns 1 for zero termination
    // chars: -1 means both source and destination are zero terminated
    errno_t (*utf16to8)(char* utf8, int32_t capacity,
                        const uint16_t* utf16, int32_t chars);
    // bytes: -1 means both source and destination are zero terminated
    errno_t (*utf8to16)(uint16_t* utf16, int32_t capacity,
                        const char* utf8, int32_t bytes);
    // https://compart.com/en/unicode/U+1F41E
    // Lady Beetle: utf16 L"\xD83D\xDC1E" utf8 "\xF0\x9F\x90\x9E"
    //           surrogates:  high   low
    bool (*utf16_is_low_surrogate)(uint16_t utf16char);
    bool (*utf16_is_high_surrogate)(uint16_t utf16char);
    uint32_t (*utf32)(const char* utf8, int32_t bytes); // single codepoint
    // string formatting printf style:
    void (*format_va)(char* utf8, int32_t count, const char* format, va_list va);
    void (*format)(char* utf8, int32_t count, const char* format, ...);
    // format "dg" digit grouped; see below for known grouping separators:
    const char* (*grouping_separator)(void); // locale
    // Returned const char* pointer is short-living thread local and
    // intended to be used in the arguments list of .format() or .printf()
    // like functions, not stored or passed for prolonged call chains.
    // See implementation for details.
    str64_t (*int64_dg)(int64_t v, bool uint, const char* gs);
    str64_t (*int64)(int64_t v);      // with UTF-8 thin space
    str64_t (*uint64)(uint64_t v);    // with UTF-8 thin space
    str64_t (*int64_lc)(int64_t v);   // with locale separator
    str64_t (*uint64_lc)(uint64_t v); // with locale separator
    str128_t (*fp)(const char* format, fp64_t v); // respects locale
    // errors to strings
    str1024_t (*error)(int32_t error);     // en-US
    str1024_t (*error_nls)(int32_t error); // national locale string
    void (*test)(void);
} ut_str_if;

// Known grouping separators
// https://en.wikipedia.org/wiki/Decimal_separator#Digit_grouping
// coma "," separated decimal
// other commonly used separators:
// underscore "_" (Fortran, Kotlin)
// apostrophe "'" (C++14, Rebol, Red)
// backtick "`"
// space "\x20"
// thin_space "\xE2\x80\x89" Unicode: U+2009

extern ut_str_if ut_str;

ut_end_c
