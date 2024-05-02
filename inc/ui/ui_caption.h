/* Copyright (c) Dmitry "Leo" Kuznetsov 2021-24 see LICENSE for details */
#include "ui/ui.h"

begin_c

typedef struct ui_caption_s {
    ui_view_t view;
    // caption`s children:
    ui_button_t button_menu; // use: ui_caption.button_menu.cb := your callback
    ui_button_t button_mini;
    ui_button_t button_maxi;
    ui_button_t button_full;
    ui_button_t button_quit;
} ui_caption_t;

extern ui_caption_t ui_caption;

end_c
