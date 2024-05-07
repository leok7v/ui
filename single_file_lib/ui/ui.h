#ifndef ui_definition
#define ui_definition

// _________________________________ ut_std.h _________________________________

#include <ctype.h>
#include <errno.h>
#include <malloc.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Type aliases for floating-point types similar to <stdint.h>
typedef float  fp32_t;
typedef double fp64_t;
// "long fp64_t" is required by C standard but the bitness
// of it is not specified.

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

#define ut_stackalloc(n) (_Pragma("warning(suppress: 6255 6263)") alloca(n))
// ________________________________ ui_core.h _________________________________

#include "ut/ut_std.h"


typedef struct ui_point_s { int32_t x, y; } ui_point_t;
typedef struct ui_rect_s { int32_t x, y, w, h; } ui_rect_t;

typedef struct ui_window_s* ui_window_t;
typedef struct ui_icon_s*   ui_icon_t;
typedef struct ui_canvas_s* ui_canvas_t;
typedef struct ui_bitmap_s* ui_bitmap_t;
typedef struct ui_font_s*   ui_font_t;
typedef struct ui_brush_s*  ui_brush_t;
typedef struct ui_pen_s*    ui_pen_t;
typedef struct ui_cursor_s* ui_cursor_t;
typedef struct ui_region_s* ui_region_t;

typedef uintptr_t ui_timer_t; // timer not the same as "id" in set_timer()!

typedef struct ui_image_s { // TODO: ui_ namespace
    int32_t w; // width
    int32_t h; // height
    int32_t bpp;    // "components" bytes per pixel
    int32_t stride; // bytes per scanline rounded up to: (w * bpp + 3) & ~3
    ui_bitmap_t bitmap;
    void* pixels;
} ui_image_t;

typedef struct ui_dpi_s { // max(dpi_x, dpi_y)
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

// in inches (because monitors customary are)
// it is not in points (1/72 inch) like font size
// because it is awkward to express large are
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

// ui_gaps_t are used for padding and insets and expressed
// in partial "em"s not in pixels, inches or points.
// Pay attention that "em" is not square. "M" measurement
// for most fonts are em.w = 0.5 * em.h

typedef struct ui_gaps_s { // in partial "em"s
    fp32_t left;
    fp32_t top;
    fp32_t right;
    fp32_t bottom;
} ui_gaps_t;

typedef struct ui_s {
    const int32_t infinity; // = INT32_MAX, look better
    struct { // align bitset
        int32_t const center; // = 0, default
        int32_t const left;   // left|top, left|bottom, right|bottom
        int32_t const top;
        int32_t const right;  // right|top, right|bottom
        int32_t const bottom;
    } const align;
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
    } const visibility;
    struct { // message:
        int32_t const character; // translated from key pressed/released to utf8
        int32_t const key_pressed;
        int32_t const key_released;
        int32_t const left_button_pressed;
        int32_t const left_button_released;
        int32_t const right_button_pressed;
        int32_t const right_button_released;
        int32_t const mouse_move;
        int32_t const mouse_hover;
        int32_t const left_double_click;
        int32_t const right_double_click;
        int32_t const animate;
        int32_t const opening;
        int32_t const closing;
        // wp: 0,1,2 (left, middle, right) button index, lp: client x,y
        int32_t const tap;
        int32_t const dtap;
        int32_t const press;
   } const message;
   struct { // mouse buttons bitset mask
        struct {
            int32_t const left;
            int32_t const right;
        } button;
    } const mouse;
    struct { // window decorations hit test results
        int32_t const error;            // -2
        int32_t const transparent;      // -1
        int32_t const nowhere;          // 0
        int32_t const client;           // 1
        int32_t const caption;          // 2
        int32_t const system_menu;      // 3
        int32_t const grow_box;         // 4
        int32_t const menu;             // 5
        int32_t const horizontal_scroll;// 6
        int32_t const vertical_scroll;  // 7
        int32_t const min_button;       // 8
        int32_t const max_button;       // 9
        int32_t const left;             // 10
        int32_t const right;            // 11
        int32_t const top;              // 12
        int32_t const top_left;         // 13
        int32_t const top_right;        // 14
        int32_t const bottom;           // 15
        int32_t const bottom_left;      // 16
        int32_t const bottom_right;     // 17
        int32_t const border;           // 18
        int32_t const object;           // 19
        int32_t const close;            // 20
        int32_t const help;             // 21
    } const hit_test;
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
    } const key;
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
    } const folder;
    bool (*point_in_rect)(const ui_point_t* p, const ui_rect_t* r);
    // intersect_rect(null, r0, r1) and intersect_rect(r0, r0, r1) supported.
    bool (*intersect_rect)(ui_rect_t* destination, const ui_rect_t* r0,
                                                   const ui_rect_t* r1);
    int32_t (*gaps_em2px)(int32_t em, fp32_t ratio);
} ui_if;

extern ui_if ui;

// ui_gaps_t in "em"s:
//
// The reason is that UI fonts may become larger smaller
// for accessibility reasons with the same display
// density in DPIs. Humanoid would expect the gaps around
// larger font text to grow with font size increase.
// SwingUI and MacOS is using "pt" for padding which does
// not account to font size changes. MacOS does wierd stuff
// with font increase - it actually decreases GPU resolution.
// Android uses "dp" which is pretty much the same as scaled
// "pixels" on MacOS. Windows used to use "dialog units" which
// is font size based and this is where the idea is inherited from.


// _______________________________ ui_colors.h ________________________________

#include "ut/ut_std.h"


typedef uint64_t ui_color_t; // top 2 bits determine color format

/* TODO: make ui_color_t uint64_t RGBA remove pens and brushes
         support upto 16-16-16-15(A)bit per pixel color
         components with 'transparent/hollow' bit
*/

#define ui_color_mask        ((ui_color_t)0xC000000000000000ULL)
#define ui_color_undefined   ((ui_color_t)0x8000000000000000ULL)
#define ui_color_transparent ((ui_color_t)0x4000000000000000ULL)
#define ui_color_hdr         ((ui_color_t)0xC000000000000000ULL)

#define ui_color_is_8bit(c)         (((c) & ui_color_mask) == 0)
#define ui_color_is_hdr(c)          (((c) & ui_color_mask) == ui_color_hdr)
#define ui_color_is_undefined(c)    (((c) & ui_color_mask) == ui_color_undefined)
#define ui_color_is_transparent(c)  ((((c) & ui_color_mask) == ui_color_transparent) && \
                                    (((c) & ~ui_color_mask) == 0))
// if any other special colors or formats need to be introduced
// (c) & ~ui_color_mask) has 2^62 possible extensions bits

// ui_color_hdr A - 14 bit, R,G,B - 16 bit, all in range [0..0xFFFF]
#define ui_color_hdr_a(c)    ((uint16_t)((((c) >> 48) & 0x3FFF) << 2))
#define ui_color_hdr_r(c)    ((uint16_t)(((c) >>   0) & 0xFFFF))
#define ui_color_hdr_g(c)    ((uint16_t)(((c) >>  16) & 0xFFFF))
#define ui_color_hdr_b(c)    ((uint16_t)(((c) >>  32) & 0xFFFF))

#define ui_color_a(c)        ((uint8_t)(((c) >> 24) & 0xFFU))
#define ui_color_r(c)        ((uint8_t)(((c) >>  0) & 0xFFU))
#define ui_color_g(c)        ((uint8_t)(((c) >>  8) & 0xFFU))
#define ui_color_b(c)        ((uint8_t)(((c) >> 16) & 0xFFU))

#define ui_color_rgb(c)      ((uint32_t)((c) & 0x00FFFFFFU))
#define ui_color_rgba(c)     ((uint32_t)((c) & 0xFFFFFFFFU))

#define ui_rgb(r,g,b) ((ui_color_t)(((uint8_t)(r) |    \
                      ((uint16_t)((uint8_t)(g))<<8)) | \
                     (((uint32_t)(uint8_t)(b))<<16)))

#define ui_rgba(r, g, b, a) (ui_color_t)((ui_rgb(r, g, b)) | \
                                       (((uint8_t)a) << 24))

typedef struct ui_colors_s {
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
    // miscelaneous:
    const int32_t orange;
    const int32_t dkgreen;
    const int32_t pink;
    const int32_t ochre;
    const int32_t gold;
    const int32_t teal;
    const int32_t wheat;
    const int32_t tan;
    const int32_t brown;
    const int32_t maroon;
    const int32_t barbie_pink;
    const int32_t steel_pink;
    const int32_t salmon_pink;
    const int32_t gainsboro;
    const int32_t light_gray;
    const int32_t silver;
    const int32_t dark_gray;
    const int32_t dim_gray;
    const int32_t light_slate_gray;
    const int32_t slate_gray;
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

    /* Named colors */

    /* Main Panel Backgrounds */
    const int32_t ennui_black; // rgb(18, 18, 18) 0x121212
    const int32_t charcoal;
    const int32_t onyx;
    const int32_t gunmetal;
    const int32_t jet_black;
    const int32_t outer_space;
    const int32_t eerie_black;
    const int32_t oil;
    const int32_t black_coral;
    const int32_t obsidian;

    /* Secondary Panels or Sidebars */
    const int32_t raisin_black;
    const int32_t dark_charcoal;
    const int32_t dark_jungle_green;
    const int32_t pine_tree;
    const int32_t rich_black;
    const int32_t eclipse;
    const int32_t cafe_noir;

    /* Flat Buttons */
    const int32_t prussian_blue;
    const int32_t midnight_green;
    const int32_t charleston_green;
    const int32_t rich_black_fogra;
    const int32_t dark_liver;
    const int32_t dark_slate_gray;
    const int32_t black_olive;
    const int32_t cadet;

    /* Button highlights (hover) */
    const int32_t dark_sienna;
    const int32_t bistre_brown;
    const int32_t dark_puce;
    const int32_t wenge;

    /* Raised button effects */
    const int32_t dark_scarlet;
    const int32_t burnt_umber;
    const int32_t caput_mortuum;
    const int32_t barn_red;

    /* Text and Icons */
    const int32_t platinum;
    const int32_t anti_flash_white;
    const int32_t silver_sand;
    const int32_t quick_silver;

    /* Links and Selections */
    const int32_t dark_powder_blue;
    const int32_t sapphire_blue;
    const int32_t international_klein_blue;
    const int32_t zaffre;

    /* Additional Colors */
    const int32_t fish_belly;
    const int32_t rusty_red;
    const int32_t falu_red;
    const int32_t cordovan;
    const int32_t dark_raspberry;
    const int32_t deep_magenta;
    const int32_t byzantium;
    const int32_t amethyst;
    const int32_t wisteria;
    const int32_t lavender_purple;
    const int32_t opera_mauve;
    const int32_t mauve_taupe;
    const int32_t rich_lavender;
    const int32_t pansy_purple;
    const int32_t violet_eggplant;
    const int32_t jazzberry_jam;
    const int32_t dark_orchid;
    const int32_t electric_purple;
    const int32_t sky_magenta;
    const int32_t brilliant_rose;
    const int32_t fuchsia_purple;
    const int32_t french_raspberry;
    const int32_t wild_watermelon;
    const int32_t neon_carrot;
    const int32_t burnt_orange;
    const int32_t carrot_orange;
    const int32_t tiger_orange;
    const int32_t giant_onion;
    const int32_t rust;
    const int32_t copper_red;
    const int32_t dark_tangerine;
    const int32_t bright_marigold;
    const int32_t bone;

    /* Earthy Tones */
    const int32_t sienna;
    const int32_t sandy_brown;
    const int32_t golden_brown;
    const int32_t camel;
    const int32_t burnt_sienna;
    const int32_t khaki;
    const int32_t dark_khaki;

    /* Greens */
    const int32_t fern_green;
    const int32_t moss_green;
    const int32_t myrtle_green;
    const int32_t pine_green;
    const int32_t jungle_green;
    const int32_t sacramento_green;

    /* Blues */
    const int32_t yale_blue;
    const int32_t cobalt_blue;
    const int32_t persian_blue;
    const int32_t royal_blue;
    const int32_t iceberg;
    const int32_t blue_yonder;

    /* Miscellaneous */
    const int32_t cocoa_brown;
    const int32_t cinnamon_satin;
    const int32_t fallow;
    const int32_t cafe_au_lait;
    const int32_t liver;
    const int32_t shadow;
    const int32_t cool_grey;
    const int32_t payne_grey;

    /* Lighter Tones for Contrast */
    const int32_t timberwolf;
    const int32_t silver_chalice;
    const int32_t roman_silver;

    /* Dark Mode Specific Highlights */
    const int32_t electric_lavender;
    const int32_t magenta_haze;
    const int32_t cyber_grape;
    const int32_t purple_navy;
    const int32_t liberty;
    const int32_t purple_mountain_majesty;
    const int32_t ceil;
    const int32_t moonstone_blue;
    const int32_t independence;
} ui_colors_t;

extern ui_colors_t ui_colors;

// TODO:
// https://ankiewicz.com/colors/
// https://htmlcolorcodes.com/color-names/
// it would be super cool to implement a plethora of palettes
// with named colors and app "themes" that can be switched

// _________________________________ ui_gdi.h _________________________________

#include "ut/ut_std.h"


// Graphic Device Interface (selected parts of Windows GDI)

enum {  // TODO: ui_ namespace and into gdi int32_t const
    ui_gdi_font_quality_default = 0,
    ui_gdi_font_quality_draft = 1,
    ui_gdi_font_quality_proof = 2, // anti-aliased w/o ClearType rainbows
    ui_gdi_font_quality_nonantialiased = 3,
    ui_gdi_font_quality_antialiased = 4,
    ui_gdi_font_quality_cleartype = 5,
    ui_gdi_font_quality_cleartype_natural = 6
};

typedef struct {
    ui_brush_t  brush_color;
    ui_brush_t  brush_hollow;
    ui_pen_t pen_hollow;
    ui_region_t clip;
    void (*init)(void);
    uint32_t (*color_rgb)(ui_color_t c); // rgb color
    // bpp bytes (not bits!) per pixel. bpp = -3 or -4 does not swap RGB to BRG:
    void (*image_init)(ui_image_t* image, int32_t w, int32_t h, int32_t bpp,
        const uint8_t* pixels);
    void (*image_init_rgbx)(ui_image_t* image, int32_t w, int32_t h,
        int32_t bpp, const uint8_t* pixels); // sets all alphas to 0xFF
    void (*image_dispose)(ui_image_t* image);
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
    void (*push)(int32_t x, int32_t y); // also calls SaveDC(ui_app.canvas)
    void (*pop)(void); // also calls RestoreDC(-1, ui_app.canvas)
    void (*pixel)(int32_t x, int32_t y, ui_color_t c);
    ui_point_t (*move_to)(int32_t x, int32_t y); // returns previous (x, y)
    void (*line)(int32_t x, int32_t y); // line to x, y with ui_gdi.pen moves x, y
    void (*frame)(int32_t x, int32_t y, int32_t w, int32_t h); // ui_gdi.pen only
    void (*rect)(int32_t x, int32_t y, int32_t w, int32_t h);  // ui_gdi.pen & brush
    void (*fill)(int32_t x, int32_t y, int32_t w, int32_t h);  // ui_gdi.brush only
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
        ui_image_t* image, fp64_t alpha);
    void (*draw_image)(int32_t x, int32_t y, int32_t w, int32_t h,
        ui_image_t* image);
    void (*draw_icon)(int32_t x, int32_t y, int32_t w, int32_t h,
        ui_icon_t icon);
    // text:
    void (*cleartype)(bool on);
    void (*font_smoothing_contrast)(int32_t c); // [1000..2202] or -1 for 1400 default
    ui_font_t (*create_font)(const char* family, int32_t height, int32_t quality);
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
    ui_point_t (*get_em)(ui_font_t f);  // pixel size of glyph "M" close to square
    // get_em(f) returns { "M".w, "M".h - f.descent - (f.height - f.baseline)};
    ui_point_t (*measure_text)(ui_font_t f, const char* format, ...);
    // width can be -1 which measures text with "\n" or
    // positive number of pixels
    ui_point_t (*measure_multiline)(ui_font_t f, int32_t w, const char* format, ...);
    fp64_t height_multiplier; // see line_spacing()
    fp64_t (*line_spacing)(fp64_t height_multiplier); // default 1.0
    int32_t x; // incremented by text, print
    int32_t y; // incremented by textln, println
    // proportional:
    void (*vtext)(const char* format, va_list vl); // x += width
    void (*text)(const char* format, ...);         // x += width
    // ui_gdi.y += height * line_spacing
    void (*vtextln)(const char* format, va_list vl);
    void (*textln)(const char* format, ...);
    // mono:
    void (*vprint)(const char* format,va_list vl); // x += width
    void (*print)(const char* format, ...);        // x += width
    // ui_gdi.y += height * line_spacing
    void (*vprintln)(const char* format, va_list vl);
    void (*println)(const char* format, ...);
    // multiline(null, format, ...) only increments ui_gdi.y
    ui_point_t (*multiline)(int32_t width, const char* format, ...);
} ui_gdi_if;

extern ui_gdi_if ui_gdi;

// _______________________________ ui_glyphs.h ________________________________

#include "ut/ut_std.h"

// Square Four Corners (caption full screen button)
// https://www.compart.com/en/unicode/U+26F6
#define ui_glyph_square_four_corners                    "\xE2\x9B\xB6"

// White Large Square (caption maximize button)
// https://www.compart.com/en/unicode/U+2B1C
#define ui_glyph_white_large_square                     "\xE2\xAC\x9C"

// N-Ary Times Operator (caption close button)
// https://www.compart.com/en/unicode/U+2A09
#define ui_glyph_n_ary_times_operator                   "\xE2\xA8\x89"

// Heavy Minus Sign (caption minimize button)
// https://www.compart.com/en/unicode/U+2796
#define ui_glyph_heavy_minus_sign                       "\xE2\x9E\x96"

// Heavy Plus Sign
// https://www.compart.com/en/unicode/U+2795
#define ui_glyph_heavy_plus_sign                        "\xE2\x9E\x95"

// Heavy Multiplication X
// https://www.compart.com/en/unicode/U+2716
#define ui_glyph_heavy_multiplication_x                 "\xE2\x9C\x96"

// Multiplication Sign
// https://www.compart.com/en/unicode/U+00D7
#define ui_glyph_multiplication_sign                    "\xC3\x97"

// Trigram For Heaven (caption menu button)
// https://www.compart.com/en/unicode/U+2630
#define ui_glyph_trigram_for_heaven                     "\xE2\x98\xB0"

// Braille Pattern Dots-12345678 (tool bar drag handle like: msvc toolbars)
// https://www.compart.com/en/unicode/U+28FF
#define ui_glyph_braille_pattern_dots_12345678          "\xE2\xA3\xBF"

// White Square with Upper Left Quadrant
// https://www.compart.com/en/unicode/U+25F0
#define ui_glyph_white_square_with_upper_left_quadrant "\xE2\x97\xB0"

// White Square with Lower Left Quadrant
// https://www.compart.com/en/unicode/U+25F1
#define ui_glyph_white_square_with_lower_left_quadrant "\xE2\x97\xB1"

// White Square with Lower Right Quadrant
// https://www.compart.com/en/unicode/U+25F2
#define ui_glyph_white_square_with_lower_right_quadrant "\xE2\x97\xB2"

// White Square with Upper Right Quadrant
// https://www.compart.com/en/unicode/U+25F3
#define ui_glyph_white_square_with_upper_right_quadrant "\xE2\x97\xB3"

// White Square with Vertical Bisecting Line
// https://www.compart.com/en/unicode/U+25EB
#define ui_glyph_white_square_with_vertical_bisecting_line "\xE2\x97\xAB"

// Squared Minus (White Square with Horizontal Bisecting Line)
// https://www.compart.com/en/unicode/U+229F
#define ui_glyph_squared_minus                          "\xE2\x8A\x9F"

// North East and South West Arrow
// https://www.compart.com/en/unicode/U+2922
#define ui_glyph_north_east_and_south_west_arrow        "\xE2\xA4\xA2"

// South East Arrow to Corner
// https://www.compart.com/en/unicode/U+21F2
#define ui_glyph_south_east_white_arrow_to_corner       "\xE2\x87\xB2"

// North West Arrow to Corner
// https://www.compart.com/en/unicode/U+21F1
#define ui_glyph_north_west_white_arrow_to_corner       "\xE2\x87\xB1"

// Leftwards Arrow to Bar
// https://www.compart.com/en/unicode/U+21E6
#define ui_glyph_leftwards_white_arrow_to_bar           "\xE2\x87\xA6"

// Rightwards Arrow to Bar
// https://www.compart.com/en/unicode/U+21E8
#define ui_glyph_rightwards_white_arrow_to_bar          "\xE2\x87\xA8"

// Upwards White Arrow
// https://www.compart.com/en/unicode/U+21E7
#define ui_glyph_upwards_white_arrow                    "\xE2\x87\xA7"

// Downwards White Arrow
// https://www.compart.com/en/unicode/U+21E9
#define ui_glyph_downwards_white_arrow                  "\xE2\x87\xA9"

// Leftwards White Arrow
// https://www.compart.com/en/unicode/U+21E4
#define ui_glyph_leftwards_white_arrow                  "\xE2\x87\xA4"

// Rightwards White Arrow
// https://www.compart.com/en/unicode/U+21E5
#define ui_glyph_rightwards_white_arrow                 "\xE2\x87\xA5"

// Upwards White Arrow on Pedestal
// https://www.compart.com/en/unicode/U+21EB
#define ui_glyph_upwards_white_arrow_on_pedestal        "\xE2\x87\xAB"

// Braille Pattern Dots-678
// https://www.compart.com/en/unicode/U+28E0
#define ui_glyph_3_dots_tiny_right_bottom_triangle      "\xE2\xA3\xA0"

// Braille Pattern Dots-2345678
// https://www.compart.com/en/unicode/U+28FE
// Combining the two into:
#define ui_glyph_dotted_right_bottom_triangle           "\xE2\xA3\xA0\xE2\xA3\xBE"

// Two Joined Squares
// https://www.compart.com/en/unicode/U+29C9
#define ui_glyph_two_joined_squares                     "\xE2\xA7\x89"

// Upper Right Drop-Shadowed White Square
// https://www.compart.com/en/unicode/U+2750
#define ui_glyph_upper_right_drop_shadowed_white_square "\xE2\x9D\x90"

// No-Break Space (NBSP)
// https://www.compart.com/en/unicode/U+00A0
#define ui_glyph_nbsp                                  "\xC2\xA0"

// Infinity
// https://www.compart.com/en/unicode/U+221E
#define ui_glyph_infinity                              "\xE2\x88\x9E"

// Black Large Circle
// https://www.compart.com/en/unicode/U+2B24
#define ui_glyph_black_large_circle                    "\xE2\xAC\xA4"

// Heavy Leftwards Arrow with Equilateral Arrowhead
// https://www.compart.com/en/unicode/U+1F818
#define ui_glyph_heavy_leftwards_arrow_with_equilateral_arrowhead           "\xF0\x9F\xA0\x98"

// Heavy Rightwards Arrow with Equilateral Arrowhead
// https://www.compart.com/en/unicode/U+1F81A
#define ui_glyph_heavy_rightwards_arrow_with_equilateral_arrowhead          "\xF0\x9F\xA0\x9A"

// Heavy Leftwards Arrow with Large Equilateral Arrowhead
// https://www.compart.com/en/unicode/U+1F81C
#define ui_glyph_heavy_leftwards_arrow_with_large_equilateral_arrowhead     "\xF0\x9F\xA0\x9C"

// Heavy Rightwards Arrow with Large Equilateral Arrowhead
// https://www.compart.com/en/unicode/U+1F81E
#define ui_glyph_heavy_rightwards_arrow_with_large_equilateral_arrowhead    "\xF0\x9F\xA0\x9E"

// CJK Unified Ideograph-5973: Kanji Onna "Female"
// https://www.compart.com/en/unicode/U+5973
#define ui_glyph_kanji_onna_female                                          "\xE2\xBC\xA5"

// Leftwards Arrow
// https://www.compart.com/en/unicode/U+2190
#define ui_glyph_leftward_arrow                                             "\xE2\x86\x90"

// Upwards Arrow
// https://www.compart.com/en/unicode/U+2191
#define ui_glyph_upwards_arrow                                              "\xE2\x86\x91"

// Rightwards Arrow
// https://www.compart.com/en/unicode/U+2192
#define ui_glyph_rightwards_arrow                                           "\xE2\x86\x92"

// Downwards Arrow
// https://www.compart.com/en/unicode/U+2193
#define ui_glyph_downwards_arrow                                            "\xE2\x86\x93"
// ________________________________ ui_view.h _________________________________

#include "ut/ut_std.h"


enum ui_view_type_t {
    ui_view_container = 'vwct',
    ui_view_label     = 'vwlb',
    ui_view_mbx       = 'vwmb',
    ui_view_button    = 'vwbt',
    ui_view_toggle    = 'vwtg',
    ui_view_slider    = 'vwsl',
    ui_view_text      = 'vwtx',
    ui_view_span      = 'vwhs',
    ui_view_list      = 'vwvs',
    ui_view_spacer    = 'vwsp',
    ui_view_scroll    = 'vwsc'
};

typedef struct ui_view_s ui_view_t;

typedef struct ui_view_s {
    enum ui_view_type_t type;
    void (*init)(ui_view_t* view); // called once before first layout
    ui_view_t* parent;
    ui_view_t* child; // first child, circular doubly linked list
    ui_view_t* prev;  // left or top sibling
    ui_view_t* next;  // right or top sibling
    int32_t x;
    int32_t y;
    int32_t w;
    int32_t h;
    ui_gaps_t insets;
    ui_gaps_t padding;
    int32_t align; // see ui.alignment values
    int32_t max_w; // > 0 maximum width in pixels the view agrees to
    int32_t max_h; // > 0 maximum height in pixels
    fp32_t  min_w_em; // > 0 minimum width  of a view in "em"s
    fp32_t  min_h_em; // > 0 minimum height of a view in "em"s
    char text[2048];
    ui_icon_t icon; // used instead of text if != null
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
    void (*click)(ui_view_t* view); // interpretation depends on a view
    void (*callback)(ui_view_t* b); // view state change callback
    void (*mouse)(ui_view_t* view, int32_t message, int64_t flags);
    void (*mouse_wheel)(ui_view_t* view, int32_t dx, int32_t dy); // touchpad scroll
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
    void (*key_pressed)(ui_view_t* view, int64_t key);
    void (*key_released)(ui_view_t* view, int64_t key);
    // timer() every_100ms() and every_sec() called
    // even for hidden and disabled views
    void (*timer)(ui_view_t* view, ui_timer_t id);
    void (*every_100ms)(ui_view_t* view); // ~10 x times per second
    void (*every_sec)(ui_view_t* view); // ~once a second
    int64_t (*hit_test)(int32_t x, int32_t y); // default: ui.hit_test.client
    fp64_t armed_until; // ut_clock.seconds() - when to release
    bool hidden; // paint() is not called on hidden
    bool armed;
    bool hover;
    bool pressed;   // for ui_button_t and ui_toggle_t
    bool disabled;  // mouse, keyboard, key_up/down not called on disabled
    bool focusable; // can be target for keyboard focus
    bool flat;      // no-border appearance of views
    bool highlightable; // paint highlight rectangle when hover over label
    fp64_t  hover_when;   // time in seconds when to call hovered()
    ui_color_t color;     // interpretation depends on view type
    int32_t    color_id;  // 0 is default. Otherwise ui_color_id_* when color is undefined
    ui_color_t background;// interpretation depends on view type
    ui_font_t* font;
    int32_t baseline;  // font ascent; descent = height - baseline
    int32_t descent;   // font descent
    int32_t label_dy;  // vertical shift down (to line up baselines of diff fonts)
    char    hint[256]; // tooltip hint text (to be shown while hovering over view)
} ui_view_t;

// tap() / press() APIs guarantee that single tap() is not coming
// before fp64_t tap/click in expense of fp64_t click delay (0.5 seconds)
// which is OK for buttons and many other UI controls but absolutely not
// OK for text editing. Thus edit uses raw mouse events to react
// on clicks and fp64_t clicks.

void ui_view_init(ui_view_t* view);

typedef struct ui_view_if {
    // children va_args must be null terminated
    ui_view_t* (*add)(ui_view_t* parent, ...);
    void (*add_first)(ui_view_t* parent, ui_view_t* child);
    void (*add_last)(ui_view_t* parent,  ui_view_t* child);
    void (*add_after)(ui_view_t* child,  ui_view_t* after);
    void (*add_before)(ui_view_t* child, ui_view_t* before);
    void (*remove)(ui_view_t* view); // removes view from it`s parent
    void (*remove_all)(ui_view_t* parent); // removes all children
    void (*disband)(ui_view_t* parent); // removes all children recursively
    bool (*inside)(ui_view_t* view, const ui_point_t* pt);
    void (*set_text)(ui_view_t* view, const char* text);
    void (*invalidate)(const ui_view_t* view); // prone to delays
    void (*measure)(ui_view_t* view);     // if text[] != "" sets w, h
    bool (*is_hidden)(ui_view_t* view);   // view or any parent is hidden
    bool (*is_disabled)(ui_view_t* view); // view or any parent is disabled
    const char* (*nls)(ui_view_t* view);  // returns localized text
    void (*localize)(ui_view_t* view);    // set strid based ui .text field
    void (*timer)(ui_view_t* view, ui_timer_t id);
    void (*every_sec)(ui_view_t* view);
    void (*every_100ms)(ui_view_t* view);
    void (*key_pressed)(ui_view_t* view, int64_t v_key);
    void (*key_released)(ui_view_t* view, int64_t v_key);
    void (*character)(ui_view_t* view, const char* utf8);
    void (*paint)(ui_view_t* view);
    bool (*set_focus)(ui_view_t* view);
    void (*kill_focus)(ui_view_t* view);
    void (*kill_hidden_focus)(ui_view_t* view);
    void (*hovering)(ui_view_t* view, bool start);
    void (*mouse)(ui_view_t* view, int32_t m, int64_t f);
    void (*mouse_wheel)(ui_view_t* view, int32_t dx, int32_t dy);
    void (*measure_children)(ui_view_t* view);
    void (*layout_children)(ui_view_t* view);
    void (*hover_changed)(ui_view_t* view);
    bool (*is_shortcut_key)(ui_view_t* view, int64_t key);
    bool (*context_menu)(ui_view_t* view);
    bool (*tap)(ui_view_t* view, int32_t ix); // 0: left 1: middle 2: right
    bool (*press)(ui_view_t* view, int32_t ix); // 0: left 1: middle 2: right
    bool (*message)(ui_view_t* view, int32_t m, int64_t wp, int64_t lp,
                                     int64_t* ret);
    void (*debug_paint)(ui_view_t* v);
    void (*test)(void);
} ui_view_if;

extern ui_view_if ui_view;

// view children iterator:

#define ui_view_for_each_begin(v, it) do {       \
    ui_view_t* it = (v)->child;                  \
    if (it != null) {                            \
        do {                                     \


#define ui_view_for_each_end(v, it)              \
            it = it->next;                       \
        } while (it != (v)->child);              \
    }                                            \
} while (0)

#define ui_view_for_each(v, it, ...) \
    ui_view_for_each_begin(v, it)    \
    { __VA_ARGS__ }                  \
    ui_view_for_each_end(v, it)

// #define code(statements) statements
//
// used as:
// {
//     macro({
//        foo();
//        bar();
//     })
// }
//
// except in m4 preprocessor loses new line
// between foo() and bar() and makes debugging and
// using __LINE__ difficult to impossible.
//
// Also
// #define code(...) { __VA_ARGS__ }
// is way easier on preprocessor


#define ui_view_call_init(v) do {                   \
    if ((v)->init != null) {                        \
        void (*_init_)(ui_view_t* _v_) = (v)->init; \
        (v)->init = null; /* before! call */        \
        _init_((v));                                \
    }                                               \
} while (0)


// _____________________________ ui_containers.h ______________________________

/* Copyright (c) Dmitry "Leo" Kuznetsov 2021-24 see LICENSE for details */
// _______________________________ ui_layout.h ________________________________

#include "ut/ut_std.h"


 // TODO: ui_ namespace

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

// ________________________________ ui_label.h ________________________________

#include "ut/ut_std.h"


typedef ui_view_t ui_label_t;

void ui_view_init_label(ui_view_t* view);

#define ui_label(min_width_em, s) {                             \
      .type = ui_view_label, .init = ui_view_init_label,        \
      .font = &ui_app.fonts.regular, .min_w_em = min_width_em,  \
      .text = s                                                 \
}

// text with "&" keyboard shortcuts:

void ui_label_init(ui_label_t* t, fp32_t min_w_em, const char* format, ...);
void ui_label_init_va(ui_label_t* t, fp32_t min_w_em, const char* format, va_list vl);

// use this macro for initilizization:
//    ui_label_t label = ui_label(min_width_em, s);
// or:
//    label = (ui_label_t)ui_label(min_width_em, s);
// which is subtle C difference of constant and
// variable initialization and I did not find universal way

// _______________________________ ui_button.h ________________________________

#include "ut/ut_std.h"


typedef ui_view_t ui_button_t;

void ui_view_init_button(ui_view_t* view);

void ui_button_init(ui_button_t* b, const char* label, fp32_t min_width_em,
    void (*callback)(ui_button_t* b));

#define static_ui_button(name, s, min_width_em, ...)             \
    static void name ## _callback(ui_button_t* name) {           \
        (void)name; /* no warning if unused */                   \
        { __VA_ARGS__ }                                          \
    }                                                            \
    static                                                       \
    ui_button_t name = {                                         \
        .type = ui_view_button, .init = ui_view_init_button,     \
        .font = &ui_app.fonts.regular, .min_w_em = min_width_em, \
        .text = s, .callback = name ## _callback                 \
    }

#define ui_button(s, min_width_em, call_back) {              \
    .type = ui_view_button, .init = ui_view_init_button,     \
    .font = &ui_app.fonts.regular, .min_w_em = min_width_em, \
    .text = s, .callback = call_back }                       \

// usage:
//
// static_ui_button(button, "&Button", 7.0, {
//      button->pressed = !button->pressed;
// })
//
// or:
//
// static void button_flipped(ui_button_t* b) {
//      b->pressed = !b->pressed;
// }
//
// ui_button_t button = ui_button(7.0, "&Button", button_flipped);
//
// or
//
// ui_button_t button = ui_view)button(button);
// strprintf(button.text, "&Button");
// button.min_w_em = 7.0;
// button.callback = button_flipped;


// ________________________________ ui_theme.h ________________________________

/* Copyright (c) Dmitry "Leo" Kuznetsov 2021-24 see LICENSE for details */


enum {
    ui_color_id_undefined           =  0,
    ui_color_id_active_title        =  1,
    ui_color_id_button_face         =  2,
    ui_color_id_button_text         =  3,
    ui_color_id_gray_text           =  4,
    ui_color_id_highlight           =  5,
    ui_color_id_highlight_text      =  6,
    ui_color_id_hot_tracking_color  =  7,
    ui_color_id_inactive_title      =  8,
    ui_color_id_inactive_title_text =  9,
    ui_color_id_menu_highlight      = 10,
    ui_color_id_title_text          = 11,
    ui_color_id_window              = 12,
    ui_color_id_window_text         = 13
};

typedef struct  {
    ui_color_t (*get_color)(int32_t color_id);
    bool (*is_system_dark)(void);
    bool (*are_apps_dark)(void);
    void (*refresh)(void* window);
    void (*test)(void);
} ui_theme_if;

extern ui_theme_if ui_theme;

// _______________________________ ui_toggle.h ________________________________

#include "ut/ut_std.h"



typedef ui_view_t ui_toggle_t;

// label may contain "___" which will be replaced with "On" / "Off"
void ui_toggle_init(ui_toggle_t* b, const char* label, fp32_t ems,
    void (*callback)(ui_toggle_t* b));

void ui_view_init_toggle(ui_view_t* view);

#define static_ui_toggle(name, s, min_width_em, ...)          \
    static void name ## _callback(ui_toggle_t* name) {        \
        (void)name; /* no warning if unused */                \
        { __VA_ARGS__ }                                       \
    }                                                         \
    static                                                    \
    ui_toggle_t name = {                                      \
        .type = ui_view_toggle, .init = ui_view_init_toggle,  \
        .font = &ui_app.fonts.regular, .min_w_em = min_width_em, \
        .text = s, .callback = name ## _callback              \
   }

#define ui_toggle(s, min_width_em, call_back) {           \
    .type = ui_view_toggle, .init = ui_view_init_toggle,  \
    .font = &ui_app.fonts.regular, .min_w_em = min_width_em, \
    .text = s, .callback = call_back                      \
}

// _______________________________ ui_slider.h ________________________________

#include "ut/ut_std.h"


typedef struct ui_slider_s ui_slider_t;

typedef struct ui_slider_s {
    ui_view_t view;
    int32_t step;
    fp64_t time;   // time last button was pressed
    ui_point_t tm; // text measurement (special case for %0*d)
    ui_button_t inc;
    ui_button_t dec;
    int32_t value;  // for ui_slider_t range slider control
    int32_t value_min;
    int32_t value_max;
} ui_slider_t;

void ui_view_init_slider(ui_view_t* view);

void ui_slider_init(ui_slider_t* r, const char* label, fp32_t min_w_em,
    int32_t value_min, int32_t value_max, void (*callback)(ui_view_t* r));

#define static_ui_slider(name, s, min_width_em, vmn, vmx, ...)            \
    static void name ## _callback(ui_slider_t* name) {                    \
        (void)name; /* no warning if unused */                            \
        { __VA_ARGS__ }                                                   \
    }                                                                     \
    static                                                                \
    ui_slider_t name = {                                                  \
        .view = { .type = ui_view_slider, .font = &ui_app.fonts.regular,     \
                  .min_w_em = min_width_em, .init = ui_view_init_slider,  \
                   .text = s, .callback = name ## _callback               \
        },                                                                \
        .value_min = vmn, .value_max = vmx, .value = vmn,                 \
    }

#define ui_slider(s, min_width_em, vmn, vmx, call_back) {                 \
    .view = { .type = ui_view_slider, .font = &ui_app.fonts.regular,         \
        .min_w_em = min_width_em, .text = s, .init = ui_view_init_slider, \
        .callback = call_back                                             \
    },                                                                    \
    .value_min = vmn, .value_max = vmx, .value = vmn,                     \
}

// _________________________________ ui_mbx.h _________________________________

#include "ut/ut_std.h"


// Options like:
//   "Yes"|"No"|"Abort"|"Retry"|"Ignore"|"Cancel"|"Try"|"Continue"
// maximum number of choices presentable to human is 4.

typedef struct {
    ui_view_t   view;
    ui_label_t  label;
    ui_button_t button[4];
    int32_t option; // -1 or option chosen by user
    const char** options;
} ui_mbx_t;

void ui_view_init_mbx(ui_view_t* view);

void ui_mbx_init(ui_mbx_t* mx, const char* option[], const char* format, ...);

#define static_ui_mbx(name, s, code, ...)                        \
                                                                 \
    static char* name ## _options[] = { __VA_ARGS__, null };     \
                                                                 \
    static void name ## _callback(ui_mbx_t* m, int32_t option) { \
        (void)m; (void)option; /* no warnings if unused */       \
        code                                                     \
    }                                                            \
    static                                                       \
    ui_mbx_t name = {                                            \
        .view = { .type = ui_view_mbx, .init = ui_view_init_mbx, \
                  .font = &ui_app.fonts.regular,                    \
                  .text = s, .callback = name ## _callback       \
                },                                               \
        .options = name ## _options                              \
    }

#define ui_mbx(s, call_back, ...) {                          \
    .view = { .type = ui_view_mbx, .init = ui_view_init_mbx, \
              .font = &ui_app.fonts.regular,                    \
              .text = s, .callback = call_back               \
    },                                                       \
    .options = (const char*[]){ __VA_ARGS__, null },         \
}

// _______________________________ ui_caption.h _______________________________

/* Copyright (c) Dmitry "Leo" Kuznetsov 2021-24 see LICENSE for details */


typedef struct ui_caption_s {
    ui_view_t view;
    // caption`s children:
    ui_button_t icon;
    ui_label_t title;
    ui_view_t spacer;
    ui_button_t menu; // use: ui_caption.button_menu.cb := your callback
    ui_button_t mini;
    ui_button_t maxi;
    ui_button_t full;
    ui_button_t quit;
} ui_caption_t;

extern ui_caption_t ui_caption;

// _________________________________ ui_app.h _________________________________

#include "ut/ut_std.h"


// link.exe /SUBSYSTEM:WINDOWS single window application

typedef struct {
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
    int32_t visibility; // initial window_visibility state
    int32_t last_visibility;    // last window_visibility state from last run
    int32_t startup_visibility; // window_visibility from parent process
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
    bool aero;     // retro Windows 7 decoration (just for the fun of it)
    ui_icon_t icon; // may be null
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
    // not to call ut_clock.seconds() too often:
    fp64_t now;     // ssb "seconds since boot" updated on each message
    ui_view_t* view;      // show_window() changes ui.hidden
    ui_view_t* focus;   // does not affect message routing - free for all
    ui_fonts_t fonts;
    ui_cursor_t cursor; // current cursor
    ui_cursor_t cursor_arrow;
    ui_cursor_t cursor_wait;
    ui_cursor_t cursor_ibeam;
    ui_cursor_t cursor_size_nwse; // north west - south east
    ui_cursor_t cursor_size_nesw; // north east - south west
    ui_cursor_t cursor_size_we;   // west - east
    ui_cursor_t cursor_size_ns;   // north - south
    ui_cursor_t cursor_size_all;  // north - south
    // keyboard state now:
    bool alt;
    bool ctrl;
    bool shift;
    ui_point_t mouse; // mouse/touchpad pointer
    ui_canvas_t canvas;  // set by message.paint
    struct { // animation state
        ui_view_t* view;
        int32_t step;
        fp64_t time; // closing time or zero
        int32_t x; // (x,y) for tooltip (-1,y) for toast
        int32_t y; // screen coordinates for tooltip
    } animating;
    // inch to pixels and reverse translation via ui_app.dpi.window
    fp32_t   (*px2in)(int32_t pixels);
    int32_t (*in2px)(fp32_t inches);
    // color: ui_color_undefined or R8G8B8, alpha: [0..1.0] or -1.0
    errno_t (*set_layered_window)(ui_color_t color, float alpha);
    int64_t (*hit_test)(int32_t x, int32_t y); // see ui.hit_test.*
    bool (*is_active)(void); // is application window active
    bool (*is_minimized)(void);
    bool (*is_maximized)(void);
    bool (*has_focus)(void); // application window has keyboard focus
    void (*activate)(void); // request application window activation
    ui_color_t (*get_color)(int32_t color_id); // ui.colors.*
    void (*set_title)(const char* title);
    void (*capture_mouse)(bool on); // capture mouse global input on/of
    void (*move_and_resize)(const ui_rect_t* rc);
    void (*bring_to_foreground)(void); // not necessary topmost
    void (*make_topmost)(void);   // in foreground hierarchy of windows
    void (*request_focus)(void);  // request application window keyboard focus
    void (*bring_to_front)(void); // activate() + bring_to_foreground() +
                                  // make_topmost() + request_focus()
    // measure and layout:
    void (*layout)(void); // requests layout on UI tree before paint()
    void (*invalidate)(const ui_rect_t* rc);
    void (*full_screen)(bool on);
    void (*redraw)(void); // very fast (5 microseconds) InvalidateRect(null)
    void (*draw)(void);   // UpdateWindow()
    void (*set_cursor)(ui_cursor_t c);
    void (*close)(void); // attempts to close (can_close() permitting)
    // forced quit() even if can_close() returns false
    void (*quit)(int32_t ec);  // ui_app.exit_code = ec; PostQuitMessage(ec);
    ui_timer_t (*set_timer)(uintptr_t id, int32_t milliseconds); // see notes
    void (*kill_timer)(ui_timer_t id);
    void (*post)(int32_t message, int64_t wp, int64_t lp);
    void (*show_window)(int32_t show); // see show_window enum
    void (*show_toast)(ui_view_t* toast, fp64_t seconds); // toast(null) to cancel
    void (*show_tooltip)(ui_view_t* tooltip, int32_t x, int32_t y, fp64_t seconds);
    void (*toast_va)(fp64_t seconds, const char* format, va_list vl);
    void (*toast)(fp64_t seconds, const char* format, ...);
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
    // const char* fn = ui_app.open_filename("C:\\", filter, countof(filter));
    const char* (*open_filename)(const char* folder, const char* filter[], int32_t n);
    const char* (*known_folder)(int32_t kfid);
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
} ui_app_t;

extern ui_app_t ui_app;



// Usage:
//
// ui_view_t* container  = ui_view(container);
// ui_view_t* horizontal = ui_view(ui_view_span);
// ui_view_t* vertical   = ui_view(ui_view_list);
//
// containers automatically layout child views
// similar to SwiftUI HStack and VStack taking .align
// .insets and .padding into account.
//
// Container positions every child views in the center,
// top bottom left right edge or any of 4 corners
// depending on .align values.
// if child view has .max_w or .max_h set to ui.infinity == INT32_MAX
// the views are expanded to fill the container in specified
// direction. If child .max_w or .max_h is set to > .w or .h
// the child view .w .h measurement are expanded accordingly.

typedef struct ui_view_s ui_view_t;

// Usage:
//
// ui_view_t* container  = ui_view(container);
// ui_view_t* horizontal = ui_view(ui_view_span);
// ui_view_t* vertical   = ui_view(ui_view_list);
//
// containers automatically layout child views
// similar to SwiftUI HStack and VStack taking .align
// .insets and .padding into account.
//
// Container positions every child views in the center,
// top bottom left right edge or any of 4 corners
// depending on .align values.
// if child view has .max_w or .max_h set to ui.infinity == INT32_MAX
// the views are expanded to fill the container in specified
// direction. If child .max_w or .max_h is set to > .w or .h
// the child view .w .h measurement are expanded accordingly.
//
// All containers are transparent and inset by 1/4 of an "em"
// Except ui_app.view which is also container but it is not
// inset and has default background color.
//
// Application implementer can override this after
//
// void opened(void) {
//     ui_view.add(ui_app.view, ..., null);
//     ui_app.view->insets = (ui_gaps_t) {
//         .left = 0.25,  .top = 0.25,
//         .right = 0.25, .bottom = 0.25 };
//     ui_app.view->color = ui_colors.dark_scarlet;
// }

typedef struct ui_view_s ui_view_t;

#define ui_view(view_type) {            \
    .type = (ui_view_ ## view_type),    \
    .init = ui_view_init_ ## view_type, \
    .font = &ui_app.fonts.regular          \
}

void ui_view_init_container(ui_view_t* view);
void ui_view_init_span(ui_view_t* view);
void ui_view_init_list(ui_view_t* view);
void ui_view_init_spacer(ui_view_t* view);


end_c

#endif // ui_definition

#ifdef ui_implementation
// _________________________________ ui_app.c _________________________________

#include "ut/ut.h"
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
    static ui_image_t image_dark;
    if (image_dark.bitmap == null) {
        uint8_t pixels[4] = { 0x3F, 0x3F, 0x3F };
        ui_gdi.image_init(&image_dark, 1, 1, 3, pixels);
    }
    static ui_image_t image_light;
    if (image_dark.bitmap == null) {
        uint8_t pixels[4] = { 0xC0, 0xC0, 0xC0 };
        ui_gdi.image_init(&image_light, 1, 1, 3, pixels);
    }
    if (ui_app.animating.view != null) {
        ui_view.measure_children(ui_app.animating.view);
        ui_gdi.push(0, 0);
        bool tooltip = ui_app.animating.x >= 0 && ui_app.animating.y >= 0;
        const int32_t em_x = ui_app.animating.view->em.x;
        const int32_t em_y = ui_app.animating.view->em.y;
        ui_gdi.set_brush(ui_gdi.brush_color);
        if (ui_theme.are_apps_dark()) {
            ui_gdi.set_brush_color(ui_colors.toast);
        } else {
            ui_gdi.set_brush_color(ui_app.get_color(ui_color_id_button_face));
        }
        if (!tooltip) {
            assert(0 <= ui_app.animating.step && ui_app.animating.step < ui_app_animation_steps);
            int32_t step = ui_app.animating.step - (ui_app_animation_steps - 1);
            ui_app.animating.view->y = ui_app.animating.view->h * step / (ui_app_animation_steps - 1);
//          traceln("step=%d of %d y=%d", ui_app.animating.step,
//                  ui_app_toast_steps, ui_app.animating.view->y);
            ui_app_measure_and_layout(ui_app.animating.view);
            if (ui_theme.are_apps_dark()) {
                fp64_t alpha = ut_min(0.40, 0.40 * ui_app.animating.step / (fp64_t)ui_app_animation_steps);
                ui_gdi.alpha_blend(0, 0, ui_app.width, ui_app.height, &image_dark, alpha);
            } else {
                traceln("TODO:");
            }
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
    assert(v == ui_app.view && v->x == 0 && v->y == 0 &&
           v->w >= ui_app.crc.w && v->h >= ui_app.crc.h);
    v->color = ui_app.get_color(ui_color_id_window);
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
    ui_color_t c = ui_app.is_active() ?
        ui_app.get_color(ui_color_id_highlight) : // ui_colors.btn_hover_highlight
        ui_app.get_color(ui_color_id_inactive_title);
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
//      traceln("wp: 0x%08X", wp);
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
//  TODO: investigate that it holds for Light Theme too
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

// _______________________________ ui_button.c ________________________________

#include "ut/ut.h"

static void ui_button_every_100ms(ui_view_t* v) { // every 100ms
    assert(v->type == ui_view_button);
    if (v->armed_until != 0 && ui_app.now > v->armed_until) {
        v->armed_until = 0;
        v->armed = false;
        ui_view.invalidate(v);
    }
}

// TODO: generalize and move to ui_colors.c to avoid slider dup

static ui_color_t ui_button_gradient_darker(void) {
    if (ui_theme.are_apps_dark()) {
        return ui_colors.btn_gradient_darker;
    } else {
        ui_color_t c = ui_app.get_color(ui_color_id_button_face);
        uint32_t r = ui_color_r(c);
        uint32_t g = ui_color_r(c);
        uint32_t b = ui_color_r(c);
        r = ut_max(0, ut_min(0xFF, (uint32_t)(r * 0.75)));
        g = ut_max(0, ut_min(0xFF, (uint32_t)(g * 0.75)));
        b = ut_max(0, ut_min(0xFF, (uint32_t)(b * 0.75)));
        ui_color_t d = ui_rgb(r, g, b);
//      traceln("c: 0%06X -> 0%06X", c, d);
        return d;
    }
}

static ui_color_t ui_button_gradient_dark(void) {
    if (ui_theme.are_apps_dark()) {
        return ui_colors.btn_gradient_dark;
    } else {
        ui_color_t c = ui_app.get_color(ui_color_id_button_face);
        uint32_t r = ui_color_r(c);
        uint32_t g = ui_color_r(c);
        uint32_t b = ui_color_r(c);
        r = ut_max(0, ut_min(0xFF, (uint32_t)(r * 1.25)));
        g = ut_max(0, ut_min(0xFF, (uint32_t)(g * 1.25)));
        b = ut_max(0, ut_min(0xFF, (uint32_t)(b * 1.25)));
        ui_color_t d = ui_rgb(r, g, b);
//      traceln("c: 0%06X -> 0%06X", c, d);
        return d;
    }
}

static void ui_button_paint(ui_view_t* v) {
    assert(v->type == ui_view_button);
    assert(!v->hidden);
    v->color = ui_app.get_color(ui_color_id_button_text);
    ui_gdi.push(v->x, v->y);
    bool pressed = (v->armed ^ v->pressed) == 0;
    if (v->armed_until != 0) { pressed = true; }
    int32_t sign = 1 - pressed * 2; // -1, +1
    int32_t w = sign * v->w;
    int32_t h = sign * v->h;
    int32_t x = v->x + (int32_t)pressed * v->w;
    int32_t y = v->y + (int32_t)pressed * v->h;
    if (!v->flat || v->hover) {
        ui_gdi.gradient(x, y, w, h,
            ui_button_gradient_darker(),
            ui_button_gradient_dark(), true);
    }
    ui_color_t c = v->color;
    if (!v->flat && v->armed) {
        c = ui_colors.btn_armed;
    }else if (!v->flat && v->hover && !v->armed) {
        c = ui_app.get_color(ui_color_id_hot_tracking_color);
    }
    if (v->disabled) { c = ui_app.get_color(ui_color_id_gray_text); }
    if (v->icon == null) {
        ui_font_t  f = *v->font;
        ui_point_t m = ui_gdi.measure_text(f, ui_view.nls(v));
        ui_gdi.set_text_color(c);
        ui_gdi.x = v->x + (v->w - m.x) / 2;
        ui_gdi.y = v->y + (v->h - m.y) / 2;
        f = ui_gdi.set_font(f);
        ui_gdi.text("%s", ui_view.nls(v));
        ui_gdi.set_font(f);
    } else {
        ui_gdi.draw_icon(v->x, v->y, v->w, v->h, v->icon);
    }
    const int32_t pw = ut_max(1, v->em.y / 32); // pen width
    ui_color_t color = v->armed ? ui_colors.dkgray4 : ui_colors.gray;
    if (v->hover && !v->armed) { color = ui_colors.blue; }
    if (v->disabled) { color = ui_colors.dkgray1; }
    if (!v->flat) {
        ui_pen_t p = ui_gdi.create_pen(color, pw);
        ui_gdi.set_pen(p);
        ui_gdi.set_brush(ui_gdi.brush_hollow);
        ui_gdi.rounded(v->x, v->y, v->w, v->h, v->em.y / 4, v->em.y / 4);
        ui_gdi.delete_pen(p);
    }
    ui_gdi.pop();
}

static bool ui_button_hit_test(ui_button_t* b, ui_point_t pt) {
    assert(b->type == ui_view_button);
    return ui_view.inside(b, &pt);
}

static void ui_button_callback(ui_button_t* b) {
    assert(b->type == ui_view_button);
    ui_app.show_tooltip(null, -1, -1, 0);
    if (b->callback != null) { b->callback(b); }
}

static void ui_button_trigger(ui_view_t* v) {
    assert(v->type == ui_view_button);
    assert(!v->hidden && !v->disabled);
    ui_button_t* b = (ui_button_t*)v;
    v->armed = true;
    ui_view.invalidate(v);
    ui_app.draw();
    v->armed_until = ui_app.now + 0.250;
    ui_button_callback(b);
    ui_view.invalidate(v);
}

static void ui_button_character(ui_view_t* v, const char* utf8) {
    assert(v->type == ui_view_button);
    assert(!v->hidden && !v->disabled);
    char ch = utf8[0]; // TODO: multibyte shortcuts?
    if (ui_view.is_shortcut_key(v, ch)) {
        ui_button_trigger(v);
    }
}

static void ui_button_key_pressed(ui_view_t* view, int64_t key) {
    if (ui_app.alt && ui_view.is_shortcut_key(view, key)) {
//      traceln("key: 0x%02X shortcut: %d", key, ui_view.is_shortcut_key(v, key));
        ui_button_trigger(view);
    }
}

/* processes mouse clicks and invokes callback  */

static void ui_button_mouse(ui_view_t* v, int32_t message, int64_t flags) {
    assert(v->type == ui_view_button);
    (void)flags; // unused
    assert(!v->hidden && !v->disabled);
    ui_button_t* b = (ui_button_t*)v;
    bool a = v->armed;
    bool on = false;
    if (message == ui.message.left_button_pressed ||
        message == ui.message.right_button_pressed) {
        v->armed = ui_button_hit_test(b, ui_app.mouse);
        if (v->armed) { ui_app.focus = v; }
        if (v->armed) { ui_app.show_tooltip(null, -1, -1, 0); }
    }
    if (message == ui.message.left_button_released ||
        message == ui.message.right_button_released) {
        if (v->armed) { on = ui_button_hit_test(b, ui_app.mouse); }
        v->armed = false;
    }
    if (on) { ui_button_callback(b); }
    if (a != v->armed) { ui_view.invalidate(v); }
}

static void ui_button_measure(ui_view_t* v) {
    assert(v->type == ui_view_button || v->type == ui_view_label);
    ui_view.measure(v);
    const int32_t em2  = ut_max(1, v->em.x / 2);
    v->w = v->w;
    v->h = v->h + em2;
    if (v->w < v->h) { v->w = v->h; }
}

void ui_view_init_button(ui_view_t* v) {
    assert(v->type == ui_view_button);
    ui_view_init(v);
    v->mouse       = ui_button_mouse;
    v->measure     = ui_button_measure;
    v->paint       = ui_button_paint;
    v->character   = ui_button_character;
    v->every_100ms = ui_button_every_100ms;
    v->key_pressed = ui_button_key_pressed;
    v->color       = ui_app.get_color(ui_color_id_window_text);
    ui_view.set_text(v, v->text);
    ui_view.localize(v);
}

void ui_button_init(ui_button_t* b, const char* label, fp32_t ems,
        void (*callback)(ui_button_t* b)) {
    b->type = ui_view_button;
    strprintf(b->text, "%s", label);
    b->callback = callback;
    b->min_w_em = ems;
    ui_view_init_button(b);
}
// _______________________________ ui_caption.c _______________________________

/* Copyright (c) Dmitry "Leo" Kuznetsov 2021-24 see LICENSE for details */
#include "ut/ut.h"

#define ui_caption_glyph_rest ui_glyph_upper_right_drop_shadowed_white_square
#define ui_caption_glyph_menu ui_glyph_trigram_for_heaven
#define ui_caption_glyph_mini ui_glyph_heavy_minus_sign
#define ui_caption_glyph_maxi ui_glyph_white_large_square
#define ui_caption_glyph_full ui_glyph_square_four_corners
#define ui_caption_glyph_quit ui_glyph_n_ary_times_operator

static void ui_caption_toggle_full(void) {
    ui_app.full_screen(!ui_app.is_full_screen);
    ui_caption.view.hidden = ui_app.is_full_screen;
    ui_app.layout();
}

static void ui_app_view_character(ui_view_t* v, const char utf8[]) {
    swear(v == ui_app.view);
    // TODO: inside ui_app.c instead of here
    if (utf8[0] == 033 && ui_app.is_full_screen) { ui_caption_toggle_full(); }
}

static void ui_caption_quit(ui_button_t* unused(b)) {
    ui_app.close();
}

static void ui_caption_mini(ui_button_t* unused(b)) {
    ui_app.show_window(ui.visibility.minimize);
}

static void ui_caption_maximize_or_restore(void) {
    strprintf(ui_caption.maxi.text, "%s",
        ui_app.is_maximized() ?
        ui_caption_glyph_rest : ui_caption_glyph_maxi);
}

static void ui_caption_maxi(ui_button_t* unused(b)) {
    if (!ui_app.is_maximized()) {
        ui_app.show_window(ui.visibility.maximize);
    } else if (ui_app.is_maximized() || ui_app.is_minimized()) {
        ui_app.show_window(ui.visibility.restore);
    }
    ui_caption_maximize_or_restore();
}

static void ui_caption_full(ui_button_t* unused(b)) {
    ui_caption_toggle_full();
}

static int64_t ui_caption_hit_test(int32_t x, int32_t y) {
    ui_point_t pt = { x, y };
    if (ui_app.is_full_screen) {
        return ui.hit_test.client;
    } else if (ui_view.inside(&ui_caption.icon, &pt)) {
        return ui.hit_test.system_menu;
    } else {
        ui_view_for_each(&ui_caption.view, c, {
            bool ignore = c->type == ui_view_container ||
                          c->type == ui_view_spacer ||
                          c->type == ui_view_label;
            if (!ignore && ui_view.inside(c, &pt)) {
                return ui.hit_test.client;
            }
        });
        return ui.hit_test.caption;
    }
}

static ui_color_t ui_caption_color(void) {
    ui_color_t c = ui_app.is_active() ?
        ui_theme.get_color(ui_color_id_active_title) :
        ui_theme.get_color(ui_color_id_inactive_title);
    return c;
}

static void ui_caption_paint(ui_view_t* v) {
    v->color = ui_caption_color();
//  traceln("%s 0x%016llX", v->text, v->color);
    if (!ui_color_is_transparent(v->color)) {
        ui_gdi.fill_with(v->x, v->y, v->w, v->h, v->color);
    }
}

static void ui_caption_init(ui_view_t* v) {
    swear(v == &ui_caption.view, "caption is a singleton");
    ui_view_init_span(v);
    ui_caption.view.insets = (ui_gaps_t){ 0, 0, 0, 0 };
    ui_caption.view.hidden = false;
    ui_app.view->character = ui_app_view_character; // ESC for full screen
    ui_view.add(&ui_caption.view,
        &ui_caption.icon,
        &ui_caption.menu,
        &ui_caption.title,
        &ui_caption.spacer,
        &ui_caption.mini,
        &ui_caption.maxi,
        &ui_caption.full,
        &ui_caption.quit,
        null);
    static const ui_gaps_t p = { .left  = 0.25, .top    = 0.25,
                                 .right = 0.25, .bottom = 0.25};
    ui_view_for_each(&ui_caption.view, c, {
        c->font = &ui_app.fonts.H3;
        c->color = ui_app.get_color(ui_color_id_window_text); // ui_colors.white;
        c->flat = true;
        c->padding = p;
    });
    ui_caption.icon.icon = ui_app.icon;
    ui_caption.view.max_w = INT32_MAX;
    ui_caption.view.align = ui.align.top;
    strprintf(ui_caption.view.text, "ui_caption");
    ui_caption_maximize_or_restore();
    ui_caption.view.paint = ui_caption_paint;
}

ui_caption_t ui_caption =  {
    .view = {
        .type     = ui_view_span,
        .font     = &ui_app.fonts.regular,
        .init     = ui_caption_init,
        .hit_test = ui_caption_hit_test,
        .hidden = true
    },
    .icon   = ui_button(ui_glyph_nbsp, 0.0, null),
    .title  = ui_label(0, ""),
    .spacer = ui_view(spacer),
    .menu   = ui_button(ui_caption_glyph_menu, 0.0, null),
    .mini   = ui_button(ui_caption_glyph_mini, 0.0, ui_caption_mini),
    .maxi   = ui_button(ui_caption_glyph_maxi, 0.0, ui_caption_maxi),
    .full   = ui_button(ui_caption_glyph_full, 0.0, ui_caption_full),
    .quit   = ui_button(ui_caption_glyph_quit, 0.0, ui_caption_quit),
};
// _______________________________ ui_colors.c ________________________________

#include "ut/ut.h"

enum {
    _colors_white     = ui_rgb(255, 255, 255),
    _colors_off_white = ui_rgb(192, 192, 192),
    _colors_dkgray0   = ui_rgb(16, 16, 16),
    _colors_dkgray1   = ui_rgb(30, 30, 30),
    _colors_dkgray2   = ui_rgb(37, 38, 38),
    _colors_dkgray3   = ui_rgb(45, 45, 48),
    _colors_dkgray4   = ui_rgb(63, 63, 70),
    _colors_blue_highlight = ui_rgb(128, 128, 255)
};

ui_colors_t ui_colors = {
    .none             = (int32_t)0xFFFFFFFFU, // aka CLR_INVALID in wingdi
    .text             = ui_rgb(240, 231, 220),
    .white            = _colors_white,
    .black            = ui_rgb(0,     0,   0),
    .red              = ui_rgb(255,   0,   0),
    .green            = ui_rgb(0,   255,   0),
    .blue             = ui_rgb(0,   0,   255),
    .yellow           = ui_rgb(255, 255,   0),
    .cyan             = ui_rgb(0,   255, 255),
    .magenta          = ui_rgb(255,   0, 255),
    .gray             = ui_rgb(128, 128, 128),
    .dkgray1          = _colors_dkgray1,
    .dkgray2          = _colors_dkgray2,
    .dkgray3          = _colors_dkgray3,
    .dkgray4          = _colors_dkgray4,
    // tone down RGB colors:
    .tone_white       = ui_rgb(164, 164, 164),
    .tone_red         = ui_rgb(192,  64,  64),
    .tone_green       = ui_rgb(64,  192,  64),
    .tone_blue        = ui_rgb(64,   64, 192),
    .tone_yellow      = ui_rgb(192, 192,  64),
    .tone_cyan        = ui_rgb(64,  192, 192),
    .tone_magenta     = ui_rgb(192,  64, 192),
    // miscelaneous:
    .orange           = ui_rgb(255, 165,   0), // 0xFFA500
    .dkgreen          = ui_rgb(  1,  50,  32), // 0x013220
    .pink             = ui_rgb(255, 192, 203), // 0xFFC0CB
    .ochre            = ui_rgb(204, 119,  34), // 0xCC7722
    .gold             = ui_rgb(255, 215,   0), // 0xFFD700
    .teal             = ui_rgb(  0, 128, 128), // 0x008080
    .wheat            = ui_rgb(245, 222, 179), // 0xF5DEB3
    .tan              = ui_rgb(210, 180, 140), // 0xD2B48C
    .brown            = ui_rgb(165,  42,  42), // 0xA52A2A
    .maroon           = ui_rgb(128,   0,   0), // 0x800000
    .barbie_pink      = ui_rgb(224,  33, 138), // 0xE0218A
    .steel_pink       = ui_rgb(204,  51, 204), // 0xCC33CC
    .salmon_pink      = ui_rgb(255, 145, 164), // 0xFF91A4
    .gainsboro        = ui_rgb(220, 220, 220), // 0xDCDCDC
    .light_gray       = ui_rgb(211, 211, 211), // 0xD3D3D3
    .silver           = ui_rgb(192, 192, 192), // 0xC0C0C0
    .dark_gray        = ui_rgb(169, 169, 169), // 0xA9A9A9
    .dim_gray         = ui_rgb(105, 105, 105), // 0x696969
    .light_slate_gray = ui_rgb(119, 136, 153), // 0x778899
    .slate_gray       = ui_rgb(112, 128, 144), // 0x708090

    // highlights:
    .text_highlight      = ui_rgb(190, 200, 255), // bluish off-white
    .blue_highlight      = _colors_blue_highlight,
    .off_white           = _colors_off_white,

    .btn_gradient_darker = _colors_dkgray1,
    .btn_gradient_dark   = _colors_dkgray3,
    .btn_hover_highlight = _colors_blue_highlight,
    .btn_disabled        = _colors_dkgray4,
    .btn_armed           = _colors_white,
    .btn_text            = _colors_off_white,
    .toast               = _colors_dkgray3, // ui_rgb(8, 40, 24), // toast background

    /* Main Panel Backgrounds */
    .ennui_black                = ui_rgb( 18,  18,  18), // 0x1212121
    .charcoal                   = ui_rgb( 54,  69,  79), // 0x36454F
    .onyx                       = ui_rgb( 53,  56,  57), // 0x353839
    .gunmetal                   = ui_rgb( 42,  52,  57), // 0x2A3439
    .jet_black                  = ui_rgb( 52,  52,  52), // 0x343434
    .outer_space                = ui_rgb( 65,  74,  76), // 0x414A4C
    .eerie_black                = ui_rgb( 27,  27,  27), // 0x1B1B1B
    .oil                        = ui_rgb( 59,  60,  54), // 0x3B3C36
    .black_coral                = ui_rgb( 84,  98, 111), // 0x54626F
    .obsidian                   = ui_rgb( 58,  50,  45), // 0x3A322D

    /* Secondary Panels or Sidebars */
    .raisin_black               = ui_rgb( 39,  38,  53), // 0x272635
    .dark_charcoal              = ui_rgb( 48,  48,  48), // 0x303030
    .dark_jungle_green          = ui_rgb( 26,  36,  33), // 0x1A2421
    .pine_tree                  = ui_rgb( 42,  47,  35), // 0x2A2F23
    .rich_black                 = ui_rgb(  0,  64,  64), // 0x004040
    .eclipse                    = ui_rgb( 63,  57,  57), // 0x3F3939
    .cafe_noir                  = ui_rgb( 75,  54,  33), // 0x4B3621

    /* Flat Buttons */
    .prussian_blue              = ui_rgb(  0,  49,  83), // 0x003153
    .midnight_green             = ui_rgb(  0,  73,  83), // 0x004953
    .charleston_green           = ui_rgb( 35,  43,  43), // 0x232B2B
    .rich_black_fogra           = ui_rgb( 10,  15,  13), // 0x0A0F0D
    .dark_liver                 = ui_rgb( 83,  75,  79), // 0x534B4F
    .dark_slate_gray            = ui_rgb( 47,  79,  79), // 0x2F4F4F
    .black_olive                = ui_rgb( 59,  60,  54), // 0x3B3C36
    .cadet                      = ui_rgb( 83, 104, 114), // 0x536872

    /* Button highlights (hover) */
    .dark_sienna                = ui_rgb( 60,  20,  20), // 0x3C1414
    .bistre_brown               = ui_rgb(150, 113,  23), // 0x967117
    .dark_puce                  = ui_rgb( 79,  58,  60), // 0x4F3A3C
    .wenge                      = ui_rgb(100,  84,  82), // 0x645452

    /* Raised button effects */
    .dark_scarlet               = ui_rgb( 86,   3,  25), // 0x560319
    .burnt_umber                = ui_rgb(138,  51,  36), // 0x8A3324
    .caput_mortuum              = ui_rgb( 89,  39,  32), // 0x592720
    .barn_red                   = ui_rgb(124,  10,   2), // 0x7C0A02

    /* Text and Icons */
    .platinum                   = ui_rgb(229, 228, 226), // 0xE5E4E2
    .anti_flash_white           = ui_rgb(242, 243, 244), // 0xF2F3F4
    .silver_sand                = ui_rgb(191, 193, 194), // 0xBFC1C2
    .quick_silver               = ui_rgb(166, 166, 166), // 0xA6A6A6

    /* Links and Selections */
    .dark_powder_blue           = ui_rgb(  0,  51, 153), // 0x003399
    .sapphire_blue              = ui_rgb( 15,  82, 186), // 0x0F52BA
    .international_klein_blue   = ui_rgb(  0,  47, 167), // 0x002FA7
    .zaffre                     = ui_rgb(  0,  20, 168), // 0x0014A8

    /* Additional Colors */
    .fish_belly                 = ui_rgb(232, 241, 212), // 0xE8F1D4
    .rusty_red                  = ui_rgb(218,  44,  67), // 0xDA2C43
    .falu_red                   = ui_rgb(128,  24,  24), // 0x801818
    .cordovan                   = ui_rgb(137,  63,  69), // 0x893F45
    .dark_raspberry             = ui_rgb(135,  38,  87), // 0x872657
    .deep_magenta               = ui_rgb(204,   0, 204), // 0xCC00CC
    .byzantium                  = ui_rgb(112,  41,  99), // 0x702963
    .amethyst                   = ui_rgb(153, 102, 204), // 0x9966CC
    .wisteria                   = ui_rgb(201, 160, 220), // 0xC9A0DC
    .lavender_purple            = ui_rgb(150, 123, 182), // 0x967BB6
    .opera_mauve                = ui_rgb(183, 132, 167), // 0xB784A7
    .mauve_taupe                = ui_rgb(145,  95, 109), // 0x915F6D
    .rich_lavender              = ui_rgb(167, 107, 207), // 0xA76BCF
    .pansy_purple               = ui_rgb(120,  24,  74), // 0x78184A
    .violet_eggplant            = ui_rgb(153,  17, 153), // 0x991199
    .jazzberry_jam              = ui_rgb(165,  11,  94), // 0xA50B5E
    .dark_orchid                = ui_rgb(153,  50, 204), // 0x9932CC
    .electric_purple            = ui_rgb(191,   0, 255), // 0xBF00FF
    .sky_magenta                = ui_rgb(207, 113, 175), // 0xCF71AF
    .brilliant_rose             = ui_rgb(230, 103, 206), // 0xE667CE
    .fuchsia_purple             = ui_rgb(204,  57, 123), // 0xCC397B
    .french_raspberry           = ui_rgb(199,  44,  72), // 0xC72C48
    .wild_watermelon            = ui_rgb(252, 108, 133), // 0xFC6C85
    .neon_carrot                = ui_rgb(255, 163,  67), // 0xFFA343
    .burnt_orange               = ui_rgb(204,  85,   0), // 0xCC5500
    .carrot_orange              = ui_rgb(237, 145,  33), // 0xED9121
    .tiger_orange               = ui_rgb(253, 106,   2), // 0xFD6A02
    .giant_onion                = ui_rgb(176, 181, 137), // 0xB0B589
    .rust                       = ui_rgb(183,  65,  14), // 0xB7410E
    .copper_red                 = ui_rgb(203, 109,  81), // 0xCB6D51
    .dark_tangerine             = ui_rgb(255, 168,  18), // 0xFFA812
    .bright_marigold            = ui_rgb(252, 192,   6), // 0xFCC006
    .bone                       = ui_rgb(227, 218, 201), // 0xE3DAC9

    /* Earthy Tones */
    .ochre                      = ui_rgb(204, 119,  34), // 0xCC7722
    .sienna                     = ui_rgb(160,  82,  45), // 0xA0522D
    .sandy_brown                = ui_rgb(244, 164,  96), // 0xF4A460
    .golden_brown               = ui_rgb(153, 101,  21), // 0x996515
    .camel                      = ui_rgb(193, 154, 107), // 0xC19A6B
    .burnt_sienna               = ui_rgb(238, 124,  88), // 0xEE7C58
    .khaki                      = ui_rgb(195, 176, 145), // 0xC3B091
    .dark_khaki                 = ui_rgb(189, 183, 107), // 0xBDB76B

    /* Greens */
    .fern_green                 = ui_rgb( 79, 121,  66), // 0x4F7942
    .moss_green                 = ui_rgb(138, 154,  91), // 0x8A9A5B
    .myrtle_green               = ui_rgb( 49, 120, 115), // 0x317873
    .pine_green                 = ui_rgb(  1, 121, 111), // 0x01796F
    .jungle_green               = ui_rgb( 41, 171, 135), // 0x29AB87
    .sacramento_green           = ui_rgb(  4,  57,  39), // 0x043927

    /* Blues */
    .yale_blue                  = ui_rgb( 15,  77, 146), // 0x0F4D92
    .cobalt_blue                = ui_rgb(  0,  71, 171), // 0x0047AB
    .persian_blue               = ui_rgb( 28,  57, 187), // 0x1C39BB
    .royal_blue                 = ui_rgb( 65, 105, 225), // 0x4169E1
    .iceberg                    = ui_rgb(113, 166, 210), // 0x71A6D2
    .blue_yonder                = ui_rgb( 80, 114, 167), // 0x5072A7

    /* Miscellaneous */
    .cocoa_brown                = ui_rgb(210, 105,  30), // 0xD2691E
    .cinnamon_satin             = ui_rgb(205,  96, 126), // 0xCD607E
    .fallow                     = ui_rgb(193, 154, 107), // 0xC19A6B
    .cafe_au_lait               = ui_rgb(166, 123,  91), // 0xA67B5B
    .liver                      = ui_rgb(103,  76,  71), // 0x674C47
    .shadow                     = ui_rgb(138, 121,  93), // 0x8A795D
    .cool_grey                  = ui_rgb(140, 146, 172), // 0x8C92AC
    .payne_grey                 = ui_rgb( 83, 104, 120), // 0x536878

    /* Lighter Tones for Contrast */
    .timberwolf                 = ui_rgb(219, 215, 210), // 0xDBD7D2
    .silver_chalice             = ui_rgb(172, 172, 172), // 0xACACAC
    .roman_silver               = ui_rgb(131, 137, 150), // 0x838996

    /* Dark Mode Specific Highlights */
    .electric_lavender          = ui_rgb(244, 191, 255), // 0xF4BFFF
    .magenta_haze               = ui_rgb(159,  69, 118), // 0x9F4576
    .cyber_grape                = ui_rgb( 88,  66, 124), // 0x58427C
    .purple_navy                = ui_rgb( 78,  81, 128), // 0x4E5180
    .liberty                    = ui_rgb( 84,  90, 167), // 0x545AA7
    .purple_mountain_majesty    = ui_rgb(150, 120, 182), // 0x9678B6
    .ceil                       = ui_rgb(146, 161, 207), // 0x92A1CF
    .moonstone_blue             = ui_rgb(115, 169, 194), // 0x73A9C2
    .independence               = ui_rgb( 76,  81, 109)  // 0x4C516D
};
// _____________________________ ui_containers.c ______________________________

/* Copyright (c) Dmitry "Leo" Kuznetsov 2021-24 see LICENSE for details */
#include "ut/ut.h"

static bool ui_containers_debug = false;

#pragma push_macro("debugln")
#pragma push_macro("dump")

// Usage of: ui_view_for_each_begin(p, c) { ... } ui_view_for_each_end(p, c)
// makes code inside iterator debugger friendly and ensures correct __LINE__

#define debugln(...) do {                                \
    if (ui_containers_debug) {  traceln(__VA_ARGS__); }  \
} while (0)

static const char* ui_container_finite_int(int32_t v, char* text, int32_t count) {
    swear(v >= 0);
    if (v == ui.infinity) {
        ut_str.format(text, count, "%s", ui_glyph_infinity);
    } else {
        ut_str.format(text, count, "%d", v);
    }
    return text;
}

#define dump(v) do {                                                          \
    char maxw[32];                                                            \
    char maxh[32];                                                            \
    debugln("%s[%4.4s] %d,%d %dx%d, max[%sx%s] "                              \
        "padding { %.3f %.3f %.3f %.3f } "                                    \
        "insets { %.3f %.3f %.3f %.3f } align: 0x%02X",                       \
        v->text, &v->type, v->x, v->y, v->w, v->h,                            \
        ui_container_finite_int(v->max_w, maxw, countof(maxw)),               \
        ui_container_finite_int(v->max_h, maxh, countof(maxh)),               \
        v->padding.left, v->padding.top, v->padding.right, v->padding.bottom, \
        v->insets.left, v->insets.top,v->insets.right, v->insets.bottom,      \
        v->align);                                                            \
} while (0)

static void ui_span_measure(ui_view_t* p) {
    swear(p->type == ui_view_span, "type %4.4s 0x%08X", &p->type, p->type);
    const int32_t i_lf = ui.gaps_em2px(p->em.x, p->insets.left);
    const int32_t i_tp = ui.gaps_em2px(p->em.y, p->insets.top);
    const int32_t i_rt = ui.gaps_em2px(p->em.x, p->insets.right);
    const int32_t i_bt = ui.gaps_em2px(p->em.y, p->insets.bottom);
    int32_t max_w = i_lf;
    int32_t w = i_lf;
    int32_t h = 0;
    ui_view_for_each_begin(p, c) {
        swear(c->max_w == 0 || c->max_w >= c->w,
              "max_w: %d w: %d", c->max_w, c->w);
        if (c->type == ui_view_spacer) {
            c->w = 0; // layout will distribute excess here
            max_w = ui.infinity; // spacer make width greedy
        } else {
            const int32_t p_lf = ui.gaps_em2px(c->em.x, c->padding.left);
            const int32_t p_tp = ui.gaps_em2px(c->em.y, c->padding.top);
            const int32_t p_rt = ui.gaps_em2px(c->em.x, c->padding.right);
            const int32_t p_bt = ui.gaps_em2px(c->em.y, c->padding.bottom);
            h = ut_max(h, p_tp + c->h + p_bt);
            const int32_t cw = p_lf + c->w + p_rt;
            if (c->max_w == ui.infinity) {
                max_w = ui.infinity;
            } else if (max_w < ui.infinity && c->max_w != 0) {
                swear(c->max_w >= cw, "Constraint violation: c->max_w < cw, "
                                      "max_w: %d, cw: %d", c->max_w, cw);
                max_w += c->max_w;
            } else if (max_w < ui.infinity) {
                swear(0 <= max_w + cw && max_w + cw < ui.infinity,
                      "Width overflow: max_w + cw = %d", max_w + cw);
                max_w += cw;
            }
            w += cw;
        }
    } ui_view_for_each_end(p, c);
    if (max_w < ui.infinity) {
        swear(0 <= max_w + i_rt && max_w + i_rt < ui.infinity,
             "Width overflow at right inset: max_w + right = %d",
              max_w + i_rt);
        max_w += i_rt;
    }
    w += i_rt;
    h += i_tp + i_bt;
    swear(max_w == 0 || max_w >= w,
         "max_w: %d is less than actual width w: %d", max_w, w);
    // Handle max width only if it differs from actual width
// TODO: I doubt we can touch p->max_w unless it is zero... think about it
    p->max_w = max_w == w ? p->max_w : ut_max(max_w, p->max_w);
    // do not touch max_h, caller may have set it to something
    p->w = w;
    p->h = h;
    // add top and bottom insets
    p->h += ui.gaps_em2px(p->em.y, p->insets.top);
    p->h += ui.gaps_em2px(p->em.y, p->insets.bottom);
    swear(p->max_w == 0 || p->max_w >= p->w, "max_w is less than actual width w");
}

// after measure of the subtree is concluded the parent ui_span
// may adjust span_w wider number depending on it's own width
// and ui_span.max_w agreement

static void ui_span_layout(ui_view_t* p) {
    debugln("> %s %d,%d %dx%d", p->text, p->x, p->y, p->w, p->h);
    swear(p->type == ui_view_span, "type %4.4s 0x%08X", &p->type, p->type);
    int32_t spacers = 0; // Number of spacers
    // Left and right insets
    const int32_t i_lf = ui.gaps_em2px(p->em.x, p->insets.left);
    const int32_t i_rt = ui.gaps_em2px(p->em.x, p->insets.right);
    // Top and bottom insets
    const int32_t i_tp = ui.gaps_em2px(p->em.y, p->insets.top);
    const int32_t i_bt = ui.gaps_em2px(p->em.y, p->insets.bottom);
    const int32_t lf = p->x + i_lf;
    const int32_t rt = p->x + p->w - i_rt;
    swear(lf < rt, "Inverted or zero-width conditions: lf: %d, rt: %d", lf, rt);
    // Top and bottom y coordinates
    const int32_t top = p->y + i_tp;
    // Mitigation for vertical overflow:
    const int32_t bot = p->y + p->h - i_bt < top ? top + p->h : p->y + p->h - i_bt;
    int32_t max_w_sum = 0;
    int32_t max_w_count = 0;
    int32_t x = lf;
    ui_view_for_each_begin(p, c) {
        const int32_t p_lf = ui.gaps_em2px(c->em.x, c->padding.left);
        const int32_t p_tp = ui.gaps_em2px(c->em.y, c->padding.top);
        const int32_t p_rt = ui.gaps_em2px(c->em.x, c->padding.right);
        const int32_t p_bt = ui.gaps_em2px(c->em.y, c->padding.bottom);
        if (c->type == ui_view_spacer) {
            c->y = p->y + i_tp + p_tp;
            c->h = bot - top - p_tp - p_bt;
            spacers++;
        } else {
            // setting child`s max_h to infinity means that child`s height is
            // *always* fill vertical view size of the parent
            // childs.h can exceed parent.h (vertical overflow) - not encouraged but allowed
            if (c->max_h == ui.infinity) { c->h = p->h - i_tp - i_bt - p_tp - p_bt; }
            if ((c->align & ui.align.top) != 0) {
                c->y = top + p_tp;
            } else if ((c->align & ui.align.bottom) != 0) {
                c->y = bot - (c->h + p_bt);
            } else {
                const int32_t ch = p_tp + c->h + p_bt;
                c->y = top + p_tp + (bot - top - ch) / 2;
            }
            c->x = x + p_lf;
            x = c->x + c->w + p_rt;
            swear(c->max_w == 0 || c->max_w > c->w, "max_w must be greater "
                  "than current width: max_w: %d, w: %d", c->max_w, c->w);
            if (c->max_w > 0) {
                // if max_w is infinity, it can occupy the whole parent width:
                max_w_sum += (c->max_w == ui.infinity) ? p->w : c->max_w;
                max_w_count++;
            }
        }
    } ui_view_for_each_end(p, c);
    if (x < rt && max_w_count > 0) {
        int32_t sum = 0;
        int32_t diff = rt - x;
        x = lf;
        int32_t k = 0;
        ui_view_for_each_begin(p, c) {
            const int32_t p_lf = ui.gaps_em2px(c->em.x, c->padding.left);
            const int32_t p_rt = ui.gaps_em2px(c->em.x, c->padding.right);
            if (c->type != ui_view_spacer && c->max_w > 0) {
                // if max_w is infinity, it can occupy the whole parent width:
                const int32_t max_w = (c->max_w == ui.infinity) ? p->w : c->max_w;
                int32_t proportional = (diff * max_w) / (max_w_count * max_w_sum);
                int32_t cw = k == max_w_count - 1 ?
                    diff - sum - p_lf - p_rt : proportional;
                c->w = ut_min(max_w, c->w + cw);
                sum += p_lf + c->w + p_rt;
                k++;
            }
            int32_t cw = p_lf + c->w + p_rt;
            // TODO: take into account .align of a child and adjust x
            //       depending on ui.align.left/right/center
            //       distributing excess width on the left and right of a child
            c->x = p_lf + x;
            x += cw;
        } ui_view_for_each_end(p, c);
        assert(k == max_w_count);
    }
    if (x < rt && spacers > 0) {
        // evenly distribute excess among spacers
        int32_t sum = 0;
        int32_t diff = rt - x;
        int32_t partial = diff / spacers;
        x = lf;
        ui_view_for_each_begin(p, c) {
            const int32_t p_lf = ui.gaps_em2px(c->em.x, c->padding.left);
            const int32_t p_rt = ui.gaps_em2px(c->em.x, c->padding.right);
            if (c->type == ui_view_spacer) {
                c->w = spacers == 1 ? diff - sum - p_lf - p_rt : partial;
                sum += p_lf + c->w + p_rt;
                spacers--;
            }
            c->x = x + p_lf;
            x += p_lf + c->w + p_rt;
        } ui_view_for_each_end(p, c);
    }
}

static void ui_list_measure(ui_view_t* p) {
    swear(p->type == ui_view_list, "type %4.4s 0x%08X", &p->type, p->type);
    const int32_t i_lf = ui.gaps_em2px(p->em.x, p->insets.left);
    const int32_t i_rt = ui.gaps_em2px(p->em.x, p->insets.right);
    const int32_t i_tp = ui.gaps_em2px(p->em.y, p->insets.top);
    const int32_t i_bt = ui.gaps_em2px(p->em.y, p->insets.bottom);
    int32_t max_h = i_tp;
    int32_t h = i_tp;
    int32_t w = 0;
    ui_view_for_each_begin(p, c) {
        swear(c->max_h == 0 || c->max_h >= c->h,
              "max_h: %d h: %d", c->max_h, c->h);
        if (c->type == ui_view_spacer) {
            c->h = 0; // layout will distribute excess here
            max_h = ui.infinity; // spacer make height greedy
        } else {
            const int32_t p_tp = ui.gaps_em2px(c->em.y, c->padding.top);
            const int32_t p_lf = ui.gaps_em2px(c->em.x, c->padding.left);
            const int32_t p_bt = ui.gaps_em2px(c->em.y, c->padding.bottom);
            const int32_t p_rt = ui.gaps_em2px(c->em.x, c->padding.right);
            w = ut_max(w, p_lf + c->w + p_rt);
            const int32_t ch = p_tp + c->h + p_bt;
            if (c->max_h == ui.infinity) {
                max_h = ui.infinity;
            } else if (max_h < ui.infinity && c->max_h != 0) {
                swear(c->max_h >= ch, "Constraint violation: c->max_h < ch, "
                                      "max_h: %d, ch: %d", c->max_h, ch);
                max_h += c->max_h;
            } else if (max_h < ui.infinity) {
                swear(0 <= max_h + ch && max_h + ch < ui.infinity,
                      "Height overflow: max_h + ch = %d", max_h + ch);
                max_h += ch;
            }
            h += ch;
        }
    } ui_view_for_each_end(p, c);
    if (max_h < ui.infinity) {
        swear(0 <= max_h + i_bt && max_h + i_bt < ui.infinity,
             "Height overflow at bottom inset: max_h + bottom = %d",
              max_h + i_bt);
        max_h += i_bt;
    }
    h += i_bt;
    w += i_lf + i_rt;
    // Handle max height only if it differs from actual height
    p->max_h = max_h == h ? p->max_h : ut_max(max_h, p->max_h);
    // do not touch max_w, caller may have set it to something
    swear(max_h == 0 || max_h >= h, "max_h is less than actual height h");
    p->h = h;
    p->w = w;
    // add left and right insets
    p->h += ui.gaps_em2px(p->em.y, p->insets.top);
    p->h += ui.gaps_em2px(p->em.y, p->insets.bottom);
}

static void ui_list_layout(ui_view_t* p) {
    debugln(">%s.(x,y): (%d,%d) .h: %d", p->text, p->x, p->y, p->h);
    swear(p->type == ui_view_list, "type %4.4s 0x%08X", &p->type, p->type);
    int32_t spacers = 0; // Number of spacers
    const int32_t i_tp = ui.gaps_em2px(p->em.y, p->insets.top);
    const int32_t i_bt = ui.gaps_em2px(p->em.y, p->insets.bottom);
    const int32_t i_lf = ui.gaps_em2px(p->em.x, p->insets.left);
    const int32_t i_rt = ui.gaps_em2px(p->em.x, p->insets.right);
    const int32_t top = p->y + i_tp;
    // Mitigation for vertical overflow:
    const int32_t bot = p->y + p->h - i_bt < top ? top + p->h : p->y + p->h - i_bt;
    const int32_t lf = p->x + i_lf;
    // Mitigation for horizontal overflow:
    const int32_t rt = p->x + p->w - i_rt < lf ? lf + p->h : p->x + p->w - i_rt;
    int32_t max_h_sum = 0;
    int32_t max_h_count = 0;
    int32_t y = top;
    ui_view_for_each_begin(p, c) {
        const int32_t p_lf = ui.gaps_em2px(c->em.x, c->padding.left);
        const int32_t p_rt = ui.gaps_em2px(c->em.x, c->padding.right);
        const int32_t p_tp = ui.gaps_em2px(c->em.y, c->padding.top);
        const int32_t p_bt = ui.gaps_em2px(c->em.y, c->padding.bottom);
        if (c->type == ui_view_spacer) {
            c->x = p->x + i_lf + p_lf;
            c->w = rt - lf - p_lf - p_rt;
            spacers++;
        } else {
            // setting child`s max_w to infinity means that child`s height is
            // *always* fill vertical view size of the parent
            // childs.w can exceed parent.w (horizontal overflow) - not encouraged but allowed
            if (c->max_w == ui.infinity) { c->w = p->w - i_lf - i_rt - p_lf - p_rt; }
            if ((c->align & ui.align.left) != 0) {
                c->x = lf + p_lf;
            } else if ((c->align & ui.align.right) != 0) {
                c->x = rt - p_rt - c->w;
            } else {
                const int32_t cw = p_lf + c->w + p_rt;
                c->x = lf + p_lf + (rt - lf - cw) / 2;
            }
            c->y = y + p_tp;
            y = c->y + c->h + p_bt;
            swear(c->max_h == 0 || c->max_h > c->h, "max_h must be greater"
                "than current height: max_h: %d, h: %d", c->max_h, c->h);
            if (c->max_h > 0) {
                max_h_sum += (c->max_h == ui.infinity) ? p->h : c->max_h;
                max_h_count++;
            }
        }
    } ui_view_for_each_end(p, c);
    if (y < bot && max_h_count > 0) {
        int32_t sum = 0;
        int32_t diff = bot - y;
        y = top;
        int32_t k = 0;
        ui_view_for_each_begin(p, c) {
            const int32_t p_tp = ui.gaps_em2px(c->em.y, c->padding.top);
            const int32_t p_bt = ui.gaps_em2px(c->em.y, c->padding.bottom);
            if (c->type != ui_view_spacer && c->max_h > 0) {
                const int32_t max_h = (c->max_h == ui.infinity) ? p->h : c->max_h;
                int32_t proportional = (diff * max_h) / (max_h_count * max_h_sum);
                int32_t ch = k == max_h_count - 1 ?
                        diff - sum - p_tp - p_bt : proportional;
                c->h = ut_min(max_h, c->h + ch);
                sum += p_tp + c->h + p_bt;
                k++;
            }
            int32_t ch = p_tp + c->h + p_bt;
            c->y = y + p_tp;
            y += ch;
        } ui_view_for_each_end(p, c);
        assert(k == max_h_count);
    }
    if (y < bot && spacers > 0) {
        // evenly distribute excess among spacers
        int32_t sum = 0;
        int32_t diff = bot - y;
        int32_t partial = diff / spacers;
        y = top;
        ui_view_for_each_begin(p, c) {
            const int32_t p_tp = ui.gaps_em2px(c->em.y, c->padding.top);
            const int32_t p_bt = ui.gaps_em2px(c->em.y, c->padding.bottom);
            if (c->type == ui_view_spacer) {
                c->h = spacers == 1 ? diff - sum - p_tp - p_bt : partial;
                sum += c->h;
                spacers--;
            }
            int32_t ch = p_tp + c->h + p_bt;
            c->y = y + p_tp;
            y += ch;
        } ui_view_for_each_end(p, c);
    }
    debugln("<%s.(x,y): (%d,%d) .h: %d", p->text, p->x, p->y, p->h);
}

static void ui_container_measure(ui_view_t* p) {
    swear(p->type == ui_view_container, "type %4.4s 0x%08X", &p->type, p->type);
    const int32_t i_lf = ui.gaps_em2px(p->em.x, p->insets.left);
    const int32_t i_rt = ui.gaps_em2px(p->em.x, p->insets.right);
    const int32_t i_tp = ui.gaps_em2px(p->em.y, p->insets.top);
    const int32_t i_bt = ui.gaps_em2px(p->em.y, p->insets.bottom);
    // empty container minimum size:
    if (p != ui_app.view) {
        p->w = i_lf + i_rt;
        p->h = i_tp + i_bt;
    } else { // ui_app.view is special case (expanded to a window)
        p->w = ut_max(p->w, i_lf + i_rt);
        p->h = ut_max(p->h, i_tp + i_bt);
    }
    ui_view_for_each_begin(p, c) {
        const int32_t p_lf = ui.gaps_em2px(c->em.x, c->padding.left);
        const int32_t p_rt = ui.gaps_em2px(c->em.x, c->padding.right);
        const int32_t p_tp = ui.gaps_em2px(c->em.y, c->padding.top);
        const int32_t p_bt = ui.gaps_em2px(c->em.y, c->padding.bottom);
        p->w = ut_max(p->w, p_lf + c->w + p_rt);
        p->h = ut_max(p->h, p_tp + c->h + p_bt);
    } ui_view_for_each_end(p, c);
}

static void ui_container_layout(ui_view_t* p) {
    debugln("> %s %d,%d %dx%d", p->text, p->x, p->y, p->w, p->h);
    swear(p->type == ui_view_container, "type %4.4s 0x%08X", &p->type, p->type);
    const int32_t i_lf = ui.gaps_em2px(p->em.x, p->insets.left);
    const int32_t i_rt = ui.gaps_em2px(p->em.x, p->insets.right);
    const int32_t i_tp = ui.gaps_em2px(p->em.y, p->insets.top);
    const int32_t i_bt = ui.gaps_em2px(p->em.y, p->insets.bottom);
    const int32_t lf = p->x + i_lf;
    const int32_t rt = p->x + p->w - i_rt;
    const int32_t tp = p->y + i_tp;
    const int32_t bt = p->y + p->h - i_bt;
    ui_view_for_each_begin(p, c) {
        if (c->type != ui_view_spacer) {
            const int32_t p_lf = ui.gaps_em2px(c->em.x, c->padding.left);
            const int32_t p_rt = ui.gaps_em2px(c->em.x, c->padding.right);
            const int32_t p_tp = ui.gaps_em2px(c->em.y, c->padding.top);
            const int32_t p_bt = ui.gaps_em2px(c->em.y, c->padding.bottom);

            const int32_t pw = p->w - i_lf - i_rt - p_lf - p_rt;
            const int32_t ph = p->h - i_tp - i_bt - p_tp - p_bt;
            int32_t cw = c->max_w == ui.infinity ? pw : c->max_w;
            if (cw > 0) {
                c->w = ut_min(cw, pw);
            }
            int32_t ch = c->max_h == ui.infinity ? ph : c->max_h;
            if (ch > 0) {
                c->h = ut_min(ch, ph);
            }
            swear((c->align & (ui.align.left|ui.align.right)) !=
                               (ui.align.left|ui.align.right),
                   "Constraint violation align: left|right 0x%02X", c->align);
            swear((c->align & (ui.align.top|ui.align.bottom)) !=
                               (ui.align.top|ui.align.bottom),
                   "Constraint violation align: top|bottom 0x%02X", c->align);
            if ((c->align & ui.align.left) != 0) {
                c->x = lf + p_lf;
            } else if ((c->align & ui.align.right) != 0) {
                c->x = rt - c->w - p_rt;
            } else {
                const int32_t w = rt - lf; // effective width
                c->x = lf + p_lf + (w - (p_lf + c->w + p_rt)) / 2;
            }
            if ((c->align & ui.align.top) != 0) {
                c->y = tp + p_tp;
            } else if ((c->align & ui.align.bottom) != 0) {
                c->y = bt - c->h - p_bt;
            } else {
                const int32_t h = bt - tp; // effective height
                c->y = tp + p_tp + (h - (p_tp + c->h + p_bt)) / 2;
            }
        }
    } ui_view_for_each_end(p, c);
    debugln("<%s %d,%d %dx%d", p->text, p->x, p->y, p->w, p->h);
}

static void ui_container_paint(ui_view_t* v) {
//  traceln("%s 0x%016llX", v->text, v->color);
    if (!ui_color_is_transparent(v->color)) {
        ui_gdi.fill_with(v->x, v->y, v->w, v->h, v->color);
    }
}

void ui_view_init_span(ui_view_t* v) {
    swear(v->type == ui_view_span, "type %4.4s 0x%08X", &v->type, v->type);
    ui_view_init_container(v);
    v->measure = ui_span_measure;
    v->layout  = ui_span_layout;
    if (v->text[0] == 0) { strprintf(v->text, "ui_span"); }
}

void ui_view_init_list(ui_view_t* v) {
    swear(v->type == ui_view_list, "type %4.4s 0x%08X", &v->type, v->type);
    ui_view_init_container(v);
    // TODO: not sure about default insets
    v->insets  = (ui_gaps_t){ .left = 0.5, .top = 0.25, .right = 0.5, .bottom = 0.25 };
    v->measure = ui_list_measure;
    v->layout  = ui_list_layout;
    if (v->text[0] == 0) { strprintf(v->text, "ui_list"); }
}

void ui_view_init_spacer(ui_view_t* v) {
    swear(v->type == ui_view_spacer, "type %4.4s 0x%08X", &v->type, v->type);
    ui_view_init(v);
    v->w = 0;
    v->h = 0;
    v->max_w = ui.infinity;
    v->max_h = ui.infinity;
    if (v->text[0] == 0) { strprintf(v->text, "ui_spacer"); }
}

void ui_view_init_container(ui_view_t* v) {
    ui_view_init(v);
    v->color   = ui_color_transparent;
    v->insets  = (ui_gaps_t){ .left  = 0.25, .top    = 0.25,
                              .right = 0.25, .bottom = 0.25 };
    // do not overwrite if already set
    if (v->measure == null) { v->measure = ui_container_measure; }
    if (v->layout  == null) { v->layout  = ui_container_layout; }
    if (v->paint   == null) { v->paint   = ui_container_paint; }
    if (v->text[0] == 0) { strprintf(v->text, "ui_container"); }
}

#pragma pop_macro("dump")
#pragma pop_macro("debugln")
// ________________________________ ui_core.c _________________________________

#include "ut/ut.h"
#include "ut/ut_win32.h"

#define UI_WM_ANIMATE  (WM_APP + 0x7FFF)
#define UI_WM_OPENING  (WM_APP + 0x7FFE)
#define UI_WM_CLOSING  (WM_APP + 0x7FFD)
#define UI_WM_TAP      (WM_APP + 0x7FFC)
#define UI_WM_DTAP     (WM_APP + 0x7FFB) // fp64_t tap (aka click)
#define UI_WM_PRESS    (WM_APP + 0x7FFA)

static bool ui_point_in_rect(const ui_point_t* p, const ui_rect_t* r) {
    return r->x <= p->x && p->x < r->x + r->w &&
           r->y <= p->y && p->y < r->y + r->h;
}

static bool ui_intersect_rect(ui_rect_t* i, const ui_rect_t* r0,
                                     const ui_rect_t* r1) {
    ui_rect_t r = {0};
    r.x = ut_max(r0->x, r1->x);  // Maximum of left edges
    r.y = ut_max(r0->y, r1->y);  // Maximum of top edges
    r.w = ut_min(r0->x + r0->w, r1->x + r1->w) - r.x;  // Width of overlap
    r.h = ut_min(r0->y + r0->h, r1->y + r1->h) - r.y;  // Height of overlap
    bool b = r.w > 0 && r.h > 0;
    if (!b) {
        r.w = 0;
        r.h = 0;
    }
    if (i != null) { *i = r; }
    return b;
}

static int32_t ui_gaps_em2px(int32_t em, fp32_t ratio) {
    return em == 0 ? 0 : (int32_t)(em * ratio + 0.5f);
}

extern ui_if ui = {
    .infinity = INT32_MAX,
    .align = {
        .center = 0,
        .left   = 0x01,
        .top    = 0x02,
        .right  = 0x10,
        .bottom = 0x20
    },
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
        .mouse_hover           = WM_MOUSEHOVER,
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
    .hit_test = {
        .error             = HTERROR,
        .transparent       = HTTRANSPARENT,
        .nowhere           = HTNOWHERE,
        .client            = HTCLIENT,
        .caption           = HTCAPTION,
        .system_menu       = HTSYSMENU,
        .grow_box          = HTGROWBOX,
        .menu              = HTMENU,
        .horizontal_scroll = HTHSCROLL,
        .vertical_scroll   = HTVSCROLL,
        .min_button        = HTMINBUTTON,
        .max_button        = HTMAXBUTTON,
        .left              = HTLEFT,
        .right             = HTRIGHT,
        .top               = HTTOP,
        .top_left          = HTTOPLEFT,
        .top_right         = HTTOPRIGHT,
        .bottom            = HTBOTTOM,
        .bottom_left       = HTBOTTOMLEFT,
        .bottom_right      = HTBOTTOMRIGHT,
        .border            = HTBORDER,
        .object            = HTOBJECT,
        .close             = HTCLOSE,
        .help              = HTHELP
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
    },
    .point_in_rect  = ui_point_in_rect,
    .intersect_rect = ui_intersect_rect,
    .gaps_em2px     = ui_gaps_em2px
};

// https://docs.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-showwindow
// _________________________________ ui_gdi.c _________________________________

#include "ut/ut.h"
#include "ut/ut_win32.h"

#pragma push_macro("app_window")
#pragma push_macro("app_canvas")
#pragma push_macro("gdi_with_hdc")
#pragma push_macro("gdi_hdc_with_font")

#define ui_app_window() ((HWND)ui_app.window)
#define ui_app_canvas() ((HDC)ui_app.canvas)

typedef struct ui_gdi_xyc_s {
    int32_t x;
    int32_t y;
    ui_color_t c;
} ui_gdi_xyc_t;

static int32_t ui_gdi_top;
static ui_gdi_xyc_t ui_gdi_stack[256];

static void ui_gdi_init(void) {
    ui_gdi.brush_hollow = (ui_brush_t)GetStockBrush(HOLLOW_BRUSH);
    ui_gdi.brush_color  = (ui_brush_t)GetStockBrush(DC_BRUSH);
    ui_gdi.pen_hollow = (ui_pen_t)GetStockPen(NULL_PEN);
}

static uint32_t ui_gdi_color_rgb(ui_color_t c) {
    assert(ui_color_is_8bit(c));
    return (COLORREF)(c & 0xFFFFFFFF);
}

static COLORREF ui_gdi_color_ref(ui_color_t c) {
    return ui_gdi.color_rgb(c);
}

static ui_color_t ui_gdi_set_text_color(ui_color_t c) {
    return SetTextColor(ui_app_canvas(), ui_gdi_color_ref(c));
}

static ui_pen_t ui_gdi_set_pen(ui_pen_t p) {
    not_null(p);
    return (ui_pen_t)SelectPen(ui_app_canvas(), (HPEN)p);
}

static ui_pen_t ui_gdi_set_colored_pen(ui_color_t c) {
    ui_pen_t p = (ui_pen_t)SelectPen(ui_app_canvas(), GetStockPen(DC_PEN));
    SetDCPenColor(ui_app_canvas(), ui_gdi_color_ref(c));
    return p;
}

static ui_pen_t ui_gdi_create_pen(ui_color_t c, int32_t width) {
    assert(width >= 1);
    ui_pen_t pen = (ui_pen_t)CreatePen(PS_SOLID, width, ui_gdi_color_ref(c));
    not_null(pen);
    return pen;
}

static void ui_gdi_delete_pen(ui_pen_t p) {
    fatal_if_false(DeletePen(p));
}

static ui_brush_t ui_gdi_create_brush(ui_color_t c) {
    return (ui_brush_t)CreateSolidBrush(ui_gdi_color_ref(c));
}

static void ui_gdi_delete_brush(ui_brush_t b) {
    DeleteBrush((HBRUSH)b);
}

static ui_brush_t ui_gdi_set_brush(ui_brush_t b) {
    not_null(b);
    return (ui_brush_t)SelectBrush(ui_app_canvas(), b);
}

static ui_color_t ui_gdi_set_brush_color(ui_color_t c) {
    return SetDCBrushColor(ui_app_canvas(), ui_gdi_color_ref(c));
}

static void ui_gdi_set_clip(int32_t x, int32_t y, int32_t w, int32_t h) {
    if (ui_gdi.clip != null) { DeleteRgn(ui_gdi.clip); ui_gdi.clip = null; }
    if (w > 0 && h > 0) {
        ui_gdi.clip = (ui_region_t)CreateRectRgn(x, y, x + w, y + h);
        not_null(ui_gdi.clip);
    }
    fatal_if(SelectClipRgn(ui_app_canvas(), (HRGN)ui_gdi.clip) == ERROR);
}

static void ui_gdi_push(int32_t x, int32_t y) {
    assert(ui_gdi_top < countof(ui_gdi_stack));
    fatal_if(ui_gdi_top >= countof(ui_gdi_stack));
    ui_gdi_stack[ui_gdi_top].x = ui_gdi.x;
    ui_gdi_stack[ui_gdi_top].y = ui_gdi.y;
    fatal_if(SaveDC(ui_app_canvas()) == 0);
    ui_gdi_top++;
    ui_gdi.x = x;
    ui_gdi.y = y;
}

static void ui_gdi_pop(void) {
    assert(0 < ui_gdi_top && ui_gdi_top <= countof(ui_gdi_stack));
    fatal_if(ui_gdi_top <= 0);
    ui_gdi_top--;
    ui_gdi.x = ui_gdi_stack[ui_gdi_top].x;
    ui_gdi.y = ui_gdi_stack[ui_gdi_top].y;
    fatal_if_false(RestoreDC(ui_app_canvas(), -1));
}

static void ui_gdi_pixel(int32_t x, int32_t y, ui_color_t c) {
    not_null(ui_app.canvas);
    fatal_if_false(SetPixel(ui_app_canvas(), x, y, ui_gdi_color_ref(c)));
}

static ui_point_t ui_gdi_move_to(int32_t x, int32_t y) {
    POINT pt;
    pt.x = ui_gdi.x;
    pt.y = ui_gdi.y;
    fatal_if_false(MoveToEx(ui_app_canvas(), x, y, &pt));
    ui_gdi.x = x;
    ui_gdi.y = y;
    ui_point_t p = { pt.x, pt.y };
    return p;
}

static void ui_gdi_line(int32_t x, int32_t y) {
    fatal_if_false(LineTo(ui_app_canvas(), x, y));
    ui_gdi.x = x;
    ui_gdi.y = y;
}

static void ui_gdi_frame(int32_t x, int32_t y, int32_t w, int32_t h) {
    ui_brush_t b = ui_gdi.set_brush(ui_gdi.brush_hollow);
    ui_gdi.rect(x, y, w, h);
    ui_gdi.set_brush(b);
}

static void ui_gdi_rect(int32_t x, int32_t y, int32_t w, int32_t h) {
    fatal_if_false(Rectangle(ui_app_canvas(), x, y, x + w, y + h));
}

static void ui_gdi_fill(int32_t x, int32_t y, int32_t w, int32_t h) {
    RECT rc = { x, y, x + w, y + h };
    ui_brush_t b = (ui_brush_t)GetCurrentObject(ui_app_canvas(), OBJ_BRUSH);
    fatal_if_false(FillRect(ui_app_canvas(), &rc, (HBRUSH)b));
}

static void ui_gdi_frame_with(int32_t x, int32_t y, int32_t w, int32_t h,
        ui_color_t c) {
    ui_brush_t b = ui_gdi.set_brush(ui_gdi.brush_hollow);
    ui_pen_t p = ui_gdi.set_colored_pen(c);
    ui_gdi.rect(x, y, w, h);
    ui_gdi.set_pen(p);
    ui_gdi.set_brush(b);
}

static void ui_gdi_rect_with(int32_t x, int32_t y, int32_t w, int32_t h,
        ui_color_t border, ui_color_t fill) {
    ui_brush_t b = ui_gdi.set_brush(ui_gdi.brush_color);
    ui_color_t c = ui_gdi.set_brush_color(fill);
    ui_pen_t p = ui_gdi.set_colored_pen(border);
    ui_gdi.rect(x, y, w, h);
    ui_gdi.set_brush_color(c);
    ui_gdi.set_pen(p);
    ui_gdi.set_brush(b);
}

static void ui_gdi_fill_with(int32_t x, int32_t y, int32_t w, int32_t h,
        ui_color_t c) {
    ui_brush_t b = ui_gdi.set_brush(ui_gdi.brush_color);
    c = ui_gdi.set_brush_color(c);
    ui_gdi.fill(x, y, w, h);
    ui_gdi.set_brush_color(c);
    ui_gdi.set_brush(b);
}

static void ui_gdi_poly(ui_point_t* points, int32_t count) {
    // make sure ui_point_t and POINT have the same memory layout:
    static_assert(sizeof(points->x) == sizeof(((POINT*)0)->x), "ui_point_t");
    static_assert(sizeof(points->y) == sizeof(((POINT*)0)->y), "ui_point_t");
    static_assert(sizeof(points[0]) == sizeof(*((POINT*)0)), "ui_point_t");
    assert(ui_app_canvas() != null && count > 1);
    fatal_if_false(Polyline(ui_app_canvas(), (POINT*)points, count));
}

static void ui_gdi_rounded(int32_t x, int32_t y, int32_t w, int32_t h,
        int32_t rx, int32_t ry) {
    fatal_if_false(RoundRect(ui_app_canvas(), x, y, x + w, y + h, rx, ry));
}

static void ui_gdi_gradient(int32_t x, int32_t y, int32_t w, int32_t h,
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
    GradientFill(ui_app_canvas(), vertex, 2, &gRect, 1, mode);
}

static BITMAPINFO* ui_gdi_greyscale_bitmap_info(void) {
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

static void ui_gdi_draw_greyscale(int32_t sx, int32_t sy, int32_t sw, int32_t sh,
        int32_t x, int32_t y, int32_t w, int32_t h,
        int32_t iw, int32_t ih, int32_t stride, const uint8_t* pixels) {
    fatal_if(stride != ((iw + 3) & ~0x3));
    assert(w > 0 && h != 0); // h can be negative
    if (w > 0 && h != 0) {
        BITMAPINFO *bi = ui_gdi_greyscale_bitmap_info(); // global! not thread safe
        BITMAPINFOHEADER* bih = &bi->bmiHeader;
        bih->biWidth = iw;
        bih->biHeight = -ih; // top down image
        bih->biSizeImage = w * h;
        POINT pt = { 0 };
        fatal_if_false(SetBrushOrgEx(ui_app_canvas(), 0, 0, &pt));
        fatal_if(StretchDIBits(ui_app_canvas(), sx, sy, sw, sh, x, y, w, h,
            pixels, bi, DIB_RGB_COLORS, SRCCOPY) == 0);
        fatal_if_false(SetBrushOrgEx(ui_app_canvas(), pt.x, pt.y, &pt));
    }
}

static BITMAPINFOHEADER ui_gdi_bgrx_init_bi(int32_t w, int32_t h, int32_t bpp) {
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
// if this is not the case use ui_gdi.image_init() that will unpack
// and align scanlines prior to draw

static void ui_gdi_draw_bgr(int32_t sx, int32_t sy, int32_t sw, int32_t sh,
        int32_t x, int32_t y, int32_t w, int32_t h,
        int32_t iw, int32_t ih, int32_t stride,
        const uint8_t* pixels) {
    fatal_if(stride != ((iw * 3 + 3) & ~0x3));
    assert(w > 0 && h != 0); // h can be negative
    if (w > 0 && h != 0) {
        BITMAPINFOHEADER bi = ui_gdi_bgrx_init_bi(iw, ih, 3);
        POINT pt = { 0 };
        fatal_if_false(SetBrushOrgEx(ui_app_canvas(), 0, 0, &pt));
        fatal_if(StretchDIBits(ui_app_canvas(), sx, sy, sw, sh, x, y, w, h,
            pixels, (BITMAPINFO*)&bi, DIB_RGB_COLORS, SRCCOPY) == 0);
        fatal_if_false(SetBrushOrgEx(ui_app_canvas(), pt.x, pt.y, &pt));
    }
}

static void ui_gdi_draw_bgrx(int32_t sx, int32_t sy, int32_t sw, int32_t sh,
        int32_t x, int32_t y, int32_t w, int32_t h,
        int32_t iw, int32_t ih, int32_t stride,
        const uint8_t* pixels) {
    fatal_if(stride != ((iw * 4 + 3) & ~0x3));
    assert(w > 0 && h != 0); // h can be negative
    if (w > 0 && h != 0) {
        BITMAPINFOHEADER bi = ui_gdi_bgrx_init_bi(iw, ih, 4);
        POINT pt = { 0 };
        fatal_if_false(SetBrushOrgEx(ui_app_canvas(), 0, 0, &pt));
        fatal_if(StretchDIBits(ui_app_canvas(), sx, sy, sw, sh, x, y, w, h,
            pixels, (BITMAPINFO*)&bi, DIB_RGB_COLORS, SRCCOPY) == 0);
        fatal_if_false(SetBrushOrgEx(ui_app_canvas(), pt.x, pt.y, &pt));
    }
}

static BITMAPINFO* ui_gdi_init_bitmap_info(int32_t w, int32_t h, int32_t bpp,
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

static void ui_gdi_create_dib_section(ui_image_t* image, int32_t w, int32_t h,
        int32_t bpp) {
    fatal_if(image->bitmap != null, "image_dispose() not called?");
    // not using GetWindowDC(ui_app_window()) will allow to initialize images
    // before window is created
    HDC c = CreateCompatibleDC(null); // GetWindowDC(ui_app_window());
    BITMAPINFO local = { {sizeof(BITMAPINFOHEADER)} };
    BITMAPINFO* bi = bpp == 1 ? ui_gdi_greyscale_bitmap_info() : &local;
    image->bitmap = (ui_bitmap_t)CreateDIBSection(c, ui_gdi_init_bitmap_info(w, h, bpp, bi),
                                               DIB_RGB_COLORS, &image->pixels, null, 0x0);
    fatal_if(image->bitmap == null || image->pixels == null);
//  fatal_if_false(ReleaseDC(ui_app_window(), c));
    fatal_if_false(DeleteDC(c));
}

static void ui_gdi_image_init_rgbx(ui_image_t* image, int32_t w, int32_t h,
        int32_t bpp, const uint8_t* pixels) {
    bool swapped = bpp < 0;
    bpp = abs(bpp);
    fatal_if(bpp != 4, "bpp: %d", bpp);
    ui_gdi_create_dib_section(image, w, h, bpp);
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

static void ui_gdi_image_init(ui_image_t* image, int32_t w, int32_t h, int32_t bpp,
        const uint8_t* pixels) {
    bool swapped = bpp < 0;
    bpp = abs(bpp);
    fatal_if(bpp < 0 || bpp == 2 || bpp > 4, "bpp=%d not {1, 3, 4}", bpp);
    ui_gdi_create_dib_section(image, w, h, bpp);
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

static void ui_gdi_alpha_blend(int32_t x, int32_t y, int32_t w, int32_t h,
        ui_image_t* image, fp64_t alpha) {
    assert(image->bpp > 0);
    assert(0 <= alpha && alpha <= 1);
    not_null(ui_app_canvas());
    HDC c = CreateCompatibleDC(ui_app_canvas());
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
    fatal_if_false(AlphaBlend(ui_app_canvas(), x, y, w, h,
        c, 0, 0, image->w, image->h, bf));
    SelectBitmap((HDC)c, zero1x1);
    fatal_if_false(DeleteDC(c));
}

static void ui_gdi_draw_image(int32_t x, int32_t y, int32_t w, int32_t h,
        ui_image_t* image) {
    assert(image->bpp == 1 || image->bpp == 3 || image->bpp == 4);
    not_null(ui_app_canvas());
    if (image->bpp == 1) { // StretchBlt() is bad for greyscale
        BITMAPINFO* bi = ui_gdi_greyscale_bitmap_info();
        fatal_if(StretchDIBits(ui_app_canvas(), x, y, w, h, 0, 0, image->w, image->h,
            image->pixels, ui_gdi_init_bitmap_info(image->w, image->h, 1, bi),
            DIB_RGB_COLORS, SRCCOPY) == 0);
    } else {
        HDC c = CreateCompatibleDC(ui_app_canvas());
        not_null(c);
        HBITMAP zero1x1 = SelectBitmap(c, image->bitmap);
        fatal_if_false(StretchBlt(ui_app_canvas(), x, y, w, h,
            c, 0, 0, image->w, image->h, SRCCOPY));
        SelectBitmap(c, zero1x1);
        fatal_if_false(DeleteDC(c));
    }
}

static void ui_gdi_draw_icon(int32_t x, int32_t y, int32_t w, int32_t h,
        ui_icon_t icon) {
    DrawIconEx(ui_app_canvas(), x, y, (HICON)icon, w, h, 0, NULL, DI_NORMAL | DI_COMPAT);
}

static void ui_gdi_cleartype(bool on) {
    enum { spif = SPIF_UPDATEINIFILE | SPIF_SENDCHANGE };
    fatal_if_false(SystemParametersInfoA(SPI_SETFONTSMOOTHING, true, 0, spif));
    uintptr_t s = on ? FE_FONTSMOOTHINGCLEARTYPE : FE_FONTSMOOTHINGSTANDARD;
    fatal_if_false(SystemParametersInfoA(SPI_SETFONTSMOOTHINGTYPE, 0,
        (void*)s, spif));
}

static void ui_gdi_font_smoothing_contrast(int32_t c) {
    fatal_if(!(c == -1 || 1000 <= c && c <= 2200), "contrast: %d", c);
    if (c == -1) { c = 1400; }
    fatal_if_false(SystemParametersInfoA(SPI_SETFONTSMOOTHINGCONTRAST, 0,
                   (void*)(uintptr_t)c, SPIF_UPDATEINIFILE | SPIF_SENDCHANGE));
}

static_assertion(ui_gdi_font_quality_default == DEFAULT_QUALITY);
static_assertion(ui_gdi_font_quality_draft == DRAFT_QUALITY);
static_assertion(ui_gdi_font_quality_proof == PROOF_QUALITY);
static_assertion(ui_gdi_font_quality_nonantialiased == NONANTIALIASED_QUALITY);
static_assertion(ui_gdi_font_quality_antialiased == ANTIALIASED_QUALITY);
static_assertion(ui_gdi_font_quality_cleartype == CLEARTYPE_QUALITY);
static_assertion(ui_gdi_font_quality_cleartype_natural == CLEARTYPE_NATURAL_QUALITY);

static ui_font_t ui_gdi_create_font(const char* family, int32_t height, int32_t quality) {
    assert(height > 0);
    LOGFONTA lf = {0};
    int32_t n = GetObjectA(ui_app.fonts.regular, sizeof(lf), &lf);
    fatal_if_false(n == (int32_t)sizeof(lf));
    lf.lfHeight = -height;
    strprintf(lf.lfFaceName, "%s", family);
    if (ui_gdi_font_quality_default <= quality && quality <= ui_gdi_font_quality_cleartype_natural) {
        lf.lfQuality = (uint8_t)quality;
    } else {
        fatal_if(quality != -1, "use -1 for do not care quality");
    }
    return (ui_font_t)CreateFontIndirectA(&lf);
}


static ui_font_t ui_gdi_font(ui_font_t f, int32_t height, int32_t quality) {
    assert(f != null && height > 0);
    LOGFONTA lf = {0};
    int32_t n = GetObjectA(f, sizeof(lf), &lf);
    fatal_if_false(n == (int32_t)sizeof(lf));
    lf.lfHeight = -height;
    if (ui_gdi_font_quality_default <= quality && quality <= ui_gdi_font_quality_cleartype_natural) {
        lf.lfQuality = (uint8_t)quality;
    } else {
        fatal_if(quality != -1, "use -1 for do not care quality");
    }
    return (ui_font_t)CreateFontIndirectA(&lf);
}

static int32_t ui_gdi_font_height(ui_font_t f) {
    assert(f != null);
    LOGFONTA lf = {0};
    int32_t n = GetObjectA(f, sizeof(lf), &lf);
    fatal_if_false(n == (int32_t)sizeof(lf));
    assert(lf.lfHeight < 0);
    return abs(lf.lfHeight);
}

static void ui_gdi_delete_font(ui_font_t f) {
    fatal_if_false(DeleteFont(f));
}

static ui_font_t ui_gdi_set_font(ui_font_t f) {
    not_null(f);
    return (ui_font_t)SelectFont(ui_app_canvas(), (HFONT)f);
}

#define ui_gdi_with_hdc(code) do {                                          \
    not_null(ui_app_window());                                              \
    HDC hdc = ui_app_canvas() != null ? ui_app_canvas() : GetDC(ui_app_window()); \
    not_null(hdc);                                                       \
    code                                                                 \
    if (ui_app_canvas() == null) {                                          \
        ReleaseDC(ui_app_window(), hdc);                                    \
    }                                                                    \
} while (0);

#define ui_gdi_hdc_with_font(f, ...) do {                                   \
    not_null(f);                                                         \
    not_null(ui_app_window());                                              \
    HDC hdc = ui_app_canvas() != null ? ui_app_canvas() : GetDC(ui_app_window()); \
    not_null(hdc);                                                       \
    HFONT _font_ = SelectFont(hdc, (HFONT)f);                            \
    { __VA_ARGS__ }                                                      \
    SelectFont(hdc, _font_);                                             \
    if (ui_app_canvas() == null) {                                          \
        ReleaseDC(ui_app_window(), hdc);                                        \
    }                                                                    \
} while (0);


static int32_t ui_gdi_baseline(ui_font_t f) {
    TEXTMETRICA tm;
    ui_gdi_hdc_with_font(f, {
        fatal_if_false(GetTextMetricsA(hdc, &tm));
    })
    return tm.tmAscent;
}

static int32_t ui_gdi_descent(ui_font_t f) {
    TEXTMETRICA tm;
    ui_gdi_hdc_with_font(f, {
        fatal_if_false(GetTextMetricsA(hdc, &tm));
    });
    return tm.tmDescent;
}

// get_em() is relatively expensive:
// 24 microseconds Core i-7 3667U 2.0 GHz (2012)
// but in small app with few views the number
// of calls to get_em() is very small on layout.
// Few dozens at most. No reason to cache or optimize.

static ui_point_t ui_gdi_get_em(ui_font_t f) {
    SIZE cell = {0, 0};
    int32_t height   = 0;
    int32_t descent  = 0;
    int32_t baseline = 0;
    ui_gdi_hdc_with_font(f, {
        // ui_glyph_nbsp and "M" have the same result
        fatal_if_false(GetTextExtentPoint32A(hdc, "M", 1, &cell));
        height = ui_gdi.font_height(f);
        descent = ui_gdi.descent(f);
        baseline = ui_gdi.baseline(f);
    });
    assert(baseline >= height);
    ui_point_t c = {cell.cx, cell.cy - descent - (height - baseline)};
    return c;
}

static bool ui_gdi_is_mono(ui_font_t f) {
    SIZE em = {0}; // "M"
    SIZE vl = {0}; // "|" Vertical Line https://www.compart.com/en/unicode/U+007C
    SIZE e3 = {0}; // "\xE2\xB8\xBB" Three-Em Dash https://www.compart.com/en/unicode/U+2E3B
    ui_gdi_hdc_with_font(f, {
        fatal_if_false(GetTextExtentPoint32A(hdc, "M", 1, &em));
        fatal_if_false(GetTextExtentPoint32A(hdc, "|", 1, &vl));
        fatal_if_false(GetTextExtentPoint32A(hdc, "\xE2\xB8\xBB", 1, &e3));
    });
    return em.cx == vl.cx && vl.cx == e3.cx;
}

static fp64_t ui_gdi_line_spacing(fp64_t height_multiplier) {
    assert(0.1 <= height_multiplier && height_multiplier <= 2.0);
    fp64_t hm = ui_gdi.height_multiplier;
    ui_gdi.height_multiplier = height_multiplier;
    return hm;
}

static int32_t ui_gdi_draw_utf16(ui_font_t font, const char* s, int32_t n,
        RECT* r, uint32_t format) { // ~70 microsecond Core i-7 3667U 2.0 GHz (2012)
    // if font == null, draws on HDC with selected font
    int32_t height = 0; // return value is the height of the text in logical units
    if (font != null) {
        ui_gdi_hdc_with_font(font, {
            height = DrawTextW(hdc, utf8to16(s), n, r, format);
        });
    } else {
        ui_gdi_with_hdc({
            height = DrawTextW(hdc, utf8to16(s), n, r, format);
        });
    }
    return height;
}

typedef struct { // draw text parameters
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
} ui_gdi_dtp_t;

static void ui_gdi_text_draw(ui_gdi_dtp_t* p) {
    int32_t n = 1024;
    char* text = (char*)ut_stackalloc(n);
    ut_str.format_va(text, n - 1, p->format, p->vl);
    int32_t k = (int32_t)strlen(text);
    // Microsoft returns -1 not posix required sizeof buffer
    while (k >= n - 1 || k < 0) {
        n = n * 2;
        text = (char*)ut_stackalloc(n);
        ut_str.format_va(text, n - 1, p->format, p->vl);
        k = (int32_t)strlen(text);
    }
    assert(k >= 0 && k <= n, "k=%d n=%d fmt=%s", k, n, p->format);
    // rectangle is always calculated - it makes draw text
    // much slower but UI layer is mostly uses bitmap caching:
    if ((p->flags & DT_CALCRECT) == 0) {
        // no actual drawing just calculate rectangle
        bool b = ui_gdi_draw_utf16(p->font, text, -1, &p->rc, p->flags | DT_CALCRECT);
        assert(b, "draw_text_utf16(%s) failed", text); (void)b;
    }
    bool b = ui_gdi_draw_utf16(p->font, text, -1, &p->rc, p->flags);
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

static ui_point_t ui_gdi_text_measure(ui_gdi_dtp_t* p) {
    ui_gdi_text_draw(p);
    ui_point_t cell = {p->rc.right - p->rc.left, p->rc.bottom - p->rc.top};
    return cell;
}

static ui_point_t ui_gdi_measure_singleline(ui_font_t f, const char* format, ...) {
    va_list vl;
    va_start(vl, format);
    ui_gdi_dtp_t p = { f, format, vl, {0, 0, 0, 0}, sl_measure };
    ui_point_t cell = ui_gdi_text_measure(&p);
    va_end(vl);
    return cell;
}

static ui_point_t ui_gdi_measure_multiline(ui_font_t f, int32_t w, const char* format, ...) {
    va_list vl;
    va_start(vl, format);
    uint32_t flags = w <= 0 ? ml_measure : ml_measure_break;
    ui_gdi_dtp_t p = { f, format, vl, {ui_gdi.x, ui_gdi.y, ui_gdi.x + (w <= 0 ? 1 : w), ui_gdi.y}, flags };
    ui_point_t cell = ui_gdi_text_measure(&p);
    va_end(vl);
    return cell;
}

static void ui_gdi_vtext(const char* format, va_list vl) {
    ui_gdi_dtp_t p = { null, format, vl, {ui_gdi.x, ui_gdi.y, 0, 0}, sl_draw };
    ui_gdi_text_draw(&p);
    ui_gdi.x += p.rc.right - p.rc.left;
}

static void ui_gdi_vtextln(const char* format, va_list vl) {
    ui_gdi_dtp_t p = { null, format, vl, {ui_gdi.x, ui_gdi.y, ui_gdi.x, ui_gdi.y}, sl_draw };
    ui_gdi_text_draw(&p);
    ui_gdi.y += (int32_t)((p.rc.bottom - p.rc.top) * ui_gdi.height_multiplier + 0.5f);
}

static void ui_gdi_text(const char* format, ...) {
    va_list vl;
    va_start(vl, format);
    ui_gdi.vtext(format, vl);
    va_end(vl);
}

static void ui_gdi_textln(const char* format, ...) {
    va_list vl;
    va_start(vl, format);
    ui_gdi.vtextln(format, vl);
    va_end(vl);
}

static ui_point_t ui_gdi_multiline(int32_t w, const char* f, ...) {
    va_list vl;
    va_start(vl, f);
    uint32_t flags = w <= 0 ? ml_draw : ml_draw_break;
    ui_gdi_dtp_t p = { null, f, vl, {ui_gdi.x, ui_gdi.y, ui_gdi.x + (w <= 0 ? 1 : w), ui_gdi.y}, flags };
    ui_gdi_text_draw(&p);
    va_end(vl);
    ui_point_t c = { p.rc.right - p.rc.left, p.rc.bottom - p.rc.top };
    return c;
}

static void ui_gdi_vprint(const char* format, va_list vl) {
    not_null(ui_app.fonts.mono);
    ui_font_t f = ui_gdi.set_font(ui_app.fonts.mono);
    ui_gdi.vtext(format, vl);
    ui_gdi.set_font(f);
}

static void ui_gdi_print(const char* format, ...) {
    va_list vl;
    va_start(vl, format);
    ui_gdi.vprint(format, vl);
    va_end(vl);
}

static void ui_gdi_vprintln(const char* format, va_list vl) {
    not_null(ui_app.fonts.mono);
    ui_font_t f = ui_gdi.set_font(ui_app.fonts.mono);
    ui_gdi.vtextln(format, vl);
    ui_gdi.set_font(f);
}

static void ui_gdi_println(const char* format, ...) {
    va_list vl;
    va_start(vl, format);
    ui_gdi.vprintln(format, vl);
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

static uint8_t* ui_gdi_load_image(const void* data, int32_t bytes, int* w, int* h,
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

static void ui_gdi_image_dispose(ui_image_t* image) {
    fatal_if_false(DeleteBitmap(image->bitmap));
    memset(image, 0, sizeof(ui_image_t));
}

ui_gdi_if ui_gdi = {
    .height_multiplier             = 1.0,
    .init                          = ui_gdi_init,
    .color_rgb                     = ui_gdi_color_rgb,
    .image_init                    = ui_gdi_image_init,
    .image_init_rgbx               = ui_gdi_image_init_rgbx,
    .image_dispose                 = ui_gdi_image_dispose,
    .alpha_blend                   = ui_gdi_alpha_blend,
    .draw_image                    = ui_gdi_draw_image,
    .draw_icon                     = ui_gdi_draw_icon,
    .set_text_color                = ui_gdi_set_text_color,
    .create_brush                  = ui_gdi_create_brush,
    .delete_brush                  = ui_gdi_delete_brush,
    .set_brush                     = ui_gdi_set_brush,
    .set_brush_color               = ui_gdi_set_brush_color,
    .set_colored_pen               = ui_gdi_set_colored_pen,
    .create_pen                    = ui_gdi_create_pen,
    .set_pen                       = ui_gdi_set_pen,
    .delete_pen                    = ui_gdi_delete_pen,
    .set_clip                      = ui_gdi_set_clip,
    .push                          = ui_gdi_push,
    .pop                           = ui_gdi_pop,
    .pixel                         = ui_gdi_pixel,
    .move_to                       = ui_gdi_move_to,
    .line                          = ui_gdi_line,
    .frame                         = ui_gdi_frame,
    .rect                          = ui_gdi_rect,
    .fill                          = ui_gdi_fill,
    .frame_with                    = ui_gdi_frame_with,
    .rect_with                     = ui_gdi_rect_with,
    .fill_with                     = ui_gdi_fill_with,
    .poly                          = ui_gdi_poly,
    .rounded                       = ui_gdi_rounded,
    .gradient                      = ui_gdi_gradient,
    .draw_greyscale                = ui_gdi_draw_greyscale,
    .draw_bgr                      = ui_gdi_draw_bgr,
    .draw_bgrx                     = ui_gdi_draw_bgrx,
    .cleartype                     = ui_gdi_cleartype,
    .font_smoothing_contrast       = ui_gdi_font_smoothing_contrast,
    .create_font                   = ui_gdi_create_font,
    .font                          = ui_gdi_font,
    .delete_font                   = ui_gdi_delete_font,
    .set_font                      = ui_gdi_set_font,
    .font_height                   = ui_gdi_font_height,
    .descent                       = ui_gdi_descent,
    .baseline                      = ui_gdi_baseline,
    .is_mono                       = ui_gdi_is_mono,
    .get_em                        = ui_gdi_get_em,
    .line_spacing                  = ui_gdi_line_spacing,
    .measure_text                  = ui_gdi_measure_singleline,
    .measure_multiline             = ui_gdi_measure_multiline,
    .vtext                         = ui_gdi_vtext,
    .vtextln                       = ui_gdi_vtextln,
    .text                          = ui_gdi_text,
    .textln                        = ui_gdi_textln,
    .vprint                        = ui_gdi_vprint,
    .vprintln                      = ui_gdi_vprintln,
    .print                         = ui_gdi_print,
    .println                       = ui_gdi_println,
    .multiline                     = ui_gdi_multiline
};

#pragma pop_macro("gdi_hdc_with_font")
#pragma pop_macro("gdi_with_hdc")
#pragma pop_macro("app_canvas")
#pragma pop_macro("app_window")
// ________________________________ ui_label.c ________________________________

#include "ut/ut.h"

static void ui_label_paint(ui_view_t* v) {
    assert(v->type == ui_view_label);
    assert(!v->hidden);
    // at later stages of layout text height can grow:
    ui_gdi.push(v->x, v->y + v->label_dy);
    ui_font_t f = *v->font;
    ui_gdi.set_font(f);
//  traceln("%s h=%d dy=%d baseline=%d", v->text, v->h,
//          v->label_dy, v->baseline);
    ui_color_t c = v->hover && v->highlightable ?
        ui_colors.text_highlight : v->color;
    ui_gdi.set_text_color(c);
    // paint for text also does lightweight re-layout
    // which is useful for simplifying dynamic text changes
    bool multiline = strchr(v->text, '\n') != null;
    if (!multiline) {
        ui_gdi.text("%s", ui_view.nls(v));
    } else {
        int32_t w = (int32_t)(v->min_w_em * v->em.x + 0.5);
        ui_gdi.multiline(w == 0 ? -1 : w, "%s", ui_view.nls(v));
    }
    if (v->hover && !v->flat && v->highlightable) {
        // ui_colors.btn_hover_highlight
        ui_gdi.set_colored_pen(ui_app.get_color(ui_color_id_highlight));
        ui_gdi.set_brush(ui_gdi.brush_hollow);
        int32_t cr = v->em.y / 4; // corner radius
        int32_t h = multiline ? v->h : v->baseline + v->descent;
        ui_gdi.rounded(v->x - cr, v->y + v->label_dy,
                       v->w + 2 * cr, h, cr, cr);
    }
    ui_gdi.pop();
}

static void ui_label_context_menu(ui_view_t* v) {
    assert(v->type == ui_view_label);
    if (!ui_view.is_hidden(v) && !ui_view.is_disabled(v)) {
        ut_clipboard.put_text(ui_view.nls(v));
        static bool first_time = true;
        ui_app.toast(first_time ? 2.2 : 0.5,
            ut_nls.str("Text copied to clipboard"));
        first_time = false;
    }
}

static void ui_label_character(ui_view_t* v, const char* utf8) {
    assert(v->type == ui_view_label);
    if (v->hover && !ui_view.is_hidden(v)) {
        char ch = utf8[0];
        // Copy to clipboard works for hover over text
        if ((ch == 3 || ch == 'c' || ch == 'C') && ui_app.ctrl) {
            ut_clipboard.put_text(ui_view.nls(v)); // 3 is ASCII for Ctrl+C
        }
    }
}

void ui_view_init_label(ui_view_t* v) {
    assert(v->type == ui_view_label);
    ui_view_init(v);
    v->color_id     = ui_color_id_window_text;
    v->paint        = ui_label_paint;
    v->character    = ui_label_character;
    v->context_menu = ui_label_context_menu;
}

void ui_label_init_va(ui_label_t* v, fp32_t min_w_em,
        const char* format, va_list vl) {
    ut_str.format_va(v->text, countof(v->text), format, vl);
    v->min_w_em = min_w_em;
    v->type = ui_view_label;
    ui_view_init_label(v);
}

void ui_label_init(ui_label_t* v, fp32_t min_w_em, const char* format, ...) {
    va_list vl;
    va_start(vl, format);
    ui_label_init_va(v, min_w_em, format, vl);
    va_end(vl);
}
// _______________________________ ui_layout.c ________________________________

#include "ut/ut.h"

static void measurements_center(ui_view_t* view) {
    assert(view->child != null && view->child->next == view->child,
        "must be a single child parent");
    ui_view_t* c = view->child; // even if hidden measure it
    c->w = view->w;
    c->h = view->h;
}

static void measurements_horizontal(ui_view_t* view, int32_t gap) {
    assert(view->child != null, "not a single child?");
    view->w = 0;
    view->h = 0;
    bool seen = false;
    ui_view_for_each(view, c, {
        if (!c->hidden) {
            if (seen) { view->w += gap; }
            view->w += c->w;
            view->h = ut_max(view->h, c->h);
            seen = true;
        }
    });
}

static void measurements_vertical(ui_view_t* view, int32_t gap) {
    assert(view->child != null, "not a single child?");
    view->h = 0;
    bool seen = false;
    ui_view_for_each(view, c, {
        if (!c->hidden) {
            if (seen) { view->h += gap; }
            view->h += c->h;
            view->w = ut_max(view->w, c->w);
            seen = true;
        }
    });
}

static void measurements_grid(ui_view_t* view, int32_t gap_h, int32_t gap_v) {
    int32_t cols = 0;
    ui_view_for_each(view, r, {
        int32_t n = 0;
        ui_view_for_each(r, c, { n++; });
        if (cols == 0) { cols = n; }
        assert(n > 0 && cols == n);
    });
    #pragma warning(push) // mxw[] IntelliSense confusion
    #pragma warning(disable: 6385)
    #pragma warning(disable: 6386)
    int32_t* mxw = (int32_t*)ut_stackalloc(cols * sizeof(int32_t));
    memset(mxw, 0, cols * sizeof(int32_t));
    ui_view_for_each(view, row, {
        if (!row->hidden) {
            row->h = 0;
            row->baseline = 0;
            int32_t i = 0;
            ui_view_for_each(row, col, {
                if (!col->hidden) {
                    mxw[i] = ut_max(mxw[i], col->w);
                    row->h = ut_max(row->h, col->h);
//                  traceln("[%d] row.baseline: %d col.baseline: %d ", i, row->baseline, col->baseline);
                    row->baseline = ut_max(row->baseline, col->baseline);
                }
                i++;
            });
        }
    });
    view->h = 0;
    view->w = 0;
    int32_t rows_seen = 0; // number of visible rows so far
    ui_view_for_each(view, r, {
        if (!r->hidden) {
            r->w = 0;
            int32_t i = 0;
            int32_t cols_seen = 0; // number of visible columns so far
            ui_view_for_each(view, c, {
                if (!c->hidden) {
                    c->h = r->h; // all cells are same height
                    // TODO: label_dy needs to be transfered to containers
                    //       ratinale: labels and buttons baselines must align
                    if (c->type == ui_view_label) { // lineup text baselines
                        c->label_dy = r->baseline - c->baseline;
                    }
                    c->w = mxw[i++];
                    r->w += c->w;
                    if (cols_seen > 0) { r->w += gap_h; }
                    view->w = ut_max(view->w, r->w);
                    cols_seen++;
                }
            });
            view->h += r->h;
            if (rows_seen > 0) { view->h += gap_v; }
            rows_seen++;
        }
    });
    #pragma warning(pop)
}

measurements_if measurements = {
    .center     = measurements_center,
    .horizontal = measurements_horizontal,
    .vertical   = measurements_vertical,
    .grid       = measurements_grid,
};

// layouts

static void layouts_center(ui_view_t* view) {
    assert(view->child != null && view->child->next == view->child,
        "must be a single child parent");
    ui_view_t* c = view->child;
    c->x = (view->w - c->w) / 2;
    c->y = (view->h - c->h) / 2;
}

static void layouts_horizontal(ui_view_t* view, int32_t x, int32_t y,
        int32_t gap) {
    assert(view->child != null, "not a single child?");
    bool seen = false;
    ui_view_for_each(view, c, {
        if (!c->hidden) {
            if (seen) { x += gap; }
            c->x = x;
            c->y = y;
            x += c->w;
            seen = true;
        }
    });
}

static void layouts_vertical(ui_view_t* view, int32_t x, int32_t y,
        int32_t gap) {
    assert(view->child != null, "not a single child?");
    bool seen = false;
    ui_view_for_each(view, c, {
        if (!c->hidden) {
            if (seen) { y += gap; }
            c->x = x;
            c->y = y;
            y += c->h;
            seen = true;
        }
    });
}

static void layouts_grid(ui_view_t* view, int32_t gap_h, int32_t gap_v) {
    assert(view->child != null, "not a single child?");
    int32_t x = view->x;
    int32_t y = view->y;
    bool row_seen = false;
    ui_view_for_each(view, row, {
        if (!row->hidden) {
            if (row_seen) { y += gap_v; }
            int32_t xc = x;
            bool col_seen = false;
            ui_view_for_each(row, col, {
                if (!col->hidden) {
                    if (col_seen) { xc += gap_h; }
                    col->x = xc;
                    col->y = y;
                    xc += col->w;
                    col_seen = true;
                }
            });
            y += row->h;
            row_seen = true;
        }
    });
}

layouts_if layouts = {
    .center     = layouts_center,
    .horizontal = layouts_horizontal,
    .vertical   = layouts_vertical,
    .grid       = layouts_grid
};
// _________________________________ ui_mbx.c _________________________________

#include "ut/ut.h"

static void ui_mbx_button(ui_button_t* b) {
    ui_mbx_t* mx = (ui_mbx_t*)b->parent;
    assert(mx->view.type == ui_view_mbx);
    mx->option = -1;
    for (int32_t i = 0; i < countof(mx->button) && mx->option < 0; i++) {
        if (b == &mx->button[i]) {
            mx->option = i;
            if (mx->view.callback != null) { mx->view.callback(&mx->view); }
        }
    }
    ui_app.show_toast(null, 0);
}

static void ui_mbx_measure(ui_view_t* view) {
    ui_mbx_t* mx = (ui_mbx_t*)view;
    assert(view->type == ui_view_mbx);
    int32_t n = 0;
    ui_view_for_each(view, c, { n++; });
    n--; // number of buttons
//  TODO: not needed, remove me
//  if (mx->label.measure != null) {
//      mx->label.measure(&mx->label);
//  } else {
//      ui_view.measure(&mx->label);
//  }
    const int32_t em_x = mx->label.em.x;
    const int32_t em_y = mx->label.em.y;
    const int32_t tw = mx->label.w;
    const int32_t th = mx->label.h;
    if (n > 0) {
        int32_t bw = 0;
        for (int32_t i = 0; i < n; i++) {
            bw += mx->button[i].w;
        }
        view->w = ut_max(tw, bw + em_x * 2);
        view->h = th + mx->button[0].h + em_y + em_y / 2;
    } else {
        view->h = th + em_y / 2;
        view->w = tw;
    }
}

static void ui_mbx_layout(ui_view_t* view) {
    ui_mbx_t* mx = (ui_mbx_t*)view;
    assert(view->type == ui_view_mbx);
    int32_t n = 0;
    ui_view_for_each(view, c, { n++; });
    n--; // number of buttons
    const int32_t em_y = mx->label.em.y;
    mx->label.x = view->x;
    mx->label.y = view->y + em_y * 2 / 3;
    const int32_t tw = mx->label.w;
    const int32_t th = mx->label.h;
    if (n > 0) {
        int32_t bw = 0;
        for (int32_t i = 0; i < n; i++) {
            bw += mx->button[i].w;
        }
        // center text:
        mx->label.x = view->x + (view->w - tw) / 2;
        // spacing between buttons:
        int32_t sp = (view->w - bw) / (n + 1);
        int32_t x = sp;
        for (int32_t i = 0; i < n; i++) {
            mx->button[i].x = view->x + x;
            mx->button[i].y = view->y + th + em_y * 3 / 2;
            x += mx->button[i].w + sp;
        }
    }
}

void ui_view_init_mbx(ui_view_t* view) {
    assert(view->type == ui_view_mbx);
    ui_mbx_t* mx = (ui_mbx_t*)view;
    ui_view_init(view);
    view->measure = ui_mbx_measure;
    view->layout  = ui_mbx_layout;
    mx->view.font = &ui_app.fonts.H3;
    const char** options = mx->options;
    int32_t n = 0;
    while (options[n] != null && n < countof(mx->button) - 1) {
        ui_button_init(&mx->button[n], options[n], 6.0, ui_mbx_button);
        n++;
    }
    swear(n <= countof(mx->button), "inhumane: %d buttons", n);
    if (n > countof(mx->button)) { n = countof(mx->button); }
    ui_label_init(&mx->label, 0.0, "%s", mx->view.text);
    ui_view.add_last(&mx->view, &mx->label);
    for (int32_t i = 0; i < n; i++) {
        ui_view.add_last(&mx->view, &mx->button[i]);
        mx->button[i].font = mx->view.font;
        ui_view.localize(&mx->button[i]);
        // TODO: remove assert below
        assert(mx->button[i].parent == &mx->view);
    }
    mx->label.font = mx->view.font;
    ui_view.localize(&mx->label);
    mx->view.text[0] = 0;
    mx->option = -1;
}

void ui_mbx_init(ui_mbx_t* mx, const char* options[],
        const char* format, ...) {
    mx->view.type = ui_view_mbx;
    mx->view.measure  = ui_mbx_measure;
    mx->view.layout   = ui_mbx_layout;
    mx->view.color_id = ui_color_id_window;
    mx->options = options;
    va_list vl;
    va_start(vl, format);
    ut_str.format_va(mx->view.text, countof(mx->view.text), format, vl);
    ui_label_init(&mx->label, 0.0, mx->view.text);
    va_end(vl);
    ui_view_init_mbx(&mx->view);
}
// _______________________________ ui_slider.c ________________________________

#include "ut/ut.h"

static void ui_slider_measure(ui_view_t* v) {
    assert(v->type == ui_view_slider);
    ui_view.measure(v);
    ui_slider_t* r = (ui_slider_t*)v;
    assert(r->inc.w == r->dec.w && r->inc.h == r->dec.h);
    const int32_t em = v->em.x;
    ui_font_t f = v->font != null ? *v->font : ui_app.fonts.regular;
    const int32_t w = (int32_t)(v->min_w_em * v->em.x);
    r->tm = ui_gdi.measure_text(f, ui_view.nls(v), r->value_max);
    if (w > r->tm.x) { r->tm.x = w; }
    v->w = r->dec.w + r->tm.x + r->inc.w + em * 2;
    v->h = r->inc.h;
}

static void ui_slider_layout(ui_view_t* v) {
    assert(v->type == ui_view_slider);
    ui_slider_t* r = (ui_slider_t*)v;
    assert(r->inc.w == r->dec.w && r->inc.h == r->dec.h);
    const int32_t em = v->em.x;
    r->dec.x = v->x;
    r->dec.y = v->y;
    r->inc.x = v->x + r->dec.w + r->tm.x + em * 2;
    r->inc.y = v->y;
}

// TODO: generalize and move to ui_colors.c to avoid slider dup

static ui_color_t ui_slider_gradient_darker(void) {
    if (ui_theme.are_apps_dark()) {
        return ui_colors.btn_gradient_darker;
    } else {
        ui_color_t c = ui_app.get_color(ui_color_id_button_face);
        uint32_t r = ui_color_r(c);
        uint32_t g = ui_color_r(c);
        uint32_t b = ui_color_r(c);
        r = ut_max(0, ut_min(0xFF, (uint32_t)(r * 0.75)));
        g = ut_max(0, ut_min(0xFF, (uint32_t)(g * 0.75)));
        b = ut_max(0, ut_min(0xFF, (uint32_t)(b * 0.75)));
        ui_color_t d = ui_rgb(r, g, b);
//      traceln("c: 0%06X -> 0%06X", c, d);
        return d;
    }
}

static ui_color_t ui_slider_gradient_dark(void) {
    if (ui_theme.are_apps_dark()) {
        return ui_colors.btn_gradient_dark;
    } else {
        ui_color_t c = ui_app.get_color(ui_color_id_button_face);
        uint32_t r = ui_color_r(c);
        uint32_t g = ui_color_r(c);
        uint32_t b = ui_color_r(c);
        r = ut_max(0, ut_min(0xFF, (uint32_t)(r * 1.25)));
        g = ut_max(0, ut_min(0xFF, (uint32_t)(g * 1.25)));
        b = ut_max(0, ut_min(0xFF, (uint32_t)(b * 1.25)));
        ui_color_t d = ui_rgb(r, g, b);
//      traceln("c: 0%06X -> 0%06X", c, d);
        return d;
    }
}

static void ui_slider_paint(ui_view_t* v) {
    assert(v->type == ui_view_slider);
    ui_slider_t* r = (ui_slider_t*)v;
    ui_gdi.push(v->x, v->y);
    ui_gdi.set_clip(v->x, v->y, v->w, v->h);
    const int32_t em = v->em.x;
    const int32_t em2  = ut_max(1, em / 2);
    const int32_t em4  = ut_max(1, em / 8);
    const int32_t em8  = ut_max(1, em / 8);
    const int32_t em16 = ut_max(1, em / 16);
    ui_color_t c0 = ui_theme.are_apps_dark() ?
                    ui_colors.dkgray3 :
                    ui_app.get_color(ui_color_id_button_face);
    ui_gdi.set_brush(ui_gdi.brush_color);
    ui_pen_t pen_c0 = ui_gdi.create_pen(c0, em16);
    ui_gdi.set_pen(pen_c0);
    ui_gdi.set_brush_color(c0);
    const int32_t x = v->x + r->dec.w + em2;
    const int32_t y = v->y;
    const int32_t w = r->tm.x + em;
    const int32_t h = v->h;
    ui_gdi.rounded(x - em8, y, w + em4, h, em4, em4);
    if (ui_theme.are_apps_dark()) {
        ui_gdi.gradient(x, y, w, h / 2, c0, ui_slider_gradient_darker(), true);
        ui_gdi.gradient(x, y + h / 2, w, v->h - h / 2, ui_slider_gradient_dark(), c0, true);
        ui_gdi.set_brush_color(ui_colors.dkgreen);
    } else {
        ui_gdi.gradient(x, y, w, h / 2, ui_slider_gradient_dark(), c0, true);
        ui_gdi.gradient(x, y + h / 2, w, v->h - h / 2, c0, ui_slider_gradient_darker(), true);
        ui_gdi.set_brush_color(ui_colors.jungle_green);
    }
    ui_color_t c1 = ui_theme.are_apps_dark() ?
                    ui_colors.dkgray1 :
                    ui_app.get_color(ui_color_id_button_face); // ???
    ui_pen_t pen_c1 = ui_gdi.create_pen(c1, em16);
    ui_gdi.set_pen(pen_c1);
    const fp64_t range = (fp64_t)r->value_max - (fp64_t)r->value_min;
    fp64_t vw = (fp64_t)(r->tm.x + em) * (r->value - r->value_min) / range;
    ui_gdi.rect(x, v->y, (int32_t)(vw + 0.5), v->h);
    ui_gdi.x += r->dec.w + em;
    v->color = ui_app.get_color(ui_color_id_window_text);
    const char* format = ut_nls.str(v->text);
    ui_color_t c = ui_gdi.set_text_color(v->color);
    ui_gdi.text(format, r->value);
    ui_gdi.set_text_color(c);
    ui_gdi.set_clip(0, 0, 0, 0);
    ui_gdi.delete_pen(pen_c1);
    ui_gdi.delete_pen(pen_c0);
    ui_gdi.pop();
}

static void ui_slider_mouse(ui_view_t* v, int32_t message, int64_t f) {
    if (!v->hidden && !v->disabled) {
        assert(v->type == ui_view_slider);
        ui_slider_t* r = (ui_slider_t*)v;
        bool drag = message == ui.message.mouse_move &&
            (f & (ui.mouse.button.left|ui.mouse.button.right)) != 0;
        if (message == ui.message.left_button_pressed ||
            message == ui.message.right_button_pressed || drag) {
            const int32_t x = ui_app.mouse.x - v->x - r->dec.w;
            const int32_t y = ui_app.mouse.y - v->y;
            const int32_t x0 = v->em.x / 2;
            const int32_t x1 = r->tm.x + v->em.x;
            if (x0 <= x && x < x1 && 0 <= y && y < v->h) {
                ui_app.focus = v;
                const fp64_t range = (fp64_t)r->value_max - (fp64_t)r->value_min;
                fp64_t val = ((fp64_t)x - x0) * range / (fp64_t)(x1 - x0 - 1);
                int32_t vw = (int32_t)(val + r->value_min + 0.5);
                r->value = ut_min(ut_max(vw, r->value_min), r->value_max);
                if (r->view.callback != null) { r->view.callback(&r->view); }
                ui_view.invalidate(v);
            }
        }
    }
}

static void ui_slider_inc_dec_value(ui_slider_t* r, int32_t sign, int32_t mul) {
    if (!r->view.hidden && !r->view.disabled) {
        // full 0x80000000..0x7FFFFFFF (-2147483648..2147483647) range
        int32_t v = r->value;
        if (v > r->value_min && sign < 0) {
            mul = ut_min(v - r->value_min, mul);
            v += mul * sign;
        } else if (v < r->value_max && sign > 0) {
            mul = ut_min(r->value_max - v, mul);
            v += mul * sign;
        }
        if (r->value != v) {
            r->value = v;
            if (r->view.callback != null) { r->view.callback(&r->view); }
            ui_view.invalidate(&r->view);
        }
    }
}

static void ui_slider_inc_dec(ui_button_t* b) {
    ui_slider_t* r = (ui_slider_t*)b->parent;
    if (!r->view.hidden && !r->view.disabled) {
        int32_t sign = b == &r->inc ? +1 : -1;
        int32_t mul = ui_app.shift && ui_app.ctrl ? 1000 :
            ui_app.shift ? 100 : ui_app.ctrl ? 10 : 1;
        ui_slider_inc_dec_value(r, sign, mul);
    }
}

static void ui_slider_every_100ms(ui_view_t* v) { // 100ms
    assert(v->type == ui_view_slider);
    ui_slider_t* r = (ui_slider_t*)v;
    if (r->view.hidden || r->view.disabled) {
        r->time = 0;
    } else if (!r->dec.armed && !r->inc.armed) {
        r->time = 0;
    } else {
        if (r->time == 0) {
            r->time = ui_app.now;
        } else if (ui_app.now - r->time > 1.0) {
            const int32_t sign = r->dec.armed ? -1 : +1;
            int32_t s = (int32_t)(ui_app.now - r->time + 0.5);
            int32_t mul = s >= 1 ? 1 << (s - 1) : 1;
            const int64_t range = (int64_t)r->value_max - r->value_min;
            if (mul > range / 8) { mul = (int32_t)(range / 8); }
            ui_slider_inc_dec_value(r, sign, ut_max(mul, 1));
        }
    }
}

void ui_view_init_slider(ui_view_t* v) {
    assert(v->type == ui_view_slider);
    ui_view_init(v);
    ui_view.set_text(v, v->text);
    v->mouse       = ui_slider_mouse;
    v->measure     = ui_slider_measure;
    v->layout      = ui_slider_layout;
    v->paint       = ui_slider_paint;
    v->every_100ms = ui_slider_every_100ms;
    v->color = ui_app.get_color(ui_color_id_window_text);
    ui_slider_t* s = (ui_slider_t*)v;
    // Heavy Minus Sign
    ui_button_init(&s->dec, "\xE2\x9E\x96", 0, ui_slider_inc_dec);
    // Heavy Plus Sign
    ui_button_init(&s->inc, "\xE2\x9E\x95", 0, ui_slider_inc_dec);
    static const char* accel =
        " Hold key while clicking\n Ctrl: x 10 Shift: x 100 \n Ctrl+Shift: x 1000 \n for step multiplier.";
    strprintf(s->inc.hint, "%s", accel);
    strprintf(s->dec.hint, "%s", accel);
    ui_view.add(&s->view, &s->dec, &s->inc, null);
    ui_view.localize(&s->view);
}

void ui_slider_init(ui_slider_t* s, const char* label, fp32_t min_w_em,
        int32_t value_min, int32_t value_max,
        void (*callback)(ui_view_t* r)) {
    static_assert(offsetof(ui_slider_t, view) == 0, "offsetof(.view)");
    assert(min_w_em >= 3.0, "allow 1em for each of [-] and [+] buttons");
    s->view.type = ui_view_slider;
    strprintf(s->view.text, "%s", label);
    s->view.callback = callback;
    s->view.min_w_em = min_w_em;
    s->value_min = value_min;
    s->value_max = value_max;
    s->value = value_min;
    ui_view_init_slider(&s->view);
}
// ________________________________ ui_theme.c ________________________________

/* Copyright (c) Dmitry "Leo" Kuznetsov 2021-24 see LICENSE for details */
#include "ut/ut.h"
// ________________________________ ut_win32.h ________________________________

#ifdef WIN32

#pragma warning(push)
#pragma warning(disable: 4255) // no function prototype: '()' to '(void)'
#pragma warning(disable: 4459) // declaration of '...' hides global declaration

// ut:
#include <Windows.h>  // used by:
#include <psapi.h>    // both ut_loader.c and ut_processes.c
#include <shellapi.h> // ut_processes.c
#include <winternl.h> // ut_processes.c
#include <initguid.h>     // for knownfolders
#include <knownfolders.h> // ut_files.c
#include <aclapi.h>       // ut_files.c
#include <shlobj_core.h>  // ut_files.c
#include <shlwapi.h>      // ut_files.c
// ui:
#include <windowsx.h>
#include <commdlg.h>
#include <dwmapi.h>
#include <ShellScalingApi.h>
#include <VersionHelpers.h>

#pragma warning(pop)

#include <fcntl.h>

#define export __declspec(dllexport)

#define b2e(call) (call ? 0 : GetLastError()) // BOOL -> errno_t

#define wait2e(ix) (errno_t)                                                     \
    ((int32_t)WAIT_OBJECT_0 <= (int32_t)(ix) && (ix) <= WAIT_OBJECT_0 + 63 ? 0 : \
      ((ix) == WAIT_ABANDONED ? ERROR_REQUEST_ABORTED :                          \
        ((ix) == WAIT_TIMEOUT ? ERROR_TIMEOUT :                                  \
          ((ix) == WAIT_FAILED) ? (errno_t)GetLastError() : ERROR_INVALID_HANDLE \
        )                                                                        \
      )                                                                          \
    )


#endif // WIN32

static int32_t ui_theme_dark = -1; // -1 unknown

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

static struct {
    const char* name;
    ui_color_t  dark;
    ui_color_t  light;
} ui_theme_colors[] = { // empirical
    { .name = "Undefiled"        ,.dark = ui_color_undefined, .light = ui_color_undefined },
    { .name = "ActiveTitle"      ,.dark = 0x00000000, .light = 0x00D1B499 },
    { .name = "ButtonFace"       ,.dark = 0x00333333, .light = 0x00F0F0F0 },
    { .name = "ButtonText"       ,.dark = 0x00F6F3EE, .light = 0x00000000 },
    { .name = "GrayText"         ,.dark = 0x00666666, .light = 0x006D6D6D },
    { .name = "Hilight"          ,.dark = 0x00626262, .light = 0x00D77800 },
    { .name = "HilightText"      ,.dark = 0x00000000, .light = 0x00FFFFFF },
    { .name = "HotTrackingColor" ,.dark = 0x00B77878, .light = 0x00CC6600 },
    { .name = "InactiveTitle"    ,.dark = 0x002B2B2B, .light = 0x00DBCDBF },
    { .name = "InactiveTitleText",.dark = 0x00969696, .light = 0x00000000 },
    { .name = "MenuHilight"      ,.dark = 0x00002642, .light = 0x00FF9933 },
    { .name = "TitleText"        ,.dark = 0x00FFFFFF, .light = 0x00000000 },
    { .name = "Window"           ,.dark = 0x00000000, .light = 0x00FFFFFF },
    { .name = "WindowText"       ,.dark = 0x00FFFFFF, .light = 0x00000000 },
};


#pragma push_macro("ux_theme_reg_cv")
#pragma push_macro("ux_theme_reg_default_colors")

#define ux_theme_reg_cv "SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\"
#define ux_theme_reg_default_colors ux_theme_reg_cv "Themes\\DefaultColors\\"

static bool ui_theme_use_light_theme(const char* key) {
    if (!ui_app.dark_mode && !ui_app.light_mode ||
         ui_app.dark_mode && ui_app.light_mode) {
        const char* personalize  = ux_theme_reg_cv "Themes\\Personalize";
        DWORD light_theme = 0;
        ui_theme_reg_get_uint32(HKEY_CURRENT_USER, personalize, key, &light_theme);
        return light_theme != 0;
    } else if (ui_app.light_mode) {
        return true;
    } else {
        assert(ui_app.dark_mode);
        return false;
    }
}

#pragma pop_macro("ux_theme_reg_cv")
#pragma pop_macro("ux_theme_reg_default_colors")

static bool ui_theme_are_apps_dark(void) {
    return !ui_theme_use_light_theme("AppsUseLightTheme");
}

static bool ui_theme_is_system_dark(void) {
    return !ui_theme_use_light_theme("SystemUsesLightTheme");
}

static void ui_theme_refresh(void* window) {
    ui_theme_dark = -1;
    BOOL dark_mode = ui_theme.are_apps_dark();
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

static ui_color_t ui_theme_get_color(int32_t color_id) {
    swear(0 < color_id && color_id < countof(ui_theme_colors));
    if (ui_theme_dark < 0) {
        ui_theme_dark = ui_theme.are_apps_dark();
    }
    return ui_theme_dark ? ui_theme_colors[color_id].dark :
                           ui_theme_colors[color_id].light;
}

ui_theme_if ui_theme = {
    .get_color                    = ui_theme_get_color,
    .is_system_dark               = ui_theme_is_system_dark,
    .are_apps_dark                = ui_theme_are_apps_dark,
    .refresh                      = ui_theme_refresh,
};


// _______________________________ ui_toggle.c ________________________________

#include "ut/ut.h"

static int ui_toggle_paint_on_off(ui_view_t* view) {
    ui_gdi.push(view->x, view->y);
    ui_color_t background = view->pressed ? ui_colors.tone_green : ui_colors.dkgray4;
    ui_color_t foreground = view->color;
    ui_gdi.set_text_color(background);
    int32_t x = view->x;
    int32_t x1 = view->x + view->em.x * 3 / 4;
    while (x < x1) {
        ui_gdi.x = x;
        ui_gdi.text("%s", ui_glyph_black_large_circle);
        x++;
    }
    int32_t rx = ui_gdi.x;
    ui_gdi.set_text_color(foreground);
    ui_gdi.x = view->pressed ? x : view->x;
    ui_gdi.text("%s", ui_glyph_black_large_circle);
    ui_gdi.pop();
    return rx;
}

static const char* ui_toggle_on_off_label(ui_view_t* view, char* label, int32_t count)  {
    ut_str.format(label, count, "%s", ui_view.nls(view));
    char* s = strstr(label, "___");
    if (s != null) {
        memcpy(s, view->pressed ? "On " : "Off", 3);
    }
    return ut_nls.str(label);
}

static void ui_toggle_measure(ui_view_t* view) {
    assert(view->type == ui_view_toggle);
    ui_view.measure(view);
    view->w += view->em.x * 2;
}

static void ui_toggle_paint(ui_view_t* view) {
    assert(view->type == ui_view_toggle);
    char text[countof(view->text)];
    const char* label = ui_toggle_on_off_label(view, text, countof(text));
    ui_gdi.push(view->x, view->y);
    ui_font_t f = *view->font;
    ui_font_t font = ui_gdi.set_font(f);
    ui_gdi.x = ui_toggle_paint_on_off(view) + view->em.x * 3 / 4;
    ui_gdi.text("%s", label);
    ui_gdi.set_font(font);
    ui_gdi.pop();
}

static void ui_toggle_flip(ui_toggle_t* c) {
    assert(c->type == ui_view_toggle);
    ui_app.redraw();
    c->pressed = !c->pressed;
    if (c->callback != null) { c->callback(c); }
}

static void ui_toggle_character(ui_view_t* view, const char* utf8) {
    assert(view->type == ui_view_toggle);
    assert(!view->hidden && !view->disabled);
    char ch = utf8[0];
    if (ui_view.is_shortcut_key(view, ch)) {
         ui_toggle_flip((ui_toggle_t*)view);
    }
}

static void ui_toggle_key_pressed(ui_view_t* view, int64_t key) {
    if (ui_app.alt && ui_view.is_shortcut_key(view, key)) {
//      traceln("key: 0x%02X shortcut: %d", key, ui_view.is_shortcut_key(view, key));
        ui_toggle_flip((ui_toggle_t*)view);
    }
}

static void ui_toggle_mouse(ui_view_t* view, int32_t message, int64_t flags) {
    assert(view->type == ui_view_toggle);
    (void)flags; // unused
    assert(!view->hidden && !view->disabled);
    if (message == ui.message.left_button_pressed ||
        message == ui.message.right_button_pressed) {
        int32_t x = ui_app.mouse.x - view->x;
        int32_t y = ui_app.mouse.y - view->y;
        if (0 <= x && x < view->w && 0 <= y && y < view->h) {
            ui_app.focus = view;
            ui_toggle_flip((ui_toggle_t*)view);
        }
    }
}

void ui_view_init_toggle(ui_view_t* view) {
    assert(view->type == ui_view_toggle);
    ui_view_init(view);
    ui_view.set_text(view, view->text);
    view->mouse       = ui_toggle_mouse;
    view->measure     = ui_toggle_measure;
    view->paint       = ui_toggle_paint;
    view->character   = ui_toggle_character;
    view->key_pressed = ui_toggle_key_pressed;
    ui_view.localize(view);
    view->color = ui_app.get_color(ui_color_id_button_text);
}

void ui_toggle_init(ui_toggle_t* c, const char* label, fp32_t ems,
       void (*callback)(ui_toggle_t* b)) {
    ui_view_init(c);
    strprintf(c->text, "%s", label);
    c->min_w_em = ems;
    c->callback = callback;
    c->type = ui_view_toggle;
    ui_view_init_toggle(c);
}
// ________________________________ ui_view.c _________________________________

#include "ut/ut.h"

static const fp64_t ui_view_hover_delay = 1.5; // seconds

#pragma push_macro("ui_view_for_each")

// adding and removing views is not expected to be frequent
// actions by application code (human factor - UI design)
// thus extra checks and verifications are there even in
// release code because C is not type safety champion language.

static inline void ui_view_check_type(ui_view_t* v) {
    // little endian:
    static_assertion(('vwXX' & 0xFFFF0000U) ==
                     ('vwZZ' & 0xFFFF0000U));
    static_assertion((ui_view_container & 0xFFFF0000U) ==
                                ('vw??' & 0xFFFF0000U));
    swear((v->type & 0xFFFF0000) ==
          ('vw??'  & 0xFFFF0000),
          "not a view: %4.4s 0x%08X (forgotten &static_view?)",
          &v->type, v->type);
}

static void ui_view_verify(ui_view_t* p) {
    ui_view_check_type(p);
    ui_view_for_each(p, c, {
        ui_view_check_type(c);
        swear(c->parent == p);
        swear(c == c->next->prev);
        swear(c == c->prev->next);
    });
}

static ui_view_t* ui_view_add(ui_view_t* p, ...) {
    va_list vl;
    va_start(vl, p);
    ui_view_t* c = va_arg(vl, ui_view_t*);
    while (c != null) {
        swear(c->parent == null && c->prev == null && c->next == null);
        ui_view.add_last(p, c);
        c = va_arg(vl, ui_view_t*);
    }
    va_end(vl);
    ui_view_call_init(p);
    ui_app.layout();
    return p;
}

static void ui_view_add_first(ui_view_t* p, ui_view_t* c) {
    swear(c->parent == null && c->prev == null && c->next == null);
    c->parent = p;
    if (p->child == null) {
        c->prev = c;
        c->next = c;
    } else {
        c->prev = p->child->prev;
        c->next = p->child;
        c->prev->next = c;
        c->next->prev = c;
    }
    p->child = c;
    ui_view_call_init(c);
    ui_app.layout();
}

static void ui_view_add_last(ui_view_t* p, ui_view_t* c) {
    swear(c->parent == null && c->prev == null && c->next == null);
    c->parent = p;
    if (p->child == null) {
        c->prev = c;
        c->next = c;
        p->child = c;
    } else {
        c->prev = p->child->prev;
        c->next = p->child;
        c->prev->next = c;
        c->next->prev = c;
    }
    ui_view_call_init(c);
    ui_view_verify(p);
    ui_app.layout();
}

static void ui_view_add_after(ui_view_t* c, ui_view_t* a) {
    swear(c->parent == null && c->prev == null && c->next == null);
    not_null(a->parent);
    c->parent = a->parent;
    c->next = a->next;
    c->prev = a;
    a->next = c;
    c->prev->next = c;
    c->next->prev = c;
    ui_view_call_init(c);
    ui_view_verify(c->parent);
    ui_app.layout();
}

static void ui_view_add_before(ui_view_t* c, ui_view_t* b) {
    swear(c->parent == null && c->prev == null && c->next == null);
    not_null(b->parent);
    c->parent = b->parent;
    c->prev = b->prev;
    c->next = b;
    b->prev = c;
    c->prev->next = c;
    c->next->prev = c;
    ui_view_call_init(c);
    ui_view_verify(c->parent);
    ui_app.layout();
}

static void ui_view_remove(ui_view_t* c) {
    not_null(c->parent);
    not_null(c->parent->child);
    if (c->prev == c) {
        swear(c->next == c);
        c->parent->child = null;
    } else {
        c->prev->next = c->next;
        c->next->prev = c->prev;
        if (c->parent->child == c) {
            c->parent->child = c->next;
        }
    }
    c->prev = null;
    c->next = null;
    ui_view_verify(c->parent);
    c->parent = null;
    ui_app.layout();
}

static void ui_view_remove_all(ui_view_t* p) {
    while (p->child != null) { ui_view.remove(p->child); }
    ui_app.layout();
}

static void ui_view_disband(ui_view_t* p) {
    while (p->child != null) {
        ui_view_disband(p->child);
        ui_view.remove(p->child);
    }
    ui_app.layout();
}

static void ui_view_invalidate(const ui_view_t* view) {
    ui_rect_t rc = { view->x, view->y, view->w, view->h};
    rc.x -= view->em.x;
    rc.y -= view->em.y;
    rc.w += view->em.x * 2;
    rc.h += view->em.y * 2;
    ui_app.invalidate(&rc);
}

static const char* ui_view_nls(ui_view_t* view) {
    return view->strid != 0 ?
        ut_nls.string(view->strid, view->text) : view->text;
}

static void ui_view_measure(ui_view_t* view) {
    ui_font_t f = *view->font;
    view->baseline = ui_gdi.baseline(f);
    view->descent  = ui_gdi.descent(f);
    if (view->text[0] != 0) {
        view->w = (int32_t)(view->em.x * view->min_w_em + 0.5f);
        ui_point_t mt = { 0 };
        bool multiline = strchr(view->text, '\n') != null;
        if (view->type == ui_view_label && multiline) {
            int32_t w = (int32_t)(view->min_w_em * view->em.x + 0.5f);
            mt = ui_gdi.measure_multiline(f, w == 0 ? -1 : w, ui_view.nls(view));
        } else {
            mt = ui_gdi.measure_text(f, ui_view.nls(view));
        }
        view->h = mt.y;
        view->w = ut_max(view->w, mt.x);
    }
}

static bool ui_view_inside(ui_view_t* view, const ui_point_t* pt) {
    const int32_t x = pt->x - view->x;
    const int32_t y = pt->y - view->y;
    return 0 <= x && x < view->w && 0 <= y && y < view->h;
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
        view->strid = ut_nls.strid(view->text);
    }
}

static void ui_view_show_hint(ui_view_t* v, ui_view_t* hint) {
    ui_view_call_init(hint);
    strprintf(hint->text, "%s", ut_nls.str(v->hint));
    ui_view.measure_children(hint);
    int32_t x = v->x + v->w / 2 - hint->w / 2 + hint->em.x / 4;
    int32_t y = v->y + v->h + v->em.y / 2 + hint->em.y / 4;
    if (x + hint->w > ui_app.crc.w) { x = ui_app.crc.w - hint->w - hint->em.x / 2; }
    if (x < 0) { x = hint->em.x / 2; }
    if (y + hint->h > ui_app.crc.h) { y = ui_app.crc.h - hint->h - hint->em.y / 2; }
    if (y < 0) { y = hint->em.y / 2; }
    // show_tooltip will center horizontally
    ui_app.show_tooltip(hint, x + hint->w / 2, y, 0);
}

static void ui_view_hovering(ui_view_t* view, bool start) {
    static ui_label_t hint = ui_label(0.0, "");
    if (start && ui_app.animating.view == null && view->hint[0] != 0 &&
       !ui_view.is_hidden(view)) {
        ui_view_show_hint(view, &hint);
    } else if (!start && ui_app.animating.view == &hint) {
        ui_app.show_tooltip(null, -1, -1, 0);
    }
}

static bool ui_view_is_shortcut_key(ui_view_t* view, int64_t key) {
    // Supported keyboard shortcuts are ASCII characters only for now
    // If there is not focused UI control in Alt+key [Alt] is optional.
    // If there is focused control only Alt+Key is accepted as shortcut
    char ch = 0x20 <= key && key <= 0x7F ? (char)toupper((char)key) : 0x00;
    bool need_alt = ui_app.focus != null && ui_app.focus != view;
    bool keyboard_shortcut = ch != 0x00 && view->shortcut != 0x00 &&
         (ui_app.alt || !need_alt) && toupper(view->shortcut) == ch;
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

// timers are delivered even to hidden and disabled views:

static void ui_view_timer(ui_view_t* view, ui_timer_t id) {
    if (view->timer != null) { view->timer(view, id); }
    ui_view_for_each(view, c, { ui_view_timer(c, id); });
}

static void ui_view_every_sec(ui_view_t* view) {
    if (view->every_sec != null) { view->every_sec(view); }
    ui_view_for_each(view, c, { ui_view_every_sec(c); });
}

static void ui_view_every_100ms(ui_view_t* view) {
    if (view->every_100ms != null) { view->every_100ms(view); }
    ui_view_for_each(view, c, { ui_view_every_100ms(c); });
}

static void ui_view_key_pressed(ui_view_t* view, int64_t k) {
    if (!ui_view.is_hidden(view) && !ui_view.is_disabled(view)) {
        if (view->key_pressed != null) { view->key_pressed(view, k); }
        ui_view_for_each(view, c, { ui_view_key_pressed(c, k); });
    }
}

static void ui_view_key_released(ui_view_t* view, int64_t k) {
    if (!ui_view.is_hidden(view) && !ui_view.is_disabled(view)) {
        if (view->key_released != null) { view->key_released(view, k); }
        ui_view_for_each(view, c, { ui_view_key_released(c, k); });
    }
}

static void ui_view_character(ui_view_t* view, const char* utf8) {
    if (!ui_view.is_hidden(view) && !ui_view.is_disabled(view)) {
        if (view->character != null) { view->character(view, utf8); }
        ui_view_for_each(view, c, { ui_view_character(c, utf8); });
    }
}

static void ui_view_paint(ui_view_t* view) {
    assert(ui_app.crc.w > 0 && ui_app.crc.h > 0);
    if (!view->hidden && ui_app.crc.w > 0 && ui_app.crc.h > 0) {
        if (view->paint != null) { view->paint(view); }
        ui_view_for_each(view, c, { ui_view_paint(c); });
    }
}

static bool ui_view_set_focus(ui_view_t* view) {
    bool set = false;
   ui_view_for_each(view, c, {
        set = ui_view_set_focus(c);
        if (set) { break; }
    });
    if (!set && !ui_view.is_hidden(view) && !ui_view.is_disabled(view) &&
        view->focusable && view->set_focus != null &&
       (ui_app.focus == view || ui_app.focus == null)) {
        set = view->set_focus(view);
    }
    return set;
}

static void ui_view_kill_focus(ui_view_t* view) {
    ui_view_for_each(view, c, { ui_view_kill_focus(c); });
    if (view->kill_focus != null && view->focusable) {
        view->kill_focus(view);
    }
}

static void ui_view_mouse(ui_view_t* view, int32_t m, int64_t f) {
    if (!ui_view.is_hidden(view) &&
       (m == ui.message.mouse_hover || m == ui.message.mouse_move)) {
        ui_rect_t r = { view->x, view->y, view->w, view->h};
        bool hover = view->hover;
        view->hover = ui.point_in_rect(&ui_app.mouse, &r);
        // inflate view rectangle:
        r.x -= view->w / 4;
        r.y -= view->h / 4;
        r.w += view->w / 2;
        r.h += view->h / 2;
        if (hover != view->hover) { ui_app.invalidate(&r); }
        if (hover != view->hover) {
            ui_view.hover_changed(view);
        }
    }
    if (!ui_view.is_hidden(view) && !ui_view.is_disabled(view)) {
        if (view->mouse != null) { view->mouse(view, m, f); }
        ui_view_for_each(view, c, { ui_view_mouse(c, m, f); });
    }
}

static void ui_view_mouse_wheel(ui_view_t* view, int32_t dx, int32_t dy) {
    if (!ui_view.is_hidden(view) && !ui_view.is_disabled(view)) {
        if (view->mouse_wheel != null) { view->mouse_wheel(view, dx, dy); }
        ui_view_for_each(view, c, { ui_view_mouse_wheel(c, dx, dy); });
    }
}

static void ui_view_measure_children(ui_view_t* view) {
    view->em = ui_gdi.get_em(*view->font);
    if (view->color_id > 0) {
        view->color = ui_app.get_color(view->color_id);
    }
    if (!view->hidden) {
        ui_view_for_each(view, c, { ui_view_measure_children(c); });
        if (view->measure != null) {
            view->measure(view);
        } else {
            ui_view.measure(view);
        }
    }
}

static void ui_view_layout_children(ui_view_t* view) {
    if (!view->hidden) {
        if (view->layout != null) { view->layout(view); }
        ui_view_for_each(view, c, { ui_view_layout_children(c); });
    }
}

static void ui_view_hover_changed(ui_view_t* view) {
    if (!view->hidden) {
        if (!view->hover) {
            view->hover_when = 0;
            ui_view.hovering(view, false); // cancel hover
        } else {
            swear(ui_view_hover_delay >= 0);
            if (view->hover_when >= 0) {
                view->hover_when = ui_app.now + ui_view_hover_delay;
            }
        }
    }
}

static void ui_view_kill_hidden_focus(ui_view_t* view) {
    // removes focus from hidden or disabled ui controls
    if (ui_app.focus != null) {
        if (ui_app.focus == view && (view->disabled || view->hidden)) {
            ui_app.focus = null;
            // even for disabled or hidden view notify about kill_focus:
            view->kill_focus(view);
        } else {
            ui_view_for_each(view, c, { ui_view_kill_hidden_focus(c); });
        }
    }
}

static bool ui_view_tap(ui_view_t* view, int32_t ix) { // 0: left 1: middle 2: right
    bool done = false; // consumed
    if (!ui_view.is_hidden(view) && !ui_view.is_disabled(view) &&
        ui_view_inside(view, &ui_app.mouse)) {
        ui_view_for_each(view, c, {
            done = ui_view_tap(c, ix);
            if (done) { break; }
        });

        if (view->tap != null && !done) { done = view->tap(view, ix); }
    }
    return done;
}

static bool ui_view_press(ui_view_t* view, int32_t ix) { // 0: left 1: middle 2: right
    bool done = false; // consumed
    if (!ui_view.is_hidden(view) && !ui_view.is_disabled(view)) {
        ui_view_for_each(view, c, {
            done = ui_view_press(c, ix);
            if (done) { break; }
        });
        if (view->press != null && !done) { done = view->press(view, ix); }
    }
    return done;
}

static bool ui_view_context_menu(ui_view_t* view) {
    if (!ui_view.is_hidden(view) && !ui_view.is_disabled(view)) {
        ui_view_for_each(view, c, {
            if (ui_view_context_menu(c)) { return true; }
        });
        ui_rect_t r = { view->x, view->y, view->w, view->h};
        if (ui.point_in_rect(&ui_app.mouse, &r)) {
            if (!view->hidden && !view->disabled && view->context_menu != null) {
                view->context_menu(view);
            }
        }
    }
    return false;
}

static bool ui_view_message(ui_view_t* view, int32_t m, int64_t wp, int64_t lp,
        int64_t* ret) {
    if (!view->hidden) {
        if (view->hover_when > 0 && ui_app.now > view->hover_when) {
            view->hover_when = -1; // "already called"
            ui_view.hovering(view, true);
        }
    }
    // message() callback is called even for hidden and disabled views
    // could be useful for enabling conditions of post() messages from
    // background ut_thread.
    if (view->message != null) {
        if (view->message(view, m, wp, lp, ret)) { return true; }
    }
    ui_view_for_each(view, c, {
        if (ui_view_message(c, m, wp, lp, ret)) { return true; }
    });
    return false;
}

static void ui_view_debug_paint(ui_view_t* v) {
    ui_gdi.push(v->x, v->y);
    if (v->color != ui_color_transparent) {
//      traceln("%s 0x%08X", v->text, v->color);
        ui_gdi.fill_with(v->x, v->y, v->w, v->h, v->color);
    }
    const int32_t p_lf = ui.gaps_em2px(v->em.x, v->padding.left);
    const int32_t p_tp = ui.gaps_em2px(v->em.y, v->padding.top);
    const int32_t p_rt = ui.gaps_em2px(v->em.x, v->padding.right);
    const int32_t p_bt = ui.gaps_em2px(v->em.y, v->padding.bottom);
    if (p_lf > 0) { ui_gdi.frame_with(v->x - p_lf, v->y, p_lf, v->h, ui_colors.green); }
    if (p_rt > 0) { ui_gdi.frame_with(v->x + v->w, v->y, p_rt, v->h, ui_colors.green); }
    if (p_tp > 0) { ui_gdi.frame_with(v->x, v->y - p_tp, v->w, p_tp, ui_colors.green); }
    if (p_bt > 0) { ui_gdi.frame_with(v->x, v->y + v->h, v->w, p_bt, ui_colors.green); }
    const int32_t i_lf = ui.gaps_em2px(v->em.x, v->insets.left);
    const int32_t i_tp = ui.gaps_em2px(v->em.y, v->insets.top);
    const int32_t i_rt = ui.gaps_em2px(v->em.x, v->insets.right);
    const int32_t i_bt = ui.gaps_em2px(v->em.y, v->insets.bottom);
    if (i_lf > 0) { ui_gdi.frame_with(v->x,               v->y,               i_lf, v->h, ui_colors.orange); }
    if (i_rt > 0) { ui_gdi.frame_with(v->x + v->w - i_rt, v->y,               i_rt, v->h, ui_colors.orange); }
    if (i_tp > 0) { ui_gdi.frame_with(v->x,               v->y,               v->w, i_tp, ui_colors.orange); }
    if (i_bt > 0) { ui_gdi.frame_with(v->x,               v->y + v->h - i_bt, v->w, i_bt, ui_colors.orange); }
    if (v->color != ui_color_transparent) {
        ui_gdi.set_text_color(ui_color_rgb(v->color) ^ 0xFFFFFF);
        ui_point_t mt = ui_gdi.measure_text(*v->font, v->text);
        ui_gdi.x += (v->w - mt.x) / 2;
        ui_gdi.y += (v->h - mt.y) / 2;
        ui_font_t f = ui_gdi.set_font(*v->font);
        ui_gdi.text("%s", v->text);
        ui_gdi.set_font(f);
    }
    ui_gdi.pop();
}

#pragma push_macro("ui_view_alone")

#define ui_view_alone(v) do {                          \
    swear((v)->parent == null && (v)->child == null && \
          (v)->prev == null && (v)->next == null);     \
} while (0)

static void ui_view_test(void) {
    ui_view_t p0 = ui_view(container);
    ui_view_t c1 = ui_view(container);
    ui_view_t c2 = ui_view(container);
    ui_view_t c3 = ui_view(container);
    ui_view_t c4 = ui_view(container);
    ui_view_t g1 = ui_view(container);
    ui_view_t g2 = ui_view(container);
    ui_view_t g3 = ui_view(container);
    ui_view_t g4 = ui_view(container);
    // add grand children to children:
    ui_view.add(&c2, &g1, &g2, null);               ui_view_verify(&c2);
    ui_view.add(&c3, &g3, &g4, null);               ui_view_verify(&c3);
    // single child
    ui_view.add(&p0, &c1, null);                    ui_view_verify(&p0);
    ui_view.remove(&c1);                            ui_view_verify(&p0);
    // two children
    ui_view.add(&p0, &c1, &c2, null);               ui_view_verify(&p0);
    ui_view.remove(&c1);                            ui_view_verify(&p0);
    ui_view.remove(&c2);                            ui_view_verify(&p0);
    // three children
    ui_view.add(&p0, &c1, &c2, &c3, null);          ui_view_verify(&p0);
    ui_view.remove(&c1);                            ui_view_verify(&p0);
    ui_view.remove(&c2);                            ui_view_verify(&p0);
    ui_view.remove(&c3);                            ui_view_verify(&p0);
    // add_first, add_last, add_before, add_after
    ui_view.add_first(&p0, &c1);                    ui_view_verify(&p0);
    swear(p0.child == &c1);
    ui_view.add_last(&p0, &c4);                     ui_view_verify(&p0);
    swear(p0.child == &c1 && p0.child->prev == &c4);
    ui_view.add_after(&c2, &c1);                    ui_view_verify(&p0);
    swear(p0.child == &c1);
    swear(c1.next == &c2);
    ui_view.add_before(&c3, &c4);                   ui_view_verify(&p0);
    swear(p0.child == &c1);
    swear(c4.prev == &c3);
    // removing all
    ui_view.remove(&c1);                            ui_view_verify(&p0);
    ui_view.remove(&c2);                            ui_view_verify(&p0);
    ui_view.remove(&c3);                            ui_view_verify(&p0);
    ui_view.remove(&c4);                            ui_view_verify(&p0);
    ui_view_alone(&p0); ui_view_alone(&c1); ui_view_alone(&c4);
    ui_view.remove(&g1);                            ui_view_verify(&c2);
    ui_view.remove(&g2);                            ui_view_verify(&c2);
    ui_view.remove(&g3);                            ui_view_verify(&c3);
    ui_view.remove(&g4);                            ui_view_verify(&c3);
    ui_view_alone(&c2); ui_view_alone(&c3);
    ui_view_alone(&g1); ui_view_alone(&g2); ui_view_alone(&g3); ui_view_alone(&g4);
    // a bit more intuitive (for a human) nested way to initialize tree:
    ui_view.add(&p0,
        &c1,
        ui_view.add(&c2, &g1, &g2, null),
        ui_view.add(&c3, &g3, &g4, null),
        &c4);
    ui_view_verify(&p0);
    ui_view_disband(&p0);
    ui_view_alone(&p0);
    ui_view_alone(&c1); ui_view_alone(&c2); ui_view_alone(&c3); ui_view_alone(&c4);
    ui_view_alone(&g1); ui_view_alone(&g2); ui_view_alone(&g3); ui_view_alone(&g4);
    if (ut_debug.verbosity.level > ut_debug.verbosity.quiet) { traceln("done"); }
}

#pragma pop_macro("ui_view_alone")

void ui_view_init(ui_view_t* view) {
    if (view->font == null) { view->font = &ui_app.fonts.regular; }
}

ui_view_if ui_view = {
    .add                = ui_view_add,
    .add_first          = ui_view_add_first,
    .add_last           = ui_view_add_last,
    .add_after          = ui_view_add_after,
    .add_before         = ui_view_add_before,
    .add                = ui_view_add,
    .remove             = ui_view_remove,
    .remove_all         = ui_view_remove_all,
    .disband            = ui_view_disband,
    .inside             = ui_view_inside,
    .set_text           = ui_view_set_text,
    .invalidate         = ui_view_invalidate,
    .measure            = ui_view_measure,
    .nls                = ui_view_nls,
    .localize           = ui_view_localize,
    .is_hidden          = ui_view_is_hidden,
    .is_disabled        = ui_view_is_disabled,
    .timer              = ui_view_timer,
    .every_sec          = ui_view_every_sec,
    .every_100ms        = ui_view_every_100ms,
    .key_pressed        = ui_view_key_pressed,
    .key_released       = ui_view_key_released,
    .character          = ui_view_character,
    .paint              = ui_view_paint,
    .set_focus          = ui_view_set_focus,
    .kill_focus         = ui_view_kill_focus,
    .kill_hidden_focus  = ui_view_kill_hidden_focus,
    .mouse              = ui_view_mouse,
    .mouse_wheel        = ui_view_mouse_wheel,
    .measure_children   = ui_view_measure_children,
    .layout_children    = ui_view_layout_children,
    .hovering           = ui_view_hovering,
    .hover_changed      = ui_view_hover_changed,
    .is_shortcut_key    = ui_view_is_shortcut_key,
    .context_menu       = ui_view_context_menu,
    .tap                = ui_view_tap,
    .press              = ui_view_press,
    .message            = ui_view_message,
    .debug_paint        = ui_view_debug_paint,
    .test               = ui_view_test
};

#ifdef UI_VIEW_TEST

ut_static_init(ui_view) {
    ui_view.test();
}

#endif

#pragma pop_macro("ui_view_for_each")

#endif // ui_implementation

