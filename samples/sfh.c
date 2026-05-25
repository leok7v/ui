/* Copyright (c) Dmitry "Leo" Kuznetsov 2021-24 see LICENSE for details */
// Single-File-Header (sfh) dogfood: consumes the generated grab-and-go
// amalgams from include/ the stb way -- one TU defines every module's
// implementation and includes only the top header; the layered includes
// pull sfh_posix.h -> sfh_core.h. (dxd.cpp is C++ and links separately.)
#define core_implementation
#define trace_implementation
#define posix_implementation
#define ui_implementation
#include "sfh_ui.h"

static ui_label_t hello = ui_label(0.0, "Hello from a single header");

static void opened(void) {
    ui_view.add(ui_app.content, &hello, null);
}

static void init(void) {
    ui_app.title  = "sfh";
    ui_app.opened = opened;
}

struct ui_app ui_app = {
    .class_name = "sfh",
    .init = init,
    .window_sizing = {
        .ini_w = 5.0f,
        .ini_h = 2.0f
    }
};
