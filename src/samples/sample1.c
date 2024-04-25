/* Copyright (c) Dmitry "Leo" Kuznetsov 2021-24 see LICENSE for details */
#include "ut/ut.h"
#include "ui/ui.h"

const char* title = "Sample1 - Hello World";

static void layout(ui_view_t* view) {
    // position single child at the center of the parent
    layouts.center(view); // only works for a single child view
}

// all views are transparent and expect parent to fill the background

static void paint(ui_view_t* view) {
    gdi.fill_with(0, 0, view->w, view->h, colors.black);
}

static void init(void) {
    app.title = title;
    app.view->layout = layout;
    app.view->paint = paint;
    static ui_text(text, "Hello World!");
    text.view.font = &app.fonts.H3;
    static ui_view_t* children[] = { &text.view, null };
    app.view->children = children;
}

app_t app = {
    .class_name = "sample1",
    .init = init,
    .wmin = 2.0f, // 2x1 inches
    .hmin = 1.0f
};

