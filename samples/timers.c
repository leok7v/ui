/* Copyright (c) Dmitry "Leo" Kuznetsov 2021-24 see LICENSE for details */
#include "posix/posix.h"
#include "ui/ui.h"
#include <math.h>

const char* title = "Timers";

enum { max_count = 1800 };

static ui_timer_t timer10ms;
static posix_thread_t thread;
static bool quit;

typedef struct {
    fp64_t time[max_count]; // circular buffer of timestamps
    int pos; // writing position in the buffer for next timer event
    int samples; // number of samples collected
    fp64_t dt[max_count]; // delta(t) between 2 timer events
    fp64_t min_dt;
    fp64_t max_dt;
    fp64_t avg;
    fp64_t spread;
} time_stats_t;

static volatile time_stats_t ts[2];

static struct ui_point points[max_count]; // graph polyline coordinates

static int32_t N = max_count;

static void composed(struct ui_view* view) {
    if (view->w > 0) { N = view->w < N ? view->w : N; }
    posix_println("M: %d", N);
}

static void stats(int32_t ix) {
    volatile time_stats_t* t = &ts[ix];
    posix_assert(t->samples >= 2, "no samples");
    int n = N < t->samples ? N : t->samples;
    t->min_dt = 1.0; // 1 second is 100x of 10ms
    t->max_dt = 0;
    int j = 0;
    fp64_t sum = 0;
    for (int i = 0; i < n - 1; i++) {
        int p0 = (t->pos - i - 1 + n) % n;
        int p1 = (p0 - 1 + n) % n;
        t->dt[j] = t->time[p0] - (t->time[p1] + 0.01); // expected 10ms
        t->min_dt = t->dt[j] < t->min_dt ? t->dt[j] : t->min_dt;
        t->max_dt = t->dt[j] > t->max_dt ? t->dt[j] : t->max_dt;
        sum += t->time[p0] - t->time[p1];
        j++;
    }
    t->avg = sum / (n - 1);
    j = 0;
    fp64_t d0 = fabs(t->min_dt);
    fp64_t d1 = fabs(t->max_dt);
    fp64_t spread = (d0 > d1 ? d0 : d1) * 2;
    t->spread = t->spread > spread ? t->spread : spread;
//  if (t->samples % 1000 == 0) {
//      posix_println("[%d] samples: %6d spread: %.6f min %.6f max %.6f",
//              ix, t->samples, t->spread, t->min_dt, t->max_dt);
//  }
}

static void print(int32_t *x, int32_t *y, const char* format, ...) {
    va_list va;
    va_start(va, format);
    *x += ui_draw.text_va(&ui_draw.ta.mono.normal, *x, *y, format, va).w;
    va_end(va);
}

static void println(int32_t *x, int32_t *y, const char* format, ...) {
    va_list va;
    va_start(va, format);
    *y += ui_draw.text_va(&ui_draw.ta.mono.normal, *x, *y, format, va).h;
    va_end(va);
}

static void graph(struct ui_view* v, int ix, ui_color_t c, int y) {
    volatile time_stats_t* t = &ts[ix];
    const int h2 = ui_app.root->h / 2;
    const int h4 = h2 / 2;
    const int h8 = h4 / 2;
    ui_draw.line(0, y, ui_app.root->w, y, ui_colors.white);
    if (t->samples > 2) {
        const fp64_t spread = ts[ix].spread;
        int n = N < t->samples ? N : t->samples;
        int j = 0;
        for (int i = 0; i < n; i++) {
            points[j].x = n - 1 - i;
            points[j].y = y - (int32_t)(t->dt[j] * h8 / spread);
            j++;
        }
        ui_draw.poly(points, n - 1, c);
        int32_t tx = v->fm->em.w;
        int32_t ty = y - h8 - v->fm->em.h;
        println(&tx, &ty, "min %.3f max %.3f avg %.3f ms  "
            "%.1f sps",
            t->min_dt * 1000, t->max_dt * 1000, t->avg, 1 / t->avg);
    }
}

static void paint(struct ui_view* v) {
    for (int i = 0; i < posix_countof(ts); i++) {
        if (ts[i].samples >= 2) { stats(i); }
    }
    if (ts[0].spread > 0 && ts[1].spread > 0) {
        char paint_stats[256];
        posix_str_printf(paint_stats, "avg paint time %.1f ms %.1f fps",
            ui_app.paint_avg * 1000, ui_app.paint_fps);
        struct ui_ta ta = ui_draw.ta.mono.normal;
        ta.measure = true;
        struct ui_wh wh = ui_view.text_metrics(0, 0, false, 0,
                        &ui_app.fm.mono.normal, "%s", paint_stats);
        int32_t x = v->w - wh.w - v->fm->em.w;
        int32_t y = v->fm->em.h;
        print(&x, &y, "%s", paint_stats);
        x = v->fm->em.w;
        print(&x, &y, "10ms window timer jitter ");
        print(&x, &y, "(\"sps\" is samples per second)");
        const int h2 = ui_app.root->h / 2;
        const int h4 = h2 / 2;
        graph(v, 0, ui_colors.tone_red, h4);
        y = h2;
        print(&x, &y, "10ms r/t thread sleep jitter");
        graph(v, 1, ui_colors.tone_green, h2 + h4);
        y = h2 - h4;
    }
}

static void timer_thread(void* p) {
    bool* done = (bool*)p;
    posix_thread.name("r/t timer");
    posix_thread.realtime();
    while (!*done) {
        posix_thread.sleep_for(0.0094);
        ts[1].time[ts[1].pos] = posix_clock.seconds();
        ts[1].pos = (ts[1].pos + 1) % N;
        (ts[1].samples)++;
        ui_app.request_redraw();
    }
}

static void timer(struct ui_view* view, ui_timer_t id) {
    posix_swear(view == ui_app.content);
    // there are at least 3 timers notifications coming here:
    // 1 seconds, 100ms and 10ms:
    if (id == timer10ms) {
        ts[0].time[ts[0].pos] = ui_app.now;
        ts[0].pos = (ts[0].pos + 1) % N;
        (ts[0].samples)++;
        ui_app.request_redraw();
    }
}

static void opened(void) {
    timer10ms = ui_app.set_timer((uintptr_t)&timer10ms, 10);
    posix_fatal_if(timer10ms == 0);
    thread = posix_thread.start(timer_thread, &quit);
    posix_not_null(thread);
}

static void detached_sleep(void* posix_unused(p)) {
    posix_thread.sleep_for(100.0); // seconds
}

static void detached_loop(void* posix_unused(p)) {
    uint64_t sum = 0;
    for (uint64_t i = 0; i < UINT64_MAX; i++) {
        sum += i;
    }
    // making sure that compiler won't get rid of the loop:
    posix_println("%lld", sum);
}

static void closed(void) {
    ui_app.kill_timer(timer10ms);
    quit = true;
    posix_fatal_if_error(posix_thread.join(thread, -1));
    thread = null;
    quit = false;
    // just to test that ExitProcess(0) works when there is
    // are detached threads
    posix_thread_t detached = posix_thread.start(detached_sleep, null);
    posix_thread.detach(detached);
    detached = posix_thread.start(detached_loop, null);
    posix_thread.detach(detached);
}

static void do_not_start_minimized(void) {
    // This sample does not start minimized but some applications may.
    if (ui_app.last_visibility != ui.visibility.minimize) {
        ui_app.visibility = ui_app.last_visibility;
    } else {
        ui_app.visibility = ui.visibility.defau1t;
        ui_app.last_visibility = ui.visibility.defau1t;
    }
}

static void init(void) {
    ui_app.title = title;
    posix_thread.realtime(); // both main thread and timer thread
    ui_app.closed = closed;
    ui_app.opened = opened;
    ui_app.content->timer = timer;
    ui_app.content->paint = paint;
    ui_app.content->composed = composed;
    // no minimize/maximize title bar and system menu
    ui_app.no_min = true;
    ui_app.no_max = true;
    do_not_start_minimized();
}

struct ui_app ui_app = {
    .class_name = "timers",
    .init = init,
    .dark_mode = true,
    .window_sizing = {
        .min_w =  9.0f, // 9x5 inches
        .min_h =  5.0f,
        .ini_w = 10.0f, // 10x6 inches - fits 11" laptops
        .ini_h = 6.0f
    }
};
