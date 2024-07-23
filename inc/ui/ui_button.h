#pragma once
#include "ut/ut_std.h"
#include "ui/ui_view.h"

ut_begin_c

typedef ui_view_t ui_button_t;

void ui_view_init_button(ui_view_t* v);

void ui_button_init(ui_button_t* b, const char* label, fp32_t min_width_em,
    void (*callback)(ui_button_t* b));

// ui_button_clicked can only be used on static button variables

#define ui_button_clicked(name, s, min_width_em, ...)       \
    static void name ## _clicked(ui_button_t* name) {       \
        (void)name; /* no warning if unused */              \
        { __VA_ARGS__ }                                     \
    }                                                       \
    static                                                  \
    ui_button_t name = {                                    \
        .type = ui_view_button,                             \
        .init = ui_view_init_button,                        \
        .fm = &ui_app.fm.prop.normal,                       \
        .p.text = s,                                        \
        .callback = name ## _clicked,                       \
        .color_id = ui_color_id_button_text,                \
        .min_w_em = min_width_em, .min_h_em = 1.25f,        \
        .insets  = {                                        \
            .left  = ui_view_i_lr, .top    = ui_view_i_tb,  \
            .right = ui_view_i_lr, .bottom = ui_view_i_tb   \
        },                                                  \
        .padding = {                                        \
            .left  = ui_view_p_lr, .top    = ui_view_p_tb,  \
            .right = ui_view_p_lr, .bottom = ui_view_p_tb,  \
        }                                                   \
    }

#define ui_button(s, min_width_em, clicked) {               \
    .type = ui_view_button,                                 \
    .init = ui_view_init_button,                            \
    .fm = &ui_app.fm.prop.normal,                           \
    .p.text = s,                                            \
    .callback = clicked,                                    \
    .color_id = ui_color_id_button_text,                    \
    .min_w_em = min_width_em, .min_h_em = 1.25f,            \
    .insets  = {                                            \
        .left  = ui_view_i_lr, .top    = ui_view_i_tb,      \
        .right = ui_view_i_lr, .bottom = ui_view_i_tb       \
    },                                                      \
    .padding = {                                            \
        .left  = ui_view_p_lr, .top    = ui_view_p_tb,      \
        .right = ui_view_p_lr, .bottom = ui_view_p_tb,      \
    }                                                       \
}

// usage:
//
// ui_button_clicked(button, "&Button", 7.0, {
//      if (button->state.pressed) {
//          // do something on click that happens on release mouse button
//      }
// })
//
// or:
//
// static void button_flipped(ui_button_t* b) {
//      swear(b->flip == true); // 2 state button, clicked on mouse press button
//      if (b->state.pressed) {
//          // show something:
//      } else {
//          // show something else:
//      }
// }
//
// ui_button_t button = ui_button(7.0, "&Button", button_flipped);
//
// or
//
// ui_button_t button = ui_view)button(button);
// ui_view.set_text(button.text, "&Button");
// button.min_w_em = 7.0;
// button.callback = button_flipped;
//
// Note:
// ui_button_clicked(button, "&Button", 7.0, {
//      button->state.pressed = !button->state.pressed;
//      // is similar to: button.flip = true but it leads thru
//      // multiple button paint and click happens on mouse button
//      // release not press
// }


ut_end_c
