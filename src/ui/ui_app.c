#include "rt/rt.h"
#include "ui/ui.h"
#include "rt/rt_win32.h"

#pragma push_macro("ui_app_window")
#pragma push_macro("ui_app_canvas")

static bool ui_app_trace_utf16_keyboard_input;

#define ui_app_window() ((HWND)ui_app.window)
#define ui_app_canvas() ((HDC)ui_app.canvas)

static WNDCLASSW ui_app_wc; // window class

static NONCLIENTMETRICSW ui_app_ncm = { sizeof(NONCLIENTMETRICSW) };
static MONITORINFO ui_app_mi = {sizeof(MONITORINFO)};

static rt_event_t ui_app_event_quit;
static rt_event_t ui_app_event_invalidate;
static rt_event_t ui_app_wt; // waitable timer;

static rt_work_queue_t ui_app_queue;

static uintptr_t ui_app_timer_1s_id;
static uintptr_t ui_app_timer_100ms_id;

static bool ui_app_layout_dirty; // call layout() before paint

static char ui_app_decoded_pressed[16];  // utf8 of last decoded pressed key
static char ui_app_decoded_released[16]; // utf8 of last decoded released key
static uint16_t ui_app_high_surrogate;

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

static void ui_app_post_message(int32_t m, int64_t wp, int64_t lp) {
    rt_fatal_win32err(PostMessageA(ui_app_window(), (UINT)m,
            (WPARAM)wp, (LPARAM)lp));
}

static void ui_app_update_wt_timeout(void) {
    fp64_t next_due_at = -1.0;
    rt_atomics.spinlock_acquire(&ui_app_queue.lock);
    if (ui_app_queue.head != null) {
        next_due_at = ui_app_queue.head->when;
    }
    rt_atomics.spinlock_release(&ui_app_queue.lock);
    if (next_due_at >= 0) {
        static fp64_t last_next_due_at;
        fp64_t dt = next_due_at - rt_clock.seconds();
        if (dt <= 0) {
            ui_app_post_message(WM_NULL, 0, 0);
        } else if (last_next_due_at != next_due_at) {
            // Negative values indicate relative time in 100ns intervals
            LARGE_INTEGER rt = {0}; // relative negative time
            rt.QuadPart = (LONGLONG)(-dt * 1.0E+7);
            rt_swear(rt.QuadPart < 0, "dt: %.6f %lld", dt, rt.QuadPart);
            rt_fatal_win32err(
                SetWaitableTimer(ui_app_wt, &rt, 0, null, null, 0)
            );
        }
        last_next_due_at = next_due_at;
    }
}

static void ui_app_post(rt_work_t* w) {
    if (w->queue == null) { w->queue = &ui_app_queue; }
    // work item can be reused but only with the same queue
    rt_assert(w->queue == &ui_app_queue);
    rt_work_queue.post(w);
    ui_app_update_wt_timeout();
}

static void ui_app_alarm_thread(void* rt_unused(p)) {
    rt_thread.realtime();
    rt_thread.name("ui_app.alarm");
    for (;;) {
        rt_event_t es[] = { ui_app_wt, ui_app_event_quit };
        int32_t ix = rt_event.wait_any(rt_countof(es), es);
        if (ix == 0) {
            ui_app_post_message(WM_NULL, 0, 0);
        } else {
            break;
        }
    }
}


// InvalidateRect() may wait for up to 30 milliseconds
// which is unacceptable for video drawing at monitor
// refresh rate

static void ui_app_redraw_thread(void* rt_unused(p)) {
    rt_thread.realtime();
    rt_thread.name("ui_app.redraw");
    for (;;) {
        rt_event_t es[] = { ui_app_event_invalidate, ui_app_event_quit };
        int32_t ix = rt_event.wait_any(rt_countof(es), es);
        if (ix == 0) {
            if (ui_app_window() != null) {
                InvalidateRect(ui_app_window(), null, false);
            }
        } else {
            break;
        }
    }
}


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
    rt_fatal_win32err(SystemParametersInfoForDpi(SPI_GETNONCLIENTMETRICS,
        sizeof(ui_app_ncm), &ui_app_ncm, 0, (DWORD)dpi));
}

static void ui_app_update_monitor_dpi(HMONITOR monitor, ui_dpi_t* dpi) {
    dpi->monitor_max = 72;
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
            rt_thread.sleep_for(1.0 / 32); // and retry:
            r = GetDpiForMonitor(monitor, (MONITOR_DPI_TYPE)mtd, &dpi_x, &dpi_y);
        }
        if (r == 0) {
            // EFFECTIVE_DPI 168 168 (with regard of user scaling)
            // ANGULAR_DPI 247 248 (diagonal)
            // RAW_DPI 283 284 (horizontal, vertical)
            // Parallels Desktop 16.5.0 (49183) on macOS Mac Book Air
            // EFFECTIVE_DPI 192 192 (with regard of user scaling)
            // ANGULAR_DPI 224 224 (diagonal)
            // RAW_DPI 72 72
            const int32_t max_xy = (int32_t)rt_max(dpi_x, dpi_y);
            switch (mtd) {
                case MDT_EFFECTIVE_DPI:
                    dpi->monitor_effective = max_xy;
//                  rt_println("ui_app.dpi.monitor_effective := max(%d,%d)", dpi_x, dpi_y);
                    break;
                case MDT_ANGULAR_DPI:
                    dpi->monitor_angular = max_xy;
//                  rt_println("ui_app.dpi.monitor_angular := max(%d,%d)", dpi_x, dpi_y);
                    break;
                case MDT_RAW_DPI:
                    dpi->monitor_raw = max_xy;
//                  rt_println("ui_app.dpi.monitor_raw := max(%d,%d)", dpi_x, dpi_y);
                    break;
                default: rt_assert(false);
            }
            dpi->monitor_max = rt_max(dpi->monitor_max, max_xy);
        }
    }
//  rt_println("ui_app.dpi.monitor_max := %d", dpi->monitor_max);
}

#ifndef UI_APP_DEBUG

static void ui_app_dump_dpi(void) {
    rt_println("ui_app.dpi.monitor_effective: %d", ui_app.dpi.monitor_effective  );
    rt_println("ui_app.dpi.monitor_angular  : %d", ui_app.dpi.monitor_angular    );
    rt_println("ui_app.dpi.monitor_raw      : %d", ui_app.dpi.monitor_raw        );
    rt_println("ui_app.dpi.monitor_max      : %d", ui_app.dpi.monitor_max        );
    rt_println("ui_app.dpi.window           : %d", ui_app.dpi.window             );
    rt_println("ui_app.dpi.system           : %d", ui_app.dpi.system             );
    rt_println("ui_app.dpi.process          : %d", ui_app.dpi.process            );
    rt_println("ui_app.mrc      : %d,%d %dx%d", ui_app.mrc.x, ui_app.mrc.y,
                                             ui_app.mrc.w, ui_app.mrc.h);
    rt_println("ui_app.wrc      : %d,%d %dx%d", ui_app.wrc.x, ui_app.wrc.y,
                                             ui_app.wrc.w, ui_app.wrc.h);
    rt_println("ui_app.crc      : %d,%d %dx%d", ui_app.crc.x, ui_app.crc.y,
                                             ui_app.crc.w, ui_app.crc.h);
    rt_println("ui_app.work_area: %d,%d %dx%d", ui_app.work_area.x, ui_app.work_area.y,
                                             ui_app.work_area.w, ui_app.work_area.h);
    int32_t mxt_x = GetSystemMetrics(SM_CXMAXTRACK);
    int32_t mxt_y = GetSystemMetrics(SM_CYMAXTRACK);
    rt_println("MAXTRACK: %d, %d", mxt_x, mxt_y);
    int32_t scr_x = GetSystemMetrics(SM_CXSCREEN);
    int32_t scr_y = GetSystemMetrics(SM_CYSCREEN);
    fp64_t monitor_x = (fp64_t)scr_x / (fp64_t)ui_app.dpi.monitor_max;
    fp64_t monitor_y = (fp64_t)scr_y / (fp64_t)ui_app.dpi.monitor_max;
    rt_println("SCREEN: %d, %d %.1fx%.1f\"", scr_x, scr_y, monitor_x, monitor_y);
}

#endif

static bool ui_app_update_mi(const ui_rect_t* r, uint32_t flags) {
    RECT rc = ui_app_ui2rect(r);
    HMONITOR monitor = MonitorFromRect(&rc, flags);
//  TODO: moving between monitors with different DPIs
//  HMONITOR mw = MonitorFromWindow(ui_app_window(), flags);
    if (monitor != null) {
        ui_app_update_monitor_dpi(monitor, &ui_app.dpi);
        rt_fatal_win32err(GetMonitorInfoA(monitor, &ui_app_mi));
        ui_app.work_area = ui_app_rect2ui(&ui_app_mi.rcWork);
        ui_app.mrc = ui_app_rect2ui(&ui_app_mi.rcMonitor);
//      ui_app_dump_dpi();
    }
    return monitor != null;
}

static void ui_app_update_crc(void) {
    RECT rc = {0};
    rt_fatal_win32err(GetClientRect(ui_app_window(), &rc));
    ui_app.crc = ui_app_rect2ui(&rc);
}

static void ui_app_dispose_fonts(void) {
    ui_gdi.delete_font(ui_app.fm.prop.normal.font);
    ui_gdi.delete_font(ui_app.fm.prop.tiny.font);
    ui_gdi.delete_font(ui_app.fm.prop.title.font);
    ui_gdi.delete_font(ui_app.fm.prop.rubric.font);
    ui_gdi.delete_font(ui_app.fm.prop.H1.font);
    ui_gdi.delete_font(ui_app.fm.prop.H2.font);
    ui_gdi.delete_font(ui_app.fm.prop.H3.font);
    memset(&ui_app.fm.prop, 0x00, sizeof(ui_app.fm.prop));
    ui_gdi.delete_font(ui_app.fm.mono.normal.font);
    ui_gdi.delete_font(ui_app.fm.mono.tiny.font);
    ui_gdi.delete_font(ui_app.fm.mono.title.font);
    ui_gdi.delete_font(ui_app.fm.mono.rubric.font);
    ui_gdi.delete_font(ui_app.fm.mono.H1.font);
    ui_gdi.delete_font(ui_app.fm.mono.H2.font);
    ui_gdi.delete_font(ui_app.fm.mono.H3.font);
    memset(&ui_app.fm.mono, 0x00, sizeof(ui_app.fm.mono));
}

static fp64_t ui_app_px2pt(fp64_t px) {
    rt_assert(ui_app.dpi.window >= 72.0);
    return px * 72.0 / (fp64_t)ui_app.dpi.window;
}

static int32_t ui_app_pt2px(fp64_t pt) { // rounded
    return (int32_t)(pt * (fp64_t)ui_app.dpi.window / 72.0 + 0.5);
}

static void ui_app_init_cursors(void) {
    if (ui_app.cursors.arrow == null) {
        ui_app.cursors.arrow     = (ui_cursor_t)LoadCursorW(null, IDC_ARROW);
        ui_app.cursors.wait      = (ui_cursor_t)LoadCursorW(null, IDC_WAIT);
        ui_app.cursors.ibeam     = (ui_cursor_t)LoadCursorW(null, IDC_IBEAM);
        ui_app.cursors.size_nwse = (ui_cursor_t)LoadCursorW(null, IDC_SIZENWSE);
        ui_app.cursors.size_nesw = (ui_cursor_t)LoadCursorW(null, IDC_SIZENESW);
        ui_app.cursors.size_we   = (ui_cursor_t)LoadCursorW(null, IDC_SIZEWE);
        ui_app.cursors.size_ns   = (ui_cursor_t)LoadCursorW(null, IDC_SIZENS);
        ui_app.cursors.size_all  = (ui_cursor_t)LoadCursorW(null, IDC_SIZEALL);
        ui_app.cursor = ui_app.cursors.arrow;
    }
}

static void ui_app_ncm_dump_fonts(void) {
    // Win10/Win11 all 5 fonts are exactly the same:
//  Caption  : Segoe UI 0x-12 weight: 400 quality: 0
//  SmCaption: Segoe UI 0x-12 weight: 400 quality: 0
//  Menu     : Segoe UI 0x-12 weight: 400 quality: 0
//  Status   : Segoe UI 0x-12 weight: 400 quality: 0
//  Message  : Segoe UI 0x-12 weight: 400 quality: 0
#if 0
    const LOGFONTW* fonts[] = {
        &ui_app_ncm.lfCaptionFont, &ui_app_ncm.lfSmCaptionFont,
        &ui_app_ncm.lfMenuFont, &ui_app_ncm.lfStatusFont,
        &ui_app_ncm.lfMessageFont
    };
    const char* font_names[] = {
        "Caption", "SmCaption", "Menu", "Status", "Message"
    };
    for (int32_t i = 0; i < rt_countof(fonts); i++) {
        const LOGFONTW* lf = fonts[i];
        char fn[128];
        rt_str.utf16to8(fn, rt_countof(fn), lf->lfFaceName, -1);
        rt_println("%-9s: %s %dx%d weight: %d quality: %d", font_names[i], fn,
                   lf->lfWidth, lf->lfHeight, lf->lfWeight, lf->lfQuality);
    }
#endif
}

static void ui_app_dump_font_size(const char* name, const LOGFONTW* lf,
                                  ui_fm_t* fm) {
    rt_swear(abs(lf->lfHeight) == fm->height - fm->internal_leading);
    rt_swear(fm->external_leading == 0); // "Segoe UI" and "Cascadia Mono"
    rt_swear(ui_app.dpi.window >= 72);
    // "The height, in logical units, of the font's character cell or character.
    //  The character height value (also known as the em height) is the
    //  character cell height value minus the internal-leading value."
    #ifdef UI_APP_DUMP_FONT_SIZE
        int32_t ascender = fm->baseline - fm->ascent;
        int32_t cell = fm->height - ascender - fm->descent;
        fp64_t  pt = fm->height * 72.0 / (fp64_t)ui_app.dpi.window;
        rt_println("%-6s .lfH: %+3d h: %d pt: %6.3f "
                   "a: %2d c: %2d d: %d bl: %2d il: %2d lg: %d",
                    name, lf->lfHeight, fm->height, pt,
                    ascender, cell, fm->descent, fm->baseline,
                    fm->internal_leading, fm->line_gap);
        #if 0 // TODO: need better understanding of box geometry in
              // "design units"
            // box scale factor: design units -> pixels
            fp64_t  sf = pt * 72.0 / (fp64_t)fm->design_units_per_em;
            sf *= (fp64_t)ui_app.dpi.window / 72.0; // into pixels (unclear???)
            int32_t bx = (int32_t)(fm->box.x * sf + 0.5);
            int32_t by = (int32_t)(fm->box.y * sf + 0.5);
            int32_t bw = (int32_t)(fm->box.w * sf + 0.5);
            int32_t bh = (int32_t)(fm->box.h * sf + 0.5);
            rt_println("%-6s .box: %d,%d %dx%d", name, bx, by, bw, bh);
        #endif
    #else
        (void)name; // unused
    #endif
}

static void ui_app_init_fms(ui_fms_t* fms, const LOGFONTW* base) {
    LOGFONTW lf = *base;
    // lf.lfQuality is zero (DEFAULT_QUALITY) that gets internally
    // interpreted as CLEARTYPE_QUALITY (if clear type is enabled
    // system wide and it looks really bad on 4K monitors
    // Experimentally it looks like Windows UI is using PROOF_QUALITY
    // which is anti-aliased w/o ClearType rainbows
    // TODO: maybe DEFAULT_QUALITY on 96DPI,
    //             PROOF_QUALITY below 4K
    //             ANTIALIASED_QUALITY on 4K and ?
    lf.lfQuality = ANTIALIASED_QUALITY;
    ui_gdi.update_fm(&fms->normal, (ui_font_t)CreateFontIndirectW(&lf));
    ui_app_dump_font_size("normal", &lf, &fms->normal);
    const fp64_t fh = lf.lfHeight;
    rt_swear(fh != 0);
    lf.lfHeight = (int32_t)(fh * 8.0 / 11.0 + 0.5);
    ui_gdi.update_fm(&fms->tiny, (ui_font_t)CreateFontIndirectW(&lf));
    ui_app_dump_font_size("tiny", &lf, &fms->tiny);

    lf.lfWeight = FW_SEMIBOLD;
    lf.lfHeight = (int32_t)(fh * 2.25 + 0.5);
    ui_gdi.update_fm(&fms->title, (ui_font_t)CreateFontIndirectW(&lf));
    ui_app_dump_font_size("title", &lf, &fms->title);
    lf.lfHeight = (int32_t)(fh * 2.00 + 0.5);
    ui_gdi.update_fm(&fms->rubric, (ui_font_t)CreateFontIndirectW(&lf));
    ui_app_dump_font_size("rubric", &lf, &fms->rubric);
    lf.lfHeight = (int32_t)(fh * 1.75 + 0.5);
    ui_gdi.update_fm(&fms->H1, (ui_font_t)CreateFontIndirectW(&lf));
    ui_app_dump_font_size("H1", &lf, &fms->H1);
    lf.lfHeight = (int32_t)(fh * 1.4 + 0.5);
    ui_gdi.update_fm(&fms->H2, (ui_font_t)CreateFontIndirectW(&lf));
    ui_app_dump_font_size("H2", &lf, &fms->H2);
    lf.lfHeight = (int32_t)(fh * 1.15 + 0.5);
    ui_gdi.update_fm(&fms->H3, (ui_font_t)CreateFontIndirectW(&lf));
    ui_app_dump_font_size("H3", &lf, &fms->H3);
}

static void ui_app_init_fonts(int32_t dpi) {
    ui_app_update_ncm(dpi);
    ui_app_ncm_dump_fonts();
    if (ui_app.fm.prop.normal.font != null) { ui_app_dispose_fonts(); }
    LOGFONTW mono = ui_app_ncm.lfMessageFont;
    // TODO: how to get name of monospaced from Win32 API?
    wcscpy_s(mono.lfFaceName, rt_countof(mono.lfFaceName), L"Cascadia Mono");
    mono.lfPitchAndFamily |= FIXED_PITCH;
//  rt_println("ui_app.fm.mono");
    ui_app_init_fms(&ui_app.fm.mono, &mono);
    LOGFONTW prop = ui_app_ncm.lfMessageFont;
    prop.lfHeight--; // inc by 1
//  rt_println("ui_app.fm.prop");
    ui_app_init_fms(&ui_app.fm.prop, &ui_app_ncm.lfMessageFont);
}

static void ui_app_data_save(const char* name, const void* data, int32_t bytes) {
    rt_config.save(ui_app.class_name, name, data, bytes);
}

static int32_t ui_app_data_size(const char* name) {
    return rt_config.size(ui_app.class_name, name);
}

static int32_t ui_app_data_load(const char* name, void* data, int32_t bytes) {
    return rt_config.load(ui_app.class_name, name, data, bytes);
}

typedef rt_begin_packed struct ui_app_wiw_s { // "where is window"
    // coordinates in pixels relative (0,0) top left corner
    // of primary monitor from GetWindowPlacement
    int32_t    bytes;
    int32_t    padding;      // to align rectangles and points to 8 bytes
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
} rt_end_packed ui_app_wiw_t;

static BOOL CALLBACK ui_app_monitor_enum_proc(HMONITOR monitor,
        HDC rt_unused(hdc), RECT* rt_unused(rc1), LPARAM that) {
    ui_app_wiw_t* wiw = (ui_app_wiw_t*)(uintptr_t)that;
    MONITORINFOEXA mi = { .cbSize = sizeof(MONITORINFOEXA) };
    rt_fatal_win32err(GetMonitorInfoA(monitor, (MONITORINFO*)&mi));
    // monitors can be in negative coordinate spaces and even rotated upside-down
    const int32_t min_x = rt_min(mi.rcMonitor.left, mi.rcMonitor.right);
    const int32_t min_y = rt_min(mi.rcMonitor.top,  mi.rcMonitor.bottom);
    const int32_t max_w = rt_max(mi.rcMonitor.left, mi.rcMonitor.right);
    const int32_t max_h = rt_max(mi.rcMonitor.top,  mi.rcMonitor.bottom);
    wiw->space.x = rt_min(wiw->space.x, min_x);
    wiw->space.y = rt_min(wiw->space.y, min_y);
    wiw->space.w = rt_max(wiw->space.w, max_w);
    wiw->space.h = rt_max(wiw->space.h, max_h);
    return true; // keep going
}

static void ui_app_enum_monitors(ui_app_wiw_t* wiw) {
    EnumDisplayMonitors(null, null, ui_app_monitor_enum_proc,
        (LPARAM)(uintptr_t)wiw);
    // because ui_app_monitor_enum_proc() puts max into w,h:
    wiw->space.w -= wiw->space.x;
    wiw->space.h -= wiw->space.y;
}

static void ui_app_save_window_pos(ui_window_t wnd, const char* name, bool dump) {
    RECT wr = {0};
    rt_fatal_win32err(GetWindowRect((HWND)wnd, &wr));
    ui_rect_t wrc = ui_app_rect2ui(&wr);
    ui_app_update_mi(&wrc, MONITOR_DEFAULTTONEAREST);
    WINDOWPLACEMENT wpl = { .length = sizeof(wpl) };
    rt_fatal_win32err(GetWindowPlacement((HWND)wnd, &wpl));
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
        .dpi = ui_app.dpi.monitor_max,
        .flags = (int32_t)wpl.flags,
        .show  = (int32_t)wpl.showCmd
    };
    ui_app_enum_monitors(&wiw);
    if (dump) {
        rt_println("wiw.space: %d,%d %dx%d",
              wiw.space.x, wiw.space.y, wiw.space.w, wiw.space.h);
        rt_println("MAXTRACK: %d, %d", wiw.max_track.x, wiw.max_track.y);
        rt_println("wpl.rcNormalPosition: %d,%d %dx%d",
            wpl.rcNormalPosition.left, wpl.rcNormalPosition.top,
            wpl.rcNormalPosition.right - wpl.rcNormalPosition.left,
            wpl.rcNormalPosition.bottom - wpl.rcNormalPosition.top);
        rt_println("wpl.ptMinPosition: %d,%d",
            wpl.ptMinPosition.x, wpl.ptMinPosition.y);
        rt_println("wpl.ptMaxPosition: %d,%d",
            wpl.ptMaxPosition.x, wpl.ptMaxPosition.y);
        rt_println("wpl.showCmd: %d", wpl.showCmd);
        // WPF_SETMINPOSITION. WPF_RESTORETOMAXIMIZED WPF_ASYNCWINDOWPLACEMENT
        rt_println("wpl.flags: %d", wpl.flags);
    }
//  rt_println("%d,%d %dx%d show=%d", wiw.placement.x, wiw.placement.y,
//      wiw.placement.w, wiw.placement.h, wiw.show);
    rt_config.save(ui_app.class_name, name, &wiw, sizeof(wiw));
    ui_app_update_mi(&ui_app.wrc, MONITOR_DEFAULTTONEAREST);
}

static void ui_app_save_console_pos(void) {
    HWND cw = GetConsoleWindow();
    if (cw != null) {
        ui_app_save_window_pos((ui_window_t)cw, "wic", false);
        HANDLE console = GetStdHandle(STD_OUTPUT_HANDLE);
        CONSOLE_SCREEN_BUFFER_INFOEX info = { sizeof(CONSOLE_SCREEN_BUFFER_INFOEX) };
        int32_t r = GetConsoleScreenBufferInfoEx(console, &info) ? 0 : rt_core.err();
        if (r != 0) {
            rt_println("GetConsoleScreenBufferInfoEx() %s", rt_strerr(r));
        } else {
            rt_config.save(ui_app.class_name, "console_screen_buffer_infoex",
                            &info, (int32_t)sizeof(info));
//          rt_println("info: %dx%d", info.dwSize.X, info.dwSize.Y);
//          rt_println("%d,%d %dx%d", info.srWindow.Left, info.srWindow.Top,
//              info.srWindow.Right - info.srWindow.Left,
//              info.srWindow.Bottom - info.srWindow.Top);
        }
    }
    int32_t v = ui_app.is_console_visible();
    // "icv" "is console visible"
    rt_config.save(ui_app.class_name, "icv", &v, (int32_t)sizeof(v));
}

static bool ui_app_is_fully_inside(const ui_rect_t* inner,
                                const ui_rect_t* outer) {
    return
        outer->x <= inner->x && inner->x + inner->w <= outer->x + outer->w &&
        outer->y <= inner->y && inner->y + inner->h <= outer->y + outer->h;
}

static void ui_app_bring_window_inside_monitor(const ui_rect_t* mrc, ui_rect_t* wrc) {
    rt_assert(mrc->w > 0 && mrc->h > 0);
    // Check if window rect is inside monitor rect
    if (!ui_app_is_fully_inside(wrc, mrc)) {
        // Move window into monitor rect
        wrc->x = rt_max(mrc->x, rt_min(mrc->x + mrc->w - wrc->w, wrc->x));
        wrc->y = rt_max(mrc->y, rt_min(mrc->y + mrc->h - wrc->h, wrc->y));
        // Adjust size to fit into monitor rect
        wrc->w = rt_min(wrc->w, mrc->w);
        wrc->h = rt_min(wrc->h, mrc->h);
    }
}

static bool ui_app_load_window_pos(ui_rect_t* rect, int32_t *visibility) {
    ui_app_wiw_t wiw = {0}; // where is window
    bool loaded = rt_config.load(ui_app.class_name, "wiw", &wiw, sizeof(wiw)) ==
                                sizeof(wiw);
    if (loaded) {
        #ifdef UI_APP_DEBUG
            rt_println("wiw.placement: %d,%d %dx%d", wiw.placement.x, wiw.placement.y,
                wiw.placement.w, wiw.placement.h);
            rt_println("wiw.mrc: %d,%d %dx%d", wiw.mrc.x, wiw.mrc.y, wiw.mrc.w, wiw.mrc.h);
            rt_println("wiw.work_area: %d,%d %dx%d", wiw.work_area.x, wiw.work_area.y,
                                                  wiw.work_area.w, wiw.work_area.h);
            rt_println("wiw.min_position: %d,%d", wiw.min_position.x, wiw.min_position.y);
            rt_println("wiw.max_position: %d,%d", wiw.max_position.x, wiw.max_position.y);
            rt_println("wiw.max_track: %d,%d", wiw.max_track.x, wiw.max_track.y);
            rt_println("wiw.dpi: %d", wiw.dpi);
            rt_println("wiw.flags: %d", wiw.flags);
            rt_println("wiw.show: %d", wiw.show);
        #endif
        ui_app_update_mi(&wiw.placement, MONITOR_DEFAULTTONEAREST);
        bool same_monitor = memcmp(&wiw.mrc, &ui_app.mrc, sizeof(wiw.mrc)) == 0;
//      rt_println("%d,%d %dx%d", p->x, p->y, p->w, p->h);
        if (same_monitor) {
            *rect = wiw.placement;
        } else { // moving to another monitor
            rect->x = (wiw.placement.x - wiw.mrc.x) * ui_app.mrc.w / wiw.mrc.w;
            rect->y = (wiw.placement.y - wiw.mrc.y) * ui_app.mrc.h / wiw.mrc.h;
            // adjust according to monitors DPI difference:
            // (w, h) theoretically could be as large as 0xFFFF
            const int64_t w = (int64_t)wiw.placement.w * ui_app.dpi.monitor_max;
            const int64_t h = (int64_t)wiw.placement.h * ui_app.dpi.monitor_max;
            rect->w = (int32_t)(w / wiw.dpi);
            rect->h = (int32_t)(h / wiw.dpi);
        }
        *visibility = wiw.show;
    }
//  rt_println("%d,%d %dx%d show=%d", rect->x, rect->y, rect->w, rect->h, *visibility);
    ui_app_bring_window_inside_monitor(&ui_app.mrc, rect);
//  rt_println("%d,%d %dx%d show=%d", rect->x, rect->y, rect->w, rect->h, *visibility);
    return loaded;
}

static bool ui_app_load_console_pos(ui_rect_t* rect, int32_t *visibility) {
    ui_app_wiw_t wiw = {0}; // where is window
    *visibility = 0; // boolean
    bool loaded = rt_config.load(ui_app.class_name, "wic", &wiw, sizeof(wiw)) ==
                                sizeof(wiw);
    if (loaded) {
        ui_app_update_mi(&wiw.placement, MONITOR_DEFAULTTONEAREST);
        bool same_monitor = memcmp(&wiw.mrc, &ui_app.mrc, sizeof(wiw.mrc)) == 0;
//      rt_println("%d,%d %dx%d", p->x, p->y, p->w, p->h);
        if (same_monitor) {
            *rect = wiw.placement;
        } else { // moving to another monitor
            rect->x = (wiw.placement.x - wiw.mrc.x) * ui_app.mrc.w / wiw.mrc.w;
            rect->y = (wiw.placement.y - wiw.mrc.y) * ui_app.mrc.h / wiw.mrc.h;
            // adjust according to monitors DPI difference:
            // (w, h) theoretically could be as large as 0xFFFF
            const int64_t w = (int64_t)wiw.placement.w * ui_app.dpi.monitor_max;
            const int64_t h = (int64_t)wiw.placement.h * ui_app.dpi.monitor_max;
            rect->w = (int32_t)(w / wiw.dpi);
            rect->h = (int32_t)(h / wiw.dpi);
        }
        *visibility = wiw.show != 0;
        ui_app_update_mi(&ui_app.wrc, MONITOR_DEFAULTTONEAREST);
    }
    return loaded;
}

static void ui_app_timer_kill(ui_timer_t timer) {
    rt_fatal_win32err(KillTimer(ui_app_window(), timer));
}

static ui_timer_t ui_app_timer_set(uintptr_t id, int32_t ms) {
    rt_not_null(ui_app_window());
    rt_assert(10 <= ms && ms < 0x7FFFFFFF);
    ui_timer_t tid = (ui_timer_t)SetTimer(ui_app_window(), id, (uint32_t)ms, null);
    rt_fatal_if(tid == 0);
    rt_assert(tid == id);
    return tid;
}

static void ui_app_timer(ui_view_t* view, ui_timer_t id) {
    ui_view.timer(view, id);
    if (id == ui_app_timer_1s_id) { ui_view.every_sec(view); }
    if (id == ui_app_timer_100ms_id) { ui_view.every_100ms(view); }
}

static void ui_app_animate_timer(void) {
    ui_app_post_message(ui.message.animate, (int64_t)ui_app_animate.step + 1,
        (int64_t)(uintptr_t)ui_app_animate.f);
}

static void ui_app_wm_timer(ui_timer_t id) {
    if (ui_app.animating.time != 0 && ui_app.now > ui_app.animating.time) {
        ui_app.show_toast(null, 0);
    }
    if (ui_app_animate.timer == id) { ui_app_animate_timer(); }
    ui_app_timer(ui_app.root, id);
}

static void ui_app_window_dpi(void) {
    int32_t dpi = (int32_t)GetDpiForWindow(ui_app_window());
    if (dpi == 0) { dpi = (int32_t)GetDpiForWindow(GetParent(ui_app_window())); }
    if (dpi == 0) { dpi = (int32_t)GetDpiForWindow(GetDesktopWindow()); }
    if (dpi == 0) { dpi = (int32_t)GetSystemDpiForProcess(GetCurrentProcess()); }
    if (dpi == 0) { dpi = (int32_t)GetDpiForSystem(); }
    ui_app.dpi.window = dpi;
}

static void ui_app_window_opening(void) {
    ui_app_window_dpi();
    ui_app_init_fonts(ui_app.dpi.window);
    ui_app_init_cursors();
    ui_app_timer_1s_id = ui_app.set_timer((uintptr_t)&ui_app_timer_1s_id, 1000);
    ui_app_timer_100ms_id = ui_app.set_timer((uintptr_t)&ui_app_timer_100ms_id, 100);
    rt_assert(ui_app.cursors.arrow != null);
    ui_app.set_cursor(ui_app.cursors.arrow);
    ui_app.canvas = (ui_canvas_t)GetDC(ui_app_window());
    rt_not_null(ui_app.canvas);
    if (ui_app.opened != null) { ui_app.opened(); }
    ui_view.set_text(ui_app.root, "ui_app.root"); // debugging
    ui_app_wm_timer(ui_app_timer_100ms_id);
    ui_app_wm_timer(ui_app_timer_1s_id);
    rt_fatal_if(ReleaseDC(ui_app_window(), ui_app_canvas()) == 0);
    ui_app.canvas = null;
    ui_app.request_layout(); // request layout
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
        mmi->ptMaxTrackSize.x = rt_min(max_w, wa->w);
        mmi->ptMaxTrackSize.y = rt_min(max_h, wa->h);
    }
    mmi->ptMaxSize.x = mmi->ptMaxTrackSize.x;
    mmi->ptMaxSize.y = mmi->ptMaxTrackSize.y;
}

static void ui_app_paint(ui_view_t* view) {
    rt_assert(ui_app_window() != null);
    // crc = {0,0} on minimized windows but paint is still called
    if (ui_app.crc.w > 0 && ui_app.crc.h > 0) { ui_view.paint(view); }
}

static void ui_app_measure_and_layout(ui_view_t* view) {
    // restore from minimized calls ui_app.crc.w,h == 0
    if (ui_app.crc.w > 0 && ui_app.crc.h > 0 && ui_app_window() != null) {
        ui_view.measure(view);
        ui_view.layout(view);
        ui_app_layout_dirty = false;
    }
}

static void ui_app_toast_character(const char* utf8);
static bool ui_app_toast_key_pressed(int64_t key);
static bool ui_app_toast_tap(ui_view_t* v, int32_t ix, bool pressed);

static void ui_app_dispatch_wm_char(ui_view_t* view, const uint16_t* utf16) {
    char utf8[32 + 1];
    int32_t utf8bytes = rt_str.utf8_bytes(utf16, -1);
    rt_swear(utf8bytes < rt_countof(utf8) - 1); // 32 bytes + 0x00
    rt_str.utf16to8(utf8, rt_countof(utf8), utf16, -1);
    utf8[utf8bytes] = 0x00;
    if (ui_app.animating.view != null) {
        ui_app_toast_character(utf8);
    } else {
        ui_view.character(view, utf8);
    }
    ui_app_high_surrogate = 0x0000;
}

static void ui_app_wm_char(ui_view_t* view, const uint16_t* utf16) {
    int32_t utf16chars = rt_str.len16(utf16);
    rt_swear(0 < utf16chars && utf16chars < 4); // wParam is 64bits
    const uint16_t utf16char = utf16[0];
    if (utf16chars == 1 && rt_str.utf16_is_high_surrogate(utf16char)) {
        ui_app_high_surrogate = utf16char;
    } else if (utf16chars == 1 && rt_str.utf16_is_low_surrogate(utf16char)) {
        if (ui_app_high_surrogate != 0) {
            uint16_t utf16_surrogate_pair[3] = {
                ui_app_high_surrogate,
                utf16char,
                0x0000
            };
            ui_app_dispatch_wm_char(view, utf16_surrogate_pair);
        }
    } else {
        ui_app_dispatch_wm_char(view, utf16);
    }
}

static bool ui_app_wm_key_pressed(ui_view_t* v, int64_t key) {
    if (ui_app.animating.view != null) {
        return ui_app_toast_key_pressed(key);
    } else {
        return ui_view.key_pressed(v, key);
    }
}

static bool ui_app_mouse(ui_view_t* v, int32_t m, int64_t f) {
    bool swallow = false;
    // override ui_app_update_mouse_buttons_state() (sic):
    // because mouse message can be from the past
    ui_app.mouse_left   = f & (ui_app.mouse_swapped ? MK_RBUTTON : MK_LBUTTON);
    ui_app.mouse_middle = f & MK_MBUTTON;
    ui_app.mouse_right  = f & (ui_app.mouse_swapped ? MK_LBUTTON : MK_RBUTTON);
    ui_view_t* av = ui_app.animating.view;
    if (m == WM_MOUSEHOVER) {
        ui_view.mouse_hover(av != null && av->mouse_hover != null ? av : v);
    } else if (m == WM_MOUSEMOVE) {
        ui_view.mouse_move(av != null && av->mouse_move != null ? av : v);
    } else if (m == WM_LBUTTONDOWN  ||
               m == WM_LBUTTONUP    ||
               m == WM_MBUTTONDOWN  ||
               m == WM_MBUTTONUP    ||
               m == WM_RBUTTONDOWN  ||
               m == WM_RBUTTONUP) {
        const int i =
             (m == WM_LBUTTONDOWN || m == WM_LBUTTONUP) ? 0 :
            ((m == WM_MBUTTONDOWN || m == WM_MBUTTONUP) ? 1 :
            ((m == WM_RBUTTONDOWN || m == WM_RBUTTONUP) ? 2 : -1));
        rt_swear(i >= 0);
        const int32_t ix = ui_app.mouse_swapped ? 2 - i : i;
        const bool pressed =
            m == WM_LBUTTONDOWN ||
            m == WM_MBUTTONDOWN ||
            m == WM_RBUTTONDOWN;
        if (av != null) {
            // because of "micro" close button:
            swallow = ui_app_toast_tap(ui_app.animating.view, ix, pressed);
        } else {
            if (av != null && av->tap != null) {
                swallow = ui_view.tap(av, ix, pressed);
            } else {
                // tap detector will handle the tap() calling
            }
        }
    } else if (m == WM_LBUTTONDBLCLK ||
               m == WM_MBUTTONDBLCLK ||
               m == WM_RBUTTONDBLCLK) {
        const int i =
             (m == WM_LBUTTONDBLCLK) ? 0 :
            ((m == WM_MBUTTONDBLCLK) ? 1 :
            ((m == WM_RBUTTONDBLCLK) ? 2 : -1));
        rt_swear(i >= 0);
        if (av != null && av->double_tap != null) {
            const int32_t ix = ui_app.mouse_swapped ? 2 - i : i;
            swallow = ui_view.double_tap(av, ix);
        }
        // otherwise tap detector will do the double_tap() call
    } else {
        rt_assert(false, "m: 0x%04X", m);
    }
    return swallow;
}

static void ui_app_show_sys_menu(int32_t x, int32_t y) {
    HMENU sys_menu = GetSystemMenu(ui_app_window(), false);
    if (sys_menu != null) {
        // TPM_RIGHTBUTTON means both left and right click to select menu item
        const DWORD flags = TPM_LEFTALIGN | TPM_TOPALIGN | TPM_RIGHTBUTTON |
                            TPM_RETURNCMD | TPM_VERPOSANIMATION;
        int32_t sys_cmd = TrackPopupMenu(sys_menu, flags, x, y, 0,
                                         ui_app_window(), null);
        if (sys_cmd != 0) {
            ui_app_post_message(WM_SYSCOMMAND, sys_cmd, 0);
        }
    }
}

static int32_t ui_app_nc_mouse_message(int32_t m) {
    switch (m) {
        case WM_NCMOUSEMOVE     : return WM_MOUSEMOVE;
        case WM_NCLBUTTONDOWN   : return WM_LBUTTONDOWN;
        case WM_NCLBUTTONUP     : return WM_LBUTTONUP;
        case WM_NCLBUTTONDBLCLK : return WM_LBUTTONDBLCLK;
        case WM_NCMBUTTONDOWN   : return WM_MBUTTONDOWN;
        case WM_NCMBUTTONUP     : return WM_MBUTTONUP;
        case WM_NCMBUTTONDBLCLK : return WM_MBUTTONDBLCLK;
        case WM_NCRBUTTONDOWN   : return WM_RBUTTONDOWN;
        case WM_NCRBUTTONUP     : return WM_RBUTTONUP;
        case WM_NCRBUTTONDBLCLK : return WM_RBUTTONDBLCLK;
        default: rt_swear(false, "fix me m: %d", m);
    }
    return -1;
}

static bool ui_app_nc_mouse_buttons(int32_t m, int64_t wp, int64_t lp) {
    bool swallow = false;
    POINT screen = {GET_X_LPARAM(lp), GET_Y_LPARAM(lp)};
    POINT client = screen;
    ScreenToClient(ui_app_window(), &client);
    ui_app.mouse = ui_app_point2ui(&client);
    const bool inside = ui_view.inside(ui_app.caption, &ui_app.mouse);
    if (!ui_view.is_hidden(ui_app.caption) && inside) {
        uint16_t lr = ui_app.mouse_swapped ? WM_NCLBUTTONDOWN : WM_NCRBUTTONDOWN;
        if (m == lr) {
//          rt_println("WM_NC*BUTTONDOWN %d %d", ui_app.mouse.x, ui_app.mouse.y);
            swallow = true;
            ui_app_show_sys_menu(screen.x, screen.y);
        }
    } else {
        swallow = ui_app_mouse(ui_app.root, ui_app_nc_mouse_message(m), wp);
    }
    return swallow;
}

enum { ui_app_animation_steps = 63 };

static void ui_app_toast_paint(void) {
    static ui_bitmap_t image_dark;
    if (image_dark.texture == null) {
        uint8_t pixels[4] = { 0x3F, 0x3F, 0x3F };
        ui_gdi.bitmap_init(&image_dark, 1, 1, 3, pixels);
    }
    static ui_bitmap_t image_light;
    if (image_dark.texture == null) {
        uint8_t pixels[4] = { 0xC0, 0xC0, 0xC0 };
        ui_gdi.bitmap_init(&image_light, 1, 1, 3, pixels);
    }
    ui_view_t* av = ui_app.animating.view;
    if (av != null) {
        ui_view.measure(av);
        bool hint = ui_app.animating.x >= 0 && ui_app.animating.y >= 0;
        const int32_t em_w = av->fm->em.w;
        const int32_t em_h = av->fm->em.h;
        if (!hint) {
            rt_assert(0 <= ui_app.animating.step && ui_app.animating.step < ui_app_animation_steps);
            int32_t step = ui_app.animating.step - (ui_app_animation_steps - 1);
            av->y = av->h * step / (ui_app_animation_steps - 1);
//          rt_println("step=%d of %d y=%d", ui_app.animating.step,
//                  ui_app_toast_steps, av->y);
            ui_app_measure_and_layout(av);
            // dim main window (as `disabled`):
            fp64_t alpha = rt_min(0.40, 0.40 * ui_app.animating.step / (fp64_t)ui_app_animation_steps);
            ui_gdi.alpha(0, 0, ui_app.crc.w, ui_app.crc.h,
                         0, 0, image_dark.w, image_dark.h,
                        &image_dark, alpha);
            av->x = (ui_app.root->w - av->w) / 2;
//          rt_println("ui_app.animating.y: %d av->y: %d",
//                  ui_app.animating.y, av->y);
        } else {
            av->x = ui_app.animating.x;
            av->y = ui_app.animating.y;
            ui_app_measure_and_layout(av);
            int32_t mx = ui_app.root->w - av->w - em_w;
            int32_t cx = ui_app.animating.x - av->w / 2;
            av->x = rt_min(mx, rt_max(0, cx));
            av->y = rt_min(
                ui_app.root->h - em_h, rt_max(0, ui_app.animating.y));
//          rt_println("ui_app.animating.y: %d av->y: %d",
//                  ui_app.animating.y, av->y);
        }
        int32_t x = av->x - em_w / 4;
        int32_t y = av->y - em_h / 8;
        int32_t w = av->w + em_w / 2;
        int32_t h = av->h + em_h / 4;
        int32_t radius = em_w / 2;
        if (radius % 2 == 0) { radius++; }
        ui_color_t color = ui_theme.is_app_dark() ?
            ui_color_rgb(45, 45, 48) : // TODO: hard coded
            ui_colors.get_color(ui_color_id_button_face);
        ui_color_t tint = ui_colors.interpolate(color, ui_colors.yellow, 0.5f);
        ui_gdi.rounded(x, y, w, h, radius, tint, tint);
        if (!hint) { av->y += em_h / 4; }
        ui_app_paint(av);
        if (!hint) {
            if (av->y == em_h / 4) {
                // micro "close" toast button:
                int32_t r = av->x + av->w;
                const int32_t tx = r - em_w / 2;
                const int32_t ty = 0;
                const ui_gdi_ta_t ta = {
                    .fm = &ui_app.fm.prop.normal,
                    .color = ui_color_undefined,
                    .color_id = ui_color_id_window_text
                };
                ui_gdi.text(&ta, tx, ty, "%s",
                                 rt_glyph_multiplication_sign);
            }
        }
    }
}

static void ui_app_toast_cancel(void) {
    if (ui_app.animating.view != null) {
        if (ui_app.animating.view->type == ui_view_mbx) {
            ui_mbx_t* mx = (ui_mbx_t*)ui_app.animating.view;
            if (mx->option < 0 && mx->callback != null) {
                mx->callback(&mx->view);
            }
        }
        ui_app.animating.view->parent = null;
        ui_app.animating.step = 0;
        ui_app.animating.view = null;
        ui_app.animating.time = 0;
        ui_app.animating.x = -1;
        ui_app.animating.y = -1;
        if (ui_app.animating.focused != null) {
            ui_view.set_focus(ui_app.animating.focused->focusable &&
               !ui_view.is_hidden(ui_app.animating.focused) &&
               !ui_view.is_disabled(ui_app.animating.focused) ?
                ui_app.animating.focused : null);
            ui_app.animating.focused = null;
        } else {
            ui_view.set_focus(null);
        }
        ui_app.request_redraw();
    }
}

static bool ui_app_toast_tap(ui_view_t* v, int32_t ix, bool pressed) {
    bool swallow = false;
    rt_swear(v == ui_app.animating.view);
    if (pressed) {
        const ui_fm_t* fm = v->fm;
        const int32_t right = v->x + v->w;
        const int32_t x = right - fm->em.w / 2;
        const int32_t mx = ui_app.mouse.x;
        const int32_t my = ui_app.mouse.y;
        // micro close button which is not a button
        if (x <= mx && mx <= x + fm->em.w && 0 <= my && my <= fm->em.h) {
            ui_app_toast_cancel();
        }
    }
    if (ui_app.animating.view != null) { // could have been canceled above
        swallow = ui_view.tap(v, ix, pressed); // TODO: do we need it?
    }
    return swallow;
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

static bool ui_app_toast_key_pressed(int64_t key) {
    if (ui_app.animating.view != null && key == 033) { // ESC traditionally in octal
        ui_app_toast_cancel();
        ui_app.show_toast(null, 0);
        return true;
    } else {
        return ui_view.key_pressed(ui_app.animating.view, key);
    }
}

static void ui_app_toast_dim(int32_t step) {
    ui_app.animating.step = step;
    ui_app.request_redraw();
    UpdateWindow(ui_app_window());
}

static void ui_app_animate_step(ui_app_animate_function_t f, int32_t step, int32_t steps) {
    // calls function(0..step-1) exactly step times
    bool cancel = false;
    if (f != null && f != ui_app_animate.f && step == 0 && steps > 0) {
        // start animated_groot
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
    if (cancel) {
        if (ui_app_animate.timer != 0) {
            ui_app.kill_timer(ui_app_animate.timer);
        }
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
    v->color = ui_colors.get_color(v->color_id);
    if (v->background_id > 0) {
        v->background = ui_colors.get_color(v->background_id);
    }
    if (!ui_color_is_undefined(v->background) &&
        !ui_color_is_transparent(v->background)) {
        ui_gdi.fill(v->x, v->y, v->w, v->h, v->background);
    }
}

static void ui_app_view_layout(void) {
    rt_not_null(ui_app.window);
    rt_not_null(ui_app.canvas);
    if (ui_app.no_decor) {
        ui_app.root->x = ui_app.border.w;
        ui_app.root->y = ui_app.border.h;
        ui_app.root->w = ui_app.crc.w - ui_app.border.w * 2;
        ui_app.root->h = ui_app.crc.h - ui_app.border.h * 2;
    } else {
        ui_app.root->x = 0;
        ui_app.root->y = 0;
        ui_app.root->w = ui_app.crc.w;
        ui_app.root->h = ui_app.crc.h;
    }
    ui_app_measure_and_layout(ui_app.root);
}

static void ui_app_view_active_frame_paint(void) {
    ui_color_t c = ui_app.is_active() ?
        ui_colors.get_color(ui_color_id_highlight) : // ui_colors.btn_hover_highlight
        ui_colors.get_color(ui_color_id_inactive_title);
    rt_assert(ui_app.border.w == ui_app.border.h);
    const int32_t w = ui_app.wrc.w;
    const int32_t h = ui_app.wrc.h;
    for (int32_t i = 0; i < ui_app.border.w; i++) {
        ui_gdi.frame(i, i, w - i * 2, h - i * 2, c);
    }
}

static void ui_app_paint_stats(void) {
    if (ui_app.paint_count % 128 == 0) { ui_app.paint_max = 0; }
    ui_app.paint_time = rt_clock.seconds() - ui_app.now;
    ui_app.paint_max = rt_max(ui_app.paint_time, ui_app.paint_max);
    if (ui_app.paint_avg == 0) {
        ui_app.paint_avg = ui_app.paint_time;
    } else { // EMA over 32 paint() calls
        ui_app.paint_avg = ui_app.paint_avg * (1.0 - 1.0 / 32.0) +
                        ui_app.paint_time / 32.0;
    }
    static fp64_t first_paint;
    if (first_paint == 0) { first_paint = ui_app.now; }
    fp64_t since_first_paint = ui_app.now - first_paint;
    if (since_first_paint > 0) {
        double fps = (double)ui_app.paint_count / since_first_paint;
        if (ui_app.paint_fps == 0) {
            ui_app.paint_fps = fps;
        } else {
            ui_app.paint_fps = ui_app.paint_fps * (1.0 - 1.0 / 32.0) + fps / 32.0;
        }
    }
    if (ui_app.paint_last == 0) {
        ui_app.paint_dt_min = 1.0 / 60.0; // 60Hz monitor
    } else {
        fp64_t since_last = ui_app.now - ui_app.paint_last;
        if (since_last > 1.0 / 120.0) { // 240Hz monitor
            ui_app.paint_dt_min = rt_min(ui_app.paint_dt_min, since_last);
        }
//      rt_println("paint_dt_min: %.6f since_last: %.6f",
//              ui_app.paint_dt_min, since_last);
    }
    ui_app.paint_last = ui_app.now;
}

static void ui_app_paint_on_canvas(HDC hdc) {
    ui_canvas_t canvas = ui_app.canvas;
    ui_app.canvas = (ui_canvas_t)hdc;
    ui_app_update_crc();
    if (ui_app_layout_dirty) {
        ui_app_view_layout();
    }
    ui_gdi.begin(null);
    ui_app_paint(ui_app.root);
    if (ui_app.animating.view != null) { ui_app_toast_paint(); }
    // active frame on top of everything:
    if (ui_app.no_decor && !ui_app.is_full_screen &&
        !ui_app.is_maximized()) {
        ui_app_view_active_frame_paint();
    }
    ui_gdi.end();
    ui_app.paint_count++;
    ui_app.canvas = canvas;
    ui_app_paint_stats();
}

static void ui_app_wm_paint(void) {
    // it is possible to receive WM_PAINT when window is not closed
    if (ui_app.window != null) {
        PAINTSTRUCT ps = {0};
        BeginPaint(ui_app_window(), &ps);
        ui_app.prc = ui_app_rect2ui(&ps.rcPaint);
//      rt_println("%d,%d %dx%d", ui_app.prc.x, ui_app.prc.y, ui_app.prc.w, ui_app.prc.h);
        ui_app_paint_on_canvas(ps.hdc);
        EndPaint(ui_app_window(), &ps);
    }
}

// about (x,y) being (-32000,-32000) see:
// https://chromium.googlesource.com/chromium/src.git/+/62.0.3178.1/ui/views/win/hwnd_message_handler.cc#1847

static void ui_app_window_position_changed(const WINDOWPOS* wp) {
    ui_app.root->state.hidden = !IsWindowVisible(ui_app_window());
    const bool moved  = (wp->flags & SWP_NOMOVE) == 0;
    const bool sized  = (wp->flags & SWP_NOSIZE) == 0;
    const bool hiding = (wp->flags & SWP_HIDEWINDOW) != 0 ||
                        (wp->x == -32000 && wp->y == -32000);
    HMONITOR monitor = MonitorFromWindow(ui_app_window(), MONITOR_DEFAULTTONULL);
    if (!ui_app.root->state.hidden && (moved || sized) &&
        !hiding && monitor != null) {
        RECT wrc = ui_app_ui2rect(&ui_app.wrc);
        rt_fatal_win32err(GetWindowRect(ui_app_window(), &wrc));
        ui_app.wrc = ui_app_rect2ui(&wrc);
        ui_app_update_mi(&ui_app.wrc, MONITOR_DEFAULTTONEAREST);
        ui_app_update_crc();
        if (ui_app_timer_1s_id != 0) { ui_app.request_layout(); }
    }
}

static void ui_app_setting_change(uintptr_t wp, uintptr_t lp) {
    // wp: SPI_SETWORKAREA ... SPI_SETDOCKMOVING
    //     SPI_GETACTIVEWINDOWTRACKING ... SPI_SETGESTUREVISUALIZATION
    if (wp == SPI_SETLOGICALDPIOVERRIDE) {
        ui_app_init_fonts(ui_app.dpi.window); // font scale changed
        ui_app.request_layout();
    } else if (lp != 0 &&
       (strcmp((const char*)lp, "ImmersiveColorSet") == 0 ||
        wcscmp((const uint16_t*)lp, L"ImmersiveColorSet") == 0)) {
        // expected:
        // SPI_SETICONTITLELOGFONT 0x22 ?
        // SPI_SETNONCLIENTMETRICS 0x2A ?
//      rt_println("wp: 0x%08X", wp);
        // actual wp == 0x0000
        ui_theme.refresh();
    } else if (wp == 0 && lp != 0 && strcmp((const char*)lp, "intl") == 0) {
        rt_println("wp: 0x%04X", wp); // SPI_SETLOCALEINFO 0x24 ?
        uint16_t ln[LOCALE_NAME_MAX_LENGTH + 1];
        int32_t n = GetUserDefaultLocaleName(ln, rt_countof(ln));
        rt_fatal_if(n <= 0);
        uint16_t rln[LOCALE_NAME_MAX_LENGTH + 1];
        n = ResolveLocaleName(ln, rln, rt_countof(rln));
        rt_fatal_if(n <= 0);
        LCID lc_id = LocaleNameToLCID(rln, LOCALE_ALLOW_NEUTRAL_NAMES);
        rt_fatal_win32err(SetThreadLocale(lc_id));
    }
}

static void ui_app_show_task_bar(bool show) {
    HWND taskbar = FindWindowA("Shell_TrayWnd", null);
    if (taskbar != null) {
        ShowWindow(taskbar, show ? SW_SHOW : SW_HIDE);
        UpdateWindow(taskbar);
    }
}

static bool ui_app_click_detector(uint32_t msg, WPARAM wp, LPARAM lp) {
    bool swallow = false;
    enum { tap = 1, long_press = 2, double_tap = 3 };
    // TODO: click detector does not handle WM_NCLBUTTONDOWN, ...
    //       it can be modified to do so if needed
    #pragma push_macro("ui_set_timer")
    #pragma push_macro("ui_kill_timer")
    #pragma push_macro("ui_timers_done")

    #define ui_set_timer(t, ms) do {                 \
        rt_assert(t == 0);                           \
        t = ui_app_timer_set((uintptr_t)&t, ms);     \
    } while (0)

    #define ui_kill_timer(t) do {                    \
        if (t != 0) { ui_app_timer_kill(t); t = 0; } \
    } while (0)

    #define ui_timers_done(ix) do {                  \
        clicked[ix] = 0;                             \
        pressed[ix] = false;                         \
        click_at[ix] = (ui_point_t){0, 0};           \
        ui_kill_timer(timer_p[ix]);                  \
        ui_kill_timer(timer_d[ix]);                  \
    } while (0)

    // This function should work regardless to CS_BLKCLK being present
    // 0: Left, 1: Middle, 2: Right
    static ui_point_t click_at[3];
    static fp64_t     clicked[3]; // click time
    static bool       pressed[3];
    static ui_timer_t timer_d[3]; // double tap
    static ui_timer_t timer_p[3]; // long press
    bool up = false;
    int32_t ix = -1;
    int32_t m = 0;
    switch (msg) {
        case WM_LBUTTONDOWN  : ix = 0; m = tap;        break;
        case WM_MBUTTONDOWN  : ix = 1; m = tap;        break;
        case WM_RBUTTONDOWN  : ix = 2; m = tap;        break;
        case WM_LBUTTONDBLCLK: ix = 0; m = double_tap; break;
        case WM_MBUTTONDBLCLK: ix = 1; m = double_tap; break;
        case WM_RBUTTONDBLCLK: ix = 2; m = double_tap; break;
        case WM_LBUTTONUP    : ix = 0; m = tap; up = true; break;
        case WM_MBUTTONUP    : ix = 1; m = tap; up = true; break;
        case WM_RBUTTONUP    : ix = 2; m = tap; up = true; break;
    }
    if (msg == WM_TIMER) { // long press && double tap
        for (int i = 0; i < 3; i++) {
            if (wp == timer_p[i]) {
                ui_app.mouse = (ui_point_t){ click_at[i].x, click_at[i].y };
                ui_view.long_press(ui_app.root, i);
//              rt_println("timer_p[%d] _d && _p timers done", i);
                ui_timers_done(i);
            }
            if (wp == timer_d[i]) {
//              rt_println("timer_p[%d] _d && _p timers done", i);
                ui_timers_done(i);
            }
        }
    }
    if (ix != -1) {
        ui_app.show_hint(null, -1, -1, 0); // dismiss hint on any click
        const int32_t double_click_msec = (int32_t)GetDoubleClickTime();
        const fp64_t  double_click_dt = double_click_msec / 1000.0; // seconds
//      rt_println("double_click_msec: %d double_click_dt: %.3fs",
//               double_click_msec, double_click_dt);
        const int double_click_x = GetSystemMetrics(SM_CXDOUBLECLK) / 2;
        const int double_click_y = GetSystemMetrics(SM_CYDOUBLECLK) / 2;
        ui_point_t pt = { GET_X_LPARAM(lp), GET_Y_LPARAM(lp) };
        if (m == tap && !up) {
            swallow = ui_view.tap(ui_app.root, ix, !up);
            if (ui_app.now  - clicked[ix]  <= double_click_dt &&
                abs(pt.x - click_at[ix].x) <= double_click_x &&
                abs(pt.y - click_at[ix].y) <= double_click_y) {
                ui_app.mouse = (ui_point_t){ click_at[ix].x, click_at[ix].y };
                ui_view.double_tap(ui_app.root, ix);
//              rt_println("timer_p[%d] _d && _p timers done", ix);
                ui_timers_done(ix);
            } else {
//              rt_println("timer_p[%d] _d && _p timers done", ix);
                ui_timers_done(ix); // clear timers
                clicked[ix]  = ui_app.now;
                click_at[ix] = pt;
                pressed[ix]  = true;
//              rt_println("clicked[%d] := %.1f %d,%d pressed[%d] := true",
//                          ix, clicked[ix], pt.x, pt.y, ix);
                if ((ui_app_wc.style & CS_DBLCLKS) == 0) {
                    // only if Windows are not detecting DLBCLKs
//                  rt_println("ui_set_timer(timer_d[%d])", ix);
                    ui_set_timer(timer_d[ix], double_click_msec);  // 0.5s
                }
                ui_set_timer(timer_p[ix], double_click_msec * 3 / 4); // 0.375s
            }
        } else if (up) {
            fp64_t since_clicked = ui_app.now - clicked[ix];
//          rt_println("pressed[%d]: %d %.3f", ix, pressed[ix], since_clicked);
            // only if Windows are not detecting DLBCLKs
            if ((ui_app_wc.style & CS_DBLCLKS) == 0 &&
                 pressed[ix] && since_clicked > double_click_dt) {
                ui_view.double_tap(ui_app.root, ix);
//              rt_println("timer_p[%d] _d && _p timers done", ix);
                ui_timers_done(ix);
            }
            swallow = ui_view.tap(ui_app.root, ix, !up);
            ui_kill_timer(timer_p[ix]); // long press is not the case
        } else if (m == double_tap) {
            rt_assert((ui_app_wc.style & CS_DBLCLKS) != 0);
            swallow = ui_view.double_tap(ui_app.root, ix);
            ui_timers_done(ix);
//          rt_println("timer_p[%d] _d && _p timers done", ix);
        }
    }
    #pragma pop_macro("ui_timers_done")
    #pragma pop_macro("ui_kill_timer")
    #pragma pop_macro("ui_set_timer")
    return swallow;
}

static int64_t ui_app_root_hit_test(const ui_view_t* v, ui_point_t pt) {
    rt_swear(v == ui_app.root);
    if (ui_app.no_decor) {
        rt_assert(ui_app.border.w == ui_app.border.h);
        // on 96dpi monitors ui_app.border is 1x1
        // make it easier for the user to resize window
        int32_t border = rt_max(4, ui_app.border.w * 2);
        if (ui_app.animating.view != null) {
            return ui.hit_test.client; // message box or toast is up
        } else if (!ui_view.is_hidden(&ui_caption.view) &&
                    ui_view.inside(&ui_caption.view, &pt)) {
            return ui_caption.view.hit_test(&ui_caption.view, pt);
        } else if (ui_app.is_maximized()) {
            int64_t ht = ui_view.hit_test(ui_app.content, pt);
            return ht == ui.hit_test.nowhere ? ui.hit_test.client : ht;
        } else if (ui_app.is_full_screen) {
            return ui.hit_test.client;
        } else if (pt.x < border && pt.y < border) {
            return ui.hit_test.top_left;
        } else if (pt.x > ui_app.crc.w - border && pt.y < border) {
            return ui.hit_test.top_right;
        } else if (pt.y < border) {
            return ui.hit_test.top;
        } else if (pt.x > ui_app.crc.w - border &&
                   pt.y > ui_app.crc.h - border) {
            return ui.hit_test.bottom_right;
        } else if (pt.x < border && pt.y > ui_app.crc.h - border) {
            return ui.hit_test.bottom_left;
        } else if (pt.x < border) {
            return ui.hit_test.left;
        } else if (pt.x > ui_app.crc.w - border) {
            return ui.hit_test.right;
        } else if (pt.y > ui_app.crc.h - border) {
            return ui.hit_test.bottom;
        } else {
            // drop down to content hit test
        }
    }
    return ui.hit_test.nowhere;
}

static void ui_app_wm_activate(int64_t wp) {
    bool activate = LOWORD(wp) != WA_INACTIVE;
    if (!IsWindowVisible(ui_app_window()) && activate) {
        ui_app.show_window(ui.visibility.restore);
        SwitchToThisWindow(ui_app_window(), true);
    }
    ui_app.request_redraw(); // needed for windows changing active frame color
}

static void ui_app_update_mouse_buttons_state(void) {
    ui_app.mouse_swapped = GetSystemMetrics(SM_SWAPBUTTON) != 0;
    ui_app.mouse_left  = (GetAsyncKeyState(ui_app.mouse_swapped ?
                          VK_RBUTTON : VK_LBUTTON) & 0x8000) != 0;
    ui_app.mouse_right = (GetAsyncKeyState(ui_app.mouse_swapped ?
                          VK_LBUTTON : VK_RBUTTON) & 0x8000) != 0;
}

static int64_t ui_app_wm_nc_hit_test(int64_t wp, int64_t lp) {
    ui_point_t pt = { GET_X_LPARAM(lp) - ui_app.wrc.x,
                      GET_Y_LPARAM(lp) - ui_app.wrc.y };
    int64_t ht = ui_view.hit_test(ui_app.root, pt);
    if (ht != ui.hit_test.nowhere) {
        return ht;
    } else {
        return DefWindowProcW(ui_app_window(), WM_NCHITTEST, wp, lp);
    }
}

static int64_t ui_app_wm_sys_key_down(int64_t wp, int64_t lp) {
    ui_app_alt_ctrl_shift(true, wp);
    if (ui_app_wm_key_pressed(ui_app.root, wp) || wp == VK_MENU) {
        return 0; // no DefWindowProcW()
    } else {
        return DefWindowProcW(ui_app_window(), WM_SYSKEYDOWN, wp, lp);
    }
}

static void ui_app_wm_set_focus(void) {
    if (!ui_app.root->state.hidden) {
        rt_assert(GetActiveWindow() == ui_app_window());
        if (ui_app.focus != null && ui_app.focus->focus_lost != null) {
            ui_app.focus->focus_gained(ui_app.focus);
        }
    }
}

static void ui_app_wm_kill_focus(void) {
    if (!ui_app.root->state.hidden &&
        ui_app.focus != null &&
        ui_app.focus->focus_lost != null) {
        ui_app.focus->focus_lost(ui_app.focus);
    }
}

static int64_t ui_app_wm_nc_calculate_size(int64_t wp, int64_t lp) {
//  NCCALCSIZE_PARAMS* szp = (NCCALCSIZE_PARAMS*)lp;
//  rt_println("WM_NCCALCSIZE wp: %lld is_max: %d (%d %d %d %d) (%d %d %d %d) (%d %d %d %d)",
//      wp, ui_app.is_maximized(),
//      szp->rgrc[0].left, szp->rgrc[0].top, szp->rgrc[0].right, szp->rgrc[0].bottom,
//      szp->rgrc[1].left, szp->rgrc[1].top, szp->rgrc[1].right, szp->rgrc[1].bottom,
//      szp->rgrc[2].left, szp->rgrc[2].top, szp->rgrc[2].right, szp->rgrc[2].bottom);
    // adjust window client area frame for no_decor windows
    if (wp == true && ui_app.no_decor && !ui_app.is_maximized()) {
        return 0;
    } else {
        return DefWindowProcW(ui_app_window(), WM_NCCALCSIZE, wp, lp);
    }
}

static int64_t ui_app_wm_get_dpi_scaled_size(int64_t wp) {
    // sent before WM_DPICHANGED
    #ifdef UI_APP_DEBUG
        int32_t dpi = wp;
        SIZE* sz = (SIZE*)lp; // in/out
        ui_point_t cell = { sz->cx, sz->cy };
        rt_println("WM_GETDPISCALEDSIZE dpi %d := %d "
            "size %d,%d *may/must* be adjusted",
            ui_app.dpi.window, dpi, cell.x, cell.y);
    #else
        (void)wp; // unused
    #endif
    if (ui_app_timer_1s_id != 0 && !ui_app.root->state.hidden) {
        ui_app.request_layout();
    }
    // IMPORTANT: return true because:
    // "Returning TRUE indicates that a new size has been computed.
    //  Returning FALSE indicates that the message will not be handled,
    //  and the default linear DPI scaling will apply to the window."
    // https://learn.microsoft.com/en-us/windows/win32/hidpi/wm-getdpiscaledsize
    return true;
}

static void ui_app_wm_dpi_changed(void) {
    ui_app_window_dpi();
    ui_app_init_fonts(ui_app.dpi.window);
    if (ui_app_timer_1s_id != 0 && !ui_app.root->state.hidden) {
        ui_app.request_layout();
    } else {
        ui_app_layout_dirty = true;
    }
}

static bool ui_app_wm_sys_command(int64_t wp, int64_t lp) {
    uint16_t sys_cmd = (uint16_t)(wp & 0xFF0);
//  rt_println("WM_SYSCOMMAND wp: 0x%08llX lp: 0x%016llX %lld sys: 0x%04X",
//          wp, lp, lp, sys_cmd);
    if (sys_cmd == SC_MINIMIZE && ui_app.hide_on_minimize) {
        ui_app.show_window(ui.visibility.min_na);
        ui_app.show_window(ui.visibility.hide);
    } else  if (sys_cmd == SC_MINIMIZE && ui_app.no_decor) {
        ui_app.show_window(ui.visibility.min_na);
    }
//  if (sys_cmd == SC_KEYMENU) { rt_println("SC_KEYMENU lp: %lld", lp); }
    // If the selection is in menu handle the key event
    if (sys_cmd == SC_KEYMENU && lp != 0x20) {
        return true; // handled: This prevents the error/beep sound
    }
    if (sys_cmd == SC_MAXIMIZE && ui_app.no_decor) {
        return true; // handled: prevent maximizing no decorations window
    }
//  if (sys_cmd == SC_MOUSEMENU) {
//      rt_println("SC_KEYMENU.SC_MOUSEMENU 0x%00llX %lld", wp, lp);
//  }
    return false; // drop down to to DefWindowProc
}

static void ui_app_wm_window_position_changing(int64_t wp, int64_t lp) {
    #ifdef UI_APP_DEBUG // TODO: ui_app.debug.trace.window_position?
        WINDOWPOS* pos = (WINDOWPOS*)lp;
        rt_println("WM_WINDOWPOSCHANGING flags: 0x%08X", pos->flags);
        if (pos->flags & SWP_SHOWWINDOW) {
            rt_println("SWP_SHOWWINDOW");
        } else if (pos->flags & SWP_HIDEWINDOW) {
            rt_println("SWP_HIDEWINDOW");
        }
    #else
        (void)wp; // unused
        (void)lp; // unused
    #endif
}

static bool ui_app_wm_mouse(int32_t m, int64_t wp, int64_t lp) {
    // note: x, y is already in client coordinates
    ui_app.mouse.x = GET_X_LPARAM(lp);
    ui_app.mouse.y = GET_Y_LPARAM(lp);
    return ui_app_mouse(ui_app.root, m, wp);
}

static void ui_app_wm_mouse_wheel(bool vertical, int64_t wp) {
    if (vertical) {
        ui_point_t dx_dy = { 0, GET_WHEEL_DELTA_WPARAM(wp) };
        ui_view.mouse_scroll(ui_app.root, dx_dy);
    } else {
        ui_point_t dx_dy = { GET_WHEEL_DELTA_WPARAM(wp), 0 };
        ui_view.mouse_scroll(ui_app.root, dx_dy);
    }
}

static void ui_app_wm_input_language_change(uint64_t wp) {
    #ifdef UI_APP_TRACE_WM_INPUT_LANGUAGE_CHANGE
    static struct { uint8_t charset; const char* name; } cs[] = {
        { ANSI_CHARSET       ,     "ANSI_CHARSET       " },
        { DEFAULT_CHARSET    ,     "DEFAULT_CHARSET    " },
        { SYMBOL_CHARSET     ,     "SYMBOL_CHARSET     " },
        { MAC_CHARSET        ,     "MAC_CHARSET        " },
        { SHIFTJIS_CHARSET   ,     "SHIFTJIS_CHARSET   " },
        { HANGEUL_CHARSET    ,     "HANGEUL_CHARSET    " },
        { HANGUL_CHARSET     ,     "HANGUL_CHARSET     " },
        { GB2312_CHARSET     ,     "GB2312_CHARSET     " },
        { CHINESEBIG5_CHARSET,     "CHINESEBIG5_CHARSET" },
        { OEM_CHARSET        ,     "OEM_CHARSET        " },
        { JOHAB_CHARSET      ,     "JOHAB_CHARSET      " },
        { HEBREW_CHARSET     ,     "HEBREW_CHARSET     " },
        { ARABIC_CHARSET     ,     "ARABIC_CHARSET     " },
        { GREEK_CHARSET      ,     "GREEK_CHARSET      " },
        { TURKISH_CHARSET    ,     "TURKISH_CHARSET    " },
        { VIETNAMESE_CHARSET ,     "VIETNAMESE_CHARSET " },
        { THAI_CHARSET       ,     "THAI_CHARSET       " },
        { EASTEUROPE_CHARSET ,     "EASTEUROPE_CHARSET " },
        { RUSSIAN_CHARSET    ,     "RUSSIAN_CHARSET    " },
        { BALTIC_CHARSET     ,     "BALTIC_CHARSET     " }
    };
    for (int32_t i = 0; i < rt_countof(cs); i++) {
        if (cs[i].charset == wp) {
            rt_println("WM_INPUTLANGCHANGE: 0x%08X %s", wp, cs[i].name);
            break;
        }
    }
    #else
        (void)wp; // unused
    #endif
}

static void ui_app_decode_keyboard(int32_t m, int64_t wp, int64_t lp) {
    // https://learn.microsoft.com/en-us/windows/win32/inputdev/about-keyboard-input#keystroke-message-flags
    rt_swear(m == WM_KEYDOWN || m == WM_SYSKEYDOWN ||
          m == WM_KEYUP   || m == WM_SYSKEYUP);
    uint16_t vk_code   = LOWORD(wp);
    uint16_t key_flags = HIWORD(lp);
    uint16_t scan_code = LOBYTE(key_flags);
    if ((key_flags & KF_EXTENDED) == KF_EXTENDED) {
        scan_code = MAKEWORD(scan_code, 0xE0);
    }
    // previous key-state flag, 1 on autorepeat
    bool was_key_down = (key_flags & KF_REPEAT) == KF_REPEAT;
    // repeat count, > 0 if several key down messages was combined into one
    uint16_t repeat_count = LOWORD(lp);
    // transition-state flag, 1 on key up
    bool is_key_released = (key_flags & KF_UP) == KF_UP;
    // if we want to distinguish these keys:
    switch (vk_code) {
        case VK_SHIFT:   // converts to VK_LSHIFT or VK_RSHIFT
        case VK_CONTROL: // converts to VK_LCONTROL or VK_RCONTROL
        case VK_MENU:    // converts to VK_LMENU or VK_RMENU
            vk_code = LOWORD(MapVirtualKeyW(scan_code, MAPVK_VSC_TO_VK_EX));
            break;
        default: break;
    }
    static BYTE keyboard_state[256];
    uint16_t utf16[3] = {0};
    rt_fatal_win32err(GetKeyboardState(keyboard_state));
    // HKL low word Language Identifier
    //     high word device handle to the physical layout of the keyboard
    const HKL kl = GetKeyboardLayout(0);
    // Map virtual key to scan code
    UINT vk = MapVirtualKeyEx(scan_code, MAPVK_VSC_TO_VK_EX, kl);
//  rt_println("virtual_key: %02X keyboard layout: %08X",
//              virtual_key, kl);
    memset(ui_app_decoded_released, 0x00, sizeof(ui_app_decoded_released));
    memset(ui_app_decoded_pressed,  0x00, sizeof(ui_app_decoded_pressed));
    // Translate scan code to character
    int32_t r = ToUnicodeEx(vk, scan_code, keyboard_state,
                            utf16, rt_countof(utf16), 0, kl);
    if (r > 0) {
        rt_static_assertion(rt_countof(ui_app_decoded_pressed) ==
                            rt_countof(ui_app_decoded_released));
        enum { capacity = (int32_t)rt_countof(ui_app_decoded_released) };
        char* utf8 = is_key_released ?
            ui_app_decoded_released : ui_app_decoded_pressed;
        rt_str.utf16to8(utf8, capacity, utf16, -1);
        if (ui_app_trace_utf16_keyboard_input) {
            rt_println("0x%04X%04X released: %d down: %d repeat: %d \"%s\"",
                    utf16[0], utf16[1], is_key_released, was_key_down,
                    repeat_count, utf8);
        }
    } else if (r == 0) {
        // The specified virtual key has no translation for the
        // current state of the keyboard. (E.g. arrows, enter etc)
    } else {
        rt_assert(r < 0);
        // The specified virtual key is a dead key character (accent or diacritic).
        if (ui_app_trace_utf16_keyboard_input) { rt_println("dead key"); }
    }
}

static void ui_app_ime_composition(int64_t lp) {
    if (lp & GCS_RESULTSTR) {
        HIMC imc = ImmGetContext(ui_app_window());
        if (imc != null) {
            char utf8[16];
            uint16_t utf16[4] = {0};
            uint32_t bytes = ImmGetCompositionStringW(imc, GCS_RESULTSTR, null, 0);
            uint32_t count = bytes / sizeof(uint16_t);
            if (0 < count && count < rt_countof(utf16) - 1) {
                ImmGetCompositionStringW(imc, GCS_RESULTSTR, utf16, bytes);
                utf16[count] = 0x00;
                rt_str.utf16to8(utf8, rt_countof(utf8), utf16, -1);
                rt_println("bytes: %d 0x%04X 0x%04X %s", bytes, utf16[0], utf16[1], utf8);
            }
            rt_fatal_win32err(ImmReleaseContext(ui_app_window(), imc));
        }
    }
}

static LRESULT CALLBACK ui_app_window_proc(HWND window, UINT message,
        WPARAM w_param, LPARAM l_param) {
    ui_app.now = rt_clock.seconds();
    if (ui_app.window == null) {
        ui_app.window = (ui_window_t)window;
    } else {
        rt_assert(ui_app_window() == window);
    }
    rt_work_queue.dispatch(&ui_app_queue);
    ui_app_update_wt_timeout(); // because head might have changed
    const int32_t m  = (int32_t)message;
    const int64_t wp = (int64_t)w_param;
    const int64_t lp = (int64_t)l_param;
    int64_t ret = 0;
    ui_app_update_mouse_buttons_state();
    ui_view.lose_hidden_focus(ui_app.root);
    if (ui_app_click_detector((uint32_t)m, (WPARAM)wp, (LPARAM)lp)) {
        return 0;
    }
    if (ui_view.message(ui_app.root, m, wp, lp, &ret)) {
        return (LRESULT)ret;
    }
    if (m == ui.message.opening) { ui_app_window_opening(); return 0; }
    if (m == ui.message.closing) { ui_app_window_closing(); return 0; }
    if (m == ui.message.animate) {
        ui_app_animate_step((ui_app_animate_function_t)lp, (int32_t)wp, -1);
        return 0;
    }
    ui_app_message_handler_t* handler = ui_app.handlers; 
    while (handler != null) { 
        if (handler->callback(handler, m, wp, lp, &ret)) {
            return ret;
        }
        handler = handler->next;
    }
    switch (m) {
        case WM_GETMINMAXINFO:
            ui_app_get_min_max_info((MINMAXINFO*)lp);
            break;
        case WM_CLOSE        :
            ui_view.set_focus(null); // before WM_CLOSING
            ui_app_post_message(ui.message.closing, 0, 0);
            return 0;
        case WM_DESTROY      :
            PostQuitMessage(ui_app.exit_code);
            break;
        case WM_ACTIVATE         :
            ui_app_wm_activate(wp);
            break;
        case WM_SYSCOMMAND  :
            if (ui_app_wm_sys_command(wp, lp)) { return 0; }
            break;
        case WM_WINDOWPOSCHANGING:
            ui_app_wm_window_position_changing(wp, lp);
            break;
        case WM_WINDOWPOSCHANGED:
            ui_app_window_position_changed((WINDOWPOS*)lp);
            break;
        case WM_NCHITTEST    :
            return ui_app_wm_nc_hit_test(wp, lp);
        case WM_SYSKEYDOWN   :
            return ui_app_wm_sys_key_down(wp, lp);
        case WM_SYSCHAR      :
            if (wp == VK_MENU) { return 0; } // swallow - no DefWindowProc()
            break;
        case WM_KEYDOWN      :
            ui_app_alt_ctrl_shift(true, wp);
            if (ui_app_wm_key_pressed(ui_app.root, wp)) { return 0; } // swallow
            break;
        case WM_SYSKEYUP:
        case WM_KEYUP        :
            ui_app_alt_ctrl_shift(false, wp);
            ui_view.key_released(ui_app.root, wp);
            break;
        case WM_TIMER        :
            ui_app_wm_timer((ui_timer_t)wp);
            break;
        case WM_ERASEBKGND   :
            return true; // no DefWindowProc()
        case WM_INPUTLANGCHANGE:
            ui_app_wm_input_language_change(wp);
            break;
        case WM_CHAR         :
            ui_app_wm_char(ui_app.root, (const uint16_t*)&wp);
            break;
        case WM_PRINTCLIENT  :
            ui_app_paint_on_canvas((HDC)wp);
            break;
        case WM_SETFOCUS     :
            ui_app_wm_set_focus();
            break;
        case WM_KILLFOCUS    :
            ui_app_wm_kill_focus();
            break;
        case WM_NCCALCSIZE:
            return ui_app_wm_nc_calculate_size(wp, lp);
        case WM_PAINT        :
            ui_app_wm_paint();
            break;
        case WM_CONTEXTMENU  :
            (void)ui_view.context_menu(ui_app.root);
            break;
        case WM_THEMECHANGED :
            ui_theme.refresh();
            break;
        case WM_SETTINGCHANGE:
            ui_app_setting_change((uintptr_t)wp, (uintptr_t)lp);
            break;
        case WM_GETDPISCALEDSIZE: // sent before WM_DPICHANGED
            return ui_app_wm_get_dpi_scaled_size(wp);
        case WM_DPICHANGED  :
            ui_app_wm_dpi_changed();
            break;
        case WM_NCLBUTTONDOWN   : case WM_NCRBUTTONDOWN  : case WM_NCMBUTTONDOWN  :
        case WM_NCLBUTTONUP     : case WM_NCRBUTTONUP    : case WM_NCMBUTTONUP    :
        case WM_NCLBUTTONDBLCLK : case WM_NCRBUTTONDBLCLK: case WM_NCMBUTTONDBLCLK:
        case WM_NCMOUSEMOVE     :
            ui_app_nc_mouse_buttons(m, wp, lp);
            break;
        case WM_LBUTTONDOWN     : case WM_RBUTTONDOWN  : case WM_MBUTTONDOWN  :
        case WM_LBUTTONUP       : case WM_RBUTTONUP    : case WM_MBUTTONUP    :
        case WM_LBUTTONDBLCLK   : case WM_RBUTTONDBLCLK: case WM_MBUTTONDBLCLK:
//          if (m == WM_LBUTTONDOWN)   { rt_println("WM_LBUTTONDOWN"); }
//          if (m == WM_LBUTTONUP)     { rt_println("WM_LBUTTONUP"); }
//          if (m == WM_LBUTTONDBLCLK) { rt_println("WM_LBUTTONDBLCLK"); }
            if (ui_app_wm_mouse(m, wp, lp)) { return 0; }
            break;
        case WM_MOUSEHOVER      :
        case WM_MOUSEMOVE       :
            if (ui_app_wm_mouse(m, wp, lp)) { return 0; }
            break;
        case WM_MOUSEWHEEL   :
            ui_app_wm_mouse_wheel(true, wp);
            break;
        case WM_MOUSEHWHEEL  :
            ui_app_wm_mouse_wheel(false, wp);
            break;
        // debugging:
        #ifdef UI_APP_DEBUGING_ALT_KEYBOARD_SHORTCUTS
        case WM_PARENTNOTIFY  : rt_println("WM_PARENTNOTIFY");     break;
        case WM_ENTERMENULOOP : rt_println("WM_ENTERMENULOOP");    return 0;
        case WM_EXITMENULOOP  : rt_println("WM_EXITMENULOOP");     return 0;
        case WM_INITMENU      : rt_println("WM_INITMENU");         return 0;
        case WM_MENUCHAR      : rt_println("WM_MENUCHAR");         return MNC_CLOSE << 16;
        case WM_CAPTURECHANGED: rt_println("WM_CAPTURECHANGED");   break;
        case WM_MENUSELECT    : rt_println("WM_MENUSELECT");       return 0;
        #else
        // ***Important***: prevents annoying beeps on Alt+Shortcut
        case WM_MENUCHAR      : return MNC_CLOSE << 16;
        // TODO: may be beeps are good if no UI controls reacted
        #endif
        // TODO: investigate WM_SETCURSOR in regards to wait cursor
        case WM_SETCURSOR    :
            if (LOWORD(lp) == HTCLIENT) { // see WM_NCHITTEST
                SetCursor((HCURSOR)ui_app.cursor);
                return true; // must NOT call DefWindowProc()
            }
            break;
#ifdef UI_APP_USE_WM_IME
        case WM_IME_CHAR:
            rt_println("WM_IME_CHAR: 0x%04X", wp);
            break;
        case WM_IME_NOTIFY:
            rt_println("WM_IME_NOTIFY");
            break;
        case WM_IME_REQUEST:
            rt_println("WM_IME_REQUEST");
            break;
        case WM_IME_STARTCOMPOSITION:
            rt_println("WM_IME_STARTCOMPOSITION");
            break;
        case WM_IME_ENDCOMPOSITION:
            rt_println("WM_IME_ENDCOMPOSITION");
            break;
        case WM_IME_COMPOSITION:
            rt_println("WM_IME_COMPOSITION");
            ui_app_ime_composition(lp);
            break;
#endif  // UI_APP_USE_WM_IME
        // TODO:
        case WM_UNICHAR       : // only UTF-32 via PostMessage?
            rt_println("???");
            // see: https://learn.microsoft.com/en-us/windows/win32/inputdev/about-keyboard-input
            // https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-tounicode
            break;
        default:
            break;
    }
    return DefWindowProcW(ui_app_window(), (UINT)m, (WPARAM)wp, lp);
}

static long ui_app_get_window_long(int32_t index) {
    rt_core.set_err(0);
    long v = GetWindowLongA(ui_app_window(), index);
    rt_fatal_if_error(rt_core.err());
    return v;
}

static long ui_app_set_window_long(int32_t index, long value) {
    rt_core.set_err(0);
    long r = SetWindowLongA(ui_app_window(), index, value); // r previous value
    rt_fatal_if_error(rt_core.err());
    return r;
}

static void ui_app_modify_window_style(uint32_t include, uint32_t exclude) {
    long s = ui_app_get_window_long(GWL_STYLE);
    s &= ~exclude;
    s |=  include;
    ui_app_set_window_long(GWL_STYLE, s);
}

static DWORD ui_app_window_style(void) {
    return ui_app.no_decor ? WS_POPUPWINDOW|
                             WS_THICKFRAME|
                             WS_MINIMIZEBOX
                           : WS_OVERLAPPEDWINDOW;
}

static errno_t ui_app_set_layered_window(ui_color_t color, fp32_t alpha) {
    uint8_t  a = 0; // alpha 0..255
    uint32_t c = 0; // R8G8B8
    DWORD mask = 0;
    if (0 <= alpha && alpha <= 1.0f) {
        mask |= LWA_ALPHA;
        a = (uint8_t)(alpha * 255 + 0.5f);
    }
    if (color != ui_color_undefined) {
        mask |= LWA_COLORKEY;
        rt_assert(ui_color_is_8bit(color));
        c = ui_gdi.color_rgb(color);
    }
    return rt_b2e(SetLayeredWindowAttributes(ui_app_window(), c, a, mask));
}

static void ui_app_set_dwm_attribute(uint32_t mode, void* a, DWORD bytes) {
    rt_fatal_if_error(DwmSetWindowAttribute(ui_app_window(), mode, a, bytes));
}

static void ui_app_init_dwm(void) {
    if (IsWindowsVersionOrGreater(10, 0, 22000)) {
        // do not call on Win10 - will fail
        DWM_WINDOW_CORNER_PREFERENCE c = DWMWCP_ROUND;
        ui_app_set_dwm_attribute(DWMWA_WINDOW_CORNER_PREFERENCE, &c, sizeof(c));
        COLORREF cc = (COLORREF)ui_gdi.color_rgb(ui_color_rgb(45, 45, 48));
        ui_app_set_dwm_attribute(DWMWA_CAPTION_COLOR, &cc, sizeof(cc));
    }
    BOOL e = true; // must be 32-bit BOOL because of sizeof()
    ui_app_set_dwm_attribute(DWMWA_USE_IMMERSIVE_DARK_MODE, &e, sizeof(e));
    // kudos for double negatives - so easy to make mistakes:
    ui_app_set_dwm_attribute(DWMWA_TRANSITIONS_FORCEDISABLED, &e, sizeof(e));
    enum DWMNCRENDERINGPOLICY rp = DWMNCRP_USEWINDOWSTYLE;
    ui_app_set_dwm_attribute(DWMWA_NCRENDERING_POLICY, &rp, sizeof(rp));
    if (ui_app.no_decor) {
        ui_app_set_dwm_attribute(DWMWA_ALLOW_NCPAINT, &e, sizeof(e));
        MARGINS margins = { 0, 0, 0, 0 };
        rt_fatal_if_error(
            DwmExtendFrameIntoClientArea(ui_app_window(), &margins)
        );
    }
}

static void ui_app_swp(HWND top, int32_t x, int32_t y, int32_t w, int32_t h,
        uint32_t f) {
    rt_fatal_win32err(SetWindowPos(ui_app_window(), top, x, y, w, h, f));
}

static void ui_app_swp_flags(uint32_t f) {
    rt_fatal_win32err(SetWindowPos(ui_app_window(), null, 0, 0, 0, 0, f));
}

static void ui_app_disable_sys_menu_item(HMENU sys_menu, uint32_t item) {
    const uint32_t f = MF_BYCOMMAND | MF_DISABLED;
    rt_fatal_win32err(EnableMenuItem(sys_menu, item, f));
}

static void ui_app_init_sys_menu(void) {
    // tried to remove unused items from system menu which leads to
    // AllowDarkModeForWindow() failed 0x000005B0(1456) "A menu item was not found."
    // SetPreferredAppMode() failed 0x000005B0(1456) "A menu item was not found."
    // this is why they just disabled instead.
    HMENU sys_menu = GetSystemMenu(ui_app_window(), false);
    rt_not_null(sys_menu);
    if (ui_app.no_min || ui_app.no_max) {
        int32_t exclude = WS_SIZEBOX;
        if (ui_app.no_min) { exclude = WS_MINIMIZEBOX; }
        if (ui_app.no_max) { exclude = WS_MAXIMIZEBOX; }
        ui_app_modify_window_style(0, exclude);
        if (ui_app.no_min) { ui_app_disable_sys_menu_item(sys_menu, SC_MINIMIZE); }
        if (ui_app.no_max) { ui_app_disable_sys_menu_item(sys_menu, SC_MAXIMIZE); }
    }
    if (ui_app.no_size) {
        ui_app_disable_sys_menu_item(sys_menu, SC_SIZE);
        ui_app_modify_window_style(0, WS_SIZEBOX);
        const uint32_t f = SWP_FRAMECHANGED | SWP_NOMOVE | SWP_NOSIZE |
                           SWP_NOACTIVATE;
        ui_app_swp_flags(f);
    }
}

static void ui_app_create_window(const ui_rect_t r) {
    uint16_t class_name[256];
    rt_str.utf8to16(class_name, rt_countof(class_name), ui_app.class_name, -1);
    WNDCLASSW* wc = &ui_app_wc;
    // CS_DBLCLKS no longer needed. Because code detects long-press
    // it does double click too. Editor uses both for word and paragraph select.
    wc->style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC | CS_SAVEBITS;
    wc->lpfnWndProc = ui_app_window_proc;
    wc->cbClsExtra = 0;
    wc->cbWndExtra = 256 * 1024;
    wc->hInstance = GetModuleHandleA(null);
    wc->hIcon = (HICON)ui_app.icon;
    wc->hCursor = (HCURSOR)ui_app.cursor;
    wc->hbrBackground = null;
    wc->lpszMenuName = null;
    wc->lpszClassName = class_name;
    ATOM atom = RegisterClassW(wc);
    rt_fatal_if(atom == 0);
    uint16_t title[256];
    rt_str.utf8to16(title, rt_countof(title), ui_app.title, -1);
    HWND window = CreateWindowExW(WS_EX_COMPOSITED | WS_EX_LAYERED,
        class_name, title, ui_app_window_style(),
        r.x, r.y, r.w, r.h, null, null, wc->hInstance, null);
    rt_not_null(ui_app.window);
    rt_swear(window == ui_app_window());
    ui_app.show_window(ui.visibility.hide);
    ui_view.set_text(&ui_caption.title, "%s", ui_app.title);
    ui_app.dpi.window = (int32_t)GetDpiForWindow(ui_app_window());
    RECT wrc = ui_app_ui2rect(&r);
    rt_fatal_win32err(GetWindowRect(ui_app_window(), &wrc));
    ui_app.wrc = ui_app_rect2ui(&wrc);
    ui_app_init_dwm();
    ui_app_init_sys_menu();
    ui_theme.refresh();
    if (ui_app.visibility != ui.visibility.hide) {
        AnimateWindow(ui_app_window(), 250, AW_ACTIVATE);
        ui_app.show_window(ui_app.visibility);
        ui_app_update_crc();
    }
    // even if it is hidden:
    ui_app_post_message(ui.message.opening, 0, 0);
//  SetWindowTheme(ui_app_window(), L"DarkMode_Explorer", null); ???
}

static void ui_app_full_screen(bool on) {
    static long style;
    static WINDOWPLACEMENT wp;
    if (on != ui_app.is_full_screen) {
        ui_app_show_task_bar(!on);
        if (on) {
            ui_app_modify_window_style(0, WS_OVERLAPPEDWINDOW|WS_POPUPWINDOW);
            ui_app_modify_window_style(WS_POPUP | WS_VISIBLE, 0);
            wp.length = sizeof(wp);
            rt_fatal_win32err(GetWindowPlacement(ui_app_window(), &wp));
            WINDOWPLACEMENT nwp = wp;
            nwp.showCmd = SW_SHOWNORMAL;
            nwp.rcNormalPosition = (RECT){ui_app.mrc.x, ui_app.mrc.y,
                ui_app.mrc.x + ui_app.mrc.w, ui_app.mrc.y + ui_app.mrc.h};
            rt_fatal_win32err(SetWindowPlacement(ui_app_window(), &nwp));
        } else {
            rt_fatal_win32err(SetWindowPlacement(ui_app_window(), &wp));
            ui_app_set_window_long(GWL_STYLE, ui_app_window_style());
            enum { flags = SWP_FRAMECHANGED | SWP_NOMOVE | SWP_NOSIZE |
                           SWP_NOZORDER | SWP_NOOWNERZORDER };
            ui_app_swp_flags(flags);
        }
        ui_app.is_full_screen = on;
    }
}

static bool ui_app_set_focus(ui_view_t* rt_unused(v)) { return false; }

static void ui_app_request_redraw(void) {  // < 2us
    SetEvent(ui_app_event_invalidate);
}

static void ui_app_draw(void) {
    rt_println("avoid at all cost. bad performance, bad UX");
    UpdateWindow(ui_app_window());
}

static void ui_app_invalidate_rect(const ui_rect_t* r) {
    RECT rc = ui_app_ui2rect(r);
    InvalidateRect(ui_app_window(), &rc, false);
//  rt_backtrace_here();
}

static int32_t ui_app_message_loop(void) {
    MSG msg = {0};
    while (GetMessageW(&msg, null, 0, 0)) {
        if (msg.message == WM_KEYDOWN    || msg.message == WM_KEYUP ||
            msg.message == WM_SYSKEYDOWN || msg.message == WM_SYSKEYUP) {
            // before TranslateMessage():
            ui_app_decode_keyboard(msg.message, msg.wParam, msg.lParam);
        }
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }
    rt_work_queue.flush(&ui_app_queue);
    rt_assert(msg.message == WM_QUIT);
    return (int32_t)msg.wParam;
}

static void ui_app_dispose(void) {
    ui_app_dispose_fonts();
    rt_event.dispose(ui_app_event_invalidate);
    ui_app_event_invalidate = null;
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

static void ui_app_show_hint_or_toast(ui_view_t* v, int32_t x, int32_t y,
        fp64_t timeout) {
    if (v != null) {
        ui_app.animating.x = x;
        ui_app.animating.y = y;
        ui_app.animating.focused = ui_app.focus;
        if (v->type == ui_view_mbx) {
            ((ui_mbx_t*)v)->option = -1;
            if (v->focusable) {
                 ui_view.set_focus(v);
            }
        }
        // allow unparented ui for toast and hint
        ui_view_call_init(v);
        const int32_t steps = x < 0 && y < 0 ? ui_app_animation_steps : 1;
        ui_app_animate_start(ui_app_toast_dim, steps);
        ui_app.animating.view = v;
        v->parent = ui_app.root;
        if (v->focusable) { ui_view.set_focus(v); }
        ui_app.animating.time = timeout > 0 ? ui_app.now + timeout : 0;
    } else {
        ui_app_toast_cancel();
    }
}

static void ui_app_show_toast(ui_view_t* view, fp64_t timeout) {
    ui_app_show_hint_or_toast(view, -1, -1, timeout);
}

static void ui_app_show_hint(ui_view_t* view, int32_t x, int32_t y,
        fp64_t timeout) {
    if (view != null) {
        ui_app_show_hint_or_toast(view, x, y, timeout);
    } else if (ui_app.animating.view != null && ui_app.animating.x >= 0 &&
               ui_app.animating.y >= 0) {
        ui_app_toast_cancel(); // only cancel hints not toasts
    }
}

static void ui_app_formatted_toast_va(fp64_t timeout, const char* format, va_list va) {
    ui_app_show_toast(null, 0);
    static ui_label_t label = ui_label(0.0, "");
    ui_label_init_va(&label, 0.0, format, va);
    ui_app_show_toast(&label, timeout);
}

static void ui_app_formatted_toast(fp64_t timeout, const char* format, ...) {
    va_list va;
    va_start(va, format);
    ui_app_formatted_toast_va(timeout, format, va);
    va_end(va);
}

static int32_t ui_app_caret_w;
static int32_t ui_app_caret_h;
static int32_t ui_app_caret_x = -1;
static int32_t ui_app_caret_y = -1;
static bool    ui_app_caret_shown;

static void ui_app_create_caret(int32_t w, int32_t h) {
    ui_app_caret_w = w;
    ui_app_caret_h = h;
    rt_fatal_win32err(CreateCaret(ui_app_window(), null, w, h));
    rt_assert(GetSystemMetrics(SM_CARETBLINKINGENABLED));
}

static void ui_app_invalidate_caret(void) {
    if (ui_app_caret_w >  0 && ui_app_caret_h >  0 &&
        ui_app_caret_x >= 0 && ui_app_caret_y >= 0 &&
        ui_app_caret_shown) {
        RECT rc = { ui_app_caret_x, ui_app_caret_y,
                    ui_app_caret_x + ui_app_caret_w,
                    ui_app_caret_y + ui_app_caret_h };
        rt_fatal_win32err(InvalidateRect(ui_app_window(), &rc, false));
    }
}

static void ui_app_show_caret(void) {
    rt_assert(!ui_app_caret_shown);
    rt_fatal_win32err(ShowCaret(ui_app_window()));
    ui_app_caret_shown = true;
    ui_app_invalidate_caret();
}

static void ui_app_move_caret(int32_t x, int32_t y) {
    ui_app_invalidate_caret(); // where is was
    ui_app_caret_x = x;
    ui_app_caret_y = y;
    rt_fatal_win32err(SetCaretPos(x, y));
    ui_app_invalidate_caret(); // where it is now
}

static void ui_app_hide_caret(void) {
    rt_assert(ui_app_caret_shown);
    rt_fatal_win32err(HideCaret(ui_app_window()));
    ui_app_invalidate_caret();
    ui_app_caret_shown = false;
}

static void ui_app_destroy_caret(void) {
    ui_app_caret_w = 0;
    ui_app_caret_h = 0;
    rt_fatal_win32err(DestroyCaret());
}

static void ui_app_beep(int32_t kind) {
    static int32_t beep_id[] = { MB_OK, MB_ICONINFORMATION, MB_ICONQUESTION,
                          MB_ICONWARNING, MB_ICONERROR};
    rt_swear(0 <= kind && kind < rt_countof(beep_id));
    rt_fatal_win32err(MessageBeep(beep_id[kind]));
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
    int r = AttachConsole(ATTACH_PARENT_PROCESS) ? 0 : rt_core.err();
    if (r == 0) {
        ui_app_console_disable_close();
        rt_thread.sleep_for(0.1); // give cmd.exe a chance to print prompt again
        printf("\n");
    }
    return r;
}

static bool ui_app_is_stdout_redirected(void) {
    // https://stackoverflow.com/questions/30126490/how-to-check-if-stdout-is-redirected-to-a-file-or-to-a-console
    HANDLE out = GetStdHandle(STD_OUTPUT_HANDLE);
    DWORD type = out == null ? FILE_TYPE_UNKNOWN : GetFileType(out);
    type &= ~(DWORD)FILE_TYPE_REMOTE;
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
    int r = GetConsoleScreenBufferInfoEx(console, &info) ? 0 : rt_core.err();
    if (r != 0) {
        rt_println("GetConsoleScreenBufferInfoEx() %s", rt_strerr(r));
    } else {
        // tricky because correct order of the calls
        // SetConsoleWindowInfo() SetConsoleScreenBufferSize() depends on
        // current Window Size (in pixels) ConsoleWindowSize(in characters)
        // and SetConsoleScreenBufferSize().
        // After a lot of experimentation and reading docs most sensible option
        // is to try both calls in two different orders.
        COORD c = {w, h};
        SMALL_RECT const min_win = { 0, 0, c.X - 1, c.Y - 1 };
        c.Y = 9001; // maximum buffer number of rows at the moment of implementation
        int r0 = SetConsoleWindowInfo(console, true, &min_win) ? 0 : rt_core.err();
//      if (r0 != 0) { rt_println("SetConsoleWindowInfo() %s", rt_strerr(r0)); }
        int r1 = SetConsoleScreenBufferSize(console, c) ? 0 : rt_core.err();
//      if (r1 != 0) { rt_println("SetConsoleScreenBufferSize() %s", rt_strerr(r1)); }
        if (r0 != 0 || r1 != 0) { // try in reverse order (which expected to work):
            r0 = SetConsoleScreenBufferSize(console, c) ? 0 : rt_core.err();
            if (r0 != 0) { rt_println("SetConsoleScreenBufferSize() %s", rt_strerr(r0)); }
            r1 = SetConsoleWindowInfo(console, true, &min_win) ? 0 : rt_core.err();
            if (r1 != 0) { rt_println("SetConsoleWindowInfo() %s", rt_strerr(r1)); }
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
    r = GetConsoleMode(console, &mode) ? 0 : rt_core.err();
    rt_fatal_if_error(r, "GetConsoleMode() %s", rt_strerr(r));
    mode &= ~ENABLE_AUTO_POSITION;
    r = SetConsoleMode(console, &mode) ? 0 : rt_core.err();
    rt_fatal_if_error(r, "SetConsoleMode() %s", rt_strerr(r));
    */
    CONSOLE_SCREEN_BUFFER_INFOEX info = { sizeof(CONSOLE_SCREEN_BUFFER_INFOEX) };
    int r = GetConsoleScreenBufferInfoEx(console, &info) ? 0 : rt_core.err();
    rt_fatal_if_error(r, "GetConsoleScreenBufferInfoEx() %s", rt_strerr(r));
    COORD c = GetLargestConsoleWindowSize(console);
    if (c.X > 80) { c.X &= ~0x7; }
    if (c.Y > 24) { c.Y &= ~0x3; }
    if (c.X > 80) { c.X -= 8; }
    if (c.Y > 24) { c.Y -= 4; }
    ui_app_set_console_size(c.X, c.Y);
    r = GetConsoleScreenBufferInfoEx(console, &info) ? 0 : rt_core.err();
    rt_fatal_if_error(r, "GetConsoleScreenBufferInfoEx() %s", rt_strerr(r));
    info.dwSize.Y = 9999; // maximum value at the moment of implementation
    r = SetConsoleScreenBufferInfoEx(console, &info) ? 0 : rt_core.err();
    rt_fatal_if_error(r, "SetConsoleScreenBufferInfoEx() %s", rt_strerr(r));
    ui_app_save_console_pos();
}

static void ui_app_make_topmost(void) {
    //  Places the window above all non-topmost windows.
    // The window maintains its topmost position even when it is deactivated.
    enum { swp = SWP_SHOWWINDOW | SWP_NOREPOSITION | SWP_NOMOVE | SWP_NOSIZE };
    ui_app_swp(HWND_TOPMOST, 0, 0, 0, 0, swp);
}

static void ui_app_activate(void) {
    rt_core.set_err(0);
    HWND previous = SetActiveWindow(ui_app_window());
    if (previous == null) { rt_fatal_if_error(rt_core.err()); }
}

static void ui_app_bring_to_foreground(void) {
    // SetForegroundWindow() does not activate window:
    rt_fatal_win32err(SetForegroundWindow(ui_app_window()));
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
    ui_view.set_text(&ui_caption.title, "%s", title);
    rt_fatal_win32err(SetWindowTextA(ui_app_window(), rt_nls.str(title)));
}

static void ui_app_capture_mouse(bool on) {
    static int32_t mouse_capture;
    if (on) {
        rt_swear(mouse_capture == 0);
        mouse_capture++;
        SetCapture(ui_app_window());
    } else {
        rt_swear(mouse_capture == 1);
        mouse_capture--;
        ReleaseCapture();
    }
}

static void ui_app_move_and_resize(const ui_rect_t* rc) {
    enum { swp = SWP_NOZORDER | SWP_NOACTIVATE };
    ui_app_swp(null, rc->x, rc->y, rc->w, rc->h, swp);
}

static void ui_app_set_console_title(HWND cw) {
    rt_swear(rt_thread.id() == ui_app.tid);
    static char text[256];
    text[0] = 0;
    GetWindowTextA((HWND)ui_app.window, text, rt_countof(text));
    text[rt_countof(text) - 1] = 0;
    char title[256];
    rt_str_printf(title, "%s - Console", text);
    rt_fatal_win32err(SetWindowTextA(cw, title));
}

static void ui_app_restore_console(int32_t *visibility) {
    HWND cw = GetConsoleWindow();
    if (cw != null) {
        RECT wr = {0};
        GetWindowRect(cw, &wr);
        ui_rect_t rc = ui_app_rect2ui(&wr);
        ui_app_load_console_pos(&rc, visibility);
        if (rc.w > 0 && rc.h > 0) {
//          rt_println("%d,%d %dx%d px", rc.x, rc.y, rc.w, rc.h);
            CONSOLE_SCREEN_BUFFER_INFOEX info = {
                sizeof(CONSOLE_SCREEN_BUFFER_INFOEX)
            };
            int32_t r = rt_config.load(ui_app.class_name,
                "console_screen_buffer_infoex", &info, (int32_t)sizeof(info));
            if (r == sizeof(info)) { // 24x80
                SMALL_RECT sr = info.srWindow;
                int16_t w = (int16_t)rt_max(sr.Right - sr.Left + 1, 80);
                int16_t h = (int16_t)rt_max(sr.Bottom - sr.Top + 1, 24);
//              rt_println("info: %dx%d", info.dwSize.X, info.dwSize.Y);
//              rt_println("%d,%d %dx%d", sr.Left, sr.Top, w, h);
                if (w > 0 && h > 0) { ui_app_set_console_size(w, h); }
    	    }
            // do not resize console window just restore it's position
            enum { flags = SWP_NOZORDER | SWP_NOACTIVATE | SWP_NOSIZE };
            rt_fatal_win32err(SetWindowPos(cw, null,
                    rc.x, rc.y, rc.w, rc.h, flags));
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
    int r = AllocConsole() ? 0 : rt_core.err();
    if (r == 0) {
        ui_app_console_disable_close();
        int32_t visibility = 0;
        ui_app_restore_console(&visibility);
        ui_app.console_show(visibility != 0);
    }
    return r;
}

static fp32_t ui_app_px2in(int32_t pixels) {
    rt_assert(ui_app.dpi.monitor_max > 0);
//  rt_println("ui_app.dpi.monitor_raw: %d", ui_app.dpi.monitor_max);
    return ui_app.dpi.monitor_max > 0 ?
           (fp32_t)pixels / (fp32_t)ui_app.dpi.monitor_max : 0;
}

static int32_t ui_app_in2px(fp32_t inches) {
    rt_assert(ui_app.dpi.monitor_max > 0);
//  rt_println("ui_app.dpi.monitor_raw: %d", ui_app.dpi.monitor_max);
    return (int32_t)(inches * (fp64_t)ui_app.dpi.monitor_max + 0.5);
}

static void ui_app_request_layout(void) {
    ui_app_layout_dirty = true;
    ui_app.request_redraw();
}

static void ui_app_show_window(int32_t show) {
    rt_assert(ui.visibility.hide <= show &&
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
        enum { flags = SWP_SHOWWINDOW | SWP_NOZORDER | SWP_NOSIZE |
                       SWP_NOREPOSITION | SWP_NOMOVE };
        ui_app_swp_flags(flags);
        ui_app.request_focus();
    } else if (show == ui.visibility.hide ||
               show == ui.visibility.minimize ||
               show == ui.visibility.min_na) {
        ui_app_toast_cancel();
    }
}

static const char* ui_app_open_file(const char* folder,
        const char* pairs[], int32_t n) {
    rt_swear(rt_thread.id() == ui_app.tid);
    rt_assert(pairs == null && n == 0 || n >= 2 && n % 2 == 0);
    static uint16_t memory[4 * 1024];
    uint16_t* filter = memory;
    if (pairs == null || n == 0) {
        filter = L"All Files\0*\0\0";
    } else {
        int32_t left = rt_countof(memory) - 2;
        uint16_t* s = memory;
        for (int32_t i = 0; i < n; i+= 2) {
            uint16_t* s0 = s;
            rt_str.utf8to16(s0, left, pairs[i + 0], -1);
            int32_t n0 = (int32_t)rt_str.len16(s0);
            rt_assert(n0 > 0);
            s += n0 + 1;
            left -= n0 + 1;
            uint16_t* s1 = s;
            rt_str.utf8to16(s1, left, pairs[i + 1], -1);
            int32_t n1 = (int32_t)rt_str.len16(s1);
            rt_assert(n1 > 0);
            s[n1] = 0;
            s += n1 + 1;
            left -= n1 + 1;
        }
        *s++ = 0;
    }
    static uint16_t dir[rt_files_max_path];
    dir[0] = 0;
    rt_str.utf8to16(dir, rt_countof(dir), folder, -1);
    static uint16_t path[rt_files_max_path];
    path[0] = 0;
    OPENFILENAMEW ofn = { sizeof(ofn) };
    ofn.hwndOwner = (HWND)ui_app.window;
    ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;
    ofn.lpstrFilter = filter;
    ofn.lpstrInitialDir = dir;
    ofn.lpstrFile = path;
    ofn.nMaxFile = sizeof(path);
    static rt_file_name_t fn;
    fn.s[0] = 0;
    if (GetOpenFileNameW(&ofn) && path[0] != 0) {
        rt_str.utf16to8(fn.s, rt_countof(fn.s), path, -1);
    } else {
        fn.s[0] = 0;
    }
    return fn.s;
}

// TODO: use clipboard instead?

static errno_t ui_app_clipboard_put_image(ui_bitmap_t* im) {
    HDC canvas = GetDC(null);
    rt_not_null(canvas);
    HDC src = CreateCompatibleDC(canvas); rt_not_null(src);
    HDC dst = CreateCompatibleDC(canvas); rt_not_null(dst);
    // CreateCompatibleBitmap(dst) will create monochrome bitmap!
    // CreateCompatibleBitmap(canvas) will create display compatible
    HBITMAP texture = CreateCompatibleBitmap(canvas, im->w, im->h);
    rt_not_null(texture);
    HBITMAP s = SelectBitmap(src, im->texture); rt_not_null(s);
    HBITMAP d = SelectBitmap(dst, texture);     rt_not_null(d);
    POINT pt = { 0 };
    rt_fatal_win32err(SetBrushOrgEx(dst, 0, 0, &pt));
    rt_fatal_win32err(StretchBlt(dst, 0, 0, im->w, im->h, src, 0, 0,
        im->w, im->h, SRCCOPY));
    errno_t r = rt_b2e(OpenClipboard(GetDesktopWindow()));
    if (r != 0) { rt_println("OpenClipboard() failed %s", rt_strerr(r)); }
    if (r == 0) {
        r = rt_b2e(EmptyClipboard());
        if (r != 0) { rt_println("EmptyClipboard() failed %s", rt_strerr(r)); }
    }
    if (r == 0) {
        r = rt_b2e(SetClipboardData(CF_BITMAP, texture));
        if (r != 0) {
            rt_println("SetClipboardData() failed %s", rt_strerr(r));
        }
    }
    if (r == 0) {
        r = rt_b2e(CloseClipboard());
        if (r != 0) {
            rt_println("CloseClipboard() failed %s", rt_strerr(r));
        }
    }
    rt_not_null(SelectBitmap(dst, d));
    rt_not_null(SelectBitmap(src, s));
    rt_fatal_win32err(DeleteBitmap(texture));
    rt_fatal_win32err(DeleteDC(dst));
    rt_fatal_win32err(DeleteDC(src));
    rt_fatal_win32err(ReleaseDC(null, canvas));
    return r;
}

static ui_view_t ui_app_view = ui_view(list);
static ui_view_t ui_app_content = ui_view(stack);

static bool ui_app_is_active(void) { return GetActiveWindow() == ui_app_window(); }

static bool ui_app_is_minimized(void) { return IsIconic(ui_app_window()); }

static bool ui_app_is_maximized(void) { return IsZoomed(ui_app_window()); }

static bool ui_app_focused(void) { return GetFocus() == ui_app_window(); }

static void window_request_focus(void* w) {
    // https://stackoverflow.com/questions/62649124/pywin32-setfocus-resulting-in-access-is-denied-error
    // https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-attachthreadinput
    rt_assert(rt_thread.id() == ui_app.tid, "cannot be called from background thread");
    rt_core.set_err(0);
    HWND previous = SetFocus((HWND)w); // previously focused window
    if (previous == null) { rt_fatal_if_error(rt_core.err()); }
}

static void ui_app_request_focus(void) {
    window_request_focus(ui_app.window);
}

static void ui_app_init(void) {
    ui_app_event_quit           = rt_event.create_manual();
    ui_app_event_invalidate     = rt_event.create();
    ui_app.request_redraw       = ui_app_request_redraw;
    ui_app.post                 = ui_app_post;
    ui_app.draw                 = ui_app_draw;
    ui_app.px2in                = ui_app_px2in;
    ui_app.in2px                = ui_app_in2px;
    ui_app.set_layered_window   = ui_app_set_layered_window;
    ui_app.is_active            = ui_app_is_active;
    ui_app.is_minimized         = ui_app_is_minimized;
    ui_app.is_maximized         = ui_app_is_maximized;
    ui_app.focused              = ui_app_focused;
    ui_app.request_focus        = ui_app_request_focus;
    ui_app.activate             = ui_app_activate;
    ui_app.set_title            = ui_app_set_title;
    ui_app.capture_mouse        = ui_app_capture_mouse;
    ui_app.move_and_resize      = ui_app_move_and_resize;
    ui_app.bring_to_foreground  = ui_app_bring_to_foreground;
    ui_app.make_topmost         = ui_app_make_topmost;
    ui_app.bring_to_front       = ui_app_bring_to_front;
    ui_app.request_layout       = ui_app_request_layout;
    ui_app.invalidate           = ui_app_invalidate_rect;
    ui_app.full_screen          = ui_app_full_screen;
    ui_app.set_cursor           = ui_app_cursor_set;
    ui_app.close                = ui_app_close_window;
    ui_app.quit                 = ui_app_quit;
    ui_app.set_timer            = ui_app_timer_set;
    ui_app.kill_timer           = ui_app_timer_kill;
    ui_app.show_window          = ui_app_show_window;
    ui_app.show_toast           = ui_app_show_toast;
    ui_app.show_hint            = ui_app_show_hint;
    ui_app.toast_va             = ui_app_formatted_toast_va;
    ui_app.toast                = ui_app_formatted_toast;
    ui_app.create_caret         = ui_app_create_caret;
    ui_app.show_caret           = ui_app_show_caret;
    ui_app.move_caret           = ui_app_move_caret;
    ui_app.hide_caret           = ui_app_hide_caret;
    ui_app.destroy_caret        = ui_app_destroy_caret;
    ui_app.beep                 = ui_app_beep;
    ui_app.data_save            = ui_app_data_save;
    ui_app.data_size            = ui_app_data_size;
    ui_app.data_load            = ui_app_data_load;
    ui_app.open_file            = ui_app_open_file;
    ui_app.is_stdout_redirected = ui_app_is_stdout_redirected;
    ui_app.is_console_visible   = ui_app_is_console_visible;
    ui_app.console_attach       = ui_app_console_attach;
    ui_app.console_create       = ui_app_console_create;
    ui_app.console_show         = ui_app_console_show;
    ui_app.root    = &ui_app_view;
    ui_app.content = &ui_app_content;
    ui_app.caption = &ui_caption.view;
    ui_app.root->hit_test = ui_app_root_hit_test;
    ui_view.add(ui_app.root, ui_app.caption, ui_app.content, null);
    ui_view_call_init(ui_app.root); // to get done with container_init()
    rt_assert(ui_app.content->type == ui_view_stack);
    rt_assert(ui_app.content->background == ui_colors.transparent);
    ui_app.root->color_id = ui_color_id_window_text;
    ui_app.root->background_id = ui_color_id_window;
    ui_app.root->insets  = (ui_margins_t){ 0, 0, 0, 0 };
    ui_app.root->padding = (ui_margins_t){ 0, 0, 0, 0 };
    ui_app.root->paint = ui_app_view_paint;
    ui_app.root->max_w = ui.infinity;
    ui_app.root->max_h = ui.infinity;
    ui_app.content->insets  = (ui_margins_t){ 0, 0, 0, 0 };
    ui_app.content->padding = (ui_margins_t){ 0, 0, 0, 0 };
    ui_app.content->max_w = ui.infinity;
    ui_app.content->max_h = ui.infinity;
    ui_app.caption->state.hidden = !ui_app.no_decor;
    // for ui_view_debug_paint:
    ui_view.set_text(ui_app.root, "ui_app.root");
    ui_view.set_text(ui_app.content, "ui_app.content");
    if (ui_app.init != null) { ui_app.init(); }
}

static void ui_app_set_dpi_awareness(void) {
    // Mutually exclusive:
    // BOOL SetProcessDpiAwarenessContext()
    // https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-setprocessdpiawarenesscontext
    // and
    // HRESULT SetProcessDpiAwareness()
    // https://learn.microsoft.com/en-us/windows/win32/api/shellscalingapi/nf-shellscalingapi-setprocessdpiawareness
    // Plus DPI awareness can be set by APP .exe shell properties, registry
    // or Windows policy. See:
    // https://blogs.windows.com/windowsdeveloper/2017/05/19/improving-high-dpi-experience-gdi-based-desktop-apps/
    DPI_AWARENESS_CONTEXT dpi_awareness_context_1 =
        GetThreadDpiAwarenessContext();
    // https://blogs.windows.com/windowsdeveloper/2017/05/19/improving-high-dpi-experience-gdi-based-desktop-apps/
    errno_t error = rt_b2e(SetProcessDpiAwarenessContext(
            DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2));
    if (error == ERROR_ACCESS_DENIED) {
        rt_println("Warning: SetProcessDpiAwarenessContext(): ERROR_ACCESS_DENIED");
        // dpi awareness already set, manifest, registry, windows policy
        // Try via Shell:
        HRESULT hr = SetProcessDpiAwareness(PROCESS_PER_MONITOR_DPI_AWARE);
        if (hr == E_ACCESSDENIED) {
            rt_println("Warning: SetProcessDpiAwareness(): E_ACCESSDENIED");
        }
    }
    DPI_AWARENESS_CONTEXT dpi_awareness_context_2 =
        GetThreadDpiAwarenessContext();
    rt_swear(dpi_awareness_context_1 != dpi_awareness_context_2);
}

static void ui_app_init_windows(void) {
    ui_app_set_dpi_awareness();
    InitCommonControls(); // otherwise GetOpenFileName does not work
    ui_app.dpi.process = (int32_t)GetSystemDpiForProcess(GetCurrentProcess());
    ui_app.dpi.system = (int32_t)GetDpiForSystem(); // default was 96DPI
    // monitor dpi will be reinitialized in load_window_pos
    ui_app.dpi.monitor_effective = ui_app.dpi.system;
    ui_app.dpi.monitor_angular = ui_app.dpi.system;
    ui_app.dpi.monitor_raw = ui_app.dpi.system;
    ui_app.dpi.monitor_max = ui_app.dpi.system;
//  rt_println("ui_app.dpi.monitor_max := %d", ui_app.dpi.system);
    static const RECT nowhere = {0x7FFFFFFF, 0x7FFFFFFF, 0x7FFFFFFF, 0x7FFFFFFF};
    ui_rect_t r = ui_app_rect2ui(&nowhere);
    ui_app_update_mi(&r, MONITOR_DEFAULTTOPRIMARY);
    ui_app.dpi.window = ui_app.dpi.monitor_effective;
}

static ui_rect_t ui_app_window_initial_rectangle(void) {
    const ui_window_sizing_t* ws = &ui_app.window_sizing;
    // it is not practical and thus not implemented handling
    // == (0, 0) and != (0, 0) for sizing half dimension (only w or only h)
    rt_swear((ws->min_w != 0) == (ws->min_h != 0) &&
           ws->min_w >= 0 && ws->min_h >= 0,
          "ui_app.window_sizing .min_w=%.1f .min_h=%.1f", ws->min_w, ws->min_h);
    rt_swear((ws->ini_w != 0) == (ws->ini_h != 0) &&
           ws->ini_w >= 0 && ws->ini_h >= 0,
          "ui_app.window_sizing .ini_w=%.1f .ini_h=%.1f", ws->ini_w, ws->ini_h);
    rt_swear((ws->max_w != 0) == (ws->max_h != 0) &&
           ws->max_w >= 0 && ws->max_h >= 0,
          "ui_app.window_sizing .max_w=%.1f .max_h=%.1f", ws->max_w, ws->max_h);
    // if max is set then min and ini must be less than max
    if (ws->max_w != 0 || ws->max_h != 0) {
        rt_swear(ws->min_w <= ws->max_w && ws->min_h <= ws->max_h,
            "ui_app.window_sizing .min_w=%.1f .min_h=%.1f .max_w=%1.f .max_h=%.1f",
             ws->min_w, ws->min_h, ws->max_w, ws->max_h);
        rt_swear(ws->ini_w <= ws->max_w && ws->ini_h <= ws->max_h,
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

static FILE* ui_app_crash_log;

static bool ui_app_write_backtrace(const char* s, int32_t n) {
    if (n > 0 && s[n - 1] == 0) { n--; }
    if (n > 0 && ui_app_crash_log != null) {
        fwrite(s, n, 1, ui_app_crash_log);
    }
    return false;
}

static LONG ui_app_exception_filter(EXCEPTION_POINTERS* ep) {
    char fn[1024];
    DWORD ex = ep->ExceptionRecord->ExceptionCode; // exception code
    // T-connector for intercepting rt_debug.output:
    bool (*tee)(const char* s, int32_t n) = rt_debug.tee;
    rt_debug.tee = ui_app_write_backtrace;
    const char* home = rt_files.known_folder(rt_files.folder.home);
    if (home != null) {
        const char* name = ui_app.class_name  != null ?
                           ui_app.class_name : "ui_app";
        rt_str_printf(fn, "%s\\%s_crash_log.txt", home, name);
        ui_app_crash_log = fopen(fn, "w");
    }
    rt_debug.println(null, 0, null,
        "To file and issue report copy this log and");
    rt_debug.println(null, 0, null,
        "paste it here: https://github.com/leok7v/ui/discussions/4");
    rt_debug.println(null, 0, null,
        "%s exception: %s", rt_args.basename(), rt_str.error(ex));
    rt_backtrace_t bt = {{0}};
    rt_backtrace.context(rt_thread.self(), ep->ContextRecord, &bt);
    rt_backtrace.trace(&bt, "*");
    rt_backtrace.trace_all_but_self();
    rt_debug.tee = tee;
    if (ui_app_crash_log != null) {
        fclose(ui_app_crash_log);
        char cmd[1024];
        rt_str_printf(cmd, "cmd.exe /c start notepad \"%s\"", fn);
        system(cmd);
    }
    return EXCEPTION_CONTINUE_SEARCH;
}

#undef UI_APP_TEST_POST

#ifdef UI_APP_TEST_POST

// The dispatch_until() is just for testing purposes.
// Usually rt_work_queue.dispatch(q) will be called inside each
// iteration of message loop of a dispatch [UI] thread.

static void ui_app_test_dispatch_until(rt_work_queue_t* q, int32_t* i,
        const int32_t n) {
    while (q->head != null && *i < n) {
        rt_thread.sleep_for(0.0001); // 100 microseconds
        rt_work_queue.dispatch(q);
    }
    rt_work_queue.flush(q);
}

// simple way of passing a single pointer to call_later

static void ui_app_test_every_100ms(rt_work_t* w) {
    int32_t* i = (int32_t*)w->data;
    rt_println("i: %d", *i);
    (*i)++;
    w->when = rt_clock.seconds() + 0.100;
    rt_work_queue.post(w);
}

static void ui_app_test_work_queue_1(void) {
    rt_work_queue_t queue = {0};
    // if a single pointer will suffice
    int32_t i = 0;
    rt_work_t work = {
        .queue = &queue,
        .when  = rt_clock.seconds() + 0.100,
        .work  = ui_app_test_every_100ms,
        .data  = &i
    };
    rt_work_queue.post(&work);
    ui_app_test_dispatch_until(&queue, &i, 4);
}

// extending rt_work_t with extra data:

typedef struct rt_work_ex_s {
    union {
        rt_work_t base;
        struct rt_work_s;
    };
    struct { int32_t a; int32_t b; } s;
    int32_t i;
} rt_work_ex_t;

static void ui_app_test_every_200ms(rt_work_t* w) {
    rt_work_ex_t* ex = (rt_work_ex_t*)w;
    rt_println("ex { .i: %d, .s.a: %d .s.b: %d}", ex->i, ex->s.a, ex->s.b);
    ex->i++;
    const int32_t swap = ex->s.a; ex->s.a = ex->s.b; ex->s.b = swap;
    w->when = rt_clock.seconds() + 0.200;
    rt_work_queue.post(w);
}

static void ui_app_test_work_queue_2(void) {
    rt_work_queue_t queue = {0};
    rt_work_ex_t work = {
        .queue = &queue,
        .when  = rt_clock.seconds() + 0.200,
        .work  = ui_app_test_every_200ms,
        .data  = null,
        .s = { .a = 1, .b = 2 },
        .i = 0
    };
    rt_work_queue.post(&work.base);
    ui_app_test_dispatch_until(&queue, &work.i, 4);
}

static fp64_t ui_app_test_timestamp_0;
static fp64_t ui_app_test_timestamp_2;
static fp64_t ui_app_test_timestamp_3;
static fp64_t ui_app_test_timestamp_4;

static void ui_app_test_in_1_second(rt_work_t* rt_unused(work)) {
    ui_app_test_timestamp_3 = rt_clock.seconds();
    rt_println("ETA 3 seconds");
}

static void ui_app_test_in_2_seconds(rt_work_t* rt_unused(work)) {
    ui_app_test_timestamp_2 = rt_clock.seconds();
    rt_println("ETA 2 seconds");
    static rt_work_t invoke_in_1_seconds;
    invoke_in_1_seconds = (rt_work_t){
        .queue = null, // &ui_app_queue will be used
        .when = rt_clock.seconds() + 1.0, // seconds
        .work = ui_app_test_in_1_second
    };
    ui_app.post(&invoke_in_1_seconds);
}

static void ui_app_test_in_4_seconds(rt_work_t* rt_unused(work)) {
    ui_app_test_timestamp_4 = rt_clock.seconds();
    rt_println("ETA 4 seconds");
//  expected sequence of callbacks:
//  2:732 ui_app_test_in_2_seconds ETA 2 seconds
//  3:724 ui_app_test_in_1_second  ETA 3 seconds
//  4:735 ui_app_test_in_4_seconds ETA 4 seconds
    fp64_t dt2 = ui_app_test_timestamp_2 - ui_app_test_timestamp_0;
    fp64_t dt3 = ui_app_test_timestamp_3 - ui_app_test_timestamp_0;
    fp64_t dt4 = ui_app_test_timestamp_4 - ui_app_test_timestamp_0;
//  Assuming there were no huge startup delays:
    swear(1.75 < dt2 < 2.25);
    swear(2.75 < dt3 < 3.25);
    swear(3.75 < dt4 < 4.25);
}

static void ui_app_test_post(void) {
    ui_app_test_work_queue_1();
    ui_app_test_work_queue_2();
    rt_println("see Output/Timestamps");
    static rt_work_t invoke_in_2_seconds;
    static rt_work_t invoke_in_4_seconds;
    ui_app_test_timestamp_0 = rt_clock.seconds();
    invoke_in_2_seconds = (rt_work_t){
        .queue = null, // &ui_app_queue will be used
        .when = rt_clock.seconds() + 2.0, // seconds
        .work = ui_app_test_in_2_seconds
    };
    invoke_in_4_seconds = (rt_work_t){
        .queue = null, // &ui_app_queue will be used
        .when = rt_clock.seconds() + 4.0, // seconds
        .work = ui_app_test_in_4_seconds
    };
    ui_app.post(&invoke_in_4_seconds);
    ui_app.post(&invoke_in_2_seconds);
}

#endif

static int ui_app_win_main(HINSTANCE instance) {
    // IDI_ICON 101:
    ui_app.icon = (ui_icon_t)LoadIconW(instance, MAKEINTRESOURCE(101));
    ui_app_init_windows();
    ui_gdi.init();
    rt_clipboard.put_image = ui_app_clipboard_put_image;
    ui_app.last_visibility = ui.visibility.defau1t;
    ui_app_init();
    int r = 0;
//  ui_app_dump_dpi();
    // It is possible (but not trivial) to ask DWM to create taller tittle bar:
    // https://learn.microsoft.com/en-us/windows/win32/dwm/customframe
    // TODO: if any app need to make to app store they will probably ask for it
    // "wr" Window Rect in pixels: default is -1,-1, ini_w, ini_h
    ui_rect_t wr = ui_app_window_initial_rectangle();
    ui_app.caption_height = (int32_t)GetSystemMetricsForDpi(SM_CYCAPTION,
                                (uint32_t)ui_app.dpi.process);
    ui_app.border.w = (int32_t)GetSystemMetricsForDpi(SM_CXSIZEFRAME,
                                (uint32_t)ui_app.dpi.process);
    ui_app.border.h = (int32_t)GetSystemMetricsForDpi(SM_CYSIZEFRAME,
                                (uint32_t)ui_app.dpi.process);

    if (ui_app.no_decor) {
        // border is too think (5 pixels) narrow down to 3x3
        const int32_t max_border = ui_app.dpi.window <= 100 ? 1 :
            (ui_app.dpi.window >= 192 ? 3 : 2);
        ui_app.border.w = rt_min(max_border, ui_app.border.w);
        ui_app.border.h = rt_min(max_border, ui_app.border.h);
    }
//  rt_println("frame: %d,%d caption_height: %d", ui_app.border.w, ui_app.border.h, ui_app.caption_height);
    // TODO: use AdjustWindowRectEx instead
    // https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-adjustwindowrectex
    wr.x -= ui_app.border.w;
    wr.w += ui_app.border.w * 2;
    wr.y -= ui_app.border.h + ui_app.caption_height;
    wr.h += ui_app.border.h * 2 + ui_app.caption_height;
    if (!ui_app_load_window_pos(&wr, &ui_app.last_visibility)) {
        // first time - center window
        wr.x = ui_app.work_area.x + (ui_app.work_area.w - wr.w) / 2;
        wr.y = ui_app.work_area.y + (ui_app.work_area.h - wr.h) / 2;
        ui_app_bring_window_inside_monitor(&ui_app.mrc, &wr);
    }
    ui_app.root->state.hidden = true; // start with ui hidden
    ui_app.root->fm = &ui_app.fm.prop.normal;
    ui_app.root->w = wr.w - ui_app.border.w * 2;
    ui_app.root->h = wr.h - ui_app.border.h * 2 - ui_app.caption_height;
    ui_app_layout_dirty = true; // layout will be done before first paint
    rt_not_null(ui_app.class_name);
    ui_app_wt = (rt_event_t)CreateWaitableTimerA(null, false, null);
    rt_thread_t alarm  = rt_thread.start(ui_app_alarm_thread, null);
    if (!ui_app.no_ui) {
        ui_app_create_window(wr);
        ui_app_init_fonts(ui_app.dpi.window);
        rt_thread_t redraw = rt_thread.start(ui_app_redraw_thread, null);
        #ifdef UI_APP_TEST_POST
            ui_app_test_post();
        #endif
        r = ui_app_message_loop();
        // ui_app.fini() must be called before ui_app_dispose()
        if (ui_app.fini != null) { ui_app.fini(); }
        rt_event.set(ui_app_event_quit);
        rt_thread.join(redraw, -1);
        ui_app_dispose();
        if (r == 0 && ui_app.exit_code != 0) { r = ui_app.exit_code; }
    } else {
        r = ui_app.main();
        if (ui_app.fini != null) { ui_app.fini(); }
    }
    rt_event.set(ui_app_event_quit);
    rt_thread.join(alarm, -1);
    rt_event.dispose(ui_app_event_quit);
    ui_app_event_quit = null;
    rt_event.dispose(ui_app_wt);
    ui_app_wt = null;
    ui_gdi.fini();
    return r;
}

#pragma warning(disable: 28251) // inconsistent annotations

int WINAPI WinMain(HINSTANCE instance, HINSTANCE rt_unused(previous),
        char* rt_unused(command), int show) {
    SetUnhandledExceptionFilter(ui_app_exception_filter);
    const COINIT co_init = COINIT_MULTITHREADED | COINIT_SPEED_OVER_MEMORY;
    rt_fatal_if_error(CoInitializeEx(0, co_init));
    SetConsoleCP(CP_UTF8);
    // Expected manifest.xml containing UTF-8 code page
    // for TranslateMessage and WM_CHAR to deliver UTF-8 characters
    // see:
    // https://learn.microsoft.com/en-us/windows/apps/design/globalizing/use-utf8-code-page
    // .rc file must have:
    // 1 RT_MANIFEST "manifest.xml"
    if (GetACP() != 65001) {
        rt_println("codepage: %d UTF-8 will not be supported", GetACP());
    }
    // at the moment of writing there is no API call to inform Windows about process
    // preferred codepage except manifest.xml file in resource #1.
    // Absence of manifest.xml will result to ancient and useless ANSI 1252 codepage
    // TODO: may need to change CreateWindowA() to CreateWindowW() and
    // translate UTF16 to UTF8
    ui_app.tid = rt_thread.id();
    rt_nls.init();
    ui_app.visibility = show;
    rt_args.WinMain();
    int32_t r = ui_app_win_main(instance);
    rt_args.fini();
    return r;
}

int main(int argc, const char* argv[], const char** envp) {
    SetUnhandledExceptionFilter(ui_app_exception_filter);
    rt_fatal_if_error(CoInitializeEx(0, COINIT_MULTITHREADED | COINIT_SPEED_OVER_MEMORY));
    rt_args.main(argc, argv, envp);
    rt_nls.init();
    ui_app.tid = rt_thread.id();
    int r = ui_app.main();
    rt_args.fini();
    return r;
}

#pragma pop_macro("ui_app_canvas")
#pragma pop_macro("ui_app_window")

#pragma comment(lib, "comctl32")
#pragma comment(lib, "comdlg32")
#pragma comment(lib, "dwmapi")
#pragma comment(lib, "gdi32")
#pragma comment(lib, "msimg32")
#pragma comment(lib, "shcore")
#pragma comment(lib, "uxtheme")
