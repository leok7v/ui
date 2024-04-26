#ifndef ui_definition
#define ui_definition

// __________________________________ std.h ___________________________________

#include <ctype.h>
#include <errno.h>
#include <malloc.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


#ifdef __cplusplus
    #define begin_c extern "C" {
    #define end_c } // extern "C"
#else
    #define begin_c // C headers compiled as C++
    #define end_c
#endif

#ifndef countof
    #define countof(a) ((int)(sizeof(a) / sizeof((a)[0])))
#endif

#ifndef max // min/max is convoluted story use minimum/maximum
#define max(a,b)     (((a) > (b)) ? (a) : (b))
#endif
#define maximum(a,b) (((a) > (b)) ? (a) : (b)) // preferred

#ifndef min
#define min(a,b)     (((a) < (b)) ? (a) : (b))
#endif
#define minimum(a,b) (((a) < (b)) ? (a) : (b)) // preferred

#if defined(__GNUC__) || defined(__clang__)
    #define force_inline __attribute__((always_inline))
#elif defined(_MSC_VER)
    #define force_inline __forceinline
#endif

#ifndef __cplusplus
    #define null ((void*)0) // better than NULL which is zero
#else
    #define null nullptr
#endif

#if defined(_MSC_VER)
    #define thread_local __declspec(thread)
#else
    #ifndef __cplusplus
        #define thread_local _Thread_local // C99
    #else
        // C++ supports thread_local keyword
    #endif
#endif

// begin_packed end_packed
// usage: typedef begin_packed struct foo_s { ... } end_packed foo_t;

#if defined(__GNUC__) || defined(__clang__)
#define attribute_packed __attribute__((packed))
#define begin_packed
#define end_packed attribute_packed
#else
#define begin_packed __pragma( pack(push, 1) )
#define end_packed __pragma( pack(pop) )
#define attribute_packed
#endif

// In callbacks the formal parameters are
// frequently unused. Also sometimes parameters
// are used in debug configuration only (e.g. assert() checks)
// but not in release.
// C does not have anonymous parameters like C++
// Instead of:
//      void foo(param_type_t param) { (void)param; / *unused */ }
// use:
//      vod foo(param_type_t unused(param)) { }

#define unused(name) _Pragma("warning(suppress:  4100)") name

// Because MS C compiler is unhappy about alloca() and
// does not implement (C99 optional) dynamic arrays on the stack:

#define stackalloc(n) (_Pragma("warning(suppress: 6255 6263)") alloca(n))

begin_c
// __________________________________ core.h __________________________________

#include "ut/std.h"


typedef struct ui_point_s { int32_t x, y; } ui_point_t;
typedef struct ui_rect_s { int32_t x, y, w, h; } ui_rect_t;
typedef uint64_t ui_color_t; // top 2 bits determine color format

typedef struct ui_window_s* ui_window_t;
typedef struct ui_canvas_s* ui_canvas_t;
typedef struct ui_bitmap_s* ui_bitmap_t;
typedef struct ui_font_s*   ui_font_t;
typedef struct ui_brush_s*  ui_brush_t;
typedef struct ui_pen_s*    ui_pen_t;
typedef struct ui_cursor_s* ui_cursor_t;
typedef struct ui_region_s* ui_region_t;

typedef uintptr_t ui_timer_t; // timer not the same as "id" in set_timer()!

typedef struct image_s {
    int32_t w; // width
    int32_t h; // height
    int32_t bpp;    // "components" bytes per pixel
    int32_t stride; // bytes per scanline rounded up to: (w * bpp + 3) & ~3
    ui_bitmap_t bitmap;
    void* pixels;
} image_t;

typedef struct dpi_s { // max(dpi_x, dpi_y)
    int32_t system;  // system dpi
    int32_t process; // process dpi
    // 15" diagonal monitor 3840x2160 175% scaled
    // monitor dpi effective 168, angular 248 raw 284
    int32_t monitor_effective; // effective with regard of user scaling
    int32_t monitor_raw;       // with regard of physical screen size
    int32_t monitor_angular;   // diagonal raw
    int32_t window;            // main window dpi
} ui_dpi_t;

typedef struct ui_fonts_s {
    // font handles re-created on scale change
    ui_font_t regular; // proportional UI font
    ui_font_t mono; // monospaced  UI font
    ui_font_t H1; // bold header font
    ui_font_t H2;
    ui_font_t H3;
} ui_fonts_t;

enum ui_view_type_t {
    ui_view_container  = 'cnt',
    ui_view_messagebox = 'mbx',
    ui_view_button     = 'btn',
    ui_view_checkbox   = 'cbx',
    ui_view_slider     = 'sld',
    ui_view_text       = 'txt',
    ui_view_edit       = 'edt'
};

typedef struct ui_s {
    struct { // window visibility
        int32_t const hide;
        int32_t const normal;   // should be use for first .show()
        int32_t const minimize; // activate and minimize
        int32_t const maximize; // activate and maximize
        int32_t const normal_na;// same as .normal but no activate
        int32_t const show;     // shows and activates in current size and position
        int32_t const min_next; // minimize and activate next window in Z order
        int32_t const min_na;   // minimize but do not activate
        int32_t const show_na;  // same as .show but no activate
        int32_t const restore;  // from min/max to normal window size/pos
        int32_t const defau1t;  // use Windows STARTUPINFO value
        int32_t const force_min;// minimize even if dispatch thread not responding
    } visibility;
    struct { // message:
        int32_t const character; // translated from key pressed/released to utf8
        int32_t const key_pressed;
        int32_t const key_released;
        int32_t const left_button_pressed;
        int32_t const left_button_released;
        int32_t const right_button_pressed;
        int32_t const right_button_released;
        int32_t const mouse_move;
        int32_t const left_double_click;
        int32_t const right_double_click;
        int32_t const animate;
        int32_t const opening;
        int32_t const closing;
        // wp: 0,1,2 (left, middle, right) button index, lp: client x,y
        int32_t const tap;
        int32_t const dtap;
        int32_t const press;
   } message;
   struct { // mouse buttons bitset mask
        struct {
            int32_t const left;
            int32_t const right;
        } button;
    } mouse;
    struct { // virtual keyboard keys
        int32_t const up;
        int32_t const down;
        int32_t const left;
        int32_t const right;
        int32_t const home;
        int32_t const end;
        int32_t const pageup;
        int32_t const pagedw;
        int32_t const insert;
        int32_t const del;
        int32_t const back;
        int32_t const escape;
        int32_t const enter;
        int32_t const plus;
        int32_t const minus;
        int32_t const f1;
        int32_t const f2;
        int32_t const f3;
        int32_t const f4;
        int32_t const f5;
        int32_t const f6;
        int32_t const f7;
        int32_t const f8;
        int32_t const f9;
        int32_t const f10;
        int32_t const f11;
        int32_t const f12;
        int32_t const f13;
        int32_t const f14;
        int32_t const f15;
        int32_t const f16;
        int32_t const f17;
        int32_t const f18;
        int32_t const f19;
        int32_t const f20;
        int32_t const f21;
        int32_t const f22;
        int32_t const f23;
        int32_t const f24;
    } key;
    struct { // known folders:
        int32_t const home     ; // c:\Users\<username>
        int32_t const desktop  ;
        int32_t const documents;
        int32_t const downloads;
        int32_t const music    ;
        int32_t const pictures ;
        int32_t const videos   ;
        int32_t const shared   ; // c:\Users\Public
        int32_t const bin      ; // c:\Program Files
        int32_t const data     ; // c:\ProgramData
    } folder;
} ui_if;

extern ui_if ui;

// _________________________________ colors.h _________________________________

#include "ut/std.h"


/* TODO: make ui_color_t uint64_t RGBA remove pens and brushes
         support upto 16-16-16-15(A)bit per pixel color
         components with 'transparent/hollow' bit
*/

#define color_mask        ((ui_color_t)0xC000000000000000ULL)

#define color_mask        ((ui_color_t)0xC000000000000000ULL)
#define color_undefined   ((ui_color_t)0x8000000000000000ULL)
#define color_transparent ((ui_color_t)0x4000000000000000ULL)
#define color_hdr         ((ui_color_t)0xC000000000000000ULL)

#define color_is_8bit(c)         (((c) & color_mask) == 0)
#define color_is_hdr(c)          (((c) & color_mask) == color_hdr)
#define color_is_undefined(c)    (((c) & color_mask) == color_undefined)
#define color_is_transparent(c) ((((c) & color_mask) == color_transparent) && \
                                 (((c) & ~color_mask) == 0))
// if any other special colors or formats need to be introduced
// (c) & ~color_mask) has 2^62 possible extensions bits

// color_hdr A - 14 bit, R,G,B - 16 bit, all in range [0..0xFFFF]
#define color_hdr_a(c)    ((((c) >> 48) & 0x3FFF) << 2)
#define color_hdr_r(c)    (((c) >>  0) & 0xFFFF)
#define color_hdr_g(c)    (((c) >> 16) & 0xFFFF)
#define color_hdr_b(c)    (((c) >> 32) & 0xFFFF)

#define rgb(r,g,b) ((ui_color_t)(((uint8_t)(r) | ((uint16_t)((uint8_t)(g))<<8)) | \
    (((uint32_t)(uint8_t)(b))<<16)))
#define rgba(r, g, b, a) (ui_color_t)((rgb(r, g, b)) | (((uint8_t)a) << 24))

typedef struct colors_s {
    const int32_t none; // aka CLR_INVALID in wingdi
    const int32_t text;
    const int32_t white;
    const int32_t black;
    const int32_t red;
    const int32_t green;
    const int32_t blue;
    const int32_t yellow;
    const int32_t cyan;
    const int32_t magenta;
    const int32_t gray;
    // darker shades of grey:
    const int32_t dkgray1; // 30 / 255 = 11.7%
    const int32_t dkgray2; // 38 / 255 = 15%
    const int32_t dkgray3; // 45 / 255 = 17.6%
    const int32_t dkgray4; // 63 / 255 = 24.0%
    // tone down RGB colors:
    const int32_t tone_white;
    const int32_t tone_red;
    const int32_t tone_green;
    const int32_t tone_blue;
    const int32_t tone_yellow;
    const int32_t tone_cyan;
    const int32_t tone_magenta;
    // misc:
    const int32_t orange;
    const int32_t dkgreen;
    // highlights:
    const int32_t text_highlight; // bluish off-white
    const int32_t blue_highlight;
    const int32_t off_white;
    // button and other UI colors
    const int32_t btn_gradient_darker;
    const int32_t btn_gradient_dark;
    const int32_t btn_hover_highlight;
    const int32_t btn_disabled;
    const int32_t btn_armed;
    const int32_t btn_text;
    const int32_t toast; // toast background
} colors_t;

extern colors_t colors;

// __________________________________ gdi.h ___________________________________

#include "ut/std.h"


// Graphic Device Interface (selected parts of Windows GDI)

enum {
    gdi_font_quality_default = 0,
    gdi_font_quality_draft = 1,
    gdi_font_quality_proof = 2, // anti-aliased w/o ClearType rainbows
    gdi_font_quality_nonantialiased = 3,
    gdi_font_quality_antialiased = 4,
    gdi_font_quality_cleartype = 5,
    gdi_font_quality_cleartype_natural = 6
};

typedef struct gdi_s {
    ui_brush_t  brush_color;
    ui_brush_t  brush_hollow;
    ui_pen_t pen_hollow;
    ui_region_t clip;
    void (*init)(void);
    uint32_t (*color_rgb)(ui_color_t c); // rgb color
    // bpp bytes (not bits!) per pixel. bpp = -3 or -4 does not swap RGB to BRG:
    void (*image_init)(image_t* image, int32_t w, int32_t h, int32_t bpp,
        const uint8_t* pixels);
    void (*image_init_rgbx)(image_t* image, int32_t w, int32_t h,
        int32_t bpp, const uint8_t* pixels); // sets all alphas to 0xFF
    void (*image_dispose)(image_t* image);
    ui_color_t (*set_text_color)(ui_color_t c);
    ui_brush_t (*create_brush)(ui_color_t c);
    void    (*delete_brush)(ui_brush_t b);
    ui_color_t (*set_brush_color)(ui_color_t c);
    ui_brush_t (*set_brush)(ui_brush_t b); // color or hollow
    ui_pen_t (*set_colored_pen)(ui_color_t c); // always 1px wide
    ui_pen_t (*create_pen)(ui_color_t c, int32_t pixels); // pixels wide pen
    ui_pen_t (*set_pen)(ui_pen_t p);
    void  (*delete_pen)(ui_pen_t p);
    void (*set_clip)(int32_t x, int32_t y, int32_t w, int32_t h);
    // use set_clip(0, 0, 0, 0) to clear clip region
    void (*push)(int32_t x, int32_t y); // also calls SaveDC(app.canvas)
    void (*pop)(void); // also calls RestoreDC(-1, app.canvas)
    void (*pixel)(int32_t x, int32_t y, ui_color_t c);
    ui_point_t (*move_to)(int32_t x, int32_t y); // returns previous (x, y)
    void (*line)(int32_t x, int32_t y); // line to x, y with gdi.pen moves x, y
    void (*frame)(int32_t x, int32_t y, int32_t w, int32_t h); // gdi.pen only
    void (*rect)(int32_t x, int32_t y, int32_t w, int32_t h);  // gdi.pen & brush
    void (*fill)(int32_t x, int32_t y, int32_t w, int32_t h);  // gdi.brush only
    void (*frame_with)(int32_t x, int32_t y, int32_t w, int32_t h, ui_color_t c);
    void (*rect_with)(int32_t x, int32_t y, int32_t w, int32_t h,
                      ui_color_t border, ui_color_t fill);
    void (*fill_with)(int32_t x, int32_t y, int32_t w, int32_t h, ui_color_t c);
    void (*poly)(ui_point_t* points, int32_t count);
    void (*rounded)(int32_t x, int32_t y, int32_t w, int32_t h,
        int32_t rx, int32_t ry); // see RoundRect, pen, brush
    void (*gradient)(int32_t x, int32_t y, int32_t w, int32_t h,
        ui_color_t rgba_from, ui_color_t rgba_to, bool vertical);
    // draw images: (x,y remains untouched after drawing)
    // draw_greyscale() sx, sy, sw, sh screen rectangle
    // x, y, w, h rectangle inside pixels[ih][iw] uint8_t array
    void (*draw_greyscale)(int32_t sx, int32_t sy, int32_t sw, int32_t sh,
        int32_t x, int32_t y, int32_t w, int32_t h,
        int32_t iw, int32_t ih, int32_t stride, const uint8_t* pixels);
    void (*draw_bgr)(int32_t sx, int32_t sy, int32_t sw, int32_t sh,
        int32_t x, int32_t y, int32_t w, int32_t h,
        int32_t iw, int32_t ih, int32_t stride, const uint8_t* pixels);
    void (*draw_bgrx)(int32_t sx, int32_t sy, int32_t sw, int32_t sh,
        int32_t x, int32_t y, int32_t w, int32_t h,
        int32_t iw, int32_t ih, int32_t stride, const uint8_t* pixels);
    void (*alpha_blend)(int32_t x, int32_t y, int32_t w, int32_t h,
        image_t* image, double alpha);
    void (*draw_image)(int32_t x, int32_t y, int32_t w, int32_t h,
        image_t* image);
    // text:
    void (*cleartype)(bool on);
    void (*font_smoothing_contrast)(int32_t c); // [1000..2202] or -1 for 1400 default
    ui_font_t (*font)(ui_font_t f, int32_t height, int32_t quality); // custom font, quality: -1 as is
    void   (*delete_font)(ui_font_t f);
    ui_font_t (*set_font)(ui_font_t f);
    // IMPORTANT: relationship between font_height(), baseline(), descent()
    // E.g. for monospaced font on dpi=96 (monitor_raw=101) and font_height=15
    // the get_em() is: 9 20 font_height(): 15 baseline: 16 descent: 4
    // Monospaced fonts are not `em` "M" square!
    int32_t (*font_height)(ui_font_t f); // font height in pixels
    int32_t (*descent)(ui_font_t f);     // font descent (glyphs below baseline)
    int32_t (*baseline)(ui_font_t f);    // height - baseline (aka ascent) = descent
    bool    (*is_mono)(ui_font_t f);     // is font monospaced?
    // https://en.wikipedia.org/wiki/Em_(typography)
    ui_point_t (*get_em)(ui_font_t f);  // pixel size of glyph "M"
    ui_point_t (*measure_text)(ui_font_t f, const char* format, ...);
    // width can be -1 which measures text with "\n" or
    // positive number of pixels
    ui_point_t (*measure_multiline)(ui_font_t f, int32_t w, const char* format, ...);
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
    // multiline(null, format, ...) only increments gdi.y
    ui_point_t (*multiline)(int32_t width, const char* format, ...);
} gdi_t;

extern gdi_t gdi;

// __________________________________ view.h __________________________________

#include "ut/std.h"


typedef struct ui_view_s ui_view_t;

typedef struct ui_view_s {
    enum ui_view_type_t type;
    void (*init)(ui_view_t* view); // called once before first layout
    ui_view_t** children; // null terminated array[] of children
    double width;    // > 0 width of UI element in "em"s
    char text[2048];
    ui_view_t* parent;
    int32_t x;
    int32_t y;
    int32_t w;
    int32_t h;
    // updated on layout() call
    ui_point_t em; // cached pixel dimensions of "M"
    int32_t shortcut; // keyboard shortcut
    int32_t strid; // 0 for not localized ui
    void* that;  // for the application use
    void (*notify)(ui_view_t* view, void* p); // for the application use
    // two pass layout: measure() .w, .h layout() .x .y
    // first  measure() bottom up - children.layout before parent.layout
    // second layout() top down - parent.layout before children.layout
    void (*measure)(ui_view_t* view); // determine w, h (bottom up)
    void (*layout)(ui_view_t* view); // set x, y possibly adjust w, h (top down)
    void (*paint)(ui_view_t* view);
    bool (*message)(ui_view_t* view, int32_t message, int64_t wp, int64_t lp,
        int64_t* rt); // return true and value in rt to stop processing
    void (*click)(ui_view_t* view); // interpretation depends on ui element
    void (*mouse)(ui_view_t* view, int32_t message, int32_t flags);
    void (*mousewheel)(ui_view_t* view, int32_t dx, int32_t dy); // touchpad scroll
    // tap(ui, button_index) press(ui, button_index) see note below
    // button index 0: left, 1: middle, 2: right
    // bottom up (leaves to root or children to parent)
    // return true if consumed (halts further calls up the tree)
    bool (*tap)(ui_view_t* view, int32_t ix);   // single click/tap inside ui
    bool (*press)(ui_view_t* view, int32_t ix); // two finger click/tap or long press
    void (*context_menu)(ui_view_t* view); // right mouse click or long press
    bool (*set_focus)(ui_view_t* view); // returns true if focus is set
    void (*kill_focus)(ui_view_t* view);
    // translated from key pressed/released to utf8:
    void (*character)(ui_view_t* view, const char* utf8);
    void (*key_pressed)(ui_view_t* view, int32_t key);
    void (*key_released)(ui_view_t* view, int32_t key);
    bool (*is_keyboard_shortcut)(ui_view_t* view, int32_t key);
    void (*hovering)(ui_view_t* view, bool start);
    // timer() every_100ms() and every_sec() called
    // even for hidden and disabled ui elements
    void (*timer)(ui_view_t* view, ui_timer_t id);
    void (*every_100ms)(ui_view_t* view); // ~10 x times per second
    void (*every_sec)(ui_view_t* view); // ~once a second
    bool hidden; // paint() is not called on hidden
    bool armed;
    bool hover;
    bool pressed;   // for ui_button_t and  checkbox_t
    bool disabled;  // mouse, keyboard, key_up/down not called on disabled
    bool focusable; // can be target for keyboard focus
    double  hover_delay; // delta time in seconds before hovered(true)
    double  hover_at;    // time in seconds when to call hovered()
    ui_color_t color;      // interpretation depends on ui element type
    ui_color_t background; // interpretation depends on ui element type
    ui_font_t* font;
    int32_t baseline; // font ascent; descent = height - baseline
    int32_t descent;  // font descent
    char    tip[256]; // tooltip text
} ui_view_t;

// tap() / press() APIs guarantee that single tap() is not coming
// before double tap/click in expense of double click delay (0.5 seconds)
// which is OK for buttons and many other UI controls but absolutely not
// OK for text editing. Thus edit uses raw mouse events to react
// on clicks and double clicks.

void ui_view_init(ui_view_t* view);

typedef struct ui_view_if {
    void (*set_text)(ui_view_t* view, const char* text);
    void (*invalidate)(const ui_view_t* view); // more prone to delays than app.redraw()
    void (*measure)(ui_view_t* view);     // if text[] != "" sets w, h
    bool (*is_hidden)(ui_view_t* view);   // view or any parent is hidden
    bool (*is_disabled)(ui_view_t* view); // view or any parent is disabled
    const char* (*nls)(ui_view_t* view);  // returns localized text
    void (*localize)(ui_view_t* view);    // set strid based ui .text field
    void (*init_children)(ui_view_t* view);
} ui_view_if;

extern ui_view_if ui_view;

#define ui_container(name, ini, ...)                                       \
static ui_view_t* _ ## name ## _ ## children ## _[] = {__VA_ARGS__, null}; \
static ui_view_t name = { .type = ui_view_container, .init = ini,          \
                       .children = (_ ## name ## _ ## children ## _),      \
                       .text = #name                                       \
}

// _________________________________ layout.h _________________________________

#include "ut/std.h"


typedef struct {
    void (*center)(ui_view_t* view); // exactly one child
    void (*horizontal)(ui_view_t* view, int32_t gap);
    void (*vertical)(ui_view_t* view, int32_t gap);
    void (*grid)(ui_view_t* view, int32_t gap_h, int32_t gap_v);
} measurements_if;

extern measurements_if measurements;

typedef struct {
    void (*center)(ui_view_t* view); // exactly one child
    void (*horizontal)(ui_view_t* view, int32_t x, int32_t y, int32_t gap);
    void (*vertical)(ui_view_t* view, int32_t x, int32_t y, int32_t gap);
    void (*grid)(ui_view_t* view, int32_t gap_h, int32_t gap_v);
} layouts_if;

extern layouts_if layouts;

// _________________________________ label.h __________________________________

#include "ut/std.h"


typedef struct ui_label_s {
    ui_view_t view;
    bool multiline;
    bool editable;  // can be edited
    bool highlight; // paint with highlight color
    bool hovered;   // paint highlight rectangle when hover over
    bool label;     // do not copy text to clipboard, do not highlight
    int32_t dy; // vertical shift down (to line up baselines of diff fonts)
} ui_label_t;

void ui_label_init_(ui_view_t* view); // do not call use ui_text() and ui_multiline()

#define ui_text(t, s)                                                        \
    ui_label_t t = { .view = { .type = ui_view_text, .init = ui_label_init_, \
    .children = null, .width = 0.0, .text = s}, .multiline = false}

#define ui_multiline(t, w, s)                                                \
    ui_label_t t = { .view = { .type = ui_view_text, .init = ui_label_init_, \
    .children = null, .width = w, .text = s}, .multiline = true}

// single line of text with "&" keyboard shortcuts:
void ui_label_init_va(ui_label_t* t, const char* format, va_list vl);
void ui_label_init(ui_label_t* t, const char* format, ...);
// multiline
void ui_label_init_ml(ui_label_t* t, double width, const char* format, ...);

// __________________________________ nls.h ___________________________________

#include "ut/std.h"


typedef struct nsl_if { // i18n national language support
    void (*init)(void);
    const char* (*locale)(void);  // "en-US" "zh-CN" etc...
    // force locale for debugging and testing:
    void (*set_locale)(const char* locale); // only for calling thread
    // nls(s) is same as string(strid(s), s)
    const char* (*str)(const char* defau1t); // returns localized string
    // strid("foo") returns 0 if there is no matching ENGLISH NEUTRAL
    // STRINGTABLE entry
    int32_t (*strid)(const char* s);
    // given strid > 0 returns localized string or defau1t value
    const char* (*string)(int32_t strid, const char* defau1t);
} nls_if;

extern nls_if nls;

// _________________________________ button.h _________________________________

#include "ut/std.h"


typedef struct ui_button_s ui_button_t;

typedef struct ui_button_s {
    ui_view_t view;
    void (*cb)(ui_button_t* b); // callback
    double armed_until;   // seconds - when to release
} ui_button_t;

void ui_button_init(ui_button_t* b, const char* label, double ems,
    void (*cb)(ui_button_t* b));

void ui_button_init_(ui_view_t* view); // do not call use ui_button() macro

#define ui_button(name, s, w, code)                                   \
    static void name ## _callback(ui_button_t* name) {                \
        (void)name; /* no warning if unused */                        \
        code                                                          \
    }                                                                 \
    static                                                            \
    ui_button_t name = {                                              \
    .view = { .type = ui_view_button, .init = ui_button_init_,        \
    .children = null, .width = w, .text = s}, .cb = name ## _callback }

// usage:
// ui_button(button, 7.0, "&Button", { b->view.pressed = !b->view.pressed; })

// ________________________________ checkbox.h ________________________________

#include "ut/std.h"


typedef struct ui_checkbox_s  checkbox_t;

typedef struct ui_checkbox_s {
    ui_view_t view;
    void (*cb)( checkbox_t* b); // callback
}  checkbox_t;

// label may contain "___" which will be replaced with "On" / "Off"
void ui_checkbox_init( checkbox_t* b, const char* label, double ems,
    void (*cb)( checkbox_t* b));

void ui_checkbox_init_(ui_view_t* view); // do not call use ui_checkbox() macro

#define ui_checkbox(name, s, w, code)                                 \
    static void name ## _callback(checkbox_t* name) {                 \
        (void)name; /* no warning if unused */                        \
        code                                                          \
    }                                                                 \
    static                                                            \
    checkbox_t name = {                                               \
    .view = { .type = ui_view_checkbox, .init = ui_checkbox_init_,    \
    .children = null, .width = w, .text = s}, .cb = name ## _callback }

// _________________________________ slider.h _________________________________

#include "ut/std.h"


typedef struct ui_slider_s ui_slider_t;

typedef struct ui_slider_s {
    ui_view_t view;
    void (*cb)(ui_slider_t* b); // callback
    int32_t step;
    double time;   // time last button was pressed
    ui_point_t tm; // text measurement (special case for %0*d)
    ui_button_t inc;
    ui_button_t dec;
    ui_view_t* buttons[3]; // = { dec, inc, null }
    int32_t value;  // for ui_slider_t range slider control
    int32_t vmin;
    int32_t vmax;
} ui_slider_t;

void _slider_init_(ui_view_t* view);

void ui_slider_init(ui_slider_t* r, const char* label, double ems,
    int32_t vmin, int32_t vmax, void (*cb)(ui_slider_t* r));

#define ui_slider(name, s, ems, vmn, vmx, code)             \
    static void name ## _callback(ui_slider_t* name) {      \
        (void)name; /* no warning if unused */              \
        code                                                \
    }                                                       \
    static                                                  \
    ui_slider_t name = {                                    \
        .view = { .type = ui_view_slider, .children = null, \
        .width = ems, .text = s, .init = _slider_init_,     \
    }, .vmin = vmn, .vmax = vmx, .value = vmn,              \
    .cb = name ## _callback }

// _______________________________ messagebox.h _______________________________

#include "ut/std.h"


typedef struct ui_messagebox_s ui_messagebox_t;

typedef struct ui_messagebox_s {
    ui_view_t view;
    void (*cb)(ui_messagebox_t* m, int32_t option); // callback -1 on cancel
    ui_label_t text;
    ui_button_t button[16];
    ui_view_t* children[17];
    int32_t option; // -1 or option chosen by user
    const char** opts;
} ui_messagebox_t;

void ui_messagebox_init_(ui_view_t* view);

void ui_messagebox_init(ui_messagebox_t* mx, const char* option[],
    void (*cb)(ui_messagebox_t* m, int32_t option), const char* format, ...);

#define ui_messagebox(name, s, code, ...)                                \
                                                                         \
    static char* name ## _options[] = { __VA_ARGS__, null };             \
                                                                         \
    static void name ## _callback(ui_messagebox_t* m, int32_t option) {  \
        (void)m; (void)option; /* no warnings if unused */               \
        code                                                             \
    }                                                                    \
    static                                                               \
    ui_messagebox_t name = {                                             \
    .view = { .type = ui_view_messagebox, .init = ui_messagebox_init_,   \
    .children = null, .text = s}, .opts = name ## _options,              \
    .cb = name ## _callback }

// __________________________________ app.h ___________________________________

#include "ut/std.h"


// link.exe /SUBSYSTEM:WINDOWS single window application

// every_sec() and every_100ms() also called on all UICs

typedef struct app_s {
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
    // min/max width/height are prefilled according to monitor size
    float wmin; // inches
    float hmin; // inches
    float wmax; // inches
    float hmax; // inches
    // TODO: need wstart/hstart which are between min/max
    int32_t visibility; // initial window_visibility state
    int32_t last_visibility;    // last window_visibility state from last run
    int32_t startup_visibility; // window_visibility from parent process
    bool is_full_screen;
    // ui flags:
    bool no_ui;    // do not create application window at all
    bool no_decor; // window w/o title bar, min/max close buttons
    bool no_min;   // window w/o minimize button on title bar and sys menu
    bool no_max;   // window w/o maximize button on title bar
    bool no_size;  // window w/o maximize button on title bar
    bool no_clip;  // allows to resize window above hosting monitor size
    bool hide_on_minimize; // like task manager minimize means hide
    bool aero;     // retro Windows 7 decoration (just for the fun of it)
    int32_t exit_code; // application exit code
    int32_t tid; // main thread id
    // drawing context:
    ui_dpi_t dpi;
    ui_window_t window;
    ui_rect_t wrc;  // window rectangle including non-client area
    ui_rect_t crc;  // client rectangle
    ui_rect_t mrc;  // monitor rectangle
    ui_rect_t work_area; // current monitor work area
    int32_t width;  // client width
    int32_t height; // client height
    // not to call clock.seconds() too often:
    double now;     // ssb "seconds since boot" updated on each message
    ui_view_t* view;      // show_window() changes ui.hidden
    ui_view_t* focus;   // does not affect message routing - free for all
    ui_fonts_t fonts;
    ui_cursor_t cursor; // current cursor
    ui_cursor_t cursor_arrow;
    ui_cursor_t cursor_wait;
    ui_cursor_t cursor_ibeam;
    // keyboard state now:
    bool alt;
    bool ctrl;
    bool shift;
    ui_point_t mouse; // mouse/touchpad pointer
    ui_canvas_t canvas;  // set by message.paint
    struct { // animation state
        ui_view_t* view;
        int32_t step;
        double time; // closing time or zero
        int32_t x; // (x,y) for tooltip (-1,y) for toast
        int32_t y; // screen coordinates for tooltip
    } animating;
    // inch to pixels and reverse translation via app.dpi.window
    float   (*px2in)(int32_t pixels);
    int32_t (*in2px)(float inches);
    bool    (*point_in_rect)(const ui_point_t* p, const ui_rect_t* r);
    // intersect_rect(null, r0, r1) and intersect_rect(r0, r0, r1) are OK.
    bool    (*intersect_rect)(ui_rect_t* r, const ui_rect_t* r0,
                                            const ui_rect_t* r1);
    bool (*is_active)(void); // is application window active
    bool (*has_focus)(void); // application window has keyboard focus
    void (*activate)(void); // request application window activation
    void (*bring_to_foreground)(void); // not necessary topmost
    void (*make_topmost)(void);   // in foreground hierarchy of windows
    void (*request_focus)(void);  // request application window keyboard focus
    void (*bring_to_front)(void); // activate() + bring_to_foreground() +
                                  // make_topmost() + request_focus()
    // measure and layout:
    void (*measure)(ui_view_t* view); // bottom up measure all children
    void (*layout)(void); // requests layout on UI tree before paint()
    void (*invalidate)(const ui_rect_t* rc);
    void (*full_screen)(bool on);
    void (*redraw)(void); // very fast (5 microseconds) InvalidateRect(null)
    void (*draw)(void);   // UpdateWindow()
    void (*set_cursor)(ui_cursor_t c);
    void (*close)(void); // attempts to close (can_close() permitting)
    // forced quit() even if can_close() returns false
    void (*quit)(int32_t ec);  // app.exit_code = ec; PostQuitMessage(ec);
    ui_timer_t (*set_timer)(uintptr_t id, int32_t milliseconds); // see notes
    void (*kill_timer)(ui_timer_t id);
    void (*post)(int32_t message, int64_t wp, int64_t lp);
    void (*show_window)(int32_t show); // see show_window enum
    void (*show_toast)(ui_view_t* toast, double seconds); // toast(null) to cancel
    void (*show_tooltip)(ui_view_t* tooltip, int32_t x, int32_t y, double seconds);
    void (*toast_va)(double seconds, const char* format, va_list vl);
    void (*toast)(double seconds, const char* format, ...);
    // caret calls must be balanced by caller
    void (*create_caret)(int32_t w, int32_t h);
    void (*show_caret)(void);
    void (*move_caret)(int32_t x, int32_t y);
    void (*hide_caret)(void);
    void (*destroy_caret)(void);
    // registry interface:
    void (*data_save)(const char* name, const void* data, int32_t bytes);
    int32_t (*data_size)(const char* name);
    int32_t (*data_load)(const char* name, void* data, int32_t bytes); // returns bytes read
    // filename dialog:
    // const char* filter[] =
    //     {"Text Files", ".txt;.doc;.ini",
    //      "Executables", ".exe",
    //      "All Files", "*"};
    // const char* fn = app.open_filename("C:\\", filter, countof(filter));
    const char* (*open_filename)(const char* folder, const char* filter[], int32_t n);
    const char* (*known_folder)(int32_t kfid);
    bool (*is_stdout_redirected)(void);
    bool (*is_console_visible)(void);
    int  (*console_attach)(void); // attempts to attach to parent terminal
    int  (*console_create)(void); // allocates new console
    void (*console_show)(bool b);
    // stats:
    int32_t paint_count; // number of paint calls
    double paint_time; // last paint duration in seconds
    double paint_max;  // max of last 128 paint
    double paint_avg;  // EMA of last 128 paints
} app_t;

extern app_t app;


end_c

#endif // ui_definition

#ifdef ui_implementation
// __________________________________ app.c ___________________________________

#include "ut/ut.h"
#include "ut/win32.h"

#pragma push_macro("app_window")
#pragma push_macro("app_canvas")

#define app_window() ((HWND)app.window)
#define app_canvas() ((HDC)app.canvas)

enum { ui_long_press_msec = 250 };

static NONCLIENTMETRICSW app_ncm = { sizeof(NONCLIENTMETRICSW) };
static MONITORINFO app_mi = {sizeof(MONITORINFO)};

static HANDLE app_event_quit;
static HANDLE app_event_invalidate;

static uintptr_t app_timer_1s_id;
static uintptr_t app_timer_100ms_id;

static bool app_layout_dirty; // call layout() before paint

typedef void (*app_animate_function_t)(int32_t step);

static struct {
    app_animate_function_t f;
    int32_t count;
    int32_t step;
    ui_timer_t timer;
} app_animate;

// Animation timer is Windows minimum of 10ms, but in reality the timer
// messages are far from isochronous and more likely to arrive at 16 or
// 32ms intervals and can be delayed.

static void app_on_every_message(ui_view_t* view);

// https://docs.microsoft.com/en-us/windows/win32/inputdev/virtual-key-codes
// https://docs.microsoft.com/en-us/windows/win32/inputdev/wm-keydown

static void app_alt_ctrl_shift(bool down, int32_t key) {
    if (key == VK_MENU)    { app.alt   = down; }
    if (key == VK_CONTROL) { app.ctrl  = down; }
    if (key == VK_SHIFT)   { app.shift = down; }
}

static inline ui_point_t app_point2ui(const POINT* p) {
    ui_point_t u = { p->x, p->y };
    return u;
}

static inline POINT app_ui2point(const ui_point_t* u) {
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

static void app_update_ncm(int32_t dpi) {
    // Only UTF-16 version supported SystemParametersInfoForDpi
    fatal_if_false(SystemParametersInfoForDpi(SPI_GETNONCLIENTMETRICS,
        sizeof(app_ncm), &app_ncm, 0, dpi));
}

static void app_update_monitor_dpi(HMONITOR monitor, ui_dpi_t* dpi) {
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
            threads.sleep_for(1.0 / 32); // and retry:
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
                    dpi->monitor_effective = max(dpi_x, dpi_y); break;
                case MDT_ANGULAR_DPI:
                    dpi->monitor_angular = max(dpi_x, dpi_y); break;
                case MDT_RAW_DPI:
                    dpi->monitor_raw = max(dpi_x, dpi_y); break;
                default: assert(false);
            }
        }
    }
}

#ifndef QUICK_DEBUG

static void app_dump_dpi(void) {
    traceln("app.dpi.monitor_effective: %d", app.dpi.monitor_effective  );
    traceln("app.dpi.monitor_angular  : %d", app.dpi.monitor_angular    );
    traceln("app.dpi.monitor_raw      : %d", app.dpi.monitor_raw        );
    traceln("app.dpi.window           : %d", app.dpi.window             );
    traceln("app.dpi.system           : %d", app.dpi.system             );
    traceln("app.dpi.process          : %d", app.dpi.process            );

    traceln("app.mrc      : %d,%d %dx%d", app.mrc.x, app.mrc.y, app.mrc.w, app.mrc.h);
    traceln("app.wrc      : %d,%d %dx%d", app.wrc.x, app.wrc.y, app.wrc.w, app.wrc.h);
    traceln("app.crc      : %d,%d %dx%d", app.crc.x, app.crc.y, app.crc.w, app.crc.h);
    traceln("app.work_area: %d,%d %dx%d", app.work_area.x, app.work_area.y, app.work_area.w, app.work_area.h);

    int32_t mxt_x = GetSystemMetrics(SM_CXMAXTRACK);
    int32_t mxt_y = GetSystemMetrics(SM_CYMAXTRACK);
    traceln("MAXTRACK: %d, %d", mxt_x, mxt_y);
    int32_t scr_x = GetSystemMetrics(SM_CXSCREEN);
    int32_t scr_y = GetSystemMetrics(SM_CYSCREEN);
    float monitor_x = scr_x / (float)app.dpi.monitor_raw;
    float monitor_y = scr_y / (float)app.dpi.monitor_raw;
    traceln("SCREEN: %d, %d %.1fx%.1f\"", scr_x, scr_y, monitor_x, monitor_y);
}

#endif

static bool app_update_mi(const ui_rect_t* r, uint32_t flags) {
    RECT rc = app_ui2rect(r);
    HMONITOR monitor = MonitorFromRect(&rc, flags);
//  TODO: moving between monitors with different DPIs
//  HMONITOR mw = MonitorFromWindow(app_window(), flags);
    if (monitor != null) {
        app_update_monitor_dpi(monitor, &app.dpi);
        fatal_if_false(GetMonitorInfoA(monitor, &app_mi));
        app.work_area = app_rect2ui(&app_mi.rcWork);
        app.mrc = app_rect2ui(&app_mi.rcMonitor);
//      app_dump_dpi();
    }
    return monitor != null;
}

static void app_update_crc(void) {
    RECT rc = {0};
    fatal_if_false(GetClientRect(app_window(), &rc));
    app.crc = app_rect2ui(&rc);
    app.width = app.crc.w;
    app.height = app.crc.h;
}

static void app_dispose_fonts(void) {
    fatal_if_false(DeleteFont(app.fonts.regular));
    fatal_if_false(DeleteFont(app.fonts.H1));
    fatal_if_false(DeleteFont(app.fonts.H2));
    fatal_if_false(DeleteFont(app.fonts.H3));
    fatal_if_false(DeleteFont(app.fonts.mono));
}

static void app_init_fonts(int32_t dpi) {
    app_update_ncm(dpi);
    if (app.fonts.regular != null) { app_dispose_fonts(); }
    LOGFONTW lf = app_ncm.lfMessageFont;
    // lf.lfQuality is CLEARTYPE_QUALITY which looks bad on 4K monitors
    // Windows UI uses PROOF_QUALITY which is aliased w/o ClearType rainbows
    lf.lfQuality = PROOF_QUALITY;
    app.fonts.regular = (ui_font_t)CreateFontIndirectW(&lf);
    not_null(app.fonts.regular);
    const double fh = app_ncm.lfMessageFont.lfHeight;
//  traceln("lfHeight=%.1f", fh);
    assert(fh != 0);
    lf.lfWeight = FW_SEMIBOLD;
    lf.lfHeight = (int)(fh * 1.75);
    app.fonts.H1 = (ui_font_t)CreateFontIndirectW(&lf);
    lf.lfWeight = FW_SEMIBOLD;
    lf.lfHeight = (int)(fh * 1.4);
    app.fonts.H2 = (ui_font_t)CreateFontIndirectW(&lf);
    lf.lfWeight = FW_SEMIBOLD;
    lf.lfHeight = (int)(fh * 1.15);
    app.fonts.H3 = (ui_font_t)CreateFontIndirectW(&lf);
    lf = app_ncm.lfMessageFont;
    lf.lfPitchAndFamily = FIXED_PITCH;
    #define monospaced "Cascadia Code"
    wcscpy(lf.lfFaceName, L"Cascadia Code");
    app.fonts.mono = (ui_font_t)CreateFontIndirectW(&lf);
    app.cursor_arrow = (ui_cursor_t)LoadCursorA(null, IDC_ARROW);
    app.cursor_wait  = (ui_cursor_t)LoadCursorA(null, IDC_WAIT);
    app.cursor_ibeam = (ui_cursor_t)LoadCursorA(null, IDC_IBEAM);
    app.cursor = app.cursor_arrow;
}

static void app_data_save(const char* name, const void* data, int32_t bytes) {
    config.save(app.class_name, name, data, bytes);
}

static int32_t app_data_size(const char* name) {
    return config.size(app.class_name, name);
}

static int32_t app_data_load(const char* name, void* data, int32_t bytes) {
    return config.load(app.class_name, name, data, bytes);
}

typedef begin_packed struct app_wiw_s { // "where is window"
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
} end_packed app_wiw_t;

static BOOL CALLBACK app_monitor_enum_proc(HMONITOR monitor,
        HDC unused(hdc), RECT* rc1, LPARAM that) {
    app_wiw_t* wiw = (app_wiw_t*)(uintptr_t)that;
    ui_rect_t* space = &wiw->space;
    MONITORINFOEX mi = { .cbSize = sizeof(MONITORINFOEX) };
    fatal_if_false(GetMonitorInfoA(monitor, (MONITORINFO*)&mi));
    space->x = min(space->x, min(mi.rcMonitor.left, mi.rcMonitor.right));
    space->y = min(space->y, min(mi.rcMonitor.top,  mi.rcMonitor.bottom));
    space->w = max(space->w, max(mi.rcMonitor.left, mi.rcMonitor.right));
    space->h = max(space->h, max(mi.rcMonitor.top,  mi.rcMonitor.bottom));
    return true; // keep going
}

static void app_enum_monitors(app_wiw_t* wiw) {
    EnumDisplayMonitors(null, null, app_monitor_enum_proc, (uintptr_t)wiw);
    // because app_monitor_enum_proc() puts max into w,h:
    wiw->space.w -= wiw->space.x;
    wiw->space.h -= wiw->space.y;
}

static void app_save_window_pos(ui_window_t wnd, const char* name, bool dump) {
    RECT wr = {0};
    fatal_if_false(GetWindowRect((HWND)wnd, &wr));
    ui_rect_t wrc = app_rect2ui(&wr);
    app_update_mi(&wrc, MONITOR_DEFAULTTONEAREST);
    WINDOWPLACEMENT wpl = { .length = sizeof(wpl) };
    fatal_if_false(GetWindowPlacement((HWND)wnd, &wpl));
    // note the replacement of wpl.rcNormalPosition with wrc:
    app_wiw_t wiw = { // where is window
        .bytes = sizeof(app_wiw_t),
        .placement = wrc,
        .mrc = app.mrc,
        .work_area = app.work_area,
        .min_position = app_point2ui(&wpl.ptMinPosition),
        .max_position = app_point2ui(&wpl.ptMaxPosition),
        .max_track = {
            .x = GetSystemMetrics(SM_CXMAXTRACK),
            .y = GetSystemMetrics(SM_CYMAXTRACK)
        },
        .dpi = app.dpi.monitor_raw,
        .flags = wpl.flags,
        .show = wpl.showCmd
    };
    app_enum_monitors(&wiw);
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
    config.save(app.class_name, name, &wiw, sizeof(wiw));
    app_update_mi(&app.wrc, MONITOR_DEFAULTTONEAREST);
}

static void app_save_console_pos(void) {
    HWND cw = GetConsoleWindow();
    if (cw != null) {
        app_save_window_pos((ui_window_t)cw, "wic", false);
        HANDLE console = GetStdHandle(STD_OUTPUT_HANDLE);
        CONSOLE_SCREEN_BUFFER_INFOEX info = { sizeof(CONSOLE_SCREEN_BUFFER_INFOEX) };
        int32_t r = GetConsoleScreenBufferInfoEx(console, &info) ? 0 : runtime.err();
        if (r != 0) {
            traceln("GetConsoleScreenBufferInfoEx() %s", str.error(r));
        } else {
            config.save(app.class_name, "console_screen_buffer_infoex",
                            &info, (int)sizeof(info));
//          traceln("info: %dx%d", info.dwSize.X, info.dwSize.Y);
//          traceln("%d,%d %dx%d", info.srWindow.Left, info.srWindow.Top,
//              info.srWindow.Right - info.srWindow.Left,
//              info.srWindow.Bottom - info.srWindow.Top);
        }
    }
    int32_t v = app.is_console_visible();
    // "icv" "is console visible"
    config.save(app.class_name, "icv", &v, (int)sizeof(v));
}

static bool app_point_in_rect(const ui_point_t* p, const ui_rect_t* r) {
    return r->x <= p->x && p->x < r->x + r->w &&
           r->y <= p->y && p->y < r->y + r->h;
}

static bool app_intersect_rect(ui_rect_t* i, const ui_rect_t* r0,
                               const ui_rect_t* r1) {
    ui_rect_t r = {0};
    r.x = max(r0->x, r1->x);  // Maximum of left edges
    r.y = max(r0->y, r1->y);  // Maximum of top edges
    r.w = min(r0->x + r0->w, r1->x + r1->w) - r.x;  // Width of overlap
    r.h = min(r0->y + r0->h, r1->y + r1->h) - r.y;  // Height of overlap
    bool b = r.w > 0 && r.h > 0;
    if (!b) {
        r.w = 0;
        r.h = 0;
    }
    if (i != null) { *i = r; }
    return b;
}

static bool app_is_fully_inside(const ui_rect_t* inner,
                                const ui_rect_t* outer) {
    return
        outer->x <= inner->x && inner->x + inner->w <= outer->x + outer->w &&
        outer->y <= inner->y && inner->y + inner->h <= outer->y + outer->h;
}

static void app_bring_window_inside_monitor(const ui_rect_t* mrc, ui_rect_t* wrc) {
    assert(mrc->w > 0 && mrc->h > 0);
    // Check if window rect is inside monitor rect
    if (!app_is_fully_inside(wrc, mrc)) {
        // Move window into monitor rect
        wrc->x = max(mrc->x, min(mrc->x + mrc->w - wrc->w, wrc->x));
        wrc->y = max(mrc->y, min(mrc->y + mrc->h - wrc->h, wrc->y));
        // Adjust size to fit into monitor rect
        wrc->w = min(wrc->w, mrc->w);
        wrc->h = min(wrc->h, mrc->h);
    }
}

static bool app_load_window_pos(ui_rect_t* rect, int32_t *visibility) {
    app_wiw_t wiw = {0}; // where is window
    bool loaded = config.load(app.class_name, "wiw", &wiw, sizeof(wiw)) ==
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
        app_update_mi(&wiw.placement, MONITOR_DEFAULTTONEAREST);
        bool same_monitor = memcmp(&wiw.mrc, &app.mrc, sizeof(wiw.mrc)) == 0;
//      traceln("%d,%d %dx%d", p->x, p->y, p->w, p->h);
        if (same_monitor) {
            *rect = *p;
        } else { // moving to another monitor
            rect->x = (p->x - wiw.mrc.x) * app.mrc.w / wiw.mrc.w;
            rect->y = (p->y - wiw.mrc.y) * app.mrc.h / wiw.mrc.h;
            // adjust according to monitors DPI difference:
            // (w, h) theoretically could be as large as 0xFFFF
            const int64_t w = (int64_t)p->w * app.dpi.monitor_raw;
            const int64_t h = (int64_t)p->h * app.dpi.monitor_raw;
            rect->w = (int32_t)(w / wiw.dpi);
            rect->h = (int32_t)(h / wiw.dpi);
        }
        *visibility = wiw.show;
    }
//  traceln("%d,%d %dx%d show=%d", rect->x, rect->y, rect->w, rect->h, *visibility);
    app_bring_window_inside_monitor(&app.mrc, rect);
//  traceln("%d,%d %dx%d show=%d", rect->x, rect->y, rect->w, rect->h, *visibility);
    return loaded;
}

static bool app_load_console_pos(ui_rect_t* rect, int32_t *visibility) {
    app_wiw_t wiw = {0}; // where is window
    *visibility = 0; // boolean
    bool loaded = config.load(app.class_name, "wic", &wiw, sizeof(wiw)) ==
                                sizeof(wiw);
    if (loaded) {
        ui_rect_t* p = &wiw.placement;
        app_update_mi(&wiw.placement, MONITOR_DEFAULTTONEAREST);
        bool same_monitor = memcmp(&wiw.mrc, &app.mrc, sizeof(wiw.mrc)) == 0;
//      traceln("%d,%d %dx%d", p->x, p->y, p->w, p->h);
        if (same_monitor) {
            *rect = *p;
        } else { // moving to another monitor
            rect->x = (p->x - wiw.mrc.x) * app.mrc.w / wiw.mrc.w;
            rect->y = (p->y - wiw.mrc.y) * app.mrc.h / wiw.mrc.h;
            // adjust according to monitors DPI difference:
            // (w, h) theoretically could be as large as 0xFFFF
            const int64_t w = (int64_t)p->w * app.dpi.monitor_raw;
            const int64_t h = (int64_t)p->h * app.dpi.monitor_raw;
            rect->w = (int32_t)(w / wiw.dpi);
            rect->h = (int32_t)(h / wiw.dpi);
        }
        *visibility = wiw.show != 0;
        app_update_mi(&app.wrc, MONITOR_DEFAULTTONEAREST);
    }
    return loaded;
}

static void app_timer_kill(ui_timer_t timer) {
    fatal_if_false(KillTimer(app_window(), timer));
}

static ui_timer_t app_timer_set(uintptr_t id, int32_t ms) {
    not_null(app_window());
    assert(10 <= ms && ms < 0x7FFFFFFF);
    ui_timer_t tid = (ui_timer_t)SetTimer(app_window(), id, (uint32_t)ms, null);
    fatal_if(tid == 0);
    assert(tid == id);
    return tid;
}

static void app_set_parents(ui_view_t* view) {
    for (ui_view_t** c = view->children; c != null && *c != null; c++) {
        if ((*c)->parent == null) {
            (*c)->parent = view;
            app_set_parents(*c);
        } else {
            assert((*c)->parent == view, "no reparenting");
        }
    }
}

static void app_init_children(ui_view_t* view) {
    for (ui_view_t** c = view->children; c != null && *c != null; c++) {
        if ((*c)->init != null) { (*c)->init(*c); (*c)->init = null; }
        if ((*c)->font == null) { (*c)->font = &app.fonts.regular; }
        if ((*c)->em.x == 0 || (*c)->em.y == 0) {
            (*c)->em = gdi.get_em(*view->font);
        }
        if ((*c)->text[0] != 0) { ui_view.localize(*c); }
        app_init_children(*c);
    }
}

static void app_post_message(int32_t m, int64_t wp, int64_t lp) {
    fatal_if_false(PostMessageA(app_window(), m, wp, lp));
}

static void app_timer(ui_view_t* view, ui_timer_t id) {
    if (view->timer != null) {
        view->timer(view, id);
    }
    if (id == app_timer_1s_id && view->every_sec != null) {
        view->every_sec(view);
    }
    if (id == app_timer_100ms_id && view->every_100ms != null) {
        view->every_100ms(view);
    }
    ui_view_t** c = view->children;
    while (c != null && *c != null) { app_timer(*c, id); c++; }
}

static void app_every_100ms(ui_timer_t id) {
    if (id == app_timer_1s_id && app.every_sec != null) {
        app.every_sec();
    }
    if (id == app_timer_100ms_id && app.every_100ms != null) {
        app.every_100ms();
    }
    if (app.animating.time != 0 && app.now > app.animating.time) {
        app.show_toast(null, 0);
    }
}

static void app_animate_timer(void) {
    app_post_message(ui.message.animate, (uint64_t)app_animate.step + 1,
        (uintptr_t)app_animate.f);
}

static void app_wm_timer(ui_timer_t id) {
    app_every_100ms(id);
    if (app_animate.timer == id) { app_animate_timer(); }
    app_timer(app.view, id);
}

static void app_window_dpi(void) {
    int32_t dpi = GetDpiForWindow(app_window());
    if (dpi == 0) { dpi = GetDpiForWindow(GetParent(app_window())); }
    if (dpi == 0) { dpi = GetDpiForWindow(GetDesktopWindow()); }
    if (dpi == 0) { dpi = GetSystemDpiForProcess(GetCurrentProcess()); }
    if (dpi == 0) { dpi = GetDpiForSystem(); }
    app.dpi.window = dpi;
}

static void app_window_opening(void) {
    app_window_dpi();
    app_init_fonts(app.dpi.window);
    app_timer_1s_id = app.set_timer((uintptr_t)&app_timer_1s_id, 1000);
    app_timer_100ms_id = app.set_timer((uintptr_t)&app_timer_100ms_id, 100);
    app.set_cursor(app.cursor_arrow);
    app.canvas = (ui_canvas_t)GetDC(app_window());
    not_null(app.canvas);
    if (app.opened != null) { app.opened(); }
    app.view->em = gdi.get_em(*app.view->font);
    app_set_parents(app.view);
    ui_view.init_children(app.view);
    app_wm_timer(app_timer_100ms_id);
    app_wm_timer(app_timer_1s_id);
    fatal_if(ReleaseDC(app_window(), app_canvas()) == 0);
    app.canvas = null;
    app.layout(); // request layout
    if (app.last_visibility == ui.visibility.maximize) {
        ShowWindow(app_window(), ui.visibility.maximize);
    }
//  app_dump_dpi();
//  if (forced_locale != 0) {
//      SendMessageTimeoutA(HWND_BROADCAST, WM_SETTINGCHANGE, 0, (uintptr_t)"intl", 0, 1000, null);
//  }
}

static void app_window_closing(void) {
    if (app.can_close == null || app.can_close()) {
        if (app.is_full_screen) { app.full_screen(false); }
        app.kill_timer(app_timer_1s_id);
        app.kill_timer(app_timer_100ms_id);
        app_timer_1s_id = 0;
        app_timer_100ms_id = 0;
        if (app.closed != null) { app.closed(); }
        app_save_window_pos(app.window, "wiw", false);
        app_save_console_pos();
        DestroyWindow(app_window());
        app.window = null;
    }
}

static void app_get_min_max_info(MINMAXINFO* mmi) {
    if (app.wmax != 0 && app.hmax != 0) {
        // if wmax hmax are set then wmin hmin must be less than wmax hmax
        assert(app.wmin <= app.wmax && app.hmin <= app.hmax,
            "app.wmax=%d app.hmax=%d app.wmin=%d app.hmin=%d",
             app.wmax, app.hmax, app.wmin, app.hmin);
    }
    const ui_rect_t* wa = &app.work_area;
    const int32_t wmin = app.wmin > 0 ? app.in2px(app.wmin) : wa->w / 2;
    const int32_t hmin = app.hmin > 0 ? app.in2px(app.hmin) : wa->h / 2;
    mmi->ptMinTrackSize.x = wmin;
    mmi->ptMinTrackSize.y = hmin;
    const int32_t wmax = app.wmax > 0 ? app.in2px(app.wmax) : wa->w;
    const int32_t hmax = app.hmax > 0 ? app.in2px(app.hmax) : wa->h;
    if (app.no_clip) {
        mmi->ptMaxTrackSize.x = wmax;
        mmi->ptMaxTrackSize.y = hmax;
    } else {
        // clip wmax and hmax to monitor work area
        mmi->ptMaxTrackSize.x = min(wmax, wa->w);
        mmi->ptMaxTrackSize.y = min(hmax, wa->h);
    }
    mmi->ptMaxSize.x = mmi->ptMaxTrackSize.x;
    mmi->ptMaxSize.y = mmi->ptMaxTrackSize.y;
}

static void app_key_pressed(ui_view_t* view, int32_t p) {
    if (!ui_view.is_hidden(view) && !ui_view.is_disabled(view)) {
        if (view->key_pressed != null) { view->key_pressed(view, p); }
        ui_view_t** c = view->children;
        while (c != null && *c != null) { app_key_pressed(*c, p); c++; }
    }
}

static void app_key_released(ui_view_t* view, int32_t p) {
    if (!ui_view.is_hidden(view) && !ui_view.is_disabled(view)) {
        if (view->key_released != null) { view->key_released(view, p); }
        ui_view_t** c = view->children;
        while (c != null && *c != null) { app_key_released(*c, p); c++; }
    }
}

static void app_character(ui_view_t* view, const char* utf8) {
    if (!ui_view.is_hidden(view) && !ui_view.is_disabled(view)) {
        if (view->character != null) { view->character(view, utf8); }
        ui_view_t** c = view->children;
        while (c != null && *c != null) { app_character(*c, utf8); c++; }
    }
}

static void app_paint(ui_view_t* view) {
    if (!view->hidden && app.crc.w > 0 && app.crc.h > 0) {
        if (view->paint != null) { view->paint(view); }
        ui_view_t** c = view->children;
        while (c != null && *c != null) { app_paint(*c); c++; }
    }
}

static bool app_set_focus(ui_view_t* view) {
    bool set = false;
    assert(GetActiveWindow() == app_window());
    ui_view_t** c = view->children;
    while (c != null && *c != null && !set) { set = app_set_focus(*c); c++; }
    if (view->focusable && view->set_focus != null &&
       (app.focus == view || app.focus == null)) {
        set = view->set_focus(view);
    }
    return set;
}

static void app_kill_focus(ui_view_t* view) {
    ui_view_t** c = view->children;
    while (c != null && *c != null) { app_kill_focus(*c); c++; }
    if (view->set_focus != null && view->focusable) {
        view->kill_focus(view);
    }
}

static void app_mousewheel(ui_view_t* view, int32_t dx, int32_t dy) {
    if (!ui_view.is_hidden(view) && !ui_view.is_disabled(view)) {
        if (view->mousewheel != null) { view->mousewheel(view, dx, dy); }
        ui_view_t** c = view->children;
        while (c != null && *c != null) { app_mousewheel(*c, dx, dy); c++; }
    }
}

static void app_measure_children(ui_view_t* view) {
    if (!view->hidden && app.crc.w > 0 && app.crc.h > 0) {
        ui_view_t** c = view->children;
        while (c != null && *c != null) { app_measure_children(*c); c++; }
        if (view->measure != null) { view->measure(view); }
    }
}

static void app_layout_children(ui_view_t* view) {
    if (!view->hidden && app.crc.w > 0 && app.crc.h > 0) {
        if (view->layout != null) { view->layout(view); }
        ui_view_t** c = view->children;
        while (c != null && *c != null) { app_layout_children(*c); c++; }
    }
}

static void app_layout_ui(ui_view_t* view) {
    app_measure_children(view);
    app_layout_children(view);
}

static bool app_message(ui_view_t* view, int32_t m, int64_t wp, int64_t lp,
        int64_t* ret) {
    // message() callback is called even for hidden and disabled ui-elements
    // consider timers, and other useful messages
    app_on_every_message(view);
    if (view->message != null) {
        if (view->message(view, m, wp, lp, ret)) { return true; }
    }
    ui_view_t** c = view->children;
    while (c != null && *c != null) {
        if (app_message(*c, m, wp, lp, ret)) { return true; }
        c++;
    }
    return false;
}

static void app_kill_hidden_focus(ui_view_t* view) {
    // removes focus from hidden or disabled ui controls
    if (app.focus == view && (view->disabled || view->hidden)) {
        app.focus = null;
    } else {
        ui_view_t** c = view->children;
        while (c != null && *c != null) {
            app_kill_hidden_focus(*c);
            c++;
        }
    }
}

static void app_toast_mouse(int32_t m, int32_t f);
static void app_toast_character(const char* utf8);

static void app_wm_char(ui_view_t* view, const char* utf8) {
    if (app.animating.view != null) {
        app_toast_character(utf8);
    } else {
        app_character(view, utf8);
    }
}

static void app_hover_changed(ui_view_t* view) {
    if (view->hovering != null && !view->hidden) {
        if (!view->hover) {
            view->hover_at = 0;
            view->hovering(view, false); // cancel hover
        } else {
            assert(view->hover_delay >= 0);
            if (view->hover_delay == 0) {
                view->hover_at = -1;
                view->hovering(view, true); // call immediately
            } else if (view->hover_delay != 0 && view->hover_at >= 0) {
                view->hover_at = app.now + view->hover_delay;
            }
        }
    }
}

// app_on_every_message() is called on every message including timers
// allowing ui elements to do scheduled actions like e.g. hovering()

static void app_on_every_message(ui_view_t* view) {
    if (view->hovering != null && !view->hidden) {
        if (view->hover_at > 0 && app.now > view->hover_at) {
            view->hover_at = -1; // "already called"
            view->hovering(view, true);
        }
    }
}

static void app_ui_mouse(ui_view_t* view, int32_t m, int32_t f) {
    if (!ui_view.is_hidden(view) &&
       (m == WM_MOUSEHOVER || m == ui.message.mouse_move)) {
        RECT rc = { view->x, view->y, view->x + view->w, view->y + view->h};
        bool hover = view->hover;
        POINT pt = app_ui2point(&app.mouse);
        view->hover = PtInRect(&rc, pt);
        InflateRect(&rc, view->w / 4, view->h / 4);
        ui_rect_t r = app_rect2ui(&rc);
        if (hover != view->hover) { app.invalidate(&r); }
        if (hover != view->hover && view->hovering != null) {
            app_hover_changed(view);
        }
    }
    if (!ui_view.is_hidden(view) && !ui_view.is_disabled(view)) {
        if (view->mouse != null) { view->mouse(view, m, f); }
        for (ui_view_t** c = view->children; c != null && *c != null; c++) {
            app_ui_mouse(*c, m, f);
        }
    }
}

static bool app_context_menu(ui_view_t* view) {
    if (!ui_view.is_hidden(view) && !ui_view.is_disabled(view)) {
        for (ui_view_t** c = view->children; c != null && *c != null; c++) {
            if (app_context_menu(*c)) { return true; }
        }
        RECT rc = { view->x, view->y, view->x + view->w, view->y + view->h};
        POINT pt = app_ui2point(&app.mouse);
        if (PtInRect(&rc, pt)) {
            if (!view->hidden && !view->disabled && view->context_menu != null) {
                view->context_menu(view);
            }
        }
    }
    return false;
}

static bool app_inside(ui_view_t* view) {
    const int32_t x = app.mouse.x - view->x;
    const int32_t y = app.mouse.y - view->y;
    return 0 <= x && x < view->w && 0 <= y && y < view->h;
}

static bool app_tap(ui_view_t* view, int32_t ix) { // 0: left 1: middle 2: right
    bool done = false; // consumed
    if (!ui_view.is_hidden(view) && !ui_view.is_disabled(view) && app_inside(view)) {
        for (ui_view_t** c = view->children; c != null && *c != null && !done; c++) {
            done = app_tap(*c, ix);
        }
        if (view->tap != null && !done) { done = view->tap(view, ix); }
    }
    return done;
}

static bool app_press(ui_view_t* view, int32_t ix) { // 0: left 1: middle 2: right
    bool done = false; // consumed
    if (!ui_view.is_hidden(view) && !ui_view.is_disabled(view)) {
        for (ui_view_t** c = view->children; c != null && *c != null && !done; c++) {
            done = app_press(*c, ix);
        }
        if (view->press != null && !done) { done = view->press(view, ix); }
    }
    return done;
}

static void app_mouse(ui_view_t* view, int32_t m, int32_t f) {
    if (app.animating.view != null && app.animating.view->mouse != null) {
        app_ui_mouse(app.animating.view, m, f);
    } else if (app.animating.view != null && app.animating.view->mouse == null) {
        app_toast_mouse(m, f);
        bool tooltip = app.animating.x >= 0 && app.animating.y >= 0;
        if (tooltip) { app_ui_mouse(view, m, f); }
    } else {
        app_ui_mouse(view, m, f);
    }
}

static void app_tap_press(int32_t m, WPARAM wp, LPARAM lp) {
    app.mouse.x = GET_X_LPARAM(lp);
    app.mouse.y = GET_Y_LPARAM(lp);
    // dispatch as generic mouse message:
    app_mouse(app.view, (int32_t)m, (int32_t)wp);
    int32_t ix = (int32_t)wp;
    assert(0 <= ix && ix <= 2);
    // for now long press and double tap/double click
    // treated as press() call - can be separated if desired:
    if (m == ui.message.tap) {
        app_tap(app.view, ix);
    } else if (m == ui.message.dtap) {
        app_press(app.view, ix);
    } else if (m == ui.message.press) {
        app_press(app.view, ix);
    } else {
        assert(false, "unexpected message: 0x%04X", m);
    }
}

enum { app_animation_steps = 15 };

static void app_toast_paint(void) {
    static image_t image;
    if (image.bitmap == null) {
        uint8_t pixels[4] = { 0x3F, 0x3F, 0x3F };
        gdi.image_init(&image, 1, 1, 3, pixels);
    }
    if (app.animating.view != null) {
        ui_font_t f = app.animating.view->font != null ?
            *app.animating.view->font : app.fonts.regular;
        const ui_point_t em = gdi.get_em(f);
        app.animating.view->em = em;
        // allow unparented and unmeasured toasts:
        if (app.animating.view->measure != null) {
            app.animating.view->measure(app.animating.view);
        }
        gdi.push(0, 0);
        bool tooltip = app.animating.x >= 0 && app.animating.y >= 0;
        int32_t em_x = em.x;
        int32_t em_y = em.y;
        gdi.set_brush(gdi.brush_color);
        gdi.set_brush_color(colors.toast);
        if (!tooltip) {
            assert(0 <= app.animating.step && app.animating.step < app_animation_steps);
            int32_t step = app.animating.step - (app_animation_steps - 1);
            app.animating.view->y = app.animating.view->h * step / (app_animation_steps - 1);
//          traceln("step=%d of %d y=%d", app.animating.step,
//                  app_toast_steps, app.animating.view->y);
            app_layout_ui(app.animating.view);
            double alpha = min(0.40, 0.40 * app.animating.step / (double)app_animation_steps);
            gdi.alpha_blend(0, 0, app.width, app.height, &image, alpha);
            app.animating.view->x = (app.width - app.animating.view->w) / 2;
        } else {
            app.animating.view->x = app.animating.x;
            app.animating.view->y = app.animating.y;
            app_layout_ui(app.animating.view);
            int32_t mx = app.width - app.animating.view->w - em_x;
            app.animating.view->x = min(mx, max(0, app.animating.x - app.animating.view->w / 2));
            app.animating.view->y = min(app.crc.h - em_y, max(0, app.animating.y));
        }
        int32_t x = app.animating.view->x - em_x;
        int32_t y = app.animating.view->y - em_y / 2;
        int32_t w = app.animating.view->w + em_x * 2;
        int32_t h = app.animating.view->h + em_y;
        gdi.rounded(x, y, w, h, em_x, em_y);
        if (!tooltip) { app.animating.view->y += em_y / 4; }
        app_paint(app.animating.view);
        if (!tooltip) {
            if (app.animating.view->y == em_y / 4) {
                // micro "close" toast button:
                gdi.x = app.animating.view->x + app.animating.view->w;
                gdi.y = 0;
                gdi.text("\xC3\x97"); // Heavy Multiplication X
            }
        }
        gdi.pop();
    }
}

static void app_toast_cancel(void) {
    if (app.animating.view != null && app.animating.view->type == ui_view_messagebox) {
        ui_messagebox_t* mx = (ui_messagebox_t*)app.animating.view;
        if (mx->option < 0) { mx->cb(mx, -1); }
    }
    app.animating.step = 0;
    app.animating.view = null;
    app.animating.time = 0;
    app.animating.x = -1;
    app.animating.y = -1;
    app.redraw();
}

static void app_toast_mouse(int32_t m, int32_t flags) {
    bool pressed = m == ui.message.left_button_pressed ||
                   m == ui.message.right_button_pressed;
    if (app.animating.view != null && pressed) {
        const ui_point_t em = app.animating.view->em;
        int32_t x = app.animating.view->x + app.animating.view->w;
        if (x <= app.mouse.x && app.mouse.x <= x + em.x &&
            0 <= app.mouse.y && app.mouse.y <= em.y) {
            app_toast_cancel();
        } else {
            app_ui_mouse(app.animating.view, m, flags);
        }
    } else {
        app_ui_mouse(app.animating.view, m, flags);
    }
}

static void app_toast_character(const char* utf8) {
    char ch = utf8[0];
    if (app.animating.view != null && ch == 033) { // ESC traditionally in octal
        app_toast_cancel();
        app.show_toast(null, 0);
    } else {
        app_character(app.animating.view, utf8);
    }
}

static void app_toast_dim(int32_t step) {
    app.animating.step = step;
    app.redraw();
    UpdateWindow(app_window());
}

static void app_animate_step(app_animate_function_t f, int32_t step, int32_t steps) {
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

static void app_animate_start(app_animate_function_t f, int32_t steps) {
    // calls f(0..step-1) exactly steps times, unless cancelled with call
    // animate(null, 0) or animate(other_function, n > 0)
    app_animate_step(f, 0, steps);
}

static void app_layout_root(void) {
    not_null(app.window);
    not_null(app.canvas);
    app.view->w = app.crc.w; // crc is window client rectangle
    app.view->h = app.crc.h;
    app_layout_ui(app.view);
}

static void app_paint_on_canvas(HDC hdc) {
    ui_canvas_t canvas = app.canvas;
    app.canvas = (ui_canvas_t)hdc;
    gdi.push(0, 0);
    double time = clock.seconds();
    gdi.x = 0;
    gdi.y = 0;
    app_update_crc();
    if (app_layout_dirty) {
        app_layout_dirty = false;
        app_layout_root();
    }
    ui_font_t font = gdi.set_font(app.fonts.regular);
    ui_color_t c = gdi.set_text_color(colors.text);
    int32_t bm = SetBkMode(app_canvas(), TRANSPARENT);
    int32_t stretch_mode = SetStretchBltMode(app_canvas(), HALFTONE);
    ui_point_t pt = {0};
    fatal_if_false(SetBrushOrgEx(app_canvas(), 0, 0, (POINT*)&pt));
    ui_brush_t br = gdi.set_brush(gdi.brush_hollow);
    app_paint(app.view);
    if (app.animating.view != null) { app_toast_paint(); }
    fatal_if_false(SetBrushOrgEx(app_canvas(), pt.x, pt.y, null));
    SetStretchBltMode(app_canvas(), stretch_mode);
    SetBkMode(app_canvas(), bm);
    gdi.set_brush(br);
    gdi.set_text_color(c);
    gdi.set_font(font);
    app.paint_count++;
    if (app.paint_count % 128 == 0) { app.paint_max = 0; }
    app.paint_time = clock.seconds() - time;
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

static void app_wm_paint(void) {
    // it is possible to receive WM_PAINT when window is not closed
    if (app.window != null) {
        PAINTSTRUCT ps = {0};
        BeginPaint(app_window(), &ps);
        app_paint_on_canvas(ps.hdc);
        EndPaint(app_window(), &ps);
    }
}

// about (x,y) being (-32000,-32000) see:
// https://chromium.googlesource.com/chromium/src.git/+/62.0.3178.1/ui/views/win/hwnd_message_handler.cc#1847

static void app_window_position_changed(const WINDOWPOS* wp) {
    app.view->hidden = !IsWindowVisible(app_window());
    const bool moved  = (wp->flags & SWP_NOMOVE) == 0;
    const bool sized  = (wp->flags & SWP_NOSIZE) == 0;
    const bool hiding = (wp->flags & SWP_HIDEWINDOW) != 0 ||
                        wp->x == -32000 && wp->y == -32000;
    HMONITOR monitor = MonitorFromWindow(app_window(), MONITOR_DEFAULTTONULL);
    if (!app.view->hidden && (moved || sized) && !hiding && monitor != null) {
        RECT wrc = app_ui2rect(&app.wrc);
        fatal_if_false(GetWindowRect(app_window(), &wrc));
        app.wrc = app_rect2ui(&wrc);
        app_update_mi(&app.wrc, MONITOR_DEFAULTTONEAREST);
        app_update_crc();
        if (app_timer_1s_id != 0) { app.layout(); }
    }
}

static void app_setting_change(uintptr_t wp, uintptr_t lp) {
    if (wp == 0 && lp != 0 && strcmp((const char*)lp, "intl") == 0) {
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

static void app_show_task_bar(bool show) {
    HWND taskbar = FindWindowA("Shell_TrayWnd", null);
    if (taskbar != null) {
        ShowWindow(taskbar, show ? SW_SHOW : SW_HIDE);
        UpdateWindow(taskbar);
    }
}

static void app_click_detector(uint32_t msg, WPARAM wp, LPARAM lp) {
    // TODO: click detector does not handle WM_NCLBUTTONDOWN, ...
    //       it can be modified to do so if needed
    #pragma push_macro("set_timer")
    #pragma push_macro("kill_timer")
    #pragma push_macro("done")

    #define set_timer(t, ms) do {                   \
        assert(t == 0);                             \
        t = app_timer_set((uintptr_t)&t, ms);       \
    } while (0)

    #define kill_timer(t) do {                      \
        if (t != 0) { app_timer_kill(t); t = 0; }   \
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
    static double     clicked[3]; // click time
    static bool       pressed[3];
    static ui_timer_t       timer_d[3]; // double tap
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
                app_post_message(ui.message.press, i, lp);
                done(i);
            }
            if (wp == timer_d[i]) {
                lp = MAKELONG(click_at[i].x, click_at[i].y);
                app_post_message(ui.message.tap, i, lp);
                done(i);
            }
        }
    }
    if (ix != -1) {
        const uint32_t dtap_msec = GetDoubleClickTime();
        const double double_click_dt = dtap_msec / 1000.0;
        const int double_click_x = GetSystemMetrics(SM_CXDOUBLECLK) / 2;
        const int double_click_y = GetSystemMetrics(SM_CYDOUBLECLK) / 2;
        ui_point_t pt = { GET_X_LPARAM(lp), GET_Y_LPARAM(lp) };
        if ((int32_t)m == ui.message.tap) {
            if (app.now  - clicked[ix]  <= double_click_dt &&
                abs(pt.x - click_at[ix].x) <= double_click_x &&
                abs(pt.y - click_at[ix].y) <= double_click_y) {
                app_post_message(ui.message.dtap, ix, lp);
                done(ix);
            } else {
                done(ix); // clear timers
                clicked[ix]  = app.now;
                click_at[ix] = pt;
                pressed[ix]  = true;
                set_timer(timer_p[ix], ui_long_press_msec); // 0.25s
                set_timer(timer_d[ix], dtap_msec); // 0.5s
            }
        } else if (up) {
//          traceln("pressed[%d]: %d %.3f", ix, pressed[ix], app.now - clicked[ix]);
            if (pressed[ix] && app.now - clicked[ix] > double_click_dt) {
                app_post_message(ui.message.dtap, ix, lp);
                done(ix);
            }
            kill_timer(timer_p[ix]); // long press is no the case
        } else if ((int32_t)m == ui.message.dtap) {
            app_post_message(ui.message.dtap, ix, lp);
            done(ix);
        }
    }
    #pragma pop_macro("done")
    #pragma pop_macro("kill_timer")
    #pragma pop_macro("set_timer")
}

static LRESULT CALLBACK window_proc(HWND window, UINT msg, WPARAM wp, LPARAM lp) {
    app.now = clock.seconds();
    if (app.window == null) {
        app.window = (ui_window_t)window;
    } else {
        assert(app_window() == window);
    }
    int64_t ret = 0;
    app_kill_hidden_focus(app.view);
    app_click_detector(msg, wp, lp);
    if (app_message(app.view, msg, wp, lp, &ret)) {
        return (LRESULT)ret;
    }
    if ((int32_t)msg == ui.message.opening) { app_window_opening(); return 0; }
    if ((int32_t)msg == ui.message.closing) { app_window_closing(); return 0; }
    if ((int32_t)msg == ui.message.tap || (int32_t)msg == ui.message.dtap ||
        (int32_t)msg == ui.message.press) {
            app_tap_press((int32_t)msg, wp, lp);
            return 0;
    }
    if ((int32_t)msg == ui.message.animate) {
        app_animate_step((app_animate_function_t)lp, (int)wp, -1);
        return 0;
    }
    switch (msg) {
        case WM_GETMINMAXINFO: app_get_min_max_info((MINMAXINFO*)lp); break;
        case WM_SETTINGCHANGE: app_setting_change(wp, lp); break;
        case WM_CLOSE        : app.focus = null; // before WM_CLOSING
                               app_post_message(ui.message.closing, 0, 0); return 0;
        case WM_DESTROY      : PostQuitMessage(app.exit_code); break;
        case WM_SYSKEYDOWN: // for ALT (aka VK_MENU)
        case WM_KEYDOWN      : app_alt_ctrl_shift(true, (int32_t)wp);
                               app_key_pressed(app.view, (int32_t)wp);
                               break;
        case WM_SYSKEYUP:
        case WM_KEYUP        : app_alt_ctrl_shift(false, (int32_t)wp);
                               app_key_released(app.view, (int32_t)wp);
                               break;
        case WM_TIMER        : app_wm_timer((ui_timer_t)wp);
                               break;
        case WM_ERASEBKGND   : return true; // no DefWindowProc()
        case WM_SETCURSOR    : SetCursor((HCURSOR)app.cursor);
                               break; // must call DefWindowProc()
        // see: https://learn.microsoft.com/en-us/windows/win32/inputdev/about-keyboard-input
        // https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-tounicode
//      case WM_UNICHAR      : // only UTF-32 via PostMessage
        case WM_CHAR         : app_wm_char(app.view, (const char*)&wp);
                               break; // TODO: CreateWindowW() and utf16->utf8
        case WM_PRINTCLIENT  : app_paint_on_canvas((HDC)wp); break;
        case WM_SETFOCUS     : if (!app.view->hidden) { app_set_focus(app.view); }
                               break;
        case WM_KILLFOCUS    : if (!app.view->hidden) { app_kill_focus(app.view); }
                               break;
        case WM_PAINT        : app_wm_paint(); break;
        case WM_CONTEXTMENU  : (void)app_context_menu(app.view); break;
        case WM_MOUSEWHEEL   :
            app_mousewheel(app.view, 0, GET_WHEEL_DELTA_WPARAM(wp)); break;
        case WM_MOUSEHWHEEL  :
            app_mousewheel(app.view, GET_WHEEL_DELTA_WPARAM(wp), 0); break;
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
            ScreenToClient(app_window(), &pt);
            app.mouse = app_point2ui(&pt);
            app_mouse(app.view, (int32_t)msg, (int32_t)wp);
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
            app.mouse.x = GET_X_LPARAM(lp);
            app.mouse.y = GET_Y_LPARAM(lp);
//          traceln("%d %d", app.mouse.x, app.mouse.y);
            // note: ScreenToClient() is not needed for this messages
            app_mouse(app.view, (int32_t)msg, (int32_t)wp);
            break;
        }
        case WM_GETDPISCALEDSIZE: { // sent before WM_DPICHANGED
//          traceln("WM_GETDPISCALEDSIZE");
            #ifdef QUICK_DEBUG
                int32_t dpi = (int32_t)wp;
                SIZE* sz = (SIZE*)lp; // in/out
                ui_point_t cell = { sz->cx, sz->cy };
                traceln("WM_GETDPISCALEDSIZE dpi %d := %d "
                    "size %d,%d *may/must* be adjusted",
                    app.dpi.window, dpi, cell.x, cell.y);
            #endif
            if (app_timer_1s_id != 0 && !app.view->hidden) { app.layout(); }
            // IMPORTANT: return true because otherwise linear, see
            // https://learn.microsoft.com/en-us/windows/win32/hidpi/wm-getdpiscaledsize
            return true;
        }
        case WM_DPICHANGED: {
//          traceln("WM_DPICHANGED");
            app_window_dpi();
            app_init_fonts(app.dpi.window);
            if (app_timer_1s_id != 0 && !app.view->hidden) {
                app.layout();
            } else {
                app_layout_dirty = true;
            }
            break;
        }
        case WM_SYSCOMMAND:
            if (wp == SC_MINIMIZE && app.hide_on_minimize) {
                app.show_window(ui.visibility.min_na);
                app.show_window(ui.visibility.hide);
            }
            // If the selection is in menu handle the key event
            if (wp == SC_KEYMENU && lp != 0x20) {
                return 0; // This prevents the error/beep sound
            }
            break;
        case WM_ACTIVATE:
            if (!IsWindowVisible(app_window()) && LOWORD(wp) != WA_INACTIVE) {
                app.show_window(ui.visibility.restore);
                SwitchToThisWindow(app_window(), true);
            }
            break;
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
            app_window_position_changed((WINDOWPOS*)lp);
            break;
        default:
            break;
    }
    return DefWindowProcA(app_window(), msg, wp, lp);
}

static long app_set_window_long(int32_t index, long value) {
    runtime.seterr(0);
    long r = SetWindowLongA(app_window(), index, value); // r previous value
    fatal_if_not_zero(runtime.err());
    return r;
}

static void app_create_window(const ui_rect_t r) {
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
    uint32_t style = app.no_decor ? WS_POPUP : WS_OVERLAPPEDWINDOW;
    HWND window = CreateWindowExA(WS_EX_COMPOSITED | WS_EX_LAYERED,
        app.class_name, app.title, style,
        r.x, r.y, r.w, r.h, null, null, wc.hInstance, null);
    assert(window == app_window()); (void)window;
    not_null(app.window);
    app.dpi.window = GetDpiForWindow(app_window());
//  traceln("app.dpi.window=%d", app.dpi.window);
    RECT wrc = app_ui2rect(&r);
    fatal_if_false(GetWindowRect(app_window(), &wrc));
    app.wrc = app_rect2ui(&wrc);
    // DWMWA_CAPTION_COLOR is supported starting with Windows 11 Build 22000.
    if (IsWindowsVersionOrGreater(10, 0, 22000)) {
        COLORREF caption_color = (COLORREF)gdi.color_rgb(colors.dkgray3);
        fatal_if_not_zero(DwmSetWindowAttribute(app_window(),
            DWMWA_CAPTION_COLOR, &caption_color, sizeof(caption_color)));
        BOOL immersive = TRUE;
        fatal_if_not_zero(DwmSetWindowAttribute(app_window(),
            DWMWA_USE_IMMERSIVE_DARK_MODE, &immersive, sizeof(immersive)));
        // also available but not yet used:
//      DWMWA_USE_HOSTBACKDROPBRUSH
//      DWMWA_WINDOW_CORNER_PREFERENCE
//      DWMWA_BORDER_COLOR
//      DWMWA_CAPTION_COLOR
    }
    if (app.aero) { // It makes app look like retro Windows 7 Aero style :)
        enum DWMNCRENDERINGPOLICY ncrp = DWMNCRP_DISABLED;
        (void)DwmSetWindowAttribute(app_window(),
            DWMWA_NCRENDERING_POLICY, &ncrp, sizeof(ncrp));
    }
    // always start with window hidden and let application show it
    app.show_window(ui.visibility.hide);
    if (app.no_min || app.no_max) {
        uint32_t exclude = WS_SIZEBOX;
        if (app.no_min) { exclude = WS_MINIMIZEBOX; }
        if (app.no_max) { exclude = WS_MAXIMIZEBOX; }
        uint32_t s = GetWindowLongA(app_window(), GWL_STYLE);
        app_set_window_long(GWL_STYLE, s & ~exclude);
        // even for windows without maximize/minimize
        // make sure "Minimize All Windows" still works:
        // ???
//      EnableMenuItem(GetSystemMenu(app_window(), false),
//          SC_MINIMIZE, MF_BYCOMMAND | MF_ENABLED);
    }
    if (app.no_size) {
        uint32_t s = GetWindowLong(app_window(), GWL_STYLE);
        app_set_window_long(GWL_STYLE, s & ~WS_SIZEBOX);
        enum { swp = SWP_FRAMECHANGED |
                     SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE };
        SetWindowPos(app_window(), NULL, 0, 0, 0, 0, swp);
    }
    if (app.visibility != ui.visibility.hide) {
        app.view->w = app.wrc.w;
        app.view->h = app.wrc.h;
        AnimateWindow(app_window(), 250, AW_ACTIVATE);
        app.show_window(app.visibility);
        app_update_crc();
//      app.view->w = app.crc.w; // app.crc is "client rectangle"
//      app.view->h = app.crc.h;
    }
    // even if it is hidden:
    app_post_message(ui.message.opening, 0, 0);
//  SetWindowTheme(app_window(), L"DarkMode_Explorer", null); ???
}

static void app_full_screen(bool on) {
    static int32_t style;
    static WINDOWPLACEMENT wp;
    if (on != app.is_full_screen) {
        app_show_task_bar(!on);
        if (on) {
            style = GetWindowLongA(app_window(), GWL_STYLE);
            app_set_window_long(GWL_STYLE, (style | WS_POPUP | WS_VISIBLE) &
                ~(WS_OVERLAPPEDWINDOW));
            wp.length = sizeof(wp);
            fatal_if_false(GetWindowPlacement(app_window(), &wp));
            WINDOWPLACEMENT nwp = wp;
            nwp.showCmd = SW_SHOWNORMAL;
            nwp.rcNormalPosition = (RECT){app.mrc.x, app.mrc.y,
                app.mrc.x + app.mrc.w, app.mrc.y + app.mrc.h};
            fatal_if_false(SetWindowPlacement(app_window(), &nwp));
        } else {
            fatal_if_false(SetWindowPlacement(app_window(), &wp));
            app_set_window_long(GWL_STYLE,  style | WS_OVERLAPPED);
            enum { swp = SWP_FRAMECHANGED | SWP_NOMOVE | SWP_NOSIZE |
                         SWP_NOZORDER | SWP_NOOWNERZORDER };
            fatal_if_false(SetWindowPos(app_window(), null, 0, 0, 0, 0, swp));
            enum DWMNCRENDERINGPOLICY ncrp = DWMNCRP_ENABLED;
            fatal_if_not_zero(DwmSetWindowAttribute(app_window(),
                DWMWA_NCRENDERING_POLICY, &ncrp, sizeof(ncrp)));
        }
        app.is_full_screen = on;
    }
}

static void app_fast_redraw(void) { SetEvent(app_event_invalidate); } // < 2us

static void app_draw(void) { UpdateWindow(app_window()); }

static void app_invalidate_rect(const ui_rect_t* r) {
    RECT rc = app_ui2rect(r);
    InvalidateRect(app_window(), &rc, false);
}

// InvalidateRect() may wait for up to 30 milliseconds
// which is unacceptable for video drawing at monitor
// refresh rate

static void app_redraw_thread(void* unused(p)) {
    threads.realtime();
    threads.name("app.redraw");
    for (;;) {
        event_t es[] = { app_event_invalidate, app_event_quit };
        int32_t r = events.wait_any(countof(es), es);
        if (r == 0) {
            if (app_window() != null) {
                InvalidateRect(app_window(), null, false);
            }
        } else {
            break;
        }
    }
}

static int32_t app_message_loop(void) {
    MSG msg = {0};
    while (GetMessage(&msg, null, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessageA(&msg);
    }
    assert(msg.message == WM_QUIT);
    return (int32_t)msg.wParam;
}

static void app_dispose(void) {
    app_dispose_fonts();
    if (gdi.clip != null) { DeleteRgn(gdi.clip); }
    fatal_if_false(CloseHandle(app_event_quit));
    fatal_if_false(CloseHandle(app_event_invalidate));
}

static void app_cursor_set(ui_cursor_t c) {
    // https://docs.microsoft.com/en-us/windows/win32/menurc/using-cursors
    app.cursor = c;
    SetClassLongPtr(app_window(), GCLP_HCURSOR, (LONG_PTR)c);
    POINT pt = {0};
    if (GetCursorPos(&pt)) { SetCursorPos(pt.x + 1, pt.y); SetCursorPos(pt.x, pt.y); }
}

static void app_close_window(void) {
    app_post_message(WM_CLOSE, 0, 0);
}

static void app_quit(int32_t exit_code) {
    app.exit_code = exit_code;
    if (app.can_close != null) {
        (void)app.can_close(); // and deliberately ignore result
    }
    app.can_close = null; // will not be called again
    app.close(); // close and destroy app only window
}

static void app_show_tooltip_or_toast(ui_view_t* view, int32_t x, int32_t y,
        double timeout) {
    if (view != null) {
        app.animating.x = x;
        app.animating.y = y;
        if (view->type == ui_view_messagebox) {
            ((ui_messagebox_t*)view)->option = -1;
        }
        // allow unparented ui for toast and tooltip
        if (view->init != null) { view->init(view); view->init = null; }
        ui_view.localize(view);
        app_animate_start(app_toast_dim, app_animation_steps);
        app.animating.view = view;
        app.animating.view->font = &app.fonts.H1;
        app.animating.time = timeout > 0 ? app.now + timeout : 0;
    } else {
        app_toast_cancel();
    }
}

static void app_show_toast(ui_view_t* view, double timeout) {
    app_show_tooltip_or_toast(view, -1, -1, timeout);
}

static void app_show_tooltip(ui_view_t* view, int32_t x, int32_t y,
        double timeout) {
    if (view != null) {
        app_show_tooltip_or_toast(view, x, y, timeout);
    } else if (app.animating.view != null && app.animating.x >= 0 &&
               app.animating.y >= 0) {
        app_toast_cancel(); // only cancel tooltips not toasts
    }
}

static void app_formatted_toast_va(double timeout, const char* format, va_list vl) {
    app_show_toast(null, 0);
    static ui_label_t txt;
    ui_label_init_va(&txt, format, vl);
    txt.multiline = true;
    app_show_toast(&txt.view, timeout);
}

static void app_formatted_toast(double timeout, const char* format, ...) {
    va_list vl;
    va_start(vl, format);
    app_formatted_toast_va(timeout, format, vl);
    va_end(vl);
}

static void app_create_caret(int32_t w, int32_t h) {
    fatal_if_false(CreateCaret(app_window(), null, w, h));
    assert(GetSystemMetrics(SM_CARETBLINKINGENABLED));
}

static void app_show_caret(void) {
    fatal_if_false(ShowCaret(app_window()));
}

static void app_move_caret(int32_t x, int32_t y) {
    fatal_if_false(SetCaretPos(x, y));
}

static void app_hide_caret(void) {
    fatal_if_false(HideCaret(app_window()));
}

static void app_destroy_caret(void) {
    fatal_if_false(DestroyCaret());
}

static void app_enable_sys_command_close(void) {
    EnableMenuItem(GetSystemMenu(GetConsoleWindow(), false),
        SC_CLOSE, MF_BYCOMMAND | MF_ENABLED);
}

static void app_console_disable_close(void) {
    EnableMenuItem(GetSystemMenu(GetConsoleWindow(), false),
        SC_CLOSE, MF_BYCOMMAND | MF_DISABLED | MF_GRAYED);
    (void)freopen("CONOUT$", "w", stdout);
    (void)freopen("CONOUT$", "w", stderr);
    atexit(app_enable_sys_command_close);
}

static int app_console_attach(void) {
    int r = AttachConsole(ATTACH_PARENT_PROCESS) ? 0 : runtime.err();
    if (r == 0) {
        app_console_disable_close();
        threads.sleep_for(0.1); // give cmd.exe a chance to print prompt again
        printf("\n");
    }
    return r;
}

static bool app_is_stdout_redirected(void) {
    // https://stackoverflow.com/questions/30126490/how-to-check-if-stdout-is-redirected-to-a-file-or-to-a-console
    HANDLE out = GetStdHandle(STD_OUTPUT_HANDLE);
    DWORD type = out == null ? FILE_TYPE_UNKNOWN : GetFileType(out);
    type &= ~FILE_TYPE_REMOTE;
    // FILE_TYPE_DISK or FILE_TYPE_CHAR or FILE_TYPE_PIPE
    return type != FILE_TYPE_UNKNOWN;
}

static bool app_is_console_visible(void) {
    HWND cw = GetConsoleWindow();
    return cw != null && IsWindowVisible(cw);
}

static int app_set_console_size(int16_t w, int16_t h) {
    // width/height in characters
    HANDLE console = GetStdHandle(STD_OUTPUT_HANDLE);
    CONSOLE_SCREEN_BUFFER_INFOEX info = { sizeof(CONSOLE_SCREEN_BUFFER_INFOEX) };
    int r = GetConsoleScreenBufferInfoEx(console, &info) ? 0 : runtime.err();
    if (r != 0) {
        traceln("GetConsoleScreenBufferInfoEx() %s", str.error(r));
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
        int r0 = SetConsoleWindowInfo(console, true, &minwin) ? 0 : runtime.err();
//      if (r0 != 0) { traceln("SetConsoleWindowInfo() %s", str.error(r0)); }
        int r1 = SetConsoleScreenBufferSize(console, c) ? 0 : runtime.err();
//      if (r1 != 0) { traceln("SetConsoleScreenBufferSize() %s", str.error(r1)); }
        if (r0 != 0 || r1 != 0) { // try in reverse order (which expected to work):
            r0 = SetConsoleScreenBufferSize(console, c) ? 0 : runtime.err();
            if (r0 != 0) { traceln("SetConsoleScreenBufferSize() %s", str.error(r0)); }
            r1 = SetConsoleWindowInfo(console, true, &minwin) ? 0 : runtime.err();
            if (r1 != 0) { traceln("SetConsoleWindowInfo() %s", str.error(r1)); }
	    }
        r = r0 == 0 ? r1 : r0; // first of two errors
    }
    return r;
}

static void app_console_largest(void) {
    HANDLE console = GetStdHandle(STD_OUTPUT_HANDLE);
    // User have to manual uncheck "[x] Let system position window" in console
    // Properties -> Layout -> Window Position because I did not find the way
    // to programmatically unchecked it.
    // commented code below does not work.
    // see: https://www.os2museum.com/wp/disabling-quick-edit-mode/
    // and: https://learn.microsoft.com/en-us/windows/console/setconsolemode
    /* DOES NOT WORK:
    DWORD mode = 0;
    r = GetConsoleMode(console, &mode) ? 0 : runtime.err();
    fatal_if_not_zero(r, "GetConsoleMode() %s", str.error(r));
    mode &= ~ENABLE_AUTO_POSITION;
    r = SetConsoleMode(console, &mode) ? 0 : runtime.err();
    fatal_if_not_zero(r, "SetConsoleMode() %s", str.error(r));
    */
    CONSOLE_SCREEN_BUFFER_INFOEX info = { sizeof(CONSOLE_SCREEN_BUFFER_INFOEX) };
    int r = GetConsoleScreenBufferInfoEx(console, &info) ? 0 : runtime.err();
    fatal_if_not_zero(r, "GetConsoleScreenBufferInfoEx() %s", str.error(r));
    COORD c = GetLargestConsoleWindowSize(console);
    if (c.X > 80) { c.X &= ~0x7; }
    if (c.Y > 24) { c.Y &= ~0x3; }
    if (c.X > 80) { c.X -= 8; }
    if (c.Y > 24) { c.Y -= 4; }
    app_set_console_size(c.X, c.Y);
    r = GetConsoleScreenBufferInfoEx(console, &info) ? 0 : runtime.err();
    fatal_if_not_zero(r, "GetConsoleScreenBufferInfoEx() %s", str.error(r));
    info.dwSize.Y = 9999; // maximum value at the moment of implementation
    r = SetConsoleScreenBufferInfoEx(console, &info) ? 0 : runtime.err();
    fatal_if_not_zero(r, "SetConsoleScreenBufferInfoEx() %s", str.error(r));
    app_save_console_pos();
}

static void app_make_topmost(void) {
    //  Places the window above all non-topmost windows.
    // The window maintains its topmost position even when it is deactivated.
    enum { swp = SWP_SHOWWINDOW | SWP_NOREPOSITION | SWP_NOMOVE | SWP_NOSIZE };
    fatal_if_false(SetWindowPos(app_window(), HWND_TOPMOST, 0, 0, 0, 0, swp));
}

static void app_activate(void) {
    runtime.seterr(0);
    HWND previous = SetActiveWindow(app_window());
    if (previous == null) { fatal_if_not_zero(runtime.err()); }
}

static void app_bring_to_foreground(void) {
    // SetForegroundWindow() does not activate window:
    fatal_if_false(SetForegroundWindow(app_window()));
}

static void app_bring_to_front(void) {
    app.bring_to_foreground();
    app.make_topmost();
    app.bring_to_foreground();
    // because bring_to_foreground() does not activate
    app.activate();
    app.request_focus();
}

static void app_set_console_title(HWND cw) {
    char text[256];
    text[0] = 0;
    GetWindowTextA((HWND)app.window, text, countof(text));
    text[countof(text) - 1] = 0;
    char title[256];
    strprintf(title, "%s - Console", text);
    fatal_if_false(SetWindowTextA(cw, title));
}

static void app_restore_console(int32_t *visibility) {
    HWND cw = GetConsoleWindow();
    if (cw != null) {
        RECT wr = {0};
        GetWindowRect(cw, &wr);
        ui_rect_t rc = app_rect2ui(&wr);
        app_load_console_pos(&rc, visibility);
        if (rc.w > 0 && rc.h > 0) {
//          traceln("%d,%d %dx%d px", rc.x, rc.y, rc.w, rc.h);
            CONSOLE_SCREEN_BUFFER_INFOEX info = {
                sizeof(CONSOLE_SCREEN_BUFFER_INFOEX)
            };
            int32_t r = config.load(app.class_name,
                "console_screen_buffer_infoex", &info, (int)sizeof(info));
            if (r == sizeof(info)) { // 24x80
                SMALL_RECT sr = info.srWindow;
                int16_t w = max(sr.Right - sr.Left + 1, 80);
                int16_t h = max(sr.Bottom - sr.Top + 1, 24);
//              traceln("info: %dx%d", info.dwSize.X, info.dwSize.Y);
//              traceln("%d,%d %dx%d", sr.Left, sr.Top, w, h);
                if (w > 0 && h > 0) { app_set_console_size(w, h); }
    	    }
            // do not resize console window just restore it's position
            enum { swp = SWP_NOZORDER | SWP_NOACTIVATE | SWP_NOSIZE };
            fatal_if_false(SetWindowPos(cw, null,
                    rc.x, rc.y, rc.w, rc.h, swp));
        } else {
            app_console_largest();
        }
    }
}

static void app_console_show(bool b) {
    HWND cw = GetConsoleWindow();
    if (cw != null && b != app.is_console_visible()) {
        if (app.is_console_visible()) { app_save_console_pos(); }
        if (b) {
            int32_t ignored_visibility = 0;
            app_restore_console(&ignored_visibility);
            app_set_console_title(cw);
        }
        // If the window was previously visible, the return value is nonzero.
        // If the window was previously hidden, the return value is zero.
        bool unused_was_visible = ShowWindow(cw, b ? SW_SHOWNOACTIVATE : SW_HIDE);
        (void)unused_was_visible;
        if (b) { InvalidateRect(cw, null, true); SetActiveWindow(cw); }
        app_save_console_pos(); // again after visibility changed
    }
}

static int app_console_create(void) {
    int r = AllocConsole() ? 0 : runtime.err();
    if (r == 0) {
        app_console_disable_close();
        int32_t visibility = 0;
        app_restore_console(&visibility);
        app.console_show(visibility != 0);
    }
    return r;
}

static float app_px2in(int pixels) {
    assert(app.dpi.monitor_raw > 0);
    return app.dpi.monitor_raw > 0 ?
           pixels / (float)app.dpi.monitor_raw : 0;
}

static int32_t app_in2px(float inches) {
    assert(app.dpi.monitor_raw > 0);
    return (int32_t)(inches * app.dpi.monitor_raw + 0.5f);
}

static void app_request_layout(void) {
    app_layout_dirty = true;
    app.redraw();
}

static void app_show_window(int32_t show) {
    assert(ui.visibility.hide <= show &&
           show <= ui.visibility.force_min);
    // ShowWindow() does not have documented error reporting
    bool was_visible = ShowWindow(app_window(), show);
    (void)was_visible;
    const bool hiding =
        show == ui.visibility.hide ||
        show == ui.visibility.minimize ||
        show == ui.visibility.show_na ||
        show == ui.visibility.min_na;
    if (!hiding) {
        app.bring_to_foreground(); // this does not make it ActiveWindow
        enum { swp = SWP_SHOWWINDOW | SWP_NOZORDER | SWP_NOSIZE |
                     SWP_NOREPOSITION | SWP_NOMOVE };
        SetWindowPos(app_window(), null, 0, 0, 0, 0, swp);
        app.request_focus();
    } else if (show == ui.visibility.hide ||
               show == ui.visibility.minimize ||
               show == ui.visibility.min_na) {
        app_toast_cancel();
    }
}

static const char* app_open_filename(const char* folder,
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
            int32_t n0 = (int)wcslen(s0);
            int32_t n1 = (int)wcslen(s1);
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
    static thread_local char text[MAX_PATH];
    if (GetOpenFileNameW(&ofn) && path[0] != 0) {
        strprintf(text, "%s", utf16to8(path));
    } else {
        text[0] = 0;
    }
    return text;
}

static errno_t app_clipboard_put_image(image_t* im) {
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
    if (r != 0) { traceln("OpenClipboard() failed %s", str.error(r)); }
    if (r == 0) {
        r = EmptyClipboard() ? 0 : GetLastError();
        if (r != 0) { traceln("EmptyClipboard() failed %s", str.error(r)); }
    }
    if (r == 0) {
        r = SetClipboardData(CF_BITMAP, bitmap) ? 0 : GetLastError();
        if (r != 0) {
            traceln("SetClipboardData() failed %s", str.error(r));
        }
    }
    if (r == 0) {
        r = CloseClipboard() ? 0 : GetLastError();
        if (r != 0) {
            traceln("CloseClipboard() failed %s", str.error(r));
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

const char* app_known_folder(int32_t kf) {
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

static ui_view_t app_ui;

static bool app_is_active(void) {
    return GetActiveWindow() == app_window();
}

static bool app_has_focus(void) {
    return GetFocus() == app_window();
}

static void window_request_focus(void* w) {
    // https://stackoverflow.com/questions/62649124/pywin32-setfocus-resulting-in-access-is-denied-error
    // https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-attachthreadinput
    assert(threads.id() == app.tid, "cannot be called from background thread");
    runtime.seterr(0);
    w = SetFocus((HWND)w); // w previous focused window
    if (w == null) { fatal_if_not_zero(runtime.err()); }
}

static void app_request_focus(void) {
    window_request_focus(app.window);
}

static void app_init(void) {
    app.view = &app_ui;
    ui_view_init(app.view);
    app.view->measure = null; // always measured by crc
    app.view->layout  = null; // always at 0,0 app can override
    app.redraw = app_fast_redraw;
    app.draw = app_draw;
    app.px2in = app_px2in;
    app.in2px = app_in2px;
    app.point_in_rect = app_point_in_rect;
    app.intersect_rect = app_intersect_rect;
    app.is_active = app_is_active,
    app.has_focus = app_has_focus,
    app.request_focus = app_request_focus,
    app.activate = app_activate,
    app.bring_to_foreground = app_bring_to_foreground,
    app.make_topmost = app_make_topmost,
    app.bring_to_front = app_bring_to_front,
    app.measure = app_measure_children;
    app.layout = app_request_layout;
    app.invalidate = app_invalidate_rect;
    app.full_screen = app_full_screen;
    app.set_cursor = app_cursor_set;
    app.close = app_close_window;
    app.quit = app_quit;
    app.set_timer = app_timer_set;
    app.kill_timer = app_timer_kill;
    app.post = app_post_message;
    app.show_window = app_show_window;
    app.show_toast = app_show_toast;
    app.show_tooltip = app_show_tooltip;
    app.toast_va = app_formatted_toast_va;
    app.toast = app_formatted_toast;
    app.create_caret = app_create_caret;
    app.show_caret = app_show_caret;
    app.move_caret = app_move_caret;
    app.hide_caret = app_hide_caret;
    app.destroy_caret = app_destroy_caret;
    app.data_save = app_data_save;
    app.data_size = app_data_size;
    app.data_load = app_data_load;
    app.open_filename = app_open_filename;
    app.known_folder = app_known_folder;
    app.is_stdout_redirected = app_is_stdout_redirected;
    app.is_console_visible = app_is_console_visible;
    app.console_attach = app_console_attach;
    app.console_create = app_console_create;
    app.console_show = app_console_show;
    app_event_quit = events.create();
    app_event_invalidate = events.create();
    app.init();
}

static void app_init_windows(void) {
    fatal_if_not_zero(SetProcessDpiAwareness(PROCESS_PER_MONITOR_DPI_AWARE));
    not_null(SetThreadDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2));
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
    app_init_fonts(app.dpi.window); // for default monitor
}

static int app_win_main(void) {
    not_null(app.init);
    app_init_windows();
    gdi.init();
    clipboard.put_image = app_clipboard_put_image;
    app.last_visibility = ui.visibility.defau1t;
    app_init();
    int r = 0;
//  app_dump_dpi();
    // "wr" Window Rect in pixels: default is 100,100, wmin, hmin
    int32_t wmin = app.wmin > 0 ?
        app.in2px(app.wmin) : app.work_area.w / 4;
    int32_t hmin = app.hmin > 0 ?
        app.in2px(app.hmin) : app.work_area.h / 4;
    ui_rect_t wr = {100, 100, wmin, hmin};
    int32_t size_frame = GetSystemMetricsForDpi(SM_CXSIZEFRAME, app.dpi.process);
    int32_t caption_height = GetSystemMetricsForDpi(SM_CYCAPTION, app.dpi.process);
    wr.x -= size_frame;
    wr.w += size_frame * 2;
    wr.y -= size_frame + caption_height;
    wr.h += size_frame * 2 + caption_height;
    if (!app_load_window_pos(&wr, &app.last_visibility)) {
        // first time - center window
        wr.x = app.work_area.x + (app.work_area.w - wr.w) / 2;
        wr.y = app.work_area.y + (app.work_area.h - wr.h) / 2;
        app_bring_window_inside_monitor(&app.mrc, &wr);
    }
    app.view->hidden = true; // start with ui hidden
    app.view->font = &app.fonts.regular;
    app.view->w = wr.w - size_frame * 2;
    app.view->h = wr.h - size_frame * 2 - caption_height;
    app_layout_dirty = true; // layout will be done before first paint
    not_null(app.class_name);
    if (!app.no_ui) {
        app_create_window(wr);
        thread_t thread = threads.start(app_redraw_thread, null);
        r = app_message_loop();
        fatal_if_false(SetEvent(app_event_quit));
        threads.join(thread, -1);
        app_dispose();
        if (r == 0 && app.exit_code != 0) { r = app.exit_code; }
    } else {
        r = app.main();
    }
    if (app.fini != null) { app.fini(); }
    return r;
}

#pragma warning(disable: 28251) // inconsistent annotations

int WINAPI WinMain(HINSTANCE unused(instance), HINSTANCE unused(previous),
        char* unused(command), int show) {
    app.tid = threads.id();
    fatal_if_not_zero(CoInitializeEx(0, COINIT_MULTITHREADED | COINIT_SPEED_OVER_MEMORY));
// TODO: remove it?
    // https://learn.microsoft.com/en-us/windows/win32/api/imm/nf-imm-immdisablelegacyime
//  ImmDisableLegacyIME();
    // https://developercommunity.visualstudio.com/t/MSCTFdll-timcpp-An-assertion-failure-h/10513796
//  ImmDisableIME(0); // temporarily disable IME till MS fixes that assert
    SetConsoleCP(CP_UTF8);
    nls.init();
    app.visibility = show;
    args.WinMain();
    int32_t r = app_win_main();
    args.fini();
    return r;
}

int main(int argc, const char* argv[], const char** envp) {
    fatal_if_not_zero(CoInitializeEx(0, COINIT_MULTITHREADED | COINIT_SPEED_OVER_MEMORY));
    args.main(argc, argv, envp);
    nls.init();
    app.tid = threads.id();
    int r = app.main();
    args.fini();
    return r;
}

#pragma pop_macro("app_canvas")
#pragma pop_macro("app_window")

#pragma comment(lib, "comctl32")
#pragma comment(lib, "comdlg32")
#pragma comment(lib, "dwmapi")
#pragma comment(lib, "gdi32")
// #pragma comment(lib, "imm32")
#pragma comment(lib, "msimg32")
#pragma comment(lib, "ole32")
#pragma comment(lib, "shcore")
// _________________________________ button.c _________________________________

#include "ut/ut.h"

static void ui_button_every_100ms(ui_view_t* view) { // every 100ms
    assert(view->type == ui_view_button);
    ui_button_t* b = (ui_button_t*)view;
    if (b->armed_until != 0 && app.now > b->armed_until) {
        b->armed_until = 0;
        view->armed = false;
        ui_view.invalidate(view);
    }
}

static void ui_button_paint(ui_view_t* view) {
    assert(view->type == ui_view_button);
    assert(!view->hidden);
    ui_button_t* b = (ui_button_t*)view;
    gdi.push(view->x, view->y);
    bool pressed = (view->armed ^ view->pressed) == 0;
    if (b->armed_until != 0) { pressed = true; }
    int32_t sign = 1 - pressed * 2; // -1, +1
    int32_t w = sign * view->w;
    int32_t h = sign * view->h;
    int32_t x = b->view.x + (int)pressed * view->w;
    int32_t y = b->view.y + (int)pressed * view->h;
    gdi.gradient(x, y, w, h, colors.btn_gradient_darker,
        colors.btn_gradient_dark, true);
    ui_color_t c = view->armed ? colors.btn_armed : view->color;
    if (b->view.hover && !view->armed) { c = colors.btn_hover_highlight; }
    if (view->disabled) { c = colors.btn_disabled; }
    ui_font_t f = view->font != null ? *view->font : app.fonts.regular;
    ui_point_t m = gdi.measure_text(f, ui_view.nls(view));
    gdi.set_text_color(c);
    gdi.x = view->x + (view->w - m.x) / 2;
    gdi.y = view->y + (view->h - m.y) / 2;
    f = gdi.set_font(f);
    gdi.text("%s", ui_view.nls(view));
    gdi.set_font(f);
    const int32_t pw = max(1, view->em.y / 32); // pen width
    ui_color_t color = view->armed ? colors.dkgray4 : colors.gray;
    if (view->hover && !view->armed) { color = colors.blue; }
    if (view->disabled) { color = colors.dkgray1; }
    ui_pen_t p = gdi.create_pen(color, pw);
    gdi.set_pen(p);
    gdi.set_brush(gdi.brush_hollow);
    gdi.rounded(view->x, view->y, view->w, view->h, view->em.y / 4, view->em.y / 4);
    gdi.delete_pen(p);
    gdi.pop();
}

static bool ui_button_hit_test(ui_button_t* b, ui_point_t pt) {
    assert(b->view.type == ui_view_button);
    pt.x -= b->view.x;
    pt.y -= b->view.y;
    return 0 <= pt.x && pt.x < b->view.w && 0 <= pt.y && pt.y < b->view.h;
}

static void ui_button_callback(ui_button_t* b) {
    assert(b->view.type == ui_view_button);
    app.show_tooltip(null, -1, -1, 0);
    if (b->cb != null) { b->cb(b); }
}

static void ui_button_trigger(ui_view_t* view) {
    assert(view->type == ui_view_button);
    assert(!view->hidden && !view->disabled);
    ui_button_t* b = (ui_button_t*)view;
    view->armed = true;
    ui_view.invalidate(view);
    app.draw();
    b->armed_until = app.now + 0.250;
    ui_button_callback(b);
    ui_view.invalidate(view);
}

static void ui_button_character(ui_view_t* view, const char* utf8) {
    assert(view->type == ui_view_button);
    assert(!view->hidden && !view->disabled);
    char ch = utf8[0]; // TODO: multibyte shortcuts?
    if (view->is_keyboard_shortcut(view, ch)) {
        ui_button_trigger(view);
    }
}

static void ui_button_key_pressed(ui_view_t* view, int32_t key) {
    if (app.alt && view->is_keyboard_shortcut(view, key)) {
//      traceln("key: 0x%02X shortcut: %d", key, view->is_keyboard_shortcut(view, key));
        ui_button_trigger(view);
    }
}

/* processes mouse clicks and invokes callback  */

static void ui_button_mouse(ui_view_t* view, int32_t message, int32_t flags) {
    assert(view->type == ui_view_button);
    (void)flags; // unused
    assert(!view->hidden && !view->disabled);
    ui_button_t* b = (ui_button_t*)view;
    bool a = view->armed;
    bool on = false;
    if (message == ui.message.left_button_pressed ||
        message == ui.message.right_button_pressed) {
        view->armed = ui_button_hit_test(b, app.mouse);
        if (view->armed) { app.focus = view; }
        if (view->armed) { app.show_tooltip(null, -1, -1, 0); }
    }
    if (message == ui.message.left_button_released ||
        message == ui.message.right_button_released) {
        if (view->armed) { on = ui_button_hit_test(b, app.mouse); }
        view->armed = false;
    }
    if (on) { ui_button_callback(b); }
    if (a != view->armed) { ui_view.invalidate(view); }
}

static void ui_button_measure(ui_view_t* view) {
    assert(view->type == ui_view_button || view->type == ui_view_text);
    ui_view.measure(view);
    const int32_t em2  = max(1, view->em.x / 2);
    view->w = view->w;
    view->h = view->h + em2;
    if (view->w < view->h) { view->w = view->h; }
}

void ui_button_init_(ui_view_t* view) {
    assert(view->type == ui_view_button);
    ui_view_init(view);
    view->mouse       = ui_button_mouse;
    view->measure     = ui_button_measure;
    view->paint       = ui_button_paint;
    view->character   = ui_button_character;
    view->every_100ms = ui_button_every_100ms;
    view->key_pressed = ui_button_key_pressed;
    ui_view.set_text(view, view->text);
    ui_view.localize(view);
    view->color = colors.btn_text;
}

void ui_button_init(ui_button_t* b, const char* label, double ems,
        void (*cb)(ui_button_t* b)) {
    static_assert(offsetof(ui_button_t, view) == 0, "offsetof(.view)");
    b->view.type = ui_view_button;
    strprintf(b->view.text, "%s", label);
    b->cb = cb;
    b->view.width = ems;
    ui_button_init_(&b->view);
}
// ________________________________ checkbox.c ________________________________

#include "ut/ut.h"

static int ui_checkbox_paint_on_off(ui_view_t* view) {
    // https://www.compart.com/en/unicode/U+2B24
    static const char* circle = "\xE2\xAC\xA4"; // Black Large Circle
    gdi.push(view->x, view->y);
    ui_color_t background = view->pressed ? colors.tone_green : colors.dkgray4;
    ui_color_t foreground = view->color;
    gdi.set_text_color(background);
    int32_t x = view->x;
    int32_t x1 = view->x + view->em.x * 3 / 4;
    while (x < x1) {
        gdi.x = x;
        gdi.text("%s", circle);
        x++;
    }
    int32_t rx = gdi.x;
    gdi.set_text_color(foreground);
    gdi.x = view->pressed ? x : view->x;
    gdi.text("%s", circle);
    gdi.pop();
    return rx;
}

static const char* ui_checkbox_on_off_label(ui_view_t* view, char* label, int32_t count)  {
    str.format(label, count, "%s", ui_view.nls(view));
    char* s = strstr(label, "___");
    if (s != null) {
        memcpy(s, view->pressed ? "On " : "Off", 3);
    }
    return nls.str(label);
}

static void ui_checkbox_measure(ui_view_t* view) {
    assert(view->type == ui_view_checkbox);
    ui_view.measure(view);
    view->w += view->em.x * 2;
}

static void ui_checkbox_paint(ui_view_t* view) {
    assert(view->type == ui_view_checkbox);
    char text[countof(view->text)];
    const char* label = ui_checkbox_on_off_label(view, text, countof(text));
    gdi.push(view->x, view->y);
    ui_font_t f = view->font != null ? *view->font : app.fonts.regular;
    ui_font_t font = gdi.set_font(f);
    gdi.x = ui_checkbox_paint_on_off(view) + view->em.x * 3 / 4;
    gdi.text("%s", label);
    gdi.set_font(font);
    gdi.pop();
}

static void ui_checkbox_flip(checkbox_t* c) {
    assert(c->view.type == ui_view_checkbox);
    app.redraw();
    c->view.pressed = !c->view.pressed;
    if (c->cb != null) { c->cb(c); }
}

static void ui_checkbox_character(ui_view_t* view, const char* utf8) {
    assert(view->type == ui_view_checkbox);
    assert(!view->hidden && !view->disabled);
    char ch = utf8[0];
    if (view->is_keyboard_shortcut(view, ch)) {
         ui_checkbox_flip((checkbox_t*)view);
    }
}

static void ui_checkbox_key_pressed(ui_view_t* view, int32_t key) {
    if (app.alt && view->is_keyboard_shortcut(view, key)) {
//      traceln("key: 0x%02X shortcut: %d", key, view->is_keyboard_shortcut(view, key));
        ui_checkbox_flip((checkbox_t*)view);
    }
}

static void ui_checkbox_mouse(ui_view_t* view, int32_t message, int32_t flags) {
    assert(view->type == ui_view_checkbox);
    (void)flags; // unused
    assert(!view->hidden && !view->disabled);
    if (message == ui.message.left_button_pressed ||
        message == ui.message.right_button_pressed) {
        int32_t x = app.mouse.x - view->x;
        int32_t y = app.mouse.y - view->y;
        if (0 <= x && x < view->w && 0 <= y && y < view->h) {
            app.focus = view;
            ui_checkbox_flip((checkbox_t*)view);
        }
    }
}

void ui_checkbox_init_(ui_view_t* view) {
    assert(view->type == ui_view_checkbox);
    ui_view_init(view);
    ui_view.set_text(view, view->text);
    view->mouse       = ui_checkbox_mouse;
    view->measure     = ui_checkbox_measure;
    view->paint       = ui_checkbox_paint;
    view->character   = ui_checkbox_character;
    view->key_pressed = ui_checkbox_key_pressed;
    ui_view.localize(view);
    view->color = colors.btn_text;
}

void ui_checkbox_init(checkbox_t* c, const char* label, double ems,
       void (*cb)( checkbox_t* b)) {
    static_assert(offsetof( checkbox_t, view) == 0, "offsetof(.view)");
    ui_view_init(&c->view);
    strprintf(c->view.text, "%s", label);
    c->view.width = ems;
    c->cb = cb;
    c->view.type = ui_view_checkbox;
    ui_checkbox_init_(&c->view);
}
// _________________________________ colors.c _________________________________

#include "ut/ut.h"

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
    .black   = rgb(0,     0,   0),
    .red     = rgb(255,   0,   0),
    .green   = rgb(0,   255,   0),
    .blue    = rgb(0,   0,   255),
    .yellow  = rgb(255, 255,   0),
    .cyan    = rgb(0,   255, 255),
    .magenta = rgb(255,   0, 255),
    .gray    = rgb(128, 128, 128),
    .dkgray1  = rgb(30, 30, 30),
    .dkgray2  = rgb(37, 38, 38),
    .dkgray3  = rgb(45, 45, 48),
    .dkgray4  = _colors_dkgray4,
    // tone down RGB colors:
    .tone_white   = rgb(164, 164, 164),
    .tone_red     = rgb(192,  64,  64),
    .tone_green   = rgb(64,  192,  64),
    .tone_blue    = rgb(64,   64, 192),
    .tone_yellow  = rgb(192, 192,  64),
    .tone_cyan    = rgb(64,  192, 192),
    .tone_magenta = rgb(192,  64, 192),
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
// __________________________________ core.c __________________________________

#include "ut/ut.h"
#include "ut/win32.h"

#define UI_WM_ANIMATE  (WM_APP + 0x7FFF)
#define UI_WM_OPENING  (WM_APP + 0x7FFE)
#define UI_WM_CLOSING  (WM_APP + 0x7FFD)
#define UI_WM_TAP      (WM_APP + 0x7FFC)
#define UI_WM_DTAP     (WM_APP + 0x7FFB) // double tap (aka click)
#define UI_WM_PRESS    (WM_APP + 0x7FFA)

extern ui_if ui = {
    .visibility = { // window visibility see ShowWindow link below
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
    },
    .message = {
        .character             = WM_CHAR,
        .key_pressed           = WM_KEYDOWN,
        .key_released          = WM_KEYUP,
        .left_button_pressed   = WM_LBUTTONDOWN,
        .left_button_released  = WM_LBUTTONUP,
        .right_button_pressed  = WM_RBUTTONDOWN,
        .right_button_released = WM_RBUTTONUP,
        .mouse_move            = WM_MOUSEMOVE,
        .left_double_click     = WM_LBUTTONDBLCLK,
        .right_double_click    = WM_RBUTTONDBLCLK,
        .animate               = UI_WM_ANIMATE,
        .opening               = UI_WM_OPENING,
        .closing               = UI_WM_CLOSING,
        .tap                   = UI_WM_TAP,
        .dtap                  = UI_WM_DTAP,
        .press                 = UI_WM_PRESS
    },
    .mouse = {
        .button = {
            .left  = MK_LBUTTON,
            .right = MK_RBUTTON
        }
    },
    .key = {
        .up     = VK_UP,
        .down   = VK_DOWN,
        .left   = VK_LEFT,
        .right  = VK_RIGHT,
        .home   = VK_HOME,
        .end    = VK_END,
        .pageup = VK_PRIOR,
        .pagedw = VK_NEXT,
        .insert = VK_INSERT,
        .del    = VK_DELETE,
        .back   = VK_BACK,
        .escape = VK_ESCAPE,
        .enter  = VK_RETURN,
        .minus  = VK_OEM_MINUS,
        .plus   = VK_OEM_PLUS,
        .f1     = VK_F1,
        .f2     = VK_F2,
        .f3     = VK_F3,
        .f4     = VK_F4,
        .f5     = VK_F5,
        .f6     = VK_F6,
        .f7     = VK_F7,
        .f8     = VK_F8,
        .f9     = VK_F9,
        .f10    = VK_F10,
        .f11    = VK_F11,
        .f12    = VK_F12,
        .f13    = VK_F13,
        .f14    = VK_F14,
        .f15    = VK_F15,
        .f16    = VK_F16,
        .f17    = VK_F17,
        .f18    = VK_F18,
        .f19    = VK_F19,
        .f20    = VK_F20,
        .f21    = VK_F21,
        .f22    = VK_F22,
        .f23    = VK_F23,
        .f24    = VK_F24,
    },
    .folder = {
        .home      = 0, // c:\Users\<username>
        .desktop   = 1,
        .documents = 2,
        .downloads = 3,
        .music     = 4,
        .pictures  = 5,
        .videos    = 6,
        .shared    = 7, // c:\Users\Public
        .bin       = 8, // c:\Program Files
        .data      = 9  // c:\ProgramData
    }
};

// https://docs.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-showwindow
// __________________________________ gdi.c ___________________________________

#include "ut/ut.h"
#include "ut/win32.h"

#pragma push_macro("app_window")
#pragma push_macro("app_canvas")
#pragma push_macro("gdi_with_hdc")
#pragma push_macro("gdi_hdc_with_font")

#define app_window() ((HWND)app.window)
#define app_canvas() ((HDC)app.canvas)

typedef struct gdi_xyc_s {
    int32_t x;
    int32_t y;
    ui_color_t c;
} gdi_xyc_t;

static int32_t gdi_top;
static gdi_xyc_t gdi_stack[256];

static void gdi_init(void) {
    gdi.brush_hollow = (ui_brush_t)GetStockBrush(HOLLOW_BRUSH);
    gdi.brush_color  = (ui_brush_t)GetStockBrush(DC_BRUSH);
    gdi.pen_hollow = (ui_pen_t)GetStockPen(NULL_PEN);
}

static uint32_t gdi_color_rgb(ui_color_t c) {
    assert(color_is_8bit(c));
    return (COLORREF)(c & 0xFFFFFFFF);
}

static COLORREF gdi_color_ref(ui_color_t c) {
    return gdi.color_rgb(c);
}

static ui_color_t gdi_set_text_color(ui_color_t c) {
    return SetTextColor(app_canvas(), gdi_color_ref(c));
}

static ui_pen_t gdi_set_pen(ui_pen_t p) {
    not_null(p);
    return (ui_pen_t)SelectPen(app_canvas(), (HPEN)p);
}

static ui_pen_t gdi_set_colored_pen(ui_color_t c) {
    ui_pen_t p = (ui_pen_t)SelectPen(app_canvas(), GetStockPen(DC_PEN));
    SetDCPenColor(app_canvas(), gdi_color_ref(c));
    return p;
}

static ui_pen_t gdi_create_pen(ui_color_t c, int32_t width) {
    assert(width >= 1);
    ui_pen_t pen = (ui_pen_t)CreatePen(PS_SOLID, width, gdi_color_ref(c));
    not_null(pen);
    return pen;
}

static void gdi_delete_pen(ui_pen_t p) {
    fatal_if_false(DeletePen(p));
}

static ui_brush_t gdi_create_brush(ui_color_t c) {
    return (ui_brush_t)CreateSolidBrush(gdi_color_ref(c));
}

static void gdi_delete_brush(ui_brush_t b) {
    DeleteBrush((HBRUSH)b);
}

static ui_brush_t gdi_set_brush(ui_brush_t b) {
    not_null(b);
    return (ui_brush_t)SelectBrush(app_canvas(), b);
}

static ui_color_t gdi_set_brush_color(ui_color_t c) {
    return SetDCBrushColor(app_canvas(), gdi_color_ref(c));
}

static void gdi_set_clip(int32_t x, int32_t y, int32_t w, int32_t h) {
    if (gdi.clip != null) { DeleteRgn(gdi.clip); gdi.clip = null; }
    if (w > 0 && h > 0) {
        gdi.clip = (ui_region_t)CreateRectRgn(x, y, x + w, y + h);
        not_null(gdi.clip);
    }
    fatal_if(SelectClipRgn(app_canvas(), (HRGN)gdi.clip) == ERROR);
}

static void gdi_push(int32_t x, int32_t y) {
    assert(gdi_top < countof(gdi_stack));
    fatal_if(gdi_top >= countof(gdi_stack));
    gdi_stack[gdi_top].x = gdi.x;
    gdi_stack[gdi_top].y = gdi.y;
    fatal_if(SaveDC(app_canvas()) == 0);
    gdi_top++;
    gdi.x = x;
    gdi.y = y;
}

static void gdi_pop(void) {
    assert(0 < gdi_top && gdi_top <= countof(gdi_stack));
    fatal_if(gdi_top <= 0);
    gdi_top--;
    gdi.x = gdi_stack[gdi_top].x;
    gdi.y = gdi_stack[gdi_top].y;
    fatal_if_false(RestoreDC(app_canvas(), -1));
}

static void gdi_pixel(int32_t x, int32_t y, ui_color_t c) {
    not_null(app.canvas);
    fatal_if_false(SetPixel(app_canvas(), x, y, gdi_color_ref(c)));
}

static ui_point_t gdi_move_to(int32_t x, int32_t y) {
    POINT pt;
    pt.x = gdi.x;
    pt.y = gdi.y;
    fatal_if_false(MoveToEx(app_canvas(), x, y, &pt));
    gdi.x = x;
    gdi.y = y;
    ui_point_t p = { pt.x, pt.y };
    return p;
}

static void gdi_line(int32_t x, int32_t y) {
    fatal_if_false(LineTo(app_canvas(), x, y));
    gdi.x = x;
    gdi.y = y;
}

static void gdi_frame(int32_t x, int32_t y, int32_t w, int32_t h) {
    ui_brush_t b = gdi.set_brush(gdi.brush_hollow);
    gdi.rect(x, y, w, h);
    gdi.set_brush(b);
}

static void gdi_rect(int32_t x, int32_t y, int32_t w, int32_t h) {
    fatal_if_false(Rectangle(app_canvas(), x, y, x + w, y + h));
}

static void gdi_fill(int32_t x, int32_t y, int32_t w, int32_t h) {
    RECT rc = { x, y, x + w, y + h };
    ui_brush_t b = (ui_brush_t)GetCurrentObject(app_canvas(), OBJ_BRUSH);
    fatal_if_false(FillRect(app_canvas(), &rc, (HBRUSH)b));
}

static void gdi_frame_with(int32_t x, int32_t y, int32_t w, int32_t h,
        ui_color_t c) {
    ui_brush_t b = gdi.set_brush(gdi.brush_hollow);
    ui_pen_t p = gdi.set_colored_pen(c);
    gdi.rect(x, y, w, h);
    gdi.set_pen(p);
    gdi.set_brush(b);
}

static void gdi_rect_with(int32_t x, int32_t y, int32_t w, int32_t h,
        ui_color_t border, ui_color_t fill) {
    ui_brush_t b = gdi.set_brush(gdi.brush_color);
    ui_color_t c = gdi.set_brush_color(fill);
    ui_pen_t p = gdi.set_colored_pen(border);
    gdi.rect(x, y, w, h);
    gdi.set_brush_color(c);
    gdi.set_pen(p);
    gdi.set_brush(b);
}

static void gdi_fill_with(int32_t x, int32_t y, int32_t w, int32_t h,
        ui_color_t c) {
    ui_brush_t b = gdi.set_brush(gdi.brush_color);
    c = gdi.set_brush_color(c);
    gdi.fill(x, y, w, h);
    gdi.set_brush_color(c);
    gdi.set_brush(b);
}

static void gdi_poly(ui_point_t* points, int32_t count) {
    // make sure ui_point_t and POINT have the same memory layout:
    static_assert(sizeof(points->x) == sizeof(((POINT*)0)->x), "ui_point_t");
    static_assert(sizeof(points->y) == sizeof(((POINT*)0)->y), "ui_point_t");
    static_assert(sizeof(points[0]) == sizeof(*((POINT*)0)), "ui_point_t");
    assert(app_canvas() != null && count > 1);
    fatal_if_false(Polyline(app_canvas(), (POINT*)points, count));
}

static void gdi_rounded(int32_t x, int32_t y, int32_t w, int32_t h,
        int32_t rx, int32_t ry) {
    fatal_if_false(RoundRect(app_canvas(), x, y, x + w, y + h, rx, ry));
}

static void gdi_gradient(int32_t x, int32_t y, int32_t w, int32_t h,
        ui_color_t rgba_from, ui_color_t rgba_to, bool vertical) {
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
    const int32_t mode = vertical ? GRADIENT_FILL_RECT_V : GRADIENT_FILL_RECT_H;
    GradientFill(app_canvas(), vertex, 2, &gRect, 1, mode);
}

static BITMAPINFO* gdi_greyscale_bitmap_info(void) {
    typedef struct bitmap_rgb_s {
        BITMAPINFO bi;
        RGBQUAD rgb[256];
    } bitmap_rgb_t;
    static bitmap_rgb_t storage; // for grayscale palette
    static BITMAPINFO* bi = &storage.bi;
    BITMAPINFOHEADER* bih = &bi->bmiHeader;
    if (bih->biSize == 0) { // once
        bih->biSize = sizeof(BITMAPINFOHEADER);
        for (int32_t i = 0; i < 256; i++) {
            RGBQUAD* q = &bi->bmiColors[i];
            q->rgbReserved = 0;
            q->rgbBlue = q->rgbGreen = q->rgbRed = (uint8_t)i;
        }
        bih->biPlanes = 1;
        bih->biBitCount = 8;
        bih->biCompression = BI_RGB;
        bih->biClrUsed = 256;
        bih->biClrImportant = 256;
    }
    return bi;
}

static void gdi_draw_greyscale(int32_t sx, int32_t sy, int32_t sw, int32_t sh,
        int32_t x, int32_t y, int32_t w, int32_t h,
        int32_t iw, int32_t ih, int32_t stride, const uint8_t* pixels) {
    fatal_if(stride != ((iw + 3) & ~0x3));
    assert(w > 0 && h != 0); // h can be negative
    if (w > 0 && h != 0) {
        BITMAPINFO *bi = gdi_greyscale_bitmap_info(); // global! not thread safe
        BITMAPINFOHEADER* bih = &bi->bmiHeader;
        bih->biWidth = iw;
        bih->biHeight = -ih; // top down image
        bih->biSizeImage = w * h;
        POINT pt = { 0 };
        fatal_if_false(SetBrushOrgEx(app_canvas(), 0, 0, &pt));
        fatal_if(StretchDIBits(app_canvas(), sx, sy, sw, sh, x, y, w, h,
            pixels, bi, DIB_RGB_COLORS, SRCCOPY) == 0);
        fatal_if_false(SetBrushOrgEx(app_canvas(), pt.x, pt.y, &pt));
    }
}

static BITMAPINFOHEADER gdi_bgrx_init_bi(int32_t w, int32_t h, int32_t bpp) {
    BITMAPINFOHEADER bi = {
        .biSize = sizeof(BITMAPINFOHEADER),
        .biPlanes = 1,
        .biBitCount = (uint16_t)(bpp * 8),
        .biCompression = BI_RGB,
        .biWidth = w,
        .biHeight = -h, // top down image
        .biSizeImage = w * h * bpp,
        .biClrUsed = 0,
        .biClrImportant = 0
   };
   return bi;
}

// draw_bgr(iw) assumes strides are padded and rounded up to 4 bytes
// if this is not the case use gdi.image_init() that will unpack
// and align scanlines prior to draw

static void gdi_draw_bgr(int32_t sx, int32_t sy, int32_t sw, int32_t sh,
        int32_t x, int32_t y, int32_t w, int32_t h,
        int32_t iw, int32_t ih, int32_t stride,
        const uint8_t* pixels) {
    fatal_if(stride != ((iw * 3 + 3) & ~0x3));
    assert(w > 0 && h != 0); // h can be negative
    if (w > 0 && h != 0) {
        BITMAPINFOHEADER bi = gdi_bgrx_init_bi(iw, ih, 3);
        POINT pt = { 0 };
        fatal_if_false(SetBrushOrgEx(app_canvas(), 0, 0, &pt));
        fatal_if(StretchDIBits(app_canvas(), sx, sy, sw, sh, x, y, w, h,
            pixels, (BITMAPINFO*)&bi, DIB_RGB_COLORS, SRCCOPY) == 0);
        fatal_if_false(SetBrushOrgEx(app_canvas(), pt.x, pt.y, &pt));
    }
}

static void gdi_draw_bgrx(int32_t sx, int32_t sy, int32_t sw, int32_t sh,
        int32_t x, int32_t y, int32_t w, int32_t h,
        int32_t iw, int32_t ih, int32_t stride,
        const uint8_t* pixels) {
    fatal_if(stride != ((iw * 4 + 3) & ~0x3));
    assert(w > 0 && h != 0); // h can be negative
    if (w > 0 && h != 0) {
        BITMAPINFOHEADER bi = gdi_bgrx_init_bi(iw, ih, 4);
        POINT pt = { 0 };
        fatal_if_false(SetBrushOrgEx(app_canvas(), 0, 0, &pt));
        fatal_if(StretchDIBits(app_canvas(), sx, sy, sw, sh, x, y, w, h,
            pixels, (BITMAPINFO*)&bi, DIB_RGB_COLORS, SRCCOPY) == 0);
        fatal_if_false(SetBrushOrgEx(app_canvas(), pt.x, pt.y, &pt));
    }
}

static BITMAPINFO* gdi_init_bitmap_info(int32_t w, int32_t h, int32_t bpp,
        BITMAPINFO* bi) {
    bi->bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bi->bmiHeader.biWidth = w;
    bi->bmiHeader.biHeight = -h;  // top down image
    bi->bmiHeader.biPlanes = 1;
    bi->bmiHeader.biBitCount = (uint16_t)(bpp * 8);
    bi->bmiHeader.biCompression = BI_RGB;
    bi->bmiHeader.biSizeImage = w * h * bpp;
    return bi;
}

static void gdi_create_dib_section(image_t* image, int32_t w, int32_t h,
        int32_t bpp) {
    fatal_if(image->bitmap != null, "image_dispose() not called?");
    // not using GetWindowDC(app_window()) will allow to initialize images
    // before window is created
    HDC c = CreateCompatibleDC(null); // GetWindowDC(app_window());
    BITMAPINFO local = { {sizeof(BITMAPINFOHEADER)} };
    BITMAPINFO* bi = bpp == 1 ? gdi_greyscale_bitmap_info() : &local;
    image->bitmap = (ui_bitmap_t)CreateDIBSection(c, gdi_init_bitmap_info(w, h, bpp, bi),
                                               DIB_RGB_COLORS, &image->pixels, null, 0x0);
    fatal_if(image->bitmap == null || image->pixels == null);
//  fatal_if_false(ReleaseDC(app_window(), c));
    fatal_if_false(DeleteDC(c));
}

static void gdi_image_init_rgbx(image_t* image, int32_t w, int32_t h,
        int32_t bpp, const uint8_t* pixels) {
    bool swapped = bpp < 0;
    bpp = abs(bpp);
    fatal_if(bpp != 4, "bpp: %d", bpp);
    gdi_create_dib_section(image, w, h, bpp);
    const int32_t stride = (w * bpp + 3) & ~0x3;
    uint8_t* scanline = image->pixels;
    const uint8_t* rgbx = pixels;
    if (!swapped) {
        for (int32_t y = 0; y < h; y++) {
            uint8_t* bgra = scanline;
            for (int32_t x = 0; x < w; x++) {
                bgra[0] = rgbx[2];
                bgra[1] = rgbx[1];
                bgra[2] = rgbx[0];
                bgra[3] = 0xFF;
                bgra += 4;
                rgbx += 4;
            }
            pixels += w * 4;
            scanline += stride;
        }
    } else {
        for (int32_t y = 0; y < h; y++) {
            uint8_t* bgra = scanline;
            for (int32_t x = 0; x < w; x++) {
                bgra[0] = rgbx[0];
                bgra[1] = rgbx[1];
                bgra[2] = rgbx[2];
                bgra[3] = 0xFF;
                bgra += 4;
                rgbx += 4;
            }
            pixels += w * 4;
            scanline += stride;
        }
    }
    image->w = w;
    image->h = h;
    image->bpp = bpp;
    image->stride = stride;
}

static void gdi_image_init(image_t* image, int32_t w, int32_t h, int32_t bpp,
        const uint8_t* pixels) {
    bool swapped = bpp < 0;
    bpp = abs(bpp);
    fatal_if(bpp < 0 || bpp == 2 || bpp > 4, "bpp=%d not {1, 3, 4}", bpp);
    gdi_create_dib_section(image, w, h, bpp);
    // Win32 bitmaps stride is rounded up to 4 bytes
    const int32_t stride = (w * bpp + 3) & ~0x3;
    uint8_t* scanline = image->pixels;
    if (bpp == 1) {
        for (int32_t y = 0; y < h; y++) {
            memcpy(scanline, pixels, w);
            pixels += w;
            scanline += stride;
        }
    } else if (bpp == 3 && !swapped) {
        const uint8_t* rgb = pixels;
        for (int32_t y = 0; y < h; y++) {
            uint8_t* bgr = scanline;
            for (int32_t x = 0; x < w; x++) {
                bgr[0] = rgb[2];
                bgr[1] = rgb[1];
                bgr[2] = rgb[0];
                bgr += 3;
                rgb += 3;
            }
            pixels += w * bpp;
            scanline += stride;
        }
    } else if (bpp == 3 && swapped) {
        const uint8_t* rgb = pixels;
        for (int32_t y = 0; y < h; y++) {
            uint8_t* bgr = scanline;
            for (int32_t x = 0; x < w; x++) {
                bgr[0] = rgb[0];
                bgr[1] = rgb[1];
                bgr[2] = rgb[2];
                bgr += 3;
                rgb += 3;
            }
            pixels += w * bpp;
            scanline += stride;
        }
    } else if (bpp == 4 && !swapped) {
        // premultiply alpha, see:
        // https://stackoverflow.com/questions/24595717/alphablend-generating-incorrect-colors
        const uint8_t* rgba = pixels;
        for (int32_t y = 0; y < h; y++) {
            uint8_t* bgra = scanline;
            for (int32_t x = 0; x < w; x++) {
                int32_t alpha = rgba[3];
                bgra[0] = (uint8_t)(rgba[2] * alpha / 255);
                bgra[1] = (uint8_t)(rgba[1] * alpha / 255);
                bgra[2] = (uint8_t)(rgba[0] * alpha / 255);
                bgra[3] = rgba[3];
                bgra += 4;
                rgba += 4;
            }
            pixels += w * 4;
            scanline += stride;
        }
    } else if (bpp == 4 && swapped) {
        // premultiply alpha, see:
        // https://stackoverflow.com/questions/24595717/alphablend-generating-incorrect-colors
        const uint8_t* rgba = pixels;
        for (int32_t y = 0; y < h; y++) {
            uint8_t* bgra = scanline;
            for (int32_t x = 0; x < w; x++) {
                int32_t alpha = rgba[3];
                bgra[0] = (uint8_t)(rgba[0] * alpha / 255);
                bgra[1] = (uint8_t)(rgba[1] * alpha / 255);
                bgra[2] = (uint8_t)(rgba[2] * alpha / 255);
                bgra[3] = rgba[3];
                bgra += 4;
                rgba += 4;
            }
            pixels += w * 4;
            scanline += stride;
        }
    }
    image->w = w;
    image->h = h;
    image->bpp = bpp;
    image->stride = stride;
}

static void gdi_alpha_blend(int32_t x, int32_t y, int32_t w, int32_t h,
        image_t* image, double alpha) {
    assert(image->bpp > 0);
    assert(0 <= alpha && alpha <= 1);
    not_null(app_canvas());
    HDC c = CreateCompatibleDC(app_canvas());
    not_null(c);
    HBITMAP zero1x1 = SelectBitmap((HDC)c, (HBITMAP)image->bitmap);
    BLENDFUNCTION bf = { 0 };
    bf.SourceConstantAlpha = (uint8_t)(0xFF * alpha + 0.49);
    if (image->bpp == 4) {
        bf.BlendOp = AC_SRC_OVER;
        bf.BlendFlags = 0;
        bf.AlphaFormat = AC_SRC_ALPHA;
    } else {
        bf.BlendOp = AC_SRC_OVER;
        bf.BlendFlags = 0;
        bf.AlphaFormat = 0;
    }
    fatal_if_false(AlphaBlend(app_canvas(), x, y, w, h,
        c, 0, 0, image->w, image->h, bf));
    SelectBitmap((HDC)c, zero1x1);
    fatal_if_false(DeleteDC(c));
}

static void gdi_draw_image(int32_t x, int32_t y, int32_t w, int32_t h,
        image_t* image) {
    assert(image->bpp == 1 || image->bpp == 3 || image->bpp == 4);
    not_null(app_canvas());
    if (image->bpp == 1) { // StretchBlt() is bad for greyscale
        BITMAPINFO* bi = gdi_greyscale_bitmap_info();
        fatal_if(StretchDIBits(app_canvas(), x, y, w, h, 0, 0, image->w, image->h,
            image->pixels, gdi_init_bitmap_info(image->w, image->h, 1, bi),
            DIB_RGB_COLORS, SRCCOPY) == 0);
    } else {
        HDC c = CreateCompatibleDC(app_canvas());
        not_null(c);
        HBITMAP zero1x1 = SelectBitmap(c, image->bitmap);
        fatal_if_false(StretchBlt(app_canvas(), x, y, w, h,
            c, 0, 0, image->w, image->h, SRCCOPY));
        SelectBitmap(c, zero1x1);
        fatal_if_false(DeleteDC(c));
    }
}

static void gdi_cleartype(bool on) {
    enum { spif = SPIF_UPDATEINIFILE | SPIF_SENDCHANGE };
    fatal_if_false(SystemParametersInfoA(SPI_SETFONTSMOOTHING, true, 0, spif));
    uintptr_t s = on ? FE_FONTSMOOTHINGCLEARTYPE : FE_FONTSMOOTHINGSTANDARD;
    fatal_if_false(SystemParametersInfoA(SPI_SETFONTSMOOTHINGTYPE, 0,
        (void*)s, spif));
}

static void gdi_font_smoothing_contrast(int32_t c) {
    fatal_if(!(c == -1 || 1000 <= c && c <= 2200), "contrast: %d", c);
    if (c == -1) { c = 1400; }
    fatal_if_false(SystemParametersInfoA(SPI_SETFONTSMOOTHINGCONTRAST, 0,
                   (void*)(uintptr_t)c, SPIF_UPDATEINIFILE | SPIF_SENDCHANGE));
}

static_assertion(gdi_font_quality_default == DEFAULT_QUALITY);
static_assertion(gdi_font_quality_draft == DRAFT_QUALITY);
static_assertion(gdi_font_quality_proof == PROOF_QUALITY);
static_assertion(gdi_font_quality_nonantialiased == NONANTIALIASED_QUALITY);
static_assertion(gdi_font_quality_antialiased == ANTIALIASED_QUALITY);
static_assertion(gdi_font_quality_cleartype == CLEARTYPE_QUALITY);
static_assertion(gdi_font_quality_cleartype_natural == CLEARTYPE_NATURAL_QUALITY);

static ui_font_t gdi_font(ui_font_t f, int32_t height, int32_t quality) {
    assert(f != null && height > 0);
    LOGFONTA lf = {0};
    int32_t n = GetObjectA(f, sizeof(lf), &lf);
    fatal_if_false(n == (int)sizeof(lf));
    lf.lfHeight = -height;
    if (gdi_font_quality_default <= quality && quality <= gdi_font_quality_cleartype_natural) {
        lf.lfQuality = (uint8_t)quality;
    } else {
        fatal_if(quality != -1, "use -1 for do not care quality");
    }
    return (ui_font_t)CreateFontIndirectA(&lf);
}

static int32_t gdi_font_height(ui_font_t f) {
    assert(f != null);
    LOGFONTA lf = {0};
    int32_t n = GetObjectA(f, sizeof(lf), &lf);
    fatal_if_false(n == (int)sizeof(lf));
    assert(lf.lfHeight < 0);
    return abs(lf.lfHeight);
}

static void gdi_delete_font(ui_font_t f) {
    fatal_if_false(DeleteFont(f));
}

static ui_font_t gdi_set_font(ui_font_t f) {
    not_null(f);
    return (ui_font_t)SelectFont(app_canvas(), (HFONT)f);
}

#define gdi_with_hdc(code) do {                                          \
    not_null(app_window());                                              \
    HDC hdc = app_canvas() != null ? app_canvas() : GetDC(app_window()); \
    not_null(hdc);                                                       \
    code                                                                 \
    if (app_canvas() == null) {                                          \
        ReleaseDC(app_window(), hdc);                                    \
    }                                                                    \
} while (0);

#define gdi_hdc_with_font(f, code) do {                                  \
    not_null(f);                                                         \
    not_null(app_window());                                              \
    HDC hdc = app_canvas() != null ? app_canvas() : GetDC(app_window()); \
    not_null(hdc);                                                       \
    HFONT _font_ = SelectFont(hdc, (HFONT)f);                            \
    code                                                                 \
    SelectFont(hdc, _font_);                                             \
    if (app_canvas() == null) {                                          \
        ReleaseDC(app_window(), hdc);                                        \
    }                                                                    \
} while (0);


static int32_t gdi_baseline(ui_font_t f) {
    TEXTMETRICA tm;
    gdi_hdc_with_font(f, {
        fatal_if_false(GetTextMetricsA(hdc, &tm));
    })
    return tm.tmAscent;
}

static int32_t gdi_descent(ui_font_t f) {
    TEXTMETRICA tm;
    gdi_hdc_with_font(f, {
        fatal_if_false(GetTextMetricsA(hdc, &tm));
    });
    return tm.tmDescent;
}

static ui_point_t gdi_get_em(ui_font_t f) {
    SIZE cell = {0};
    gdi_hdc_with_font(f, {
        fatal_if_false(GetTextExtentPoint32A(hdc, "M", 1, &cell));
    });
    ui_point_t c = {cell.cx, cell.cy};
    return c;
}

static bool gdi_is_mono(ui_font_t f) {
    SIZE em = {0}; // "M"
    SIZE vl = {0}; // "|" Vertical Line https://www.compart.com/en/unicode/U+007C
    SIZE e3 = {0}; // "\xE2\xB8\xBB" Three-Em Dash https://www.compart.com/en/unicode/U+2E3B
    gdi_hdc_with_font(f, {
        fatal_if_false(GetTextExtentPoint32A(hdc, "M", 1, &em));
        fatal_if_false(GetTextExtentPoint32A(hdc, "|", 1, &vl));
        fatal_if_false(GetTextExtentPoint32A(hdc, "\xE2\xB8\xBB", 1, &e3));
    });
    return em.cx == vl.cx && vl.cx == e3.cx;
}

static double gdi_line_spacing(double height_multiplier) {
    assert(0.1 <= height_multiplier && height_multiplier <= 2.0);
    double hm = gdi.height_multiplier;
    gdi.height_multiplier = height_multiplier;
    return hm;
}

static int32_t gdi_draw_utf16(ui_font_t font, const char* s, int32_t n,
        RECT* r, uint32_t format) {
    // if font == null, draws on HDC with selected font
    int32_t height = 0; // return value is the height of the text in logical units
    if (font != null) {
        gdi_hdc_with_font(font, {
            height = DrawTextW(hdc, utf8to16(s), n, r, format);
        });
    } else {
        gdi_with_hdc({
            height = DrawTextW(hdc, utf8to16(s), n, r, format);
        });
    }
    return height;
}

typedef struct gdi_dtp_s { // draw text params
    ui_font_t font;
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
    int32_t n = 1024;
    char* text = (char*)alloca(n);
    str.format_va(text, n - 1, p->format, p->vl);
    int32_t k = (int32_t)strlen(text);
    // Microsoft returns -1 not posix required sizeof buffer
    while (k >= n - 1 || k < 0) {
        n = n * 2;
        text = (char*)alloca(n);
        str.format_va(text, n - 1, p->format, p->vl);
        k = (int)strlen(text);
    }
    assert(k >= 0 && k <= n, "k=%d n=%d fmt=%s", k, n, p->format);
    // rectangle is always calculated - it makes draw text
    // much slower but UI layer is mostly uses bitmap caching:
    if ((p->flags & DT_CALCRECT) == 0) {
        // no actual drawing just calculate rectangle
        bool b = gdi_draw_utf16(p->font, text, -1, &p->rc, p->flags | DT_CALCRECT);
        assert(b, "draw_text_utf16(%s) failed", text); (void)b;
    }
    bool b = gdi_draw_utf16(p->font, text, -1, &p->rc, p->flags);
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

static ui_point_t gdi_text_measure(gdi_dtp_t* p) {
    gdi_text_draw(p);
    ui_point_t cell = {p->rc.right - p->rc.left, p->rc.bottom - p->rc.top};
    return cell;
}

static ui_point_t gdi_measure_singleline(ui_font_t f, const char* format, ...) {
    va_list vl;
    va_start(vl, format);
    gdi_dtp_t p = { f, format, vl, {0, 0, 0, 0}, sl_measure };
    ui_point_t cell = gdi_text_measure(&p);
    va_end(vl);
    return cell;
}

static ui_point_t gdi_measure_multiline(ui_font_t f, int32_t w, const char* format, ...) {
    va_list vl;
    va_start(vl, format);
    uint32_t flags = w <= 0 ? ml_measure : ml_measure_break;
    gdi_dtp_t p = { f, format, vl, {gdi.x, gdi.y, gdi.x + (w <= 0 ? 1 : w), gdi.y}, flags };
    ui_point_t cell = gdi_text_measure(&p);
    va_end(vl);
    return cell;
}

static void gdi_vtext(const char* format, va_list vl) {
    gdi_dtp_t p = { null, format, vl, {gdi.x, gdi.y, 0, 0}, sl_draw };
    gdi_text_draw(&p);
    gdi.x += p.rc.right - p.rc.left;
}

static void gdi_vtextln(const char* format, va_list vl) {
    gdi_dtp_t p = { null, format, vl, {gdi.x, gdi.y, gdi.x, gdi.y}, sl_draw };
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

static ui_point_t gdi_multiline(int32_t w, const char* f, ...) {
    va_list vl;
    va_start(vl, f);
    uint32_t flags = w <= 0 ? ml_draw : ml_draw_break;
    gdi_dtp_t p = { null, f, vl, {gdi.x, gdi.y, gdi.x + (w <= 0 ? 1 : w), gdi.y}, flags };
    gdi_text_draw(&p);
    va_end(vl);
    ui_point_t c = { p.rc.right - p.rc.left, p.rc.bottom - p.rc.top };
    return c;
}

static void gdi_vprint(const char* format, va_list vl) {
    not_null(app.fonts.mono);
    ui_font_t f = gdi.set_font(app.fonts.mono);
    gdi.vtext(format, vl);
    gdi.set_font(f);
}

static void gdi_print(const char* format, ...) {
    va_list vl;
    va_start(vl, format);
    gdi.vprint(format, vl);
    va_end(vl);
}

static void gdi_vprintln(const char* format, va_list vl) {
    not_null(app.fonts.mono);
    ui_font_t f = gdi.set_font(app.fonts.mono);
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

static uint8_t* gdi_load_image(const void* data, int32_t bytes, int* w, int* h,
        int* bytes_per_pixel, int32_t preferred_bytes_per_pixel) {
    #ifdef STBI_VERSION
        return stbi_load_from_memory((uint8_t const*)data, bytes, w, h,
            bytes_per_pixel, preferred_bytes_per_pixel);
    #else // see instructions above
        (void)data; (void)bytes; (void)data; (void)w; (void)h;
        (void)bytes_per_pixel; (void)preferred_bytes_per_pixel;
        fatal_if(true, "curl.exe --silent --fail --create-dirs "
            "https://raw.githubusercontent.com/nothings/stb/master/stb_image.h "
            "--output ext/stb_image.h");
        return null;
    #endif
}

static void gdi_image_dispose(image_t* image) {
    fatal_if_false(DeleteBitmap(image->bitmap));
    memset(image, 0, sizeof(image_t));
}

gdi_t gdi = {
    .height_multiplier = 1.0,
    .init = gdi_init,
    .color_rgb = gdi_color_rgb,
    .image_init = gdi_image_init,
    .image_init_rgbx = gdi_image_init_rgbx,
    .image_dispose = gdi_image_dispose,
    .alpha_blend = gdi_alpha_blend,
    .draw_image = gdi_draw_image,
    .set_text_color = gdi_set_text_color,
    .create_brush = gdi_create_brush,
    .delete_brush = gdi_delete_brush,
    .set_brush = gdi_set_brush,
    .set_brush_color = gdi_set_brush_color,
    .set_colored_pen = gdi_set_colored_pen,
    .create_pen = gdi_create_pen,
    .set_pen = gdi_set_pen,
    .delete_pen = gdi_delete_pen,
    .set_clip = gdi_set_clip,
    .push = gdi_push,
    .pop = gdi_pop,
    .pixel = gdi_pixel,
    .move_to = gdi_move_to,
    .line = gdi_line,
    .frame = gdi_frame,
    .rect = gdi_rect,
    .fill = gdi_fill,
    .frame_with = gdi_frame_with,
    .rect_with = gdi_rect_with,
    .fill_with = gdi_fill_with,
    .poly = gdi_poly,
    .rounded = gdi_rounded,
    .gradient = gdi_gradient,
    .draw_greyscale = gdi_draw_greyscale,
    .draw_bgr = gdi_draw_bgr,
    .draw_bgrx = gdi_draw_bgrx,
    .cleartype = gdi_cleartype,
    .font_smoothing_contrast = gdi_font_smoothing_contrast,
    .font = gdi_font,
    .delete_font = gdi_delete_font,
    .set_font = gdi_set_font,
    .font_height = gdi_font_height,
    .descent = gdi_descent,
    .baseline = gdi_baseline,
    .is_mono = gdi_is_mono,
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

#pragma pop_macro("gdi_hdc_with_font")
#pragma pop_macro("gdi_with_hdc")
#pragma pop_macro("app_canvas")
#pragma pop_macro("app_window")
// _________________________________ label.c __________________________________

#include "ut/ut.h"

static void ui_label_paint(ui_view_t* view) {
    assert(view->type == ui_view_text);
    assert(!view->hidden);
    ui_label_t* t = (ui_label_t*)view;
    // at later stages of layout text height can grow:
    gdi.push(view->x, view->y + t->dy);
    ui_font_t f = view->font != null ? *view->font : app.fonts.regular;
    gdi.set_font(f);
//  traceln("%s h=%d dy=%d baseline=%d", view->text, view->h, t->dy, view->baseline);
    ui_color_t c = view->hover && t->highlight && !t->label ?
        colors.text_highlight : view->color;
    gdi.set_text_color(c);
    // paint for text also does lightweight re-layout
    // which is useful for simplifying dynamic text changes
    if (!t->multiline) {
        gdi.text("%s", ui_view.nls(view));
    } else {
        int32_t w = (int)(view->width * view->em.x + 0.5);
        gdi.multiline(w == 0 ? -1 : w, "%s", ui_view.nls(view));
    }
    if (view->hover && t->hovered && !t->label) {
        gdi.set_colored_pen(colors.btn_hover_highlight);
        gdi.set_brush(gdi.brush_hollow);
        int32_t cr = view->em.y / 4; // corner radius
        int32_t h = t->multiline ? view->h : view->baseline + view->descent;
        gdi.rounded(view->x - cr, view->y + t->dy, view->w + 2 * cr,
            h, cr, cr);
    }
    gdi.pop();
}

static void ui_label_context_menu(ui_view_t* view) {
    assert(view->type == ui_view_text);
    ui_label_t* t = (ui_label_t*)view;
    if (!t->label && !ui_view.is_hidden(view) && !ui_view.is_disabled(view)) {
        clipboard.put_text(ui_view.nls(view));
        static bool first_time = true;
        app.toast(first_time ? 2.15 : 0.75,
            nls.str("Text copied to clipboard"));
        first_time = false;
    }
}

static void ui_label_character(ui_view_t* view, const char* utf8) {
    assert(view->type == ui_view_text);
    ui_label_t* t = (ui_label_t*)view;
    if (view->hover && !t->label &&
       !ui_view.is_hidden(view) && !ui_view.is_disabled(view)) {
        char ch = utf8[0];
        // Copy to clipboard works for hover over text
        if ((ch == 3 || ch == 'c' || ch == 'C') && app.ctrl) {
            clipboard.put_text(ui_view.nls(view)); // 3 is ASCII for Ctrl+C
        }
    }
}

void ui_label_init_(ui_view_t* view) {
    static_assert(offsetof(ui_label_t, view) == 0, "offsetof(.view)");
    assert(view->type == ui_view_text);
    ui_view_init(view);
    if (view->font == null) { view->font = &app.fonts.regular; }
    view->color = colors.text;
    view->paint = ui_label_paint;
    view->character = ui_label_character;
    view->context_menu = ui_label_context_menu;
}

void ui_label_init_va(ui_label_t* t, const char* format, va_list vl) {
    static_assert(offsetof(ui_label_t, view) == 0, "offsetof(.view)");
    str.format_va(t->view.text, countof(t->view.text), format, vl);
    t->view.type = ui_view_text;
    ui_label_init_(&t->view);
}

void ui_label_init(ui_label_t* t, const char* format, ...) {
    va_list vl;
    va_start(vl, format);
    ui_label_init_va(t, format, vl);
    va_end(vl);
}

void ui_label_init_ml(ui_label_t* t, double width, const char* format, ...) {
    va_list vl;
    va_start(vl, format);
    ui_label_init_va(t, format, vl);
    va_end(vl);
    t->view.width = width;
    t->multiline = true;
}
// _________________________________ layout.c _________________________________

#include "ut/ut.h"

static void measurements_center(ui_view_t* view) {
    assert(view->children != null && view->children[0] != null, "no children?");
    assert(view->children[1] == null, "must be single child");
    ui_view_t* c = view->children[0]; // even if hidden measure it
    c->w = view->w;
    c->h = view->h;
}

static void measurements_horizontal(ui_view_t* view, int32_t gap) {
    assert(view->children != null && view->children[0] != null, "no children?");
    ui_view_t** c = view->children;
    view->w = 0;
    view->h = 0;
    bool seen = false;
    while (*c != null) {
        ui_view_t* u = *c;
        if (!u->hidden) {
            if (seen) { view->w += gap; }
            view->w += u->w;
            view->h = max(view->h, u->h);
            seen = true;
        }
        c++;
    }
}

static void measurements_vertical(ui_view_t* view, int32_t gap) {
    assert(view->children != null && view->children[0] != null, "no children?");
    ui_view_t** c = view->children;
    view->h = 0;
    bool seen = false;
    while (*c != null) {
        ui_view_t* u = *c;
        if (!u->hidden) {
            if (seen) { view->h += gap; }
            view->h += u->h;
            view->w = max(view->w, u->w);
            seen = true;
        }
        c++;
    }
}

static void measurements_grid(ui_view_t* view, int32_t gap_h, int32_t gap_v) {
    int32_t cols = 0;
    for (ui_view_t** row = view->children; *row != null; row++) {
        ui_view_t* r = *row;
        int32_t n = 0;
        for (ui_view_t** col = r->children; *col != null; col++) { n++; }
        if (cols == 0) { cols = n; }
        assert(n > 0 && cols == n);
    }
    int32_t* mxw = (int32_t*)alloca(cols * sizeof(int32_t));
    memset(mxw, 0, cols * sizeof(int32_t));
    for (ui_view_t** row = view->children; *row != null; row++) {
        if (!(*row)->hidden) {
            (*row)->h = 0;
            (*row)->baseline = 0;
            int32_t i = 0;
            for (ui_view_t** col = (*row)->children; *col != null; col++) {
                if (!(*col)->hidden) {
                    mxw[i] = max(mxw[i], (*col)->w);
                    (*row)->h = max((*row)->h, (*col)->h);
//                  traceln("[%d] row.baseline: %d col.baseline: %d ", i, (*row)->baseline, (*col)->baseline);
                    (*row)->baseline = max((*row)->baseline, (*col)->baseline);
                }
                i++;
            }
        }
    }
    view->h = 0;
    view->w = 0;
    int32_t rows_seen = 0; // number of visible rows so far
    for (ui_view_t** row = view->children; *row != null; row++) {
        ui_view_t* r = *row;
        if (!r->hidden) {
            r->w = 0;
            int32_t i = 0;
            int32_t cols_seen = 0; // number of visible columns so far
            for (ui_view_t** col = r->children; *col != null; col++) {
                ui_view_t* c = *col;
                if (!c->hidden) {
                    c->h = r->h; // all cells are same height
                    if (c->type == ui_view_text) { // lineup text baselines
                        ui_label_t* t = (ui_label_t*)c;
                        t->dy = r->baseline - c->baseline;
                    }
                    c->w = mxw[i++];
                    r->w += c->w;
                    if (cols_seen > 0) { r->w += gap_h; }
                    view->w = max(view->w, r->w);
                    cols_seen++;
                }
            }
            view->h += r->h;
            if (rows_seen > 0) { view->h += gap_v; }
            rows_seen++;
        }
    }
}

measurements_if measurements = {
    .center     = measurements_center,
    .horizontal = measurements_horizontal,
    .vertical   = measurements_vertical,
    .grid       = measurements_grid,
};

// layouts

static void layouts_center(ui_view_t* view) {
    assert(view->children != null && view->children[0] != null, "no children?");
    assert(view->children[1] == null, "must be single child");
    ui_view_t* c = view->children[0];
    c->x = (view->w - c->w) / 2;
    c->y = (view->h - c->h) / 2;
}

static void layouts_horizontal(ui_view_t* view, int32_t x, int32_t y, int32_t gap) {
    assert(view->children != null && view->children[0] != null, "no children?");
    ui_view_t** c = view->children;
    bool seen = false;
    while (*c != null) {
        ui_view_t* u = *c;
        if (!u->hidden) {
            if (seen) { x += gap; }
            u->x = x;
            u->y = y;
            x += u->w;
            seen = true;
        }
        c++;
    }
}

static void layouts_vertical(ui_view_t* view, int32_t x, int32_t y, int32_t gap) {
    assert(view->children != null && view->children[0] != null, "no children?");
    ui_view_t** c = view->children;
    bool seen = false;
    while (*c != null) {
        ui_view_t* u = *c;
        if (!u->hidden) {
            if (seen) { y += gap; }
            u->x = x;
            u->y = y;
            y += u->h;
            seen = true;
        }
        c++;
    }
}

static void layouts_grid(ui_view_t* view, int32_t gap_h, int32_t gap_v) {
    assert(view->children != null, "layout_grid() with no children?");
    int32_t x = view->x;
    int32_t y = view->y;
    bool row_seen = false;
    for (ui_view_t** row = view->children; *row != null; row++) {
        if (!(*row)->hidden) {
            if (row_seen) { y += gap_v; }
            int32_t xc = x;
            bool col_seen = false;
            for (ui_view_t** col = (*row)->children; *col != null; col++) {
                if (!(*col)->hidden) {
                    if (col_seen) { xc += gap_h; }
                    (*col)->x = xc;
                    (*col)->y = y;
                    xc += (*col)->w;
                    col_seen = true;
                }
            }
            y += (*row)->h;
            row_seen = true;
        }
    }
}

layouts_if layouts = {
    .center     = layouts_center,
    .horizontal = layouts_horizontal,
    .vertical   = layouts_vertical,
    .grid       = layouts_grid
};
// _______________________________ messagebox.c _______________________________

#include "ut/ut.h"

static void ui_messagebox_button(ui_button_t* b) {
    ui_messagebox_t* mx = (ui_messagebox_t*)b->view.parent;
    assert(mx->view.type == ui_view_messagebox);
    mx->option = -1;
    for (int32_t i = 0; i < countof(mx->button) && mx->option < 0; i++) {
        if (b == &mx->button[i]) {
            mx->option = i;
            mx->cb(mx, i);
        }
    }
    app.show_toast(null, 0);
}

static void ui_messagebox_measure(ui_view_t* view) {
    ui_messagebox_t* mx = (ui_messagebox_t*)view;
    assert(view->type == ui_view_messagebox);
    int32_t n = 0;
    for (ui_view_t** c = view->children; c != null && *c != null; c++) { n++; }
    n--; // number of buttons
    mx->text.view.measure(&mx->text.view);
    const int32_t em_x = mx->text.view.em.x;
    const int32_t em_y = mx->text.view.em.y;
    const int32_t tw = mx->text.view.w;
    const int32_t th = mx->text.view.h;
    if (n > 0) {
        int32_t bw = 0;
        for (int32_t i = 0; i < n; i++) {
            bw += mx->button[i].view.w;
        }
        view->w = max(tw, bw + em_x * 2);
        view->h = th + mx->button[0].view.h + em_y + em_y / 2;
    } else {
        view->h = th + em_y / 2;
        view->w = tw;
    }
}

static void ui_messagebox_layout(ui_view_t* view) {
    ui_messagebox_t* mx = (ui_messagebox_t*)view;
    assert(view->type == ui_view_messagebox);
    int32_t n = 0;
    for (ui_view_t** c = view->children; c != null && *c != null; c++) { n++; }
    n--; // number of buttons
    const int32_t em_y = mx->text.view.em.y;
    mx->text.view.x = view->x;
    mx->text.view.y = view->y + em_y * 2 / 3;
    const int32_t tw = mx->text.view.w;
    const int32_t th = mx->text.view.h;
    if (n > 0) {
        int32_t bw = 0;
        for (int32_t i = 0; i < n; i++) {
            bw += mx->button[i].view.w;
        }
        // center text:
        mx->text.view.x = view->x + (view->w - tw) / 2;
        // spacing between buttons:
        int32_t sp = (view->w - bw) / (n + 1);
        int32_t x = sp;
        for (int32_t i = 0; i < n; i++) {
            mx->button[i].view.x = view->x + x;
            mx->button[i].view.y = view->y + th + em_y * 3 / 2;
            x += mx->button[i].view.w + sp;
        }
    }
}

void ui_messagebox_init_(ui_view_t* view) {
    assert(view->type == ui_view_messagebox);
    ui_messagebox_t* mx = (ui_messagebox_t*)view;
    ui_view_init(view);
    view->measure = ui_messagebox_measure;
    view->layout  = ui_messagebox_layout;
    mx->view.font = &app.fonts.H3;
    const char** opts = mx->opts;
    int32_t n = 0;
    while (opts[n] != null && n < countof(mx->button) - 1) {
        ui_button_init(&mx->button[n], opts[n], 6.0, ui_messagebox_button);
        mx->button[n].view.parent = &mx->view;
        n++;
    }
    assert(n <= countof(mx->button));
    if (n > countof(mx->button)) { n = countof(mx->button); }
    mx->children[0] = &mx->text.view;
    for (int32_t i = 0; i < n; i++) {
        mx->children[i + 1] = &mx->button[i].view;
        mx->children[i + 1]->font = mx->view.font;
        ui_view.localize(&mx->button[i].view);
    }
    mx->view.children = mx->children;
    ui_label_init_ml(&mx->text, 0.0, "%s", mx->view.text);
    mx->text.view.font = mx->view.font;
    ui_view.localize(&mx->text.view);
    mx->view.text[0] = 0;
    mx->option = -1;
}

void ui_messagebox_init(ui_messagebox_t* mx, const char* opts[],
        void (*cb)(ui_messagebox_t* m, int32_t option),
        const char* format, ...) {
    mx->view.type = ui_view_messagebox;
    mx->view.measure = ui_messagebox_measure;
    mx->view.layout  = ui_messagebox_layout;
    mx->opts = opts;
    mx->cb = cb;
    va_list vl;
    va_start(vl, format);
    str.format_va(mx->view.text, countof(mx->view.text), format, vl);
    ui_label_init_ml(&mx->text, 0.0, mx->view.text);
    va_end(vl);
    ui_messagebox_init_(&mx->view);
}
// __________________________________ nls.c ___________________________________

#include "ut/ut.h"
#include "ut/win32.h"

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
    nls_str_count_max = 1024,
    nls_str_mem_max = 64 * nls_str_count_max
};

static char nls_strings_memory[nls_str_mem_max]; // increase if overflows
static char* nls_strings_free = nls_strings_memory;
static int32_t nls_strings_count;
static const char* nls_ls[nls_str_count_max]; // localized strings
static const char* nls_ns[nls_str_count_max]; // neutral language strings

wchar_t* nls_load_string(int32_t strid, LANGID langid) {
    assert(0 <= strid && strid < countof(nls_ns));
    wchar_t* r = null;
    int32_t block = strid / 16 + 1;
    int32_t index  = strid % 16;
    HRSRC res = FindResourceExA(((HMODULE)null), RT_STRING,
        MAKEINTRESOURCE(block), langid);
//  traceln("FindResourceExA(block=%d langid=%04X)=%p", block, langid, res);
    uint8_t* memory = res == null ? null : (uint8_t*)LoadResource(null, res);
    wchar_t* ws = memory == null ? null : (wchar_t*)LockResource(memory);
//  traceln("LockResource(block=%d langid=%04X)=%p", block, langid, ws);
    if (ws != null) {
        for (int32_t i = 0; i < 16 && r == null; i++) {
            if (ws[0] != 0) {
                int32_t count = (int)ws[0];  // String size in characters.
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

static const char* nls_save_string(wchar_t* memory) {
    const char* utf8 = utf16to8(memory);
    uintptr_t n = strlen(utf8) + 1;
    assert(n > 1);
    uintptr_t left = countof(nls_strings_memory) - (
        nls_strings_free - nls_strings_memory);
    fatal_if_false(left >= n, "string_memory[] overflow");
    memcpy(nls_strings_free, utf8, n);
    const char* s = nls_strings_free;
    nls_strings_free += n;
    return s;
}

const char* nls_localize_string(int32_t strid) {
    assert(0 < strid && strid < countof(nls_ns));
    const char* r = null;
    if (0 < strid && strid < countof(nls_ns)) {
        if (nls_ls[strid] != null) {
            r = nls_ls[strid];
        } else {
            LCID lcid = GetThreadLocale();
            LANGID langid = LANGIDFROMLCID(lcid);
            wchar_t* ws = nls_load_string(strid, langid);
            if (ws == null) { // try default dialect:
                LANGID primary = PRIMARYLANGID(langid);
                langid = MAKELANGID(primary, SUBLANG_NEUTRAL);
                ws = nls_load_string(strid, langid);
            }
            if (ws != null) {
                r = nls_save_string(ws);
                nls_ls[strid] = r;
            }
        }
    }
    return r;
}

static int32_t nls_strid(const char* s) {
    int32_t strid = 0;
    for (int32_t i = 1; i < nls_strings_count && strid == 0; i++) {
        if (nls_ns[i] != null && strcmp(s, nls_ns[i]) == 0) {
            strid = i;
            nls_localize_string(strid); // to save it, ignore result
        }
    }
    return strid;
}

static const char* nls_string(int32_t strid, const char* defau1t) {
    const char* r = nls_localize_string(strid);
    return r == null ? defau1t : r;
}

const char* nls_str(const char* s) {
    int32_t id = nls_strid(s);
    return id == 0 ? s : nls_string(id, s);
}

static const char* nls_locale(void) {
    wchar_t wln[LOCALE_NAME_MAX_LENGTH + 1];
    LCID lcid = GetThreadLocale();
    int32_t n = LCIDToLocaleName(lcid, wln, countof(wln),
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

static void nls_set_locale(const char* locale) {
    wchar_t rln[LOCALE_NAME_MAX_LENGTH + 1];
    int32_t n = ResolveLocaleName(utf8to16(locale), rln, countof(rln));
    if (n == 0) {
        // TODO: log error
    } else {
        LCID lcid = LocaleNameToLCID(rln, LOCALE_ALLOW_NEUTRAL_NAMES);
        if (lcid == 0) {
            // TODO: log error
        } else {
            fatal_if_false(SetThreadLocale(lcid));
            memset((void*)nls_ls, 0, sizeof(nls_ls)); // start all over
        }
    }
}

static void nls_init(void) {
    static_assert(countof(nls_ns) % 16 == 0, "countof(ns) must be multiple of 16");
    LANGID langid = MAKELANGID(LANG_ENGLISH, SUBLANG_NEUTRAL);
    for (int32_t strid = 0; strid < countof(nls_ns); strid += 16) {
        int32_t block = strid / 16 + 1;
        HRSRC res = FindResourceExA(((HMODULE)null), RT_STRING,
            MAKEINTRESOURCE(block), langid);
        uint8_t* memory = res == null ? null : (uint8_t*)LoadResource(null, res);
        wchar_t* ws = memory == null ? null : (wchar_t*)LockResource(memory);
        if (ws == null) { break; }
        for (int32_t i = 0; i < 16; i++) {
            int32_t ix = strid + i;
            uint16_t count = ws[0];
            if (count > 0) {
                ws++;
                fatal_if_false(ws[count - 1] == 0, "use rc.exe /n");
                nls_ns[ix] = nls_save_string(ws);
                nls_strings_count = ix + 1;
//              traceln("ns[%d] := %d \"%s\"", ix, strlen(ns[ix]), ns[ix]);
                ws += count;
            } else {
                ws++;
            }
        }
    }
}

nls_if nls = {
    .init   = nls_init,
    .strid  = nls_strid,
    .str    = nls_str,
    .string = nls_string,
    .locale = nls_locale,
    .set_locale = nls_set_locale,
};

// _________________________________ slider.c _________________________________

#include "ut/ut.h"

static void ui_slider_measure(ui_view_t* view) {
    assert(view->type == ui_view_slider);
    ui_view.measure(view);
    ui_slider_t* r = (ui_slider_t*)view;
    assert(r->inc.view.w == r->dec.view.w && r->inc.view.h == r->dec.view.h);
    const int32_t em = view->em.x;
    ui_font_t f = view->font != null ? *view->font : app.fonts.regular;
    const int32_t w = (int)(view->width * view->em.x);
    r->tm = gdi.measure_text(f, ui_view.nls(view), r->vmax);
    if (w > r->tm.x) { r->tm.x = w; }
    view->w = r->dec.view.w + r->tm.x + r->inc.view.w + em * 2;
    view->h = r->inc.view.h;
}

static void ui_slider_layout(ui_view_t* view) {
    assert(view->type == ui_view_slider);
    ui_slider_t* r = (ui_slider_t*)view;
    assert(r->inc.view.w == r->dec.view.w && r->inc.view.h == r->dec.view.h);
    const int32_t em = view->em.x;
    r->dec.view.x = view->x;
    r->dec.view.y = view->y;
    r->inc.view.x = view->x + r->dec.view.w + r->tm.x + em * 2;
    r->inc.view.y = view->y;
}

static void ui_slider_paint(ui_view_t* view) {
    assert(view->type == ui_view_slider);
    ui_slider_t* r = (ui_slider_t*)view;
    gdi.push(view->x, view->y);
    gdi.set_clip(view->x, view->y, view->w, view->h);
    const int32_t em = view->em.x;
    const int32_t em2  = max(1, em / 2);
    const int32_t em4  = max(1, em / 8);
    const int32_t em8  = max(1, em / 8);
    const int32_t em16 = max(1, em / 16);
    gdi.set_brush(gdi.brush_color);
    ui_pen_t pen_grey45 = gdi.create_pen(colors.dkgray3, em16);
    gdi.set_pen(pen_grey45);
    gdi.set_brush_color(colors.dkgray3);
    const int32_t x = view->x + r->dec.view.w + em2;
    const int32_t y = view->y;
    const int32_t w = r->tm.x + em;
    const int32_t h = view->h;
    gdi.rounded(x - em8, y, w + em4, h, em4, em4);
    gdi.gradient(x, y, w, h / 2,
        colors.dkgray3, colors.btn_gradient_darker, true);
    gdi.gradient(x, y + h / 2, w, view->h - h / 2,
        colors.btn_gradient_darker, colors.dkgray3, true);
    gdi.set_brush_color(colors.dkgreen);
    ui_pen_t pen_grey30 = gdi.create_pen(colors.dkgray1, em16);
    gdi.set_pen(pen_grey30);
    const double range = (double)r->vmax - (double)r->vmin;
    double vw = (double)(r->tm.x + em) * (r->value - r->vmin) / range;
    gdi.rect(x, view->y, (int32_t)(vw + 0.5), view->h);
    gdi.x += r->dec.view.w + em;
    const char* format = nls.str(view->text);
    gdi.text(format, r->value);
    gdi.set_clip(0, 0, 0, 0);
    gdi.delete_pen(pen_grey30);
    gdi.delete_pen(pen_grey45);
    gdi.pop();
}

static void ui_slider_mouse(ui_view_t* view, int32_t message, int32_t f) {
    if (!view->hidden && !view->disabled) {
        assert(view->type == ui_view_slider);
        ui_slider_t* r = (ui_slider_t*)view;
        bool drag = message == ui.message.mouse_move &&
            (f & (ui.mouse.button.left|ui.mouse.button.right)) != 0;
        if (message == ui.message.left_button_pressed ||
            message == ui.message.right_button_pressed || drag) {
            const int32_t x = app.mouse.x - view->x - r->dec.view.w;
            const int32_t y = app.mouse.y - view->y;
            const int32_t x0 = view->em.x / 2;
            const int32_t x1 = r->tm.x + view->em.x;
            if (x0 <= x && x < x1 && 0 <= y && y < view->h) {
                app.focus = view;
                const double range = (double)r->vmax - (double)r->vmin;
                double v = ((double)x - x0) * range / (double)(x1 - x0 - 1);
                int32_t vw = (int32_t)(v + r->vmin + 0.5);
                r->value = min(max(vw, r->vmin), r->vmax);
                if (r->cb != null) { r->cb(r); }
                ui_view.invalidate(view);
            }
        }
    }
}

static void ui_slider_inc_dec_value(ui_slider_t* r, int32_t sign, int32_t mul) {
    if (!r->view.hidden && !r->view.disabled) {
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
            ui_view.invalidate(&r->view);
        }
    }
}

static void ui_slider_inc_dec(ui_button_t* b) {
    ui_slider_t* r = (ui_slider_t*)b->view.parent;
    if (!r->view.hidden && !r->view.disabled) {
        int32_t sign = b == &r->inc ? +1 : -1;
        int32_t mul = app.shift && app.ctrl ? 1000 :
            app.shift ? 100 : app.ctrl ? 10 : 1;
        ui_slider_inc_dec_value(r, sign, mul);
    }
}

static void ui_slider_every_100ms(ui_view_t* view) { // 100ms
    assert(view->type == ui_view_slider);
    ui_slider_t* r = (ui_slider_t*)view;
    if (r->view.hidden || r->view.disabled) {
        r->time = 0;
    } else if (!r->dec.view.armed && !r->inc.view.armed) {
        r->time = 0;
    } else {
        if (r->time == 0) {
            r->time = app.now;
        } else if (app.now - r->time > 1.0) {
            const int32_t sign = r->dec.view.armed ? -1 : +1;
            int32_t s = (int)(app.now - r->time + 0.5);
            int32_t mul = s >= 1 ? 1 << (s - 1) : 1;
            const int64_t range = (int64_t)r->vmax - r->vmin;
            if (mul > range / 8) { mul = (int32_t)(range / 8); }
            ui_slider_inc_dec_value(r, sign, max(mul, 1));
        }
    }
}

void ui_slider_init_(ui_view_t* view) {
    assert(view->type == ui_view_slider);
    ui_view_init(view);
    ui_view.set_text(view, view->text);
    view->mouse       = ui_slider_mouse;
    view->measure     = ui_slider_measure;
    view->layout      = ui_slider_layout;
    view->paint       = ui_slider_paint;
    view->every_100ms = ui_slider_every_100ms;
    ui_slider_t* r = (ui_slider_t*)view;
    r->buttons[0] = &r->dec.view;
    r->buttons[1] = &r->inc.view;
    r->buttons[2] = null;
    r->view.children = r->buttons;
    // Heavy Minus Sign
    ui_button_init(&r->dec, "\xE2\x9E\x96", 0, ui_slider_inc_dec);
    // Heavy Plus Sign
    ui_button_init(&r->inc, "\xE2\x9E\x95", 0, ui_slider_inc_dec);
    static const char* accel =
        "Accelerate by holding Ctrl x10 Shift x100 and Ctrl+Shift x1000";
    strprintf(r->inc.view.tip, "%s", accel);
    strprintf(r->dec.view.tip, "%s", accel);
    r->dec.view.parent = &r->view;
    r->inc.view.parent = &r->view;
    ui_view.localize(&r->view);
}

void ui_slider_init(ui_slider_t* r, const char* label, double ems,
        int32_t vmin, int32_t vmax, void (*cb)(ui_slider_t* r)) {
    static_assert(offsetof(ui_slider_t, view) == 0, "offsetof(.view)");
    assert(ems >= 3.0, "allow 1em for each of [-] and [+] buttons");
    r->view.type = ui_view_slider;
    strprintf(r->view.text, "%s", label);
    r->cb = cb;
    r->view.width = ems;
    r->vmin = vmin;
    r->vmax = vmax;
    r->value = vmin;
    ui_slider_init_(&r->view);
}
// __________________________________ view.c __________________________________

#include "ut/ut.h"

static void ui_view_invalidate(const ui_view_t* view) {
    ui_rect_t rc = { view->x, view->y, view->w, view->h};
    rc.x -= view->em.x;
    rc.y -= view->em.y;
    rc.w += view->em.x * 2;
    rc.h += view->em.y * 2;
    app.invalidate(&rc);
}

static const char* ui_view_nls(ui_view_t* view) {
    return view->strid != 0 ?
        nls.string(view->strid, view->text) : view->text;
}

static void ui_view_measure_text(ui_view_t* view) {
    ui_font_t f = view->font != null ? *view->font : app.fonts.regular;
    view->em = gdi.get_em(f);
    view->baseline = gdi.baseline(f);
    view->descent  = gdi.descent(f);
    if (view->text[0] != 0) {
        view->w = (int32_t)(view->em.x * view->width + 0.5);
        ui_point_t mt = { 0 };
        if (view->type == ui_view_text && ((ui_label_t*)view)->multiline) {
            int32_t w = (int)(view->width * view->em.x + 0.5);
            mt = gdi.measure_multiline(f, w == 0 ? -1 : w, ui_view.nls(view));
        } else {
            mt = gdi.measure_text(f, ui_view.nls(view));
        }
        view->h = mt.y;
        view->w = max(view->w, mt.x);
    }
}

static void ui_view_measure(ui_view_t* view) {
    ui_view.measure(view);
}

static void ui_view_set_text(ui_view_t* view, const char* text) {
    int32_t n = (int32_t)strlen(text);
    strprintf(view->text, "%s", text);
    view->strid = 0; // next call to nls() will localize this text
    for (int32_t i = 0; i < n; i++) {
        if (text[i] == '&' && i < n - 1 && text[i + 1] != '&') {
            view->shortcut = text[i + 1];
            break;
        }
    }
}

static void ui_view_localize(ui_view_t* view) {
    if (view->text[0] != 0) {
        view->strid = nls.strid(view->text);
    }
}

static void ui_view_hovering(ui_view_t* view, bool start) {
    static ui_text(btn_tooltip,  "");
    if (start && app.animating.view == null && view->tip[0] != 0 &&
       !ui_view.is_hidden(view)) {
        strprintf(btn_tooltip.view.text, "%s", nls.str(view->tip));
        btn_tooltip.view.font = &app.fonts.H1;
        int32_t y = app.mouse.y - view->em.y;
        // enough space above? if not show below
        if (y < view->em.y) { y = app.mouse.y + view->em.y * 3 / 2; }
        y = min(app.crc.h - view->em.y * 3 / 2, max(0, y));
        app.show_tooltip(&btn_tooltip.view, app.mouse.x, y, 0);
    } else if (!start && app.animating.view == &btn_tooltip.view) {
        app.show_tooltip(null, -1, -1, 0);
    }
}

static bool ui_view_is_keyboard_shortcut(ui_view_t* view, int32_t key) {
    // Supported keyboard shortcuts are ASCII characters only for now
    // If there is not focused UI control in Alt+key [Alt] is optional.
    // If there is focused control only Alt+Key is accepted as shortcut
    char ch = 0x20 <= key && key <= 0x7F ? (char)toupper(key) : 0x00;
    bool need_alt = app.focus != null && app.focus != view;
    bool keyboard_shortcut = ch != 0x00 && view->shortcut != 0x00 &&
         (app.alt || !need_alt) && toupper(view->shortcut) == ch;
    return keyboard_shortcut;
}

static bool ui_view_is_hidden(const ui_view_t* view) {
    bool hidden = view->hidden;
    while (!hidden && view->parent != null) {
        view = view->parent;
        hidden = view->hidden;
    }
    return hidden;
}

static bool ui_view_is_disabled(const ui_view_t* view) {
    bool disabled = view->disabled;
    while (!disabled && view->parent != null) {
        view = view->parent;
        disabled = view->disabled;
    }
    return disabled;
}

static void ui_view_init_children(ui_view_t* view) {
    for (ui_view_t** c = view->children; c != null && *c != null; c++) {
        if ((*c)->init != null) { (*c)->init(*c); (*c)->init = null; }
        if ((*c)->font == null) { (*c)->font = &app.fonts.regular; }
        if ((*c)->em.x == 0 || (*c)->em.y == 0) {
            (*c)->em = gdi.get_em(*view->font);
        }
        if ((*c)->text[0] != 0) { ui_view.localize(*c); }
        ui_view_init_children(*c);
    }
}

void ui_view_init(ui_view_t* view) {
    view->measure      = ui_view_measure;
    view->hovering     = ui_view_hovering;
    view->hover_delay = 1.5;
    view->is_keyboard_shortcut = ui_view_is_keyboard_shortcut;
}

ui_view_if ui_view = {
    .set_text      = ui_view_set_text,
    .invalidate    = ui_view_invalidate,
    .measure       = ui_view_measure_text,
    .nls           = ui_view_nls,
    .localize      = ui_view_localize,
    .is_hidden     = ui_view_is_hidden,
    .is_disabled   = ui_view_is_disabled,
    .init_children = ui_view_init_children
};

#endif // ui_implementation

