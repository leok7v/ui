/* Copyright (c) Dmitry "Leo" Kuznetsov 2021-24 see LICENSE for details */
#include "ut/ut.h"
#include "ui/ui.h"
#include "ui/ut_win32.h"

#pragma push_macro("ux_theme_reg_cv")
#pragma push_macro("ux_theme_reg_default_colors")

#define ux_theme_reg_cv "SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\"
#define ux_theme_reg_default_colors ux_theme_reg_cv "Themes\\DefaultColors\\"

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

static errno_t ui_theme_reg_get_uint32(HKEY root, const char* path,
        const char* key, DWORD *v) {
    *v = 0;
    DWORD type = REG_DWORD;
    DWORD light_theme = 0;
    DWORD bytes = sizeof(light_theme);
    errno_t r = RegGetValueA(root, path, key, RRF_RT_DWORD, &type, v, &bytes);
    if (r != 0) {
        traceln("RegGetValueA(%s\\%s) failed %s", path, key, ut_str.error(r));
    }
    return r;
}

static errno_t ui_theme_reg_get_bin(HKEY root, const char* path,
        const char* key, void *data, uint32_t *bytes) {
    memset(data, 0, *bytes);
    DWORD type = REG_BINARY;
    DWORD n = *bytes;
    errno_t r = RegGetValueA(root, path, key, RRF_RT_REG_BINARY, &type, data, &n);
    if (r == 0) { *bytes = n; }
    if (r != 0) {
        traceln("RegGetValueA(%s\\%s) failed %s", path, key, ut_str.error(r));
    }
    return r;
}

typedef struct {
    int32_t id;
    const char* name;
    ui_color_t  dark;
    ui_color_t  light;
} ui_theme_color_map_t;

// ActiveTitle         : dark 0x006E0037 light 0x00D1B499
// ButtonFace          : dark 0x00000000 light 0x00F0F0F0
// ButtonText          : dark 0x00FFFFFF light 0x00000000
// GrayText            : dark 0x003FF23F light 0x006D6D6D
// Hilight             : dark 0x00FFEB1A light 0x00D77800
// HilightText         : dark 0x00000000 light 0x00FFFFFF
// HotTrackingColor    : dark 0x0000FFFF light 0x00CC6600
// InactiveTitle       : dark 0x002F0000 light 0x00DBCDBF
// InactiveTitleText   : dark 0x00FFFFFF light 0x00000000
// MenuHilight         : dark 0x00800080 light 0x00FF9933
// TitleText           : dark 0x00FFFFFF light 0x00000000
// Window              : dark 0x00000000 light 0x00FFFFFF
// WindowText          : dark 0x00FFFFFF light 0x00000000

static int32_t ui_theme_dark = -1; // -1 unknown

static ui_theme_color_map_t ui_theme_colors[13];

static void ui_theme_init_colors(void) {
    // this is empirically determined for the dark theme, and from Win10
    // the registry in standard light theme.
    // Because all of the:
    // HKEY_CURRENT_USER\Control Panel\Desktop\Colors
    // HKEY_CURRENT_USER\Control Panel\Colors
    // HKEY_USERS\.DEFAULT\Control Panel\Colors
    // HKEY_LOCAL_MACHINE\SOFTWARE\Microsoft\Windows\CurrentVersion\Themes\DefaultColors\Standard
    // Have different colors even for Light mode and none for the Dark Mode simply
    // keep it hardcoded for time being:
    ui_theme_colors[ 0] = (ui_theme_color_map_t){ .id = ui.colors.active_title,        .name = "ActiveTitle"      ,.dark = 0x00000000, .light = 0x00D1B499 };
    ui_theme_colors[ 1] = (ui_theme_color_map_t){ .id = ui.colors.button_face,         .name = "ButtonFace"       ,.dark = 0x00333333, .light = 0x00F0F0F0 };
    ui_theme_colors[ 2] = (ui_theme_color_map_t){ .id = ui.colors.button_text,         .name = "ButtonText"       ,.dark = 0x00FFFFFF, .light = 0x00000000 };
    ui_theme_colors[ 3] = (ui_theme_color_map_t){ .id = ui.colors.gray_text,           .name = "GrayText"         ,.dark = 0x00666666, .light = 0x006D6D6D };
    ui_theme_colors[ 4] = (ui_theme_color_map_t){ .id = ui.colors.highlight,           .name = "Hilight"          ,.dark = 0x00626262, .light = 0x00D77800 };
    ui_theme_colors[ 5] = (ui_theme_color_map_t){ .id = ui.colors.highlight_text,      .name = "HilightText"      ,.dark = 0x00000000, .light = 0x00FFFFFF };
    ui_theme_colors[ 6] = (ui_theme_color_map_t){ .id = ui.colors.hot_tracking_color,  .name = "HotTrackingColor" ,.dark = 0x004D8DFA, .light = 0x00CC6600 };
    ui_theme_colors[ 7] = (ui_theme_color_map_t){ .id = ui.colors.inactive_title,      .name = "InactiveTitle"    ,.dark = 0x002B2B2B, .light = 0x00DBCDBF };
    ui_theme_colors[ 8] = (ui_theme_color_map_t){ .id = ui.colors.inactive_title_text, .name = "InactiveTitleText",.dark = 0x00969696, .light = 0x00000000 };
    ui_theme_colors[ 9] = (ui_theme_color_map_t){ .id = ui.colors.menu_highlight,      .name = "MenuHilight"      ,.dark = 0x00002642, .light = 0x00FF9933 };
    ui_theme_colors[10] = (ui_theme_color_map_t){ .id =  ui.colors.title_text,         .name = "TitleText"        ,.dark = 0x00FFFFFF, .light = 0x00000000 };
    ui_theme_colors[11] = (ui_theme_color_map_t){ .id =  ui.colors.window,             .name = "Window"           ,.dark = 0x00000000, .light = 0x00FFFFFF };
    ui_theme_colors[12] = (ui_theme_color_map_t){ .id =  ui.colors.window_text,        .name = "WindowText"       ,.dark = 0x00FFFFFF, .light = 0x00000000 };
    #ifdef UX_THEME_READ_COLORS_FROM_REGISTRY // when the dust of Win11 settles.
    errno_t r = 0;
    const char* dark  = ux_theme_reg_default_colors "HighContrast";
    const char* light = ux_theme_reg_default_colors "Standard";
    for (int32_t i = 0; i < countof(ui_theme_colors); i++) {
        const char* name = ui_theme_colors[i].name;
        DWORD dc = 0;
        DWORD lc = 0;
        r = ui_theme_reg_get_uint32(HKEY_LOCAL_MACHINE, dark,  name, &dc);
        if (r == 0) { ui_theme_colors[i].dark = dc; }
        ui_theme_reg_get_uint32(HKEY_LOCAL_MACHINE, light, name, &lc);
        if (r == 0) { ui_theme_colors[i].light = lc; }
//      traceln("%-20s: dark %08X light %08X", name, dc, lc);
    }
    #endif
}

static bool ui_theme_use_light_theme(const char* key) {
    const char* personalize  = ux_theme_reg_cv "Themes\\Personalize";
    DWORD light_theme = 0;
    ui_theme_reg_get_uint32(HKEY_CURRENT_USER, personalize, key, &light_theme);
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
    if (!initialized) { ui_theme_init_colors(); initialized = true; }
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
        errno_t r = b2e(SetPreferredAppMode(mode));
        // fails on Windows 10 with: ERROR_RESOURCE_NAME_NOT_FOUND (1814)
        if (r != 0 && r != ERROR_RESOURCE_NAME_NOT_FOUND) { // ignore
            traceln("SetPreferredAppMode(%d) failed %s", mode, ut_str.error(r));
        }
    }
}

static ui_color_t ui_theme_explorer_accents[] = {
    0x00FFD8A6, 0x00EDB976, 0x00E39C42, 0x00D77800,
    0x009E5A00, 0x00754200, 0x00422600, 0x00981788
};

static void ui_theme_test(void) {
    DWORD window = GetSysColor(COLOR_WINDOW);
    DWORD text   = GetSysColor(COLOR_WINDOWTEXT);
    traceln("COLOR_WINDOW: 0x%08X COLOR_WINDOWTEXT: 0x%08X", window, text);
    DWORD colors[8] = {0};
    uint32_t bytes = sizeof(colors);
    ui_theme_reg_get_bin(HKEY_CURRENT_USER,
        ux_theme_reg_cv "Explorer\\Accent",
        "AccentPalette", &colors, &bytes);
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

ut_static_init(ui_theme) { ui_theme.test(); }

// Experimental Dark:

// ActiveTitle         : dark 0x00000000 ***
// ButtonFace          : dark 0x00333333 ***
// ButtonText          : dark 0x00FFFFFF ***
// GrayText            : dark 0x00666666 ***
// Hilight             : dark 0x00626262 ***
// HilightText         : dark 0x00000000 ***
// HotTrackingColor    : dark 0x004D8DFA ***  0x00D77800 is light
// InactiveTitle       : dark 0x002B2B2B ***
// InactiveTitleText   : dark 0x00969696 *** (alt A1A1A1 or AAAAAA)
// MenuHilight         : dark 0x00002642 ***
// TitleText           : dark 0x00FFFFFF ***
// Window              : dark 0x00000000 ***
// WindowText          : dark 0x00FFFFFF ***


//
//  Computer\HKEY_CURRENT_USER\Control Panel\Desktop\
//  AutoColorization=1
//
//  Computer\HKEY_CURRENT_USER\SOFTWARE\Microsoft\Windows\DWM
//  ColorizationColor=0xC43A3A3A // (alpha: C4!)
//  EnableWindowColorization=0x84 // (bitset of something)
//
//  TODO: these values differ:
//
// Computer\HKEY_CURRENT_USER\Control Panel\Desktop\Colors
// Computer\HKEY_CURRENT_USER\Control Panel\Colors
//
//  TODO: may be relevant too:
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
//
// Computer\HKEY_USERS\.DEFAULT\Control Panel\Colors
// a lot of colors

// https://superuser.com/questions/1245923/registry-keys-to-change-personalization-settings/1395560#1395560
// Active Window Border
// [HKEY_CURRENT_USER\Software\Microsoft\Windows\CurrentVersion\Explorer\Accent]
// "AccentColorMenu"=dword:ffb16300
// Active Window Title Bar
// [HKEY_CURRENT_USER\Software\Microsoft\Windows\DWM]
// "AccentColor"=dword:ffb16300
// Inactive Window Title Bar
// [HKEY_CURRENT_USER\Software\Microsoft\Windows\DWM]
// "AccentColorInactive"=dword:ffb16300
//
//
//
//
//
//
//
//
//
//
//


#pragma pop_macro("ux_theme_reg_cv")
#pragma pop_macro("ux_theme_reg_default_colors")


