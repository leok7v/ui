#include "ut/ut.h"
#include "ui/ui.h"

static void ui_button_every_100ms(ui_view_t* v) { // every 100ms
    assert(v->type == ui_view_button);
    if (v->p.armed_until != 0 && ui_app.now > v->p.armed_until) {
        v->p.armed_until = 0;
        v->armed = false;
        ui_view.invalidate(v, null);
        traceln("ui_view.invalidate(v, null)");
    }
}

static void ui_button_paint(ui_view_t* v) {
    assert(v->type == ui_view_button);
    assert(!v->hidden);
    if (strcmp(v->p.text, "&Button") == 0 && v->fm == &ui_app.fm.H1) {
        traceln("v->fm: .h: %d .a:%d .d:%d .b:%d",
            v->fm->height, v->fm->ascent, v->fm->descent, v->fm->baseline);
    }
    bool pressed = (v->armed ^ v->pressed) == 0;
    if (v->p.armed_until != 0) { pressed = true; }
    int32_t w = v->w;
    int32_t h = v->h;
    int32_t x = v->x;
    int32_t y = v->y;
    const int32_t r = (0x1 | ut_max(3, v->fm->em.h / 4));  // odd radius
    const fp32_t d = ui_theme.is_app_dark() ? 0.50f : 0.25f;
    ui_color_t d0 = ui_colors.darken(v->background, d);
    const fp32_t d2 = d / 2;
    if (v->flat) {
        ui_color_t d1 = ui_theme.is_app_dark() ?
                ui_colors.lighten(v->background, d2) :
                ui_colors.darken(v->background,  d2);
        if (!pressed) {
            ui_gdi.gradient(x, y, w, h, d0, d1, true);
        } else {
            ui_gdi.gradient(x, y, w, h, d1, d0, true);
        }
    } else {
        // `bc` border color
        ui_color_t bc = ui_colors.get_color(ui_color_id_gray_text);
        if (v->armed) { bc = ui_colors.lighten(bc, 0.125f); }
        if (v->disabled) { bc = ui_color_rgb(30, 30, 30); } // TODO: hardcoded
        if (v->hover && !v->armed) {
            bc = ui_colors.get_color(ui_color_id_hot_tracking);
        }
        ui_color_t d1 = ui_colors.darken(v->background, d2);
        ui_color_t fc = ui_colors.interpolate(d0, d1, 0.5f); // fill color
        ui_gdi.rounded(v->x, v->y, v->w, v->h, r, bc, fc);
    }
    const ui_ltrb_t i = ui_view.gaps(v, &v->insets);
    const int32_t t_w = v->w - i.left - i.right;
    const int32_t t_h = v->h - i.top - i.bottom;
    if (v->icon == null) {
        const ui_wh_t wh = ui_view.text_metrics(0, 0,
                false, 0, v->fm, "%s", ui_view.string(v));
        int32_t t_x = (t_w - wh.w) / 2;
        const int32_t h_align = v->text_align & ~(ui.align.top|ui.align.bottom);
        const int32_t v_align = v->text_align & ~(ui.align.left|ui.align.right);
        if (h_align & ui.align.left) {
            t_x = 0;
        } else if (h_align & ui.align.right) {
            t_x = t_w - wh.w - i.right;
        }
        assert(wh.h == v->fm->height, "wh.h:%d fm.height:%d",
               wh.h, v->fm->height);
        int32_t t_y = (t_h - v->fm->ascent) / 2 - (v->fm->baseline  - v->fm->ascent);
    int32_t t_y_1 = (t_h - wh.h) / 2;
if (strcmp(v->p.text, "&Button") == 0 && v->fm == &ui_app.fm.H1) {
    traceln("t_y:%d t_y_1:%d", t_y, t_y_1);
}
        if (v_align & ui.align.top) {
            t_y = 0;
        } else if (v_align & ui.align.bottom) {
            t_y = t_h - wh.h - i.bottom;
        }
        const int32_t tx = v->x + i.left + t_x;
        const int32_t ty = v->y + i.top  + t_y;
        ui_color_t c = v->color;
//      traceln("v->hover: %d armed: %d c: %08X", v->hover, v->armed, (uint32_t)c);
        if (v->hover && !v->armed) {
    //      c = ui_theme.is_app_dark() ? ui_colors.white : ui_colors.black;
            c = ui_theme.is_app_dark() ? ui_color_rgb(0xFF, 0xE0, 0xE0) :
                                         ui_color_rgb(0x00, 0x40, 0xFF);
//          traceln("text_color: %08X", c);
        }
        if (v->disabled) { c = ui_colors.get_color(ui_color_id_gray_text); }
//      traceln("text_color: %08X", (uint32_t)c);
static bool debug;
        debug = true;
        if (v->debug || debug) {
            const int32_t y_0 = y + i.top;
            const int32_t y_b = y_0 + v->fm->baseline;
            const int32_t y_a = y_b - v->fm->ascent;
            const int32_t y_h = y_0 + v->fm->height;
            const int32_t y_x = y_b - v->fm->x_height;
            const int32_t y_d = y_b + v->fm->descent;
            ui_gdi.line(x, y_0, x + w, y_0, ui_colors.orange);
            ui_gdi.line(x, y_a, x + w, y_a, ui_colors.green);
            ui_gdi.line(x, y_x, x + w, y_x, ui_colors.orange);
            ui_gdi.line(x, y_b, x + w, y_b, ui_colors.red);
            // next two lines overlap:
            ui_gdi.line(x, y_d, x + w / 2, y_d, ui_colors.blue);
            ui_gdi.line(x + w / 2, y_h, x + w, y_h, ui_colors.yellow);
        }
        const ui_gdi_ta_t ta = { .fm = v->fm, .color = c };
        ui_gdi.text(&ta, tx, ty, "%s", ui_view.string(v));
    } else {
        ui_gdi.icon(v->x + i.left, v->y + i.top, t_w, t_h, v->icon);
    }
}

static bool ui_button_hit_test(ui_button_t* b, ui_point_t pt) {
    assert(b->type == ui_view_button);
    return ui_view.inside(b, &pt);
}

static void ui_button_callback(ui_button_t* b) {
    assert(b->type == ui_view_button);
    ui_app.show_hint(null, -1, -1, 0);
    if (b->callback != null) { b->callback(b); }
}

static void ui_button_trigger(ui_view_t* v) {
    assert(v->type == ui_view_button);
    assert(!v->hidden && !v->disabled);
    ui_button_t* b = (ui_button_t*)v;
    v->armed = true;
    ui_view.invalidate(v, null);
    ui_app.draw();
    v->p.armed_until = ui_app.now + 0.250;
    ui_button_callback(b);
    ui_view.invalidate(v, null);
}

static void ui_button_character(ui_view_t* v, const char* utf8) {
    assert(v->type == ui_view_button);
    assert(!v->hidden && !v->disabled);
    char ch = utf8[0]; // TODO: multibyte shortcuts?
    if (ui_view.is_shortcut_key(v, ch)) {
        ui_button_trigger(v);
    }
}

static void ui_button_key_pressed(ui_view_t* view, int64_t key) {
    if (ui_app.alt && ui_view.is_shortcut_key(view, key)) {
        ui_button_trigger(view);
    }
}

/* processes mouse clicks and invokes callback  */

static void ui_button_mouse(ui_view_t* v, int32_t message, int64_t flags) {
    assert(v->type == ui_view_button);
    (void)flags; // unused
    assert(!v->hidden && !v->disabled);
    ui_button_t* b = (ui_button_t*)v;
    bool a = v->armed;
    bool on = false;
    if (message == ui.message.left_button_pressed ||
        message == ui.message.right_button_pressed) {
        v->armed = ui_button_hit_test(b, ui_app.mouse);
        if (v->armed) { ui_app.focus = v; }
        if (v->armed) { ui_app.show_hint(null, -1, -1, 0); }
    }
    if (message == ui.message.left_button_released ||
        message == ui.message.right_button_released) {
        if (v->armed) { on = ui_button_hit_test(b, ui_app.mouse); }
        v->armed = false;
    }
    if (on) { ui_button_callback(b); }
    if (a != v->armed) { ui_view.invalidate(v, null); }
}

static void ui_button_measure(ui_view_t* v) {
    assert(v->type == ui_view_button || v->type == ui_view_label);
    ui_view.measure_text(v);
    if (v->w < v->h) { v->w = v->h; } // make square is narrow letter like "I"
}

void ui_view_init_button(ui_view_t* v) {
    assert(v->type == ui_view_button);
    v->mouse         = ui_button_mouse;
    v->measure       = ui_button_measure;
    v->paint         = ui_button_paint;
    v->character     = ui_button_character;
    v->every_100ms   = ui_button_every_100ms;
    v->key_pressed   = ui_button_key_pressed;
    v->color_id      = ui_color_id_button_text;
    v->background_id = ui_color_id_button_face;
}

void ui_button_init(ui_button_t* b, const char* label, fp32_t ems,
        void (*callback)(ui_button_t* b)) {
    b->type = ui_view_button;
    ui_view.set_text(b, "%s", label);
    b->callback = callback;
    b->min_w_em = ems;
    ui_view_init_button(b);
}
