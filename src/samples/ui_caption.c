/* Copyright (c) Dmitry "Leo" Kuznetsov 2021-24 see LICENSE for details */
#include "ut/ut.h"
#include "ui/ui.h"
#include "ui_caption.h"


static void ui_caption_mouse(ui_view_t* v, int32_t m, int64_t flags) {
    swear(v == &ui_caption.view, "window dragging only by caption");
    ui_caption_t* c = (ui_caption_t*)v;
    bool resizing = c->resizing.x != 0 || c->resizing.y != 0;
    if (!resizing) {
        bool dragging = c->dragging.x != 0 || c->dragging.y != 0;
        bool pressed =
            m == ui.message.left_button_pressed ||
            m == ui.message.right_button_pressed;
        bool released =
            m == ui.message.left_button_released ||
            m == ui.message.right_button_released;
        bool holding = flags & (ui.mouse.button.left|ui.mouse.button.left);
        if (m == ui.message.mouse_move && !holding) {
            released = true;
        }
        if (ui_view.inside(v, &app.mouse)) {
            if (pressed && !dragging) {
                c->dragging = app.mouse;
                app.capture_mouse(true);
            } else if (dragging && released) {
                c->dragging = (ui_point_t){0, 0};
                app.capture_mouse(false);
                dragging = false;
            }
        }
        if (m == ui.message.mouse_move && dragging && holding) {
            const int32_t dx = app.mouse.x - c->dragging.x;
            const int32_t dy = app.mouse.y - c->dragging.y;
    //      traceln("%d,%d", dx, dy);
            ui_rect_t r = app.wrc;
            r.x += dx;
            r.y += dy;
            app.move_and_resize(&r);
        }
    }
}

static void ui_content_view_mouse(ui_view_t* v, int32_t m, int64_t flags) {
    ui_caption_t* c = (ui_caption_t*)v;
    swear(v == app.view);
    static int64_t ht = 1; /* hit test result: ui.hit_test.client */
    assert(ui.hit_test.client == 1);
    bool dragging = c->dragging.x != 0 || c->dragging.y != 0;
//  traceln("dragging: %d title_bar_drag_start: %d,%d", dragging,
//      title_bar_drag_start.x, title_bar_drag_start.y);
    if (!dragging) {
        bool started  = c->resizing.x != 0 || c->resizing.y != 0;
        bool pressed  = (m == ui.message.left_button_pressed);
        bool released = (m == ui.message.left_button_released);
        bool holding = flags & (ui.mouse.button.left|ui.mouse.button.left);
        if (m == ui.message.mouse_move && !holding) {
            released = true;
        }
        if (pressed && !started) {
            c->resizing = app.mouse; // save starting mouse position
            ht = app.hit_test(app.mouse.x, app.mouse.y);
        }
        if (released) { // reset start position and hit test result
            c->resizing = (ui_point_t){0, 0};
            ht = ui.hit_test.client;
        }
        if (m == ui.message.mouse_move && started && holding) {
            int32_t dx = app.mouse.x - c->resizing.x;
            int32_t dy = app.mouse.y - c->resizing.y;
            if (ht != ui.hit_test.client) {
                traceln("hit_test_result: %d", ht);
                ui_rect_t r = app.wrc;
                if (ht == ui.hit_test.top_left) {
                    r.x += dx; r.y += dy; r.w -= dx; r.h -= dy;
                } else if (ht == ui.hit_test.top_right) {
                    r.y += dy; r.w += dx; r.h -= dy;
                } else if (ht == ui.hit_test.bottom_left) {
                    r.x += dx; r.w -= dx; r.h += dy;
                } else if (ht == ui.hit_test.bottom_right) {
                    r.w += dx; r.h += dy;
                } else if (ht == ui.hit_test.left) {
                    r.x += dx; r.w -= dx;
                } else if (ht == ui.hit_test.right) {
                    r.w += dx;
                } else if (ht == ui.hit_test.top) {
                    r.y += dy; r.h -= dy;
                } else if (ht == ui.hit_test.bottom) {
                    r.h += dy;
                }
                // assumes no padding in structs:
                if (memcmp(&r, &app.wrc, sizeof(r)) != 0) {
                    app.move_and_resize(&r);
                }
            }
        }
    }
}

static void ui_caption_paint(ui_view_t* v) {
    swear(v == &ui_caption.view);
    gdi.fill_with(v->x, v->y, v->w, v->h, v->color);
    if (v->text[0] != 0) {
        gdi.push(v->x, v->y);
        ui_point_t mt = gdi.measure_text(*v->font, v->text);
        gdi.x += (v->w - mt.x) / 2;
        gdi.y += (v->h - mt.y) / 2;
        gdi.set_text_color((ui_color_t)(v->color ^ 0xFFFFFF));
        gdi.text("%s", v->text);
        gdi.pop();
    }
}

static void ui_content_view_paint(ui_view_t* v) {
    swear(v == app.view);
    gdi.fill_with(0, 0, v->w, v->h, colors.dkgray1);
    ui_color_t c = app.is_active() ? colors.orange : colors.btn_hover_highlight;
    gdi.frame_with(0, 0, v->w - 0, v->h - 0, colors.dkgray1);
    gdi.frame_with(1, 1, v->w - 2, v->h - 2, c);
}

static void ui_caption_init(ui_view_t* v) {
    swear(v == &ui_caption.view, "caption is a singleton");
    v->color = colors.dkgray2;
    v->paint = ui_caption_paint;
    v->mouse = ui_caption_mouse;
    app.view->paint = ui_content_view_paint;
    app.view->mouse = ui_content_view_mouse;
}

ui_caption_t ui_caption =  {
    .view = {
        .type = ui_view_container,
        .init = ui_caption_init,
    }
};
