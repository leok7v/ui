/* Copyright (c) Dmitry "Leo" Kuznetsov 2021-24 see LICENSE for details */
#include "single_file_lib/rt/rt.h"
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

static ui_bitmap_t image;
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
    rt_glyph_leftwards_white_arrow rt_glyph_upwards_white_arrow
    rt_glyph_downwards_white_arrow rt_glyph_rightwards_white_arrow
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
    rt_glyph_multiplication_sign
    " button in right top corner "
    "to dismiss this message or just wait - it will disappear by "
    "itself in 10 seconds.\n");

#ifdef SAMPLE9_USE_STATIC_UI_VIEW_MACROS

ui_mbx_chosen(mbx, // message box
    "\"Pneumonoultramicroscopicsilicovolcanoconiosis\"\n"
    "is it the longest English language word or not?", {
    rt_println("option=%d", option); // -1 or index of { "&Yes", "&No" }
}, "&Yes", "&No");

#else

static void mbx_callback(ui_view_t* v) {
    ui_mbx_t* mbx = (ui_mbx_t*)v;
    rt_assert(-1 <= mbx->option && mbx->option < 2);
    static const char* name[] = { "Cancel", "Yes", "No" };
    rt_println("option: %d \"%s\"", mbx->option, name[mbx->option + 1]);
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

static void open_file(ui_button_t* rt_unused(b)) {
    const char* home = rt_files.known_folder(rt_files.folder.home);
    //  all files filer: null, 0
    const char* fn = ui_app.open_file(home, filter, rt_countof(filter));
    if (fn[0] != 0) {
        ui_view.set_text(&toast_filename, "\n%s\n", fn);
        rt_println("\"%s\"", fn);
        ui_app.show_toast(&toast_filename, 3.3);
    }
}

ui_button_t button_open_file = ui_button("&Open", 7.5, open_file);

static void flip_full_clicked(ui_button_t* b) {
    b->state.pressed = !b->state.pressed;
    ui_app.full_screen(b->state.pressed);
    if (b->state.pressed) {
        ui_app.toast(1.75, "Press ESC to exit full screen");
    }
}

static ui_button_t button_full_screen = ui_button(
        rt_glyph_square_four_corners, 1, flip_full_clicked);

static void flip_locale(ui_button_t* b) {
    b->state.pressed = !b->state.pressed;
    rt_fatal_if_error(rt_nls.set_locale(b->state.pressed ? "zh-CN" : "en-US"));
    ui_app.request_layout(); // because center panel layout changed
}

static ui_button_t button_locale = ui_button(
    rt_glyph_kanji_onna_female "A", 1, flip_locale);

static void about_clicked(ui_button_t* rt_unused(b)) {
    ui_app.show_toast(&about, 10.0);
}

static ui_button_t button_about = ui_button("&About", 7.5, about_clicked);

ui_button_clicked(button_mbx, "&Message Box", 7.5, {
    ui_app.show_toast(&mbx.view, 0);
});


static void scroll_toggle(ui_button_t* rt_unused(b)) {
    ui_app.request_redraw();
}

static ui_toggle_t scroll = ui_toggle("Scroll &Direction:",
                                      /* min_w_em: */ 3.0f,
                              /* callback:*/ scroll_toggle);

static ui_view_t panel_top    = ui_view(stack);
static ui_view_t panel_bottom = ui_view(stack);
static ui_view_t panel_center = ui_view(stack);
static ui_view_t panel_right  = ui_view(stack);

static const ui_gdi_ta_t* ta = &ui_gdi.ta.prop.normal;

static void print(int32_t *x, int32_t *y, const char* format, ...) {
    va_list va;
    va_start(va, format);
    *x += ui_gdi.text_va(ta, *x, *y, format, va).w;
    va_end(va);
}

static void println(int32_t *x, int32_t *y, const char* format, ...) {
    va_list va;
    va_start(va, format);
    *y += ui_gdi.text_va(ta, *x, *y, format, va).h;
    va_end(va);
}

static void after(ui_view_t* v, const char* format, ...) {
    const ui_ltrb_t insets = ui_view.margins(v, &v->insets);
    int32_t x = v->x + v->w + v->fm->em.w;
    int32_t y = v->y + insets.top;
    va_list va;
    va_start(va, format);
    ui_gdi.text_va(ta, x, y, format, va);
    va_end(va);
}

static void panel_paint(ui_view_t* v) {
    if (v->color == ui_colors.transparent) {
        v->color = ui_app.content->color;
    }
    ui_gdi.fill(v->x, v->y, v->w, v->h, ui_color_rgb(30, 30, 30));
    ui_color_t c = ui_color_rgb(63, 63, 70);
    if (v == &panel_right) {
        ui_gdi.line(v->x, v->y, v->x + v->w, v->y, c);
        ui_gdi.line(v->x + v->w, v->y, v->x + v->w, v->y + v->h, c);
        ui_gdi.line(v->x + v->w, v->y + v->h, v->x, v->y + v->h, c);
        ui_gdi.line(v->x, v->y + v->h, v->x, v->y, c);
    } else if (v == &panel_top || v == &panel_bottom) {
        ui_gdi.line(v->x, v->y, v->x, v->y + v->h, c);
        ui_gdi.line(v->x, v->y + v->h, v->x + v->w, v->y + v->h, c);
        ui_gdi.line(v->x + v->w, v->y, v->x, v->y, c);
    } else {
        rt_assert(v == &panel_center);
        ui_gdi.line(v->x, v->y, v->x, v->y + v->h, c);
    }
    int32_t x = v->x + panel_border + rt_max(1, v->fm->em.w / 8);
    int32_t y = v->y + panel_border + rt_max(1, v->fm->em.h / 4);
    const int32_t radius = (v->fm->em.h / 4) | 0x1;
    ui_gdi.rounded(x, y, v->fm->em.w * 12, v->fm->em.h,
           radius,  v->color, ui_colors.transparent);
    x = v->x + panel_border + rt_max(1, v->fm->em.w / 2);
    y = v->y + panel_border + rt_max(1, v->fm->em.h / 4);
    ui_gdi.text(ta, x, y, "%d,%d %dx%d %s",
                     v->x, v->y, v->w, v->h, ui_view.string(v));
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

static void right_paint(ui_view_t* v) {
    panel_paint(v);
    const ui_gdi_ta_t* restore = ta;
    after(&button_locale, "&Locale %s", button_locale.state.pressed ?
        "zh-CN" : "en-US");
    after(&button_full_screen, "%s",
        ui_app.is_full_screen ?
        rt_nls.str("Restore from &Full Screen") :
        rt_nls.str("&Full Screen"));
    int32_t x = label_multiline.x;
    int32_t y = label_multiline.y + label_multiline.h + v->fm->em.h / 4;
//  rt_println("%d,%d %dx%d",
//      label_multiline.x,
//      label_multiline.y,
//      label_multiline.w,
//      label_multiline.h
//  );

    println(&x, &y, "%s", rt_nls.str("Proportional"));
    ta = &ui_gdi.ta.mono.normal;
    println(&x, &y, "%s", rt_nls.str("Monospaced"));
    ta = &ui_gdi.ta.prop.H1;
    println(&x, &y, "H1 %s", rt_nls.str("Header"));
    ta = &ui_gdi.ta.prop.H2;
    println(&x, &y, "H2 %s", rt_nls.str("Header"));
    ta = &ui_gdi.ta.prop.H3;
    println(&x, &y, "H3 %s", rt_nls.str("Header"));
    ta = &ui_gdi.ta.prop.normal;
    println(&x, &y, "%s %dx%d root: %d,%d %dx%d", rt_nls.str("Client area"),
            ui_app.crc.w, ui_app.crc.h,
            ui_app.root->x, ui_app.root->y,
            ui_app.root->w, ui_app.root->h);
    println(&x, &y, "%s %dx%d dpi: %d", rt_nls.str("Window"),
            ui_app.wrc.w, ui_app.wrc.h, ui_app.dpi.window);
    println(&x, &y, "%s %dx%d dpi: %d ang %d raw %d",
            rt_nls.str("Monitor"),
            ui_app.mrc.w, ui_app.mrc.h,
            ui_app.dpi.monitor_effective,
            ui_app.dpi.monitor_angular,
            ui_app.dpi.monitor_raw);
    println(&x, &y, "%s %d %d", rt_nls.str("Left Top"),
            ui_app.wrc.x, ui_app.wrc.y);
    println(&x, &y, "%s %d %d", rt_nls.str("Mouse"),
            ui_app.mouse.x, ui_app.mouse.y);
    println(&x, &y, "%d x paint()", ui_app.paint_count);
    println(&x, &y, "%.1fms (%s %.1f %s %.1f)",
            ui_app.paint_time * 1000.0,
            rt_nls.str("max"), ui_app.paint_max * 1000.0,
            rt_nls.str("avg"), ui_app.paint_avg * 1000.0);
    after(&zoomer.view, "%.16f", zoom);
    after(&scroll, "%s", scroll.state.pressed ?
        rt_nls.str("Natural") : rt_nls.str("Reverse"));
    ta = restore;
}

static void center_paint(ui_view_t* view) {
//  ui_gdi.set_clip(view->x, view->y, view->w, view->h);
    ui_gdi.fill(view->x, view->y, view->w, view->h, ui_colors.black);
    int x = (view->w - image.w) / 2;
    int y = (view->h - image.h) / 2;
//  ui_gdi.alpha(view->x + x, view->y + y, image.w, image.h, &image, 0.5);
    ui_gdi.bitmap(view->x + x, view->y + y, image.w, image.h,
                 0, 0, image.w, image.h, &image);
//  ui_gdi.set_clip(0, 0, 0, 0);
}

static void measure(ui_view_t* v) {
    v->fm = &ui_app.fm.mono.normal;
    panel_border = rt_max(1, v->fm->em.h / 4);
    frame_border = rt_max(1, v->fm->em.h / 8);
    rt_assert(panel_border > 0 && frame_border > 0);
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

static void layout(ui_view_t* rt_unused(view)) {
    rt_assert(view->fm->em.w > 0 && view->fm->em.h > 0);
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
    rt_assert(top > 0);
    top--;
    sx = stack[top].x;
    sy = stack[top].y;
    zoom *= 2;
}

static void zoom_in(int x, int y) {
    rt_assert(top < rt_countof(stack));
    stack[top].x = sx;
    stack[top].y = sy;
    top++;
    zoom /= 2;
    sx += zoom * x / image.w;
    sy += zoom * y / image.h;
}

static bool tap(ui_view_t* rt_unused(v), int32_t ix, bool pressed) {
    const bool inside = ui_view.inside(&panel_center, &ui_app.mouse);
    if (pressed && inside) {
        int x = ui_app.mouse.x - (panel_center.w - image.w) / 2 - panel_center.x;
        int y = ui_app.mouse.y - (panel_center.h - image.h) / 2 - panel_center.y;
        if (0 <= x && x < image.w && 0 <= y && y < image.h) {
            if (pressed && ix == 2) {
                if (zoom < 1) { zoom_out(); refresh(); }
            } else if (pressed && ix == 0) {
                if (top < rt_countof(stack)) { zoom_in(x, y); refresh(); }
            }
        }
        ui_app.request_redraw();
    }
    return pressed && inside;
}

static void slider_format(ui_view_t* v) {
    ui_slider_t* slider = (ui_slider_t*)v;
    ui_view.set_text(v, "%s", rt_str.uint64(slider->value));
}

static void zoomer_callback(ui_view_t* v) {
    ui_slider_t* slider = (ui_slider_t*)v;
    fp64_t z = 1;
    for (int i = 0; i < slider->value; i++) { z /= 2; }
    while (zoom > z) { zoom_in(image.w / 2, image.h / 2); }
    while (zoom < z) { zoom_out(); }
    refresh();
}

static void mouse_scroll(ui_view_t* unused, ui_point_t dx_dy) {
    (void)unused;
    if (!scroll.state.pressed) { dx_dy.y = -dx_dy.y; }
    if (!scroll.state.pressed) { dx_dy.x = -dx_dy.x; }
    sx = sx + zoom * dx_dy.x / image.w;
    sy = sy + zoom * dx_dy.y / image.h;
    refresh();
}

static void character(ui_view_t* view, const char* utf8) {
    char ch = utf8[0];
    if (ch == 'q' || ch == 'Q') {
        ui_app.close();
    } else if (ch == 033 && ui_app.is_full_screen) {
        flip_full_clicked(&button_full_screen);
    } else if (ch == '+' || ch == '=') {
        zoom /= 2; refresh();
    } else if (ch == '-' || ch == '_') {
        zoom = rt_min(zoom * 2, 1.0); refresh();
    } else if (ch == '<' || ch == ',') {
        ui_point_t pt = {+image.w / 8, 0};
        mouse_scroll(view, pt);
    } else if (ch == '>' || ch == '.') {
        ui_point_t pt = {-image.w / 8, 0};
        mouse_scroll(view, pt);
    } else if (ch == 3) { // Ctrl+C
        rt_clipboard.put_image(&image);
    }
}

static bool keyboard(ui_view_t* view, int64_t vk) {
    bool swallow = true;
    if (vk == ui.key.up) {
        ui_point_t pt = {0, +image.h / 8};
        mouse_scroll(view, pt);
    } else if (vk == ui.key.down) {
        ui_point_t pt = {0, -image.h / 8};
        mouse_scroll(view, pt);
    } else if (vk == ui.key.left) {
        ui_point_t pt = {+image.w / 8, 0};
        mouse_scroll(view, pt);
    } else if (vk == ui.key.right) {
        ui_point_t pt = {-image.w / 8, 0};
        mouse_scroll(view, pt);
    } else {
        swallow = false;
    }
    return swallow;
}

static void init_panel(ui_view_t* panel, const char* text, ui_color_t color,
        void (*paint)(ui_view_t*)) {
    ui_view.set_text(panel, "%s", text);
    panel->color = color;
    panel->paint = paint;
}

static void opened(void) {
    ui_app.content->measure      = measure;
    ui_app.content->layout       = layout;
    ui_app.content->character    = character;
    ui_app.content->key_pressed  = keyboard; // virtual_keys
    ui_app.content->mouse_scroll = mouse_scroll;
    panel_center.tap = tap;
    int n = rt_countof(pixels);
    static_assert(sizeof(pixels[0][0]) == 4, "4 bytes per pixel");
    static_assert(rt_countof(pixels) == rt_countof(pixels[0]), "square");
    ui_gdi.bitmap_init(&image, n, n, (int32_t)sizeof(pixels[0][0]), (uint8_t*)pixels);
    init_panel(&panel_top,    "top",    ui_colors.orange, panel_paint);
    init_panel(&panel_center, "center", ui_colors.white, center_paint);
    init_panel(&panel_bottom, "bottom", ui_colors.tone_blue, panel_paint);
    init_panel(&panel_right,  "right",  ui_colors.tone_green, right_paint);
    panel_right.layout = right_layout;
    label_single_line.highlightable = true;
    label_single_line.flat = true;
    label_multiline.highlightable = true;
    rt_str_printf(label_multiline.hint, "%s",
        "Ctrl+C or Right Mouse click to copy text to clipboard");
    ui_view.set_text(&label_multiline, "%s", rt_nls.string(str_help, ""));
    button_locale.shortcut = 'l';
    button_full_screen.shortcut = 'f';
#ifdef SAMPLE9_USE_STATIC_UI_VIEW_MACROS
    ui_slider_init(&zoomer, "Zoom: 1 / (2^%d)", 7.0, 0, rt_countof(stack) - 1,
        zoomer_callback);
#else
    zoomer = (ui_slider_t)ui_slider("Zoom: 1 / (2^%d)", 7.0, 0, rt_countof(stack) - 1,
        slider_format, zoomer_callback);
#endif
    rt_str_printf(button_mbx.hint, "Show Yes/No message box");
    rt_str_printf(button_about.hint, "Show About message box");
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
    rt_assert(0 <= v && v <= 1);
    return v * (hi - lo) + lo;
}

static void mandelbrot(ui_bitmap_t* im) {
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
                ui_color_rgb( 66,  30,  15),  ui_color_rgb( 25,   7,  26),
                ui_color_rgb(  9,   1,  47),  ui_color_rgb(  4,   4,  73),
                ui_color_rgb(  0,   7, 100),  ui_color_rgb( 12,  44, 138),
                ui_color_rgb( 24,  82, 177),  ui_color_rgb( 57, 125, 209),
                ui_color_rgb(134, 181, 229),  ui_color_rgb(211, 236, 248),
                ui_color_rgb(241, 233, 191),  ui_color_rgb(248, 201,  95),
                ui_color_rgb(255, 170,   0),  ui_color_rgb(204, 128,   0),
                ui_color_rgb(153,  87,   0),  ui_color_rgb(106,  52,   3)
            };
            ui_color_t color = palette[iteration % rt_countof(palette)];
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
    zoomer.value = rt_min(zoomer.value, zoomer.value_max);
    mandelbrot(&image);
    ui_app.request_redraw();
}

