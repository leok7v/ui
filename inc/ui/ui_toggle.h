#pragma once
#include "ut/ut_std.h"
#include "ui/ui_view.h"

begin_c

typedef struct ui_toggle_s ui_toggle_t;

typedef struct ui_toggle_s {
    ui_view_t view;
    void (*cb)(ui_toggle_t* b); // callback
} ui_toggle_t;

// label may contain "___" which will be replaced with "On" / "Off"
void ui_toggle_init(ui_toggle_t* b, const char* label, fp64_t ems,
    void (*cb)(ui_toggle_t* b));

void ui_toggle_init_(ui_view_t* view); // do not call use ui_toggle() macro

#define ui_toggle(name, s, w, code)                                   \
    static void name ## _callback(ui_toggle_t* name) {                \
        (void)name; /* no warning if unused */                        \
        code                                                          \
    }                                                                 \
    static                                                            \
   ui_toggle_t name = {                                               \
    .view = { .type = ui_view_toggle, .init = ui_toggle_init_,        \
    .child = null, .width = w, .text = s}, .cb = name ## _callback }

end_c
