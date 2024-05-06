#pragma once
#include "ut/ut_std.h"
#include "ui/ui_view.h"

begin_c


typedef ui_view_t ui_toggle_t;

// label may contain "___" which will be replaced with "On" / "Off"
void ui_toggle_init(ui_toggle_t* b, const char* label, fp32_t ems,
    void (*callback)(ui_toggle_t* b));

void ui_view_init_toggle(ui_view_t* view);

#define static_ui_toggle(name, s, min_width_em, ...)          \
    static void name ## _callback(ui_toggle_t* name) {        \
        (void)name; /* no warning if unused */                \
        { __VA_ARGS__ }                                       \
    }                                                         \
    static                                                    \
    ui_toggle_t name = {                                      \
        .type = ui_view_toggle, .init = ui_view_init_toggle,  \
        .font = &ui_app.fonts.regular, .min_w_em = min_width_em, \
        .text = s, .callback = name ## _callback              \
   }

#define ui_toggle(s, min_width_em, call_back) {           \
    .type = ui_view_toggle, .init = ui_view_init_toggle,  \
    .font = &ui_app.fonts.regular, .min_w_em = min_width_em, \
    .text = s, .callback = call_back                      \
}

end_c
