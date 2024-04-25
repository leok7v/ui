#pragma once
#include "ut/std.h"

begin_c

typedef struct image_s image_t;

typedef struct clipboard_if {
    errno_t (*put_text)(const char* s);
    errno_t (*get_text)(char* text, int32_t* bytes);
    errno_t (*put_image)(image_t* im); // only for Windows apps
    void (*test)(void);
} clipboard_if;

extern clipboard_if clipboard;

end_c
