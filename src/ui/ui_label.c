#include "ut/ut.h"
#include "ui/ui.h"

static void ui_label_paint(ui_view_t* v) {
    assert(v->type == ui_view_label);
    assert(!v->hidden);
    // at later stages of layout text height can grow:
    ui_gdi.push(v->x, v->y + v->label_dy);
    ui_font_t f = *v->font;
    ui_gdi.set_font(f);
//  traceln("%s h=%d dy=%d baseline=%d", v->text, v->h,
//          v->label_dy, v->baseline);
    ui_color_t c = v->hover && v->highlightable ?
        ui_colors.text_highlight : v->color;
    ui_gdi.set_text_color(c);
    // paint for text also does lightweight re-layout
    // which is useful for simplifying dynamic text changes
    bool multiline = strchr(v->text, '\n') != null;
    if (!multiline) {
        ui_gdi.text("%s", ui_view.nls(v));
    } else {
        int32_t w = (int32_t)(v->min_w_em * v->em.body.w + 0.5);
        ui_gdi.multiline(w == 0 ? -1 : w, "%s", ui_view.nls(v));
    }
    if (v->hover && !v->flat && v->highlightable) {
        // ui_colors.btn_hover_highlight
        ui_gdi.set_colored_pen(ui_app.get_color(ui_color_id_highlight));
        ui_gdi.set_brush(ui_gdi.brush_hollow);
        int32_t cr = v->em.body.h / 4; // corner radius
        int32_t h = multiline ? v->h : v->em.baseline + v->em.descent;
        ui_gdi.rounded(v->x - cr, v->y + v->label_dy,
                       v->w + 2 * cr, h, cr, cr);
    }
    ui_gdi.pop();
}

static void ui_label_context_menu(ui_view_t* v) {
    assert(v->type == ui_view_label);
    if (!ui_view.is_hidden(v) && !ui_view.is_disabled(v)) {
        ut_clipboard.put_text(ui_view.nls(v));
        static bool first_time = true;
        ui_app.toast(first_time ? 2.2 : 0.5,
            ut_nls.str("Text copied to clipboard"));
        first_time = false;
    }
}

static void ui_label_character(ui_view_t* v, const char* utf8) {
    assert(v->type == ui_view_label);
    if (v->hover && !ui_view.is_hidden(v)) {
        char ch = utf8[0];
        // Copy to clipboard works for hover over text
        if ((ch == 3 || ch == 'c' || ch == 'C') && ui_app.ctrl) {
            ut_clipboard.put_text(ui_view.nls(v)); // 3 is ASCII for Ctrl+C
        }
    }
}

void ui_view_init_label(ui_view_t* v) {
    assert(v->type == ui_view_label);
    ui_view_init(v);
    v->color_id     = ui_color_id_window_text;
    v->paint        = ui_label_paint;
    v->character    = ui_label_character;
    v->context_menu = ui_label_context_menu;
}

void ui_label_init_va(ui_label_t* v, fp32_t min_w_em,
        const char* format, va_list vl) {
    ut_str.format_va(v->text, countof(v->text), format, vl);
    v->min_w_em = min_w_em;
    v->type = ui_view_label;
    ui_view_init_label(v);
}

void ui_label_init(ui_label_t* v, fp32_t min_w_em, const char* format, ...) {
    va_list vl;
    va_start(vl, format);
    ui_label_init_va(v, min_w_em, format, vl);
    va_end(vl);
}
