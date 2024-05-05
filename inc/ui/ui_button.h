#pragma once
#include "ut/ut_std.h"
#include "ui/ui_view.h"

begin_c

typedef struct ui_button_s ui_button_t;

typedef struct ui_button_s {
    ui_view_t view;
    void (*cb)(ui_button_t* b); // callback TODO: move into view?
} ui_button_t;

void ui_button_init(ui_button_t* b, const char* label, fp64_t ems,
    void (*cb)(ui_button_t* b));

void ui_button_init_(ui_view_t* view); // do not call use static_ui_button() macro

#define static_ui_button(name, s, w, ...)                             \
    static void name ## _callback(ui_button_t* name) {                \
        (void)name; /* no warning if unused */                        \
        { __VA_ARGS__ }                                               \
    }                                                                 \
    static                                                            \
    ui_button_t name = {                                              \
        .view = { .type = ui_view_button, .init = ui_button_init_,    \
        .font = &app.fonts.regular, .width = w, .text = s},           \
    .cb = name ## _callback }

#define ui_button(s, w, callback) {                               \
    .view = { .type = ui_view_button, .init = ui_button_init_,    \
              .font = &app.fonts.regular, .width = w, .text = s}, \
    .cb = callback }

// usage:
//
// static_ui_button(button, "&Button", 7.0, {
//      button->view.pressed = !button->view.pressed;
// })
//
// or:
//
// static void button_flipped(ui_button_t* b) {
//      b->view.pressed = !b->view.pressed;
// }
//
// ui_button_t button = ui_button(7.0, "&Button", button_flipped);

end_c
