#pragma once
#include "ut/ut_std.h"
#include "ui/ui_view.h"

begin_c

typedef struct ui_mbx_s ui_mbx_t;

typedef struct ui_mbx_s {
    ui_view_t view;
    void (*cb)(ui_mbx_t* m, int32_t option); // callback -1 on cancel
    ui_label_t text;
    ui_button_t button[16];
    int32_t option; // -1 or option chosen by user
    const char** opts;
} ui_mbx_t;

void ui_mbx_init_(ui_view_t* view);

void ui_mbx_init(ui_mbx_t* mx, const char* option[],
    void (*cb)(ui_mbx_t* m, int32_t option), const char* format, ...);

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
    .view = { .type = ui_view_mbx, .init = ui_mbx_init_,         \
    .child = null, .text = s}, .opts = name ## _options,         \
    .cb = name ## _callback }


#define ui_mbx(s, callback, ...) {                       \
    .view = { .type = ui_view_mbx, .init = ui_mbx_init_, \
    .child = null, .text = s},                           \
    .opts = (const char*[]){ __VA_ARGS__, null },        \
    .cb = callback }

end_c
