/* Copyright (c) Dmitry "Leo" Kuznetsov 2021 see LICENSE for details */
#include "quick.h"

#ifndef CONSOLE 

// cmd.exe> cl.exe sample.c

#define quick_implementation
#include "quick.h"

begin_c

const char* title = "Sample";

static void layout(uic_t* ui) {
    layouts.center(ui);
}

static void paint(uic_t* ui) {
    // all UIC are transparent and expect parent to paint background
    // UI control paint is always called with a hollow brush
    gdi.set_brush(gdi.brush_color);
    gdi.set_brush_color(colors.black);
    gdi.fill(0, 0, ui->w, ui->h);
}

static void init() {
    app.title = title;
    app.ui->layout = layout;
    app.ui->paint = paint;
    static uic_text(text, "Hello World!");
    static uic_t* children[] = { &text.ui, null };
    app.ui->children = children;
}

app_t app = {
    .class_name = "quick-sample",
    .init = init,
    .min_width = 400,
    .min_height = 200
};

end_c

#else

// cmd.exe> cl.exe -DCONSOLE sample.c
	
#define quick_implementation_console
#include "quick.h"

static int console() {
    printf("main(%d)\n", app.argc);
    for (int i = 0; i < app.argc; i++) {
        printf("argv[%d]=\"%s\"\n", i, app.argv[i]);
    }
    return 0;
}

app_t app = { .main = console };

#endif
