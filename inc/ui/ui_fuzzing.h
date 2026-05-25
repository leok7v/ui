#pragma once
/* Copyright (c) Dmitry "Leo" Kuznetsov 2021-24 see LICENSE for details */
#include "posix.h"
#include "ui/ui.h"

posix_begin_c

// https://en.wikipedia.org/wiki/Fuzzing
// aka "Monkey" testing

struct ui_fuzzing {
    struct posix_work    base;
    const char*  utf8; // .character(utf8)
    int32_t      key;  // .key_pressed(key)/.key_released(key)
    struct ui_point*  pt;   // .move_move()
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
};

struct ui_fuzzing_if {
    void (*start)(uint32_t seed);
    bool (*is_running)(void);
    bool (*from_inside)(void); // true if called originated inside fuzzing
    void (*next_random)(struct ui_fuzzing* f); // called if `next` is null
    void (*dispatch)(struct ui_fuzzing* f);    // dispatch work
    // next() called instead of random if not null
    void (*next)(struct ui_fuzzing* f);
    // custom() called instead of dispatch() if not null
    void (*custom)(struct ui_fuzzing* f);
    void (*stop)(void);
};

extern struct ui_fuzzing_if ui_fuzzing;

posix_end_c

