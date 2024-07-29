/* Copyright (c) Dmitry "Leo" Kuznetsov 2021-24 see LICENSE for details */
#include "rt/rt.h"
#include "ui/ui.h"
#include "ui/rt_win32.h"

static int32_t ui_theme_dark = -1; // -1 unknown

static errno_t ui_theme_reg_get_uint32(HKEY root, const char* path,
        const char* key, DWORD *v) {
    *v = 0;
    DWORD type = REG_DWORD;
    DWORD light_theme = 0;
    DWORD bytes = sizeof(light_theme);
    errno_t r = RegGetValueA(root, path, key, RRF_RT_DWORD, &type, v, &bytes);
    if (r != 0) {
        rt_println("RegGetValueA(%s\\%s) failed %s", path, key, rt_strerr(r));
    }
    return r;
}

#pragma push_macro("ux_theme_reg_cv")
#pragma push_macro("ux_theme_reg_default_colors")

#define ux_theme_reg_cv "SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\"
#define ux_theme_reg_default_colors ux_theme_reg_cv "Themes\\DefaultColors\\"

static bool ui_theme_use_light_theme(const char* key) {
    if ((!ui_app.dark_mode && !ui_app.light_mode) ||
        ( ui_app.dark_mode &&  ui_app.light_mode)) {
        const char* personalize  = ux_theme_reg_cv "Themes\\Personalize";
        DWORD light_theme = 0;
        ui_theme_reg_get_uint32(HKEY_CURRENT_USER, personalize, key, &light_theme);
        return light_theme != 0;
    } else if (ui_app.light_mode) {
        return true;
    } else {
        rt_assert(ui_app.dark_mode);
        return false;
    }
}

#pragma pop_macro("ux_theme_reg_cv")
#pragma pop_macro("ux_theme_reg_default_colors")

static HMODULE ui_theme_uxtheme(void) {
    static HMODULE uxtheme;
    if (uxtheme == null) {
        uxtheme = GetModuleHandleA("uxtheme.dll");
        if (uxtheme == null) {
            uxtheme = LoadLibraryA("uxtheme.dll");
        }
    }
    rt_not_null(uxtheme);
    return uxtheme;
}

static void* ui_theme_uxtheme_func(uint16_t ordinal) {
    HMODULE uxtheme = ui_theme_uxtheme();
    void* proc = (void*)GetProcAddress(uxtheme, MAKEINTRESOURCEA(ordinal));
    rt_not_null(proc);
    return proc;
}

static void ui_theme_set_preferred_app_mode(int32_t mode) {
    typedef BOOL (__stdcall *SetPreferredAppMode_t)(int32_t mode);
    SetPreferredAppMode_t SetPreferredAppMode = (SetPreferredAppMode_t)
            (SetPreferredAppMode_t)ui_theme_uxtheme_func(135);
    errno_t r = rt_b2e(SetPreferredAppMode(mode));
    // On Win11: 10.0.22631
    // SetPreferredAppMode(true) failed 0x0000047E(1150) ERROR_OLD_WIN_VERSION
    // "The specified program requires a newer version of Windows."
    if (r != 0 && r != ERROR_PROC_NOT_FOUND && r != ERROR_OLD_WIN_VERSION) {
        rt_println("SetPreferredAppMode(AllowDark) failed %s", rt_strerr(r));
    }
}

// https://stackoverflow.com/questions/75835069/dark-system-contextmenu-in-window

static void ui_theme_flush_menu_themes(void) {
    typedef BOOL (__stdcall *FlushMenuThemes_t)(void);
    FlushMenuThemes_t FlushMenuThemes = (FlushMenuThemes_t)
            (FlushMenuThemes_t)ui_theme_uxtheme_func(136);
    errno_t r = rt_b2e(FlushMenuThemes());
    // FlushMenuThemes() works but returns ERROR_OLD_WIN_VERSION
    // on newest Windows 11 but it is not documented thus no complains.
    if (r != 0 && r != ERROR_PROC_NOT_FOUND && r != ERROR_OLD_WIN_VERSION) {
        rt_println("FlushMenuThemes(AllowDark) failed %s", rt_strerr(r));
    }
}

static void ui_theme_allow_dark_mode_for_app(bool allow) {
    // https://github.com/rizonesoft/Notepad3/tree/96a48bd829a3f3192bbc93cd6944cafb3228b96d/src/DarkMode
    typedef BOOL (__stdcall *AllowDarkModeForApp_t)(bool allow);
    AllowDarkModeForApp_t AllowDarkModeForApp =
            (AllowDarkModeForApp_t)ui_theme_uxtheme_func(132);
    if (AllowDarkModeForApp != null) {
        errno_t r = rt_b2e(AllowDarkModeForApp(allow));
        if (r != 0 && r != ERROR_PROC_NOT_FOUND) {
            rt_println("AllowDarkModeForApp(true) failed %s", rt_strerr(r));
        }
    }
}

static void ui_theme_allow_dark_mode_for_window(bool allow) {
    typedef BOOL (__stdcall *AllowDarkModeForWindow_t)(HWND hWnd, bool allow);
    AllowDarkModeForWindow_t AllowDarkModeForWindow =
        (AllowDarkModeForWindow_t)ui_theme_uxtheme_func(133);
    if (AllowDarkModeForWindow != null) {
        int r = rt_b2e(AllowDarkModeForWindow((HWND)ui_app.window, allow));
        // On Win11: 10.0.22631
        // AllowDarkModeForWindow(true) failed 0x0000047E(1150) ERROR_OLD_WIN_VERSION
        // "The specified program requires a newer version of Windows."
        if (r != 0 && r != ERROR_PROC_NOT_FOUND && r != ERROR_OLD_WIN_VERSION) {
            rt_println("AllowDarkModeForWindow(true) failed %s", rt_strerr(r));
        }
    }
}

static bool ui_theme_are_apps_dark(void) {
    return !ui_theme_use_light_theme("AppsUseLightTheme");
}

static bool ui_theme_is_system_dark(void) {
    return !ui_theme_use_light_theme("SystemUsesLightTheme");
}

static bool ui_theme_is_app_dark(void) {
    if (ui_theme_dark < 0) { ui_theme_dark = ui_theme.are_apps_dark(); }
    return ui_theme_dark;
}

static void ui_theme_refresh(void) {
    rt_swear(ui_app.window != null);
    ui_theme_dark = -1;
    BOOL dark_mode = ui_theme_is_app_dark(); // must be 32-bit "BOOL"
    static const DWORD DWMWA_USE_IMMERSIVE_DARK_MODE = 20;
    /* 20 == DWMWA_USE_IMMERSIVE_DARK_MODE in Windows 11 SDK.
       This value was undocumented for Windows 10 versions 2004
       and later, supported for Windows 11 Build 22000 and later. */
    errno_t r = DwmSetWindowAttribute((HWND)ui_app.window,
        DWMWA_USE_IMMERSIVE_DARK_MODE, &dark_mode, sizeof(dark_mode));
    if (r != 0) {
        rt_println("DwmSetWindowAttribute(DWMWA_USE_IMMERSIVE_DARK_MODE) "
                "failed %s", rt_strerr(r));
    }
    ui_theme.allow_dark_mode_for_app(dark_mode);
    ui_theme.allow_dark_mode_for_window(dark_mode);
    ui_theme.set_preferred_app_mode(dark_mode ?
        ui_theme_app_mode_force_dark : ui_theme_app_mode_force_light);
    ui_theme.flush_menu_themes();
    ui_app.request_layout();
}

ui_theme_if ui_theme = {
    .is_app_dark                  = ui_theme_is_app_dark,
    .is_system_dark               = ui_theme_is_system_dark,
    .are_apps_dark                = ui_theme_are_apps_dark,
    .set_preferred_app_mode       = ui_theme_set_preferred_app_mode,
    .flush_menu_themes            = ui_theme_flush_menu_themes,
    .allow_dark_mode_for_app      = ui_theme_allow_dark_mode_for_app,
    .allow_dark_mode_for_window   = ui_theme_allow_dark_mode_for_window,
    .refresh                      = ui_theme_refresh,
};


