#include "ut/ut.h"
#include "ui/ui.h"

static const fp64_t ui_view_hover_delay = 1.5; // seconds

#pragma push_macro("ui_view_for_each")

static void ui_view_update_shortcut(ui_view_t* v);

// adding and removing views is not expected to be frequent
// actions by application code (human factor - UI design)
// thus extra checks and verifications are there even in
// release code because C is not type safety champion language.

static inline void ui_view_check_type(ui_view_t* v) {
    // little endian:
    ut_static_assertion(('vwXX' & 0xFFFF0000U) == ('vwZZ' & 0xFFFF0000U));
    ut_static_assertion((ui_view_stack & 0xFFFF0000U) == ('vwXX' & 0xFFFF0000U));
    swear(((uint32_t)v->type & 0xFFFF0000U) == ('vwXX'  & 0xFFFF0000U),
          "not a view: %4.4s 0x%08X (forgotten &static_view?)",
          &v->type, v->type);
}

static void ui_view_verify(ui_view_t* p) {
    ui_view_check_type(p);
    ui_view_for_each(p, c, {
        ui_view_check_type(c);
        ui_view_update_shortcut(c);
        swear(c->parent == p);
        swear(c == c->next->prev);
        swear(c == c->prev->next);
    });
}

static ui_view_t* ui_view_add(ui_view_t* p, ...) {
    va_list va;
    va_start(va, p);
    ui_view_t* c = va_arg(va, ui_view_t*);
    while (c != null) {
        swear(c->parent == null && c->prev == null && c->next == null);
        ui_view.add_last(p, c);
        c = va_arg(va, ui_view_t*);
    }
    va_end(va);
    ui_view_call_init(p);
    ui_app.request_layout();
    return p;
}

static void ui_view_add_first(ui_view_t* p, ui_view_t* c) {
    swear(c->parent == null && c->prev == null && c->next == null);
    c->parent = p;
    if (p->child == null) {
        c->prev = c;
        c->next = c;
    } else {
        c->prev = p->child->prev;
        c->next = p->child;
        c->prev->next = c;
        c->next->prev = c;
    }
    p->child = c;
    ui_view_call_init(c);
    ui_app.request_layout();
}

static void ui_view_add_last(ui_view_t* p, ui_view_t* c) {
    swear(c->parent == null && c->prev == null && c->next == null);
    c->parent = p;
    if (p->child == null) {
        c->prev = c;
        c->next = c;
        p->child = c;
    } else {
        c->prev = p->child->prev;
        c->next = p->child;
        c->prev->next = c;
        c->next->prev = c;
    }
    ui_view_call_init(c);
    ui_view_verify(p);
    ui_app.request_layout();
}

static void ui_view_add_after(ui_view_t* c, ui_view_t* a) {
    swear(c->parent == null && c->prev == null && c->next == null);
    ut_not_null(a->parent);
    c->parent = a->parent;
    c->next = a->next;
    c->prev = a;
    a->next = c;
    c->prev->next = c;
    c->next->prev = c;
    ui_view_call_init(c);
    ui_view_verify(c->parent);
    ui_app.request_layout();
}

static void ui_view_add_before(ui_view_t* c, ui_view_t* b) {
    swear(c->parent == null && c->prev == null && c->next == null);
    ut_not_null(b->parent);
    c->parent = b->parent;
    c->prev = b->prev;
    c->next = b;
    b->prev = c;
    c->prev->next = c;
    c->next->prev = c;
    ui_view_call_init(c);
    ui_view_verify(c->parent);
    ui_app.request_layout();
}

static void ui_view_remove(ui_view_t* c) {
    ut_not_null(c->parent);
    ut_not_null(c->parent->child);
    // if a view that has focus is removed from parent:
    if (c == ui_app.focus) { ui_view.set_focus(null); }
    if (c->prev == c) {
        swear(c->next == c);
        c->parent->child = null;
    } else {
        c->prev->next = c->next;
        c->next->prev = c->prev;
        if (c->parent->child == c) {
            c->parent->child = c->next;
        }
    }
    c->prev = null;
    c->next = null;
    ui_view_verify(c->parent);
    c->parent = null;
    ui_app.request_layout();
}

static void ui_view_remove_all(ui_view_t* p) {
    while (p->child != null) { ui_view.remove(p->child); }
    ui_app.request_layout();
}

static void ui_view_disband(ui_view_t* p) {
    // do not disband composite controls
    if (p->type != ui_view_mbx && p->type != ui_view_slider) {
        while (p->child != null) {
            ui_view_disband(p->child);
            ui_view.remove(p->child);
        }
    }
    ui_app.request_layout();
}

static void ui_view_invalidate(const ui_view_t* v, const ui_rect_t* r) {
    if (ui_view.is_hidden(v)) {
        ut_traceln("hidden: %s", ui_view_debug_id(v));
    } else {
        ui_rect_t rc = {0};
        if (r != null) {
            rc = (ui_rect_t){
                .x = v->x + r->x,
                .y = v->y + r->y,
                .w = r->w,
                .h = r->h
            };
        } else {
            rc = (ui_rect_t){ v->x, v->y, v->w, v->h};
            // expand view rectangle by padding
            const ui_ltrb_t p = ui_view.margins(v, &v->padding);
            rc.x -= p.left;
            rc.y -= p.top;
            rc.w += p.left + p.right;
            rc.h += p.top + p.bottom;
        }
        if (v->debug.trace.prc) {
            ut_traceln("%d,%d %dx%d", rc.x, rc.y, rc.w, rc.h);
        }
        ui_app.invalidate(&rc);
    }
}

static const char* ui_view_string(ui_view_t* v) {
    if (v->p.strid == 0) {
        int32_t id = ut_nls.strid(v->p.text);
        v->p.strid = id > 0 ? id : -1;
    }
    return v->p.strid < 0 ? v->p.text : // not localized
        ut_nls.string(v->p.strid, v->p.text);
}

static ui_wh_t ui_view_text_metrics_va(int32_t x, int32_t y,
        bool multiline, int32_t w, const ui_fm_t* fm,
        const char* format, va_list va) {
    const ui_gdi_ta_t ta = { .fm = fm, .color = ui_colors.transparent,
                             .measure = true };
    return multiline ?
        ui_gdi.multiline_va(&ta, x, y, w, format, va) :
        ui_gdi.text_va(&ta, x, y, format, va);
}

static ui_wh_t ui_view_text_metrics(int32_t x, int32_t y,
        bool multiline, int32_t w, const ui_fm_t* fm,
        const char* format, ...) {
    va_list va;
    va_start(va, format);
    ui_wh_t wh = ui_view_text_metrics_va(x, y, multiline, w, fm, format, va);
    va_end(va);
    return wh;
}

static void ui_view_text_measure(ui_view_t* v, const char* s,
        ui_view_text_metrics_t* tm) {
    const ui_fm_t* fm = v->fm;
    tm->mt = (ui_wh_t){ .w = 0, .h = fm->height };
    if (s[0] == 0) {
        tm->multiline = false;
    } else {
        tm->multiline = strchr(s, '\n') != null;
        if (v->type == ui_view_label && tm->multiline) {
            int32_t w = (int32_t)((fp64_t)v->min_w_em * (fp64_t)fm->em.w + 0.5);
            tm->mt = ui_view.text_metrics(v->x, v->y, true,  w, fm, "%s", s);
        } else {
            tm->mt = ui_view.text_metrics(v->x, v->y, false, 0, fm, "%s", s);
        }
    }
}

static void ui_view_text_align(ui_view_t* v, ui_view_text_metrics_t* tm) {
    const ui_fm_t* fm = v->fm;
    tm->xy = (ui_point_t){ .x = -1, .y = -1 };
    const ui_ltrb_t i = ui_view.margins(v, &v->insets);
    // i_wh the inside insets w x h:
    const ui_wh_t i_wh = { .w = v->w - i.left - i.right,
                           .h = v->h - i.top - i.bottom };
    const int32_t h_align = v->text_align & ~(ui.align.top|ui.align.bottom);
    const int32_t v_align = v->text_align & ~(ui.align.left|ui.align.right);
    tm->xy.x = i.left + (i_wh.w - tm->mt.w) / 2;
    if (h_align & ui.align.left) {
        tm->xy.x = i.left;
    } else if (h_align & ui.align.right) {
        tm->xy.x = i_wh.w - tm->mt.w - i.right;
    }
    // vertical centering is trickier.
    // mt.h is height of all measured lines of text
    tm->xy.y = i.top + (i_wh.h - tm->mt.h) / 2;
    if (v_align & ui.align.top) {
        tm->xy.y = i.top;
    } else if (v_align & ui.align.bottom) {
        tm->xy.y = i_wh.h - tm->mt.h - i.bottom;
    } else if (!tm->multiline) {
        // UI controls should have x-height line in the dead center
        // of the control to be visually balanced.
        // y offset of "x-line" of the glyph:
        const int32_t y_of_x_line = fm->baseline - fm->x_height;
        // `dy` offset of the center to x-line (middle of glyph cell)
        const int32_t dy = tm->mt.h / 2 - y_of_x_line;
        tm->xy.y += dy / 2;
        if (v->debug.trace.mt) {
            ut_traceln(" x-line: %d mt.h: %d mt.h / 2 - x_line: %d",
                      y_of_x_line, tm->mt.h, dy);
        }
    }
}

static void ui_view_measure_control(ui_view_t* v) {
    v->p.strid = 0;
    const char* s = ui_view.string(v);
    const ui_fm_t* fm = v->fm;
    const ui_ltrb_t i = ui_view.margins(v, &v->insets);
    v->w = (int32_t)((fp64_t)fm->em.w * (fp64_t)v->min_w_em + 0.5);
    v->h = (int32_t)((fp64_t)fm->em.h * (fp64_t)v->min_h_em + 0.5);
    if (v->debug.trace.mt) {
        const ui_ltrb_t p = ui_view.margins(v, &v->padding);
        ut_traceln(">%dx%d em: %dx%d min: %.1fx%.1f "
                "i: %d %d %d %d p: %d %d %d %d \"%.*s\"",
            v->w, v->h, fm->em.w, fm->em.h, v->min_w_em, v->min_h_em,
            i.left, i.top, i.right, i.bottom,
            p.left, p.top, p.right, p.bottom,
            ut_min(64, strlen(s)), s);
        const ui_margins_t in = v->insets;
        const ui_margins_t pd = v->padding;
        ut_traceln(" i: %.3f %.3f %.3f %.3f l+r: %.3f t+b: %.3f"
                " p: %.3f %.3f %.3f %.3f l+r: %.3f t+b: %.3f",
            in.left, in.top, in.right, in.bottom,
            in.left + in.right, in.top + in.bottom,
            pd.left, pd.top, pd.right, pd.bottom,
            pd.left + pd.right, pd.top + pd.bottom);
    }
    ui_view_text_measure(v, s, &v->text);
    if (v->debug.trace.mt) {
        ut_traceln(" mt: %d %d", v->text.mt.w, v->text.mt.h);
    }
    v->w = ut_max(v->w, i.left + v->text.mt.w + i.right);
    v->h = ut_max(v->h, i.top  + v->text.mt.h + i.bottom);
    ui_view_text_align(v, &v->text);
    if (v->debug.trace.mt) {
        ut_traceln("<%dx%d text_align x,y: %d,%d",
                v->w, v->h, v->text.xy.x, v->text.xy.y);
    }
}

static void ui_view_measure_children(ui_view_t* v) {
    if (!ui_view.is_hidden(v)) {
        ui_view_for_each(v, c, { ui_view.measure(c); });
    }
}

static void ui_view_measure(ui_view_t* v) {
    if (!ui_view.is_hidden(v)) {
        ui_view_measure_children(v);
        if (v->prepare != null) { v->prepare(v); }
        if (v->measure != null && v->measure != ui_view_measure) {
            v->measure(v);
        } else {
            ui_view.measure_control(v);
        }
        if (v->measured != null) { v->measured(v); }
    }
}

static void ui_layout_view(ui_view_t* unused(v)) {
//  ui_ltrb_t i = ui_view.margins(v, &v->insets);
//  ui_ltrb_t p = ui_view.margins(v, &v->padding);
//  ut_traceln(">%s %d,%d %dx%d p: %d %d %d %d  i: %d %d %d %d",
//               v->p.text, v->x, v->y, v->w, v->h,
//               p.left, p.top, p.right, p.bottom,
//               i.left, i.top, i.right, i.bottom);
//  ut_traceln("<%s %d,%d %dx%d", v->p.text, v->x, v->y, v->w, v->h);
}

static void ui_view_layout_children(ui_view_t* v) {
    if (!ui_view.is_hidden(v)) {
        ui_view_for_each(v, c, { ui_view.layout(c); });
    }
}

static void ui_view_layout(ui_view_t* v) {
//  ut_traceln(">%s %d,%d %dx%d", v->p.text, v->x, v->y, v->w, v->h);
    if (!ui_view.is_hidden(v)) {
        if (v->layout != null && v->layout != ui_view_layout) {
            v->layout(v);
        } else {
            ui_layout_view(v);
        }
        if (v->composed != null) { v->composed(v); }
        ui_view_layout_children(v);
    }
//  ut_traceln("<%s %d,%d %dx%d", v->p.text, v->x, v->y, v->w, v->h);
}

static bool ui_view_inside(const ui_view_t* v, const ui_point_t* pt) {
    const int32_t x = pt->x - v->x;
    const int32_t y = pt->y - v->y;
    return 0 <= x && x < v->w && 0 <= y && y < v->h;
}

static bool ui_view_is_parent_of(const ui_view_t* parent,
        const ui_view_t* child) {
    swear(parent != null && child != null);
    const ui_view_t* p = child->parent;
    while (p != null) {
        if (parent == p) { return true; }
        p = p->parent;
    }
    return false;
}

static ui_ltrb_t ui_view_margins(const ui_view_t* v, const ui_margins_t* g) {
    const fp64_t gw = (fp64_t)g->left + (fp64_t)g->right;
    const fp64_t gh = (fp64_t)g->top  + (fp64_t)g->bottom;
    const ui_wh_t* em = &v->fm->em;
    const int32_t em_w = (int32_t)(em->w * gw + 0.5);
    const int32_t em_h = (int32_t)(em->h * gh + 0.5);
    const int32_t left = (int32_t)((fp64_t)em->w * (fp64_t)g->left + 0.5);
    const int32_t top  = (int32_t)((fp64_t)em->h * (fp64_t)g->top  + 0.5);
    return (ui_ltrb_t) {
        .left   = left,         .top    = top,
        .right  = em_w - left,  .bottom = em_h - top
    };
}

static void ui_view_inbox(const ui_view_t* v, ui_rect_t* r, ui_ltrb_t* insets) {
    swear(r != null || insets != null);
    swear(v->max_w >= 0 && v->max_h >= 0);
    const ui_ltrb_t i = ui_view_margins(v, &v->insets);
    if (insets != null) { *insets = i; }
    if (r != null) {
        *r = (ui_rect_t) {
            .x = v->x + i.left,
            .y = v->y + i.top,
            .w = v->w - i.left - i.right,
            .h = v->h - i.top  - i.bottom,
        };
    }
}

static void ui_view_outbox(const ui_view_t* v, ui_rect_t* r, ui_ltrb_t* padding) {
    swear(r != null || padding != null);
    swear(v->max_w >= 0 && v->max_h >= 0);
    const ui_ltrb_t p = ui_view_margins(v, &v->padding);
    if (padding != null) { *padding = p; }
    if (r != null) {
//      ut_traceln("%s %d,%d %dx%d %.1f %.1f %.1f %.1f", v->p.text,
//          v->x, v->y, v->w, v->h,
//          v->padding.left, v->padding.top, v->padding.right, v->padding.bottom);
        *r = (ui_rect_t) {
            .x = v->x - p.left,
            .y = v->y - p.top,
            .w = v->w + p.left + p.right,
            .h = v->h + p.top  + p.bottom,
        };
//      ut_traceln("%s %d,%d %dx%d", v->p.text,
//          r->x, r->y, r->w, r->h);
    }
}

static void ui_view_update_shortcut(ui_view_t* v) {
    if (ui_view.is_control(v) && v->type != ui_view_text &&
        v->shortcut == 0x00) {
        const char* s = ui_view.string(v);
        const char* a = strchr(s, '&');
        if (a != null && a[1] != 0 && a[1] != '&') {
            // TODO: utf-8 shortcuts? possible
            v->shortcut = a[1];
        }
    }
}

static void ui_view_set_text_va(ui_view_t* v, const char* format, va_list va) {
    char t[ut_countof(v->p.text)];
    ut_str.format_va(t, ut_count_of(t), format, va);
    char* s = v->p.text;
    if (strcmp(s, t) != 0) {
        int32_t n = (int32_t)strlen(t);
        memcpy(s, t, (size_t)n + 1);
        v->p.strid = 0;  // next call to nls() will localize it
        ui_view_update_shortcut(v);
        ui_app.request_layout();
    }
}

static void ui_view_set_text(ui_view_t* v, const char* format, ...) {
    va_list va;
    va_start(va, format);
    ui_view.set_text_va(v, format, va);
    va_end(va);
}

static void ui_view_show_hint(ui_view_t* v, ui_view_t* hint) {
    ui_view_call_init(hint);
    ui_view.set_text(hint, v->hint);
    ui_view.measure(hint);
    int32_t x = v->x + v->w / 2 - hint->w / 2 + hint->fm->em.w / 4;
    int32_t y = v->y + v->h + hint->fm->em.h / 4;
    if (x + hint->w > ui_app.crc.w) {
        x = ui_app.crc.w - hint->w - hint->fm->em.w / 2;
    }
    if (x < 0) { x = hint->fm->em.w / 2; }
    if (y + hint->h > ui_app.crc.h) {
        y = ui_app.crc.h - hint->h - hint->fm->em.h / 2;
    }
    if (y < 0) { y = hint->fm->em.h / 2; }
    // show_tooltip will center horizontally
    ui_app.show_hint(hint, x + hint->w / 2, y, 0);
}

static void ui_view_hovering(ui_view_t* v, bool start) {
    static ui_label_t hint = ui_label(0.0, "");
    if (start && ui_app.animating.view == null && v->hint[0] != 0 &&
       !ui_view.is_hidden(v)) {
        hint.padding = (ui_margins_t){0, 0, 0, 0};
        ui_view_show_hint(v, &hint);
    } else if (!start && ui_app.animating.view == &hint) {
        ui_app.show_hint(null, -1, -1, 0);
    }
}

static bool ui_view_is_shortcut_key(ui_view_t* v, int64_t key) {
    // Supported keyboard shortcuts are ASCII characters only for now
    // If there is not focused UI control in Alt+key [Alt] is optional.
    // If there is focused control only Alt+Key is accepted as shortcut
    char ch = 0x20 <= key && key <= 0x7F ? (char)toupper((char)key) : 0x00;
    bool needs_alt = ui_app.focus != null && ui_app.focus != v &&
         !ui_view.is_parent_of(ui_app.focus, v);
    bool keyboard_shortcut = ch != 0x00 && v->shortcut != 0x00 &&
         (ui_app.alt || ui_app.ctrl || !needs_alt) && toupper(v->shortcut) == ch;
    return keyboard_shortcut;
}

static bool ui_view_is_orphan(const ui_view_t* v) {
    while (v != ui_app.root && v != null) { v = v->parent; }
    return v == null;
}

static bool ui_view_is_hidden(const ui_view_t* v) {
    bool hidden = v->state.hidden || ui_view.is_orphan(v);
    while (!hidden && v->parent != null) {
        v = v->parent;
        hidden = v->state.hidden;
    }
    return hidden;
}

static bool ui_view_is_disabled(const ui_view_t* v) {
    bool disabled = v->state.disabled;
    while (!disabled && v->parent != null) {
        v = v->parent;
        disabled = v->state.disabled;
    }
    return disabled;
}

static void ui_view_timer(ui_view_t* v, ui_timer_t id) {
    if (v->timer != null) { v->timer(v, id); }
    // timers are delivered even to hidden and disabled views:
    ui_view_for_each(v, c, { ui_view_timer(c, id); });
}

static void ui_view_every_sec(ui_view_t* v) {
    if (v->every_sec != null) { v->every_sec(v); }
    ui_view_for_each(v, c, { ui_view_every_sec(c); });
}

static void ui_view_every_100ms(ui_view_t* v) {
    if (v->every_100ms != null) { v->every_100ms(v); }
    ui_view_for_each(v, c, { ui_view_every_100ms(c); });
}

static bool ui_view_key_pressed(ui_view_t* v, int64_t k) {
    bool done = false;
    if (!ui_view.is_hidden(v) && !ui_view.is_disabled(v)) {
        if (v->key_pressed != null) {
            ui_view_update_shortcut(v);
            done = v->key_pressed(v, k);
        }
        if (!done) {
            ui_view_for_each(v, c, {
                done = ui_view_key_pressed(c, k);
                if (done) { break; }
            });
        }
    }
    return done;
}

static bool ui_view_key_released(ui_view_t* v, int64_t k) {
    bool done = false;
    if (!ui_view.is_hidden(v) && !ui_view.is_disabled(v)) {
        if (v->key_released != null) {
            done = v->key_released(v, k);
        }
        if (!done) {
            ui_view_for_each(v, c, {
                done = ui_view_key_released(c, k);
                if (done) { break; }
            });
        }
    }
    return done;
}

static void ui_view_character(ui_view_t* v, const char* utf8) {
    if (!ui_view.is_hidden(v) && !ui_view.is_disabled(v)) {
        if (v->character != null) {
            ui_view_update_shortcut(v);
            v->character(v, utf8);
        }
        ui_view_for_each(v, c, { ui_view_character(c, utf8); });
    }
}

static void ui_view_resolve_color_ids(ui_view_t* v) {
    if (v->color_id > 0) {
        v->color = ui_colors.get_color(v->color_id);
    }
    if (v->background_id > 0) {
        v->background = ui_colors.get_color(v->background_id);
    }
}

static void ui_view_paint(ui_view_t* v) {
    assert(ui_app.crc.w > 0 && ui_app.crc.h > 0);
    ui_view_resolve_color_ids(v);
    if (v->debug.trace.prc) {
        const char* s = ui_view.string(v);
        ut_traceln("%d,%d %dx%d prc: %d,%d %dx%d \"%.*s\"", v->x, v->y, v->w, v->h,
                ui_app.prc.x, ui_app.prc.y, ui_app.prc.w, ui_app.prc.h,
                ut_min(64, strlen(s)), s);
    }
    if (!v->state.hidden && ui_app.crc.w > 0 && ui_app.crc.h > 0) {
        if (v->paint != null) { v->paint(v); }
        if (v->painted != null) { v->painted(v); }
        if (v->debug.paint.margins) { ui_view.debug_paint_margins(v); }
        if (v->debug.paint.fm)   { ui_view.debug_paint_fm(v); }
        if (v->debug.paint.call && v->debug_paint != null) { v->debug_paint(v); }
        ui_view_for_each(v, c, { ui_view_paint(c); });
    }
}

static bool ui_view_has_focus(const ui_view_t* v) {
    return ui_app.focused() && ui_app.focus == v;
}

static void ui_view_set_focus(ui_view_t* v) {
    if (ui_app.focus != v) {
        ui_view_t* loosing = ui_app.focus;
        ui_view_t* gaining = v;
        if (gaining != null) {
            swear(gaining->focusable && !ui_view.is_hidden(gaining) &&
                                        !ui_view.is_disabled(gaining));
        }
        if (loosing != null) { swear(loosing->focusable); }
        ui_app.focus = v;
        if (loosing != null && loosing->focus_lost != null) {
            loosing->focus_lost(loosing);
        }
        if (gaining != null && gaining->focus_gained != null) {
            gaining->focus_gained(gaining);
        }
    }
}

static int64_t ui_view_hit_test(const ui_view_t* v, ui_point_t pt) {
    int64_t ht = ui.hit_test.nowhere;
    if (!ui_view.is_hidden(v) && v->hit_test != null) {
         ht = v->hit_test(v, pt);
    }
    if (ht == ui.hit_test.nowhere) {
        ui_view_for_each(v, c, {
            if (!c->state.hidden && ui_view.inside(c, &pt)) {
                ht = ui_view_hit_test(c, pt);
                if (ht != ui.hit_test.nowhere) { break; }
            }
        });
    }
    return ht;
}

static void ui_view_update_hover(ui_view_t* v, bool hidden) {
    const bool hover  = v->state.hover;
    const bool inside = ui_view.inside(v, &ui_app.mouse);
    v->state.hover = !ui_view.is_hidden(v) && inside;
    if (hover != v->state.hover) {
//      ut_traceln("hover := %d %p %s", v->state.hover, v, ui_view_debug_id(v));
        ui_view.hover_changed(v); // even for hidden
        if (!hidden) { ui_view.invalidate(v, null); }
    }
}

static void ui_view_mouse_hover(ui_view_t* v) {
//  ut_traceln("%d,%d %s", ui_app.mouse.x, ui_app.mouse.y,
//          ui_app.mouse_left  ? "L" : "_",
//          ui_app.mouse_right ? "R" : "_");
    // mouse hover over is dispatched even to disabled views
    const bool hidden = ui_view.is_hidden(v);
    ui_view_update_hover(v, hidden);
    if (!hidden && v->mouse_hover != null) { v->mouse_hover(v); }
    ui_view_for_each(v, c, { ui_view_mouse_hover(c); });
}

static void ui_view_mouse_move(ui_view_t* v) {
//  ut_traceln("%d,%d %s", ui_app.mouse.x, ui_app.mouse.y,
//          ui_app.mouse_left  ? "L" : "_",
//          ui_app.mouse_right ? "R" : "_");
    // mouse move is dispatched even to disabled views
    const bool hidden = ui_view.is_hidden(v);
    ui_view_update_hover(v, hidden);
    if (!hidden && v->mouse_move != null) { v->mouse_move(v); }
    ui_view_for_each(v, c, { ui_view_mouse_move(c); });
}

static void ui_view_double_click(ui_view_t* v, int32_t ix) {
    if (!ui_view.is_hidden(v) && !ui_view.is_disabled(v)) {
        const bool inside = ui_view.inside(v, &ui_app.mouse);
        if (inside) {
            if (v->focusable) { ui_view.set_focus(v); }
            if (v->double_click != null) { v->double_click(v, ix); }
        }
        ui_view_for_each(v, c, { ui_view_double_click(c, ix); });
    }
}

static void ui_view_mouse_click(ui_view_t* v, int32_t ix, bool pressed) {
    if (!ui_view.is_hidden(v) && !ui_view.is_disabled(v)) {
        ui_view_for_each(v, c, { ui_view_mouse_click(c, ix, pressed); });
        const bool inside = ui_view.inside(v, &ui_app.mouse);
        if (inside) {
            if (v->focusable) { ui_view.set_focus(v); }
            if (v->mouse_click != null) { v->mouse_click(v, ix, pressed); }
        }
    }
}

static void ui_view_mouse_scroll(ui_view_t* v, ui_point_t dx_dy) {
    if (!ui_view.is_hidden(v) && !ui_view.is_disabled(v)) {
        if (v->mouse_scroll != null) { v->mouse_scroll(v, dx_dy); }
        ui_view_for_each(v, c, { ui_view_mouse_scroll(c, dx_dy); });
    }
}

static void ui_view_hover_changed(ui_view_t* v) {
    if (!v->state.hidden) {
        if (!v->state.hover) {
            v->p.hover_when = 0;
            ui_view.hovering(v, false); // cancel hover
        } else {
            swear(ui_view_hover_delay >= 0);
            if (v->p.hover_when >= 0) {
                v->p.hover_when = ui_app.now + ui_view_hover_delay;
            }
        }
    }
}

static void ui_view_lose_hidden_focus(ui_view_t* v) {
    // removes focus from hidden or disabled ui controls
    if (ui_app.focus != null) {
        if (ui_app.focus == v && (v->state.disabled || v->state.hidden)) {
            ui_view.set_focus(null);
        } else {
            ui_view_for_each(v, c, {
                if (ui_app.focus != null) { ui_view_lose_hidden_focus(c); }
            });
        }
    }
}

static bool ui_view_tap(ui_view_t* v, int32_t ix, bool pressed) {
    bool swallow = false; // consumed
    if (!ui_view.is_hidden(v) && !ui_view.is_disabled(v)) {
        ui_view_for_each(v, c, {
            swallow = ui_view_tap(c, ix, pressed);
            if (swallow) { break; }
        });
        const bool inside = ui_view.inside(v, &ui_app.mouse);
        if (!swallow && inside) {
            if (v->focusable) { ui_view.set_focus(v); }
            if (v->tap != null) { swallow = v->tap(v, ix, pressed); }
        }
    }
    return swallow;
}

static bool ui_view_long_press(ui_view_t* v, int32_t ix) {
    bool swallow = false; // consumed
    if (!ui_view.is_hidden(v) && !ui_view.is_disabled(v)) {
        ui_view_for_each(v, c, {
            swallow = ui_view_long_press(c, ix);
            if (swallow) { break; }
        });
        const bool inside = ui_view.inside(v, &ui_app.mouse);
        if (!swallow && inside && v->long_press != null) {
            swallow = v->long_press(v, ix);
        }
    }
    return swallow;
}

static bool ui_view_double_tap(ui_view_t* v, int32_t ix) { // 0: left 1: middle 2: right
    bool swallow = false; // consumed
    if (!ui_view.is_hidden(v) && !ui_view.is_disabled(v)) {
        ui_view_for_each(v, c, {
            swallow = ui_view_double_tap(c, ix);
            if (swallow) { break; }
        });
        const bool inside = ui_view.inside(v, &ui_app.mouse);
        if (!swallow && inside && v->double_tap != null) {
            swallow = v->double_tap(v, ix);
        }
    }
    return swallow;
}

static bool ui_view_context_menu(ui_view_t* v) {
    bool swallow = false;
    if (!ui_view.is_hidden(v) && !ui_view.is_disabled(v)) {
        ui_view_for_each(v, c, {
            swallow = ui_view_context_menu(c);
            if (swallow) { break; }
        });
        const bool inside = ui_view.inside(v, &ui_app.mouse);
        if (!swallow && inside && v->context_menu != null) {
            swallow = v->context_menu(v);
        }
    }
    return swallow;
}

static bool ui_view_message(ui_view_t* view, int32_t m, int64_t wp, int64_t lp,
        int64_t* ret) {
    if (!view->state.hidden) {
        if (view->p.hover_when > 0 && ui_app.now > view->p.hover_when) {
            view->p.hover_when = -1; // "already called"
            ui_view.hovering(view, true);
        }
    }
    // message() callback is called even for hidden and disabled views
    // could be useful for enabling conditions of post() messages from
    // background ut_thread.
    if (view->message != null) {
        if (view->message(view, m, wp, lp, ret)) { return true; }
    }
    ui_view_for_each(view, c, {
        if (ui_view_message(c, m, wp, lp, ret)) { return true; }
    });
    return false;
}

static bool ui_view_is_container(const ui_view_t* v) {
    return  v->type == ui_view_stack ||
            v->type == ui_view_span  ||
            v->type == ui_view_list;
}

static bool ui_view_is_spacer(const ui_view_t* v) {
    return  v->type == ui_view_spacer;
}

static bool ui_view_is_control(const ui_view_t* v) {
    return  v->type == ui_view_text   ||
            v->type == ui_view_label  ||
            v->type == ui_view_toggle ||
            v->type == ui_view_button ||
            v->type == ui_view_slider ||
            v->type == ui_view_mbx;
}

static void ui_view_debug_paint_margins(ui_view_t* v) {
    if (v->debug.paint.margins) {
        if (v->type == ui_view_spacer) {
            ui_gdi.fill(v->x, v->y, v->w, v->h, ui_color_rgb(128, 128, 128));
        }
        const ui_ltrb_t p = ui_view.margins(v, &v->padding);
        const ui_ltrb_t i = ui_view.margins(v, &v->insets);
        ui_color_t c = ui_colors.green;
        const int32_t pl = p.left;
        const int32_t pr = p.right;
        const int32_t pt = p.top;
        const int32_t pb = p.bottom;
        if (pl > 0) { ui_gdi.frame(v->x - pl, v->y, pl, v->h, c); }
        if (pr > 0) { ui_gdi.frame(v->x + v->w, v->y, pr, v->h, c); }
        if (pt > 0) { ui_gdi.frame(v->x, v->y - pt, v->w, pt, c); }
        if (p.bottom > 0) {
            ui_gdi.frame(v->x, v->y + v->h, v->w, pb, c);
        }
        c = ui_colors.orange;
        const int32_t il = i.left;
        const int32_t ir = i.right;
        const int32_t it = i.top;
        const int32_t ib = i.bottom;
        if (il > 0) { ui_gdi.frame(v->x, v->y, il, v->h, c); }
        if (ir > 0) { ui_gdi.frame(v->x + v->w - ir, v->y, ir, v->h, c); }
        if (it > 0) { ui_gdi.frame(v->x, v->y, v->w, it, c); }
        if (ib > 0) { ui_gdi.frame(v->x, v->y + v->h - ib, v->w, ib, c); }
        if ((ui_view.is_container(v) || ui_view.is_spacer(v)) &&
            v->w > 0 && v->h > 0 && v->color != ui_colors.transparent) {
            ui_wh_t mt = ui_view_text_metrics(v->x, v->y, false, 0,
                                              v->fm, "%s", ui_view.string(v));
            const int32_t tx = v->x + (v->w - mt.w) / 2;
            const int32_t ty = v->y + (v->h - mt.h) / 2;
            c = ui_color_is_rgb(v->color) ^ 0xFFFFFF;
            const ui_gdi_ta_t ta = { .fm = v->fm, .color = c };
            ui_gdi.text(&ta, tx, ty, "%s", ui_view.string(v));
        }
    }
}

static void ui_view_debug_paint_fm(ui_view_t* v) {
    if (v->debug.paint.fm && v->p.text[0] != 0 &&
       !ui_view_is_container(v) && !ui_view_is_spacer(v)) {
        const ui_point_t t = v->text.xy;
        const int32_t x = v->x;
        const int32_t y = v->y;
        const int32_t w = v->w;
        const int32_t y_0 = y + t.y;
        const int32_t y_b = y_0 + v->fm->baseline;
        const int32_t y_a = y_b - v->fm->ascent;
        const int32_t y_h = y_0 + v->fm->height;
        const int32_t y_x = y_b - v->fm->x_height;
        const int32_t y_d = y_b + v->fm->descent;
        // fm.height y == 0 line is painted one pixel higher:
        ui_gdi.line(x, y_0 - 1, x + w, y_0 - 1, ui_colors.red);
        ui_gdi.line(x, y_a, x + w, y_a, ui_colors.green);
        ui_gdi.line(x, y_x, x + w, y_x, ui_colors.orange);
        ui_gdi.line(x, y_b, x + w, y_b, ui_colors.red);
        ui_gdi.line(x, y_d, x + w, y_d, ui_colors.green);
        if (y_h != y_d) {
            ui_gdi.line(x, y_d, x + w, y_d, ui_colors.green);
            ui_gdi.line(x, y_h, x + w, y_h, ui_colors.red);
        } else {
            ui_gdi.line(x, y_h, x + w, y_h, ui_colors.orange);
        }
        // fm.height line painted _under_ the actual height
    }
}

#pragma push_macro("ui_view_no_siblings")

#define ui_view_no_siblings(v) do {                    \
    swear((v)->parent == null && (v)->child == null && \
          (v)->prev == null && (v)->next == null);     \
} while (0)

static void ui_view_test(void) {
    ui_view_t p0 = ui_view(stack);
    ui_view_t c1 = ui_view(stack);
    ui_view_t c2 = ui_view(stack);
    ui_view_t c3 = ui_view(stack);
    ui_view_t c4 = ui_view(stack);
    ui_view_t g1 = ui_view(stack);
    ui_view_t g2 = ui_view(stack);
    ui_view_t g3 = ui_view(stack);
    ui_view_t g4 = ui_view(stack);
    // add grand children to children:
    ui_view.add(&c2, &g1, &g2, null);               ui_view_verify(&c2);
    ui_view.add(&c3, &g3, &g4, null);               ui_view_verify(&c3);
    // single child
    ui_view.add(&p0, &c1, null);                    ui_view_verify(&p0);
    ui_view.remove(&c1);                            ui_view_verify(&p0);
    // two children
    ui_view.add(&p0, &c1, &c2, null);               ui_view_verify(&p0);
    ui_view.remove(&c1);                            ui_view_verify(&p0);
    ui_view.remove(&c2);                            ui_view_verify(&p0);
    // three children
    ui_view.add(&p0, &c1, &c2, &c3, null);          ui_view_verify(&p0);
    ui_view.remove(&c1);                            ui_view_verify(&p0);
    ui_view.remove(&c2);                            ui_view_verify(&p0);
    ui_view.remove(&c3);                            ui_view_verify(&p0);
    // add_first, add_last, add_before, add_after
    ui_view.add_first(&p0, &c1);                    ui_view_verify(&p0);
    swear(p0.child == &c1);
    ui_view.add_last(&p0, &c4);                     ui_view_verify(&p0);
    swear(p0.child == &c1 && p0.child->prev == &c4);
    ui_view.add_after(&c2, &c1);                    ui_view_verify(&p0);
    swear(p0.child == &c1);
    swear(c1.next == &c2);
    ui_view.add_before(&c3, &c4);                   ui_view_verify(&p0);
    swear(p0.child == &c1);
    swear(c4.prev == &c3);
    // removing all
    ui_view.remove(&c1);                            ui_view_verify(&p0);
    ui_view.remove(&c2);                            ui_view_verify(&p0);
    ui_view.remove(&c3);                            ui_view_verify(&p0);
    ui_view.remove(&c4);                            ui_view_verify(&p0);
    ui_view_no_siblings(&p0);
    ui_view_no_siblings(&c1);
    ui_view_no_siblings(&c4);
    ui_view.remove(&g1);                            ui_view_verify(&c2);
    ui_view.remove(&g2);                            ui_view_verify(&c2);
    ui_view.remove(&g3);                            ui_view_verify(&c3);
    ui_view.remove(&g4);                            ui_view_verify(&c3);
    ui_view_no_siblings(&c2); ui_view_no_siblings(&c3);
    ui_view_no_siblings(&g1); ui_view_no_siblings(&g2);
    ui_view_no_siblings(&g3); ui_view_no_siblings(&g4);
    // a bit more intuitive (for a human) nested way to initialize tree:
    ui_view.add(&p0,
        &c1,
        ui_view.add(&c2, &g1, &g2, null),
        ui_view.add(&c3, &g3, &g4, null),
        &c4);
    ui_view_verify(&p0);
    ui_view_disband(&p0);
    ui_view_no_siblings(&p0);
    ui_view_no_siblings(&c1); ui_view_no_siblings(&c2);
    ui_view_no_siblings(&c3); ui_view_no_siblings(&c4);
    ui_view_no_siblings(&g1); ui_view_no_siblings(&g2);
    ui_view_no_siblings(&g3); ui_view_no_siblings(&g4);
    if (ut_debug.verbosity.level > ut_debug.verbosity.quiet) { ut_traceln("done"); }
}

#pragma pop_macro("ui_view_no_siblings")

ui_view_if ui_view = {
    .add                 = ui_view_add,
    .add_first           = ui_view_add_first,
    .add_last            = ui_view_add_last,
    .add_after           = ui_view_add_after,
    .add_before          = ui_view_add_before,
    .remove              = ui_view_remove,
    .remove_all          = ui_view_remove_all,
    .disband             = ui_view_disband,
    .inside              = ui_view_inside,
    .is_parent_of        = ui_view_is_parent_of,
    .margins             = ui_view_margins,
    .inbox               = ui_view_inbox,
    .outbox              = ui_view_outbox,
    .set_text            = ui_view_set_text,
    .set_text_va         = ui_view_set_text_va,
    .invalidate          = ui_view_invalidate,
    .text_metrics_va     = ui_view_text_metrics_va,
    .text_metrics        = ui_view_text_metrics,
    .text_measure        = ui_view_text_measure,
    .text_align          = ui_view_text_align,
    .measure_control     = ui_view_measure_control,
    .measure_children    = ui_view_measure_children,
    .layout_children     = ui_view_layout_children,
    .measure             = ui_view_measure,
    .layout              = ui_view_layout,
    .string              = ui_view_string,
    .is_orphan           = ui_view_is_orphan,
    .is_hidden           = ui_view_is_hidden,
    .is_disabled         = ui_view_is_disabled,
    .is_control          = ui_view_is_control,
    .is_container        = ui_view_is_container,
    .is_spacer           = ui_view_is_spacer,
    .timer               = ui_view_timer,
    .every_sec           = ui_view_every_sec,
    .every_100ms         = ui_view_every_100ms,
    .hit_test            = ui_view_hit_test,
    .key_pressed         = ui_view_key_pressed,
    .key_released        = ui_view_key_released,
    .character           = ui_view_character,
    .paint               = ui_view_paint,
    .has_focus           = ui_view_has_focus,
    .set_focus           = ui_view_set_focus,
    .lose_hidden_focus   = ui_view_lose_hidden_focus,
    .mouse_hover         = ui_view_mouse_hover,
    .mouse_move          = ui_view_mouse_move,
    .mouse_click         = ui_view_mouse_click,
    .double_click        = ui_view_double_click,
    .mouse_scroll        = ui_view_mouse_scroll,
    .hovering            = ui_view_hovering,
    .hover_changed       = ui_view_hover_changed,
    .is_shortcut_key     = ui_view_is_shortcut_key,
    .context_menu        = ui_view_context_menu,
    .tap                 = ui_view_tap,
    .long_press          = ui_view_long_press,
    .double_tap          = ui_view_double_tap,
    .message             = ui_view_message,
    .debug_paint_margins = ui_view_debug_paint_margins,
    .debug_paint_fm      = ui_view_debug_paint_fm,
    .test                = ui_view_test
};

#ifdef UI_VIEW_TEST

ut_static_init(ui_view) {
    ui_view.test();
}

#endif

#pragma pop_macro("ui_view_for_each")
