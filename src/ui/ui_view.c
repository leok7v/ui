#include "ut/ut.h"
#include "ui/ui.h"

static const fp64_t ui_view_hover_delay = 1.5; // seconds

#pragma push_macro("ui_view_for_each")

// adding and removing views is not expected to be frequent
// actions by application code (human factor - UI design)
// thus extra checks and verifications are there even in
// release code because C is not type safety champion language.

static inline void ui_view_check_type(ui_view_t* v) {
    // little endian:
    static_assertion(('vwXX' & 0xFFFF0000U) == ('vwZZ' & 0xFFFF0000U));
    static_assertion((ui_view_container & 0xFFFF0000U) == ('vwXX' & 0xFFFF0000U));
    swear(((uint32_t)v->type & 0xFFFF0000U) == ('vwXX'  & 0xFFFF0000U),
          "not a view: %4.4s 0x%08X (forgotten &static_view?)",
          &v->type, v->type);
}

static void ui_view_verify(ui_view_t* p) {
    ui_view_check_type(p);
    ui_view_for_each(p, c, {
        ui_view_check_type(c);
        swear(c->parent == p);
        swear(c == c->next->prev);
        swear(c == c->prev->next);
    });
}

static ui_view_t* ui_view_add(ui_view_t* p, ...) {
    va_list vl;
    va_start(vl, p);
    ui_view_t* c = va_arg(vl, ui_view_t*);
    while (c != null) {
        swear(c->parent == null && c->prev == null && c->next == null);
        ui_view.add_last(p, c);
        c = va_arg(vl, ui_view_t*);
    }
    va_end(vl);
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
    not_null(a->parent);
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
    not_null(b->parent);
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
    not_null(c->parent);
    not_null(c->parent->child);
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

static void ui_view_invalidate(const ui_view_t* v) {
    ui_rect_t rc = { v->x, v->y, v->w, v->h};
    rc.x -= v->fm->em.w;
    rc.y -= v->fm->em.h;
    rc.w += v->fm->em.w * 2;
    rc.h += v->fm->em.h * 2;
    ui_app.invalidate(&rc);
}

static const char* ui_view_nls(ui_view_t* v) {
    return v->strid != 0 ? ut_nls.string(v->strid, v->text) : v->text;
}

static void ui_measure_view(ui_view_t* v) {
    ui_ltrb_t i = ui_view.gaps(v, &v->insets);
//  ui_ltrb_t p = ui_view.gaps(v, &v->padding);
//  traceln(">%s %d,%d %dx%d p: %d %d %d %d  i: %d %d %d %d",
//               v->text, v->x, v->y, v->w, v->h,
//               p.left, p.top, p.right, p.bottom,
//               i.left, i.top, i.right, i.bottom);
    if (v->text[0] != 0) {
        v->w = (int32_t)((fp64_t)v->fm->em.w * (fp64_t)v->min_w_em + 0.5);
        ui_wh_t mt = { 0 };
        bool multiline = strchr(v->text, '\n') != null;
        if (v->type == ui_view_label && multiline) {
            int32_t w = (int32_t)((fp64_t)v->min_w_em * (fp64_t)v->fm->em.w + 0.5);
            mt = ui_gdi.measure_multiline(v->fm->font, w == 0 ? -1 : w,
                                          ui_view.nls(v));
        } else {
            mt = ui_gdi.measure_text(v->fm->font, ui_view.nls(v));
        }
//      traceln(" mt: %dx%d", mt.x, mt.y);
        v->w = i.left + ut_max(v->w, mt.w) + i.right;
        v->h = i.top + mt.h + i.bottom;
    } else {
        // TODO: minimum view 1x1 em?
        v->w = i.left + v->fm->em.w + i.right;
        v->h = i.top + v->fm->em.h + i.bottom;
        v->w = ut_min(v->w, ui.gaps_em2px(v->fm->em.w, v->min_w_em));
        v->h = ut_min(v->h, ui.gaps_em2px(v->fm->em.h, v->min_h_em));
    }
//  traceln("<%s %d,%d %dx%d", v->text, v->x, v->y, v->w, v->h);
}

static void ui_view_measure(ui_view_t* v) {
    if (!ui_view.is_hidden(v)) {
        ui_view_for_each(v, c, { ui_view.measure(c); });
        if (v->prepare != null) { v->prepare(v); }
        if (v->measure != null && v->measure != ui_view_measure) {
            v->measure(v);
        } else {
            ui_measure_view(v);
        }
        if (v->measured != null) { v->measured(v); }
    }
}

static void ui_layout_view(ui_view_t* unused(v)) {
//  ui_ltrb_t i = ui_view.gaps(v, &v->insets);
//  ui_ltrb_t p = ui_view.gaps(v, &v->padding);
//  traceln(">%s %d,%d %dx%d p: %d %d %d %d  i: %d %d %d %d",
//               v->text, v->x, v->y, v->w, v->h,
//               p.left, p.top, p.right, p.bottom,
//               i.left, i.top, i.right, i.bottom);
//  traceln("<%s %d,%d %dx%d", v->text, v->x, v->y, v->w, v->h);
}

static void ui_view_layout(ui_view_t* v) {
//  traceln(">%s %d,%d %dx%d", v->text, v->x, v->y, v->w, v->h);
    if (!ui_view.is_hidden(v)) {
        if (v->layout != null && v->layout != ui_view_layout) {
            v->layout(v);
        } else {
            ui_layout_view(v);
        }
        if (v->composed != null) { v->composed(v); }
        ui_view_for_each(v, c, { ui_view.layout(c); });
    }
//  traceln("<%s %d,%d %dx%d", v->text, v->x, v->y, v->w, v->h);
}

static bool ui_view_inside(ui_view_t* v, const ui_point_t* pt) {
    const int32_t x = pt->x - v->x;
    const int32_t y = pt->y - v->y;
    return 0 <= x && x < v->w && 0 <= y && y < v->h;
}

static ui_ltrb_t ui_view_gaps(ui_view_t* v, const ui_gaps_t* g) {
    return (ui_ltrb_t) {
        .left   = ui.gaps_em2px(v->fm->em.w, g->left),
        .top    = ui.gaps_em2px(v->fm->em.h, g->top),
        .right  = ui.gaps_em2px(v->fm->em.w, g->right),
        .bottom = ui.gaps_em2px(v->fm->em.h, g->bottom)
    };
}

static void ui_view_inbox(ui_view_t* v, ui_rect_t* r, ui_ltrb_t* insets) {
    swear(r != null || insets != null);
    swear(v->max_w >= 0 && v->max_h >= 0);
    const ui_ltrb_t i = ui_view_gaps(v, &v->insets);
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

static void ui_view_outbox(ui_view_t* v, ui_rect_t* r, ui_ltrb_t* padding) {
    swear(r != null || padding != null);
    swear(v->max_w >= 0 && v->max_h >= 0);
    const ui_ltrb_t p = ui_view_gaps(v, &v->padding);
    if (padding != null) { *padding = p; }
    if (r != null) {
//      traceln("%s %d,%d %dx%d %.1f %.1f %.1f %.1f", v->text,
//          v->x, v->y, v->w, v->h,
//          v->padding.left, v->padding.top, v->padding.right, v->padding.bottom);
        *r = (ui_rect_t) {
            .x = v->x - p.left,
            .y = v->y - p.top,
            .w = v->w + p.left + p.right,
            .h = v->h + p.top  + p.bottom,
        };
//      traceln("%s %d,%d %dx%d", v->text,
//          r->x, r->y, r->w, r->h);
    }
}

static void ui_view_position_by_outbox(ui_view_t* v, const ui_rect_t* r,
            const ui_ltrb_t* padding) {
    v->x = r->x + padding->left;
    v->y = r->y + padding->top;
    v->w = r->w - padding->left - padding->right;
    v->h = r->h - padding->top  - padding->bottom;
}

static void ui_view_set_text(ui_view_t* v, const char* text) {
    int32_t n = (int32_t)strlen(text);
    ut_str_printf(v->text, "%s", text);
    v->strid = 0; // next call to nls() will localize this text
    for (int32_t i = 0; i < n; i++) {
        if (text[i] == '&' && i < n - 1 && text[i + 1] != '&') {
            v->shortcut = text[i + 1];
            break;
        }
    }
}

static void ui_view_localize(ui_view_t* v) {
    if (v->text[0] != 0) {
        v->strid = ut_nls.strid(v->text);
    }
}

static void ui_view_show_hint(ui_view_t* v, ui_view_t* hint) {
    ui_view_call_init(hint);
    ut_str_printf(hint->text, "%s", ut_nls.str(v->hint));
    ui_view.measure(hint);
    int32_t x = v->x + v->w / 2 - hint->w / 2 + hint->fm->em.w / 4;
    int32_t y = v->y + v->h + v->fm->em.h / 2 + hint->fm->em.h / 4;
    if (x + hint->w > ui_app.crc.w) { x = ui_app.crc.w - hint->w - hint->fm->em.w / 2; }
    if (x < 0) { x = hint->fm->em.w / 2; }
    if (y + hint->h > ui_app.crc.h) { y = ui_app.crc.h - hint->h - hint->fm->em.h / 2; }
    if (y < 0) { y = hint->fm->em.h / 2; }
    // show_tooltip will center horizontally
    ui_app.show_tooltip(hint, x + hint->w / 2, y, 0);
}

static void ui_view_hovering(ui_view_t* v, bool start) {
    static ui_label_t hint = ui_label(0.0, "");
    if (start && ui_app.animating.view == null && v->hint[0] != 0 &&
       !ui_view.is_hidden(v)) {
        ui_view_show_hint(v, &hint);
    } else if (!start && ui_app.animating.view == &hint) {
        ui_app.show_tooltip(null, -1, -1, 0);
    }
}

static bool ui_view_is_shortcut_key(ui_view_t* v, int64_t key) {
    // Supported keyboard shortcuts are ASCII characters only for now
    // If there is not focused UI control in Alt+key [Alt] is optional.
    // If there is focused control only Alt+Key is accepted as shortcut
    char ch = 0x20 <= key && key <= 0x7F ? (char)toupper((char)key) : 0x00;
    bool need_alt = ui_app.focus != null && ui_app.focus != v;
    bool keyboard_shortcut = ch != 0x00 && v->shortcut != 0x00 &&
         (ui_app.alt || !need_alt) && toupper(v->shortcut) == ch;
    return keyboard_shortcut;
}

static bool ui_view_is_hidden(const ui_view_t* v) {
    bool hidden = v->hidden;
    while (!hidden && v->parent != null) {
        v = v->parent;
        hidden = v->hidden;
    }
    return hidden;
}

static bool ui_view_is_disabled(const ui_view_t* v) {
    bool disabled = v->disabled;
    while (!disabled && v->parent != null) {
        v = v->parent;
        disabled = v->disabled;
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

static void ui_view_key_pressed(ui_view_t* v, int64_t k) {
    if (!ui_view.is_hidden(v) && !ui_view.is_disabled(v)) {
        if (v->key_pressed != null) { v->key_pressed(v, k); }
        ui_view_for_each(v, c, { ui_view_key_pressed(c, k); });
    }
}

static void ui_view_key_released(ui_view_t* v, int64_t k) {
    if (!ui_view.is_hidden(v) && !ui_view.is_disabled(v)) {
        if (v->key_released != null) { v->key_released(v, k); }
        ui_view_for_each(v, c, { ui_view_key_released(c, k); });
    }
}

static void ui_view_character(ui_view_t* v, const char* utf8) {
    if (!ui_view.is_hidden(v) && !ui_view.is_disabled(v)) {
        if (v->character != null) { v->character(v, utf8); }
        ui_view_for_each(v, c, { ui_view_character(c, utf8); });
    }
}

static void ui_view_resolve_color_ids(ui_view_t* v) {
    if (v->color_id > 0) {
        v->color = ui_app.get_color(v->color_id);
    }
    if (v->background_id > 0) {
        v->background = ui_app.get_color(v->background_id);
    }
}

static void ui_view_paint(ui_view_t* v) {
    assert(ui_app.crc.w > 0 && ui_app.crc.h > 0);
    ui_view_resolve_color_ids(v);
    if (!v->hidden && ui_app.crc.w > 0 && ui_app.crc.h > 0) {
        if (v->paint != null) { v->paint(v); }
        if (v->painted != null) { v->painted(v); }
        if (v->debug_paint != null && v->debug) { v->debug_paint(v); }
        if (v->debug) { ui_view.debug_paint(v); }
        ui_view_for_each(v, c, { ui_view_paint(c); });
    }
}

static bool ui_view_set_focus(ui_view_t* v) {
    bool set = false;
   ui_view_for_each(v, c, {
        set = ui_view_set_focus(c);
        if (set) { break; }
    });
    if (!set && !ui_view.is_hidden(v) && !ui_view.is_disabled(v) &&
        v->focusable && v->set_focus != null &&
       (ui_app.focus == v || ui_app.focus == null)) {
        set = v->set_focus(v);
    }
    return set;
}

static void ui_view_kill_focus(ui_view_t* v) {
    ui_view_for_each(v, c, { ui_view_kill_focus(c); });
    if (v->kill_focus != null && v->focusable) {
        v->kill_focus(v);
    }
}

static int64_t ui_view_hit_test(ui_view_t* v, int32_t cx, int32_t cy) {
    int64_t ht = ui.hit_test.nowhere;
    if (!ui_view.is_hidden(v) && v->hit_test != null) {
         ht = v->hit_test(v, cx, cy);
    }
    if (ht == ui.hit_test.nowhere) {
        ui_view_for_each(v, c, {
            if (!c->hidden) {
                ht = ui_view_hit_test(c, cx, cy);
                if (ht != ui.hit_test.nowhere) { break; }
            }
        });
    }
    return ht;
}

static void ui_view_mouse(ui_view_t* v, int32_t m, int64_t f) {
    if (!ui_view.is_hidden(v) &&
       (m == ui.message.mouse_hover || m == ui.message.mouse_move)) {
        ui_rect_t r = { v->x, v->y, v->w, v->h};
        bool hover = v->hover;
        v->hover = ui.point_in_rect(&ui_app.mouse, &r);
        // inflate view rectangle:
        r.x -= v->w / 4;
        r.y -= v->h / 4;
        r.w += v->w / 2;
        r.h += v->h / 2;
        if (hover != v->hover) { ui_app.invalidate(&r); }
        if (hover != v->hover) {
            ui_view.hover_changed(v);
        }
    }
    if (!ui_view.is_hidden(v) && !ui_view.is_disabled(v)) {
        if (v->mouse != null) { v->mouse(v, m, f); }
        ui_view_for_each(v, c, { ui_view_mouse(c, m, f); });
    }
}

static void ui_view_mouse_wheel(ui_view_t* v, int32_t dx, int32_t dy) {
    if (!ui_view.is_hidden(v) && !ui_view.is_disabled(v)) {
        if (v->mouse_wheel != null) { v->mouse_wheel(v, dx, dy); }
        ui_view_for_each(v, c, { ui_view_mouse_wheel(c, dx, dy); });
    }
}

static void ui_view_hover_changed(ui_view_t* v) {
    if (!v->hidden) {
        if (!v->hover) {
            v->hover_when = 0;
            ui_view.hovering(v, false); // cancel hover
        } else {
            swear(ui_view_hover_delay >= 0);
            if (v->hover_when >= 0) {
                v->hover_when = ui_app.now + ui_view_hover_delay;
            }
        }
    }
}

static void ui_view_kill_hidden_focus(ui_view_t* v) {
    // removes focus from hidden or disabled ui controls
    if (ui_app.focus != null) {
        if (ui_app.focus == v && (v->disabled || v->hidden)) {
            ui_app.focus = null;
            // even for disabled or hidden view notify about kill_focus:
            v->kill_focus(v);
        } else {
            ui_view_for_each(v, c, { ui_view_kill_hidden_focus(c); });
        }
    }
}

static bool ui_view_tap(ui_view_t* v, int32_t ix) { // 0: left 1: middle 2: right
    bool done = false; // consumed
    if (!ui_view.is_hidden(v) && !ui_view.is_disabled(v) &&
        ui_view_inside(v, &ui_app.mouse)) {
        ui_view_for_each(v, c, {
            done = ui_view_tap(c, ix);
            if (done) { break; }
        });

        if (v->tap != null && !done) { done = v->tap(v, ix); }
    }
    return done;
}

static bool ui_view_press(ui_view_t* v, int32_t ix) { // 0: left 1: middle 2: right
    bool done = false; // consumed
    if (!ui_view.is_hidden(v) && !ui_view.is_disabled(v)) {
        ui_view_for_each(v, c, {
            done = ui_view_press(c, ix);
            if (done) { break; }
        });
        if (v->press != null && !done) { done = v->press(v, ix); }
    }
    return done;
}

static bool ui_view_context_menu(ui_view_t* v) {
    if (!ui_view.is_hidden(v) && !ui_view.is_disabled(v)) {
        ui_view_for_each(v, c, {
            if (ui_view_context_menu(c)) { return true; }
        });
        ui_rect_t r = { v->x, v->y, v->w, v->h};
        if (ui.point_in_rect(&ui_app.mouse, &r)) {
            if (!v->hidden && !v->disabled && v->context_menu != null) {
                v->context_menu(v);
            }
        }
    }
    return false;
}

static bool ui_view_message(ui_view_t* view, int32_t m, int64_t wp, int64_t lp,
        int64_t* ret) {
    if (!view->hidden) {
        if (view->hover_when > 0 && ui_app.now > view->hover_when) {
            view->hover_when = -1; // "already called"
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

static void ui_view_debug_paint(ui_view_t* v) {
    ui_gdi.push(v->x, v->y);
    bool container = v->type == ui_view_container ||
                     v->type == ui_view_span ||
                     v->type == ui_view_list ||
                     v->type == ui_view_spacer;
    if (v->type == ui_view_spacer) {
        ui_gdi.fill_with(v->x, v->y, v->w, v->h, ui_rgb(128, 128, 128));
    } else if (container && v->color != ui_colors.transparent) {
//      traceln("%s 0x%08X", v->text, v->color);
        ui_gdi.fill_with(v->x, v->y, v->w, v->h, v->background);
    }
    ui_ltrb_t p = ui_view.gaps(v, &v->padding);
    ui_ltrb_t i = ui_view.gaps(v, &v->insets);
    if (p.left   > 0) { ui_gdi.frame_with(v->x - p.left, v->y, p.left, v->h, ui_colors.green); }
    if (p.right  > 0) { ui_gdi.frame_with(v->x + v->w, v->y, p.right, v->h, ui_colors.green); }
    if (p.top    > 0) { ui_gdi.frame_with(v->x, v->y - p.top, v->w, p.top, ui_colors.green); }
    if (p.bottom > 0) { ui_gdi.frame_with(v->x, v->y + v->h, v->w, p.bottom, ui_colors.green); }
    if (i.left  > 0)  { ui_gdi.frame_with(v->x, v->y,               i.left, v->h, ui_colors.orange); }
    if (i.right > 0)  { ui_gdi.frame_with(v->x + v->w - i.right, v->y, i.right, v->h, ui_colors.orange); }
    if (i.top   > 0)  { ui_gdi.frame_with(v->x, v->y,            v->w, i.top, ui_colors.orange); }
    if (i.bottom > 0) { ui_gdi.frame_with(v->x, v->y + v->h - i.bottom, v->w, i.bottom, ui_colors.orange); }
    if (container && v->w > 0 && v->h > 0 && v->color != ui_colors.transparent) {
        ui_gdi.set_text_color(ui_color_rgb(v->color) ^ 0xFFFFFF);
        ui_wh_t mt = ui_gdi.measure_text(v->fm->font, v->text);
        ui_gdi.x += (v->w - mt.w) / 2;
        ui_gdi.y += (v->h - mt.h) / 2;
        ui_font_t f = ui_gdi.set_font(v->fm->font);
        ui_gdi.text("%s", v->text);
        ui_gdi.set_font(f);
    }
    ui_gdi.pop();
}

#pragma push_macro("ui_view_no_siblings")

#define ui_view_no_siblings(v) do {                    \
    swear((v)->parent == null && (v)->child == null && \
          (v)->prev == null && (v)->next == null);     \
} while (0)

static void ui_view_test(void) {
    ui_view_t p0 = ui_view(container);
    ui_view_t c1 = ui_view(container);
    ui_view_t c2 = ui_view(container);
    ui_view_t c3 = ui_view(container);
    ui_view_t c4 = ui_view(container);
    ui_view_t g1 = ui_view(container);
    ui_view_t g2 = ui_view(container);
    ui_view_t g3 = ui_view(container);
    ui_view_t g4 = ui_view(container);
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
    if (ut_debug.verbosity.level > ut_debug.verbosity.quiet) { traceln("done"); }
}

#pragma pop_macro("ui_view_no_siblings")

ui_view_if ui_view = {
    .add                = ui_view_add,
    .add_first          = ui_view_add_first,
    .add_last           = ui_view_add_last,
    .add_after          = ui_view_add_after,
    .add_before         = ui_view_add_before,
    .remove             = ui_view_remove,
    .remove_all         = ui_view_remove_all,
    .disband            = ui_view_disband,
    .inside             = ui_view_inside,
    .gaps               = ui_view_gaps,
    .inbox              = ui_view_inbox,
    .outbox             = ui_view_outbox,
    .position_by_outbox = ui_view_position_by_outbox,
    .set_text           = ui_view_set_text,
    .invalidate         = ui_view_invalidate,
    .measure            = ui_view_measure,
    .layout             = ui_view_layout,
    .nls                = ui_view_nls,
    .localize           = ui_view_localize,
    .is_hidden          = ui_view_is_hidden,
    .is_disabled        = ui_view_is_disabled,
    .timer              = ui_view_timer,
    .every_sec          = ui_view_every_sec,
    .every_100ms        = ui_view_every_100ms,
    .hit_test           = ui_view_hit_test,
    .key_pressed        = ui_view_key_pressed,
    .key_released       = ui_view_key_released,
    .character          = ui_view_character,
    .paint              = ui_view_paint,
    .set_focus          = ui_view_set_focus,
    .kill_focus         = ui_view_kill_focus,
    .kill_hidden_focus  = ui_view_kill_hidden_focus,
    .mouse              = ui_view_mouse,
    .mouse_wheel        = ui_view_mouse_wheel,
    .hovering           = ui_view_hovering,
    .hover_changed      = ui_view_hover_changed,
    .is_shortcut_key    = ui_view_is_shortcut_key,
    .context_menu       = ui_view_context_menu,
    .tap                = ui_view_tap,
    .press              = ui_view_press,
    .message            = ui_view_message,
    .debug_paint        = ui_view_debug_paint,
    .test               = ui_view_test
};

#ifdef UI_VIEW_TEST

ut_static_init(ui_view) {
    ui_view.test();
}

#endif

#pragma pop_macro("ui_view_for_each")
