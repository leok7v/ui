#pragma once
#include "ut/std.h"

begin_c

#define __suppress_alloca_warnings__ _Pragma("warning(suppress: 6255 6263)")
#define __suppress_buffer_overrun__  _Pragma("warning(suppress: 6386)")

#define stackalloc(bytes) (__suppress_alloca_warnings__ alloca(bytes))
#define zero_initialized_stackalloc(bytes) memset(stackalloc(bytes), 0, (bytes))

// Since a lot of str*() operations are preprocessor defines
// care should be exercised that arguments of macro invocations
// do not have side effects or not computationally expensive.
// None of the definitions are performance champions - if the
// code needs extreme cpu cycle savings working with utf8 strings

#define strempty(s) ((s) == null || (s)[0] == 0)

#pragma deprecated(strconcat, strtolowercase) // use strprintf() instead

#define strconcat(a, b) __suppress_buffer_overrun__ \
    (strcat(strcpy((char*)stackalloc(strlen(a) + strlen(b) + 1), (a)), (b)))

#define strequ(s1, s2)  (((void*)(s1) == (void*)(s2)) || \
    (((void*)(s1) != null && (void*)(s2) != null) && strcmp((s1), (s2)) == 0))

#define striequ(s1, s2)  (((void*)(s1) == (void*)(s2)) || \
    (((void*)(s1) != null && (void*)(s2) != null) && stricmp((s1), (s2)) == 0))

#define strendswith(s1, s2) \
    (strlen(s1) >= strlen(s2) && strcmp((s1) + strlen(s1) - strlen(s2), (s2)) == 0)

#define strlength(s) ((int)strlen(s)) // avoid code analysis noise
// a lot of posix like API consumes "int" instead of size_t which
// is acceptable for majority of char* zero terminated strings usage
// since most of them work with filepath that are relatively short
// and on Windows are limited to 260 chars or 32KB - 1 chars.

#define strcopy(s1, s2) /* use with extreme caution */                      \
    do {                                                                   \
        strncpy((s1), (s2), countof((s1)) - 1); s1[countof((s1)) - 1] = 0; \
} while (0)

char* strnchr(const char* s, int32_t n, char ch);

#define strtolowercase(s) \
    str.to_lowercase((char*)stackalloc(strlen(s) + 1), strlen(s) + 1, s)

#define utf16to8(utf16) str.utf16_utf8((char*) \
    stackalloc((size_t)str.utf8_bytes(utf16) + 1), utf16)

#define utf8to16(s) str.utf8_utf16((wchar_t*)stackalloc((str.utf16_chars(s) + 1) * \
    sizeof(wchar_t)), s)

#define strprintf(s, ...) str.sformat((s), countof(s), "" __VA_ARGS__)

#define strstartswith(a, b) \
    (strlen(a) >= strlen(b) && memcmp((a), (b), strlen(b)) == 0)

#define strncasecmp _strnicmp
#define strcasecmp _stricmp

// case insensitive functions with postfix _nc
// operate only on ASCII characters. No ANSI no UTF-8

typedef struct {
    const char* (*error)(int32_t error);     // en-US
    const char* (*error_nls)(int32_t error); // national locale string
    // deprecated: use str.*
    int32_t (*utf8_bytes)(const wchar_t* utf16);
    int32_t (*utf16_chars)(const char* s);
    char* (*utf16_utf8)(char* destination, const wchar_t* utf16);
    wchar_t* (*utf8_utf16)(wchar_t* destination, const char* utf8);
    // string formatting printf style:
    void (*vformat)(char* utf8, int32_t count, const char* format, va_list vl);
    void (*sformat)(char* utf8, int32_t count, const char* format, ...);
    bool (*is_empty)(const char* s); // null or empty string
    bool (*equal)(const char* s1, const char* s2);
    bool (*equal_nc)(const char* s1, const char* s2);
    int32_t (*length)(const char* s);
    // copy(s1, countof(s1), s2, /*bytes*/-1) means zero terminated
    bool  (*copy)(char* d, int32_t capacity,
                 const char* s, int32_t bytes); // false on overflow
    char* (*first_char)(const char* s1, int32_t bytes, char ch);
    char* (*last_char)(const char* s1, char ch); // strrchr
    char* (*first)(const char* s1, const char* s2);
    bool  (*to_lower)(char* d, int32_t capacity, const char* s);
    bool  (*to_upper)(char* d, int32_t capacity, const char* s);
    int32_t (*compare)(const char* s1, int32_t bytes, const char* s2);
    int32_t (*compare_nc)(const char* s1, int32_t bytes,
                        const char* s2); // no-case ASCII only
    bool (*starts_with)(const char* s1, const char* s2);
    bool (*ends_with)(const char* s1, const char* s2);
    bool (*ends_with_nc)(const char* s1, const char* s2);
    void (*test)(void);
} str_if;

extern str_if str;

end_c

