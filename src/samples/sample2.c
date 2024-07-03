/* Copyright (c) Dmitry "Leo" Kuznetsov 2021-24 see LICENSE for details */
#include "single_file_lib/ut/ut.h"
#include "single_file_lib/ui/ui.h"

static int64_t hit_test(ui_view_t* unused(v), int32_t x, int32_t y) {
    ui_point_t pt = { x, y };
    if (ui_view.inside(v, &pt)) {
        if (y < v->fm->em.h && ui_app.caption->state.hidden) {
            ui_app.caption->state.hidden = false;
            ui_app.request_layout();
        } else if (y > v->fm->em.h && !ui_app.caption->state.hidden) {
            ui_app.caption->state.hidden = true;
            ui_app.request_layout();
        }
        return ui.hit_test.caption;
    }
    return ui.hit_test.nowhere;
}

static void painted(ui_view_t* v) {
    ui_view.debug_paint_gaps(v);
    ui_gdi.frame(v->x, v->y, v->w, v->h, ui_colors.white);
}

static void opened(void) {
//  ui_app.content->insets = (ui_gaps_t){ 0, 0, 0, 0 };
    static ui_label_t hello = ui_label(0.0, "Hello");
//  hello.padding = (ui_gaps_t){ 0, 0, 0, 0 };
//  hello.insets  = (ui_gaps_t){ 0, 0, 0, 0 };
    static ui_fm_t fm;
    ui_gdi.update_fm(&fm, ui_gdi.create_font("Segoe Script", ui_app.in2px(0.5), -1));
    hello.fm = &fm;
    ui_app.set_layered_window(ui_color_rgb(30, 30, 30), 0.75f);
    ui_view.add_last(ui_app.content, &hello);
    ui_app.caption->state.hidden = true;
    ui_app.content->hit_test = hit_test;
    hello.painted = painted;
}

static void init(void) {
    ui_app.title  = "Sample2: translucent";
    ui_app.opened = opened;
    // for custom caption or no caption .no_decor can be set to true
    ui_app.no_decor = true;
    ui_caption.menu.state.hidden = true;
}

ui_app_t ui_app = {
    .class_name = "sample2",
    .dark_mode = true,
    .init = init,
    .window_sizing = {
        .min_w =  1.8f, // 2x1 inches
        .min_h =  1.0f,
        .ini_w =  4.0f, // 4x2 inches
        .ini_h =  2.0f
    }
};
