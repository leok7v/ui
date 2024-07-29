/* Copyright (c) Dmitry "Leo" Kuznetsov 2021-24 see LICENSE for details */
#include "single_file_lib/rt/rt.h"
#include "single_file_lib/ui/ui.h"

static int64_t hit_test(const ui_view_t* v, ui_point_t pt) {
    rt_swear(v == ui_app.content);
    if (ui_view.inside(v, &pt)) {
        if (pt.y < v->fm->em.h && ui_app.caption->state.hidden) {
            ui_app.caption->state.hidden = false;
            ui_app.request_layout();
        } else if (pt.y > v->fm->em.h && !ui_app.caption->state.hidden) {
            ui_app.caption->state.hidden = true;
            ui_app.request_layout();
        }
        return ui.hit_test.caption;
    }
    return ui.hit_test.nowhere;
}

static void opened(void) {
//  ui_app.content->insets = (ui_margins_t){ 0, 0, 0, 0 };
    static ui_label_t hello = ui_label(0.0, "Hello");
//  hello.padding = (ui_margins_t){ 0, 0, 0, 0 };
//  hello.insets  = (ui_margins_t){ 0, 0, 0, 0 };
    static ui_fm_t fm;
    ui_gdi.update_fm(&fm, ui_gdi.create_font("Segoe Script", ui_app.in2px(0.5f), -1));
    hello.fm = &fm;
    ui_app.set_layered_window(ui_color_rgb(30, 30, 30), 0.75f);
    ui_view.add_last(ui_app.content, &hello);
    ui_app.caption->state.hidden = true;
    ui_app.content->hit_test = hit_test;
}

static void character(ui_view_t* rt_unused(v), const char* utf8) {
    if (utf8[0] == 033) { // escape
        if (!ui_app.is_full_screen) { ui_app.quit(0); }
        if ( ui_app.is_full_screen) { ui_app.full_screen(false); }
    }
}

static bool key_pressed(ui_view_t* rt_unused(v), int64_t key) {
    bool swallow = key == ui.key.f11;
    if (swallow) { ui_app.full_screen(!ui_app.is_full_screen); }
    return swallow;
}

static void init(void) {
    ui_app.title  = "Sample2: translucent";
    ui_app.opened = opened;
    ui_app.root->character = character;
    ui_app.root->key_pressed = key_pressed;
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
