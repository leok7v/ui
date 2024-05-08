#pragma once
#include "ut/ut_std.h"
#include "ui/ui_view.h"

begin_c

typedef ui_view_t ui_button_t;

void ui_view_init_button(ui_view_t* view);

void ui_button_init(ui_button_t* b, const char* label, fp32_t min_width_em,
    void (*callback)(ui_button_t* b));

// ui_button_on_click can only be used on static button variables

#define ui_button_on_click(name, s, min_width_em, ...)           \
    static void name ## _callback(ui_button_t* name) {           \
        (void)name; /* no warning if unused */                   \
        { __VA_ARGS__ }                                          \
    }                                                            \
    static                                                       \
    ui_button_t name = {                                         \
        .type = ui_view_button, .init = ui_view_init_button,     \
        .fm = &ui_app.fonts.regular, .min_w_em = min_width_em,   \
        .text = s, .callback = name ## _callback                 \
    }

#define ui_button(s, min_width_em, call_back) {              \
    .type = ui_view_button, .init = ui_view_init_button,     \
    .fm = &ui_app.fonts.regular, .min_w_em = min_width_em,   \
    .text = s, .callback = call_back }                       \

// usage:
//
// ui_button_on_click(button, "&Button", 7.0, {
//      button->pressed = !button->pressed;
// })
//
// or:
//
// static void button_flipped(ui_button_t* b) {
//      b->pressed = !b->pressed;
// }
//
// ui_button_t button = ui_button(7.0, "&Button", button_flipped);
//
// or
//
// ui_button_t button = ui_view)button(button);
// strprintf(button.text, "&Button");
// button.min_w_em = 7.0;
// button.callback = button_flipped;


end_c
