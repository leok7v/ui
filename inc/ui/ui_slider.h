#pragma once
#include "ut/ut_std.h"
#include "ui/ui_button.h"

begin_c

typedef struct ui_slider_s ui_slider_t;

typedef struct ui_slider_s {
    ui_view_t view;
    void (*cb)(ui_slider_t* b); // callback
    int32_t step;
    fp64_t time;   // time last button was pressed
    ui_point_t tm; // text measurement (special case for %0*d)
    ui_button_t inc;
    ui_button_t dec;
    ui_view_t* buttons[2]; // = { dec, inc }
    int32_t value;  // for ui_slider_t range slider control
    int32_t value_min;
    int32_t value_max;
} ui_slider_t;

void _slider_init_(ui_view_t* view);

void ui_slider_init(ui_slider_t* r, const char* label, fp64_t ems,
    int32_t value_min, int32_t value_max, void (*cb)(ui_slider_t* r));

#define ui_slider(name, s, ems, vmn, vmx, code)             \
    static void name ## _callback(ui_slider_t* name) {      \
        (void)name; /* no warning if unused */              \
        code                                                \
    }                                                       \
    static                                                  \
    ui_slider_t name = {                                    \
        .view = { .type = ui_view_slider, .child = null,    \
        .width = ems, .text = s, .init = _slider_init_,     \
    }, .value_min = vmn, .value_max = vmx, .value = vmn,    \
    .cb = name ## _callback }

end_c
