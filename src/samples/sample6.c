/* Copyright (c) Dmitry "Leo" Kuznetsov 2021-24 see LICENSE for details */
#include "single_file_lib/rt/rt.h"
#include "single_file_lib/ui/ui.h"
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
    rt_event_t  quit;
    rt_thread_t thread;
    uint32_t seed; // for rt_num.random32()
    int32_t  x;
    int32_t  y;
    int32_t  speed_x;
    int32_t  speed_y;
} animation;

static bool   muted;
static fp64_t volume; // be mute

static ui_midi_t midi;

static ui_bitmap_t  background;

static void init(void);
static void fini(void);
static void character(ui_view_t* view, const char* utf8);
static void stop_and_close(void);
static void open_and_play(void);

static void* load_image(const uint8_t* data, int64_t bytes, int32_t* w, int32_t* h,
    int32_t* bpp, int32_t preferred_bytes_per_pixel);

static void* load_animated_gif(const uint8_t* data, int64_t bytes,
    int32_t** delays, int32_t* w, int32_t* h, int32_t* frames, int32_t* bpp,
    int32_t preferred_bytes_per_pixel);

static const char* midi_file(void) {
    static char filename[rt_files_max_path];
    if (filename[0] == 0) {
        void* data = null;
        int64_t bytes = 0;
        int r = rt_mem.map_resource("mr_blue_sky_midi", &data, &bytes);
        rt_fatal_if_error(r);
        rt_fatal_if_error(rt_files.create_tmp(filename,
                                      rt_countof(filename)));
        rt_assert(filename[0] != 0);
        int64_t written = 0;
        rt_fatal_if_error(rt_files.write_fully(filename, data, bytes,
                                                        &written));
        rt_assert(written == bytes);
    }
    return filename;
}

static void paint(ui_view_t* view) {
    if (animation.x < 0 && animation.y < 0) {
        animation.x = (view->w - gif.w) / 2;
        animation.y = (view->h - gif.h) / 2;
    }
    ui_gdi.fill(0, 0, view->w, view->h, ui_colors.black);
    int32_t w = rt_min(view->w, background.w);
    int32_t h = rt_min(view->h, background.h);
    int32_t x = (view->w - w) / 2;
    int32_t y = (view->h - h) / 2;
    ui_gdi.set_clip(0, 0, view->w, view->h);
    ui_gdi.bitmap(x, y, w, h, 0, 0, background.w, background.h, &background);
    ui_gdi.set_clip(0, 0, 0, 0);
    if (gif.pixels != null) {
        uint8_t* p = gif.pixels + gif.w * gif.h * gif.bpp * animation.index;
        ui_bitmap_t frame = { 0 };
        ui_gdi.bitmap_init(&frame, gif.w, gif.h, gif.bpp, p);
        x = animation.x - gif.w / 2;
        y = animation.y - gif.h / 2;
        ui_gdi.alpha(x, y, gif.w, gif.h, 0,0, frame.w, frame.h, &frame, 1.0);
        ui_gdi.bitmap_dispose(&frame);
    }
    ui_gdi_ta_t ta = ui_gdi.ta.prop.H1;
    ta.color_id = 0;
    ta.color = muted ? ui_colors.green : ui_colors.red;
    ui_gdi.text(&ta, 0, 0, "%s", muted ?
        rt_glyph_speaker : rt_glyph_mute);
}

static void character(ui_view_t* rt_unused(view), const char* utf8) {
    if (utf8[0] == 'q' || utf8[0] == 'Q' || utf8[0] == 033) {
        ui_app.close();
    }
}

// midi sequencer can "pause()" but actually by stopping
//      playing the stream and sending `aborted` notification.
//      .resume() says this operation is not supported.
// .open() after .stop() .close() takes 4+ seconds

static bool tap(ui_view_t* rt_unused(v), int32_t ix, bool pressed) {
    const bool inside =
        0 <= ui_app.mouse.x && ui_app.mouse.x < ui_app.fm.prop.H1.em.w &&
        0 <= ui_app.mouse.y && ui_app.mouse.y < ui_app.fm.prop.H1.em.h;
    if (pressed && inside) {
        muted = !muted;
        if (muted) {
            if (ui_midi.is_playing(&midi) && !ui_midi.is_paused(&midi)) {
                rt_fatal_if_error(ui_midi.get_volume(&midi, &volume));
                rt_fatal_if_error(ui_midi.set_volume(&midi,  0));
#ifdef UI_MIDI_PAUSE_RESUME
                rt_fatal_if_error(ui_midi.pause(&midi));
#endif
            }
        } else {
            rt_fatal_if_error(ui_midi.set_volume(&midi,  volume));
#ifdef UI_MIDI_PAUSE_RESUME
            if (!ui_midi.is_open(&midi)) {
                open_and_play();
            } else if (!ui_midi.is_playing(&midi)) {
                rt_fatal_if_error(ui_midi.play(&midi));
            } else {
                rt_fatal_if_error(ui_midi.resume(&midi));
            }
#endif
        }
    }
    return pressed && inside; // swallow mouse clicks inside mute button
}

static int64_t notify(ui_midi_t* m, int64_t flags) {
    rt_swear(&midi == m);
    rt_println();
    if (flags & ui_midi.aborted)    { rt_println("aborted"); }
    if (flags & ui_midi.successful) { rt_println("successful"); }
    if (flags & ui_midi.superseded) { rt_println("superseded"); }
    if (flags & ui_midi.failure)    { rt_println("failure"); }
    if (flags & ui_midi.success)    { rt_println("success"); }
    // aborted : is received on ui_midi.stop()
    // success : is received on the end of mini sequence playback
//  if ((flags & ui_midi.aborted) != 0 || (flags & ui_midi.success) != 0) {
    if ((flags & ui_midi.aborted) != 0) {
        stop_and_close();
    } else if ((flags & ui_midi.success) != 0) {
        rt_fatal_if_error(ui_midi.stop(&midi));
        rt_fatal_if_error(ui_midi.rewind(&midi));
        rt_fatal_if_error(ui_midi.play(&midi));
    }
    return 0;
}

static void stop_and_close(void) {
    if (ui_midi.is_open(&midi)) {
        if (ui_midi.is_playing(&midi)) {
            midi.notify = null;
            rt_fatal_if_error(ui_midi.stop(&midi));
            ui_midi.close(&midi);
        }
    }
}

static void open_and_play(void) {
    if (!ui_midi.is_open(&midi)) {
//      fp64_t t = rt_clock.seconds();
        // first call to MIDI Sequencer .open() takes 1.122 seconds
        // next  attempt to .open() after .close() takes 4.237 seconds!
        rt_fatal_if_error(ui_midi.open(&midi, midi_file()));
//      rt_println("%.6f seconds", rt_clock.seconds() - t);
    }
    if (!ui_midi.is_playing(&midi)) {
        midi.notify = notify;
        rt_fatal_if_error(ui_midi.play(&midi));
    }
}

static void delete_midi_file(void) {
    rt_fatal_if_error(rt_files.unlink(midi_file()));
}

static void load_gif(void) {
    void* data = null;
    int64_t bytes = 0;
    errno_t r = rt_mem.map_resource("groot_gif", &data, &bytes);
    rt_fatal_if_error(r);
    // load_animated_gif() calls realloc(delays) w/o first alloc()
    r = rt_heap.allocate(null, (void**)&gif.delays, sizeof(int32_t), false);
    rt_swear(r == 0 && gif.delays != null);
    gif.pixels = load_animated_gif(data, bytes, &gif.delays,
        &gif.w, &gif.h, &gif.frames, &gif.bpp, 4);
    if (gif.pixels == null || gif.bpp != 4 || gif.frames < 1) {
        rt_println("%s", stbi_failure_reason());
    }
    rt_fatal_if(gif.pixels == null || gif.bpp != 4 || gif.frames < 1);
    // resources cannot be unmapped do not call rt_mem.unmap()
}

static void animate(void) {
    for (;;) {
        ui_app.request_redraw();
        fp64_t delay_in_seconds = gif.delays[animation.index] * 0.001;
        if (rt_event.wait_or_timeout(animation.quit, delay_in_seconds) == 0) {
            break;
        }
        if (animation.x >= 0 && animation.y >= 0) {
//          rt_println("%d %d speed: %d %d", animation.x, animation.y, animation.speed_x, animation.speed_y);
            animation.index = (animation.index + 1) % gif.frames;
            while (animation.speed_x == 0) {
                animation.speed_x = rt_num.random32(&animation.seed) % (max_speed * 2 + 1) - max_speed;
            }
            while (animation.speed_y == 0) {
                animation.speed_y = rt_num.random32(&animation.seed) % (max_speed * 2 + 1) - max_speed;
            }
            animation.x += animation.speed_x;
            animation.y += animation.speed_y;
            if (animation.x - gif.w / 2 < 0) {
                animation.x = gif.w / 2;
                animation.speed_x = -animation.speed_x;
            } else if (animation.x + gif.w / 2 >= ui_app.root->w) {
                animation.x = ui_app.root->w - gif.w / 2 - 1;
                animation.speed_x = -animation.speed_x;
            }
            if (animation.y - gif.h / 2 < 0) {
                animation.y = gif.h / 2;
                animation.speed_y = -animation.speed_y;
            } else if (animation.y + gif.h / 2 >= ui_app.root->h) {
                animation.y = ui_app.root->h - gif.h / 2 - 1;
                animation.speed_y = -animation.speed_y;
            }
            int inc = rt_num.random32(&animation.seed) % 2 == 0 ? -1 : +1;
            if (rt_num.random32(&animation.seed) % 2 == 0) {
                if (1 <= animation.speed_x + inc &&
                    animation.speed_x + inc < max_speed) {
                    animation.speed_x += inc;
                }
            } else {
                if (1 <= animation.speed_y + inc &&
                    animation.speed_y + inc < max_speed) {
                    animation.speed_y += inc;
                }
            }
        }
    }
}

static void animated_gif_loader(void* rt_unused(ignored)) {
    ui_cursor_t cursor = ui_app.cursor;
    ui_app.set_cursor(ui_app.cursors.wait);
    load_gif();
    ui_app.set_cursor(cursor);
    animate();
}

static void opened(void) {
    animation.seed = (uint32_t)rt_clock.nanoseconds();
    animation.x = -1;
    animation.y = -1;
    animation.quit = rt_event.create();
    animation.thread = rt_thread.start(animated_gif_loader, null);
    open_and_play();
}

static void init(void) {
    ui_app.title = title;
    ui_app.content->paint     = paint;
    ui_app.content->character = character;
    ui_app.content->tap       = tap;
    ui_app.opened             = opened;
    void* data = null;
    int64_t bytes = 0;
    rt_fatal_if_error(rt_mem.map_resource("sample_png", &data, &bytes));
    int w = 0;
    int h = 0;
    int bpp = 0; // bytes (!) per pixel
    void* pixels = load_image(data, bytes, &w, &h, &bpp, 0);
    if (pixels == null) {
        rt_println("%s", stbi_failure_reason());
    }
    rt_not_null(pixels);
    ui_gdi.bitmap_init(&background, w, h, bpp, pixels);
    stbi_image_free(pixels);
}

static void fini(void) {
    rt_event.set(animation.quit);
    rt_thread.join(animation.thread, -1);
    rt_event.dispose(animation.quit);
    ui_gdi.bitmap_dispose(&background);
    stbi_image_free(gif.pixels);
    stbi_image_free(gif.delays);
    stop_and_close();
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
    rt_fatal_if(true, "%s only SUBSYSTEM:WINDOWS", rt_args.basename());
    return 1;
}

ui_app_t ui_app = {
    .class_name = "sample6",
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
