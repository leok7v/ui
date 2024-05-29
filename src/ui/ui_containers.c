/* Copyright (c) Dmitry "Leo" Kuznetsov 2021-24 see LICENSE for details */
#include "ut/ut.h"
#include "ui/ui.h"

static bool ui_containers_debug = false;

#pragma push_macro("debugln")
#pragma push_macro("ui_layout_dump")
#pragma push_macro("ui_layout_enter")
#pragma push_macro("ui_layout_exit")

// Usage of: ui_view_for_each_begin(p, c) { ... } ui_view_for_each_end(p, c)
// makes code inside iterator debugger friendly and ensures correct __LINE__

#define debugln(...) do {                                \
    if (ui_containers_debug) {  traceln(__VA_ARGS__); }  \
} while (0)

#define ui_layout_enter(v) do {                               \
    ui_ltrb_t i_ = ui_view.gaps(v, &v->insets);               \
    ui_ltrb_t p_ = ui_view.gaps(v, &v->padding);              \
    debugln(">%s %d,%d %dx%d p: %d %d %d %d  i: %d %d %d %d", \
                 v->text, v->x, v->y, v->w, v->h,             \
                 p_.left, p_.top, p_.right, p_.bottom,        \
                 i_.left, i_.top, i_.right, i_.bottom);       \
} while (0)

#define ui_layout_exit(v) do {                                   \
    debugln("<%s %d,%d %dx%d", v->text, v->x, v->y, v->w, v->h); \
} while (0)

static const char* ui_container_finite_int(int32_t v, char* text, int32_t count) {
    swear(v >= 0);
    if (v == ui.infinity) {
        ut_str.format(text, count, "%s", ui_glyph_infinity);
    } else {
        ut_str.format(text, count, "%d", v);
    }
    return text;
}

#define ui_layout_dump(v) do {                                                \
    char maxw[32];                                                            \
    char maxh[32];                                                            \
    debugln("%s[%4.4s] %d,%d %dx%d, max[%sx%s] "                              \
        "padding { %.3f %.3f %.3f %.3f } "                                    \
        "insets { %.3f %.3f %.3f %.3f } align: 0x%02X",                       \
        v->text, &v->type, v->x, v->y, v->w, v->h,                            \
        ui_container_finite_int(v->max_w, maxw, countof(maxw)),               \
        ui_container_finite_int(v->max_h, maxh, countof(maxh)),               \
        v->padding.left, v->padding.top, v->padding.right, v->padding.bottom, \
        v->insets.left, v->insets.top, v->insets.right, v->insets.bottom,     \
        v->align);                                                            \
} while (0)

static void ui_span_measure(ui_view_t* p) {
    ui_layout_enter(p);
    swear(p->type == ui_view_span, "type %4.4s 0x%08X", &p->type, p->type);
    ui_ltrb_t insets;
    ui_view.inbox(p, null, &insets);
    int32_t w = insets.left;
    int32_t h = 0;
    int32_t max_w = w;
    ui_view_for_each_begin(p, c) {
        swear(c->max_w == 0 || c->max_w >= c->w,
              "max_w: %d w: %d", c->max_w, c->w);
        if (c->hidden) {
            // nothing
        } else if (c->type == ui_view_spacer) {
            c->padding = (ui_gaps_t){ 0, 0, 0, 0 };
            c->w = 0; // layout will distribute excess here
            c->h = 0; // starts with zero
            max_w = ui.infinity; // spacer make width greedy
        } else {
            ui_rect_t cbx; // child "out" box expanded by padding
            ui_ltrb_t padding;
            ui_view.outbox(c, &cbx, &padding);
            h = ut_max(h, cbx.h);
            if (c->max_w == ui.infinity) {
                max_w = ui.infinity;
            } else if (max_w < ui.infinity && c->max_w != 0) {
                swear(c->max_w >= cbx.w, "Constraint violation: "
                        "c->max_w %d < cbx.w %d, ", c->max_w, cbx.w);
                max_w += c->max_w;
            } else if (max_w < ui.infinity) {
                swear(0 <= max_w + cbx.w &&
                      (int64_t)max_w + (int64_t)cbx.w < (int64_t)ui.infinity,
                      "max_w:%d + cbx.w:%d = %d", max_w, cbx.w, max_w + cbx.w);
                max_w += cbx.w;
            }
            w += cbx.w;
        }
    } ui_view_for_each_end(p, c);
    if (0 < max_w && max_w < ui.infinity) {
        swear(0 <= max_w + insets.right &&
              (int64_t)max_w + (int64_t)insets.right < (int64_t)ui.infinity,
             "max_w:%d + right:%d = %d", max_w, insets.right, max_w + insets.right);
        max_w += insets.right;
    }
    swear(max_w == 0 || max_w >= w, "max_w: %d w: %d", max_w, w);
//  TODO: childrens max_w is infinity does NOT mean
//        max_w of the parent is infinity? if this is correct remove
//        commented section
//  if (max_w != w) { // only if max_w differs from actual width
//      p->max_w = ut_max(max_w, p->max_w);
//  }
    if (p->hidden) {
        p->w = 0;
        p->h = 0;
    } else {
        p->w = w + insets.right;
        p->h = insets.top + h + insets.bottom;
        swear(p->max_w == 0 || p->max_w >= p->w,
              "max_w: %d is less than actual width: %d", p->max_w, p->w);
    }
    ui_layout_exit(p);
}

// after measure of the subtree is concluded the parent ui_span
// may adjust span_w wider number depending on it's own width
// and ui_span.max_w agreement

static int32_t ui_span_place_child(ui_view_t* c, ui_rect_t pbx, int32_t x) {
    ui_ltrb_t padding = ui_view.gaps(c, &c->padding);
    // setting child`s max_h to infinity means that child`s height is
    // *always* fill vertical view size of the parent
    // childs.h can exceed parent.h (vertical overflow) - is not
    // encouraged but allowed
    if (c->max_h == ui.infinity) {
        // important c->h changed, cbx.h is no longer valid
        c->h = ut_max(c->h, pbx.h - padding.top - padding.bottom);
    }
    int32_t min_y = pbx.y + padding.top;
    if ((c->align & ui.align.top) != 0) {
        assert(c->align == ui.align.top);
        c->y = min_y;
    } else if ((c->align & ui.align.bottom) != 0) {
        assert(c->align == ui.align.bottom);
        c->y = ut_max(min_y, pbx.y + pbx.h - c->h - padding.bottom);
    } else { // effective height (c->h might have been changed)
        assert(c->align == ui.align.center);
        const int32_t ch = padding.top + c->h + padding.bottom;
        c->y = ut_max(min_y, pbx.y + (pbx.h - ch) / 2 + padding.top);
    }
    c->x = x + padding.left;
    return c->x + c->w + padding.right;
}

static void ui_span_layout(ui_view_t* p) {
    debugln(">%s (%d,%d) %dx%d", p->text, p->x, p->y, p->w, p->h);
    swear(p->type == ui_view_span, "type %4.4s 0x%08X", &p->type, p->type);
    ui_rect_t pbx; // parent "in" box (sans insets)
    ui_ltrb_t insets;
    ui_view.inbox(p, &pbx, &insets);
    int32_t spacers = 0; // Number of spacers
    int32_t max_w_count = 0;
    int32_t x = p->x + insets.left;
    ui_view_for_each_begin(p, c) {
        if (c->hidden) {
            // nothing
        } else if (c->type == ui_view_spacer) {
            c->x = x;
            c->y = pbx.y;
            c->h = pbx.h;
            c->w = 0;
            spacers++;
        } else {
            x = ui_span_place_child(c, pbx, x);
            swear(c->max_w == 0 || c->max_w > c->w,
                  "max_w:%d < w:%d", c->max_w, c->w);
            if (c->max_w > 0) {
                max_w_count++;
            }
        }
    } ui_view_for_each_end(p, c);
    int32_t xw = ut_max(0, pbx.x + pbx.w - x); // excess width
    int32_t max_w_sum = 0;
    if (xw > 0 && max_w_count > 0) {
        ui_view_for_each_begin(p, c) {
            if (!c->hidden && c->type != ui_view_spacer && c->max_w > 0) {
                max_w_sum += ut_min(c->max_w, xw);
            }
        } ui_view_for_each_end(p, c);
    }
    if (xw > 0 && max_w_count > 0) {
        x = p->x + insets.left;
        int32_t k = 0;
        ui_view_for_each_begin(p, c) {
            if (!c->hidden) {
                ui_rect_t cbx; // child "out" box expanded by padding
                ui_ltrb_t padding;
                ui_view.outbox(c, &cbx, &padding);
                if (c->hidden) {
                    // nothing
                } else if (c->type == ui_view_spacer) {
                    swear(padding.left == 0 && padding.right == 0);
                } else if (c->max_w > 0) {
                    const int32_t max_w = ut_min(c->max_w, xw);
                    int64_t proportional = (xw * (int64_t)max_w) / max_w_sum;
                    assert(proportional <= (int64_t)INT32_MAX);
                    int32_t cw = (int32_t)proportional;
                    c->w = ut_min(c->max_w, c->w + cw);
                    k++;
                }
                // TODO: take into account .align of a child and adjust x
                //       depending on ui.align.left/right/center
                //       distributing excess width on the left and right of a child
                c->x = padding.left + x;
                x = c->x + padding.left + c->w + padding.right;
            }
        } ui_view_for_each_end(p, c);
        swear(k == max_w_count);
    }
    // excess width after max_w of non-spacers taken into account
    xw = ut_max(0, pbx.x + pbx.w - x);
    if (xw > 0 && spacers > 0) {
        // evenly distribute excess among spacers
        int32_t partial = xw / spacers;
        x = p->x + insets.left;
        ui_view_for_each_begin(p, c) {
            if (!c->hidden) {
                ui_rect_t cbx; // child "out" box expanded by padding
                ui_ltrb_t padding;
                ui_view.outbox(c, &cbx, &padding);
                if (c->type == ui_view_spacer) {
                    c->y = pbx.y;
                    c->w = partial;
                    c->h = pbx.h;
                    spacers--;
                }
                c->x = x + padding.left;
                x = c->x + c->w + padding.right;
            }
        } ui_view_for_each_end(p, c);
    }
    debugln("<%s (%d,%d) %dx%d", p->text, p->x, p->y, p->w, p->h);
}

static void ui_list_measure(ui_view_t* p) {
    debugln(">%s (%d,%d) %dx%d", p->text, p->x, p->y, p->w, p->h);
    swear(p->type == ui_view_list, "type %4.4s 0x%08X", &p->type, p->type);
    ui_rect_t pbx; // parent "in" box (sans insets)
    ui_ltrb_t insets;
    ui_view.inbox(p, &pbx, &insets);
    int32_t max_h = insets.top;
    int32_t h = insets.top;
    int32_t w = 0;
    ui_view_for_each_begin(p, c) {
        swear(c->max_h == 0 || c->max_h >= c->h, "max_h: %d h: %d",
              c->max_h, c->h);
        if (c->hidden) {
            // nothing
        } else if (c->type == ui_view_spacer) {
            c->padding = (ui_gaps_t){ 0, 0, 0, 0 };
            c->h = 0; // layout will distribute excess here
            max_h = ui.infinity; // spacer make height greedy
        } else {
            ui_rect_t cbx; // child "out" box expanded by padding
            ui_ltrb_t padding;
            ui_view.outbox(c, &cbx, &padding);
            w = ut_max(w, cbx.w);
            if (c->max_h == ui.infinity) {
                max_h = ui.infinity;
            } else if (max_h < ui.infinity && c->max_h != 0) {
                swear(c->max_h >= cbx.h, "c->max_h:%d < cbx.h: %d",
                      c->max_h, cbx.h);
                max_h += c->max_h;
            } else if (max_h < ui.infinity) {
                swear(0 <= max_h + cbx.h &&
                      (int64_t)max_h + (int64_t)cbx.h < (int64_t)ui.infinity,
                      "max_h:%d + ch:%d = %d", max_h, cbx.h, max_h + cbx.h);
                max_h += cbx.h;
            }
            h += cbx.h;
        }
    } ui_view_for_each_end(p, c);
    if (max_h < ui.infinity) {
        swear(0 <= max_h + insets.bottom &&
              (int64_t)max_h + (int64_t)insets.bottom < (int64_t)ui.infinity,
             "max_h:%d + bottom:%d = %d",
              max_h, insets.bottom, max_h + insets.bottom);
        max_h += insets.bottom;
    }
//  TODO: childrens max_w is infinity does NOT mean
//        max_w of the parent is infinity? if this is correct remove
//        commented section
//  swear(max_h == 0 || max_h >= h, "max_h is less than actual height h");
//  if (max_h != h) { // only if max_h differs from actual height
//      p->max_h = ut_max(max_h, p->max_h);
//  }
    if (p->hidden) {
        p->w = 0;
        p->h = 0;
    } else if (p == ui_app.root) {
        // ui_app.root is special case (expanded to a window)
        // TODO: when get_min_max() start taking content into account
        //       the code below may be changed to asserts() and removed
        //       after confirming the rest of the logic
        p->w = ui_app.no_decor ? ui_app.wrc.w : ui_app.crc.w;
        p->h = ui_app.no_decor ? ui_app.wrc.h : ui_app.crc.h;
    } else {
        p->h = h + insets.bottom;
        p->w = insets.left + w + insets.right;
    }
    debugln("<%s (%d,%d) %dx%d", p->text, p->x, p->y, p->w, p->h);
}

static int32_t ui_list_place_child(ui_view_t* c, ui_rect_t pbx, int32_t y) {
    ui_ltrb_t padding = ui_view.gaps(c, &c->padding);
    // setting child`s max_w to infinity means that child`s height is
    // *always* fill vertical view size of the parent
    // childs.w can exceed parent.w (horizontal overflow) - not encouraged but allowed
    if (c->max_w == ui.infinity) {
        c->w = ut_max(c->w, pbx.w - padding.left - padding.right);
    }
    int32_t min_x = pbx.x + padding.left;
    if ((c->align & ui.align.left) != 0) {
        assert(c->align == ui.align.left);
        c->x = min_x;
    } else if ((c->align & ui.align.right) != 0) {
        assert(c->align == ui.align.right);
        c->x = ut_max(min_x, pbx.x + pbx.w - c->w - padding.right);
    } else {
        assert(c->align == ui.align.center);
        const int32_t cw = padding.left + c->w + padding.right;
        c->x = ut_max(min_x, pbx.x + (pbx.w - cw) / 2 + padding.left);
    }
    c->y = y + padding.top;
    return c->y + c->h + padding.bottom;
}

static void ui_list_layout(ui_view_t* p) {
    debugln(">%s (%d,%d) %dx%d", p->text, p->x, p->y, p->w, p->h);
    swear(p->type == ui_view_list, "type %4.4s 0x%08X", &p->type, p->type);
    ui_rect_t pbx; // parent "in" box (sans insets)
    ui_ltrb_t insets;
    ui_view.inbox(p, &pbx, &insets);
    int32_t spacers = 0; // Number of spacers
    int32_t max_h_sum = 0;
    int32_t max_h_count = 0;
    int32_t y = pbx.y;
    ui_view_for_each_begin(p, c) {
        if (c->hidden) {
            // nothing
        } else if (c->type == ui_view_spacer) {
            c->x = pbx.x;
            c->y = y;
            c->w = pbx.w;
            c->h = 0;
            spacers++;
        } else {
            y = ui_list_place_child(c, pbx, y);
            swear(c->max_h == 0 || c->max_h > c->h,
                  "max_h:%d < h:%d", c->max_h, c->h);
            if (c->max_h > 0) {
                // clamp max_h to the effective parent height
                max_h_count++;
            }
        }
    } ui_view_for_each_end(p, c);
    int32_t xh = ut_max(0, pbx.y + pbx.h - y); // excess height
    if (xh > 0 && max_h_count > 0) {
        ui_view_for_each_begin(p, c) {
            if (!c->hidden && c->type != ui_view_spacer && c->max_h > 0) {
                max_h_sum += ut_min(c->max_h, xh);
            }
        } ui_view_for_each_end(p, c);
    }
    if (xh > 0 && max_h_count > 0) {
        y = pbx.y;
        int32_t k = 0;
        ui_view_for_each_begin(p, c) {
            if (!c->hidden) {
                ui_rect_t cbx; // child "out" box expanded by padding
                ui_ltrb_t padding;
                ui_view.outbox(c, &cbx, &padding);
                if (c->type != ui_view_spacer && c->max_h > 0) {
                    const int32_t max_h = ut_min(c->max_h, xh);
                    int64_t proportional = (xh * (int64_t)max_h) / max_h_sum;
                    assert(proportional <= (int64_t)INT32_MAX);
                    int32_t ch = (int32_t)proportional;
                    c->h = ut_min(c->max_h, c->h + ch);
                    k++;
                }
                int32_t ch = padding.top + c->h + padding.bottom;
                c->y = y + padding.top;
                y += ch;
            }
        } ui_view_for_each_end(p, c);
        swear(k == max_h_count);
    }
    // excess height after max_h of non-spacers taken into account
    xh = ut_max(0, pbx.y + pbx.h - y); // excess height
    if (xh > 0 && spacers > 0) {
        // evenly distribute excess among spacers
        int32_t partial = xh / spacers;
        y = pbx.y;
        ui_view_for_each_begin(p, c) {
            if (!c->hidden) {
                ui_rect_t cbx; // child "out" box expanded by padding
                ui_ltrb_t padding;
                ui_view.outbox(c, &cbx, &padding);
                if (c->type == ui_view_spacer) {
                    c->x = pbx.x;
                    c->w = pbx.x + pbx.w - pbx.x;
                    c->h = partial; // TODO: xxxxx last?
                    spacers--;
                }
                int32_t ch = padding.top + c->h + padding.bottom;
                c->y = y + padding.top;
                y += ch;
            }
        } ui_view_for_each_end(p, c);
    }
    debugln("<%s (%d,%d) %dx%d", p->text, p->x, p->y, p->w, p->h);
}

static void ui_container_measure(ui_view_t* p) {
    ui_layout_enter(p);
    swear(p->type == ui_view_container, "type %4.4s 0x%08X", &p->type, p->type);
    ui_rect_t pbx; // parent "in" box (sans insets)
    ui_ltrb_t insets;
    ui_view.inbox(p, &pbx, &insets);
    // empty container minimum size:
    p->w = insets.left + insets.right;
    p->h = insets.top + insets.bottom;
    ui_view_for_each_begin(p, c) {
        if (!c->hidden) {
            ui_rect_t cbx; // child "out" box expanded by padding
            ui_ltrb_t padding;
            ui_view.outbox(c, &cbx, &padding);
            p->w = ut_max(p->w, padding.left + c->w + padding.right);
            p->h = ut_max(p->h, padding.top + c->h + padding.bottom);
        }
    } ui_view_for_each_end(p, c);
    ui_layout_exit(p);
}

static void ui_container_layout(ui_view_t* p) {
    ui_layout_enter(p);
    swear(p->type == ui_view_container, "type %4.4s 0x%08X", &p->type, p->type);
    ui_rect_t pbx; // parent "in" box (sans insets)
    ui_ltrb_t insets;
    ui_view.inbox(p, &pbx, &insets);
    ui_view_for_each_begin(p, c) {
        if (c->type != ui_view_spacer && !c->hidden) {
            ui_rect_t cbx; // child "out" box expanded by padding
            ui_ltrb_t padding;
            ui_view.outbox(c, &cbx, &padding);
            const int32_t pw = p->w - insets.left - insets.right - padding.left - padding.right;
            const int32_t ph = p->h - insets.top - insets.bottom - padding.top - padding.bottom;
            int32_t cw = c->max_w == ui.infinity ? pw : c->max_w;
            if (cw > 0) {
                c->w = ut_min(cw, pw);
            }
            int32_t ch = c->max_h == ui.infinity ? ph : c->max_h;
            if (ch > 0) {
                c->h = ut_min(ch, ph);
            }
            swear((c->align & (ui.align.left|ui.align.right)) !=
                               (ui.align.left|ui.align.right),
                   "align: left|right 0x%02X", c->align);
            swear((c->align & (ui.align.top|ui.align.bottom)) !=
                               (ui.align.top|ui.align.bottom),
                   "align: top|bottom 0x%02X", c->align);
            int32_t min_x = pbx.x + padding.left;
            if ((c->align & ui.align.left) != 0) {
                c->x = min_x;
            } else if ((c->align & ui.align.right) != 0) {
                c->x = ut_max(min_x, pbx.x + pbx.w - c->w - padding.right);
            } else {
                c->x = ut_max(min_x, min_x + (pbx.w - (padding.left + c->w + padding.right)) / 2);
            }
            int32_t min_y = pbx.y + padding.top;
            if ((c->align & ui.align.top) != 0) {
                c->y = min_y;
            } else if ((c->align & ui.align.bottom) != 0) {
                c->y = ut_max(min_y, pbx.x + pbx.h - c->h - padding.bottom);
            } else {
                c->y = ut_max(min_y, min_y + (pbx.h - (padding.top + c->h + padding.bottom)) / 2);
            }
//          debugln(" %s %d,%d %dx%d", c->text, c->x, c->y, c->w, c->h);
        }
    } ui_view_for_each_end(p, c);
    ui_layout_exit(p);
}

static void ui_paint_container(ui_view_t* v) {
    if (!ui_color_is_undefined(v->background) &&
        !ui_color_is_transparent(v->background)) {
        ui_gdi.fill_with(v->x, v->y, v->w, v->h, v->background);
    } else {
//      traceln("%s undefined", v->text);
    }
}

static void ui_view_container_init(ui_view_t* v) {
    v->background = ui_colors.transparent;
    v->insets  = (ui_gaps_t){ .left  = 0.25, .top    = 0.25,
                              .right = 0.25, .bottom = 0.25 };
}

void ui_view_init_span(ui_view_t* v) {
    swear(v->type == ui_view_span, "type %4.4s 0x%08X", &v->type, v->type);
    ui_view_container_init(v);
    if (v->measure == null) { v->measure = ui_span_measure; }
    if (v->layout  == null) { v->layout  = ui_span_layout; }
    if (v->paint   == null) { v->paint   = ui_paint_container; }
    if (v->text[0] == 0) { ut_str_printf(v->text, "ui_span"); }
}

void ui_view_init_list(ui_view_t* v) {
    swear(v->type == ui_view_list, "type %4.4s 0x%08X", &v->type, v->type);
    ui_view_container_init(v);
    if (v->measure == null) { v->measure = ui_list_measure; }
    if (v->layout  == null) { v->layout  = ui_list_layout; }
    if (v->paint   == null) { v->paint   = ui_paint_container; }
    if (v->text[0] == 0) { ut_str_printf(v->text, "ui_list"); }
}

void ui_view_init_spacer(ui_view_t* v) {
    swear(v->type == ui_view_spacer, "type %4.4s 0x%08X", &v->type, v->type);
    v->w = 0;
    v->h = 0;
    v->max_w = ui.infinity;
    v->max_h = ui.infinity;
    if (v->text[0] == 0) { ut_str_printf(v->text, "ui_spacer"); }
}

void ui_view_init_container(ui_view_t* v) {
    ui_view_container_init(v);
    if (v->measure == null) { v->measure = ui_container_measure; }
    if (v->layout  == null) { v->layout  = ui_container_layout; }
    if (v->paint   == null) { v->paint   = ui_paint_container; }
    if (v->text[0] == 0) { ut_str_printf(v->text, "ui_container"); }
}

#pragma pop_macro("ui_layout_exit")
#pragma pop_macro("ui_layout_enter")
#pragma pop_macro("ui_layout_dump")
#pragma pop_macro("debugln")
