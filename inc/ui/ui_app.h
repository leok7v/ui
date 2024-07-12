#pragma once
#include "ut/ut_std.h"

begin_c

// link.exe /SUBSYSTEM:WINDOWS single window application

typedef struct ui_dpi_s { // max(dpi_x, dpi_y)
    int32_t system;  // system dpi
    int32_t process; // process dpi
    // 15" diagonal monitor 3840x2160 175% scaled
    // monitor dpi effective 168, angular 248 raw 284
    int32_t monitor_effective; // effective with regard of user scaling
    int32_t monitor_raw;       // with regard of physical screen size
    int32_t monitor_angular;   // diagonal raw
    int32_t monitor_max;       // maximum of effective,raw,angular
    int32_t window;            // main window dpi
} ui_dpi_t;

// in inches (because monitors customary are)
// it is not in points (1/72 inch) like font size
// because it is awkward to express large area
// size in typography measurements.

typedef struct ui_window_sizing_s {
    fp32_t ini_w; // initial window width in inches
    fp32_t ini_h; // 0,0 means set to min_w, min_h
    fp32_t min_w; // minimum window width in inches
    fp32_t min_h; // 0,0 means - do not care use content size
    fp32_t max_w; // maximum window width in inches
    fp32_t max_h; // 0,0 means as big as user wants
    // "sizing" "estimate or measure something's dimensions."
	// initial window sizing only used on the first invocation
	// actual user sizing is stored in the configuration and used
	// on all launches except the very first.
} ui_window_sizing_t;

typedef struct { // TODO: split to ui_app_t and ui_app_if, move data after methods
    // implemented by client:
    const char* class_name;
    // called before creating main window
    void (*init)(void);
    // called instead of init() for console apps and when .no_ui=true
    int (*main)(void);
    // class_name and init must be set before main()
    void (*opened)(void);      // window has been created and shown
    void (*every_sec)(void);   // if not null called ~ once a second
    void (*every_100ms)(void); // called ~10 times per second
    // .can_close() called before window is closed and can be
    // used in a meaning of .closing()
    bool (*can_close)(void);   // window can be closed
    void (*closed)(void);      // window has been closed
    void (*fini)(void);        // called before WinMain() return
    // must be filled by application:
    const char* title;
    ui_window_sizing_t const window_sizing;
    // TODO: struct {} visibility;
    // see: ui.visibility.*
    int32_t visibility;         // initial window_visibility state
    int32_t last_visibility;    // last window_visibility state from last run
    int32_t startup_visibility; // window_visibility from parent process
    ui_canvas_t canvas;  // set by message.paint
    // ui flags:
    bool is_full_screen;
    bool no_ui;      // do not create application window at all
    bool dark_mode;  // forced dark  mode for the whole application
    bool light_mode; // forced light mode for the whole application
    bool no_decor;   // window w/o title bar, min/max close buttons
    bool no_min;     // window w/o minimize button on title bar and sys menu
    bool no_max;     // window w/o maximize button on title bar
    bool no_size;    // window w/o maximize button on title bar
    bool no_clip;    // allows to resize window above hosting monitor size
    bool hide_on_minimize; // like task manager minimize means hide
    ui_window_t window;
    ui_icon_t icon; // may be null
    uint64_t  tid; // main thread id
    int32_t   exit_code; // application exit code
    ui_dpi_t  dpi;
    ui_rect_t wrc;  // window rectangle including non-client area
    ui_rect_t crc;  // client rectangle
    ui_rect_t mrc;  // monitor rectangle
    ui_rect_t prc;  // previously invalidated paint rectagle inside crc
    ui_rect_t work_area; // current monitor work area
    int32_t   caption_height; // caption height
    ui_wh_t   border;    // frame border size
    // not to call ut_clock.seconds() too often:
    fp64_t     now;  // ssb "seconds since boot" updated on each message
    ui_view_t* root; // show_window() changes ui.hidden
    ui_view_t* content;
    ui_view_t* caption;
    ui_view_t* focus; // does not affect message routing
    ui_fms_t   fm;
    // TODO: struct {} keyboard
    // keyboard state now:
    bool alt;
    bool ctrl;
    bool shift;
    // TODO: struct {} mouse
    // mouse buttons state
    bool mouse_swapped;
    bool mouse_left;   // left or if buttons are swapped - right button pressed
    bool mouse_middle; // rarely useful
    bool mouse_right;  // context button pressed
    ui_point_t mouse; // mouse/touchpad pointer
    ui_cursor_t cursor; // current cursor
    struct {
        ui_cursor_t arrow;
        ui_cursor_t wait;
        ui_cursor_t ibeam;
        ui_cursor_t size_nwse; // north west - south east
        ui_cursor_t size_nesw; // north east - south west
        ui_cursor_t size_we;   // west - east
        ui_cursor_t size_ns;   // north - south
        ui_cursor_t size_all;  // north - south
    } cursors;
    struct { // animation state
        ui_view_t* view;
        ui_view_t* focused; // focused view before animation started
        int32_t step;
        fp64_t time; // closing time or zero
        int32_t x; // (x,y) for tooltip (-1,y) for toast
        int32_t y; // screen coordinates for tooltip
    } animating;
    // call_later(..., delay_in_seconds, ...) can be scheduled from any thread executed
    // on UI thread
    void (*post)(ut_work_t* ui_fuzzing_work); // work.when == 0 meaning ASAP
    void (*request_redraw)(void);  // very fast <2 microseconds
    void (*draw)(void); // paint window now - bad idea do not use
    // inch to pixels and reverse translation via ui_app.dpi.window
    fp32_t  (*px2in)(int32_t pixels);
    int32_t (*in2px)(fp32_t inches);
    errno_t (*set_layered_window)(ui_color_t color, float alpha);
    bool (*is_active)(void); // is application window active
    bool (*is_minimized)(void);
    bool (*is_maximized)(void);
    bool (*focused)(void); // application window has keyboard focus
    void (*activate)(void); // request application window activation
    void (*set_title)(const char* title);
    void (*capture_mouse)(bool on); // capture mouse global input on/of
    void (*move_and_resize)(const ui_rect_t* rc);
    void (*bring_to_foreground)(void); // not necessary topmost
    void (*make_topmost)(void);   // in foreground hierarchy of windows
    void (*request_focus)(void);  // request application window keyboard focus
    void (*bring_to_front)(void); // activate() + bring_to_foreground() +
                                  // make_topmost() + request_focus()
    // measure and layout:
    void (*request_layout)(void); // requests layout on UI tree before paint()
    void (*invalidate)(const ui_rect_t* rc);
    void (*full_screen)(bool on);
    void (*set_cursor)(ui_cursor_t c);
    void (*close)(void); // attempts to close (can_close() permitting)
    // forced quit() even if can_close() returns false
    void (*quit)(int32_t ec);  // ui_app.exit_code = ec; PostQuitMessage(ec);
    ui_timer_t (*set_timer)(uintptr_t id, int32_t milliseconds); // see notes
    void (*kill_timer)(ui_timer_t id);
    void (*show_window)(int32_t show); // see show_window enum
    void (*show_toast)(ui_view_t* toast, fp64_t seconds); // toast(null) to cancel
    void (*show_hint)(ui_view_t* tooltip, int32_t x, int32_t y, fp64_t seconds);
    void (*toast_va)(fp64_t seconds, const char* format, va_list va);
    void (*toast)(fp64_t seconds, const char* format, ...);
    // caret calls must be balanced by caller
    void (*create_caret)(int32_t w, int32_t h);
    void (*show_caret)(void);
    void (*move_caret)(int32_t x, int32_t y);
    void (*hide_caret)(void);
    void (*destroy_caret)(void);
    // beep sounds:
    void (*beep)(int32_t kind);
    // registry interface:
    void (*data_save)(const char* name, const void* data, int32_t bytes);
    int32_t (*data_size)(const char* name);
    int32_t (*data_load)(const char* name, void* data, int32_t bytes); // returns bytes read
    // filename dialog:
    // const char* filter[] =
    //     {"Text Files", ".txt;.doc;.ini",
    //      "Executables", ".exe",
    //      "All Files", "*"};
    // const char* fn = ui_app.open_filename("C:\\", filter, ut_count_of(filter));
    const char* (*open_file)(const char* folder, const char* filter[], int32_t n);
    bool (*is_stdout_redirected)(void);
    bool (*is_console_visible)(void);
    int  (*console_attach)(void); // attempts to attach to parent terminal
    int  (*console_create)(void); // allocates new console
    void (*console_show)(bool b);
    // stats:
    int32_t paint_count; // number of paint calls
    fp64_t paint_time; // last paint duration in seconds
    fp64_t paint_max;  // max of last 128 paint
    fp64_t paint_avg;  // EMA of last 128 paints
    fp64_t paint_fps;  // EMA of last 128 paints
    fp64_t paint_last; // ut_clock.seconds() of last paint
    fp64_t paint_dt_min; // minimum time between 2 paints
} ui_app_t;

extern ui_app_t ui_app;

end_c
