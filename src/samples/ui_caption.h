/* Copyright (c) Dmitry "Leo" Kuznetsov 2021-24 see LICENSE for details */
#include "ui/ui.h"

begin_c

typedef struct ui_caption_s {
    ui_view_t view;
    ui_point_t dragging;
    ui_point_t resizing;
} ui_caption_t;

extern ui_caption_t ui_caption;

end_c
