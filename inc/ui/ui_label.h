#pragma once
#include "ut/ut_std.h"
#include "ui/ui_view.h"

begin_c

typedef struct ui_label_s {
    ui_view_t view;
    bool multiline;
    bool editable;  // can be edited
    bool highlight; // paint with highlight color
    bool hovered;   // paint highlight rectangle when hover over
    bool label;     // do not copy text to clipboard, do not highlight
    int32_t dy; // vertical shift down (to line up baselines of diff fonts)
} ui_label_t;

// do not call ui_label_init_ use ui_label() and ui_label_ml() instead
void ui_label_init_(ui_view_t* view);

#define ui_label(t, s)                                                        \
    ui_label_t t = { .view = { .type = ui_view_label, .init = ui_label_init_, \
    .child = null, .width = 0.0, .text = s}, .multiline = false}

#define ui_label_ml(t, w, s)  /* multiline */                                 \
    ui_label_t t = { .view = { .type = ui_view_label, .init = ui_label_init_, \
    .child = null, .width = w, .text = s}, .multiline = true}

// single line of text with "&" keyboard shortcuts:

void ui_label_init(ui_label_t* t, const char* format, ...);
void ui_label_init_va(ui_label_t* t, const char* format, va_list vl);

// multiline
void ui_label_init_ml(ui_label_t* t, fp64_t width, const char* format, ...);

end_c
