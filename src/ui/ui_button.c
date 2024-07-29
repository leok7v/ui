#include "ut/ut.h"
#include "ui/ui.h"

static void ui_button_every_100ms(ui_view_t* v) { // every 100ms
    if (!v->state.hidden) {
        v->p.armed_until = 0;
        v->state.armed = false;
    } else if (v->p.armed_until != 0 && ui_app.now > v->p.armed_until) {
        v->p.armed_until = 0;
        v->state.armed = false;
        ui_view.invalidate(v, null);
    }
    if (v->p.armed_until != 0) { ui_app.show_hint(null, -1, -1, 0); }
}

static void ui_button_paint(ui_view_t* v) {
    bool pressed = (v->state.armed ^ v->state.pressed) == 0;
    if (v->p.armed_until != 0) { pressed = true; }
    const int32_t w = v->w;
    const int32_t h = v->h;
    const int32_t x = v->x;
    const int32_t y = v->y;
    const int32_t r = (0x1 | rt_max(3, v->fm->em.h / 4));  // odd radius
    const fp32_t d = ui_theme.is_app_dark() ? 0.50f : 0.25f;
    ui_color_t d0 = ui_colors.darken(v->background, d);
    const fp32_t d2 = d / 2;
    if (v->flat) {
        if (v->state.hover) {
            ui_color_t d1 = ui_theme.is_app_dark() ?
                    ui_colors.lighten(v->background, d2) :
                    ui_colors.darken(v->background,  d2);
            if (!pressed) {
                ui_gdi.gradient(x, y, w, h, d0, d1, true);
            } else {
                ui_gdi.gradient(x, y, w, h, d1, d0, true);
            }
        }
    } else {
        // `bc` border color
        ui_color_t bc = ui_colors.get_color(ui_color_id_gray_text);
        if (v->state.armed) { bc = ui_colors.lighten(bc, 0.125f); }
        if (ui_view.is_disabled(v)) { bc = ui_color_rgb(30, 30, 30); } // TODO: hardcoded
        if (v->state.hover && !v->state.armed) {
            bc = ui_colors.get_color(ui_color_id_hot_tracking);
        }
        ui_color_t d1 = ui_colors.darken(v->background, d2);
        ui_color_t fc = ui_colors.interpolate(d0, d1, 0.5f); // fill color
        if (v->state.armed) {
            fc = ui_colors.lighten(fc, 0.250f);
        } else if (v->state.hover) {
            fc = ui_colors.darken(fc, 0.250f);
        }
        ui_gdi.rounded(v->x, v->y, v->w, v->h, r, bc, fc);
    }
    const int32_t tx = v->x + v->text.xy.x;
    const int32_t ty = v->y + v->text.xy.y;
    if (v->icon == null) {
        ui_color_t c = v->color;
        if (v->state.hover && !v->state.armed) {
            c = ui_theme.is_app_dark() ? ui_color_rgb(0xFF, 0xE0, 0xE0) :
                                         ui_color_rgb(0x00, 0x40, 0xFF);
        }
        if (ui_view.is_disabled(v)) { c = ui_colors.get_color(ui_color_id_gray_text); }
        if (v->debug.paint.fm) {
            ui_view.debug_paint_fm(v);
        }
        const ui_gdi_ta_t ta = { .fm = v->fm, .color = c };
        ui_gdi.text(&ta, tx, ty, "%s", ui_view.string(v));
    } else {
        const ui_ltrb_t i = ui_view.margins(v, &v->insets);
        const ui_wh_t i_wh = { .w = v->w - i.left - i.right,
                               .h = v->h - i.top - i.bottom };
        // TODO: icon text alignment
        ui_gdi.icon(tx, ty + v->text.xy.y, i_wh.w, i_wh.h, v->icon);
    }
}

static void ui_button_callback(ui_button_t* b) {
    // for flip buttons the state of the button flips
    // *before* callback.
    if (b->flip) { b->state.pressed = !b->state.pressed; }
    const bool pressed = b->state.pressed;
    if (b->callback != null) { b->callback(b); }
    if (pressed != b->state.pressed) {
        if (b->flip) { // warn the client of strange logic:
            ut_println("strange flip the button with button.flip: true");
            // if client wants to flip pressed state manually it
            // should do it for the button.flip = false
        }
//      ut_println("disarmed immediately");
        b->p.armed_until = 0;
        b->state.armed = false;
    } else {
        if (b->flip) {
//          ut_println("disarmed immediately");
            b->p.armed_until = 0;
            b->state.armed = false;
        } else {
//          ut_println("will disarm in 1/4 seconds");
            b->p.armed_until = ui_app.now + 0.250;
        }
    }
}

static void ui_button_trigger(ui_view_t* v) {
    ui_button_t* b = (ui_button_t*)v;
    v->state.armed = true;
    ui_view.invalidate(v, null);
    ui_button_callback(b);
}

static void ui_button_character(ui_view_t* v, const char* utf8) {
    char ch = utf8[0]; // TODO: multibyte utf8 shortcuts?
    if (ui_view.is_shortcut_key(v, ch)) {
        ui_button_trigger(v);
    }
}

static bool ui_button_key_pressed(ui_view_t* v, int64_t key) {
    ut_assert(!ui_view.is_hidden(v) && !ui_view.is_disabled(v));
    const bool trigger = ui_app.alt && ui_view.is_shortcut_key(v, key);
    if (trigger) { ui_button_trigger(v); }
    return trigger; // swallow if true
}

static bool ui_button_tap(ui_view_t* v, int32_t ut_unused(ix),
        bool pressed) {
    // 'ix' ignored - button index acts on any mouse button
    const bool inside = ui_view.inside(v, &ui_app.mouse);
    if (inside) {
        ui_view.invalidate(v, null); // always on any press/release inside
        ui_button_t* b = (ui_button_t*)v;
        if (pressed && b->flip) {
            if (b->flip) { ui_button_callback(b); }
        } else if (pressed) {
            v->state.armed = true;
        } else { // released
            if (!b->flip) { ui_button_callback(b); }
        }
    }
    return pressed && inside; // swallow clicks inside
}

void ui_view_init_button(ui_view_t* v) {
    ut_assert(v->type == ui_view_button);
    v->tap           = ui_button_tap;
    v->paint         = ui_button_paint;
    v->character     = ui_button_character;
    v->every_100ms   = ui_button_every_100ms;
    v->key_pressed   = ui_button_key_pressed;
    v->color_id      = ui_color_id_button_text;
    v->background_id = ui_color_id_button_face;
    if (v->debug.id == null) { v->debug.id = "#button"; }
}

void ui_button_init(ui_button_t* b, const char* label, fp32_t ems,
        void (*callback)(ui_button_t* b)) {
    b->type = ui_view_button;
    ui_view.set_text(b, "%s", label);
    b->callback = callback;
    b->min_w_em = ems;
    ui_view_init_button(b);
}
