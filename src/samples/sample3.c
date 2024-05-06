/* Copyright (c) Dmitry "Leo" Kuznetsov 2021-24 see LICENSE for details */
#include "single_file_lib/ut/ut.h"
#include "single_file_lib/ui/ui.h"

static volatile int index; // index of image to paint, !ix to render
static ui_image_t image[2];
static uint8_t pixels[2][4 * 4096 * 4096];

static thread_t thread;
static event_t wake;
static event_t quit;

static volatile bool rendering;
static volatile bool stop;
static volatile fp64_t render_time;

#define glyph_two_squares "\xE2\xA7\x89" // "Two Joined Squares"

static void toggle_full_screen(ui_button_t* b) {
    b->pressed = !b->pressed;
    app.full_screen(b->pressed);
}

static_ui_button(button_fs, glyph_two_squares, 1.0, {
    toggle_full_screen(button_fs);
});

static void paint(ui_view_t* view) {
    int k = index;
    gdi.draw_image(0, 0, view->w, view->h, &image[k]);
    gdi.x = view->em.x;
    gdi.y = view->em.y / 4;
    gdi.set_text_color(ui_colors.orange);
    gdi.textln("Try Full Screen Button there --->");
    gdi.y = view->h - view->em.y * 3 / 2;
    gdi.set_text_color(ui_colors.orange);
    gdi.textln("render time %.1f ms / avg paint time %.1f ms",
        render_time * 1000, app.paint_avg * 1000);
    if (!rendering) {
        app.set_cursor(app.cursor_arrow);
    }
}

static void request_rendering(void) {
    app.set_cursor(app.cursor_wait);
    rendering = true;
    ut_event.set(wake);
}

static void stop_rendering(void) {
    if (rendering) {
        stop = true;
        while (rendering || stop) { ut_thread.sleep_for(0.01); }
        app.set_cursor(app.cursor_arrow);
    }
}

static void measure(ui_view_t* view) {
    // called on window resize
    assert(view->w == app.crc.w);
    assert(view->h == app.crc.h);
    const int w = view->w;
    const int h = view->h;
    ui_image_t* im = &image[index];
    if (w != im->w || h != im->h) {
        stop_rendering();
        im = &image[!index];
        gdi.image_dispose(im);
        fatal_if(w * h * 4 > countof(pixels[!index]),
            "increase size of pixels[][%d * %d * 4]", w, h);
        gdi.image_init(im, w, h, 4, pixels[!index]);
        request_rendering();
    }
}

static void layout(ui_view_t* v) {
    button_fs.x = v->w - button_fs.w - v->em.x / 4;
    button_fs.y = v->em.y / 4;
}

static void renderer(void* unused); // renderer thread

static void character(ui_view_t* unused(view), const char* utf8) {
    char ch = utf8[0];
    if (ch == 'q' || ch == 'Q') { app.close(); }
    if (app.is_full_screen && ch == 033) {
        toggle_full_screen(&button_fs);
    }
}

static void closed(void) {
    ut_event.set(quit);
    ut_thread.join(thread, -1);
    thread = null;
    gdi.image_dispose(&image[0]);
    gdi.image_dispose(&image[1]);
}

static void fini(void) {
    ut_event.dispose(wake);
    ut_event.dispose(quit);
    wake = null;
    quit = null;
}

static void opened(void) {
    fatal_if(app.crc.w * app.crc.h * 4 > countof(pixels[0]),
        "increase size of pixels[][%d * %d * 4]", app.crc.w, app.crc.h);
    app.fini = fini;
    app.closed = closed;
    ui_view.add(app.view, &button_fs, null);
    app.view->layout    = layout;
    app.view->measure   = measure;
    app.view->paint     = paint;
    app.view->character = character;
    wake = ut_event.create();
    quit = ut_event.create();
    // images:
    gdi.image_init(&image[0], app.crc.w, app.crc.h, 4, pixels[0]);
    gdi.image_init(&image[1], app.crc.w, app.crc.h, 4, pixels[1]);
    thread = ut_thread.start(renderer, null);
    request_rendering();
    strprintf(button_fs.hint, "&Full Screen");
    button_fs.shortcut = 'F';
}

static void init(void) {
    app.opened = opened;
}

app_t app = {
    .class_name = "sample3",
    .title = "Sample3: Mandelbrot",
    .init = init,
    // 6x4 inches. Thinking of 6x4 timbers columns, beams, supporting posts :)
    .window_sizing = {
        .min_w =  6.0f,
        .min_h =  4.0f,
        .ini_w =  6.0f,
        .ini_h =  4.0f
    }
};

static fp64_t scale(int x, int n, fp64_t low, fp64_t hi) {
    return x / (fp64_t)(n - 1) * (hi - low) + low;
}

static void mandelbrot(ui_image_t* im) {
    fp64_t time = ut_clock.seconds();
    for (int r = 0; r < im->h && !stop; r++) {
        fp64_t y0 = scale(r, im->h, -1.12, 1.12);
        for (int c = 0; c < im->w && !stop; c++) {
            fp64_t x0 = scale(c, im->w, -2.00, 0.47);
            fp64_t x = 0;
            fp64_t y = 0;
            int iteration = 0;
            enum { max_iteration = 100 };
            while (x* x + y * y <= 2 * 2 && iteration < max_iteration && !stop) {
                fp64_t t = x * x - y * y + x0;
                y = 2 * x * y + y0;
                x = t;
                iteration++;
            }
            static ui_color_t palette[16] = {
                ui_rgb( 66,  30,  15),  ui_rgb( 25,   7,  26),
                ui_rgb(  9,   1,  47),  ui_rgb(  4,   4,  73),
                ui_rgb(  0,   7, 100),  ui_rgb( 12,  44, 138),
                ui_rgb( 24,  82, 177),  ui_rgb( 57, 125, 209),
                ui_rgb(134, 181, 229),  ui_rgb(211, 236, 248),
                ui_rgb(241, 233, 191),  ui_rgb(248, 201,  95),
                ui_rgb(255, 170,   0),  ui_rgb(204, 128,   0),
                ui_rgb(153,  87,   0),  ui_rgb(106,  52,   3)
            };
            ui_color_t color = palette[iteration % countof(palette)];
            uint8_t* px = &((uint8_t*)im->pixels)[r * im->w * 4 + c * 4];
            px[3] = 0xFF;
            px[0] = (color >> 16) & 0xFF;
            px[1] = (color >>  8) & 0xFF;
            px[2] = (color >>  0) & 0xFF;
        }
    }
    render_time = ut_clock.seconds() - time;
}

static void renderer(void* unused) {
    (void)unused;
    ut_thread.name("renderer");
    ut_thread.realtime();
    event_t es[2] = {wake, quit};
    for (;;) {
        int e = ut_event.wait_any(countof(es), es);
        if (e != 0) { break; }
        int k = !index;
        mandelbrot(&image[k]);
        if (!stop) { index = !index; app.redraw(); }
        stop = false;
        rendering = false;
    }
}
