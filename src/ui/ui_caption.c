/* Copyright (c) Dmitry "Leo" Kuznetsov 2021-24 see LICENSE for details */
#include "ut/ut.h"
#include "ui/ui.h"

#define ui_caption_glyph_rest ui_glyph_upper_right_drop_shadowed_white_square
#define ui_caption_glyph_menu ui_glyph_trigram_for_heaven
#define ui_caption_glyph_mini ui_glyph_heavy_minus_sign
#define ui_caption_glyph_maxi ui_glyph_white_large_square
#define ui_caption_glyph_full ui_glyph_square_four_corners
#define ui_caption_glyph_quit ui_glyph_n_ary_times_operator


static void ui_caption_draw_icon(int32_t x, int32_t y, int32_t w, int32_t h) {
    int32_t n = 16; // minimize distortion
    while (n * 2 < ut_min(w, h)) { n += n; }
    gdi.draw_icon(x + (w - n) / 2, y + (h - n) / 2, n, n, app.icon);
}

static void ui_caption_paint(ui_view_t* v) {
    swear(v == &ui_caption.view);
    gdi.push(v->x, v->y);
    gdi.fill_with(v->x, v->y, v->w, v->h, v->color);
    if (app.icon != null) {
        ui_caption_draw_icon(
            gdi.x,
            ui_caption.button_menu.view.y,
            ui_caption.button_menu.view.w,
            ui_caption.button_menu.view.h);
    }
    if (v->text[0] != 0) {
        ui_point_t mt = gdi.measure_text(*v->font, v->text);
        gdi.x += (v->w - mt.x) / 2;
        gdi.y += (v->h - mt.y) / 2;
        gdi.set_text_color((ui_color_t)(v->color ^ 0xFFFFFF));
        gdi.text("%s", v->text);
    }
    gdi.pop();
}

static void ui_caption_toggle_full(void) {
    app.full_screen(!app.is_full_screen);
    ui_caption.view.hidden = app.is_full_screen;
    app.layout();
}

static void ui_app_view_character(ui_view_t* v, const char utf8[]) {
    swear(v == app.view);
    if (utf8[0] == 033 && app.is_full_screen) { ui_caption_toggle_full(); }
}

static void ui_app_view_paint(ui_view_t* v) {
    swear(v == app.view);
    gdi.fill_with(0, 0, v->w, v->h, colors.dkgray1);
    ui_color_t c = app.is_active() ? colors.orange : colors.btn_hover_highlight;
    if (app.is_full_screen) { c = colors.dkgray1; }
    gdi.frame_with(0, 0, v->w - 0, v->h - 0, colors.dkgray1);
    gdi.frame_with(1, 1, v->w - 2, v->h - 2, c);
}

static void ui_caption_init(ui_view_t* v) {
    swear(v == &ui_caption.view, "caption is a singleton");
    v->hidden = false,
    v->color = colors.dkgray2;
    v->paint = ui_caption_paint;
//  v->mouse = ui_caption_mouse;
    app.view->character = ui_app_view_character; // ESC for full screen
    app.view->paint = ui_app_view_paint;
//  app.view->mouse = ui_app_view_mouse;
    ui_view.add(&ui_caption.view,
        &ui_caption.button_menu,
        &ui_caption.button_mini,
        &ui_caption.button_maxi,
        &ui_caption.button_full,
        &ui_caption.button_quit,
        null);
    ui_view_for_each(&ui_caption.view, c, {
        c->font = &app.fonts.H3;
        c->color = colors.white;
        c->flat = true;
        });
}

static void ui_caption_quit(ui_button_t* unused(b)) {
    app.close();
}

static void ui_caption_mini(ui_button_t* unused(b)) {
    app.show_window(ui.visibility.minimize);
}

static void ui_caption_maxi(ui_button_t* unused(b)) {
    // TODO: Absolutely correct handling is much more
    //       complicated than this. Because of the
    //       initial state of window may possibly be
    //       full screen or maximized (see app.visibility)
    //       Note that show_window() does not update
    //       app.visibility because e.g. restore or show_na
    //       may result if a variety of visibility states
    static bool maximized;
    if (!maximized) {
        app.show_window(ui.visibility.maximize);
        maximized = true;
        strprintf(ui_caption.button_maxi.view.text, "%s",
                  ui_caption_glyph_rest);
    } else {
        app.show_window(ui.visibility.restore);
        maximized = false;
        strprintf(ui_caption.button_maxi.view.text, "%s",
                  ui_caption_glyph_maxi);
    }
}

static void ui_caption_full(ui_button_t* unused(b)) {
    ui_caption_toggle_full();
}

static int64_t ui_caption_hit_test(int32_t x, int32_t y) {
    ui_point_t pt = {x, y};
    ui_view_for_each(&ui_caption.view, c, {
        if (ui_view.inside(c, &pt)) { return ui.hit_test.client; }
    });
    if (x < ui_caption.view.h && y < ui_caption.view.h) {
        return ui.hit_test.system_menu;
    }
    return ui.hit_test.caption;
}

ui_caption_t ui_caption =  {
    .view = {
        .type = ui_view_container,
        .init = ui_caption_init,
        .hidden = true,
    },
    .hit_test = ui_caption_hit_test,
    .button_menu = ui_button(ui_caption_glyph_menu, 0.0, null),
    .button_mini = ui_button(ui_caption_glyph_mini, 0.0, ui_caption_mini),
    .button_maxi = ui_button(ui_caption_glyph_maxi, 0.0, ui_caption_maxi),
    .button_full = ui_button(ui_caption_glyph_full, 0.0, ui_caption_full),
    .button_quit = ui_button(ui_caption_glyph_quit, 0.0, ui_caption_quit),
};
