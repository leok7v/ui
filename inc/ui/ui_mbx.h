#pragma once
#include "rt/rt_std.h"
#include "ui/ui_view.h"

rt_begin_c

// Options like:
//   "Yes"|"No"|"Abort"|"Retry"|"Ignore"|"Cancel"|"Try"|"Continue"
// maximum number of choices presentable to human is 4.

typedef struct {
    union {
        ui_view_t view;
        struct ui_view_s;
    };
    ui_label_t   label;
    ui_button_t  button[4];
    int32_t      option; // -1 or option chosen by user
    const char** options;
} ui_mbx_t;

void ui_view_init_mbx(ui_view_t* v);

void ui_mbx_init(ui_mbx_t* mx, const char* option[], const char* format, ...);

// ui_mbx_on_choice can only be used on static mbx variables


#define ui_mbx_chosen(name, s, code, ...)                        \
                                                                 \
    static char* name ## _options[] = { __VA_ARGS__, null };     \
                                                                 \
    static void name ## _chosen(ui_mbx_t* m, int32_t option) {   \
        (void)m; (void)option; /* no warnings if unused */       \
        code                                                     \
    }                                                            \
    static                                                       \
    ui_mbx_t name = {                                            \
        .view = {                                                \
            .type = ui_view_mbx,                                 \
            .init = ui_view_init_mbx,                            \
            .fm = &ui_app.fm.prop.normal,                        \
            .p.text = s,                                         \
            .callback = name ## _chosen,                         \
            .padding = { .left  = 0.125, .top    = 0.25,         \
                         .right = 0.125, .bottom = 0.25 },       \
            .insets  = { .left  = 0.125, .top    = 0.25,         \
                         .right = 0.125, .bottom = 0.25 }        \
        },                                                       \
        .options = name ## _options                              \
    }

#define ui_mbx(s, chosen, ...) {                            \
    .view = {                                               \
        .type = ui_view_mbx, .init = ui_view_init_mbx,      \
        .fm = &ui_app.fm.prop.normal,                       \
        .p.text = s,                                        \
        .callback = chosen,                                 \
        .padding = { .left  = 0.125, .top    = 0.25,        \
                     .right = 0.125, .bottom = 0.25 },      \
        .insets  = { .left  = 0.125, .top    = 0.25,        \
                     .right = 0.125, .bottom = 0.25 }       \
    },                                                      \
    .options = (const char*[]){ __VA_ARGS__, null },        \
}

rt_end_c
