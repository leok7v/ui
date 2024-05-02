/* Copyright (c) Dmitry "Leo" Kuznetsov 2021-24 see LICENSE for details */
#include "ut/ut.h"
#include "ui/ui.h"

static const char* title = "Sample8: Panels";

static ui_view_t left_pane    = ui_view(container);
static ui_view_t content_pane = ui_view(container);
static ui_view_t right_pane   = ui_view(container);
static ui_view_t center_pane  = ui_view(container);
static ui_view_t bottom_pane  = ui_view(container);
static ui_view_t status_bar   = ui_view(container);

static void bottom_callback(ui_button_t* b) {
    b->view.pressed = !b->view.pressed;
    bottom_pane.hidden = b->view.pressed;
    app.layout();
}

#define ui_glyph_bottom ui_glyph_squared_minus
#define ui_glyph_side   ui_glyph_white_square_with_vertical_bisecting_line

static ui_button_t button_bottom = ui_button(ui_glyph_bottom, 0.0, bottom_callback);

static void paint_panel(ui_view_t* v) {
    gdi.push(v->x, v->y);
    gdi.fill_with(v->x, v->y, v->w, v->h, v->color);
    ui_point_t mt = gdi.measure_text(*v->font, v->text);
    gdi.x += (v->w - mt.x) / 2;
    gdi.y += (v->h - mt.y) / 2;
    gdi.set_text_color((ui_color_t)(v->color ^ 0xFFFFFF));
    gdi.print("%s", v->text);
    gdi.pop();
}

static void root_measure(ui_view_t* view) { // root measure
    assert(view == app.view);
    measurements.horizontal(&ui_caption.view, view->em.x);
    ui_caption.view.w  = view->w - 4;
    ui_caption.view.h += view->em.y / 2;
//  traceln("ui_caption: %dx%d", ui_caption.view.w, ui_caption.view.h);
    measurements.horizontal(&status_bar, view->em.x);
    status_bar.w = view->w - 4;
    status_bar.h += view->em.y / 2;
//  traceln("status_bar: %dx%d", status_bar.w, status_bar.h);
}

static void root_layout(ui_view_t* view) { // root layout
    assert(view == app.view && view->x == 0 && view->y == 0);
    ui_caption.view.x = 2;
    ui_caption.view.y = 2;
    layouts.horizontal(&ui_caption.view,
                        ui_caption.view.x + ui_caption.button_menu.view.w + view->em.x / 4,
                        ui_caption.view.y + view->em.y / 4, view->em.x / 8);
    center_pane.x = 2;
    center_pane.y = ui_caption.view.y + ui_caption.view.h;
//  traceln("center_pane: %d,%d", center_pane.x, center_pane.y);
    status_bar.x = 2;
    status_bar.y = view->h - status_bar.h - 2;
    layouts.horizontal(&status_bar, status_bar.x + view->em.x / 4,
                        status_bar.y + view->em.x / 4, view->em.x);
//  traceln("status_bar: %d,%d", status_bar.x, status_bar.y);
//  traceln("button_bottom: %d,%d", button_bottom.view.x, button_bottom.view.y);
}

static void root_paint(ui_view_t* v) {
    assert(v == app.view && v->x == 0 && v->y == 0);
    gdi.fill_with(v->x, v->y, v->w, v->h, v->color);
}

static void opened(void) {
    app.view->measure   = root_measure;
    app.view->layout    = root_layout;
    app.view->paint     = root_paint;
    ui_view.add(app.view,
        &ui_caption.view,
        ui_view.add(&center_pane,
                    &left_pane,
                    &content_pane,
                    &right_pane,
                    null),
        &bottom_pane,
        ui_view.add(&status_bar,
                    &button_bottom,
                     null),
        null);

    center_pane.hidden = true;
    bottom_pane.hidden = true;

    strprintf(ui_caption.view.text, "Sample8: Panels");
    button_bottom.view.font = &app.fonts.H3;
    button_bottom.view.color = colors.white;
    button_bottom.view.flat = true;

    // debug:
    strprintf(left_pane.text,    "left_pane");
    strprintf(content_pane.text, "content_pane");
    strprintf(right_pane.text,   "right_pane");
    strprintf(center_pane.text,  "center_pane");
    strprintf(status_bar.text,   "status_bar");

    ui_caption.view.color  = colors.dkgray2;
    left_pane.color     = colors.blue;
    content_pane.color  = colors.green;
    right_pane.color    = colors.red;
    center_pane.color   = colors.cyan;
    status_bar.color    = colors.dkgray3;

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
        .min_w =   4.0f,
        .min_h =   3.0f,
        .ini_w =  10.0f,
        .ini_h =   7.0f
    }
};

