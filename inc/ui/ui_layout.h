#pragma once
#include "ut/ut_std.h"

ut_begin_c

 // TODO: ui_ namespace

typedef struct {
    void (*center)(ui_view_t* view); // exactly one child
    void (*horizontal)(ui_view_t* view, int32_t gap);
    void (*vertical)(ui_view_t* view, int32_t gap);
    void (*grid)(ui_view_t* view, int32_t gap_h, int32_t gap_v);
} measurements_if;

extern measurements_if measurements;

typedef struct {
    void (*center)(ui_view_t* view); // exactly one child
    void (*horizontal)(ui_view_t* view, int32_t x, int32_t y, int32_t gap);
    void (*vertical)(ui_view_t* view, int32_t x, int32_t y, int32_t gap);
    void (*grid)(ui_view_t* view, int32_t gap_h, int32_t gap_v);
} layouts_if;

extern layouts_if layouts;

ut_end_c
