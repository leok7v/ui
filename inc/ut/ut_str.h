#pragma once
#include "ut/ut_std.h"

begin_c

// Since a lot of str*() operations are preprocessor defines
// care should be exercised that arguments of macro invocations
// do not have side effects or not computationally expensive.
// None of the definitions are performance champions - if the
// code needs extreme cpu cycle savings working with utf8 strings.
//
// str* macros and functions assume zero terminated UTF-8 strings
// in C tradition. Use ut_str.* functions for non-zero terminated
// strings.

#define strempty(s) ((const void*)(s) == null || (s)[0] == 0)

#define strconcat(a, b) _Pragma("warning(suppress: 6386)") \
    (strcat(strcpy((char*)ut_stackalloc(strlen(a) + strlen(b) + 1), (a)), (b)))

#define strequ(s1, s2)                                              \
    ut_gcc_pragma(GCC diagnostic push)                              \
    ut_gcc_pragma(GCC diagnostic ignored "-Wstring-compare")        \
    (((const void*)(s1) == (const void*)(s2)) ||                    \
    (((const void*)(s1) != null && (const void*)(s2) != null) &&    \
    strcmp((s1), (s2)) == 0))                                       \
    ut_gcc_pragma(GCC diagnostic pop)

#define striequ(s1, s2)                                               \
    ut_gcc_pragma(GCC diagnostic push)                                \
    ut_gcc_pragma(GCC diagnostic ignored "-Wstring-compare")          \
    (((const void*)(s1) == (const void*)(s2)) ||                      \
    (((const void*)(s1) != null && (const void*)(s2) != null) &&      \
    stricmp((s1), (s2)) == 0))                                        \
    ut_gcc_pragma(GCC diagnostic pop)

#define strstartswith(a, b) \
    (strlen(a) >= strlen(b) && memcmp((a), (b), strlen(b)) == 0)

#define strendswith(s1, s2) \
    (strlen(s1) >= strlen(s2) && strcmp((s1) + strlen(s1) - strlen(s2), (s2)) == 0)

#define strlength(s) ((int32_t)strlen(s)) // avoid code analysis noise
// a lot of posix like API consumes "int" instead of size_t which
// is acceptable for majority of char* zero terminated strings usage
// since most of them work with filepath that are relatively short
// and on Windows are limited to 260 chars or 32KB - 1 chars.

#define strcopy(s1, s2) /* use with extreme caution */                      \
    do {                                                                    \
        static_assertion(countof(s1) > sizeof(void*));                      \
        strncpy((s1), (s2), countof((s1)) - 1); s1[countof((s1)) - 1] = 0;  \
} while (0)

char* strnchr(const char* s, int32_t n, char ch);

#define strtolowercase(s) \
    ut_str.to_lowercase((char*)ut_stackalloc(strlen(s) + 1), strlen(s) + 1, s)

#define utf16to8(utf16) ut_str.utf16_utf8((char*) \
    ut_stackalloc((size_t)ut_str.utf8_bytes(utf16) + 1), utf16)

#define utf8to16(s) ut_str.utf8_utf16((uint16_t*) \
    ut_stackalloc((size_t)(ut_str.utf16_chars(s) + 1) * sizeof(uint16_t)), s)

#define strprintf(s, ...) ut_str.format((s), countof(s), "" __VA_ARGS__)

#define strncasecmp _strnicmp
#define strcasecmp _stricmp

// ut_str.functions are capable of working with both 0x00 terminated
// strings and UTF-8 strings of fixed length denoted by n1 and n2.
// case insensitive functions with postfix _nc
// operate only on ASCII characters. No ANSI no UTF-8

typedef struct {
    const char* (*error)(int32_t error);     // en-US
    const char* (*error_nls)(int32_t error); // national locale string
    int32_t (*utf8_bytes)(const uint16_t* utf16);
    int32_t (*utf16_chars)(const char* s);
    char* (*utf16_utf8)(char* destination, const uint16_t* utf16);
    uint16_t* (*utf8_utf16)(uint16_t* destination, const char* utf8);
    // string formatting printf style:
    void (*format_va)(char* utf8, int32_t count, const char* format, va_list vl);
    void (*format)(char* utf8, int32_t count, const char* format, ...);
    bool (*is_empty)(const char* s); // null or empty string
    bool (*equal)(const char* s1, int32_t n1, const char* s2, int32_t n2);
    bool (*equal_nc)(const char* s1, int32_t n1, const char* s2, int32_t n2);
    int32_t (*length)(const char* s);
    // copy(s1, countof(s1), s2, /*bytes*/-1) means zero terminated
    bool  (*copy)(char* d, int32_t capacity,
                 const char* s, int32_t bytes); // false on overflow
    char* (*first_char)(const char* s, int32_t bytes, char ch);
    char* (*last_char)(const char* s, int32_t n, char ch); // strrchr
    char* (*first)(const char* s1, int32_t n1, const char* s2, int32_t n2);
    bool  (*to_lower)(char* d, int32_t capacity, const char* s, int32_t n);
    bool  (*to_upper)(char* d, int32_t capacity, const char* s, int32_t n);
    int32_t (*compare)(const char* s1, int32_t n1, const char* s2, int32_t n2);
    int32_t (*compare_nc)(const char* s1, int32_t n1,
                        const char* s2, int32_t n2);
    bool (*starts_with)(const char* s1, int32_t n1, const char* s2, int32_t n2);
    bool (*ends_with)(const char* s1, int32_t n1, const char* s2, int32_t n2);
    bool (*ends_with_nc)(const char* s1, int32_t n1, const char* s2, int32_t n2);
    bool (*starts_with_nc)(const char* s1, int32_t n1, const char* s2, int32_t n2);
    char* (*drop_const)(const char* s);
    // removes quotes from a head and tail of the string `s` if present
    const char* (*unquote)(char* *s, int32_t n); // modifies `s` in place
    void (*test)(void);
} ut_str_if;

extern ut_str_if ut_str;

end_c

