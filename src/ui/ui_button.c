#include "ut/ut.h"
#include "ui/ui.h"

static void ui_button_every_100ms(ui_view_t* v) { // every 100ms
    assert(v->type == ui_view_button);
    if (v->armed_until != 0 && app.now > v->armed_until) {
        v->armed_until = 0;
        v->armed = false;
        ui_view.invalidate(v);
    }
}

static void ui_button_paint(ui_view_t* view) {
    assert(view->type == ui_view_button);
    assert(!view->hidden);
    gdi.push(view->x, view->y);
    bool pressed = (view->armed ^ view->pressed) == 0;
    if (view->armed_until != 0) { pressed = true; }
    int32_t sign = 1 - pressed * 2; // -1, +1
    int32_t w = sign * view->w;
    int32_t h = sign * view->h;
    int32_t x = view->x + (int)pressed * view->w;
    int32_t y = view->y + (int)pressed * view->h;
    if (!view->flat || view->hover) {
        gdi.gradient(x, y, w, h, colors.btn_gradient_darker,
            colors.btn_gradient_dark, true);
    }
    ui_color_t c = view->color;
    if (!view->flat && view->armed) {
        c = colors.btn_armed;
    }else if (!view->flat && view->hover && !view->armed) {
        c = colors.btn_hover_highlight;
    }
    if (view->disabled) { c = colors.btn_disabled; }
    if (view->icon == null) {
        ui_font_t  f = view->font != null ? *view->font : app.fonts.regular;
        ui_point_t m = gdi.measure_text(f, ui_view.nls(view));
        gdi.set_text_color(c);
        gdi.x = view->x + (view->w - m.x) / 2;
        gdi.y = view->y + (view->h - m.y) / 2;
        f = gdi.set_font(f);
        gdi.text("%s", ui_view.nls(view));
        gdi.set_font(f);
    } else {
        gdi.draw_icon(view->x, view->y, view->w, view->h, view->icon);
    }
    const int32_t pw = ut_max(1, view->em.y / 32); // pen width
    ui_color_t color = view->armed ? colors.dkgray4 : colors.gray;
    if (view->hover && !view->armed) { color = colors.blue; }
    if (view->disabled) { color = colors.dkgray1; }
    if (!view->flat) {
        ui_pen_t p = gdi.create_pen(color, pw);
        gdi.set_pen(p);
        gdi.set_brush(gdi.brush_hollow);
        gdi.rounded(view->x, view->y, view->w, view->h, view->em.y / 4, view->em.y / 4);
        gdi.delete_pen(p);
    }
    gdi.pop();
}

static bool ui_button_hit_test(ui_button_t* b, ui_point_t pt) {
    assert(b->view.type == ui_view_button);
    pt.x -= b->view.x;
    pt.y -= b->view.y;
    return 0 <= pt.x && pt.x < b->view.w && 0 <= pt.y && pt.y < b->view.h;
}

static void ui_button_callback(ui_button_t* b) {
    assert(b->view.type == ui_view_button);
    app.show_tooltip(null, -1, -1, 0);
    if (b->cb != null) { b->cb(b); }
}

static void ui_button_trigger(ui_view_t* view) {
    assert(view->type == ui_view_button);
    assert(!view->hidden && !view->disabled);
    ui_button_t* b = (ui_button_t*)view;
    view->armed = true;
    ui_view.invalidate(view);
    app.draw();
    view->armed_until = app.now + 0.250;
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
    if (app.alt && ui_view.is_shortcut_key(view, key)) {
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
        view->armed = ui_button_hit_test(b, app.mouse);
        if (view->armed) { app.focus = view; }
        if (view->armed) { app.show_tooltip(null, -1, -1, 0); }
    }
    if (message == ui.message.left_button_released ||
        message == ui.message.right_button_released) {
        if (view->armed) { on = ui_button_hit_test(b, app.mouse); }
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

void ui_button_init_(ui_view_t* view) {
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
    view->color = colors.btn_text;
}

void ui_button_init(ui_button_t* b, const char* label, fp64_t ems,
        void (*cb)(ui_button_t* b)) {
    static_assert(offsetof(ui_button_t, view) == 0, "offsetof(.view)");
    b->view.type = ui_view_button;
    strprintf(b->view.text, "%s", label);
    b->cb = cb;
    b->view.width = ems;
    ui_button_init_(&b->view);
}
