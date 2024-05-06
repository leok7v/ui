/* Copyright (c) Dmitry "Leo" Kuznetsov 2021-24 see LICENSE for details */
#include "single_file_lib/ut/ut.h"
#include "single_file_lib/ui/ui.h"
#include <math.h>

const char* title = "Sample7 : timers";

enum { N = 1800 };

static ui_timer_t timer10ms;
static thread_t thread;
static bool quit;

typedef struct {
    fp64_t time[N]; // circular buffer of timestamps
    int pos; // writing position in the buffer for next timer event
    int samples; // number of samples collected
    fp64_t dt[N]; // delta(t) between 2 timer events
    fp64_t min_dt;
    fp64_t max_dt;
    fp64_t avg;
    fp64_t spread;
} time_stats_t;

static volatile time_stats_t ts[2];

static ui_point_t points[N]; // graph polyline coordinates

static void stats(volatile time_stats_t* t) {
    int n = ut_min(N - 1, t->samples);
    t->min_dt = 1.0; // 1 second is 100x of 10ms
    t->max_dt = 0;
    int j = 0;
    fp64_t sum = 0;
    for (int i = 0; i < n - 1; i++) {
        int p0 = (t->pos - i - 1 + N) % N;
        int p1 = (p0 - 1 + N) % N;
        t->dt[j] = t->time[p0] - (t->time[p1] + 0.01); // expected 10ms
        t->min_dt = ut_min(t->dt[j], t->min_dt);
        t->max_dt = ut_max(t->dt[j], t->max_dt);
        sum += t->time[p0] - t->time[p1];
        j++;
    }
    t->avg = sum / (n - 1);
    j = 0;
    t->spread = ut_max(fabs(t->max_dt), fabs(t->min_dt));
}

static void graph(ui_view_t* view, volatile time_stats_t* t, ui_color_t c, int y) {
    const int h2 = ui_app.crc.h / 2;
    const int h4 = h2 / 2;
    const int h8 = h4 / 2;
    ui_gdi.y = y - h8;
    ui_gdi.set_colored_pen(ui_colors.white);
    ui_gdi.move_to(0, y);
    ui_gdi.line(ui_app.crc.w, y);
    ui_gdi.set_colored_pen(c);
    if (ts[0].samples > 2 && ts[1].samples > 2) {
        const fp64_t spread = ut_max(ts[0].spread, ts[0].spread);
        int n = ut_min(N - 1, t->samples);
        int j = 0;
        for (int i = 0; i < n - 1; i++) {
            fp64_t v = t->dt[j] / spread;
            points[j].x = n - 1 - i;
            points[j].y = y + (int32_t)(v * h8);
            j++;
        }
        ui_gdi.poly(points, n - 1);
        ui_gdi.x = view->em.x;
        ui_gdi.y = y - h8 - view->em.y;
        ui_gdi.println("min %.3f max %.3f avg %.3f ms  "
            "%.1f sps",
            t->min_dt * 1000, t->max_dt * 1000, t->avg, 1 / t->avg);
    }
}

static void timer_thread(void* p) {
    bool* done = (bool*)p;
    ut_thread.name("r/t timer");
    ut_thread.realtime();
    while (!*done) {
        ut_thread.sleep_for(0.0094);
        ts[1].time[ts[1].pos] = ut_clock.seconds();
        ts[1].pos = (ts[1].pos + 1) % N;
        (ts[1].samples)++;
        ui_app.redraw();
    }
}

static void paint(ui_view_t* view) {
    stats(&ts[0]);
    stats(&ts[1]);
    ui_gdi.fill_with(0, 0, view->w, view->h, ui_colors.dkgray1);
    ui_gdi.y = view->em.y;
    ui_gdi.x = view->w - ui_gdi.measure_text(ui_app.fonts.mono,
        "avg paint time %.1f ms", ui_app.paint_avg * 1000).x - view->em.x;
    ui_gdi.print("avg paint time %.1f ms", ui_app.paint_avg * 1000);
    ui_gdi.x = view->em.x;
    ui_gdi.print("10ms window timer jitter ");
    ui_gdi.print("(\"sps\" stands for samples per second)");
    const int h2 = ui_app.crc.h / 2;
    const int h4 = h2 / 2;
    graph(view, &ts[0], ui_colors.tone_red, h4);
    ui_gdi.y = h2;
    ui_gdi.print("10ms r/t thread sleep jitter");
    graph(view, &ts[1], ui_colors.tone_green, h2 + h4);
    ui_gdi.y = h2 - h4;

}

static void timer(ui_view_t* unused(view), ui_timer_t id) {
    assert(view == ui_app.view);
    // there are at least 3 timers notifications coming here:
    // 1 seconds, 100ms and 10ms:
    if (id == timer10ms) {
        ts[0].time[ts[0].pos] = ui_app.now;
        ts[0].pos = (ts[0].pos + 1) % N;
        (ts[0].samples)++;
    }
}

static void opened(void) {
    timer10ms = ui_app.set_timer((uintptr_t)&timer10ms, 10);
    fatal_if(timer10ms == 0);
    thread = ut_thread.start(timer_thread, &quit);
    fatal_if_null(thread);
}

static void detached_sleep(void* unused(p)) {
    ut_thread.sleep_for(100.0); // seconds
}

static void detached_loop(void* unused(p)) {
    uint64_t sum = 0;
    for (uint64_t i = 0; i < UINT64_MAX; i++) {
        sum += i;
    }
    // making sure that compiler won't get rid of the loop:
    traceln("%lld", sum);
}

static void closed(void) {
    ui_app.kill_timer(timer10ms);
    quit = true;
    fatal_if_not_zero(ut_thread.join(thread, -1));
    thread = null;
    quit = false;
    // just to test that ExitProcess(0) works when there is
    // are detached threads
    thread_t detached = ut_thread.start(detached_sleep, null);
    ut_thread.detach(detached);
    detached = ut_thread.start(detached_loop, null);
    ut_thread.detach(detached);
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
    ut_thread.realtime(); // both main thread and timer thread
    ui_app.view->timer = timer;
    ui_app.closed = closed;
    ui_app.opened = opened;
    ui_app.view->paint = paint;
    // no minimize/maximize title bar and system menu
    ui_app.no_min = true;
    ui_app.no_max = true;
    do_not_start_minimized();
}

ui_app_t ui_app = {
    .class_name = "sample2",
    .init = init,
    .window_sizing = {
        .min_w =  9.0f, // 9x5 inches
        .min_h =  5.0f,
        .ini_w = 10.0f, // 10x6 inches - fits 11" laptops
        .ini_h = 6.0f
    }
};
