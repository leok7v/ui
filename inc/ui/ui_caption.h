/* Copyright (c) Dmitry "Leo" Kuznetsov 2021-24 see LICENSE for details */
#include "ui/ui.h"

begin_c

typedef struct ui_caption_s {
    ui_view_t view;
    int64_t (*hit_test)(int32_t x, int32_t y);
    // children of caption
    ui_button_t button_menu; // ui_caption.button_menu.cb = callback
    ui_button_t button_mini;
    ui_button_t button_maxi;
    ui_button_t button_full;
    ui_button_t button_quit;
} ui_caption_t;

extern ui_caption_t ui_caption;

end_c
