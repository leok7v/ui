/* Copyright (c) Dmitry "Leo" Kuznetsov 2021-24 see LICENSE for details */
#include "ui/ui.h"

begin_c

typedef struct  {
    int32_t const mode_default;     // = 0
    int32_t const mode_allow_dark;  // = 1
    int32_t const mode_force_dark;  // = 2
    int32_t const mode_force_light; // = 3
    // is Windows Desktop Manager and Apps in Light or Dark mode?
    bool (*is_system_light)(void);
    bool (*are_apps_light)(void);
    bool (*should_apps_use_dark_mode)(void);
    bool (*is_dark_mode_allowed_for_app)(void);
    void (*set_preferred_app_mode)(int32_t mode);
    ui_color_t (*get_color)(void* window, int32_t color_id);
    void (*refresh)(void* window);
    void (*test)(void);
} ui_theme_if;

extern ui_theme_if ui_theme;

end_c
