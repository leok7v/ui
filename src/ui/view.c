#include "ut/ut.h"
#include "ui/ui.h"

static const fp64_t ui_view_hover_delay = 1.5; // seconds

#define ui_view_for_each(v, it, code) do { \
    ui_view_t* it = (v)->child;            \
    if (it != null) {                      \
        do {                               \
            { code }                       \
            it = it->next;                 \
        } while (it != (v)->child);        \
    }                                      \
} while (0)

static void ui_view_verify(ui_view_t* p) {
    ui_view_for_each(p, c, {
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
    if (p->init != null) { p->init(p); p->init = null; }
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
    if (c->init != null) { c->init(c); c->init = null; }
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
    if (c->init != null) { c->init(c); c->init = null; }
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
    if (c->init != null) { c->init(c); c->init = null; }
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
    if (c->init != null) { c->init(c); c->init = null; }
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
    c->parent = null;
}

static void ui_view_remove_tree(ui_view_t* p) {
    while (p->child != null) {
        ui_view_remove_tree(p->child);
        ui_view.remove(p->child);
    }
}

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
        nls.string(view->strid, view->text) : view->text;
}

static void ui_view_measure(ui_view_t* view) {
    ui_font_t f = view->font != null ? *view->font : app.fonts.regular;
    view->em = gdi.get_em(f);
    view->baseline = gdi.baseline(f);
    view->descent  = gdi.descent(f);
    if (view->text[0] != 0) {
        view->w = (int32_t)(view->em.x * view->width + 0.5);
        ui_point_t mt = { 0 };
        if (view->type == ui_view_label && ((ui_label_t*)view)->multiline) {
            int32_t w = (int)(view->width * view->em.x + 0.5);
            mt = gdi.measure_multiline(f, w == 0 ? -1 : w, ui_view.nls(view));
        } else {
            mt = gdi.measure_text(f, ui_view.nls(view));
        }
        view->h = mt.y;
        view->w = maximum(view->w, mt.x);
    }
}

static bool ui_view_inside(ui_view_t* view, const ui_point_t* pt) {
    const int32_t x = pt->x - view->x;
    const int32_t y = pt->y - view->y;
    return 0 <= x && x < view->w && 0 <= y && y < view->h;
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
        view->strid = nls.strid(view->text);
    }
}

static void ui_view_hovering(ui_view_t* view, bool start) {
    static ui_label(btn_tooltip,  "");
    if (start && app.animating.view == null && view->tip[0] != 0 &&
       !ui_view.is_hidden(view)) {
        strprintf(btn_tooltip.view.text, "%s", nls.str(view->tip));
        btn_tooltip.view.font = &app.fonts.H1;
        int32_t y = app.mouse.y - view->em.y;
        // enough space above? if not show below
        if (y < view->em.y) { y = app.mouse.y + view->em.y * 3 / 2; }
        y = minimum(app.crc.h - view->em.y * 3 / 2, maximum(0, y));
        app.show_tooltip(&btn_tooltip.view, app.mouse.x, y, 0);
    } else if (!start && app.animating.view == &btn_tooltip.view) {
        app.show_tooltip(null, -1, -1, 0);
    }
}

static bool ui_view_is_shortcut_key(ui_view_t* view, int32_t key) {
    // Supported keyboard shortcuts are ASCII characters only for now
    // If there is not focused UI control in Alt+key [Alt] is optional.
    // If there is focused control only Alt+Key is accepted as shortcut
    char ch = 0x20 <= key && key <= 0x7F ? (char)toupper(key) : 0x00;
    bool need_alt = app.focus != null && app.focus != view;
    bool keyboard_shortcut = ch != 0x00 && view->shortcut != 0x00 &&
         (app.alt || !need_alt) && toupper(view->shortcut) == ch;
    return keyboard_shortcut;
}

static bool ui_view_is_hidden(const ui_view_t* view) {
    bool hidden = view->hidden;
    while (!hidden && view->parent != null) {
        view = view->parent;
        hidden = view->hidden;
    }
    return hidden;
}

static bool ui_view_is_disabled(const ui_view_t* view) {
    bool disabled = view->disabled;
    while (!disabled && view->parent != null) {
        view = view->parent;
        disabled = view->disabled;
    }
    return disabled;
}

static void ui_view_init_children(ui_view_t* view) {
    for (ui_view_t** c = view->children; c != null && *c != null; c++) {
        if ((*c)->init != null) { (*c)->init(*c); (*c)->init = null; }
        if ((*c)->font == null) { (*c)->font = &app.fonts.regular; }
        if ((*c)->em.x == 0 || (*c)->em.y == 0) {
            (*c)->em = gdi.get_em(*view->font);
        }
        if ((*c)->text[0] != 0) { ui_view.localize(*c); }
        ui_view_init_children(*c);
    }
}

static void ui_view_parents(ui_view_t* view) {
    for (ui_view_t** c = view->children; c != null && *c != null; c++) {
        if ((*c)->parent == null) {
            (*c)->parent = view;
            ui_view_parents(*c);
        } else {
            assert((*c)->parent == view, "no reparenting");
        }
    }
}

// timers are delivered even to hidden and disabled views:

static void ui_view_timer(ui_view_t* view, ui_timer_t id) {
    if (view->timer != null) { view->timer(view, id); }
    ui_view_t** c = view->children;
    while (c != null && *c != null) { ui_view_timer(*c, id); c++; }
}

static void ui_view_every_sec(ui_view_t* view) {
    if (view->every_sec != null) { view->every_sec(view); }
    ui_view_t** c = view->children;
    while (c != null && *c != null) { ui_view_every_sec(*c); c++; }
}

static void ui_view_every_100ms(ui_view_t* view) {
    if (view->every_100ms != null) { view->every_100ms(view); }
    ui_view_t** c = view->children;
    while (c != null && *c != null) { ui_view_every_100ms(*c); c++; }
}

static void ui_view_key_pressed(ui_view_t* view, int32_t p) {
    if (!view->hidden && !view->disabled) {
        if (view->key_pressed != null) { view->key_pressed(view, p); }
        ui_view_t** c = view->children;
        while (c != null && *c != null) { ui_view_key_pressed(*c, p); c++; }
    }
}

static void ui_view_key_released(ui_view_t* view, int32_t p) {
    if (!view->hidden && !view->disabled) {
        if (view->key_released != null) { view->key_released(view, p); }
        ui_view_t** c = view->children;
        while (c != null && *c != null) { ui_view_key_released(*c, p); c++; }
    }
}

static void ui_view_character(ui_view_t* view, const char* utf8) {
    if (!view->hidden && !view->disabled) {
        if (view->character != null) { view->character(view, utf8); }
        ui_view_t** c = view->children;
        while (c != null && *c != null) { ui_view_character(*c, utf8); c++; }
    }
}

static void ui_view_paint(ui_view_t* view) {
    if (!view->hidden && app.crc.w > 0 && app.crc.h > 0) {
        if (view->paint != null) { view->paint(view); }
        ui_view_t** c = view->children;
        while (c != null && *c != null) { ui_view_paint(*c); c++; }
    }
}

static bool ui_view_set_focus(ui_view_t* view) {
    bool set = false;
    ui_view_t** c = view->children;
    while (c != null && *c != null && !set) { set = ui_view_set_focus(*c); c++; }
    if (!ui_view.is_hidden(view) && !ui_view.is_disabled(view) &&
        view->focusable && view->set_focus != null &&
       (app.focus == view || app.focus == null)) {
        set = view->set_focus(view);
    }
    return set;
}

static void ui_view_kill_focus(ui_view_t* view) {
    ui_view_t** c = view->children;
    while (c != null && *c != null) { ui_view_kill_focus(*c); c++; }
    if (view->kill_focus != null && view->focusable) {
        view->kill_focus(view);
    }
}

static void ui_view_mouse(ui_view_t* view, int32_t m, int32_t f) {
    if (!ui_view.is_hidden(view) &&
       (m == ui.message.mouse_hover || m == ui.message.mouse_move)) {
        ui_rect_t r = { view->x, view->y, view->w, view->h};
        bool hover = view->hover;
        view->hover = ui.point_in_rect(&app.mouse, &r);
        // inflate view rectangle:
        r.x -= view->w / 4;
        r.y -= view->h / 4;
        r.w += view->w / 2;
        r.h += view->h / 2;
        if (hover != view->hover) { app.invalidate(&r); }
        if (hover != view->hover) {
            ui_view.hover_changed(view);
        }
    }
    if (!ui_view.is_hidden(view) && !ui_view.is_disabled(view)) {
        if (view->mouse != null) { view->mouse(view, m, f); }
        for (ui_view_t** c = view->children; c != null && *c != null; c++) {
            ui_view_mouse(*c, m, f);
        }
    }
}

static void ui_view_mouse_wheel(ui_view_t* view, int32_t dx, int32_t dy) {
    if (!ui_view.is_hidden(view) && !ui_view.is_disabled(view)) {
        if (view->mouse_wheel != null) { view->mouse_wheel(view, dx, dy); }
        ui_view_t** c = view->children;
        while (c != null && *c != null) { ui_view_mouse_wheel(*c, dx, dy); c++; }
    }
}

static void ui_view_measure_children(ui_view_t* view) {
    if (!view->hidden) {
        ui_view_t** c = view->children;
        while (c != null && *c != null) { ui_view_measure_children(*c); c++; }
        if (view->measure != null) {
            view->measure(view);
        } else {
            ui_view.measure(view);
        }
    }
}

static void ui_view_layout_children(ui_view_t* view) {
    if (!view->hidden) {
        if (view->layout != null) { view->layout(view); }
        ui_view_t** c = view->children;
        while (c != null && *c != null) { ui_view_layout_children(*c); c++; }
    }
}

static void ui_view_hover_changed(ui_view_t* view) {
    if (!view->hidden) {
        if (!view->hover) {
            view->hover_at = 0;
            ui_view.hovering(view, false); // cancel hover
        } else {
            swear(ui_view_hover_delay >= 0);
            if (view->hover_at >= 0) {
                view->hover_at = app.now + ui_view_hover_delay;
            }
        }
    }
}

static void ui_view_kill_hidden_focus(ui_view_t* view) {
    // removes focus from hidden or disabled ui controls
    if (app.focus != null) {
        if (app.focus == view && (view->disabled || view->hidden)) {
            app.focus = null;
            // even for disabled or hidden view notify about kill_focus:
            view->kill_focus(view);
        } else {
            ui_view_t** c = view->children;
            while (c != null && *c != null) {
                ui_view_kill_hidden_focus(*c);
                c++;
            }
        }
    }
}

static bool ui_view_tap(ui_view_t* view, int32_t ix) { // 0: left 1: middle 2: right
    bool done = false; // consumed
    if (!ui_view.is_hidden(view) && !ui_view.is_disabled(view) &&
         ui_view_inside(view, &app.mouse)) {
        for (ui_view_t** c = view->children; c != null && *c != null && !done; c++) {
            done = ui_view_tap(*c, ix);
        }
        if (view->tap != null && !done) { done = view->tap(view, ix); }
    }
    return done;
}

static bool ui_view_press(ui_view_t* view, int32_t ix) { // 0: left 1: middle 2: right
    bool done = false; // consumed
    if (!ui_view.is_hidden(view) && !ui_view.is_disabled(view)) {
        for (ui_view_t** c = view->children; c != null && *c != null && !done; c++) {
            done = ui_view_press(*c, ix);
        }
        if (view->press != null && !done) { done = view->press(view, ix); }
    }
    return done;
}

static bool ui_view_context_menu(ui_view_t* view) {
    if (!ui_view.is_hidden(view) && !ui_view.is_disabled(view)) {
        for (ui_view_t** c = view->children; c != null && *c != null; c++) {
            if (ui_view_context_menu(*c)) { return true; }
        }
        ui_rect_t r = { view->x, view->y, view->w, view->h};
        if (ui.point_in_rect(&app.mouse, &r)) {
            if (!view->hidden && !view->disabled && view->context_menu != null) {
                view->context_menu(view);
            }
        }
    }
    return false;
}

static bool ui_view_message(ui_view_t* view, int32_t m, int64_t wp, int64_t lp,
        int64_t* ret) {
    if (!view->hidden) {
        if (view->hover_at > 0 && app.now > view->hover_at) {
            view->hover_at = -1; // "already called"
            ui_view.hovering(view, true);
        }
    }
    // message() callback is called even for hidden and disabled views
    // could be useful for enabling conditions of post() messages from
    // background threads.
    if (view->message != null) {
        if (view->message(view, m, wp, lp, ret)) { return true; }
    }
    ui_view_t** c = view->children;
    while (c != null && *c != null) {
        if (ui_view_message(*c, m, wp, lp, ret)) { return true; }
        c++;
    }
    return false;
}

#pragma push_macro("ui_view_alone")

#define ui_view_alone(v) do {                          \
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
    ui_view_alone(&p0); ui_view_alone(&c1); ui_view_alone(&c4);
    ui_view.remove(&g1);                            ui_view_verify(&c2);
    ui_view.remove(&g2);                            ui_view_verify(&c2);
    ui_view.remove(&g3);                            ui_view_verify(&c3);
    ui_view.remove(&g4);                            ui_view_verify(&c3);
    ui_view_alone(&c2); ui_view_alone(&c3);
    ui_view_alone(&g1); ui_view_alone(&g2); ui_view_alone(&g3); ui_view_alone(&g4);
    // a bit more intuitive (for a human) nested way to initialize tree:
    ui_view.add(&p0,
        &c1,
        ui_view.add(&c2, &g1, &g2, null),
        ui_view.add(&c3, &g3, &g4, null),
        &c4);
    ui_view_verify(&p0);
    ui_view_remove_tree(&p0);
    ui_view_alone(&p0);
    ui_view_alone(&c1); ui_view_alone(&c2); ui_view_alone(&c3); ui_view_alone(&c4);
    ui_view_alone(&g1); ui_view_alone(&g2); ui_view_alone(&g3); ui_view_alone(&g4);
    if (debug.verbosity.level > debug.verbosity.quiet) { traceln("done"); }
}

#pragma pop_macro("ui_view_alone")

void ui_view_init(ui_view_t* unused(view)) { }

void ui_view_init_container(ui_view_t* view) { ui_view_init(view); }

ui_view_if ui_view = {
    .add                = ui_view_add,
    .add_first          = ui_view_add_first,
    .add_last           = ui_view_add_last,
    .add_after          = ui_view_add_after,
    .add_before         = ui_view_add_before,
    .add                = ui_view_add,
    .remove             = ui_view_remove,
    .inside             = ui_view_inside,
    .set_text           = ui_view_set_text,
    .invalidate         = ui_view_invalidate,
    .measure            = ui_view_measure,
    .nls                = ui_view_nls,
    .localize           = ui_view_localize,
    .is_hidden          = ui_view_is_hidden,
    .is_disabled        = ui_view_is_disabled,
    .init_children      = ui_view_init_children,
    .set_parents        = ui_view_parents,
    .timer              = ui_view_timer,
    .every_sec          = ui_view_every_sec,
    .every_100ms        = ui_view_every_100ms,
    .key_pressed        = ui_view_key_pressed,
    .key_released       = ui_view_key_released,
    .character          = ui_view_character,
    .paint              = ui_view_paint,
    .set_focus          = ui_view_set_focus,
    .kill_focus         = ui_view_kill_focus,
    .kill_hidden_focus  = ui_view_kill_hidden_focus,
    .mouse              = ui_view_mouse,
    .mouse_wheel        = ui_view_mouse_wheel,
    .measure_children   = ui_view_measure_children,
    .layout_children    = ui_view_layout_children,
    .hovering           = ui_view_hovering,
    .hover_changed      = ui_view_hover_changed,
    .is_shortcut_key    = ui_view_is_shortcut_key,
    .context_menu       = ui_view_context_menu,
    .tap                = ui_view_tap,
    .press              = ui_view_press,
    .message            = ui_view_message,
    .test               = ui_view_test
};

static_init(ui_view) {
    ui_view.test();
}