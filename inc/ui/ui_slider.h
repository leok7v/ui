#pragma once
#include "ut/ut_std.h"
#include "ui/ui_button.h"

begin_c

typedef struct ui_slider_s ui_slider_t;

typedef struct ui_slider_s {
    ui_view_t view;
    int32_t step;
    fp64_t time;   // time last button was pressed
    ui_point_t tm; // text measurement (special case for %0*d)
    ui_button_t inc;
    ui_button_t dec;
    int32_t value;  // for ui_slider_t range slider control
    int32_t value_min;
    int32_t value_max;
} ui_slider_t;

void ui_view_init_slider(ui_view_t* view);

void ui_slider_init(ui_slider_t* r, const char* label, fp32_t min_w_em,
    int32_t value_min, int32_t value_max, void (*callback)(ui_view_t* r));

#define static_ui_slider(name, s, min_width_em, vmn, vmx, ...)            \
    static void name ## _callback(ui_slider_t* name) {                    \
        (void)name; /* no warning if unused */                            \
        { __VA_ARGS__ }                                                   \
    }                                                                     \
    static                                                                \
    ui_slider_t name = {                                                  \
        .view = { .type = ui_view_slider, .font = &app.fonts.regular,     \
                  .min_w_em = min_width_em, .init = ui_view_init_slider,  \
                   .text = s, .callback = name ## _callback               \
        },                                                                \
        .value_min = vmn, .value_max = vmx, .value = vmn,                 \
    }

#define ui_slider(s, min_width_em, vmn, vmx, call_back) {                 \
    .view = { .type = ui_view_slider, .font = &app.fonts.regular,         \
        .min_w_em = min_width_em, .text = s, .init = ui_view_init_slider, \
        .callback = call_back                                             \
    },                                                                    \
    .value_min = vmn, .value_max = vmx, .value = vmn,                     \
}

end_c
