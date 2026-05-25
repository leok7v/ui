#pragma once
#include "posix.h"
#include "ui/ui_button.h"

posix_begin_c

struct ui_slider;

struct ui_slider {
    union {
        struct ui_view view;
        struct ui_view;
    };
    int32_t step;
    fp64_t time; // time last button was pressed
    struct ui_wh wh;  // text measurement (special case for %0*d)
    ui_button_t inc; // can be hidden
    ui_button_t dec; // can be hidden
    int32_t value;  // for struct ui_slider range slider control
    int32_t value_min;
    int32_t value_max;
    // style:
    bool notched; // true if marked with a notches and has a thumb
};

void ui_view_init_slider(struct ui_view* v);

void ui_slider_init(struct ui_slider* r, const char* label, fp32_t min_w_em,
    int32_t value_min, int32_t value_max, void (*callback)(struct ui_view* r));

// ui_slider_changed can only be used on static slider variables

#define ui_slider_changed(name, s, min_width_em, mn,  mx, fmt, ...) \
    static void name ## _changed(struct ui_slider* name) {               \
        (void)name; /* no warning if unused */                      \
        { __VA_ARGS__ }                                             \
    }                                                               \
    static                                                          \
    struct ui_slider name = {                                            \
        .view = {                                                   \
            .type = ui_view_slider,                                 \
            .init = ui_view_init_slider,                            \
            .fm = &ui_app.fm.prop.normal,                           \
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
        .fm = &ui_app.fm.prop.normal,                               \
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

posix_end_c
