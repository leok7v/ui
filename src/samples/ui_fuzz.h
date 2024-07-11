#pragma once
/* Copyright (c) Dmitry "Leo" Kuznetsov 2021-24 see LICENSE for details */
#include "ut/ut.h"
#include "ui/ui.h"

begin_c

// https://en.wikipedia.org/wiki/Fuzzing
// aka "Monkey" testing

typedef struct ui_fuzz_s {
    ut_thread_t   thread;
    volatile bool quit;
    uint32_t      seed;
} ui_fuzz_t;

extern ui_fuzz_t ui_fuzz;

typedef struct ui_fuzzing_if {
    void (*start)(void);
    void (*stop)(void);
} ui_fuzzing_if;

extern ui_fuzzing_if ui_fuzzing;

end_c

