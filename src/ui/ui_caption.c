/* Copyright (c) Dmitry "Leo" Kuznetsov 2021-24 see LICENSE for details */
#include "ut/ut.h"
#include "ui/ui.h"

#pragma push_macro("ui_caption_glyph_rest")
#pragma push_macro("ui_caption_glyph_menu")
#pragma push_macro("ui_caption_glyph_dark")
#pragma push_macro("ui_caption_glyph_light")
#pragma push_macro("ui_caption_glyph_mini")
#pragma push_macro("ui_caption_glyph_maxi")
#pragma push_macro("ui_caption_glyph_full")
#pragma push_macro("ui_caption_glyph_quit")

#define ui_caption_glyph_rest  rt_glyph_white_square_with_upper_right_quadrant // instead of rt_glyph_desktop_window
#define ui_caption_glyph_menu  rt_glyph_trigram_for_heaven
#define ui_caption_glyph_dark  rt_glyph_crescent_moon
#define ui_caption_glyph_light rt_glyph_white_sun_with_rays
#define ui_caption_glyph_mini  rt_glyph_minimize
#define ui_caption_glyph_maxi  rt_glyph_white_square_with_lower_left_quadrant // instead of rt_glyph_maximize
#define ui_caption_glyph_full  rt_glyph_square_four_corners
#define ui_caption_glyph_quit  rt_glyph_cancellation_x

static void ui_caption_toggle_full(void) {
    ui_app.full_screen(!ui_app.is_full_screen);
    ui_caption.view.state.hidden = ui_app.is_full_screen;
    ui_app.request_layout();
}

static void ui_caption_esc_full_screen(ui_view_t* v, const char utf8[]) {
    rt_swear(v == ui_caption.view.parent);
    // TODO: inside ui_app.c instead of here?
    if (utf8[0] == 033 && ui_app.is_full_screen) { ui_caption_toggle_full(); }
}

static void ui_caption_quit(ui_button_t* rt_unused(b)) {
    ui_app.close();
}

static void ui_caption_mini(ui_button_t* rt_unused(b)) {
    ui_app.show_window(ui.visibility.minimize);
}

static void ui_caption_mode_appearance(void) {
    if (ui_theme.is_app_dark()) {
        ui_view.set_text(&ui_caption.mode, "%s", ui_caption_glyph_light);
        rt_str_printf(ui_caption.mode.hint, "%s", rt_nls.str("Switch to Light Mode"));
    } else {
        ui_view.set_text(&ui_caption.mode, "%s", ui_caption_glyph_dark);
        rt_str_printf(ui_caption.mode.hint, "%s", rt_nls.str("Switch to Dark Mode"));
    }
}

static void ui_caption_mode(ui_button_t* rt_unused(b)) {
    bool was_dark = ui_theme.is_app_dark();
    ui_app.light_mode =  was_dark;
    ui_app.dark_mode  = !was_dark;
    ui_theme.refresh();
    ui_caption_mode_appearance();
}

static void ui_caption_maximize_or_restore(void) {
    ui_view.set_text(&ui_caption.maxi, "%s",
        ui_app.is_maximized() ?
        ui_caption_glyph_rest : ui_caption_glyph_maxi);
    rt_str_printf(ui_caption.maxi.hint, "%s",
        ui_app.is_maximized() ?
        rt_nls.str("Restore") : rt_nls.str("Maximize"));
    // non-decorated windows on Win32 are "popup" style
    // that cannot be maximized. Full screen will serve
    // the purpose of maximization.
    ui_caption.maxi.state.hidden = ui_app.no_decor;
}

static void ui_caption_maxi(ui_button_t* rt_unused(b)) {
    if (!ui_app.is_maximized()) {
        ui_app.show_window(ui.visibility.maximize);
    } else if (ui_app.is_maximized() || ui_app.is_minimized()) {
        ui_app.show_window(ui.visibility.restore);
    }
    ui_caption_maximize_or_restore();
}

static void ui_caption_full(ui_button_t* rt_unused(b)) {
    ui_caption_toggle_full();
}

static int64_t ui_caption_hit_test(const ui_view_t* v, ui_point_t pt) {
    rt_swear(v == &ui_caption.view);
    rt_assert(ui_view.inside(v, &pt));
//  rt_println("%d,%d ui_caption.icon: %d,%d %dx%d inside: %d",
//      x, y,
//      ui_caption.icon.x, ui_caption.icon.y,
//      ui_caption.icon.w, ui_caption.icon.h,
//      ui_view.inside(&ui_caption.icon, &pt));
    if (ui_app.is_full_screen) {
        return ui.hit_test.client;
    } else if (!ui_caption.icon.state.hidden &&
                ui_view.inside(&ui_caption.icon, &pt)) {
        return ui.hit_test.system_menu;
    } else {
        ui_view_for_each(&ui_caption.view, c, {
            bool ignore = c->type == ui_view_stack ||
                          c->type == ui_view_spacer ||
                          c->type == ui_view_label;
            if (!ignore && ui_view.inside(c, &pt)) {
                return ui.hit_test.client;
            }
        });
        return ui.hit_test.caption;
    }
}

static ui_color_t ui_caption_color(void) {
    ui_color_t c = ui_app.is_active() ?
        ui_colors.get_color(ui_color_id_active_title) :
        ui_colors.get_color(ui_color_id_inactive_title);
    return c;
}

static const ui_margins_t ui_caption_button_button_padding =
    { .left  = 0.25,  .top    = 0.0,
      .right = 0.25,  .bottom = 0.0};

static void ui_caption_button_measure(ui_view_t* v) {
    rt_assert(v->type == ui_view_button);
    ui_view.measure_control(v);
    const int32_t dx = ui_app.caption_height - v->w;
    const int32_t dy = ui_app.caption_height - v->h;
    v->w += dx;
    v->h += dy;
    v->text.xy.x += dx / 2;
    v->text.xy.y += dy / 2;
    v->padding = ui_caption_button_button_padding;
}

static void ui_caption_button_icon_paint(ui_view_t* v) {
    int32_t w = v->w;
    int32_t h = v->h;
    while (h > 16 && (h & (h - 1)) != 0) { h--; }
    w = h;
    int32_t dx = (v->w - w) / 2;
    int32_t dy = (v->h - h) / 2;
    ui_gdi.icon(v->x + dx, v->y + dy, w, h, v->icon);
}

static void ui_caption_prepare(ui_view_t* rt_unused(v)) {
    ui_caption.title.state.hidden = false;
}

static void ui_caption_measured(ui_view_t* v) {
    // remeasure all child buttons with hard override:
    int32_t w = 0;
    ui_view_for_each(v, it, {
        if (it->type == ui_view_button) {
            it->fm = &ui_app.fm.mono.normal;
            it->flat = true;
            ui_caption_button_measure(it);
        }
        if (!it->state.hidden) {
            const ui_ltrb_t p = ui_view.margins(it, &it->padding);
            w += it->w + p.left + p.right;
        }
    });
    const ui_ltrb_t p = ui_view.margins(v, &v->padding);
    w += p.left + p.right;
    // do not show title if there is not enough space
    ui_caption.title.state.hidden = w > ui_app.root->w;
    v->w = ui_app.root->w;
    const ui_ltrb_t insets = ui_view.margins(v, &v->insets);
    v->h = insets.top + ui_app.caption_height + insets.bottom;
}

static void ui_caption_composed(ui_view_t* v) {
    v->x = ui_app.root->x;
    v->y = ui_app.root->y;
}

static void ui_caption_paint(ui_view_t* v) {
    ui_color_t background = ui_caption_color();
    ui_gdi.fill(v->x, v->y, v->w, v->h, background);
}

static void ui_caption_init(ui_view_t* v) {
    rt_swear(v == &ui_caption.view, "caption is a singleton");
    ui_view_init_span(v);
    ui_caption.view.insets = (ui_margins_t){ 0.125, 0.0, 0.125, 0.0 };
    ui_caption.view.state.hidden = false;
    v->parent->character = ui_caption_esc_full_screen; // ESC for full screen
    ui_view.add(&ui_caption.view,
        &ui_caption.icon,
        &ui_caption.menu,
        &ui_caption.title,
        &ui_caption.spacer,
        &ui_caption.mode,
        &ui_caption.mini,
        &ui_caption.maxi,
        &ui_caption.full,
        &ui_caption.quit,
        null);
    ui_caption.view.color_id = ui_color_id_window_text;
    static const ui_margins_t p0 = { .left  = 0.0,   .top    = 0.0,
                                     .right = 0.0,   .bottom = 0.0};
    static const ui_margins_t pd = { .left  = 0.25,  .top    = 0.0,
                                     .right = 0.25,  .bottom = 0.0};
    static const ui_margins_t in = { .left  = 0.0,   .top    = 0.0,
                                     .right = 0.0,   .bottom = 0.0};
    ui_view_for_each(&ui_caption.view, c, {
        c->fm = &ui_app.fm.prop.normal;
        c->color_id = ui_caption.view.color_id;
        if (c->type != ui_view_button) {
            c->padding = pd;
        }
        c->insets  = in;
        c->h = ui_app.caption_height;
        c->min_w_em = 0.5f;
        c->min_h_em = 0.5f;
    });
    rt_str_printf(ui_caption.menu.hint, "%s", rt_nls.str("Menu"));
    rt_str_printf(ui_caption.mode.hint, "%s", rt_nls.str("Switch to Light Mode"));
    rt_str_printf(ui_caption.mini.hint, "%s", rt_nls.str("Minimize"));
    rt_str_printf(ui_caption.maxi.hint, "%s", rt_nls.str("Maximize"));
    rt_str_printf(ui_caption.full.hint, "%s", rt_nls.str("Full Screen (ESC to restore)"));
    rt_str_printf(ui_caption.quit.hint, "%s", rt_nls.str("Close"));
    ui_caption.icon.icon     = ui_app.icon;
    ui_caption.icon.padding  = p0;
    ui_caption.icon.paint    = ui_caption_button_icon_paint;
    ui_caption.view.align    = ui.align.left;
    ui_caption.view.prepare  = ui_caption_prepare;
    ui_caption.view.measured = ui_caption_measured;
    ui_caption.view.composed = ui_caption_composed;
    ui_view.set_text(&ui_caption.view, "#ui_caption"); // for debugging
    ui_caption_maximize_or_restore();
    ui_caption.view.paint = ui_caption_paint;
    ui_caption_mode_appearance();
    ui_caption.icon.debug.id = "#caption.icon";
    ui_caption.menu.debug.id = "#caption.menu";
    ui_caption.mode.debug.id = "#caption.mode";
    ui_caption.mini.debug.id = "#caption.mini";
    ui_caption.maxi.debug.id = "#caption.maxi";
    ui_caption.full.debug.id = "#caption.full";
    ui_caption.quit.debug.id = "#caption.quit";
    ui_caption.title.debug.id  = "#caption.title";
    ui_caption.spacer.debug.id = "#caption.spacer";

}

ui_caption_t ui_caption =  {
    .view = {
        .type     = ui_view_span,
        .fm       = &ui_app.fm.prop.normal,
        .init     = ui_caption_init,
        .hit_test = ui_caption_hit_test,
        .state.hidden = true
    },
    .icon   = ui_button(rt_glyph_nbsp, 0.0, null),
    .title  = ui_label(0, ""),
    .spacer = ui_view(spacer),
    .menu   = ui_button(ui_caption_glyph_menu, 0.0, null),
    .mode   = ui_button(ui_caption_glyph_mini, 0.0, ui_caption_mode),
    .mini   = ui_button(ui_caption_glyph_mini, 0.0, ui_caption_mini),
    .maxi   = ui_button(ui_caption_glyph_maxi, 0.0, ui_caption_maxi),
    .full   = ui_button(ui_caption_glyph_full, 0.0, ui_caption_full),
    .quit   = ui_button(ui_caption_glyph_quit, 0.0, ui_caption_quit),
};

#pragma pop_macro("ui_caption_glyph_rest")
#pragma pop_macro("ui_caption_glyph_menu")
#pragma pop_macro("ui_caption_glyph_dark")
#pragma pop_macro("ui_caption_glyph_light")
#pragma pop_macro("ui_caption_glyph_mini")
#pragma pop_macro("ui_caption_glyph_maxi")
#pragma pop_macro("ui_caption_glyph_full")
#pragma pop_macro("ui_caption_glyph_quit")
