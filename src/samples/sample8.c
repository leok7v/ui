/* Copyright (c) Dmitry "Leo" Kuznetsov 2021-24 see LICENSE for details */
#include "ut/ut.h"
#include "ui/ui.h"

static const char* title = "Sample8: Panels";

static ui_view_t title_bar    = ui_view(container);
static ui_view_t left_pane    = ui_view(container);
static ui_view_t content_pane = ui_view(container);
static ui_view_t right_pane   = ui_view(container);
static ui_view_t center_pane  = ui_view(container);
static ui_view_t bottom_pane  = ui_view(container);
static ui_view_t status_bar   = ui_view(container);

static void button_close_callback(ui_button_t* unused(b)) {
    app.close();
}

static void button_bottom_callback(ui_button_t* b) {
    b->view.pressed = !b->view.pressed;
    bottom_pane.hidden = b->view.pressed;
    app.layout();
}

static ui_button_t button_close  = ui_button("X",  0.0, button_close_callback);
static ui_button_t button_bottom = ui_button("[]", 0.0, button_bottom_callback);

static void title_bar_mouse(ui_view_t* view, int32_t m, int32_t flags) {
    swear(view == &title_bar, "window dragging only by title_bar");
    static ui_point_t start;
    bool started = start.x != 0 || start.y != 0;
    bool pressed =
        m == ui.message.left_button_pressed ||
        m == ui.message.right_button_pressed;
    bool released =
        m == ui.message.left_button_released ||
        m == ui.message.right_button_released;
    if (m == ui.message.mouse_move &&
        (flags & (ui.mouse.button.left|ui.mouse.button.left)) == 0) {
        released = true;
    }
    if (ui_view.inside(view, &app.mouse)) {
        if (pressed && !started) {
            start = app.mouse;
            app.capture_mouse(true);
        } else if (started && released) {
            start.x = 0;
            start.y = 0;
            app.capture_mouse(false);
            started = false;
        }
    }
    if (m == ui.message.mouse_move && started) {
        const int32_t dx = app.mouse.x - start.x;
        const int32_t dy = app.mouse.y - start.y;
//      traceln("%d,%d", dx, dy);
        ui_rect_t r = app.wrc;
        r.x += dx;
        r.y += dy;
        app.move_and_resize(&r);
    }
}

static void paint_panel(ui_view_t* view) {
    gdi.push(view->x, view->y);
    gdi.fill_with(view->x, view->y, view->w, view->h, view->color);
    ui_point_t mt = gdi.measure_text(*view->font, view->text);
    gdi.x += (view->w - mt.x) / 2;
    gdi.y += (view->h - mt.y) / 2;
    gdi.set_text_color((ui_color_t)(view->color ^ 0xFFFFFF));
    gdi.print("%s", view->text);
    gdi.pop();
}

static void root_paint(ui_view_t* view) {
    gdi.fill_with(0, 0, view->w, view->h, colors.dkgray1);
    ui_color_t c = app.is_active() ? colors.orange : colors.btn_hover_highlight;
    gdi.frame_with(0, 0, view->w, view->h, c);
}

static void root_layout(ui_view_t* view) { // root layout
    assert(view == app.view && view->x == 0 && view->y == 0);
    title_bar.x = 1;
    title_bar.y = 1;
    layouts.horizontal(&title_bar, title_bar.x +  + view->em.x / 4,
                        title_bar.y + view->em.y / 4, view->em.x);
    center_pane.x = 1;
    center_pane.y = title_bar.y + title_bar.h;
//  traceln("center_pane: %d,%d", center_pane.x, center_pane.y);
    status_bar.x = 1;
    status_bar.y = view->h - status_bar.h - 1;
    layouts.horizontal(&status_bar, status_bar.x + view->em.x / 4,
                        status_bar.y + view->em.x / 4, view->em.x);
//  traceln("status_bar: %d,%d", status_bar.x, status_bar.y);
//  traceln("button_bottom: %d,%d", button_bottom.view.x, button_bottom.view.y);
}

static void root_measure(ui_view_t* view) { // root measure
    assert(view == app.view);
    measurements.horizontal(&title_bar, view->em.x);
    title_bar.w = view->w - 2;
    title_bar.h += view->em.y / 2;
//  traceln("title_bar: %dx%d", title_bar.w, title_bar.h);
    measurements.horizontal(&status_bar, view->em.x);
    status_bar.w = view->w - 2;
    status_bar.h += view->em.y / 2;
//  traceln("status_bar: %dx%d", status_bar.w, status_bar.h);
}

static void opened(void) {
    app.view->paint     = root_paint;
    app.view->measure   = root_measure;
    app.view->layout    = root_layout;
    ui_view.add(app.view,
        ui_view.add(&title_bar, &button_close, null),
        ui_view.add(&center_pane, &left_pane, &content_pane, &right_pane, null),
        &bottom_pane,
        ui_view.add(&status_bar, &button_bottom, null),
        null);

    title_bar.mouse = title_bar_mouse;

    center_pane.hidden = true;
    bottom_pane.hidden = true;
//  status_bar.hidden = true;

    strprintf(title_bar.text,    "title_bar");
    strprintf(left_pane.text,    "left_pane");
    strprintf(content_pane.text, "content_pane");
    strprintf(right_pane.text,   "right_pane");
    strprintf(center_pane.text,  "center_pane");
    strprintf(status_bar.text,   "status_bar");

    title_bar.color     = colors.dkgray2;
    left_pane.color     = colors.blue;
    content_pane.color  = colors.green;
    right_pane.color    = colors.red;
    center_pane.color   = colors.cyan;
    status_bar.color    = colors.dkgray3;

    title_bar.paint     = paint_panel;
    left_pane.paint     = paint_panel;
    content_pane.paint  = paint_panel;
    right_pane.paint    = paint_panel;
    center_pane.paint   = paint_panel;
    status_bar.paint    = paint_panel;


}

static void init(void) {
    app.title    = title;
    app.no_decor = true;
    app.opened   = opened;
}

app_t app = {
    .class_name = "sample8",
    .init = init,
    .window_sizing = {
        .min_w =  10.0f,
        .min_h =   7.0f,
        .ini_w =   9.0f,
        .ini_h =   6.0f
    }
};

