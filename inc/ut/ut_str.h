#pragma once
#include "ut/ut_std.h"

begin_c

#define ut_str_printf(s, ...) ut_str.format((s), countof(s), "" __VA_ARGS__)

// The strings are expected to be UTF-8 encoded.
// Copy functions fatal fail if the destination buffer is too small.
// It is responsibility of the caller to make sure it won't happen.

typedef struct {
    char* (*drop_const)(const char* s); // because of strstr() and alike
    int32_t (*len)(const char* s);
    int32_t (*utf16len)(const uint16_t* ws);
    // string truncation is fatal use strlen() to ensure memory at call site
    void  (*lower)(char* d, int32_t capacity, const char* s); // ASCII only
    void  (*upper)(char* d, int32_t capacity, const char* s); // ASCII only
    bool  (*starts)(const char* s1, const char* s2); // s1 starts with s2
    bool  (*ends)(const char* s1, const char* s2);   // s1 ends with s2
    bool  (*i_starts)(const char* s1, const char* s2); // ignore case
    bool  (*i_ends)(const char* s1, const char* s2);   // ignore case
    // utf8/utf16 conversion
    int32_t (*utf8_bytes)(const uint16_t* utf16); // UTF-8 byte required
    int32_t (*utf16_chars)(const char* s); // UTF-16 chars required
    // utf8_bytes() and utf16_chars() return -1 on invalid UTF-8/UTF-16
    // utf8_bytes(L"") returns 1 for zero termination
    // utf16_chars("") returns 1 for zero termination
    void (*utf16to8)(char* d, int32_t capacity, const uint16_t* utf16);
    void (*utf8to16)(uint16_t* d, int32_t capacity, const char* utf8);
    // string formatting printf style:
    void (*format_va)(char* utf8, int32_t count, const char* format, va_list vl);
    void (*format)(char* utf8, int32_t count, const char* format, ...);
    // format "dg" digit grouped; see below for known grouping separators:
    const char* (*grouping_separator)(void); // locale
    // Returned const char* pointer is short-living thread local and
    // intended to be used in the arguments list of .format() or .printf()
    // like functions, not stored or passed for prolonged call chains.
    // See implementation for details.
    const char* (*int64_dg)(int64_t v, bool uint, const char* gs);
    const char* (*int64)(int64_t v);   // with UTF-8 thin space
    const char* (*uint64)(uint64_t v); // with UTF-8 thin space
    const char* (*int64_lc)(int64_t v);   // with locale separator
    const char* (*uint64_lc)(uint64_t v); // with locale separator
    const char* (*fp)(const char* format, fp64_t v); // respects locale
    // errors to strings (return thread local)
    const char* (*error)(int32_t error);     // en-US
    const char* (*error_nls)(int32_t error); // national locale string
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

end_c

