#include "ut/ut.h"
#include "ui/ui.h"

static int ui_toggle_paint_on_off(ui_view_t* v) {
    ui_gdi.push(v->x, v->y);
    ui_color_t b = v->background;
    if (!ui_theme.are_apps_dark()) { b = ui_colors.darken(b, 0.25f); }
    ui_color_t background = v->pressed ? ui_colors.tone_green : b;
    ui_gdi.set_text_color(background);
    int32_t x = v->x;
    int32_t x1 = v->x + v->fm->em.w * 3 / 4;
    while (x < x1) {
        ui_gdi.x = x;
        ui_gdi.text("%s", ui_glyph_black_large_circle);
        x++;
    }
    int32_t rx = ui_gdi.x;
    ui_gdi.x = v->pressed ? x : v->x;
    ui_color_t c = ui_gdi.set_text_color(v->color);
    ui_gdi.text("%s", ui_glyph_black_large_circle);
    ui_gdi.set_text_color(c);
    ui_gdi.pop();
    return rx;
}

static const char* ui_toggle_on_off_label(ui_view_t* v,
        char* label, int32_t count)  {
    ut_str.format(label, count, "%s", ui_view.nls(v));
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
    char text[countof(v->text)];
    const char* label = ui_toggle_on_off_label(v, text, countof(text));
    ui_gdi.push(v->x, v->y);
    ui_font_t f = ui_gdi.set_font(v->fm->font);
    ui_gdi.x = ui_toggle_paint_on_off(v) + v->fm->em.w * 3 / 4;
    ui_color_t c = ui_gdi.set_text_color(v->color);
    ui_gdi.text("%s", label);
    ui_gdi.set_text_color(c);
    ui_gdi.set_font(f);
    ui_gdi.pop();
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
    ui_view.set_text(v, v->text);
    v->mouse         = ui_toggle_mouse;
    v->paint         = ui_toggle_paint;
    v->measured = ui_toggle_measured;
    v->character     = ui_toggle_character;
    v->key_pressed   = ui_toggle_key_pressed;
    v->color_id      = ui_color_id_button_text;
    v->background_id = ui_color_id_button_face;
    ui_view.localize(v);
}

void ui_toggle_init(ui_toggle_t* t, const char* label, fp32_t ems,
       void (*callback)(ui_toggle_t* b)) {
    ut_str_printf(t->text, "%s", label);
    t->min_w_em = ems;
    t->callback = callback;
    t->type = ui_view_toggle;
    ui_view_init_toggle(t);
}
