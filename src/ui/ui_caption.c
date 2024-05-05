/* Copyright (c) Dmitry "Leo" Kuznetsov 2021-24 see LICENSE for details */
#include "ut/ut.h"
#include "ui/ui.h"

#define ui_caption_glyph_rest ui_glyph_upper_right_drop_shadowed_white_square
#define ui_caption_glyph_menu ui_glyph_trigram_for_heaven
#define ui_caption_glyph_mini ui_glyph_heavy_minus_sign
#define ui_caption_glyph_maxi ui_glyph_white_large_square
#define ui_caption_glyph_full ui_glyph_square_four_corners
#define ui_caption_glyph_quit ui_glyph_n_ary_times_operator

// TODO: remove me
static void ui_caption_paint(ui_view_t* v) {
    // TODO: add app title label instead
    swear(v == &ui_caption.view);
    gdi.fill_with(v->x, v->y, v->w, v->h, v->color);
    gdi.push(v->x, v->y);
    if (v->text[0] != 0) {
        ui_font_t f = v->font != null ? *v->font : app.fonts.regular;
        ui_point_t mt = gdi.measure_text(f, v->text);
        gdi.x += (v->w - mt.x) / 2;
        gdi.y += (v->h - mt.y) / 2;
        gdi.set_text_color((ui_color_t)(v->color ^ 0xFFFFFF));
        f = gdi.set_font(f);
        gdi.text("%s", v->text);
        gdi.set_font(f);
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
    // TODO: inside app.c instead of here
    if (utf8[0] == 033 && app.is_full_screen) { ui_caption_toggle_full(); }
}

static void ui_caption_quit(ui_button_t* unused(b)) {
    app.close();
}

static void ui_caption_mini(ui_button_t* unused(b)) {
    app.show_window(ui.visibility.minimize);
}

static void ui_caption_maximize_or_restore(void) {
    strprintf(ui_caption.maxi.text, "%s",
        app.is_maximized() ?
        ui_caption_glyph_rest : ui_caption_glyph_maxi);
}

static void ui_caption_maxi(ui_button_t* unused(b)) {
    if (!app.is_maximized()) {
        app.show_window(ui.visibility.maximize);
    } else if (app.is_maximized() || app.is_minimized()) {
        app.show_window(ui.visibility.restore);
    }
    ui_caption_maximize_or_restore();
}

static void ui_caption_full(ui_button_t* unused(b)) {
    ui_caption_toggle_full();
}

static int64_t ui_caption_hit_test(int32_t x, int32_t y) {
    ui_point_t pt = { x, y };
    if (app.is_full_screen) {
        return ui.hit_test.client;
    } else if (ui_view.inside(&ui_caption.icon, &pt)) {
        return ui.hit_test.system_menu;
    } else {
        ui_view_for_each(&ui_caption.view, c, {
            bool ignore = c->type == ui_view_container ||
                          c->type == ui_view_spacer ||
                          c->type == ui_view_label;
            if (!ignore && ui_view.inside(c, &pt)) {
                return ui.hit_test.client;
            }
        });
        return ui.hit_test.caption;
    }
}

static void ui_caption_init(ui_view_t* v) {
    swear(v == &ui_caption.view, "caption is a singleton");
    ui_view_init_span(v);
    ui_caption.view.insets = (ui_gaps_t){ 0, 0, 0, 0 };
    ui_caption.view.hidden = false;
    app.view->character = ui_app_view_character; // ESC for full screen
    ui_view.add(&ui_caption.view,
        &ui_caption.icon,
        &ui_caption.menu,
        &ui_caption.title,
        &ui_caption.spacer,
        &ui_caption.mini,
        &ui_caption.maxi,
        &ui_caption.full,
        &ui_caption.quit,
        null);
    static const ui_gaps_t p = { .left  = 0.25, .top    = 0.25,
                                 .right = 0.25, .bottom = 0.25};
    ui_view_for_each(&ui_caption.view, c, {
        c->font = &app.fonts.H3;
        c->color = ui_colors.white;
        c->flat = true;
        c->padding = p;
    });
    ui_caption.icon.icon = app.icon;
    ui_caption.view.max_w = INT32_MAX;
    ui_caption.view.align = ui.align.top;
    ui_caption_maximize_or_restore();
}

ui_caption_t ui_caption =  {
    .view = {
        .type     = ui_view_span,
        .font     = &app.fonts.regular,
        .init     = ui_caption_init,
        .hit_test = ui_caption_hit_test,
        .hidden = true
    },
    .icon   = ui_button(ui_glyph_nbsp, 0.0, null),
    .title  = ui_label(0, ""),
    .spacer = ui_view(spacer),
    .menu   = ui_button(ui_caption_glyph_menu, 0.0, null),
    .mini   = ui_button(ui_caption_glyph_mini, 0.0, ui_caption_mini),
    .maxi   = ui_button(ui_caption_glyph_maxi, 0.0, ui_caption_maxi),
    .full   = ui_button(ui_caption_glyph_full, 0.0, ui_caption_full),
    .quit   = ui_button(ui_caption_glyph_quit, 0.0, ui_caption_quit),
};
