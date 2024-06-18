/* Copyright (c) Dmitry "Leo" Kuznetsov 2021-24 see LICENSE for details */
#include "ui/ui.h"

begin_c

enum {
    ui_color_id_undefined           =  0,
    ui_color_id_active_title        =  1,
    ui_color_id_button_face         =  2,
    ui_color_id_button_text         =  3,
    ui_color_id_gray_text           =  4,
    ui_color_id_highlight           =  5,
    ui_color_id_highlight_text      =  6,
    ui_color_id_hot_tracking_color  =  7,
    ui_color_id_inactive_title      =  8,
    ui_color_id_inactive_title_text =  9,
    ui_color_id_menu_highlight      = 10,
    ui_color_id_title_text          = 11,
    ui_color_id_window              = 12,
    ui_color_id_window_text         = 13
};

enum {
    ui_theme_app_mode_default     = 0,
    ui_theme_app_mode_allow_dark  = 1,
    ui_theme_app_mode_force_dark  = 2,
    ui_theme_app_mode_force_light = 3
};

typedef struct  {
    bool (*is_app_dark)(void);
    bool (*is_system_dark)(void);
    bool (*are_apps_dark)(void);
    void (*set_preferred_app_mode)(int32_t mode);
    void (*flush_menu_themes)(void);
    void (*allow_dark_mode_for_app)(bool allow);
    void (*allow_dark_mode_for_window)(bool allow);
    void (*refresh)(void);
    void (*test)(void);
} ui_theme_if;


extern ui_theme_if ui_theme;

end_c
