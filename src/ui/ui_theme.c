/* Copyright (c) Dmitry "Leo" Kuznetsov 2021-24 see LICENSE for details */
#include "ut/ut.h"
#include "ui/ui.h"
#include "ui/ut_win32.h"

static HMODULE ui_theme_ux_theme(void) {
    static HMODULE ux_theme;
    if (ux_theme == null) {
        ux_theme = GetModuleHandleA("uxtheme.dll");
    }
    if (ux_theme == null) {
        ux_theme = (HMODULE)ut_loader.open("uxtheme.dll", ut_loader.local);
    }
    not_null(ux_theme);
    return ux_theme;
}

static void ui_theme_reg_get_word(HKEY root, const char* path,
        const char* key, DWORD *v) {
    *v = 0;
    DWORD type = REG_DWORD;
    DWORD light_theme = 0;
    DWORD bytes = sizeof(light_theme);
    errno_t r = RegGetValueA(root, path, key, RRF_RT_DWORD, &type, v, &bytes);
    fatal_if_not_zero(r, "RegGetValueA(%s) failed %s", key, ut_str.error(r));
}

#define ux_theme_reg_cv "SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\"
#define ux_theme_reg_default_colors ux_theme_reg_cv "Themes\\DefaultColors\\"

typedef struct {
    int32_t id;
    const char* name;
    ui_color_t  dark;
    ui_color_t  light;
} ui_theme_color_map_t;

static int32_t ui_theme_dark = -1; // -1 unknown

static ui_theme_color_map_t ui_theme_colors[13];

static void ui_theme_init_colors(void) {
    ui_theme_colors[0] = (ui_theme_color_map_t){ ui.colors.active_title, "ActiveTitle" };
    ui_theme_colors[1] = (ui_theme_color_map_t){ ui.colors.button_face, "ButtonFace" };
    ui_theme_colors[2] = (ui_theme_color_map_t){ ui.colors.button_text, "ButtonText" };
    ui_theme_colors[3] = (ui_theme_color_map_t){ ui.colors.gray_text, "GrayText" };
    ui_theme_colors[4] = (ui_theme_color_map_t){ ui.colors.highlight, "Hilight" };
    ui_theme_colors[5] = (ui_theme_color_map_t){ ui.colors.highlight_text, "HilightText" };
    ui_theme_colors[6] = (ui_theme_color_map_t){ ui.colors.hot_tracking_color, "HotTrackingColor" };
    ui_theme_colors[7] = (ui_theme_color_map_t){ ui.colors.inactive_title, "InactiveTitle" };
    ui_theme_colors[8] = (ui_theme_color_map_t){ ui.colors.inactive_title_text, "InactiveTitleText" };
    ui_theme_colors[9] = (ui_theme_color_map_t){ ui.colors.menu_highlight, "MenuHilight" };
    ui_theme_colors[10] = (ui_theme_color_map_t){ ui.colors.title_text, "TitleText" };
    ui_theme_colors[11] = (ui_theme_color_map_t){ ui.colors.window, "Window" };
    ui_theme_colors[12] = (ui_theme_color_map_t){ ui.colors.window_text, "WindowText" };
    const char* dark  = ux_theme_reg_default_colors "HighContrast";
    const char* light = ux_theme_reg_default_colors "Standard";
    for (int32_t i = 0; i < countof(ui_theme_colors); i++) {
        const char* name = ui_theme_colors[i].name;
        DWORD dc = 0;
        DWORD lc = 0;
        ui_theme_reg_get_word(HKEY_LOCAL_MACHINE, dark,  name, &dc);
        ui_theme_colors[i].dark = dc;
        ui_theme_reg_get_word(HKEY_LOCAL_MACHINE, light, name, &lc);
        ui_theme_colors[i].light = lc;
//      traceln("%-20s: dark %08X light %08X", name, dc, lc);
    }
}

static bool ui_theme_use_light_theme(const char* key) {
    const char* personalize  = ux_theme_reg_cv "Themes\\Personalize";
    DWORD light_theme = 0;
    ui_theme_reg_get_word(HKEY_CURRENT_USER, personalize, key, &light_theme);
    return light_theme != 0;
}

static bool ui_theme_are_apps_light(void) {
    return ui_theme_use_light_theme("AppsUseLightTheme");
}

static bool ui_theme_is_system_light(void) {
    return ui_theme_use_light_theme("SystemUsesLightTheme");
}

static ui_color_t ui_theme_get_color(int32_t color_id) {
    swear(0 <= color_id && color_id < countof(ui_theme_colors));
    static bool initialized;
    if (!initialized) { ui_theme_init_colors(); }
    if (ui_theme_dark < 0) {
        bool are_apps_light = ui_theme.are_apps_light();
        bool is_system_light  = ui_theme.is_system_light();
        bool allowed  = ui_theme.is_dark_mode_allowed_for_app();
        bool dark  = ui_theme.should_apps_use_dark_mode();
        ui_theme_dark = !is_system_light && !are_apps_light && allowed && dark;
        if (ui_theme_dark) {
            ui_theme.set_preferred_app_mode(ui_theme.mode_force_dark);
        }
    }
    return ui_theme_dark ? ui_theme_colors[color_id].dark :
                           ui_theme_colors[color_id].light;
}

static void ui_theme_refresh(void* window) {
    ui_theme_dark = -1;
    BOOL dark_mode = !ui_theme.are_apps_light();
    static const DWORD DWMWA_USE_IMMERSIVE_DARK_MODE = 20;
    /* 20 == DWMWA_USE_IMMERSIVE_DARK_MODE in Windows 11 SDK.
       This value was undocumented for Windows 10 versions 2004
       and later, supported for Windows 11 Build 22000 and later. */
    errno_t r = DwmSetWindowAttribute((HWND)window,
        DWMWA_USE_IMMERSIVE_DARK_MODE, &dark_mode, sizeof(dark_mode));
    if (r != 0) {
        traceln("DwmSetWindowAttribute(DWMWA_USE_IMMERSIVE_DARK_MODE) "
                "failed %s", ut_str.error(r));
    }
    ui_app.layout();
}

static bool ui_theme_is_dark_mode_allowed_for_app(void) {
    typedef BOOL (__stdcall *IsDarkModeAllowedForApp_t)(void);
    IsDarkModeAllowedForApp_t IsDarkModeAllowedForApp = (IsDarkModeAllowedForApp_t)
            (void*)GetProcAddress(ui_theme_ux_theme(), MAKEINTRESOURCE(136));
    if (IsDarkModeAllowedForApp != null) {
        return IsDarkModeAllowedForApp();
    }
    return false;
}

static bool ui_theme_should_apps_use_dark_mode(void) {
    typedef BOOL (__stdcall *ShouldAppsUseDarkMode_t)(void);
    ShouldAppsUseDarkMode_t ShouldAppsUseDarkMode = (ShouldAppsUseDarkMode_t)
            (void*)GetProcAddress(ui_theme_ux_theme(), MAKEINTRESOURCE(132));
    if (ShouldAppsUseDarkMode != null) {
        return ShouldAppsUseDarkMode();
    }
    return false;
}

static void ui_theme_set_preferred_app_mode(int32_t mode) {
    typedef BOOL (__stdcall *SetPreferredAppMode_t)(bool allow);
    SetPreferredAppMode_t SetPreferredAppMode = (SetPreferredAppMode_t)
            (void*)GetProcAddress(ui_theme_ux_theme(), MAKEINTRESOURCE(135));
    if (SetPreferredAppMode != null) {
        int r = b2e(SetPreferredAppMode(mode));
        // fails on Windows 10 with: ERROR_RESOURCE_NAME_NOT_FOUND (1814)
        if (r != 0 && r != ERROR_RESOURCE_NAME_NOT_FOUND) { // ignore
            traceln("SetPreferredAppMode(%d) failed %s", mode, ut_str.error(r));
        }
    }
}

static void ui_theme_test(void) {
    ui_theme_init_colors();
    HMODULE ux_theme = ui_theme_ux_theme();
    traceln("ux_theme: %p", ux_theme);
    bool are_apps_light = ui_theme.are_apps_light();
    bool is_system_light  = ui_theme.is_system_light();
    bool dark  = ui_theme.should_apps_use_dark_mode();
    bool allowed  = ui_theme.is_dark_mode_allowed_for_app();
    traceln("light is_system_light(): %d are_apps_light(): %d "
            "should_apps_use_dark_mode(): %d "
            "is_dark_mode_allowed_for_app(): %d",
            is_system_light, are_apps_light, dark, allowed);
    if (dark) {
        ui_theme.set_preferred_app_mode(ui_theme.mode_force_dark);
    }
    for (int32_t i = 0; i < countof(ui_theme_colors); i++) {
        ui_color_t c = ui_theme.get_color(ui_theme_colors[i].id);
        traceln("%-20s 0x%08X", ui_theme_colors[i].name, ui_color_rgb(c));
    }
}

ui_theme_if ui_theme = {
    .mode_default     = 0,
    .mode_allow_dark  = 1,
    .mode_force_dark  = 2,
    .mode_force_light = 3,
    .is_system_light              = ui_theme_is_system_light,
    .are_apps_light               = ui_theme_are_apps_light,
    .should_apps_use_dark_mode    = ui_theme_should_apps_use_dark_mode,
    .is_dark_mode_allowed_for_app = ui_theme_is_dark_mode_allowed_for_app,
    .set_preferred_app_mode       = ui_theme_set_preferred_app_mode,
    .get_color                    = ui_theme_get_color,
    .refresh                      = ui_theme_refresh,
    .test                         = ui_theme_test
};

// ut_static_init(ui_theme) { ui_theme.test(); }

//  TODO: may be relevant too:
//
//  HKEY_CURRENT_USER\SOFTWARE\Microsoft\Windows\DWM\
//  absent:
//  AccentColorInactive
//  present:
//  AccentColor ff3a3a3a
//  ColorizationAfterglow
//  ColorizationColor
//
//  HKEY_CURRENT_USER\SOFTWARE\Microsoft\Windows\CurrentVersion\Explorer\Accent
//  AccentColorMenu
//  StartColorMenu
//  AccentPalette binary 8x4byte colors
