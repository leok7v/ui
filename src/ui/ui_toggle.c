#include "ut/ut.h"
#include "ui/ui.h"

static int ui_toggle_paint_on_off(ui_view_t* view) {
    // https://www.compart.com/en/unicode/U+2B24
    static const char* circle = "\xE2\xAC\xA4"; // Black Large Circle
    gdi.push(view->x, view->y);
    ui_color_t background = view->pressed ? ui_colors.tone_green : ui_colors.dkgray4;
    ui_color_t foreground = view->color;
    gdi.set_text_color(background);
    int32_t x = view->x;
    int32_t x1 = view->x + view->em.x * 3 / 4;
    while (x < x1) {
        gdi.x = x;
        gdi.text("%s", circle);
        x++;
    }
    int32_t rx = gdi.x;
    gdi.set_text_color(foreground);
    gdi.x = view->pressed ? x : view->x;
    gdi.text("%s", circle);
    gdi.pop();
    return rx;
}

static const char* ui_toggle_on_off_label(ui_view_t* view, char* label, int32_t count)  {
    ut_str.format(label, count, "%s", ui_view.nls(view));
    char* s = strstr(label, "___");
    if (s != null) {
        memcpy(s, view->pressed ? "On " : "Off", 3);
    }
    return nls.str(label);
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
    gdi.push(view->x, view->y);
    ui_font_t f = *view->font;
    ui_font_t font = gdi.set_font(f);
    gdi.x = ui_toggle_paint_on_off(view) + view->em.x * 3 / 4;
    gdi.text("%s", label);
    gdi.set_font(font);
    gdi.pop();
}

static void ui_toggle_flip(ui_toggle_t* c) {
    assert(c->view.type == ui_view_toggle);
    app.redraw();
    c->view.pressed = !c->view.pressed;
    if (c->cb != null) { c->cb(c); }
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
    if (app.alt && ui_view.is_shortcut_key(view, key)) {
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
        int32_t x = app.mouse.x - view->x;
        int32_t y = app.mouse.y - view->y;
        if (0 <= x && x < view->w && 0 <= y && y < view->h) {
            app.focus = view;
            ui_toggle_flip((ui_toggle_t*)view);
        }
    }
}

void ui_toggle_init_(ui_view_t* view) {
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

void ui_toggle_init(ui_toggle_t* c, const char* label, fp64_t ems,
       void (*cb)(ui_toggle_t* b)) {
    static_assert(offsetof(ui_toggle_t, view) == 0, "offsetof(.view)");
    ui_view_init(&c->view);
    strprintf(c->view.text, "%s", label);
    c->view.width = ems;
    c->cb = cb;
    c->view.type = ui_view_toggle;
    ui_toggle_init_(&c->view);
}
