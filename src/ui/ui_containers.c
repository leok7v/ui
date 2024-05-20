/* Copyright (c) Dmitry "Leo" Kuznetsov 2021-24 see LICENSE for details */
#include "ut/ut.h"
#include "ui/ui.h"

static bool ui_containers_debug = false;

#pragma push_macro("debugln")
#pragma push_macro("dump")

// Usage of: ui_view_for_each_begin(p, c) { ... } ui_view_for_each_end(p, c)
// makes code inside iterator debugger friendly and ensures correct __LINE__

#define debugln(...) do {                                \
    if (ui_containers_debug) {  traceln(__VA_ARGS__); }  \
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

#define dump(v) do {                                                          \
    char maxw[32];                                                            \
    char maxh[32];                                                            \
    debugln("%s[%4.4s] %d,%d %dx%d, max[%sx%s] "                              \
        "padding { %.3f %.3f %.3f %.3f } "                                    \
        "insets { %.3f %.3f %.3f %.3f } align: 0x%02X",                       \
        v->text, &v->type, v->x, v->y, v->w, v->h,                            \
        ui_container_finite_int(v->max_w, maxw, countof(maxw)),               \
        ui_container_finite_int(v->max_h, maxh, countof(maxh)),               \
        v->padding.left, v->padding.top, v->padding.right, v->padding.bottom, \
        v->insets.left, v->insets.top,v->insets.right, v->insets.bottom,      \
        v->align);                                                            \
} while (0)

static void ui_span_measure(ui_view_t* p) {
//  traceln(">%s %d,%d %dx%d", p->text, p->x, p->y, p->w, p->h);
    swear(p->type == ui_view_span, "type %4.4s 0x%08X", &p->type, p->type);
    ui_ltrb_t insets;
    ui_view.inbox(p, null, &insets);
    int32_t max_w = insets.left;
    int32_t w = max_w;
    int32_t h = 0;
    ui_view_for_each_begin(p, c) {
        swear(c->max_w == 0 || c->max_w >= c->w,
              "max_w: %d w: %d", c->max_w, c->w);
        if (c->type == ui_view_spacer) {
            c->padding = (ui_gaps_t){ 0, 0, 0, 0 };
            c->w = 0; // layout will distribute excess here
            c->h = 0; // starts with zero
            max_w = ui.infinity; // spacer make width greedy
        } else {
            ui_rect_t cbx; // child "out" box expanded by padding
            ui_ltrb_t padding;
            ui_view.outbox(c, &cbx, &padding);
// traceln(" %s %dx%d out: %dx%d", c->text, c->w, c->h, cbx.w, cbx.h);
            h = ut_max(h, cbx.h);
            if (c->max_w == ui.infinity) {
                max_w = ui.infinity;
            } else if (max_w < ui.infinity && c->max_w != 0) {
                swear(c->max_w >= cbx.w, "Constraint violation: "
                        "c->max_w %d < cbx.w %d, ", c->max_w, cbx.w);
                max_w += c->max_w;
            } else if (max_w < ui.infinity) {
                swear(0 <= max_w + cbx.w && max_w + cbx.w < ui.infinity,
                      "Width overflow: max_w + cbx.w = %d", max_w + cbx.w);
                max_w += cbx.w;
            }
            w += cbx.w;
        }
    } ui_view_for_each_end(p, c);
    if (max_w < ui.infinity) {
        swear(0 <= max_w + insets.right && max_w + insets.right < ui.infinity,
             "Width overflow at right inset: max_w + right = %d",
              max_w + insets.right);
        max_w += insets.right;
    }
    w += insets.right;
    h += insets.top + insets.bottom;
    swear(max_w == 0 || max_w >= w,
         "max_w: %d is less than actual width w: %d", max_w, w);
    // Handle max width only if it differs from actual width
    p->max_w = max_w == w ? p->max_w : ut_max(max_w, p->max_w);
    p->w = w;
    p->h = h;
    swear(p->max_w == 0 || p->max_w >= p->w, "max_w is less than actual width w");
//  traceln("<%s %d,%d %dx%d", p->text, p->x, p->y, p->w, p->h);
}

// after measure of the subtree is concluded the parent ui_span
// may adjust span_w wider number depending on it's own width
// and ui_span.max_w agreement

static void ui_span_layout(ui_view_t* p) {
//  traceln(">%s %d,%d %dx%d", p->text, p->x, p->y, p->w, p->h);
    swear(p->type == ui_view_span, "type %4.4s 0x%08X", &p->type, p->type);
    ui_rect_t pbx; // parent "in" box (sans insets)
    ui_ltrb_t insets;
    ui_view.inbox(p, &pbx, &insets);
    int32_t spacers = 0; // Number of spacers
//  // Left and right insets
//  const int32_t i_lf = ui.gaps_em2px(p->fm->em.w, p->insets.left);
//  const int32_t i_rt = ui.gaps_em2px(p->fm->em.w, p->insets.right);
//  // Top and bottom insets
//  const int32_t i_tp = ui.gaps_em2px(p->fm->em.h, p->insets.top);
//  const int32_t i_bt = ui.gaps_em2px(p->fm->em.h, p->insets.bottom);
//  const int32_t lf = p->x + i_lf;
//  swear(lf < rt, "Inverted or zero-width conditions: lf: %d, rt: %d", lf, rt);
    // Top and bottom y coordinates
    const int32_t top = p->y + insets.top;
swear(top == pbx.y);
    // Mitigation for vertical overflow:
    const int32_t bot = p->y + p->h - insets.bottom < top ? 
                        top + p->h : p->y + p->h - insets.bottom;
    int32_t max_w_count = 0;
    int32_t x = p->x + insets.left;
    ui_view_for_each_begin(p, c) {
        if (c->type == ui_view_spacer) {
            c->x = x;
            c->y = pbx.y;
            c->h = pbx.h;
            c->w = 0;
            spacers++;
        } else {
            ui_rect_t cbx; // child "out" box expanded by padding
            ui_ltrb_t padding;
            ui_view.outbox(c, &cbx, &padding);
const int32_t p_lf = ui.gaps_em2px(c->fm->em.w, c->padding.left);
const int32_t p_tp = ui.gaps_em2px(c->fm->em.h, c->padding.top);
const int32_t p_rt = ui.gaps_em2px(c->fm->em.w, c->padding.right);
const int32_t p_bt = ui.gaps_em2px(c->fm->em.h, c->padding.bottom);
            // setting child`s max_h to infinity means that child`s height is
            // *always* fill vertical view size of the parent
            // childs.h can exceed parent.h (vertical overflow) - is not
            // encouraged but allowed
            if (c->max_h == ui.infinity) {
                // important c->h changed, cbx.h is no longer valid
                c->h = ut_max(c->h, pbx.h - padding.top - padding.bottom);
            }
            if ((c->align & ui.align.top) != 0) {
swear(top + p_tp == pbx.y + padding.top);
                c->y = pbx.y + padding.top;
            } else if ((c->align & ui.align.bottom) != 0) {
swear(bot - (c->h + p_bt) == pbx.y + pbx.h - c->h - padding.bottom);
                c->y = bot - (c->h + p_bt);
            } else {
                const int32_t ch0 = p_tp + c->h + p_bt;
                const int32_t ch1 = padding.top + c->h + padding.bottom;
assert(ch0 == ch1);
assert(bot - top == pbx.h);
                c->y = top + p_tp + (bot - top - ch0) / 2;
                int32_t cy = pbx.y + (pbx.h - ch1) / 2 + padding.top;
                assert(cy == c->y);
            }
            c->x = x + p_lf;
            x = c->x + c->w + p_rt;
            swear(c->max_w == 0 || c->max_w > c->w, "max_w must be greater "
                  "than current width: max_w: %d, w: %d", c->max_w, c->w);
            if (c->max_w > 0) {
                max_w_count++;
            }
        }
    } ui_view_for_each_end(p, c);
    int32_t xw = ut_max(0, pbx.x + pbx.w - x); // excess width
    int32_t max_w_sum = 0;
    if (xw > 0 && max_w_count > 0) {
        ui_view_for_each_begin(p, c) {
            if (c->type != ui_view_spacer && c->max_w > 0) {
                max_w_sum += ut_min(c->max_w, xw);
            }
        } ui_view_for_each_end(p, c);
    }
    if (xw > 0 && max_w_count > 0) {
        x = p->x + insets.left;
        int32_t k = 0;
        ui_view_for_each_begin(p, c) {
            ui_rect_t cbx; // child "out" box expanded by padding
            ui_ltrb_t padding;
            ui_view.outbox(c, &cbx, &padding);
            const int32_t p_lf = ui.gaps_em2px(c->fm->em.w, c->padding.left);
            const int32_t p_rt = ui.gaps_em2px(c->fm->em.w, c->padding.right);
            if (c->type == ui_view_spacer) {
                swear(p_lf == 0 && p_rt == 0);
            } else if (c->max_w > 0) {
                const int32_t max_w = ut_min(c->max_w, xw);
                int64_t proportional = ((int64_t)xw * (int64_t)max_w) / max_w_sum;
                assert(proportional <= INT32_MAX);
                int32_t cw = (int32_t)proportional;
                c->w = ut_min(c->max_w, c->w + cw);
                k++;
            }
            // TODO: take into account .align of a child and adjust x
            //       depending on ui.align.left/right/center
            //       distributing excess width on the left and right of a child
            c->x = p_lf + x;
            x = c->x + p_lf + c->w + p_rt;
        } ui_view_for_each_end(p, c);
        assert(k == max_w_count);
    }
    // excess width after max_w of non-spacers taken into account
    xw = ut_max(0, pbx.x + pbx.w - x);
    if (xw > 0 && spacers > 0) {
        // evenly distribute excess among spacers
        int32_t partial = xw / spacers;
        x = p->x + insets.left;
        ui_view_for_each_begin(p, c) {
            ui_rect_t cbx; // child "out" box expanded by padding
            ui_ltrb_t padding;
            ui_view.outbox(c, &cbx, &padding);
            const int32_t p_lf = ui.gaps_em2px(c->fm->em.w, c->padding.left);
            const int32_t p_rt = ui.gaps_em2px(c->fm->em.w, c->padding.right);
            if (c->type == ui_view_spacer) {
                c->y = top;
                c->w = partial;
                c->h = bot - top;
                spacers--;
            }
            c->x = x + p_lf;
//debugln(" %s %d,%d %dx%d", c->text, c->x, c->y, c->w, c->h);
            x = c->x + c->w + p_rt;
        } ui_view_for_each_end(p, c);
    }
//  traceln("<%s %d,%d %dx%d", p->text, p->x, p->y, p->w, p->h);
}

static void ui_list_measure(ui_view_t* p) {
    swear(p->type == ui_view_list, "type %4.4s 0x%08X", &p->type, p->type);
    const int32_t i_lf = ui.gaps_em2px(p->fm->em.w, p->insets.left);
    const int32_t i_rt = ui.gaps_em2px(p->fm->em.w, p->insets.right);
    const int32_t i_tp = ui.gaps_em2px(p->fm->em.h, p->insets.top);
    const int32_t i_bt = ui.gaps_em2px(p->fm->em.h, p->insets.bottom);
    int32_t max_h = i_tp;
    int32_t h = i_tp;
    int32_t w = 0;
    ui_view_for_each_begin(p, c) {
        swear(c->max_h == 0 || c->max_h >= c->h,
              "max_h: %d h: %d", c->max_h, c->h);
        if (c->type == ui_view_spacer) {
            c->padding = (ui_gaps_t){ 0, 0, 0, 0 };
            c->h = 0; // layout will distribute excess here
            max_h = ui.infinity; // spacer make height greedy
        } else {
            const int32_t p_tp = ui.gaps_em2px(c->fm->em.h, c->padding.top);
            const int32_t p_lf = ui.gaps_em2px(c->fm->em.w, c->padding.left);
            const int32_t p_bt = ui.gaps_em2px(c->fm->em.h, c->padding.bottom);
            const int32_t p_rt = ui.gaps_em2px(c->fm->em.w, c->padding.right);
            w = ut_max(w, p_lf + c->w + p_rt);
            const int32_t ch = p_tp + c->h + p_bt;
            if (c->max_h == ui.infinity) {
                max_h = ui.infinity;
            } else if (max_h < ui.infinity && c->max_h != 0) {
                swear(c->max_h >= ch, "Constraint violation: c->max_h < ch, "
                                      "max_h: %d, ch: %d", c->max_h, ch);
                max_h += c->max_h;
            } else if (max_h < ui.infinity) {
                swear(0 <= max_h + ch && max_h + ch < ui.infinity,
                      "Height overflow: max_h + ch = %d", max_h + ch);
                max_h += ch;
            }
            h += ch;
        }
    } ui_view_for_each_end(p, c);
    if (max_h < ui.infinity) {
        swear(0 <= max_h + i_bt && max_h + i_bt < ui.infinity,
             "Height overflow at bottom inset: max_h + bottom = %d",
              max_h + i_bt);
        max_h += i_bt;
    }
    h += i_bt;
    w += i_lf + i_rt;
    // Handle max height only if it differs from actual height
    p->max_h = max_h == h ? p->max_h : ut_max(max_h, p->max_h);
    // do not touch max_w, caller may have set it to something
    swear(max_h == 0 || max_h >= h, "max_h is less than actual height h");
    p->h = h;
    p->w = w;
    // add left and right insets
    p->h += ui.gaps_em2px(p->fm->em.h, p->insets.top);
    p->h += ui.gaps_em2px(p->fm->em.h, p->insets.bottom);
}

static void ui_list_layout(ui_view_t* p) {
//  debugln(">%s (%d,%d) %dx%d", p->text, p->x, p->y, p->w, p->h);
    swear(p->type == ui_view_list, "type %4.4s 0x%08X", &p->type, p->type);
    int32_t spacers = 0; // Number of spacers
    const int32_t i_tp = ui.gaps_em2px(p->fm->em.h, p->insets.top);
    const int32_t i_bt = ui.gaps_em2px(p->fm->em.h, p->insets.bottom);
    const int32_t i_lf = ui.gaps_em2px(p->fm->em.w, p->insets.left);
    const int32_t i_rt = ui.gaps_em2px(p->fm->em.w, p->insets.right);
    const int32_t top = p->y + i_tp;
    // Mitigation for vertical overflow:
    const int32_t bot = p->y + p->h - i_bt < top ? top + p->h : p->y + p->h - i_bt;
    const int32_t lf = p->x + i_lf;
    // Mitigation for horizontal overflow:
    const int32_t rt = p->x + p->w - i_rt < lf ? lf + p->h : p->x + p->w - i_rt;
    int32_t max_h_sum = 0;
    int32_t max_h_count = 0;
    int32_t y = top;
    ui_view_for_each_begin(p, c) {
        if (c->type == ui_view_spacer) {
            c->x = 0;
            c->y = 0;
            c->w = 0;
            c->h = 0;
            spacers++;
        } else {
            const int32_t p_lf = ui.gaps_em2px(c->fm->em.w, c->padding.left);
            const int32_t p_rt = ui.gaps_em2px(c->fm->em.w, c->padding.right);
            const int32_t p_tp = ui.gaps_em2px(c->fm->em.h, c->padding.top);
            const int32_t p_bt = ui.gaps_em2px(c->fm->em.h, c->padding.bottom);
            // setting child`s max_w to infinity means that child`s height is
            // *always* fill vertical view size of the parent
            // childs.w can exceed parent.w (horizontal overflow) - not encouraged but allowed
            if (c->max_w == ui.infinity) {
                c->w = ut_max(c->w, p->w - i_lf - i_rt - p_lf - p_rt);
            }
            if ((c->align & ui.align.left) != 0) {
                c->x = lf + p_lf;
            } else if ((c->align & ui.align.right) != 0) {
                c->x = rt - p_rt - c->w;
            } else {
                const int32_t cw = p_lf + c->w + p_rt;
                c->x = lf + p_lf + (rt - lf - cw) / 2;
            }
            c->y = y + p_tp;
            y = c->y + c->h + p_bt;
            swear(c->max_h == 0 || c->max_h > c->h, "max_h must be greater"
                "than current height: max_h: %d, h: %d", c->max_h, c->h);
            if (c->max_h > 0) {
                // clamp max_h to the effective parent height
                max_h_count++;
            }
        }
    } ui_view_for_each_end(p, c);
    int32_t xh = ut_max(0, bot - y); // excess height
    if (xh > 0 && max_h_count > 0) {
        ui_view_for_each_begin(p, c) {
            if (c->type != ui_view_spacer && c->max_h > 0) {
                max_h_sum += ut_min(c->max_h, xh);
            }
        } ui_view_for_each_end(p, c);
    }
    if (xh > 0 && max_h_count > 0) {
        y = top;
        int32_t k = 0;
        ui_view_for_each_begin(p, c) {
            const int32_t p_tp = ui.gaps_em2px(c->fm->em.h, c->padding.top);
            const int32_t p_bt = ui.gaps_em2px(c->fm->em.h, c->padding.bottom);
            if (c->type != ui_view_spacer && c->max_h > 0) {
                const int32_t max_h = ut_min(c->max_h, xh);
                int64_t proportional = ((int64_t)xh * (int64_t)max_h) / max_h_sum;
                assert(proportional <= INT32_MAX);
                int32_t ch = (int32_t)proportional;
                c->h = ut_min(c->max_h, c->h + ch);
                k++;
            }
            int32_t ch = p_tp + c->h + p_bt;
            c->y = y + p_tp;
            y += ch;
        } ui_view_for_each_end(p, c);
        assert(k == max_h_count);
    }
    // excess height after max_h of non-spacers taken into account
    xh = ut_max(0, bot - y); // excess height
    if (xh > 0 && spacers > 0) {
        // evenly distribute excess among spacers
        int32_t partial = xh / spacers;
        y = top;
        ui_view_for_each_begin(p, c) {
            const int32_t p_tp = ui.gaps_em2px(c->fm->em.h, c->padding.top);
            const int32_t p_bt = ui.gaps_em2px(c->fm->em.h, c->padding.bottom);
            if (c->type == ui_view_spacer) {
                c->x = lf;
                c->w = rt - lf;
                c->h = partial; // TODO: xxxxx last?
                spacers--;
            }
            int32_t ch = p_tp + c->h + p_bt;
            c->y = y + p_tp;
            y += ch;
        } ui_view_for_each_end(p, c);
    }
//  debugln("<%s (%d,%d) %dx%d", p->text, p->x, p->y, p->w, p->h);
}

static void ui_container_measure(ui_view_t* p) {
//  debugln(">%s %d,%d %dx%d", p->text, p->x, p->y, p->w, p->h);
    swear(p->type == ui_view_container, "type %4.4s 0x%08X", &p->type, p->type);
    const int32_t i_lf = ui.gaps_em2px(p->fm->em.w, p->insets.left);
    const int32_t i_rt = ui.gaps_em2px(p->fm->em.w, p->insets.right);
    const int32_t i_tp = ui.gaps_em2px(p->fm->em.h, p->insets.top);
    const int32_t i_bt = ui.gaps_em2px(p->fm->em.h, p->insets.bottom);
    // empty container minimum size:
    if (p != ui_app.view) {
        p->w = i_lf + i_rt;
        p->h = i_tp + i_bt;
    } else { // ui_app.view is special case (expanded to a window)
        p->w = ut_max(p->w, i_lf + i_rt);
        p->h = ut_max(p->h, i_tp + i_bt);
    }
    ui_view_for_each_begin(p, c) {
        const int32_t p_lf = ui.gaps_em2px(c->fm->em.w, c->padding.left);
        const int32_t p_rt = ui.gaps_em2px(c->fm->em.w, c->padding.right);
        const int32_t p_tp = ui.gaps_em2px(c->fm->em.h, c->padding.top);
        const int32_t p_bt = ui.gaps_em2px(c->fm->em.h, c->padding.bottom);
        p->w = ut_max(p->w, p_lf + c->w + p_rt);
        p->h = ut_max(p->h, p_tp + c->h + p_bt);
    } ui_view_for_each_end(p, c);
//  debugln("<%s %d,%d %dx%d", p->text, p->x, p->y, p->w, p->h);
}

static void ui_container_layout(ui_view_t* p) {
//  debugln(">%s %d,%d %dx%d", p->text, p->x, p->y, p->w, p->h);
    swear(p->type == ui_view_container, "type %4.4s 0x%08X", &p->type, p->type);
    const int32_t i_lf = ui.gaps_em2px(p->fm->em.w, p->insets.left);
    const int32_t i_rt = ui.gaps_em2px(p->fm->em.w, p->insets.right);
    const int32_t i_tp = ui.gaps_em2px(p->fm->em.h, p->insets.top);
    const int32_t i_bt = ui.gaps_em2px(p->fm->em.h, p->insets.bottom);
    const int32_t lf = p->x + i_lf;
    const int32_t rt = p->x + p->w - i_rt;
    const int32_t tp = p->y + i_tp;
    const int32_t bt = p->y + p->h - i_bt;
    ui_view_for_each_begin(p, c) {
        if (c->type != ui_view_spacer) {
            const int32_t p_lf = ui.gaps_em2px(c->fm->em.w, c->padding.left);
            const int32_t p_rt = ui.gaps_em2px(c->fm->em.w, c->padding.right);
            const int32_t p_tp = ui.gaps_em2px(c->fm->em.h, c->padding.top);
            const int32_t p_bt = ui.gaps_em2px(c->fm->em.h, c->padding.bottom);

            const int32_t pw = p->w - i_lf - i_rt - p_lf - p_rt;
            const int32_t ph = p->h - i_tp - i_bt - p_tp - p_bt;
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
                   "Constraint violation align: left|right 0x%02X", c->align);
            swear((c->align & (ui.align.top|ui.align.bottom)) !=
                               (ui.align.top|ui.align.bottom),
                   "Constraint violation align: top|bottom 0x%02X", c->align);
            if ((c->align & ui.align.left) != 0) {
                c->x = lf + p_lf;
            } else if ((c->align & ui.align.right) != 0) {
                c->x = rt - c->w - p_rt;
            } else {
                const int32_t w = rt - lf; // effective width
                c->x = lf + p_lf + (w - (p_lf + c->w + p_rt)) / 2;
            }
            if ((c->align & ui.align.top) != 0) {
                c->y = tp + p_tp;
            } else if ((c->align & ui.align.bottom) != 0) {
                c->y = bt - c->h - p_bt;
            } else {
                const int32_t h = bt - tp; // effective height
                c->y = tp + p_tp + (h - (p_tp + c->h + p_bt)) / 2;
            }
//          debugln(" %s %d,%d %dx%d", c->text, c->x, c->y, c->w, c->h);
        }
    } ui_view_for_each_end(p, c);
//  debugln("<%s %d,%d %dx%d", p->text, p->x, p->y, p->w, p->h);
}

static void ui_container_paint(ui_view_t* v) {
    if (!ui_color_is_undefined(v->background) &&
        !ui_color_is_transparent(v->background)) {
//      traceln("%s [%d] 0x%016llX", v->text, v->background_id, v->background);
        ui_gdi.fill_with(v->x, v->y, v->w, v->h, v->background);
    } else {
//      traceln("%s undefined", v->text);
    }
}

void ui_view_init_span(ui_view_t* v) {
    swear(v->type == ui_view_span, "type %4.4s 0x%08X", &v->type, v->type);
    ui_view_init_container(v);
    v->measure = ui_span_measure;
    v->layout  = ui_span_layout;
    if (v->text[0] == 0) { strprintf(v->text, "ui_span"); }
}

void ui_view_init_list(ui_view_t* v) {
    swear(v->type == ui_view_list, "type %4.4s 0x%08X", &v->type, v->type);
    ui_view_init_container(v);
    // TODO: not sure about default insets
    v->insets  = (ui_gaps_t){ .left = 0.5, .top = 0.25, .right = 0.5, .bottom = 0.25 };
    v->measure = ui_list_measure;
    v->layout  = ui_list_layout;
    if (v->text[0] == 0) { strprintf(v->text, "ui_list"); }
}

void ui_view_init_spacer(ui_view_t* v) {
    swear(v->type == ui_view_spacer, "type %4.4s 0x%08X", &v->type, v->type);
    ui_view_init(v);
    v->w = 0;
    v->h = 0;
    v->max_w = ui.infinity;
    v->max_h = ui.infinity;
    if (v->text[0] == 0) { strprintf(v->text, "ui_spacer"); }
}

void ui_view_init_container(ui_view_t* v) {
    ui_view_init(v);
    v->background = ui_color_transparent;
    v->insets  = (ui_gaps_t){ .left  = 0.25, .top    = 0.25,
                              .right = 0.25, .bottom = 0.25 };
    // do not overwrite if already set
    if (v->measure == null) { v->measure = ui_container_measure; }
    if (v->layout  == null) { v->layout  = ui_container_layout; }
    if (v->paint   == null) { v->paint   = ui_container_paint; }
    if (v->text[0] == 0) { strprintf(v->text, "ui_container"); }
}

#pragma pop_macro("dump")
#pragma pop_macro("debugln")
