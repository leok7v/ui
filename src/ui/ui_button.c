#include "ut/ut.h"
#include "ui/ui.h"

static void ui_button_every_100ms(ui_view_t* v) { // every 100ms
    assert(v->type == ui_view_button);
    if (v->armed_until != 0 && ui_app.now > v->armed_until) {
        v->armed_until = 0;
        v->armed = false;
        ui_view.invalidate(v);
    }
}

// TODO: generalize and move to ui_colors.c to avoid slider dup

static ui_color_t ui_button_gradient_darker(void) {
    if (ui_theme.are_apps_dark()) {
        ui_color_t c = ui_app.get_color(ui_color_id_button_face);
        // 0x1E1E1E
        // 0x333333 button face
        ui_color_t d0 = ui_colors.darken(c, 0.50f);
        return d0;
//      traceln("ui_color_id_button_face: 0x%06X", c);
//      traceln("ui_colors.btn_gradient_darker: 0x%06X", ui_colors.btn_gradient_darker);
//      return ui_colors.btn_gradient_darker;
    } else {
        ui_color_t c = ui_app.get_color(ui_color_id_button_face);
        uint32_t r = ui_color_r(c);
        uint32_t g = ui_color_r(c);
        uint32_t b = ui_color_r(c);
        r = ut_max(0, ut_min(0xFF, (uint32_t)(r * 0.75)));
        g = ut_max(0, ut_min(0xFF, (uint32_t)(g * 0.75)));
        b = ut_max(0, ut_min(0xFF, (uint32_t)(b * 0.75)));
        ui_color_t d = ui_rgb(r, g, b);
        traceln("c: 0x%06X -> 0x%06X", c, d);
        return d;
    }
}

static ui_color_t ui_button_gradient_dark(void) {
    if (ui_theme.are_apps_dark()) {
        // 0x302D2D
        ui_color_t c = ui_app.get_color(ui_color_id_button_face);
        ui_color_t d1 = ui_colors.darken(c, 0.125f);
        return d1;
//      traceln("ui_colors.btn_gradient_dark: 0x%06X", ui_colors.btn_gradient_dark);
//      return ui_colors.btn_gradient_dark;
    } else {
        ui_color_t c = ui_app.get_color(ui_color_id_button_face);
        uint32_t r = ui_color_r(c);
        uint32_t g = ui_color_r(c);
        uint32_t b = ui_color_r(c);
        r = ut_max(0, ut_min(0xFF, (uint32_t)(r * 1.25)));
        g = ut_max(0, ut_min(0xFF, (uint32_t)(g * 1.25)));
        b = ut_max(0, ut_min(0xFF, (uint32_t)(b * 1.25)));
        ui_color_t d = ui_rgb(r, g, b);
//      traceln("c: 0%06X -> 0%06X", c, d);
        return d;
    }
}

static void ui_button_paint(ui_view_t* v) {
    assert(v->type == ui_view_button);
    assert(!v->hidden);
    ui_gdi.push(v->x, v->y);
    bool pressed = (v->armed ^ v->pressed) == 0;
    if (v->armed_until != 0) { pressed = true; }
    int32_t sign = 1 - pressed * 2; // -1, +1
    int32_t w = sign * (v->w - 2);
    int32_t h = sign * (v->h - 2);
    int32_t x = (v->x + (int32_t)pressed * v->w) + sign;
    int32_t y = (v->y + (int32_t)pressed * v->h) + sign;
    fp32_t d = ui_theme.are_apps_dark() ? 0.50f : 0.25f;
    ui_color_t d0 = ui_colors.darken(v->background, d);
    d /= 4;
    ui_color_t d1 = ui_colors.darken(v->background, d);
    if (!v->flat || v->hover) {
        ui_gdi.gradient(x, y, w, h, d0, d1, true);
    }
    ui_color_t c = v->color;
    if (!v->flat && v->armed) {
        c = ui_colors.btn_armed;
    }else if (!v->flat && v->hover && !v->armed) {
        c = ui_app.get_color(ui_color_id_hot_tracking_color);
    }
    if (v->disabled) { c = ui_app.get_color(ui_color_id_gray_text); }
    if (v->icon == null) {
        ui_font_t f = v->fm->font;
        ui_wh_t m = ui_gdi.measure_text(f, ui_view.nls(v));
        ui_gdi.set_text_color(c);
        ui_gdi.x = v->x + (v->w - m.w) / 2;
        ui_gdi.y = v->y + (v->h - m.h) / 2;
        f = ui_gdi.set_font(f);
        ui_gdi.text("%s", ui_view.nls(v));
        ui_gdi.set_font(f);
    } else {
        ui_gdi.draw_icon(v->x, v->y, v->w, v->h, v->icon);
    }
    const int32_t pw = ut_max(1, v->fm->em.h / 8); // pen width
    ui_color_t color = v->armed ? ui_colors.lighten(v->background, 0.125f) : d1;
    if (v->hover && !v->armed) { color = ui_colors.blue; }
    if (v->disabled) { color = ui_colors.dkgray1; }
    if (!v->flat) {
        ui_pen_t p = ui_gdi.create_pen(color, pw);
        ui_gdi.set_pen(p);
        ui_gdi.set_brush(ui_gdi.brush_hollow);
        int32_t r = ut_max(3, v->fm->em.h / 4);
        if (r % 2 == 0) { r++; }
        ui_gdi.rounded(v->x, v->y, v->w, v->h, r, r);
        ui_gdi.delete_pen(p);
    }
    ui_gdi.pop();
}

static bool ui_button_hit_test(ui_button_t* b, ui_point_t pt) {
    assert(b->type == ui_view_button);
    return ui_view.inside(b, &pt);
}

static void ui_button_callback(ui_button_t* b) {
    assert(b->type == ui_view_button);
    ui_app.show_tooltip(null, -1, -1, 0);
    if (b->callback != null) { b->callback(b); }
}

static void ui_button_trigger(ui_view_t* v) {
    assert(v->type == ui_view_button);
    assert(!v->hidden && !v->disabled);
    ui_button_t* b = (ui_button_t*)v;
    v->armed = true;
    ui_view.invalidate(v);
    ui_app.draw();
    v->armed_until = ui_app.now + 0.250;
    ui_button_callback(b);
    ui_view.invalidate(v);
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
//      traceln("key: 0x%02X shortcut: %d", key, ui_view.is_shortcut_key(v, key));
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
        if (v->armed) { ui_app.show_tooltip(null, -1, -1, 0); }
    }
    if (message == ui.message.left_button_released ||
        message == ui.message.right_button_released) {
        if (v->armed) { on = ui_button_hit_test(b, ui_app.mouse); }
        v->armed = false;
    }
    if (on) { ui_button_callback(b); }
    if (a != v->armed) { ui_view.invalidate(v); }
}

static void ui_button_measured(ui_view_t* v) {
    assert(v->type == ui_view_button || v->type == ui_view_label);
    if (v->w < v->h) { v->w = v->h; } // make square is narrow letter like "I"
}

void ui_view_init_button(ui_view_t* v) {
    assert(v->type == ui_view_button);
    v->mouse         = ui_button_mouse;
    v->measured      = ui_button_measured;
    v->paint         = ui_button_paint;
    v->character     = ui_button_character;
    v->every_100ms   = ui_button_every_100ms;
    v->key_pressed   = ui_button_key_pressed;
    v->color_id      = ui_color_id_window_text;
    v->background_id = ui_color_id_button_face;
    ui_view.set_text(v, v->text);
    ui_view.localize(v);
}

void ui_button_init(ui_button_t* b, const char* label, fp32_t ems,
        void (*callback)(ui_button_t* b)) {
    b->type = ui_view_button;
    strprintf(b->text, "%s", label);
    b->callback = callback;
    b->min_w_em = ems;
    ui_view_init_button(b);
}
