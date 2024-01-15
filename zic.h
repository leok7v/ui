#ifndef zic_defintion
#define zic_defintion
#include "crt.h"
#include "quick/quick.h"

begin_c

// "zic" stands for "zoom/pan image coontainer"

enum {
    uic_tag_zic  = 'zic'
};

typedef struct zic_s zic_t;

typedef struct zic_s {
    uic_t ui;
    // image to display:
    const uint8_t* pixels;
    int32_t w; // width
    int32_t h; // height
    int32_t c; // 1, 3, 4
    int32_t s; // stride - usually w * c but may differ
    // actual zoom: z = 2 ^ (zn - 1) / 2 ^ (zd - 1)
    int32_t value; // 0..8
    // 0=16:1 1=8:1 2=4:1 3=2:1 4=1:1 5=1:2 6=1:4 7=1:8 8=1:16
    int32_t zn; // zoom nominator (1, 2, 3, ...)
    int32_t zd; // zoom denominator (1, 2, 3, ...)
    double  sx; // shift x [0..1.0] in view coordinates
    double  sy; // shift y [0..1.0]
    double    (*scale)(zic_t* zic); // 2 ^ (zn - 1) / 2 ^ (zd - 1)
    ui_rect_t (*position)(zic_t* zic);
    uic_t  toolbar;
    // toolbar.children[] = tools = {zoom in, zoom 1:1, zoom out, help, null}
    uic_t* tools[6];
    // buttons only visible when focused
    uic_button_t copy; // copy image to clipboard
    uic_button_t zoom_in;
    uic_button_t zoom_1t1; // 1:1
    uic_button_t zoom_out;
    uic_button_t help;
    // zic.children[] = { toolbar, null }
    uic_t* children[2];
    ui_point_t drag_start;
    font_t buttons_tiny_font;
    font_t buttons_font;
} zic_t;

void zic_init(zic_t* zic);
void zic_fini(zic_t* zic);

end_c

#endif zic_defintion

#if defined(zic_implementation)
#undef zic_implementation

begin_c

static double zic_scaleof(int32_t nominator, int32_t denominator) {
    const int32_t zn = 1 << (nominator - 1);
    const int32_t zd = 1 << (denominator - 1);
    return (double)zn / (double)zd;
}

static double zic_scale(zic_t* zic) {
    return zic_scaleof(zic->zn, zic->zd);
}

static bool zic_is_focused(zic_t* zic) {
    uic_t* focus = app.focus;
    while (focus != null && focus != &zic->ui) { focus = focus->parent; }
    return focus == &zic->ui;
}

static ui_rect_t zic_position(zic_t* zic) {
    ui_rect_t rc = { 0, 0, 0, 0 };
    if (zic->pixels != null) {
        int32_t iw = zic->w;
        int32_t ih = zic->h;
        // zoomed image width and height
        rc.w = iw * (1 << (zic->zn - 1)) / (1 << (zic->zd - 1));
        rc.h = ih * (1 << (zic->zn - 1)) / (1 << (zic->zd - 1));
        int32_t shift_x = (int32_t)((rc.w - zic->ui.w) * zic->sx);
        int32_t shift_y = (int32_t)((rc.h - zic->ui.h) * zic->sy);
        // shift_x and shift_y are in zoomed image coordinates
        rc.x = zic->ui.x - shift_x; // screen x
        rc.y = zic->ui.y - shift_y; // screen y
    }
    return rc;
}

static void zic_paint(uic_t* ui) {
    assert(ui->tag == uic_tag_zic);
    assert(!ui->hidden);
    zic_t* zic = (zic_t*)ui;
    brush_t b = gdi.set_brush(gdi.brush_color);
    gdi.set_brush_color(colors.black);
    gdi.fill(ui->x, ui->y, ui->w, ui->h);
    gdi.set_brush(b);
    if (zic->pixels != null) {
        gdi.push(gdi.x, gdi.y); // including clip
        gdi.set_clip(ui->x, ui->y, ui->w, ui->h);
        assert(0 < zic->zn && zic->zn <= 16);
        assert(0 < zic->zd && zic->zd <= 16);
        // only 1:2 and 2:1 etc are supported:
        if (zic->zn != 1) { assert(zic->zd == 1); }
        if (zic->zd != 1) { assert(zic->zn == 1); }
        const int32_t iw = zic->w;
        const int32_t ih = zic->h;
        ui_rect_t rc = zic_position(zic);
        if (zic->c == 1) {
            gdi.draw_greyscale(rc.x, rc.y, rc.w, rc.h,
                0, 0, iw, ih,
                iw, ih, zic->s, zic->pixels);
        } else if (zic->c == 3) {
            gdi.draw_bgr(rc.x, rc.y, rc.w, rc.h,
                         0, 0, iw, ih,
                         iw, ih, zic->s, zic->pixels);
        } else if (zic->c == 4) {
            gdi.draw_bgrx(rc.x, rc.y, rc.w, rc.h,
                          0, 0, iw, ih,
                          iw, ih, zic->s, zic->pixels);
        } else {
            assert(false, "unsupported .c: %d", zic->c);
        }
        if (zic_is_focused(zic)) {
            b = gdi.set_brush(gdi.brush_hollow);
            pen_t p = gdi.set_colored_pen(colors.btn_hover_highlight);
            gdi.rect(ui->x, ui->y, ui->w, ui->h);
            gdi.set_pen(p);
            gdi.set_brush(b);
        }
        gdi.pop();
    }
}

static void zic_hide_toolbar(zic_t* zic, bool hide) {
    if (zic->toolbar.hidden != hide) {
        zic->toolbar.hidden   = hide;
        zic->toolbar.disabled = hide;
        for (uic_t** c = zic->toolbar.children; c != null && *c != null; c++) {
            (*c)->pressed  = false;
        }
        app.layout(); // request layout (will request redraw)
    }
}

static void zic_focus(zic_t* zic, bool focused) {
    if (zic_is_focused(zic) != focused) {
        zic_hide_toolbar(zic, !focused);
        app.focus = focused ? &zic->ui : null;
    }
}

static void zic_enable_zoom(zic_t* zic) {
    double scale = zic->scale(zic);
    // whole image is visible
    bool whole = (int32_t)(zic->w * scale) <= zic->ui.w &&
                 (int32_t)(zic->h * scale) <= zic->ui.h;
    zic->zoom_out.ui.disabled = whole;
    zic->zoom_1t1.ui.disabled = zic->value == 4;
}

static void zic_measure(uic_t* ui) {
    assert(ui->tag == uic_tag_zic);
    zic_t* zic = (zic_t*)ui;
    measurements.horizontal(&zic->toolbar, 0);
}

static void zic_layout(uic_t* ui) {
    assert(ui->tag == uic_tag_zic);
    zic_t* zic = (zic_t*)ui;
    int32_t x = ui->x + ui->w - zic->toolbar.w - ui->em.x / 8;
    layouts.horizontal(&zic->toolbar, x, ui->y + ui->em.y / 8, 0);
    zic_enable_zoom(zic);
}

static void zic_every_100ms(uic_t* ui) {
    assert(ui->tag == uic_tag_zic);
    zic_t* zic = (zic_t*)ui;
//  traceln("%p zic_is_focused: %d zic->toolbar.hidden: %d",
//      zic, zic_is_focused(zic), zic->toolbar.hidden);
    if (!zic_is_focused(zic) && !zic->toolbar.hidden) {
        zic_hide_toolbar(zic, true);
    }
    zic_enable_zoom(zic);
}

static void zic_zoomed(zic_t* zic) {
    // 0=16:1 1=8:1 2=4:1 3=2:1 4=1:1 5=1:2 6=1:4 7=1:8 8=1:16
    int32_t n  = zic->value - 4;
    int32_t zn = zic->zn;
    int32_t zd = zic->zd;
    double scale_before = zic->scale(zic);
    // whole image is visible
    bool whole = (int32_t)(zic->w * scale_before) <= zic->ui.w &&
                 (int32_t)(zic->h * scale_before) <= zic->ui.h;
    if (n > 0 && !zic->zoom_in.ui.disabled) {
        zn = n + 1;
        zd = 1;
    } else if (n < 0 && !zic->zoom_out.ui.disabled) {
        zn = 1;
        zd = -n + 1;
    } else if (n == 0) {
        zn = 1;
        zd = 1;
    }
    double scale_after = zic_scaleof(zn, zd);
    if (!whole || scale_after >= scale_before) {
        zic->zn = zn;
        zic->zd = zd;
        const int32_t nm = 1 << (zic->zn - 1);
        const int32_t dm = 1 << (zic->zd - 1);
        strprintf(zic->zoom_1t1.ui.text, "%d:%d", nm, dm);
    }
    if (zic->zn == 1) {
        zic->value = 4 - (zic->zd - 1);
    } else if (zic->zd == 1) {
        zic->value = 4 + (zic->zn - 1);
    } else {
        assert(false);
    }
    double s = zic->scale(zic);
    whole = (int32_t)(zic->w * s) <= zic->ui.w &&
            (int32_t)(zic->h * s) <= zic->ui.h;
    if (whole) { zic->sx = 0.5; zic->sy = 0.5; }
    zic_enable_zoom(zic);
    zic->ui.invalidate(&zic->ui);
}

static void zic_mousewheel(uic_t* ui, int32_t dx, int32_t dy) {
    assert(ui->tag == uic_tag_zic);
    zic_t* zic = (zic_t*)ui;
    if (zic_is_focused(zic)) {
        double s = zic->scale(zic);
        if (zic->w * s > zic->ui.w || zic->h * s > zic->ui.h) {
            zic->sx = max(0.0, min(zic->sx + (double)dx / zic->w, 1.0));
        } else {
            zic->sx = 0.5;
        }
        if (zic->h * s > zic->ui.h) {
            zic->sy = max(0.0, min(zic->sy + (double)dy / zic->h, 1.0));
        } else {
            zic->sy = 0.5;
        }
        zic->ui.invalidate(&zic->ui);
    }
}

static void zic_mouse(uic_t* ui, int m, int unused(flags)) {
    assert(ui->tag == uic_tag_zic);
    zic_t* zic = (zic_t*)ui;
    const int32_t x = app.mouse.x - zic->ui.x;
    const int32_t y = app.mouse.y - zic->ui.y;
    bool inside = 0 <= x && x < ui->w && 0 <= y && y < ui->h;
    bool left  = m == messages.left_button_pressed;
    bool right = m == messages.right_button_pressed;
    if ((left || right)) { zic_focus(zic, inside); }
    bool drag_started = zic->drag_start.x >= 0 && zic->drag_start.y >= 0;
    if (left && inside && !drag_started) {
        zic->drag_start = (ui_point_t){x, y};
    }
    if (m == messages.left_button_released && drag_started) {
        zic_mousewheel(ui, zic->drag_start.x - x, zic->drag_start.y - y);
    }
    if (m == messages.left_button_released) {
        zic->drag_start = (ui_point_t){-1, -1};
    }
}

static void zic_key_pressed(uic_t* ui, int32_t vk) {
    zic_t* zic = (zic_t*)ui;
    assert(zic->ui.tag == uic_tag_zic);
    if (zic_is_focused(zic)) {
        if (vk == virtual_keys.up) {
            zic_mousewheel(ui, 0, -zic->ui.h / 8);
        } else if (vk == virtual_keys.down) {
            zic_mousewheel(ui, 0, +zic->ui.h / 8);
        } else if (vk == virtual_keys.left) {
            zic_mousewheel(ui, -zic->ui.w / 8, 0);
        } else if (vk == virtual_keys.right) {
            zic_mousewheel(ui, +zic->ui.w / 8, 0);
        }
    }
}

static void zic_zoom_in(uic_button_t* b) {
    zic_t* zic = (zic_t*)b->ui.that;
    assert(zic->ui.tag == uic_tag_zic);
    if (!zic->zoom_in.ui.disabled && zic->value < 8) {
        zic->value++;
        zic_zoomed(zic);
    }
}

static void zic_zoom_out(uic_button_t* b) {
    zic_t* zic = (zic_t*)b->ui.that;
    assert(zic->ui.tag == uic_tag_zic);
    if (!zic->zoom_out.ui.disabled && zic->value > 0) {
        zic->value--;
        zic_zoomed(zic);
    }
}

static void zic_zoom_1t1(uic_button_t* b) {
    zic_t* zic = (zic_t*)b->ui.that;
    assert(zic->ui.tag == uic_tag_zic);
    zic->value = 4;
    zic_zoomed(zic);
}

#define glyph_left        "\xE2\x86\x90" // "ShortLeftArrow"
#define glyph_up          "\xE2\x86\x91" // "ShortUpArrow"
#define glyph_right       "\xE2\x86\x92" // "ShortRightArrow"
#define glyph_down        "\xE2\x86\x93" // "ShortDownArrow"
// telephone_recorder is magnifying glass shaped
#define glyph_loupe       "\xE2\x8C\x95"
#define glyph_heavy_plus_sign    "\xE2\x9E\x95"
#define glyph_heavy_minus_sign   "\xE2\x9E\x96"
// none of empty set and slashed zero or capital letter O with a stroke
// look like a good slashed zero:
#define glyph_empty_set   "\xE2\x88\x85"
#define glyph_latin_capital_letter_O_with_stroke "\xC3\x98"

static void zic_zoom_in_out_button_paint(zic_t* zic, uic_t* ui, bool in) {
    gdi.push(gdi.x, gdi.y);
    ui_point_t m = gdi.measure_text(zic->buttons_font, glyph_loupe);
    int sx = ui->w / 16; // small shift of x to the left
    int sy = ui->h / 16; // small shift of y to the top
    // below heuristic heavely depends on mono font loupe glyph geometry:
    gdi.x += m.x * 5 / 32 - sx;
    gdi.y += m.y * 1 / 32 - sy;
    gdi.text(glyph_loupe);
    ui_point_t pt = gdi.get_em(zic->buttons_tiny_font);
    gdi.x = ui->x + (ui->w - pt.x) / 2 - sx;
    gdi.y = ui->y + (ui->h - pt.y) / 2 - sy;
    font_t f = gdi.set_font(zic->buttons_tiny_font);
    gdi.text("%s", in ? glyph_heavy_plus_sign : glyph_heavy_minus_sign);
    gdi.set_font(f);
    gdi.pop();
}

static void zic_button_paint(uic_t* ui) {
    assert(ui->tag == uic_tag_button);
    zic_t* zic = (zic_t*)ui->parent->parent;
    assert(zic->ui.tag == uic_tag_zic);
    uic_button_t* b = (uic_button_t*)ui;
    gdi.push(ui->x, ui->y);
    bool pressed = (ui->armed ^ ui->pressed) == 0;
    if (b->armed_until != 0) { pressed = true; }
    int sign = 1 - pressed * 2; // -1, +1
    int32_t w = sign * ui->w;
    int32_t h = sign * ui->h;
    int32_t x = b->ui.x + (int)pressed * ui->w;
    int32_t y = b->ui.y + (int)pressed * ui->h;
    gdi.gradient(x, y, w, h, colors.btn_gradient_darker,
        colors.btn_gradient_dark, true);
    color_t c = ui->armed ? colors.btn_armed : ui->color;
    if (b->ui.hover && !ui->armed) { c = colors.btn_hover_highlight; }
    if (ui->disabled) { c = colors.btn_disabled; }
    font_t f = ui->font != null ? *ui->font : app.fonts.regular;
    // special case for 1:1 button:
    if (&zic->zoom_1t1 == b) { f = app.fonts.mono; }
    ui_point_t m = gdi.measure_text(f, ui->text);
    gdi.set_text_color(c);
    gdi.x = ui->x + (ui->w - m.x) / 2;
    gdi.y = ui->y + (ui->h - m.y) / 2;
    f = gdi.set_font(f);
    if (&zic->zoom_in == b || &zic->zoom_out == b) {
        zic_zoom_in_out_button_paint(zic, ui, &zic->zoom_in == b);
    } else {
        gdi.text("%s", ui->text);
    }
    gdi.set_font(f);
    const int32_t pw = max(1, ui->em.y / 32); // pen width
    color_t color = ui->armed ? colors.dkgray4 : colors.gray;
    if (ui->hover && !ui->armed) { color = colors.blue; }
    if (ui->disabled) { color = colors.dkgray1; }
    pen_t p = gdi.create_pen(color, pw);
    gdi.set_pen(p);
    gdi.set_brush(gdi.brush_hollow);
    gdi.rounded(ui->x, ui->y, ui->w, ui->h, ui->em.y / 4, ui->em.y / 4);
    gdi.delete_pen(p);
    gdi.pop();
}

static uic_multiline(zic_about, 34.56,
    "Keyboard shortcuts:\n\n"
    "  Ctrl+C copies image to the clipboard.\n"
    "  \xE2\x9E\x95 zoom in; \xE2\x9E\x96 zoom out; "
    glyph_latin_capital_letter_O_with_stroke " 1:1\n\n"
    "  Arrows " glyph_left glyph_up glyph_down glyph_right
    " to pan an image inside the view.\n\n"
    "Try mouse wheel or mouse / touchpad hold and drag to pan."
);

static void zic_help(uic_button_t* unused(b)) {
    app.show_toast(&zic_about.ui, 7.0);
}

static void zic_copy_to_clipboard(zic_t* zic) {
    image_t image = {0};
    gdi.image_init(&image, zic->w, zic->h, zic->c, zic->pixels);
    clipboard.copy_bitmap(&image);
    gdi.image_dispose(&image);
    app.toast(3.3, "  Image copied to Clipboard  ");
}

static void zic_copy(uic_button_t* b) {
    zic_t* zic = (zic_t*)b->ui.that;
    assert(zic->ui.tag == uic_tag_zic);
    zic_copy_to_clipboard(zic);
}

static void zic_character(uic_t* ui, const char* utf8) {
    assert(ui->tag == uic_tag_zic);
    zic_t* zic = (zic_t*)ui;
    if (zic_is_focused(zic)) {  // && app.ctrl
        char ch = utf8[0];
        if (ch == '+' || ch == '=') {
            if (!zic->zoom_in.ui.disabled && zic->value < 8) {
                zic->value++;
                zic_zoomed(zic);
            }
        } else if (ch == '-' || ch == '_') {
            if (!zic->zoom_out.ui.disabled && zic->value > 0) {
                zic->value--;
                zic_zoomed(zic);
            }
        } else if (ch == '<' || ch == ',') {
            zic_mousewheel(ui, -zic->ui.w / 8, 0);
        } else if (ch == '>' || ch == '.') {
            zic_mousewheel(ui, +zic->ui.w / 8, 0);
        } else if (ch == '0') {
            zic->value = 4;
            zic_zoomed(zic);
        } else if (ch == 3 && zic->pixels != null) { // Ctrl+C
            zic_copy_to_clipboard(zic);
        }
    }
}

static void zic_init_button(zic_t* zic, uic_button_t* b,
    const char* label, void (*cb)(uic_button_t* b),
    const char* tip) {
    uic_button_init(b, label, 0.0, cb);
    b->ui.that = zic;
    strprintf(b->ui.tip, "%s", tip);
    bool appended = false;
    for (int i = 0; i < countof(zic->tools) - 1 && !appended; i++) {
        if (zic->tools[i] == null) {
            zic->tools[i] = &b->ui;
            assert(zic->tools[i + 1] == null);
            appended = true;;
        }
    }
    fatal_if(!appended, "resize tools[%d]", countof(zic->tools));
    b->ui.font = &zic->buttons_font;
    b->ui.paint = zic_button_paint;
}

void zic_init(zic_t* zic) {
    memset(zic, 0x00, sizeof(*zic));
    uic_init(&zic->ui);
    zic->ui.tag          = uic_tag_zic;
    zic->scale           = zic_scale;
    zic->position        = zic_position;
    zic->ui.children     = zic->children;
    zic->ui.paint        = zic_paint;
    zic->ui.mouse        = zic_mouse;
    zic->ui.measure      = zic_measure;
    zic->ui.layout       = zic_layout;
    zic->ui.every_100ms  = zic_every_100ms;
    zic->ui.mousewheel   = zic_mousewheel;
    zic->ui.character    = zic_character;
    zic->ui.key_pressed  = zic_key_pressed;
    // slightly enlarged fonts because loupe zoom in/out
    // does not look good in H1/regular fonts
    ui_point_t em_mono = gdi.get_em(app.fonts.mono);
    zic->buttons_tiny_font = gdi.font(app.fonts.mono, em_mono.y * 1 / 2, -1);
    zic->buttons_font = gdi.font(app.fonts.mono, em_mono.y * 11 / 8, -1);
    // buttons:
    zic_init_button(zic, &zic->copy, "\xF0\x9F\x93\x8B", zic_copy,
        "Copy to Clipboard Ctrl+C");
    zic_init_button(zic, &zic->zoom_out, glyph_loupe, zic_zoom_out, "Zoom Out");
    zic_init_button(zic, &zic->zoom_1t1, "1:1", zic_zoom_1t1, "reset to 1:1");
    zic_init_button(zic, &zic->zoom_in,  glyph_loupe, zic_zoom_in,  "Zoom In");
    zic_init_button(zic, &zic->help,     "?", zic_help, "Help");
    zic->toolbar.children = zic->tools;
    zic->children[0] = &zic->toolbar;
    zic->toolbar.hidden = true;
    zic->toolbar.disabled = true;
    zic->value = 4;
    zic->zn = 1;
    zic->zd = 1;
    zic->sx = 0.5;
    zic->sy = 0.5;
    zic->drag_start = (ui_point_t){-1, -1};
}

void zic_fini(zic_t* zic) {
    gdi.delete_font(zic->buttons_font);
    gdi.delete_font(zic->buttons_tiny_font);
}

#endif // zic_implementation