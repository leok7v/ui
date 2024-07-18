#pragma once
#include "ut/ut_std.h"
#include "ui/ui_view.h"

ut_begin_c

typedef ui_view_t ui_toggle_t;

// label may contain "___" which will be replaced with "On" / "Off"
void ui_toggle_init(ui_toggle_t* b, const char* label, fp32_t ems,
    void (*callback)(ui_toggle_t* b));

void ui_view_init_toggle(ui_view_t* v);

// ui_toggle_on_off can only be used on static toggle variables

#define ui_toggle_on_off(name, s, min_width_em, ...)        \
    static void name ## _on_off(ui_toggle_t* name) {        \
        (void)name; /* no warning if unused */              \
        { __VA_ARGS__ }                                     \
    }                                                       \
    static                                                  \
    ui_toggle_t name = {                                    \
        .type = ui_view_toggle,                             \
        .init = ui_view_init_toggle,                        \
        .fm = &ui_app.fm.regular,                           \
        .min_w_em = min_width_em,  .min_h_em = 1.25f,       \
        .p.text = s,                                        \
        .callback = name ## _on_off,                        \
        .insets  = {                                        \
            .left  = 1.75f,        .top    = ui_view_i_tb,  \
            .right = ui_view_i_lr, .bottom = ui_view_i_tb   \
        },                                                  \
        .padding = {                                        \
            .left  = ui_view_p_lr, .top    = ui_view_p_tb,  \
            .right = ui_view_p_lr, .bottom = ui_view_p_tb,  \
        }                                                   \
    }

#define ui_toggle(s, min_width_em, on_off) {                \
    .type = ui_view_toggle,                                 \
    .init = ui_view_init_toggle,                            \
    .fm = &ui_app.fm.regular,                               \
    .p.text = s,                                            \
    .callback = on_off,                                     \
    .min_w_em = min_width_em,  .min_h_em = 1.25f,           \
    .insets  = {                                            \
        .left  = 1.75f,        .top    = ui_view_i_tb,      \
        .right = ui_view_i_lr, .bottom = ui_view_i_tb       \
    },                                                      \
    .padding = {                                            \
        .left  = ui_view_p_lr, .top    = ui_view_p_tb,      \
        .right = ui_view_p_lr, .bottom = ui_view_p_tb,      \
    }                                                       \
}

ut_end_c
