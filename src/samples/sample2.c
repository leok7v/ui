/* Copyright (c) Dmitry "Leo" Kuznetsov 2021-24 see LICENSE for details */
#include "single_file_lib/ut/ut.h"
#include "single_file_lib/ui/ui.h"

begin_c

static const char* title = "Sample2: translucent";

static ui_font_t font;

static void paint(ui_view_t* view) {
    ui_font_t f = gdi.set_font(font);
    ui_point_t mt = gdi.measure_text(font, "Hello");
    gdi.x = (view->w - mt.x) / 2;
    gdi.y = (view->h - mt.y) / 2;
    gdi.text("Hello");
    gdi.set_font(f);
}

static void opened(void) {
    font = gdi.create_font("Segoe Script", app.in2px(0.5), -1);
    app.set_layered_window(rgb(255,255,255), 0.75);
    app.view->paint = paint;
}

static void init(void) {
    app.title  = title;
    app.opened = opened;
    // when app.no_decor == true title bar is not draw at all
    // and extra code will be required for resizing and moving
    // window on press hold down and move... Not implemented here.
//  app.no_decor = true;
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

end_c
