/* Copyright (c) Dmitry "Leo" Kuznetsov 2021 see LICENSE for details */
#include "quick.h"

#ifndef CONSOLE 

// cmd.exe> cl.exe sample.c

begin_c

static void layout(uic_t* ui) {
    layouts.center(ui);
}

static void paint(uic_t* ui) {
    // all UIC are transparent and expect parent to paint background
    // UI control paint is always called with a hollow brush
    gdi.fill_with(0, 0, ui->w, ui->h, colors.black);
}

static void init(void) {
    app.title = "Hello";
    app.ui->layout = layout;
    app.ui->paint = paint;
    static uic_text(text, "Hello World!");
    static uic_t* children[] = { &text.ui, null };
    app.ui->children = children;
}

app_t app = {
    .class_name = "quick-sample",
    .init = init,
    .wmin = 4.00, // inch
    .hmin = 2.00
};

end_c

#define quick_implementation
#include "quick.h"

#else

// cmd.exe> cl.exe -DCONSOLE sample.c

begin_c

static int console(void) {
    printf("main(%d)\n", app.argc);
    for (int i = 0; i < app.argc; i++) {
        printf("argv[%d]=\"%s\"\n", i, app.argv[i]);
    }
    return 0;
}

app_t app = { .main = console };

end_c

#define quick_implementation_console
#include "quick.h"

#endif

#define crt_implementation
#include "crt.h"

