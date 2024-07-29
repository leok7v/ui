#include "rt/rt.h"
#include "ui/ui.h"

static void ui_toggle_paint_on_off(ui_view_t* v) {
    const ui_ltrb_t i = ui_view.margins(v, &v->insets);
    int32_t x = v->x;
    int32_t y = v->y + i.top;
    ui_color_t c = ui_colors.darken(v->background,
        !ui_theme.is_app_dark() ? 0.125f : 0.5f);
    ui_color_t b = v->state.pressed ? ui_colors.tone_green : c;
    const int32_t a = v->fm->ascent;
    const int32_t d = v->fm->descent;
    const int32_t w = v->fm->em.w;
    int32_t r = ((a + d + 1) / 2) | 0x1; // radius must be odd
    int32_t h = r * 2 + 1;
    y += (v->h - i.top - i.bottom - h + 1) / 2;
    y += r + 1; // because radius is odd
    x += r;
    ui_color_t border = ui_theme.is_app_dark() ?
        ui_colors.darken(v->color, 0.5) :
        ui_colors.lighten(v->color, 0.5);
    if (v->state.hover) {
        border = ui_colors.get_color(ui_color_id_hot_tracking);
    }
    ui_gdi.circle(x, y, r, border, b);
    ui_gdi.circle(x + w - r, y, r, border, b);
    ui_gdi.fill(x, y - r, w - r + 1, h, b);
    ui_gdi.line(x, y - r, x + w - r + 1, y - r, border);
    ui_gdi.line(x, y + r, x + w - r + 1, y + r, border);
    int32_t x1 = v->state.pressed ? x + w - r : x;
    // circle is too bold in control color - water it down
    ui_color_t fill = ui_theme.is_app_dark() ?
        ui_colors.darken(v->color, 0.5f) : ui_colors.lighten(v->color, 0.5f);
    border = ui_theme.is_app_dark() ?
        ui_colors.darken(fill, 0.0625f) : ui_colors.lighten(fill, 0.0625f);
    ui_gdi.circle(x1, y, r - 2, border, fill);
}

static const char* ui_toggle_on_off_label(ui_view_t* v,
        char* label, int32_t count)  {
    rt_str.format(label, count, "%s", ui_view.string(v));
    char* s = strstr(label, "___");
    if (s != null) {
        memcpy(s, v->state.pressed ? "On " : "Off", 3);
    }
    return rt_nls.str(label);
}

static void ui_toggle_measure(ui_view_t* v) {
    if (v->min_w_em < 3.0f) {
        rt_println("3.0f em minimum width");
        v->min_w_em = 4.0f;
    }
    ui_view.measure_control(v);
    rt_assert(v->type == ui_view_toggle);
}

static void ui_toggle_paint(ui_view_t* v) {
    rt_assert(v->type == ui_view_toggle);
    char txt[rt_countof(v->p.text)];
    const char* label = ui_toggle_on_off_label(v, txt, rt_countof(txt));
    const char* text = rt_nls.str(label);
    ui_view.text_measure(v, text, &v->text);
    ui_view.text_align(v, &v->text);
    ui_toggle_paint_on_off(v);
    const ui_color_t text_color = !v->state.hover ? v->color :
            (ui_theme.is_app_dark() ? ui_colors.white : ui_colors.black);
    const ui_gdi_ta_t ta = { .fm = v->fm, .color = text_color };
    ui_gdi.text(&ta, v->x + v->text.xy.x, v->y + v->text.xy.y, "%s", text);
}

static void ui_toggle_flip(ui_toggle_t* t) {
    ui_view.invalidate((ui_view_t*)t, null);
    t->state.pressed = !t->state.pressed;
    if (t->callback != null) { t->callback(t); }
}

static void ui_toggle_character(ui_view_t* v, const char* utf8) {
    char ch = utf8[0];
    if (ui_view.is_shortcut_key(v, ch)) {
         ui_toggle_flip((ui_toggle_t*)v);
    }
}

static bool ui_toggle_key_pressed(ui_view_t* v, int64_t key) {
    const bool trigger = ui_app.alt && ui_view.is_shortcut_key(v, key);
    if (trigger) { ui_toggle_flip((ui_toggle_t*)v); }
    return trigger; // swallow if true
}

static bool ui_toggle_tap(ui_view_t* v, int32_t rt_unused(ix),
        bool pressed) {
    const bool inside = ui_view.inside(v, &ui_app.mouse);
    if (pressed && inside) { ui_toggle_flip((ui_toggle_t*)v); }
    return pressed && inside;
}

void ui_view_init_toggle(ui_view_t* v) {
    rt_assert(v->type == ui_view_toggle);
    v->tap           = ui_toggle_tap;
    v->paint         = ui_toggle_paint;
    v->measure       = ui_toggle_measure;
    v->character     = ui_toggle_character;
    v->key_pressed   = ui_toggle_key_pressed;
    v->color_id      = ui_color_id_button_text;
    v->background_id = ui_color_id_button_face;
    v->text_align    = ui.align.left;
    if (v->debug.id == null) { v->debug.id = "#toggle"; }
}

void ui_toggle_init(ui_toggle_t* t, const char* label, fp32_t ems,
       void (*callback)(ui_toggle_t* b)) {
    ui_view.set_text(t, "%s", label);
    t->min_w_em = ems;
    t->callback = callback;
    t->type = ui_view_toggle;
    ui_view_init_toggle(t);
}
