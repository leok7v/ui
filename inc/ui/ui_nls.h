#pragma once
#include "ut/ut_std.h"

begin_c

typedef struct { // i18n national language support
    void (*init)(void);
    const char* (*locale)(void);  // "en-US" "zh-CN" etc...
    // force locale for debugging and testing:
    void (*set_locale)(const char* locale); // only for calling thread
    // nls(s) is same as string(strid(s), s)
    const char* (*str)(const char* defau1t); // returns localized string
    // strid("foo") returns 0 if there is no matching ENGLISH NEUTRAL
    // STRINGTABLE entry
    int32_t (*strid)(const char* s);
    // given strid > 0 returns localized string or defau1t value
    const char* (*string)(int32_t strid, const char* defau1t);
} ui_nls_if;

extern ui_nls_if ui_nls;

end_c
