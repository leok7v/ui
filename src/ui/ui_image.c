#include "rt/rt.h"
#include "ui/ui.h"

static fp64_t ui_image_scale_of(int32_t nominator, int32_t denominator) {
    const int32_t zn = 1 << (nominator - 1);
    const int32_t zd = 1 << (denominator - 1);
    return (fp64_t)zn / (fp64_t)zd;
}

static fp64_t ui_image_scale(ui_image_t* iv) {
    if (iv->fit && iv->w > 0 && iv->h > 0) {
        return min((fp64_t)iv->w / iv->image.w,
                   (fp64_t)iv->h / iv->image.h);
    } else if (iv->fill && iv->w > 0 && iv->h > 0) {
        return max((fp64_t)iv->w / iv->image.w,
                   (fp64_t)iv->h / iv->image.h);
    } else {
        return ui_image_scale_of(iv->zn, iv->zd);
    }
}

static ui_rect_t ui_image_position(ui_image_t* iv) {
    ui_rect_t rc = { 0, 0, 0, 0 };
    if (iv->image.pixels != null) {
        int32_t iw = iv->image.w;
        int32_t ih = iv->image.h;
        // zoomed image width and height
        rc.w = (int32_t)((fp64_t)iw * ui_image.scale(iv));
        rc.h = (int32_t)((fp64_t)ih * ui_image.scale(iv));
        int32_t shift_x = (int32_t)((rc.w - iv->w) * iv->sx);
        int32_t shift_y = (int32_t)((rc.h - iv->h) * iv->sy);
        // shift_x and shift_y are in zoomed image coordinates
        rc.x = iv->x - shift_x; // screen x
        rc.y = iv->y - shift_y; // screen y
    }
    return rc;
}

static void ui_image_paint(ui_view_t* v) {
    ui_image_t* iv = (ui_image_t*)v;
//  ui_gdi.fill(v->x, v->y, v->w, v->h, ui_colors.black);
    if (iv->image.pixels != null) {
        ui_gdi.set_clip(v->x, v->y, v->w, v->h);
        rt_swear(!iv->fit || !iv->fill, "make up your mind");
        rt_swear(0 < iv->zn && iv->zn <= 16);
        rt_swear(0 < iv->zd && iv->zd <= 16);
        // only 1:2 and 2:1 etc are supported:
        if (iv->zn != 1) { rt_swear(iv->zd == 1); }
        if (iv->zd != 1) { rt_swear(iv->zn == 1); }
        const int32_t iw = iv->image.w;
        const int32_t ih = iv->image.h;
        ui_rect_t rc = ui_image_position(iv);
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
            if (iv->image.texture == null) {
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
            rt_swear(false, "unsupported .c: %d", iv->image.bpp);
        }
        if (ui_view.has_focus(v)) {
            ui_color_t highlight = ui_colors.get_color(ui_color_id_highlight);
            ui_gdi.frame(v->x, v->y, v->w, v->h, highlight);
        }
        ui_gdi.set_clip(0, 0, 0, 0);
    }
}

static void ui_image_tools_background(ui_view_t* v) {
    ui_color_t face = ui_colors.get_color(ui_color_id_button_face);
    ui_color_t highlight = ui_colors.get_color(ui_color_id_highlight);
    ui_gdi.fill(v->x, v->y, v->w, v->h, face);
    ui_gdi.frame(v->x, v->y, v->w, v->h, highlight);
}

static void ui_image_show_tools(ui_image_t* iv, bool show) {
    if (iv->focusable) {
        if (iv->tool.bar.state.hidden  != !show) {
            iv->tool.bar.state.hidden   = !show;
            iv->tool.bar.state.disabled = !show;
            iv->tool.ratio.state.hidden = !show;
            ui_app.request_layout();
        }
        if (show) { // hide in 3.3 seconds:
            iv->when = rt_clock.seconds() + 3.3;
        } else {
            iv->when = 0;
        }
    }
}

static void ui_image_fit_fill_scale(ui_image_t* iv) {
    fp64_t s = ui_image.scale(iv);
    rt_assert(s != 0);
    if (s > 1) {
        ui_view.set_text(&iv->tool.ratio, "1:%.3f", s);
    } else if (s != 0 && s <= 1) {
        ui_view.set_text(&iv->tool.ratio, "%.3f:1", 1.0 / s);
    } else {
        // s should not be zero ever
    }
}

static void ui_image_measure(ui_view_t* v) {
    ui_image_t* iv = (ui_image_t*)v;
    if (!v->focusable) {
        v->w = (int32_t)(iv->image.w * ui_image.scale(iv));
        v->h = (int32_t)(iv->image.h * ui_image.scale(iv));
        if (iv->fit || iv->fill) {
            ui_image_fit_fill_scale(iv);
        }
    } else {
        v->w = 0;
        v->h = 0;
    }
}

static void ui_image_layout(ui_view_t* v) {
    ui_image_t* iv = (ui_image_t*)v;
    if (iv->fit || iv->fill) {
        ui_image_fit_fill_scale(iv);
        ui_view.measure_control(&iv->tool.ratio);
    }
    iv->tool.bar.x = v->x + v->w - iv->tool.bar.w;
    iv->tool.bar.y = v->y;
    iv->tool.ratio.x = v->x + v->w - iv->tool.ratio.w;
    iv->tool.ratio.y = v->y + v->h - iv->tool.ratio.h;
}

static void ui_image_every_100ms(ui_view_t* v) {
    ui_image_t* iv = (ui_image_t*)v;
    if (iv->when != 0 && rt_clock.seconds() > iv->when) {
        ui_image_show_tools(iv, false);
    }
}

static void ui_image_focus_lost(ui_view_t* v) {
    ui_image_t* iv = (ui_image_t*)v;
    ui_image_show_tools(iv, ui_view.has_focus(v));
}

static void ui_image_focus_gained(ui_view_t* v) {
    ui_image_t* iv = (ui_image_t*)v;
    ui_image_show_tools(iv, ui_view.has_focus(v));
}

static void ui_image_zoomed(ui_image_t* iv) {
    iv->fill = false;
    iv->fit  = false;
    // 0=16:1 1=8:1 2=4:1 3=2:1 4=1:1 5=1:2 6=1:4 7=1:8 8=1:16
    int32_t n  = iv->zoom - 4;
    int32_t zn = iv->zn;
    int32_t zd = iv->zd;
    fp64_t scale_before = ui_image.scale(iv);
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
    fp64_t scale_after = ui_image_scale_of(zn, zd);
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
        rt_swear(false);
    }
    // is whole image visible?
    fp64_t s = ui_image.scale(iv);
    bool whole = (int32_t)(iv->image.w * s) <= iv->w &&
                 (int32_t)(iv->image.h * s) <= iv->h;
    if (whole) { iv->sx = 0.5; iv->sy = 0.5; }
    ui_view.invalidate(&iv->view, null);
    ui_image_show_tools(iv, true);
}

static void ui_image_mouse_scroll(ui_view_t* v, ui_point_t dx_dy) {
    fp64_t dx = (fp64_t)dx_dy.x;
    fp64_t dy = (fp64_t)dx_dy.y;
    ui_image_t* iv = (ui_image_t*)v;
    if (ui_view.has_focus(v)) {
        fp64_t s = ui_image.scale(iv);
        if (iv->image.w * s > iv->w || iv->image.h * s > iv->h) {
            iv->sx = max(0.0, min(iv->sx + dx / iv->image.w, 1.0));
        } else {
            iv->sx = 0.5;
        }
        if (iv->image.h * s > iv->h) {
            iv->sy = max(0.0, min(iv->sy + dy / iv->image.h, 1.0));
        } else {
            iv->sy = 0.5;
        }
        ui_view.invalidate(&iv->view, null);
    }
}

static bool ui_image_tap(ui_view_t* v, int32_t ix, bool pressed) {
    bool swallow = false;
    if (v->focusable) {
        ui_image_t* iv = (ui_image_t*)v;
        const int32_t x = ui_app.mouse.x - iv->x;
        const int32_t y = ui_app.mouse.y - iv->y;
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
        swallow = inside || tools;
    }
//  rt_println("inside %s", inside ? "true" : "false");
    return swallow;
}

static bool ui_image_mouse_move(ui_view_t* v) {
    ui_image_t* iv = (ui_image_t*)v;
    bool drag_started = iv->drag_start.x >= 0 && iv->drag_start.y >= 0;
    bool tools  = !iv->tool.bar.state.hidden &&
                  ui_view.inside(&iv->tool.bar, &ui_app.mouse);
    bool inside = ui_view.inside(&iv->view, &ui_app.mouse) && !tools;
    if (drag_started && inside) {
        ui_image_show_tools(iv, false);
        const int32_t x = ui_app.mouse.x - iv->x;
        const int32_t y = ui_app.mouse.y - iv->y;
        ui_point_t dx_dy = {iv->drag_start.x - x, iv->drag_start.y - y};
        ui_image_mouse_scroll(v, dx_dy);
        iv->drag_start = (ui_point_t){x, y};
    } else if (inside) {
        ui_image_show_tools(iv, true);
    } else if (!inside && !tools) {
        ui_image_show_tools(iv, false);
    }
//  rt_println("inside %s", inside ? "true" : "false");
    return inside;
}

static bool ui_image_key_pressed(ui_view_t* v, int64_t vk) {
    ui_image_t* iv = (ui_image_t*)v;
    bool swallowed = false;
    if (ui_view.has_focus(v)) {
        swallowed = true;
        if (vk == ui.key.up) {
            ui_image_mouse_scroll(v, (ui_point_t){0, -iv->h / 8});
        } else if (vk == ui.key.down) {
            ui_image_mouse_scroll(v, (ui_point_t){0, +iv->h / 8});
        } else if (vk == ui.key.left) {
            ui_image_mouse_scroll(v, (ui_point_t){-iv->w / 8, 0});
        } else if (vk == ui.key.right) {
            ui_image_mouse_scroll(v, (ui_point_t){+iv->w / 8, 0});
        } else if (vk == ui.key.plus) {
            if (iv->zoom < 8) {
                iv->zoom++;
                ui_image_zoomed(iv);
            }
        } else if (vk == ui.key.minus) {
            if (iv->zoom > 0) {
                iv->zoom--;
                ui_image_zoomed(iv);
            }
        } else {
            swallowed = false;
        }
    }
    return swallowed;
}

static void ui_image_zoom_in(ui_button_t* b) {
    ui_image_t* iv = (ui_image_t*)b->that;
    if (iv->zoom < 8) {
        iv->zoom++;
        ui_image_zoomed(iv);
    }
}

static void ui_image_zoom_out(ui_button_t* b) {
    ui_image_t* iv = (ui_image_t*)b->that;
    if (iv->zoom > 0) {
        iv->zoom--;
        ui_image_zoomed(iv);
    }
}

static void ui_image_fit(ui_button_t* b) {
    ui_image_t* iv = (ui_image_t*)b->that;
    iv->fit  = true;
    iv->fill = false;
    ui_image_fit_fill_scale(iv);
    ui_view.invalidate(&iv->view, null);
}

static void ui_image_fill(ui_button_t* b) {
    ui_image_t* iv = (ui_image_t*)b->that;
    iv->fill = true;
    iv->fit  = false;
    ui_image_fit_fill_scale(iv);
    ui_view.invalidate(&iv->view, null);
}

static void ui_image_zoom_1t1(ui_button_t* b) {
    ui_image_t* iv = (ui_image_t*)b->that;
    iv->zoom = 4;
    ui_image_zoomed(iv);
}

static ui_label_t ui_image_about = ui_label(0,
    "Keyboard shortcuts:\n\n"
    "Ctrl+C copies image to the clipboard.\n\n"
    rt_glyph_heavy_plus_sign " zoom in; "
    rt_glyph_heavy_minus_sign " zoom out;\n"
    rt_glyph_open_circle_arrows_one_overlay " 1:1.\n\n"
    rt_glyph_up_down_arrow " Fit;\n"
    rt_glyph_left_right_arrow " Fill.\n\n"
    "Left/Right Arrows "
    rt_glyph_leftward_arrow
    rt_glyph_rightwards_arrow
    "Up/Down Arrows "
    rt_glyph_upwards_arrow
    rt_glyph_downwards_arrow
    "\npans the image inside view.\n\n"
    "Mouse wheel or mouse / touchpad hold and drag to pan.\n"
);

static void ui_image_help(ui_button_t* rt_unused(b)) {
    ui_app.show_toast(&ui_image_about, 7.0);
}

static void ui_image_copy_to_clipboard(ui_image_t* iv) {
    ui_bitmap_t image = {0};
    if (iv->image.texture != null) {
        rt_clipboard.put_image(&iv->image);
    } else {
        ui_gdi.bitmap_init(&image, iv->image.w, iv->image.h,
                                  iv->image.bpp, iv->image.pixels);
        rt_clipboard.put_image(&image);
        ui_gdi.bitmap_dispose(&image);
    }
    static ui_label_t hint = ui_label(0.0f, "copied to clipboard");
    ui_app.show_hint(&hint, ui_app.mouse.x,
                            ui_app.mouse.y + iv->fm->height,
                     1.5);
}

static void ui_image_copy(ui_button_t* b) {
    ui_image_t* iv = (ui_image_t*)b->that;
    ui_image_copy_to_clipboard(iv);
}

static void ui_image_character(ui_view_t* v, const char* utf8) {
    ui_image_t* iv = (ui_image_t*)v;
    if (ui_view.has_focus(v)) { // && ui_app.ctrl ?
        char ch = utf8[0];
        if (ch == '+' || ch == '=') {
            if (iv->zoom < 8) {
                iv->zoom++;
                ui_image_zoomed(iv);
            }
        } else if (ch == '-' || ch == '_') {
            if (iv->zoom > 0) {
                iv->zoom--;
                ui_image_zoomed(iv);
            }
        } else if (ch == '<' || ch == ',') {
            ui_image_mouse_scroll(v, (ui_point_t){-iv->w / 8, 0});
        } else if (ch == '>' || ch == '.') {
            ui_image_mouse_scroll(v, (ui_point_t){+iv->w / 8, 0});
        } else if (ch == '0') {
            iv->zoom = 4;
            ui_image_zoomed(iv);
        } else if (ch == 3 && iv->image.pixels != null) { // Ctrl+C
            ui_image_copy_to_clipboard(iv);
        }
    }
}

static void ui_image_add_button(ui_image_t* iv, ui_button_t* b,
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
    b->fm = &ui_app.fm.mono.normal;
    b->min_w_em = 1.5f;
    rt_str_printf(b->hint, "%s", hint);
    ui_view.add_last(&iv->tool.bar, b);
}

void ui_image_init(ui_image_t* iv) {
    memset(iv, 0x00, sizeof(*iv));
    iv->type         = ui_view_image;
    iv->paint        = ui_image_paint;
    iv->tap          = ui_image_tap;
    iv->mouse_move   = ui_image_mouse_move;
    iv->measure      = ui_image_measure;
    iv->layout       = ui_image_layout;
    iv->every_100ms  = ui_image_every_100ms;
    iv->focus_lost   = ui_image_focus_lost;
    iv->focus_gained = ui_image_focus_gained;
    iv->mouse_scroll = ui_image_mouse_scroll;
    iv->character    = ui_image_character;
    iv->key_pressed  = ui_image_key_pressed;
    iv->fm           = &ui_app.fm.prop.normal;
    iv->tool.bar = (ui_view_t)ui_view(span);
    // buttons:
    ui_image_add_button(iv, &iv->tool.copy, "\xF0\x9F\x93\x8B", ui_image_copy,
        "Copy to Clipboard Ctrl+C");
    ui_image_add_button(iv, &iv->tool.zoom_out,
                    rt_glyph_heavy_minus_sign,
                    ui_image_zoom_out, "Zoom Out");
    ui_image_add_button(iv, &iv->tool.zoom_1t1,
                    rt_glyph_open_circle_arrows_one_overlay,
                    ui_image_zoom_1t1, "Reset to 1:1");
    ui_image_add_button(iv, &iv->tool.zoom_in,
                     rt_glyph_heavy_plus_sign,
                     ui_image_zoom_in,  "Zoom In");
    ui_image_add_button(iv, &iv->tool.fit,
                     rt_glyph_up_down_arrow,
                     ui_image_fit,  "Fit");
    ui_image_add_button(iv, &iv->tool.fill,
                     rt_glyph_left_right_arrow,
                     ui_image_fill,  "Fill");
    ui_image_add_button(iv, &iv->tool.help,
                     "?", ui_image_help, "Help");
    iv->tool.zoom_1t1.min_w_em = 1.25f;
    iv->tool.ratio = (ui_label_t)ui_label(0, "1:1");
    iv->tool.ratio.color = ui_colors.get_color(ui_color_id_highlight);
    iv->tool.ratio.color_id = ui_color_id_highlight;
    ui_view.add_last(&iv->view, &iv->tool.bar);
    ui_view.add_last(&iv->view, &iv->tool.ratio);
    iv->tool.bar.state.hidden = true;
    iv->tool.ratio.state.hidden = true;
    iv->tool.bar.erase   = ui_image_tools_background;
    iv->tool.ratio.erase = ui_image_tools_background;
    iv->zoom = 4;
    iv->zn = 1;
    iv->zd = 1;
    iv->sx = 0.5;
    iv->sy = 0.5;
    iv->drag_start = (ui_point_t){-1, -1};
    iv->debug.id = "#image";
}

void ui_image_init_with(ui_image_t* iv, const uint8_t* pixels,
                                  int32_t w, int32_t h,
                                  int32_t c, int32_t s) {
    ui_image_init(iv);
    iv->image.pixels = (uint8_t*)pixels;
    iv->image.w = w;
    iv->image.h = h;
    iv->image.bpp = c;
    iv->image.stride = s;
}

static void ui_image_ratio(ui_image_t* iv, int32_t zn, int32_t zd) {
    rt_swear(0 < zn && zn <= 16);
    rt_swear(0 < zd && zd <= 16);
    // only 1:2 and 2:1 etc are supported:
    if (zn != 1) { rt_swear(zd == 1); }
    if (zd != 1) { rt_swear(zn == 1); }
    iv->zn = zn;
    iv->zd = zd;
    iv->fit  = false;
    iv->fill = false;
}

ui_image_if ui_image = {
    .init      = ui_image_init,
    .init_with = ui_image_init_with,
    .ratio     = ui_image_ratio,
    .scale     = ui_image_scale,
    .position  = ui_image_position
};

