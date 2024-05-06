#include "ut/ut.h"
#include "ui/ui.h"

static int ui_toggle_paint_on_off(ui_view_t* view) {
    ui_gdi.push(view->x, view->y);
    ui_color_t background = view->pressed ? ui_colors.tone_green : ui_colors.dkgray4;
    ui_color_t foreground = view->color;
    ui_gdi.set_text_color(background);
    int32_t x = view->x;
    int32_t x1 = view->x + view->em.x * 3 / 4;
    while (x < x1) {
        ui_gdi.x = x;
        ui_gdi.text("%s", ui_glyph_black_large_circle);
        x++;
    }
    int32_t rx = ui_gdi.x;
    ui_gdi.set_text_color(foreground);
    ui_gdi.x = view->pressed ? x : view->x;
    ui_gdi.text("%s", ui_glyph_black_large_circle);
    ui_gdi.pop();
    return rx;
}

static const char* ui_toggle_on_off_label(ui_view_t* view, char* label, int32_t count)  {
    ut_str.format(label, count, "%s", ui_view.nls(view));
    char* s = strstr(label, "___");
    if (s != null) {
        memcpy(s, view->pressed ? "On " : "Off", 3);
    }
    return ui_nls.str(label);
}

static void ui_toggle_measure(ui_view_t* view) {
    assert(view->type == ui_view_toggle);
    ui_view.measure(view);
    view->w += view->em.x * 2;
}

static void ui_toggle_paint(ui_view_t* view) {
    assert(view->type == ui_view_toggle);
    char text[countof(view->text)];
    const char* label = ui_toggle_on_off_label(view, text, countof(text));
    ui_gdi.push(view->x, view->y);
    ui_font_t f = *view->font;
    ui_font_t font = ui_gdi.set_font(f);
    ui_gdi.x = ui_toggle_paint_on_off(view) + view->em.x * 3 / 4;
    ui_gdi.text("%s", label);
    ui_gdi.set_font(font);
    ui_gdi.pop();
}

static void ui_toggle_flip(ui_toggle_t* c) {
    assert(c->type == ui_view_toggle);
    ui_app.redraw();
    c->pressed = !c->pressed;
    if (c->callback != null) { c->callback(c); }
}

static void ui_toggle_character(ui_view_t* view, const char* utf8) {
    assert(view->type == ui_view_toggle);
    assert(!view->hidden && !view->disabled);
    char ch = utf8[0];
    if (ui_view.is_shortcut_key(view, ch)) {
         ui_toggle_flip((ui_toggle_t*)view);
    }
}

static void ui_toggle_key_pressed(ui_view_t* view, int64_t key) {
    if (ui_app.alt && ui_view.is_shortcut_key(view, key)) {
//      traceln("key: 0x%02X shortcut: %d", key, ui_view.is_shortcut_key(view, key));
        ui_toggle_flip((ui_toggle_t*)view);
    }
}

static void ui_toggle_mouse(ui_view_t* view, int32_t message, int64_t flags) {
    assert(view->type == ui_view_toggle);
    (void)flags; // unused
    assert(!view->hidden && !view->disabled);
    if (message == ui.message.left_button_pressed ||
        message == ui.message.right_button_pressed) {
        int32_t x = ui_app.mouse.x - view->x;
        int32_t y = ui_app.mouse.y - view->y;
        if (0 <= x && x < view->w && 0 <= y && y < view->h) {
            ui_app.focus = view;
            ui_toggle_flip((ui_toggle_t*)view);
        }
    }
}

void ui_view_init_toggle(ui_view_t* view) {
    assert(view->type == ui_view_toggle);
    ui_view_init(view);
    ui_view.set_text(view, view->text);
    view->mouse       = ui_toggle_mouse;
    view->measure     = ui_toggle_measure;
    view->paint       = ui_toggle_paint;
    view->character   = ui_toggle_character;
    view->key_pressed = ui_toggle_key_pressed;
    ui_view.localize(view);
    view->color = ui_colors.btn_text;
}

void ui_toggle_init(ui_toggle_t* c, const char* label, fp32_t ems,
       void (*callback)(ui_toggle_t* b)) {
    ui_view_init(c);
    strprintf(c->text, "%s", label);
    c->min_w_em = ems;
    c->callback = callback;
    c->type = ui_view_toggle;
    ui_view_init_toggle(c);
}
