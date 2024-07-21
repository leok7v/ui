/* Copyright (c) Dmitry "Leo" Kuznetsov 2021-24 see LICENSE for details */
#include "ut/ut.h"
#include "ui/ui.h"

static bool ui_containers_debug;

#pragma push_macro("debugln")
#pragma push_macro("ui_layout_dump")
#pragma push_macro("ui_layout_enter")
#pragma push_macro("ui_layout_exit")

// Usage of: ui_view_for_each_begin(p, c) { ... } ui_view_for_each_end(p, c)
// makes code inside iterator debugger friendly and ensures correct __LINE__

#define debugln(...) do {                                \
    if (ui_containers_debug) {  ut_println(__VA_ARGS__); }  \
} while (0)

static int32_t ui_layout_nesting;

#define ui_layout_enter(v) do {                                  \
    ui_ltrb_t i_ = ui_view.margins(v, &v->insets);               \
    ui_ltrb_t p_ = ui_view.margins(v, &v->padding);              \
    debugln("%*c> %d,%d %dx%d p: %d %d %d %d  i: %d %d %d %d %s",\
            ui_layout_nesting, 0x20,                             \
            v->x, v->y, v->w, v->h,                              \
            p_.left, p_.top, p_.right, p_.bottom,                \
            i_.left, i_.top, i_.right, i_.bottom,                \
            ui_view_debug_id(v));                                \
    ui_layout_nesting += 4;                                      \
} while (0)

#define ui_layout_exit(v) do {                                   \
    ui_layout_nesting -= 4;                                      \
    debugln("%*c< %d,%d %dx%d %s",                               \
            ui_layout_nesting, 0x20,                             \
            v->x, v->y, v->w, v->h, ui_view_debug_id(v));        \
} while (0)

#define ui_layout_clild(v) do {                                  \
    debugln("%*c %d,%d %dx%d %s", ui_layout_nesting, 0x20,       \
            c->x, c->y, c->w, c->h, ui_view_debug_id(v));        \
} while (0)

static const char* ui_stack_finite_int(int32_t v, char* text, int32_t count) {
    ut_swear(v >= 0);
    if (v == ui.infinity) {
        ut_str.format(text, count, "%s", ut_glyph_infinity);
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
        ui_view_debug_id(v),                                                  \
        &v->type, v->x, v->y, v->w, v->h,                                     \
        ui_stack_finite_int(v->max_w, maxw, ut_countof(maxw)),               \
        ui_stack_finite_int(v->max_h, maxh, ut_countof(maxh)),               \
        v->padding.left, v->padding.top, v->padding.right, v->padding.bottom, \
        v->insets.left, v->insets.top, v->insets.right, v->insets.bottom,     \
        v->align);                                                            \
} while (0)

static void ui_span_measure(ui_view_t* p) {
    ui_layout_enter(p);
    ut_swear(p->type == ui_view_span, "type %4.4s 0x%08X", &p->type, p->type);
    ui_ltrb_t insets;
    ui_view.inbox(p, null, &insets);
    int32_t w = insets.left;
    int32_t h = 0;
    int32_t max_w = w;
    ui_view_for_each_begin(p, c) {
        ut_swear(c->max_w == 0 || c->max_w >= c->w,
              "max_w: %d w: %d", c->max_w, c->w);
        if (ui_view.is_hidden(c)) {
            // nothing
        } else if (c->type == ui_view_spacer) {
            c->padding = (ui_margins_t){ 0, 0, 0, 0 };
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
                ut_swear(c->max_w >= c->w, "c->max_w %d < c->w %d ",
                      c->max_w, c->w);
                max_w += c->max_w;
            } else if (max_w < ui.infinity) {
                ut_swear(0 <= max_w + cbx.w &&
                      (int64_t)max_w + (int64_t)cbx.w < (int64_t)ui.infinity,
                      "max_w:%d + cbx.w:%d = %d", max_w, cbx.w, max_w + cbx.w);
                max_w += cbx.w;
            }
            w += cbx.w;
        }
        ui_layout_clild(c);
    } ui_view_for_each_end(p, c);
    if (0 < max_w && max_w < ui.infinity) {
        ut_swear(0 <= max_w + insets.right &&
              (int64_t)max_w + (int64_t)insets.right < (int64_t)ui.infinity,
             "max_w:%d + right:%d = %d", max_w, insets.right, max_w + insets.right);
        max_w += insets.right;
    }
    ut_swear(max_w == 0 || max_w >= w, "max_w: %d w: %d", max_w, w);
    if (ui_view.is_hidden(p)) {
        p->w = 0;
        p->h = 0;
    } else {
        p->w = w + insets.right;
        p->h = insets.top + h + insets.bottom;
        ut_swear(p->max_w == 0 || p->max_w >= p->w,
              "max_w: %d is less than actual width: %d", p->max_w, p->w);
    }
    ui_layout_exit(p);
}

// after measure of the subtree is concluded the parent ui_span
// may adjust span_w wider number depending on it's own width
// and ui_span.max_w agreement

static int32_t ui_span_place_child(ui_view_t* c, ui_rect_t pbx, int32_t x) {
    ui_ltrb_t padding = ui_view.margins(c, &c->padding);
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
        ut_assert(c->align == ui.align.top);
        c->y = min_y;
    } else if ((c->align & ui.align.bottom) != 0) {
        ut_assert(c->align == ui.align.bottom);
        c->y = ut_max(min_y, pbx.y + pbx.h - c->h - padding.bottom);
    } else { // effective height (c->h might have been changed)
        ut_assert(c->align == ui.align.center,
                  "only top, center, bottom alignment for span");
        const int32_t ch = padding.top + c->h + padding.bottom;
        c->y = ut_max(min_y, pbx.y + (pbx.h - ch) / 2 + padding.top);
    }
    c->x = x + padding.left;
    return c->x + c->w + padding.right;
}

static void ui_span_layout(ui_view_t* p) {
    ui_layout_enter(p);
    ut_swear(p->type == ui_view_span, "type %4.4s 0x%08X", &p->type, p->type);
    ui_rect_t pbx; // parent "in" box (sans insets)
    ui_ltrb_t insets;
    ui_view.inbox(p, &pbx, &insets);
    int32_t spacers = 0; // Number of spacers
    int32_t max_w_count = 0;
    int32_t x = p->x + insets.left;
    ui_view_for_each_begin(p, c) {
        if (!ui_view.is_hidden(c)) {
            if (c->type == ui_view_spacer) {
                c->x = x;
                c->y = pbx.y;
                c->h = pbx.h;
                c->w = 0;
                spacers++;
            } else {
                x = ui_span_place_child(c, pbx, x);
                ut_swear(c->max_w == 0 || c->max_w >= c->w,
                      "max_w:%d < w:%d", c->max_w, c->w);
                if (c->max_w > 0) {
                    max_w_count++;
                }
            }
            ui_layout_clild(c);
        }
    } ui_view_for_each_end(p, c);
    int32_t xw = ut_max(0, pbx.x + pbx.w - x); // excess width
    int32_t max_w_sum = 0;
    if (xw > 0 && max_w_count > 0) {
        ui_view_for_each_begin(p, c) {
            if (!ui_view.is_hidden(c) && c->type != ui_view_spacer &&
                 c->max_w > 0) {
                max_w_sum += ut_min(c->max_w, xw);
                ui_layout_clild(c);
            }
        } ui_view_for_each_end(p, c);
    }
    if (xw > 0 && max_w_count > 0) {
        x = p->x + insets.left;
        int32_t k = 0;
        ui_view_for_each_begin(p, c) {
            if (!ui_view.is_hidden(c)) {
                ui_rect_t cbx; // child "out" box expanded by padding
                ui_ltrb_t padding;
                ui_view.outbox(c, &cbx, &padding);
                if (c->type == ui_view_spacer) {
                    ut_swear(padding.left == 0 && padding.right == 0);
                } else if (c->max_w > 0) {
                    const int32_t max_w = ut_min(c->max_w, xw);
                    int64_t proportional = (xw * (int64_t)max_w) / max_w_sum;
                    ut_assert(proportional <= (int64_t)INT32_MAX);
                    int32_t cw = (int32_t)proportional;
                    c->w = ut_min(c->max_w, c->w + cw);
                    k++;
                }
                // TODO: take into account .align of a child and adjust x
                //       depending on ui.align.left/right/center
                //       distributing excess width on the left and right of a child
                c->x = padding.left + x;
                x = c->x + padding.left + c->w + padding.right;
                ui_layout_clild(c);
            }
        } ui_view_for_each_end(p, c);
        ut_swear(k == max_w_count);
    }
    // excess width after max_w of non-spacers taken into account
    xw = ut_max(0, pbx.x + pbx.w - x);
    if (xw > 0 && spacers > 0) {
        // evenly distribute excess among spacers
        int32_t partial = xw / spacers;
        x = p->x + insets.left;
        ui_view_for_each_begin(p, c) {
            if (!ui_view.is_hidden(c)) {
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
                ui_layout_clild(c);
            }
        } ui_view_for_each_end(p, c);
    }
    ui_layout_exit(p);
}

static void ui_list_measure(ui_view_t* p) {
    ui_layout_enter(p);
    ut_swear(p->type == ui_view_list, "type %4.4s 0x%08X", &p->type, p->type);
    ui_rect_t pbx; // parent "in" box (sans insets)
    ui_ltrb_t insets;
    ui_view.inbox(p, &pbx, &insets);
    int32_t max_h = insets.top;
    int32_t h = insets.top;
    int32_t w = 0;
    ui_view_for_each_begin(p, c) {
        ut_swear(c->max_h == 0 || c->max_h >= c->h, "max_h: %d h: %d",
              c->max_h, c->h);
        if (!ui_view.is_hidden(c)) {
            if (c->type == ui_view_spacer) {
                c->padding = (ui_margins_t){ 0, 0, 0, 0 };
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
                    ut_swear(c->max_h >= c->h, "c->max_h:%d < c->h: %d",
                          c->max_h, c->h);
                    max_h += c->max_h;
                } else if (max_h < ui.infinity) {
                    ut_swear(0 <= max_h + cbx.h &&
                          (int64_t)max_h + (int64_t)cbx.h < (int64_t)ui.infinity,
                          "max_h:%d + ch:%d = %d", max_h, cbx.h, max_h + cbx.h);
                    max_h += cbx.h;
                }
                h += cbx.h;
            }
            ui_layout_clild(c);
        }
    } ui_view_for_each_end(p, c);
    if (max_h < ui.infinity) {
        ut_swear(0 <= max_h + insets.bottom &&
              (int64_t)max_h + (int64_t)insets.bottom < (int64_t)ui.infinity,
             "max_h:%d + bottom:%d = %d",
              max_h, insets.bottom, max_h + insets.bottom);
        max_h += insets.bottom;
    }
    if (ui_view.is_hidden(p)) {
        p->w = 0;
        p->h = 0;
    } else if (p == ui_app.root) {
        // ui_app.root is special occupying whole window client rectangle
        // sans borders and caption thus it should not be re-measured
    } else {
        p->h = h + insets.bottom;
        p->w = insets.left + w + insets.right;
    }
    ui_layout_exit(p);
}

static int32_t ui_list_place_child(ui_view_t* c, ui_rect_t pbx, int32_t y) {
    ui_ltrb_t padding = ui_view.margins(c, &c->padding);
    // setting child`s max_w to infinity means that child`s height is
    // *always* fill vertical view size of the parent
    // childs.w can exceed parent.w (horizontal overflow) - not encouraged but allowed
    if (c->max_w == ui.infinity) {
        c->w = ut_max(c->w, pbx.w - padding.left - padding.right);
    }
    int32_t min_x = pbx.x + padding.left;
    if ((c->align & ui.align.left) != 0) {
        ut_assert(c->align == ui.align.left);
        c->x = min_x;
    } else if ((c->align & ui.align.right) != 0) {
        ut_assert(c->align == ui.align.right);
        c->x = ut_max(min_x, pbx.x + pbx.w - c->w - padding.right);
    } else {
        ut_assert(c->align == ui.align.center,
                  "only left, center, right, alignment for list");
        const int32_t cw = padding.left + c->w + padding.right;
        c->x = ut_max(min_x, pbx.x + (pbx.w - cw) / 2 + padding.left);
    }
    c->y = y + padding.top;
    return c->y + c->h + padding.bottom;
}

static void ui_list_layout(ui_view_t* p) {
    ui_layout_enter(p);
    ut_swear(p->type == ui_view_list, "type %4.4s 0x%08X", &p->type, p->type);
    ui_rect_t pbx; // parent "in" box (sans insets)
    ui_ltrb_t insets;
    ui_view.inbox(p, &pbx, &insets);
    int32_t spacers = 0; // Number of spacers
    int32_t max_h_sum = 0;
    int32_t max_h_count = 0;
    int32_t y = pbx.y;
    ui_view_for_each_begin(p, c) {
        if (ui_view.is_hidden(c)) {
            // nothing
        } else if (c->type == ui_view_spacer) {
            c->x = pbx.x;
            c->y = y;
            c->w = pbx.w;
            c->h = 0;
            spacers++;
        } else {
            y = ui_list_place_child(c, pbx, y);
            ut_swear(c->max_h == 0 || c->max_h >= c->h,
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
            if (!ui_view.is_hidden(c) && c->type != ui_view_spacer &&
                 c->max_h > 0) {
                max_h_sum += ut_min(c->max_h, xh);
            }
        } ui_view_for_each_end(p, c);
    }
    if (xh > 0 && max_h_count > 0) {
        y = pbx.y;
        int32_t k = 0;
        ui_view_for_each_begin(p, c) {
            if (!ui_view.is_hidden(c)) {
                ui_rect_t cbx; // child "out" box expanded by padding
                ui_ltrb_t padding;
                ui_view.outbox(c, &cbx, &padding);
                if (c->type != ui_view_spacer && c->max_h > 0) {
                    const int32_t max_h = ut_min(c->max_h, xh);
                    int64_t proportional = (xh * (int64_t)max_h) / max_h_sum;
                    ut_assert(proportional <= (int64_t)INT32_MAX);
                    int32_t ch = (int32_t)proportional;
                    c->h = ut_min(c->max_h, c->h + ch);
                    k++;
                }
                int32_t ch = padding.top + c->h + padding.bottom;
                c->y = y + padding.top;
                y += ch;
                ui_layout_clild(c);
            }
        } ui_view_for_each_end(p, c);
        ut_swear(k == max_h_count);
    }
    // excess height after max_h of non-spacers taken into account
    xh = ut_max(0, pbx.y + pbx.h - y); // excess height
    if (xh > 0 && spacers > 0) {
        // evenly distribute excess among spacers
        int32_t partial = xh / spacers;
        y = pbx.y;
        ui_view_for_each_begin(p, c) {
            if (!ui_view.is_hidden(c)) {
                ui_rect_t cbx; // child "out" box expanded by padding
                ui_ltrb_t padding;
                ui_view.outbox(c, &cbx, &padding);
                if (c->type == ui_view_spacer) {
                    c->x = pbx.x;
                    c->w = pbx.x + pbx.w - pbx.x;
                    c->h = partial; // TODO: last?
                    spacers--;
                }
                int32_t ch = padding.top + c->h + padding.bottom;
                c->y = y + padding.top;
                y += ch;
                ui_layout_clild(c);
            }
        } ui_view_for_each_end(p, c);
    }
    ui_layout_exit(p);
}

static void ui_stack_child_3x3(ui_view_t* c, int32_t *row, int32_t *col) {
    *row = 0; *col = 0; // makes code analysis happier
    if (c->align == (ui.align.left|ui.align.top)) {
        *row = 0; *col = 0;
    } else if (c->align == ui.align.top) {
        *row = 0; *col = 1;
    } else if (c->align == (ui.align.right|ui.align.top)) {
        *row = 0; *col = 2;
    } else if (c->align == ui.align.left) {
        *row = 1; *col = 0;
    } else if (c->align == ui.align.center) {
        *row = 1; *col = 1;
    } else if (c->align == ui.align.right) {
        *row = 1; *col = 2;
    } else if (c->align == (ui.align.left|ui.align.bottom)) {
        *row = 2; *col = 0;
    } else if (c->align == ui.align.bottom) {
        *row = 2; *col = 1;
    } else if (c->align == (ui.align.right|ui.align.bottom)) {
        *row = 2; *col = 2;
    } else {
        ut_swear(false, "invalid child align: 0x%02X", c->align);
    }
}

static void ui_stack_measure(ui_view_t* p) {
    ui_layout_enter(p);
    ut_swear(p->type == ui_view_stack, "type %4.4s 0x%08X", &p->type, p->type);
    ui_rect_t pbx; // parent "in" box (sans insets)
    ui_ltrb_t insets;
    ui_view.inbox(p, &pbx, &insets);
    ui_wh_t sides[3][3] = { {0, 0} };
    ui_view_for_each_begin(p, c) {
        if (!ui_view.is_hidden(c)) {
            ui_rect_t cbx; // child "out" box expanded by padding
            ui_ltrb_t padding;
            ui_view.outbox(c, &cbx, &padding);
            int32_t row = 0;
            int32_t col = 0;
            ui_stack_child_3x3(c, &row, &col);
            sides[row][col].w = ut_max(sides[row][col].w, cbx.w);
            sides[row][col].h = ut_max(sides[row][col].h, cbx.h);
            ui_layout_clild(c);
        }
    } ui_view_for_each_end(p, c);
    if (ui_containers_debug) {
        for (int32_t r = 0; r < ut_countof(sides); r++) {
            char text[1024];
            text[0] = 0;
            for (int32_t c = 0; c < ut_countof(sides[r]); c++) {
                char line[128];
                ut_str_printf(line, " %4dx%-4d", sides[r][c].w, sides[r][c].h);
                strcat(text, line);
            }
            debugln("%*c sides[%d] %s", ui_layout_nesting, 0x20, r, text);
        }
    }
    ui_wh_t wh = {0, 0};
    for (int32_t r = 0; r < 3; r++) {
        int32_t sum_w = 0;
        for (int32_t c = 0; c < 3; c++) {
            sum_w += sides[r][c].w;
        }
        wh.w = ut_max(wh.w, sum_w);
    }
    for (int32_t c = 0; c < 3; c++) {
        int32_t sum_h = 0;
        for (int32_t r = 0; r < 3; r++) {
            sum_h += sides[r][c].h;
        }
        wh.h = ut_max(wh.h, sum_h);
    }
    debugln("%*c wh %dx%d", ui_layout_nesting, 0x20, wh.w, wh.h);
    p->w = insets.left + wh.w + insets.right;
    p->h = insets.top  + wh.h + insets.bottom;
    ui_layout_exit(p);
}

static void ui_stack_layout(ui_view_t* p) {
    ui_layout_enter(p);
    ut_swear(p->type == ui_view_stack, "type %4.4s 0x%08X", &p->type, p->type);
    ui_rect_t pbx; // parent "in" box (sans insets)
    ui_ltrb_t insets;
    ui_view.inbox(p, &pbx, &insets);
    ui_view_for_each_begin(p, c) {
        if (c->type != ui_view_spacer && !ui_view.is_hidden(c)) {
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
            ut_swear((c->align & (ui.align.left|ui.align.right)) !=
                               (ui.align.left|ui.align.right),
                   "align: left|right 0x%02X", c->align);
            ut_swear((c->align & (ui.align.top|ui.align.bottom)) !=
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
                c->y = ut_max(min_y, pbx.y + pbx.h - c->h - padding.bottom);
            } else {
                c->y = ut_max(min_y, min_y + (pbx.h - (padding.top + c->h + padding.bottom)) / 2);
            }
            ui_layout_clild(c);
        }
    } ui_view_for_each_end(p, c);
    ui_layout_exit(p);
}

static void ui_container_paint(ui_view_t* v) {
    if (!ui_color_is_undefined(v->background) &&
        !ui_color_is_transparent(v->background)) {
        ui_gdi.fill(v->x, v->y, v->w, v->h, v->background);
    } else {
//      ut_println("%s undefined", ui_view_debug_id(v));
    }
}

static void ui_view_container_init(ui_view_t* v) {
    v->background = ui_colors.transparent;
    v->insets  = (ui_margins_t){
       .left  = 0.25, .top    = 0.125,
        .right = 0.25, .bottom = 0.125
//      .left  = 0.25, .top    = 0.0625,  // TODO: why?
//      .right = 0.25, .bottom = 0.1875
    };
}

void ui_view_init_span(ui_view_t* v) {
    ut_swear(v->type == ui_view_span, "type %4.4s 0x%08X", &v->type, v->type);
    ui_view_container_init(v);
    if (v->measure == null) { v->measure = ui_span_measure; }
    if (v->layout  == null) { v->layout  = ui_span_layout; }
    if (v->paint   == null) { v->paint   = ui_container_paint; }
    if (ui_view.string(v)[0] == 0) { ui_view.set_text(v, "ui_span"); }
    if (v->debug.id == null) { v->debug.id = "#ui_span"; }
}

void ui_view_init_list(ui_view_t* v) {
    ut_swear(v->type == ui_view_list, "type %4.4s 0x%08X", &v->type, v->type);
    ui_view_container_init(v);
    if (v->measure == null) { v->measure = ui_list_measure; }
    if (v->layout  == null) { v->layout  = ui_list_layout; }
    if (v->paint   == null) { v->paint   = ui_container_paint; }
    if (ui_view.string(v)[0] == 0) { ui_view.set_text(v, "ui_list"); }
    if (v->debug.id == null) { v->debug.id = "#ui_list"; }
}

void ui_view_init_spacer(ui_view_t* v) {
    ut_swear(v->type == ui_view_spacer, "type %4.4s 0x%08X", &v->type, v->type);
    v->w = 0;
    v->h = 0;
    v->max_w = ui.infinity;
    v->max_h = ui.infinity;
    if (ui_view.string(v)[0] == 0) { ui_view.set_text(v, "ui_spacer"); }
    if (v->debug.id == null) { v->debug.id = "#ui_spacer"; }

}

void ui_view_init_stack(ui_view_t* v) {
    ui_view_container_init(v);
    if (v->measure == null) { v->measure = ui_stack_measure; }
    if (v->layout  == null) { v->layout  = ui_stack_layout; }
    if (v->paint   == null) { v->paint   = ui_container_paint; }
    if (ui_view.string(v)[0] == 0) { ui_view.set_text(v, "ui_stack"); }
    if (v->debug.id == null) { v->debug.id = "#ui_stack"; }
}

#pragma pop_macro("ui_layout_exit")
#pragma pop_macro("ui_layout_enter")
#pragma pop_macro("ui_layout_dump")
#pragma pop_macro("debugln")
