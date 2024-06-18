#pragma once
#include "ut/ut_std.h"
#include "ui/ui_view.h"

begin_c

typedef ui_view_t ui_label_t;

void ui_view_init_label(ui_view_t* view);

// label insets and padding left/right are intentionally
// smaller than button/slider/toggle controls

#define ui_label(min_width_em, s) {                    \
    .type = ui_view_label, .init = ui_view_init_label, \
    .fm = &ui_app.fonts.regular,                       \
    .p.text = s,                                       \
    .min_w_em = min_width_em, .min_h_em = 1.0,         \
    .insets  = {                                       \
        .left  = 0.25f, .top    = ui_view_i_t,         \
        .right = 0.25f, .bottom = ui_view_i_b          \
    },                                                 \
    .padding = {                                       \
        .left  = 0.25, .top    = ui_view_p_t,          \
        .right = 0.25, .bottom = ui_view_p_b,          \
    }                                                  \
}

// text with "&" keyboard shortcuts:

void ui_label_init(ui_label_t* t, fp32_t min_w_em, const char* format, ...);
void ui_label_init_va(ui_label_t* t, fp32_t min_w_em, const char* format, va_list vl);

// use this macro for initialization:
//    ui_label_t label = ui_label(min_width_em, s);
// or:
//    label = (ui_label_t)ui_label(min_width_em, s);
// which is subtle C difference of constant and
// variable initialization and I did not find universal way

end_c
