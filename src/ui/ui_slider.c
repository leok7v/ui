#include "ut/ut.h"
#include "ui/ui.h"

static void ui_slider_measure(ui_view_t* v) {
    assert(v->type == ui_view_slider);
    ui_slider_t* s = (ui_slider_t*)v;
    s->inc.fm = v->fm;
    s->dec.fm = v->fm;
    ui_view.measure(v);
    ui_view.measure(&s->dec);
    ui_view.measure(&s->inc);
    assert(s->inc.w == s->dec.w && s->inc.h == s->dec.h);
    const int32_t em = v->fm->em.w;
    const int32_t w = (int32_t)((fp64_t)v->min_w_em * (fp64_t)v->fm->em.w + 0.5);
    s->tm = ui_gdi.measure_text(v->fm->font, ui_view.nls(v), s->value_max);
//  if (w > r->tm.x) { r->tm.x = w; }
    s->tm.x = w != 0 ? w : s->tm.x;
    v->w = s->dec.w + s->tm.x + s->inc.w + em * 2;
    v->h = s->inc.h;
}

static void ui_slider_layout(ui_view_t* v) {
    assert(v->type == ui_view_slider);
    ui_slider_t* s = (ui_slider_t*)v;
    assert(s->inc.w == s->dec.w && s->inc.h == s->dec.h);
    const int32_t em = v->fm->em.w;
    s->dec.x = v->x;
    s->dec.y = v->y;
    s->inc.x = v->x + s->dec.w + s->tm.x + em * 2;
    s->inc.y = v->y;
}

// TODO: generalize and move to ui_colors.c to avoid slider dup

static ui_color_t ui_slider_gradient_darker(void) {
    if (ui_theme.are_apps_dark()) {
        return ui_colors.btn_gradient_darker;
    } else {
        ui_color_t c = ui_app.get_color(ui_color_id_button_face);
        uint32_t r = ui_color_r(c);
        uint32_t g = ui_color_r(c);
        uint32_t b = ui_color_r(c);
        r = ut_max(0, ut_min(0xFF, (uint32_t)(r * 0.75)));
        g = ut_max(0, ut_min(0xFF, (uint32_t)(g * 0.75)));
        b = ut_max(0, ut_min(0xFF, (uint32_t)(b * 0.75)));
        ui_color_t d = ui_rgb(r, g, b);
//      traceln("c: 0%06X -> 0%06X", c, d);
        return d;
    }
}

static ui_color_t ui_slider_gradient_dark(void) {
    if (ui_theme.are_apps_dark()) {
        return ui_colors.btn_gradient_dark;
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

static void ui_slider_paint(ui_view_t* v) {
    assert(v->type == ui_view_slider);
    ui_slider_t* s = (ui_slider_t*)v;
    ui_gdi.push(v->x, v->y);
    ui_gdi.set_clip(v->x, v->y, v->w, v->h);
    const int32_t em = v->fm->em.w;
    const int32_t em2  = ut_max(1, em / 2);
    const int32_t em4  = ut_max(1, em / 8);
    const int32_t em8  = ut_max(1, em / 8);
    const int32_t em16 = ut_max(1, em / 16);
    ui_color_t c0 = ui_theme.are_apps_dark() ?
                    ui_colors.dkgray3 :
                    ui_app.get_color(ui_color_id_button_face);
    ui_gdi.set_brush(ui_gdi.brush_color);
    ui_pen_t pen_c0 = ui_gdi.create_pen(c0, em16);
    ui_gdi.set_pen(pen_c0);
    ui_gdi.set_brush_color(c0);
    const int32_t x = v->x + s->dec.w + em2;
    const int32_t y = v->y;
    const int32_t w = s->tm.x + em;
    const int32_t h = v->h;
    ui_gdi.rounded(x - em8, y, w + em4, h, em4, em4);
    if (ui_theme.are_apps_dark()) {
        ui_gdi.gradient(x, y, w, h / 2, c0, ui_slider_gradient_darker(), true);
        ui_gdi.gradient(x, y + h / 2, w, v->h - h / 2, ui_slider_gradient_dark(), c0, true);
        ui_gdi.set_brush_color(ui_colors.dkgreen);
    } else {
        ui_gdi.gradient(x, y, w, h / 2, ui_slider_gradient_dark(), c0, true);
        ui_gdi.gradient(x, y + h / 2, w, v->h - h / 2, c0, ui_slider_gradient_darker(), true);
        ui_gdi.set_brush_color(ui_colors.jungle_green);
    }
    ui_color_t c1 = ui_theme.are_apps_dark() ?
                    ui_colors.dkgray1 :
                    ui_app.get_color(ui_color_id_button_face); // ???
    ui_pen_t pen_c1 = ui_gdi.create_pen(c1, em16);
    ui_gdi.set_pen(pen_c1);
    const fp64_t range = (fp64_t)s->value_max - (fp64_t)s->value_min;
    fp64_t vw = (fp64_t)(s->tm.x + em) * (s->value - s->value_min) / range;
    ui_gdi.rect(x, v->y, (int32_t)(vw + 0.5), v->h);
    ui_gdi.x += s->dec.w + em;
    ui_gdi.y = y;
    ui_color_t c = ui_gdi.set_text_color(v->color);
    ui_font_t f = ui_gdi.set_font(v->fm->font);
    const char* format = ut_nls.str(v->text);
    ui_gdi.text(format, s->value);
    ui_gdi.set_font(f);
    ui_gdi.set_text_color(c);
    ui_gdi.set_clip(0, 0, 0, 0);
    ui_gdi.delete_pen(pen_c1);
    ui_gdi.delete_pen(pen_c0);
    ui_gdi.pop();
}

static void ui_slider_mouse(ui_view_t* v, int32_t message, int64_t f) {
    if (!v->hidden && !v->disabled) {
        assert(v->type == ui_view_slider);
        ui_slider_t* s = (ui_slider_t*)v;
        bool drag = message == ui.message.mouse_move &&
            (f & (ui.mouse.button.left|ui.mouse.button.right)) != 0;
        if (message == ui.message.left_button_pressed ||
            message == ui.message.right_button_pressed || drag) {
            const int32_t x = ui_app.mouse.x - v->x - s->dec.w;
            const int32_t y = ui_app.mouse.y - v->y;
            const int32_t x0 = v->fm->em.w / 2;
            const int32_t x1 = s->tm.x + v->fm->em.w;
            if (x0 <= x && x < x1 && 0 <= y && y < v->h) {
                ui_app.focus = v;
                const fp64_t range = (fp64_t)s->value_max - (fp64_t)s->value_min;
                fp64_t val = ((fp64_t)x - x0) * range / (fp64_t)(x1 - x0 - 1);
                int32_t vw = (int32_t)(val + s->value_min + 0.5);
                s->value = ut_min(ut_max(vw, s->value_min), s->value_max);
                if (s->view.callback != null) { s->view.callback(&s->view); }
                ui_view.invalidate(v);
            }
        }
    }
}

static void ui_slider_inc_dec_value(ui_slider_t* s, int32_t sign, int32_t mul) {
    if (!ui_view.is_hidden(&s->view) && !ui_view.is_disabled(&s->view)) {
        // full 0x80000000..0x7FFFFFFF (-2147483648..2147483647) range
        int32_t v = s->value;
        if (v > s->value_min && sign < 0) {
            mul = ut_min(v - s->value_min, mul);
            v += mul * sign;
        } else if (v < s->value_max && sign > 0) {
            mul = ut_min(s->value_max - v, mul);
            v += mul * sign;
        }
        if (s->value != v) {
            s->value = v;
            if (s->view.callback != null) { s->view.callback(&s->view); }
            ui_view.invalidate(&s->view);
        }
    }
}

static void ui_slider_inc_dec(ui_button_t* b) {
    ui_slider_t* s = (ui_slider_t*)b->parent;
    if (!ui_view.is_hidden(&s->view) && !ui_view.is_disabled(&s->view)) {
        int32_t sign = b == &s->inc ? +1 : -1;
        int32_t mul = ui_app.shift && ui_app.ctrl ? 1000 :
            ui_app.shift ? 100 : ui_app.ctrl ? 10 : 1;
        ui_slider_inc_dec_value(s, sign, mul);
    }
}

static void ui_slider_every_100ms(ui_view_t* v) { // 100ms
    assert(v->type == ui_view_slider);
    ui_slider_t* s = (ui_slider_t*)v;
    if (ui_view.is_hidden(v) || ui_view.is_disabled(v)) {
        s->time = 0;
    } else if (!s->dec.armed && !s->inc.armed) {
        s->time = 0;
    } else {
        if (s->time == 0) {
            s->time = ui_app.now;
        } else if (ui_app.now - s->time > 1.0) {
            const int32_t sign = s->dec.armed ? -1 : +1;
            int32_t sec = (int32_t)(ui_app.now - s->time + 0.5);
            int32_t mul = sec >= 1 ? 1 << (sec - 1) : 1;
            const int64_t range = (int64_t)s->value_max - s->value_min;
            if (mul > range / 8) { mul = (int32_t)(range / 8); }
            ui_slider_inc_dec_value(s, sign, ut_max(mul, 1));
        }
    }
}

void ui_view_init_slider(ui_view_t* v) {
    assert(v->type == ui_view_slider);
    ui_view_init(v);
    ui_view.set_text(v, v->text);
    v->mouse       = ui_slider_mouse;
    v->measure     = ui_slider_measure;
    v->layout      = ui_slider_layout;
    v->paint       = ui_slider_paint;
    v->every_100ms = ui_slider_every_100ms;
    v->color_id    = ui_color_id_window_text;
    ui_slider_t* s = (ui_slider_t*)v;
    static const char* accel =
        " Hold key while clicking\n"
        " Ctrl: x 10 Shift: x 100 \n"
        " Ctrl+Shift: x 1000 \n for step multiplier.";
    s->dec = (ui_button_t)ui_button(ui_glyph_heavy_minus_sign, 0,
                                    ui_slider_inc_dec);
    s->dec.fm = v->fm;
    strprintf(s->dec.hint, "%s", accel);
    s->inc = (ui_button_t)ui_button(ui_glyph_heavy_minus_sign, 0,
                                    ui_slider_inc_dec);
    s->inc.fm = v->fm;
    strprintf(s->inc.hint, "%s", accel);
    ui_view.add(&s->view, &s->dec, &s->inc, null);
    ui_view.localize(&s->view);
}

void ui_slider_init(ui_slider_t* s, const char* label, fp32_t min_w_em,
        int32_t value_min, int32_t value_max,
        void (*callback)(ui_view_t* r)) {
    static_assert(offsetof(ui_slider_t, view) == 0, "offsetof(.view)");
    assert(min_w_em >= 3.0, "allow 1em for each of [-] and [+] buttons");
    s->view.type = ui_view_slider;
    strprintf(s->view.text, "%s", label);
    s->view.callback = callback;
    s->view.min_w_em = min_w_em;
    s->value_min = value_min;
    s->value_max = value_max;
    s->value = value_min;
    ui_view_init_slider(&s->view);
}
