#include "ut/ut.h"
#include "ui/ui.h"

static void ui_slider_invalidate(const ui_slider_t* s) {
    const ui_view_t* v = &s->view;
    ui_view.invalidate(v, null);
    if (!s->dec.state.hidden) { ui_view.invalidate(&s->dec, null); }
    if (!s->inc.state.hidden) { ui_view.invalidate(&s->dec, null); }
}

static int32_t ui_slider_width_without_insets(const ui_slider_t* s) {
    const int32_t em = s->view.fm->em.w;
    const fp64_t min_w = (fp64_t)s->view.min_w_em;
    const int32_t mw = (int32_t)(min_w * (fp64_t)em + 0.5);
    return ut_max(em, ut_max(mw, s->mt.w));
}

static int32_t ui_slider_width_with_insets(const ui_slider_t* s) {
    const ui_ltrb_t i = ui_view.gaps(&s->view, &s->view.insets);
    return i.left + ui_slider_width_without_insets(s) + i.right;
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
    char formatted[countof(s->view.p.text)];
    const char* text = ui_view.string(&s->view);
    ui_wh_t mt = s->view.fm->em;
    if (s->view.format != null) {
        s->view.format(&s->view);
        strprintf(formatted, "%s", text);
        mt = measure_text(s->view.fm, "%s", formatted);
        // TODO: format string 0x08X?
    } else if (text != null && (strstr(text, "%d") != null ||
                                strstr(text, "%u") != null)) {
        ui_wh_t mt_min = measure_text(s->view.fm, text, s->value_min);
        ui_wh_t mt_max = measure_text(s->view.fm, text, s->value_max);
        ui_wh_t mt_val = measure_text(s->view.fm, text, s->value);
        mt.h = ut_max(mt_val.h, ut_max(mt_min.h, mt_max.h));
        mt.w = ut_max(mt_val.w, ut_max(mt_min.w, mt_max.w));
    } else if (text != null && text[0] != 0) {
        mt = measure_text(s->view.fm, "%s", text);
    }
    return mt;
}

static void ui_slider_measure(ui_view_t* v) {
    assert(v->type == ui_view_slider);
    ui_slider_t* s = (ui_slider_t*)v;
    // dec and inc have same font metrics as a slider:
    s->dec.fm = v->fm;
    s->inc.fm = v->fm;
    ui_view.measure(&s->dec);
    ui_view.measure(&s->inc);
    const ui_ltrb_t dec_p = ui_view.gaps(&s->dec, &s->dec.padding);
    const ui_ltrb_t inc_p = ui_view.gaps(&s->inc, &s->inc.padding);
    s->mt = ui_slider_measure_text(s);
    assert(s->dec.state.hidden == s->inc.state.hidden, "not the same");
    const int32_t sw = ui_slider_width_with_insets(s);
    if (s->dec.state.hidden) {
        v->w = sw;
    } else {
        v->w = s->dec.w + dec_p.right + sw + inc_p.left + s->inc.w;
    }
    ui_ltrb_t i = ui_view.gaps(v, &v->insets);
    v->h = i.top + v->fm->em.h + i.bottom;
    v->h = ut_max(v->h, ui.gaps_em2px(v->fm->em.h, v->min_h_em));
}

static void ui_slider_layout(ui_view_t* v) {
    assert(v->type == ui_view_slider);
    ui_slider_t* s = (ui_slider_t*)v;
    // disregard inc/dec .state.hidden bit for layout:
    s->dec.x = v->x;
    s->dec.y = v->y;
    s->inc.x = v->x + v->w - s->inc.w;
    s->inc.y = v->y;
}

static void ui_slider_paint(ui_view_t* v) {
    assert(v->type == ui_view_slider);
    ui_slider_t* s = (ui_slider_t*)v;
    ui_gdi.set_clip(v->x, v->y, v->w, v->h);
    const ui_ltrb_t i = ui_view.gaps(v, &v->insets);
    const ui_ltrb_t dec_p = ui_view.gaps(&s->dec, &s->dec.padding);
    // dec button is sticking to the left into slider padding
    const int32_t dec_w = s->dec.w + dec_p.right;
    assert(s->dec.state.hidden == s->inc.state.hidden, "hidden or not together");
    const int32_t dx = s->dec.state.hidden ? 0 : dec_w;
    const int32_t x = v->x + dx + i.left;
    const int32_t w = ui_slider_width_without_insets(s);
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
    assert(range > 0, "range: %.6f", range);
    const fp64_t  vw = (fp64_t)w * (s->value - s->value_min) / range;
    const int32_t wi = (int32_t)(vw + 0.5);
    ui_gdi.gradient(x, v->y, wi, v->h, d1, d0, true);
    if (!v->state.flat) {
        ui_color_t color = v->state.hover ?
            ui_colors.get_color(ui_color_id_hot_tracking) :
            ui_colors.get_color(ui_color_id_gray_text);
        if (ui_view.is_disabled(v)) { color = ui_color_rgb(30, 30, 30); } // TODO: hardcoded
        ui_gdi.frame(x, v->y, w, v->h, color);
    }
    // text:
    const char* text = ui_view.string(v);
    char formatted[countof(v->p.text)];
    if (s->view.format != null) {
        s->view.format(v);
        s->view.p.strid = 0; // nls again
        text = ui_view.string(v);
    } else if (text != null && (strstr(text, "%d") != null) ||
                                strstr(text, "%u") != null) {
        ut_str.format(formatted, countof(formatted), text, s->value);
        s->view.p.strid = 0; // nls again
        text = ut_nls.str(formatted);
    }
    ui_wh_t mt = ui_slider_measure_text(s);
    const int32_t sw = ui_slider_width_with_insets(s); // slider width
    const int32_t cx = (sw - mt.w) / 2; // centering offset
    const int32_t tx = v->x + cx + (s->dec.state.hidden ? 0 : dec_w);
    const int32_t ty = v->y + i.top;
    const ui_color_t text_color = !v->state.hover ? v->color :
            (ui_theme.is_app_dark() ? ui_colors.white : ui_colors.black);
    const ui_gdi_ta_t ta = { .fm = v->fm, .color = text_color };
    ui_gdi.text(&ta, tx, ty, "%s", text);
    ui_gdi.set_clip(0, 0, 0, 0);
}

static void ui_slider_mouse(ui_view_t* v, int32_t message, int64_t f) {
    if (!ui_view.is_hidden(v) && !ui_view.is_disabled(v)) {
        assert(v->type == ui_view_slider);
        ui_slider_t* s = (ui_slider_t*)v;
        bool drag = message == ui.message.mouse_move &&
            (f & (ui.mouse.button.left|ui.mouse.button.right)) != 0;
        if (message == ui.message.left_button_pressed ||
            message == ui.message.right_button_pressed || drag) {
            const ui_ltrb_t dec_p = ui_view.gaps(&s->dec, &s->dec.padding);
            const int32_t dec_w = s->dec.w + dec_p.right;
            assert(s->dec.state.hidden == s->inc.state.hidden, "hidden or not together");
            const int32_t sw = ui_slider_width_with_insets(s); // slider width
            const int32_t dx = s->dec.state.hidden ? 0 : dec_w;
            const int32_t vx = v->x + dx;
            const int32_t x = ui_app.mouse.x - vx;
            const int32_t y = ui_app.mouse.y - v->y;
            if (0 <= x && x < sw && 0 <= y && y < v->h) {
                ui_app.focus = v;
                const fp64_t range = (fp64_t)s->value_max - (fp64_t)s->value_min;
                fp64_t val = (fp64_t)x * range / (fp64_t)(sw - 1);
                int32_t vw = (int32_t)(val + s->value_min + 0.5);
                s->value = ut_min(ut_max(vw, s->value_min), s->value_max);
                if (s->view.callback != null) { s->view.callback(&s->view); }
                ui_slider_invalidate(s);
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
    assert(v->type == ui_view_slider);
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
    assert(v->type == ui_view_slider);
    v->measure       = ui_slider_measure;
    v->layout        = ui_slider_layout;
    v->paint         = ui_slider_paint;
    v->mouse         = ui_slider_mouse;
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
    s->dec.padding.right = 0.125f;
    s->inc.padding.left  = 0.125f;
    ut_str_printf(s->inc.hint, "%s", accel);
    v->color_id      = ui_color_id_button_text;
    v->background_id = ui_color_id_button_face;
}

void ui_slider_init(ui_slider_t* s, const char* label, fp32_t min_w_em,
        int32_t value_min, int32_t value_max,
        void (*callback)(ui_view_t* r)) {
    static_assert(offsetof(ui_slider_t, view) == 0, "offsetof(.view)");
    assert(min_w_em >= 3.0, "allow 1em for each of [-] and [+] buttons");
    s->view.type = ui_view_slider;
    ui_view.set_text(&s->view, "%s", label);
    s->view.callback = callback;
    s->view.min_w_em = min_w_em;
    s->value_min = value_min;
    s->value_max = value_max;
    s->value = value_min;
    ui_view_init_slider(&s->view);
}
