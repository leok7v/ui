#include "ut/ut.h"
#include "ui/ui.h"
#include "ut/ut_win32.h"

#pragma push_macro("ui_app_window")
#pragma push_macro("ui_app_canvas")

#define ui_app_window() ((HWND)ui_app.window)
#define ui_app_canvas() ((HDC)ui_app.canvas)

enum { ui_long_press_msec = 250 };

static NONCLIENTMETRICSW ui_app_ncm = { sizeof(NONCLIENTMETRICSW) };
static MONITORINFO ui_app_mi = {sizeof(MONITORINFO)};

static HANDLE ui_app_event_quit;
static HANDLE ui_app_event_invalidate;

static uintptr_t ui_app_timer_1s_id;
static uintptr_t ui_app_timer_100ms_id;

static bool ui_app_layout_dirty; // call layout() before paint

typedef void (*ui_app_animate_function_t)(int32_t step);

static struct {
    ui_app_animate_function_t f;
    int32_t count;
    int32_t step;
    ui_timer_t timer;
} ui_app_animate;

// Animation timer is Windows minimum of 10ms, but in reality the timer
// messages are far from isochronous and more likely to arrive at 16 or
// 32ms intervals and can be delayed.

// https://docs.microsoft.com/en-us/windows/win32/inputdev/virtual-key-codes
// https://docs.microsoft.com/en-us/windows/win32/inputdev/wm-keydown

static void ui_app_alt_ctrl_shift(bool down, int64_t key) {
    if (key == VK_MENU)    { ui_app.alt   = down; }
    if (key == VK_CONTROL) { ui_app.ctrl  = down; }
    if (key == VK_SHIFT)   { ui_app.shift = down; }
}

static inline ui_point_t ui_app_point2ui(const POINT* p) {
    ui_point_t u = { p->x, p->y };
    return u;
}

static inline POINT ui_app_ui2point(const ui_point_t* u) {
    POINT p = { u->x, u->y };
    return p;
}

static ui_rect_t ui_app_rect2ui(const RECT* r) {
    ui_rect_t u = { r->left, r->top, r->right - r->left, r->bottom - r->top };
    return u;
}

static RECT ui_app_ui2rect(const ui_rect_t* u) {
    RECT r = { u->x, u->y, u->x + u->w, u->y + u->h };
    return r;
}

static void ui_app_update_ncm(int32_t dpi) {
    // Only UTF-16 version supported SystemParametersInfoForDpi
    fatal_if_false(SystemParametersInfoForDpi(SPI_GETNONCLIENTMETRICS,
        sizeof(ui_app_ncm), &ui_app_ncm, 0, dpi));
}

static void ui_app_update_monitor_dpi(HMONITOR monitor, ui_dpi_t* dpi) {
    for (int32_t mtd = MDT_EFFECTIVE_DPI; mtd <= MDT_RAW_DPI; mtd++) {
        uint32_t dpi_x = 0;
        uint32_t dpi_y = 0;
        // GetDpiForMonitor() may return ERROR_GEN_FAILURE 0x8007001F when
        // system wakes up from sleep:
        // ""A device attached to the system is not functioning."
        // docs say:
        // "May be used to indicate that the device has stopped responding
        // or a general failure has occurred on the device.
        // The device may need to be manually reset."
        int32_t r = GetDpiForMonitor(monitor, (MONITOR_DPI_TYPE)mtd, &dpi_x, &dpi_y);
        if (r != 0) {
            ut_thread.sleep_for(1.0 / 32); // and retry:
            r = GetDpiForMonitor(monitor, (MONITOR_DPI_TYPE)mtd, &dpi_x, &dpi_y);
        }
        if (r == 0) {
//          const char* names[] = {"EFFECTIVE_DPI", "ANGULAR_DPI","RAW_DPI"};
//          traceln("%s %d %d", names[mtd], dpi_x, dpi_y);
            // EFFECTIVE_DPI 168 168 (with regard of user scaling)
            // ANGULAR_DPI 247 248 (diagonal)
            // RAW_DPI 283 284 (horizontal, vertical)
            switch (mtd) {
                case MDT_EFFECTIVE_DPI:
                    dpi->monitor_effective = ut_max(dpi_x, dpi_y); break;
                case MDT_ANGULAR_DPI:
                    dpi->monitor_angular = ut_max(dpi_x, dpi_y); break;
                case MDT_RAW_DPI:
                    dpi->monitor_raw = ut_max(dpi_x, dpi_y); break;
                default: assert(false);
            }
        }
    }
}

#ifndef QUICK_DEBUG

static void ui_app_dump_dpi(void) {
    traceln("ui_app.dpi.monitor_effective: %d", ui_app.dpi.monitor_effective  );
    traceln("ui_app.dpi.monitor_angular  : %d", ui_app.dpi.monitor_angular    );
    traceln("ui_app.dpi.monitor_raw      : %d", ui_app.dpi.monitor_raw        );
    traceln("ui_app.dpi.window           : %d", ui_app.dpi.window             );
    traceln("ui_app.dpi.system           : %d", ui_app.dpi.system             );
    traceln("ui_app.dpi.process          : %d", ui_app.dpi.process            );

    traceln("ui_app.mrc      : %d,%d %dx%d", ui_app.mrc.x, ui_app.mrc.y, ui_app.mrc.w, ui_app.mrc.h);
    traceln("ui_app.wrc      : %d,%d %dx%d", ui_app.wrc.x, ui_app.wrc.y, ui_app.wrc.w, ui_app.wrc.h);
    traceln("ui_app.crc      : %d,%d %dx%d", ui_app.crc.x, ui_app.crc.y, ui_app.crc.w, ui_app.crc.h);
    traceln("ui_app.work_area: %d,%d %dx%d", ui_app.work_area.x, ui_app.work_area.y, ui_app.work_area.w, ui_app.work_area.h);

    int32_t mxt_x = GetSystemMetrics(SM_CXMAXTRACK);
    int32_t mxt_y = GetSystemMetrics(SM_CYMAXTRACK);
    traceln("MAXTRACK: %d, %d", mxt_x, mxt_y);
    int32_t scr_x = GetSystemMetrics(SM_CXSCREEN);
    int32_t scr_y = GetSystemMetrics(SM_CYSCREEN);
    fp32_t monitor_x = scr_x / (fp32_t)ui_app.dpi.monitor_raw;
    fp32_t monitor_y = scr_y / (fp32_t)ui_app.dpi.monitor_raw;
    traceln("SCREEN: %d, %d %.1fx%.1f\"", scr_x, scr_y, monitor_x, monitor_y);
}

#endif

static bool ui_app_update_mi(const ui_rect_t* r, uint32_t flags) {
    RECT rc = ui_app_ui2rect(r);
    HMONITOR monitor = MonitorFromRect(&rc, flags);
//  TODO: moving between monitors with different DPIs
//  HMONITOR mw = MonitorFromWindow(ui_app_window(), flags);
    if (monitor != null) {
        ui_app_update_monitor_dpi(monitor, &ui_app.dpi);
        fatal_if_false(GetMonitorInfoA(monitor, &ui_app_mi));
        ui_app.work_area = ui_app_rect2ui(&ui_app_mi.rcWork);
        ui_app.mrc = ui_app_rect2ui(&ui_app_mi.rcMonitor);
//      ui_app_dump_dpi();
    }
    return monitor != null;
}

static void ui_app_update_crc(void) {
    RECT rc = {0};
    fatal_if_false(GetClientRect(ui_app_window(), &rc));
    ui_app.crc = ui_app_rect2ui(&rc);
    ui_app.width = ui_app.crc.w;
    ui_app.height = ui_app.crc.h;
}

static void ui_app_dispose_fonts(void) {
    fatal_if_false(DeleteFont(ui_app.fonts.regular));
    fatal_if_false(DeleteFont(ui_app.fonts.H1));
    fatal_if_false(DeleteFont(ui_app.fonts.H2));
    fatal_if_false(DeleteFont(ui_app.fonts.H3));
    fatal_if_false(DeleteFont(ui_app.fonts.mono));
}

static void ui_app_init_fonts(int32_t dpi) {
    ui_app_update_ncm(dpi);
    if (ui_app.fonts.regular != null) { ui_app_dispose_fonts(); }
    LOGFONTW lf = ui_app_ncm.lfMessageFont;
    // lf.lfQuality is CLEARTYPE_QUALITY which looks bad on 4K monitors
    // Windows UI uses PROOF_QUALITY which is aliased w/o ClearType rainbows
    lf.lfQuality = PROOF_QUALITY;
    ui_app.fonts.regular = (ui_font_t)CreateFontIndirectW(&lf);
    not_null(ui_app.fonts.regular);
    const fp64_t fh = ui_app_ncm.lfMessageFont.lfHeight;
//  traceln("lfHeight=%.1f", fh);
    assert(fh != 0);
    lf.lfWeight = FW_SEMIBOLD;
    lf.lfHeight = (int32_t)(fh * 1.75);
    ui_app.fonts.H1 = (ui_font_t)CreateFontIndirectW(&lf);
    lf.lfWeight = FW_SEMIBOLD;
    lf.lfHeight = (int32_t)(fh * 1.4);
    ui_app.fonts.H2 = (ui_font_t)CreateFontIndirectW(&lf);
    lf.lfWeight = FW_SEMIBOLD;
    lf.lfHeight = (int32_t)(fh * 1.15);
    ui_app.fonts.H3 = (ui_font_t)CreateFontIndirectW(&lf);
    lf = ui_app_ncm.lfMessageFont;
    lf.lfPitchAndFamily = FIXED_PITCH;
    #define monospaced "Cascadia Code"
    wcscpy(lf.lfFaceName, L"Cascadia Code");
    ui_app.fonts.mono = (ui_font_t)CreateFontIndirectW(&lf);
    ui_app.cursor_arrow     = (ui_cursor_t)LoadCursorA(null, IDC_ARROW);
    ui_app.cursor_wait      = (ui_cursor_t)LoadCursorA(null, IDC_WAIT);
    ui_app.cursor_ibeam     = (ui_cursor_t)LoadCursorA(null, IDC_IBEAM);
    ui_app.cursor_size_nwse = (ui_cursor_t)LoadCursorA(null, IDC_SIZENWSE);
    ui_app.cursor_size_nesw = (ui_cursor_t)LoadCursorA(null, IDC_SIZENESW);
    ui_app.cursor_size_we   = (ui_cursor_t)LoadCursorA(null, IDC_SIZEWE);
    ui_app.cursor_size_ns   = (ui_cursor_t)LoadCursorA(null, IDC_SIZENS);
    ui_app.cursor_size_all  = (ui_cursor_t)LoadCursorA(null, IDC_SIZEALL);
    ui_app.cursor = ui_app.cursor_arrow;
}

static void ui_app_data_save(const char* name, const void* data, int32_t bytes) {
    ut_config.save(ui_app.class_name, name, data, bytes);
}

static int32_t ui_app_data_size(const char* name) {
    return ut_config.size(ui_app.class_name, name);
}

static int32_t ui_app_data_load(const char* name, void* data, int32_t bytes) {
    return ut_config.load(ui_app.class_name, name, data, bytes);
}

typedef begin_packed struct ui_app_wiw_s { // "where is window"
    // coordinates in pixels relative (0,0) top left corner
    // of primary monitor from GetWindowPlacement
    int32_t    bytes;
    ui_rect_t  placement;
    ui_rect_t  mrc;          // monitor rectangle
    ui_rect_t  work_area;    // monitor work area (mrc sans taskbar etc)
    ui_point_t min_position; // not used (-1, -1)
    ui_point_t max_position; // not used (-1, -1)
    ui_point_t max_track;    // maximum window size (spawning all monitors)
    ui_rect_t  space;        // surrounding rect x,y,w,h of all monitors
    int32_t    dpi;          // of the monitor on which window (x,y) is located
    int32_t    flags;        // WPF_SETMINPOSITION. WPF_RESTORETOMAXIMIZED
    int32_t    show;         // show command
} end_packed ui_app_wiw_t;

static BOOL CALLBACK ui_app_monitor_enum_proc(HMONITOR monitor,
        HDC unused(hdc), RECT* rc1, LPARAM that) {
    ui_app_wiw_t* wiw = (ui_app_wiw_t*)(uintptr_t)that;
    ui_rect_t* space = &wiw->space;
    MONITORINFOEX mi = { .cbSize = sizeof(MONITORINFOEX) };
    fatal_if_false(GetMonitorInfoA(monitor, (MONITORINFO*)&mi));
    // monitors can be in negative coordinate spaces and even rotated upside-down
    const int32_t min_x = ut_min(mi.rcMonitor.left, mi.rcMonitor.right);
    const int32_t min_y = ut_min(mi.rcMonitor.top,  mi.rcMonitor.bottom);
    const int32_t max_w = ut_max(mi.rcMonitor.left, mi.rcMonitor.right);
    const int32_t max_h = ut_max(mi.rcMonitor.top,  mi.rcMonitor.bottom);
    space->x = ut_min(space->x, min_x);
    space->y = ut_min(space->y, min_y);
    space->w = ut_max(space->w, max_w);
    space->h = ut_max(space->h, max_h);
    return true; // keep going
}

static void ui_app_enum_monitors(ui_app_wiw_t* wiw) {
    EnumDisplayMonitors(null, null, ui_app_monitor_enum_proc, (uintptr_t)wiw);
    // because ui_app_monitor_enum_proc() puts max into w,h:
    wiw->space.w -= wiw->space.x;
    wiw->space.h -= wiw->space.y;
}

static void ui_app_save_window_pos(ui_window_t wnd, const char* name, bool dump) {
    RECT wr = {0};
    fatal_if_false(GetWindowRect((HWND)wnd, &wr));
    ui_rect_t wrc = ui_app_rect2ui(&wr);
    ui_app_update_mi(&wrc, MONITOR_DEFAULTTONEAREST);
    WINDOWPLACEMENT wpl = { .length = sizeof(wpl) };
    fatal_if_false(GetWindowPlacement((HWND)wnd, &wpl));
    // note the replacement of wpl.rcNormalPosition with wrc:
    ui_app_wiw_t wiw = { // where is window
        .bytes = sizeof(ui_app_wiw_t),
        .placement = wrc,
        .mrc = ui_app.mrc,
        .work_area = ui_app.work_area,
        .min_position = ui_app_point2ui(&wpl.ptMinPosition),
        .max_position = ui_app_point2ui(&wpl.ptMaxPosition),
        .max_track = {
            .x = GetSystemMetrics(SM_CXMAXTRACK),
            .y = GetSystemMetrics(SM_CYMAXTRACK)
        },
        .dpi = ui_app.dpi.monitor_raw,
        .flags = wpl.flags,
        .show = wpl.showCmd
    };
    ui_app_enum_monitors(&wiw);
    if (dump) {
        traceln("wiw.space: %d,%d %dx%d",
              wiw.space.x, wiw.space.y, wiw.space.w, wiw.space.h);
        traceln("MAXTRACK: %d, %d", wiw.max_track.x, wiw.max_track.y);
        traceln("wpl.rcNormalPosition: %d,%d %dx%d",
            wpl.rcNormalPosition.left, wpl.rcNormalPosition.top,
            wpl.rcNormalPosition.right - wpl.rcNormalPosition.left,
            wpl.rcNormalPosition.bottom - wpl.rcNormalPosition.top);
        traceln("wpl.ptMinPosition: %d,%d",
            wpl.ptMinPosition.x, wpl.ptMinPosition.y);
        traceln("wpl.ptMaxPosition: %d,%d",
            wpl.ptMaxPosition.x, wpl.ptMaxPosition.y);
        traceln("wpl.showCmd: %d", wpl.showCmd);
        // WPF_SETMINPOSITION. WPF_RESTORETOMAXIMIZED WPF_ASYNCWINDOWPLACEMENT
        traceln("wpl.flags: %d", wpl.flags);
    }
//  traceln("%d,%d %dx%d show=%d", wiw.placement.x, wiw.placement.y,
//      wiw.placement.w, wiw.placement.h, wiw.show);
    ut_config.save(ui_app.class_name, name, &wiw, sizeof(wiw));
    ui_app_update_mi(&ui_app.wrc, MONITOR_DEFAULTTONEAREST);
}

static void ui_app_save_console_pos(void) {
    HWND cw = GetConsoleWindow();
    if (cw != null) {
        ui_app_save_window_pos((ui_window_t)cw, "wic", false);
        HANDLE console = GetStdHandle(STD_OUTPUT_HANDLE);
        CONSOLE_SCREEN_BUFFER_INFOEX info = { sizeof(CONSOLE_SCREEN_BUFFER_INFOEX) };
        int32_t r = GetConsoleScreenBufferInfoEx(console, &info) ? 0 : ut_runtime.err();
        if (r != 0) {
            traceln("GetConsoleScreenBufferInfoEx() %s", ut_str.error(r));
        } else {
            ut_config.save(ui_app.class_name, "console_screen_buffer_infoex",
                            &info, (int32_t)sizeof(info));
//          traceln("info: %dx%d", info.dwSize.X, info.dwSize.Y);
//          traceln("%d,%d %dx%d", info.srWindow.Left, info.srWindow.Top,
//              info.srWindow.Right - info.srWindow.Left,
//              info.srWindow.Bottom - info.srWindow.Top);
        }
    }
    int32_t v = ui_app.is_console_visible();
    // "icv" "is console visible"
    ut_config.save(ui_app.class_name, "icv", &v, (int32_t)sizeof(v));
}

static bool ui_app_is_fully_inside(const ui_rect_t* inner,
                                const ui_rect_t* outer) {
    return
        outer->x <= inner->x && inner->x + inner->w <= outer->x + outer->w &&
        outer->y <= inner->y && inner->y + inner->h <= outer->y + outer->h;
}

static void ui_app_bring_window_inside_monitor(const ui_rect_t* mrc, ui_rect_t* wrc) {
    assert(mrc->w > 0 && mrc->h > 0);
    // Check if window rect is inside monitor rect
    if (!ui_app_is_fully_inside(wrc, mrc)) {
        // Move window into monitor rect
        wrc->x = ut_max(mrc->x, ut_min(mrc->x + mrc->w - wrc->w, wrc->x));
        wrc->y = ut_max(mrc->y, ut_min(mrc->y + mrc->h - wrc->h, wrc->y));
        // Adjust size to fit into monitor rect
        wrc->w = ut_min(wrc->w, mrc->w);
        wrc->h = ut_min(wrc->h, mrc->h);
    }
}

static bool ui_app_load_window_pos(ui_rect_t* rect, int32_t *visibility) {
    ui_app_wiw_t wiw = {0}; // where is window
    bool loaded = ut_config.load(ui_app.class_name, "wiw", &wiw, sizeof(wiw)) ==
                                sizeof(wiw);
    if (loaded) {
        #ifdef QUICK_DEBUG
            traceln("wiw.placement: %d,%d %dx%d", wiw.placement.x, wiw.placement.y,
                wiw.placement.w, wiw.placement.h);
            traceln("wiw.mrc: %d,%d %dx%d", wiw.mrc.x, wiw.mrc.y, wiw.mrc.w, wiw.mrc.h);
            traceln("wiw.work_area: %d,%d %dx%d", wiw.work_area.x, wiw.work_area.y,
                                                  wiw.work_area.w, wiw.work_area.h);
            traceln("wiw.min_position: %d,%d", wiw.min_position.x, wiw.min_position.y);
            traceln("wiw.max_position: %d,%d", wiw.max_position.x, wiw.max_position.y);
            traceln("wiw.max_track: %d,%d", wiw.max_track.x, wiw.max_track.y);
            traceln("wiw.dpi: %d", wiw.dpi);
            traceln("wiw.flags: %d", wiw.flags);
            traceln("wiw.show: %d", wiw.show);
        #endif
        ui_rect_t* p = &wiw.placement;
        ui_app_update_mi(&wiw.placement, MONITOR_DEFAULTTONEAREST);
        bool same_monitor = memcmp(&wiw.mrc, &ui_app.mrc, sizeof(wiw.mrc)) == 0;
//      traceln("%d,%d %dx%d", p->x, p->y, p->w, p->h);
        if (same_monitor) {
            *rect = *p;
        } else { // moving to another monitor
            rect->x = (p->x - wiw.mrc.x) * ui_app.mrc.w / wiw.mrc.w;
            rect->y = (p->y - wiw.mrc.y) * ui_app.mrc.h / wiw.mrc.h;
            // adjust according to monitors DPI difference:
            // (w, h) theoretically could be as large as 0xFFFF
            const int64_t w = (int64_t)p->w * ui_app.dpi.monitor_raw;
            const int64_t h = (int64_t)p->h * ui_app.dpi.monitor_raw;
            rect->w = (int32_t)(w / wiw.dpi);
            rect->h = (int32_t)(h / wiw.dpi);
        }
        *visibility = wiw.show;
    }
//  traceln("%d,%d %dx%d show=%d", rect->x, rect->y, rect->w, rect->h, *visibility);
    ui_app_bring_window_inside_monitor(&ui_app.mrc, rect);
//  traceln("%d,%d %dx%d show=%d", rect->x, rect->y, rect->w, rect->h, *visibility);
    return loaded;
}

static bool ui_app_load_console_pos(ui_rect_t* rect, int32_t *visibility) {
    ui_app_wiw_t wiw = {0}; // where is window
    *visibility = 0; // boolean
    bool loaded = ut_config.load(ui_app.class_name, "wic", &wiw, sizeof(wiw)) ==
                                sizeof(wiw);
    if (loaded) {
        ui_rect_t* p = &wiw.placement;
        ui_app_update_mi(&wiw.placement, MONITOR_DEFAULTTONEAREST);
        bool same_monitor = memcmp(&wiw.mrc, &ui_app.mrc, sizeof(wiw.mrc)) == 0;
//      traceln("%d,%d %dx%d", p->x, p->y, p->w, p->h);
        if (same_monitor) {
            *rect = *p;
        } else { // moving to another monitor
            rect->x = (p->x - wiw.mrc.x) * ui_app.mrc.w / wiw.mrc.w;
            rect->y = (p->y - wiw.mrc.y) * ui_app.mrc.h / wiw.mrc.h;
            // adjust according to monitors DPI difference:
            // (w, h) theoretically could be as large as 0xFFFF
            const int64_t w = (int64_t)p->w * ui_app.dpi.monitor_raw;
            const int64_t h = (int64_t)p->h * ui_app.dpi.monitor_raw;
            rect->w = (int32_t)(w / wiw.dpi);
            rect->h = (int32_t)(h / wiw.dpi);
        }
        *visibility = wiw.show != 0;
        ui_app_update_mi(&ui_app.wrc, MONITOR_DEFAULTTONEAREST);
    }
    return loaded;
}

static void ui_app_timer_kill(ui_timer_t timer) {
    fatal_if_false(KillTimer(ui_app_window(), timer));
}

static ui_timer_t ui_app_timer_set(uintptr_t id, int32_t ms) {
    not_null(ui_app_window());
    assert(10 <= ms && ms < 0x7FFFFFFF);
    ui_timer_t tid = (ui_timer_t)SetTimer(ui_app_window(), id, (uint32_t)ms, null);
    fatal_if(tid == 0);
    assert(tid == id);
    return tid;
}

static void ui_app_post_message(int32_t m, int64_t wp, int64_t lp) {
    fatal_if_false(PostMessageA(ui_app_window(), m, wp, lp));
}

static void ui_app_timer(ui_view_t* view, ui_timer_t id) {
    if (view->timer != null) { ui_view.timer(view, id); }
    if (id == ui_app_timer_1s_id) { ui_view.every_sec(view); }
    if (id == ui_app_timer_100ms_id) { ui_view.every_100ms(view); }
}

static void ui_app_animate_timer(void) {
    ui_app_post_message(ui.message.animate, (uint64_t)ui_app_animate.step + 1,
        (uintptr_t)ui_app_animate.f);
}

static void ui_app_wm_timer(ui_timer_t id) {
    if (ui_app.animating.time != 0 && ui_app.now > ui_app.animating.time) {
        ui_app.show_toast(null, 0);
    }
    if (ui_app_animate.timer == id) { ui_app_animate_timer(); }
    ui_app_timer(ui_app.view, id);
}

static void ui_app_window_dpi(void) {
    int32_t dpi = GetDpiForWindow(ui_app_window());
    if (dpi == 0) { dpi = GetDpiForWindow(GetParent(ui_app_window())); }
    if (dpi == 0) { dpi = GetDpiForWindow(GetDesktopWindow()); }
    if (dpi == 0) { dpi = GetSystemDpiForProcess(GetCurrentProcess()); }
    if (dpi == 0) { dpi = GetDpiForSystem(); }
    ui_app.dpi.window = dpi;
}

static void ui_app_window_opening(void) {
    ui_app_window_dpi();
    ui_app_init_fonts(ui_app.dpi.window);
    ui_app.view->em = ui_gdi.get_em(*ui_app.view->font);
    ui_app_timer_1s_id = ui_app.set_timer((uintptr_t)&ui_app_timer_1s_id, 1000);
    ui_app_timer_100ms_id = ui_app.set_timer((uintptr_t)&ui_app_timer_100ms_id, 100);
    ui_app.set_cursor(ui_app.cursor_arrow);
    ui_app.canvas = (ui_canvas_t)GetDC(ui_app_window());
    not_null(ui_app.canvas);
    if (ui_app.opened != null) { ui_app.opened(); }
    strprintf(ui_app.view->text, "ui_app.view"); // debugging
    ui_app_wm_timer(ui_app_timer_100ms_id);
    ui_app_wm_timer(ui_app_timer_1s_id);
    fatal_if(ReleaseDC(ui_app_window(), ui_app_canvas()) == 0);
    ui_app.canvas = null;
    ui_app.layout(); // request layout
    if (ui_app.last_visibility == ui.visibility.maximize) {
        ShowWindow(ui_app_window(), ui.visibility.maximize);
    }
//  ui_app_dump_dpi();
//  if (forced_locale != 0) {
//      SendMessageTimeoutA(HWND_BROADCAST, WM_SETTINGCHANGE, 0, (uintptr_t)"intl", 0, 1000, null);
//  }
}

static void ui_app_window_closing(void) {
    if (ui_app.can_close == null || ui_app.can_close()) {
        if (ui_app.is_full_screen) { ui_app.full_screen(false); }
        ui_app.kill_timer(ui_app_timer_1s_id);
        ui_app.kill_timer(ui_app_timer_100ms_id);
        ui_app_timer_1s_id = 0;
        ui_app_timer_100ms_id = 0;
        if (ui_app.closed != null) { ui_app.closed(); }
        ui_app_save_window_pos(ui_app.window, "wiw", false);
        ui_app_save_console_pos();
        DestroyWindow(ui_app_window());
        ui_app.window = null;
    }
}

static void ui_app_allow_dark_mode_for_app(void) {
    // https://github.com/rizonesoft/Notepad3/tree/96a48bd829a3f3192bbc93cd6944cafb3228b96d/src/DarkMode
    HMODULE uxtheme = GetModuleHandleA("uxtheme.dll");
    not_null(uxtheme);
    typedef BOOL (__stdcall *AllowDarkModeForApp_t)(bool allow);
    AllowDarkModeForApp_t AllowDarkModeForApp = (AllowDarkModeForApp_t)
            (void*)GetProcAddress(uxtheme, MAKEINTRESOURCE(132));
    if (AllowDarkModeForApp != null) {
        int r = b2e(AllowDarkModeForApp(true));
        if (r != 0 && r != ERROR_PROC_NOT_FOUND) {
            traceln("AllowDarkModeForApp(true) failed %s", ut_str.error(r));
        }
    }
    enum { Default = 0, AllowDark = 1, ForceDark = 2, ForceLight = 3 };
    typedef BOOL (__stdcall *SetPreferredAppMode_t)(bool allow);
    SetPreferredAppMode_t SetPreferredAppMode = (SetPreferredAppMode_t)
            (void*)GetProcAddress(uxtheme, MAKEINTRESOURCE(135));
    if (SetPreferredAppMode != null) {
        int r = b2e(SetPreferredAppMode(AllowDark));
        // 1814 ERROR_RESOURCE_NAME_NOT_FOUND
        if (r != 0 && r != ERROR_PROC_NOT_FOUND) {
            traceln("SetPreferredAppMode(AllowDark) failed %s",
                    ut_str.error(r));
        }
    }
}

static void ui_app_allow_dark_mode_for_window(void) {
    HMODULE uxtheme = GetModuleHandleA("uxtheme.dll");
    not_null(uxtheme);
    typedef BOOL (__stdcall *AllowDarkModeForWindow_t)(HWND hWnd, bool allow);
    AllowDarkModeForWindow_t AllowDarkModeForWindow = (AllowDarkModeForWindow_t)
        (void*)GetProcAddress(uxtheme, MAKEINTRESOURCE(133));
    if (AllowDarkModeForWindow != null) {
        int r = b2e(AllowDarkModeForWindow(ui_app_window(), true));
        if (r != 0 && r != ERROR_PROC_NOT_FOUND) {
            traceln("AllowDarkModeForWindow(true) failed %s", ut_str.error(r));
        }
    }
}

static void ui_app_get_min_max_info(MINMAXINFO* mmi) {
    const ui_window_sizing_t* ws = &ui_app.window_sizing;
    const ui_rect_t* wa = &ui_app.work_area;
    const int32_t min_w = ws->min_w > 0 ? ui_app.in2px(ws->min_w) : ui_app.in2px(1.0);
    const int32_t min_h = ws->min_h > 0 ? ui_app.in2px(ws->min_h) : ui_app.in2px(0.5);
    mmi->ptMinTrackSize.x = min_w;
    mmi->ptMinTrackSize.y = min_h;
    const int32_t max_w = ws->max_w > 0 ? ui_app.in2px(ws->max_w) : wa->w;
    const int32_t max_h = ws->max_h > 0 ? ui_app.in2px(ws->max_h) : wa->h;
    if (ui_app.no_clip) {
        mmi->ptMaxTrackSize.x = max_w;
        mmi->ptMaxTrackSize.y = max_h;
    } else {
        // clip max_w and max_h to monitor work area
        mmi->ptMaxTrackSize.x = ut_min(max_w, wa->w);
        mmi->ptMaxTrackSize.y = ut_min(max_h, wa->h);
    }
    mmi->ptMaxSize.x = mmi->ptMaxTrackSize.x;
    mmi->ptMaxSize.y = mmi->ptMaxTrackSize.y;
}

static void ui_app_paint(ui_view_t* view) {
    assert(ui_app_window() != null);
    // crc = {0,0} on minimized windows but paint is still called
    if (ui_app.crc.w > 0 && ui_app.crc.h > 0) { ui_view.paint(view); }
}

static void ui_app_measure_and_layout(ui_view_t* view) {
    assert(ui_app.crc.w > 0 && ui_app.crc.h > 0 && ui_app_window() != null);
    ui_view.measure_children(view);
    ui_view.layout_children(view);
}

static void ui_app_toast_mouse(int32_t m, int64_t f);
static void ui_app_toast_character(const char* utf8);

static void ui_app_wm_char(ui_view_t* view, const char* utf8) {
    if (ui_app.animating.view != null) {
        ui_app_toast_character(utf8);
    } else {
        ui_view.character(view, utf8);
    }
}

static void ui_app_mouse(ui_view_t* view, int32_t m, int64_t f) {
    if (ui_app.animating.view != null && ui_app.animating.view->mouse != null) {
        ui_view.mouse(ui_app.animating.view, m, f);
    } else if (ui_app.animating.view != null && ui_app.animating.view->mouse == null) {
        ui_app_toast_mouse(m, f);
        bool tooltip = ui_app.animating.x >= 0 && ui_app.animating.y >= 0;
        if (tooltip) { ui_view.mouse(view, m, f); }
    } else {
        ui_view.mouse(view, m, f);
    }
}

static void ui_app_tap_press(int32_t m, int64_t wp, int64_t lp) {
    ui_app.mouse.x = GET_X_LPARAM(lp);
    ui_app.mouse.y = GET_Y_LPARAM(lp);
    // dispatch as generic mouse message:
    ui_app_mouse(ui_app.view, (int32_t)m, wp);
    int32_t ix = (int32_t)wp;
    assert(0 <= ix && ix <= 2);
    // for now long press and fp64_t tap/fp64_t click
    // treated as press() call - can be separated if desired:
    if (m == ui.message.tap) {
        ui_view.tap(ui_app.view, ix);
    } else if (m == ui.message.dtap) {
        ui_view.press(ui_app.view, ix);
    } else if (m == ui.message.press) {
        ui_view.press(ui_app.view, ix);
    } else {
        assert(false, "unexpected message: 0x%04X", m);
    }
}

enum { ui_app_animation_steps = 15 };

static void ui_app_toast_paint(void) {
    static ui_image_t image;
    if (image.bitmap == null) {
        uint8_t pixels[4] = { 0x3F, 0x3F, 0x3F };
        ui_gdi.image_init(&image, 1, 1, 3, pixels);
    }
    if (ui_app.animating.view != null) {
        ui_view.measure_children(ui_app.animating.view);
        ui_gdi.push(0, 0);
        bool tooltip = ui_app.animating.x >= 0 && ui_app.animating.y >= 0;
        const int32_t em_x = ui_app.animating.view->em.x;
        const int32_t em_y = ui_app.animating.view->em.y;
        ui_gdi.set_brush(ui_gdi.brush_color);
        ui_gdi.set_brush_color(ui_colors.toast);
        if (!tooltip) {
            assert(0 <= ui_app.animating.step && ui_app.animating.step < ui_app_animation_steps);
            int32_t step = ui_app.animating.step - (ui_app_animation_steps - 1);
            ui_app.animating.view->y = ui_app.animating.view->h * step / (ui_app_animation_steps - 1);
//          traceln("step=%d of %d y=%d", ui_app.animating.step,
//                  ui_app_toast_steps, ui_app.animating.view->y);
            ui_app_measure_and_layout(ui_app.animating.view);
            fp64_t alpha = ut_min(0.40, 0.40 * ui_app.animating.step / (fp64_t)ui_app_animation_steps);
            ui_gdi.alpha_blend(0, 0, ui_app.width, ui_app.height, &image, alpha);
            ui_app.animating.view->x = (ui_app.width - ui_app.animating.view->w) / 2;
        } else {
            ui_app.animating.view->x = ui_app.animating.x;
            ui_app.animating.view->y = ui_app.animating.y;
            ui_app_measure_and_layout(ui_app.animating.view);
            int32_t mx = ui_app.width - ui_app.animating.view->w - em_x;
            ui_app.animating.view->x = ut_min(mx, ut_max(0, ui_app.animating.x - ui_app.animating.view->w / 2));
            ui_app.animating.view->y = ut_min(ui_app.crc.h - em_y, ut_max(0, ui_app.animating.y));
        }
        int32_t x = ui_app.animating.view->x - em_x;
        int32_t y = ui_app.animating.view->y - em_y / 2;
        int32_t w = ui_app.animating.view->w + em_x * 2;
        int32_t h = ui_app.animating.view->h + em_y;
        ui_gdi.rounded(x, y, w, h, em_x, em_y);
        if (!tooltip) { ui_app.animating.view->y += em_y / 4; }
        ui_app_paint(ui_app.animating.view);
        if (!tooltip) {
            if (ui_app.animating.view->y == em_y / 4) {
                // micro "close" toast button:
                ui_gdi.x = ui_app.animating.view->x + ui_app.animating.view->w;
                ui_gdi.y = 0;
                ui_gdi.text("%s", ui_glyph_multiplication_sign);
            }
        }
        ui_gdi.pop();
    }
}

static void ui_app_toast_cancel(void) {
    if (ui_app.animating.view != null && ui_app.animating.view->type == ui_view_mbx) {
        ui_mbx_t* mx = (ui_mbx_t*)ui_app.animating.view;
        if (mx->option < 0 && mx->view.callback != null) {
            mx->view.callback(&mx->view);
        }
    }
    ui_app.animating.step = 0;
    ui_app.animating.view = null;
    ui_app.animating.time = 0;
    ui_app.animating.x = -1;
    ui_app.animating.y = -1;
    ui_app.redraw();
}

static void ui_app_toast_mouse(int32_t m, int64_t flags) {
    bool pressed = m == ui.message.left_button_pressed ||
                   m == ui.message.right_button_pressed;
    if (ui_app.animating.view != null && pressed) {
        const ui_point_t em = ui_app.animating.view->em;
        int32_t x = ui_app.animating.view->x + ui_app.animating.view->w;
        if (x <= ui_app.mouse.x && ui_app.mouse.x <= x + em.x &&
            0 <= ui_app.mouse.y && ui_app.mouse.y <= em.y) {
            ui_app_toast_cancel();
        } else {
            ui_view.mouse(ui_app.animating.view, m, flags);
        }
    } else {
        ui_view.mouse(ui_app.animating.view, m, flags);
    }
}

static void ui_app_toast_character(const char* utf8) {
    char ch = utf8[0];
    if (ui_app.animating.view != null && ch == 033) { // ESC traditionally in octal
        ui_app_toast_cancel();
        ui_app.show_toast(null, 0);
    } else {
        ui_view.character(ui_app.animating.view, utf8);
    }
}

static void ui_app_toast_dim(int32_t step) {
    ui_app.animating.step = step;
    ui_app.redraw();
    UpdateWindow(ui_app_window());
}

static void ui_app_animate_step(ui_app_animate_function_t f, int32_t step, int32_t steps) {
    // calls function(0..step-1) exactly step times
    bool cancel = false;
    if (f != null && f != ui_app_animate.f && step == 0 && steps > 0) {
        // start animation
        ui_app_animate.count = steps;
        ui_app_animate.f = f;
        f(step);
        ui_app_animate.timer = ui_app.set_timer((uintptr_t)&ui_app_animate.timer, 10);
    } else if (f != null && ui_app_animate.f == f && step > 0) {
        cancel = step >= ui_app_animate.count;
        if (!cancel) {
            ui_app_animate.step = step;
            f(step);
        }
    } else if (f == null) {
        cancel = true;
    }
    ui_app.focus = null;
    if (cancel) {
        if (ui_app_animate.timer != 0) { ui_app.kill_timer(ui_app_animate.timer); }
        ui_app_animate.step = 0;
        ui_app_animate.timer = 0;
        ui_app_animate.f = null;
        ui_app_animate.count = 0;
    }
}

static void ui_app_animate_start(ui_app_animate_function_t f, int32_t steps) {
    // calls f(0..step-1) exactly steps times, unless cancelled with call
    // animate(null, 0) or animate(other_function, n > 0)
    ui_app_animate_step(f, 0, steps);
}

static void ui_app_view_paint(ui_view_t* v) {
    assert(v == ui_app.view && v->x == 0 && v->y == 0);
    if (!ui_color_is_transparent(v->color)) {
        ui_gdi.fill_with(v->x, v->y, v->w, v->h, v->color);
    }
}

static void ui_app_view_layout(void) {
    not_null(ui_app.window);
    not_null(ui_app.canvas);
    ui_app.view->w = ui_app.crc.w; // crc is window client rectangle
    ui_app.view->h = ui_app.crc.h;
    ui_app_measure_and_layout(ui_app.view);
}

static void ui_app_view_active_frame_paint(void) {
#if 0
    ui_color_t c = ui_app.is_active() ?
        ui_app.get_color(ui.colors.highlight) : // ui_colors.btn_hover_highlight
        ui_app.get_color(ui.colors.active_title);
#else
    ui_color_t c = ui_app.is_active() ?
        ui_colors.dkgray4 : ui_caption.view.color;
#endif
    ui_gdi.frame_with(0, 0, ui_app.view->w - 0, ui_app.view->h - 0, c);
}

static void ui_app_paint_on_canvas(HDC hdc) {
    ui_canvas_t canvas = ui_app.canvas;
    ui_app.canvas = (ui_canvas_t)hdc;
    ui_gdi.push(0, 0);
    fp64_t time = ut_clock.seconds();
    ui_gdi.x = 0;
    ui_gdi.y = 0;
    ui_app_update_crc();
    if (ui_app_layout_dirty) {
        ui_app_layout_dirty = false;
        ui_app_view_layout();
    }
    ui_font_t font = ui_gdi.set_font(ui_app.fonts.regular);
    ui_color_t c = ui_gdi.set_text_color(ui_colors.text);
    int32_t bm = SetBkMode(ui_app_canvas(), TRANSPARENT);
    int32_t stretch_mode = SetStretchBltMode(ui_app_canvas(), HALFTONE);
    ui_point_t pt = {0};
    fatal_if_false(SetBrushOrgEx(ui_app_canvas(), 0, 0, (POINT*)&pt));
    ui_brush_t br = ui_gdi.set_brush(ui_gdi.brush_hollow);
    ui_app_paint(ui_app.view);
    if (ui_app.animating.view != null) { ui_app_toast_paint(); }
    fatal_if_false(SetBrushOrgEx(ui_app_canvas(), pt.x, pt.y, null));
    SetStretchBltMode(ui_app_canvas(), stretch_mode);
    SetBkMode(ui_app_canvas(), bm);
    ui_gdi.set_brush(br);
    ui_gdi.set_text_color(c);
    ui_gdi.set_font(font);
    ui_app.paint_count++;
    if (ui_app.paint_count % 128 == 0) { ui_app.paint_max = 0; }
    ui_app.paint_time = ut_clock.seconds() - time;
    ui_app.paint_max = ut_max(ui_app.paint_time, ui_app.paint_max);
    if (ui_app.paint_avg == 0) {
        ui_app.paint_avg = ui_app.paint_time;
    } else { // EMA over 32 paint() calls
        ui_app.paint_avg = ui_app.paint_avg * (1.0 - 1.0 / 32.0) +
                        ui_app.paint_time / 32.0;
    }
    if (ui_app.no_decor && !ui_app.is_full_screen && !ui_app.is_maximized()) {
        ui_app_view_active_frame_paint();
    }
    ui_gdi.pop();
    ui_app.canvas = canvas;
}

static void ui_app_wm_paint(void) {
    // it is possible to receive WM_PAINT when window is not closed
    if (ui_app.window != null) {
        PAINTSTRUCT ps = {0};
        BeginPaint(ui_app_window(), &ps);
        ui_app_paint_on_canvas(ps.hdc);
        EndPaint(ui_app_window(), &ps);
    }
}

// about (x,y) being (-32000,-32000) see:
// https://chromium.googlesource.com/chromium/src.git/+/62.0.3178.1/ui/views/win/hwnd_message_handler.cc#1847

static void ui_app_window_position_changed(const WINDOWPOS* wp) {
    ui_app.view->hidden = !IsWindowVisible(ui_app_window());
    const bool moved  = (wp->flags & SWP_NOMOVE) == 0;
    const bool sized  = (wp->flags & SWP_NOSIZE) == 0;
    const bool hiding = (wp->flags & SWP_HIDEWINDOW) != 0 ||
                        wp->x == -32000 && wp->y == -32000;
    HMONITOR monitor = MonitorFromWindow(ui_app_window(), MONITOR_DEFAULTTONULL);
    if (!ui_app.view->hidden && (moved || sized) && !hiding && monitor != null) {
        RECT wrc = ui_app_ui2rect(&ui_app.wrc);
        fatal_if_false(GetWindowRect(ui_app_window(), &wrc));
        ui_app.wrc = ui_app_rect2ui(&wrc);
        ui_app_update_mi(&ui_app.wrc, MONITOR_DEFAULTTONEAREST);
        ui_app_update_crc();
        if (ui_app_timer_1s_id != 0) { ui_app.layout(); }
    }
}

static void ui_app_setting_change(uintptr_t wp, uintptr_t lp) {
    // wp: SPI_SETWORKAREA ... SPI_SETDOCKMOVING
    //     SPI_GETACTIVEWINDOWTRACKING ... SPI_SETGESTUREVISUALIZATION
    if (lp != 0 && strcmp((const char*)lp, "ImmersiveColorSet") == 0 ||
        wcscmp((const wchar_t*)lp, L"ImmersiveColorSet") == 0) {
        // expected:
        // SPI_SETICONTITLELOGFONT 0x22 ?
        // SPI_SETNONCLIENTMETRICS 0x2A ?
        traceln("wp: 0x%08X", wp);
        // actual wp == 0x0000
        ui_theme.refresh(ui_app.window);
    }
    if (wp == 0 && lp != 0 && strcmp((const char*)lp, "intl") == 0) {
        traceln("wp: 0x%04X", wp); // SPI_SETLOCALEINFO 0x24 ?
        wchar_t ln[LOCALE_NAME_MAX_LENGTH + 1];
        int32_t n = GetUserDefaultLocaleName(ln, countof(ln));
        fatal_if_false(n > 0);
        wchar_t rln[LOCALE_NAME_MAX_LENGTH + 1];
        n = ResolveLocaleName(ln, rln, countof(rln));
        fatal_if_false(n > 0);
        LCID lcid = LocaleNameToLCID(rln, LOCALE_ALLOW_NEUTRAL_NAMES);
        fatal_if_false(SetThreadLocale(lcid));
    }
}

static void ui_app_show_task_bar(bool show) {
    HWND taskbar = FindWindowA("Shell_TrayWnd", null);
    if (taskbar != null) {
        ShowWindow(taskbar, show ? SW_SHOW : SW_HIDE);
        UpdateWindow(taskbar);
    }
}

static void ui_app_click_detector(uint32_t msg, WPARAM wp, LPARAM lp) {
    // TODO: click detector does not handle WM_NCLBUTTONDOWN, ...
    //       it can be modified to do so if needed
    #pragma push_macro("set_timer")
    #pragma push_macro("kill_timer")
    #pragma push_macro("done")

    #define set_timer(t, ms) do {                   \
        assert(t == 0);                             \
        t = ui_app_timer_set((uintptr_t)&t, ms);       \
    } while (0)

    #define kill_timer(t) do {                      \
        if (t != 0) { ui_app_timer_kill(t); t = 0; }   \
    } while (0)

    #define done(ix) do {                           \
        clicked[ix] = 0;                            \
        pressed[ix] = false;                        \
        click_at[ix] = (ui_point_t){0, 0};          \
        kill_timer(timer_p[ix]);                    \
        kill_timer(timer_d[ix]);                    \
    } while (0)

    // This function should work regardless to CS_BLKCLK being present
    // 0: Left, 1: Middle, 2: Right
    static ui_point_t click_at[3];
    static fp64_t     clicked[3]; // click time
    static bool       pressed[3];
    static ui_timer_t       timer_d[3]; // fp64_t tap
    static ui_timer_t       timer_p[3]; // long press
    bool up = false;
    int32_t ix = -1;
    uint32_t m = 0;
    switch (msg) {
        case WM_LBUTTONDOWN  : ix = 0; m = ui.message.tap;  break;
        case WM_MBUTTONDOWN  : ix = 1; m = ui.message.tap;  break;
        case WM_RBUTTONDOWN  : ix = 2; m = ui.message.tap;  break;
        case WM_LBUTTONDBLCLK: ix = 0; m = ui.message.dtap; break;
        case WM_MBUTTONDBLCLK: ix = 1; m = ui.message.dtap; break;
        case WM_RBUTTONDBLCLK: ix = 2; m = ui.message.dtap; break;
        case WM_LBUTTONUP    : ix = 0; up = true;   break;
        case WM_MBUTTONUP    : ix = 1; up = true;   break;
        case WM_RBUTTONUP    : ix = 2; up = true;   break;
    }
    if (msg == WM_TIMER) { // long press && dtap
        for (int i = 0; i < 3; i++) {
            if (wp == timer_p[i]) {
                lp = MAKELONG(click_at[i].x, click_at[i].y);
                ui_app_post_message(ui.message.press, i, lp);
                done(i);
            }
            if (wp == timer_d[i]) {
                lp = MAKELONG(click_at[i].x, click_at[i].y);
                ui_app_post_message(ui.message.tap, i, lp);
                done(i);
            }
        }
    }
    if (ix != -1) {
        const uint32_t dtap_msec = GetDoubleClickTime();
        const fp64_t double_click_dt = dtap_msec / 1000.0;
        const int double_click_x = GetSystemMetrics(SM_CXDOUBLECLK) / 2;
        const int double_click_y = GetSystemMetrics(SM_CYDOUBLECLK) / 2;
        ui_point_t pt = { GET_X_LPARAM(lp), GET_Y_LPARAM(lp) };
        if ((int32_t)m == ui.message.tap) {
            if (ui_app.now  - clicked[ix]  <= double_click_dt &&
                abs(pt.x - click_at[ix].x) <= double_click_x &&
                abs(pt.y - click_at[ix].y) <= double_click_y) {
                ui_app_post_message(ui.message.dtap, ix, lp);
                done(ix);
            } else {
                done(ix); // clear timers
                clicked[ix]  = ui_app.now;
                click_at[ix] = pt;
                pressed[ix]  = true;
                set_timer(timer_p[ix], ui_long_press_msec); // 0.25s
                set_timer(timer_d[ix], dtap_msec); // 0.5s
            }
        } else if (up) {
//          traceln("pressed[%d]: %d %.3f", ix, pressed[ix], ui_app.now - clicked[ix]);
            if (pressed[ix] && ui_app.now - clicked[ix] > double_click_dt) {
                ui_app_post_message(ui.message.dtap, ix, lp);
                done(ix);
            }
            kill_timer(timer_p[ix]); // long press is no the case
        } else if ((int32_t)m == ui.message.dtap) {
            ui_app_post_message(ui.message.dtap, ix, lp);
            done(ix);
        }
    }
    #pragma pop_macro("done")
    #pragma pop_macro("kill_timer")
    #pragma pop_macro("set_timer")
}

static int64_t ui_app_hit_test(int32_t x, int32_t y) {
    assert(!ui_caption.view.hidden);
    int32_t bt = ut_max(4, ui_app.in2px(1.0 / 16.0));
    int32_t cx = x - ui_app.wrc.x;
    int32_t cy = y - ui_app.wrc.y;
    if (ui_app.animating.view != null) {
        return ui.hit_test.client; // message box or toast is up
    } else if (ui_app.is_maximized()) {
        return ui_caption.view.hit_test(cx, cy);
    } else if (ui_app.is_full_screen) {
        return ui.hit_test.client;
    } else if (cx < bt && cy < bt) {
        return ui.hit_test.top_left;
    } else if (cx > ui_app.crc.w - bt && cy < bt) {
        return ui.hit_test.top_right;
    } else if (cy < bt) {
        return ui.hit_test.top;
    } else if (cy < ui_caption.view.h) {
        return ui_caption.view.hit_test(cx, cy);
    } else if (cx > ui_app.crc.w - bt && cy > ui_app.crc.h - bt) {
        return ui.hit_test.bottom_right;
    } else if (cx < bt && cy > ui_app.crc.h - bt) {
        return ui.hit_test.bottom_left;
    } else if (cx < bt) {
        return ui.hit_test.left;
    } else if (cx > ui_app.crc.w - bt) {
        return ui.hit_test.right;
    } else if (cy > ui_app.crc.h - bt) {
        return ui.hit_test.bottom;
    } else {
        return ui.hit_test.client;
    }
}

static void ui_app_wm_activate(int64_t wp) {
    bool activate = LOWORD(wp) != WA_INACTIVE;
    if (!IsWindowVisible(ui_app_window()) && activate) {
        ui_app.show_window(ui.visibility.restore);
        SwitchToThisWindow(ui_app_window(), true);
    }
    ui_app.redraw(); // needed for windows changing active frame color
}

static LRESULT CALLBACK ui_app_window_proc(HWND window, UINT message,
        WPARAM w_param, LPARAM l_param) {
    ui_app.now = ut_clock.seconds();
    if (ui_app.window == null) {
        ui_app.window = (ui_window_t)window;
    } else {
        assert(ui_app_window() == window);
    }
    const int32_t m  = (int32_t)message;
    const int64_t wp = (int64_t)w_param;
    const int64_t lp = (int64_t)l_param;
    int64_t ret = 0;
    ui_view.kill_hidden_focus(ui_app.view);
    ui_app_click_detector(m, wp, lp);
    if (ui_view.message(ui_app.view, m, wp, lp, &ret)) {
        return (LRESULT)ret;
    }
    if (m == ui.message.opening) { ui_app_window_opening(); return 0; }
    if (m == ui.message.closing) { ui_app_window_closing(); return 0; }
    if (m == ui.message.tap || m == ui.message.dtap ||
        m == ui.message.press) {
            ui_app_tap_press(m, wp, lp);
            return 0;
    }
    if (m == ui.message.animate) {
        ui_app_animate_step((ui_app_animate_function_t)lp, (int32_t)wp, -1);
        return 0;
    }
    switch (m) {
        case WM_GETMINMAXINFO: ui_app_get_min_max_info((MINMAXINFO*)lp); break;
        case WM_THEMECHANGED : break;
        case WM_SETTINGCHANGE: ui_app_setting_change(wp, lp); break;
        case WM_CLOSE        : ui_app.focus = null; // before WM_CLOSING
                               ui_app_post_message(ui.message.closing, 0, 0); return 0;
        case WM_DESTROY      : PostQuitMessage(ui_app.exit_code); break;
        case WM_NCHITTEST    :
            if (ui_app.no_decor && !ui_app.no_size && !ui_caption.view.hidden) {
                return ui_app.hit_test(GET_X_LPARAM(lp), GET_Y_LPARAM(lp));
            } else {
                break;
            }
        case WM_SYSKEYDOWN: // for ALT (aka VK_MENU)
        case WM_KEYDOWN      : ui_app_alt_ctrl_shift(true, wp);
                               ui_view.key_pressed(ui_app.view, wp);
                               break;
        case WM_SYSKEYUP:
        case WM_KEYUP        : ui_app_alt_ctrl_shift(false, wp);
                               ui_view.key_released(ui_app.view, wp);
                               break;
        case WM_TIMER        : ui_app_wm_timer((ui_timer_t)wp);
                               break;
        case WM_ERASEBKGND   : return true; // no DefWindowProc()
        case WM_SETCURSOR    : // TODO: investigate more in regards to wait cursor
            if (LOWORD(lp) == HTCLIENT) { // see WM_NCHITTEST
                SetCursor((HCURSOR)ui_app.cursor);
                return true; // must NOT call DefWindowProc()
            }
            break;
        // see: https://learn.microsoft.com/en-us/windows/win32/inputdev/about-keyboard-input
        // https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-tounicode
//      case WM_UNICHAR      : // only UTF-32 via PostMessage
        case WM_CHAR         : ui_app_wm_char(ui_app.view, (const char*)&wp);
                               break; // TODO: CreateWindowW() and utf16->utf8
        case WM_PRINTCLIENT  : ui_app_paint_on_canvas((HDC)wp); break;
        case WM_SETFOCUS     :
            if (!ui_app.view->hidden) {
                assert(GetActiveWindow() == ui_app_window());
                ui_view.set_focus(ui_app.view);
            }
            break;
        case WM_KILLFOCUS    : if (!ui_app.view->hidden) { ui_view.kill_focus(ui_app.view); }
                               break;
        case WM_NCCALCSIZE:
//          NCCALCSIZE_PARAMS* szp = (NCCALCSIZE_PARAMS*)lp;
//          traceln("WM_NCCALCSIZE wp: %lld is_max: %d (%d %d %d %d) (%d %d %d %d) (%d %d %d %d)",
//              wp, ui_app.is_maximized(),
//              szp->rgrc[0].left, szp->rgrc[0].top, szp->rgrc[0].right, szp->rgrc[0].bottom,
//              szp->rgrc[1].left, szp->rgrc[1].top, szp->rgrc[1].right, szp->rgrc[1].bottom,
//              szp->rgrc[2].left, szp->rgrc[2].top, szp->rgrc[2].right, szp->rgrc[2].bottom);
            // adjust window client area frame for no_decor windows
            if (wp == true && ui_app.no_decor && !ui_app.is_maximized()) {
                return 0;
            }
            break;
        case WM_PAINT        : ui_app_wm_paint(); break;
        case WM_CONTEXTMENU  : (void)ui_view.context_menu(ui_app.view); break;
        case WM_MOUSEWHEEL   :
            ui_view.mouse_wheel(ui_app.view, 0, GET_WHEEL_DELTA_WPARAM(wp)); break;
        case WM_MOUSEHWHEEL  :
            ui_view.mouse_wheel(ui_app.view, GET_WHEEL_DELTA_WPARAM(wp), 0); break;
        case WM_NCMOUSEMOVE    :
        case WM_NCLBUTTONDOWN  :
        case WM_NCLBUTTONUP    :
        case WM_NCLBUTTONDBLCLK:
        case WM_NCRBUTTONDOWN  :
        case WM_NCRBUTTONUP    :
        case WM_NCRBUTTONDBLCLK:
        case WM_NCMBUTTONDOWN  :
        case WM_NCMBUTTONUP    :
        case WM_NCMBUTTONDBLCLK: {
            POINT pt = {GET_X_LPARAM(lp), GET_Y_LPARAM(lp)};
//          traceln("%d %d", pt.x, pt.y);
            ScreenToClient(ui_app_window(), &pt);
            ui_app.mouse = ui_app_point2ui(&pt);
            ui_app_mouse(ui_app.view, m, wp);
            break;
        }
        case WM_MOUSEHOVER   : // see TrackMouseEvent()
        case WM_MOUSEMOVE    :
        case WM_LBUTTONDOWN  :
        case WM_LBUTTONUP    :
        case WM_LBUTTONDBLCLK:
        case WM_RBUTTONDOWN  :
        case WM_RBUTTONUP    :
        case WM_RBUTTONDBLCLK:
        case WM_MBUTTONDOWN  :
        case WM_MBUTTONUP    :
        case WM_MBUTTONDBLCLK: {
            ui_app.mouse.x = GET_X_LPARAM(lp);
            ui_app.mouse.y = GET_Y_LPARAM(lp);
//          traceln("%d %d", ui_app.mouse.x, ui_app.mouse.y);
            // note: ScreenToClient() is not needed for this messages
            ui_app_mouse(ui_app.view, m, wp);
            break;
        }
        case WM_GETDPISCALEDSIZE: { // sent before WM_DPICHANGED
//          traceln("WM_GETDPISCALEDSIZE");
            #ifdef QUICK_DEBUG
                int32_t dpi = wp;
                SIZE* sz = (SIZE*)lp; // in/out
                ui_point_t cell = { sz->cx, sz->cy };
                traceln("WM_GETDPISCALEDSIZE dpi %d := %d "
                    "size %d,%d *may/must* be adjusted",
                    ui_app.dpi.window, dpi, cell.x, cell.y);
            #endif
            if (ui_app_timer_1s_id != 0 && !ui_app.view->hidden) { ui_app.layout(); }
            // IMPORTANT: return true because otherwise linear, see
            // https://learn.microsoft.com/en-us/windows/win32/hidpi/wm-getdpiscaledsize
            return true;
        }
        case WM_DPICHANGED: {
//          traceln("WM_DPICHANGED");
            ui_app_window_dpi();
            ui_app_init_fonts(ui_app.dpi.window);
            if (ui_app_timer_1s_id != 0 && !ui_app.view->hidden) {
                ui_app.layout();
            } else {
                ui_app_layout_dirty = true;
            }
            break;
        }
        case WM_SYSCOMMAND:
            if (wp == SC_MINIMIZE && ui_app.hide_on_minimize) {
                ui_app.show_window(ui.visibility.min_na);
                ui_app.show_window(ui.visibility.hide);
            } else  if (wp == SC_MINIMIZE && ui_app.no_decor) {
                ui_app.show_window(ui.visibility.min_na);
            }
            // If the selection is in menu handle the key event
            if (wp == SC_KEYMENU && lp != 0x20) {
                return 0; // This prevents the error/beep sound
            }
            break;
        case WM_ACTIVATE: ui_app_wm_activate(wp); break;
        case WM_WINDOWPOSCHANGING: {
            #ifdef QUICK_DEBUG
                WINDOWPOS* pos = (WINDOWPOS*)lp;
//              traceln("WM_WINDOWPOSCHANGING flags: 0x%08X", pos->flags);
                if (pos->flags & SWP_SHOWWINDOW) {
//                  traceln("SWP_SHOWWINDOW");
                } else if (pos->flags & SWP_HIDEWINDOW) {
//                  traceln("SWP_HIDEWINDOW");
                }
            #endif
            break;
        }
        case WM_WINDOWPOSCHANGED:
            ui_app_window_position_changed((WINDOWPOS*)lp);
            break;
        default:
            break;
    }
    return DefWindowProcA(ui_app_window(), m, wp, lp);
}

static long ui_app_set_window_long(int32_t index, long value) {
    ut_runtime.seterr(0);
    long r = SetWindowLongA(ui_app_window(), index, value); // r previous value
    fatal_if_not_zero(ut_runtime.err());
    return r;
}

static errno_t ui_app_set_layered_window(ui_color_t color, float alpha) {
    uint8_t  a = 0; // alpha 0..255
    uint32_t c = 0; // R8G8B8
    DWORD mask = 0;
    if (0 <= alpha && alpha <= 1.0) {
        mask |= LWA_ALPHA;
        a = (uint8_t)(alpha * 255 + 0.5);
    }
    if (color != ui_color_undefined) {
        mask |= LWA_COLORKEY;
        assert(ui_color_is_8bit(color));
        c = ui_gdi.color_rgb(color);
    }
    return b2e(SetLayeredWindowAttributes(ui_app_window(), c, a, mask));
}

static void ui_app_create_window(const ui_rect_t r) {
    WNDCLASSA wc = { 0 };
    wc.style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC | CS_DBLCLKS;
    wc.lpfnWndProc = ui_app_window_proc;
    wc.cbClsExtra = 0;
    wc.cbWndExtra = 256 * 1024;
    wc.hInstance = GetModuleHandleA(null);
    wc.hIcon = LoadIconA(wc.hInstance, MAKEINTRESOURCE(101)); // IDI_ICON 101
    wc.hCursor = (HCURSOR)ui_app.cursor;
    wc.hbrBackground = null;
    wc.lpszMenuName = null;
    wc.lpszClassName = ui_app.class_name;
    ui_app.icon = (ui_icon_t)wc.hIcon;
    ATOM atom = RegisterClassA(&wc);
    fatal_if(atom == 0);
    const DWORD WS_POPUP_EX = WS_POPUP|WS_SYSMENU|WS_THICKFRAME|
//                            WS_MAXIMIZE|WS_MAXIMIZEBOX| // this does not work for popup
                              WS_MINIMIZE|WS_MINIMIZEBOX;
    uint32_t style = ui_app.no_decor ? WS_POPUP_EX : WS_OVERLAPPEDWINDOW;
    HWND window = CreateWindowExA(WS_EX_COMPOSITED | WS_EX_LAYERED,
        ui_app.class_name, ui_app.title, style,
        r.x, r.y, r.w, r.h, null, null, wc.hInstance, null);
    not_null(ui_app.window);
    assert(window == ui_app_window()); (void)window;
    strprintf(ui_caption.title.text, "%s", ui_app.title);
    not_null(GetSystemMenu(ui_app_window(), false));
    ui_app.dpi.window = GetDpiForWindow(ui_app_window());
//  traceln("ui_app.dpi.window=%d", ui_app.dpi.window);
    RECT wrc = ui_app_ui2rect(&r);
    fatal_if_false(GetWindowRect(ui_app_window(), &wrc));
    ui_app.wrc = ui_app_rect2ui(&wrc);
    // DWMWA_CAPTION_COLOR is supported starting with Windows 11 Build 22000.
    if (IsWindowsVersionOrGreater(10, 0, 22000)) {
        COLORREF caption_color = (COLORREF)ui_gdi.color_rgb(ui_colors.dkgray3);
        fatal_if_not_zero(DwmSetWindowAttribute(ui_app_window(),
            DWMWA_CAPTION_COLOR, &caption_color, sizeof(caption_color)));
        BOOL immersive = TRUE;
        fatal_if_not_zero(DwmSetWindowAttribute(ui_app_window(),
            DWMWA_USE_IMMERSIVE_DARK_MODE, &immersive, sizeof(immersive)));
        // also available but not yet used:
//      DWMWA_USE_HOSTBACKDROPBRUSH
//      DWMWA_WINDOW_CORNER_PREFERENCE
//      DWMWA_BORDER_COLOR
//      DWMWA_CAPTION_COLOR
    }
    if (ui_app.aero) { // It makes app look like retro Windows 7 Aero style :)
        enum DWMNCRENDERINGPOLICY ncrp = DWMNCRP_DISABLED;
        (void)DwmSetWindowAttribute(ui_app_window(),
            DWMWA_NCRENDERING_POLICY, &ncrp, sizeof(ncrp));
    }
    // always start with window hidden and let application show it
    ui_app.show_window(ui.visibility.hide);
    if (ui_app.no_min || ui_app.no_max) {
        uint32_t exclude = WS_SIZEBOX;
        if (ui_app.no_min) { exclude = WS_MINIMIZEBOX; }
        if (ui_app.no_max) { exclude = WS_MAXIMIZEBOX; }
        uint32_t s = GetWindowLongA(ui_app_window(), GWL_STYLE);
        ui_app_set_window_long(GWL_STYLE, s & ~exclude);
        // even for windows without maximize/minimize
        // make sure "Minimize All Windows" still works:
        // ???
//      EnableMenuItem(GetSystemMenu(ui_app_window(), false),
//          SC_MINIMIZE, MF_BYCOMMAND | MF_ENABLED);
    } else if (!ui_app.no_min) {
        EnableMenuItem(GetSystemMenu(ui_app_window(), false),
            SC_MINIMIZE, MF_BYCOMMAND | MF_ENABLED);
    } else if (!ui_app.no_max) {
        EnableMenuItem(GetSystemMenu(ui_app_window(), false),
            SC_MINIMIZE, MF_BYCOMMAND | MF_ENABLED);
    }
    if (ui_app.no_size) {
        uint32_t s = GetWindowLong(ui_app_window(), GWL_STYLE);
        ui_app_set_window_long(GWL_STYLE, s & ~WS_SIZEBOX);
        enum { swp = SWP_FRAMECHANGED |
                     SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE };
        SetWindowPos(ui_app_window(), null, 0, 0, 0, 0, swp);
    }
    ui_theme.refresh(ui_app.window);
    if (ui_app.visibility != ui.visibility.hide) {
        ui_app.view->w = ui_app.wrc.w;
        ui_app.view->h = ui_app.wrc.h;
        AnimateWindow(ui_app_window(), 250, AW_ACTIVATE);
        ui_app.show_window(ui_app.visibility);
        ui_app_update_crc();
//      ui_app.view->w = ui_app.crc.w; // ui_app.crc is "client rectangle"
//      ui_app.view->h = ui_app.crc.h;
    }
    // even if it is hidden:
    ui_app_post_message(ui.message.opening, 0, 0);
//  SetWindowTheme(ui_app_window(), L"DarkMode_Explorer", null); ???
}

static void ui_app_full_screen(bool on) {
    static int32_t style;
    static WINDOWPLACEMENT wp;
    if (on != ui_app.is_full_screen) {
        ui_app_show_task_bar(!on);
        if (on) {
            style = GetWindowLongA(ui_app_window(), GWL_STYLE);
            ui_app_set_window_long(GWL_STYLE, (style | WS_POPUP | WS_VISIBLE) &
                ~(WS_OVERLAPPEDWINDOW));
            wp.length = sizeof(wp);
            fatal_if_false(GetWindowPlacement(ui_app_window(), &wp));
            WINDOWPLACEMENT nwp = wp;
            nwp.showCmd = SW_SHOWNORMAL;
            nwp.rcNormalPosition = (RECT){ui_app.mrc.x, ui_app.mrc.y,
                ui_app.mrc.x + ui_app.mrc.w, ui_app.mrc.y + ui_app.mrc.h};
            fatal_if_false(SetWindowPlacement(ui_app_window(), &nwp));
        } else {
            fatal_if_false(SetWindowPlacement(ui_app_window(), &wp));
            ui_app_set_window_long(GWL_STYLE,  style | WS_OVERLAPPED);
            enum { swp = SWP_FRAMECHANGED | SWP_NOMOVE | SWP_NOSIZE |
                         SWP_NOZORDER | SWP_NOOWNERZORDER };
            fatal_if_false(SetWindowPos(ui_app_window(), null, 0, 0, 0, 0, swp));
            enum DWMNCRENDERINGPOLICY ncrp = DWMNCRP_ENABLED;
            fatal_if_not_zero(DwmSetWindowAttribute(ui_app_window(),
                DWMWA_NCRENDERING_POLICY, &ncrp, sizeof(ncrp)));
        }
        ui_app.is_full_screen = on;
    }
}

static void ui_app_fast_redraw(void) { SetEvent(ui_app_event_invalidate); } // < 2us

static void ui_app_draw(void) { UpdateWindow(ui_app_window()); }

static void ui_app_invalidate_rect(const ui_rect_t* r) {
    RECT rc = ui_app_ui2rect(r);
    InvalidateRect(ui_app_window(), &rc, false);
}

// InvalidateRect() may wait for up to 30 milliseconds
// which is unacceptable for video drawing at monitor
// refresh rate

static void ui_app_redraw_thread(void* unused(p)) {
    ut_thread.realtime();
    ut_thread.name("ui_app.redraw");
    for (;;) {
        event_t es[] = { ui_app_event_invalidate, ui_app_event_quit };
        int32_t r = ut_event.wait_any(countof(es), es);
        if (r == 0) {
            if (ui_app_window() != null) {
                InvalidateRect(ui_app_window(), null, false);
            }
        } else {
            break;
        }
    }
}

static int32_t ui_app_message_loop(void) {
    MSG msg = {0};
    while (GetMessage(&msg, null, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessageA(&msg);
    }
    assert(msg.message == WM_QUIT);
    return (int32_t)msg.wParam;
}

static void ui_app_dispose(void) {
    ui_app_dispose_fonts();
    if (ui_gdi.clip != null) { DeleteRgn(ui_gdi.clip); }
    fatal_if_false(CloseHandle(ui_app_event_quit));
    fatal_if_false(CloseHandle(ui_app_event_invalidate));
}

static void ui_app_cursor_set(ui_cursor_t c) {
    // https://docs.microsoft.com/en-us/windows/win32/menurc/using-cursors
    ui_app.cursor = c;
    SetClassLongPtr(ui_app_window(), GCLP_HCURSOR, (LONG_PTR)c);
    POINT pt = {0};
    if (GetCursorPos(&pt)) { SetCursorPos(pt.x + 1, pt.y); SetCursorPos(pt.x, pt.y); }
}

static void ui_app_close_window(void) {
    // TODO: fix me. Band aid - start up with maximized no_decor window is broken
    if (ui_app.is_maximized()) { ui_app.show_window(ui.visibility.restore); }
    ui_app_post_message(WM_CLOSE, 0, 0);
}

static void ui_app_quit(int32_t exit_code) {
    ui_app.exit_code = exit_code;
    if (ui_app.can_close != null) {
        (void)ui_app.can_close(); // and deliberately ignore result
    }
    ui_app.can_close = null; // will not be called again
    ui_app.close(); // close and destroy app only window
}

static void ui_app_show_tooltip_or_toast(ui_view_t* view, int32_t x, int32_t y,
        fp64_t timeout) {
    if (view != null) {
        ui_app.animating.x = x;
        ui_app.animating.y = y;
        if (view->type == ui_view_mbx) {
            ((ui_mbx_t*)view)->option = -1;
        }
        // allow unparented ui for toast and tooltip
        ui_view_call_init(view);
        ui_view.localize(view);
        ui_app_animate_start(ui_app_toast_dim, ui_app_animation_steps);
        ui_app.animating.view = view;
        ui_app.animating.time = timeout > 0 ? ui_app.now + timeout : 0;
        ui_app.focus = null;
    } else {
        ui_app_toast_cancel();
    }
}

static void ui_app_show_toast(ui_view_t* view, fp64_t timeout) {
    ui_app_show_tooltip_or_toast(view, -1, -1, timeout);
}

static void ui_app_show_tooltip(ui_view_t* view, int32_t x, int32_t y,
        fp64_t timeout) {
    if (view != null) {
        ui_app_show_tooltip_or_toast(view, x, y, timeout);
    } else if (ui_app.animating.view != null && ui_app.animating.x >= 0 &&
               ui_app.animating.y >= 0) {
        ui_app_toast_cancel(); // only cancel tooltips not toasts
    }
}

static void ui_app_formatted_toast_va(fp64_t timeout, const char* format, va_list vl) {
    ui_app_show_toast(null, 0);
    static ui_label_t label;
    ui_label_init_va(&label, 0.0, format, vl);
    ui_app_show_toast(&label, timeout);
}

static void ui_app_formatted_toast(fp64_t timeout, const char* format, ...) {
    va_list vl;
    va_start(vl, format);
    ui_app_formatted_toast_va(timeout, format, vl);
    va_end(vl);
}

static void ui_app_create_caret(int32_t w, int32_t h) {
    fatal_if_false(CreateCaret(ui_app_window(), null, w, h));
    assert(GetSystemMetrics(SM_CARETBLINKINGENABLED));
}

static void ui_app_show_caret(void) {
    fatal_if_false(ShowCaret(ui_app_window()));
}

static void ui_app_move_caret(int32_t x, int32_t y) {
    fatal_if_false(SetCaretPos(x, y));
}

static void ui_app_hide_caret(void) {
    fatal_if_false(HideCaret(ui_app_window()));
}

static void ui_app_destroy_caret(void) {
    fatal_if_false(DestroyCaret());
}

static void ui_app_enable_sys_command_close(void) {
    EnableMenuItem(GetSystemMenu(GetConsoleWindow(), false),
        SC_CLOSE, MF_BYCOMMAND | MF_ENABLED);
}

static void ui_app_console_disable_close(void) {
    EnableMenuItem(GetSystemMenu(GetConsoleWindow(), false),
        SC_CLOSE, MF_BYCOMMAND | MF_DISABLED | MF_GRAYED);
    (void)freopen("CONOUT$", "w", stdout);
    (void)freopen("CONOUT$", "w", stderr);
    atexit(ui_app_enable_sys_command_close);
}

static int ui_app_console_attach(void) {
    int r = AttachConsole(ATTACH_PARENT_PROCESS) ? 0 : ut_runtime.err();
    if (r == 0) {
        ui_app_console_disable_close();
        ut_thread.sleep_for(0.1); // give cmd.exe a chance to print prompt again
        printf("\n");
    }
    return r;
}

static bool ui_app_is_stdout_redirected(void) {
    // https://stackoverflow.com/questions/30126490/how-to-check-if-stdout-is-redirected-to-a-file-or-to-a-console
    HANDLE out = GetStdHandle(STD_OUTPUT_HANDLE);
    DWORD type = out == null ? FILE_TYPE_UNKNOWN : GetFileType(out);
    type &= ~FILE_TYPE_REMOTE;
    // FILE_TYPE_DISK or FILE_TYPE_CHAR or FILE_TYPE_PIPE
    return type != FILE_TYPE_UNKNOWN;
}

static bool ui_app_is_console_visible(void) {
    HWND cw = GetConsoleWindow();
    return cw != null && IsWindowVisible(cw);
}

static int ui_app_set_console_size(int16_t w, int16_t h) {
    // width/height in characters
    HANDLE console = GetStdHandle(STD_OUTPUT_HANDLE);
    CONSOLE_SCREEN_BUFFER_INFOEX info = { sizeof(CONSOLE_SCREEN_BUFFER_INFOEX) };
    int r = GetConsoleScreenBufferInfoEx(console, &info) ? 0 : ut_runtime.err();
    if (r != 0) {
        traceln("GetConsoleScreenBufferInfoEx() %s", ut_str.error(r));
    } else {
        // tricky because correct order of the calls
        // SetConsoleWindowInfo() SetConsoleScreenBufferSize() depends on
        // current Window Size (in pixels) ConsoleWindowSize(in characters)
        // and SetConsoleScreenBufferSize().
        // After a lot of experimentation and reading docs most sensible option
        // is to try both calls in two differen orders.
        COORD c = {w, h};
        SMALL_RECT const minwin = { 0, 0, c.X - 1, c.Y - 1 };
        c.Y = 9001; // maximum buffer number of rows at the moment of implementation
        int r0 = SetConsoleWindowInfo(console, true, &minwin) ? 0 : ut_runtime.err();
//      if (r0 != 0) { traceln("SetConsoleWindowInfo() %s", ut_str.error(r0)); }
        int r1 = SetConsoleScreenBufferSize(console, c) ? 0 : ut_runtime.err();
//      if (r1 != 0) { traceln("SetConsoleScreenBufferSize() %s", ut_str.error(r1)); }
        if (r0 != 0 || r1 != 0) { // try in reverse order (which expected to work):
            r0 = SetConsoleScreenBufferSize(console, c) ? 0 : ut_runtime.err();
            if (r0 != 0) { traceln("SetConsoleScreenBufferSize() %s", ut_str.error(r0)); }
            r1 = SetConsoleWindowInfo(console, true, &minwin) ? 0 : ut_runtime.err();
            if (r1 != 0) { traceln("SetConsoleWindowInfo() %s", ut_str.error(r1)); }
	    }
        r = r0 == 0 ? r1 : r0; // first of two errors
    }
    return r;
}

static void ui_app_console_largest(void) {
    HANDLE console = GetStdHandle(STD_OUTPUT_HANDLE);
    // User have to manual uncheck "[x] Let system position window" in console
    // Properties -> Layout -> Window Position because I did not find the way
    // to programmatically unchecked it.
    // commented code below does not work.
    // see: https://www.os2museum.com/wp/disabling-quick-edit-mode/
    // and: https://learn.microsoft.com/en-us/windows/console/setconsolemode
    /* DOES NOT WORK:
    DWORD mode = 0;
    r = GetConsoleMode(console, &mode) ? 0 : ut_runtime.err();
    fatal_if_not_zero(r, "GetConsoleMode() %s", ut_str.error(r));
    mode &= ~ENABLE_AUTO_POSITION;
    r = SetConsoleMode(console, &mode) ? 0 : ut_runtime.err();
    fatal_if_not_zero(r, "SetConsoleMode() %s", ut_str.error(r));
    */
    CONSOLE_SCREEN_BUFFER_INFOEX info = { sizeof(CONSOLE_SCREEN_BUFFER_INFOEX) };
    int r = GetConsoleScreenBufferInfoEx(console, &info) ? 0 : ut_runtime.err();
    fatal_if_not_zero(r, "GetConsoleScreenBufferInfoEx() %s", ut_str.error(r));
    COORD c = GetLargestConsoleWindowSize(console);
    if (c.X > 80) { c.X &= ~0x7; }
    if (c.Y > 24) { c.Y &= ~0x3; }
    if (c.X > 80) { c.X -= 8; }
    if (c.Y > 24) { c.Y -= 4; }
    ui_app_set_console_size(c.X, c.Y);
    r = GetConsoleScreenBufferInfoEx(console, &info) ? 0 : ut_runtime.err();
    fatal_if_not_zero(r, "GetConsoleScreenBufferInfoEx() %s", ut_str.error(r));
    info.dwSize.Y = 9999; // maximum value at the moment of implementation
    r = SetConsoleScreenBufferInfoEx(console, &info) ? 0 : ut_runtime.err();
    fatal_if_not_zero(r, "SetConsoleScreenBufferInfoEx() %s", ut_str.error(r));
    ui_app_save_console_pos();
}

static void ui_app_make_topmost(void) {
    //  Places the window above all non-topmost windows.
    // The window maintains its topmost position even when it is deactivated.
    enum { swp = SWP_SHOWWINDOW | SWP_NOREPOSITION | SWP_NOMOVE | SWP_NOSIZE };
    fatal_if_false(SetWindowPos(ui_app_window(), HWND_TOPMOST, 0, 0, 0, 0, swp));
}

static void ui_app_activate(void) {
    ut_runtime.seterr(0);
    HWND previous = SetActiveWindow(ui_app_window());
    if (previous == null) { fatal_if_not_zero(ut_runtime.err()); }
}

static void ui_app_bring_to_foreground(void) {
    // SetForegroundWindow() does not activate window:
    fatal_if_false(SetForegroundWindow(ui_app_window()));
}

static void ui_app_bring_to_front(void) {
    ui_app.bring_to_foreground();
    ui_app.make_topmost();
    ui_app.bring_to_foreground();
    // because bring_to_foreground() does not activate
    ui_app.activate();
    ui_app.request_focus();
}

static void ui_app_set_title(const char* title) {
    strprintf(ui_caption.title.text, "%s", title);
    fatal_if_false(SetWindowTextA(ui_app_window(), title));
    if (!ui_caption.view.hidden) { ui_app.layout(); }
}

static ui_color_t ui_app_get_color(int32_t color_id) {
    return ui_theme.get_color(color_id); // SysGetColor() does not work on Win10
}

static void ui_app_capture_mouse(bool on) {
    static int32_t mouse_capture;
    if (on) {
        assert(mouse_capture == 0);
        mouse_capture++;
        SetCapture(ui_app_window());
    } else {
        assert(mouse_capture == 1);
        mouse_capture--;
        ReleaseCapture();
    }
}

static void ui_app_move_and_resize(const ui_rect_t* rc) {
    enum { swp = SWP_NOZORDER | SWP_NOACTIVATE };
    fatal_if_false(SetWindowPos(ui_app_window(), null,
            rc->x, rc->y, rc->w, rc->h, swp));
}

static void ui_app_set_console_title(HWND cw) {
    char text[256];
    text[0] = 0;
    GetWindowTextA((HWND)ui_app.window, text, countof(text));
    text[countof(text) - 1] = 0;
    char title[256];
    strprintf(title, "%s - Console", text);
    fatal_if_false(SetWindowTextA(cw, title));
}

static void ui_app_restore_console(int32_t *visibility) {
    HWND cw = GetConsoleWindow();
    if (cw != null) {
        RECT wr = {0};
        GetWindowRect(cw, &wr);
        ui_rect_t rc = ui_app_rect2ui(&wr);
        ui_app_load_console_pos(&rc, visibility);
        if (rc.w > 0 && rc.h > 0) {
//          traceln("%d,%d %dx%d px", rc.x, rc.y, rc.w, rc.h);
            CONSOLE_SCREEN_BUFFER_INFOEX info = {
                sizeof(CONSOLE_SCREEN_BUFFER_INFOEX)
            };
            int32_t r = ut_config.load(ui_app.class_name,
                "console_screen_buffer_infoex", &info, (int32_t)sizeof(info));
            if (r == sizeof(info)) { // 24x80
                SMALL_RECT sr = info.srWindow;
                int16_t w = (int16_t)ut_max(sr.Right - sr.Left + 1, 80);
                int16_t h = (int16_t)ut_max(sr.Bottom - sr.Top + 1, 24);
//              traceln("info: %dx%d", info.dwSize.X, info.dwSize.Y);
//              traceln("%d,%d %dx%d", sr.Left, sr.Top, w, h);
                if (w > 0 && h > 0) { ui_app_set_console_size(w, h); }
    	    }
            // do not resize console window just restore it's position
            enum { swp = SWP_NOZORDER | SWP_NOACTIVATE | SWP_NOSIZE };
            fatal_if_false(SetWindowPos(cw, null,
                    rc.x, rc.y, rc.w, rc.h, swp));
        } else {
            ui_app_console_largest();
        }
    }
}

static void ui_app_console_show(bool b) {
    HWND cw = GetConsoleWindow();
    if (cw != null && b != ui_app.is_console_visible()) {
        if (ui_app.is_console_visible()) { ui_app_save_console_pos(); }
        if (b) {
            int32_t ignored_visibility = 0;
            ui_app_restore_console(&ignored_visibility);
            ui_app_set_console_title(cw);
        }
        // If the window was previously visible, the return value is nonzero.
        // If the window was previously hidden, the return value is zero.
        bool unused_was_visible = ShowWindow(cw, b ? SW_SHOWNOACTIVATE : SW_HIDE);
        (void)unused_was_visible;
        if (b) { InvalidateRect(cw, null, true); SetActiveWindow(cw); }
        ui_app_save_console_pos(); // again after visibility changed
    }
}

static int ui_app_console_create(void) {
    int r = AllocConsole() ? 0 : ut_runtime.err();
    if (r == 0) {
        ui_app_console_disable_close();
        int32_t visibility = 0;
        ui_app_restore_console(&visibility);
        ui_app.console_show(visibility != 0);
    }
    return r;
}

static fp32_t ui_app_px2in(int pixels) {
    assert(ui_app.dpi.monitor_raw > 0);
    return ui_app.dpi.monitor_raw > 0 ?
           pixels / (fp32_t)ui_app.dpi.monitor_raw : 0;
}

static int32_t ui_app_in2px(fp32_t inches) {
    assert(ui_app.dpi.monitor_raw > 0);
    return (int32_t)(inches * ui_app.dpi.monitor_raw + 0.5f);
}

static void ui_app_request_layout(void) {
    ui_app_layout_dirty = true;
    ui_app.redraw();
}

static void ui_app_show_window(int32_t show) {
    assert(ui.visibility.hide <= show &&
           show <= ui.visibility.force_min);
    // ShowWindow() does not have documented error reporting
    bool was_visible = ShowWindow(ui_app_window(), show);
    (void)was_visible;
    const bool hiding =
        show == ui.visibility.hide ||
        show == ui.visibility.minimize ||
        show == ui.visibility.show_na ||
        show == ui.visibility.min_na;
    if (!hiding) {
        ui_app.bring_to_foreground(); // this does not make it ActiveWindow
        enum { swp = SWP_SHOWWINDOW | SWP_NOZORDER | SWP_NOSIZE |
                     SWP_NOREPOSITION | SWP_NOMOVE };
        SetWindowPos(ui_app_window(), null, 0, 0, 0, 0, swp);
        ui_app.request_focus();
    } else if (show == ui.visibility.hide ||
               show == ui.visibility.minimize ||
               show == ui.visibility.min_na) {
        ui_app_toast_cancel();
    }
}

static const char* ui_app_open_filename(const char* folder,
        const char* pairs[], int32_t n) {
    assert(pairs == null && n == 0 ||
           n >= 2 && n % 2 == 0);
    wchar_t memory[32 * 1024];
    wchar_t* filter = memory;
    if (pairs == null || n == 0) {
        filter = L"All Files\0*\0\0";
    } else {
        int32_t left = countof(memory) - 2;
        wchar_t* s = memory;
        for (int32_t i = 0; i < n; i+= 2) {
            wchar_t* s0 = utf8to16(pairs[i + 0]);
            wchar_t* s1 = utf8to16(pairs[i + 1]);
            int32_t n0 = (int32_t)wcslen(s0);
            int32_t n1 = (int32_t)wcslen(s1);
            assert(n0 > 0 && n1 > 0);
            fatal_if(n0 + n1 + 3 >= left, "too many filters");
            memcpy(s, s0, (n0 + 1) * 2);
            s += n0 + 1;
            left -= n0 + 1;
            memcpy(s, s1, (n1 + 1) * 2);
            s[n1] = 0;
            s += n1 + 1;
            left -= n1 + 1;
        }
        *s++ = 0;
    }
    wchar_t path[MAX_PATH];
    path[0] = 0;
    OPENFILENAMEW ofn = { sizeof(ofn) };
    ofn.hwndOwner = (HWND)ui_app.window;
    ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;
    ofn.lpstrFilter = filter;
    ofn.lpstrInitialDir = utf8to16(folder);
    ofn.lpstrFile = path;
    ofn.nMaxFile = sizeof(path);
    static thread_local char text[MAX_PATH];
    if (GetOpenFileNameW(&ofn) && path[0] != 0) {
        strprintf(text, "%s", utf16to8(path));
    } else {
        text[0] = 0;
    }
    return text;
}

static errno_t ui_app_clipboard_put_image(ui_image_t* im) {
    HDC canvas = GetDC(null);
    not_null(canvas);
    HDC src = CreateCompatibleDC(canvas); not_null(src);
    HDC dst = CreateCompatibleDC(canvas); not_null(dst);
    // CreateCompatibleBitmap(dst) will create monochrome bitmap!
    // CreateCompatibleBitmap(canvas) will create display compatible
    HBITMAP bitmap = CreateCompatibleBitmap(canvas, im->w, im->h);
//  HBITMAP bitmap = CreateBitmap(image.w, image.h, 1, 32, null);
    not_null(bitmap);
    HBITMAP s = SelectBitmap(src, im->bitmap); not_null(s);
    HBITMAP d = SelectBitmap(dst, bitmap);     not_null(d);
    POINT pt = { 0 };
    fatal_if_false(SetBrushOrgEx(dst, 0, 0, &pt));
    fatal_if_false(StretchBlt(dst, 0, 0, im->w, im->h, src, 0, 0,
        im->w, im->h, SRCCOPY));
    errno_t r = OpenClipboard(GetDesktopWindow()) ? 0 : GetLastError();
    if (r != 0) { traceln("OpenClipboard() failed %s", ut_str.error(r)); }
    if (r == 0) {
        r = EmptyClipboard() ? 0 : GetLastError();
        if (r != 0) { traceln("EmptyClipboard() failed %s", ut_str.error(r)); }
    }
    if (r == 0) {
        r = SetClipboardData(CF_BITMAP, bitmap) ? 0 : GetLastError();
        if (r != 0) {
            traceln("SetClipboardData() failed %s", ut_str.error(r));
        }
    }
    if (r == 0) {
        r = CloseClipboard() ? 0 : GetLastError();
        if (r != 0) {
            traceln("CloseClipboard() failed %s", ut_str.error(r));
        }
    }
    not_null(SelectBitmap(dst, d));
    not_null(SelectBitmap(src, s));
    fatal_if_false(DeleteBitmap(bitmap));
    fatal_if_false(DeleteDC(dst));
    fatal_if_false(DeleteDC(src));
    fatal_if_false(ReleaseDC(null, canvas));
    return r;
}

const char* ui_app_known_folder(int32_t kf) {
    // known folder ids order must match enum
    static const GUID* kfrid[] = {
        &FOLDERID_Profile,
        &FOLDERID_Desktop,
        &FOLDERID_Documents,
        &FOLDERID_Downloads,
        &FOLDERID_Music,
        &FOLDERID_Pictures,
        &FOLDERID_Videos,
        &FOLDERID_Public,
        &FOLDERID_ProgramFiles,
        &FOLDERID_ProgramData
    };
    static char known_foders[countof(kfrid)][MAX_PATH];
    fatal_if(!(0 <= kf && kf < countof(kfrid)), "invalide kf=%d", kf);
    if (known_foders[kf][0] == 0) {
        wchar_t* path = null;
        fatal_if_not_zero(SHGetKnownFolderPath(kfrid[kf], 0, null, &path));
        strprintf(known_foders[kf], "%s", utf16to8(path));
        CoTaskMemFree(path);
	}
    return known_foders[kf];
}

static ui_view_t ui_app_view = ui_view(container);

static bool ui_app_is_active(void) { return GetActiveWindow() == ui_app_window(); }

static bool ui_app_is_minimized(void) { return IsIconic(ui_app_window()); }

static bool ui_app_is_maximized(void) { return IsZoomed(ui_app_window()); }

static bool ui_app_has_focus(void) { return GetFocus() == ui_app_window(); }

static void window_request_focus(void* w) {
    // https://stackoverflow.com/questions/62649124/pywin32-setfocus-resulting-in-access-is-denied-error
    // https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-attachthreadinput
    assert(ut_thread.id() == ui_app.tid, "cannot be called from background thread");
    ut_runtime.seterr(0);
    w = SetFocus((HWND)w); // w previous focused window
    if (w == null) { fatal_if_not_zero(ut_runtime.err()); }
}

static void ui_app_request_focus(void) {
    window_request_focus(ui_app.window);
}

static void ui_app_init(void) {
    ui_app.view = &ui_app_view;
    ui_view_call_init(ui_app.view); // to get done with container_init()
    // for ui_view_debug_paint:
    strprintf(ui_app.view->text, "ui_app.view");
    ui_app.view->type          = ui_view_container;
    ui_app.view->color         = ui_app_get_color(ui_color_id_window);
    ui_app.view->paint         = ui_app_view_paint;
    ui_app.view->insets        = (ui_gaps_t){ 0, 0, 0, 0 };
    ui_app.redraw              = ui_app_fast_redraw;
    ui_app.draw                = ui_app_draw;
    ui_app.px2in               = ui_app_px2in;
    ui_app.in2px               = ui_app_in2px;
    ui_app.set_layered_window  = ui_app_set_layered_window;
    ui_app.hit_test            = ui_app_hit_test;
    ui_app.is_active           = ui_app_is_active,
    ui_app.is_minimized        = ui_app_is_minimized,
    ui_app.is_maximized        = ui_app_is_maximized,
    ui_app.has_focus           = ui_app_has_focus,
    ui_app.request_focus       = ui_app_request_focus,
    ui_app.activate            = ui_app_activate,
    ui_app.get_color           = ui_app_get_color,
    ui_app.set_title           = ui_app_set_title,
    ui_app.capture_mouse       = ui_app_capture_mouse,
    ui_app.move_and_resize     = ui_app_move_and_resize,
    ui_app.bring_to_foreground = ui_app_bring_to_foreground,
    ui_app.make_topmost        = ui_app_make_topmost,
    ui_app.bring_to_front      = ui_app_bring_to_front,
    ui_app.layout              = ui_app_request_layout;
    ui_app.invalidate          = ui_app_invalidate_rect;
    ui_app.full_screen         = ui_app_full_screen;
    ui_app.set_cursor          = ui_app_cursor_set;
    ui_app.close               = ui_app_close_window;
    ui_app.quit                = ui_app_quit;
    ui_app.set_timer           = ui_app_timer_set;
    ui_app.kill_timer          = ui_app_timer_kill;
    ui_app.post                = ui_app_post_message;
    ui_app.show_window         = ui_app_show_window;
    ui_app.show_toast          = ui_app_show_toast;
    ui_app.show_tooltip        = ui_app_show_tooltip;
    ui_app.toast_va            = ui_app_formatted_toast_va;
    ui_app.toast               = ui_app_formatted_toast;
    ui_app.create_caret        = ui_app_create_caret;
    ui_app.show_caret          = ui_app_show_caret;
    ui_app.move_caret          = ui_app_move_caret;
    ui_app.hide_caret          = ui_app_hide_caret;
    ui_app.destroy_caret       = ui_app_destroy_caret;
    ui_app.data_save           = ui_app_data_save;
    ui_app.data_size           = ui_app_data_size;
    ui_app.data_load           = ui_app_data_load;
    ui_app.open_filename       = ui_app_open_filename;
    ui_app.known_folder        = ui_app_known_folder;
    ui_app.is_stdout_redirected = ui_app_is_stdout_redirected;
    ui_app.is_console_visible  = ui_app_is_console_visible;
    ui_app.console_attach      = ui_app_console_attach;
    ui_app.console_create      = ui_app_console_create;
    ui_app.console_show        = ui_app_console_show;
    ui_app_event_quit          = ut_event.create();
    ui_app_event_invalidate    = ut_event.create();
    ui_app.init();
}

static void ui_app_init_windows(void) {
    fatal_if_not_zero(SetProcessDpiAwareness(PROCESS_PER_MONITOR_DPI_AWARE));
    not_null(SetThreadDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2));
    InitCommonControls(); // otherwise GetOpenFileName does not work
    ui_app.dpi.process = GetSystemDpiForProcess(GetCurrentProcess());
    ui_app.dpi.system = GetDpiForSystem(); // default was 96DPI
    // monitor dpi will be reinitialized in load_window_pos
    ui_app.dpi.monitor_effective = ui_app.dpi.system;
    ui_app.dpi.monitor_angular = ui_app.dpi.system;
    ui_app.dpi.monitor_raw = ui_app.dpi.system;
    static const RECT nowhere = {0x7FFFFFFF, 0x7FFFFFFF, 0x7FFFFFFF, 0x7FFFFFFF};
    ui_rect_t r = ui_app_rect2ui(&nowhere);
    ui_app_update_mi(&r, MONITOR_DEFAULTTOPRIMARY);
    ui_app.dpi.window = ui_app.dpi.monitor_effective;
    ui_app_init_fonts(ui_app.dpi.window); // for default monitor
}

static ui_rect_t ui_app_window_initial_rectangle(void) {
    const ui_window_sizing_t* ws = &ui_app.window_sizing;
    // it is not practical and thus not implemented handling
    // == (0, 0) and != (0, 0) for sizing half dimension (only w or only h)
    swear((ws->min_w != 0) == (ws->min_h != 0) &&
           ws->min_w >= 0 && ws->min_h >= 0,
          "ui_app.window_sizing .min_w=%.1f .min_h=%.1f", ws->min_w, ws->min_h);
    swear((ws->ini_w != 0) == (ws->ini_h != 0) &&
           ws->ini_w >= 0 && ws->ini_h >= 0,
          "ui_app.window_sizing .ini_w=%.1f .ini_h=%.1f", ws->ini_w, ws->ini_h);
    swear((ws->max_w != 0) == (ws->max_h != 0) &&
           ws->max_w >= 0 && ws->max_h >= 0,
          "ui_app.window_sizing .max_w=%.1f .max_h=%.1f", ws->max_w, ws->max_h);
    // if max is set then min and ini must be less than max
    if (ws->max_w != 0 || ws->max_h != 0) {
        swear(ws->min_w <= ws->max_w && ws->min_h <= ws->max_h,
            "ui_app.window_sizing .min_w=%.1f .min_h=%.1f .max_w=%1.f .max_h=%.1f",
             ws->min_w, ws->min_h, ws->max_w, ws->max_h);
        swear(ws->ini_w <= ws->max_w && ws->ini_h <= ws->max_h,
            "ui_app.window_sizing .min_w=%.1f .min_h=%.1f .max_w=%1.f .max_h=%.1f",
                ws->ini_w, ws->ini_h, ws->max_w, ws->max_h);
    }
    const int32_t ini_w = ui_app.in2px(ws->ini_w);
    const int32_t ini_h = ui_app.in2px(ws->ini_h);
    int32_t min_w = ws->min_w > 0 ? ui_app.in2px(ws->min_w) : ui_app.work_area.w / 4;
    int32_t min_h = ws->min_h > 0 ? ui_app.in2px(ws->min_h) : ui_app.work_area.h / 4;
    // (x, y) (-1, -1) means "let Windows manager position the window"
    ui_rect_t r = {-1, -1,
                   ini_w > 0 ? ini_w : min_w, ini_h > 0 ? ini_h : min_h};
    return r;
}

static int ui_app_win_main(void) {
    not_null(ui_app.init);
    ui_app_init_windows();
    ui_gdi.init();
    ut_clipboard.put_image = ui_app_clipboard_put_image;
    ui_app.last_visibility = ui.visibility.defau1t;
    ui_app_init();
    int r = 0;
//  ui_app_dump_dpi();
    // "wr" Window Rect in pixels: default is -1,-1, ini_w, ini_h
    ui_rect_t wr = ui_app_window_initial_rectangle();
    int32_t size_frame = GetSystemMetricsForDpi(SM_CXSIZEFRAME, ui_app.dpi.process);
    int32_t caption_height = GetSystemMetricsForDpi(SM_CYCAPTION, ui_app.dpi.process);
    wr.x -= size_frame;
    wr.w += size_frame * 2;
    wr.y -= size_frame + caption_height;
    wr.h += size_frame * 2 + caption_height;
    if (!ui_app_load_window_pos(&wr, &ui_app.last_visibility)) {
        // first time - center window
        wr.x = ui_app.work_area.x + (ui_app.work_area.w - wr.w) / 2;
        wr.y = ui_app.work_area.y + (ui_app.work_area.h - wr.h) / 2;
        ui_app_bring_window_inside_monitor(&ui_app.mrc, &wr);
    }
    ui_app.view->hidden = true; // start with ui hidden
    ui_app.view->font = &ui_app.fonts.regular;
    ui_app.view->w = wr.w - size_frame * 2;
    ui_app.view->h = wr.h - size_frame * 2 - caption_height;
    ui_app_layout_dirty = true; // layout will be done before first paint
    not_null(ui_app.class_name);
    if (!ui_app.no_ui) {
        ui_app_create_window(wr);
        thread_t thread = ut_thread.start(ui_app_redraw_thread, null);
        r = ui_app_message_loop();
        fatal_if_false(SetEvent(ui_app_event_quit));
        ut_thread.join(thread, -1);
        ui_app_dispose();
        if (r == 0 && ui_app.exit_code != 0) { r = ui_app.exit_code; }
    } else {
        r = ui_app.main();
    }
    if (ui_app.fini != null) { ui_app.fini(); }
    return r;
}

#pragma warning(disable: 28251) // inconsistent annotations

int WINAPI WinMain(HINSTANCE unused(instance), HINSTANCE unused(previous),
        char* unused(command), int show) {
    ui_app.tid = ut_thread.id();
    fatal_if_not_zero(CoInitializeEx(0, COINIT_MULTITHREADED | COINIT_SPEED_OVER_MEMORY));
    SetConsoleCP(CP_UTF8);
    ut_nls.init();
    ui_app.visibility = show;
    ut_args.WinMain();
    int32_t r = ui_app_win_main();
    ut_args.fini();
    return r;
}

int main(int argc, const char* argv[], const char** envp) {
    fatal_if_not_zero(CoInitializeEx(0, COINIT_MULTITHREADED | COINIT_SPEED_OVER_MEMORY));
    ut_args.main(argc, argv, envp);
    ut_nls.init();
    ui_app.tid = ut_thread.id();
    int r = ui_app.main();
    ut_args.fini();
    return r;
}

#pragma pop_macro("ui_app_canvas")
#pragma pop_macro("ui_app_window")

#pragma comment(lib, "comctl32")
#pragma comment(lib, "comdlg32")
#pragma comment(lib, "dwmapi")
#pragma comment(lib, "gdi32")
#pragma comment(lib, "msimg32")
#pragma comment(lib, "ole32")
#pragma comment(lib, "shcore")
#pragma comment(lib, "uxtheme")

