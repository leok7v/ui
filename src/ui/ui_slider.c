#include "ut/ut.h"
#include "ui/ui.h"

static void ui_slider_invalidate(const ui_slider_t* s) {
    const ui_view_t* v = &s->view;
    ui_view.invalidate(v, null);
    if (!s->dec.state.hidden) { ui_view.invalidate(&s->dec, null); }
    if (!s->inc.state.hidden) { ui_view.invalidate(&s->dec, null); }
}

static int32_t ui_slider_width(const ui_slider_t* s) {
    const ui_ltrb_t i = ui_view.margins(&s->view, &s->insets);
    int32_t w = s->w - i.left - i.right;
    if (!s->dec.state.hidden) {
        const ui_ltrb_t dec_p = ui_view.margins(&s->dec, &s->dec.padding);
        const ui_ltrb_t inc_p = ui_view.margins(&s->inc, &s->inc.padding);
        w -= s->dec.w + s->inc.w + dec_p.right + inc_p.left;
    }
    return w;
}

static ui_wh_t measure_text(const ui_fm_t* fm, const char* format, ...) {
    va_list va;
    va_start(va, format);
    const ui_gdi_ta_t ta = { .fm = fm, .color = ui_colors.white, .measure = true };
    ui_wh_t wh = ui_gdi.text_va(&ta, 0, 0, format, va);
    va_end(va);
    return wh;
}

static ui_wh_t ui_slider_measure_text(ui_slider_t* s) {
    char formatted[ut_countof(s->p.text)];
    const ui_fm_t* fm = s->fm;
    const char* text = ui_view.string(&s->view);
    const ui_ltrb_t i = ui_view.margins(&s->view, &s->insets);
    ui_wh_t wh = s->fm->em;
    if (s->debug.trace.mt) {
        const ui_ltrb_t p = ui_view.margins(&s->view, &s->padding);
        ut_println(">%dx%d em: %dx%d min: %.1fx%.1f "
                "i: %d %d %d %d p: %d %d %d %d \"%.*s\"",
            s->w, s->h, fm->em.w, fm->em.h, s->min_w_em, s->min_h_em,
            i.left, i.top, i.right, i.bottom,
            p.left, p.top, p.right, p.bottom,
            ut_min(64, strlen(text)), text);
        const ui_margins_t in = s->insets;
        const ui_margins_t pd = s->padding;
        ut_println(" i: %.3f %.3f %.3f %.3f l+r: %.3f t+b: %.3f"
                " p: %.3f %.3f %.3f %.3f l+r: %.3f t+b: %.3f",
            in.left, in.top, in.right, in.bottom,
            in.left + in.right, in.top + in.bottom,
            pd.left, pd.top, pd.right, pd.bottom,
            pd.left + pd.right, pd.top + pd.bottom);
    }
    if (s->format != null) {
        s->format(&s->view);
        ut_str_printf(formatted, "%s", text);
        wh = measure_text(s->fm, "%s", formatted);
        // TODO: format string 0x08X?
    } else if (text != null && (strstr(text, "%d") != null ||
                                strstr(text, "%u") != null)) {
        ui_wh_t mt_min = measure_text(s->fm, text, s->value_min);
        ui_wh_t mt_max = measure_text(s->fm, text, s->value_max);
        ui_wh_t mt_val = measure_text(s->fm, text, s->value);
        wh.h = ut_max(mt_val.h, ut_max(mt_min.h, mt_max.h));
        wh.w = ut_max(mt_val.w, ut_max(mt_min.w, mt_max.w));
    } else if (text != null && text[0] != 0) {
        wh = measure_text(s->fm, "%s", text);
    }
    if (s->debug.trace.mt) {
        ut_println(" mt: %dx%d", wh.w, wh.h);
    }
    return wh;
}

static void ui_slider_measure(ui_view_t* v) {
    ut_assert(v->type == ui_view_slider);
    ui_slider_t* s = (ui_slider_t*)v;
    const ui_fm_t* fm = v->fm;
    const ui_ltrb_t i = ui_view.margins(v, &v->insets);
    // slider cannot be smaller than 2*em
    const fp32_t min_w_em = ut_max(2.0f, v->min_w_em);
    v->w = (int32_t)((fp64_t)fm->em.w * (fp64_t)   min_w_em + 0.5);
    v->h = (int32_t)((fp64_t)fm->em.h * (fp64_t)v->min_h_em + 0.5);
    // dec and inc have same font metrics as a slider:
    s->dec.fm = fm;
    s->inc.fm = fm;
    ut_assert(s->dec.state.hidden == s->inc.state.hidden, "not the same");
    ui_view.measure_control(v);
//  s->text.mt = ui_slider_measure_text(s);
    if (s->dec.state.hidden) {
        v->w = ut_max(v->w, i.left + s->wh.w + i.right);
    } else {
        ui_view.measure(&s->dec); // remeasure with inherited metrics
        ui_view.measure(&s->inc);
        const ui_ltrb_t dec_p = ui_view.margins(&s->dec, &s->dec.padding);
        const ui_ltrb_t inc_p = ui_view.margins(&s->inc, &s->inc.padding);
        v->w = ut_max(v->w, s->dec.w + dec_p.right + s->wh.w + inc_p.left + s->inc.w);
    }
    v->h = ut_max(v->h, i.top + fm->em.h + i.bottom);
    if (s->debug.trace.mt) {
        ut_println("<%dx%d", s->w, s->h);
    }
}

static void ui_slider_layout(ui_view_t* v) {
    ut_assert(v->type == ui_view_slider);
    ui_slider_t* s = (ui_slider_t*)v;
    // disregard inc/dec .state.hidden bit for layout:
    const ui_ltrb_t i = ui_view.margins(v, &v->insets);
    s->dec.x = v->x + i.left;
    s->dec.y = v->y;
    s->inc.x = v->x + v->w - i.right - s->inc.w;
    s->inc.y = v->y;
}

static void ui_slider_paint(ui_view_t* v) {
    ut_assert(v->type == ui_view_slider);
    ui_slider_t* s = (ui_slider_t*)v;
    const ui_fm_t* fm = v->fm;
    const ui_ltrb_t i = ui_view.margins(v, &v->insets);
    const ui_ltrb_t dec_p = ui_view.margins(&s->dec, &s->dec.padding);
    // dec button is sticking to the left into slider padding
    const int32_t dec_w = s->dec.w + dec_p.right;
    ut_assert(s->dec.state.hidden == s->inc.state.hidden, "hidden or not together");
    const int32_t dx = s->dec.state.hidden ? 0 : dec_w;
    const int32_t x = v->x + dx + i.left;
    const int32_t w = ui_slider_width(s);
    // draw background:
    fp32_t d = ui_theme.is_app_dark() ? 0.50f : 0.25f;
    ui_color_t d0 = ui_colors.darken(v->background, d);
    d /= 4;
    ui_color_t d1 = ui_colors.darken(v->background, d);
    ui_gdi.gradient(x, v->y, w, v->h, d1, d0, true);
    // draw value:
    ui_color_t c = ui_theme.is_app_dark() ?
        ui_colors.darken(ui_colors.green, 1.0f / 128.0f) :
        ui_colors.jungle_green;
    d1 = c;
    d0 = ui_colors.darken(c, 1.0f / 64.0f);
    const fp64_t range = (fp64_t)s->value_max - (fp64_t)s->value_min;
    ut_assert(range > 0, "range: %.6f", range);
    const fp64_t  vw = (fp64_t)w * (s->value - s->value_min) / range;
    const int32_t wi = (int32_t)(vw + 0.5);
    ui_gdi.gradient(x, v->y, wi, v->h, d1, d0, true);
    if (!v->flat) {
        ui_color_t color = v->state.hover ?
            ui_colors.get_color(ui_color_id_hot_tracking) :
            ui_colors.get_color(ui_color_id_gray_text);
        if (ui_view.is_disabled(v)) { color = ui_color_rgb(30, 30, 30); } // TODO: hardcoded
        ui_gdi.frame(x, v->y, w, v->h, color);
    }
    // text:
    const char* text = ui_view.string(v);
    char formatted[ut_countof(v->p.text)];
    if (s->format != null) {
        s->format(v);
        s->p.strid = 0; // nls again
        text = ui_view.string(v);
    } else if (text != null &&
        (strstr(text, "%d") != null || strstr(text, "%u") != null)) {
        ut_str.format(formatted, ut_countof(formatted), text, s->value);
        s->p.strid = 0; // nls again
        text = ut_nls.str(formatted);
    }
    // because current value was formatted into `text` need to
    // remeasure and align text again:
    ui_view.text_measure(v, text, &v->text);
    ui_view.text_align(v, &v->text);
    const ui_color_t text_color = !v->state.hover ? v->color :
            (ui_theme.is_app_dark() ? ui_colors.white : ui_colors.black);
    const ui_gdi_ta_t ta = { .fm = fm, .color = text_color };
    ui_gdi.text(&ta, v->x + v->text.xy.x, v->y + v->text.xy.y, "%s", text);
}

static void ui_slider_mouse_click(ui_view_t* v, int32_t ut_unused(ix),
        bool pressed) {
    ui_slider_t* s = (ui_slider_t*)v;
    if (pressed) {
        const ui_ltrb_t i = ui_view.margins(v, &v->insets);
        const ui_ltrb_t dec_p = ui_view.margins(&s->dec, &s->dec.padding);
        const int32_t dec_w = s->dec.w + dec_p.right;
        ut_assert(s->dec.state.hidden == s->inc.state.hidden, "hidden or not together");
        const int32_t sw = ui_slider_width(s); // slider width
        const int32_t dx = s->dec.state.hidden ? 0 : dec_w + dec_p.right;
        const int32_t vx = v->x + i.left + dx;
        const int32_t x = ui_app.mouse.x - vx;
        const int32_t y = ui_app.mouse.y - (v->y + i.top);
        if (0 <= x && x < sw && 0 <= y && y < v->h) {
            const fp64_t range = (fp64_t)s->value_max - (fp64_t)s->value_min;
            fp64_t val = (fp64_t)x * range / (fp64_t)(sw - 1);
            int32_t vw = (int32_t)(val + s->value_min + 0.5);
            s->value = ut_min(ut_max(vw, s->value_min), s->value_max);
            if (s->callback != null) { s->callback(&s->view); }
            ui_slider_invalidate(s);
        }
    }
}

static void ui_slider_mouse_move(ui_view_t* v) {
    const ui_ltrb_t i = ui_view.margins(v, &v->insets);
    ui_slider_t* s = (ui_slider_t*)v;
    bool drag = ui_app.mouse_left || ui_app.mouse_right;
    if (drag) {
        const ui_ltrb_t dec_p = ui_view.margins(&s->dec, &s->dec.padding);
        const int32_t dec_w = s->dec.w + dec_p.right;
        ut_assert(s->dec.state.hidden == s->inc.state.hidden, "hidden or not together");
        const int32_t sw = ui_slider_width(s); // slider width
        const int32_t dx = s->dec.state.hidden ? 0 : dec_w + dec_p.right;
        const int32_t vx = v->x + i.left + dx;
        const int32_t x = ui_app.mouse.x - vx;
        const int32_t y = ui_app.mouse.y - (v->y + i.top);
        if (0 <= x && x < sw && 0 <= y && y < v->h) {
            const fp64_t range = (fp64_t)s->value_max - (fp64_t)s->value_min;
            fp64_t val = (fp64_t)x * range / (fp64_t)(sw - 1);
            int32_t vw = (int32_t)(val + s->value_min + 0.5);
            s->value = ut_min(ut_max(vw, s->value_min), s->value_max);
            if (s->callback != null) { s->callback(&s->view); }
            ui_slider_invalidate(s);
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
            if (s->callback != null) { s->callback(&s->view); }
            ui_slider_invalidate(s);
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
    ut_assert(v->type == ui_view_slider);
    ui_slider_t* s = (ui_slider_t*)v;
    if (ui_view.is_hidden(v) || ui_view.is_disabled(v)) {
        s->time = 0;
    } else if (!s->dec.state.armed && !s->inc.state.armed) {
        s->time = 0;
    } else {
        if (s->time == 0) {
            s->time = ui_app.now;
        } else if (ui_app.now - s->time > 1.0) {
            const int32_t sign = s->dec.state.armed ? -1 : +1;
            const int32_t sec = (int32_t)(ui_app.now - s->time + 0.5);
            int32_t initial = ui_app.shift && ui_app.ctrl ? 1000 :
                ui_app.shift ? 100 : ui_app.ctrl ? 10 : 1;
            int32_t mul = sec >= 1 ? initial << (sec - 1) : initial;
            const int64_t range = (int64_t)s->value_max - (int64_t)s->value_min;
            if (mul > range / 8) { mul = (int32_t)(range / 8); }
            ui_slider_inc_dec_value(s, sign, ut_max(mul, 1));
        }
    }
}

void ui_view_init_slider(ui_view_t* v) {
    ut_assert(v->type == ui_view_slider);
    v->measure       = ui_slider_measure;
    v->layout        = ui_slider_layout;
    v->paint         = ui_slider_paint;
    v->mouse_click   = ui_slider_mouse_click;
    v->mouse_move    = ui_slider_mouse_move;
    v->every_100ms   = ui_slider_every_100ms;
    v->color_id      = ui_color_id_window_text;
    v->background_id = ui_color_id_button_face;
    ui_slider_t* s = (ui_slider_t*)v;
    static const char* accel =
        " Hold key while clicking\n"
        " Ctrl: x 10 Shift: x 100 \n"
        " Ctrl+Shift: x 1000 \n for step multiplier.";
    s->dec = (ui_button_t)ui_button(ut_glyph_fullwidth_hyphen_minus, 0, // ut_glyph_heavy_minus_sign
                                    ui_slider_inc_dec);
    s->dec.fm = v->fm;
    ut_str_printf(s->dec.hint, "%s", accel);
    s->inc = (ui_button_t)ui_button(ut_glyph_fullwidth_plus_sign, 0, // ut_glyph_heavy_plus_sign
                                    ui_slider_inc_dec);
    s->inc.fm = v->fm;
    ui_view.add(&s->view, &s->dec, &s->inc, null);
    // single glyph buttons less insets look better:
    ui_view_for_each(&s->view, it, {
        it->insets.left   = 0.125f;
        it->insets.right  = 0.125f;
    });
    // inherit initial padding and insets from buttons.
    // caller may change those later and it should be accounted to
    // in measure() and layout()
    v->insets  = s->dec.insets;
    v->padding = s->dec.padding;
    s->dec.padding.right = 0;
    s->dec.padding.left  = 0;
    s->inc.padding.left  = 0;
    s->inc.padding.right = 0;
    s->dec.flat = true;
    s->inc.flat = true;
    s->dec.min_h_em = 1.0f + ui_view_i_tb * 2;
    s->dec.min_w_em = 1.0f + ui_view_i_tb * 2;
    s->inc.min_h_em = 1.0f + ui_view_i_tb * 2;
    s->inc.min_w_em = 1.0f + ui_view_i_tb * 2;
    ut_str_printf(s->inc.hint, "%s", accel);
    v->color_id      = ui_color_id_button_text;
    v->background_id = ui_color_id_button_face;
    if (v->debug.id == null) { v->debug.id = "#slider"; }
}

void ui_slider_init(ui_slider_t* s, const char* label, fp32_t min_w_em,
        int32_t value_min, int32_t value_max,
        void (*callback)(ui_view_t* r)) {
    static_assert(offsetof(ui_slider_t, view) == 0, "offsetof(.view)");
    if (min_w_em < 6.0) { ut_println("6.0 em minimum"); }
    s->type = ui_view_slider;
    ui_view.set_text(&s->view, "%s", label);
    s->callback = callback;
    s->min_w_em = ut_max(6.0f, min_w_em);
    s->value_min = value_min;
    s->value_max = value_max;
    s->value = value_min;
    ui_view_init_slider(&s->view);
}
