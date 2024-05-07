/* Copyright (c) Dmitry "Leo" Kuznetsov 2021-24 see LICENSE for details */
#include "single_file_lib/ut/ut.h"
#include "single_file_lib/ui/ui.h"

static void opened(void) {
    static ui_font_t font;
    font = ui_gdi.create_font("Segoe Script", ui_app.in2px(0.5), -1);
    static ui_label_t hello = ui_label(0.0, "Hello");
    hello.font = &font;
    ui_app.set_layered_window(ui_colors.dkgray1, 0.75f);
    if (ui_app.no_decor) { ui_view.add_last(ui_app.view, &ui_caption.view); }
    ui_view.add_last(ui_app.view, &hello);
}

static void init(void) {
    ui_app.title  = "Sample2: translucent";
    ui_app.opened = opened;
    ui_app.no_decor = false;
    // for custom caption or no caption .no_decor can be set to true
}

ui_app_t ui_app = {
    .class_name = "sample2",
    .dark_mode = true,
    .init = init,
    .window_sizing = {
        .min_w =  2.0f, // 2x1 inches
        .min_h =  1.0f,
        .ini_w =  4.0f, // 4x2 inches
        .ini_h =  2.0f
    }
};
