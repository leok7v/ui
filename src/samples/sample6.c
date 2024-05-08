/* Copyright (c) Dmitry "Leo" Kuznetsov 2021-24 see LICENSE for details */
#include "single_file_lib/ut/ut.h"
#include "single_file_lib/ui/ui.h"
#include "midi.h"
#include "stb_image.h"

const char* title = "Sample6: I am groot";

static struct {
    int32_t bpp; // bytes per pixel
    int32_t w;
    int32_t h;
    int32_t  frames;
    int32_t* delays; // delays[frames];
    uint8_t* pixels;
} gif; // animated

enum { max_speed = 3 };

static struct {
    int32_t  index; // animation index 0..gif.frames - 1
    event_t  quit;
    thread_t thread;
    uint32_t seed; // for ut_num.random32()
    int32_t  x;
    int32_t  y;
    int32_t  speed_x;
    int32_t  speed_y;
} animation;

static bool muted;

static midi_t mds;

#define glyph_mute "\xF0\x9F\x94\x87"
#define glyph_speaker "\xF0\x9F\x94\x88"

static ui_image_t  background;

static void init(void);
static void fini(void);
static void character(ui_view_t* view, const char* utf8);

static void* load_image(const uint8_t* data, int64_t bytes, int32_t* w, int32_t* h,
    int32_t* bpp, int32_t preferred_bytes_per_pixel);

static void* load_animated_gif(const uint8_t* data, int64_t bytes,
    int32_t** delays, int32_t* w, int32_t* h, int32_t* frames, int32_t* bpp,
    int32_t preferred_bytes_per_pixel);

static const char* midi_file(void) {
    static char filename[ut_files_max_path];
    if (filename[0] == 0) {
        void* data = null;
        int64_t bytes = 0;
        int r = ut_mem.map_resource("mr_blue_sky_midi", &data, &bytes);
        fatal_if_not_zero(r);
        fatal_if_not_zero(ut_files.create_tmp(filename,
                                      countof(filename)));
        assert(filename[0] != 0);
        int64_t written = 0;
        fatal_if_not_zero(ut_files.write_fully(filename, data, bytes,
                                                        &written));
        assert(written == bytes);
    }
    return filename;
}

static void paint(ui_view_t* view) {
    if (animation.x < 0 && animation.y < 0) {
        animation.x = (view->w - gif.w) / 2;
        animation.y = (view->h - gif.h) / 2;
    }
    ui_gdi.set_brush(ui_gdi.brush_color);
    ui_gdi.set_brush_color(ui_colors.black);
    ui_gdi.fill(0, 0, view->w, view->h);
    int32_t w = ut_min(view->w, background.w);
    int32_t h = ut_min(view->h, background.h);
    int32_t x = (view->w - w) / 2;
    int32_t y = (view->h - h) / 2;
    ui_gdi.set_clip(0, 0, view->w, view->h);
    ui_gdi.draw_image(x, y, w, h, &background);
    ui_gdi.set_clip(0, 0, 0, 0);
    if (gif.pixels != null) {
        uint8_t* p = gif.pixels + gif.w * gif.h * gif.bpp * animation.index;
        ui_image_t frame = { 0 };
        ui_gdi.image_init(&frame, gif.w, gif.h, gif.bpp, p);
        x = animation.x - gif.w / 2;
        y = animation.y - gif.h / 2;
        ui_gdi.alpha_blend(x, y, gif.w, gif.h, &frame, 1.0);
        ui_gdi.image_dispose(&frame);
    }
    ui_font_t f = ui_gdi.set_font(ui_app.fonts.H1.font);
    ui_gdi.x = 0;
    ui_gdi.y = 0;
    ui_gdi.set_text_color(muted ? ui_colors.green : ui_colors.red);
    ui_gdi.text("%s", muted ? glyph_speaker : glyph_mute);
    ui_gdi.set_font(f);
}

static void character(ui_view_t* unused(view), const char* utf8) {
    if (utf8[0] == 'q' || utf8[0] == 'Q' || utf8[0] == 033) {
        ui_app.close();
    }
}

static void mouse(ui_view_t* unused(view), int32_t m, int64_t unused(f)) {
    if ((m == ui.message.left_button_pressed ||
        m == ui.message.right_button_pressed) &&
        0 <= ui_app.mouse.x && ui_app.mouse.x < ui_app.fonts.H1.em.w &&
        0 <= ui_app.mouse.y && ui_app.mouse.y < ui_app.fonts.H1.em.h) {
        muted = !muted;
        if (muted) {
            midi.stop(&mds);
//          midi.close(&mds);
        } else {
//          midi.open(&mds, ui_app.window, midi_file());
            midi.play(&mds);
        }
    }
}

static void opened(void) {
    midi.open(&mds, ui_app.window, midi_file());
    midi.play(&mds);
}

static bool message(ui_view_t* unused(view), int32_t m, int64_t wp, int64_t lp,
        int64_t* unused(ret)) {
    if (m == midi.notify && (wp & midi.successful) != 0 && lp == mds.device_id) {
        midi.stop(&mds);
        midi.close(&mds);
        midi.open(&mds, ui_app.window, midi_file());
        midi.play(&mds);
    }
    return m == midi.notify;
}

static void delete_midi_file(void) {
    fatal_if_not_zero(ut_files.unlink(midi_file()));
}

static void load_gif(void) {
    void* data = null;
    int64_t bytes = 0;
    int r = ut_mem.map_resource("groot_gif", &data, &bytes);
    fatal_if_not_zero(r);
    // load_animated_gif() calls realloc(delays) w/o first malloc()
    gif.delays = malloc(sizeof(int32_t));
    gif.pixels = load_animated_gif(data, bytes, &gif.delays,
        &gif.w, &gif.h, &gif.frames, &gif.bpp, 4);
    fatal_if(gif.pixels == null || gif.bpp != 4 || gif.frames < 1);
    // resources cannot be unmapped do not call ut_mem.unmap()
}

static void animate(void) {
    for (;;) {
        ui_app.request_redraw();
        fp64_t delay_in_seconds = gif.delays[animation.index] * 0.001;
        if (ut_event.wait_or_timeout(animation.quit, delay_in_seconds) == 0) {
            break;
        }
        if (animation.x >= 0 && animation.y >= 0) {
//          traceln("%d %d speed: %d %d", animation.x, animation.y, animation.speed_x, animation.speed_y);
            animation.index = (animation.index + 1) % gif.frames;
            while (animation.speed_x == 0) {
                animation.speed_x = ut_num.random32(&animation.seed) % (max_speed * 2 + 1) - max_speed;
            }
            while (animation.speed_y == 0) {
                animation.speed_y = ut_num.random32(&animation.seed) % (max_speed * 2 + 1) - max_speed;
            }
            animation.x += animation.speed_x;
            animation.y += animation.speed_y;
            if (animation.x - gif.w / 2 < 0) {
                animation.x = gif.w / 2;
                animation.speed_x = -animation.speed_x;
            } else if (animation.x + gif.w / 2 >= ui_app.crc.w) {
                animation.x = ui_app.crc.w - gif.w / 2 - 1;
                animation.speed_x = -animation.speed_x;
            }
            if (animation.y - gif.h / 2 < 0) {
                animation.y = gif.h / 2;
                animation.speed_y = -animation.speed_y;
            } else if (animation.y + gif.h / 2 >= ui_app.crc.h) {
                animation.y = ui_app.crc.h - gif.h / 2 - 1;
                animation.speed_y = -animation.speed_y;
            }
            int inc = ut_num.random32(&animation.seed) % 2 == 0 ? -1 : +1;
            if (ut_num.random32(&animation.seed) % 2 == 0) {
                if (1 <= animation.speed_x + inc && animation.speed_x + inc < max_speed) {
                    animation.speed_x += inc;
                }
            } else {
                if (1 <= animation.speed_y + inc && animation.speed_y + inc < max_speed) {
                    animation.speed_y += inc;
                }
            }
        }
    }
}

static void startup(void* unused(ignored)) {
    ui_cursor_t cursor = ui_app.cursor;
    ui_app.set_cursor(ui_app.cursor_wait);
    load_gif();
    ui_app.set_cursor(cursor);
    animate();
}

static void init(void) {
    ui_app.title = title;
    ui_app.view->paint     = paint;
    ui_app.view->character = character;
    ui_app.view->message   = message;
    ui_app.view->mouse     = mouse;
    ui_app.opened        = opened;
    animation.seed = (uint32_t)ut_clock.nanoseconds();
    animation.x = -1;
    animation.y = -1;
    animation.quit = ut_event.create();
    animation.thread = ut_thread.start(startup, null);
    void* data = null;
    int64_t bytes = 0;
    fatal_if_not_zero(ut_mem.map_resource("sample_png", &data, &bytes));
    int w = 0;
    int h = 0;
    int bpp = 0; // bytes (!) per pixel
    void* pixels = load_image(data, bytes, &w, &h, &bpp, 0);
    fatal_if_null(pixels);
    ui_gdi.image_init(&background, w, h, bpp, pixels);
    free(pixels);
}

static void fini(void) {
    ui_gdi.image_dispose(&background);
    free(gif.pixels);
    free(gif.delays);
    ut_event.set(animation.quit);
    ut_thread.join(animation.thread, -1);
    ut_event.dispose(animation.quit);
    midi.stop(&mds);
    midi.close(&mds);
    delete_midi_file();
}

static void* load_image(const uint8_t* data, int64_t bytes, int32_t* w, int32_t* h,
    int32_t* bpp, int32_t preferred_bytes_per_pixel) {
    void* pixels = stbi_load_from_memory((uint8_t const*)data, (int32_t)bytes, w, h,
        bpp, preferred_bytes_per_pixel);
    return pixels;
}

static void* load_animated_gif(const uint8_t* data, int64_t bytes,
    int32_t** delays, int32_t* w, int32_t* h, int32_t* frames, int32_t* bpp,
    int32_t preferred_bytes_per_pixel) {
    stbi_uc* pixels = stbi_load_gif_from_memory(data, (int32_t)bytes,
        delays, w, h, frames,
        bpp, preferred_bytes_per_pixel);
    return pixels;
}


static int  console(void) {
    fatal_if(true, "%s only SUBSYSTEM:WINDOWS", ut_args.basename());
    return 1;
}

ui_app_t ui_app = {
    .class_name = "sample4",
    .dark_mode = true,
    .init = init,
    .fini = fini,
    .main = console,
    .window_sizing = {
        .min_w =  6.0f,
        .min_h =  5.0f,
        .ini_w =  8.0f,
        .ini_h =  6.0f
    }
};
