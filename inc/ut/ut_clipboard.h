#pragma once
#include "ut/ut_std.h"

ut_begin_c

typedef struct ui_image_s ui_image_t;

typedef struct {
    errno_t (*put_text)(const char* s);
    errno_t (*get_text)(char* text, int32_t* bytes);
    errno_t (*put_image)(ui_image_t* image); // only for Windows apps
    void (*test)(void);
} ut_clipboard_if;

extern ut_clipboard_if ut_clipboard;

ut_end_c
