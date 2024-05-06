#include "ut/ut.h"
#include "ui/ui.h"

static void ui_label_paint(ui_view_t* view) {
    assert(view->type == ui_view_label);
    assert(!view->hidden);
    ui_label_t* t = (ui_label_t*)view;
    // at later stages of layout text height can grow:
    ui_gdi.push(view->x, view->y + t->dy);
    ui_font_t f = *view->font;
    ui_gdi.set_font(f);
//  traceln("%s h=%d dy=%d baseline=%d", view->text, view->h, t->dy, view->baseline);
    ui_color_t c = view->hover && t->highlight && !t->label ?
        ui_colors.text_highlight : view->color;
    ui_gdi.set_text_color(c);
    // paint for text also does lightweight re-layout
    // which is useful for simplifying dynamic text changes
    bool multiline = strchr(t->view.text, '\n') != null;
    if (!multiline) {
        ui_gdi.text("%s", ui_view.nls(view));
    } else {
        int32_t w = (int32_t)(view->min_w_em * view->em.x + 0.5);
        ui_gdi.multiline(w == 0 ? -1 : w, "%s", ui_view.nls(view));
    }
    if (view->hover && t->hovered && !t->label) {
        ui_gdi.set_colored_pen(ui_colors.btn_hover_highlight);
        ui_gdi.set_brush(ui_gdi.brush_hollow);
        int32_t cr = view->em.y / 4; // corner radius
        int32_t h = multiline ? view->h : view->baseline + view->descent;
        ui_gdi.rounded(view->x - cr, view->y + t->dy, view->w + 2 * cr,
            h, cr, cr);
    }
    ui_gdi.pop();
}

static void ui_label_context_menu(ui_view_t* view) {
    assert(view->type == ui_view_label);
    ui_label_t* t = (ui_label_t*)view;
    if (!t->label && !ui_view.is_hidden(view) && !ui_view.is_disabled(view)) {
        ut_clipboard.put_text(ui_view.nls(view));
        static bool first_time = true;
        app.toast(first_time ? 2.15 : 0.75,
            nls.str("Text copied to clipboard"));
        first_time = false;
    }
}

static void ui_label_character(ui_view_t* view, const char* utf8) {
    assert(view->type == ui_view_label);
    ui_label_t* t = (ui_label_t*)view;
    if (view->hover && !t->label &&
       !ui_view.is_hidden(view) && !ui_view.is_disabled(view)) {
        char ch = utf8[0];
        // Copy to clipboard works for hover over text
        if ((ch == 3 || ch == 'c' || ch == 'C') && app.ctrl) {
            ut_clipboard.put_text(ui_view.nls(view)); // 3 is ASCII for Ctrl+C
        }
    }
}

void ui_view_init_label(ui_view_t* view) {
    static_assert(offsetof(ui_label_t, view) == 0, "offsetof(.view)");
    assert(view->type == ui_view_label);
    ui_view_init(view);
    view->color = ui_colors.text;
    view->paint = ui_label_paint;
    view->character = ui_label_character;
    view->context_menu = ui_label_context_menu;
}

void ui_label_init_va(ui_label_t* t, fp32_t min_w_em, const char* format, va_list vl) {
    static_assert(offsetof(ui_label_t, view) == 0, "offsetof(.view)");
    ut_str.format_va(t->view.text, countof(t->view.text), format, vl);
    t->view.min_w_em = min_w_em;
    t->view.type = ui_view_label;
    ui_view_init_label(&t->view);
}

void ui_label_init(ui_label_t* t, fp32_t min_w_em, const char* format, ...) {
    va_list vl;
    va_start(vl, format);
    ui_label_init_va(t, min_w_em, format, vl);
    va_end(vl);
}
