#include "ut/ut.h"
#include "ui/ui.h"

static void ui_view_invalidate(const ui_view_t* view) {
    ui_rect_t rc = { view->x, view->y, view->w, view->h};
    rc.x -= view->em.x;
    rc.y -= view->em.y;
    rc.w += view->em.x * 2;
    rc.h += view->em.y * 2;
    app.invalidate(&rc);
}

static const char* ui_view_nls(ui_view_t* view) {
    return view->strid != 0 ?
        app.string(view->strid, view->text) : view->text;
}

static void ui_view_measure(ui_view_t* view) {
    ui_font_t f = view->font != null ? *view->font : app.fonts.regular;
    view->em = gdi.get_em(f);
    assert(view->em.x > 0 && view->em.y > 0);
    view->w = (int32_t)(view->em.x * view->width + 0.5);
    ui_point_t mt = { 0 };
    if (view->type == ui_view_text && ((ui_label_t*)view)->multiline) {
        int32_t w = (int)(view->width * view->em.x + 0.5);
        mt = gdi.measure_multiline(f, w == 0 ? -1 : w, view->nls(view));
    } else {
        mt = gdi.measure_text(f, view->nls(view));
    }
    view->h = mt.y;
    view->w = max(view->w, mt.x);
    view->baseline = gdi.baseline(f);
    view->descent  = gdi.descent(f);
}

static void ui_view_set_text(ui_view_t* view, const char* text) {
    int32_t n = (int32_t)strlen(text);
    strprintf(view->text, "%s", text);
    view->strid = 0; // next call to nls() will localize this text
    for (int32_t i = 0; i < n; i++) {
        if (text[i] == '&' && i < n - 1 && text[i + 1] != '&') {
            view->shortcut = text[i + 1];
            break;
        }
    }
}

static void ui_view_localize(ui_view_t* view) {
    if (view->text[0] != 0) {
        view->strid = app.strid(view->text);
    }
}

static bool ui_view_hidden_or_disabled(ui_view_t* view) {
    return app.is_hidden(view) || app.is_disabled(view);
}

static void ui_view_hovering(ui_view_t* view, bool start) {
    static ui_text(btn_tooltip,  "");
    if (start && app.toasting.view == null && view->tip[0] != 0 &&
       !app.is_hidden(view)) {
        strprintf(btn_tooltip.view.text, "%s", app.nls(view->tip));
        btn_tooltip.view.font = &app.fonts.H1;
        int32_t y = app.mouse.y - view->em.y;
        // enough space above? if not show below
        if (y < view->em.y) { y = app.mouse.y + view->em.y * 3 / 2; }
        y = min(app.crc.h - view->em.y * 3 / 2, max(0, y));
        app.show_tooltip(&btn_tooltip.view, app.mouse.x, y, 0);
    } else if (!start && app.toasting.view == &btn_tooltip.view) {
        app.show_tooltip(null, -1, -1, 0);
    }
}

static bool ui_view_is_keyboard_shortcut(ui_view_t* view, int32_t key) {
    // Supported keyboard shortcuts are ASCII characters only for now
    // If there is not focused UI control in Alt+key [Alt] is optional.
    // If there is focused control only Alt+Key is accepted as shortcut
    char ch = 0x20 <= key && key <= 0x7F ? (char)toupper(key) : 0x00;
    bool need_alt = app.focus != null && app.focus != view;
    bool keyboard_shortcut = ch != 0x00 && view->shortcut != 0x00 &&
         (app.alt || !need_alt) && toupper(view->shortcut) == ch;
    return keyboard_shortcut;
}

void ui_view_init(ui_view_t* view) {
    view->set_text   = ui_view_set_text;
    view->invalidate = ui_view_invalidate;
    view->nls        = ui_view_nls;
    view->localize   = ui_view_localize;
    view->measure    = ui_view_measure;
    view->hovering   = ui_view_hovering;
    view->hover_delay = 1.5;
    view->is_keyboard_shortcut = ui_view_is_keyboard_shortcut;
}
