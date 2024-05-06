/* Copyright (c) Dmitry "Leo" Kuznetsov 2021-24 see LICENSE for details */
#include "single_file_lib/ut/ut.h"
#include "single_file_lib/ui/ui.h"

static void opened(void) {
    static ui_font_t font;
    font = ui_gdi.create_font("Segoe Script", ui_app.in2px(0.5), -1);
    static ui_label_t hello = ui_label(0.0, "Hello");
    hello.font = &font;
    ui_app.set_layered_window(ui_colors.black, 0.90f);
    ui_view.add(ui_app.view,
        &ui_caption.view, // custom caption for no_decor window
        &hello,
    null);
}

static void init(void) {
    ui_app.title  = "Sample2: translucent";
    ui_app.opened = opened;
    ui_app.no_decor = true;
}

ui_app_t ui_app = {
    .class_name = "sample2",
    .init = init,
    .window_sizing = {
        .min_w =  2.0f, // 2x1 inches
        .min_h =  1.0f,
        .ini_w =  4.0f, // 4x2 inches
        .ini_h =  2.0f
    }
};
