#include "ut/ut.h"
#include "ui/ui.h"
#include "rocket.h"
#include "groot.h"
#include "stb_image.h"
#include "ui_iv.h"

static ui_image_t image_rgba;
static ui_image_t image_rgb;

static ui_view_t  list         = ui_view(list);
static ui_label_t label_left   = ui_label(0, "Left");
static ui_label_t label_right  = ui_label(0, "Right");
static ui_label_t label_top    = ui_label(0, "Top");
static ui_label_t label_bottom = ui_label(0, "Bottom");

static ui_view_t  view_image   = ui_view(stack);
static ui_iv_t    iv;

static ui_label_t button_right = ui_button("&Button", 0, null);

static void init_image(ui_image_t* image, const uint8_t* data, int64_t bytes);
static void opened(void);

static void panel_paint(ui_view_t* v) {
//  ui_gdi.fill(v->x, v->y, v->w, v->h, ui_colors.gray);
//  ui_gdi.frame(v->x + 0, v->y + 0, v->w + 1, v->h + 1, ui_colors.onyx);
    ui_gdi.frame(v->x + 1, v->y + 1, v->w - 1, v->h - 1, ui_colors.black);
}

static void view_image_measure(ui_view_t* v) {
    v->w = image_rgba.w * 2;
    v->h = image_rgba.h * 2;
}

static void view_image_paint(ui_view_t* v) {
    ui_gdi.alpha(v->x, v->y, image_rgba.w * 2, image_rgba.h * 2, &image_rgba, 0.99);
}

static ui_view_t* align(ui_view_t* v, int32_t align) {
    v->align = align;
    return v;
}

static void opened(void) {
    ui_iv_init(&iv);
    static ui_view_t  top    = ui_view(stack);
    static ui_view_t  center = ui_view(span);
    static ui_view_t  left   = ui_view(list);
    static ui_view_t  right  = ui_view(list);
    static ui_view_t  bottom = ui_view(stack);
    static ui_view_t  spacer = ui_view(spacer);
    static ui_view_t* panels[] = { &top, &left, &right, &bottom  };
    ui_view.add(&left,   align(&iv.view, ui.align.left), null);
    ui_view.add(&right,  &label_right, &spacer, &view_image, &button_right, null);
    ui_view.add(&top,    &label_top,    null);
    ui_view.add(&bottom, &label_bottom, null);
    ui_view.add(&center,
                align(&left,  ui.align.top),
                align(&right, ui.align.top),
                null);
    left.max_h  = ui.infinity;
    left.max_w  = ui.infinity;
    ui_view.add(ui_app.content,
        ui_view.add(&list,
            align(&top,    ui.align.center),
            align(&center, ui.align.left),
            align(&bottom, ui.align.center),
            null
        ),
        null
    );
//  list.debug.paint.margins = true;
    list.max_w  = ui.infinity;
    list.max_h  = ui.infinity;
    center.insets  = (ui_margins_t){0};
    center.padding = (ui_margins_t){0};
    for (int32_t i = 0; i < ut_countof(panels); i++) {
        panels[i]->paint = panel_paint;
        panels[i]->insets  = (ui_margins_t){0};
        panels[i]->padding = (ui_margins_t){0};
    }
    view_image.measure = view_image_measure;
    view_image.paint   = view_image_paint;
    list.background_id = ui_color_id_window;
    list.debug.id = "#list";
    ui_view_for_each(&list, it, {
//      it->debug.paint.margins = true;
        it->max_w   = ui.infinity;
    });
    ui_view_for_each(&center, it, {
//      it->debug.paint.margins = true;
        it->max_h   = ui.infinity;
    });
    center.max_h = ui.infinity;
    iv.view.max_h = ui.infinity;
    iv.view.max_w = ui.infinity;
    iv.image.pixels = image_rgb.pixels;
    iv.image.w = image_rgb.w;
    iv.image.h = image_rgb.h;
    iv.image.c = image_rgb.bpp;
    iv.image.s = image_rgb.stride;
}

static void closed(void) {
    ui_view.disband(ui_app.content);
    ui_iv_fini(&iv);
}

static void init(void) {
    init_image(&image_rgb, rocket, ut_countof(rocket));
    init_image(&image_rgba, groot, ut_countof(groot));
}

static void fini(void) {
    // not strictly necessary on exit:
    ui_gdi.image_dispose(&image_rgb);
    ui_gdi.image_dispose(&image_rgba);
}

ui_app_t ui_app = {
    .class_name = "groot",
    .title = "Groot",
    .dark_mode = true,
    .init = init,
    .fini = fini,
    .opened = opened,
    .closed = closed,
    .window_sizing = { // inches
        .min_w = 4.0f,
        .min_h = 2.0f,
        .ini_w = 11.0f,
        .ini_h = 7.0f
    }
};

static void* load_image(const uint8_t* data, int64_t bytes, int32_t* w, int32_t* h,
    int32_t* bpp, int32_t preferred_bytes_per_pixel) {
    void* pixels = stbi_load_from_memory((uint8_t const*)data, (int32_t)bytes, w, h,
        bpp, preferred_bytes_per_pixel);
    return pixels;
}

static void init_image(ui_image_t* image, const uint8_t* data, int64_t bytes) {
    int32_t w = 0;
    int32_t h = 0;
    int32_t c = 0;
    void* pixels = load_image(data, bytes, &w, &h, &c, 0);
    ut_not_null(pixels);
    ui_gdi.image_init(image, w, h, c, pixels);
    stbi_image_free(pixels);
}

#if 0
#define TITLE "Groot"

enum {
    WIDTH = 640,
    HEIGHT = 480
};

static ui_image_t image_rgba;
static ui_image_t image_rgb;

static int32_t panel_border;
static int32_t frame_border;

static ui_button_t btn_flip; // frames row[0] and row[1]
static ui_slider_t rsc_alpha;
static ui_toggle_t cbx_on_off;
static ui_button_t btn_open; // file open dialog
static ui_button_t btn_tst;  // show Toast
static ui_button_t btn_mbx;  // show MessageBox
static ui_label_t  txt_sl;   // single line static text
static ui_label_t  txt_ml;   // multiline static text
static ui_label_t  tst_txt;  // "Lorem ipsum" text toast
static ui_label_t  tst_fn;   // filename text toast
static ui_mbx_t    mbx;      // message box

static struct {
    ui_view_t top;
    ui_view_t center;
    ui_view_t right;
    ui_view_t bottom;
} panels;

static ui_view_t* ui_children[] = { // root v children
    &panels.top,
    &panels.center,
    &panels.right,
    &panels.bottom,
    null
};

static ui_view_t ui_frames[4];

static ui_view_t* center_children[] = {
    &ui_frames[0],
    &ui_frames[1],
    &ui_frames[2],
    &ui_frames[3],
    null
};

static ui_view_t* right_children[] = {
    &btn_flip,
    &cbx_on_off,
    &rsc_alpha.view,
    &btn_open,
    &btn_tst,
    &btn_mbx,
    &txt_sl,
    &txt_ml,
    null
};

static ui_timer_t app_timer;

static uint8_t greyscale[WIDTH * HEIGHT];

static void panel_paint(ui_view_t* v) {
    ui_gdi.fill(v->x, v->y, v->w, v->h, ui_colors.gray);
    if (v == &panels.right) {
        ui_gdi.line(v->x, v->y, v->x + v->w, v->y, ui_colors.red);
        ui_gdi.line(v->x + v->w, v->y, v->x + v->w, v->y + v->h, ui_colors.red);
        ui_gdi.line(v->x + v->w, v->y + v->h, v->x, v->y + v->h, ui_colors.red);
        ui_gdi.line(v->x, v->y + v->h, v->x, v->y, ui_colors.red);
    } else if (v == &panels.top || v == &panels.bottom) {
        ui_gdi.line(v->x, v->y, v->x, v->y + v->h, ui_colors.green);
        ui_gdi.line(v->x, v->y + v->h, v->x + v->w, v->y + v->h, ui_colors.green);
        ui_gdi.line(v->x + v->w, v->y, v->x, v->y, ui_colors.green);
    } else {
        ut_assert(v == &panels.center);
        ui_gdi.line(v->x, v->y, v->x, v->y + v->h, ui_colors.blue);
    }
/*
    int32_t x = v->x + panel_border + max(1, em.w / 8);
    int32_t y = v->y + panel_border + max(1, em.h / 4);
    pen_t p = CreatePen(PS_SOLID, max(1, em.h / 8), v->color);
    pen_t s = app.set_pen(p);
    app.set_brush(app.brush.hollow);
    app.rounded(x, y, em.w * 12, em.h, max(1, em.h / 4), max(1, em.h / 4));
    app.set_pen(s);
    DeletePen(p);
    color_t color = app.set_text_color(v->color);
    x = v->x + panel_border + max(1, em.w / 2);
    y = v->y + panel_border + max(1, em.h / 4);
    app.text("%d,%d %dx%d %s", v->x, v->y, v->w, v->h, v->text);
    app.set_text_color(color);
    app.set_clip(0, 0, 0, 0);
    app.pop();
*/
}

static int frame_index(ui_view_t* v) {
    for (int i = 0; i < ut_countof(ui_frames); i++) {
        if (&ui_frames[i] == v) { return i; }
    }
    ut_assert(false);
    return -1;
}

static void frame_cell(ui_view_t* v, ui_wh_t* cell) {
    int num = 1;
    int denom = 1;
    if (v->w - frame_border * 2 < WIDTH || v->h - frame_border * 2 < HEIGHT) {
        num = 1; denom = 2; // 1:2
    } else if (v->w - frame_border * 2 < WIDTH * 2 || v->h - frame_border * 2 < HEIGHT * 2) {
        num = 1; denom = 1; // 1:1
    } else {
        num = 2; denom = 1; // 2:1
    }
    cell->w = WIDTH * num / denom;
    cell->h = HEIGHT * num / denom;
}

static void frame_pos(ui_view_t* v, ui_point_t* pt, ui_wh_t* cell) {
    frame_cell(v, cell);
    pt->x = v->x + (v->w - 2 * frame_border - cell->w) / 2;
    pt->y = v->y + (v->h - 2 * frame_border - cell->h) / 2;
}

static void frame_paint(ui_view_t* v) {
    ui_point_t pt;
    ui_wh_t cell;
    frame_pos(v, &pt, &cell);
//  traceln("%d %d %d", frame_index(v), pt.x, pt.y);
//  ui_gdi.greyscale(pt.x, pt.y, cell.w, cell.h, 0, 0, WIDTH, HEIGHT, WIDTH,
//      greyscale);
/*
    x = pt.x;
    y = pt.y;
    app.set_font(app.fonts.H1);
    app.set_text_color(color_tone_red);
    app.text("%d", frame_index(v));
    int32_t x = pt.x - frame_border;
    int32_t y = pt.y - frame_border;
    app.set_pen(app.pen[frame_border].green);
    app.rect(x, y, cell.cx + frame_border * 2, cell.cy + frame_border * 2);
    app.set_clip(0, 0, 0, 0);
    app.pop();
*/
}

static void right_layout(ui_view_t* ut_unused(v)) {
#if 0
    ui_wh_t em_mono = ui_app.fm.mono.em;
    ui_wh_t em = ui_app.fm.regular.em;
    if (v->child != null) {
        int x = v->x + em.w;
        int y = v->y + em.h * 2;
        for (ui_view_t** it = v->child; *it != null; it++) {
            ui_view_t* ch = *it;
            ch->x = x;
            ch->y = y;
            y += ch->h + max(1, em.h / 2);
        }
    }
#endif
}

static void right_paint(ui_view_t* v) {
    panel_paint(v);
//  ui_wh_t em_mono = ui_app.fm.mono.em;
    ui_wh_t em = ui_app.fm.regular.em;
    int32_t x = btn_flip.x + btn_flip.w + em.w;
    int32_t y = btn_flip.y;
    const ui_gdi_ta_t ta = { .fm = v->fm, .color = ui_colors.white };
    ui_gdi.text(&ta, x, y, "&Flip %d", btn_flip.state.pressed);
    x = txt_ml.x;
    y = txt_ml.y + txt_ml.h + max(1, em.h / 4);
    ui_gdi.alpha(x, y, image_rgb.w * 4, image_rgb.h * 4,
        &image_rgb, rsc_alpha.value / 255.0);
    ui_gdi.text(&ta, x, y, "Proportional");
    ui_gdi.text(&ta, x, y, "Monospaced");
    ui_gdi.text(&ta, x, y, "H1 System");
    ui_gdi.text(&ta, x, y, "H2 Host");
    ui_gdi.text(&ta, x, y, "H3 Drive");
    ui_gdi.text(&ta, x, y, "Client area %dx%d", ui_app.crc.w, ui_app.crc.h);
    ui_gdi.text(&ta, x, y, "Window %dx%d", ui_app.wrc.w, ui_app.wrc.h);
    ui_gdi.text(&ta, x, y, "Monitor %dx%d", ui_app.mrc.w, ui_app.mrc.h);
    ui_gdi.text(&ta, x, y, "Left Top %d %d", ui_app.wrc.x, ui_app.wrc.y);
    ui_gdi.text(&ta, x, y, "Mouse %d %d", ui_app.mouse.x, ui_app.mouse.y);
    ui_gdi.text(&ta, x, y, "%d paints %.1fms (max %.1f avg %.1f)",
        ui_app.paint_count, ui_app.paint_time * 1000.0,
        ui_app.paint_max * 1000.0, ui_app.paint_avg * 1000.0);
    y += em.h;
    ui_gdi.alpha(x, y, image_rgba.w * 2, image_rgba.h * 2, &image_rgba, 0.99);
    y += image_rgba.h * 2 + em.h;
//  ui_gdi.bgr(x, y, image_rgb.w * 2, image_rgb.h * 2, &image_rgb);
}

static void flip_frames(ui_view_t* unused) { // flip row[0] row[1] layouts
    (void)unused;
    if (btn_flip.state.pressed) {
        for (int i = 0; i < 2; i++) {
            int32_t swap = ui_frames[i].h;
            ui_frames[i].h = ui_frames[i + 2].h;
            ui_frames[i + 2].h = swap;
            ui_frames[i + 2].y = ui_frames[i].y + ui_frames[i].h;
        }
    }
}

static void layout_frames(ui_view_t* v, bool down) {
//  traceln("%d x %d p %d f %d", panels.center.w, panels.center.h, panel_border, frame_border);
    int div = 2;
    int ph = panels.center.h - panel_border * 2;
    int ph2 = ph / 2;
    if (ph2 > HEIGHT * 2 + frame_border * 2) {
        div = 2;
    } else if (ph > HEIGHT + HEIGHT / 2 + frame_border * 2) {
        div = 3;
    } else if (ph2 > HEIGHT + frame_border * 2) {
        div = 2;
    } else {
        div = 3;
    }
    ui_wh_t f0;
    ui_wh_t f2;
    const int32_t fw = panels.center.w / 2 - frame_border * 2 - panel_border * 2;
    int32_t fh = ph / div;
    if (!down) {
        for (int row = 0; row < 2; row++) {
            for (int col = 0; col < 2; col++) {
                int i = row * 2 + col;
                ui_frames[i].w = fw + frame_border * 2;
                if (row == 0) {
                    ui_frames[i].h = fh;
                } else {
                    ui_frames[i].h = panels.center.h - ui_frames[0].h;
                }
            }
        }
    } else {
        int32_t y = panels.center.y + panel_border;
        for (int row = 0; row < 2; row++) {
            int32_t x = panels.center.x + frame_border + panel_border;
            for (int col = 0; col < 2; col++) {
                int i = row * 2 + col;
                ui_frames[i].x = x;
                ui_frames[i].y = y;
                x += ui_frames[i].w + frame_border * 2;
            }
            y += ui_frames[0].h;
        }
    }
    frame_cell(&ui_frames[0], &f0);
    frame_cell(&ui_frames[2], &f2);
    if (div == 3 && f0.h == f2.h) {
        fh = ph / 2;
        if (!down) {
            for (int i = 0; i < countof(ui_frames); i++) { ui_frames[i].h = fh; }
        } else {
            for (int i = 2; i < countof(ui_frames); i++) {
                ui_frames[i].y = ui_frames[i - 2].y + fh + 1;
            }
        }
    }
    if (down) {
        flip_frames(v);
    }
//  for (int row = 0; row < 2; row++) {
//      for (int col = 0; col < 2; col++) {
//          int i = row * 2 + col;
//          traceln("[%d] %d,%d %dx%d", i, ui_frames[i].x, ui_frames[i].y, ui_frames[i].w, ui_frames[i].h);
//      }
//  }
}

static void measure(ui_view_t* v) {
    ui_wh_t em_mono = ui_app.fm.mono.em;
    ui_wh_t em = ui_app.fm.regular.em;
    panel_border = max(1, em_mono.h / 4);
    frame_border = max(1, em_mono.h / 8);
    ut_assert(panel_border > 0 && frame_border > 0);
    const int32_t w = ui_app.crc.w;
    const int32_t h = ui_app.crc.h;
    // measure elements
    panels.top.w = (int32_t)(0.80 * w);
    panels.top.h = em.h * 2;
    panels.bottom.w = panels.top.w;
    panels.bottom.h = em.h * 2;
    panels.right.w = w - panels.bottom.w;
    panels.right.h = h;
    panels.center.w = panels.bottom.w;
    panels.center.h = h - panels.bottom.h - panels.top.h;
    layout_frames(v, false);
}

static void layout(ui_view_t* v) {
    const int32_t w = ui_app.crc.w;
    const int32_t h = ui_app.crc.h;
    // position v elements
    panels.top.x = 0;
    panels.top.y = 0;
    panels.bottom.x = 0;
    panels.bottom.y = h - panels.bottom.h;
    panels.right.x = panels.bottom.w;
    panels.right.y = 0;
    panels.center.x = 0;
    panels.center.y = panels.top.h;
    layout_frames(v, true);
}

static void key_down(ui_view_t* unused, int32_t key) {
    (void)unused; (void)key;
//  traceln("0x%04X (%d) %c", key, key, 0x20 <= key && key < 128 ? key : 0x20);
}

static void key_up(ui_view_t* unused, int32_t key) {
    (void)unused; (void)key;
//  traceln("0x%04X (%d) %c", key, key, 0x20 <= key && key < 128 ? key : 0x20);
}

static void keyboard(ui_view_t* unused, int32_t ch) {
    (void)unused;
//  traceln("0x%04X (%d) %c", ch, ch, 0x20 <= ch && ch < 128 ? ch : 0x20);
    ui_app.request_redraw();
    if (ch == 'q' || ch == 'Q') { ui_app.close(); }
    if (ch == '1') { ui_app.set_timer((uintptr_t)&app_timer, 1000); }
    if (ch == '5') { ui_app.set_timer((uintptr_t)&app_timer, 5000); }
}

static bool message(ui_view_t* unused, int32_t m, int64_t wp, int64_t lp,
        int64_t *rt) { (void)unused; (void)m; (void)wp; (void)lp; (void)rt;
    //  traceln("0x%04X %016llX (%lld) %016llX (%lld)", m, wp, wp, lp, lp);
    return false;
}

static void mouse(ui_view_t* unused, int32_t m, int32_t flags) {
    (void)unused; (void)m; (void)flags;
//  traceln("0x%04X %08X %d %d", m, flags, app.mouse.x, app.mouse.y);
}

static void timer(ui_view_t* unused, ui_timer_t id) {
    (void)unused; (void)id;
//  traceln("id=0x%016llX", id);
}

static void once_upon_a_second(ui_view_t* unused) {
    (void)unused;
//  log_info(".....");
}

static void periodically(ui_view_t* unused) {
    (void)unused;
//  log_info(".");
}

static void btn_open_cb(ui_view_t* unused) {
    (void)unused;
/*
    // TODO: move to v? host inside toast-like panel?
    char path[MAX_PATH] = { 0 };
    OPENFILENAMEA ofn = { sizeof(ofn) };
    ofn.hwndOwner = app.window;
    ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;
    ofn.lpstrFilter = "*\0\0";
    ofn.lpstrInitialDir = "C:\\";
    ofn.lpstrFile = path;
    ofn.nMaxFile = sizeof(path);
    if (GetOpenFileNameA(&ofn) && path[0] != 0) {
        strncpy0(tst_fn.v.text, path, countof(tst_fn.v.text));
        app.show_toast(&tst_fn.v, 2.0);
    }
*/
}

static void btn_toast_cb(ui_view_t* unused) {
    (void)unused;
    ui_app.show_toast(&tst_txt, 10.0);
}

static void btn_mbx_cb(ui_view_t* unused) {
    (void)unused;
    ui_app.show_toast(&mbx, 0);
}

static void btn_cb(ui_view_t* b) {
    if (b == &btn_flip) {
        btn_flip.state.pressed = !btn_flip.state.pressed;
        ui_app.request_layout(); // because center panel layout changed
    }
//  traceln("%s pressed=%d", b->v.text, b->v.pressed);
}

static void cbx_cb(ui_toggle_t* unused) {
    (void)unused;
//  traceln("%s on=%d", c->v.text, c->v.pressed);
}

static void rsc_cb(ui_slider_t* unused) {
    (void)unused;
//  traceln("%d in [%d..%d]", r->v.value, r->v.vmin, r->v.vmax);
    ui_app.request_redraw();
}

static void mbx_cb(ui_mbx_t* unused, int option) {
    (void)unused; (void)option;
//  traceln("message box option %d", option);
}

#define glyph_full_block "\xE2\x96\x88"

static void opened() {
    #ifdef LOAD_FROM_RESOURCES
    app.image_load(&image_rgb, "rocket");
    app.image_load(&image_rgba, "groot");
    #else
    image_decode(&image_rgb, rocket, countof(rocket));
    image_decode(&image_rgba, groot, countof(groot));
    #endif
/*
    panels.top.tag = ui_tag_cnt;
    panels.top.paint = panel_paint;
    strncpy0(panels.top.text, "top", countof(panels.top.text));
    panels.top.color = RGB(255, 192, 192);

    panels.center.tag = ui_tag_cnt;
    strncpy0(panels.center.text, "center", countof(panels.center.text));
    panels.center.color = RGB(200, 200, 200);
    panels.center.children = center_children;

    panels.bottom.tag = ui_tag_cnt;
    panels.bottom.paint = panel_paint;
    strncpy0(panels.bottom.text, "bottom", countof(panels.bottom.text));
    panels.bottom.color = RGB(192, 192, 255);

    panels.right.tag = ui_tag_cnt;
    strncpy0(panels.right.text, "right", countof(panels.right.text));
    panels.right.color = RGB(192, 255, 192);
    panels.right.paint = right_paint;
    panels.right.children = right_children;
    panels.right.layout = right_layout;

    panels.center.paint = panel_paint;
*/
    for (int i = 0; i < countof(ui_frames); i++) {
        ui_frames[i].paint = frame_paint;
    }
    btn_init(&btn_open, "&Open", 7.5, btn_open_cb);
    btn_init(&btn_tst, "&Toast", 7.5, btn_toast_cb);
    btn_init(&btn_mbx, "&Message Box", 7.5, btn_mbx_cb);
    txt_init(&txt_sl, "Sample Text");
    txt_sl.highlightable = true;
    txt_init_ml(&txt_ml, 12.34, "Big brown fox jumps over the lazy dog.");
    txt_ml.highlightable = true;
    cbx_init(&cbx_on_off, "&Checkbox ___", 3.5, cbx_cb);
    rsc_init(&rsc_alpha, "Transparency: %03d", 3.1, 0, 255, rsc_cb);
    rsc_alpha.value = 128;

    txt_init(&tst_fn, "filename placeholder");
    tst_fn.fm = &ui_app.fm.H1;

    txt_init_ml(&tst_txt, 34.56, "Lorem ipsum dolor sit amet, consectetur "
        "adipiscing elit, sed do eiusmod tempor incididunt ut labore et "
        "dolore magna aliqua. Ut enim ad minim veniam, quis nostrud "
        "exercitation ullamco laboris nisi ut aliquip ex ea commodo "
        "consequat.\n\n"
        "Translation:\n"
        "\"Press ESC or click the \xC3\x97 button in right top corner to stop "
        "reading this gibberish or just wait - it will disappear soon\".");
    tst_txt.fm = &ui_app.fm.H3;

    const char* yes_no[] = { "&Yes", "&No", null };
    mbx_init(&mbx, yes_no, mbx_cb,
        "\"Pneumonoultramicroscopicsilicovolcanoconiosis\"\n"
        "is it the longest English language word or not?");

//  btn_init(&btn_flip, "\xE2\x86\x9D", 1, &btn_cb);
//  btn_init(&btn_flip, "\xE2\xA7\x89", 1.75, btn_cb); // "Two Joined Squares"
    btn_init(&btn_flip, "\xE2\x86\xAF", 1, &btn_cb); // "Downward Zigzag Arrow"
    btn_flip.shortcut = 'f';
    app_timer = ui_app.set_timer((uintptr_t)&app_timer, 1000);
//  traceln("timer=0x%016llX", app_timer);
}

static void closed() {
    ui_gdi.image_dispose(&image_rgb);
    ui_gdi.image_dispose(&image_rgba);
    ui_app.kill_timer(app_timer);
}

static void init() {
    ui_app.title = TITLE;
    ui_app.content->measure = measure;
    ui_app.content->layout = layout;
    ui_app.content->paint = null;
    ui_app.content->timer = timer;
    ui_app.content->every_sec   = once_upon_a_second;
    ui_app.content->every_100ms = periodically;
    ui_app.content->message = message;
    ui_app.content->tap = mouse;
    ui_app.content->character = keyboard;
    ui_app.content->key_pressed = key_down;
    ui_app.content->key_released = key_up;
    ui_app.opened = opened;
    ui_app.closed = closed;
//  ui_app.content->children = ui_children;
/*
    ui_app.min_width = 1800;
    ui_app.min_height = 1020;
    // do not startup minimized
    if (app.last_show != show_window_min) {
        app.show = app.last_show;
    } else {
        app.show = show_window_default;
        app.last_show = show_window_default;
    }
    for (int y = 0; y < HEIGHT; y++) {
        for (int x = 0; x < WIDTH; x++) {
            greyscale[y * WIDTH + x] = ((x / 8) % 2) == ((y / 8) % 2) ? 0xFF : 0x00;
        }
    }
*/
    // for visual check of upsidedown or left-right mirrored images:
    for (int y = 0; y < 480; y++) {
        for (int x = 0; x < y; x++) {
            greyscale[y * WIDTH + x] = 0xFF;
        }
    }
}

ui_app_t ui_app = {
    .class_name = "groot",
    .dark_mode = true,
    .init = init,
    .window_sizing = {
        .min_w = 11.0f, // inches
        .min_h = 7.0f,
        .ini_w = 11.0f, // inches
        .ini_h = 7.0f
    }
};
#endif