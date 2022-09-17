/* Copyright (c) Dmitry "Leo" Kuznetsov 2021 see LICENSE at the end of file */
#ifndef qucik_defintion
#define qucik_defintion

#define _CRT_SECURE_NO_WARNINGS // shutdown MSVC _s() function suggestions
#include <assert.h>
#undef assert
#include <ctype.h>
#include <io.h>
#include <errno.h>
#include <malloc.h> // posix: <alloca.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <tgmath.h>

#ifdef __cplusplus
#define begin_c extern "C" {
#define end_c } // extern "C"
#else
#define begin_c
#define end_c
#endif

begin_c

typedef unsigned char byte;

#ifndef __cplusplus
#define thread_local __declspec(thread) // supposed to be _Thread_local
#define null NULL
#define thread_local_storage __declspec(thread) // posix: __thread
#else
#define null nullptr
#define thread_local_storage thread_local
#endif

// see: https://docs.microsoft.com/en-us/cpp/cpp/inline-functions-cpp

#define force_inline __forceinline
#define inline_c __inline // inline is only in C++ __inline in both C and C++

#ifndef countof
#define countof(a) ((int)(sizeof(a) / sizeof((a)[0])))
#endif

#ifndef max
#define max(a,b)     (((a) > (b)) ? (a) : (b))
#endif
#define maximum(a,b) (((a) > (b)) ? (a) : (b)) // prefered

#ifndef min
#define min(a,b)     (((a) < (b)) ? (a) : (b))
#endif
#define minimum(a,b) (((a) < (b)) ? (a) : (b)) // prefered

#define breakpoint() do { if (IsDebuggerPresent()) { DebugBreak(); } } while (0)

// C-runtime crt.functions()

#if defined(DEBUG)
  #define assert(b, ...) (void)((!!(b)) || \
    crt.assertion_failed(__FILE__, __LINE__, __FUNCTION__, #b, "" __VA_ARGS__))
#else
  #define assert(b, ...) (void)(0)
#endif

#define traceln(...) crt.traceline(__FILE__, __LINE__, __FUNCTION__, \
    "" __VA_ARGS__)

#define fatal_if_(condition, call, err2str, err, ...) \
    do { \
        bool _b_##__LINE__ = (condition); \
        if (_b_##__LINE__) { \
            char va[256]; \
            crt.sformat(va, sizeof(va), "" __VA_ARGS__); \
            crt.fatal(__FILE__, __LINE__, __FUNCTION__, err2str, err, call, va); \
        } \
    } while (0)

#define fatal_if(api_call, ...) \
    fatal_if_(api_call, #api_call, crt.error, crt.err(), __VA_ARGS__)

#define fatal_if_false(api_call, ...) \
    fatal_if_(!(api_call), #api_call, crt.error, crt.err(), __VA_ARGS__)

#define fatal_if_null(api_call, ...) \
    fatal_if_((api_call) == null, #api_call, crt.error, crt.err(), \
              __VA_ARGS__)

#define fatal_if_not_zero(api_call, ...) \
    do { \
        uint32_t _r_##__LINE__ = (api_call); \
        fatal_if_(_r_##__LINE__ != 0, #api_call, crt.error, _r_##__LINE__, \
                  __VA_ARGS__); \
    } while (0)

#define utf16to8(utf16) crt.utf16to8((char*) \
    alloca(crt.utf8_bytes(utf16) + 1), utf16)

#define utf8to16(s) crt.utf8to16((wchar_t*)alloca((crt.utf16_bytes(s) + 1) * \
    (int)sizeof(wchar_t)), s)

#define strprintf(s, ...) crt.sformat((s), countof(s), "" __VA_ARGS__)

typedef struct {
    int32_t (*err)(); // errno or GetLastError()
    void (*seterr)(int32_t err); // errno = err or SetLastError()
    // non-crypro strong pseudo-random number generators (thread safe)
    uint32_t (*random32)(uint32_t *state); // "Mulberry32"
    uint64_t (*random64)(uint64_t *state); // "Trust"
    int (*memmap_read)(const char* filename, void** data, int64_t* bytes);
    int (*memmap_rw)(const char* filename, void** data, int64_t* bytes);
    void (*memunmap)(void* data, int64_t bytes);
    // memmap_res() maps data from resources, do NOT unmap!
    int (*memmap_res)(const char* label, void** data, int64_t* bytes);
    void (*sleep)(double seconds);
    double (*seconds)(); // since boot
    void (*vformat)(char* utf8, int count, const char* format, va_list vl);
    void (*sformat)(char* utf8, int count, const char* format, ...);
    const char* (*error)(int32_t error);
    // do not call directly used by macros above
    int (*utf8_bytes)(const wchar_t* wcs);
    int (*utf16_bytes)(const char* s);
    char* (*utf16to8)(char* s, const wchar_t* wcs);
    wchar_t* (*utf8to16)(wchar_t* wcs, const char* s);
    void (*traceline)(const char* file, int line, const char* function,
        const char* format, ...);
    int (*assertion_failed)(const char* file, int line, const char* function,
                         const char* condition, const char* format, ...);
    void (*fatal)(const char* file, int line, const char* function,
        const char* (*err2str)(int32_t error), int32_t error,
        const char* call, const char* extra);
} crt_if;

extern crt_if crt;

typedef void* thread_t;

typedef struct {
    thread_t (*start)(void (*func)(void*), void* p);
    void (*join)(thread_t thread);
    void (*name)(const char* name); // names the thread
    void (*realtime)(); // bumps calling thread priority
} threads_if;

extern threads_if threads;

typedef struct {
    int (*option_index)(int argc, const char* argv[], const char* option);
    int (*remove_at)(int ix, int argc, const char* argv[]);
    /* argc=3 argv={"foo", "--verbose"} -> returns true; argc=1 argv={"foo"} */
    bool (*option_bool)(int *argc, const char* argv[], const char* option);
    /* argc=3 argv={"foo", "--n", "153"} -> value==153, true; argc=1 argv={"foo"}
       also handles negative values (e.g. "-153") and hex (e.g. 0xBADF00D)
    */
    bool (*option_int)(int *argc, const char* argv[], const char* option, int64_t *value);
    /* argc=3 argv={"foo", "--path", "bar"} -> returns "bar" argc=1 argv={"foo"} */
    const char* (*option_str)(int *argc, const char* argv[], const char* option);
    int (*parse)(const char* cl, const char** argv, char* buff);
} args_if;

extern args_if args;

typedef void* event_t;

typedef struct {
    event_t (*create)();
    event_t (*create_manual)();
    void (*set)(event_t e);
    void (*reset)(event_t e);
    void (*wait)(event_t e);
    // returns 0 or -1 on timeout
    int (*wait_or_timeout)(event_t e, double seconds); // seconds < 0 forever
    // returns event index or -1 on timeout or abandon
    int (*wait_any)(int n, event_t events[]); // -1 on abandon
    int (*wait_any_or_timeout)(int n, event_t e[], double milliseconds);
    void (*dispose)(event_t e);
} events_if;

extern events_if events;

// Graphic Device Interface (selected parts of Windows GDI)

typedef struct ui_point_s { int32_t x, y; } ui_point_t;
typedef struct ui_rect_s { int32_t x, y, w, h; } ui_rect_t;
typedef uint32_t color_t;

typedef struct window_s__* window_t;
typedef struct canvas_s__* canvas_t;
typedef struct bitmap_s__* bitmap_t;
typedef struct font_s__*   font_t;
typedef struct brush_s__*  brush_t;
typedef struct pen_s__*    pen_t;
typedef struct cursor_s__* cursor_t;
typedef struct region_s__* region_t;

typedef uintptr_t tm_t; // timer not the same as "id" in set_timer()!

#define rgb(r,g,b) ((color_t)(((byte)(r) | ((uint16_t)((byte)(g))<<8)) | \
    (((uint32_t)(byte)(b))<<16)))
#define rgba(r, g, b, a) (color_t)((rgb(r, g, b)) | (((byte)a) << 24))

typedef struct image_s {
    int32_t w, h, bpp;
    bitmap_t bitmap;
    void* pixels;
} image_t;

typedef struct gdi_s {
    brush_t  brush_color;
    brush_t  brush_hollow;
    pen_t pen_hollow;
    region_t clip;
    void (*image_init)(image_t* image, int32_t w, int32_t h, int32_t bpp,
        const byte* pixels); // bpp bytes (not bits!) per pixel
    void (*image_dispose)(image_t* image);
    color_t (*set_text_color)(color_t c);
    color_t (*set_brush_color)(color_t c);
    brush_t (*set_brush)(brush_t b); // color or hollow
    pen_t (*set_colored_pen)(color_t c); // always 1px wide
    pen_t (*create_pen)(color_t c, int pixels); // pixels wide pen
    pen_t (*set_pen)(pen_t p);
    void  (*delete_pen)(pen_t p);
    void (*set_clip)(int32_t x, int32_t y, int32_t w, int32_t h);
    // use set_clip(0, 0, 0, 0) to clear clip region
    void (*push)(int32_t x, int32_t y); // also calls SaveDC(app.canvas)
    void (*pop)(); // also calls RestoreDC(-1, app.canvas)
    ui_point_t (*move_to)(int32_t x, int32_t y); // returns previous (x, y)
    void (*line)(int32_t x, int32_t y); // line to x, y with gdi.pen moves x, y
    void (*rect)(int32_t x, int32_t y, int32_t w, int32_t h); // app.pen
    void (*fill)(int32_t x, int32_t y, int32_t w, int32_t h); // app.brush
    void (*poly)(ui_point_t* points, int32_t count);
    void (*rounded)(int32_t x, int32_t y, int32_t w, int32_t h,
        int32_t rx, int32_t ry); // see RoundRect, pen, brush
    void (*gradient)(int32_t x, int32_t y, int32_t w, int32_t h,
        color_t rgba_from, color_t rgba_to, bool vertical);
    // images: (x,y remains untouched after swaring)
    void (*draw_greyscale)(int32_t sx, int32_t sy, int32_t sw, int32_t sh,
        int32_t x, int32_t y, int32_t w, int32_t h, const byte* pixels);
    void (*alpha_blend)(int32_t x, int32_t y, int32_t w, int32_t h,
        image_t* image, double alpha);
    void (*draw_image)(int32_t x, int32_t y, int32_t w, int32_t h,
        image_t* image);
    // text:
    font_t (*font)(font_t f, int height); // custom font
    void   (*delete_font)(font_t f);
    font_t (*set_font)(font_t f);
    int    (*descent)(font_t f);  // font descent (glyphs blow baseline)
    int    (*baseline)(font_t f); // height - baseline (aka ascent) = descent
    ui_point_t (*get_em)(font_t f); // pixel size of glyph "M"
    ui_point_t (*measure_text)(font_t f, const char* format, ...);
    // width can be -1 which measures text with "\n" or
    // positive number of pixels
    ui_point_t (*measure_multiline)(font_t f, int w, const char* format, ...);
    double height_multiplier; // see line_spacing()
    double (*line_spacing)(double height_multiplier); // default 1.0
    int32_t x; // incremented by text, print
    int32_t y; // incremented by textln, println
    // proportional:
    void (*vtext)(const char* format, va_list vl); // x += width
    void (*text)(const char* format, ...);         // x += width
    // gdi.y += height * line_spacing
    void (*vtextln)(const char* format, va_list vl);
    void (*textln)(const char* format, ...);
    // mono:
    void (*vprint)(const char* format,va_list vl); // x += width
    void (*print)(const char* format, ...);        // x += width
    // gdi.y += height * line_spacing
    void (*vprintln)(const char* format, va_list vl);
    void (*println)(const char* format, ...);
    // multiline(null, formate, ...) only increments app.y
    ui_point_t (*multiline)(int width, const char* format, ...);
} gdi_t;

extern gdi_t gdi;

typedef struct colors_s {
    const int none; // aka CLR_INVALID in wingdi
    const int text;
    const int white;
    const int black;
    const int red;
    const int green;
    const int blue;
    const int yellow;
    const int cyan;
    const int magenta;
    const int gray;
    // darker shades of grey:
    const int dkgray1; // 30 / 255 = 11.7%
    const int dkgray2; // 38 / 255 = 15%
    const int dkgray3; // 45 / 255 = 17.6%
    const int dkgray4; // 63 / 255 = 24.0%
    // tone down RGB colors:
    const int tone_white;
    const int tone_red;
    const int tone_green;
    const int tone_blue;
    const int tone_yellow;
    const int tone_cyan;
    const int tone_magenta;
    // misc:
    const int orange;
    const int dkgreen;
    // highlights:
    const int text_highlight; // bluish off-white
    const int blue_highlight;
    const int off_white;

    const int btn_gradient_darker;
    const int btn_gradient_dark;
    const int btn_hover_highlight;
    const int btn_disabled;
    const int btn_armed;
    const int btn_text;
    const int toast; // toast background
} colors_t;

extern colors_t colors;

// UIC User Interface Controls

enum {
    uic_tag_container  = 'cnt',
    uic_tag_messagebox = 'mbx',
    uic_tag_button     = 'btn',
    uic_tag_checkbox   = 'cbx',
    uic_tag_slider     = 'sld',
    uic_tag_text       = 'txt'
};

typedef struct uic_s uic_t;

typedef struct uic_s { // ui element container/control
    int32_t tag;
    void (*init)(uic_t* ui); // called once before first layout
    uic_t** children; // null terminated array[] of children
    double width;    // > 0 width of UI element in "em"s
    char text[2048];
    uic_t* parent;
    int32_t x;
    int32_t y;
    int32_t w;
    int32_t h;
    // updated on layout() call
    ui_point_t em; // cached pixel dimensions of "M"
    int32_t shortcut; // keyboard shortcut
    int32_t strid; // 0 for not localized ui
    void* that;  // for the application use
    void (*notify)(uic_t* ui); // for the application use
    // two pass layout: measure() .w, .h layout() .x .y
    // first  measure() bottom up - children.layout before parent.layout
    // second layout() top down - parent.layout before children.layout
    void (*measure)(uic_t* ui); // determine w, h (bottom up)
    void (*layout)(uic_t* ui);  // set x, y possibly adjust w, h (top down)
    void (*localize)(uic_t* ui); // set strid based ui .text field
    void (*paint)(uic_t* ui);
    bool (*message)(uic_t* ui, int32_t message, int64_t wp, int64_t lp,
        int64_t* rt); // return true and value in rt to stop processing
    void (*click)(uic_t* ui); // interpretation depends on ui element
    void (*mouse)(uic_t* ui, int32_t message, int32_t flags);
    void (*mousewheel)(uic_t* ui, int32_t dx, int32_t dy); // touchpad scroll
    void (*context_menu)(uic_t* ui); // right mouse click or long press
    void (*keyboard)(uic_t* ui, int32_t character);
    void (*key_down)(uic_t* ui, int32_t key);
    void (*key_up)(uic_t* ui, int32_t key);
    void (*hovering)(uic_t* ui, bool start);
    void (*invalidate)(uic_t* ui); // more prone to delays than app.redraw()
    // timer() periodically() and once_upon_a_second() called
    // even for hidden and disabled ui elements
    void (*timer)(uic_t* ui, tm_t id);
    void (*periodically)(uic_t* ui); // ~10 x times per second
    void (*once_upon_a_second)(uic_t* ui); // ~once a second
    bool hidden; // paint() is not called on hidden
    bool armed;
    bool hover;
    bool pressed;   // for uic_button_t and  uic_checkbox_t
    bool disabled;  // mouse, keyboard, key_up/down not called on disabled
    bool focusable; // can be target for keyboard focus
    double  hover_delay; // delta time in seconds before hovered(true)
    double  hover_at;    // time in seconds when to call hovered()
    color_t color;      // interpretation depends on ui element type
    color_t background; // interpretation depends on ui element type
    font_t* font;
    int32_t baseline; // font ascent; descent = height - baseline
    int32_t descent;  // font descent
    char    tip[256]; // tooltip text
} uic_t;

typedef struct layouts_s {
    void (*center)(uic_t* ui); // exactly one child
    void (*horizontal)(uic_t* ui, int x, int y, int gap);
    void (*vertical)(uic_t* ui, int x, int y, int gap);
    void (*measure_grid)(uic_t* ui, int gap_h, int gap_v);
    void (*layout_grid)(uic_t* ui, int gap_h, int gap_v);
} layouts_t;

extern layouts_t layouts;

#define uic_container(name, init, ...) \
static uic_t* _ ## name ## children ## _[] = {__VA_ARGS__, null}; \
uic_t name = { uic_tag_container, init, (_ ## name ## children ## _) }

#define static_uic_container(name, init, ...) \
static uic_t* _ ## name ## children ## _[] = {__VA_ARGS__, null}; \
static uic_t name = { uic_tag_container, init, (_ ## name ## children ## _) }

typedef struct uic_text_s {
    uic_t ui;
    bool multiline;
    bool editable;  // can be edited
    bool highlight; // paint with highlight color
    bool hovered;   // paint highligh rectangle when hover over
    bool label;     // do not copy text to clipboard, do not highligh
    int32_t dy; // vertical shift down (to line up baselines of diff fonts)
} uic_text_t;

void _uic_text_init_(uic_t* ui); // do not call use ui_text() and ui_multiline()

#define uic_text(t, s) \
    uic_text_t t = { .ui = {.tag = uic_tag_text, .init = _uic_text_init_, \
    .children = null, .width = 0.0, .text = s}, .multiline = false}
#define uic_multiline(t, w, s) \
    uic_text_t t = {.ui = {.tag = uic_tag_text, .init = _uic_text_init_, \
    .children = null, .width = w, .text = s}, .multiline = true}

// single line of text with "&" keyboard shortcuts:
void uic_text_vinit(uic_text_t* t, const char* format, va_list vl);
void uic_text_init(uic_text_t* t, const char* format, ...);
// multiline
void uic_text_init_ml(uic_text_t* t, double width, const char* format, ...);

typedef struct uic_button_s uic_button_t;

typedef struct uic_button_s {
    uic_t ui;
    void (*cb)(uic_button_t* b); // callback
    double armed_until;   // seconds - when to release
} uic_button_t;

void uic_button_init(uic_button_t* b, const char* label, double ems,
    void (*cb)(uic_button_t* b));

void _uic_button_init_(uic_t* ui); // do not call use uic_button() macro

#define uic_button(name, s, w, code)                     \
    static void name ## _callback(uic_button_t* b) {     \
        (void)b; /* no warning if unused */              \
        code                                             \
    }                                                    \
                                                         \
    uic_button_t name = {                                \
    .ui = {.tag = uic_tag_button, .init = _uic_button_init_, \
    .children = null, .width = w, .text = s}, .cb = name ## _callback }

// usage:
// uic_button(button, 7.0, "&Button", { b->ui.pressed = !b->ui.pressed; })

typedef struct  uic_checkbox_s  uic_checkbox_t; // checkbox

typedef struct  uic_checkbox_s {
    uic_t ui;
    void (*cb)( uic_checkbox_t* b); // callback
}  uic_checkbox_t;

// label may contain "___" which will be replaced with "On" / "Off"
void  uic_checkbox_init( uic_checkbox_t* b, const char* label, double ems,
    void (*cb)( uic_checkbox_t* b));

void _uic_checkbox_init_(uic_t* ui); // do not call use uic_checkbox() macro

#define uic_checkbox(name, s, w, code)                       \
    static void name ## _callback(uic_checkbox_t* c) {       \
        (void)c; /* no warning if unused */                  \
        code                                                 \
    }                                                        \
                                                             \
    uic_checkbox_t name = {                                  \
    .ui = {.tag = uic_tag_checkbox, .init = _uic_checkbox_init_, \
    .children = null, .width = w, .text = s}, .cb = name ## _callback }

typedef struct uic_slider_s uic_slider_t;

typedef struct uic_slider_s {
    uic_t ui;
    void (*cb)(uic_slider_t* b); // callback
    int32_t step;
    double time;   // time last button was pressed
    ui_point_t tm; // text measurement (special case for %0*d)
    uic_button_t inc;
    uic_button_t dec;
    uic_t* buttons[3]; // = { dec, inc, null }
    int32_t value;  // for uic_slider_t range slider control
    int32_t vmin;
    int32_t vmax;
} uic_slider_t;

void uic_slider_init(uic_slider_t* r, const char* label, double ems,
    int32_t vmin, int32_t vmax, void (*cb)(uic_slider_t* r));

typedef struct uic_messagebox_s uic_messagebox_t;

typedef struct uic_messagebox_s {
    uic_t  ui;
    void (*cb)(uic_messagebox_t* m, int option); // callback -1 on cancel
    uic_text_t text;
    uic_button_t button[16];
    uic_t* children[17];
    int option; // -1 or option chosen by user
    const char** opts;
} uic_messagebox_t;

void uic_messagebox_init(uic_messagebox_t* mx, const char* option[],
    void (*cb)(uic_messagebox_t* m, int option), const char* format, ...);

void _uic_messagebox_init_(uic_t* ui); // do not call use uic_checkbox() macro

#define uic_messagebox(name, s, code, ...)                           \
                                                                     \
    static char* name ## _options[] = { __VA_ARGS__, null };         \
                                                                     \
    static void name ## _callback(uic_messagebox_t* m, int option) { \
        (void)m; (void)option; /* no warnings if unused */           \
        code                                                         \
    }                                                                \
                                                                     \
    uic_messagebox_t name = {                                        \
    .ui = {.tag = uic_tag_messagebox, .init = _messagebox_init_,     \
    .children = null, .text = s}, .opts = name ## _options,          \
    .cb = name ## _callback }

// Subsystem:WINDOWS single window application

typedef struct window_visibility_s {
    const int hide;
    const int normal;   // should be use for first .show()
    const int minimize; // activate and minimize
    const int maximize; // activate and maximinze
    const int normal_na;// same as .normal but no activate
    const int show;     // shows and activates in current size and position
    const int min_next; // minimize and activate next window in Z order
    const int min_na;   // minimize but do not activate
    const int show_na;  // same as .show but no activate
    const int restore;  // from min/max to normal window size/pos
    const int defau1t;  // use Windows STARTUPINFO value
    const int force_min;// minimize even if dispatch thread not responding
} window_visibility_t;

extern window_visibility_t window_visibility;

typedef struct dpi_s { // max(dpi_x, dpi_y)
    int32_t system;  // system dpi
    int32_t process; // process dpi
    // 15" diagonal monitor 3840x2160 175% scaled
    // monitor dpi effective 168, angular 248 raw 284
    int32_t monitor_effective; // effective with regard of user scaling
    int32_t monitor_raw;       // with regard of physical screen size
    int32_t monitor_angular;   // diagonal raw
    int32_t window; // main window dpi
} dpi_t;

typedef struct fonts_s {
    // font handles re-created on scale change
    font_t regular; // proprtional UI font
    font_t mono; // monospaced  UI font
    font_t H1; // bold header font
    font_t H2;
    font_t H3;
} fonts_t;

enum {
    known_folder_home      = 0, // c:\Users\<username>
    known_folder_desktop   = 1,
    known_folder_documents = 2,
    known_folder_downloads = 3,
    known_folder_music     = 4,
    known_folder_pictures  = 5,
    known_folder_videos    = 6,
    known_folder_shared    = 7, // c:\Users\Public
    known_folder_bin       = 8, // c:\ProgramFiles
    known_folder_data      = 9  // c:\ProgramData
};

typedef struct uic_s uic_t;

// once_upon_a_second() and periodically() also called on all UICs

typedef struct app_s {
    // implemented by client:
    const char* class_name;
    // called before creating main window
    void (*init)();
    // called instead of init() for console apps and when .no_ui=true
    int (*main)();
    // class_name and init must be set before main()
    void (*openned)(); // window has been created and shown
    void (*once_upon_a_second)(); // if not null called ~ once a second
    void (*periodically)(); // called ~10 times per second
    bool (*can_close)();  // window can be closed
    void (*closed)();  // window has been closed
    void (*fini)(); // called before WinMain() return
    // must be filled by application:
    const char* title;
    // min/max width/heigh are prefilled according to monitor size
    int32_t min_width;  // client width
    int32_t min_height; // client height
    int32_t max_width;
    int32_t max_height;
    int32_t visibility; // initial window_visibility state
    int32_t last_visibility;  // last window_visibility state from last run
    int32_t startup_visibility; // window_visibility from parent process
    bool is_full_screen;
    // ui flags:
    bool no_ui; // do not create main window at all
    bool no_decor; // main window w/o title bar, min/max close buttons
    bool no_min_max; // main window w/o min/max buttons
    bool aero; // retro Windows 7 decoration (just for the fun of it)
    // main(argc, argv)
    int32_t argc;
    const char** argv;
    const char** command_line;
    // application exit code:
    int32_t exit_code;
    // drawing context:
    dpi_t dpi;
    window_t window;
    ui_rect_t wrc;  // window rectangle incl non-client area
    ui_rect_t crc;  // client rectangle
    ui_rect_t mrc;  // monitor rectangle
    ui_rect_t work_area; // current monitor work area
    int32_t width;  // client width
    int32_t height; // client height
    // not to call clocks.seconds() too often:
    double now; // ssb "seconds since boot" updated on each message
    uic_t* ui; // show_window() changes ui.hidden
    fonts_t fonts;
    cursor_t cursor; // current cursor
    cursor_t cursor_arrow;
    cursor_t cursor_wait;
    // keyboard state now:
    bool alt;
    bool ctrl;
    bool shift;
    ui_point_t mouse; // mouse/touchpad pointer
    canvas_t canvas; // set by WM_PAINT message
    // i18n
    // strid("foo") returns 0 if there is no matching ENGLISH NEUTRAL
    // STRINGTABLE entry
    int (*strid)(const char* s);
    // given strid > 0 returns localized string or defau1t value
    const char* (*string)(int strid, const char* defau1t);
    // nls(s) is same as string(strid(s), s)
    const char* (*nls)(const char* defau1t); // national localized string
    const char* (*locale)(); // "en-US" "zh-CN" etc...
    // force locale for debugging and testing:
    void (*set_locale)(const char* locale); // only for calling thread
    // layout:
    void (*layout)(); // requests layout on UI tree before paint()
    void (*invalidate)(ui_rect_t* rc);
    void (*full_screen)(bool on);
    void (*redraw)(); // very fast (5 microseconds) InvalidateRect(null)
    void (*draw)();   // UpdateWindow()
    void (*set_cursor)(cursor_t c);
    void (*close)(); // window
    tm_t (*set_timer)(uintptr_t id, int32_t milliseconds); // see notes
    void (*kill_timer)(tm_t id);
    void (*show_window)(int32_t show); // see show_window enum
    void (*show_toast)(uic_t* toast, double seconds); // toast(null) to cancel
    void (*show_tooltip)(uic_t* tooltip, int x, int y, double seconds);
    void (*vtoast)(double seconds, const char* format, va_list vl);
    void (*toast)(double seconds, const char* format, ...);
    // registry interface:
    void (*data_save)(const char* name, const void* data, int bytes);
    int  (*data_size)(const char* name);
    int  (*data_load)(const char* name, void* data, int bytes);
    // filename dialog:
    // const char* filter[] =
    //     {"Text Files", ".txt;.doc;.ini",
    //      "Executables", ".exe",
    //      "All Files", "*"};
    // const char* fn = app.open_filename("C:\\", filter, countof(filter));
    const char* (*open_filename)(const char* folder, const char* filter[], int n);
    const char* (*known_folder)(int kfid);
    // attempts to attach to parent command line console:
    void (*attach_console)();
    // stats:
    int32_t paint_count; // number of paint calls
    double paint_time; // last paint duration in seconds
    double paint_max;  // max of last 128 paint
    double paint_avg;  // EMA of last 128 paints
} app_t;

extern app_t app;

typedef struct clipboard_s {
    int (*copy_text)(const char* s); // returns error or 0
    int (*copy_bitmap)(image_t* im); // returns error or 0
} clipboard_t;

extern clipboard_t clipboard;

typedef struct messages_s {
    const int left_button_down;
    const int left_button_up;
    const int right_button_down;
    const int right_button_up;
    const int mouse_move;
} messages_t;

extern messages_t messages;

typedef struct mouse_flags_s { // which buttons are pressed
    const int left_button;
    const int right_button;
} mouse_flags_t;

extern mouse_flags_t mouse_flags;

typedef struct virtual_keys_s {
    const int up;
    const int down;
    const int left;
    const int right;
} virtual_keys_t;

extern virtual_keys_t virtual_keys;

#endif qucik_defintion

#if defined(quick_implementation) || defined(quick_implementation_console)
#undef quick_implementation

// CRT implementation

static int args_option_index(int argc, const char* argv[], const char* option) {
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--") == 0) { break; } // no options after '--'
        if (strcmp(argv[i], option) == 0) { return i; }
    }
    return -1;
}

static int args_remove_at(int ix, int argc, const char* argv[]) { // returns new argc
    assert(0 < argc);
    assert(0 < ix && ix < argc); // cannot remove argv[0]
    for (int i = ix; i < argc; i++) {
        argv[i] = argv[i+1];
    }
    argv[argc - 1] = "";
    return argc - 1;
}

static bool args_option_bool(int *argc, const char* argv[], const char* option) {
    int ix = args_option_index(*argc, argv, option);
    if (ix > 0) {
        *argc = args_remove_at(ix, *argc, argv);
    }
    return ix > 0;
}

static bool args_option_int(int *argc, const char* argv[], const char* option, int64_t *value) {
    int ix = args_option_index(*argc, argv, option);
    if (ix > 0 && ix < *argc - 1) {
        const char* s = argv[ix + 1];
        int base = (strstr(s, "0x") == s || strstr(s, "0X") == s) ? 16 : 10;
        const char* b = s + (base == 10 ? 0 : 2);
        char* e = null;
        errno = 0;
        int64_t v = strtoll(b, &e, base);
        if (errno == 0 && e > b && *e == 0) {
            *value = v;
        } else {
            ix = -1;
        }
    } else {
        ix = -1;
    }
    if (ix > 0) {
        *argc = args_remove_at(ix, *argc, argv); // remove option
        *argc = args_remove_at(ix, *argc, argv); // remove following number
    }
    return ix > 0;
}

static const char* args_option_str(int *argc, const char* argv[], const char* option) {
    int ix = args_option_index(*argc, argv, option);
    const char* s = null;
    if (ix > 0 && ix < *argc - 1) {
        s = argv[ix + 1];
    } else {
        ix = -1;
    }
    if (ix > 0) {
        *argc = args_remove_at(ix, *argc, argv); // remove option
        *argc = args_remove_at(ix, *argc, argv); // remove following string
    }
    return ix > 0 ? s : null;
}

static const char BACKSLASH = '\\';
static const char QUOTE = '\"';

static char next_char(const char** cl, int* escaped) {
    char ch = **cl;
    (*cl)++;
    *escaped = false;
    if (ch == BACKSLASH) {
        if (**cl == BACKSLASH) {
            (*cl)++;
            *escaped = true;
        } else if (**cl == QUOTE) {
            ch = QUOTE;
            (*cl)++;
            *escaped = true;
        } else { /* keep the backslash and copy it into the resulting argument */ }
    }
    return ch;
}

static int args_parse(const char* cl, const char** argv, char* buff) {
    int escaped = 0;
    int argc = 0;
    int j = 0;
    char ch = next_char(&cl, &escaped);
    while (ch != 0) {
        while (isspace(ch)) { ch = next_char(&cl, &escaped); }
        if (ch == 0) { break; }
        argv[argc++] = buff + j;
        if (ch == QUOTE) {
            ch = next_char(&cl, &escaped);
            while (ch != 0) {
                if (ch == QUOTE && !escaped) { break; }
                buff[j++] = ch;
                ch = next_char(&cl, &escaped);
            }
            buff[j++] = 0;
            if (ch == 0) { break; }
            ch = next_char(&cl, &escaped); // skip closing quote maerk
        } else {
            while (ch != 0 && !isspace(ch)) {
                buff[j++] = ch;
                ch = next_char(&cl, &escaped);
            }
            buff[j++] = 0;
        }
    }
    return argc;
}

args_if args = {
    args_option_index,
    args_remove_at,
    args_option_bool,
    args_option_int,
    args_option_str,
    args_parse
};

static void crt_vformat(char* utf8, int count, const char* format, va_list vl) {
    vsnprintf(utf8, count, format, vl);
    utf8[count - 1] = 0;
}

static void crt_sformat(char* utf8, int count, const char* format, ...) {
    va_list vl;
    va_start(vl, format);
    crt.vformat(utf8, count, format, vl);
    va_end(vl);
}

end_c

#ifdef _WIN32 // it is possible and trivial to implement for other platforms

#pragma once
#if !defined(STRICT)
#define STRICT
#endif
#define WIN32_LEAN_AND_MEAN
#define VC_EXTRALEAN
#define _CRT_NONSTDC_NO_WARNINGS
#ifndef _SCL_SECURE_NO_WARNINGS
#define _SCL_SECURE_NO_WARNINGS
#endif
#include <Windows.h>
#include <WindowsX.h>
#include <timeapi.h>

begin_c

static void crt_fatal(const char* file, int line, const char* function,
        const char* (*err2str)(int32_t err), int32_t error,
        const char* call, const char* extra) {
    crt.traceline(file, line, function, "FATAL: %s failed %d 0x%08X \"%s\" %s",
        call, error, error, err2str(error), extra);
    breakpoint();
    if (file != null) { ExitProcess(error); }
}

static const char* crt_error(int32_t error) {
    static thread_local_storage char text[256];
    const DWORD neutral = MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT);
    const DWORD format = FORMAT_MESSAGE_FROM_SYSTEM|
        FORMAT_MESSAGE_IGNORE_INSERTS;
    wchar_t s[256];
    HRESULT hr = 0 <= error && error <= 0xFFFF ?
        HRESULT_FROM_WIN32(error) : error;
    if (FormatMessageW(format, null, hr, neutral, s, countof(s) - 1, (va_list*)null) > 0) {
        s[countof(s) - 1] = 0;
        // remove trailing '\r\n'
        int k = (int)wcslen(s);
        if (k > 0 && s[k - 1] == '\n') { s[k - 1] = 0; }
        k = (int)wcslen(s);
        if (k > 0 && s[k - 1] == '\r') { s[k - 1] = 0; }
        strprintf(text, "0x%08X(%d) \"%s\"", error, error, utf16to8(s));
    } else {
        strprintf(text, "0x%08X(%d)", error, error);
    }
    return text;
}

static void crt_sleep(double seconds) {
    assert(seconds >= 0);
    if (seconds < 0) { seconds = 0; }
    int64_t ns100 = (int64_t)(seconds * 1.0e+7); // in 0.1 microsecond aka 100ns
    typedef int (__stdcall *nt_delay_execution_t)(BOOLEAN Alertable, PLARGE_INTEGER DelayInterval);
    static nt_delay_execution_t NtDelayExecution;
    // delay in 100-ns units. negative value means delay relative to current.
    LARGE_INTEGER delay; // delay in 100-ns units.
    delay.QuadPart = -ns100; // negative value means delay relative to current.
    if (NtDelayExecution == null) {
        HMODULE ntdll = LoadLibraryA("ntdll.dll");
        fatal_if_null(ntdll);
        NtDelayExecution = (nt_delay_execution_t)GetProcAddress(ntdll, "NtDelayExecution");
        fatal_if_null(NtDelayExecution);
    }
    //  If "alertable" is set, execution can break in a result of NtAlertThread call.
    NtDelayExecution(false, &delay);
}

static double crt_seconds() { // since_boot
    LARGE_INTEGER qpc;
    QueryPerformanceCounter(&qpc);
    static double one_over_freq;
    if (one_over_freq == 0) {
        LARGE_INTEGER frequency;
        QueryPerformanceFrequency(&frequency);
        one_over_freq = 1.0 / frequency.QuadPart;
    }
    return (double)qpc.QuadPart * one_over_freq;
}

enum {
    NS_IN_US = 1000, // nanoseconds in microsecond
    NS_IN_MS = NS_IN_US * 1000 // nanoseconds in millisecond
};

static int64_t ns2ms(int64_t ns) { return (ns + NS_IN_MS - 1) / NS_IN_MS; }

typedef int (*nt_query_timer_resolution_t)(ULONG* minimum_resolution,
    ULONG* maximum_resolution, ULONG* actual_resolution);
typedef int (*nt_settimer_resolution_t)(ULONG RequestedResolution,
    BOOLEAN Set, ULONG* ActualResolution);

static int crt_scheduler_set_timer_resolution(int64_t ns) { // nanoseconds
    const int ns100 = (int)(ns / 100);
    nt_query_timer_resolution_t NtQueryTimerResolution =
        (nt_query_timer_resolution_t)
        GetProcAddress(LoadLibraryA("NtDll.dll"), "NtQueryTimerResolution");
    nt_settimer_resolution_t NtSetTimerResolution = (nt_settimer_resolution_t)
        GetProcAddress(LoadLibraryA("NtDll.dll"), "NtSetTimerResolution");
    // it is resolution not frequency this is why it is in reverse
    // to common sense and what is not on Windows?
    unsigned long min_100ns = 16 * 10 * 1000;
    unsigned long max_100ns = 1 * 10 * 1000;
    unsigned long actual_100ns = 0;
    int r = 0;
    if (NtQueryTimerResolution != null &&
        NtQueryTimerResolution(&min_100ns, &max_100ns, &actual_100ns) == 0) {
        int64_t minimum_ns = min_100ns * 100LL;
        int64_t maximum_ns = max_100ns * 100LL;
//      int64_t actual_ns  = actual_100ns  * 100LL;
        // note that maximum resolution is actually < minimum
        if (NtSetTimerResolution == null) {
            const int milliseconds = (int)(ns2ms(ns) + 0.5);
            r = (int)maximum_ns <= ns && ns <= (int)minimum_ns ?
                timeBeginPeriod(milliseconds) : ERROR_INVALID_PARAMETER;
        } else {
            r = (int)maximum_ns <= ns && ns <= (int)minimum_ns ?
                NtSetTimerResolution(ns100, true, &actual_100ns) :
                ERROR_INVALID_PARAMETER;
        }
        NtQueryTimerResolution(&min_100ns, &max_100ns, &actual_100ns);
    } else {
        const int milliseconds = (int)(ns2ms(ns) + 0.5);
        r = 1 <= milliseconds && milliseconds <= 16 ?
            timeBeginPeriod(milliseconds) : ERROR_INVALID_PARAMETER;
    }
    return r;
}

static void crt_power_throttling_disable_for_process() {
    static bool disabled_for_the_process;
    if (!disabled_for_the_process) {
        PROCESS_POWER_THROTTLING_STATE pt = { 0 };
        pt.Version = PROCESS_POWER_THROTTLING_CURRENT_VERSION;
        pt.ControlMask = PROCESS_POWER_THROTTLING_EXECUTION_SPEED |
            PROCESS_POWER_THROTTLING_IGNORE_TIMER_RESOLUTION;
        pt.StateMask = 0;
        fatal_if_false(SetProcessInformation(GetCurrentProcess(),
            ProcessPowerThrottling, &pt, sizeof(pt)));
        disabled_for_the_process = true;
    }
}

static void crt_power_throttling_disable_for_thread(HANDLE thread) {
    THREAD_POWER_THROTTLING_STATE pt = { 0 };
    pt.Version = THREAD_POWER_THROTTLING_CURRENT_VERSION;
    pt.ControlMask = THREAD_POWER_THROTTLING_EXECUTION_SPEED;
    pt.StateMask = 0;
    fatal_if_false(SetThreadInformation(thread, ThreadPowerThrottling, &pt, sizeof(pt)));
}

static void crt_disable_power_throttling() {
    crt_power_throttling_disable_for_process();
    crt_power_throttling_disable_for_thread(GetCurrentThread());
}

static void threads_realtime() {
    fatal_if_false(SetPriorityClass(GetCurrentProcess(),
        REALTIME_PRIORITY_CLASS));
    fatal_if_false(SetThreadPriority(GetCurrentThread(),
        THREAD_PRIORITY_TIME_CRITICAL));
    fatal_if_false(SetThreadPriorityBoost(GetCurrentThread(),
        /* bDisablePriorityBoost = */ false));
    fatal_if_not_zero(
        crt_scheduler_set_timer_resolution(NS_IN_MS));
    crt_disable_power_throttling();
}

static thread_t threads_start(void (*func)(void*), void* p) {
    thread_t t = (thread_t)CreateThread(null, 0,
        (LPTHREAD_START_ROUTINE)func, p, 0, null);
    fatal_if_null(t);
    return t;
}

static void threads_join(thread_t t) {
    DWORD flags = 0;
    fatal_if_false(GetHandleInformation(t, &flags));
    fatal_if_null(t);
    int r = WaitForSingleObject(t, INFINITE);
    fatal_if(r != 0);
    fatal_if_false(CloseHandle(t));
}

static void threads_name(const char* name) {
    HRESULT r = SetThreadDescription(GetCurrentThread(), utf8to16(name));
    // notoriously returns 0x10000000 for no good reason whatsoever
    if (!SUCCEEDED(r)) { fatal_if_not_zero(r); }
}

threads_if threads = {
    .start = threads_start,
    .join = threads_join,
    .name = threads_name,
    .realtime = threads_realtime
};

static event_t events_create() {
    HANDLE e = null;
    fatal_if_null(e = CreateEvent(null, false, false, null));
    return (event_t)e;
}

static event_t events_create_manual() {
    HANDLE e = null;
    fatal_if_null(e = CreateEvent(null, true, false, null));
    return (event_t)e;
}

static void events_set(event_t e) {
    fatal_if_false(SetEvent((HANDLE)e));
}

static void events_reset(event_t e) {
    fatal_if_false(ResetEvent((HANDLE)e));
}

static int events_wait_or_timeout(event_t e, double seconds) {
    uint32_t ms = seconds < 0 ? INFINITE : (int32_t)(seconds * 1000.0 + 0.5);
    uint32_t r = 0;
    fatal_if_false((r = WaitForSingleObject(e, ms)) != WAIT_FAILED);
    return r == WAIT_OBJECT_0 ? 0 : -1; // all WAIT_ABANDONED as -1
}

static void events_wait(event_t e) { events_wait_or_timeout(e, -1); }

static int events_wait_any_or_timeout(int n, event_t events_[], double s) {
    uint32_t ms = s < 0 ? INFINITE : (int32_t)(s * 1000.0 + 0.5);
    uint32_t r = 0;
    fatal_if_false((r = WaitForMultipleObjects(n, events_, false, ms)) != WAIT_FAILED);
    // all WAIT_ABANDONED_0 and WAIT_IO_COMPLETION 0xC0 as -1
    return WAIT_OBJECT_0 <= r && r < WAIT_OBJECT_0 + n ? r - WAIT_OBJECT_0 : -1;
}

static int events_wait_any(int n, event_t e[]) {
    return events_wait_any_or_timeout(n, e, -1);
}

static void handles_close(event_t handle) {
    fatal_if_false(CloseHandle(handle));
}

events_if events = {
    events_create,
    events_create_manual,
    events_set,
    events_reset,
    events_wait,
    events_wait_or_timeout,
    events_wait_any,
    events_wait_any_or_timeout,
    handles_close
};

static void utf8_ods(const char* s) { OutputDebugStringW(utf8to16(s)); }

enum { NO_LINEFEED = -2 };

#define gettid() (GetCurrentThreadId())

static int vformat_(char* s, int count, const char* file, int line,
    const char* function, const char* format, va_list vl) {
    s[0] = 0;
    char* sb = s;
    int left = count - 1;
    if (file != null && line >= 0) {
        crt.sformat(sb, left, "%s(%d): [%05d] %s ", file, line, GetCurrentThreadId(), function);
        int n = (int)strlen(sb);
        sb += n;
        left -= n;
    }
    crt.vformat(sb, left, format, vl);
    int k = (int)strlen(sb);
    sb += k;
    if (k == 0 || sb[-1] != '\n') {
        *sb = '\n';
        sb++;
        *sb = 0;
    }
    return (int)(sb - s);
}

// traceln() both OutputDebugStringA() and vfprintf() are subject
// of synchronizarion/seralization and may enter wait state which
// in combination with Windows scheduler can take milliseconds to
// complete. In time critical threads use traceln() instead which
// does NOT lock at all and completes in microseconds

static void vtraceln(const char* file, int line, const char* function, const char* format, va_list vl) {
    enum { max_ods_count = 32 * 1024 - 1 };
    char s[max_ods_count * 2 + 1];
    const char* basename = strrchr(file, '/');
    if (basename == (void*)0) { basename = strrchr(file, '\\'); }
    basename = basename != (void*)0 ? basename + 1 : file;
    (void)vformat_(s, (int)countof(s), file, line, function, format, vl);
    utf8_ods(s);
    if (GetStdHandle(STD_ERROR_HANDLE) != null) {
        fprintf(stderr, "%s(%d): [%05d] %s ", (file + (int)(basename - file)), line,
            gettid(), function);
        vfprintf(stderr, format, vl);
        fprintf(stderr, "\n");
    }
}

static void crt_traceline(const char* path, int line, const char* function, const char* format, ...) {
    va_list vl;
    va_start(vl, format);
    vtraceln(path, line, function, format, vl);
    va_end(vl);
}

static int crt_utf8_bytes(const wchar_t* utf16) {
    int required_bytes_count = WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS,
        utf16, -1, null, 0, null, null);
    assert(required_bytes_count > 0);
    return required_bytes_count;
}

static int crt_utf16_bytes(const char* utf8) {
    int required_bytes_count = MultiByteToWideChar(CP_UTF8, 0, utf8, -1, null, 0);
    assert(required_bytes_count > 0);
    return required_bytes_count;
}

static char* crt_utf16to8(char* s, const wchar_t* utf16) {
    int r = WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS, utf16, -1, s,
        crt.utf8_bytes(utf16), null, null);
    if (r == 0) {
        traceln("WideCharToMultiByte() failed %s", crt.error(crt.err()));
    }
    return s;
}

static wchar_t* crt_utf8to16(wchar_t* utf16, const char* s) {
    int r = MultiByteToWideChar(CP_UTF8, 0, s, -1, utf16, crt.utf16_bytes(s));
    if (r == 0) {
        traceln("WideCharToMultiByte() failed %d", crt.err());
    }
    return utf16;
}

static int crt_assertion_failed(const char* file, int line, const char* function,
                      const char* condition, const char* format, ...) {
    const int n = (int)strlen(format);
    if (n == 0) {
        crt.traceline(file, line, function, "assertion failed: \"%s\"", condition);
    } else {
        crt.traceline(file, line, function, "assertion failed: \"%s\"", condition);
        va_list va;
        va_start(va, format);
        vtraceln(file, line, function, format, va);
        va_end(va);
    }
    crt.sleep(0.25); // give other threads some time to flush buffers
    breakpoint();
    // pretty much always true but avoids "warning: unreachable code"
    if (file != null || line != 0 || function != null) { exit(1); }
    return 0;
}

static int32_t crt_err() { return GetLastError(); }

static void crt_seterr(int32_t err) { SetLastError(err); }

static uint32_t crt_random32(uint32_t* state) {
    // https://gist.github.com/tommyettinger/46a874533244883189143505d203312c
    static thread_local bool started; // first seed must be odd
    if (!started) { started = true; *state |= 1; }
    uint32_t z = (*state += 0x6D2B79F5UL);
    z = (z ^ (z >> 15)) * (z | 1UL);
    z ^= z + (z ^ (z >> 7)) * (z | 61UL);
    return z ^ (z >> 14);
}

static uint64_t crt_random64(uint64_t *state) {
    // https://gist.github.com/tommyettinger/e6d3e8816da79b45bfe582384c2fe14a
    static thread_local bool started; // first seed must be odd
    if (!started) { started = true; *state |= 1; }
	const uint64_t s = *state;
	const uint64_t z = (s ^ s >> 25) * (*state += 0x6A5D39EAE12657AAULL);
	return z ^ (z >> 22);
}

static int crt_memmap_file(HANDLE file, void* *data, int64_t *bytes, bool rw) {
    int r = 0;
    void* address = null;
    LARGE_INTEGER size = {{0, 0}};
    if (GetFileSizeEx(file, &size)) {
        HANDLE mapping = CreateFileMapping(file, null,
            rw ? PAGE_READWRITE : PAGE_READONLY,
            0, (DWORD)size.QuadPart, null);
        if (mapping == null) {
            r = GetLastError();
        } else {
            address = MapViewOfFile(mapping, FILE_MAP_READ,
                0, 0, (int64_t)size.QuadPart);
            if (address != null) {
                *bytes = (int64_t)size.QuadPart;
            } else {
                r = GetLastError();
            }
            fatal_if_false(CloseHandle(mapping));
        }
    } else {
        r = GetLastError();
    }
    if (r == 0) { *data = address; }
    return r;
}

static int crt_memmap(const char* filename, void* *data,
        int64_t *bytes, bool rw) {
    *bytes = 0;
    int r = 0;
    const DWORD flags = GENERIC_READ | (rw ? GENERIC_WRITE : 0);
    const DWORD share = FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE;
    HANDLE file = CreateFileA(filename, flags, share, null,
        OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, null);
    if (file == INVALID_HANDLE_VALUE) {
        r = GetLastError();
    } else {
        r = crt_memmap_file(file, data, bytes, rw);
        fatal_if_false(CloseHandle(file));
    }
    return r;
}

static int crt_memmap_read(const char* filename, void* *data, int64_t *bytes) {
    return crt_memmap(filename, data, bytes, false);
}

static int crt_memmap_rw(const char* filename, void* *data, int64_t *bytes) {
    return crt_memmap(filename, data, bytes, true);
}

static void crt_memunmap(void* data, int64_t bytes) {
    assert(data != null && bytes > 0);
    (void)bytes; /* unused only need for posix version */
    if (data != null && bytes > 0) {
        fatal_if_false(UnmapViewOfFile(data));
    }
}

static int crt_memmap_res(const char* label, void* *data, int64_t *bytes) {
    HRSRC res = FindResourceA(null, label, (const char*)RT_RCDATA);
    // "LockResource does not actually lock memory; it is just used to
    // obtain a pointer to the memory containing the resource data.
    // The name of the function comes from versions prior to Windows XP,
    // when it was used to lock a global memory block allocated by LoadResource."
    if (res != null) { *bytes = SizeofResource(null, res); }
    HGLOBAL g = res != null ? LoadResource(null, res) : null;
    *data = g != null ? LockResource(g) : null;
    return *data != null ? 0 : GetLastError();
}

crt_if crt = {
    .err = crt_err,
    .seterr = crt_seterr,
    .sleep = crt_sleep,
    .random32 = crt_random32,
    .random64 = crt_random64,
    .memmap_read = crt_memmap_read,
    .memmap_rw = crt_memmap_rw,
    .memunmap = crt_memunmap,
    .memmap_res = crt_memmap_res,
    .seconds = crt_seconds,
    .vformat = crt_vformat,
    .sformat = crt_sformat,
    .error = crt_error,
    .utf8_bytes = crt_utf8_bytes,
    .utf16_bytes = crt_utf16_bytes,
    .utf16to8 = crt_utf16to8,
    .utf8to16 = crt_utf8to16,
    .traceline = crt_traceline,
    .assertion_failed = crt_assertion_failed,
    .fatal = crt_fatal
};

#pragma comment(lib, "advapi32")
#pragma comment(lib, "comctl32")
#pragma comment(lib, "comdlg32")
#pragma comment(lib, "dwmapi")
#pragma comment(lib, "gdi32")
#pragma comment(lib, "msimg32")
#pragma comment(lib, "ole32")
#pragma comment(lib, "shcore")
#pragma comment(lib, "shell32")
#pragma comment(lib, "winmm")
#pragma comment(lib, "user32")

#else

// TODO: implement for other platforms e.g. posix/Linux and MacOS

#endif // _WIN32

end_c

// GDI implementation

begin_c

#define window() ((HWND)app.window)
#define canvas() ((HDC)app.canvas)

typedef struct gdi_xyc_s {
    int32_t x;
    int32_t y;
    color_t c;
} gdi_xyc_t;

static int gdi_top;
static gdi_xyc_t gdi_stack[256];

static void __gdi_init__() {
    gdi.brush_hollow = (brush_t)GetStockBrush(HOLLOW_BRUSH);
    gdi.brush_color  = (brush_t)GetStockBrush(DC_BRUSH);
    gdi.pen_hollow = (pen_t)GetStockPen(NULL_PEN);
}

static color_t gdi_set_text_color(color_t c) {
    return SetTextColor(canvas(), c);
}

static pen_t gdi_set_pen(pen_t p) {
    assert(p != null);
    return (pen_t)SelectPen(canvas(), (HPEN)p);
}

static pen_t gdi_set_colored_pen(color_t c) {
    pen_t p = (pen_t)SelectPen(canvas(), GetStockPen(DC_PEN));
    SetDCPenColor(canvas(), c);
    return p;
}

static pen_t gdi_create_pen(color_t color, int width) {
    assert(width >= 1);
    pen_t pen = (pen_t)CreatePen(PS_SOLID, width, color);
    fatal_if_null(pen);
    return pen;
}

static void gdi_delete_pen(pen_t p) {
    fatal_if_false(DeletePen(p));
}

static brush_t gdi_set_brush(brush_t b) {
    assert(b != null);
    return (brush_t)SelectBrush(canvas(), b);
}

static color_t gdi_set_brush_color(color_t c) {
    return SetDCBrushColor(canvas(), c);
}

static void gdi_set_clip(int x, int y, int w, int h) {
    if (gdi.clip != null) { DeleteRgn(gdi.clip); gdi.clip = null; }
    if (w > 0 && h > 0) {
        gdi.clip = (region_t)CreateRectRgn(x, y, x + w, y + h);
        assert(gdi.clip != null);
        fatal_if_null(gdi.clip);
    }
    fatal_if(SelectClipRgn(canvas(), (HRGN)gdi.clip) == ERROR);
}

static void gdi_push(int x, int y) {
    assert(gdi_top < countof(gdi_stack));
    fatal_if(gdi_top >= countof(gdi_stack));
    gdi_stack[gdi_top].x = gdi.x;
    gdi_stack[gdi_top].y = gdi.y;
    fatal_if(SaveDC(canvas()) == 0);
    gdi_top++;
    gdi.x = x;
    gdi.y = y;
}

static void gdi_pop() {
    assert(0 < gdi_top && gdi_top <= countof(gdi_stack));
    fatal_if(gdi_top <= 0);
    gdi_top--;
    gdi.x = gdi_stack[gdi_top].x;
    gdi.y = gdi_stack[gdi_top].y;
    fatal_if_false(RestoreDC(canvas(), -1));
}

static ui_point_t gdi_move_to(int x, int y) {
    POINT pt;
    pt.x = gdi.x;
    pt.y = gdi.y;
    fatal_if_false(MoveToEx(canvas(), x, y, &pt));
    gdi.x = x;
    gdi.y = y;
    ui_point_t p = { pt.x, pt.y };
    return p;
}

static void gdi_line(int32_t x, int32_t y) {
    fatal_if_false(LineTo(canvas(), x, y));
    gdi.x = x;
    gdi.y = y;
}

static void gdi_rect(int32_t x, int32_t y, int32_t w, int32_t h) {
    fatal_if_false(Rectangle(canvas(), x, y, x + w, y + h));
}

static void gdi_fill(int32_t x, int32_t y, int32_t w, int32_t h) {
    RECT rc = { x, y, x + w, y + h };
    brush_t b = (brush_t)GetCurrentObject(canvas(), OBJ_BRUSH);
    fatal_if_false(FillRect(canvas(), &rc, (HBRUSH)b));
}

static void gdi_poly(ui_point_t* points, int32_t count) {
    // make sure ui_point_t and POINT have the same memory layout:
    static_assert(sizeof(points->x) == sizeof(((POINT*)0)->x), "ui_point_t");
    static_assert(sizeof(points->y) == sizeof(((POINT*)0)->y), "ui_point_t");
    static_assert(sizeof(points[0]) == sizeof(*((POINT*)0)), "ui_point_t");
    assert(canvas() != null && count > 1);
    fatal_if_false(Polyline(canvas(), (POINT*)points, count));
}

static void gdi_rounded(int32_t x, int32_t y, int32_t w, int32_t h,
        int32_t rx, int32_t ry) {
    fatal_if_false(RoundRect(canvas(), x, y, x + w, y + h, rx, ry));
}

static void gdi_gradient(int32_t x, int32_t y, int32_t w, int32_t h,
        color_t rgba_from, color_t rgba_to, bool vertical) {
    TRIVERTEX vertex[2];
    vertex[0].x = x;
    vertex[0].y = y;
    // TODO: colors:
    vertex[0].Red   = ((rgba_from >>  0) & 0xFF) << 8;
    vertex[0].Green = ((rgba_from >>  8) & 0xFF) << 8;
    vertex[0].Blue  = ((rgba_from >> 16) & 0xFF) << 8;
    vertex[0].Alpha = ((rgba_from >> 24) & 0xFF) << 8;
    vertex[1].x = x + w;
    vertex[1].y = y + h;
    vertex[1].Red   = ((rgba_to >>  0) & 0xFF) << 8;
    vertex[1].Green = ((rgba_to >>  8) & 0xFF) << 8;
    vertex[1].Blue  = ((rgba_to >> 16) & 0xFF) << 8;
    vertex[1].Alpha = ((rgba_to >> 24) & 0xFF) << 8;
    GRADIENT_RECT gRect = {0, 1};
    const int mode = vertical ? GRADIENT_FILL_RECT_V : GRADIENT_FILL_RECT_H;
    GradientFill(canvas(), vertex, 2, &gRect, 1, mode);
}

static void gdi_draw_greyscale(int32_t sx, int32_t sy, int32_t sw, int32_t sh,
        int32_t x, int32_t y, int32_t w, int32_t h, const byte* pixels) {
    typedef struct bitmap_rgb_s {
        BITMAPINFO bi;
        RGBQUAD rgb[256];
    } bitmap_rgb_t;
    static bitmap_rgb_t storage; // for grayscale palette
    static BITMAPINFO *bi = &storage.bi;
    BITMAPINFOHEADER* bih = &bi->bmiHeader;
    if (bih->biSize == 0) { // once
        bih->biSize = sizeof(BITMAPINFOHEADER);
        for (int i = 0; i < 256; i++) {
            RGBQUAD* q = &bi->bmiColors[i];
            q->rgbReserved = 0;
            q->rgbBlue = q->rgbGreen = q->rgbRed = (byte)i;
        }
        bih->biPlanes = 1;
        bih->biBitCount = 8;
        bih->biCompression = BI_RGB;
        bih->biClrUsed = 256;
        bih->biClrImportant = 256;
    }
    bih->biWidth = w;
    bih->biHeight = -h; // top down image
    bih->biSizeImage = w * h;
    POINT pt = { 0 };
    fatal_if_false(SetBrushOrgEx(canvas(), 0, 0, &pt));
    StretchDIBits(canvas(), sx, sy, sw, sh, x, y, w, h,
        pixels, bi, DIB_RGB_COLORS, SRCCOPY);
    fatal_if_false(SetBrushOrgEx(canvas(), pt.x, pt.y, &pt));
}

static BITMAPINFO* gdi_bmp(int32_t w, int32_t h, int32_t bpp, BITMAPINFO* bi) {
    bi->bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bi->bmiHeader.biWidth = w;
    bi->bmiHeader.biHeight = -h;  // top down image
    bi->bmiHeader.biPlanes = 1;
    bi->bmiHeader.biBitCount = (uint16_t)(bpp * 8);
    bi->bmiHeader.biCompression = BI_RGB;
    bi->bmiHeader.biSizeImage = w * h * bpp;
    return bi;
}

static void gdi_image_init(image_t* image, int32_t w, int32_t h, int32_t bpp,
        const byte* pixels) {
    fatal_if(bpp < 0 || bpp == 2 || bpp > 4, "bpp=%d not {1, 3, 4}", bpp);
    HDC c = GetWindowDC(window());
    BITMAPINFO bi = { {sizeof(BITMAPINFOHEADER)} };
    image->bitmap = (bitmap_t)CreateDIBSection(c, gdi_bmp(w, h, bpp, &bi),
        DIB_RGB_COLORS, &image->pixels, null, 0x0);
    fatal_if(image->bitmap == null || image->pixels == null);
    memcpy(image->pixels, pixels, bi.bmiHeader.biSizeImage);
    const int32_t n = w * h;
    byte* bgra = (byte*)image->pixels;
    if (bpp >= 3) {
        for (int i = 0; i < n; i++) {
            // swap R <-> B RGBA -> BGRA
            byte red = bgra[0]; bgra[0] = bgra[2]; bgra[2] = red;
            if (bpp == 4) {
                // premultiply alpha, see:
                // https://stackoverflow.com/questions/24595717/alphablend-generating-incorrect-colors
                int alpha = bgra[3];
                bgra[0] = (byte)(bgra[0] * alpha / 255);
                bgra[1] = (byte)(bgra[1] * alpha / 255);
                bgra[2] = (byte)(bgra[2] * alpha / 255);
            }
            bgra += bpp;
        }
    }
    image->w = w;
    image->h = h;
    image->bpp = bpp;
    fatal_if_false(ReleaseDC(window(), c));
}

static void gdi_alpha_blend(int32_t x, int32_t y, int32_t w, int32_t h,
        image_t* image, double alpha) {
    assert(image->bpp > 0);
    assert(0 <= alpha && alpha <= 1);
    fatal_if_null(canvas());
    HDC c = CreateCompatibleDC(canvas());
    fatal_if_null(c);
    HBITMAP zero1x1 = SelectBitmap((HDC)c, (HBITMAP)image->bitmap);
    BLENDFUNCTION bf = { 0 };
    bf.SourceConstantAlpha = (byte)(0xFF * alpha + 0.49);
    if (image->bpp == 4) {
        bf.BlendOp = AC_SRC_OVER;
        bf.BlendFlags = 0;
        bf.AlphaFormat = AC_SRC_ALPHA;
    } else {
        bf.BlendOp = AC_SRC_OVER;
        bf.BlendFlags = 0;
        bf.AlphaFormat = 0;
    }
    fatal_if_false(AlphaBlend(canvas(), x, y, w, h,
        c, 0, 0, image->w, image->h, bf));
    SelectBitmap((HDC)c, zero1x1);
    fatal_if_false(DeleteDC(c));
}

static void gdi_draw_image(int32_t x, int32_t y, int32_t w, int32_t h,
        image_t* image) {
    assert(image->bpp == 3 || image->bpp == 4);
    fatal_if_null(canvas());
    HDC c = CreateCompatibleDC(canvas());
    fatal_if_null(c);
    HBITMAP zero1x1 = SelectBitmap(c, image->bitmap);
    fatal_if_false(StretchBlt(canvas(), x, y, w, h,
        c, 0, 0, image->w, image->h, SRCCOPY));
    SelectBitmap(c, zero1x1);
    fatal_if_false(DeleteDC(c));
}

static font_t gdi_font(font_t f, int height) {
    assert(f != null && height > 0);
    LOGFONTA lf = {0};
    int n = GetObjectA(f, sizeof(lf), &lf);
    fatal_if_false(n == (int)sizeof(lf));
    lf.lfHeight = -height;
    return (font_t)CreateFontIndirectA(&lf);
}

static void gdi_delete_font(font_t f) {
    fatal_if_false(DeleteFont(f));
}

static font_t gdi_set_font(font_t f) {
    assert(f != null);
    return (font_t)SelectFont(canvas(), (HFONT)f);
}

static int gdi_baseline(font_t f) {
    assert(canvas() != null && f != null);
    f = gdi.set_font(f);
    TEXTMETRICA tm;
    fatal_if_false(GetTextMetricsA(canvas(), &tm));
    gdi.set_font(f);
    return tm.tmAscent;
}

static int gdi_descent(font_t f) {
    assert(canvas() != null && f != null);
    f = gdi.set_font(f);
    TEXTMETRICA tm;
    fatal_if_false(GetTextMetricsA(canvas(), &tm));
    gdi.set_font(f);
    return tm.tmDescent;
}

static ui_point_t gdi_get_em(font_t f) {
    assert(canvas() != null && f != null);
    f = gdi.set_font(f);
    SIZE cell = {0};
    fatal_if_false(GetTextExtentPoint32A(canvas(), "M", 1, &cell));
    gdi.set_font(f);
    ui_point_t c = {cell.cx, cell.cy};
    return c;
}

static double gdi_line_spacing(double height_multiplier) {
    assert(0.1 <= height_multiplier && height_multiplier <= 2.0);
    double hm = gdi.height_multiplier;
    gdi.height_multiplier = height_multiplier;
    return hm;
}

static int gdi_draw_utf16(const char* s, int n, RECT* r, uint32_t f) {
    assert(canvas() != null);
    return DrawTextW(canvas(), utf8to16(s), n, r, f);
}

typedef struct gdi_dtp_s { // draw text params
    const char* format; // format string
    va_list vl;
    RECT rc;
    uint32_t flags; // flags:
    // https://docs.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-drawtextw
    // DT_CALCRECT DT_NOCLIP useful for measure
    // DT_END_ELLIPSIS useful for clipping
    // DT_LEFT, DT_RIGHT, DT_CENTER useful for paragraphs
    // DT_WORDBREAK is not good (GDI does not break nicely)
    // DT_BOTTOM, DT_VCENTER limited usablity in wierd cases (layout is better)
    // DT_NOPREFIX not to draw underline at "&Keyboard shortcuts
    // DT_SINGLELINE versus multiline
} gdi_dtp_t;

static void gdi_text_draw(gdi_dtp_t* p) {
    int n = 1024;
    char* text = (char*)alloca(n);
    crt.vformat(text, n - 1, p->format, p->vl);
    int k = (int)strlen(text);
    // Microsoft returns -1 not posix required sizeof buffer
    while (k >= n - 1 || k < 0) {
        n = n * 2;
        text = (char*)alloca(n);
        crt.vformat(text, n - 1, p->format, p->vl);
        k = (int)strlen(text);
    }
    assert(k >= 0 && k <= n, "k=%d n=%d fmt=%s", k, n, p->format);
    // rectangle is always calculated - it makes draw text
    // much slower but UI layer is mostly uses bitmap caching:
    if ((p->flags & DT_CALCRECT) == 0) {
        // no actual drawing just calculate rectangle
        bool b = gdi_draw_utf16(text, -1, &p->rc, p->flags | DT_CALCRECT);
        assert(b, "draw_text_utf16(%s) failed", text); (void)b;
    }
    bool b = gdi_draw_utf16(text, -1, &p->rc, p->flags);
    assert(b, "draw_text_utf16(%s) failed", text); (void)b;
}

enum {
    sl_draw          = DT_LEFT|DT_NOCLIP|DT_SINGLELINE|DT_NOCLIP,
    sl_measure       = sl_draw|DT_CALCRECT,
    ml_draw_break    = DT_LEFT|DT_NOPREFIX|DT_NOCLIP|DT_NOFULLWIDTHCHARBREAK|
                       DT_WORDBREAK,
    ml_measure_break = ml_draw_break|DT_CALCRECT,
    ml_draw          = DT_LEFT|DT_NOPREFIX|DT_NOCLIP|DT_NOFULLWIDTHCHARBREAK,
    ml_measure       = ml_draw|DT_CALCRECT
};

static ui_point_t gdi_text_measure(font_t f, gdi_dtp_t* p) {
    assert(f != null);
    f = gdi.set_font(f);
    gdi_text_draw(p);
    gdi.set_font(f);
    ui_point_t cell = {p->rc.right - p->rc.left, p->rc.bottom - p->rc.top};
    return cell;
}

static ui_point_t gdi_measure_singleline(font_t f, const char* format, ...) {
    va_list vl;
    va_start(vl, format);
    gdi_dtp_t p = { format, vl, {0, 0, 0, 0}, sl_measure };
    ui_point_t cell = gdi_text_measure(f, &p);
    va_end(vl);
    return cell;
}

static ui_point_t gdi_measure_multiline(font_t f, int w, const char* format, ...) {
    va_list vl;
    va_start(vl, format);
    uint32_t flags = w <= 0 ? ml_measure : ml_measure_break;
    gdi_dtp_t p = { format, vl, {gdi.x, gdi.y, gdi.x + (w <= 0 ? 1 : w), gdi.y}, flags };
    ui_point_t cell = gdi_text_measure(f, &p);
    va_end(vl);
    return cell;
}

static void gdi_vtext(const char* format, va_list vl) {
    assert(canvas() != null);
    gdi_dtp_t p = { format, vl, {gdi.x, gdi.y, 0, 0}, sl_draw };
    gdi_text_draw(&p);
    gdi.x += p.rc.right - p.rc.left;
}

static void gdi_vtextln(const char* format, va_list vl) {
    assert(canvas() != null);
    gdi_dtp_t p = { format, vl, {gdi.x, gdi.y, gdi.x, gdi.y}, sl_draw };
    gdi_text_draw(&p);
    gdi.y += (int)((p.rc.bottom - p.rc.top) * gdi.height_multiplier + 0.5f);
}

static void gdi_text(const char* format, ...) {
    va_list vl;
    va_start(vl, format);
    gdi.vtext(format, vl);
    va_end(vl);
}

static void gdi_textln(const char* format, ...) {
    va_list vl;
    va_start(vl, format);
    gdi.vtextln(format, vl);
    va_end(vl);
}

static ui_point_t gdi_multiline(int w, const char* f, ...) {
    va_list vl;
    va_start(vl, f);
    uint32_t flags = w <= 0 ? ml_draw : ml_draw_break;
    gdi_dtp_t p = { f, vl, {gdi.x, gdi.y, gdi.x + (w <= 0 ? 1 : w), gdi.y}, flags };
    gdi_text_draw(&p);
    va_end(vl);
    ui_point_t c = { p.rc.right - p.rc.left, p.rc.bottom - p.rc.top };
    return c;
}

static void gdi_vprint(const char* format, va_list vl) {
    assert(app.fonts.mono != null);
    font_t f = gdi.set_font(app.fonts.mono);
    gdi.vtext(format, vl);
    gdi.set_font(f);
}

static void gdi_print(const char* format, ...) {
    va_list vl;
    va_start(vl, format);
    gdi.vprint(format, vl);
}

static void gdi_vprintln(const char* format, va_list vl) {
    assert(app.fonts.mono != null);
    font_t f = gdi.set_font(app.fonts.mono);
    gdi.vtextln(format, vl);
    gdi.set_font(f);
}

static void gdi_println(const char* format, ...) {
    va_list vl;
    va_start(vl, format);
    gdi.vprintln(format, vl);
    va_end(vl);
}

// to enable load_image() function
// 1. Add
//    curl.exe https://raw.githubusercontent.com/nothings/stb/master/stb_image.h stb_image.h
//    to the project precompile build step
// 2. After
//    #define quick_implementation
//    include "quick.h"
//    add
//    #define STBI_ASSERT(x) assert(x)
//    #define STB_IMAGE_IMPLEMENTATION
//    #include "stb_image.h"

static byte* gdi_load_image(const void* data, int bytes, int* w, int* h,
        int* bytes_per_pixel, int preffered_bytes_per_pixel) {
    #ifdef STBI_VERSION
        return stbi_load_from_memory((byte const*)data, bytes, w, h,
            bytes_per_pixel, preffered_bytes_per_pixel);
    #else // see instructions above
        (void)data; (void)bytes; (void)data; (void)w; (void)h;
        (void)bytes_per_pixel; (void)preffered_bytes_per_pixel;
        fatal_if(true, "curl.exe --silent --fail --create-dirs "
            "https://raw.githubusercontent.com/nothings/stb/master/stb_image.h "
            "--output ext/stb_image.h");
        return null;
    #endif
}

static void gdi_image_dispose(image_t* image) {
    fatal_if_false(DeleteBitmap(image->bitmap));
}

gdi_t gdi = {
    .height_multiplier = 1.0,
    .image_init = gdi_image_init,
    .image_dispose = gdi_image_dispose,
    .alpha_blend = gdi_alpha_blend,
    .draw_image = gdi_draw_image,
    .set_text_color = gdi_set_text_color,
    .set_brush = gdi_set_brush,
    .set_brush_color = gdi_set_brush_color,
    .set_colored_pen = gdi_set_colored_pen,
    .create_pen = gdi_create_pen,
    .set_pen = gdi_set_pen,
    .delete_pen = gdi_delete_pen,
    .set_clip = gdi_set_clip,
    .push = gdi_push,
    .pop = gdi_pop,
    .move_to = gdi_move_to,
    .line = gdi_line,
    .rect = gdi_rect,
    .fill = gdi_fill,
    .poly = gdi_poly,
    .rounded = gdi_rounded,
    .gradient = gdi_gradient,
    .draw_greyscale = gdi_draw_greyscale,
    .font = gdi_font,
    .delete_font = gdi_delete_font,
    .set_font = gdi_set_font,
    .descent = gdi_descent,
    .baseline = gdi_baseline,
    .get_em = gdi_get_em,
    .line_spacing = gdi_line_spacing,
    .measure_text = gdi_measure_singleline,
    .measure_multiline = gdi_measure_multiline,
    .vtext = gdi_vtext,
    .vtextln = gdi_vtextln,
    .text = gdi_text,
    .textln = gdi_textln,
    .vprint = gdi_vprint,
    .vprintln = gdi_vprintln,
    .print = gdi_print,
    .println = gdi_println,
    .multiline = gdi_multiline
};

enum {
    _colors_white     = rgb(255, 255, 255),
    _colors_off_white = rgb(192, 192, 192),
    _colors_dkgray4   = rgb(63, 63, 70),
    _colors_blue_highlight = rgb(128, 128, 255)
};

colors_t colors = {
    .none    = (int)0xFFFFFFFF, // aka CLR_INVALID in wingdi
    .text    = rgb(240, 231, 220),
    .white   = _colors_white,
    .black   = rgb(0, 0, 0),
    .red     = rgb(255, 0, 0),
    .green   = rgb(0, 255, 0),
    .blue    = rgb(0, 0, 255),
    .yellow  = rgb(255, 255, 0),
    .cyan    = rgb(0, 255, 255),
    .magenta = rgb(255, 0, 255),
    .gray    = rgb(128, 128, 128),
    .dkgray1  = rgb(30, 30, 30),
    .dkgray2  = rgb(37, 38, 38),
    .dkgray3  = rgb(45, 45, 48),
    .dkgray4  = _colors_dkgray4,
    // tone down RGB colors:
    .tone_white   = rgb(164, 164, 164),
    .tone_red     = rgb(192, 64, 64),
    .tone_green   = rgb(64, 192, 64),
    .tone_blue    = rgb(64, 64, 192),
    .tone_yellow  = rgb(192, 192, 64),
    .tone_cyan    = rgb(64, 192, 192),
    .tone_magenta = rgb(192, 64, 192),
    // misc:
    .orange  = rgb(255, 165, 0), // 0xFFA500
    .dkgreen = rgb(1, 50, 32),   // 0x013220
    // highlights:
    .text_highlight = rgb(190, 200, 255), // bluish off-white
    .blue_highlight = _colors_blue_highlight,
    .off_white = _colors_off_white,

    .btn_gradient_darker = rgb(16, 16, 16),
    .btn_gradient_dark   = _colors_dkgray4,
    .btn_hover_highlight = _colors_blue_highlight,
    .btn_disabled = _colors_dkgray4,
    .btn_armed = _colors_white,
    .btn_text = _colors_off_white,
    .toast = rgb(8, 40, 24) // toast background
};

end_c

// UIC implementation

begin_c

static void uic_invalidate(uic_t* ui) {
    ui_rect_t rc = { ui->x, ui->y, ui->w, ui->h};
    rc.x -= ui->em.x;
    rc.y -= ui->em.y;
    rc.w += ui->em.x * 2;
    rc.h += ui->em.y * 2;
    app.invalidate(&rc);
}

static const char* uic_nsl(uic_t* ui) {
    return ui->strid != 0 ? app.string(ui->strid, ui->text) : ui->text;
}

static void uic_measure(uic_t* ui) {
    font_t f = ui->font != null ? *ui->font : app.fonts.regular;
    ui->em = gdi.get_em(f);
    assert(ui->em.x > 0 && ui->em.y > 0);
    ui->w = (int32_t)(ui->em.x * ui->width + 0.5);
    ui_point_t mt = { 0 };
    if (ui->tag == uic_tag_text && ((uic_text_t*)ui)->multiline) {
        int w = (int)(ui->width * ui->em.x + 0.5);
        mt = gdi.measure_multiline(f, w == 0 ? -1 : w, uic_nsl(ui));
    } else {
        mt = gdi.measure_text(f, uic_nsl(ui));
    }
    ui->h = mt.y;
    ui->w = max(ui->w, mt.x);
    ui->baseline = gdi.baseline(f);
    ui->descent = gdi.descent(f);
}

static void uic_set_label(uic_t* ui, const char* label) {
    int n = (int)strlen(label);
    strprintf(ui->text, "%s", label);
    for (int i = 0; i < n; i++) {
        if (label[i] == '&' && i < n - 1 && label[i + 1] != '&') {
            ui->shortcut = label[i + 1];
            break;
        }
    }
}

static void uic_localize(uic_t* ui) {
    if (ui->text[0] != 0) {
        ui->strid = app.strid(ui->text);
    }
}

static void uic_hovering(uic_t* ui, bool start) {
    static uic_text(btn_tooltip,  "");
    if (start && ui->tip[0] != 0 && !ui->disabled && !ui->hidden) {
        strprintf(btn_tooltip.ui.text, "%s", app.nls(ui->tip));
        int32_t y = app.mouse.y - ui->em.y;
        // enough space above? if not show below
        if (y < ui->em.y) { y = app.mouse.y + ui->em.y * 3 / 2; }
        y = min(app.crc.h - ui->em.y * 3 / 2, max(0, y));
        app.show_tooltip(&btn_tooltip.ui, app.mouse.x, y, 0);
    } else if (!start) {
        app.show_tooltip(null, -1, -1, 0);
    }
}

static void ui_init(uic_t* ui) {
    ui->invalidate = uic_invalidate;
    ui->localize = uic_localize;
    ui->measure  = uic_measure;
    ui->hovering = uic_hovering;
    ui->hover_delay = 2.5;
}

// text

static void uic_text_paint(uic_t* ui) {
    assert(ui->tag == uic_tag_text);
    assert(!ui->hidden);
    uic_text_t* t = (uic_text_t*)ui;
    // at later stages of layout text height can grow:
    gdi.push(ui->x, ui->y + t->dy);
    font_t f = ui->font != null ? *ui->font : app.fonts.regular;
    gdi.set_font(f);
//  traceln("%s h=%d dy=%d baseline=%d", ui->text, ui->h, t->dy, ui->baseline);
    color_t c = ui->hover && t->highlight && !t->label ?
        colors.text_highlight : ui->color;
    gdi.set_text_color(c);
    // paint for text also does lightweight re-layout
    // which is useful for simplifying dynamic text changes
    if (!t->multiline) {
        gdi.text("%s", uic_nsl(ui));
    } else {
        int w = (int)(ui->width * ui->em.x + 0.5);
        gdi.multiline(w == 0 ? -1 : w, "%s", uic_nsl(ui));
    }
    if (ui->hover && t->hovered && !t->label) {
        gdi.set_colored_pen(colors.btn_hover_highlight);
        gdi.set_brush(gdi.brush_hollow);
        int cr = ui->em.y / 4; // corner radius
        int h = t->multiline ? ui->h : ui->baseline + ui->descent;
        gdi.rounded(ui->x - cr, ui->y + t->dy, ui->w + 2 * cr,
            h, cr, cr);
    }
    gdi.pop();
}

static void uic_text_context_menu(uic_t* ui) {
    assert(ui->tag == uic_tag_text);
    uic_text_t* t = (uic_text_t*)ui;
    if (!t->label && !ui->hidden) {
        clipboard.copy_text(uic_nsl(ui));
        static bool first_time = true;
        app.toast(first_time ? 2.15 : 0.75,
            app.nls("Text copied to clipboard"));
        first_time = false;
    }
}

static void uic_text_keyboard(uic_t* ui, int ch) {
    assert(ui->tag == uic_tag_text);
    uic_text_t* t = (uic_text_t*)ui;
    if (ui->hover && !ui->hidden && !t->label) {
        // Copy to clipboard works for hover over text
        if ((ch == 3 || ch == 'c' || ch == 'C') && app.ctrl) {
            clipboard.copy_text(uic_nsl(ui)); // 3 is ASCII for Ctrl+C
        }
    }
}

void _uic_text_init_(uic_t* ui) {
    static_assert(offsetof(uic_text_t, ui) == 0, "offsetof(.ui)");
    assert(ui->tag == uic_tag_text);
    ui_init(ui);
    if (ui->font == null) { ui->font = &app.fonts.regular; }
    ui->color = colors.text;
    ui->paint = uic_text_paint;
    ui->keyboard = uic_text_keyboard;
    ui->context_menu = uic_text_context_menu;
}

void uic_text_vinit(uic_text_t* t, const char* format, va_list vl) {
    static_assert(offsetof(uic_text_t, ui) == 0, "offsetof(.ui)");
    crt.vformat(t->ui.text, countof(t->ui.text), format, vl);
    t->ui.tag = uic_tag_text;
    _uic_text_init_(&t->ui);
}

void uic_text_init(uic_text_t* t, const char* format, ...) {
    va_list vl;
    va_start(vl, format);
    uic_text_vinit(t, format, vl);
    va_end(vl);
}

void uic_text_init_ml(uic_text_t* t, double width, const char* format, ...) {
    va_list vl;
    va_start(vl, format);
    uic_text_vinit(t, format, vl);
    va_end(vl);
    t->ui.width = width;
    t->multiline = true;
}

// button

static void uic_button_periodically(uic_t* ui) { // every 100ms
    assert(ui->tag == uic_tag_button);
    uic_button_t* b = (uic_button_t*)ui;
    if (b->armed_until != 0 && app.now > b->armed_until) {
        b->armed_until = 0;
        ui->armed = false;
        ui->invalidate(ui);
    }
}

static void uic_button_paint(uic_t* ui) {
    assert(ui->tag == uic_tag_button);
    assert(!ui->hidden);
    uic_button_t* b = (uic_button_t*)ui;
    gdi.push(ui->x, ui->y);
    bool pressed = (ui->armed ^ ui->pressed) == 0;
    if (b->armed_until != 0) { pressed = true; }
    int sign = 1 - pressed * 2; // -1, +1
    int32_t w = sign * ui->w;
    int32_t h = sign * ui->h;
    int32_t x = b->ui.x + (int)pressed * ui->w;
    int32_t y = b->ui.y + (int)pressed * ui->h;
    gdi.gradient(x, y, w, h, colors.btn_gradient_darker,
        colors.btn_gradient_dark, true);
    color_t c = ui->armed ? colors.btn_armed : colors.off_white;
    if (b->ui.hover && !ui->armed) { c = colors.btn_hover_highlight; }
    if (ui->disabled) { c = colors.btn_disabled; }
    font_t f = ui->font != null ? *ui->font : app.fonts.regular;
    ui_point_t m = gdi.measure_text(f, uic_nsl(ui));
    gdi.set_text_color(c);
    gdi.x = ui->x + (ui->w - m.x) / 2;
    gdi.y = ui->y + (ui->h - m.y) / 2;
    f = gdi.set_font(f);
    gdi.text("%s", uic_nsl(ui));
    gdi.set_font(f);
    const int32_t pw = max(1, ui->em.y / 32); // pen width
    color_t color = ui->armed ? colors.dkgray4 : colors.gray;
    if (ui->hover && !ui->armed) { color = colors.blue; }
    if (ui->disabled) { color = colors.dkgray1; }
    pen_t p = gdi.create_pen(color, pw);
    gdi.set_pen(p);
    gdi.set_brush(gdi.brush_hollow);
    gdi.rounded(ui->x, ui->y, ui->w, ui->h, ui->em.y / 4, ui->em.y / 4);
    gdi.delete_pen(p);
    gdi.pop();
}

static bool uic_button_hit_test(uic_button_t* b, ui_point_t pt) {
    assert(b->ui.tag == uic_tag_button);
    pt.x -= b->ui.x;
    pt.y -= b->ui.y;
    return 0 <= pt.x && pt.x < b->ui.w && 0 <= pt.y && pt.y < b->ui.h;
}

static void uic_button_callback(uic_button_t* b) {
    assert(b->ui.tag == uic_tag_button);
    if (b->cb != null) { b->cb(b); }
    app.show_tooltip(null, -1, -1, 0);
}

static void uic_button_keyboard(uic_t* ui, int ch) {
    assert(ui->tag == uic_tag_button);
    assert(!ui->hidden && !ui->disabled);
    uic_button_t* b = (uic_button_t*)ui;
    if (toupper(ui->shortcut) == toupper(ch)) {
        ui->armed = true;
        ui->invalidate(ui);
        app.draw();
        b->armed_until = app.now + 0.250;
//      ui->armed = false;
        uic_button_callback(b);
        ui->invalidate(ui);
    }
}

/* processes mouse clicks and invokes callback  */

static void uic_button_mouse(uic_t* ui, int message, int flags) {
    assert(ui->tag == uic_tag_button);
    (void)flags; // unused
    assert(!ui->hidden && !ui->disabled);
    uic_button_t* b = (uic_button_t*)ui;
    bool a = ui->armed;
    bool on = false;
    if (message == messages.left_button_down ||
        message == messages.right_button_down) {
        ui->armed = uic_button_hit_test(b, app.mouse);
        if (ui->armed) { app.show_tooltip(null, -1, -1, 0); }
    }
    if (message == messages.left_button_up ||
        message == messages.right_button_up) {
        if (ui->armed) { on = uic_button_hit_test(b, app.mouse); }
        ui->armed = false;
    }
    if (on) { uic_button_callback(b); }
    if (a != ui->armed) { ui->invalidate(ui); }
}

static void uic_button_measure(uic_t* ui) {
    assert(ui->tag == uic_tag_button || ui->tag == uic_tag_text);
    uic_measure(ui);
    const int32_t em2  = max(1, ui->em.x / 2);
    ui->w = ui->w;
    ui->h = ui->h + em2;
    if (ui->w < ui->h) { ui->w = ui->h; }
}

void _uic_button_init_(uic_t* ui) {
    assert(ui->tag == uic_tag_button);
    ui_init(ui);
    ui->mouse = uic_button_mouse;
    ui->measure = uic_button_measure;
    ui->paint = uic_button_paint;
    ui->keyboard = uic_button_keyboard;
    ui->periodically = uic_button_periodically;
    uic_set_label(ui, ui->text);
    ui->localize(ui);
}

void uic_button_init(uic_button_t* b, const char* label, double ems,
        void (*cb)(uic_button_t* b)) {
    static_assert(offsetof(uic_button_t, ui) == 0, "offsetof(.ui)");
    b->ui.tag = uic_tag_button;
    strprintf(b->ui.text, "%s", label);
    b->cb = cb;
    b->ui.width = ems;
    _uic_button_init_(&b->ui);
}

// checkbox

static int  uic_checkbox_paint_on_off(uic_t* ui) {
    // https://www.compart.com/en/unicode/U+2B24
    static const char* circle = "\xE2\xAC\xA4"; // Black Large Circle
    gdi.push(ui->x, ui->y);
    color_t background = ui->pressed ? colors.tone_green : colors.dkgray4;
    color_t foreground = colors.off_white;
    gdi.set_text_color(background);
    int32_t x = ui->x;
    int32_t x1 = ui->x + ui->em.x * 3 / 4;
    while (x < x1) {
        gdi.x = x;
        gdi.text("%s", circle);
        x++;
    }
    int rx = gdi.x;
    gdi.set_text_color(foreground);
    gdi.x = ui->pressed ? x : ui->x;
    gdi.text("%s", circle);
    gdi.pop();
    return rx;
}

static const char*  uic_checkbox_on_off_label(uic_t* ui, char* label, int count)  {
    crt.sformat(label, count, "%s", uic_nsl(ui));
    char* s = strstr(label, "___");
    if (s != null) {
        memcpy(s, ui->pressed ? "On " : "Off", 3);
    }
    return app.nls(label);
}

static void  uic_checkbox_measure(uic_t* ui) {
    assert(ui->tag == uic_tag_checkbox);
    uic_measure(ui);
    ui->w += ui->em.x * 2;
}

static void  uic_checkbox_paint(uic_t* ui) {
    assert(ui->tag == uic_tag_checkbox);
    char text[countof(ui->text)];
    const char* label =  uic_checkbox_on_off_label(ui, text, countof(text));
    gdi.push(ui->x, ui->y);
    gdi.x =  uic_checkbox_paint_on_off(ui) + ui->em.x * 3 / 4;
    gdi.text("%s", label);
    gdi.pop();
}

static void  uic_checkbox_flip( uic_checkbox_t* c) {
    assert(c->ui.tag == uic_tag_checkbox);
    app.redraw();
    c->ui.pressed = !c->ui.pressed;
    if (c->cb != null) { c->cb(c); }
}

static void  uic_checkbox_keyboard(uic_t* ui, int ch) {
    assert(ui->tag == uic_tag_checkbox);
    assert(!ui->hidden && !ui->disabled);
    if (toupper(ui->shortcut) == toupper(ch)) {
         uic_checkbox_flip(( uic_checkbox_t*)ui);
    }
}

static void  uic_checkbox_mouse(uic_t* ui, int message, int flags) {
    assert(ui->tag == uic_tag_checkbox);
    (void)flags; // unused
    assert(!ui->hidden && !ui->disabled);
    if (message == messages.left_button_down ||
        message == messages.right_button_down) {
        int32_t x = app.mouse.x - ui->x;
        int32_t y = app.mouse.y - ui->y;
        if (0 <= x && x < ui->w && 0 <= y && y < ui->h) {
             uic_checkbox_flip(( uic_checkbox_t*)ui);
        }
    }
}

void _uic_checkbox_init_(uic_t* ui) {
    assert(ui->tag == uic_tag_checkbox);
    ui_init(ui);
    uic_set_label(ui, ui->text);
    ui->mouse =  uic_checkbox_mouse;
    ui->measure = uic_checkbox_measure;
    ui->paint =  uic_checkbox_paint;
    ui->keyboard =  uic_checkbox_keyboard;
    ui->localize(ui);
}

void  uic_checkbox_init( uic_checkbox_t* c, const char* label, double ems,
       void (*cb)( uic_checkbox_t* b)) {
    static_assert(offsetof( uic_checkbox_t, ui) == 0, "offsetof(.ui)");
    ui_init(&c->ui);
    strprintf(c->ui.text, "%s", label);
    c->ui.width = ems;
    c->cb = cb;
    c->ui.tag = uic_tag_checkbox;
    _uic_checkbox_init_(&c->ui);
}

// slider control

static void uic_slider_measure(uic_t* ui) {
    assert(ui->tag == uic_tag_slider);
    uic_measure(ui);
    uic_slider_t* r = (uic_slider_t*)ui;
    assert(r->inc.ui.w == r->dec.ui.w && r->inc.ui.h == r->dec.ui.h);
    const int32_t em = ui->em.x;
    font_t f = ui->font != null ? *ui->font : app.fonts.regular;
    r->tm = gdi.measure_text(f, uic_nsl(ui), r->vmax);
    ui->w = r->dec.ui.w + r->tm.x + r->inc.ui.w + em * 2;
    ui->h = r->inc.ui.h;
}

static void uic_slider_layout(uic_t* ui) {
    assert(ui->tag == uic_tag_slider);
    uic_slider_t* r = (uic_slider_t*)ui;
    assert(r->inc.ui.w == r->dec.ui.w && r->inc.ui.h == r->dec.ui.h);
    const int32_t em = ui->em.x;
    font_t f = ui->font != null ? *ui->font : app.fonts.regular;
    r->tm = gdi.measure_text(f, uic_nsl(ui), r->vmax);
    r->dec.ui.x = ui->x;
    r->dec.ui.y = ui->y;
    r->inc.ui.x = ui->x + r->dec.ui.w + r->tm.x + em * 2;
    r->inc.ui.y = ui->y;
//  traceln("%d, %d dec=%dx%d mt.x=%d em=%d inc %d %d", ui->x, ui->y,
//      r->dec.ui.w, r->dec.ui.h, r->tm.x, em, r->inc.ui.x, r->inc.ui.y);
}

static void uic_slider_paint(uic_t* ui) {
    assert(ui->tag == uic_tag_slider);
    uic_slider_t* r = (uic_slider_t*)ui;
    gdi.push(ui->x, ui->y);
    gdi.set_clip(ui->x, ui->y, ui->w, ui->h);
    const int32_t em = ui->em.x;
    const int32_t em2  = max(1, em / 2);
    const int32_t em4  = max(1, em / 8);
    const int32_t em8  = max(1, em / 8);
    const int32_t em16 = max(1, em / 16);
    gdi.set_brush(gdi.brush_color);
    pen_t pen_grey45 = gdi.create_pen(colors.dkgray3, em16);
    gdi.set_pen(pen_grey45);
    gdi.set_brush_color(colors.dkgray3);
    const int32_t x = ui->x + r->dec.ui.w + em2;
    const int32_t y = ui->y;
    const int32_t w = r->tm.x + em;
    const int32_t h = ui->h;
    gdi.rounded(x - em8, y, w + em4, h, em4, em4);
    gdi.gradient(x, y, w, h / 2,
        colors.dkgray3, colors.btn_gradient_darker, true);
    gdi.gradient(x, y + h / 2, w, ui->h - h / 2,
        colors.btn_gradient_darker, colors.dkgray3, true);
    gdi.set_brush_color(colors.dkgreen);
    pen_t pen_grey30 = gdi.create_pen(colors.dkgray1, em16);
    gdi.set_pen(pen_grey30);
    const double range = (double)r->vmax - (double)r->vmin;
    double vw = (double)(r->tm.x + em) * (r->value - r->vmin) / range;
    gdi.rect(x, ui->y, (int32_t)(vw + 0.5), ui->h);
    gdi.x += r->dec.ui.w + em;
    const char* format = app.nls(ui->text);
    gdi.text(format, r->value);
    gdi.set_clip(0, 0, 0, 0);
    gdi.delete_pen(pen_grey30);
    gdi.delete_pen(pen_grey45);
    gdi.pop();
}

static void uic_slider_keyboard(uic_t* ui, int unused) {
    (void)ui; (void)unused;
    assert(ui->tag == uic_tag_slider);
}

static void uic_slider_mouse(uic_t* ui, int message, int f) {
    assert(ui->tag == uic_tag_slider);
    uic_slider_t* r = (uic_slider_t*)ui;
    assert(!ui->hidden && !ui->disabled);
    bool drag = message == messages.mouse_move &&
        (f & (mouse_flags.left_button|mouse_flags.right_button)) != 0;
    if (message == messages.left_button_down ||
        message == messages.right_button_down || drag) {
        const int32_t x = app.mouse.x - ui->x - r->dec.ui.w;
        const int32_t y = app.mouse.y - ui->y;
        const int32_t x0 = ui->em.x / 2;
        const int32_t x1 = r->tm.x + ui->em.x;
        if (x0 <= x && x < x1 && 0 <= y && y < ui->h) {
            const double range = (double)r->vmax - (double)r->vmin;
            double v = ((double)x - x0) * range / (double)(x1 - x0 - 1);
            int32_t vw = (int32_t)(v + r->vmin + 0.5);
            r->value = min(max(vw, r->vmin), r->vmax);
            if (r->cb != null) { r->cb(r); }
            ui->invalidate(ui);
        }
    }
}

static void uic_slider_inc_dec_value(uic_slider_t* r, int sign, int mul) {
    // full 0x80000000..0x7FFFFFFF (-2147483648..2147483647) range
    int32_t v = r->value;
    if (v > r->vmin && sign < 0) {
        mul = min(v - r->vmin, mul);
        v += mul * sign;
    } else if (v < r->vmax && sign > 0) {
        mul = min(r->vmax - v, mul);
        v += mul * sign;
    }
    if (r->value != v) {
        r->value = v;
        if (r->cb != null) { r->cb(r); }
        r->ui.invalidate(&r->ui);
    }
}

static void uic_slider_inc_dec(uic_button_t* b) {
    uic_slider_t* r = (uic_slider_t*)b->ui.parent;
    int32_t sign = b == &r->inc ? +1 : -1;
    int32_t mul = app.shift && app.ctrl ? 1000 :
        app.shift ? 100 : app.ctrl ? 10 : 1;
    uic_slider_inc_dec_value(r, sign, mul);
}

static void uic_slider_periodically(uic_t* ui) { // 100ms
    assert(ui->tag == uic_tag_slider);
    uic_slider_t* r = (uic_slider_t*)ui;
    if (!r->dec.ui.armed && !r->inc.ui.armed) {
        r->time = 0;
    } else {
        if (r->time == 0) {
            r->time = app.now;
        } else if (app.now - r->time > 1.0) {
            const int sign = r->dec.ui.armed ? -1 : +1;
            int s = (int)(app.now - r->time + 0.5);
            int32_t mul = s >= 1 ? 1 << (s - 1) : 1;
            const int64_t range = (int64_t)r->vmax - r->vmin;
            if (mul > range / 8) { mul = (int32_t)(range / 8); }
            uic_slider_inc_dec_value(r, sign, max(mul, 1));
        }
    }
}

void uic_slider_init(uic_slider_t* r, const char* label, double ems,
        int32_t vmin, int32_t vmax, void (*cb)(uic_slider_t* r)) {
    static_assert(offsetof(uic_slider_t, ui) == 0, "offsetof(.ui)");
    assert(ems >= 3.0, "allow 1em for each of [-] and [+] buttons");
    ui_init(&r->ui);
    uic_set_label(&r->ui, label);
    r->cb = cb;
    r->ui.tag = uic_tag_slider;
    r->ui.mouse = uic_slider_mouse;
    r->ui.measure = uic_slider_measure;
    r->ui.layout = uic_slider_layout;
    r->ui.paint = uic_slider_paint;
    r->ui.keyboard = uic_slider_keyboard;
    r->ui.periodically = uic_slider_periodically;
    r->ui.width = ems;
    r->vmin = vmin;
    r->vmax = vmax;
    r->value = vmin;
    r->buttons[0] = &r->dec.ui;
    r->buttons[1] = &r->inc.ui;
    r->buttons[2] = null;
    r->ui.children = r->buttons;
    // Heavy Minus Sign
    uic_button_init(&r->dec, "\xE2\x9E\x96", 0, uic_slider_inc_dec);
    // Heavy Plus Sign
    uic_button_init(&r->inc, "\xE2\x9E\x95", 0, uic_slider_inc_dec);
    static const char* accel =
        "Accelerate by Ctrl x10 Shift x100 and Ctrl+Shift x1000";
    strprintf(r->inc.ui.tip, "%s", accel);
    strprintf(r->dec.ui.tip, "%s", accel);
    r->dec.ui.parent = &r->ui;
    r->inc.ui.parent = &r->ui;
    r->ui.localize(&r->ui);
}

// message box

static void uic_messagebox_button(uic_button_t* b) {
    uic_messagebox_t* mx = (uic_messagebox_t*)b->ui.parent;
    assert(mx->ui.tag == uic_tag_messagebox);
    mx->option = -1;
    for (int i = 0; i < countof(mx->button) && mx->option < 0; i++) {
        if (b == &mx->button[i]) {
            mx->option = i;
            mx->cb(mx, i);
        }
    }
    app.show_toast(null, 0);
}

static void uic_messagebox_measure(uic_t* ui) {
    uic_messagebox_t* mx = (uic_messagebox_t*)ui;
    assert(ui->tag == uic_tag_messagebox);
    int n = 0;
    for (uic_t** c = ui->children; c != null && *c != null; c++) { n++; }
    n--; // number of buttons
    mx->text.ui.measure(&mx->text.ui);
    const int em_x = mx->text.ui.em.x;
    const int em_y = mx->text.ui.em.y;
    const int tw = mx->text.ui.w;
    const int th = mx->text.ui.h;
    if (n > 0) {
        int bw = 0;
        for (int i = 0; i < n; i++) {
            bw += mx->button[i].ui.w;
        }
        ui->w = max(tw, bw + em_x * 2);
        ui->h = th + mx->button[0].ui.h + em_y + em_y / 2;
    } else {
        ui->h = th + em_y / 2;
        ui->w = tw;
    }
}

static void uic_messagebox_layout(uic_t* ui) {
    uic_messagebox_t* mx = (uic_messagebox_t*)ui;
    assert(ui->tag == uic_tag_messagebox);
//  traceln("ui.y=%d", ui->y);
    int n = 0;
    for (uic_t** c = ui->children; c != null && *c != null; c++) { n++; }
    n--; // number of buttons
    const int em_y = mx->text.ui.em.y;
    mx->text.ui.x = ui->x;
    mx->text.ui.y = ui->y + em_y * 2 / 3;
    const int tw = mx->text.ui.w;
    const int th = mx->text.ui.h;
    if (n > 0) {
        int bw = 0;
        for (int i = 0; i < n; i++) {
            bw += mx->button[i].ui.w;
        }
        // center text:
        mx->text.ui.x = ui->x + (ui->w - tw) / 2;
        // spacing between buttons:
        int sp = (ui->w - bw) / (n + 1);
        int x = sp;
        for (int i = 0; i < n; i++) {
            mx->button[i].ui.x = ui->x + x;
            mx->button[i].ui.y = ui->y + th + em_y * 3 / 2;
            x += mx->button[i].ui.w + sp;
        }
    }
}

void _messagebox_init_(uic_t* ui) {
    assert(ui->tag == uic_tag_messagebox);
    uic_messagebox_t* mx = (uic_messagebox_t*)ui;
    ui_init(ui);
    ui->measure = uic_messagebox_measure;
    ui->layout = uic_messagebox_layout;
    mx->ui.font = &app.fonts.H3;
    const char** opts = mx->opts;
    int n = 0;
    while (opts[n] != null && n < countof(mx->button) - 1) {
        uic_button_init(&mx->button[n], opts[n], 6.0, uic_messagebox_button);
        mx->button[n].ui.parent = &mx->ui;
        n++;
    }
    assert(n <= countof(mx->button));
    if (n > countof(mx->button)) { n = countof(mx->button); }
    mx->children[0] = &mx->text.ui;
    for (int i = 0; i < n; i++) {
        mx->children[i + 1] = &mx->button[i].ui;
        mx->children[i + 1]->font = mx->ui.font;
        mx->button[i].ui.localize(&mx->button[i].ui);
    }
    mx->ui.children = mx->children;
    uic_text_init_ml(&mx->text, 0.0, "%s", mx->ui.text);
    mx->text.ui.font = mx->ui.font;
    mx->text.ui.localize(&mx->text.ui);
    mx->ui.text[0] = 0;
    mx->option = -1;
}

void uic_messagebox_init(uic_messagebox_t* mx, const char* opts[],
        void (*cb)(uic_messagebox_t* m, int option),
        const char* format, ...) {
    mx->ui.tag = uic_tag_messagebox;
    mx->ui.measure = uic_messagebox_measure;
    mx->ui.layout = uic_messagebox_layout;
    mx->opts = opts;
    mx->cb = cb;
    va_list vl;
    va_start(vl, format);
    crt.vformat(mx->ui.text, countof(mx->ui.text), format, vl);
    uic_text_init_ml(&mx->text, 0.0, mx->ui.text);
    va_end(vl);
    _messagebox_init_(&mx->ui);
}

// layouts

static void layout_center(uic_t* ui) {
    assert(ui->children != null && ui->children[0] != null, "no children?");
    assert(ui->children[1] == null, "must be single child");
    uic_t* c = ui->children[0];
    c->x = (ui->w - c->w) / 2;
    c->y = (ui->h - c->h) / 2;
}

static void layout_horizontal(uic_t* ui, int x, int y, int gap) {
    assert(ui->children != null && ui->children[0] != null, "no children?");
    uic_t** c = ui->children;
    while (*c != null) {
        uic_t* u = *c;
        u->x = x;
        u->y = y;
        x += u->w + gap;
        c++;
    }
}

static void layout_vertical(uic_t* ui, int x, int y, int gap) {
    assert(ui->children != null && ui->children[0] != null, "no children?");
    uic_t** c = ui->children;
    while (*c != null) {
        uic_t* u = *c;
        u->x = x;
        u->y = y;
        y += u->h + gap;
        c++;
    }
}

static void measure_grid(uic_t* ui, int gap_h, int gap_v) {
    int cols = 0;
    for (uic_t** row = ui->children; *row != null; row++) {
        uic_t* r = *row;
        int n = 0;
        for (uic_t** col = r->children; *col != null; col++) { n++; }
        if (cols == 0) { cols = n; }
        assert(n > 0 && cols == n);
    }
    int32_t* mxw = (int32_t*)alloca(cols * sizeof(int32_t));
    memset(mxw, 0, cols * sizeof(int32_t));
    for (uic_t** row = ui->children; *row != null; row++) {
        (*row)->h = 0;
        int i = 0;
        for (uic_t** col = (*row)->children; *col != null; col++) {
            mxw[i] = max(mxw[i], (*col)->w);
            (*row)->h = max((*row)->h, (*col)->h);
            (*row)->baseline = max((*row)->baseline, (*col)->baseline);
            i++;
        }
    }
    ui->h = 0;
    ui->w = 0;
    for (uic_t** row = ui->children; *row != null; row++) {
        uic_t* r = *row;
        r->w = 0;
        int i = 0;
        for (uic_t** col = r->children; *col != null; col++) {
            uic_t* c = *col;
            c->h = r->h; // all cells are same height
            if (c->tag == uic_tag_text) { // lineup text baselines
                uic_text_t* t = (uic_text_t*)c;
                t->dy = r->baseline - c->baseline;
            }
            c->w = mxw[i++];
            r->w += c->w;
            if (col[1] != null) { r->w += gap_h; }
            ui->w = max(ui->w, r->w);
        }
        ui->h += r->h;
        if (row[1] != null) { ui->h += gap_v; }
    }
}

static void layout_grid(uic_t* ui, int gap_h, int gap_v) {
    assert(ui->children != null, "layout_grid() with no children?");
    int x = ui->x;
    int y = ui->y;
    for (uic_t** row = ui->children; *row != null; row++) {
        int xc = x;
        for (uic_t** col = (*row)->children; *col != null; col++) {
            (*col)->x = xc;
            (*col)->y = y;
            xc += (*col)->w + gap_h;
        }
        y += (*row)->h + gap_v;
    }
}

layouts_t layouts = {
    .center = layout_center,
    .horizontal = layout_horizontal,
    .vertical = layout_vertical,
    .measure_grid = measure_grid,
    .layout_grid = layout_grid
};

end_c

// app implementation

#define INIT_GUID
#include <Shlobj.h>
#include <ShellScalingApi.h>
#include <dwmapi.h>
#include <commdlg.h>

begin_c

#define WM_ANIMATE  (WM_APP + 0x7FFF)
#define WM_OPENNING (WM_APP + 0x7FFE)
#define WM_CLOSING  (WM_APP + 0x7FFD)

#define window() ((HWND)app.window)
#define canvas() ((HDC)app.canvas)

messages_t messages = {
    .left_button_down  = WM_LBUTTONDOWN,
    .left_button_up    = WM_LBUTTONUP,
    .right_button_down = WM_RBUTTONDOWN,
    .right_button_up   = WM_RBUTTONUP,
    .mouse_move        = WM_MOUSEMOVE
};

mouse_flags_t mouse_flags = {
    .left_button = MK_LBUTTON,
    .right_button = MK_RBUTTON,
};

virtual_keys_t virtual_keys = {
    .up    = VK_UP,
    .down  = VK_DOWN,
    .left  = VK_LEFT,
    .right = VK_RIGHT
};


// https://docs.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-showwindow

window_visibility_t window_visibility = {
    .hide      = SW_HIDE,
    .normal    = SW_SHOWNORMAL,
    .minimize  = SW_SHOWMINIMIZED,
    .maximize  = SW_SHOWMAXIMIZED,
    .normal_na = SW_SHOWNOACTIVATE,
    .show      = SW_SHOW,
    .min_next  = SW_MINIMIZE,
    .min_na    = SW_SHOWMINNOACTIVE,
    .show_na   = SW_SHOWNA,
    .restore   = SW_RESTORE,
    .defau1t   = SW_SHOWDEFAULT,
    .force_min = SW_FORCEMINIMIZE
};

typedef LPARAM lparam_t;
typedef WPARAM wparam_t;

static NONCLIENTMETRICSW ncm = { sizeof(NONCLIENTMETRICSW) };
static MONITORINFO mi = {sizeof(MONITORINFO)};

static HANDLE app_event_quit;
static HANDLE app_event_invalidate;

static uintptr_t app_timer_1s_id;
static uintptr_t app_timer_100ms_id;

static bool app_layout_dirty; // call layout() before paint

typedef void (*app_animate_function_t)(int step);

static struct {
    app_animate_function_t f;
    int count;
    int step;
    tm_t timer;
} app_animate;

// Animation timer is Windows minimum of 10ms, but in reality the timer
// messages are far from isochronous and more likely to arrive at 16 or
// 32ms intervals and can be delayed.

enum { toast_steps = 15 }; // number of animation steps

static struct {
    uic_t* ui;
    int step;
    double time; // closing time or zero
    int x; // -1 for toast
    int y; // screen coordinates for tooltip
} toast;

static void app_on_every_message(uic_t* ui);

// https://docs.microsoft.com/en-us/windows/win32/inputdev/virtual-key-codes
// https://docs.microsoft.com/en-us/windows/win32/inputdev/wm-keydown

static void app_alt_ctrl_shift(bool down, int key) {
    if (key == VK_MENU)    { app.alt   = down; }
    if (key == VK_CONTROL) { app.ctrl  = down; }
    if (key == VK_SHIFT)   { app.shift = down; }
}

static inline_c ui_point_t app_point2ui(const POINT* p) {
    ui_point_t u = { p->x, p->y };
    return u;
}

static inline_c POINT app_ui2point(const ui_point_t* u) {
    POINT p = { u->x, u->y };
    return p;
}

static ui_rect_t app_rect2ui(const RECT* r) {
    ui_rect_t u = { r->left, r->top, r->right - r->left, r->bottom - r->top };
    return u;
}

static RECT app_ui2rect(const ui_rect_t* u) {
    RECT r = { u->x, u->y, u->x + u->w, u->y + u->h };
    return r;
}

static void app_update_ncm(int dpi) {
    // Only UTF-16 version supported SystemParametersInfoForDpi
    fatal_if_false(SystemParametersInfoForDpi(SPI_GETNONCLIENTMETRICS, sizeof(ncm), &ncm, 0, dpi));
}

static void app_update_monitor_dpi(HMONITOR monitor) {
    for (int mtd = MDT_EFFECTIVE_DPI; mtd <= MDT_RAW_DPI; mtd++) {
        uint32_t dpi_x = 0;
        uint32_t dpi_y = 0;
        // GetDpiForMonitor() may return ERROR_GEN_FAILURE 0x8007001F when
        // system wakes up from sleep:
        // ""A device attached to the system is not functioning."
        // docs say:
        // "May be used to indicate that the device has stopped responding
        // or a general failure has occurred on the device.
        // The device may need to be manually reset."
        int r = GetDpiForMonitor(monitor, (MONITOR_DPI_TYPE)mtd, &dpi_x, &dpi_y);
        if (r != 0) {
            crt.sleep(1.0 / 32); // and retry:
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
                    app.dpi.monitor_effective = max(dpi_x, dpi_y); break;
                case MDT_ANGULAR_DPI:
                    app.dpi.monitor_angular = max(dpi_x, dpi_y); break;
                case MDT_RAW_DPI:
                    app.dpi.monitor_raw = max(dpi_x, dpi_y); break;
                default: assert(false);
            }
        }
    }
}

static void app_update_mi(const ui_rect_t* r, uint32_t flags) {
    RECT rc = app_ui2rect(r);
    HMONITOR monitor = MonitorFromRect(&rc, flags);
    if (monitor != null) {
        app_update_monitor_dpi(monitor);
        fatal_if_false(GetMonitorInfoA(monitor, &mi));
        app.work_area = app_rect2ui(&mi.rcWork);
        app.mrc = app_rect2ui(&mi.rcMonitor);
    }
}

static void app_update_crc() {
    RECT rc = {0};
    fatal_if_false(GetClientRect(window(), &rc));
    app.crc = app_rect2ui(&rc);
    app.width = app.crc.w;
    app.height = app.crc.h;
}

static void app_dispose_fonts() {
    fatal_if_false(DeleteFont(app.fonts.regular));
    fatal_if_false(DeleteFont(app.fonts.H1));
    fatal_if_false(DeleteFont(app.fonts.H2));
    fatal_if_false(DeleteFont(app.fonts.H3));
    fatal_if_false(DeleteFont(app.fonts.mono));
}

static void app_init_fonts(int dpi) {
    app_update_ncm(dpi);
    if (app.fonts.regular != null) { app_dispose_fonts(); }
    app.fonts.regular = (font_t)CreateFontIndirectW(&ncm.lfMessageFont);
    fatal_if_null(app.fonts.regular);
    const double fh = ncm.lfMessageFont.lfHeight;
//  traceln("lfHeight=%.1f", fh);
    assert(fh != 0);
    LOGFONTW lf = ncm.lfMessageFont;
    lf.lfWeight = FW_SEMIBOLD;
    lf.lfHeight = (int)(fh * 1.75);
    app.fonts.H1 = (font_t)CreateFontIndirectW(&lf);
    lf.lfWeight = FW_SEMIBOLD;
    lf.lfHeight = (int)(fh * 1.4);
    app.fonts.H2 = (font_t)CreateFontIndirectW(&lf);
    lf.lfWeight = FW_SEMIBOLD;
    lf.lfHeight = (int)(fh * 1.15);
    app.fonts.H3 = (font_t)CreateFontIndirectW(&lf);
    lf = ncm.lfMessageFont;
    lf.lfPitchAndFamily = FIXED_PITCH;
    #define monospaced "Cascadia Code"
    wcscpy(lf.lfFaceName, L"Cascadia Code");
    app.fonts.mono = (font_t)CreateFontIndirectW(&lf);
    app.cursor_arrow = (cursor_t)LoadCursorA(null, IDC_ARROW);
    app.cursor_wait = (cursor_t)LoadCursorA(null, IDC_WAIT);
    app.cursor = app.cursor_arrow;
}

static HKEY app_get_reg_key() {
    char path[MAX_PATH];
    strprintf(path, "Software\\app\\%s", app.class_name);
    HKEY key = null;
    if (RegOpenKey(HKEY_CURRENT_USER, path, &key) != 0) {
        RegCreateKey(HKEY_CURRENT_USER, path, &key);
    }
    assert(key != null);
    return key;
}

static void app_data_save(const char* name, const void* data, int bytes) {
    HKEY key = app_get_reg_key();
    if (key != null) {
        fatal_if_not_zero(RegSetValueExA(key, name, 0, REG_BINARY,
            (byte*)data, bytes));
        fatal_if_not_zero(RegCloseKey(key));
    }
}

static int app_data_size(const char* name) {
    int bytes = -1;
    HKEY key = app_get_reg_key();
    if (key != null) {
        DWORD type = REG_BINARY;
        DWORD cb = sizeof(app.last_visibility);
        int r = RegQueryValueExA(key, name, null, &type, null, &cb);
        if (r == ERROR_FILE_NOT_FOUND) {
            bytes = 0; // do not report data_size() often used this way
        } else if (r != 0) {
            traceln("RegQueryValueExA(\"%s\") failed %s", name,
                crt.error(r));
        } else {
            bytes = (int)cb;
        }
        fatal_if_not_zero(RegCloseKey(key));
    }
    return bytes;
}

static int app_data_load(const char* name, void* data, int bytes) {
    int read = -1;
    HKEY key = app_get_reg_key();
    if (key != null) {
        DWORD type = REG_BINARY;
        DWORD cb = (DWORD)bytes;
        int r = RegQueryValueExA(key, "window", null, &type, (byte*)data, &cb);
        if (r != ERROR_MORE_DATA) {
            traceln("RegQueryValueExA(\"%s\") failed %s", name,
                crt.error(r));
        } else {
            read = (int)cb;
        }
        fatal_if_not_zero(RegCloseKey(key));
    }
    return read;
}

static void app_save_window_pos() {
    HKEY key = app_get_reg_key();
    if (key != null) {
        WINDOWPLACEMENT wpl = {0};
        wpl.length = sizeof(wpl);
        fatal_if_false(GetWindowPlacement(window(), &wpl));
        fatal_if_not_zero(RegSetValueExA(key, "window", 0, REG_BINARY, (byte*)&wpl.rcNormalPosition, sizeof(wpl.rcNormalPosition)));
        fatal_if_not_zero(RegSetValueExA(key, "show", 0, REG_DWORD, (byte*)&wpl.showCmd, sizeof(wpl.showCmd)));
        fatal_if_not_zero(RegCloseKey(key));
    }
}

static void load_window_pos(ui_rect_t* rect) {
    HKEY key = app_get_reg_key();
    if (key != null) {
        DWORD type = REG_DWORD;
        DWORD cb = sizeof(app.last_visibility);
        if (RegQueryValueExA(key, "show", null, &type, (byte*)&app.last_visibility, &cb) == 0) {
            // if there is last show_command state in the registry it supersedes
            // startup info
        } else {
            app.last_visibility = window_visibility.defau1t;
        }
        RECT rc = {0};
        cb = sizeof(rc);
        type = REG_BINARY;
        if (RegQueryValueExA(key, "window", null, &type, (byte*)&rc, &cb) == 0 &&
            type == REG_BINARY && cb == sizeof(RECT)) {
            RECT screen = {0, 0, 1920, 1080};
            RECT work_area = {0, 0, 1920, 1080};
            if (SystemParametersInfoA(SPI_GETWORKAREA, 0, &work_area, false)) {
                screen = work_area;
            }
            app.work_area = app_rect2ui(&screen);
            app_update_mi(&app.work_area, MONITOR_DEFAULTTONEAREST);
            RECT intersect = {0};
            if (IntersectRect(&intersect, &rc, &screen) && !IsRectEmpty(&intersect)) {
                *rect = app_rect2ui(&intersect);
            } else {
                traceln("WARNING: out of work area");
            }
        }
        fatal_if_not_zero(RegCloseKey(key));
    }
}

static void timer_kill(tm_t timer) {
    fatal_if_false(KillTimer(window(), timer));
}

static tm_t timer_set(uintptr_t id, int32_t ms) {
    assert(window() != null);
    assert(0 <= id); // can be zero see notes in .h file
    assert(10 <= ms && ms < 0x7FFFFFFF);
    tm_t tid = (tm_t)SetTimer(window(), id, (uint32_t)ms, null);
    fatal_if(tid == 0);
    assert(tid == id);
    return tid;
}

static void set_parents(uic_t* ui) {
    for (uic_t** c = ui->children; c != null && *c != null; c++) {
        if ((*c)->parent == null) {
            (*c)->parent = ui;
            set_parents(*c);
        } else {
            assert((*c)->parent == ui, "no reparenting");
        }
    }
}

static void init_children(uic_t* ui) {
    for (uic_t** c = ui->children; c != null && *c != null; c++) {
        if ((*c)->init != null) { (*c)->init(*c); (*c)->init = null; }
        init_children(*c);
    }
}

static void app_post_message(int m, int64_t wp, int64_t lp) {
    fatal_if_false(PostMessageA(window(), m, wp, lp));
}

static void app_timer(uic_t* ui, tm_t id) {
    if (ui->timer != null) {
        ui->timer(ui, id);
    }
    if (id == app_timer_1s_id && ui->once_upon_a_second != null) {
        ui->once_upon_a_second(ui);
    }
    if (id == app_timer_100ms_id && ui->periodically != null) {
        ui->periodically(ui);
    }
    uic_t** c = ui->children;
    while (c != null && *c != null) { app_timer(*c, id); c++; }
}

static void app_periodically(tm_t id) {
    if (id == app_timer_1s_id && app.once_upon_a_second != null) {
        app.once_upon_a_second();
    }
    if (id == app_timer_100ms_id && app.periodically != null) {
        app.periodically();
    }
    if (toast.time != 0 && app.now > toast.time) {
        app.show_toast(null, 0);
    }
}

static void app_animate_timer() {
    app_post_message(WM_ANIMATE, (uint64_t)app_animate.step + 1,
        (uintptr_t)app_animate.f);
}

static void app_wm_timer(tm_t id) {
    app_periodically(id);
    if (app_animate.timer == id) { app_animate_timer(); }
    app_timer(app.ui, id);
}

static void app_window_openning() {
    app_timer_1s_id = app.set_timer((uintptr_t)&app_timer_1s_id, 1000);
    app_timer_100ms_id = app.set_timer((uintptr_t)&app_timer_100ms_id, 100);
    app.set_cursor(app.cursor_arrow);
    app.canvas = (canvas_t)GetDC(window());
    fatal_if_null(app.canvas);
    if (app.openned != null) { app.openned(); }
    app.ui->em = gdi.get_em(*app.ui->font);
    set_parents(app.ui);
    init_children(app.ui);
    app_wm_timer(app_timer_100ms_id);
    app_wm_timer(app_timer_1s_id);
    fatal_if(ReleaseDC(window(), canvas()) == 0);
    app.canvas = null;
    app.layout(); // request layout
    if (app.last_visibility == window_visibility.maximize) {
        ShowWindow(window(), window_visibility.maximize);
    }
//  if (forced_locale != 0) {
//      SendMessageTimeoutA(HWND_BROADCAST, WM_SETTINGCHANGE, 0, (uintptr_t)"intl", 0, 1000, null);
//  }
}

static void app_window_closing() {
    if (app.can_close == null || app.can_close()) {
        if (app.is_full_screen) { app.full_screen(false); }
        app_save_window_pos();
        app.kill_timer(app_timer_1s_id);
        app.kill_timer(app_timer_100ms_id);
        if (app.closed != null) { app.closed(); }
        app_timer_1s_id = 0;
        app_timer_100ms_id = 0;
        DestroyWindow(window());
        app.window = null;
    }
}

static void app_get_min_max_info(MINMAXINFO* mmi) {
    mmi->ptMinTrackSize.x = app.min_width;
    mmi->ptMinTrackSize.y = app.min_height;
    mmi->ptMaxTrackSize.x = app.max_width;
    mmi->ptMaxTrackSize.y = app.max_height;
    mmi->ptMaxSize.x = app.max_width;
    mmi->ptMaxSize.y = app.max_height;
}

#define app_method_int32(name)                                  \
static void app_##name(uic_t* ui, int32_t p) {                  \
    if (ui->name != null && !ui->hidden && !ui->disabled) {     \
        ui->name(ui, p);                                        \
    }                                                           \
    uic_t** c = ui->children;                                   \
    while (c != null && *c != null) { app_##name(*c, p); c++; } \
}

app_method_int32(keyboard)
app_method_int32(key_up)
app_method_int32(key_down)

static void app_paint(uic_t* ui) {
    if (!ui->hidden) {
        if (ui->paint != null) { ui->paint(ui); }
        uic_t** c = ui->children;
        while (c != null && *c != null) { app_paint(*c); c++; }
    }
}

static void app_mousewheel(uic_t* ui, int dx, int dy) {
    if (!ui->hidden && !ui->disabled) {
        if (ui->mousewheel != null) { ui->mousewheel(ui, dx, dy); }
        uic_t** c = ui->children;
        while (c != null && *c != null) { app_mousewheel(*c, dx, dy); c++; }
    }
}

static void app_measure_children(uic_t* ui) {
    if (!ui->hidden) {
        uic_t** c = ui->children;
        while (c != null && *c != null) { app_measure_children(*c); c++; }
        if (ui->measure != null) { ui->measure(ui); }
    }
}

static void app_layout_children(uic_t* ui) {
    if (!ui->hidden) {
        if (ui->layout != null) { ui->layout(ui); }
        uic_t** c = ui->children;
        while (c != null && *c != null) { app_layout_children(*c); c++; }
    }
}

static void app_layout_ui(uic_t* ui) {
    app_measure_children(ui);
    app_layout_children(ui);
}

static bool app_message(uic_t* ui, int32_t m, int64_t wp, int64_t lp,
        int64_t* ret) {
    // message() callback is called even for hidden and disabled ui-elements
    // consider timers, and other useful messages
    app_on_every_message(ui);
    if (ui->message != null) {
        if (ui->message(ui, m, wp, lp, ret)) { return true; }
    }
    uic_t** c = ui->children;
    while (c != null && *c != null) {
        if (app_message(*c, m, wp, lp, ret)) { return true; }
        c++;
    }
    return false;
}

static void app_toast_mouse(int32_t m, int32_t f);
static void app_toast_keyboard(int32_t ch);

static void app_ui_keyboard(uic_t* ui, int32_t ch) {
    if (toast.ui != null) {
        app_toast_keyboard(ch);
    } else {
        app_keyboard(ui, ch);
    }
}

static void app_hover_changed(uic_t* ui) {
    if (ui->hovering != null) {
        if (!ui->hover) {
            ui->hover_at = 0;
            ui->hovering(ui, false); // cancel hover
        } else {
            assert(ui->hover_delay >= 0);
            if (ui->hover_delay == 0) {
                ui->hover_at = -1;
                ui->hovering(ui, true); // call immediately
            } else if (ui->hover_delay != 0 && ui->hover_at >= 0) {
                ui->hover_at = app.now + ui->hover_delay;
            }
        }
    }
}

// app_on_every_message() is called on every message including timers
// allowing ui elements to do scheduled actions like e.g. hovering()

static void app_on_every_message(uic_t* ui) {
    if (ui->hovering != null) {
        if (ui->hover_at > 0 && app.now > ui->hover_at) {
            ui->hover_at = -1; // "already called"
            ui->hovering(ui, true);
        }
    }
}

static void app_ui_mouse(uic_t* ui, int32_t m, int32_t f) {
    if (m == WM_MOUSEHOVER || m == WM_MOUSEMOVE) {
        RECT rc = { ui->x, ui->y, ui->x + ui->w, ui->y + ui->h};
        bool hover = ui->hover;
        POINT pt = app_ui2point(&app.mouse);
        ui->hover = PtInRect(&rc, pt);
        InflateRect(&rc, ui->w / 4, ui->h / 4);
        ui_rect_t r = app_rect2ui(&rc);
        if (hover != ui->hover) { app.invalidate(&r); }
        if (hover != ui->hover && ui->hovering != null) {
            app_hover_changed(ui);
        }
    }
    if (ui->mouse != null && !ui->hidden && !ui->disabled) {
        ui->mouse(ui, m, f);
    }
    for (uic_t** c = ui->children; c != null && *c != null; c++) {
        app_ui_mouse(*c, m, f);
    }
}

static bool app_context_menu(uic_t* ui) {
    for (uic_t** c = ui->children; c != null && *c != null; c++) {
        if (app_context_menu(*c)) { return true; }
    }
    RECT rc = { ui->x, ui->y, ui->x + ui->w, ui->y + ui->h};
    POINT pt = app_ui2point(&app.mouse);
    if (PtInRect(&rc, pt)) {
        if (!ui->hidden && !ui->disabled && ui->context_menu != null) {
            ui->context_menu(ui);
        }
    }
    return false;
}

static void app_mouse(uic_t* ui, int32_t m, int32_t f) {
    if (toast.ui != null && toast.ui->mouse != null) {
        app_ui_mouse(toast.ui, m, f);
    } else if (toast.ui != null && toast.ui->mouse == null) {
        app_toast_mouse(m, f);
        bool tooltip = toast.x >= 0 && toast.y >= 0;
        if (tooltip) { app_ui_mouse(ui, m, f); }
    } else {
        app_ui_mouse(ui, m, f);
    }
}

static void app_toast_paint() {
    static image_t image;
    if (image.bitmap == null) {
        byte pixels[4] = { 0x3F, 0x3F, 0x3F };
        gdi.image_init(&image, 1, 1, 3, pixels);
    }
    if (toast.ui != null) {
        font_t f = toast.ui->font != null ? *toast.ui->font : app.fonts.regular;
        const ui_point_t em = gdi.get_em(f);
        toast.ui->em = em;
        // allow unparented and unmeasureed toasts:
        if (toast.ui->measure != null) { toast.ui->measure(toast.ui); }
        gdi.push(0, 0);
        bool tooltip = toast.x >= 0 && toast.y >= 0;
        int em_x = em.x;
        int em_y = em.y;
        gdi.set_brush(gdi.brush_color);
        gdi.set_brush_color(colors.toast);
        if (!tooltip) {
            assert(0 <= toast.step && toast.step < toast_steps);
            int step = toast.step - (toast_steps - 1);
            toast.ui->y = toast.ui->h * step / (toast_steps - 1);
//          traceln("step=%d of %d y=%d", toast.step, toast_steps, toast.ui->y);
            app_layout_ui(toast.ui);
            double alpha = min(0.40, 0.40 * toast.step / (double)toast_steps);
            gdi.alpha_blend(0, 0, app.width, app.height, &image, alpha);
            toast.ui->x = (app.width - toast.ui->w) / 2;
        } else {
            toast.ui->x = toast.x;
            toast.ui->y = toast.y;
            app_layout_ui(toast.ui);
            int mx = app.width - toast.ui->w - em_x;
            toast.ui->x = min(mx, max(0, toast.x - toast.ui->w / 2));
            toast.ui->y = min(app.crc.h - em_y, max(0, toast.y));
        }
        int x = toast.ui->x - em_x;
        int y = toast.ui->y - em_y / 2;
        int w = toast.ui->w + em_x * 2;
        int h = toast.ui->h + em_y;
        gdi.rounded(x, y, w, h, em_x, em_y);
        if (!tooltip) { toast.ui->y += em_y / 4; }
        app_paint(toast.ui);
        if (!tooltip) {
            if (toast.ui->y == em_y / 4) {
                // micro "close" toast button:
                gdi.x = toast.ui->x + toast.ui->w;
                gdi.y = 0;
                gdi.text("\xC3\x97"); // Heavy Multiplication X
            }
        }
        gdi.pop();
    }
}

static void app_toast_cancel() {
    if (toast.ui != null && toast.ui->tag == uic_tag_messagebox) {
        uic_messagebox_t* mx = (uic_messagebox_t*)toast.ui;
        if (mx->option < 0) { mx->cb(mx, -1); }
    }
    toast.step = 0;
    toast.ui = null;
    toast.time = 0;
    toast.x = -1;
    toast.y = -1;
    app.redraw();
}

static void app_toast_mouse(int32_t m, int32_t flags) {
    bool down = (m == WM_LBUTTONDOWN || m == WM_RBUTTONDOWN);
    if (toast.ui != null && down) {
        const ui_point_t em = toast.ui->em;
        int x = toast.ui->x + toast.ui->w;
        if (x <= app.mouse.x && app.mouse.x <= x + em.x &&
            0 <= app.mouse.y && app.mouse.y <= em.y) {
            app_toast_cancel();
        } else {
            app_ui_mouse(toast.ui, m, flags);
        }
    } else {
        app_ui_mouse(toast.ui, m, flags);
    }
}

static void app_toast_keyboard(int32_t ch) {
    if (toast.ui != null && ch == 033) { // ESC traditionally in octal
        app_toast_cancel();
        app.show_toast(null, 0);
    } else {
        app_keyboard(toast.ui, ch);
    }
}

static void app_toast_dim(int step) {
    toast.step = step;
    app.redraw();
    UpdateWindow(window());
}

static void app_animate_step(app_animate_function_t f, int step, int steps) {
    // calls function(0..step-1) exactly step times
    bool cancel = false;
    if (f != null && f != app_animate.f && step == 0 && steps > 0) {
        // start animation
        app_animate.count = steps;
        app_animate.f = f;
        f(step);
        app_animate.timer = app.set_timer((uintptr_t)&app_animate.timer, 10);
    } else if (f != null && app_animate.f == f && step > 0) {
        cancel = step >= app_animate.count;
        if (!cancel) {
            app_animate.step = step;
            f(step);
        }
    } else if (f == null) {
        cancel = true;
    }
    if (cancel) {
        if (app_animate.timer != 0) { app.kill_timer(app_animate.timer); }
        app_animate.step = 0;
        app_animate.timer = 0;
        app_animate.f = null;
        app_animate.count = 0;
    }
}

static void app_animate_start(app_animate_function_t f, int steps) {
    // calls f(0..step-1) exactly steps times, unless cancelled with call
    // animate(null, 0) or animate(other_function, n > 0)
    app_animate_step(f, 0, steps);
}

static void app_layout_root() {
    assert(app.window != null);
    assert(app.canvas != null);
    app.ui->w = app.crc.w; // crc is window client rectangle
    app.ui->h = app.crc.h;
    app_layout_ui(app.ui);
}

static void app_paint_on_canvas(HDC hdc) {
    canvas_t canvas = app.canvas;
    app.canvas = (canvas_t)hdc;
    gdi.push(0, 0);
    if (app_layout_dirty) {
        app_layout_dirty = false;
        app_layout_root();
    }
    double time = crt.seconds();
    gdi.x = 0;
    gdi.y = 0;
    app_update_crc();
    font_t font = gdi.set_font(app.fonts.regular);
    color_t c = gdi.set_text_color(colors.text);
    int bm = SetBkMode(canvas(), TRANSPARENT);
    int sm = SetStretchBltMode(canvas(), COLORONCOLOR);
    brush_t br = gdi.set_brush(gdi.brush_hollow);
    app_paint(app.ui);
    if (toast.ui != null) { app_toast_paint(); }
    SetStretchBltMode(canvas(), sm);
    SetBkMode(canvas(), bm);
    gdi.set_brush(br);
    gdi.set_text_color(c);
    gdi.set_font(font);
    app.paint_count++;
    if (app.paint_count % 128 == 0) { app.paint_max = 0; }
    app.paint_time = (crt.seconds() - time);
    app.paint_max = max(app.paint_time, app.paint_max);
    if (app.paint_avg == 0) {
        app.paint_avg = app.paint_time;
    } else { // EMA over 32 paint() calls
        app.paint_avg = app.paint_avg * (1.0 - 1.0 / 32.0) +
                        app.paint_time / 32.0;
    }
    gdi.pop();
    app.canvas = canvas;
}

static void app_window_position_changed() {
    RECT wrc = app_ui2rect(&app.wrc);
    fatal_if_false(GetWindowRect(window(), &wrc));
    app.wrc = app_rect2ui(&wrc);
    app_update_mi(&app.wrc, MONITOR_DEFAULTTONEAREST);
    app_update_crc();
    // call layout() only if window was already openned
//  traceln("WM_WINDOWPOSCHANGED %p", timer_1s_id);
    app.ui->hidden = !IsWindowVisible(window());
    if (app_timer_1s_id != 0 && !app.ui->hidden) { app.layout(); }
}

static void app_setting_change(uintptr_t wp, uintptr_t lp) {
    if (wp == 0 && lp != 0 && strcmp((const char*)lp, "intl") == 0) {
        wchar_t ln[LOCALE_NAME_MAX_LENGTH + 1];
        int n = GetUserDefaultLocaleName(ln, countof(ln));
        fatal_if_false(n > 0);
        wchar_t rln[LOCALE_NAME_MAX_LENGTH + 1];
        n = ResolveLocaleName(ln, rln, countof(rln));
        fatal_if_false(n > 0);
        LCID lcid = LocaleNameToLCID(rln, LOCALE_ALLOW_NEUTRAL_NAMES);
        fatal_if_false(SetThreadLocale(lcid));
    }
}

static void app_show_task_bar(bool show) {
    HWND taskbar = FindWindow("Shell_TrayWnd", null);
    if (taskbar != null) {
        ShowWindow(taskbar, show ? SW_SHOW : SW_HIDE);
        UpdateWindow(taskbar);
    }
}

static LRESULT CALLBACK window_proc(HWND window, UINT msg, WPARAM wp, LPARAM lp) {
    app.now = crt.seconds();
    if (app.window == null) {
        app.window = (window_t)window;
    } else {
        assert(window() == window);
    }
    int64_t ret = 0;
    if (app_message(app.ui, msg, wp, lp, &ret)) {
        return (LRESULT)ret;
    }
    switch (msg) {
        case WM_GETMINMAXINFO: app_get_min_max_info((MINMAXINFO*)lp); break;
        case WM_SETTINGCHANGE: app_setting_change(wp, lp); break;
        case WM_CLOSE        : app_post_message(WM_CLOSING, 0, 0); return 0;
        case WM_OPENNING     : app_window_openning(); return 0;
        case WM_CLOSING      : app_window_closing(); return 0;
        case WM_DESTROY      : PostQuitMessage(0); break;
        case WM_SYSKEYDOWN: // for ALT (aka VK_MENU)
        case WM_KEYDOWN      : app_alt_ctrl_shift(true, (int32_t)wp);
                               app_key_down(app.ui, (int32_t)wp);
                               break;
        case WM_SYSKEYUP:
        case WM_KEYUP        : app_alt_ctrl_shift(false, (int32_t)wp);
                               app_key_up(app.ui, (int32_t)wp);
                               break;
        case WM_TIMER        : app_wm_timer((tm_t)wp); break;
        case WM_ERASEBKGND   : return true; // no DefWindowProc()
        case WM_SETCURSOR    : SetCursor((HCURSOR)app.cursor); break;
        case WM_CHAR         : app_ui_keyboard(app.ui, (int32_t)wp);
                               break;
        case WM_PRINTCLIENT  : app_paint_on_canvas((HDC)wp); break;
        case WM_ANIMATE      : app_animate_step((app_animate_function_t)lp,
                                    (int)wp, -1);
                               break;
        case WM_PAINT        : {
            PAINTSTRUCT ps = {0};
            BeginPaint(window(), &ps);
            app_paint_on_canvas(ps.hdc);
            EndPaint(window(), &ps);
            break;
        }
        case WM_MOUSEWHEEL:
            app_mousewheel(app.ui, 0, GET_WHEEL_DELTA_WPARAM(wp)); break;
        case WM_MOUSEHWHEEL:
            app_mousewheel(app.ui, GET_WHEEL_DELTA_WPARAM(wp), 0); break;
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
            POINT pt = {(int32_t)GET_X_LPARAM(lp), (int32_t)GET_Y_LPARAM(lp)};
            ScreenToClient(window(), &pt);
            app.mouse = app_point2ui(&pt);
            app_mouse(app.ui, (int32_t)msg, (int32_t)wp);
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
//          traceln("%d %d", app.mouse.x, app.mouse.y);
            app.mouse.x = (int32_t)GET_X_LPARAM(lp);
            app.mouse.y = (int32_t)GET_Y_LPARAM(lp);
            // note: ScreenToClient() is not needed for this messages
            app_mouse(app.ui, (int32_t)msg, (int32_t)wp);
            break;
        }
        case WM_CONTEXTMENU  : (void)app_context_menu(app.ui); break;
        case WM_GETDPISCALEDSIZE: {
            // sent before WM_DPICHANGED
            int32_t dpi = (int32_t)wp;
            SIZE* sz = (SIZE*)lp; // in/out
            ui_point_t cell = { sz->cx, sz->cy };
            traceln("WM_GETDPISCALEDSIZE dpi %d := %d "
                "size %d,%d *may/must* be adjusted",
                app.dpi.window, dpi, cell.x, cell.y);
            // TODO: not clear which way adjustment should go and what
            // the scale whould be
            if (app_timer_1s_id != 0 && !app.ui->hidden) { app.layout(); }
            break;
        }
        case WM_DPICHANGED: { // TODO
            app.dpi.window = GetDpiForWindow(window());
            app_init_fonts(app.dpi.window);
            if (app_timer_1s_id != 0 && !app.ui->hidden) { app.layout(); }
            break;
        }
        case WM_WINDOWPOSCHANGED: app_window_position_changed(); break;
        default:
            break;
    }
    return DefWindowProcA(window(), msg, wp, lp);
}

static long app_set_window_long(int index, long value) {
    SetLastError(0);
    long r = SetWindowLongA(window(), index, value);
    fatal_if_not_zero(GetLastError());
    return r;
}

static void app_create_window(ui_rect_t r, int32_t width, int32_t height) {
    WNDCLASSA wc = { 0 };
    wc.style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC | CS_DBLCLKS;
    wc.lpfnWndProc = window_proc;
    wc.cbClsExtra = 0;
    wc.cbWndExtra = 256 * 1024;
    wc.hInstance = GetModuleHandleA(null);
    #define IDI_ICON 101
    wc.hIcon = LoadIconA(wc.hInstance, MAKEINTRESOURCE(IDI_ICON));
    wc.hCursor = (HCURSOR)app.cursor;
    wc.hbrBackground = null;
    wc.lpszMenuName = null;
    wc.lpszClassName = app.class_name;
    ATOM atom = RegisterClassA(&wc);
    fatal_if(atom == 0);
    width = max(r.w, width);
    height = max(r.h, height);
    uint32_t style = app.no_decor ? WS_POPUP : WS_OVERLAPPEDWINDOW;
    HWND window = CreateWindowExA(WS_EX_COMPOSITED | WS_EX_LAYERED,
        app.class_name, app.title, style,
        r.x, r.y, width, height, null, null, wc.hInstance, null);
    assert(window == window()); (void)window;
    fatal_if_null(app.window);
    app.dpi.window = GetDpiForWindow(window());
//  traceln("app.dpi.window=%d", app.dpi.window);
    RECT wrc = app_ui2rect(&app.wrc);
    fatal_if_false(GetWindowRect(window(), &wrc));
    app.wrc = app_rect2ui(&wrc);
    color_t caption_color = colors.dkgray3;
    fatal_if_not_zero(DwmSetWindowAttribute(window(),
        DWMWA_CAPTION_COLOR, &caption_color, sizeof(caption_color)));
    if (app.aero) { // It makes app look like retro Windows 7 Aero style :)
        enum DWMNCRENDERINGPOLICY ncrp = DWMNCRP_DISABLED;
        fatal_if_not_zero(DwmSetWindowAttribute(window(),
            DWMWA_NCRENDERING_POLICY, &ncrp, sizeof(ncrp)));
    }
    // always start with window hidden and let
    app.show_window(window_visibility.hide);
    if (app.no_min_max) {
        uint32_t exclude = WS_MINIMIZEBOX|WS_MAXIMIZEBOX|WS_SIZEBOX;
        uint32_t s = GetWindowLongA(window(), GWL_STYLE);
        app_set_window_long(GWL_STYLE, s & ~exclude);
    }
    if (app.visibility != window_visibility.hide) {
        AnimateWindow(window(), 250, AW_ACTIVATE);
        app.show_window(app.visibility);
        app_update_crc();
        app.ui->w = app.crc.w; // app.crc is "client rectangle"
        app.ui->h = app.crc.h;
    }
    // even if it is hidden:
    app_post_message(WM_OPENNING, 0, 0);
//  SetWindowTheme(window(), L"DarkMode_Explorer", null); ???
}

static void app_full_screen(bool on) {
    static int32_t style;
    static WINDOWPLACEMENT wp;
    if (on != app.is_full_screen) {
        app_show_task_bar(!on);
        if (on) {
            style = GetWindowLongA(window(), GWL_STYLE);
            wp.length = sizeof(wp);
            fatal_if_false(GetWindowPlacement(window(), &wp));
            WINDOWPLACEMENT nwp = wp;
            nwp.showCmd = SW_SHOWNORMAL;
            nwp.rcNormalPosition = (RECT){app.mrc.x, app.mrc.y,
                app.mrc.x + app.mrc.w, app.mrc.y + app.mrc.h};
            fatal_if_false(SetWindowPlacement(window(), &nwp));
            app_set_window_long(GWL_STYLE, (style | WS_VISIBLE) &
                ~(WS_OVERLAPPEDWINDOW));
        } else {
            fatal_if_false(SetWindowPlacement(window(), &wp));
            app_set_window_long(GWL_STYLE,  style | WS_OVERLAPPED);
            uint32_t flags = SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER |
                SWP_NOOWNERZORDER | SWP_FRAMECHANGED;
            fatal_if_false(SetWindowPos(window(), null, 0, 0, 0, 0, flags));
            enum DWMNCRENDERINGPOLICY ncrp = DWMNCRP_ENABLED;
            fatal_if_not_zero(DwmSetWindowAttribute(window(),
                DWMWA_NCRENDERING_POLICY, &ncrp, sizeof(ncrp)));
        }
        app.is_full_screen = on;
    }
}

static void app_fast_redraw() { SetEvent(app_event_invalidate); } // < 2us

static void app_draw() { UpdateWindow(window()); }

static void app_invalidate_rect(ui_rect_t* r) {
    RECT rc = app_ui2rect(r);
    InvalidateRect(window(), &rc, false);
}

// InvalidateRect() may wait for up to 30 milliseconds
// which is unacceptable for video drawing at monitor
// refresh rate

static void app_redraw_thread(void* unused) {
    (void)unused;
    threads.realtime();
    threads.name("app.redraw");
    for (;;) {
        void* hs[] = { app_event_invalidate, app_event_quit };
        int r = events.wait_any(2, hs);
        if (r == 0) {
            InvalidateRect(window(), null, false);
        } else {
            break;
        }
    }
}

static int32_t app_message_loop() {
    MSG msg = {0};
    while (GetMessage(&msg, null, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessageA(&msg);
    }
    assert(msg.message == WM_QUIT);
    return (int32_t)msg.wParam;
}

static void dispose() {
    app_dispose_fonts();
    if (gdi.clip != null) { DeleteRgn(gdi.clip); }
    fatal_if_false(CloseHandle(app_event_quit));
    fatal_if_false(CloseHandle(app_event_invalidate));
}

static void cursor_set(cursor_t c) {
    // https://docs.microsoft.com/en-us/windows/win32/menurc/using-cursors
    app.cursor = c;
    SetClassLongPtr(window(), GCLP_HCURSOR, (LONG_PTR)c);
    POINT pt = {0};
    if (GetCursorPos(&pt)) { SetCursorPos(pt.x + 1, pt.y); SetCursorPos(pt.x, pt.y); }
}

static void close_window() {
    app_post_message(WM_CLOSE, 0, 0);
}

static void show_tooltip_or_toast(uic_t* ui, int x, int y, double timeout) {
    if (ui != null) {
        toast.x = x;
        toast.y = y;
        if (ui->tag == uic_tag_messagebox) {
            ((uic_messagebox_t*)ui)->option = -1;
        }
        // allow unparented ui for toast and tooltip
        if (ui->init != null) { ui->init(ui); ui->init = null; }
        ui->localize(ui);
        app_animate_start(app_toast_dim, toast_steps);
        toast.ui = ui;
        toast.time = timeout > 0 ? app.now + timeout : 0;
    } else {
        app_toast_cancel();
    }
}

static void app_show_toast(uic_t* ui, double timeout) {
    show_tooltip_or_toast(ui, -1, -1, timeout);
}

static void app_show_tooltip(uic_t* ui, int x, int y, double timeout) {
    if (ui != null) {
        show_tooltip_or_toast(ui, x, y, timeout);
    } else if (toast.ui != null && toast.x >= 0 && toast.y >= 0) {
        app_toast_cancel(); // only cancel tooltips not toasts
    }
}

static void app_formatted_vtoast(double timeout, const char* format, va_list vl) {
    app_show_toast(null, 0);
    static uic_text_t txt;
    uic_text_vinit(&txt, format, vl);
    app_show_toast(&txt.ui, timeout);
}

static void app_formatted_toast(double timeout, const char* format, ...) {
    va_list vl;
    va_start(vl, format);
    app_formatted_vtoast(timeout, format, vl);
    va_end(vl);
}

static void app_enable_sys_command_close(void) {
    EnableMenuItem(GetSystemMenu(GetConsoleWindow(), false),
        SC_CLOSE, MF_BYCOMMAND | MF_ENABLED);
}

static void app_console_attach() {
    if (AttachConsole(ATTACH_PARENT_PROCESS)) {
        // disable SC_CLOSE not to kill firmware update...
        EnableMenuItem(GetSystemMenu(GetConsoleWindow(), false),
            SC_CLOSE, MF_BYCOMMAND | MF_DISABLED | MF_GRAYED);
        (void)freopen("CONOUT$", "w", stdout);
        (void)freopen("CONOUT$", "w", stderr);
        crt.sleep(0.1); // give cmd.exe a chance to print prompt again
        printf("\n");
        atexit(app_enable_sys_command_close);
    }
}

static void app_request_layout() {
    app_layout_dirty = true;
    app.redraw();
}

static void app_show_window(int32_t show) {
    assert(window_visibility.hide <= show &&
           show <= window_visibility.force_min);
    // ShowWindow() does not have documented error reporting
    bool was_visible = ShowWindow(window(), show);
    (void)was_visible;
    if (show == window_visibility.show) {
        SetForegroundWindow(window()); // this does not make it ActiveWindow
        const int SWP = SWP_NOZORDER | SWP_SHOWWINDOW | SWP_NOREPOSITION |
            SWP_NOMOVE;
        SetWindowPos(window(), null, 0, 0, 0, 0, SWP);
        SetFocus(window());
    }
}

static const char* app_open_filename(const char* folder, const char* pairs[], int n) {
    assert(pairs == null && n == 0 ||
           n >= 2 && n % 2 == 0);
    wchar_t mem[32 * 1024];
    wchar_t* filter = mem;
    if (pairs == null && n == 0) {
        filter = L"All Files\0*\0\0";
    } else {
        int left = countof(mem) - 2;
        wchar_t* s = mem;
        for (int i = 0; i < n; i+= 2) {
            wchar_t* s0 = utf8to16(pairs[i + 0]);
            wchar_t* s1 = utf8to16(pairs[i + 1]);
            int n0 = (int)wcslen(s0);
            int n1 = (int)wcslen(s1);
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
    ofn.hwndOwner = (HWND)app.window;
    ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;
    ofn.lpstrFilter = filter;
    ofn.lpstrInitialDir = utf8to16(folder);
    ofn.lpstrFile = path;
    ofn.nMaxFile = sizeof(path);
    static thread_local_storage char text[MAX_PATH];
    if (GetOpenFileNameW(&ofn) && path[0] != 0) {
        strprintf(text, "%s", utf16to8(path));
    } else {
        text[0] = 0;
    }
    return text;
}

static int clipboard_copy_text(const char* s) {
    const int n = (int)strlen(s) + 1;
    int r = OpenClipboard(GetDesktopWindow()) ? 0 : GetLastError();
    if (r != 0) { traceln("OpenClipboard() failed %s", crt.error(r)); }
    if (r == 0) {
        r = EmptyClipboard() ? 0 : GetLastError();
        if (r != 0) { traceln("EmptyClipboard() failed %s", crt.error(r)); }
    }
    void* global = null;
    if (r == 0) {
        global = GlobalAlloc(GMEM_MOVEABLE, n);
        r = global != null ? 0 : GetLastError();
        if (r != 0) { traceln("GlobalAlloc() failed %s", crt.error(r)); }
    }
    if (r == 0) {
        char* d = (char*)GlobalLock(global);
        fatal_if_null(d);
        memcpy(d, s, n);
        r = SetClipboardData(CF_TEXT, global) ? 0 : GetLastError();
        GlobalUnlock(global);
        if (r != 0) {
            traceln("SetClipboardData() failed %s", crt.error(r));
            GlobalFree(global);
        } else {
            // do not free global memory. It's owned by system clipboard now
        }
    }
    if (r == 0) {
        r = CloseClipboard() ? 0 : GetLastError();
        if (r != 0) {
            traceln("CloseClipboard() failed %s", crt.error(r));
        }
    }
    return r;
}

static int clipboard_copy_bitmap(image_t* im) {
    HDC canvas = GetDC(null);
    fatal_if_null(canvas);
    HDC src = CreateCompatibleDC(canvas); fatal_if_null(src);
    HDC dst = CreateCompatibleDC(canvas); fatal_if_null(dst);
    // CreateCompatibleBitmap(dst) will create monochrome bitmap!
    // CreateCompatibleBitmap(canvas) will create display compatible
    HBITMAP bitmap = CreateCompatibleBitmap(canvas, im->w, im->h);
//  HBITMAP bitmap = CreateBitmap(image.w, image.h, 1, 32, null);
    fatal_if_null(bitmap);
    HBITMAP s = SelectBitmap(src, im->bitmap); fatal_if_null(s);
    HBITMAP d = SelectBitmap(dst, bitmap);     fatal_if_null(d);
    POINT pt = { 0 };
    fatal_if_false(SetBrushOrgEx(dst, 0, 0, &pt));
    fatal_if_false(StretchBlt(dst, 0, 0, im->w, im->h, src, 0, 0,
        im->w, im->h, SRCCOPY));
    int r = OpenClipboard(GetDesktopWindow()) ? 0 : GetLastError();
    if (r != 0) { traceln("OpenClipboard() failed %s", crt.error(r)); }
    if (r == 0) {
        r = EmptyClipboard() ? 0 : GetLastError();
        if (r != 0) { traceln("EmptyClipboard() failed %s", crt.error(r)); }
    }
    if (r == 0) {
        r = SetClipboardData(CF_BITMAP, bitmap) ? 0 : GetLastError();
        if (r != 0) {
            traceln("SetClipboardData() failed %s", crt.error(r));
        }
    }
    if (r == 0) {
        r = CloseClipboard() ? 0 : GetLastError();
        if (r != 0) {
            traceln("CloseClipboard() failed %s", crt.error(r));
        }
    }
    fatal_if_null(SelectBitmap(dst, d));
    fatal_if_null(SelectBitmap(src, s));
    fatal_if_false(DeleteBitmap(bitmap));
    fatal_if_false(DeleteDC(dst));
    fatal_if_false(DeleteDC(src));
    fatal_if_false(ReleaseDC(null, canvas));
    return r;
}

const char* app_known_folder(int kf) {
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
    const char* folder = null;
    fatal_if(!(0 <= kf && kf < countof(kfrid)), "invalide kf=%d", kf);
    if (known_foders[kf][0] == 0) {
        wchar_t* path = null;
        fatal_if_not_zero(SHGetKnownFolderPath(kfrid[kf], 0, null, &path));
        strprintf(known_foders[kf], "%s", utf16to8(path));
        CoTaskMemFree(path);
        folder = known_foders[kf];
    }
    return folder;
}

clipboard_t clipboard;

static void clipboard_init() {
    clipboard.copy_text = clipboard_copy_text;
    clipboard.copy_bitmap = clipboard_copy_bitmap;
}

static uic_t app_ui;

static void app_init() {
    app.ui = &app_ui;
    app.redraw = app_fast_redraw;
    app.draw = app_draw;
    app.layout = app_request_layout;
    app.invalidate = app_invalidate_rect;
    app.full_screen = app_full_screen;
    app.set_cursor = cursor_set;
    app.close = close_window;
    app.set_timer = timer_set;
    app.kill_timer = timer_kill;
    app.show_window = app_show_window;
    app.show_toast = app_show_toast;
    app.show_tooltip = app_show_tooltip;
    app.vtoast = app_formatted_vtoast;
    app.toast = app_formatted_toast;
    app.data_save = app_data_save;
    app.data_size = app_data_size;
    app.data_load = app_data_load;
    app.open_filename = app_open_filename;
    app.known_folder = app_known_folder;
    app.attach_console = app_console_attach;
    app_event_quit = events.create();
    app_event_invalidate = events.create();
}

static void __app_windows_init__() {
    fatal_if_not_zero(SetProcessDpiAwareness(PROCESS_PER_MONITOR_DPI_AWARE));
    fatal_if_null(SetThreadDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2));
    InitCommonControls(); // otherwise GetOpenFileName does not work
    app.dpi.process = GetSystemDpiForProcess(GetCurrentProcess());
    app.dpi.system = GetDpiForSystem(); // default was 96DPI
    // monitor dpi will be reinitialized in load_window_pos
    app.dpi.monitor_effective = app.dpi.system;
    app.dpi.monitor_angular = app.dpi.system;
    app.dpi.monitor_raw = app.dpi.system;
    static const RECT nowhere = {0x7FFFFFFF, 0x7FFFFFFF, 0x7FFFFFFF, 0x7FFFFFFF};
    ui_rect_t r = app_rect2ui(&nowhere);
    app_update_mi(&r, MONITOR_DEFAULTTOPRIMARY);
    app.dpi.window = app.dpi.monitor_effective;
    app_init_fonts(app.dpi.window);
}

static int app_win_main() {
    assert(app.init != null && app.class_name != null);
    __app_windows_init__();
    __gdi_init__();
    int r = 0;
    if (app.min_width  <= 0) { app.min_width = 1024; }
    if (app.min_height <= 0) { app.min_height = 768; }
    if (app.max_width  <= 0) { app.max_width = app.work_area.w; }
    if (app.max_height <= 0) { app.max_height = app.work_area.h; }
    int size_frame = GetSystemMetricsForDpi(SM_CXSIZEFRAME, app.dpi.process);
    int caption_height = GetSystemMetricsForDpi(SM_CYCAPTION, app.dpi.process);
    int width = app.min_width + size_frame * 2;
    int height = app.min_height + size_frame * 2 + caption_height;
    ui_rect_t rc = {100, 100, width, height};
    app.last_visibility = window_visibility.defau1t;
    load_window_pos(&rc);
    app_init();
    app.ui->hidden = true; // start with ui hidden
    app.ui->font = &app.fonts.regular;
    app.ui->w = rc.w;
    app.ui->h = rc.h;
    app.init(); // app.init() may change .show
    if (!app.no_ui) {
        app_create_window(rc, width, height);
        thread_t thread = threads.start(app_redraw_thread, null);
        r = app_message_loop();
        fatal_if_false(SetEvent(app_event_quit));
        threads.join(thread);
        dispose();
    } else {
        r = app.main();
    }
    return r;
}

// Simplistic Win32 implementation of national language support.
// Windows NLS family of functions is very complicated and has
// difficult history of LANGID vs LCID etc... See:
// ResolveLocaleName()
// GetThreadLocale()
// SetThreadLocale()
// GetUserDefaultLocaleName()
// WM_SETTINGCHANGE lParam="intl"
// and many others...

enum {
    str_count_max = 1024,
    str_mem_max = 64 * str_count_max
};

static char strings_memory[str_mem_max]; // increase if overflows
static char* strings_free = strings_memory;
static int strings_count;
static const char* ls[str_count_max]; // localized strings
static const char* ns[str_count_max]; // neutral language strings

wchar_t* load_string(int32_t strid, LANGID langid) {
    assert(0 <= strid && strid < countof(ns));
    wchar_t* r = null;
    int32_t block = strid / 16 + 1;
    int32_t index  = strid % 16;
    HRSRC res = FindResourceExA(((HMODULE)null), RT_STRING,
        MAKEINTRESOURCE(block), langid);
//  traceln("FindResourceExA(block=%d langid=%04X)=%p", block, langid, res);
    byte* mem = res == null ? null : (byte*)LoadResource(null, res);
    wchar_t* ws = mem == null ? null : (wchar_t*)LockResource(mem);
//  traceln("LockResource(block=%d langid=%04X)=%p", block, langid, ws);
    if (ws != null) {
        for (int i = 0; i < 16 && r == null; i++) {
            if (ws[0] != 0) {
                int count = (int)ws[0];  // String size in characters.
                ws++;
                assert(ws[count - 1] == 0, "use rc.exe /n command line option");
                if (i == index) { // the string has been found
//                  traceln("%04X found %s", langid, utf16to8(ws));
                    r = ws;
                }
                ws += count;
            } else {
                ws++;
            }
        }
    }
    return r;
}

static const char* save_string(wchar_t* mem) {
    const char* utf8 = utf16to8(mem);
    uintptr_t n = strlen(utf8) + 1;
    assert(n > 1);
    uintptr_t left = countof(strings_memory) - (
        strings_free - strings_memory);
    fatal_if_false(left >= n, "string_memory[] overflow");
    memcpy(strings_free, utf8, n);
    const char* s = strings_free;
    strings_free += n;
    return s;
}

const char* localize_string(int32_t strid) {
    assert(0 <= strid && strid < countof(ns));
    const char* r = null;
    if (0 <= strid && strid < countof(ns)) {
        if (ls[strid] != null) {
            r = ls[strid];
        } else {
            LCID lcid = GetThreadLocale();
            LANGID langid = LANGIDFROMLCID(lcid);
            wchar_t* ws = load_string(strid, langid);
            if (ws == null) { // try default dialect:
                LANGID primary = PRIMARYLANGID(langid);
                langid = MAKELANGID(primary, SUBLANG_NEUTRAL);
                ws = load_string(strid, langid);
            }
            if (ws != null) {
                r = save_string(ws);
                ls[strid] = r;
            }
        }
    }
    return r;
}

static int strid(const char* s) {
    int strid = 0;
    for (int i = 1; i < strings_count && strid == 0; i++) {
        if (ns[i] != null && strcmp(s, ns[i]) == 0) {
            strid = i;
            localize_string(strid); // to save it, ignore result
        }
    }
    return strid;
}

static const char* string(int strid, const char* defau1t) {
    const char* r = localize_string(strid);
    return r == null ? defau1t : r;
}

const char* nls(const char* s) {
    return string(strid(s), s);
}

static const char* locale() {
    wchar_t wln[LOCALE_NAME_MAX_LENGTH + 1];
    LCID lcid = GetThreadLocale();
    int n = LCIDToLocaleName(lcid, wln, countof(wln),
        LOCALE_ALLOW_NEUTRAL_NAMES);
    static char ln[LOCALE_NAME_MAX_LENGTH * 4 + 1];
    ln[0] = 0;
    if (n == 0) {
        // TODO: log error
    } else {
        if (n == 0) {
        } else {
            strprintf(ln, "%s", utf16to8(wln));
        }
    }
    return ln;
}

static void set_locale(const char* locale) {
    wchar_t rln[LOCALE_NAME_MAX_LENGTH + 1];
    int n = ResolveLocaleName(utf8to16(locale), rln, countof(rln));
    if (n == 0) {
        // TODO: log error
    } else {
        LCID lcid = LocaleNameToLCID(rln, LOCALE_ALLOW_NEUTRAL_NAMES);
        if (lcid == 0) {
            // TODO: log error
        } else {
            fatal_if_false(SetThreadLocale(lcid));
            memset((void*)ls, 0, sizeof(ls)); // start all over
        }
    }
}

static void init_ns() {
    LANGID langid = MAKELANGID(LANG_NEUTRAL, SUBLANG_NEUTRAL);
    for (int strid = 0; strid < countof(ns); strid += 16) {
        int32_t block = strid / 16 + 1;
        HRSRC res = FindResourceExA(((HMODULE)null), RT_STRING,
            MAKEINTRESOURCE(block), langid);
        byte* mem = res == null ? null : (byte*)LoadResource(null, res);
        wchar_t* ws = mem == null ? null : (wchar_t*)LockResource(mem);
        if (ws == null) { break; }
        for (int i = 0; i < 16; i++) {
            int ix = strid + i;
            uint16_t count = ws[0];
            if (count > 0) {
                ws++;
                fatal_if_false(ws[count - 1] == 0, "use rc.exe /n");
                ns[ix] = save_string(ws);
                strings_count = ix + 1;
//              traceln("ns[%d] := %d \"%s\"", ix, strlen(ns[ix]), ns[ix]);
                ws += count;
            } else {
                ws++;
            }
        }
    }
}

static void __app_winnls_init__() {
    static_assert(countof(ns) % 16 == 0, "countof(ns) must be multiple of 16");
    static bool ns_initialized;
    if (!ns_initialized) { ns_initialized = true; init_ns(); }
    app.strid = strid;
    app.nls = nls;
    app.string = string;
    app.locale = locale;
    app.set_locale = set_locale;
}

#if !defined(quick_implementation_console)

#pragma warning(disable: 28251) // inconsistent annotations

int WINAPI WinMain(HINSTANCE instance, HINSTANCE previous, char* command,
        int show_command) {
    fatal_if_not_zero(CoInitializeEx(0, COINIT_MULTITHREADED | COINIT_SPEED_OVER_MEMORY));
    __app_winnls_init__();
    clipboard_init();
    app.visibility = show_command;
    (void)command; // ASCII unused
    const char* cl = utf16to8(GetCommandLineW());
    const int len = (int)strlen(cl);
    const int k = ((len + 2) / 2) * (int)sizeof(void*) + (int)sizeof(void*);
    const int n = k + (len + 2) * (int)sizeof(char);
    app.argv = (const char**)alloca(n);
    memset(app.argv, 0, n);
    char* buff = (char*)(((char*)app.argv) + k);
    app.argc = args.parse(cl, app.argv, buff);
    (void)instance; (void)previous; // unused
    return app_win_main();
}

#else

#undef quick_implementation_console

int main(int argc, const char* argv[]) {
    fatal_if_not_zero(CoInitializeEx(0, COINIT_MULTITHREADED | COINIT_SPEED_OVER_MEMORY));
    __app_winnls_init__();
    clipboard_init();
    app.argc = argc;
    app.argv = argv;
    return app.main();
}

#endif quick_implementation_console

end_c

#endif quick_implementation

/* LICENCE

MIT License

Copyright (c) 2021-2022 Dmitry "Leo" Kuznetsov

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.

*/
