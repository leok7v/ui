#include "ut/ut.h"
#include "ui/ui.h"

static void ui_slider_measure(ui_view_t* v) {
    assert(v->type == ui_view_slider);
    ui_view.measure(v);
    ui_slider_t* r = (ui_slider_t*)v;
    assert(r->inc.view.w == r->dec.view.w && r->inc.view.h == r->dec.view.h);
    const int32_t em = v->em.x;
    ui_font_t f = v->font != null ? *v->font : app.fonts.regular;
    const int32_t w = (int)(v->width * v->em.x);
    r->tm = gdi.measure_text(f, ui_view.nls(v), r->value_max);
    if (w > r->tm.x) { r->tm.x = w; }
    v->w = r->dec.view.w + r->tm.x + r->inc.view.w + em * 2;
    v->h = r->inc.view.h;
}

static void ui_slider_layout(ui_view_t* v) {
    assert(v->type == ui_view_slider);
    ui_slider_t* r = (ui_slider_t*)v;
    assert(r->inc.view.w == r->dec.view.w && r->inc.view.h == r->dec.view.h);
    const int32_t em = v->em.x;
    r->dec.view.x = v->x;
    r->dec.view.y = v->y;
    r->inc.view.x = v->x + r->dec.view.w + r->tm.x + em * 2;
    r->inc.view.y = v->y;
}

static void ui_slider_paint(ui_view_t* v) {
    assert(v->type == ui_view_slider);
    ui_slider_t* r = (ui_slider_t*)v;
    gdi.push(v->x, v->y);
    gdi.set_clip(v->x, v->y, v->w, v->h);
    const int32_t em = v->em.x;
    const int32_t em2  = ut_max(1, em / 2);
    const int32_t em4  = ut_max(1, em / 8);
    const int32_t em8  = ut_max(1, em / 8);
    const int32_t em16 = ut_max(1, em / 16);
    gdi.set_brush(gdi.brush_color);
    ui_pen_t pen_grey45 = gdi.create_pen(colors.dkgray3, em16);
    gdi.set_pen(pen_grey45);
    gdi.set_brush_color(colors.dkgray3);
    const int32_t x = v->x + r->dec.view.w + em2;
    const int32_t y = v->y;
    const int32_t w = r->tm.x + em;
    const int32_t h = v->h;
    gdi.rounded(x - em8, y, w + em4, h, em4, em4);
    gdi.gradient(x, y, w, h / 2,
        colors.dkgray3, colors.btn_gradient_darker, true);
    gdi.gradient(x, y + h / 2, w, v->h - h / 2,
        colors.btn_gradient_darker, colors.dkgray3, true);
    gdi.set_brush_color(colors.dkgreen);
    ui_pen_t pen_grey30 = gdi.create_pen(colors.dkgray1, em16);
    gdi.set_pen(pen_grey30);
    const fp64_t range = (fp64_t)r->value_max - (fp64_t)r->value_min;
    fp64_t vw = (fp64_t)(r->tm.x + em) * (r->value - r->value_min) / range;
    gdi.rect(x, v->y, (int32_t)(vw + 0.5), v->h);
    gdi.x += r->dec.view.w + em;
    const char* format = nls.str(v->text);
    gdi.text(format, r->value);
    gdi.set_clip(0, 0, 0, 0);
    gdi.delete_pen(pen_grey30);
    gdi.delete_pen(pen_grey45);
    gdi.pop();
}

static void ui_slider_mouse(ui_view_t* v, int32_t message, int64_t f) {
    if (!v->hidden && !v->disabled) {
        assert(v->type == ui_view_slider);
        ui_slider_t* r = (ui_slider_t*)v;
        bool drag = message == ui.message.mouse_move &&
            (f & (ui.mouse.button.left|ui.mouse.button.right)) != 0;
        if (message == ui.message.left_button_pressed ||
            message == ui.message.right_button_pressed || drag) {
            const int32_t x = app.mouse.x - v->x - r->dec.view.w;
            const int32_t y = app.mouse.y - v->y;
            const int32_t x0 = v->em.x / 2;
            const int32_t x1 = r->tm.x + v->em.x;
            if (x0 <= x && x < x1 && 0 <= y && y < v->h) {
                app.focus = v;
                const fp64_t range = (fp64_t)r->value_max - (fp64_t)r->value_min;
                fp64_t val = ((fp64_t)x - x0) * range / (fp64_t)(x1 - x0 - 1);
                int32_t vw = (int32_t)(val + r->value_min + 0.5);
                r->value = ut_min(ut_max(vw, r->value_min), r->value_max);
                if (r->cb != null) { r->cb(r); }
                ui_view.invalidate(v);
            }
        }
    }
}

static void ui_slider_inc_dec_value(ui_slider_t* r, int32_t sign, int32_t mul) {
    if (!r->view.hidden && !r->view.disabled) {
        // full 0x80000000..0x7FFFFFFF (-2147483648..2147483647) range
        int32_t v = r->value;
        if (v > r->value_min && sign < 0) {
            mul = ut_min(v - r->value_min, mul);
            v += mul * sign;
        } else if (v < r->value_max && sign > 0) {
            mul = ut_min(r->value_max - v, mul);
            v += mul * sign;
        }
        if (r->value != v) {
            r->value = v;
            if (r->cb != null) { r->cb(r); }
            ui_view.invalidate(&r->view);
        }
    }
}

static void ui_slider_inc_dec(ui_button_t* b) {
    ui_slider_t* r = (ui_slider_t*)b->view.parent;
    if (!r->view.hidden && !r->view.disabled) {
        int32_t sign = b == &r->inc ? +1 : -1;
        int32_t mul = app.shift && app.ctrl ? 1000 :
            app.shift ? 100 : app.ctrl ? 10 : 1;
        ui_slider_inc_dec_value(r, sign, mul);
    }
}

static void ui_slider_every_100ms(ui_view_t* v) { // 100ms
    assert(v->type == ui_view_slider);
    ui_slider_t* r = (ui_slider_t*)v;
    if (r->view.hidden || r->view.disabled) {
        r->time = 0;
    } else if (!r->dec.view.armed && !r->inc.view.armed) {
        r->time = 0;
    } else {
        if (r->time == 0) {
            r->time = app.now;
        } else if (app.now - r->time > 1.0) {
            const int32_t sign = r->dec.view.armed ? -1 : +1;
            int32_t s = (int)(app.now - r->time + 0.5);
            int32_t mul = s >= 1 ? 1 << (s - 1) : 1;
            const int64_t range = (int64_t)r->value_max - r->value_min;
            if (mul > range / 8) { mul = (int32_t)(range / 8); }
            ui_slider_inc_dec_value(r, sign, ut_max(mul, 1));
        }
    }
}

void ui_slider_init_(ui_view_t* v) {
    assert(v->type == ui_view_slider);
    ui_view_init(v);
    ui_view.set_text(v, v->text);
    v->mouse       = ui_slider_mouse;
    v->measure     = ui_slider_measure;
    v->layout      = ui_slider_layout;
    v->paint       = ui_slider_paint;
    v->every_100ms = ui_slider_every_100ms;
    ui_slider_t* s = (ui_slider_t*)v;
    // Heavy Minus Sign
    ui_button_init(&s->dec, "\xE2\x9E\x96", 0, ui_slider_inc_dec);
    // Heavy Plus Sign
    ui_button_init(&s->inc, "\xE2\x9E\x95", 0, ui_slider_inc_dec);
    static const char* accel =
        "Accelerate by holding Ctrl x10 Shift x100 and Ctrl+Shift x1000";
    strprintf(s->inc.view.tip, "%s", accel);
    strprintf(s->dec.view.tip, "%s", accel);
    ui_view.add(&s->view, &s->dec.view, &s->inc.view, null);
    ui_view.localize(&s->view);
}

void ui_slider_init(ui_slider_t* s, const char* label, fp64_t ems,
        int32_t value_min, int32_t value_max, void (*cb)(ui_slider_t* r)) {
    static_assert(offsetof(ui_slider_t, view) == 0, "offsetof(.view)");
    assert(ems >= 3.0, "allow 1em for each of [-] and [+] buttons");
    s->view.type = ui_view_slider;
    strprintf(s->view.text, "%s", label);
    s->cb = cb;
    s->view.width = ems;
    s->value_min = value_min;
    s->value_max = value_max;
    s->value = value_min;
    ui_slider_init_(&s->view);
}
