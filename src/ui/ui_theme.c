/* Copyright (c) Dmitry "Leo" Kuznetsov 2021-24 see LICENSE for details */
#include "ut/ut.h"
#include "ui/ui.h"
#include "ui/ut_win32.h"

// https://github.com/mintty/mintty/blob/3.6.1/src/winmain.c
// https://github.com/godotengine/godot-proposals/issues/1868
// https://github.com/godotengine/godot/pull/65026
// https://projects.blender.org/blender/blender/commit/ddbac88c08ef
// https://github.com/stefankueng/sktoolslib/blob/main/DarkModeHelper.cpp
//
// GetImmersiveColorFromColorSetEx = (GetImmersiveColorFromColorSetExPtr)GetProcAddress(ux_theme_lib, MAKEINTRESOURCEA(95));
// GetImmersiveColorTypeFromName = (GetImmersiveColorTypeFromNamePtr)GetProcAddress(ux_theme_lib, MAKEINTRESOURCEA(96));
// GetImmersiveUserColorSetPreference = (GetImmersiveUserColorSetPreferencePtr)GetProcAddress(ux_theme_lib, MAKEINTRESOURCEA(98));
//
// https://stackoverflow.com/questions/33680359/getimmersivecolortypefromname-always-returning-1
// https://stackoverflow.com/questions/56865923/windows-10-taskbar-color-detection-for-tray-icon/56867641#56867641

#pragma warning(disable: 28159 4996)
#pragma warning(disable: 4996)

static HMODULE ui_theme_ux_theme(void) {
    static HMODULE ux_theme;
    OSVERSIONINFOEXA vi = { .dwOSVersionInfoSize = sizeof(OSVERSIONINFOEX) };
    if (IsWindows10OrGreater() && GetVersionExA((OSVERSIONINFOA*)&vi)) {
//      traceln("Windows %lu.%lu Build %lu",
//              vi.dwMajorVersion, vi.dwMinorVersion, vi.dwBuildNumber);
    } else {
//      traceln("Failed to get version info");
    }
    // minimum version 1809:
    if (vi.dwMajorVersion >= 10 && vi.dwBuildNumber >= 17763) {
        if (ux_theme == null) {
            ux_theme = GetModuleHandleA("uxtheme.dll");
        }
        if (ux_theme == null) {
            ux_theme = (HMODULE)ut_loader.open("uxtheme.dll", ut_loader.local);
        }
        not_null(ux_theme);
    } else {
        static const char* manifest_xml =
        "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\"?>                                                  \n"
        "<assembly manifestVersion=\"1.0\" xmlns=\"urn:schemas-microsoft-com:asm.v1\">                                  \n"
        "  <compatibility xmlns=\"urn:schemas-microsoft-com:compatibility.v1\">                                         \n"
        "    <application>                                                                                              \n"
        "      <!-- Support for Windows 10 -->                                                                          \n"
        "      <supportedOS Id=\"{8e0f7a12-bfb3-4fe8-b9a5-48fd50a15a9a}\"/>                                             \n"
        "    </application>                                                                                             \n"
        "  </compatibility>                                                                                             \n"
        "  <application>                                                                                                \n"
        "    <windowsSettings>                                                                                          \n"
        "      <activeCodePage xmlns=\"http://schemas.microsoft.com/SMI/2019/WindowsSettings\">UTF-8</activeCodePage>   \n"
        "    </windowsSettings>                                                                                         \n"
        "  </application>                                                                                               \n"
        "</assembly>                                                                                                    \n";
        fatal("need file\nmanifest.xml\n"
              "with the content\n"
              "%s\n"
              "included in msbuild project", manifest_xml);
    }
    return ux_theme;
}

static bool ui_theme_use_light_theme(const char* key) {
    DWORD type = REG_DWORD;
    DWORD light_theme = 0;
    DWORD bytes = sizeof(light_theme);
    errno_t r = RegGetValueA(HKEY_CURRENT_USER,
        "SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Themes\\Personalize",
        key, RRF_RT_DWORD, &type, &light_theme, &bytes);
    if (r != 0) {
        traceln("RegGetValueA(%s) failed %s", key, ut_str.error(r));
    }
    return light_theme != 0;
}

static bool ui_theme_are_apps_light(void) {
    return ui_theme_use_light_theme("AppsUseLightTheme");
}

static bool ui_theme_is_system_light(void) {
    return ui_theme_use_light_theme("SystemUsesLightTheme");
}

static ui_color_t ui_theme_get_color(void* window, int32_t color_id) {
    not_null(window);
    static HTHEME ui_theme_handle;
    const wchar_t* class_list = L""
        "CompositedWindow::Window;WINDOW;BUTTON;CLOCK;COMBOBOX;COMMUNICATIONS;"
        "CONTROLPANEL;DATEPICKER;DRAGDROP;EDIT;EXPLORERBAR;FLYOUT;"
        "GLOBALS;HEADER;LISTBOX;LISTVIEW;MENU;MENUBAND;NAVIGATION;"
        "PAGE;PROGRESS;REBAR;SCROLLBAR;SEARCHEDITBOX;SPIN;STARTPANEL;"
        "STATUS;TAB;TASKBAND;TASKBAR;TASKDIALOG;TEXTSTYLE;TOOLBAR;TOOLTIP;"
        "TRACKBAR;TRAYNOTIFY;TREEVIEW";
    if (ui_theme_handle == null) {
        ui_theme_handle = OpenThemeData((HWND)window, class_list);
        not_null(ui_theme_handle);
    }
    return GetThemeSysColor(ui_theme_handle, color_id);
}

// case WM_SETTINGCHANGE:
//           if (wcscmp(LPCWSTR(lParam), L"ImmersiveColorSet") == 0) {
//             ui_theme.refresh();
//           }
//           break;

static void ui_theme_refresh(void* window) {
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
        if (r != ERROR_RESOURCE_NAME_NOT_FOUND) { // ignore
            traceln("SetPreferredAppMode(%d) failed %s", mode, ut_str.error(r));
        }
    }
}

#ifdef UI_THEME_NEED_TO_USE_LEGACY_CALL

static void app_allow_dark_mode_for_window(void) {
    HMODULE ux_theme = GetModuleHandleA("ux_theme.dll");
    not_null(ux_theme);
    typedef BOOL (__stdcall *AllowDarkModeForWindow_t)(HWND hWnd, bool allow);
    AllowDarkModeForWindow_t AllowDarkModeForWindow = (AllowDarkModeForWindow_t)
        (void*)GetProcAddress(ui_theme_ux_theme(), MAKEINTRESOURCE(133));
    if (AllowDarkModeForWindow != null) {
        int r = b2e(AllowDarkModeForWindow((HWND)app.window, true));
        if (r != 0 && r != ERROR_PROC_NOT_FOUND) {
            traceln("AllowDarkModeForWindow(true) failed %s", ut_str.error(r));
        }
    }
}

#endif

static void ui_theme_test(void) {
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

// ut_static_init(ui_theme) {
//     ui_theme.test();
// }