/* Copyright (c) Dmitry "Leo" Kuznetsov 2021 see LICENSE for details */
#include "quick.h"

begin_c

const char* title = "Hello World";

static void layout(uic_t* ui) {
    layouts.center(ui);
}

static void paint(uic_t* ui) {
    // all UIC are transparent and expect parent to paint background
    // UI control paint is always called with a hollow brush
    gdi.fill_with(0, 0, ui->w, ui->h, colors.black);
}

static void init(void) {
    app.ui->layout = layout;
    app.ui->paint  = paint;
    static uic_text(text, "Hello World!");
    static uic_t* children[] = { &text.ui, null };
    app.ui->children = children;
}

app_t app = {
    .title = "Hello World",
    .class_name = "hello-world",
    .init = init,
    .wmin = 4.00, // inches
    .hmin = 2.00
};

end_c