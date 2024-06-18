#pragma once
#include "ut/ut_std.h"
#include "ui/ui_button.h"

begin_c

typedef struct ui_slider_s ui_slider_t;

typedef struct ui_slider_s {
    ui_view_t view;
    int32_t step;
    fp64_t time; // time last button was pressed
    ui_wh_t mt;  // text measurement (special case for %0*d)
    ui_button_t inc; // can be hidden
    ui_button_t dec; // can be hidden
    int32_t value;  // for ui_slider_t range slider control
    int32_t value_min;
    int32_t value_max;
    // style:
    bool notched; // true if marked with a notches and has a thumb
} ui_slider_t;

void ui_view_init_slider(ui_view_t* view);

void ui_slider_init(ui_slider_t* r, const char* label, fp32_t min_w_em,
    int32_t value_min, int32_t value_max, void (*callback)(ui_view_t* r));

// ui_slider_on_change can only be used on static slider variables

#define ui_slider_on_change(name, s, min_width_em, vmn, vmx, format_v, ...) \
    static void name ## _callback(ui_slider_t* name) {              \
        (void)name; /* no warning if unused */                      \
        { __VA_ARGS__ }                                             \
    }                                                               \
    static                                                          \
    ui_slider_t name = {                                            \
        .view = {                                                   \
            .type = ui_view_slider, .fm = &ui_app.fonts.regular,    \
            .init = ui_view_init_slider,                            \
            .p.text = s,                                            \
            .format = format_v,                                     \
            .callback = name ## _callback,                          \
            .min_w_em = min_width_em, .min_h_em = 1.0,              \
            .insets  = {                                            \
                .left  = ui_view_i_lr, .top    = ui_view_i_t,       \
                .right = ui_view_i_lr, .bottom = ui_view_i_b        \
            },                                                      \
            .padding = {                                            \
                .left  = ui_view_p_l, .top    = ui_view_p_t,        \
                .right = ui_view_p_r, .bottom = ui_view_p_b,        \
            }                                                       \
        },                                                          \
        .value_min = vmn, .value_max = vmx, .value = vmn,           \
    }

#define ui_slider(s, min_width_em, vmn, vmx, format_v, call_back) { \
    .view = { .type = ui_view_slider, .fm = &ui_app.fonts.regular,  \
        .init = ui_view_init_slider,                                \
        .p.text = s,                                                \
        .callback = call_back,                                      \
        .format = format_v,                                         \
        .min_w_em = min_width_em, .min_h_em = 1.0,                  \
            .insets  = {                                            \
                .left  = ui_view_i_lr, .top    = ui_view_i_t,       \
                .right = ui_view_i_lr, .bottom = ui_view_i_b        \
            },                                                      \
            .padding = {                                            \
                .left  = ui_view_p_l, .top    = ui_view_p_t,        \
                .right = ui_view_p_r, .bottom = ui_view_p_b,        \
            }                                                       \
    },                                                              \
    .value_min = vmn, .value_max = vmx, .value = vmn,               \
}

end_c
