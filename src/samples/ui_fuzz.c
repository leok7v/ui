/* Copyright (c) Dmitry "Leo" Kuznetsov 2021-24 see LICENSE for details */
#include "single_file_lib/ut/ut.h"
#include "single_file_lib/ui/ui.h"
#include "ui_fuzz.h"

ui_fuzz_t     ui_fuzz;
ui_fuzzing_if ui_fuzzing;

typedef struct ut_work_ex_s {
    ut_work_t    base;
    const char*  utf8; // .character(utf8)
    int32_t      key;  // .key_pressed(key)
    ui_point_t*  pt;
    bool         alt;
    bool         ctrl;
    bool         shift;
    bool         left;
    bool         right;
} ut_work_ex_t;

static ut_event_t   done;
static ut_work_ex_t work;

static void do_work(ut_work_t* unused(p)) {
    assert((ut_work_ex_t*)p == &work);
    ui_app.alt = work.alt;
    ui_app.ctrl = work.ctrl;
    ui_app.shift = work.shift;
    if (work.utf8 != null) {
        ui_view.character(ui_app.content, work.utf8);
        work.utf8 = null;
    } else if (work.key != 0) {
        ui_view.key_pressed(ui_app.content, work.key);
        ui_view.key_released(ui_app.content, work.key);
        work.key = 0;
    } else if (work.pt != null) {
        ui_app.mouse.x = work.pt->x;
        ui_app.mouse.y = work.pt->y;
        const bool l_button = ui_app.mouse_left  != work.left;
        const bool r_button = ui_app.mouse_right != work.right;
        ui_app.mouse_left  = work.left;
        ui_app.mouse_right = work.right;
        ui_view.mouse_move(ui_app.content);
        if (l_button) {
            ui_view.tap(ui_app.content, 0, work.left);
        }
        if (r_button) {
            ui_view.tap(ui_app.content, 2, work.right);
        }
        work.pt = null;
    } else {
        assert(false, "TODO: ?");
    }
}

static void post_work(void) {
    ui_app.post(&work.base);
    ut_event.wait(done);
}

static void ui_fuzzing_alt_ctrl_shift(void) {
    switch (ut_num.random32(&ui_fuzz.seed) % 8) {
        case 0: work.alt = 0; work.ctrl = 0; work.shift = 0; break;
        case 1: work.alt = 1; work.ctrl = 0; work.shift = 0; break;
        case 2: work.alt = 0; work.ctrl = 1; work.shift = 0; break;
        case 3: work.alt = 1; work.ctrl = 1; work.shift = 0; break;
        case 4: work.alt = 0; work.ctrl = 0; work.shift = 1; break;
        case 5: work.alt = 1; work.ctrl = 0; work.shift = 1; break;
        case 6: work.alt = 0; work.ctrl = 1; work.shift = 1; break;
        case 7: work.alt = 1; work.ctrl = 1; work.shift = 1; break;
        default: assert(false);
    }
}

static void ui_fuzzing_character(void) {
    uint32_t rnd = ut_num.random32(&ui_fuzz.seed);
    int ch = 0x20 + rnd % (128 - 0x20);
//  traceln("character(0x%02X %d %c)", ch, ch, ch);
    char utf8[8] = { (char)ch, 0x00 };
    work.utf8 = utf8;
    post_work();
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
    uint32_t ix = ut_num.random32(&ui_fuzz.seed) % ut_count_of(keys);
//  traceln("key(%s)", keys[ix].name);
    work.key = keys[ix].key;
    post_work();
}

static void ui_fuzzing_mouse(void) {
    // mouse events only inside edit control otherwise
    // they will start clicking buttons around
    ui_view_t* v = ui_app.content;
    int32_t x = ut_num.random32(&ui_fuzz.seed) % v->w;
    int32_t y = ut_num.random32(&ui_fuzz.seed) % v->h;
    ui_point_t pt = { x + v->x, y + v->y };
    if (ut_num.random32(&ui_fuzz.seed) % 2) {
        work.left  = !work.left;
    }
    if (ut_num.random32(&ui_fuzz.seed) % 2) {
        work.right = !work.right;
    }
//  traceln("mouse(%d,%d) %s%s", pt.x, pt.y,
//          work.left ? "L" : "_", work.right ? "R" : "_");
    work.pt = &pt;
    post_work();
}

static void ui_fuzzing_thread(void* unused(p)) {
    done = ut_event.create();
    work = (ut_work_ex_t){
        .base = { .work = do_work, .done = done },
    };
    while (!ui_fuzz.quit) {
        ut_thread.sleep_for(0.001);
        uint32_t rnd = ut_num.random32(&ui_fuzz.seed) % 100;
        if (rnd < 95) { // 95%
            ui_fuzzing_character();
        } else if (rnd < 97) { // 2%
            ui_fuzzing_key();
        } else if (rnd < 99) { // 3%
            ui_fuzzing_mouse();
        }
    }
    ut_event.dispose(done); done = null;
}

static void ui_fuzzing_start(void) {
    assert(ui_fuzz.thread == null);
    ui_fuzz.seed = 0x1;
    ui_fuzz.thread = ut_thread.start(ui_fuzzing_thread, null);
}

static void ui_fuzzing_stop(void) {
    assert(ui_fuzz.thread != null);
    ui_fuzz.quit = true;
    ut_thread.join(ui_fuzz.thread, -1);
    ui_fuzz.thread = null;
}

extern ui_fuzzing_if ui_fuzzing = {
    .start = ui_fuzzing_start,
    .stop  = ui_fuzzing_stop
};
