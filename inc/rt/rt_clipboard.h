#pragma once
#include "rt/rt_std.h"

rt_begin_c

typedef struct ui_image_s ui_image_t;

typedef struct {
    errno_t (*put_text)(const char* s);
    errno_t (*get_text)(char* text, int32_t* bytes);
    errno_t (*put_image)(ui_image_t* image); // only for Windows apps
    void (*test)(void);
} rt_clipboard_if;

extern rt_clipboard_if rt_clipboard;

rt_end_c
