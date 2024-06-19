#include "ut/ut.h"
#include "ui/ui.h"

static int32_t ui_toggle_paint_on_off(ui_view_t* v, int32_t x, int32_t y) {
    ui_color_t c = v->background;
    if (!ui_theme.is_app_dark()) { c = ui_colors.darken(c, 0.25f); }
    ui_color_t b = v->pressed ? ui_colors.tone_green : c;
    const int32_t bl = v->fm->baseline;
    const int32_t a = v->fm->ascent;
    const int32_t d = v->fm->descent;
//  traceln("v->fm baseline: %d ascent: %d descent: %d", bl, a, d);
    const int32_t w = v->fm->em.w * 3 / 4;
    int32_t h = a + d;
    int32_t r = h / 2;
    if (r % 2 == 0) { r--; }
    h = r * 2 + 1;
    y += bl - h;
    int32_t y1 = y + h - r + 1;
    ui_gdi.circle(x, y1, r, b, b);
    ui_gdi.circle(x + w - r, y1, r, b, b);
    ui_gdi.fill(x, y1 - r, w - r + 1, h, b);
    int32_t x1 = v->pressed ? x + w - r : x;
    // circle is too bold in control color - water it down
    ui_color_t fill = ui_theme.is_app_dark() ?
        ui_colors.darken(v->color, 0.5f) : ui_colors.lighten(v->color, 0.5f);
    ui_color_t border = ui_theme.is_app_dark() ?
        ui_colors.darken(fill, 0.0625f) : ui_colors.lighten(fill, 0.0625f);
    ui_gdi.circle(x1, y1, r, border, fill);
    return x + w;
}

static const char* ui_toggle_on_off_label(ui_view_t* v,
        char* label, int32_t count)  {
    ut_str.format(label, count, "%s", ui_view.string(v));
    char* s = strstr(label, "___");
    if (s != null) {
        memcpy(s, v->pressed ? "On " : "Off", 3);
    }
    return ut_nls.str(label);
}

static void ui_toggle_measured(ui_view_t* v) {
    assert(v->type == ui_view_toggle);
    v->w += v->fm->em.w * 2;
}

static void ui_toggle_paint(ui_view_t* v) {
    assert(v->type == ui_view_toggle);
    char text[countof(v->p.text)];
    const char* label = ui_toggle_on_off_label(v, text, countof(text));
    ui_ltrb_t i = ui_view.gaps(v, &v->insets);
    const int32_t tx = v->x + i.left;
    const int32_t ty = v->y + i.top;
    const int32_t x = ui_toggle_paint_on_off(v, tx, ty) + v->fm->em.w / 4;
    const ui_gdi_ta_t ta = { .fm = v->fm, .color = v->color };
    ui_gdi.text(&ta, x, ty, "%s", ut_nls.str(label));
}

static void ui_toggle_flip(ui_toggle_t* t) {
    assert(t->type == ui_view_toggle);
    ui_app.request_redraw();
    t->pressed = !t->pressed;
    if (t->callback != null) { t->callback(t); }
}

static void ui_toggle_character(ui_view_t* v, const char* utf8) {
    assert(v->type == ui_view_toggle);
    assert(!v->hidden && !v->disabled);
    char ch = utf8[0];
    if (ui_view.is_shortcut_key(v, ch)) {
         ui_toggle_flip((ui_toggle_t*)v);
    }
}

static void ui_toggle_key_pressed(ui_view_t* v, int64_t key) {
    if (ui_app.alt && ui_view.is_shortcut_key(v, key)) {
//      traceln("key: 0x%02X shortcut: %d", key, ui_view.is_shortcut_key(view, key));
        ui_toggle_flip((ui_toggle_t*)v);
    }
}

static void ui_toggle_mouse(ui_view_t* v, int32_t message, int64_t unused(flags)) {
    assert(v->type == ui_view_toggle);
    assert(!v->hidden && !v->disabled);
    if (message == ui.message.left_button_pressed ||
        message == ui.message.right_button_pressed) {
        int32_t x = ui_app.mouse.x - v->x;
        int32_t y = ui_app.mouse.y - v->y;
        if (0 <= x && x < v->w && 0 <= y && y < v->h) {
            ui_app.focus = v;
            ui_toggle_flip((ui_toggle_t*)v);
        }
    }
}

void ui_view_init_toggle(ui_view_t* v) {
    assert(v->type == ui_view_toggle);
    v->mouse         = ui_toggle_mouse;
    v->paint         = ui_toggle_paint;
    v->measured      = ui_toggle_measured;
    v->character     = ui_toggle_character;
    v->key_pressed   = ui_toggle_key_pressed;
    v->color_id      = ui_color_id_button_text;
    v->background_id = ui_color_id_button_face;
}

void ui_toggle_init(ui_toggle_t* t, const char* label, fp32_t ems,
       void (*callback)(ui_toggle_t* b)) {
    ui_view.set_text(t, "%s", label);
    t->min_w_em = ems;
    t->callback = callback;
    t->type = ui_view_toggle;
    ui_view_init_toggle(t);
}
