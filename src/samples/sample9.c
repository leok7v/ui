/* Copyright (c) Dmitry "Leo" Kuznetsov 2021-24 see LICENSE for details */
#include "single_file_lib/ut/ut.h"
#include "single_file_lib/ui/ui.h"
#include "i18n.h"

#define TITLE "Sample9"

static void init(void);

ui_app_t ui_app = {
    .class_name = "sample9",
    .init = init,
    .dark_mode = true,
    .window_sizing = {
        .min_w =  9.0f,
        .min_h =  5.5f,
        .ini_w = 10.0f,
        .ini_h =  6.0f
    }
};

static int32_t panel_border = 1;
static int32_t frame_border = 1;

static ui_image_t image;
static uint32_t pixels[1024][1024];

static fp64_t zoom = 0.5;
static fp64_t sx = 0.25; // [0..1]
static fp64_t sy = 0.25; // [0..1]

static struct { fp64_t x; fp64_t y; } stack[52];
static int top = 1; // because it is already zoomed in once above

static ui_slider_t zoomer;

static ui_label_t toast_filename = ui_label(0.0, "filename placeholder");

static ui_label_t label_single_line = ui_label(0.0, "Mandelbrot Explorer");

static ui_label_t label_multiline = ui_label(19.0,
    "Click inside or +/- to zoom;\n"
    "right mouse click to zoom out;\n"
    "use touchpad or keyboard "
    ut_glyph_leftwards_white_arrow ut_glyph_upwards_white_arrow
    ut_glyph_downwards_white_arrow ut_glyph_rightwards_white_arrow
    " to pan");

static ui_label_t about = ui_label(34.56f,
    "\nClick inside Mandelbrot Julia Set fractal to zoom in into interesting "
    "areas. Right mouse click to zoom out.\n"
    "Use Win + Shift + S to take a screenshot of something "
    "beautiful that caught your eye."
    "\n\n"
    "This sample also a showcase of controls like toggle, message box, "
    "tooltips, clipboard copy, full screen switching, open file "
    "dialog and on-the-fly locale switching for simple and possibly "
    "incorrect Simplified Chinese localization."
    "\n\n"
    "Press ESC or click the "
    ut_glyph_multiplication_sign
    " button in right top corner "
    "to dismiss this message or just wait - it will disappear by "
    "itself in 10 seconds.\n");

#ifdef SAMPLE9_USE_STATIC_UI_VIEW_MACROS

ui_mbx_on_choice(mbx, // message box
    "\"Pneumonoultramicroscopicsilicovolcanoconiosis\"\n"
    "is it the longest English language word or not?", {
    traceln("option=%d", option); // -1 or index of { "&Yes", "&No" }
}, "&Yes", "&No");

#else

static void mbx_callback(ui_view_t* v) {
    ui_mbx_t* mbx = (ui_mbx_t*)v;
    assert(-1 <= mbx->option && mbx->option < 2);
    static const char* name[] = { "Cancel", "Yes", "No" };
    traceln("option: %d \"%s\"", mbx->option, name[mbx->option + 1]);
}

static ui_mbx_t mbx = ui_mbx( // message box
    "\"Pneumonoultramicroscopicsilicovolcanoconiosis\"\n"
    "is it the longest English language word or not?", mbx_callback,
    "&Yes", "&No");

#endif

static const char* filter[] = {
    "All Files", "*",
    "Image Files", "*.png;*.jpg",
    "Text Files", "*.txt;*.doc;*.ini",
    "Executables", "*.exe"
};

static void open_file(ui_button_t* unused(b)) {
    ut_file_name_t fn = ui_app.open_file_dialog(
        ut_files.known_folder(ut_files.folder.home),
        filter, countof(filter)); //  all files filer: null, 0
    if (fn.s[0] != 0) {
        ui_view.set_text(&toast_filename, "%s", fn.s);
        traceln("%s", fn.s);
        ui_app.show_toast(&toast_filename, 2.0);
    }
}

ui_button_on_click(button_open_file, "&Open", 7.5, {
    open_file(button_open_file);
});

static void flip_full_screen(ui_button_t* b) {
    b->pressed = !b->pressed;
    ui_app.full_screen(b->pressed);
    if (b->pressed) {
        ui_app.toast(1.75, "Press ESC to exit full screen");
    }
}

ui_button_on_click(button_full_screen,
                   ut_glyph_square_four_corners, 1, {
    flip_full_screen(button_full_screen);
});

static void flip_locale(ui_button_t* b) {
    b->pressed = !b->pressed;
    ut_nls.set_locale(b->pressed ? "zh-CN" : "en-US");
//  ui_view.localize(&label_multiline);
    ui_app.request_layout(); // because center panel layout changed
}

static ui_button_t button_locale = ui_button(
    ut_glyph_kanji_onna_female "A", 1, flip_locale);

ui_button_on_click(button_about, "&About", 7.5, {
    ui_app.show_toast(&about, 10.0);
});

ui_button_on_click(button_mbx, "&Message Box", 7.5, {
    ui_app.show_toast(&mbx.view, 0);
});

#ifdef SAMPLE9_USE_STATIC_UI_VIEW_MACROS

// ui_toggle label can include "___" for "ON ": "OFF" state
ui_toggle_on_switch(scroll, "Scroll &Direction:", 0, {});

#else

static ui_toggle_t scroll = ui_toggle("Scroll &Direction:",
                                      /* min_w_em: */ 0.0f,
                                      /* callback:*/ null);

#endif

static ui_view_t panel_top    = ui_view(container);
static ui_view_t panel_bottom = ui_view(container);
static ui_view_t panel_center = ui_view(container);
static ui_view_t panel_right  = ui_view(container);

static void panel_paint(ui_view_t* v) {
    if (v->color == ui_colors.transparent) {
        v->color = ui_app.content->color;
    }
    ui_gdi.push(v->x, v->y);
    ui_gdi.set_clip(v->x, v->y, v->w, v->h);
    ui_gdi.fill_with(v->x, v->y, v->w, v->h, ui_colors.dkgray1);
    ui_pen_t p = ui_gdi.create_pen(ui_colors.dkgray4, panel_border);
    ui_gdi.set_pen(p);
    ui_gdi.move_to(v->x, v->y);
    if (v == &panel_right) {
        ui_gdi.line(v->x + v->w, v->y);
        ui_gdi.line(v->x + v->w, v->y + v->h);
        ui_gdi.line(v->x, v->y + v->h);
        ui_gdi.line(v->x, v->y);
    } else if (v == &panel_top || v == &panel_bottom) {
        ui_gdi.line(v->x, v->y + v->h);
        ui_gdi.line(v->x + v->w, v->y + v->h);
        ui_gdi.move_to(v->x + v->w, v->y);
        ui_gdi.line(v->x, v->y);
    } else {
        assert(v == &panel_center);
        ui_gdi.line(v->x, v->y + v->h);
    }
    int32_t x = v->x + panel_border + ut_max(1, v->fm->em.w / 8);
    int32_t y = v->y + panel_border + ut_max(1, v->fm->em.h / 4);
    const int32_t radius = (v->fm->em.h / 4) | 0x1;
    ui_gdi.rounded_with(x, y, v->fm->em.w * 12, v->fm->em.h,
           radius,  v->color, ui_colors.transparent);
    ui_color_t color = ui_gdi.set_text_color(v->color);
    ui_gdi.x = v->x + panel_border + ut_max(1, v->fm->em.w / 2);
    ui_gdi.y = v->y + panel_border + ut_max(1, v->fm->em.h / 4);
    ui_gdi.text("%d,%d %dx%d %s", v->x, v->y, v->w, v->h, ui_view.string(v));
    ui_gdi.set_text_color(color);
    ui_gdi.set_clip(0, 0, 0, 0);
    ui_gdi.delete_pen(p);
    ui_gdi.pop();
}

static void right_layout(ui_view_t* v) {
    int x = v->x + v->fm->em.w;
    int y = v->y + v->fm->em.h * 2;
    ui_view_for_each(v, c, {
        c->x = x;
        c->y = y;
        y += c->h + v->fm->em.h / 2;
    });
}

static void text_after(ui_view_t* view, const char* format, ...) {
    ui_gdi.x = view->x + view->w + view->fm->em.w;
    ui_gdi.y = view->y;
    va_list va;
    va_start(va, format);
    ui_gdi.vtextln(format, va);
    va_end(va);
}

static void right_paint(ui_view_t* v) {
    panel_paint(v);
    ui_gdi.push(v->x, v->y);
    ui_gdi.set_clip(v->x, v->y, v->w, v->h);
    ui_gdi.x = button_locale.x + button_locale.w + v->fm->em.w;
    ui_gdi.y = button_locale.y;
    ui_gdi.println("&Locale %s", button_locale.pressed ? "zh-CN" : "en-US");
    ui_gdi.x = button_full_screen.x + button_full_screen.w + v->fm->em.w;
    ui_gdi.y = button_full_screen.y;
    ui_gdi.println(ui_app.is_full_screen ?
        ut_nls.str("Restore from &Full Screen") :
        ut_nls.str("&Full Screen"));
    ui_gdi.x = label_multiline.x;
    ui_gdi.y = label_multiline.y + label_multiline.h + v->fm->em.h / 4;
    ui_gdi.textln(ut_nls.str("Proportional"));
    ui_gdi.println(ut_nls.str("Monospaced"));
    ui_font_t font = ui_gdi.set_font(ui_app.fonts.H1.font);
    ui_gdi.textln("H1 %s", ut_nls.str("Header"));
    ui_gdi.set_font(ui_app.fonts.H2.font);
    ui_gdi.textln("H2 %s", ut_nls.str("Header"));
    ui_gdi.set_font(ui_app.fonts.H3.font);
    ui_gdi.textln("H3 %s", ut_nls.str("Header"));
    ui_gdi.set_font(font);
    ui_gdi.textln("%s %dx%d root: %d,%d %dx%d", ut_nls.str("Client area"),
                  ui_app.crc.w, ui_app.crc.h,
                  ui_app.root->x, ui_app.root->y, ui_app.root->w, ui_app.root->h);
    ui_gdi.textln("%s %dx%d dpi: %d", ut_nls.str("Window"),
                  ui_app.wrc.w, ui_app.wrc.h, ui_app.dpi.window);
    ui_gdi.textln("%s %dx%d dpi: %d ang %d raw %d",
                  ut_nls.str("Monitor"),
                  ui_app.mrc.w, ui_app.mrc.h,
                  ui_app.dpi.monitor_effective,
                  ui_app.dpi.monitor_angular,
                  ui_app.dpi.monitor_raw);
    ui_gdi.textln("%s %d %d", ut_nls.str("Left Top"),
                  ui_app.wrc.x, ui_app.wrc.y);
    ui_gdi.textln("%s %d %d", ut_nls.str("Mouse"),
                  ui_app.mouse.x, ui_app.mouse.y);
    ui_gdi.textln("%d x paint()", ui_app.paint_count);
    ui_gdi.textln("%.1fms (%s %.1f %s %.1f)",
        ui_app.paint_time * 1000.0,
        ut_nls.str("max"), ui_app.paint_max * 1000.0,
        ut_nls.str("avg"), ui_app.paint_avg * 1000.0);
    text_after(&zoomer.view, "%.16f", zoom);
    text_after(&scroll, "%s", scroll.pressed ?
        ut_nls.str("Natural") : ut_nls.str("Reverse"));
    ui_gdi.set_clip(0, 0, 0, 0);
    ui_gdi.pop();
}

static void center_paint(ui_view_t* view) {
    ui_gdi.set_clip(view->x, view->y, view->w, view->h);
    ui_gdi.fill_with(view->x, view->y, view->w, view->h, ui_colors.black);
    int x = (view->w - image.w) / 2;
    int y = (view->h - image.h) / 2;
//  ui_gdi.alpha_blend(view->x + x, view->y + y, image.w, image.h, &image, 0.5);
    ui_gdi.draw_image(view->x + x, view->y + y, image.w, image.h, &image);
    ui_gdi.set_clip(0, 0, 0, 0);
}

static void measure(ui_view_t* v) {
    v->fm = &ui_app.fonts.mono;
    panel_border = ut_max(1, v->fm->em.h / 4);
    frame_border = ut_max(1, v->fm->em.h / 8);
    assert(panel_border > 0 && frame_border > 0);
    const int32_t w = ui_app.root->w;
    const int32_t h = ui_app.root->h;
    // measure ui elements
    panel_top.w = (int32_t)(0.70 * w);
    panel_top.h = v->fm->em.h * 2;
    panel_bottom.w = panel_top.w;
    panel_bottom.h = v->fm->em.h * 2;
    panel_right.w = w - panel_bottom.w;
    panel_right.h = h;
    panel_center.w = panel_bottom.w;
    panel_center.h = h - panel_bottom.h - panel_top.h;
}

static void layout(ui_view_t* unused(view)) {
    assert(view->fm->em.w > 0 && view->fm->em.h > 0);
    const int32_t h = ui_app.root->h;
    panel_top.x = 0;
    panel_top.y = 0;
    panel_bottom.x = 0;
    panel_bottom.y = h - panel_bottom.h;
    panel_right.x = panel_bottom.w;
    panel_right.y = 0;
    panel_center.x = 0;
    panel_center.y = panel_top.h;
}

static void refresh(void);

static void zoom_out(void) {
    assert(top > 0);
    top--;
    sx = stack[top].x;
    sy = stack[top].y;
    zoom *= 2;
}

static void zoom_in(int x, int y) {
    assert(top < countof(stack));
    stack[top].x = sx;
    stack[top].y = sy;
    top++;
    zoom /= 2;
    sx += zoom * x / image.w;
    sy += zoom * y / image.h;
}

static void mouse(ui_view_t* unused(view), int32_t m, int64_t unused(flags)) {
    int mx = ui_app.mouse.x - panel_center.x;
    int my = ui_app.mouse.y - panel_center.y;
    if (0 <= mx && mx < panel_center.w && 0 <= my && my < panel_center.h) {
        int x = ui_app.mouse.x - (panel_center.w - image.w) / 2 - panel_center.x;
        int y = ui_app.mouse.y - (panel_center.h - image.h) / 2 - panel_center.y;
        if (0 <= x && x < image.w && 0 <= y && y < image.h) {
            if (m == ui.message.right_button_pressed) {
                if (zoom < 1) { zoom_out(); refresh(); }
            } else if (m == ui.message.left_button_pressed) {
                if (top < countof(stack)) { zoom_in(x, y); refresh(); }
            }
        }
        ui_app.request_redraw();
    }
}

static void slider_format(ui_view_t* v) {
    ui_slider_t* slider = (ui_slider_t*)v;
    ui_view.set_text(v, "%s", ut_str.uint64(slider->value));
}

static void zoomer_callback(ui_view_t* v) {
    ui_slider_t* slider = (ui_slider_t*)v;
    fp64_t z = 1;
    for (int i = 0; i < slider->value; i++) { z /= 2; }
    while (zoom > z) { zoom_in(image.w / 2, image.h / 2); }
    while (zoom < z) { zoom_out(); }
    refresh();
}

static void mouse_wheel(ui_view_t* unused, int32_t dx, int32_t dy) {
    (void)unused;
    if (!scroll.pressed) { dy = -dy; }
    if (!scroll.pressed) { dx = -dx; }
    sx = sx + zoom * dx / image.w;
    sy = sy + zoom * dy / image.h;
    refresh();
}

static void character(ui_view_t* view, const char* utf8) {
    char ch = utf8[0];
    if (ch == 'q' || ch == 'Q') {
        ui_app.close();
    } else if (ch == 033 && ui_app.is_full_screen) {
        button_full_screen_callback(&button_full_screen);
    } else if (ch == '+' || ch == '=') {
        zoom /= 2; refresh();
    } else if (ch == '-' || ch == '_') {
        zoom = ut_min(zoom * 2, 1.0); refresh();
    } else if (ch == '<' || ch == ',') {
        mouse_wheel(view, +image.w / 8, 0);
    } else if (ch == '>' || ch == '.') {
        mouse_wheel(view, -image.w / 8, 0);
    } else if (ch == 3) { // Ctrl+C
        ut_clipboard.put_image(&image);
    }
}

static void keyboard(ui_view_t* view, int64_t vk) {
    if (vk == ui.key.up) {
        mouse_wheel(view, 0, +image.h / 8);
    } else if (vk == ui.key.down) {
        mouse_wheel(view, 0, -image.h / 8);
    } else if (vk == ui.key.left) {
        mouse_wheel(view, +image.w / 8, 0);
    } else if (vk == ui.key.right) {
        mouse_wheel(view, -image.w / 8, 0);
    }
}

static void init_panel(ui_view_t* panel, const char* text, ui_color_t color,
        void (*paint)(ui_view_t*)) {
    ui_view.set_text(panel, "%s", text);
    panel->color = color;
    panel->paint = paint;
}

static void opened(void) {
    ui_app.content->measure     = measure;
    ui_app.content->layout      = layout;
    ui_app.content->character   = character;
    ui_app.content->key_pressed = keyboard; // virtual_keys
    ui_app.content->mouse_wheel = mouse_wheel;
    panel_center.mouse = mouse;
    int n = countof(pixels);
    static_assert(sizeof(pixels[0][0]) == 4, "4 bytes per pixel");
    static_assert(countof(pixels) == countof(pixels[0]), "square");
    ui_gdi.image_init(&image, n, n, (int32_t)sizeof(pixels[0][0]), (uint8_t*)pixels);
    init_panel(&panel_top,    "top",    ui_colors.orange, panel_paint);
    init_panel(&panel_center, "center", ui_colors.off_white, center_paint);
    init_panel(&panel_bottom, "bottom", ui_colors.tone_blue, panel_paint);
    init_panel(&panel_right,  "right",  ui_colors.tone_green, right_paint);
    panel_right.layout = right_layout;
    label_single_line.highlightable = true;
    label_single_line.flat = true;
    label_multiline.highlightable = true;
    ut_str_printf(label_multiline.hint, "%s",
        "Ctrl+C or Right Mouse click to copy text to clipboard");
    ui_view.set_text(&label_multiline, "%s", ut_nls.string(str_help, ""));
    toast_filename.fm = &ui_app.fonts.H1;
    about.fm = &ui_app.fonts.H3;
    button_locale.shortcut = 'l';
    button_full_screen.shortcut = 'f';
#ifdef SAMPLE9_USE_STATIC_UI_VIEW_MACROS
    ui_slider_init(&zoomer, "Zoom: 1 / (2^%d)", 7.0, 0, countof(stack) - 1,
        zoomer_callback);
#else
    zoomer = (ui_slider_t)ui_slider("Zoom: 1 / (2^%d)", 7.0, 0, countof(stack) - 1,
        slider_format, zoomer_callback);
#endif
    ut_str_printf(button_mbx.hint, "Show Yes/No message box");
    ut_str_printf(button_about.hint, "Show About message box");
    ui_view.add(&panel_right,
        &button_locale,
        &button_full_screen,
        &zoomer,
        &scroll,
        &button_open_file,
        &button_about,
        &button_mbx,
        &label_single_line,
        &label_multiline,
        null
    );
    ui_view.add(ui_app.content,
        &panel_top,
        &panel_center,
        &panel_right,
        &panel_bottom,
        null);
    refresh();
}

static void init(void) {
    ui_app.title = TITLE;
    ui_app.opened = opened;
}

static fp64_t scale0to1(int v, int range, fp64_t sh, fp64_t zm) {
    return sh + zm * v / range;
}

static fp64_t into(fp64_t v, fp64_t lo, fp64_t hi) {
    assert(0 <= v && v <= 1);
    return v * (hi - lo) + lo;
}

static void mandelbrot(ui_image_t* im) {
    for (int r = 0; r < im->h; r++) {
        fp64_t y0 = into(scale0to1(r, im->h, sy, zoom), -1.12, 1.12);
        for (int c = 0; c < im->w; c++) {
            fp64_t x0 = into(scale0to1(c, im->w, sx, zoom), -2.00, 0.47);
            fp64_t x = 0;
            fp64_t y = 0;
            int iteration = 0;
            enum { max_iteration = 100 };
            while (x* x + y * y <= 2 * 2 && iteration < max_iteration) {
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
}

static void refresh(void) {
    if (sx < 0) { sx = 0; }
    if (sx > 1 - zoom) { sx = 1 - zoom; }
    if (sy < 0) { sy = 0; }
    if (sy > 1 - zoom) { sy = 1 - zoom; }
    if (zoom == 1) { sx = 0; sy = 0; }
    zoomer.value = 0;
    fp64_t z = 1;
    while (z != zoom) { zoomer.value++; z /= 2; }
    zoomer.value = ut_min(zoomer.value, zoomer.value_max);
    mandelbrot(&image);
    ui_app.request_redraw();
}

