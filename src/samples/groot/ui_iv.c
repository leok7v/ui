#include "ut/ut.h"
#include "ui/ui.h"
#include "ui_iv.h"

static fp64_t ui_iv_scaleof(int32_t nominator, int32_t denominator) {
    const int32_t zn = 1 << (nominator - 1);
    const int32_t zd = 1 << (denominator - 1);
    return (fp64_t)zn / (fp64_t)zd;
}

static fp64_t ui_iv_scale(ui_iv_t* iv) {
    if (iv->fit) {
        return min((fp64_t)iv->view.w / iv->image.w,
                   (fp64_t)iv->view.h / iv->image.h);
    } else if (iv->fill) {
        return max((fp64_t)iv->view.w / iv->image.w,
                   (fp64_t)iv->view.h / iv->image.h);
    } else {
        return ui_iv_scaleof(iv->zn, iv->zd);
    }
}

static ui_rect_t ui_iv_position(ui_iv_t* iv) {
    ui_rect_t rc = { 0, 0, 0, 0 };
    if (iv->image.pixels != null) {
        int32_t iw = iv->image.w;
        int32_t ih = iv->image.h;
        // zoomed image width and height
        rc.w = (int32_t)((fp64_t)iw * ui_iv.scale(iv));
        rc.h = (int32_t)((fp64_t)ih * ui_iv.scale(iv));
        int32_t shift_x = (int32_t)((rc.w - iv->view.w) * iv->sx);
        int32_t shift_y = (int32_t)((rc.h - iv->view.h) * iv->sy);
        // shift_x and shift_y are in zoomed image coordinates
        rc.x = iv->view.x - shift_x; // screen x
        rc.y = iv->view.y - shift_y; // screen y
    }
    return rc;
}

static void ui_iv_paint(ui_view_t* v) {
    ui_iv_t* iv = (ui_iv_t*)v;
//  ui_gdi.fill(v->x, v->y, v->w, v->h, ui_colors.black);
    if (iv->image.pixels != null) {
        ui_gdi.set_clip(v->x, v->y, v->w, v->h);
        ut_swear(!iv->fit || !iv->fill, "make up your mind");
        ut_swear(0 < iv->zn && iv->zn <= 16);
        ut_swear(0 < iv->zd && iv->zd <= 16);
        // only 1:2 and 2:1 etc are supported:
        if (iv->zn != 1) { ut_swear(iv->zd == 1); }
        if (iv->zd != 1) { ut_swear(iv->zn == 1); }
        const int32_t iw = iv->image.w;
        const int32_t ih = iv->image.h;
        ui_rect_t rc = ui_iv_position(iv);
        if (iv->image.bpp == 1) {
            ui_gdi.greyscale(rc.x, rc.y, rc.w, rc.h,
                0, 0, iw, ih,
                iw, ih, iv->image.stride,
                iv->image.pixels);
        } else if (iv->image.bpp == 3) {
            ui_gdi.bgr(rc.x, rc.y, rc.w, rc.h,
                         0, 0, iw, ih,
                         iw, ih, iv->image.stride,
                         iv->image.pixels);
        } else if (iv->image.bpp == 4) {
            if (iv->image.bitmap == null) {
                ui_gdi.bgrx(rc.x, rc.y, rc.w, rc.h,
                              0, 0, iw, ih,
                              iw, ih, iv->image.stride,
                              iv->image.pixels);
            } else {
                ui_gdi.alpha(rc.x, rc.y, rc.w, rc.h,
                              0, 0, iw, ih,
                              &iv->image, iv->alpha);
            }
        } else {
            ut_swear(false, "unsupported .c: %d", iv->image.bpp);
        }
        if (ui_view.has_focus(v)) {
            ui_color_t highlight = ui_colors.get_color(ui_color_id_highlight);
            ui_gdi.frame(v->x, v->y, v->w, v->h, highlight);
        }
        ui_gdi.set_clip(0, 0, 0, 0);
    }
}

static void ui_iv_tools_background(ui_view_t* v) {
    ui_color_t face = ui_colors.get_color(ui_color_id_button_face);
    ui_color_t highlight = ui_colors.get_color(ui_color_id_highlight);
    ui_gdi.fill(v->x, v->y, v->w, v->h, face);
    ui_gdi.frame(v->x, v->y, v->w, v->h, highlight);
}

static void ui_iv_show_tools(ui_iv_t* iv, bool show) {
    if (iv->focusable) {
        if (iv->tool.bar.state.hidden  != !show) {
            iv->tool.bar.state.hidden   = !show;
            iv->tool.bar.state.disabled = !show;
            iv->tool.ratio.state.hidden = !show;
            ui_app.request_layout();
        }
        if (show) { // hide in 3.3 seconds:
            iv->when = ut_clock.seconds() + 3.3;
        } else {
            iv->when = 0;
        }
    }
}

static void ui_iv_fit_fill_scale(ui_iv_t* iv) {
    fp64_t s = ui_iv.scale(iv);
    ut_assert(s != 0);
    if (s > 1) {
        ui_view.set_text(&iv->tool.ratio, "1:%.3f", s);
    } else if (s != 0 && s <= 1) {
        ui_view.set_text(&iv->tool.ratio, "%.3f:1", 1.0 / s);
    } else {
        // s should not be zero ever
    }
}

static void ui_iv_measure(ui_view_t* v) {
    ui_iv_t* iv = (ui_iv_t*)v;
    if (!v->focusable) {
        v->w = (int32_t)(iv->image.w * ui_iv.scale(iv));
        v->h = (int32_t)(iv->image.h * ui_iv.scale(iv));
        if (iv->fit || iv->fill) {
            ui_iv_fit_fill_scale(iv);
        }
    } else {
        v->w = 0;
        v->h = 0;
    }
}

static void ui_iv_layout(ui_view_t* v) {
    ui_iv_t* iv = (ui_iv_t*)v;
    if (iv->fit || iv->fill) {
        ui_iv_fit_fill_scale(iv);
        ui_view.measure_control(&iv->tool.ratio);
    }
    iv->tool.bar.x = v->x + v->w - iv->tool.bar.w;
    iv->tool.bar.y = v->y;
    iv->tool.ratio.x = v->x + v->w - iv->tool.ratio.w;
    iv->tool.ratio.y = v->y + v->h - iv->tool.ratio.h;
}

static void ui_iv_every_100ms(ui_view_t* v) {
    ui_iv_t* iv = (ui_iv_t*)v;
    if (iv->when != 0 && ut_clock.seconds() > iv->when) {
        ui_iv_show_tools(iv, false);
    }
}

static void ui_iv_focus_lost(ui_view_t* v) {
    ui_iv_t* iv = (ui_iv_t*)v;
    ui_iv_show_tools(iv, ui_view.has_focus(v));
}

static void ui_iv_focus_gained(ui_view_t* v) {
    ui_iv_t* iv = (ui_iv_t*)v;
    ui_iv_show_tools(iv, ui_view.has_focus(v));
}

static void ui_iv_zoomed(ui_iv_t* iv) {
    iv->fill = false;
    iv->fit  = false;
    // 0=16:1 1=8:1 2=4:1 3=2:1 4=1:1 5=1:2 6=1:4 7=1:8 8=1:16
    int32_t n  = iv->zoom - 4;
    int32_t zn = iv->zn;
    int32_t zd = iv->zd;
    fp64_t scale_before = ui_iv.scale(iv);
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
        iv->zoom = 4 - (iv->zd - 1);
    } else if (iv->zd == 1) {
        iv->zoom = 4 + (iv->zn - 1);
    } else {
        ut_swear(false);
    }
    // is whole image visible?
    fp64_t s = ui_iv.scale(iv);
    bool whole = (int32_t)(iv->image.w * s) <= iv->view.w &&
                 (int32_t)(iv->image.h * s) <= iv->view.h;
    if (whole) { iv->sx = 0.5; iv->sy = 0.5; }
    ui_view.invalidate(&iv->view, null);
    ui_iv_show_tools(iv, true);
}

static void ui_iv_mouse_scroll(ui_view_t* v, ui_point_t dx_dy) {
    fp64_t dx = (fp64_t)dx_dy.x;
    fp64_t dy = (fp64_t)dx_dy.y;
    ui_iv_t* iv = (ui_iv_t*)v;
    if (ui_view.has_focus(v)) {
        fp64_t s = ui_iv.scale(iv);
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
    }
}

static bool ui_iv_tap(ui_view_t* v, int32_t ix, bool pressed) {
    ui_iv_t* iv = (ui_iv_t*)v;
    const int32_t x = ui_app.mouse.x - iv->view.x;
    const int32_t y = ui_app.mouse.y - iv->view.y;
    bool tools  = !iv->tool.bar.state.hidden &&
                  ui_view.inside(&iv->tool.bar, &ui_app.mouse);
    bool inside = ui_view.inside(&iv->view, &ui_app.mouse) && !tools;
    bool left   = ix == 0;
    bool drag_started = iv->drag_start.x >= 0 && iv->drag_start.y >= 0;
    if (left && inside && !drag_started) {
        iv->drag_start = (ui_point_t){x, y};
    }
    if (!pressed) {
        iv->drag_start = (ui_point_t){-1, -1};
    }
//  ut_println("inside %s", inside ? "true" : "false");
    return inside;
}

static bool ui_iv_move(ui_view_t* v) {
    ui_iv_t* iv = (ui_iv_t*)v;
    bool drag_started = iv->drag_start.x >= 0 && iv->drag_start.y >= 0;
    bool tools  = !iv->tool.bar.state.hidden &&
                  ui_view.inside(&iv->tool.bar, &ui_app.mouse);
    bool inside = ui_view.inside(&iv->view, &ui_app.mouse) && !tools;
    if (drag_started && inside) {
        ui_iv_show_tools(iv, false);
        const int32_t x = ui_app.mouse.x - iv->view.x;
        const int32_t y = ui_app.mouse.y - iv->view.y;
        ui_point_t dx_dy = {iv->drag_start.x - x, iv->drag_start.y - y};
        ui_iv_mouse_scroll(v, dx_dy);
        iv->drag_start = (ui_point_t){x, y};
    } else if (inside) {
        ui_iv_show_tools(iv, true);
    }
//  ut_println("inside %s", inside ? "true" : "false");
    return inside;
}

static bool ui_iv_key_pressed(ui_view_t* v, int64_t vk) {
    ui_iv_t* iv = (ui_iv_t*)v;
    bool swallowed = false;
    if (ui_view.has_focus(v)) {
        swallowed = true;
        if (vk == ui.key.up) {
            ui_iv_mouse_scroll(v, (ui_point_t){0, -iv->view.h / 8});
        } else if (vk == ui.key.down) {
            ui_iv_mouse_scroll(v, (ui_point_t){0, +iv->view.h / 8});
        } else if (vk == ui.key.left) {
            ui_iv_mouse_scroll(v, (ui_point_t){-iv->view.w / 8, 0});
        } else if (vk == ui.key.right) {
            ui_iv_mouse_scroll(v, (ui_point_t){+iv->view.w / 8, 0});
        } else if (vk == ui.key.plus) {
            if (iv->zoom < 8) {
                iv->zoom++;
                ui_iv_zoomed(iv);
            }
        } else if (vk == ui.key.minus) {
            if (iv->zoom > 0) {
                iv->zoom--;
                ui_iv_zoomed(iv);
            }
        } else {
            swallowed = false;
        }
    }
    return swallowed;
}

static void ui_iv_zoom_in(ui_button_t* b) {
    ui_iv_t* iv = (ui_iv_t*)b->that;
    if (iv->zoom < 8) {
        iv->zoom++;
        ui_iv_zoomed(iv);
    }
}

static void ui_iv_zoom_out(ui_button_t* b) {
    ui_iv_t* iv = (ui_iv_t*)b->that;
    if (iv->zoom > 0) {
        iv->zoom--;
        ui_iv_zoomed(iv);
    }
}

static void ui_iv_fit(ui_button_t* b) {
    ui_iv_t* iv = (ui_iv_t*)b->that;
    iv->fit  = true;
    iv->fill = false;
    ui_iv_fit_fill_scale(iv);
    ui_view.invalidate(&iv->view, null);
}

static void ui_iv_fill(ui_button_t* b) {
    ui_iv_t* iv = (ui_iv_t*)b->that;
    iv->fill = true;
    iv->fit  = false;
    ui_iv_fit_fill_scale(iv);
    ui_view.invalidate(&iv->view, null);
}

static void ui_iv_zoom_1t1(ui_button_t* b) {
    ui_iv_t* iv = (ui_iv_t*)b->that;
    iv->zoom = 4;
    ui_iv_zoomed(iv);
}

static ui_label_t ui_iv_about = ui_label(0,
    "Keyboard shortcuts:\n\n"
    "Ctrl+C copies image to the clipboard.\n\n"
    ut_glyph_heavy_plus_sign " zoom in; "
    ut_glyph_heavy_minus_sign " zoom out; "
    ut_glyph_open_circle_arrows_one_overlay " 1:1.\n\n"
    "Left/Right Arrows "
    ut_glyph_leftward_arrow
    ut_glyph_rightwards_arrow
    "Up/Down Arrows "
    ut_glyph_upwards_arrow
    ut_glyph_downwards_arrow
    "\npans the image inside view.\n\n"
    "Mouse wheel or mouse / touchpad hold and drag to pan.\n"
);

static void ui_iv_help(ui_button_t* ut_unused(b)) {
    ui_app.show_toast(&ui_iv_about, 7.0);
}

static void ui_iv_copy_to_clipboard(ui_iv_t* iv) {
    ui_image_t image = {0};
    if (iv->image.bitmap != null) {
        ut_clipboard.put_image(&iv->image);
    } else {
        ui_gdi.image_init(&image, iv->image.w, iv->image.h,
                                  iv->image.bpp, iv->image.pixels);
        ut_clipboard.put_image(&image);
        ui_gdi.image_dispose(&image);
    }
    static ui_label_t hint = ui_label(0.0f, "copied to clipboard");
    ui_app.show_hint(&hint, ui_app.mouse.x,
                            ui_app.mouse.y + iv->view.fm->height,
                     1.5);
}

static void ui_iv_copy(ui_button_t* b) {
    ui_iv_t* iv = (ui_iv_t*)b->that;
    ui_iv_copy_to_clipboard(iv);
}

static void ui_iv_character(ui_view_t* v, const char* utf8) {
    ui_iv_t* iv = (ui_iv_t*)v;
    if (ui_view.has_focus(v)) { // && ui_app.ctrl ?
        char ch = utf8[0];
        if (ch == '+' || ch == '=') {
            if (iv->zoom < 8) {
                iv->zoom++;
                ui_iv_zoomed(iv);
            }
        } else if (ch == '-' || ch == '_') {
            if (iv->zoom > 0) {
                iv->zoom--;
                ui_iv_zoomed(iv);
            }
        } else if (ch == '<' || ch == ',') {
            ui_iv_mouse_scroll(v, (ui_point_t){-iv->view.w / 8, 0});
        } else if (ch == '>' || ch == '.') {
            ui_iv_mouse_scroll(v, (ui_point_t){+iv->view.w / 8, 0});
        } else if (ch == '0') {
            iv->zoom = 4;
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
    ut_str_printf(b->hint, "%s", hint);
    ui_view.add_last(&iv->tool.bar, b);
}

void ui_iv_init(ui_iv_t* iv) {
    memset(iv, 0x00, sizeof(*iv));
    iv->view.type         = ui_view_image;
    iv->view.paint        = ui_iv_paint;
    iv->view.tap          = ui_iv_tap;
    iv->view.mouse_move   = ui_iv_move;
    iv->view.measure      = ui_iv_measure;
    iv->view.layout       = ui_iv_layout;
    iv->view.every_100ms  = ui_iv_every_100ms;
    iv->view.focus_lost   = ui_iv_focus_lost;
    iv->view.focus_gained = ui_iv_focus_gained;
    iv->view.mouse_scroll = ui_iv_mouse_scroll;
    iv->view.character    = ui_iv_character;
    iv->view.key_pressed  = ui_iv_key_pressed;
    iv->view.fm           = &ui_app.fm.regular;
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
    ui_iv_add_button(iv, &iv->tool.fit,
                     ut_glyph_frame_with_tiles,
                     ui_iv_fit,  "Fit");
    ui_iv_add_button(iv, &iv->tool.fill,
                     ut_glyph_document_with_picture,
                     ui_iv_fill,  "Fill");
    ui_iv_add_button(iv, &iv->tool.help,
                     "?", ui_iv_help, "Help");
    iv->tool.zoom_1t1.min_w_em = 1.25f;
    iv->tool.ratio = (ui_label_t)ui_label(0, "1:1");
    iv->tool.ratio.color = ui_colors.get_color(ui_color_id_highlight);
    iv->tool.ratio.color_id = ui_color_id_highlight;
    ui_view.add_last(&iv->view, &iv->tool.bar);
    ui_view.add_last(&iv->view, &iv->tool.ratio);
    iv->tool.bar.state.hidden = true;
    iv->tool.ratio.state.hidden = true;
    iv->tool.bar.erase   = ui_iv_tools_background;
    iv->tool.ratio.erase = ui_iv_tools_background;
    iv->zoom = 4;
    iv->zn = 1;
    iv->zd = 1;
    iv->sx = 0.5;
    iv->sy = 0.5;
    iv->drag_start = (ui_point_t){-1, -1};
}

void ui_iv_init_with(ui_iv_t* iv, const uint8_t* pixels,
                                  int32_t w, int32_t h,
                                  int32_t c, int32_t s) {
    ui_iv_init(iv);
    iv->image.pixels = (uint8_t*)pixels;
    iv->image.w = w;
    iv->image.h = h;
    iv->image.bpp = c;
    iv->image.stride = s;
}

static void ui_iv_ratio(ui_iv_t* iv, int32_t zn, int32_t zd) {
    ut_swear(0 < zn && zn <= 16);
    ut_swear(0 < zd && zd <= 16);
    // only 1:2 and 2:1 etc are supported:
    if (zn != 1) { ut_swear(zd == 1); }
    if (zd != 1) { ut_swear(zn == 1); }
    iv->zn = zn;
    iv->zd = zd;
    iv->fit  = false;
    iv->fill = false;
}

ui_iv_if ui_iv = {
    .init      = ui_iv_init,
    .init_with = ui_iv_init_with,
    .ratio     = null, // TODO
    .scale     = ui_iv_scale,
    .position  = ui_iv_position
};

