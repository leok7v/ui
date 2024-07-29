#pragma once
#include "ut/ut_std.h"

rt_begin_c

typedef struct { // i18n national language support
    void (*init)(void);
    const char* (*locale)(void);  // "en-US" "zh-CN" etc...
    // force locale for debugging and testing:
    errno_t (*set_locale)(const char* locale); // only for calling thread
    // nls(s) is same as string(strid(s), s)
    const char* (*str)(const char* defau1t); // returns localized string
    // strid("foo") returns -1 if there is no matching
    // ENGLISH NEUTRAL STRINGTABLE entry
    int32_t (*strid)(const char* s);
    // given strid > 0 returns localized string or default value
    const char* (*string)(int32_t strid, const char* defau1t);
} rt_nls_if;

extern rt_nls_if rt_nls;

rt_end_c
