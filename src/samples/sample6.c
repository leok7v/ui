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

typedef struct animation_s {
    int32_t  index; // animation index 0..gif.frames - 1
    uint32_t seed; // for rt_num.random32()
    int32_t  x;
    int32_t  y;
    int32_t  w;
    int32_t  h;
    int32_t  speed_x;
    int32_t  speed_y;
} animation_t;

static animation_t animation;
static rt_thread_t thread; // gif loader thread

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

static void paint(ui_view_t* v) {
    int32_t w = rt_min(v->w, background.w);
    int32_t h = rt_min(v->h, background.h);
    int32_t x = (v->w - w) / 2;
    int32_t y = (v->h - h) / 2;
    ui_gdi.set_clip(0, 0, v->w, v->h);
    ui_gdi.bitmap(x, y, w, h, 0, 0, background.w, background.h, &background);
    ui_gdi.set_clip(0, 0, 0, 0);
    if (gif.pixels != null) {
        uint8_t* p = gif.pixels + gif.w * gif.h * gif.bpp * animation.index;
        ui_bitmap_t frame = { 0 };
        ui_gdi.bitmap_init(&frame, gif.w, gif.h, gif.bpp, p);
        x = animation.x - animation.w / 2;
        y = animation.y - animation.h / 2;
        ui_gdi.alpha(x, y, animation.w, animation.h, 0, 0,
                    frame.w, frame.h, &frame, 1.0);
        ui_gdi.bitmap_dispose(&frame);
    }
    ui_gdi_ta_t ta = ui_gdi.ta.prop.H1;
    ta.color_id = 0;
    ta.color = muted ? ui_colors.green : ui_colors.red;
    #define str_unmuted rt_glyph_mute  " mute"
    #define str_muted rt_glyph_speaker " unmute"
    const int32_t mx = v->x + ui_app.fm.prop.H1.em.w / 16;
    const int32_t my = v->y + ui_app.fm.prop.H1.em.h / 16;
    const int32_t mw = ui_app.fm.prop.H1.em.w * 4;
    const int32_t mh = ui_app.fm.prop.H1.em.h;
    ui_gdi.fill(mx, my, mw, mh, ui_colors.moonstone_blue);
    ui_gdi.text(&ta, 0, 0, "%s", muted ? str_muted : str_unmuted);
}

static void character(ui_view_t* rt_unused(v), const char* utf8) {
    if (utf8[0] == 'q' || utf8[0] == 'Q' || utf8[0] == 033) {
        ui_app.close();
    }
}

static bool tap(ui_view_t* rt_unused(v), int32_t ix, bool pressed) {
    const int32_t w = ui_app.fm.prop.H1.em.w * 4;
    const int32_t h = ui_app.fm.prop.H1.em.h;
    const bool inside =
        0 <= ui_app.mouse.x && ui_app.mouse.x < w &&
        0 <= ui_app.mouse.y && ui_app.mouse.y < h;
    const bool swallow = inside && pressed;
    if (swallow) {
        muted = !muted;
        if (muted) {
            if (ui_midi.is_playing(&midi)) {
                rt_fatal_if_error(ui_midi.get_volume(&midi, &volume));
                rt_fatal_if_error(ui_midi.set_volume(&midi,  0));
            }
        } else {
            rt_fatal_if_error(ui_midi.set_volume(&midi,  volume));
        }
    }
    return swallow; // swallows taps inside mute `button`
}

static int64_t notify(ui_midi_t* m, int64_t f) { // f: f
    rt_swear(&midi == m);
    #ifdef UI_MIDI_DEBUG
        if (f & ui_midi.success)    { rt_println("success"); }
        if (f & ui_midi.failure)    { rt_println("failure"); }
        if (f & ui_midi.aborted)    { rt_println("aborted"); }
        if (f & ui_midi.superseded) { rt_println("superseded"); }
    #endif // UI_MIDI_DEBUG
    if ((f & (ui_midi.aborted|ui_midi.failure)) != 0) {
        stop_and_close();
    } else if ((f & ui_midi.success) != 0) {
        // success : is received on the end of mini sequence playback
        // "when the music over..." rewind and start playing again
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
        // first call to MIDI Sequencer .open() takes 1.122 seconds
        // next  attempt to .open() after .close() takes 4.237 seconds!
        // what can possibly take 1 second on 2GH cpu?
        rt_fatal_if_error(ui_midi.open(&midi, midi_file()));
    }
    if (!ui_midi.is_playing(&midi)) {
        midi.notify = notify;
        rt_fatal_if_error(ui_midi.play(&midi));
        // it is possible that last mute call leaves the volume to 0
        rt_fatal_if_error(ui_midi.set_volume(&midi, 0.5));
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
    // gif.pixels assignment is "atomic"
    gif.pixels = load_animated_gif(data, bytes, &gif.delays,
        &gif.w, &gif.h, &gif.frames, &gif.bpp, 4);
    rt_swear(gif.pixels != null && gif.bpp == 4 && gif.frames >= 1,
             "%s", stbi_failure_reason());
    // resources cannot be unmapped do not call rt_mem.unmap()
}

static void animation_step(rt_work_t* work);

static void schedule_next_step(rt_work_t* work) {
    rt_swear(0 <= animation.index && animation.index < gif.frames);
    // milliseconds to seconds:
    fp64_t ds = gif.delays[animation.index] * 0.001; // delay in seconds
    work->when = rt_clock.seconds() + ds;
    ui_app.post(work);
}

static void animation_step(rt_work_t* work) {
    rt_swear(rt_thread.id() == ui_app.tid);
    int32_t multiplier = 1;
    while (gif.w * multiplier < ui_app.crc.w / 2 &&
           gif.h * multiplier < ui_app.crc.h / 2) {
        multiplier++;
    }
    animation_t* a = &animation; // shorthand naming
    a->w = gif.w * multiplier;
    a->h = gif.h * multiplier;
    if (a->x < 0 && a->y < 0) {
        a->x = (ui_app.crc.w - a->w) / 2;
        a->y = (ui_app.crc.h - a->h) / 2;
    }
//  rt_println("%d %d speed: %d %d", a->x, a->y,
//                                   a->speed_x,
//                                   a->speed_y);
    a->index = (a->index + 1) % gif.frames;
    while (a->speed_x == 0) {
        const uint32_t r = rt_num.random32(&a->seed);
        a->speed_x = r % (max_speed * 2 + 1) - max_speed;
    }
    while (a->speed_y == 0) {
        const uint32_t r = rt_num.random32(&a->seed);
        a->speed_y = r % (max_speed * 2 + 1) - max_speed;
    }
    a->x += a->speed_x;
    a->y += a->speed_y;
    if (a->x - a->w / 2 < 0) {
        a->x = a->w / 2;
        a->speed_x = -a->speed_x;
    } else if (a->x + a->w / 2 >= ui_app.root->w) {
        a->x = ui_app.root->w - a->w / 2 - 1;
        a->speed_x = -a->speed_x;
    }
    if (a->y - a->h / 2 < 0) {
        a->y = a->h / 2;
        a->speed_y = -a->speed_y;
    } else if (a->y + a->h / 2 >= ui_app.root->h) {
        a->y = ui_app.root->h - a->h / 2 - 1;
        a->speed_y = -a->speed_y;
    }
    int inc = rt_num.random32(&a->seed) % 2 == 0 ? -1 : +1;
    if (rt_num.random32(&a->seed) % 2 == 0) {
        if (1 <= a->speed_x + inc && a->speed_x + inc < max_speed) {
            a->speed_x += inc;
        }
    } else {
        if (1 <= a->speed_y + inc && a->speed_y + inc < max_speed) {
            a->speed_y += inc;
        }
    }
    ui_app.request_redraw();
    schedule_next_step(work);
}

static void animated_gif_loader(void* rt_unused(ignored)) {
    ui_cursor_t cursor = ui_app.cursor;
    ui_app.set_cursor(ui_app.cursors.wait);
    load_gif();
    ui_app.set_cursor(cursor);
    static rt_work_t work = { .work = animation_step };
    schedule_next_step(&work);
}

static void load_sample_png(void) { // from resources
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

static void delayed(rt_work_t* rt_unused(w)) {
    open_and_play(); // 1+ second long expensive call (after first paint)
}

static void opened(void) {
    animation.seed = (uint32_t)rt_clock.nanoseconds();
    animation.x = -1;
    animation.y = -1;
    thread = rt_thread.start(animated_gif_loader, null);
    static rt_work_t work = { .work = delayed };
    work.when = rt_clock.seconds() + 0.125;
    ui_app.post(&work);
}

static void closed(void) {
    rt_thread.join(thread, -1);
    ui_gdi.bitmap_dispose(&background);
    stbi_image_free(gif.pixels);
    stbi_image_free(gif.delays);
    if (ui_midi.is_open(&midi)) {
        // restore pre-muted volume:
        rt_fatal_if_error(ui_midi.set_volume(&midi,
                        volume != 0 ? volume : 0.5));
    }
    stop_and_close();
    delete_midi_file();
}

static void init(void) {
    ui_app.title = title;
    ui_app.content->paint     = paint;
    ui_app.content->character = character;
    ui_app.content->tap       = tap;
    ui_app.opened             = opened;
    ui_app.closed             = closed;
    load_sample_png();
}

static void* load_image(const uint8_t* data, int64_t bytes,
        int32_t* w, int32_t* h, int32_t* bpp, int32_t preferred_bpp) {
    void* pixels = stbi_load_from_memory(data, (int32_t)bytes, w, h,
        bpp, preferred_bpp);
    return pixels;
}

static void* load_animated_gif(const uint8_t* data, int64_t bytes,
        int32_t** delays,
        int32_t* w, int32_t* h, int32_t* frames, int32_t* bpp,
        int32_t preferred_bpp) {
    stbi_uc* pixels = stbi_load_gif_from_memory(data, (int32_t)bytes,
        delays, w, h, frames, bpp, preferred_bpp);
    return pixels;
}

ui_app_t ui_app = {
    .class_name = "sample6",
    .dark_mode = true,
    .init = init,
    .window_sizing = {
        .min_w =  4.0f,
        .min_h =  3.0f,
        .ini_w =  8.0f,
        .ini_h =  6.0f
    }
};
