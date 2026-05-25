/* Copyright (c) Dmitry "Leo" Kuznetsov 2021-24 see LICENSE for details */
#include "ui/ui.h"

posix_begin_c

struct ui_caption {
    struct ui_view view;
    // caption`s children:
    ui_button_t icon;
    ui_label_t title;
    struct ui_view spacer;
    ui_button_t menu; // use: ui_caption.button_menu.cb := your callback
    ui_button_t mode; // switch between dark/light mode
    ui_button_t mini;
    ui_button_t maxi;
    ui_button_t full;
    ui_button_t quit;
};

extern struct ui_caption ui_caption;

posix_end_c
