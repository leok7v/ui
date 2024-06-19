/* Copyright (c) Dmitry "Leo" Kuznetsov 2021-24 see LICENSE for details */
#include "single_file_lib/ut/ut.h"
#include "single_file_lib/ui/ui.h"

static volatile int32_t index; // index of image to paint, !ix to render
static ui_image_t image[2];
static uint8_t pixels[2][4 * 4096 * 4096];

static ut_thread_t thread;
static ut_event_t wake;
static ut_event_t quit;

static volatile bool rendering;
static volatile bool stop;
static volatile fp64_t render_time;

static void toggle_full_screen(ui_button_t* b) {
    b->pressed = !b->pressed;
    ui_app.full_screen(b->pressed);
    ui_view.set_text(b, "%s", !b->pressed ?
        ut_glyph_square_four_corners : ut_glyph_two_joined_squares);
}

ui_button_clicked(button_fs, ut_glyph_square_four_corners, 1.0, {
    toggle_full_screen(button_fs);
});

static void paint(ui_view_t* view) {
    int32_t k = index;
    ui_gdi.image(0, 0, view->w, view->h, &image[k]);
    int32_t tx = view->fm->em.w;
    int32_t ty = view->fm->em.h / 4;
    const ui_gdi_ta_t ta = { .fm = view->fm, .color = ui_colors.orange };
    ui_gdi.text(&ta, tx, ty, "%s",
                     "Try Full Screen Button there --->");
    ty = view->h - view->fm->em.h * 3 / 2;
    ui_gdi.text(&ta, tx, ty,
        "render time %.1f ms / avg paint time %.1f ms",
        render_time * 1000, ui_app.paint_avg * 1000);
    if (!rendering) {
        ui_app.set_cursor(ui_app.cursor_arrow);
    }
}

static void request_rendering(void) {
    ui_app.set_cursor(ui_app.cursor_wait);
    rendering = true;
    ut_event.set(wake);
}

static void stop_rendering(void) {
    if (rendering) {
        stop = true;
        while (rendering || stop) { ut_thread.sleep_for(0.01); }
        ui_app.set_cursor(ui_app.cursor_arrow);
    }
}

static void measure(ui_view_t* view) {
    view->w = ui_app.root->w;
    view->h = ui_app.root->h;
    const int32_t w = view->w;
    const int32_t h = view->h;
    ui_image_t* im = &image[index];
    if (w != im->w || h != im->h) {
        stop_rendering();
        im = &image[!index];
        ui_gdi.image_dispose(im);
        fatal_if(w * h * 4 > countof(pixels[!index]),
            "increase size of pixels[][%d * %d * 4]", w, h);
        ui_gdi.image_init(im, w, h, 4, pixels[!index]);
        request_rendering();
    }
}

static void layout(ui_view_t* v) {
    button_fs.x = v->w - button_fs.w - v->fm->em.w / 4;
    button_fs.y = v->fm->em.h / 4;
}

static void renderer(void* unused); // renderer thread

static void character(ui_view_t* unused(view), const char* utf8) {
    char ch = utf8[0];
    if (ch == 'q' || ch == 'Q') { ui_app.close(); }
    if (ui_app.is_full_screen && ch == 033) {
        toggle_full_screen(&button_fs);
    }
}

static void closed(void) {
    ut_event.set(quit);
    ut_thread.join(thread, -1);
    thread = null;
    ui_gdi.image_dispose(&image[0]);
    ui_gdi.image_dispose(&image[1]);
}

static void fini(void) {
    ut_event.dispose(wake);
    ut_event.dispose(quit);
    wake = null;
    quit = null;
}

static void opened(void) {
    fatal_if(ui_app.root->w * ui_app.root->h * 4 > countof(pixels[0]),
        "increase size of pixels[][%d * %d * 4]", ui_app.root->w, ui_app.root->h);
    ui_app.fini = fini;
    ui_app.closed = closed;
    ui_view.add(ui_app.content, &button_fs, null);
    ui_app.content->layout    = layout;
    ui_app.content->measure   = measure;
    ui_app.content->paint     = paint;
    ui_app.content->character = character;
    wake = ut_event.create();
    quit = ut_event.create();
    // images:
    ui_gdi.image_init(&image[0], ui_app.root->w, ui_app.root->h, 4, pixels[0]);
    ui_gdi.image_init(&image[1], ui_app.root->w, ui_app.root->h, 4, pixels[1]);
    thread = ut_thread.start(renderer, null);
    request_rendering();
    ut_str_printf(button_fs.hint, "&Full Screen");
    button_fs.shortcut = 'F';
}

static void init(void) {
    ui_app.opened = opened;
}

ui_app_t ui_app = {
    .class_name = "sample3",
    .title = "Sample3: Mandelbrot",
    .dark_mode = true,
    .init = init,
    // 6x4 inches. Thinking of 6x4 timbers columns, beams, supporting posts :)
    .window_sizing = {
        .min_w =  6.0f,
        .min_h =  4.0f,
        .ini_w =  6.0f,
        .ini_h =  4.0f
    }
};

static fp64_t scale(int32_t x, int32_t n, fp64_t low, fp64_t hi) {
    return x / (fp64_t)(n - 1) * (hi - low) + low;
}

static void mandelbrot(ui_image_t* im) {
    fp64_t time = ut_clock.seconds();
    for (int32_t r = 0; r < im->h && !stop; r++) {
        fp64_t y0 = scale(r, im->h, -1.12, 1.12);
        for (int32_t c = 0; c < im->w && !stop; c++) {
            fp64_t x0 = scale(c, im->w, -2.00, 0.47);
            fp64_t x = 0;
            fp64_t y = 0;
            int32_t iteration = 0;
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
    ut_event_t es[2] = {wake, quit};
    for (;;) {
        int32_t ix = ut_event.wait_any(countof(es), es);
        if (ix != 0) { break; }
        int32_t k = !index;
        mandelbrot(&image[k]);
        if (!stop) { index = !index; ui_app.request_redraw(); }
        stop = false;
        rendering = false;
    }
}
