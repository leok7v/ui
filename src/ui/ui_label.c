#include "rt/rt.h"
#include "ui/ui.h"

static void ui_label_paint(ui_view_t* v) {
    rt_assert(v->type == ui_view_label);
    rt_assert(!ui_view.is_hidden(v));
    const char* s = ui_view.string(v);
    ui_color_t c = v->state.hover && v->highlightable ?
        ui_colors.interpolate(v->color, ui_colors.blue, 1.0f / 8.0f) :
        v->color;
    const int32_t tx = v->x + v->text.xy.x;
    const int32_t ty = v->y + v->text.xy.y;
    const ui_gdi_ta_t ta = { .fm = v->fm, .color = c };
    const bool multiline = strchr(s, '\n') != null;
    if (multiline) {
        int32_t w = (int32_t)((fp64_t)v->min_w_em * (fp64_t)v->fm->em.w + 0.5);
        ui_gdi.multiline(&ta, tx, ty, w, "%s", ui_view.string(v));
    } else {
        ui_gdi.text(&ta, tx, ty, "%s", ui_view.string(v));
    }
    if (v->state.hover && !v->flat && v->highlightable) {
        ui_color_t highlight = ui_colors.get_color(ui_color_id_highlight);
        int32_t radius = (v->fm->em.h / 4) | 0x1; // corner radius
        int32_t h = multiline ? v->h : v->fm->baseline + v->fm->descent;
        ui_gdi.rounded(v->x - radius, v->y, v->w + 2 * radius, h,
                       radius, highlight, ui_colors.transparent);
    }
}

static bool ui_label_context_menu(ui_view_t* v) {
    rt_assert(!ui_view.is_hidden(v) && !ui_view.is_disabled(v));
    const bool inside = ui_view.inside(v, &ui_app.mouse);
    if (inside) {
        rt_clipboard.put_text(ui_view.string(v));
        static ui_label_t hint = ui_label(0.0f, "copied to clipboard");
        int32_t x = v->x + v->w / 2;
        int32_t y = v->y + v->h;
        ui_app.show_hint(&hint, x, y, 0.75);
    }
    return inside;
}

static void ui_label_character(ui_view_t* v, const char* utf8) {
    rt_assert(v->type == ui_view_label);
    if (v->state.hover && !ui_view.is_hidden(v)) {
        char ch = utf8[0];
        // Copy to clipboard works for hover over text
        if ((ch == 3 || ch == 'c' || ch == 'C') && ui_app.ctrl) {
            rt_clipboard.put_text(ui_view.string(v)); // 3 is ASCII for Ctrl+C
        }
    }
}

void ui_view_init_label(ui_view_t* v) {
    rt_assert(v->type == ui_view_label);
    v->paint         = ui_label_paint;
    v->character     = ui_label_character;
    v->context_menu  = ui_label_context_menu;
    v->color_id      = ui_color_id_button_text;
    v->background_id = ui_color_id_button_face;
    v->text_align    = ui.align.left;
}

void ui_label_init_va(ui_label_t* v, fp32_t min_w_em,
        const char* format, va_list va) {
    ui_view.set_text(v, format, va);
    v->min_w_em = min_w_em;
    v->type = ui_view_label;
    ui_view_init_label(v);
}

void ui_label_init(ui_label_t* v, fp32_t min_w_em, const char* format, ...) {
    va_list va;
    va_start(va, format);
    ui_label_init_va(v, min_w_em, format, va);
    va_end(va);
}
