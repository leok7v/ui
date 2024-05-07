/* Copyright (c) Dmitry "Leo" Kuznetsov 2021-24 see LICENSE for details */
#include "ui/ui.h"

begin_c

enum {
    ui_color_id_active_title        = 0,
    ui_color_id_button_face         = 1,
    ui_color_id_button_text         = 2,
    ui_color_id_gray_text           = 3,
    ui_color_id_highlight           = 4,
    ui_color_id_highlight_text      = 5,
    ui_color_id_hot_tracking_color  = 6,
    ui_color_id_inactive_title      = 7,
    ui_color_id_inactive_title_text = 8,
    ui_color_id_menu_highlight      = 9,
    ui_color_id_title_text          = 10,
    ui_color_id_window              = 11,
    ui_color_id_window_text         = 12
};

typedef struct  {
    ui_color_t (*get_color)(int32_t color_id);
    bool (*is_system_dark)(void);
    bool (*are_apps_dark)(void);
    void (*refresh)(void* window);
    void (*test)(void);
} ui_theme_if;

extern ui_theme_if ui_theme;

end_c
