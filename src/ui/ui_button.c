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

static void ui_button_paint(ui_view_t* view) {
    assert(view->type == ui_view_button);
    assert(!view->hidden);
    ui_gdi.push(view->x, view->y);
    bool pressed = (view->armed ^ view->pressed) == 0;
    if (view->armed_until != 0) { pressed = true; }
    int32_t sign = 1 - pressed * 2; // -1, +1
    int32_t w = sign * view->w;
    int32_t h = sign * view->h;
    int32_t x = view->x + (int32_t)pressed * view->w;
    int32_t y = view->y + (int32_t)pressed * view->h;
    if (!view->flat || view->hover) {
        ui_gdi.gradient(x, y, w, h, ui_colors.btn_gradient_darker,
            ui_colors.btn_gradient_dark, true);
    }
    ui_color_t c = view->color;
    if (!view->flat && view->armed) {
        c = ui_colors.btn_armed;
    }else if (!view->flat && view->hover && !view->armed) {
        c = ui_colors.btn_hover_highlight;
    }
    if (view->disabled) { c = ui_colors.btn_disabled; }
    if (view->icon == null) {
        ui_font_t  f = *view->font;
        ui_point_t m = ui_gdi.measure_text(f, ui_view.nls(view));
        ui_gdi.set_text_color(c);
        ui_gdi.x = view->x + (view->w - m.x) / 2;
        ui_gdi.y = view->y + (view->h - m.y) / 2;
        f = ui_gdi.set_font(f);
        ui_gdi.text("%s", ui_view.nls(view));
        ui_gdi.set_font(f);
    } else {
        ui_gdi.draw_icon(view->x, view->y, view->w, view->h, view->icon);
    }
    const int32_t pw = ut_max(1, view->em.y / 32); // pen width
    ui_color_t color = view->armed ? ui_colors.dkgray4 : ui_colors.gray;
    if (view->hover && !view->armed) { color = ui_colors.blue; }
    if (view->disabled) { color = ui_colors.dkgray1; }
    if (!view->flat) {
        ui_pen_t p = ui_gdi.create_pen(color, pw);
        ui_gdi.set_pen(p);
        ui_gdi.set_brush(ui_gdi.brush_hollow);
        ui_gdi.rounded(view->x, view->y, view->w, view->h, view->em.y / 4, view->em.y / 4);
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

static void ui_button_trigger(ui_view_t* view) {
    assert(view->type == ui_view_button);
    assert(!view->hidden && !view->disabled);
    ui_button_t* b = (ui_button_t*)view;
    view->armed = true;
    ui_view.invalidate(view);
    ui_app.draw();
    view->armed_until = ui_app.now + 0.250;
    ui_button_callback(b);
    ui_view.invalidate(view);
}

static void ui_button_character(ui_view_t* view, const char* utf8) {
    assert(view->type == ui_view_button);
    assert(!view->hidden && !view->disabled);
    char ch = utf8[0]; // TODO: multibyte shortcuts?
    if (ui_view.is_shortcut_key(view, ch)) {
        ui_button_trigger(view);
    }
}

static void ui_button_key_pressed(ui_view_t* view, int64_t key) {
    if (ui_app.alt && ui_view.is_shortcut_key(view, key)) {
//      traceln("key: 0x%02X shortcut: %d", key, ui_view.is_shortcut_key(view, key));
        ui_button_trigger(view);
    }
}

/* processes mouse clicks and invokes callback  */

static void ui_button_mouse(ui_view_t* view, int32_t message, int64_t flags) {
    assert(view->type == ui_view_button);
    (void)flags; // unused
    assert(!view->hidden && !view->disabled);
    ui_button_t* b = (ui_button_t*)view;
    bool a = view->armed;
    bool on = false;
    if (message == ui.message.left_button_pressed ||
        message == ui.message.right_button_pressed) {
        view->armed = ui_button_hit_test(b, ui_app.mouse);
        if (view->armed) { ui_app.focus = view; }
        if (view->armed) { ui_app.show_tooltip(null, -1, -1, 0); }
    }
    if (message == ui.message.left_button_released ||
        message == ui.message.right_button_released) {
        if (view->armed) { on = ui_button_hit_test(b, ui_app.mouse); }
        view->armed = false;
    }
    if (on) { ui_button_callback(b); }
    if (a != view->armed) { ui_view.invalidate(view); }
}

static void ui_button_measure(ui_view_t* view) {
    assert(view->type == ui_view_button || view->type == ui_view_label);
    ui_view.measure(view);
    const int32_t em2  = ut_max(1, view->em.x / 2);
    view->w = view->w;
    view->h = view->h + em2;
    if (view->w < view->h) { view->w = view->h; }
}

void ui_view_init_button(ui_view_t* view) {
    assert(view->type == ui_view_button);
    ui_view_init(view);
    view->mouse       = ui_button_mouse;
    view->measure     = ui_button_measure;
    view->paint       = ui_button_paint;
    view->character   = ui_button_character;
    view->every_100ms = ui_button_every_100ms;
    view->key_pressed = ui_button_key_pressed;
    ui_view.set_text(view, view->text);
    ui_view.localize(view);
    view->color = ui_colors.btn_text;
}

void ui_button_init(ui_button_t* b, const char* label, fp32_t ems,
        void (*callback)(ui_button_t* b)) {
    b->type = ui_view_button;
    strprintf(b->text, "%s", label);
    b->callback = callback;
    b->min_w_em = ems;
    ui_view_init_button(b);
}
