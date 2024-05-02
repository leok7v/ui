/* Copyright (c) Dmitry "Leo" Kuznetsov 2021-24 see LICENSE for details */
#include "ut/ut.h"
#include "ui/ui.h"

static bool ui_stack_debug = false;

#define debugln(...) do {                           \
    if (ui_stack_debug) {  traceln(__VA_ARGS__); }  \
} while (0)

static int32_t ui_stack_em2px(int32_t em, fp32_t ratio) {
    return em == 0 ? 0 : (int32_t)(em * ratio + 0.5f);
}

static void ui_h_stack_measure(ui_view_t* s) {
    swear(s->type == ui_view_h_stack, "type %4.4s 0x%08X", &s->type, s->type);
    int32_t left  = ui_stack_em2px(s->em.x, s->insets.left);
    int32_t max_w = left;
    int32_t w = left;
    int32_t h = 0;
    ui_view_for_each(s, c, {
        debugln("%s[%4.4s 0x%08X] .(x,y): (%d,%d) .w: %d max_w: %d (0x%08X)",
                c->text, &c->type, c->type, c->x, c->y, c->w, max_w, max_w);
        assert(c->max_w == 0 || c->max_w >= c->w,
               "max_w: %d w: %d", c->max_w, c->w);
        if (c->type == ui_view_spacer) {
            c->w = 0; // layout will distribute excess here
            max_w = INT32_MAX; // spacer make width greedy
            debugln("spacer");
        } else {
            const int32_t lf = ui_stack_em2px(c->em.x, c->padding.left);
            const int32_t tp = ui_stack_em2px(c->em.y, c->padding.top);
            const int32_t rt = ui_stack_em2px(c->em.x, c->padding.right);
            const int32_t bt = ui_stack_em2px(c->em.y, c->padding.bottom);
            h = ut_max(h, tp + c->h + bt);
            const int32_t cw = lf + c->w + rt;
            if (c->max_w == INT32_MAX) {
                max_w = INT32_MAX;
            } else if (max_w < INT32_MAX && c->max_w != 0) {
                assert(c->max_w >= cw);
                max_w += c->max_w;
            } else if (max_w < INT32_MAX) {
                assert(0 <= max_w + cw && max_w + cw < INT32_MAX);
                max_w += cw;
            }
            w += cw;
        }
    });
    int32_t right = ui_stack_em2px(s->em.x, s->insets.right);
    if (max_w < INT32_MAX) {
        assert(0 <= max_w + right && max_w + right < INT32_MAX);
        max_w += right;
    }
    w += right;
    s->max_w = max_w == w ? 0 : max_w;
    // do not touch max_h, caller may have set it to something
    assert(max_w == 0 || max_w >= w);
    s->w = w;
    s->h = h;
    // add top and bottom insets
    s->h += ui_stack_em2px(s->em.y, s->insets.top);
    s->h += ui_stack_em2px(s->em.y, s->insets.bottom);
}

// after measure of the subtree is concluded the parent h_stack
// may adjust h_stack_w wider number depending on it's own width
// and h_stack.max_w agreement

static void ui_h_stack_layout(ui_view_t* s) {
    swear(s->type == ui_view_h_stack, "type %4.4s 0x%08X", &s->type, s->type);
    if (s->parent != null && s->max_w >= s->parent->w) {
        s->w = s->parent->w;
    }
    int32_t spacers = 0; // number of spacers
    // left and right insets
    const int32_t i_lf = ui_stack_em2px(s->em.x, s->insets.left);
    const int32_t i_rt = ui_stack_em2px(s->em.x, s->insets.right);
    // top and bottom insets
    const int32_t i_tp = ui_stack_em2px(s->em.y, s->insets.top);
    const int32_t i_bt = ui_stack_em2px(s->em.y, s->insets.bottom);
    const int32_t lf = s->x +        i_lf;
    const int32_t rt = s->x + s->w - i_rt;
    assert(lf < rt, "insets left: %d right: %d x: %d w: %d left: %d right: %d",
                    i_lf, i_rt, s->x, s->w, lf, rt);
    // top and bottom y
    const int32_t top = s->y +        i_tp;
    const int32_t bot = s->y + s->h - i_bt;
    assert(top < bot, "insets top: %d bottom: %d y: %d h: %d top: %d bottom: %d",
                      i_tp, i_bt, s->y, s->w, top, bot);
    int32_t x = i_lf;
    ui_view_for_each(s, c, {
        assert(c->h <= s->h);
        if (c->type == ui_view_spacer) {
            spacers++;
        } else {
            const int32_t p_lf = ui_stack_em2px(c->em.x, c->padding.left);
            const int32_t p_tp = ui_stack_em2px(c->em.y, c->padding.top);
            const int32_t p_rt = ui_stack_em2px(c->em.x, c->padding.right);
            const int32_t p_bt = ui_stack_em2px(c->em.y, c->padding.bottom);
            if (c->align == ui.align.top) {
                c->y = top + p_tp;
            } else if (c->align == ui.align.bottom) {
                c->y = bot - (c->h + p_bt);
            } else {
                swear(c->align == ui.align.center);
                // y = top + p_tp + c->h + p_bt == bot
                // top + p_tp + c->h + p_bt == bot
                const int32_t d = s->h - (p_tp + c->h + p_bt);
                c->y = i_tp + (s->h - (p_tp + c->h + p_bt)) / 2;
                debugln("i_tp:%d + (s->h:%d - (p_tp:%d + c->h:%d + p_bt:%d))/2: %d y: %d",
                         i_tp,      s->h,      p_tp,     c->h,     p_bt,         d/2, c->y);
            }
            c->x = x + p_lf;
            debugln("%s.(x,y): (%d,%d) .w: %d", c->text, c->x, c->y, c->w);
            x = c->x + c->w + p_rt;
        }
    });
    if (x < rt) {
        // evenly distribute excess among spacers
        int32_t sum = 0;
        int32_t diff = rt - x;
        int32_t spacer_w = diff / spacers;
        x = i_lf;
        ui_view_for_each(s, c, {
            if (c->type == ui_view_spacer) {
                c->w = spacers == 1 ? spacer_w : diff - sum;
                sum += c->w;
                spacers--;
            }
            const int32_t p_lf = ui_stack_em2px(c->em.x, c->padding.left);
            const int32_t p_rt = ui_stack_em2px(c->em.x, c->padding.right);
            int32_t cw = p_lf + c->w + p_rt;
            debugln("%s[%4.4s 0x%08X] .(x,y): (%d:=%d,%d) .(WxH): %d=%d+%d+%d x %d max_w: %d (0x%08X)",
                    c->text, &c->type, c->type, c->x, x, c->y, cw, p_lf, c->w, p_rt, c->h, c->max_w, c->max_w);
            c->x = x;
            x += cw;
        });
    }
    debugln("%s[%4.4s 0x%08X] .(x,y): (%d,%d) .(WxH): %dx%d max_w: %d (0x%08X)",
            s->text, &s->type, s->type, s->x, s->y, s->w, s->h, s->max_w, s->max_w);
}

void ui_view_init_h_stack(ui_view_t* v) {
    ui_view_init_container(v);
    // TODO: not sure about default insets
    v->insets  = (ui_gaps_t){ .left = 0.5, .top = 0.25, .right = 0.5, .bottom = 0.25 };
    v->measure = ui_h_stack_measure;
    v->layout  = ui_h_stack_layout;
}

static void ui_v_stack_measure(ui_view_t* v) {
    swear(v->type == ui_view_v_stack, "type %4.4s 0x%08X", &v->type, v->type);
    debugln("TODO: implement");
}

static void ui_v_stack_layout(ui_view_t* v) {
    swear(v->type == ui_view_v_stack, "type %4.4s 0x%08X", &v->type, v->type);
    debugln("TODO: implement");
}

void ui_view_init_v_stack(ui_view_t* v) {
    ui_view_init_container(v);
    // TODO: not sure about default insets
    v->insets  = (ui_gaps_t){ .left = 0.5, .top = 0.25, .right = 0.5, .bottom = 0.25 };
    v->measure = ui_v_stack_measure;
    v->layout  = ui_v_stack_layout;
}

void ui_view_init_spacer(ui_view_t* v) {
    swear(v->type == ui_view_spacer, "type %4.4s 0x%08X", &v->type, v->type);
    ui_view_init(v);
    v->w = 0;
    v->h = 0;
    v->max_w = INT32_MAX;
    v->max_h = INT32_MAX;
}
