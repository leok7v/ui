#pragma once
/* Copyright (c) Dmitry "Leo" Kuznetsov 2021-24 see LICENSE for details */
#include "rt/rt.h"
#include "ui/ui.h"

rt_begin_c

// https://en.wikipedia.org/wiki/Fuzzing
// aka "Monkey" testing

typedef struct ui_fuzzing_s {
    rt_work_t    base;
    const char*  utf8; // .character(utf8)
    int32_t      key;  // .key_pressed(key)/.key_released(key)
    ui_point_t*  pt;   // .move_move()
    // key_press and character
    bool         alt;
    bool         ctrl;
    bool         shift;
    // mouse modifiers
    bool         left; // tap() buttons:
    bool         right;
    bool         double_tap;
    bool         long_press;
    // custom
    int32_t      op;
    void*        data;
} ui_fuzzing_t;

typedef struct ui_fuzzing_if {
    void (*start)(uint32_t seed);
    bool (*is_running)(void);
    bool (*from_inside)(void); // true if called originated inside fuzzing
    void (*next_random)(ui_fuzzing_t* f); // called if `next` is null
    void (*dispatch)(ui_fuzzing_t* f);    // dispatch work
    // next() called instead of random if not null
    void (*next)(ui_fuzzing_t* f);
    // custom() called instead of dispatch() if not null
    void (*custom)(ui_fuzzing_t* f);
    void (*stop)(void);
} ui_fuzzing_if;

extern ui_fuzzing_if ui_fuzzing;

rt_end_c

