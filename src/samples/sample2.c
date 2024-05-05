/* Copyright (c) Dmitry "Leo" Kuznetsov 2021-24 see LICENSE for details */
#include "single_file_lib/ut/ut.h"
#include "single_file_lib/ui/ui.h"

static void opened(void) {
    static ui_font_t font;
    font = gdi.create_font("Segoe Script", app.in2px(0.5), -1);
    static ui_label_t hello = ui_label(0.0, "Hello");
    hello.view.font = &font;
    app.set_layered_window(ui_rgb(255,255,255), 0.75);
    ui_view.add(app.view,
        &ui_caption.view, // custom caption for no_decor window
        &hello,
    null);
}

static void init(void) {
    app.title  = "Sample2: translucent";
    app.opened = opened;
    app.no_decor = true;
}

app_t app = {
    .class_name = "sample2",
    .init = init,
    .window_sizing = {
        .min_w =  2.0f, // 2x1 inches
        .min_h =  1.0f,
        .ini_w =  4.0f, // 4x2 inches
        .ini_h =  2.0f
    }
};
