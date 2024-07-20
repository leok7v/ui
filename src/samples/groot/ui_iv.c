#include "ut/ut.h"
#include "ui/ui.h"
#include "ui_iv.h"

static fp64_t ui_iv_scaleof(int32_t nominator, int32_t denominator) {
    const int32_t zn = 1 << (nominator - 1);
    const int32_t zd = 1 << (denominator - 1);
    return (fp64_t)zn / (fp64_t)zd;
}

static fp64_t ui_iv_scale(ui_iv_t* iv) {
    return ui_iv_scaleof(iv->zn, iv->zd);
}

static ui_rect_t ui_iv_position(ui_iv_t* iv) {
    ui_rect_t rc = { 0, 0, 0, 0 };
    if (iv->image.pixels != null) {
        int32_t iw = iv->image.w;
        int32_t ih = iv->image.h;
        // zoomed image width and height
        rc.w = iw * (1 << (iv->zn - 1)) / (1 << (iv->zd - 1));
        rc.h = ih * (1 << (iv->zn - 1)) / (1 << (iv->zd - 1));
        int32_t shift_x = (int32_t)((rc.w - iv->view.w) * iv->sx);
        int32_t shift_y = (int32_t)((rc.h - iv->view.h) * iv->sy);
        // shift_x and shift_y are in zoomed image coordinates
        rc.x = iv->view.x - shift_x; // screen x
        rc.y = iv->view.y - shift_y; // screen y
    }
    return rc;
}

static void ui_iv_paint(ui_view_t* v) {
//  ut_assert(v->type == ui_view_image);
    ut_assert(!v->state.hidden);
    ui_iv_t* iv = (ui_iv_t*)v;
    ui_gdi.fill(v->x, v->y, v->w, v->h, ui_colors.black);
    if (iv->image.pixels != null) {
        ui_gdi.set_clip(v->x, v->y, v->w, v->h);
        ut_assert(0 < iv->zn && iv->zn <= 16);
        ut_assert(0 < iv->zd && iv->zd <= 16);
        // only 1:2 and 2:1 etc are supported:
        if (iv->zn != 1) { ut_assert(iv->zd == 1); }
        if (iv->zd != 1) { ut_assert(iv->zn == 1); }
        const int32_t iw = iv->image.w;
        const int32_t ih = iv->image.h;
        ui_rect_t rc = ui_iv_position(iv);
        if (iv->image.c == 1) {
            ui_gdi.greyscale(rc.x, rc.y, rc.w, rc.h,
                0, 0, iw, ih,
                iw, ih, iv->image.s, iv->image.pixels);
        } else if (iv->image.c == 3) {
            ui_gdi.bgr(rc.x, rc.y, rc.w, rc.h,
                         0, 0, iw, ih,
                         iw, ih, iv->image.s, iv->image.pixels);
        } else if (iv->image.c == 4) {
            ui_gdi.bgrx(rc.x, rc.y, rc.w, rc.h,
                          0, 0, iw, ih,
                          iw, ih, iv->image.s, iv->image.pixels);
        } else {
            ut_assert(false, "unsupported .c: %d", iv->image.c);
        }
        if (ui_view.has_focus(v)) {
            ui_color_t highlight = ui_colors.get_color(ui_color_id_highlight);
            ui_gdi.frame(v->x, v->y, v->w, v->h, highlight);
        }
        ui_gdi.set_clip(0, 0, 0, 0);
    }
}

static void ui_iv_hide_toolbar(ui_iv_t* iv, bool hide) {
    if (iv->tool.bar.state.hidden != hide) {
        iv->tool.bar.state.hidden   = hide;
        iv->tool.bar.state.disabled = hide;
        iv->tool.ratio.state.hidden = hide;
        ui_view_for_each(&iv->tool.bar, it, { it->state.pressed = false; });
        ui_app.request_layout();
    }
}

static void ui_iv_enable_zoom(ui_iv_t* iv) {
#if 0 // Disabled buttons do not look good TODO: need disabled same looking button?
    fp64_t scale = iv->scale(iv);
    // whole image is visible
    bool whole = (int32_t)(iv->image.w * scale) <= iv->view.w &&
                 (int32_t)(iv->image.h * scale) <= iv->view.h;
    iv->tool.zoom_out.state.disabled = whole;
    iv->tool.zoom_1t1.state.disabled = iv->value == 4;
#endif
    iv->tool.zoom_in.state.disabled = false;
    iv->tool.zoom_out.state.disabled = false;
    iv->tool.zoom_1t1.state.disabled = false;

}

static void ui_iv_composed(ui_view_t* v) {
    ut_assert(v->type == ui_view_image);
    ui_iv_t* iv = (ui_iv_t*)v;
    iv->tool.bar.x = v->x + v->w - iv->tool.bar.w;
    iv->tool.bar.y = v->y;
    iv->tool.ratio.x = v->x + v->w - iv->tool.ratio.w;
    iv->tool.ratio.y = v->y + v->h - iv->tool.ratio.h;
    ui_iv_enable_zoom(iv);
}

static void ui_iv_every_100ms(ui_view_t* v) {
    ut_assert(v->type == ui_view_image);
    ui_iv_t* iv = (ui_iv_t*)v;
    if (iv->when != 0 && ut_clock.seconds() > iv->when) {
        ui_iv_hide_toolbar(iv, true);
    } else if (!ui_view.has_focus(v) && !iv->tool.bar.state.hidden) {
        ui_iv_hide_toolbar(iv, true);
    }
    ui_iv_enable_zoom(iv);
}

static void ui_iv_focus_lost(ui_view_t* v) {
    ut_assert(v->type == ui_view_image);
    ui_iv_t* iv = (ui_iv_t*)v;
    ui_iv_hide_toolbar(iv, !ui_view.has_focus(v));
}

static void ui_iv_focus_gained(ui_view_t* v) {
    ut_assert(v->type == ui_view_image);
    ui_iv_t* iv = (ui_iv_t*)v;
    iv->when = ut_clock.seconds() + 3.3;
    ui_iv_hide_toolbar(iv, !ui_view.has_focus(v));
}

static void ui_iv_zoomed(ui_iv_t* iv) {
    // 0=16:1 1=8:1 2=4:1 3=2:1 4=1:1 5=1:2 6=1:4 7=1:8 8=1:16
    int32_t n  = iv->value - 4;
    int32_t zn = iv->zn;
    int32_t zd = iv->zd;
    fp64_t scale_before = iv->scale(iv);
    // whole image is visible
//  bool whole = (int32_t)(iv->image.w * scale_before) <= iv->view.w &&
//               (int32_t)(iv->image.h * scale_before) <= iv->view.h;
    if (n > 0) {
        zn = n + 1;
        zd = 1;
    } else if (n < 0) {
        zn = 1;
        zd = -n + 1;
    } else if (n == 0) {
        zn = 1;
        zd = 1;
    }
    fp64_t scale_after = ui_iv_scaleof(zn, zd);
    if (scale_after != scale_before) {
        iv->zn = zn;
        iv->zd = zd;
        const int32_t nm = 1 << (iv->zn - 1);
        const int32_t dm = 1 << (iv->zd - 1);
        ui_view.set_text(&iv->tool.ratio, "%d:%d", nm, dm);
    }
    if (iv->zn == 1) {
        iv->value = 4 - (iv->zd - 1);
    } else if (iv->zd == 1) {
        iv->value = 4 + (iv->zn - 1);
    } else {
        ut_assert(false);
    }
    // is whole image visible?
    fp64_t s = iv->scale(iv);
    bool whole = (int32_t)(iv->image.w * s) <= iv->view.w &&
                 (int32_t)(iv->image.h * s) <= iv->view.h;
    if (whole) { iv->sx = 0.5; iv->sy = 0.5; }
    ui_iv_enable_zoom(iv);
    ui_view.invalidate(&iv->view, null);
    iv->when = ut_clock.seconds() + 3.3;
}

static void ui_iv_mouse_scroll(ui_view_t* v, ui_point_t dx_dy) {
    ut_assert(v->type == ui_view_image);
    fp64_t dx = (fp64_t)dx_dy.x;
    fp64_t dy = (fp64_t)dx_dy.y;
    ui_iv_t* iv = (ui_iv_t*)v;
    if (ui_view.has_focus(v)) {
        fp64_t s = iv->scale(iv);
        if (iv->image.w * s > iv->view.w || iv->image.h * s > iv->view.h) {
            iv->sx = max(0.0, min(iv->sx + dx / iv->image.w, 1.0));
        } else {
            iv->sx = 0.5;
        }
        if (iv->image.h * s > iv->view.h) {
            iv->sy = max(0.0, min(iv->sy + dy / iv->image.h, 1.0));
        } else {
            iv->sy = 0.5;
        }
        ui_view.invalidate(&iv->view, null);
        iv->when = ut_clock.seconds() + 3.3;
    }
}

static bool ui_iv_tap(ui_view_t* v, int32_t ix, bool pressed) {
    ut_assert(v->type == ui_view_image);
    ut_assert(!v->state.disabled);
    ui_iv_t* iv = (ui_iv_t*)v;
    const int32_t x = ui_app.mouse.x - iv->view.x;
    const int32_t y = ui_app.mouse.y - iv->view.y;
    bool inside = 0 <= x && x < v->w && 0 <= y && y < v->h;
    bool left  = ix == 0;
    bool drag_started = iv->drag_start.x >= 0 && iv->drag_start.y >= 0;
    if (left && inside && !drag_started) {
        iv->drag_start = (ui_point_t){x, y};
    }
    iv->when = ut_clock.seconds() + 3.3;
    ui_iv_hide_toolbar(iv, false);
    if (!pressed) {
        iv->drag_start = (ui_point_t){-1, -1};
    }
    return true;
}

static bool ui_iv_move(ui_view_t* v) {
    ut_assert(v->type == ui_view_image);
    ut_assert(!v->state.disabled);
    ui_iv_t* iv = (ui_iv_t*)v;
    bool drag_started = iv->drag_start.x >= 0 && iv->drag_start.y >= 0;
    bool inside = ui_view.inside(&iv->view, &ui_app.mouse);
    if (drag_started && inside) {
        ui_iv_hide_toolbar(iv, true);
        const int32_t x = ui_app.mouse.x - iv->view.x;
        const int32_t y = ui_app.mouse.y - iv->view.y;
        ui_point_t dx_dy = {iv->drag_start.x - x, iv->drag_start.y - y};
        ui_iv_mouse_scroll(v, dx_dy);
        iv->drag_start = (ui_point_t){x, y};
    }
    return true;
}

static bool ui_iv_key_pressed(ui_view_t* v, int64_t vk) {
    ui_iv_t* iv = (ui_iv_t*)v;
    ut_assert(iv->view.type == ui_view_image);
    bool focused = ui_view.has_focus(v);
    if (focused) {
        if (vk == ui.key.up) {
            ui_iv_mouse_scroll(v, (ui_point_t){0, -iv->view.h / 8});
        } else if (vk == ui.key.down) {
            ui_iv_mouse_scroll(v, (ui_point_t){0, +iv->view.h / 8});
        } else if (vk == ui.key.left) {
            ui_iv_mouse_scroll(v, (ui_point_t){-iv->view.w / 8, 0});
        } else if (vk == ui.key.right) {
            ui_iv_mouse_scroll(v, (ui_point_t){+iv->view.w / 8, 0});
        }
    }
    return focused;
}

static void ui_iv_zoom_in(ui_button_t* b) {
    ui_iv_t* iv = (ui_iv_t*)b->that;
    ut_assert(iv->view.type == ui_view_image);
    if (!iv->tool.zoom_in.state.disabled && iv->value < 8) {
        iv->value++;
        ui_iv_zoomed(iv);
    }
}

static void ui_iv_zoom_out(ui_button_t* b) {
    ui_iv_t* iv = (ui_iv_t*)b->that;
    ut_assert(iv->view.type == ui_view_image);
    if (!iv->tool.zoom_out.state.disabled && iv->value > 0) {
        iv->value--;
        ui_iv_zoomed(iv);
    }
}

static void ui_iv_zoom_1t1(ui_button_t* b) {
    ui_iv_t* iv = (ui_iv_t*)b->that;
    ut_assert(iv->view.type == ui_view_image);
    iv->value = 4;
    ui_iv_zoomed(iv);
}

// U+1F502 Clockwise Rightwards and Leftwards Open Circle Arrows with Circled One Overlay
// F0 9F 94 82
// ut_glyph_clockwise_rightwards_and_leftwards_open_circle_arrows_with_circled_one_overlay "\xF0\x9F\x94\x82"
#define ut_glyph_open_circle_arrows_one_overlay "\xF0\x9F\x94\x82"

static ui_label_t ui_iv_about = ui_label(0,
    "Keyboard shortcuts:\n\n"
    "  Ctrl+C copies image to the clipboard.\n"
    "  \xE2\x9E\x95 zoom in; \xE2\x9E\x96 zoom out; "
    ut_glyph_open_circle_arrows_one_overlay " 1:1\n\n"
    "  Arrows "
    ut_glyph_leftward_arrow ut_glyph_upwards_arrow
    ut_glyph_downwards_arrow ut_glyph_rightwards_arrow
    " to pan an image inside the view.\n\n"
    "Try mouse wheel or mouse / touchpad hold and drag to pan.\n"
);

static void foo1(int bar(int)) {
    bar(1);
}

static void foo2(int (*bar)(int)) {
    bar(1);
}

static void ui_iv_help(ui_button_t* ut_unused(b)) {
    ui_app.show_toast(&ui_iv_about, 7.0);
}

static void ui_iv_copy_to_clipboard(ui_iv_t* iv) {
    ui_image_t image = {0};
    ui_gdi.image_init(&image, iv->image.w, iv->image.h, iv->image.c, iv->image.pixels);
    ut_clipboard.put_image(&image);
    ui_gdi.image_dispose(&image);
    static ui_label_t hint = ui_label(0.0f, "copied to clipboard");
    ui_app.show_hint(&hint, ui_app.mouse.x,
                            ui_app.mouse.y + iv->view.fm->height,
                     1.5);
    iv->when = ut_clock.seconds() + 3.3;
}

static void ui_iv_copy(ui_button_t* b) {
    ui_iv_t* iv = (ui_iv_t*)b->that;
    ut_assert(iv->view.type == ui_view_image);
    ui_iv_copy_to_clipboard(iv);
}

static void ui_iv_character(ui_view_t* v, const char* utf8) {
    ut_assert(v->type == ui_view_image);
    ui_iv_t* iv = (ui_iv_t*)v;
    if (ui_view.has_focus(v)) { // && ui_app.ctrl ?
        char ch = utf8[0];
        if (ch == '+' || ch == '=') {
            if (!iv->tool.zoom_in.state.disabled && iv->value < 8) {
                iv->value++;
                ui_iv_zoomed(iv);
            }
        } else if (ch == '-' || ch == '_') {
            if (!iv->tool.zoom_out.state.disabled && iv->value > 0) {
                iv->value--;
                ui_iv_zoomed(iv);
            }
        } else if (ch == '<' || ch == ',') {
            ui_iv_mouse_scroll(v, (ui_point_t){-iv->view.w / 8, 0});
        } else if (ch == '>' || ch == '.') {
            ui_iv_mouse_scroll(v, (ui_point_t){+iv->view.w / 8, 0});
        } else if (ch == '0') {
            iv->value = 4;
            ui_iv_zoomed(iv);
        } else if (ch == 3 && iv->image.pixels != null) { // Ctrl+C
            ui_iv_copy_to_clipboard(iv);
        }
    }
}

static void ui_iv_add_button(ui_iv_t* iv, ui_button_t* b,
    const char* label, void (*cb)(ui_button_t* b), const char* hint) {
    *b = (ui_button_t)ui_button("", 0.0f, cb);
    ui_view.set_text(b, label);
    b->that = iv;
    b->insets.top = 0;
    b->insets.bottom = 0;
    b->padding.top = 0;
    b->padding.bottom = 0;
    b->insets  = (ui_margins_t){0};
    b->padding = (ui_margins_t){0};
    b->flat = true;
    b->fm = &ui_app.fm.mono;
    b->min_w_em = 1.5f;
//  b->debug.paint.fm = true;
//  b->paint = ui_iv_button_paint;
    ut_str_printf(b->hint, "%s", hint);
    ui_view.add_last(&iv->tool.bar, b);
}

void ui_iv_init(ui_iv_t* iv) {
    memset(iv, 0x00, sizeof(*iv));
    iv->view.type         = ui_view_image;
    iv->scale             = ui_iv_scale;
    iv->position          = ui_iv_position;
    iv->view.paint        = ui_iv_paint;
    iv->view.tap          = ui_iv_tap;
    iv->view.mouse_move   = ui_iv_move;
    iv->view.composed     = ui_iv_composed;
    iv->view.every_100ms  = ui_iv_every_100ms;
    iv->view.focus_lost   = ui_iv_focus_lost;
    iv->view.focus_gained = ui_iv_focus_gained;
    iv->view.mouse_scroll = ui_iv_mouse_scroll;
    iv->view.character    = ui_iv_character;
    iv->view.key_pressed  = ui_iv_key_pressed;
    iv->view.fm           = &ui_app.fm.regular;
    iv->view.focusable    = true;
    iv->tool.bar = (ui_view_t)ui_view(span);
    // buttons:
    ui_iv_add_button(iv, &iv->tool.copy, "\xF0\x9F\x93\x8B", ui_iv_copy,
        "Copy to Clipboard Ctrl+C");
    ui_iv_add_button(iv, &iv->tool.zoom_out,
                    ut_glyph_heavy_minus_sign,
                    ui_iv_zoom_out, "Zoom Out");
    ui_iv_add_button(iv, &iv->tool.zoom_1t1,
                    ut_glyph_open_circle_arrows_one_overlay,
                    ui_iv_zoom_1t1, "Reset to 1:1");
    ui_iv_add_button(iv, &iv->tool.zoom_in,
                     ut_glyph_heavy_plus_sign,
                     ui_iv_zoom_in,  "Zoom In");
    ui_iv_add_button(iv, &iv->tool.help,
                     "?", ui_iv_help, "Help");
    iv->tool.zoom_1t1.min_w_em = 1.25f;
    iv->tool.ratio = (ui_label_t)ui_label(0, "1:1");
    iv->tool.ratio.color = ui_colors.get_color(ui_color_id_highlight);
    iv->tool.ratio.color_id = ui_color_id_highlight;
    ui_view.add_last(&iv->view, &iv->tool.bar);
    ui_view.add_last(&iv->view, &iv->tool.ratio);
    iv->tool.bar.state.hidden = true;
    iv->tool.bar.state.disabled = true;
    iv->tool.ratio.state.hidden = true;
    iv->value = 4;
    iv->zn = 1;
    iv->zd = 1;
    iv->sx = 0.5;
    iv->sy = 0.5;
    iv->drag_start = (ui_point_t){-1, -1};
}

void ui_iv_fini(ui_iv_t* iv) {
    ut_assert(iv->parent == null && iv->next == null);
    memset(iv, 0x00, sizeof(iv));
}

