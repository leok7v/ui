#pragma once
#include "ut/ut_std.h"
#include "ui/ui_button.h"

ut_begin_c

typedef struct ui_slider_s ui_slider_t;

typedef struct ui_slider_s {
    union {
        ui_view_t view;
        struct ui_view_s;
    };
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

void ui_view_init_slider(ui_view_t* v);

void ui_slider_init(ui_slider_t* r, const char* label, fp32_t min_w_em,
    int32_t value_min, int32_t value_max, void (*callback)(ui_view_t* r));

// ui_slider_changed can only be used on static slider variables

#define ui_slider_changed(name, s, min_width_em, mn,  mx, fmt, ...) \
    static void name ## _changed(ui_slider_t* name) {               \
        (void)name; /* no warning if unused */                      \
        { __VA_ARGS__ }                                             \
    }                                                               \
    static                                                          \
    ui_slider_t name = {                                            \
        .view = {                                                   \
            .type = ui_view_slider,                                 \
            .init = ui_view_init_slider,                            \
            .fm = &ui_app.fm.regular,                               \
            .p.text = s,                                            \
            .format = fmt,                                          \
            .callback = name ## _changed,                           \
            .min_w_em = min_width_em, .min_h_em = 1.25f,            \
            .insets  = {                                            \
                .left  = ui_view_i_lr, .top    = ui_view_i_tb,      \
                .right = ui_view_i_lr, .bottom = ui_view_i_tb       \
            },                                                      \
            .padding = {                                            \
                .left  = ui_view_p_lr, .top    = ui_view_p_tb,      \
                .right = ui_view_p_lr, .bottom = ui_view_p_tb,      \
            }                                                       \
        },                                                          \
        .value_min = mn, .value_max = mx, .value = mn,              \
    }

#define ui_slider(s, min_width_em, mn, mx, fmt, changed) {          \
    .view = {                                                       \
        .type = ui_view_slider,                                     \
        .init = ui_view_init_slider,                                \
        .fm = &ui_app.fm.regular,                                   \
        .p.text = s,                                                \
        .callback = changed,                                        \
        .format = fmt,                                              \
        .min_w_em = min_width_em, .min_h_em = 1.25f,                \
            .insets  = {                                            \
                .left  = ui_view_i_lr, .top    = ui_view_i_tb,      \
                .right = ui_view_i_lr, .bottom = ui_view_i_tb       \
            },                                                      \
            .padding = {                                            \
                .left  = ui_view_p_lr, .top    = ui_view_p_tb,      \
                .right = ui_view_p_lr, .bottom = ui_view_p_tb,      \
            }                                                       \
    },                                                              \
    .value_min = mn, .value_max = mx, .value = mn,                  \
}

ut_end_c
