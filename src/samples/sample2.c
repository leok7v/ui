/* Copyright (c) Dmitry "Leo" Kuznetsov 2021-24 see LICENSE for details */
#include "single_file_lib/ut/ut.h"
#include "single_file_lib/ui/ui.h"
#include <math.h>

begin_c

const char* title = "Sample2";

enum { N = 1800 };

static ui_timer_t timer10ms;
static thread_t thread;
static bool quit;

typedef struct {
    double time[N]; // circular buffer of timestamps
    int pos; // writing position in the buffer for next timer event
    int samples; // number of samples collected
    double dt[N]; // delta(t) between 2 timer events
    double min_dt;
    double max_dt;
    double avg;
    double spread;
} time_stats_t;

static volatile time_stats_t ts[2];

static ui_point_t points[N]; // graph polyline coordinates

static void stats(volatile time_stats_t* t) {
    int n = min(N - 1, t->samples);
    t->min_dt = 1.0; // 1 second is 100x of 10ms
    t->max_dt = 0;
    int j = 0;
    double sum = 0;
    for (int i = 0; i < n - 1; i++) {
        int p0 = (t->pos - i - 1 + N) % N;
        int p1 = (p0 - 1 + N) % N;
        t->dt[j] = t->time[p0] - (t->time[p1] + 0.01); // expected 10ms
        t->min_dt = min(t->dt[j], t->min_dt);
        t->max_dt = max(t->dt[j], t->max_dt);
        sum += t->time[p0] - t->time[p1];
        j++;
    }
    t->avg = sum / (n - 1);
    j = 0;
    t->spread = max(fabs(t->max_dt), fabs(t->min_dt));
}

static void graph(ui_view_t* view, volatile time_stats_t* t, ui_color_t c, int y) {
    const int h2 = app.crc.h / 2;
    const int h4 = h2 / 2;
    const int h8 = h4 / 2;
    gdi.y = y - h8;
    gdi.set_colored_pen(colors.white);
    gdi.move_to(0, y);
    gdi.line(app.crc.w, y);
    gdi.set_colored_pen(c);
    if (ts[0].samples > 2 && ts[1].samples > 2) {
        const double spread = max(ts[0].spread, ts[0].spread);
        int n = min(N - 1, t->samples);
        int j = 0;
        for (int i = 0; i < n - 1; i++) {
            double v = t->dt[j] / spread;
            points[j].x = n - 1 - i;
            points[j].y = y + (int32_t)(v * h8);
            j++;
        }
        gdi.poly(points, n - 1);
        gdi.x = view->em.x;
        gdi.y = y - h8 - view->em.y;
        gdi.println("min %.3f max %.3f avg %.3f ms  "
            "%.1f sps",
            t->min_dt * 1000, t->max_dt * 1000, t->avg, 1 / t->avg);
    }
}

static void timer_thread(void* p) {
    bool* done = (bool*)p;
    threads.name("r/t timer");
    threads.realtime();
    while (!*done) {
        threads.sleep_for(0.0094);
        ts[1].time[ts[1].pos] = clock.seconds();
        ts[1].pos = (ts[1].pos + 1) % N;
        (ts[1].samples)++;
        app.redraw();
    }
}

static void paint(ui_view_t* view) {
    stats(&ts[0]);
    stats(&ts[1]);
    gdi.fill_with(0, 0, view->w, view->h, colors.dkgray1);
    gdi.y = view->em.y;
    gdi.x = view->w - gdi.measure_text(app.fonts.mono,
        "avg paint time %.1f ms", app.paint_avg * 1000).x - view->em.x;
    gdi.print("avg paint time %.1f ms", app.paint_avg * 1000);
    gdi.x = view->em.x;
    gdi.print("10ms window timer jitter ");
    gdi.print("(\"sps\" stands for samples per second)");
    const int h2 = app.crc.h / 2;
    const int h4 = h2 / 2;
    graph(view, &ts[0], colors.tone_red, h4);
    gdi.y = h2;
    gdi.print("10ms r/t thread sleep jitter");
    graph(view, &ts[1], colors.tone_green, h2 + h4);
    gdi.y = h2 - h4;

}

static void timer(ui_view_t* unused(view), ui_timer_t id) {
    assert(view == app.view);
    // there are at least 3 timers notifications coming here:
    // 1 seconds, 100ms and 10ms:
    if (id == timer10ms) {
        ts[0].time[ts[0].pos] = app.now;
        ts[0].pos = (ts[0].pos + 1) % N;
        (ts[0].samples)++;
    }
}

static void opened(void) {
    timer10ms = app.set_timer((uintptr_t)&timer10ms, 10);
    fatal_if(timer10ms == 0);
    thread = threads.start(timer_thread, &quit);
    fatal_if_null(thread);
}

static void detached_sleep(void* unused(p)) {
    threads.sleep_for(100.0); // seconds
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
    app.kill_timer(timer10ms);
    quit = true;
    fatal_if_not_zero(threads.join(thread, -1));
    thread = null;
    quit = false;
    // just to test that ExitProcess(0) works when there is
    // are detached threads
    thread_t detached = threads.start(detached_sleep, null);
    threads.detach(detached);
    detached = threads.start(detached_loop, null);
    threads.detach(detached);
}

static void do_not_start_minimized(void) {
    // This sample does not start minimized but some applications may.
    if (app.last_visibility != ui.visibility.minimize) {
        app.visibility = app.last_visibility;
    } else {
        app.visibility = ui.visibility.defau1t;
        app.last_visibility = ui.visibility.defau1t;
    }
}

static void init(void) {
    app.title = title;
    threads.realtime(); // both main thread and timer thread
    app.view->timer = timer;
    app.closed = closed;
    app.opened = opened;
    app.view->paint = paint;
    // no minimize/maximize title bar and system menu
    app.no_min = true;
    app.no_max = true;
    do_not_start_minimized();
}

app_t app = {
    .class_name = "sample2",
    .init = init,
    // fixed size app
    .wmin = 10.0f, // 10x6 inches
    .hmin =  6.0f,
    .wmax = 10.0f,
    .hmax =  6.0f,
};

end_c
