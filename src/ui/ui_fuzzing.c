/* Copyright (c) Dmitry "Leo" Kuznetsov 2021-24 see LICENSE for details */
#include "ut/ut.h"
#include "ui/ui.h"

static bool     ui_fuzzing_debug; // = true;
static uint32_t ui_fuzzing_seed;
static bool     ui_fuzzing_running;
static bool     ui_fuzzing_inside;

static ui_fuzzing_t ui_fuzzing_work;

static void ui_fuzzing_dispatch(ui_fuzzing_t* work) {
    swear(work == &ui_fuzzing_work);
    ui_app.alt = work->alt;
    ui_app.ctrl = work->ctrl;
    ui_app.shift = work->shift;
    if (work->utf8 != null) {
        ui_view.character(ui_app.content, work->utf8);
        work->utf8 = null;
    } else if (work->key != 0) {
        ui_view.key_pressed(ui_app.content, work->key);
        ui_view.key_released(ui_app.content, work->key);
        work->key = 0;
    } else if (work->pt != null) {
        const int32_t x = work->pt->x;
        const int32_t y = work->pt->y;
        ui_app.mouse.x = x;
        ui_app.mouse.y = y;
//      https://stackoverflow.com/questions/22259936/
//      https://stackoverflow.com/questions/65691101/
//      ut_traceln("%d,%d", x + ui_app.wrc.x, y + ui_app.wrc.y);
//      // next line works only when running as administator:
//      ut_fatal_win32err(SetCursorPos(x + ui_app.wrc.x, y + ui_app.wrc.y));
        const bool l_button = ui_app.mouse_left  != work->left;
        const bool r_button = ui_app.mouse_right != work->right;
        ui_app.mouse_left  = work->left;
        ui_app.mouse_right = work->right;
        ui_view.mouse_move(ui_app.content);
        if (l_button) {
            ui_view.tap(ui_app.content, 0, work->left);
        }
        if (r_button) {
            ui_view.tap(ui_app.content, 2, work->right);
        }
        work->pt = null;
    } else {
        assert(false, "TODO: ?");
    }
    if (ui_fuzzing_running) {
        if (ui_fuzzing.next == null) {
            ui_fuzzing.random(work);
        } else {
            ui_fuzzing.next(work);
        }
    }
}

static void ui_fuzzing_do_work(ut_work_t* p) {
    if (ui_fuzzing_running) {
        ui_fuzzing_inside = true;
        if (ui_fuzzing.custom != null) {
            ui_fuzzing.custom((ui_fuzzing_t*)p);
        } else {
            ui_fuzzing.dispatch((ui_fuzzing_t*)p);
        }
        ui_fuzzing_inside = false;
    } else {
        // fuzzing has been .stop()-ed drop it
    }
}

static void ui_fuzzing_post(void) {
    ui_app.post(&ui_fuzzing_work.base);
}

static void ui_fuzzing_alt_ctrl_shift(void) {
    switch (ut_num.random32(&ui_fuzzing_seed) % 8) {
        case 0: ui_fuzzing_work.alt = 0; ui_fuzzing_work.ctrl = 0; ui_fuzzing_work.shift = 0; break;
        case 1: ui_fuzzing_work.alt = 1; ui_fuzzing_work.ctrl = 0; ui_fuzzing_work.shift = 0; break;
        case 2: ui_fuzzing_work.alt = 0; ui_fuzzing_work.ctrl = 1; ui_fuzzing_work.shift = 0; break;
        case 3: ui_fuzzing_work.alt = 1; ui_fuzzing_work.ctrl = 1; ui_fuzzing_work.shift = 0; break;
        case 4: ui_fuzzing_work.alt = 0; ui_fuzzing_work.ctrl = 0; ui_fuzzing_work.shift = 1; break;
        case 5: ui_fuzzing_work.alt = 1; ui_fuzzing_work.ctrl = 0; ui_fuzzing_work.shift = 1; break;
        case 6: ui_fuzzing_work.alt = 0; ui_fuzzing_work.ctrl = 1; ui_fuzzing_work.shift = 1; break;
        case 7: ui_fuzzing_work.alt = 1; ui_fuzzing_work.ctrl = 1; ui_fuzzing_work.shift = 1; break;
        default: assert(false);
    }
}

static void ui_fuzzing_character(void) {
    uint32_t rnd = ut_num.random32(&ui_fuzzing_seed);
    int ch = 0x20 + rnd % (128 - 0x20);
    if (ui_fuzzing_debug) {
        ut_traceln("character(0x%02X %d %c)", ch, ch, ch);
    }
    static char utf8[8];
    utf8[0] = (char)ch;
    utf8[1] = 0;
    ui_fuzzing_work.utf8 = utf8;
    ui_fuzzing_post();
}

static void ui_fuzzing_key(void) {
    struct {
        int32_t key;
        const char* name;
    } keys[] = {
        { ui.key.up,      "up",     },
        { ui.key.down,    "down",   },
        { ui.key.left,    "left",   },
        { ui.key.right,   "right",  },
        { ui.key.home,    "home",   },
        { ui.key.end,     "end",    },
        { ui.key.pageup,  "pgup",   },
        { ui.key.pagedw,  "pgdw",   },
        { ui.key.insert,  "insert"  },
        { ui.key.enter,   "enter"   },
// TODO: cut copy paste erase need special treatment in fuzzing
//       otherwise text collapses to nothing pretty fast
//      ui.key.del,
//      ui.key.back,
    };
    ui_fuzzing_alt_ctrl_shift();
    uint32_t ix = ut_num.random32(&ui_fuzzing_seed) % ut_count_of(keys);
    if (ui_fuzzing_debug) {
        ut_traceln("key(%s)", keys[ix].name);
    }
    ui_fuzzing_work.key = keys[ix].key;
    ui_fuzzing_post();
}

static void ui_fuzzing_mouse(void) {
    // mouse events only inside edit control otherwise
    // they will start clicking buttons around
    ui_view_t* v = ui_app.content;
    int32_t x = ut_num.random32(&ui_fuzzing_seed) % v->w;
    int32_t y = ut_num.random32(&ui_fuzzing_seed) % v->h;
    static ui_point_t pt;
    pt = (ui_point_t){ x + v->x, y + v->y };
    if (ut_num.random32(&ui_fuzzing_seed) % 2) {
        ui_fuzzing_work.left  = !ui_fuzzing_work.left;
    }
    if (ut_num.random32(&ui_fuzzing_seed) % 2) {
        ui_fuzzing_work.right = !ui_fuzzing_work.right;
    }
    if (ui_fuzzing_debug) {
        ut_traceln("mouse(%d,%d) %s%s", pt.x, pt.y,
                ui_fuzzing_work.left ? "L" : "_", ui_fuzzing_work.right ? "R" : "_");
    }
    ui_fuzzing_work.pt = &pt;
    ui_fuzzing_post();
}

static void ui_fuzzing_next_random(void) {
    // TODO: 100 times per second:
    ui_fuzzing_work = (ui_fuzzing_t){
        .base = { .when = ut_clock.seconds() + 0.001, // 1ms
                  .ui_fuzzing_work = ui_fuzzing_do_work },
    };
    uint32_t rnd = ut_num.random32(&ui_fuzzing_seed) % 100;
    if (rnd < 95) { // 95%
        ui_fuzzing_character();
    } else if (rnd < 97) { // 2%
        ui_fuzzing_key();
    } else { // 3%
        ui_fuzzing_mouse();
    }
}

static void ui_fuzzing_start(uint32_t seed) {
    ui_fuzzing_seed = seed | 0x1;
    ui_fuzzing_running = true;
    if (ui_fuzzing.next == null) {
        ui_fuzzing.random(&ui_fuzzing_work);
    } else {
        ui_fuzzing.next(&ui_fuzzing_work);
    }
}

static bool ui_fuzzing_is_running(void) {
    return ui_fuzzing_running;
}

static bool ui_fuzzing_from_inside(void) {
    return ui_fuzzing_inside;
}

static void ui_fuzzing_stop(void) {
    ui_fuzzing_running = false;
}

static void ui_fuzzing_random(ui_fuzzing_t* f) {
    swear(f == &ui_fuzzing_work);
    ui_fuzzing_next_random();
}

ui_fuzzing_if ui_fuzzing = {
    .start       = ui_fuzzing_start,
    .is_running  = ui_fuzzing_is_running,
    .from_inside = ui_fuzzing_from_inside,
    .random      = ui_fuzzing_random,
    .dispatch    = ui_fuzzing_dispatch,
    .next        = null,
    .custom      = null,
    .stop        = ui_fuzzing_stop
};
