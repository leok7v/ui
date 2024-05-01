/* Copyright (c) Dmitry "Leo" Kuznetsov 2021-24 see LICENSE for details */
#include "ut/ut.h"
#include "ui/ui.h"
#include "ui_caption.h"

static const char* title = "Sample8: Panels";

static ui_view_t left_pane    = ui_view(container);
static ui_view_t content_pane = ui_view(container);
static ui_view_t right_pane   = ui_view(container);
static ui_view_t center_pane  = ui_view(container);
static ui_view_t bottom_pane  = ui_view(container);
static ui_view_t status_bar   = ui_view(container);

static void close_callback(ui_button_t* unused(b)) {
    app.close();
}

static void bottom_callback(ui_button_t* b) {
    b->view.pressed = !b->view.pressed;
    bottom_pane.hidden = b->view.pressed;
    app.layout();
}

#define ui_glyph_menu  ui_glyph_trigram_for_heaven
#define ui_glyph_mini  ui_glyph_heavy_minus_sign
#define ui_glyph_maxi  ui_glyph_white_large_square
#define ui_glyph_full  ui_glyph_square_four_corners
#define ui_glyph_close ui_glyph_n_ary_times_operator

static ui_button_t button_menu  = ui_button(ui_glyph_menu,  0.0, null);
static ui_button_t button_mini  = ui_button(ui_glyph_mini,  0.0, null);
static ui_button_t button_maxi  = ui_button(ui_glyph_maxi,  0.0, null);
static ui_button_t button_full  = ui_button(ui_glyph_full,  0.0, null);
static ui_button_t button_close = ui_button(ui_glyph_close, 0.0, close_callback);

static ui_button_t button_bottom = ui_button("[]", 0.0, bottom_callback);

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

static void root_layout(ui_view_t* view) { // root layout
    assert(view == app.view && view->x == 0 && view->y == 0);
    ui_caption.view.x = 2;
    ui_caption.view.y = 2;
    layouts.horizontal(&ui_caption.view, ui_caption.view.x + view->em.x / 4,
                        ui_caption.view.y + view->em.y / 4, view->em.x);
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

static void opened(void) {
    app.view->measure   = root_measure;
    app.view->layout    = root_layout;
    ui_view.add(app.view,
        ui_view.add(&ui_caption.view,
                    &button_menu,
                    &button_mini,
                    &button_maxi,
                    &button_full,
                    &button_close, null),
        ui_view.add(&center_pane, &left_pane, &content_pane, &right_pane, null),
        &bottom_pane,
        ui_view.add(&status_bar, &button_bottom, null),
        null);

    center_pane.hidden = true;
    bottom_pane.hidden = true;
//  status_bar.hidden = true;

    strprintf(ui_caption.view.text, "Sample8: Panels");
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

