#pragma once
#include "ut/ut_std.h"
#include "ui/ui_view.h"

begin_c

// Options like:
//   "Yes"|"No"|"Abort"|"Retry"|"Ignore"|"Cancel"|"Try"|"Continue"
// maximum number of choices presentable to human is 4.

typedef struct {
    ui_view_t   view;
    ui_label_t  label;
    ui_button_t button[4];
    int32_t option; // -1 or option chosen by user
    const char** options;
} ui_mbx_t;

void ui_view_init_mbx(ui_view_t* view);

void ui_mbx_init(ui_mbx_t* mx, const char* option[], const char* format, ...);

#define static_ui_mbx(name, s, code, ...)                        \
                                                                 \
    static char* name ## _options[] = { __VA_ARGS__, null };     \
                                                                 \
    static void name ## _callback(ui_mbx_t* m, int32_t option) { \
        (void)m; (void)option; /* no warnings if unused */       \
        code                                                     \
    }                                                            \
    static                                                       \
    ui_mbx_t name = {                                            \
        .view = { .type = ui_view_mbx, .init = ui_view_init_mbx, \
                  .font = &app.fonts.regular,                    \
                  .text = s, .callback = name ## _callback       \
                },                                               \
        .options = name ## _options                              \
    }

#define ui_mbx(s, call_back, ...) {                          \
    .view = { .type = ui_view_mbx, .init = ui_view_init_mbx, \
              .font = &app.fonts.regular,                    \
              .text = s, .callback = call_back               \
    },                                                       \
    .options = (const char*[]){ __VA_ARGS__, null },         \
}

end_c
