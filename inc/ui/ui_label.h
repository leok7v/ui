#pragma once
#include "ut/ut_std.h"
#include "ui/ui_view.h"

begin_c

typedef struct ui_label_s {
    ui_view_t view;
    bool editable;  // can be edited
    bool highlight; // paint with highlight color
    bool hovered;   // paint highlight rectangle when hover over
    bool label;     // do not copy text to clipboard, do not highlight
    int32_t dy;     // vertical shift down (to line up baselines of diff fonts)
} ui_label_t;

// do not call ui_label_init_ use ui_label() instead
void ui_view_init_label(ui_view_t* view);

#define ui_label(min_width_em, s) {                                    \
      .view = { .type = ui_view_label, .init = ui_view_init_label,     \
                .font = &ui_app.fonts.regular, .min_w_em = min_width_em,  \
                .text = s }                                            \
}

// single line of text with "&" keyboard shortcuts:

void ui_label_init(ui_label_t* t, fp32_t min_w_em, const char* format, ...);
void ui_label_init_va(ui_label_t* t, fp32_t min_w_em, const char* format, va_list vl);

end_c
