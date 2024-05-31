#ifndef ui_definition
#define ui_definition

// ___________________________________ ui.h ___________________________________

// alphabetical order is not possible because of headers interdependencies
// _________________________________ ut_std.h _________________________________

#include <ctype.h>
#include <errno.h>
#include <locale.h>
#include <malloc.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define ut_stringify(x) #x
#define ut_tostring(x) ut_stringify(x)
#define ut_pragma(x) _Pragma(ut_tostring(x))

#if defined(__GNUC__) || defined(__clang__) // TODO: remove and fix code
#pragma GCC diagnostic ignored "-Wgnu-zero-variadic-macro-arguments"
#pragma GCC diagnostic ignored "-Wdeclaration-after-statement"
#pragma GCC diagnostic ignored "-Wfour-char-constants"
#pragma GCC diagnostic ignored "-Wmissing-field-initializers"
#pragma GCC diagnostic ignored "-Wunsafe-buffer-usage"
#pragma GCC diagnostic ignored "-Wunused-function"
#pragma GCC diagnostic ignored "-Wfloat-equal"
#pragma GCC diagnostic ignored "-Wmissing-noreturn"
#pragma GCC diagnostic ignored "-Wdouble-promotion"
#pragma GCC diagnostic ignored "-Wcast-align"
#pragma GCC diagnostic ignored "-Waddress-of-packed-member"
#pragma GCC diagnostic ignored "-Wused-but-marked-unused" // because in debug only
#define ut_msvc_pragma(x)
#define ut_gcc_pragma(x) ut_pragma(x)
#else
#define ut_gcc_pragma(x)
#define ut_msvc_pragma(x) ut_pragma(x)
#endif

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
#define begin_packed ut_pragma( pack(push, 1) )
#define end_packed ut_pragma( pack(pop) )
#define attribute_packed
#endif

// usage: typedef struct ut_alligned_8 foo_s { ... } foo_t;

#if defined(__GNUC__) || defined(__clang__)
#define ut_alligned_8 __attribute__((aligned(8)))
#elif defined(_MSC_VER)
#define ut_alligned_8 __declspec(align(8))
#else
#define ut_alligned_8
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

#if defined(__GNUC__) || defined(__clang__)
#define unused(name) name __attribute__((unused))
#elif defined(_MSC_VER)
#define unused(name) _Pragma("warning(suppress:  4100)") name
#else
#define unused(name) name
#endif

// Because MS C compiler is unhappy about alloca() and
// does not implement (C99 optional) dynamic arrays on the stack:

#define ut_stackalloc(n) (_Pragma("warning(suppress: 6255 6263)") alloca(n))

// alloca() is messy and in general is a not a good idea.
// try to avoid if possible. Stack sizes vary from 64KB to 8MB in 2024.
// ________________________________ ui_core.h _________________________________

#include "ut/ut_std.h"


typedef struct ui_point_s { int32_t x, y; } ui_point_t;
typedef struct ui_rect_s { int32_t x, y, w, h; } ui_rect_t;
typedef struct ui_ltbr_s { int32_t left, top, right, bottom; } ui_ltrb_t;
typedef struct ui_wh_s   { int32_t w, h; } ui_wh_t;

typedef struct ui_window_s* ui_window_t;
typedef struct ui_icon_s*   ui_icon_t;
typedef struct ui_canvas_s* ui_canvas_t;
typedef struct ui_bitmap_s* ui_bitmap_t;
typedef struct ui_font_s*   ui_font_t;
typedef struct ui_brush_s*  ui_brush_t;
typedef struct ui_pen_s*    ui_pen_t;
typedef struct ui_cursor_s* ui_cursor_t;
typedef struct ui_region_s* ui_region_t;

typedef struct ui_fm_s { // font metrics
    ui_font_t font;
    ui_wh_t em;        // "em" almost square w/o ascend/descent
    // https://learn.microsoft.com/en-us/windows/win32/gdi/string-widths-and-heights
    int32_t height;    // font height in pixels
    int32_t baseline;  // font ascent; descent = height - baseline
    int32_t ascent;    // font ascent
    int32_t descent;   // font descent
    int32_t line_gap;  // internal_leading + external_leading
    int32_t internal_leading; // accents and diacritical marks goes there
    int32_t external_leading;
    bool mono;
} ui_fm_t;

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
    // font handles re-created "em" + font geometry filled on scale change
    ui_fm_t regular; // proportional UI font
    ui_fm_t mono; // monospaced  UI font
    ui_fm_t H1; // bold header font
    ui_fm_t H2;
    ui_fm_t H3;
} ui_fonts_t;

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
         support upto 16-16-16-14(A)bit per pixel color
         components with 'transparent' aka 'hollow' bit
*/

#define ui_color_mask        ((ui_color_t)0xC000000000000000ULL)
#define ui_color_undefined   ((ui_color_t)0x8000000000000000ULL)
#define ui_color_transparent ((ui_color_t)0x4000000000000000ULL)
#define ui_color_hdr         ((ui_color_t)0xC000000000000000ULL)

#define ui_color_is_8bit(c)         ( ((c) &  ui_color_mask) == 0)
#define ui_color_is_hdr(c)          ( ((c) &  ui_color_mask) == ui_color_hdr)
#define ui_color_is_undefined(c)    ( ((c) &  ui_color_mask) == ui_color_undefined)
#define ui_color_is_transparent(c)  ((((c) &  ui_color_mask) == ui_color_transparent) && \
                                    ( ((c) & ~ui_color_mask) == 0))
// if any other special colors or formats need to be introduced
// (c) & ~ui_color_mask) has 2^62 possible extensions bits

// ui_color_hdr A - 14 bit, R,G,B - 16 bit, all in range [0..0xFFFF]
#define ui_color_hdr_a(c)    ((uint16_t)((((c) >> 48) & 0x3FFF) << 2))
#define ui_color_hdr_r(c)    ((uint16_t)( ((c) >>  0) & 0xFFFF))
#define ui_color_hdr_g(c)    ((uint16_t)( ((c) >> 16) & 0xFFFF))
#define ui_color_hdr_b(c)    ((uint16_t)( ((c) >> 32) & 0xFFFF))

#define ui_color_a(c)        ((uint8_t)(((c) >> 24) & 0xFFU))
#define ui_color_r(c)        ((uint8_t)(((c) >>  0) & 0xFFU))
#define ui_color_g(c)        ((uint8_t)(((c) >>  8) & 0xFFU))
#define ui_color_b(c)        ((uint8_t)(((c) >> 16) & 0xFFU))

#define ui_color_rgb(c)      ((uint32_t)( (c) & 0x00FFFFFFU))
#define ui_color_rgba(c)     ((uint32_t)( (c) & 0xFFFFFFFFU))
#define ui_color_rgbFF(c)    ((uint32_t)(((c) & 0x00FFFFFFU)) | 0xFF000000U)

#define ui_rgb(r, g, b) ((ui_color_t)(                      \
                         (((uint32_t)(uint8_t)(r))      ) | \
                         (((uint32_t)(uint8_t)(g)) <<  8) | \
                         (((uint32_t)(uint8_t)(b)) << 16)))


#define ui_rgba(r, g, b, a)  ((ui_color_t)(                                   \
                              (ui_rgb(r, g, b)) |                             \
                              ((ui_color_t)((uint32_t)((uint8_t)(a))) << 24)) \
                             )

typedef struct ui_colors_s {
    void       (*rgb_to_hsi)(fp64_t r, fp64_t g, fp64_t b, fp64_t *h, fp64_t *s, fp64_t *i);
    ui_color_t (*hsi_to_rgb)(fp64_t h, fp64_t s, fp64_t i,  uint8_t a);
    // interpolate():
    //    0.0 < multiplier < 1.0 excluding boundaries
    //    alpha is interpolated as well
    ui_color_t (*interpolate)(ui_color_t c0, ui_color_t c1, fp32_t multiplier);
    ui_color_t (*gray_with_same_intensity)(ui_color_t c);
    // lighten() and darken() ignore alpha (use interpolate for alpha colors)
    ui_color_t (*lighten)(ui_color_t rgb, fp32_t multiplier); // interpolate toward white
    ui_color_t (*darken)(ui_color_t  rgb, fp32_t multiplier); // interpolate toward black
    ui_color_t (*adjust_saturation)(ui_color_t c,fp32_t multiplier);
    ui_color_t (*multiply_brightness)(ui_color_t c, fp32_t multiplier);
    ui_color_t (*multiply_saturation)(ui_color_t c, fp32_t multiplier);
    const ui_color_t transparent;
    const ui_color_t none; // aka CLR_INVALID in wingdi
    const ui_color_t text;
    const ui_color_t white;
    const ui_color_t black;
    const ui_color_t red;
    const ui_color_t green;
    const ui_color_t blue;
    const ui_color_t yellow;
    const ui_color_t cyan;
    const ui_color_t magenta;
    const ui_color_t gray;
    // darker shades of grey:
    const ui_color_t dkgray1; // 30 / 255 = 11.7%
    const ui_color_t dkgray2; // 38 / 255 = 15%
    const ui_color_t dkgray3; // 45 / 255 = 17.6%
    const ui_color_t dkgray4; // 63 / 255 = 24.0%
    // tone down RGB colors:
    const ui_color_t tone_white;
    const ui_color_t tone_red;
    const ui_color_t tone_green;
    const ui_color_t tone_blue;
    const ui_color_t tone_yellow;
    const ui_color_t tone_cyan;
    const ui_color_t tone_magenta;
    // miscelaneous:
    const ui_color_t orange;
    const ui_color_t dkgreen;
    const ui_color_t pink;
    const ui_color_t ochre;
    const ui_color_t gold;
    const ui_color_t teal;
    const ui_color_t wheat;
    const ui_color_t tan;
    const ui_color_t brown;
    const ui_color_t maroon;
    const ui_color_t barbie_pink;
    const ui_color_t steel_pink;
    const ui_color_t salmon_pink;
    const ui_color_t gainsboro;
    const ui_color_t light_gray;
    const ui_color_t silver;
    const ui_color_t dark_gray;
    const ui_color_t dim_gray;
    const ui_color_t light_slate_gray;
    const ui_color_t slate_gray;
    // highlights:
    const ui_color_t text_highlight; // bluish off-white
    const ui_color_t blue_highlight;
    const ui_color_t off_white;
    // button and other UI colors
    const ui_color_t btn_gradient_darker;
    const ui_color_t btn_gradient_dark;
    const ui_color_t btn_hover_highlight;
    const ui_color_t btn_disabled;
    const ui_color_t btn_armed;
    const ui_color_t btn_text;
    const ui_color_t toast; // toast background

    /* Named colors */

    /* Main Panel Backgrounds */
    const ui_color_t ennui_black; // rgb(18, 18, 18) 0x121212
    const ui_color_t charcoal;
    const ui_color_t onyx;
    const ui_color_t gunmetal;
    const ui_color_t jet_black;
    const ui_color_t outer_space;
    const ui_color_t eerie_black;
    const ui_color_t oil;
    const ui_color_t black_coral;
    const ui_color_t obsidian;

    /* Secondary Panels or Sidebars */
    const ui_color_t raisin_black;
    const ui_color_t dark_charcoal;
    const ui_color_t dark_jungle_green;
    const ui_color_t pine_tree;
    const ui_color_t rich_black;
    const ui_color_t eclipse;
    const ui_color_t cafe_noir;

    /* Flat Buttons */
    const ui_color_t prussian_blue;
    const ui_color_t midnight_green;
    const ui_color_t charleston_green;
    const ui_color_t rich_black_fogra;
    const ui_color_t dark_liver;
    const ui_color_t dark_slate_gray;
    const ui_color_t black_olive;
    const ui_color_t cadet;

    /* Button highlights (hover) */
    const ui_color_t dark_sienna;
    const ui_color_t bistre_brown;
    const ui_color_t dark_puce;
    const ui_color_t wenge;

    /* Raised button effects */
    const ui_color_t dark_scarlet;
    const ui_color_t burnt_umber;
    const ui_color_t caput_mortuum;
    const ui_color_t barn_red;

    /* Text and Icons */
    const ui_color_t platinum;
    const ui_color_t anti_flash_white;
    const ui_color_t silver_sand;
    const ui_color_t quick_silver;

    /* Links and Selections */
    const ui_color_t dark_powder_blue;
    const ui_color_t sapphire_blue;
    const ui_color_t international_klein_blue;
    const ui_color_t zaffre;

    /* Additional Colors */
    const ui_color_t fish_belly;
    const ui_color_t rusty_red;
    const ui_color_t falu_red;
    const ui_color_t cordovan;
    const ui_color_t dark_raspberry;
    const ui_color_t deep_magenta;
    const ui_color_t byzantium;
    const ui_color_t amethyst;
    const ui_color_t wisteria;
    const ui_color_t lavender_purple;
    const ui_color_t opera_mauve;
    const ui_color_t mauve_taupe;
    const ui_color_t rich_lavender;
    const ui_color_t pansy_purple;
    const ui_color_t violet_eggplant;
    const ui_color_t jazzberry_jam;
    const ui_color_t dark_orchid;
    const ui_color_t electric_purple;
    const ui_color_t sky_magenta;
    const ui_color_t brilliant_rose;
    const ui_color_t fuchsia_purple;
    const ui_color_t french_raspberry;
    const ui_color_t wild_watermelon;
    const ui_color_t neon_carrot;
    const ui_color_t burnt_orange;
    const ui_color_t carrot_orange;
    const ui_color_t tiger_orange;
    const ui_color_t giant_onion;
    const ui_color_t rust;
    const ui_color_t copper_red;
    const ui_color_t dark_tangerine;
    const ui_color_t bright_marigold;
    const ui_color_t bone;

    /* Earthy Tones */
    const ui_color_t sienna;
    const ui_color_t sandy_brown;
    const ui_color_t golden_brown;
    const ui_color_t camel;
    const ui_color_t burnt_sienna;
    const ui_color_t khaki;
    const ui_color_t dark_khaki;

    /* Greens */
    const ui_color_t fern_green;
    const ui_color_t moss_green;
    const ui_color_t myrtle_green;
    const ui_color_t pine_green;
    const ui_color_t jungle_green;
    const ui_color_t sacramento_green;

    /* Blues */
    const ui_color_t yale_blue;
    const ui_color_t cobalt_blue;
    const ui_color_t persian_blue;
    const ui_color_t royal_blue;
    const ui_color_t iceberg;
    const ui_color_t blue_yonder;

    /* Miscellaneous */
    const ui_color_t cocoa_brown;
    const ui_color_t cinnamon_satin;
    const ui_color_t fallow;
    const ui_color_t cafe_au_lait;
    const ui_color_t liver;
    const ui_color_t shadow;
    const ui_color_t cool_grey;
    const ui_color_t payne_grey;

    /* Lighter Tones for Contrast */
    const ui_color_t timberwolf;
    const ui_color_t silver_chalice;
    const ui_color_t roman_silver;

    /* Dark Mode Specific Highlights */
    const ui_color_t electric_lavender;
    const ui_color_t magenta_haze;
    const ui_color_t cyber_grape;
    const ui_color_t purple_navy;
    const ui_color_t liberty;
    const ui_color_t purple_mountain_majesty;
    const ui_color_t ceil;
    const ui_color_t moonstone_blue;
    const ui_color_t independence;
} ui_colors_if;

extern ui_colors_if ui_colors;

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
    void (*line_with)(int32_t x0, int32_t y1, int32_t x2, int32_t y2,
                      ui_color_t c);
    void (*frame_with)(int32_t x, int32_t y, int32_t w, int32_t h,
                      ui_color_t c);
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
    void      (*delete_font)(ui_font_t f);
    ui_font_t (*set_font)(ui_font_t f);
    // IMPORTANT: relationship between font_height(), baseline(), descent()
    // E.g. for monospaced font on dpi=96 (monitor_raw=101) and font_height=15
    // the get_em() is: 9 20 font_height(): 15 baseline: 16 descent: 4
    // Monospaced fonts are not `em` "M" square!
    int32_t (*font_height)(ui_font_t f); // font height in pixels
    int32_t (*descent)(ui_font_t f);     // font descent (glyphs below baseline)
    int32_t (*baseline)(ui_font_t f);    // height - baseline (aka ascent) = descent
    // https://en.wikipedia.org/wiki/Em_(typography)
    void (*update_fm)(ui_fm_t* fm, ui_font_t f); // fills font metrics (pixel size of glyph "M")
    // get_em(f) returns { "M".w, "M".h - f.descent - (f.height - f.baseline)};
    ui_wh_t (*measure_text)(ui_font_t f, const char* format, ...);
    // width can be -1 which measures text with "\n" or
    // positive number of pixels
    ui_wh_t (*measure_multiline)(ui_font_t f, int32_t w, const char* format, ...);
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
    ui_fm_t* fm; // font metrics
    int32_t shortcut; // keyboard shortcut
    int32_t strid; // 0 for not localized ui
    void* that;  // for the application use
    void (*notify)(ui_view_t* view, void* p); // for the application use
    // two pass layout: measure() .w, .h layout() .x .y
    // first  measure() bottom up - children.layout before parent.layout
    // second layout() top down - parent.layout before children.layout
    // before methods: called before measure()/layout()/paint()
    void (*prepare)(ui_view_t* view);  // called before measure()
    void (*measure)(ui_view_t* view);  // determine w, h (bottom up)
    void (*measured)(ui_view_t* view); // called after measure()
    void (*layout)(ui_view_t* view);   // set x, y possibly adjust w, h (top down)
    void (*composed)(ui_view_t* view); // after layout() is done (laid out)
    void (*paint)(ui_view_t* view);
    void (*painted)(ui_view_t* view);  // called after paint()
    // composed() is effectively called right before paint() and
    // can be used to prepare for painting w/o need to override paint()
    void (*debug_paint)(ui_view_t* view); // called if .debug is set to true
    // any message:
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
    int64_t (*hit_test)(ui_view_t* v, int32_t x, int32_t y);
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
    int32_t    color_id;  // 0 is default meaning use color
    ui_color_t background;    // interpretation depends on view type
    int32_t    background_id; // 0 is default meaning use background
    bool       debug; // activates debug_paint() called after painted()
    char hint[256]; // tooltip hint text (to be shown while hovering over view)
} ui_view_t;

// tap() / press() APIs guarantee that single tap() is not coming
// before fp64_t tap/click in expense of fp64_t click delay (0.5 seconds)
// which is OK for buttons and many other UI controls but absolutely not
// OK for text editing. Thus edit uses raw mouse events to react
// on clicks and fp64_t clicks.

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
    ui_ltrb_t (*gaps)(ui_view_t* view, const ui_gaps_t* g); // gaps to pixels
    void (*inbox)(ui_view_t* view, ui_rect_t* r, ui_ltrb_t* insets);
    void (*outbox)(ui_view_t* view, ui_rect_t* r, ui_ltrb_t* padding);
    void (*position_by_outbox)(ui_view_t* view, const ui_rect_t* r,
            const ui_ltrb_t* padding);
    void (*set_text)(ui_view_t* view, const char* text);
    void (*invalidate)(const ui_view_t* view); // prone to delays
    bool (*is_hidden)(const ui_view_t* view);   // view or any parent is hidden
    bool (*is_disabled)(const ui_view_t* view); // view or any parent is disabled
    const char* (*nls)(ui_view_t* view);  // returns localized text
    void (*localize)(ui_view_t* view);    // set strid based ui .text field
    void (*timer)(ui_view_t* view, ui_timer_t id);
    void (*every_sec)(ui_view_t* view);
    void (*every_100ms)(ui_view_t* view);
    int64_t (*hit_test)(ui_view_t* v, int32_t x, int32_t y);
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
    void (*measure)(ui_view_t* view);
    void (*layout)(ui_view_t* view);
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
// ___________________________________ ui.h ___________________________________

// alphabetical order is not possible because of headers interdependencies
// ________________________________ ui_edit.h _________________________________

/* Copyright (c) Dmitry "Leo" Kuznetsov 2021-24 see LICENSE for details */


// important ui_edit_t will refuse to layout into a box smaller than
// width 3 x fm->em.w height 1 x fm->em.h

typedef struct ui_edit_run_s {
    int32_t bp;     // position in bytes  since start of the paragraph
    int32_t gp;     // position in glyphs since start of the paragraph
    int32_t bytes;  // number of bytes in this `run`
    int32_t glyphs; // number of glyphs in this `run`
    int32_t pixels; // width in pixels
} ui_edit_run_t;

// ui_edit_para_t.initially text will point to readonly memory
// with .allocated == 0; as text is modified it is copied to
// heap and reallocated there.

typedef struct ui_edit_para_s { // "paragraph"
    char* text;          // text[bytes] utf-8
    int32_t capacity;   // if != 0 text copied to heap allocated bytes
    int32_t bytes;       // number of bytes in utf-8 text
    int32_t glyphs;      // number of glyphs in text <= bytes
    int32_t runs;        // number of runs in this paragraph
    ui_edit_run_t* run; // [runs] array of pointers (heap)
    int32_t* g2b;        // [bytes + 1] glyph to uint8_t positions g2b[0] = 0
    int32_t  g2b_capacity; // number of bytes on heap allocated for g2b[]
} ui_edit_para_t;

typedef struct ui_edit_pg_s { // page/glyph coordinates
    // humans used to line:column coordinates in text
    int32_t pn; // paragraph number ("line number")
    int32_t gp; // glyph position ("column")
} ui_edit_pg_t;

typedef struct ui_edit_pr_s { // page/run coordinates
    int32_t pn; // paragraph number
    int32_t rn; // run number inside paragraph
} ui_edit_pr_t;

typedef struct ui_edit_s ui_edit_t;

typedef struct ui_edit_s {
    ui_view_t view;
    void (*set_font)(ui_edit_t* e, ui_fm_t* fm); // see notes below (*)
    void (*move)(ui_edit_t* e, ui_edit_pg_t pg); // move caret clear selection
    // replace selected text. If bytes < 0 text is treated as zero terminated
    void (*paste)(ui_edit_t* e, const char* text, int32_t bytes);
    errno_t (*copy)(ui_edit_t* e, char* text, int32_t* bytes); // copy whole text
    void (*copy_to_clipboard)(ui_edit_t* e); // selected text to clipboard
    void (*cut_to_clipboard)(ui_edit_t* e);  // copy selected text to clipboard and erase it
    // replace selected text with content of clipboard:
    void (*paste_from_clipboard)(ui_edit_t* e);
    void (*select_all)(ui_edit_t* e); // select whole text
    void (*erase)(ui_edit_t* e); // delete selected text
    // keyboard actions dispatcher:
    void (*key_down)(ui_edit_t* e);
    void (*key_up)(ui_edit_t* e);
    void (*key_left)(ui_edit_t* e);
    void (*key_right)(ui_edit_t* e);
    void (*key_pageup)(ui_edit_t* e);
    void (*key_pagedw)(ui_edit_t* e);
    void (*key_home)(ui_edit_t* e);
    void (*key_end)(ui_edit_t* e);
    void (*key_delete)(ui_edit_t* e);
    void (*key_backspace)(ui_edit_t* e);
    void (*key_enter)(ui_edit_t* e);
    // called when ENTER keyboard key is pressed in single line mode
    void (*enter)(ui_edit_t* e);
    // fuzzer test:
    void (*fuzz)(ui_edit_t* e);      // start/stop fuzzing test
    void (*next_fuzz)(ui_edit_t* e); // next fuzz input event(s)
    ui_edit_pg_t selection[2]; // from selection[0] to selection[1]
    ui_point_t caret; // (-1, -1) off
    ui_edit_pr_t scroll; // left top corner paragraph/run coordinates
    int32_t last_x;    // last_x for up/down caret movement
    int32_t mouse;     // bit 0 and bit 1 for LEFT and RIGHT buttons down
    int32_t top;       // y coordinate of the top of view
    int32_t bottom;    // '' (ditto) of the bottom
    // number of fully (not partially clipped) visible `runs' from top to bottom:
    int32_t visible_runs;
    bool focused;  // is focused and created caret
    bool ro;       // Read Only
    bool sle;      // Single Line Edit
    int32_t shown; // debug: caret show/hide counter 0|1
    // https://en.wikipedia.org/wiki/Fuzzing
    volatile ut_thread_t fuzzer;     // fuzzer thread != null when fuzzing
    volatile int32_t  fuzz_count; // fuzzer event count
    volatile int32_t  fuzz_last;  // last processed fuzz
    volatile bool     fuzz_quit;  // last processed fuzz
    // random32 starts with 1 but client can seed it with (ut_clock.nanoseconds() | 1)
    uint32_t fuzz_seed;    // fuzzer random32 seed (must start with odd number)
    // paragraphs memory:
    int32_t capacity;      // number of bytes allocated for `para` array below
    int32_t paragraphs;    // number of lines in the text
    ui_edit_para_t* para; // para[paragraphs]
} ui_edit_t;

/*
    Notes:
    set_font() - neither edit.view.font = font nor measure()/layout() functions
                 do NOT dispose paragraphs layout unless geometry changed because
                 it is quite expensive operation. But choosing different font
                 on the fly needs to re-layout all paragraphs. Thus caller needs
                 to set font via this function instead which also requests
                 edit UI element re-layout.

    .ro        - readonly edit->ro is used to control readonly mode.
                 If edit control is readonly its appearance does not change but it
                 refuses to accept any changes to the rendered text.

    .wb        - wordbreak this attribute was removed as poor UX human experience
                 along with single line scroll editing. See note below about .sle.

    .sle       - Single line edit control.
                 Edit UI element does NOT support horizontal scroll and breaking
                 words semantics as it is poor UX human experience. This is not
                 how humans (apart of software developers) edit text.
                 If content of the edit UI element is wider than the bounding box
                 width the content is broken on word boundaries and vertical scrolling
                 semantics is supported. Layouts containing edit control of the single
                 line height are strongly encouraged to enlarge edit control layout
                 vertically on as needed basis similar to Google Search Box behavior
                 change implemented in 2023.
                 If multiline is set to true by the callers code the edit UI layout
                 snaps text to the top of x,y,w,h box otherwise the vertical space
                 is distributed evenly between single line of text and top bottom gaps.
                 IMPORTANT: SLE resizes itself vertically to accommodate for
                 input that is too wide. If caller wants to limit vertical space it
                 will need to hook .measure() function of SLE and do the math there.
*/

void ui_edit_init(ui_edit_t* e);

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

#define ui_label(min_width_em, s) {                      \
      .type = ui_view_label, .init = ui_view_init_label, \
      .fm = &ui_app.fonts.regular,                       \
      .text = s,                                         \
      .min_w_em = min_width_em, .min_h_em = 1.0,         \
      .padding = { .left  = 0.25, .top = 0.25,           \
                   .right = 0.25, .bottom = 0.25, },     \
      .insets  = { .left  = 0.25, .top = 0.25,           \
                   .right = 0.25, .bottom = 0.25, }      \
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

// ui_button_on_click can only be used on static button variables

#define ui_button_on_click(name, s, min_width_em, ...)         \
    static void name ## _callback(ui_button_t* name) {         \
        (void)name; /* no warning if unused */                 \
        { __VA_ARGS__ }                                        \
    }                                                          \
    static                                                     \
    ui_button_t name = {                                       \
        .type = ui_view_button, .init = ui_view_init_button,   \
        .fm = &ui_app.fonts.regular,                           \
        .text = s, .callback = name ## _callback,              \
        .min_w_em = min_width_em, .min_h_em = 1.0,             \
        .padding = { .left  = 0.25, .top = 0.25,               \
                     .right = 0.25, .bottom = 0.25, },         \
        .insets  = { .left  = 0.25, .top = 0.25,               \
                     .right = 0.25, .bottom = 0.25, }          \
    }

#define ui_button(s, min_width_em, call_back) {              \
    .type = ui_view_button, .init = ui_view_init_button,     \
    .fm = &ui_app.fonts.regular,                             \
    .text = s, .callback = call_back,                        \
    .min_w_em = min_width_em, .min_h_em = 1.0,               \
    .padding = { .left  = 0.25, .top = 0.25,                 \
                 .right = 0.25, .bottom = 0.25, },           \
    .insets  = { .left  = 0.25, .top = 0.25,                 \
                 .right = 0.25, .bottom = 0.25, }            \
}

// usage:
//
// ui_button_on_click(button, "&Button", 7.0, {
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
// ut_str_printf(button.text, "&Button");
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

// ui_toggle_on_switch can only be used on static toggle variables

#define ui_toggle_on_switch(name, s, min_width_em, ...)        \
    static void name ## _callback(ui_toggle_t* name) {         \
        (void)name; /* no warning if unused */                 \
        { __VA_ARGS__ }                                        \
    }                                                          \
    static                                                     \
    ui_toggle_t name = {                                       \
        .type = ui_view_toggle, .init = ui_view_init_toggle,   \
        .fm = &ui_app.fonts.regular, .min_w_em = min_width_em, \
        .text = s, .callback = name ## _callback,              \
        .min_w_em = 1.0, .min_h_em = 1.0,                      \
        .padding = { .left  = 0.25, .top = 0.25,               \
                     .right = 0.25, .bottom = 0.25, },         \
        .insets  = { .left  = 0.25, .top = 0.25,               \
                     .right = 0.25, .bottom = 0.25, }          \
    }

#define ui_toggle(s, min_width_em, call_back) {            \
    .type = ui_view_toggle, .init = ui_view_init_toggle,   \
    .fm = &ui_app.fonts.regular, .min_w_em = min_width_em, \
    .text = s, .callback = call_back,                      \
    .min_w_em = 1.0, .min_h_em = 1.0,                      \
    .padding = { .left  = 0.25, .top = 0.25,               \
                 .right = 0.25, .bottom = 0.25, },         \
    .insets  = { .left  = 0.25, .top = 0.25,               \
                 .right = 0.25, .bottom = 0.25, }          \
}

// _______________________________ ui_slider.h ________________________________

#include "ut/ut_std.h"


typedef struct ui_slider_s ui_slider_t;

typedef struct ui_slider_s {
    ui_view_t view;
    int32_t step;
    fp64_t time; // time last button was pressed
    ui_wh_t mt;  // text measurement (special case for %0*d)
    ui_button_t inc;
    ui_button_t dec;
    int32_t value;  // for ui_slider_t range slider control
    int32_t value_min;
    int32_t value_max;
} ui_slider_t;

void ui_view_init_slider(ui_view_t* view);

void ui_slider_init(ui_slider_t* r, const char* label, fp32_t min_w_em,
    int32_t value_min, int32_t value_max, void (*callback)(ui_view_t* r));

// ui_slider_on_change can only be used on static slider variables

#define ui_slider_on_change(name, s, min_width_em, vmn, vmx, ...)      \
    static void name ## _callback(ui_slider_t* name) {                 \
        (void)name; /* no warning if unused */                         \
        { __VA_ARGS__ }                                                \
    }                                                                  \
    static                                                             \
    ui_slider_t name = {                                               \
        .view = { .type = ui_view_slider, .fm = &ui_app.fonts.regular, \
                  .init = ui_view_init_slider,                         \
                  .text = s, .callback = name ## _callback,            \
                  .min_w_em = min_width_em, .min_h_em = 1.0,           \
                  .padding = { .left  = 0.25, .top = 0.25,             \
                               .right = 0.25, .bottom = 0.25, },       \
                  .insets  = { .left  = 0.25, .top = 0.25,             \
                               .right = 0.25, .bottom = 0.25, }        \
        },                                                             \
        .value_min = vmn, .value_max = vmx, .value = vmn,              \
    }

#define ui_slider(s, min_width_em, vmn, vmx, call_back) {              \
    .view = { .type = ui_view_slider, .fm = &ui_app.fonts.regular,     \
        .text = s, .init = ui_view_init_slider,                        \
        .callback = call_back,                                         \
        .min_w_em = min_width_em, .min_h_em = 1.0,                     \
        .padding = { .left  = 0.25, .top = 0.25,                       \
                     .right = 0.25, .bottom = 0.25, },                 \
        .insets  = { .left  = 0.25, .top = 0.25,                       \
                     .right = 0.25, .bottom = 0.25, }                  \
    },                                                                 \
    .value_min = vmn, .value_max = vmx, .value = vmn,                  \
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

// ui_mbx_on_choice can only be used on static mbx variables


#define ui_mbx_on_choice(name, s, code, ...)                     \
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
                  .fm = &ui_app.fonts.regular,                   \
                  .text = s, .callback = name ## _callback,      \
                  .padding = { .left  = 0.25, .top = 0.25,       \
                               .right = 0.25, .bottom = 0.25, }, \
                  .insets  = { .left  = 0.25, .top = 0.25,       \
                               .right = 0.25, .bottom = 0.25, }  \
                },                                               \
        .options = name ## _options                              \
    }

#define ui_mbx(s, call_back, ...) {                          \
    .view = { .type = ui_view_mbx, .init = ui_view_init_mbx, \
              .fm = &ui_app.fonts.regular,                   \
              .text = s, .callback = call_back,              \
              .padding = { .left  = 0.25, .top = 0.25,       \
                           .right = 0.25, .bottom = 0.25, }, \
              .insets  = { .left  = 0.25, .top = 0.25,       \
                           .right = 0.25, .bottom = 0.25, }  \
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
    ui_view_t* root; // show_window() changes ui.hidden
    ui_view_t* content;
    ui_view_t* caption;
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
    // mouse buttons state
    bool mouse_left;  // left or if buttons are swapped - right button pressed
    bool mouse_right; // context button pressed
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
    void (*request_layout)(void); // requests layout on UI tree before paint()
    void (*invalidate)(const ui_rect_t* rc);
    void (*full_screen)(bool on);
    void (*request_redraw)(void); // very fast (5 microseconds) InvalidateRect(null)
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
    ut_file_name_t (*open_file_dialog)(const char* folder, const char* filter[], int32_t n);
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
    .fm   = &ui_app.fonts.regular       \
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
#pragma push_macro("ui_monospaced_font")

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
        sizeof(ui_app_ncm), &ui_app_ncm, 0, (DWORD)dpi));
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
                    dpi->monitor_effective = (int32_t)ut_max(dpi_x, dpi_y); break;
                case MDT_ANGULAR_DPI:
                    dpi->monitor_angular = (int32_t)ut_max(dpi_x, dpi_y); break;
                case MDT_RAW_DPI:
                    dpi->monitor_raw = (int32_t)ut_max(dpi_x, dpi_y); break;
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
    fp64_t monitor_x = (fp64_t)scr_x / (fp64_t)ui_app.dpi.monitor_raw;
    fp64_t monitor_y = (fp64_t)scr_y / (fp64_t)ui_app.dpi.monitor_raw;
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
    ui_gdi.delete_font(ui_app.fonts.regular.font);
    ui_gdi.delete_font(ui_app.fonts.H1.font);
    ui_gdi.delete_font(ui_app.fonts.H2.font);
    ui_gdi.delete_font(ui_app.fonts.H3.font);
    ui_gdi.delete_font(ui_app.fonts.mono.font);
}

static void ui_app_init_fonts(int32_t dpi) {
    ui_app_update_ncm(dpi);
    if (ui_app.fonts.regular.font != null) { ui_app_dispose_fonts(); }
    LOGFONTW lf = ui_app_ncm.lfMessageFont;
    // lf.lfQuality is CLEARTYPE_QUALITY which looks bad on 4K monitors
    // Windows UI uses PROOF_QUALITY which is aliased w/o ClearType rainbows
    lf.lfQuality = PROOF_QUALITY;
    ui_gdi.update_fm(&ui_app.fonts.regular, (ui_font_t)CreateFontIndirectW(&lf));
    const fp64_t fh = ui_app_ncm.lfMessageFont.lfHeight;
//  traceln("lfHeight=%.1f", fh);
    assert(fh != 0);
    lf.lfWeight = FW_SEMIBOLD;
    lf.lfHeight = (int32_t)(fh * 1.75);
    ui_gdi.update_fm(&ui_app.fonts.H1, (ui_font_t)CreateFontIndirectW(&lf));
    lf.lfWeight = FW_SEMIBOLD;
    lf.lfHeight = (int32_t)(fh * 1.4);
    ui_gdi.update_fm(&ui_app.fonts.H2, (ui_font_t)CreateFontIndirectW(&lf));
    lf.lfWeight = FW_SEMIBOLD;
    lf.lfHeight = (int32_t)(fh * 1.15);
    ui_gdi.update_fm(&ui_app.fonts.H3, (ui_font_t)CreateFontIndirectW(&lf));
    lf = ui_app_ncm.lfMessageFont;
    lf.lfPitchAndFamily = FIXED_PITCH;
    // TODO: how to get monospaced from Win32 API?
    #define ui_monospaced_font L"Cascadia Code"
    wcscpy(lf.lfFaceName, ui_monospaced_font);
    ui_gdi.update_fm(&ui_app.fonts.mono, (ui_font_t)CreateFontIndirectW(&lf));
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
    int32_t    padding;      // to allign rects and points to 8 bytes
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
        HDC unused(hdc), RECT* unused(rc1), LPARAM that) {
    ui_app_wiw_t* wiw = (ui_app_wiw_t*)(uintptr_t)that;
    MONITORINFOEX mi = { .cbSize = sizeof(MONITORINFOEX) };
    fatal_if_false(GetMonitorInfoA(monitor, (MONITORINFO*)&mi));
    // monitors can be in negative coordinate spaces and even rotated upside-down
    const int32_t min_x = ut_min(mi.rcMonitor.left, mi.rcMonitor.right);
    const int32_t min_y = ut_min(mi.rcMonitor.top,  mi.rcMonitor.bottom);
    const int32_t max_w = ut_max(mi.rcMonitor.left, mi.rcMonitor.right);
    const int32_t max_h = ut_max(mi.rcMonitor.top,  mi.rcMonitor.bottom);
    wiw->space.x = ut_min(wiw->space.x, min_x);
    wiw->space.y = ut_min(wiw->space.y, min_y);
    wiw->space.w = ut_max(wiw->space.w, max_w);
    wiw->space.h = ut_max(wiw->space.h, max_h);
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
        .flags = (int32_t)wpl.flags,
        .show  = (int32_t)wpl.showCmd
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
            traceln("GetConsoleScreenBufferInfoEx() %s", strerr(r));
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
        ui_app_update_mi(&wiw.placement, MONITOR_DEFAULTTONEAREST);
        bool same_monitor = memcmp(&wiw.mrc, &ui_app.mrc, sizeof(wiw.mrc)) == 0;
//      traceln("%d,%d %dx%d", p->x, p->y, p->w, p->h);
        if (same_monitor) {
            *rect = wiw.placement;
        } else { // moving to another monitor
            rect->x = (wiw.placement.x - wiw.mrc.x) * ui_app.mrc.w / wiw.mrc.w;
            rect->y = (wiw.placement.y - wiw.mrc.y) * ui_app.mrc.h / wiw.mrc.h;
            // adjust according to monitors DPI difference:
            // (w, h) theoretically could be as large as 0xFFFF
            const int64_t w = (int64_t)wiw.placement.w * ui_app.dpi.monitor_raw;
            const int64_t h = (int64_t)wiw.placement.h * ui_app.dpi.monitor_raw;
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
        ui_app_update_mi(&wiw.placement, MONITOR_DEFAULTTONEAREST);
        bool same_monitor = memcmp(&wiw.mrc, &ui_app.mrc, sizeof(wiw.mrc)) == 0;
//      traceln("%d,%d %dx%d", p->x, p->y, p->w, p->h);
        if (same_monitor) {
            *rect = wiw.placement;
        } else { // moving to another monitor
            rect->x = (wiw.placement.x - wiw.mrc.x) * ui_app.mrc.w / wiw.mrc.w;
            rect->y = (wiw.placement.y - wiw.mrc.y) * ui_app.mrc.h / wiw.mrc.h;
            // adjust according to monitors DPI difference:
            // (w, h) theoretically could be as large as 0xFFFF
            const int64_t w = (int64_t)wiw.placement.w * ui_app.dpi.monitor_raw;
            const int64_t h = (int64_t)wiw.placement.h * ui_app.dpi.monitor_raw;
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
    fatal_if_false(PostMessageA(ui_app_window(), (UINT)m,
            (WPARAM)wp, (LPARAM)lp));
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
    traceln("ui_app.dpi.window: %d", ui_app.dpi.window);
}

static void ui_app_window_opening(void) {
    ui_app_window_dpi();
    ui_app_init_fonts(ui_app.dpi.window);
    ui_app_timer_1s_id = ui_app.set_timer((uintptr_t)&ui_app_timer_1s_id, 1000);
    ui_app_timer_100ms_id = ui_app.set_timer((uintptr_t)&ui_app_timer_100ms_id, 100);
    ui_app.set_cursor(ui_app.cursor_arrow);
    ui_app.canvas = (ui_canvas_t)GetDC(ui_app_window());
    not_null(ui_app.canvas);
    if (ui_app.opened != null) { ui_app.opened(); }
    ut_str_printf(ui_app.root->text, "ui_app.root"); // debugging
    ui_app_wm_timer(ui_app_timer_100ms_id);
    ui_app_wm_timer(ui_app_timer_1s_id);
    fatal_if(ReleaseDC(ui_app_window(), ui_app_canvas()) == 0);
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

static void ui_app_allow_dark_mode_for_app(void) {
    // https://github.com/rizonesoft/Notepad3/tree/96a48bd829a3f3192bbc93cd6944cafb3228b96d/src/DarkMode
    HMODULE uxtheme = GetModuleHandleA("uxtheme.dll");
    not_null(uxtheme);
    typedef BOOL (__stdcall *AllowDarkModeForApp_t)(bool allow);
    AllowDarkModeForApp_t AllowDarkModeForApp = (AllowDarkModeForApp_t)
            (void*)GetProcAddress(uxtheme, MAKEINTRESOURCE(132));
    if (AllowDarkModeForApp != null) {
        errno_t r = b2e(AllowDarkModeForApp(true));
        if (r != 0 && r != ERROR_PROC_NOT_FOUND) {
            traceln("AllowDarkModeForApp(true) failed %s", strerr(r));
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
                    strerr(r));
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
            traceln("AllowDarkModeForWindow(true) failed %s", strerr(r));
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
    // restore from minimized calls ui_app.crc.w,h == 0
    if (ui_app.crc.w > 0 && ui_app.crc.h > 0 && ui_app_window() != null) {
        ui_view.measure(view);
        ui_view.layout(view);
        ui_app_layout_dirty = false;
    }
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
    ui_app_mouse(ui_app.root, (int32_t)m, wp);
    int32_t ix = (int32_t)wp;
    assert(0 <= ix && ix <= 2);
    // for now long press and fp64_t tap/fp64_t click
    // treated as press() call - can be separated if desired:
    if (m == ui.message.tap) {
        ui_view.tap(ui_app.root, ix);
    } else if (m == ui.message.dtap) {
        ui_view.press(ui_app.root, ix);
    } else if (m == ui.message.press) {
        ui_view.press(ui_app.root, ix);
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
        ui_view.measure(ui_app.animating.view);
        ui_gdi.push(0, 0);
        bool tooltip = ui_app.animating.x >= 0 && ui_app.animating.y >= 0;
        const int32_t em_x = ui_app.animating.view->fm->em.w;
        const int32_t em_y = ui_app.animating.view->fm->em.h;
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
                ui_gdi.text("%s", ut_glyph_multiplication_sign);
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
    ui_app.request_redraw();
}

static void ui_app_toast_mouse(int32_t m, int64_t flags) {
    bool pressed = m == ui.message.left_button_pressed ||
                   m == ui.message.right_button_pressed;
    if (ui_app.animating.view != null && pressed) {
        const ui_fm_t* fm = ui_app.animating.view->fm;
        int32_t x = ui_app.animating.view->x + ui_app.animating.view->w;
        if (x <= ui_app.mouse.x && ui_app.mouse.x <= x + fm->em.w &&
            0 <= ui_app.mouse.y && ui_app.mouse.y <= fm->em.h) {
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
    ui_app.request_redraw();
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
    // A view can be bigger then client rectangle but shouldn't be smaller
    // or shifted:
    assert(v == ui_app.root && v->x == 0 && v->y == 0 &&
           v->w >= ui_app.crc.w && v->h >= ui_app.crc.h);
    v->color = ui_app.get_color(v->color_id);
    if (v->background_id > 0) {
        v->background = ui_app.get_color(v->background_id);
    }
    if (!ui_color_is_undefined(v->background) &&
        !ui_color_is_transparent(v->background)) {
        ui_gdi.fill_with(v->x, v->y, v->w, v->h, v->background);
    }
}

static void ui_app_view_layout(void) {
    not_null(ui_app.window);
    not_null(ui_app.canvas);
    ui_app.root->w = ui_app.crc.w; // crc is window client rectangle
    ui_app.root->h = ui_app.crc.h;
    ui_app_measure_and_layout(ui_app.root);
}

static void ui_app_view_active_frame_paint(void) {
    ui_color_t c = ui_app.is_active() ?
        ui_app.get_color(ui_color_id_highlight) : // ui_colors.btn_hover_highlight
        ui_app.get_color(ui_color_id_inactive_title);
    ui_gdi.frame_with(0, 0, ui_app.root->w - 0, ui_app.root->h - 0, c);
}

static void ui_app_paint_stats(void) {
    if (ui_app.paint_count % 128 == 0) { ui_app.paint_max = 0; }
    ui_app.paint_time = ut_clock.seconds() - ui_app.now;
    ui_app.paint_max = ut_max(ui_app.paint_time, ui_app.paint_max);
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
}

static void ui_app_paint_on_canvas(HDC hdc) {
    // GM_ADVANCED: rectangles are right bottom inclusive
    // TrueType fonts and arcs are affected by world transforms.
    if (GetGraphicsMode(hdc) != GM_ADVANCED) {
        SetGraphicsMode(hdc, GM_ADVANCED); // do it once
    }
    ui_canvas_t canvas = ui_app.canvas;
    ui_app.canvas = (ui_canvas_t)hdc;
    ui_gdi.push(0, 0);
    ui_gdi.x = 0;
    ui_gdi.y = 0;
    ui_app_update_crc();
    if (ui_app_layout_dirty) {
        ui_app_view_layout();
    }
    ui_font_t  f = ui_gdi.set_font(ui_app.fonts.regular.font);
    ui_color_t c = ui_gdi.set_text_color(ui_app.get_color(ui_color_id_window_text));
    int32_t bm = SetBkMode(ui_app_canvas(), TRANSPARENT);
    int32_t stretch_mode = SetStretchBltMode(ui_app_canvas(), HALFTONE);
    ui_point_t pt = {0};
    fatal_if_false(SetBrushOrgEx(ui_app_canvas(), 0, 0, (POINT*)&pt));
    ui_brush_t br = ui_gdi.set_brush(ui_gdi.brush_hollow);
    ui_app_paint(ui_app.root);
    if (ui_app.animating.view != null) { ui_app_toast_paint(); }
    fatal_if_false(SetBrushOrgEx(ui_app_canvas(), pt.x, pt.y, null));
    SetStretchBltMode(ui_app_canvas(), stretch_mode);
    SetBkMode(ui_app_canvas(), bm);
    ui_gdi.set_brush(br);
    ui_gdi.set_text_color(c);
    ui_gdi.set_font(f);
    ui_app.paint_count++;
    if (ui_app.no_decor && !ui_app.is_full_screen && !ui_app.is_maximized()) {
        ui_app_view_active_frame_paint();
    }
    ui_gdi.pop();
    ui_app.canvas = canvas;
    ui_app_paint_stats();
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
    ui_app.root->hidden = !IsWindowVisible(ui_app_window());
    const bool moved  = (wp->flags & SWP_NOMOVE) == 0;
    const bool sized  = (wp->flags & SWP_NOSIZE) == 0;
    const bool hiding = (wp->flags & SWP_HIDEWINDOW) != 0 ||
                        (wp->x == -32000 && wp->y == -32000);
    HMONITOR monitor = MonitorFromWindow(ui_app_window(), MONITOR_DEFAULTTONULL);
    if (!ui_app.root->hidden && (moved || sized) && !hiding && monitor != null) {
        RECT wrc = ui_app_ui2rect(&ui_app.wrc);
        fatal_if_false(GetWindowRect(ui_app_window(), &wrc));
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
//      traceln("wp: 0x%08X", wp);
        // actual wp == 0x0000
        ui_theme.refresh(ui_app.window);
    } else if (wp == 0 && lp != 0 && strcmp((const char*)lp, "intl") == 0) {
        traceln("wp: 0x%04X", wp); // SPI_SETLOCALEINFO 0x24 ?
        uint16_t ln[LOCALE_NAME_MAX_LENGTH + 1];
        int32_t n = GetUserDefaultLocaleName(ln, countof(ln));
        fatal_if_false(n > 0);
        uint16_t rln[LOCALE_NAME_MAX_LENGTH + 1];
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
    #pragma push_macro("ui_set_timer")
    #pragma push_macro("ui_kill_timer")
    #pragma push_macro("ui_done")

    #define ui_set_timer(t, ms) do {                \
        assert(t == 0);                             \
        t = ui_app_timer_set((uintptr_t)&t, ms);    \
    } while (0)

    #define ui_kill_timer(t) do {                    \
        if (t != 0) { ui_app_timer_kill(t); t = 0; } \
    } while (0)

    #define ui_done(ix) do {                        \
        clicked[ix] = 0;                            \
        pressed[ix] = false;                        \
        click_at[ix] = (ui_point_t){0, 0};          \
        ui_kill_timer(timer_p[ix]);                 \
        ui_kill_timer(timer_d[ix]);                 \
    } while (0)

    // This function should work regardless to CS_BLKCLK being present
    // 0: Left, 1: Middle, 2: Right
    static ui_point_t click_at[3];
    static fp64_t     clicked[3]; // click time
    static bool       pressed[3];
    static ui_timer_t timer_d[3]; // fp64_t tap
    static ui_timer_t timer_p[3]; // long press
    bool up = false;
    int32_t ix = -1;
    uint32_t m = 0;
    switch (msg) {
        case WM_LBUTTONDOWN  : ix = 0; m = (uint32_t)ui.message.tap;  break;
        case WM_MBUTTONDOWN  : ix = 1; m = (uint32_t)ui.message.tap;  break;
        case WM_RBUTTONDOWN  : ix = 2; m = (uint32_t)ui.message.tap;  break;
        case WM_LBUTTONDBLCLK: ix = 0; m = (uint32_t)ui.message.dtap; break;
        case WM_MBUTTONDBLCLK: ix = 1; m = (uint32_t)ui.message.dtap; break;
        case WM_RBUTTONDBLCLK: ix = 2; m = (uint32_t)ui.message.dtap; break;
        case WM_LBUTTONUP    : ix = 0; up = true; break;
        case WM_MBUTTONUP    : ix = 1; up = true; break;
        case WM_RBUTTONUP    : ix = 2; up = true; break;
    }
    if (msg == WM_TIMER) { // long press && dtap
        for (int i = 0; i < 3; i++) {
            if (wp == timer_p[i]) {
                lp = MAKELONG(click_at[i].x, click_at[i].y);
                ui_app_post_message(ui.message.press, i, lp);
                ui_done(i);
            }
            if (wp == timer_d[i]) {
                lp = MAKELONG(click_at[i].x, click_at[i].y);
                ui_app_post_message(ui.message.tap, i, lp);
                ui_done(i);
            }
        }
    }
    if (ix != -1) {
        const int32_t dtap_msec = (int32_t)GetDoubleClickTime();
        const fp64_t double_click_dt = dtap_msec / 1000.0;
        const int double_click_x = GetSystemMetrics(SM_CXDOUBLECLK) / 2;
        const int double_click_y = GetSystemMetrics(SM_CYDOUBLECLK) / 2;
        ui_point_t pt = { GET_X_LPARAM(lp), GET_Y_LPARAM(lp) };
        if ((int32_t)m == ui.message.tap) {
            if (ui_app.now  - clicked[ix]  <= double_click_dt &&
                abs(pt.x - click_at[ix].x) <= double_click_x &&
                abs(pt.y - click_at[ix].y) <= double_click_y) {
                ui_app_post_message(ui.message.dtap, ix, lp);
                ui_done(ix);
            } else {
                ui_done(ix); // clear timers
                clicked[ix]  = ui_app.now;
                click_at[ix] = pt;
                pressed[ix]  = true;
                ui_set_timer(timer_p[ix], ui_long_press_msec); // 0.25s
                ui_set_timer(timer_d[ix], dtap_msec); // 0.5s
            }
        } else if (up) {
//          traceln("pressed[%d]: %d %.3f", ix, pressed[ix], ui_app.now - clicked[ix]);
            if (pressed[ix] && ui_app.now - clicked[ix] > double_click_dt) {
                ui_app_post_message(ui.message.dtap, ix, lp);
                ui_done(ix);
            }
            ui_kill_timer(timer_p[ix]); // long press is no the case
        } else if ((int32_t)m == ui.message.dtap) {
            ui_app_post_message(ui.message.dtap, ix, lp);
            ui_done(ix);
        }
    }
    #pragma pop_macro("ui_done")
    #pragma pop_macro("ui_kill_timer")
    #pragma pop_macro("ui_set_timer")
}

static int64_t ui_app_hit_test(int32_t x, int32_t y) {
    int32_t cx = x - ui_app.wrc.x; // client coordinates
    int32_t cy = y - ui_app.wrc.y;
    if (ui_app.no_decor) {
        int32_t bt = ut_max(4, ui_app.in2px(1.0 / 16.0));
        if (ui_app.animating.view != null) {
            return ui.hit_test.client; // message box or toast is up
        } else if (ui_app.is_maximized()) {
            int64_t ht = ui_view.hit_test(ui_app.root, x, y);
            return ht == ui.hit_test.nowhere ? ui.hit_test.client : ht;
        } else if (ui_app.is_full_screen) {
            return ui.hit_test.client;
        } else if (cx < bt && cy < bt) {
            return ui.hit_test.top_left;
        } else if (cx > ui_app.crc.w - bt && cy < bt) {
            return ui.hit_test.top_right;
        } else if (cy < bt) {
            return ui.hit_test.top;
        } else if (!ui_caption.view.hidden && cy < ui_caption.view.h) {
            return ui_caption.view.hit_test(&ui_caption.view, cx, cy);
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
            // drop down to content hit test
        }
    }
    return ui_view.hit_test(ui_app.content, cx, cy);
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
    bool swapped = GetSystemMetrics(SM_SWAPBUTTON) != 0;
    ui_app.mouse_left  = (GetAsyncKeyState(swapped ? VK_RBUTTON : VK_LBUTTON)
                          & 0x8000) != 0;
    ui_app.mouse_right = (GetAsyncKeyState(swapped ? VK_LBUTTON : VK_RBUTTON)
                          & 0x8000) != 0;
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
    ui_app_update_mouse_buttons_state();
    ui_view.kill_hidden_focus(ui_app.root);
    ui_app_click_detector((uint32_t)m, (WPARAM)wp, (LPARAM)lp);
    if (ui_view.message(ui_app.root, m, wp, lp, &ret)) {
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
        case WM_SETTINGCHANGE: ui_app_setting_change((uintptr_t)wp, (uintptr_t)lp); break;
        case WM_CLOSE        : ui_app.focus = null; // before WM_CLOSING
                               ui_app_post_message(ui.message.closing, 0, 0); return 0;
        case WM_DESTROY      : PostQuitMessage(ui_app.exit_code); break;
        case WM_NCHITTEST    : {
            int64_t ht = ui_app_hit_test(GET_X_LPARAM(lp), GET_Y_LPARAM(lp));
            if (ht != ui.hit_test.nowhere) { return ht; }
            break; // drop to DefWindowProc
        }
        case WM_SYSKEYDOWN: // for ALT (aka VK_MENU)
        case WM_KEYDOWN      : ui_app_alt_ctrl_shift(true, wp);
                               ui_view.key_pressed(ui_app.root, wp);
                               break;
        case WM_SYSKEYUP:
        case WM_KEYUP        : ui_app_alt_ctrl_shift(false, wp);
                               ui_view.key_released(ui_app.root, wp);
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
        case WM_CHAR         : ui_app_wm_char(ui_app.root, (const char*)&wp);
                               break; // TODO: CreateWindowW() and utf16->utf8
        case WM_PRINTCLIENT  : ui_app_paint_on_canvas((HDC)wp); break;
        case WM_SETFOCUS     :
            if (!ui_app.root->hidden) {
                assert(GetActiveWindow() == ui_app_window());
                ui_view.set_focus(ui_app.root);
            }
            break;
        case WM_KILLFOCUS    : if (!ui_app.root->hidden) { ui_view.kill_focus(ui_app.root); }
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
        case WM_CONTEXTMENU  : (void)ui_view.context_menu(ui_app.root); break;
        case WM_MOUSEWHEEL   :
            ui_view.mouse_wheel(ui_app.root, 0, GET_WHEEL_DELTA_WPARAM(wp)); break;
        case WM_MOUSEHWHEEL  :
            ui_view.mouse_wheel(ui_app.root, GET_WHEEL_DELTA_WPARAM(wp), 0); break;
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
            ui_app_mouse(ui_app.root, m, wp);
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
            ui_app_mouse(ui_app.root, m, wp);
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
            if (ui_app_timer_1s_id != 0 && !ui_app.root->hidden) { ui_app.request_layout(); }
            // IMPORTANT: return true because otherwise linear, see
            // https://learn.microsoft.com/en-us/windows/win32/hidpi/wm-getdpiscaledsize
            return true;
        }
        case WM_DPICHANGED: {
//          traceln("WM_DPICHANGED");
            ui_app_window_dpi();
            ui_app_init_fonts(ui_app.dpi.window);
            if (ui_app_timer_1s_id != 0 && !ui_app.root->hidden) {
                ui_app.request_layout();
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
    return DefWindowProcA(ui_app_window(), (UINT)m, (WPARAM)wp, lp);
}

static long ui_app_set_window_long(int32_t index, long value) {
    ut_runtime.seterr(0);
    long r = SetWindowLongA(ui_app_window(), index, value); // r previous value
    fatal_if_not_zero(ut_runtime.err());
    return r;
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
    wc.hIcon = (HICON)ui_app.icon;
    wc.hCursor = (HCURSOR)ui_app.cursor;
    wc.hbrBackground = null;
    wc.lpszMenuName = null;
    wc.lpszClassName = ui_app.class_name;
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
    ut_str_printf(ui_caption.title.text, "%s", ui_app.title);
    not_null(GetSystemMenu(ui_app_window(), false));
    ui_app.dpi.window = (int32_t)GetDpiForWindow(ui_app_window());
    traceln("ui_app.dpi.window: %d", ui_app.dpi.window);
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
        int32_t exclude = WS_SIZEBOX;
        if (ui_app.no_min) { exclude = WS_MINIMIZEBOX; }
        if (ui_app.no_max) { exclude = WS_MAXIMIZEBOX; }
        long s = GetWindowLongA(ui_app_window(), GWL_STYLE);
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
        long s = GetWindowLong(ui_app_window(), GWL_STYLE);
        ui_app_set_window_long(GWL_STYLE, s & ~WS_SIZEBOX);
        enum { swp = SWP_FRAMECHANGED |
                     SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE };
        SetWindowPos(ui_app_window(), null, 0, 0, 0, 0, swp);
    }
    ui_theme.refresh(ui_app.window);
    if (ui_app.visibility != ui.visibility.hide) {
        ui_app.root->w = ui_app.wrc.w;
        ui_app.root->h = ui_app.wrc.h;
        AnimateWindow(ui_app_window(), 250, AW_ACTIVATE);
        ui_app.show_window(ui_app.visibility);
        ui_app_update_crc();
//      ui_app.root->w = ui_app.crc.w; // ui_app.crc is "client rectangle"
//      ui_app.root->h = ui_app.crc.h;
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
            style = GetWindowLongA(ui_app_window(), GWL_STYLE);
            // becasue WS_POPUP is defined as 0x80000000L:
            long s = (long)((uint32_t)style | WS_POPUP | WS_VISIBLE);
            s &= ~WS_OVERLAPPEDWINDOW;
            ui_app_set_window_long(GWL_STYLE, s);
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

static void ui_app_request_redraw(void) { SetEvent(ui_app_event_invalidate); } // < 2us

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
        ut_event_t es[] = { ui_app_event_invalidate, ui_app_event_quit };
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
    static ui_label_t label = ui_label(0.0, "");
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
    int r = GetConsoleScreenBufferInfoEx(console, &info) ? 0 : ut_runtime.err();
    if (r != 0) {
        traceln("GetConsoleScreenBufferInfoEx() %s", strerr(r));
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
//      if (r0 != 0) { traceln("SetConsoleWindowInfo() %s", strerr(r0)); }
        int r1 = SetConsoleScreenBufferSize(console, c) ? 0 : ut_runtime.err();
//      if (r1 != 0) { traceln("SetConsoleScreenBufferSize() %s", strerr(r1)); }
        if (r0 != 0 || r1 != 0) { // try in reverse order (which expected to work):
            r0 = SetConsoleScreenBufferSize(console, c) ? 0 : ut_runtime.err();
            if (r0 != 0) { traceln("SetConsoleScreenBufferSize() %s", strerr(r0)); }
            r1 = SetConsoleWindowInfo(console, true, &minwin) ? 0 : ut_runtime.err();
            if (r1 != 0) { traceln("SetConsoleWindowInfo() %s", strerr(r1)); }
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
    fatal_if_not_zero(r, "GetConsoleMode() %s", strerr(r));
    mode &= ~ENABLE_AUTO_POSITION;
    r = SetConsoleMode(console, &mode) ? 0 : ut_runtime.err();
    fatal_if_not_zero(r, "SetConsoleMode() %s", strerr(r));
    */
    CONSOLE_SCREEN_BUFFER_INFOEX info = { sizeof(CONSOLE_SCREEN_BUFFER_INFOEX) };
    int r = GetConsoleScreenBufferInfoEx(console, &info) ? 0 : ut_runtime.err();
    fatal_if_not_zero(r, "GetConsoleScreenBufferInfoEx() %s", strerr(r));
    COORD c = GetLargestConsoleWindowSize(console);
    if (c.X > 80) { c.X &= ~0x7; }
    if (c.Y > 24) { c.Y &= ~0x3; }
    if (c.X > 80) { c.X -= 8; }
    if (c.Y > 24) { c.Y -= 4; }
    ui_app_set_console_size(c.X, c.Y);
    r = GetConsoleScreenBufferInfoEx(console, &info) ? 0 : ut_runtime.err();
    fatal_if_not_zero(r, "GetConsoleScreenBufferInfoEx() %s", strerr(r));
    info.dwSize.Y = 9999; // maximum value at the moment of implementation
    r = SetConsoleScreenBufferInfoEx(console, &info) ? 0 : ut_runtime.err();
    fatal_if_not_zero(r, "SetConsoleScreenBufferInfoEx() %s", strerr(r));
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
    ut_str_printf(ui_caption.title.text, "%s", title);
    fatal_if_false(SetWindowTextA(ui_app_window(), title));
    if (!ui_caption.view.hidden) { ui_app.request_layout(); }
}

static ui_color_t ui_app_get_color(int32_t color_id) {
    return ui_theme.get_color(color_id); // SysGetColor() does not work on Win10
}

static void ui_app_capture_mouse(bool on) {
    static int32_t mouse_capture;
    if (on) {
        swear(mouse_capture == 0);
        mouse_capture++;
        SetCapture(ui_app_window());
    } else {
        swear(mouse_capture == 1);
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
    ut_str_printf(title, "%s - Console", text);
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

static fp32_t ui_app_px2in(int32_t pixels) {
    assert(ui_app.dpi.monitor_raw > 0);
    return ui_app.dpi.monitor_raw > 0 ?
           (fp32_t)pixels / (fp32_t)ui_app.dpi.monitor_raw : 0;
}

static int32_t ui_app_in2px(fp32_t inches) {
    assert(ui_app.dpi.monitor_raw > 0);
    return (int32_t)(inches * (fp32_t)ui_app.dpi.monitor_raw + 0.5f);
}

static void ui_app_request_layout(void) {
    ui_app_layout_dirty = true;
    ui_app.request_redraw();
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

static ut_file_name_t ui_app_open_file_dialog(const char* folder,
        const char* pairs[], int32_t n) {
    assert(pairs == null && n == 0 ||
           n >= 2 && n % 2 == 0);
    uint16_t memory[4 * 1024];
    uint16_t* filter = memory;
    if (pairs == null || n == 0) {
        filter = L"All Files\0*\0\0";
    } else {
        int32_t left = countof(memory) - 2;
        uint16_t* s = memory;
        for (int32_t i = 0; i < n; i+= 2) {
            uint16_t* s0 = s;
            ut_str.utf8to16(s0, left, pairs[i + 0]);
            int32_t n0 = (int32_t)ut_str.len16(s0);
            assert(n0 > 0);
            s += n0 + 1;
            left -= n0 + 1;
            uint16_t* s1 = s;
            ut_str.utf8to16(s1, left, pairs[i + 0]);
            int32_t n1 = (int32_t)ut_str.len16(s1);
            assert(n1 > 0);
            s[n1] = 0;
            s += n1 + 1;
            left -= n1 + 1;
        }
        *s++ = 0;
    }
    uint16_t dir[MAX_PATH];
    ut_str.utf8to16(dir, countof(dir), folder);
    uint16_t path[MAX_PATH];
    path[0] = 0;
    OPENFILENAMEW ofn = { sizeof(ofn) };
    ofn.hwndOwner = (HWND)ui_app.window;
    ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;
    ofn.lpstrFilter = filter;
    ofn.lpstrInitialDir = dir;
    ofn.lpstrFile = path;
    ofn.nMaxFile = sizeof(path);
    ut_file_name_t fn;
    if (GetOpenFileNameW(&ofn) && path[0] != 0) {
        ut_str.utf16to8(fn.s, countof(fn.s), path);
    } else {
        fn.s[0] = 0;
    }
    return fn;
}

// TODO: use clipboard instead?

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
    errno_t r = b2e(OpenClipboard(GetDesktopWindow()));
    if (r != 0) { traceln("OpenClipboard() failed %s", strerr(r)); }
    if (r == 0) {
        r = b2e(EmptyClipboard());
        if (r != 0) { traceln("EmptyClipboard() failed %s", strerr(r)); }
    }
    if (r == 0) {
        r = b2e(SetClipboardData(CF_BITMAP, bitmap));
        if (r != 0) {
            traceln("SetClipboardData() failed %s", strerr(r));
        }
    }
    if (r == 0) {
        r = b2e(CloseClipboard());
        if (r != 0) {
            traceln("CloseClipboard() failed %s", strerr(r));
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

static ui_view_t ui_app_view = ui_view(list);
static ui_view_t ui_app_content = ui_view(container);

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
    ui_app_event_quit           = ut_event.create();
    ui_app_event_invalidate     = ut_event.create();
    ui_app.request_redraw       = ui_app_request_redraw;
    ui_app.draw                 = ui_app_draw;
    ui_app.px2in                = ui_app_px2in;
    ui_app.in2px                = ui_app_in2px;
    ui_app.set_layered_window   = ui_app_set_layered_window;
    ui_app.is_active            = ui_app_is_active;
    ui_app.is_minimized         = ui_app_is_minimized;
    ui_app.is_maximized         = ui_app_is_maximized;
    ui_app.has_focus            = ui_app_has_focus;
    ui_app.request_focus        = ui_app_request_focus;
    ui_app.activate             = ui_app_activate;
    ui_app.get_color            = ui_app_get_color;
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
    ui_app.post                 = ui_app_post_message;
    ui_app.show_window          = ui_app_show_window;
    ui_app.show_toast           = ui_app_show_toast;
    ui_app.show_tooltip         = ui_app_show_tooltip;
    ui_app.toast_va             = ui_app_formatted_toast_va;
    ui_app.toast                = ui_app_formatted_toast;
    ui_app.create_caret         = ui_app_create_caret;
    ui_app.show_caret           = ui_app_show_caret;
    ui_app.move_caret           = ui_app_move_caret;
    ui_app.hide_caret           = ui_app_hide_caret;
    ui_app.destroy_caret        = ui_app_destroy_caret;
    ui_app.data_save            = ui_app_data_save;
    ui_app.data_size            = ui_app_data_size;
    ui_app.data_load            = ui_app_data_load;
    ui_app.open_file_dialog     = ui_app_open_file_dialog;
    ui_app.is_stdout_redirected = ui_app_is_stdout_redirected;
    ui_app.is_console_visible   = ui_app_is_console_visible;
    ui_app.console_attach       = ui_app_console_attach;
    ui_app.console_create       = ui_app_console_create;
    ui_app.console_show         = ui_app_console_show;
    ui_app.root    = &ui_app_view;
    ui_app.content = &ui_app_content;
    ui_app.caption = &ui_caption.view;
    ui_view.add(ui_app.root, ui_app.caption, ui_app.content, null);
    ui_view_call_init(ui_app.root); // to get done with container_init()
    assert(ui_app.content->type == ui_view_container);
    assert(ui_app.content->background == ui_colors.transparent);
    ui_app.root->color_id = ui_color_id_window_text;
    ui_app.root->background_id = ui_color_id_window;
    ui_app.root->insets = (ui_gaps_t){ 0, 0, 0, 0 };
    ui_app.root->paint = ui_app_view_paint;
    ui_app.root->max_w = ui.infinity;
    ui_app.root->max_h = ui.infinity;
    ui_app.content->insets = (ui_gaps_t){ 0, 0, 0, 0 };
    ui_app.content->max_w = ui.infinity;
    ui_app.content->max_h = ui.infinity;
    ui_app.caption->hidden = !ui_app.no_decor;
    // for ui_view_debug_paint:
    ut_str_printf(ui_app.root->text, "ui_app.root");
    ut_str_printf(ui_app.content->text, "ui_app.content");
    ui_app.init();
}

static void ui_app_init_windows(void) {
    fatal_if_not_zero(SetProcessDpiAwareness(PROCESS_PER_MONITOR_DPI_AWARE));
    not_null(SetThreadDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2));
    InitCommonControls(); // otherwise GetOpenFileName does not work
    ui_app.dpi.process = (int32_t)GetSystemDpiForProcess(GetCurrentProcess());
    ui_app.dpi.system = (int32_t)GetDpiForSystem(); // default was 96DPI
    // monitor dpi will be reinitialized in load_window_pos
    ui_app.dpi.monitor_effective = ui_app.dpi.system;
    ui_app.dpi.monitor_angular = ui_app.dpi.system;
    ui_app.dpi.monitor_raw = ui_app.dpi.system;
    static const RECT nowhere = {0x7FFFFFFF, 0x7FFFFFFF, 0x7FFFFFFF, 0x7FFFFFFF};
    ui_rect_t r = ui_app_rect2ui(&nowhere);
    ui_app_update_mi(&r, MONITOR_DEFAULTTOPRIMARY);
    ui_app.dpi.window = ui_app.dpi.monitor_effective;
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
    // TODO: use size_frame and caption_height in ui_caption.c
    int32_t size_frame = (int32_t)GetSystemMetricsForDpi(SM_CXSIZEFRAME, (uint32_t)ui_app.dpi.process);
    int32_t caption_height = (int32_t)GetSystemMetricsForDpi(SM_CYCAPTION, (uint32_t)ui_app.dpi.process);
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
    ui_app.root->hidden = true; // start with ui hidden
    ui_app.root->fm = &ui_app.fonts.regular;
    ui_app.root->w = wr.w - size_frame * 2;
    ui_app.root->h = wr.h - size_frame * 2 - caption_height;
    ui_app_layout_dirty = true; // layout will be done before first paint
    not_null(ui_app.class_name);
    if (!ui_app.no_ui) {
        ui_app_create_window(wr);
        ui_app_init_fonts(ui_app.dpi.window);
        ut_thread_t thread = ut_thread.start(ui_app_redraw_thread, null);
        r = ui_app_message_loop();
        // ui_app.fini() must be called before ui_app_dispose()
        if (ui_app.fini != null) { ui_app.fini(); }
        fatal_if_false(SetEvent(ui_app_event_quit));
        ut_thread.join(thread, -1);
        ui_app_dispose();
        if (r == 0 && ui_app.exit_code != 0) { r = ui_app.exit_code; }
    } else {
        r = ui_app.main();
        if (ui_app.fini != null) { ui_app.fini(); }
    }
    return r;
}

#pragma warning(disable: 28251) // inconsistent annotations

int WINAPI WinMain(HINSTANCE instance, HINSTANCE unused(previous),
        char* unused(command), int show) {
    ui_app.tid = ut_thread.id();
    const COINIT co_init = COINIT_MULTITHREADED | COINIT_SPEED_OVER_MEMORY;
    fatal_if_not_zero(CoInitializeEx(0, co_init));
    SetConsoleCP(CP_UTF8);
    ut_nls.init();
    ui_app.visibility = show;
    ut_args.WinMain();
    // IDI_ICON 101:
    ui_app.icon = (ui_icon_t)LoadIconA(instance, MAKEINTRESOURCE(101));
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

#pragma pop_macro("ui_monospaced_font")
#pragma pop_macro("ui_app_canvas")
#pragma pop_macro("ui_app_window")

#pragma comment(lib, "comctl32")
#pragma comment(lib, "comdlg32")
#pragma comment(lib, "dwmapi")
#pragma comment(lib, "gdi32")
#pragma comment(lib, "msimg32")
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
        ui_color_t c = ui_app.get_color(ui_color_id_button_face);
        // 0x1E1E1E
        // 0x333333 button face
        ui_color_t d0 = ui_colors.darken(c, 0.50f);
        return d0;
//      traceln("ui_color_id_button_face: 0x%06X", c);
//      traceln("ui_colors.btn_gradient_darker: 0x%06X", ui_colors.btn_gradient_darker);
//      return ui_colors.btn_gradient_darker;
    } else {
        ui_color_t c = ui_app.get_color(ui_color_id_button_face);
        uint32_t r = ui_color_r(c);
        uint32_t g = ui_color_r(c);
        uint32_t b = ui_color_r(c);
        r = ut_max(0, ut_min(0xFF, (uint32_t)(r * 0.75)));
        g = ut_max(0, ut_min(0xFF, (uint32_t)(g * 0.75)));
        b = ut_max(0, ut_min(0xFF, (uint32_t)(b * 0.75)));
        ui_color_t d = ui_rgb(r, g, b);
        traceln("c: 0x%06X -> 0x%06X", c, d);
        return d;
    }
}

static ui_color_t ui_button_gradient_dark(void) {
    if (ui_theme.are_apps_dark()) {
        // 0x302D2D
        ui_color_t c = ui_app.get_color(ui_color_id_button_face);
        ui_color_t d1 = ui_colors.darken(c, 0.125f);
        return d1;
//      traceln("ui_colors.btn_gradient_dark: 0x%06X", ui_colors.btn_gradient_dark);
//      return ui_colors.btn_gradient_dark;
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
    ui_gdi.push(v->x, v->y);
    bool pressed = (v->armed ^ v->pressed) == 0;
    if (v->armed_until != 0) { pressed = true; }
    int32_t sign = 1 - pressed * 2; // -1, +1
    int32_t w = sign * (v->w - 2);
    int32_t h = sign * (v->h - 2);
    int32_t x = (v->x + (int32_t)pressed * v->w) + sign;
    int32_t y = (v->y + (int32_t)pressed * v->h) + sign;
    fp32_t d = ui_theme.are_apps_dark() ? 0.50f : 0.25f;
    ui_color_t d0 = ui_colors.darken(v->background, d);
    d /= 4;
    ui_color_t d1 = ui_colors.darken(v->background, d);
    if (!v->flat || v->hover) {
        ui_gdi.gradient(x, y, w, h, d0, d1, true);
    }
    ui_color_t c = v->color;
    if (!v->flat && v->armed) {
        c = ui_colors.btn_armed;
    }else if (!v->flat && v->hover && !v->armed) {
        c = ui_app.get_color(ui_color_id_hot_tracking_color);
    }
    if (v->disabled) { c = ui_app.get_color(ui_color_id_gray_text); }
    if (v->icon == null) {
        ui_font_t f = v->fm->font;
        ui_wh_t m = ui_gdi.measure_text(f, ui_view.nls(v));
        ui_gdi.set_text_color(c);
        ui_gdi.x = v->x + (v->w - m.w) / 2;
        ui_gdi.y = v->y + (v->h - m.h) / 2;
        f = ui_gdi.set_font(f);
        ui_gdi.text("%s", ui_view.nls(v));
        ui_gdi.set_font(f);
    } else {
        ui_gdi.draw_icon(v->x, v->y, v->w, v->h, v->icon);
    }
    const int32_t pw = ut_max(1, v->fm->em.h / 8); // pen width
    ui_color_t color = v->armed ? ui_colors.lighten(v->background, 0.125f) : d1;
    if (v->hover && !v->armed) { color = ui_colors.blue; }
    if (v->disabled) { color = ui_colors.dkgray1; }
    if (!v->flat) {
        ui_pen_t p = ui_gdi.create_pen(color, pw);
        ui_gdi.set_pen(p);
        ui_gdi.set_brush(ui_gdi.brush_hollow);
        int32_t r = ut_max(3, v->fm->em.h / 4);
        if (r % 2 == 0) { r++; }
        ui_gdi.rounded(v->x, v->y, v->w, v->h, r, r);
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

static void ui_button_measured(ui_view_t* v) {
    assert(v->type == ui_view_button || v->type == ui_view_label);
    if (v->w < v->h) { v->w = v->h; } // make square is narrow letter like "I"
}

void ui_view_init_button(ui_view_t* v) {
    assert(v->type == ui_view_button);
    v->mouse         = ui_button_mouse;
    v->measured      = ui_button_measured;
    v->paint         = ui_button_paint;
    v->character     = ui_button_character;
    v->every_100ms   = ui_button_every_100ms;
    v->key_pressed   = ui_button_key_pressed;
    v->color_id      = ui_color_id_window_text;
    v->background_id = ui_color_id_button_face;
    ui_view.set_text(v, v->text);
    ui_view.localize(v);
}

void ui_button_init(ui_button_t* b, const char* label, fp32_t ems,
        void (*callback)(ui_button_t* b)) {
    b->type = ui_view_button;
    ut_str_printf(b->text, "%s", label);
    b->callback = callback;
    b->min_w_em = ems;
    ui_view_init_button(b);
}
// _______________________________ ui_caption.c _______________________________

/* Copyright (c) Dmitry "Leo" Kuznetsov 2021-24 see LICENSE for details */
#include "ut/ut.h"

#pragma push_macro("ui_caption_glyph_rest")
#pragma push_macro("ui_caption_glyph_menu")
#pragma push_macro("ui_caption_glyph_mini")
#pragma push_macro("ui_caption_glyph_maxi")
#pragma push_macro("ui_caption_glyph_full")
#pragma push_macro("ui_caption_glyph_quit")

#define ui_caption_glyph_rest ut_glyph_two_joined_squares
#define ui_caption_glyph_menu ut_glyph_trigram_for_heaven
#define ui_caption_glyph_mini ut_glyph_heavy_minus_sign
#define ui_caption_glyph_maxi ut_glyph_white_large_square
#define ui_caption_glyph_full ut_glyph_square_four_corners
#define ui_caption_glyph_quit ut_glyph_n_ary_times_operator

static void ui_caption_toggle_full(void) {
    ui_app.full_screen(!ui_app.is_full_screen);
    ui_caption.view.hidden = ui_app.is_full_screen;
    ui_app.request_layout();
}

static void ui_caption_esc_full_screen(ui_view_t* v, const char utf8[]) {
    swear(v == ui_caption.view.parent);
    // TODO: inside ui_app.c instead of here?
    if (utf8[0] == 033 && ui_app.is_full_screen) { ui_caption_toggle_full(); }
}

static void ui_caption_quit(ui_button_t* unused(b)) {
    ui_app.close();
}

static void ui_caption_mini(ui_button_t* unused(b)) {
    ui_app.show_window(ui.visibility.minimize);
}

static void ui_caption_maximize_or_restore(void) {
    ut_str_printf(ui_caption.maxi.text, "%s",
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

static int64_t ui_caption_hit_test(ui_view_t* v, int32_t x, int32_t y) {
    swear(v == &ui_caption.view);
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

static void ui_caption_prepare(ui_view_t* unused(v)) {
    ui_caption.title.hidden = false;
}

static void ui_caption_measured(ui_view_t* v) {
    ui_caption.title.hidden = v->w > ui_app.crc.w;
    v->w = ui_app.crc.w;
}

static void ui_caption_composed(ui_view_t* v) {
    v->x = 0;
}

static void ui_caption_paint(ui_view_t* v) {
    ui_color_t background = ui_caption_color();
    ui_gdi.fill_with(v->x, v->y, v->w, v->h, background);
}

static void ui_caption_init(ui_view_t* v) {
    swear(v == &ui_caption.view, "caption is a singleton");
    ui_view_init_span(v);
    ui_caption.view.insets = (ui_gaps_t){ 0, 0, 0, 0 };
    ui_caption.view.hidden = false;
    v->parent->character = ui_caption_esc_full_screen; // ESC for full screen
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
    ui_caption.view.color_id = ui_color_id_window_text;
    static const ui_gaps_t p = { .left  = 0.25, .top    = 0.25,
                                 .right = 0.25, .bottom = 0.25};
    ui_view_for_each(&ui_caption.view, c, {
        c->fm = &ui_app.fonts.H3;
        c->color_id = ui_caption.view.color_id;
        c->flat = true;
        c->padding = p;
    });
    ui_caption.view.insets = (ui_gaps_t) {
        .left  = 0.75,  .top    = 0.125,
        .right = 0.75,  .bottom = 0.125
    };
    ui_caption.icon.icon  = ui_app.icon;
    ui_caption.view.align = ui.align.left;
    // TODO: this does not help because parent layout will set x and w again
    ui_caption.view.prepare = ui_caption_prepare;
    ui_caption.view.measured  = ui_caption_measured;
    ui_caption.view.composed   = ui_caption_composed;
    ut_str_printf(ui_caption.view.text, "ui_caption");
    ui_caption_maximize_or_restore();
    ui_caption.view.paint = ui_caption_paint;
}

ui_caption_t ui_caption =  {
    .view = {
        .type     = ui_view_span,
        .fm       = &ui_app.fonts.regular,
        .init     = ui_caption_init,
        .hit_test = ui_caption_hit_test,
        .hidden = true
    },
    .icon   = ui_button(ut_glyph_nbsp, 0.0, null),
    .title  = ui_label(0, ""),
    .spacer = ui_view(spacer),
    .menu   = ui_button(ui_caption_glyph_menu, 0.0, null),
    .mini   = ui_button(ui_caption_glyph_mini, 0.0, ui_caption_mini),
    .maxi   = ui_button(ui_caption_glyph_maxi, 0.0, ui_caption_maxi),
    .full   = ui_button(ui_caption_glyph_full, 0.0, ui_caption_full),
    .quit   = ui_button(ui_caption_glyph_quit, 0.0, ui_caption_quit),
};

#pragma pop_macro("ui_caption_glyph_rest")
#pragma pop_macro("ui_caption_glyph_menu")
#pragma pop_macro("ui_caption_glyph_mini")
#pragma pop_macro("ui_caption_glyph_maxi")
#pragma pop_macro("ui_caption_glyph_full")
#pragma pop_macro("ui_caption_glyph_quit")
// _______________________________ ui_colors.c ________________________________

#include "ut/ut.h"

static inline uint8_t ui_color_clamp_uint8(fp64_t value) {
    return value < 0 ? 0 : (value > 255 ? 255 : (uint8_t)value);
}

static inline fp64_t ui_color_fp64_min(fp64_t x, fp64_t y) { return x < y ? x : y; }

static inline fp64_t ui_color_fp64_max(fp64_t x, fp64_t y) { return x > y ? x : y; }

static void ui_color_rgb_to_hsi(fp64_t r, fp64_t g, fp64_t b, fp64_t *h, fp64_t *s, fp64_t *i) {
    r /= 255.0;
    g /= 255.0;
    b /= 255.0;
    fp64_t min_val = ui_color_fp64_min(r, ui_color_fp64_min(g, b));
    *i = (r + g + b) / 3;
    fp64_t chroma = ui_color_fp64_max(r, ui_color_fp64_max(g, b)) - min_val;
    if (chroma == 0) {
        *h = 0;
        *s = 0;
    } else {
        *s = 1 - min_val / *i;
        if (*i > 0) { *s = chroma / (*i * 3); }
        if (r == ui_color_fp64_max(r, ui_color_fp64_max(g, b))) {
            *h = (g - b) / chroma + (g < b ? 6 : 0);
        } else if (g == ui_color_fp64_max(r, ui_color_fp64_max(g, b))) {
            *h = (b - r) / chroma + 2;
        } else {
            *h = (r - g) / chroma + 4;
        }
        *h *= 60;
    }
}

static ui_color_t ui_color_hsi_to_rgb(fp64_t h, fp64_t s, fp64_t i, uint8_t a) {
    h /= 60.0;
    fp64_t f = h - (int32_t)h;
    fp64_t p = i * (1 - s);
    fp64_t q = i * (1 - s * f);
    fp64_t t = i * (1 - s * (1 - f));
    fp64_t r = 0, g = 0, b = 0;
    switch ((int32_t)h) {
        case 0:
        case 6: r = i * 255; g = t * 255; b = p * 255; break;
        case 1: r = q * 255; g = i * 255; b = p * 255; break;
        case 2: r = p * 255; g = i * 255; b = t * 255; break;
        case 3: r = p * 255; g = q * 255; b = i * 255; break;
        case 4: r = t * 255; g = p * 255; b = i * 255; break;
        case 5: r = i * 255; g = p * 255; b = q * 255; break;
        default: swear(false); break;
    }
    assert(0 <= r && r <= 255);
    assert(0 <= g && g <= 255);
    assert(0 <= b && b <= 255);
    return ui_rgba((uint8_t)r, (uint8_t)g, (uint8_t)b, a);
}

static ui_color_t ui_color_brightness(ui_color_t c, fp32_t multiplier) {
    fp64_t h, s, i;
    ui_color_rgb_to_hsi(ui_color_r(c), ui_color_g(c), ui_color_b(c), &h, &s, &i);
    i = ui_color_fp64_max(0, ui_color_fp64_min(1, i * (fp64_t)multiplier));
    return ui_color_hsi_to_rgb(h, s, i, ui_color_a(c));
}

static ui_color_t ui_color_saturation(ui_color_t c, fp32_t multiplier) {
    fp64_t h, s, i;
    ui_color_rgb_to_hsi(ui_color_r(c), ui_color_g(c), ui_color_b(c), &h, &s, &i);
    s = ui_color_fp64_max(0, ui_color_fp64_min(1, s * (fp64_t)multiplier));
    return ui_color_hsi_to_rgb(h, s, i, ui_color_a(c));
}

// Using the ui_color_interpolate function to blend colors toward
// black or white can effectively adjust brightness and saturation,
// offering more flexibility  and potentially better results in
// terms of visual transitions between colors.

static ui_color_t ui_color_interpolate(ui_color_t c0, ui_color_t c1,
        fp32_t multiplier) {
    assert(0.0f < multiplier && multiplier < 1.0f);
    fp64_t h0, s0, i0, h1, s1, i1;
    ui_color_rgb_to_hsi(ui_color_r(c0), ui_color_g(c0), ui_color_b(c0),
                       &h0, &s0, &i0);
    ui_color_rgb_to_hsi(ui_color_r(c1), ui_color_g(c1), ui_color_b(c1),
                       &h1, &s1, &i1);
    fp64_t h = h0 + (h1 - h0) * (fp64_t)multiplier;
    fp64_t s = s0 + (s1 - s0) * (fp64_t)multiplier;
    fp64_t i = i0 + (i1 - i0) * (fp64_t)multiplier;
    // Interpolate alphas only if differ
    uint8_t a0 = ui_color_a(c0);
    uint8_t a1 = ui_color_a(c1);
    uint8_t a = a0 == a1 ? a0 : ui_color_clamp_uint8(a0 + (a1 - a0) * (fp64_t)multiplier);
    return ui_color_hsi_to_rgb(h, s, i, a);
}

// Helper to get a neutral gray with the same intensity

static ui_color_t ui_color_gray_with_same_intensity(ui_color_t c) {
    uint8_t intensity = (ui_color_r(c) + ui_color_g(c) + ui_color_b(c)) / 3;
    return ui_rgba(intensity, intensity, intensity, ui_color_a(c));
}

// Adjust brightness by interpolating towards black or white
// using interpolation:
//
// To darken the color: Interpolate between
// the color and black (rgba(0,0,0,255)).
//
// To lighten the color: Interpolate between
// the color and white (rgba(255,255,255,255)).
//
// This approach allows you to manipulate the
// brightness by specifying how close the color
// should be to either black or white,
// providing a smooth transition.

static ui_color_t ui_color_adjust_brightness(ui_color_t c,
        fp32_t multiplier, bool lighten) {
    ui_color_t target = lighten ?
        ui_rgba(255, 255, 255, ui_color_a(c)) :
        ui_rgba(  0,   0,   0, ui_color_a(c));
    return ui_color_interpolate(c, target, multiplier);
}

static ui_color_t ui_color_lighten(ui_color_t c, fp32_t multiplier) {
    const ui_color_t target = ui_rgba(255, 255, 255, ui_color_a(c));
    return ui_color_interpolate(c, target, multiplier);
}
static ui_color_t ui_color_darken(ui_color_t c, fp32_t multiplier) {
    const ui_color_t target = ui_rgba(0, 0, 0, ui_color_a(c));
    return ui_color_interpolate(c, target, multiplier);
}

// Adjust saturation by interpolating towards a gray of the same intensity
//
// To adjust saturation, the approach is similar but slightly
// more nuanced because saturation involves both the color's
// purity and its brightness:

static ui_color_t ui_color_adjust_saturation(ui_color_t c,
        fp32_t multiplier) {
    ui_color_t gray = ui_color_gray_with_same_intensity(c);
    return ui_color_interpolate(c, gray, 1 - multiplier);
}

enum { // TODO: get rid of it?
    ui_colors_white     = ui_rgb(255, 255, 255),
    ui_colors_off_white = ui_rgb(192, 192, 192),
    ui_colors_dkgray0   = ui_rgb(16, 16, 16),
    ui_colors_dkgray1   = ui_rgb(30, 30, 30),
    ui_colors_dkgray2   = ui_rgb(37, 38, 38),
    ui_colors_dkgray3   = ui_rgb(45, 45, 48),
    ui_colors_dkgray4   = ui_rgb(63, 63, 70),
    ui_colors_blue_highlight = ui_rgb(128, 128, 255)
};

ui_colors_if ui_colors = {
    .rgb_to_hsi               = ui_color_rgb_to_hsi,
    .hsi_to_rgb               = ui_color_hsi_to_rgb,
    .interpolate              = ui_color_interpolate,
    .gray_with_same_intensity = ui_color_gray_with_same_intensity,
    .lighten                  = ui_color_lighten,
    .darken                   = ui_color_darken,
    .adjust_saturation        = ui_color_adjust_saturation,
    .multiply_brightness      = ui_color_brightness,
    .multiply_saturation      = ui_color_saturation,
    .transparent      = ui_color_transparent,
    .none             = (ui_color_t)0xFFFFFFFFU, // aka CLR_INVALID in wingdi
    .text             = ui_rgb(240, 231, 220),
    .white            = ui_colors_white,
    .black            = ui_rgb(0,     0,   0),
    .red              = ui_rgb(255,   0,   0),
    .green            = ui_rgb(0,   255,   0),
    .blue             = ui_rgb(0,   0,   255),
    .yellow           = ui_rgb(255, 255,   0),
    .cyan             = ui_rgb(0,   255, 255),
    .magenta          = ui_rgb(255,   0, 255),
    .gray             = ui_rgb(128, 128, 128),
    .dkgray1          = ui_colors_dkgray1,
    .dkgray2          = ui_colors_dkgray2,
    .dkgray3          = ui_colors_dkgray3,
    .dkgray4          = ui_colors_dkgray4,
    // tone down RGB colors:
    .tone_white       = ui_rgb(164, 164, 164),
    .tone_red         = ui_rgb(192,  64,  64),
    .tone_green       = ui_rgb(64,  192,  64),
    .tone_blue        = ui_rgb(64,   64, 192),
    .tone_yellow      = ui_rgb(192, 192,  64),
    .tone_cyan        = ui_rgb(64,  192, 192),
    .tone_magenta     = ui_rgb(192,  64, 192),
    // miscellaneous:
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
    .blue_highlight      = ui_colors_blue_highlight,
    .off_white           = ui_colors_off_white,

    .btn_gradient_darker = ui_colors_dkgray1,
    .btn_gradient_dark   = ui_colors_dkgray3,
    .btn_hover_highlight = ui_colors_blue_highlight,
    .btn_disabled        = ui_colors_dkgray4,
    .btn_armed           = ui_colors_white,
    .btn_text            = ui_colors_off_white,
    .toast               = ui_colors_dkgray3, // ui_rgb(8, 40, 24), // toast background

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
#pragma push_macro("ui_layout_dump")
#pragma push_macro("ui_layout_enter")
#pragma push_macro("ui_layout_exit")

// Usage of: ui_view_for_each_begin(p, c) { ... } ui_view_for_each_end(p, c)
// makes code inside iterator debugger friendly and ensures correct __LINE__

#define debugln(...) do {                                \
    if (ui_containers_debug) {  traceln(__VA_ARGS__); }  \
} while (0)

#define ui_layout_enter(v) do {                               \
    ui_ltrb_t i_ = ui_view.gaps(v, &v->insets);               \
    ui_ltrb_t p_ = ui_view.gaps(v, &v->padding);              \
    debugln(">%s %d,%d %dx%d p: %d %d %d %d  i: %d %d %d %d", \
                 v->text, v->x, v->y, v->w, v->h,             \
                 p_.left, p_.top, p_.right, p_.bottom,        \
                 i_.left, i_.top, i_.right, i_.bottom);       \
} while (0)

#define ui_layout_exit(v) do {                                   \
    debugln("<%s %d,%d %dx%d", v->text, v->x, v->y, v->w, v->h); \
} while (0)

static const char* ui_container_finite_int(int32_t v, char* text, int32_t count) {
    swear(v >= 0);
    if (v == ui.infinity) {
        ut_str.format(text, count, "%s", ut_glyph_infinity);
    } else {
        ut_str.format(text, count, "%d", v);
    }
    return text;
}

#define ui_layout_dump(v) do {                                                \
    char maxw[32];                                                            \
    char maxh[32];                                                            \
    debugln("%s[%4.4s] %d,%d %dx%d, max[%sx%s] "                              \
        "padding { %.3f %.3f %.3f %.3f } "                                    \
        "insets { %.3f %.3f %.3f %.3f } align: 0x%02X",                       \
        v->text, &v->type, v->x, v->y, v->w, v->h,                            \
        ui_container_finite_int(v->max_w, maxw, countof(maxw)),               \
        ui_container_finite_int(v->max_h, maxh, countof(maxh)),               \
        v->padding.left, v->padding.top, v->padding.right, v->padding.bottom, \
        v->insets.left, v->insets.top, v->insets.right, v->insets.bottom,     \
        v->align);                                                            \
} while (0)

static void ui_span_measure(ui_view_t* p) {
    ui_layout_enter(p);
    swear(p->type == ui_view_span, "type %4.4s 0x%08X", &p->type, p->type);
    ui_ltrb_t insets;
    ui_view.inbox(p, null, &insets);
    int32_t w = insets.left;
    int32_t h = 0;
    int32_t max_w = w;
    ui_view_for_each_begin(p, c) {
        swear(c->max_w == 0 || c->max_w >= c->w,
              "max_w: %d w: %d", c->max_w, c->w);
        if (c->hidden) {
            // nothing
        } else if (c->type == ui_view_spacer) {
            c->padding = (ui_gaps_t){ 0, 0, 0, 0 };
            c->w = 0; // layout will distribute excess here
            c->h = 0; // starts with zero
            max_w = ui.infinity; // spacer make width greedy
        } else {
            ui_rect_t cbx; // child "out" box expanded by padding
            ui_ltrb_t padding;
            ui_view.outbox(c, &cbx, &padding);
            h = ut_max(h, cbx.h);
            if (c->max_w == ui.infinity) {
                max_w = ui.infinity;
            } else if (max_w < ui.infinity && c->max_w != 0) {
                swear(c->max_w >= cbx.w, "Constraint violation: "
                        "c->max_w %d < cbx.w %d, ", c->max_w, cbx.w);
                max_w += c->max_w;
            } else if (max_w < ui.infinity) {
                swear(0 <= max_w + cbx.w &&
                      (int64_t)max_w + (int64_t)cbx.w < (int64_t)ui.infinity,
                      "max_w:%d + cbx.w:%d = %d", max_w, cbx.w, max_w + cbx.w);
                max_w += cbx.w;
            }
            w += cbx.w;
        }
    } ui_view_for_each_end(p, c);
    if (0 < max_w && max_w < ui.infinity) {
        swear(0 <= max_w + insets.right &&
              (int64_t)max_w + (int64_t)insets.right < (int64_t)ui.infinity,
             "max_w:%d + right:%d = %d", max_w, insets.right, max_w + insets.right);
        max_w += insets.right;
    }
    swear(max_w == 0 || max_w >= w, "max_w: %d w: %d", max_w, w);
//  TODO: childrens max_w is infinity does NOT mean
//        max_w of the parent is infinity? if this is correct remove
//        commented section
//  if (max_w != w) { // only if max_w differs from actual width
//      p->max_w = ut_max(max_w, p->max_w);
//  }
    if (p->hidden) {
        p->w = 0;
        p->h = 0;
    } else {
        p->w = w + insets.right;
        p->h = insets.top + h + insets.bottom;
        swear(p->max_w == 0 || p->max_w >= p->w,
              "max_w: %d is less than actual width: %d", p->max_w, p->w);
    }
    ui_layout_exit(p);
}

// after measure of the subtree is concluded the parent ui_span
// may adjust span_w wider number depending on it's own width
// and ui_span.max_w agreement

static int32_t ui_span_place_child(ui_view_t* c, ui_rect_t pbx, int32_t x) {
    ui_ltrb_t padding = ui_view.gaps(c, &c->padding);
    // setting child`s max_h to infinity means that child`s height is
    // *always* fill vertical view size of the parent
    // childs.h can exceed parent.h (vertical overflow) - is not
    // encouraged but allowed
    if (c->max_h == ui.infinity) {
        // important c->h changed, cbx.h is no longer valid
        c->h = ut_max(c->h, pbx.h - padding.top - padding.bottom);
    }
    int32_t min_y = pbx.y + padding.top;
    if ((c->align & ui.align.top) != 0) {
        assert(c->align == ui.align.top);
        c->y = min_y;
    } else if ((c->align & ui.align.bottom) != 0) {
        assert(c->align == ui.align.bottom);
        c->y = ut_max(min_y, pbx.y + pbx.h - c->h - padding.bottom);
    } else { // effective height (c->h might have been changed)
        assert(c->align == ui.align.center);
        const int32_t ch = padding.top + c->h + padding.bottom;
        c->y = ut_max(min_y, pbx.y + (pbx.h - ch) / 2 + padding.top);
    }
    c->x = x + padding.left;
    return c->x + c->w + padding.right;
}

static void ui_span_layout(ui_view_t* p) {
    debugln(">%s (%d,%d) %dx%d", p->text, p->x, p->y, p->w, p->h);
    swear(p->type == ui_view_span, "type %4.4s 0x%08X", &p->type, p->type);
    ui_rect_t pbx; // parent "in" box (sans insets)
    ui_ltrb_t insets;
    ui_view.inbox(p, &pbx, &insets);
    int32_t spacers = 0; // Number of spacers
    int32_t max_w_count = 0;
    int32_t x = p->x + insets.left;
    ui_view_for_each_begin(p, c) {
        if (c->hidden) {
            // nothing
        } else if (c->type == ui_view_spacer) {
            c->x = x;
            c->y = pbx.y;
            c->h = pbx.h;
            c->w = 0;
            spacers++;
        } else {
            x = ui_span_place_child(c, pbx, x);
            swear(c->max_w == 0 || c->max_w > c->w,
                  "max_w:%d < w:%d", c->max_w, c->w);
            if (c->max_w > 0) {
                max_w_count++;
            }
        }
    } ui_view_for_each_end(p, c);
    int32_t xw = ut_max(0, pbx.x + pbx.w - x); // excess width
    int32_t max_w_sum = 0;
    if (xw > 0 && max_w_count > 0) {
        ui_view_for_each_begin(p, c) {
            if (!c->hidden && c->type != ui_view_spacer && c->max_w > 0) {
                max_w_sum += ut_min(c->max_w, xw);
            }
        } ui_view_for_each_end(p, c);
    }
    if (xw > 0 && max_w_count > 0) {
        x = p->x + insets.left;
        int32_t k = 0;
        ui_view_for_each_begin(p, c) {
            if (!c->hidden) {
                ui_rect_t cbx; // child "out" box expanded by padding
                ui_ltrb_t padding;
                ui_view.outbox(c, &cbx, &padding);
                if (c->hidden) {
                    // nothing
                } else if (c->type == ui_view_spacer) {
                    swear(padding.left == 0 && padding.right == 0);
                } else if (c->max_w > 0) {
                    const int32_t max_w = ut_min(c->max_w, xw);
                    int64_t proportional = (xw * (int64_t)max_w) / max_w_sum;
                    assert(proportional <= (int64_t)INT32_MAX);
                    int32_t cw = (int32_t)proportional;
                    c->w = ut_min(c->max_w, c->w + cw);
                    k++;
                }
                // TODO: take into account .align of a child and adjust x
                //       depending on ui.align.left/right/center
                //       distributing excess width on the left and right of a child
                c->x = padding.left + x;
                x = c->x + padding.left + c->w + padding.right;
            }
        } ui_view_for_each_end(p, c);
        swear(k == max_w_count);
    }
    // excess width after max_w of non-spacers taken into account
    xw = ut_max(0, pbx.x + pbx.w - x);
    if (xw > 0 && spacers > 0) {
        // evenly distribute excess among spacers
        int32_t partial = xw / spacers;
        x = p->x + insets.left;
        ui_view_for_each_begin(p, c) {
            if (!c->hidden) {
                ui_rect_t cbx; // child "out" box expanded by padding
                ui_ltrb_t padding;
                ui_view.outbox(c, &cbx, &padding);
                if (c->type == ui_view_spacer) {
                    c->y = pbx.y;
                    c->w = partial;
                    c->h = pbx.h;
                    spacers--;
                }
                c->x = x + padding.left;
                x = c->x + c->w + padding.right;
            }
        } ui_view_for_each_end(p, c);
    }
    debugln("<%s (%d,%d) %dx%d", p->text, p->x, p->y, p->w, p->h);
}

static void ui_list_measure(ui_view_t* p) {
    debugln(">%s (%d,%d) %dx%d", p->text, p->x, p->y, p->w, p->h);
    swear(p->type == ui_view_list, "type %4.4s 0x%08X", &p->type, p->type);
    ui_rect_t pbx; // parent "in" box (sans insets)
    ui_ltrb_t insets;
    ui_view.inbox(p, &pbx, &insets);
    int32_t max_h = insets.top;
    int32_t h = insets.top;
    int32_t w = 0;
    ui_view_for_each_begin(p, c) {
        swear(c->max_h == 0 || c->max_h >= c->h, "max_h: %d h: %d",
              c->max_h, c->h);
        if (c->hidden) {
            // nothing
        } else if (c->type == ui_view_spacer) {
            c->padding = (ui_gaps_t){ 0, 0, 0, 0 };
            c->h = 0; // layout will distribute excess here
            max_h = ui.infinity; // spacer make height greedy
        } else {
            ui_rect_t cbx; // child "out" box expanded by padding
            ui_ltrb_t padding;
            ui_view.outbox(c, &cbx, &padding);
            w = ut_max(w, cbx.w);
            if (c->max_h == ui.infinity) {
                max_h = ui.infinity;
            } else if (max_h < ui.infinity && c->max_h != 0) {
                swear(c->max_h >= cbx.h, "c->max_h:%d < cbx.h: %d",
                      c->max_h, cbx.h);
                max_h += c->max_h;
            } else if (max_h < ui.infinity) {
                swear(0 <= max_h + cbx.h &&
                      (int64_t)max_h + (int64_t)cbx.h < (int64_t)ui.infinity,
                      "max_h:%d + ch:%d = %d", max_h, cbx.h, max_h + cbx.h);
                max_h += cbx.h;
            }
            h += cbx.h;
        }
    } ui_view_for_each_end(p, c);
    if (max_h < ui.infinity) {
        swear(0 <= max_h + insets.bottom &&
              (int64_t)max_h + (int64_t)insets.bottom < (int64_t)ui.infinity,
             "max_h:%d + bottom:%d = %d",
              max_h, insets.bottom, max_h + insets.bottom);
        max_h += insets.bottom;
    }
//  TODO: childrens max_w is infinity does NOT mean
//        max_w of the parent is infinity? if this is correct remove
//        commented section
//  swear(max_h == 0 || max_h >= h, "max_h is less than actual height h");
//  if (max_h != h) { // only if max_h differs from actual height
//      p->max_h = ut_max(max_h, p->max_h);
//  }
    if (p->hidden) {
        p->w = 0;
        p->h = 0;
    } else if (p == ui_app.root) {
        // ui_app.root is special case (expanded to a window)
        // TODO: when get_min_max() start taking content into account
        //       the code below may be changed to asserts() and removed
        //       after confirming the rest of the logic
        p->w = ui_app.no_decor ? ui_app.wrc.w : ui_app.crc.w;
        p->h = ui_app.no_decor ? ui_app.wrc.h : ui_app.crc.h;
    } else {
        p->h = h + insets.bottom;
        p->w = insets.left + w + insets.right;
    }
    debugln("<%s (%d,%d) %dx%d", p->text, p->x, p->y, p->w, p->h);
}

static int32_t ui_list_place_child(ui_view_t* c, ui_rect_t pbx, int32_t y) {
    ui_ltrb_t padding = ui_view.gaps(c, &c->padding);
    // setting child`s max_w to infinity means that child`s height is
    // *always* fill vertical view size of the parent
    // childs.w can exceed parent.w (horizontal overflow) - not encouraged but allowed
    if (c->max_w == ui.infinity) {
        c->w = ut_max(c->w, pbx.w - padding.left - padding.right);
    }
    int32_t min_x = pbx.x + padding.left;
    if ((c->align & ui.align.left) != 0) {
        assert(c->align == ui.align.left);
        c->x = min_x;
    } else if ((c->align & ui.align.right) != 0) {
        assert(c->align == ui.align.right);
        c->x = ut_max(min_x, pbx.x + pbx.w - c->w - padding.right);
    } else {
        assert(c->align == ui.align.center);
        const int32_t cw = padding.left + c->w + padding.right;
        c->x = ut_max(min_x, pbx.x + (pbx.w - cw) / 2 + padding.left);
    }
    c->y = y + padding.top;
    return c->y + c->h + padding.bottom;
}

static void ui_list_layout(ui_view_t* p) {
    debugln(">%s (%d,%d) %dx%d", p->text, p->x, p->y, p->w, p->h);
    swear(p->type == ui_view_list, "type %4.4s 0x%08X", &p->type, p->type);
    ui_rect_t pbx; // parent "in" box (sans insets)
    ui_ltrb_t insets;
    ui_view.inbox(p, &pbx, &insets);
    int32_t spacers = 0; // Number of spacers
    int32_t max_h_sum = 0;
    int32_t max_h_count = 0;
    int32_t y = pbx.y;
    ui_view_for_each_begin(p, c) {
        if (c->hidden) {
            // nothing
        } else if (c->type == ui_view_spacer) {
            c->x = pbx.x;
            c->y = y;
            c->w = pbx.w;
            c->h = 0;
            spacers++;
        } else {
            y = ui_list_place_child(c, pbx, y);
            swear(c->max_h == 0 || c->max_h > c->h,
                  "max_h:%d < h:%d", c->max_h, c->h);
            if (c->max_h > 0) {
                // clamp max_h to the effective parent height
                max_h_count++;
            }
        }
    } ui_view_for_each_end(p, c);
    int32_t xh = ut_max(0, pbx.y + pbx.h - y); // excess height
    if (xh > 0 && max_h_count > 0) {
        ui_view_for_each_begin(p, c) {
            if (!c->hidden && c->type != ui_view_spacer && c->max_h > 0) {
                max_h_sum += ut_min(c->max_h, xh);
            }
        } ui_view_for_each_end(p, c);
    }
    if (xh > 0 && max_h_count > 0) {
        y = pbx.y;
        int32_t k = 0;
        ui_view_for_each_begin(p, c) {
            if (!c->hidden) {
                ui_rect_t cbx; // child "out" box expanded by padding
                ui_ltrb_t padding;
                ui_view.outbox(c, &cbx, &padding);
                if (c->type != ui_view_spacer && c->max_h > 0) {
                    const int32_t max_h = ut_min(c->max_h, xh);
                    int64_t proportional = (xh * (int64_t)max_h) / max_h_sum;
                    assert(proportional <= (int64_t)INT32_MAX);
                    int32_t ch = (int32_t)proportional;
                    c->h = ut_min(c->max_h, c->h + ch);
                    k++;
                }
                int32_t ch = padding.top + c->h + padding.bottom;
                c->y = y + padding.top;
                y += ch;
            }
        } ui_view_for_each_end(p, c);
        swear(k == max_h_count);
    }
    // excess height after max_h of non-spacers taken into account
    xh = ut_max(0, pbx.y + pbx.h - y); // excess height
    if (xh > 0 && spacers > 0) {
        // evenly distribute excess among spacers
        int32_t partial = xh / spacers;
        y = pbx.y;
        ui_view_for_each_begin(p, c) {
            if (!c->hidden) {
                ui_rect_t cbx; // child "out" box expanded by padding
                ui_ltrb_t padding;
                ui_view.outbox(c, &cbx, &padding);
                if (c->type == ui_view_spacer) {
                    c->x = pbx.x;
                    c->w = pbx.x + pbx.w - pbx.x;
                    c->h = partial; // TODO: xxxxx last?
                    spacers--;
                }
                int32_t ch = padding.top + c->h + padding.bottom;
                c->y = y + padding.top;
                y += ch;
            }
        } ui_view_for_each_end(p, c);
    }
    debugln("<%s (%d,%d) %dx%d", p->text, p->x, p->y, p->w, p->h);
}

static void ui_container_measure(ui_view_t* p) {
    ui_layout_enter(p);
    swear(p->type == ui_view_container, "type %4.4s 0x%08X", &p->type, p->type);
    ui_rect_t pbx; // parent "in" box (sans insets)
    ui_ltrb_t insets;
    ui_view.inbox(p, &pbx, &insets);
    // empty container minimum size:
    p->w = insets.left + insets.right;
    p->h = insets.top + insets.bottom;
    ui_view_for_each_begin(p, c) {
        if (!c->hidden) {
            ui_rect_t cbx; // child "out" box expanded by padding
            ui_ltrb_t padding;
            ui_view.outbox(c, &cbx, &padding);
            p->w = ut_max(p->w, padding.left + c->w + padding.right);
            p->h = ut_max(p->h, padding.top + c->h + padding.bottom);
        }
    } ui_view_for_each_end(p, c);
    ui_layout_exit(p);
}

static void ui_container_layout(ui_view_t* p) {
    ui_layout_enter(p);
    swear(p->type == ui_view_container, "type %4.4s 0x%08X", &p->type, p->type);
    ui_rect_t pbx; // parent "in" box (sans insets)
    ui_ltrb_t insets;
    ui_view.inbox(p, &pbx, &insets);
    ui_view_for_each_begin(p, c) {
        if (c->type != ui_view_spacer && !c->hidden) {
            ui_rect_t cbx; // child "out" box expanded by padding
            ui_ltrb_t padding;
            ui_view.outbox(c, &cbx, &padding);
            const int32_t pw = p->w - insets.left - insets.right - padding.left - padding.right;
            const int32_t ph = p->h - insets.top - insets.bottom - padding.top - padding.bottom;
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
                   "align: left|right 0x%02X", c->align);
            swear((c->align & (ui.align.top|ui.align.bottom)) !=
                               (ui.align.top|ui.align.bottom),
                   "align: top|bottom 0x%02X", c->align);
            int32_t min_x = pbx.x + padding.left;
            if ((c->align & ui.align.left) != 0) {
                c->x = min_x;
            } else if ((c->align & ui.align.right) != 0) {
                c->x = ut_max(min_x, pbx.x + pbx.w - c->w - padding.right);
            } else {
                c->x = ut_max(min_x, min_x + (pbx.w - (padding.left + c->w + padding.right)) / 2);
            }
            int32_t min_y = pbx.y + padding.top;
            if ((c->align & ui.align.top) != 0) {
                c->y = min_y;
            } else if ((c->align & ui.align.bottom) != 0) {
                c->y = ut_max(min_y, pbx.x + pbx.h - c->h - padding.bottom);
            } else {
                c->y = ut_max(min_y, min_y + (pbx.h - (padding.top + c->h + padding.bottom)) / 2);
            }
//          debugln(" %s %d,%d %dx%d", c->text, c->x, c->y, c->w, c->h);
        }
    } ui_view_for_each_end(p, c);
    ui_layout_exit(p);
}

static void ui_paint_container(ui_view_t* v) {
    if (!ui_color_is_undefined(v->background) &&
        !ui_color_is_transparent(v->background)) {
        ui_gdi.fill_with(v->x, v->y, v->w, v->h, v->background);
    } else {
//      traceln("%s undefined", v->text);
    }
}

static void ui_view_container_init(ui_view_t* v) {
    v->background = ui_colors.transparent;
    v->insets  = (ui_gaps_t){ .left  = 0.25, .top    = 0.25,
                              .right = 0.25, .bottom = 0.25 };
}

void ui_view_init_span(ui_view_t* v) {
    swear(v->type == ui_view_span, "type %4.4s 0x%08X", &v->type, v->type);
    ui_view_container_init(v);
    if (v->measure == null) { v->measure = ui_span_measure; }
    if (v->layout  == null) { v->layout  = ui_span_layout; }
    if (v->paint   == null) { v->paint   = ui_paint_container; }
    if (v->text[0] == 0) { ut_str_printf(v->text, "ui_span"); }
}

void ui_view_init_list(ui_view_t* v) {
    swear(v->type == ui_view_list, "type %4.4s 0x%08X", &v->type, v->type);
    ui_view_container_init(v);
    if (v->measure == null) { v->measure = ui_list_measure; }
    if (v->layout  == null) { v->layout  = ui_list_layout; }
    if (v->paint   == null) { v->paint   = ui_paint_container; }
    if (v->text[0] == 0) { ut_str_printf(v->text, "ui_list"); }
}

void ui_view_init_spacer(ui_view_t* v) {
    swear(v->type == ui_view_spacer, "type %4.4s 0x%08X", &v->type, v->type);
    v->w = 0;
    v->h = 0;
    v->max_w = ui.infinity;
    v->max_h = ui.infinity;
    if (v->text[0] == 0) { ut_str_printf(v->text, "ui_spacer"); }
}

void ui_view_init_container(ui_view_t* v) {
    ui_view_container_init(v);
    if (v->measure == null) { v->measure = ui_container_measure; }
    if (v->layout  == null) { v->layout  = ui_container_layout; }
    if (v->paint   == null) { v->paint   = ui_paint_container; }
    if (v->text[0] == 0) { ut_str_printf(v->text, "ui_container"); }
}

#pragma pop_macro("ui_layout_exit")
#pragma pop_macro("ui_layout_enter")
#pragma pop_macro("ui_layout_dump")
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
    return em == 0 ? 0 : (int32_t)((fp64_t)em * (fp64_t)ratio + 0.5);
}

ui_if ui = {
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
    .point_in_rect  = ui_point_in_rect,
    .intersect_rect = ui_intersect_rect,
    .gaps_em2px     = ui_gaps_em2px
};

// https://docs.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-showwindow
// ________________________________ ui_edit.c _________________________________

/* Copyright (c) Dmitry "Leo" Kuznetsov 2021-24 see LICENSE for details */
#include "ut/ut.h"

// TODO: undo/redo
// TODO: back/forward navigation
// TODO: exit/save keyboard shortcuts?
// TODO: iBeam cursor

// http://worrydream.com/refs/Tesler%20-%20A%20Personal%20History%20of%20Modeless%20Text%20Editing%20and%20Cut-Copy-Paste.pdf
// https://web.archive.org/web/20221216044359/http://worrydream.com/refs/Tesler%20-%20A%20Personal%20History%20of%20Modeless%20Text%20Editing%20and%20Cut-Copy-Paste.pdf

// Rich text options that are not addressed yet:
// * Color of ranges (useful for code editing)
// * Soft line breaks inside the paragraph (useful for e.g. bullet lists of options)
// * Bold/Italic/Underline (along with color ranges)
// * Multiple fonts (as long as run vertical size is the maximum of font)
// * Kerning (?! like in overhung "Fl")

// When implementation and header are amalgamated
// into a single file header library name_space is
// used to separate different modules namespaces.

typedef  struct ui_edit_glyph_s {
    const char* s;
    int32_t bytes;
} ui_edit_glyph_t;

static void ui_edit_layout(ui_view_t* view);

// Glyphs in monospaced Windows fonts may have different width for non-ASCII
// characters. Thus even if edit is monospaced glyph measurements are used
// in text layout.

static uint64_t ui_edit_uint64(int32_t high, int32_t low) {
    assert(high >= 0 && low >= 0);
    return ((uint64_t)high << 32) | (uint64_t)low;
}

// TODO: 
// All allocate/free functions assume 'fail fast' semantics
// if underlying OS runs out of RAM it considered to be fatal.
// It is possible to implement and hold committed 'safety region'
// of RAM and free it to general pull or reuse it on alloc() or
// reallocate() returning null, try to notify user about low memory
// conditions and attempt to save edited work but all of the
// above may only work if there is no other run-away code that
// consumes system memory at a very high rate.

static void* ui_edit_alloc(int32_t bytes) {
    void* p = null;
    errno_t r = ut_heap.alloc(&p, bytes);
    swear(r == 0 && p != null); // fatal
    return p;
}

static void ui_edit_allocate(void** pp, int32_t count, size_t element) {
    not_null(pp);
    assert(count > 0 && (int64_t)count * (int64_t)element <= (int64_t)INT_MAX);
    *pp = ui_edit_alloc(count * (int32_t)element);
}

static void ui_edit_free(void** pp) {
    not_null(pp);
    // free(null) is acceptable but may indicate unbalanced caller logic
    not_null(*pp);
    ut_heap.free(*pp);
    *pp = null;
}

static void ui_edit_reallocate(void** pp, int32_t count, size_t element) {
    not_null(pp);
    assert(count > 0 && (int64_t)count * (int64_t)element <= (int64_t)INT_MAX);
    if (*pp == null) {
        ui_edit_allocate(pp, count, element);
    } else {
        errno_t r = ut_heap.realloc(pp, (int64_t)count * (int64_t)element);
        swear(r == 0 && *pp != null); // intentionally fatal
    }
}

static void ui_edit_invalidate(ui_edit_t* e) {
//  traceln("");
    ui_view.invalidate(&e->view);
}

static int32_t ui_edit_text_width(ui_edit_t* e, const char* s, int32_t n) {
//  fp64_t time = ut_clock.seconds();
    // average measure_text() performance per character:
    // "ui_app.fonts.mono"    ~500us (microseconds)
    // "ui_app.fonts.regular" ~250us (microseconds)
    int32_t x = n == 0 ? 0 : ui_gdi.measure_text(e->view.fm->font, "%.*s", n, s).w;
//  time = (ut_clock.seconds() - time) * 1000.0;
//  static fp64_t time_sum;
//  static fp64_t length_sum;
//  time_sum += time;
//  length_sum += n;
//  traceln("avg=%.6fms per char total %.3fms", time_sum / length_sum, time_sum);
    return x;
}

static int32_t ui_edit_glyph_bytes(char start_byte_value) { // utf-8
    // return 1-4 bytes glyph starting with `start_byte_value` character
    uint8_t uc = (uint8_t)start_byte_value;
    // 0xxxxxxx
    if ((uc & 0x80) == 0x00) { return 1; }
    // 110xxxxx 10xxxxxx 0b1100=0xE 0x1100=0xC
    if ((uc & 0xE0) == 0xC0) { return 2; }
    // 1110xxxx 10xxxxxx 10xxxxxx 0b1111=0xF 0x1110=0xE
    if ((uc & 0xF0) == 0xE0) { return 3; }
    // 11110xxx 10xxxxxx 10xxxxxx 10xxxxxx 0b1111,1000=0xF8 0x1111,0000=0xF0
    if ((uc & 0xF8) == 0xF0) { return 4; }
// TODO: should NOT be fatal: try editing .exe file to see the crash
    fatal_if(true, "incorrect UTF first byte 0%02X", uc);
    return -1;
}

// g2b() return number of glyphs in text and fills optional
// g2b[] array with glyphs positions.

static int32_t ui_edit_g2b(const char* utf8, int32_t bytes, int32_t g2b[]) {
    int32_t i = 0;
    int32_t k = 1;
    // g2b[k] start postion in uint8_t offset from utf8 text of glyph[k]
    if (g2b != null) { g2b[0] = 0; }
    while (i < bytes) {
        i += ui_edit_glyph_bytes(utf8[i]);
        if (g2b != null) { g2b[k] = i; }
        k++;
    }
    return k - 1;
}

static int32_t ui_edit_glyphs(const char* utf8, int32_t bytes) {
    return ui_edit_g2b(utf8, bytes, null);
}

static int32_t ui_edit_gp_to_bytes(const char* s, int32_t bytes, int32_t gp) {
    int32_t c = 0;
    int32_t i = 0;
    if (bytes > 0) {
        while (c < gp) {
            assert(i < bytes);
            i += ui_edit_glyph_bytes(s[i]);
            c++;
        }
    }
    assert(i <= bytes);
    return i;
}

static void ui_edit_paragraph_g2b(ui_edit_t* e, int32_t pn) {
    assert(0 <= pn && pn < e->paragraphs);
    ui_edit_para_t* p = &e->para[pn];
    if (p->glyphs < 0) {
        const int32_t bytes = p->bytes;
        const int32_t n = p->bytes + 1;
        const int32_t a = (n * (int32_t)sizeof(int32_t)) * 3 / 2; // heuristic
        if (p->g2b_capacity < a) {
            ui_edit_reallocate((void**)&p->g2b, n, sizeof(int32_t));
            p->g2b_capacity = a;
        }
        const char* utf8 = p->text;
        p->g2b[0] = 0; // first glyph starts at 0
        int32_t i = 0;
        int32_t k = 1;
        // g2b[k] start postion in uint8_t offset from utf8 text of glyph[k]
        while (i < bytes) {
            i += ui_edit_glyph_bytes(utf8[i]);
            p->g2b[k] = i;
            k++;
        }
        p->glyphs = k - 1;
    }
}

static int32_t ui_edit_word_break_at(ui_edit_t* e, int32_t pn, int32_t rn,
        const int32_t width, bool allow_zero) {
    ui_edit_para_t* p = &e->para[pn];
    int32_t k = 1; // at least 1 glyph
    // offsets inside a run in glyphs and bytes from start of the paragraph:
    int32_t gp = p->run[rn].gp;
    int32_t bp = p->run[rn].bp;
    if (gp < p->glyphs - 1) {
        const char* text = p->text + bp;
        const int32_t glyphs_in_this_run = p->glyphs - gp;
        int32_t* g2b = &p->g2b[gp];
        // 4 is maximum number of bytes in a UTF-8 sequence
        int32_t gc = ut_min(4, glyphs_in_this_run);
        int32_t w = ui_edit_text_width(e, text, g2b[gc] - bp);
        while (gc < glyphs_in_this_run && w < width) {
            gc = ut_min(gc * 4, glyphs_in_this_run);
            w = ui_edit_text_width(e, text, g2b[gc] - bp);
        }
        if (w < width) {
            k = gc;
            assert(1 <= k && k <= p->glyphs - gp);
        } else {
            int32_t i = 0;
            int32_t j = gc;
            k = (i + j) / 2;
            while (i < j) {
                assert(allow_zero || 1 <= k && k < gc + 1);
                const int32_t n = g2b[k + 1] - bp;
                int32_t px = ui_edit_text_width(e, text, n);
                if (px == width) { break; }
                if (px < width) { i = k + 1; } else { j = k; }
                if (!allow_zero && (i + j) / 2 == 0) { break; }
                k = (i + j) / 2;
                assert(allow_zero || 1 <= k && k <= p->glyphs - gp);
            }
        }
    }
    assert(allow_zero || 1 <= k && k <= p->glyphs - gp);
    return k;
}

static int32_t ui_edit_word_break(ui_edit_t* e, int32_t pn, int32_t rn) {
    return ui_edit_word_break_at(e, pn, rn, e->view.w, false);
}

static int32_t ui_edit_glyph_at_x(ui_edit_t* e, int32_t pn, int32_t rn,
        int32_t x) {
    if (x == 0 || e->para[pn].bytes == 0) {
        return 0;
    } else {
        return ui_edit_word_break_at(e, pn, rn, x + 1, true);
    }
}

static ui_edit_glyph_t ui_edit_glyph_at(ui_edit_t* e, ui_edit_pg_t p) {
    ui_edit_glyph_t g = { .s = "", .bytes = 0 };
    if (p.pn == e->paragraphs) {
        assert(p.gp == 0); // last empty paragraph
    } else {
        ui_edit_paragraph_g2b(e, p.pn);
        const int32_t bytes = e->para[p.pn].bytes;
        char* s = e->para[p.pn].text;
        const int32_t bp = e->para[p.pn].g2b[p.gp];
        if (bp < bytes) {
            g.s = s + bp;
            g.bytes = ui_edit_glyph_bytes(*g.s);
//          traceln("glyph: %.*s 0x%02X bytes: %d", g.bytes, g.s, *g.s, g.bytes);
        }
    }
    return g;
}

// paragraph_runs() breaks paragraph into `runs` according to `width`

static const ui_edit_run_t* ui_edit_paragraph_runs(ui_edit_t* e, int32_t pn,
        int32_t* runs) {
//  fp64_t time = ut_clock.seconds();
    assert(e->view.w > 0);
    const ui_edit_run_t* r = null;
    if (pn == e->paragraphs) {
        static const ui_edit_run_t eof_run = { 0 };
        *runs = 1;
        r = &eof_run;
    } else if (e->para[pn].run != null) {
        *runs = e->para[pn].runs;
        r = e->para[pn].run;
    } else {
        assert(0 <= pn && pn < e->paragraphs);
        ui_edit_paragraph_g2b(e, pn);
        ui_edit_para_t* p = &e->para[pn];
        if (p->run == null) {
            assert(p->runs == 0 && p->run == null);
            const int32_t max_runs = p->bytes + 1;
            ui_edit_allocate((void**)&p->run, max_runs, sizeof(ui_edit_run_t));
            ui_edit_run_t* run = p->run;
            run[0].bp = 0;
            run[0].gp = 0;
            int32_t gc = p->bytes == 0 ? 0 : ui_edit_word_break(e, pn, 0);
            if (gc == p->glyphs) { // whole paragraph fits into width
                p->runs = 1;
                run[0].bytes  = p->bytes;
                run[0].glyphs = p->glyphs;
                int32_t pixels = ui_edit_text_width(e, p->text, p->g2b[gc]);
                run[0].pixels = pixels;
            } else {
                assert(gc < p->glyphs);
                int32_t rc = 0; // runs count
                int32_t ix = 0; // glyph index from to start of paragraph
                char* text = p->text;
                int32_t bytes = p->bytes;
                while (bytes > 0) {
                    assert(rc < max_runs);
                    run[rc].bp = (int32_t)(text - p->text);
                    run[rc].gp = ix;
                    int32_t glyphs = ui_edit_word_break(e, pn, rc);
                    int32_t utf8bytes = p->g2b[ix + glyphs] - run[rc].bp;
                    int32_t pixels = ui_edit_text_width(e, text, utf8bytes);
                    if (glyphs > 1 && utf8bytes < bytes && text[utf8bytes - 1] != 0x20) {
                        // try to find word break SPACE character. utf8 space is 0x20
                        int32_t i = utf8bytes;
                        while (i > 0 && text[i - 1] != 0x20) { i--; }
                        if (i > 0 && i != utf8bytes) {
                            utf8bytes = i;
                            glyphs = ui_edit_glyphs(text, utf8bytes);
                            pixels = ui_edit_text_width(e, text, utf8bytes);
                        }
                    }
                    run[rc].bytes  = utf8bytes;
                    run[rc].glyphs = glyphs;
                    run[rc].pixels = pixels;
                    rc++;
                    text += utf8bytes;
                    assert(0 <= utf8bytes && utf8bytes <= bytes);
                    bytes -= utf8bytes;
                    ix += glyphs;
                }
                assert(rc > 0);
                p->runs = rc; // truncate heap capacity array:
                ui_edit_reallocate((void**)&p->run, rc, sizeof(ui_edit_run_t));
            }
        }
        *runs = p->runs;
        r = p->run;
    }
    assert(r != null && *runs >= 1);
//  time = ut_clock.seconds() - time;
//  traceln("%.3fms", time * 1000.0);
    return r;
}

static int32_t ui_edit_paragraph_run_count(ui_edit_t* e, int32_t pn) {
    int32_t runs = 0;
    (void)ui_edit_paragraph_runs(e, pn, &runs);
    return runs;
}

static int32_t ui_edit_glyphs_in_paragraph(ui_edit_t* e, int32_t pn) {
    (void)ui_edit_paragraph_run_count(e, pn); // word break into runs
    return e->para[pn].glyphs;
}

static void ui_edit_create_caret(ui_edit_t* e) {
    fatal_if(e->focused);
    assert(ui_app.is_active());
    assert(ui_app.has_focus());
    int32_t caret_width = ut_min(2, ut_max(1, ui_app.dpi.monitor_effective / 100));
//  traceln("%d,%d", caret_width, e->view.fm->em.h);
    ui_app.create_caret(caret_width, e->view.fm->em.h);
    e->focused = true; // means caret was created
}

static void ui_edit_destroy_caret(ui_edit_t* e) {
    fatal_if(!e->focused);
    ui_app.destroy_caret();
    e->focused = false; // means caret was destroyed
//  traceln("");
}

static void ui_edit_show_caret(ui_edit_t* e) {
    if (e->focused) {
        assert(ui_app.is_active());
        assert(ui_app.has_focus());
        ui_app.move_caret(e->view.x + e->caret.x, e->view.y + e->caret.y);
        // TODO: it is possible to support unblinking caret if desired
        // do not set blink time - use global default
//      fatal_if_false(SetCaretBlinkTime(500));
        ui_app.show_caret();
        e->shown++;
//      traceln("shown=%d", e->shown);
        assert(e->shown == 1);
    }
}

static void ui_edit_hide_caret(ui_edit_t* e) {
    if (e->focused) {
        ui_app.hide_caret();
        e->shown--;
//      traceln("shown=%d", e->shown);
        assert(e->shown == 0);
    }
}

static void ui_edit_dispose_paragraphs_layout(ui_edit_t* e) {
    for (int32_t i = 0; i < e->paragraphs; i++) {
        ui_edit_para_t* p = &e->para[i];
        if (p->run != null) {
            ui_edit_free((void**)&p->run);
        }
        if (p->g2b != null) {
            ui_edit_free((void**)&p->g2b);
        }
        p->glyphs = -1;
        p->runs = 0;
        p->g2b_capacity = 0;
    }
}

static void ui_edit_layout_now(ui_edit_t* e) {
    if (e->view.measure != null && e->view.layout != null && e->view.w > 0) {
        ui_edit_dispose_paragraphs_layout(e);
        e->view.measure(&e->view);
        e->view.layout(&e->view);
        ui_edit_invalidate(e);
    }
}

static void ui_edit_if_sle_layout(ui_edit_t* e) {
    // only for single line edit controls that were already initialized
    // and measured horizontally at least once.
    if (e->sle && e->view.layout != null && e->view.w > 0) {
        ui_edit_layout_now(e);
    }
}

static void ui_edit_set_font(ui_edit_t* e, ui_fm_t* f) {
    ui_edit_dispose_paragraphs_layout(e);
    e->scroll.rn = 0;
    e->view.fm = f;
    ui_edit_layout_now(e);
    ui_app.request_layout();
}

// Paragraph number, glyph number -> run number

static ui_edit_pr_t ui_edit_pg_to_pr(ui_edit_t* e, const ui_edit_pg_t pg) {
    ui_edit_pr_t pr = { .pn = pg.pn, .rn = -1 };
    if (pg.pn == e->paragraphs || e->para[pg.pn].bytes == 0) { // last or empty
        assert(pg.gp == 0);
        pr.rn = 0;
    } else {
        assert(0 <= pg.pn && pg.pn < e->paragraphs);
        int32_t runs = 0;
        const ui_edit_run_t* run = ui_edit_paragraph_runs(e, pg.pn, &runs);
        if (pg.gp == e->para[pg.pn].glyphs + 1) {
            pr.rn = runs - 1; // TODO: past last glyph ??? is this correct?
        } else {
            assert(0 <= pg.gp && pg.gp <= e->para[pg.pn].glyphs);
            for (int32_t j = 0; j < runs && pr.rn < 0; j++) {
                const int32_t last_run = j == runs - 1;
                const int32_t start = run[j].gp;
                const int32_t end = run[j].gp + run[j].glyphs + last_run;
                if (start <= pg.gp && pg.gp < end) {
                    pr.rn = j;
                }
            }
            assert(pr.rn >= 0);
        }
    }
    return pr;
}

static int32_t ui_edit_runs_between(ui_edit_t* e, const ui_edit_pg_t pg0,
        const ui_edit_pg_t pg1) {
    assert(ui_edit_uint64(pg0.pn, pg0.gp) <= ui_edit_uint64(pg1.pn, pg1.gp));
    int32_t rn0 = ui_edit_pg_to_pr(e, pg0).rn;
    int32_t rn1 = ui_edit_pg_to_pr(e, pg1).rn;
    int32_t rc = 0;
    if (pg0.pn == pg1.pn) {
        assert(rn0 <= rn1);
        rc = rn1 - rn0;
    } else {
        assert(pg0.pn < pg1.pn);
        for (int32_t i = pg0.pn; i < pg1.pn; i++) {
            const int32_t runs = ui_edit_paragraph_run_count(e, i);
            if (i == pg0.pn) {
                rc += runs - rn0;
            } else { // i < pg1.pn
                rc += runs;
            }
        }
        rc += rn1;
    }
    return rc;
}

static ui_edit_pg_t ui_edit_scroll_pg(ui_edit_t* e) {
    int32_t runs = 0;
    const ui_edit_run_t* run = ui_edit_paragraph_runs(e, e->scroll.pn, &runs);
    assert(0 <= e->scroll.rn && e->scroll.rn < runs);
    return (ui_edit_pg_t) { .pn = e->scroll.pn, .gp = run[e->scroll.rn].gp };
}

static int32_t ui_edit_first_visible_run(ui_edit_t* e, int32_t pn) {
    return pn == e->scroll.pn ? e->scroll.rn : 0;
}

// ui_edit::pg_to_xy() paragraph # glyph # -> (x,y) in [0,0  width x height]

static ui_point_t ui_edit_pg_to_xy(ui_edit_t* e, const ui_edit_pg_t pg) {
    ui_point_t pt = { .x = -1, .y = 0 };
    for (int32_t i = e->scroll.pn; i < e->paragraphs && pt.x < 0; i++) {
        int32_t runs = 0;
        const ui_edit_run_t* run = ui_edit_paragraph_runs(e, i, &runs);
        for (int32_t j = ui_edit_first_visible_run(e, i); j < runs; j++) {
            const int32_t last_run = j == runs - 1;
            int32_t gc = run[j].glyphs;
            if (i == pg.pn) {
                // in the last `run` of a paragraph x after last glyph is OK
                if (run[j].gp <= pg.gp && pg.gp < run[j].gp + gc + last_run) {
                    const char* s = e->para[i].text + run[j].bp;
                    int32_t ofs = ui_edit_gp_to_bytes(s, run[j].bytes,
                        pg.gp - run[j].gp);
                    pt.x = ui_edit_text_width(e, s, ofs);
                    break;
                }
            }
            pt.y += e->view.fm->em.h;
        }
    }
    if (pg.pn == e->paragraphs) { pt.x = 0; }
    if (0 <= pt.x && pt.x < e->view.w && 0 <= pt.y && pt.y <= e->view.h) {
        // all good, inside visible rectangle or right after it
    } else {
        traceln("outside (%d,%d) %dx%d", pt.x, pt.y, e->view.w, e->view.h);
    }
    return pt;
}

static int32_t ui_edit_glyph_width_px(ui_edit_t* e, const ui_edit_pg_t pg) {
    char* text = e->para[pg.pn].text;
    int32_t gc = e->para[pg.pn].glyphs;
    if (pg.gp == 0 &&  gc == 0) {
        return 0; // empty paragraph
    } else if (pg.gp < gc) {
        char* s = text + ui_edit_gp_to_bytes(text, e->para[pg.pn].bytes, pg.gp);
        int32_t bytes_in_glyph = ui_edit_glyph_bytes(*s);
        int32_t x = ui_edit_text_width(e, s, bytes_in_glyph);
        return x;
    } else {
        assert(pg.gp == gc, "only next position past last glyph is allowed");
        return 0;
    }
}

// xy_to_pg() (x,y) (0,0, width x height) -> paragraph # glyph #

static ui_edit_pg_t ui_edit_xy_to_pg(ui_edit_t* e, int32_t x, int32_t y) {
    ui_edit_pg_t pg = {-1, -1};
    int32_t py = 0; // paragraph `y' coordinate
    for (int32_t i = e->scroll.pn; i < e->paragraphs && pg.pn < 0; i++) {
        int32_t runs = 0;
        const ui_edit_run_t* run = ui_edit_paragraph_runs(e, i, &runs);
        for (int32_t j = ui_edit_first_visible_run(e, i); j < runs && pg.pn < 0; j++) {
            const ui_edit_run_t* r = &run[j];
            char* s = e->para[i].text + run[j].bp;
            if (py <= y && y < py + e->view.fm->em.h) {
                int32_t w = ui_edit_text_width(e, s, r->bytes);
                pg.pn = i;
                if (x >= w) {
                    const int32_t last_run = j == runs - 1;
                    pg.gp = r->gp + ut_max(0, r->glyphs - 1 + last_run);
                } else {
                    pg.gp = r->gp + ui_edit_glyph_at_x(e, i, j, x);
                    if (pg.gp < r->glyphs - 1) {
                        ui_edit_pg_t right = {pg.pn, pg.gp + 1};
                        int32_t x0 = ui_edit_pg_to_xy(e, pg).x;
                        int32_t x1 = ui_edit_pg_to_xy(e, right).x;
                        if (x1 - x < x - x0) {
                            pg.gp++; // snap to closest glyph's 'x'
                        }
                    }
                }
            } else {
                py += e->view.fm->em.h;
            }
        }
        if (py > e->view.h) { break; }
    }
    if (pg.pn < 0 && pg.gp < 0) {
        pg.pn = e->paragraphs;
        pg.gp = 0;
    }
    return pg;
}

static void ui_edit_paint_selection(ui_edit_t* e, const ui_edit_run_t* r,
        const char* text, int32_t pn, int32_t c0, int32_t c1) {
    uint64_t s0 = ui_edit_uint64(e->selection[0].pn, e->selection[0].gp);
    uint64_t e0 = ui_edit_uint64(e->selection[1].pn, e->selection[1].gp);
    if (s0 > e0) {
        uint64_t swap = e0;
        e0 = s0;
        s0 = swap;
    }
    uint64_t s1 = ui_edit_uint64(pn, c0);
    uint64_t e1 = ui_edit_uint64(pn, c1);
    if (s0 <= e1 && s1 <= e0) {
        uint64_t start = ut_max(s0, s1) - (uint64_t)c0;
        uint64_t end = ut_min(e0, e1) - (uint64_t)c0;
        if (start < end) {
            int32_t fro = (int32_t)start;
            int32_t to  = (int32_t)end;
            int32_t ofs0 = ui_edit_gp_to_bytes(text, r->bytes, fro);
            int32_t ofs1 = ui_edit_gp_to_bytes(text, r->bytes, to);
            int32_t x0 = ui_edit_text_width(e, text, ofs0);
            int32_t x1 = ui_edit_text_width(e, text, ofs1);
            ui_color_t selection_color = ui_rgb(64, 72, 96);
            ui_gdi.fill_with(ui_gdi.x + x0, ui_gdi.y,
                             x1 - x0, e->view.fm->em.h, selection_color);
//  TODO: remove?
//          ui_brush_t b = ui_gdi.set_brush(ui_gdi.brush_color);
//          ui_color_t c = ui_gdi.set_brush_color(ui_rgb(48, 64, 72));
//          ui_gdi.fill(ui_gdi.x + x0, ui_gdi.y, x1 - x0, e->view.fm->em.h);
//          ui_gdi.set_brush_color(c);
//          ui_gdi.set_brush(b);
        }
    }
}

static void ui_edit_paint_paragraph(ui_edit_t* e, int32_t pn) {
    int32_t runs = 0;
    const ui_edit_run_t* run = ui_edit_paragraph_runs(e, pn, &runs);
    for (int32_t j = ui_edit_first_visible_run(e, pn);
                 j < runs && ui_gdi.y < e->view.y + e->bottom; j++) {
        char* text = e->para[pn].text + run[j].bp;
        ui_gdi.x = e->view.x;
        ui_edit_paint_selection(e, &run[j], text, pn, run[j].gp, run[j].gp + run[j].glyphs);
        ui_gdi.text("%.*s", run[j].bytes, text);
        ui_gdi.y += e->view.fm->em.h;
    }
}

static void ui_edit_set_caret(ui_edit_t* e, int32_t x, int32_t y) {
    if (e->caret.x != x || e->caret.y != y) {
        if (e->focused && ui_app.has_focus()) {
            ui_app.move_caret(e->view.x + x, e->view.y + y);
//          traceln("%d,%d", e->view.x + x, e->view.y + y);
        }
        e->caret.x = x;
        e->caret.y = y;
    }
}

// scroll_up() text moves up (north) in the visible view,
// scroll position increments moves down (south)

static void ui_edit_scroll_up(ui_edit_t* e, int32_t run_count) {
    assert(0 < run_count, "does it make sense to have 0 scroll?");
    const ui_edit_pg_t eof = {.pn = e->paragraphs, .gp = 0};
    while (run_count > 0 && e->scroll.pn < e->paragraphs) {
        ui_edit_pg_t scroll = ui_edit_scroll_pg(e);
        int32_t between = ui_edit_runs_between(e, scroll, eof);
        if (between <= e->visible_runs - 1) {
            run_count = 0; // enough
        } else {
            int32_t runs = ui_edit_paragraph_run_count(e, e->scroll.pn);
            if (e->scroll.rn < runs - 1) {
                e->scroll.rn++;
            } else if (e->scroll.pn < e->paragraphs) {
                e->scroll.pn++;
                e->scroll.rn = 0;
            }
            run_count--;
            assert(e->scroll.pn >= 0 && e->scroll.rn >= 0);
        }
    }
    ui_edit_if_sle_layout(e);
    ui_edit_invalidate(e);
}

// scroll_dw() text moves down (south) in the visible view,
// scroll position decrements moves up (north)

static void ui_edit_scroll_down(ui_edit_t* e, int32_t run_count) {
    assert(0 < run_count, "does it make sense to have 0 scroll?");
    while (run_count > 0 && (e->scroll.pn > 0 || e->scroll.rn > 0)) {
        int32_t runs = ui_edit_paragraph_run_count(e, e->scroll.pn);
        e->scroll.rn = ut_min(e->scroll.rn, runs - 1);
        if (e->scroll.rn == 0 && e->scroll.pn > 0) {
            e->scroll.pn--;
            e->scroll.rn = ui_edit_paragraph_run_count(e, e->scroll.pn) - 1;
        } else if (e->scroll.rn > 0) {
            e->scroll.rn--;
        }
        assert(e->scroll.pn >= 0 && e->scroll.rn >= 0);
        assert(0 <= e->scroll.rn &&
                    e->scroll.rn < ui_edit_paragraph_run_count(e, e->scroll.pn));
        run_count--;
    }
    ui_edit_if_sle_layout(e);
}

static void ui_edit_scroll_into_view(ui_edit_t* e, const ui_edit_pg_t pg) {
    if (e->paragraphs > 0 && e->bottom > 0) {
        if (e->sle) { assert(pg.pn == 0); }
        const int32_t rn = ui_edit_pg_to_pr(e, pg).rn;
        const uint64_t scroll = ui_edit_uint64(e->scroll.pn, e->scroll.rn);
        const uint64_t caret  = ui_edit_uint64(pg.pn, rn);
        uint64_t last = 0;
        int32_t py = 0;
        const int32_t pn = e->scroll.pn;
        const int32_t bottom = e->bottom;
        for (int32_t i = pn; i < e->paragraphs && py < bottom; i++) {
            int32_t runs = ui_edit_paragraph_run_count(e, i);
            const int32_t fvr = ui_edit_first_visible_run(e, i);
            for (int32_t j = fvr; j < runs && py < bottom; j++) {
                last = ui_edit_uint64(i, j);
                py += e->view.fm->em.h;
            }
        }
        int32_t sle_runs = e->sle && e->view.w > 0 ?
            ui_edit_paragraph_run_count(e, 0) : 0;
        ui_edit_paragraph_g2b(e, e->paragraphs - 1);
        ui_edit_pg_t last_paragraph = {.pn = e->paragraphs - 1,
            .gp = e->para[e->paragraphs - 1].glyphs };
        ui_edit_pr_t lp = ui_edit_pg_to_pr(e, last_paragraph);
        uint64_t eof = ui_edit_uint64(e->paragraphs - 1, lp.rn);
        if (last == eof && py <= bottom - e->view.fm->em.h) {
            // vertical white space for EOF on the screen
            last = ui_edit_uint64(e->paragraphs, 0);
        }
        if (scroll <= caret && caret < last) {
            // no scroll
        } else if (caret < scroll) {
            e->scroll.pn = pg.pn;
            e->scroll.rn = rn;
        } else if (e->sle && sle_runs * e->view.fm->em.h <= e->view.h) {
            // single line edit control fits vertically - no scroll
        } else {
            assert(caret >= last);
            e->scroll.pn = pg.pn;
            e->scroll.rn = rn;
            while (e->scroll.pn > 0 || e->scroll.rn > 0) {
                ui_point_t pt = ui_edit_pg_to_xy(e, pg);
                if (pt.y + e->view.fm->em.h > bottom - e->view.fm->em.h) { break; }
                if (e->scroll.rn > 0) {
                    e->scroll.rn--;
                } else {
                    e->scroll.pn--;
                    e->scroll.rn = ui_edit_paragraph_run_count(e, e->scroll.pn) - 1;
                }
            }
        }
    }
}

static void ui_edit_move_caret(ui_edit_t* e, const ui_edit_pg_t pg) {
    // single line edit control cannot move caret past fist paragraph
    bool can_move = !e->sle || pg.pn < e->paragraphs;
    if (can_move) {
        ui_edit_scroll_into_view(e, pg);
        ui_point_t pt = e->view.w > 0 ? // width == 0 means no measure/layout yet
            ui_edit_pg_to_xy(e, pg) : (ui_point_t){0, 0};
        ui_edit_set_caret(e, pt.x, pt.y + e->top);
        e->selection[1] = pg;
        if (!ui_app.shift && e->mouse == 0) {
            e->selection[0] = e->selection[1];
        }
        ui_edit_invalidate(e);
    }
}

static char* ui_edit_ensure(ui_edit_t* e, int32_t pn, int32_t bytes,
        int32_t preserve) {
    assert(bytes >= 0 && preserve <= bytes);
    if (bytes <= e->para[pn].capacity) {
        // enough memory already capacity - do nothing
    } else if (e->para[pn].capacity > 0) {
        assert(preserve <= e->para[pn].capacity);
        ui_edit_reallocate((void**)&e->para[pn].text, bytes, 1);
        fatal_if_null(e->para[pn].text);
        e->para[pn].capacity = bytes;
    } else {
        assert(e->para[pn].capacity == 0);
        char* text = ui_edit_alloc(bytes);
        e->para[pn].capacity = bytes;
        memcpy(text, e->para[pn].text, (size_t)preserve);
        e->para[pn].text = text;
        e->para[pn].bytes = preserve;
    }
    return e->para[pn].text;
}

static ui_edit_pg_t ui_edit_op(ui_edit_t* e, bool cut,
        ui_edit_pg_t from, ui_edit_pg_t to,
        char* text, int32_t* bytes) {
    #pragma push_macro("clip_append")
    #define clip_append(a, ab, mx, text, bytes) do {           \
        int32_t ba = (int32_t)(bytes); /* bytes to append */   \
        if (a != null) {                                       \
            assert(ab <= mx);                                  \
            memcpy(a, text, (size_t)ba);                       \
            a += ba;                                           \
        }                                                      \
        ab += ba;                                              \
    } while (0)
    char* a = text; // append
    int32_t ab = 0; // appended bytes
    int32_t limit = bytes != null ? *bytes : 0; // max byes in text
    uint64_t f = ui_edit_uint64(from.pn, from.gp);
    uint64_t t = ui_edit_uint64(to.pn, to.gp);
    if (f != t) {
        ui_edit_dispose_paragraphs_layout(e);
        if (f > t) { uint64_t swap = t; t = f; f = swap; }
        int32_t pn0 = (int32_t)(f >> 32);
        int32_t gp0 = (int32_t)(f);
        int32_t pn1 = (int32_t)(t >> 32);
        int32_t gp1 = (int32_t)(t);
        if (pn1 == e->paragraphs) { // last empty paragraph
            assert(gp1 == 0);
            pn1 = e->paragraphs - 1;
            gp1 = ui_edit_g2b(e->para[pn1].text, e->para[pn1].bytes, null);
        }
        const int32_t bytes0 = e->para[pn0].bytes;
        char* s0 = e->para[pn0].text;
        char* s1 = e->para[pn1].text;
        ui_edit_paragraph_g2b(e, pn0);
        const int32_t bp0 = e->para[pn0].g2b[gp0];
        if (pn0 == pn1) { // inside same paragraph
            const int32_t bp1 = e->para[pn0].g2b[gp1];
            clip_append(a, ab, limit, s0 + bp0, bp1 - bp0);
            if (cut) {
                if (e->para[pn0].capacity == 0) {
                    int32_t n = bytes0 - (bp1 - bp0);
                    s0 = ui_edit_alloc(n);
                    memcpy(s0, e->para[pn0].text, (size_t)bp0);
                    e->para[pn0].text = s0;
                    e->para[pn0].capacity = n;
                }
                assert(bytes0 - bp1 >= 0);
                memcpy(s0 + bp0, s1 + bp1, (size_t)(bytes0 - bp1));
                e->para[pn0].bytes -= (bp1 - bp0);
                e->para[pn0].glyphs = -1; // will relayout
            }
        } else {
            clip_append(a, ab, limit, s0 + bp0, bytes0 - bp0);
            clip_append(a, ab, limit, "\n", 1);
            for (int32_t i = pn0 + 1; i < pn1; i++) {
                clip_append(a, ab, limit, e->para[i].text, e->para[i].bytes);
                clip_append(a, ab, limit, "\n", 1);
            }
            const int32_t bytes1 = e->para[pn1].bytes;
            ui_edit_paragraph_g2b(e, pn1);
            const int32_t bp1 = e->para[pn1].g2b[gp1];
            clip_append(a, ab, limit, s1, bp1);
            if (cut) {
                int32_t total = bp0 + bytes1 - bp1;
                s0 = ui_edit_ensure(e, pn0, total, bp0);
                assert(bytes1 - bp1 >= 0);
                memcpy(s0 + bp0, s1 + bp1, (size_t)(bytes1 - bp1));
                e->para[pn0].bytes = bp0 + bytes1 - bp1;
                e->para[pn0].glyphs = -1; // will relayout
            }
        }
        int32_t deleted = cut ? pn1 - pn0 : 0;
        if (deleted > 0) {
            assert(pn0 + deleted < e->paragraphs);
            for (int32_t i = pn0 + 1; i <= pn0 + deleted; i++) {
                if (e->para[i].capacity > 0) {
                    ui_edit_free((void**)&e->para[i].text);
                }
            }
            for (int32_t i = pn0 + 1; i < e->paragraphs - deleted; i++) {
                e->para[i] = e->para[i + deleted];
            }
            for (int32_t i = e->paragraphs - deleted; i < e->paragraphs; i++) {
                memset(&e->para[i], 0, sizeof(e->para[i]));
            }
        }
        if (t == ui_edit_uint64(e->paragraphs, 0)) {
            clip_append(a, ab, limit, "\n", 1);
        }
        if (a != null) { assert(a == text + limit); }
        e->paragraphs -= deleted;
        from.pn = pn0;
        from.gp = gp0;
    } else {
        from.pn = -1;
        from.gp = -1;
    }
    if (bytes != null) { *bytes = ab; }
    (void)limit; // unused in release
    ui_edit_if_sle_layout(e);
    return from;
    #pragma pop_macro("clip_append")
}

static void ui_edit_insert_paragraph(ui_edit_t* e, int32_t pn) {
    ui_edit_dispose_paragraphs_layout(e);
    if (e->paragraphs + 1 > e->capacity / (int32_t)sizeof(ui_edit_para_t)) {
        int32_t n = (e->paragraphs + 1) * 3 / 2; // 1.5 times
        ui_edit_reallocate((void**)&e->para, n, sizeof(ui_edit_para_t));
        e->capacity = n * (int32_t)sizeof(ui_edit_para_t);
    }
    e->paragraphs++;
    for (int32_t i = e->paragraphs - 1; i > pn; i--) {
        e->para[i] = e->para[i - 1];
    }
    ui_edit_para_t* p = &e->para[pn];
    p->text = null;
    p->bytes = 0;
    p->glyphs = -1;
    p->capacity = 0;
    p->runs = 0;
    p->run = null;
    p->g2b = null;
    p->g2b_capacity = 0;
}

// insert_inline() inserts text (not containing \n paragraph
// break inside a paragraph)

static ui_edit_pg_t ui_edit_insert_inline(ui_edit_t* e, ui_edit_pg_t pg,
        const char* text, int32_t bytes) {
    assert(bytes > 0);
    for (int32_t i = 0; i < bytes; i++) {
        assert(text[i] != '\n',
           "text \"%s\" must not contain \\n character.", text);
    }
    if (pg.pn == e->paragraphs) {
        ui_edit_insert_paragraph(e, pg.pn);
    }
    const int32_t b = e->para[pg.pn].bytes;
    ui_edit_paragraph_g2b(e, pg.pn);
    char* s = e->para[pg.pn].text;
    const int32_t bp = e->para[pg.pn].g2b[pg.gp];
    int32_t n = (b + bytes) * 3 / 2; // heuristics 1.5 times of total
    if (e->para[pg.pn].capacity == 0) {
        s = ui_edit_alloc(n);
        memcpy(s, e->para[pg.pn].text, (size_t)b);
        e->para[pg.pn].text = s;
        e->para[pg.pn].capacity = n;
    } else if (e->para[pg.pn].capacity < b + bytes) {
        ui_edit_reallocate((void**)&s, n, 1);
        e->para[pg.pn].text = s;
        e->para[pg.pn].capacity = n;
    }
    s = e->para[pg.pn].text;
    assert(b - bp >= 0);
    memmove(s + bp + bytes, s + bp, (size_t)(b - bp)); // make space
    memcpy(s + bp, text, (size_t)bytes);
    e->para[pg.pn].bytes += bytes;
    ui_edit_dispose_paragraphs_layout(e);
    pg.gp = ui_edit_glyphs(s, bp + bytes);
    ui_edit_if_sle_layout(e);
    return pg;
}

static ui_edit_pg_t ui_edit_insert_paragraph_break(ui_edit_t* e,
        ui_edit_pg_t pg) {
    ui_edit_insert_paragraph(e, pg.pn + (pg.pn < e->paragraphs));
    const int32_t bytes = e->para[pg.pn].bytes;
    char* s = e->para[pg.pn].text;
    ui_edit_paragraph_g2b(e, pg.pn);
    const int32_t bp = e->para[pg.pn].g2b[pg.gp];
    ui_edit_pg_t next = {.pn = pg.pn + 1, .gp = 0};
    if (bp < bytes) {
        (void)ui_edit_insert_inline(e, next, s + bp, bytes - bp);
    } else {
        ui_edit_dispose_paragraphs_layout(e);
    }
    e->para[pg.pn].bytes = bp;
    return next;
}

static void ui_edit_key_left(ui_edit_t* e) {
    ui_edit_pg_t to = e->selection[1];
    if (to.pn > 0 || to.gp > 0) {
        ui_point_t pt = ui_edit_pg_to_xy(e, to);
        if (pt.x == 0 && pt.y == 0) {
            ui_edit_scroll_down(e, 1);
        }
        if (to.gp > 0) {
            to.gp--;
        } else if (to.pn > 0) {
            to.pn--;
            to.gp = ui_edit_glyphs_in_paragraph(e, to.pn);
        }
        ui_edit_move_caret(e, to);
        e->last_x = -1;
    }
}

static void ui_edit_key_right(ui_edit_t* e) {
    ui_edit_pg_t to = e->selection[1];
    if (to.pn < e->paragraphs) {
        int32_t glyphs = ui_edit_glyphs_in_paragraph(e, to.pn);
        if (to.gp < glyphs) {
            to.gp++;
            ui_edit_scroll_into_view(e, to);
        } else if (!e->sle) {
            to.pn++;
            to.gp = 0;
            ui_edit_scroll_into_view(e, to);
        }
        ui_edit_move_caret(e, to);
        e->last_x = -1;
    }
}

static void ui_edit_reuse_last_x(ui_edit_t* e, ui_point_t* pt) {
    // Vertical caret movement visually tend to move caret horizontally
    // in proportional font text. Remembering starting `x' value for vertical
    // movements alleviates this unpleasant UX experience to some degree.
    if (pt->x > 0) {
        if (e->last_x > 0) {
            int32_t prev = e->last_x - e->view.fm->em.w;
            int32_t next = e->last_x + e->view.fm->em.w;
            if (prev <= pt->x && pt->x <= next) {
                pt->x = e->last_x;
            }
        }
        e->last_x = pt->x;
    }
}

static void ui_edit_key_up(ui_edit_t* e) {
    const ui_edit_pg_t pg = e->selection[1];
    ui_edit_pg_t to = pg;
    if (to.pn == e->paragraphs) {
        assert(to.gp == 0); // positioned past EOF
        to.pn--;
        to.gp = e->para[to.pn].glyphs;
        ui_edit_scroll_into_view(e, to);
        ui_point_t pt = ui_edit_pg_to_xy(e, to);
        pt.x = 0;
        to.gp = ui_edit_xy_to_pg(e, pt.x, pt.y).gp;
    } else if (to.pn > 0 || ui_edit_pg_to_pr(e, to).rn > 0) {
        // top of the text
        ui_point_t pt = ui_edit_pg_to_xy(e, to);
        if (pt.y == 0) {
            ui_edit_scroll_down(e, 1);
        } else {
            pt.y -= 1;
        }
        ui_edit_reuse_last_x(e, &pt);
        assert(pt.y >= 0);
        to = ui_edit_xy_to_pg(e, pt.x, pt.y);
        assert(to.pn >= 0 && to.gp >= 0);
        int32_t rn0 = ui_edit_pg_to_pr(e, pg).rn;
        int32_t rn1 = ui_edit_pg_to_pr(e, to).rn;
        if (rn1 > 0 && rn0 == rn1) { // same run
            assert(to.gp > 0, "word break must not break on zero gp");
            int32_t runs = 0;
            const ui_edit_run_t* run = ui_edit_paragraph_runs(e, to.pn, &runs);
            to.gp = run[rn1].gp;
        }
    }
    ui_edit_move_caret(e, to);
}

static void ui_edit_key_down(ui_edit_t* e) {
    const ui_edit_pg_t pg = e->selection[1];
    ui_point_t pt = ui_edit_pg_to_xy(e, pg);
    ui_edit_reuse_last_x(e, &pt);
    // scroll runs guaranteed to be already layout for current state of view:
    ui_edit_pg_t scroll = ui_edit_scroll_pg(e);
    int32_t run_count = ui_edit_runs_between(e, scroll, pg);
    if (!e->sle && run_count >= e->visible_runs - 1) {
        ui_edit_scroll_up(e, 1);
    } else {
        pt.y += e->view.fm->em.h;
    }
    ui_edit_pg_t to = ui_edit_xy_to_pg(e, pt.x, pt.y);
    if (to.pn < 0 && to.gp < 0) {
        to.pn = e->paragraphs; // advance past EOF
        to.gp = 0;
    }
    ui_edit_move_caret(e, to);
}

static void ui_edit_key_home(ui_edit_t* e) {
    if (ui_app.ctrl) {
        e->scroll.pn = 0;
        e->scroll.rn = 0;
        e->selection[1].pn = 0;
        e->selection[1].gp = 0;
    }
    const int32_t pn = e->selection[1].pn;
    int32_t runs = ui_edit_paragraph_run_count(e, pn);
    const ui_edit_para_t* para = &e->para[pn];
    if (runs <= 1) {
        e->selection[1].gp = 0;
    } else {
        int32_t rn = ui_edit_pg_to_pr(e, e->selection[1]).rn;
        assert(0 <= rn && rn < runs);
        const int32_t gp = para->run[rn].gp;
        if (e->selection[1].gp != gp) {
            // first Home keystroke moves caret to start of run
            e->selection[1].gp = gp;
        } else {
            // second Home keystroke moves caret start of paragraph
            e->selection[1].gp = 0;
            if (e->scroll.pn >= e->selection[1].pn) { // scroll in
                e->scroll.pn = e->selection[1].pn;
                e->scroll.rn = 0;
            }
        }
    }
    if (!ui_app.shift) {
        e->selection[0] = e->selection[1];
    }
    ui_edit_move_caret(e, e->selection[1]);
}

static void ui_edit_key_end(ui_edit_t* e) {
    if (ui_app.ctrl) {
        int32_t py = e->bottom;
        for (int32_t i = e->paragraphs - 1; i >= 0 && py >= e->view.fm->em.h; i--) {
            int32_t runs = ui_edit_paragraph_run_count(e, i);
            for (int32_t j = runs - 1; j >= 0 && py >= e->view.fm->em.h; j--) {
                py -= e->view.fm->em.h;
                if (py < e->view.fm->em.h) {
                    e->scroll.pn = i;
                    e->scroll.rn = j;
                }
            }
        }
        e->selection[1].pn = e->paragraphs;
        e->selection[1].gp = 0;
    } else if (e->selection[1].pn == e->paragraphs) {
        assert(e->selection[1].gp == 0);
    } else {
        int32_t pn = e->selection[1].pn;
        int32_t gp = e->selection[1].gp;
        int32_t runs = 0;
        const ui_edit_run_t* run = ui_edit_paragraph_runs(e, pn, &runs);
        int32_t rn = ui_edit_pg_to_pr(e, e->selection[1]).rn;
        assert(0 <= rn && rn < runs);
        if (rn == runs - 1) {
            e->selection[1].gp = e->para[pn].glyphs;
        } else if (e->selection[1].gp == e->para[pn].glyphs) {
            // at the end of paragraph do nothing (or move caret to EOF?)
        } else if (e->para[pn].glyphs > 0 && gp != run[rn].glyphs - 1) {
            e->selection[1].gp = run[rn].gp + run[rn].glyphs - 1;
        } else {
            e->selection[1].gp = e->para[pn].glyphs;
        }
    }
    if (!ui_app.shift) {
        e->selection[0] = e->selection[1];
    }
    ui_edit_move_caret(e, e->selection[1]);
}

static void ui_edit_key_pageup(ui_edit_t* e) {
    int32_t n = ut_max(1, e->visible_runs - 1);
    ui_edit_pg_t scr = ui_edit_scroll_pg(e);
    ui_edit_pg_t bof = {.pn = 0, .gp = 0};
    int32_t m = ui_edit_runs_between(e, bof, scr);
    if (m > n) {
        ui_point_t pt = ui_edit_pg_to_xy(e, e->selection[1]);
        ui_edit_pr_t scroll = e->scroll;
        ui_edit_scroll_down(e, n);
        if (scroll.pn != e->scroll.pn || scroll.rn != e->scroll.rn) {
            ui_edit_pg_t pg = ui_edit_xy_to_pg(e, pt.x, pt.y);
            ui_edit_move_caret(e, pg);
        }
    } else {
        ui_edit_move_caret(e, bof);
    }
}

static void ui_edit_key_pagedw(ui_edit_t* e) {
    int32_t n = ut_max(1, e->visible_runs - 1);
    ui_edit_pg_t scr = ui_edit_scroll_pg(e);
    ui_edit_pg_t eof = {.pn = e->paragraphs, .gp = 0};
    int32_t m = ui_edit_runs_between(e, scr, eof);
    if (m > n) {
        ui_point_t pt = ui_edit_pg_to_xy(e, e->selection[1]);
        ui_edit_pr_t scroll = e->scroll;
        ui_edit_scroll_up(e, n);
        if (scroll.pn != e->scroll.pn || scroll.rn != e->scroll.rn) {
            ui_edit_pg_t pg = ui_edit_xy_to_pg(e, pt.x, pt.y);
            ui_edit_move_caret(e, pg);
        }
    } else {
        ui_edit_move_caret(e, eof);
    }
}

static void ui_edit_key_delete(ui_edit_t* e) {
    uint64_t f = ui_edit_uint64(e->selection[0].pn, e->selection[0].gp);
    uint64_t t = ui_edit_uint64(e->selection[1].pn, e->selection[1].gp);
    uint64_t eof = ui_edit_uint64(e->paragraphs, 0);
    if (f == t && t != eof) {
        ui_edit_pg_t s1 = e->selection[1];
        e->key_right(e);
        e->selection[1] = s1;
    }
    e->erase(e);
}

static void ui_edit_key_backspace(ui_edit_t* e) {
    uint64_t f = ui_edit_uint64(e->selection[0].pn, e->selection[0].gp);
    uint64_t t = ui_edit_uint64(e->selection[1].pn, e->selection[1].gp);
    if (t != 0 && f == t) {
        ui_edit_pg_t s1 = e->selection[1];
        e->key_left(e);
        e->selection[1] = s1;
    }
    e->erase(e);
}

static void ui_edit_key_enter(ui_edit_t* e) {
    assert(!e->ro);
    if (!e->sle) {
        e->erase(e);
        e->selection[1] = ui_edit_insert_paragraph_break(e, e->selection[1]);
        e->selection[0] = e->selection[1];
        ui_edit_move_caret(e, e->selection[1]);
    } else { // single line edit callback
        if (e->enter != null) { e->enter(e); }
    }
}

static void ui_edit_key_pressed(ui_view_t* view, int64_t key) {
    assert(view->type == ui_view_text);
    ui_edit_t* e = (ui_edit_t*)view;
    if (e->focused) {
        if (key == ui.key.down && e->selection[1].pn < e->paragraphs) {
            e->key_down(e);
        } else if (key == ui.key.up && e->paragraphs > 0) {
            e->key_up(e);
        } else if (key == ui.key.left) {
            e->key_left(e);
        } else if (key == ui.key.right) {
            e->key_right(e);
        } else if (key == ui.key.pageup) {
            e->key_pageup(e);
        } else if (key == ui.key.pagedw) {
            e->key_pagedw(e);
        } else if (key == ui.key.home) {
            e->key_home(e);
        } else if (key == ui.key.end) {
            e->key_end(e);
        } else if (key == ui.key.del && !e->ro) {
            e->key_delete(e);
        } else if (key == ui.key.back && !e->ro) {
            e->key_backspace(e);
        } else if (key == ui.key.enter && !e->ro) {
            e->key_enter(e);
        } else {
            // ignore other keys
        }
    }
    if (e->fuzzer != null) { e->next_fuzz(e); }
}

static void ui_edit_character(ui_view_t* unused(view), const char* utf8) {
    assert(view->type == ui_view_text);
    assert(!view->hidden && !view->disabled);
    #pragma push_macro("ctl")
    #define ctl(c) ((char)((c) - 'a' + 1))
    ui_edit_t* e = (ui_edit_t*)view;
    if (e->focused) {
        char ch = utf8[0];
        if (ui_app.ctrl) {
            if (ch == ctl('a')) { e->select_all(e); }
            if (ch == ctl('c')) { e->copy_to_clipboard(e); }
            if (!e->ro) {
                if (ch == ctl('x')) { e->cut_to_clipboard(e); }
                if (ch == ctl('v')) { e->paste_from_clipboard(e); }
            }
        }
        if (0x20 <= ch && !e->ro) { // 0x20 space
            int32_t bytes = ui_edit_glyph_bytes(ch);
            e->erase(e); // remove selected text to be replaced by glyph
            e->selection[1] = ui_edit_insert_inline(e, e->selection[1], utf8, bytes);
            e->selection[0] = e->selection[1];
            ui_edit_move_caret(e, e->selection[1]);
        }
        ui_edit_invalidate(e);
        if (e->fuzzer != null) { e->next_fuzz(e); }
    }
    #pragma pop_macro("ctl")
}

static void ui_edit_select_word(ui_edit_t* e, int32_t x, int32_t y) {
    ui_edit_pg_t p = ui_edit_xy_to_pg(e, x, y);
    if (0 <= p.pn && 0 <= p.gp) {
        if (p.pn > e->paragraphs) { p.pn = ut_max(0, e->paragraphs); }
        int32_t glyphs = ui_edit_glyphs_in_paragraph(e, p.pn);
        if (p.gp > glyphs) { p.gp = ut_max(0, glyphs); }
        if (p.pn == e->paragraphs || glyphs == 0) {
            // last paragraph is empty - nothing to select on fp64_t click
        } else {
            ui_edit_glyph_t glyph = ui_edit_glyph_at(e, p);
            bool not_a_white_space = glyph.bytes > 0 &&
                *(const uint8_t*)glyph.s > 0x20;
            if (!not_a_white_space && p.gp > 0) {
                p.gp--;
                glyph = ui_edit_glyph_at(e, p);
                not_a_white_space = glyph.bytes > 0 &&
                    *(const uint8_t*)glyph.s > 0x20;
            }
            if (glyph.bytes > 0 && *(const uint8_t*)glyph.s > 0x20) {
                ui_edit_pg_t from = p;
                while (from.gp > 0) {
                    from.gp--;
                    ui_edit_glyph_t g = ui_edit_glyph_at(e, from);
                    if (g.bytes == 0 || *(const uint8_t*)g.s <= 0x20) {
                        from.gp++;
                        break;
                    }
                }
                e->selection[0] = from;
                ui_edit_pg_t to = p;
                while (to.gp < glyphs) {
                    to.gp++;
                    ui_edit_glyph_t g = ui_edit_glyph_at(e, to);
                    if (g.bytes == 0 || *(const uint8_t*)g.s <= 0x20) {
                        break;
                    }
                }
                e->selection[1] = to;
                ui_edit_invalidate(e);
                e->mouse = 0;
            }
        }
    }
}

static void ui_edit_select_paragraph(ui_edit_t* e, int32_t x, int32_t y) {
    ui_edit_pg_t p = ui_edit_xy_to_pg(e, x, y);
    if (0 <= p.pn && 0 <= p.gp) {
        if (p.pn > e->paragraphs) { p.pn = ut_max(0, e->paragraphs); }
        int32_t glyphs = ui_edit_glyphs_in_paragraph(e, p.pn);
        if (p.gp > glyphs) { p.gp = ut_max(0, glyphs); }
        if (p.pn == e->paragraphs || glyphs == 0) {
            // last paragraph is empty - nothing to select on fp64_t click
        } else if (p.pn == e->selection[0].pn &&
                ((e->selection[0].gp <= p.gp && p.gp <= e->selection[1].gp) ||
                 (e->selection[1].gp <= p.gp && p.gp <= e->selection[0].gp))) {
            e->selection[0].gp = 0;
            e->selection[1].gp = 0;
            e->selection[1].pn++;
        }
        ui_edit_invalidate(e);
        e->mouse = 0;
    }
}

static void ui_edit_double_click(ui_edit_t* e, int32_t x, int32_t y) {
    if (e->selection[0].pn == e->selection[1].pn &&
        e->selection[0].gp == e->selection[1].gp) {
        ui_edit_select_word(e, x, y);
    } else {
        if (e->selection[0].pn == e->selection[1].pn &&
               e->selection[0].pn <= e->paragraphs) {
            ui_edit_select_paragraph(e, x, y);
        }
    }
}

static void ui_edit_click(ui_edit_t* e, int32_t x, int32_t y) {
    ui_edit_pg_t p = ui_edit_xy_to_pg(e, x, y);
    if (0 <= p.pn && 0 <= p.gp) {
        if (p.pn > e->paragraphs) { p.pn = ut_max(0, e->paragraphs); }
        int32_t glyphs = ui_edit_glyphs_in_paragraph(e, p.pn);
        if (p.gp > glyphs) { p.gp = ut_max(0, glyphs); }
        ui_edit_move_caret(e, p);
    }
}

static void ui_edit_focus_on_click(ui_edit_t* e, int32_t x, int32_t y) {
    if (ui_app.has_focus() && !e->focused && e->mouse != 0) {
        if (ui_app.focus != null && ui_app.focus->kill_focus != null) {
            ui_app.focus->kill_focus(ui_app.focus);
        }
        ui_app.focus = &e->view;
        bool set = e->view.set_focus(&e->view);
        fatal_if(!set);
    }
    if (ui_app.has_focus() && e->focused && e->mouse != 0) {
        e->mouse = 0;
        ui_edit_click(e, x, y);
    }
}

static void ui_edit_mouse_button_down(ui_edit_t* e, int32_t m,
        int32_t x, int32_t y) {
    if (m == ui.message.left_button_pressed)  { e->mouse |= (1 << 0); }
    if (m == ui.message.right_button_pressed) { e->mouse |= (1 << 1); }
    ui_edit_focus_on_click(e, x, y);
}

static void ui_edit_mouse_button_up(ui_edit_t* e, int32_t m) {
    if (m == ui.message.left_button_released)  { e->mouse &= ~(1 << 0); }
    if (m == ui.message.right_button_released) { e->mouse &= ~(1 << 1); }
}

#ifdef EDIT_USE_TAP

static bool ui_edit_tap(ui_view_t* view, int32_t ix) {
    traceln("ix: %d", ix);
    if (ix == 0) {
        ui_edit_t* e = (ui_edit_t*)view;
        const int32_t x = ui_app.mouse.x - e->view.x;
        const int32_t y = ui_app.mouse.y - e->view.y - e->top;
        bool inside = 0 <= x && x < view->w && 0 <= y && y < view->h;
        if (inside) {
            e->mouse = 0x1;
            ui_edit_focus_on_click(e, x, y);
            e->mouse = 0x0;
        }
        return inside;
    } else {
        return false; // do NOT consume event
    }
}

#endif // EDIT_USE_TAP

static bool ui_edit_press(ui_view_t* view, int32_t ix) {
//  traceln("ix: %d", ix);
    if (ix == 0) {
        ui_edit_t* e = (ui_edit_t*)view;
        const int32_t x = ui_app.mouse.x - e->view.x;
        const int32_t y = ui_app.mouse.y - e->view.y - e->top;
        bool inside = 0 <= x && x < view->w && 0 <= y && y < view->h;
        if (inside) {
            e->mouse = 0x1;
            ui_edit_focus_on_click(e, x, y);
            ui_edit_double_click(e, x, y);
            e->mouse = 0x0;
        }
        return inside;
    } else {
        return false; // do NOT consume event
    }
}

static void ui_edit_mouse(ui_view_t* view, int32_t m, int64_t unused(flags)) {
//  if (m == ui.message.left_button_pressed) { traceln("%p", view); }
    assert(view->type == ui_view_text);
    assert(!view->hidden);
    assert(!view->disabled);
    ui_edit_t* e = (ui_edit_t*)view;
    const int32_t x = ui_app.mouse.x - e->view.x;
    const int32_t y = ui_app.mouse.y - e->view.y - e->top;
    bool inside = 0 <= x && x < view->w && 0 <= y && y < view->h;
    if (inside) {
        if (m == ui.message.left_button_pressed ||
            m == ui.message.right_button_pressed) {
            ui_edit_mouse_button_down(e, m, x, y);
        } else if (m == ui.message.left_button_released ||
                   m == ui.message.right_button_released) {
            ui_edit_mouse_button_up(e, m);
        } else if (m == ui.message.left_double_click ||
                   m == ui.message.right_double_click) {
            ui_edit_double_click(e, x, y);
        }
    }
}

static void ui_edit_mousewheel(ui_view_t* view, int32_t unused(dx), int32_t dy) {
    // TODO: may make a use of dx in single line not-word-breaked edit control
    if (ui_app.focus == view) {
        assert(view->type == ui_view_text);
        ui_edit_t* e = (ui_edit_t*)view;
        int32_t lines = (abs(dy) + view->fm->em.h - 1) / view->fm->em.h;
        if (dy > 0) {
            ui_edit_scroll_down(e, lines);
        } else if (dy < 0) {
            ui_edit_scroll_up(e, lines);
        }
//  TODO: Ctrl UP/DW and caret of out of visible area scrolls are not
//        implemented. Not sure they are very good UX experience.
//        MacOS users may be used to scroll with touchpad, take a visual
//        peek, do NOT click and continue editing at last cursor position.
//        To me back forward stack navigation is much more intuitive and
//        much mode "modeless" in spirit of cut/copy/paste. But opinions
//        and editing habits vary. Easy to implement.
        ui_edit_pg_t pg = ui_edit_xy_to_pg(e, e->caret.x, e->caret.y);
        ui_edit_move_caret(e, pg);
    }
}

static bool ui_edit_set_focus(ui_view_t* view) {
    assert(view->type == ui_view_text);
    ui_edit_t* e = (ui_edit_t*)view;
//  traceln("active=%d has_focus=%d focused=%d",
//           ui_app.is_active(), ui_app.has_focus(), e->focused);
    assert(ui_app.focus == view || ui_app.focus == null);
    assert(view->focusable);
    ui_app.focus = view;
    if (ui_app.has_focus() && !e->focused) {
        ui_edit_create_caret(e);
        ui_edit_show_caret(e);
        ui_edit_if_sle_layout(e);
    }
    return true;
}

static void ui_edit_kill_focus(ui_view_t* view) {
    assert(view->type == ui_view_text);
    ui_edit_t* e = (ui_edit_t*)view;
//  traceln("active=%d has_focus=%d focused=%d",
//           ui_app.is_active(), ui_app.has_focus(), e->focused);
    if (e->focused) {
        ui_edit_hide_caret(e);
        ui_edit_destroy_caret(e);
        ui_edit_if_sle_layout(e);
    }
    if (ui_app.focus == view) { ui_app.focus = null; }
}

static void ui_edit_erase(ui_edit_t* e) {
    const ui_edit_pg_t from = e->selection[0];
    const ui_edit_pg_t to = e->selection[1];
    ui_edit_pg_t pg = ui_edit_op(e, true, from, to, null, null);
    if (pg.pn >= 0 && pg.gp >= 0) {
        e->selection[0] = pg;
        e->selection[1] = pg;
        ui_edit_move_caret(e, pg);
        ui_edit_invalidate(e);
    }
}

static void ui_edit_cut_copy(ui_edit_t* e, bool cut) {
    const ui_edit_pg_t from = e->selection[0];
    const ui_edit_pg_t to = e->selection[1];
    int32_t n = 0; // bytes between from..to
    ui_edit_op(e, false, from, to, null, &n);
    if (n > 0) {
        char* text = ui_edit_alloc(n + 1);
        ui_edit_pg_t pg = ui_edit_op(e, cut, from, to, text, &n);
        if (cut && pg.pn >= 0 && pg.gp >= 0) {
            e->selection[0] = pg;
            e->selection[1] = pg;
            ui_edit_move_caret(e, pg);
        }
        text[n] = 0; // make it zero terminated
        ut_clipboard.put_text(text);
        assert(n == (int32_t)strlen(text), "n=%d strlen(cb)=%d cb=\"%s\"",
               n, strlen(text), text);
        ui_edit_free((void**)&text);
    }
}

static void ui_edit_select_all(ui_edit_t* e) {
    e->selection[0] = (ui_edit_pg_t ){.pn = 0, .gp = 0};
    e->selection[1] = (ui_edit_pg_t ){.pn = e->paragraphs, .gp = 0};
    ui_edit_invalidate(e);
}

static int32_t ui_edit_copy(ui_edit_t* e, char* text, int32_t* bytes) {
    not_null(bytes);
    int32_t r = 0;
    const ui_edit_pg_t from = {.pn = 0, .gp = 0};
    const ui_edit_pg_t to = {.pn = e->paragraphs, .gp = 0};
    int32_t n = 0; // bytes between from..to
    ui_edit_op(e, false, from, to, null, &n);
    if (text != null) {
        int32_t m = ut_min(n, *bytes);
        enum { error_insufficient_buffer = 122 }; //  ERROR_INSUFFICIENT_BUFFER
        if (m < n) { r = error_insufficient_buffer; }
        ui_edit_op(e, false, from, to, text, &m);
    }
    *bytes = n;
    return r;
}

static void ui_edit_clipboard_cut(ui_edit_t* e) {
    if (!e->ro) { ui_edit_cut_copy(e, true); }
}

static void ui_edit_clipboard_copy(ui_edit_t* e) {
    ui_edit_cut_copy(e, false);
}

static ui_edit_pg_t ui_edit_paste_text(ui_edit_t* e,
        const char* s, int32_t n) {
    assert(!e->ro);
    ui_edit_pg_t pg = e->selection[1];
    int32_t i = 0;
    const char* text = s;
    while (i < n) {
        int32_t b = i;
        while (b < n && s[b] != '\n') { b++; }
        bool lf = b < n && s[b] == '\n';
        int32_t next = b + 1;
        if (b > i && s[b - 1] == '\r') { b--; } // CR LF
        if (b > i) {
            pg = ui_edit_insert_inline(e, pg, text, b - i);
        }
        if (lf && e->sle) {
            break;
        } else if (lf) {
            pg = ui_edit_insert_paragraph_break(e, pg);
        }
        text = s + next;
        i = next;
    }
    return pg;
}

static void ui_edit_paste(ui_edit_t* e, const char* s, int32_t n) {
    if (!e->ro) {
        if (n < 0) { n = (int32_t)strlen(s); }
        e->erase(e);
        e->selection[1] = ui_edit_paste_text(e, s, n);
        e->selection[0] = e->selection[1];
        if (e->view.w > 0) { ui_edit_move_caret(e, e->selection[1]); }
    }
}

static void ui_edit_clipboard_paste(ui_edit_t* e) {
    if (!e->ro) {
        ui_edit_pg_t pg = e->selection[1];
        int32_t bytes = 0;
        ut_clipboard.get_text(null, &bytes);
        if (bytes > 0) {
            char* text = ui_edit_alloc(bytes);
            int32_t r = ut_clipboard.get_text(text, &bytes);
            fatal_if_not_zero(r);
            if (bytes > 0 && text[bytes - 1] == 0) {
                bytes--; // clipboard includes zero terminator
            }
            if (bytes > 0) {
                e->erase(e);
                pg = ui_edit_paste_text(e, text, bytes);
                ui_edit_move_caret(e, pg);
            }
            ui_edit_free((void**)&text);
        }
    }
}

static void ui_edit_measure(ui_view_t* view) { // bottom up
    assert(view->type == ui_view_text);
    ui_edit_t* e = (ui_edit_t*)view;
    view->w = 0;
    view->h = 0;
    // enforce minimum size - it makes it checking corner cases much simpler
    // and it's hard to edit anything in a smaller area - will result in bad UX
    if (view->w < view->fm->em.w * 4) { view->w = view->fm->em.w * 4; }
    if (view->h < view->fm->em.h) { view->h = view->fm->em.h; }
    if (e->sle) { // for SLE if more than one run resize vertical:
        int32_t runs = ut_max(ui_edit_paragraph_run_count(e, 0), 1);
        if (view->h < view->fm->em.h * runs) { view->h = view->fm->em.h * runs; }
    }
//  traceln("%dx%d", view->w, view->h);
}

static void ui_edit_layout(ui_view_t* view) { // top down
//  traceln(">%d,%d %dx%d", view->y, view->y, view->w, view->h);
    assert(view->type == ui_view_text);
    assert(view->w > 0 && view->h > 0); // could be `if'
    ui_edit_t* e = (ui_edit_t*)view;
    // glyph position in scroll_pn paragraph:
    const ui_edit_pg_t scroll = view->w == 0 ?
        (ui_edit_pg_t){0, 0} : ui_edit_scroll_pg(e);
    // the optimization of layout disposal with cached
    // width and height cannot guarantee correct layout
    // in other changing conditions, e.g. moving UI
    // between monitors with different DPI or font
    // changes by the caller (Ctrl +/- 0)...
//  if (view->w > 0 && view->w != view->w) {
//      ui_edit_dispose_paragraphs_layout(e);
//  }
    // always dispose paragraphs layout:
    ui_edit_dispose_paragraphs_layout(e);
    int32_t sle_height = 0;
    if (e->sle) {
        int32_t runs = ut_max(ui_edit_paragraph_run_count(e, 0), 1);
        sle_height = ut_min(e->view.fm->em.h * runs, view->h);
    }
    e->top    = !e->sle ? 0 : (view->h - sle_height) / 2;
    e->bottom = !e->sle ? view->h : e->top + sle_height;
    e->visible_runs = (e->bottom - e->top) / e->view.fm->em.h; // fully visible
    // number of runs in e->scroll.pn may have changed with view->w change
    int32_t runs = ui_edit_paragraph_run_count(e, e->scroll.pn);
    e->scroll.rn = ui_edit_pg_to_pr(e, scroll).rn;
    assert(0 <= e->scroll.rn && e->scroll.rn < runs); (void)runs;
    // For single line editor distribute vertical gap evenly between
    // top and bottom. For multiline snap top line to y coordinate 0
    // otherwise resizing view will result in up-down jiggling of the
    // whole text
    if (e->focused) {
        // recreate caret because fm->em.h may have changed
        ui_edit_hide_caret(e);
        ui_edit_destroy_caret(e);
        ui_edit_create_caret(e);
        ui_edit_show_caret(e);
        ui_edit_move_caret(e, e->selection[1]);
    }
//  traceln("<%d,%d %dx%d", view->y, view->y, view->w, view->h);
}

static void ui_edit_paint(ui_view_t* view) {
    assert(view->type == ui_view_text);
    assert(!view->hidden);
    ui_edit_t* e = (ui_edit_t*)view;
    ui_gdi.push(view->x, view->y + e->top);
//  TODO: remove?
//  ui_gdi.set_brush(ui_gdi.brush_color);
//  ui_gdi.set_brush_color(ui_rgb(20, 20, 14));
    // background: ui_rgb(20, 20, 14) ?
    ui_gdi.fill_with(view->x, view->y, view->w, view->h, view->background);

    ui_gdi.set_clip(view->x, view->y, view->w, view->h);
    ui_font_t f = view->fm->font;
    f = ui_gdi.set_font(f);
    ui_gdi.set_text_color(view->color);
    const int32_t pn = e->scroll.pn;
    const int32_t bottom = view->y + e->bottom;
    assert(pn <= e->paragraphs);
    for (int32_t i = pn; i < e->paragraphs && ui_gdi.y < bottom; i++) {
        ui_edit_paint_paragraph(e, i);
    }
    ui_gdi.set_font(f);
    ui_gdi.set_clip(0, 0, 0, 0);
    ui_gdi.pop();
}

static void ui_edit_move(ui_edit_t* e, ui_edit_pg_t pg) {
    if (e->view.w > 0) {
        ui_edit_move_caret(e, pg); // may select text on move
    } else {
        e->selection[1] = pg;
    }
    e->selection[0] = e->selection[1];
}

static bool ui_edit_message(ui_view_t* view, int32_t unused(m), int64_t unused(wp),
        int64_t unused(lp), int64_t* unused(rt)) {
    ui_edit_t* e = (ui_edit_t*)view;
    if (ui_app.is_active() && ui_app.has_focus() && !view->hidden) {
        if (e->focused != (ui_app.focus == view)) {
//          traceln("message: 0x%04X e->focused != (ui_app.focus == view)", m);
            if (e->focused) {
                view->kill_focus(view);
            } else {
                view->set_focus(view);
            }
        }
    } else {
        // do nothing: when app will become active and focused
        //             it will react on app->focus changes
    }
    return false;
}

__declspec(dllimport) unsigned int __stdcall GetACP(void);

void ui_edit_init(ui_edit_t* e) {
    memset(e, 0, sizeof(*e));
    e->view.color_id = ui_color_id_window_text;
    e->view.background_id = ui_color_id_window;
    e->view.fm = &ui_app.fonts.regular;
    e->view.insets  = (ui_gaps_t){ 0.5, 0.5, 0.5, 0.5 };
    e->view.padding = (ui_gaps_t){ 0.5, 0.5, 0.5, 0.5 };
    e->view.min_w_em = 1.0;
    e->view.min_h_em = 1.0;
    e->view.type = ui_view_text;
    e->view.focusable = true;
    e->fuzz_seed = 1; // client can seed it with (ut_clock.nanoseconds() | 1)
    e->last_x    = -1;
    e->focused   = false;
    e->sle       = false;
    e->ro        = false;
//  TODO: remove?
// ui_rgb(168, 168, 150); // TODO: ui_colors.text ?
// e->view.color   = e->view.color;
    e->caret                = (ui_point_t){-1, -1};
    e->view.message         = ui_edit_message;
    e->view.paint           = ui_edit_paint;
    e->view.measure         = ui_edit_measure;
    e->view.layout          = ui_edit_layout;
    #ifdef EDIT_USE_TAP
    e->view.tap             = ui_edit_tap;
    #else
    e->view.mouse           = ui_edit_mouse;
    #endif
    e->view.press           = ui_edit_press;
    e->view.character       = ui_edit_character;
    e->view.set_focus       = ui_edit_set_focus;
    e->view.kill_focus      = ui_edit_kill_focus;
    e->view.key_pressed     = ui_edit_key_pressed;
    e->view.mouse_wheel     = ui_edit_mousewheel;
    e->set_font             = ui_edit_set_font;
    e->move                 = ui_edit_move;
    e->paste                = ui_edit_paste;
    e->copy                 = ui_edit_copy;
    e->erase                = ui_edit_erase;
    e->cut_to_clipboard     = ui_edit_clipboard_cut;
    e->copy_to_clipboard    = ui_edit_clipboard_copy;
    e->paste_from_clipboard = ui_edit_clipboard_paste;
    e->select_all           = ui_edit_select_all;
    e->key_down             = ui_edit_key_down;
    e->key_up               = ui_edit_key_up;
    e->key_left             = ui_edit_key_left;
    e->key_right            = ui_edit_key_right;
    e->key_pageup           = ui_edit_key_pageup;
    e->key_pagedw           = ui_edit_key_pagedw;
    e->key_home             = ui_edit_key_home;
    e->key_end              = ui_edit_key_end;
    e->key_delete           = ui_edit_key_delete;
    e->key_backspace        = ui_edit_key_backspace;
    e->key_enter            = ui_edit_key_enter;
    e->fuzz                 = null;
    // Expected manifest.xml containing UTF-8 code page
    // for Translate message and WM_CHAR to deliver UTF-8 characters
    // see: https://learn.microsoft.com/en-us/windows/apps/design/globalizing/use-utf8-code-page
    if (GetACP() != 65001) {
        traceln("codepage: %d UTF-8 will not be supported", GetACP());
    }
    // at the moment of writing there is no API call to inform Windows about process
    // preferred codepage except manifest.xml file in resource #1.
    // Absence of manifest.xml will result to ancient and useless ANSI 1252 codepage
    // TODO: may be change quick.h to use CreateWindowW() and translate UTF16 to UTF8
}
// _________________________________ ui_gdi.c _________________________________

#include "ut/ut.h"
#include "ut/ut_win32.h"

#pragma push_macro("ui_app_window")
#pragma push_macro("ui_app_canvas")
#pragma push_macro("ui_gdi_with_hdc")
#pragma push_macro("ui_gdi_hdc_with_font")

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
    POINT pt = (POINT){ .x = ui_gdi.x, .y = ui_gdi.y };
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

static void ui_gdi_line_with(int32_t x0, int32_t y0, int32_t x1, int32_t y1,
        ui_color_t c) {
    int32_t x = ui_gdi.x;
    int32_t y = ui_gdi.y;
    ui_gdi.x = x0;
    ui_gdi.y = y0;
    ui_gdi.move_to(x0, y0);
    ui_pen_t p = ui_gdi.set_colored_pen(c);
    ui_gdi.line(x1, y1);
    ui_gdi.set_pen(p);
    ui_gdi.x = x;
    ui_gdi.y = y;
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
    const bool tf = ui_color_is_transparent(fill);   // transparent fill
    const bool tb = ui_color_is_transparent(border); // transparent border
    ui_brush_t b = tf ? ui_gdi.brush_hollow : ui_gdi.brush_color;
    b = ui_gdi.set_brush(b);
    ui_color_t c = tf ? ui_colors.transparent : ui_gdi.set_brush_color(fill);
    ui_pen_t p = tb ? ui_gdi.set_pen(ui_gdi.pen_hollow) :
                      ui_gdi.set_colored_pen(border);
    ui_gdi.rect(x, y, w, h);
    if (!tf) { ui_gdi.set_brush_color(c); }
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
    TRIVERTEX vertex[2] = {0};
    vertex[0].x = x;
    vertex[0].y = y;
    // TODO: colors:
    vertex[0].Red   = (COLOR16)(((rgba_from >>  0) & 0xFF) << 8);
    vertex[0].Green = (COLOR16)(((rgba_from >>  8) & 0xFF) << 8);
    vertex[0].Blue  = (COLOR16)(((rgba_from >> 16) & 0xFF) << 8);
    vertex[0].Alpha = (COLOR16)(((rgba_from >> 24) & 0xFF) << 8);
    vertex[1].x = x + w;
    vertex[1].y = y + h;
    vertex[1].Red   = (COLOR16)(((rgba_to >>  0) & 0xFF) << 8);
    vertex[1].Green = (COLOR16)(((rgba_to >>  8) & 0xFF) << 8);
    vertex[1].Blue  = (COLOR16)(((rgba_to >> 16) & 0xFF) << 8);
    vertex[1].Alpha = (COLOR16)(((rgba_to >> 24) & 0xFF) << 8);
    GRADIENT_RECT gRect = {0, 1};
    const uint32_t mode = vertical ?
        GRADIENT_FILL_RECT_V : GRADIENT_FILL_RECT_H;
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
        bih->biSizeImage = (DWORD)(w * abs(h));
        POINT pt = { 0 };
        fatal_if_false(SetBrushOrgEx(ui_app_canvas(), 0, 0, &pt));
        fatal_if(StretchDIBits(ui_app_canvas(), sx, sy, sw, sh, x, y, w, h,
            pixels, bi, DIB_RGB_COLORS, SRCCOPY) == 0);
        fatal_if_false(SetBrushOrgEx(ui_app_canvas(), pt.x, pt.y, &pt));
    }
}

static BITMAPINFOHEADER ui_gdi_bgrx_init_bi(int32_t w, int32_t h, int32_t bpp) {
    assert(w > 0 && h >= 0); // h cannot be negative?
    BITMAPINFOHEADER bi = {
        .biSize = sizeof(BITMAPINFOHEADER),
        .biPlanes = 1,
        .biBitCount = (uint16_t)(bpp * 8),
        .biCompression = BI_RGB,
        .biWidth = w,
        .biHeight = -h, // top down image
        .biSizeImage = (DWORD)(w * abs(h) * bpp),
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
    assert(w > 0 && h >= 0); // h cannot be negative?
    bi->bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bi->bmiHeader.biWidth = w;
    bi->bmiHeader.biHeight = -h;  // top down image
    bi->bmiHeader.biPlanes = 1;
    bi->bmiHeader.biBitCount = (uint16_t)(bpp * 8);
    bi->bmiHeader.biCompression = BI_RGB;
    bi->bmiHeader.biSizeImage = (DWORD)(w * abs(h) * bpp);
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
            memcpy(scanline, pixels, (size_t)w);
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
    int32_t n = GetObjectA(ui_app.fonts.regular.font, sizeof(lf), &lf);
    fatal_if_false(n == (int32_t)sizeof(lf));
    lf.lfHeight = -height;
    ut_str_printf(lf.lfFaceName, "%s", family);
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

static HDC ui_gdi_get_dc(void) {
    not_null(ui_app_window());
    HDC hdc = ui_app_canvas() != null ?
              ui_app_canvas() : GetDC(ui_app_window());
    not_null(hdc);
    if (GetGraphicsMode(hdc) != GM_ADVANCED) {
        SetGraphicsMode(hdc, GM_ADVANCED);
    }
    return hdc;
}

static void ui_gdi_release_dc(HDC hdc) {
    if (ui_app_canvas() == null) {
        ReleaseDC(ui_app_window(), hdc);
    }
}

#define ui_gdi_with_hdc(code) do {           \
    HDC hdc = ui_gdi_get_dc();               \
    code                                     \
    ui_gdi_release_dc(hdc);                  \
} while (0)

#define ui_gdi_hdc_with_font(f, ...) do {    \
    not_null(f);                             \
    HDC hdc = ui_gdi_get_dc();               \
    HFONT font_ = SelectFont(hdc, (HFONT)f); \
    { __VA_ARGS__ }                          \
    SelectFont(hdc, font_);                  \
    ui_gdi_release_dc(hdc);                  \
} while (0)

static int32_t ui_gdi_baseline(ui_font_t f) {
    TEXTMETRICA tm;
    ui_gdi_hdc_with_font(f, {
        fatal_if_false(GetTextMetricsA(hdc, &tm));
    });
    return tm.tmAscent;
}

static int32_t ui_gdi_descent(ui_font_t f) {
    TEXTMETRICA tm;
    ui_gdi_hdc_with_font(f, {
        fatal_if_false(GetTextMetricsA(hdc, &tm));
    });
    return tm.tmDescent;
}

static void ui_gdi_dump_tm(HDC hdc) {
    TEXTMETRICA tm = {0};
    fatal_if_false(GetTextMetricsA(hdc, &tm));
    traceln("tm .height: %d           tm .ascent: %d           tm .descent: %d",
            tm.tmHeight, tm.tmAscent, tm.tmDescent);
    traceln("tm .internal_leading: %d tm .external_leading: %d tm .ave_char_width: %d",
            tm.tmInternalLeading, tm.tmExternalLeading, tm.tmAveCharWidth);
    traceln("tm .max_char_width: %d   tm .weight: %d           tm .overhang: %d",
            tm.tmMaxCharWidth, tm.tmWeight, tm.tmOverhang);
    traceln("tm .digitized_aspect_x: %d tm .digitized_aspect_y: %d tm .first_char: %d",
            tm.tmDigitizedAspectX, tm.tmDigitizedAspectY, tm.tmFirstChar);
    traceln("tm .last_char: %d        tm .default_char: %d     tm .break_char: %d",
            tm.tmLastChar, tm.tmDefaultChar, tm.tmBreakChar);
    traceln("tm .italic: %d           tm .underlined: %d       tm .struck_out: %d",
            tm.tmItalic, tm.tmUnderlined, tm.tmStruckOut);
    traceln("tm .pitch_and_family: %d tm .char_set: %d",
            tm.tmPitchAndFamily, tm.tmCharSet);
    char pitch[64] = { 0 };
    if ((tm.tmPitchAndFamily & 0xF0) & TMPF_FIXED_PITCH) { strcat(pitch, "FIXED_PITCH "); }
    if ((tm.tmPitchAndFamily & 0xF0) & TMPF_VECTOR)      { strcat(pitch, "VECTOR "); }
    if ((tm.tmPitchAndFamily & 0xF0) & TMPF_DEVICE)      { strcat(pitch, "DEVICE "); }
    if ((tm.tmPitchAndFamily & 0xF0) & TMPF_TRUETYPE)    { strcat(pitch, "TRUETYPE "); }
    traceln("tm .pitch_and_family: %s", pitch);
    if (tm.tmPitchAndFamily & TMPF_TRUETYPE) {
        OUTLINETEXTMETRICA otm = { .otmSize = sizeof(OUTLINETEXTMETRICA) };
        uint32_t bytes = GetOutlineTextMetricsA(hdc, otm.otmSize, &otm);
        swear(bytes == sizeof(OUTLINETEXTMETRICA));
        traceln("otm .otmSize: %d          otm .otmFiller: %d       otm .otmfsSelection: %u",
                otm.otmSize, otm.otmFiller, otm.otmfsSelection);
        traceln("otm .otmfsType: %u        otm .otmsCharSlopeRise: %d otm .otmsCharSlopeRun: %d",
                otm.otmfsType, otm.otmsCharSlopeRise, otm.otmsCharSlopeRun);
        traceln("otm .otmItalicAngle: %d   otm .otmEMSquare: %u     otm .otmAscent: %d",
                otm.otmItalicAngle, otm.otmEMSquare, otm.otmAscent);
        traceln("otm .otmDescent: %d       otm .otmLineGap: %u      otm .otmsCapEmHeight: %u",
                otm.otmDescent, otm.otmLineGap, otm.otmsCapEmHeight);
        traceln("otm .otmsXHeight: %u      otm .otmrcFontBox.left: %d otm .otmrcFontBox.top: %d",
                otm.otmsXHeight, otm.otmrcFontBox.left, otm.otmrcFontBox.top);
        traceln("otm .otmrcFontBox.right: %d otm .otmrcFontBox.bottom: %d otm .otmMacAscent: %d",
                otm.otmrcFontBox.right, otm.otmrcFontBox.bottom, otm.otmMacAscent);
        traceln("otm .otmMacDescent: %d    otm .otmMacLineGap: %u   otm .otmusMinimumPPEM: %u",
                otm.otmMacDescent, otm.otmMacLineGap, otm.otmusMinimumPPEM);
        traceln("otm .otmptSubscriptSize.x: %d otm .otmptSubscriptSize.y: %d otm .otmptSubscriptOffset.x: %d",
                otm.otmptSubscriptSize.x, otm.otmptSubscriptSize.y, otm.otmptSubscriptOffset.x);
        traceln("otm .otmptSubscriptOffset.y: %d otm .otmptSuperscriptSize.x: %d otm .otmptSuperscriptSize.y: %d",
                otm.otmptSubscriptOffset.y, otm.otmptSuperscriptSize.x, otm.otmptSuperscriptSize.y);
        traceln("otm .otmptSuperscriptOffset.x: %d otm .otmptSuperscriptOffset.y: %d otm .otmsStrikeoutSize: %u",
                otm.otmptSuperscriptOffset.x, otm.otmptSuperscriptOffset.y, otm.otmsStrikeoutSize);
        traceln("otm .otmsStrikeoutPosition: %d otm .otmsUnderscoreSize: %d otm .otmsUnderscorePosition: %d",
                otm.otmsStrikeoutPosition, otm.otmsUnderscoreSize, otm.otmsUnderscorePosition);
        traceln("dpi.system:            %d", ui_app.dpi.system);
        traceln("dpi.process:           %d", ui_app.dpi.process);
        traceln("dpi.window:            %d", ui_app.dpi.window);
        traceln("dpi.monitor_raw:       %d", ui_app.dpi.monitor_raw);
        traceln("dpi.monitor_effective: %d", ui_app.dpi.monitor_effective);
        traceln("dpi.monitor_angular:   %d", ui_app.dpi.monitor_angular);
    }
}

// get_em() is relatively expensive:
// 24 microseconds Core i-7 3667U 2.0 GHz (2012)

static void ui_gdi_update_fm(ui_fm_t* fm, ui_font_t f) {
    not_null(f);
    SIZE em = {0, 0}; // "M"
    *fm = (ui_fm_t){ .font = f };
    ui_gdi_hdc_with_font(f, {
        // https://en.wikipedia.org/wiki/Quad_(typography)
        // https://learn.microsoft.com/en-us/windows/win32/gdi/string-widths-and-heights
        // https://stackoverflow.com/questions/27631736/meaning-of-top-ascent-baseline-descent-bottom-and-leading-in-androids-font
        ui_gdi_dump_tm(hdc);
        // ut_glyph_nbsp and "M" have the same result
        fatal_if_false(GetTextExtentPoint32A(hdc, "m", 1, &em));
        traceln("em: %d %d", em.cx, em.cy);
        SIZE eq = {0, 0}; // "M"
        fatal_if_false(GetTextExtentPoint32A(hdc, ut_glyph_em_quad, 1, &eq));
        traceln("eq: %d %d", eq.cx, eq.cy);
        fm->height = ui_gdi.font_height(f);
        fm->descent = ui_gdi.descent(f);
        fm->baseline = ui_gdi.baseline(f);
        SIZE vl = {0}; // "|" Vertical Line https://www.compart.com/en/unicode/U+007C
        SIZE e3 = {0}; // Three-Em Dash
        fatal_if_false(GetTextExtentPoint32A(hdc, "|", 1, &vl));
        fatal_if_false(GetTextExtentPoint32A(hdc, ut_glyph_three_em_dash, 1, &e3));
        fm->mono = em.cx == vl.cx && vl.cx == e3.cx;
        traceln("vl: %d %d", vl.cx, vl.cy);
        traceln("e3: %d %d", e3.cx, e3.cy);
        traceln("fm->mono: %d height: %d descent: %d baseline: %d", fm->mono, fm->height, fm->descent, fm->baseline);
        traceln("");
    });
    assert(fm->baseline >= fm->height);
    const int32_t ascend = fm->descent - (fm->height - fm->baseline);
    fm->em = (ui_wh_t){em.cx, em.cy - ascend};
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
if (0) {
    HDC hdc = ui_app_canvas();
    if (hdc != null) {
        SIZE em = {0, 0}; // "M"
        fatal_if_false(GetTextExtentPoint32A(hdc, "M", 1, &em));
        traceln("em: %d %d", em.cx, em.cy);
        fatal_if_false(GetTextExtentPoint32A(hdc, ut_glyph_em_quad, 1, &em));
        traceln("em: %d %d", em.cx, em.cy);
        SIZE vl = {0}; // "|" Vertical Line https://www.compart.com/en/unicode/U+007C
        SIZE e3 = {0}; // Three-Em Dash
        fatal_if_false(GetTextExtentPoint32A(hdc, "|", 1, &vl));
        traceln("vl: %d %d", vl.cx, vl.cy);
        fatal_if_false(GetTextExtentPoint32A(hdc, ut_glyph_three_em_dash, 1, &e3));
        traceln("e3: %d %d", e3.cx, e3.cy);
    }
}
    int32_t count = ut_str.utf16_chars(s);
    assert(0 < count && count < 4096, "be reasonable count: %d?", count);
    uint16_t ws[4096];
    swear(count <= countof(ws), "find another way to draw!");
    ut_str.utf8to16(ws, count, s);
    int32_t h = 0; // return value is the height of the text
    if (font != null) {
        ui_gdi_hdc_with_font(font, { h = DrawTextW(hdc, ws, n, r, format); });
    } else { // with already selected font
        ui_gdi_with_hdc({ h = DrawTextW(hdc, ws, n, r, format); });
    }
    return h;
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
    not_null(p);
    char text[4096]; // expected to be enough for single text draw
    text[0] = 0;
    ut_str.format_va(text, countof(text), p->format, p->vl);
    text[countof(text) - 1] = 0;
    int32_t k = (int32_t)ut_str.len(text);
    swear(k > 0 && k < countof(text), "k=%d n=%d fmt=%s", k, p->format);
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

static ui_wh_t ui_gdi_text_measure(ui_gdi_dtp_t* p) {
    ui_gdi_text_draw(p);
    return (ui_wh_t){.w = p->rc.right - p->rc.left,
                     .h = p->rc.bottom - p->rc.top};
}

static ui_wh_t ui_gdi_measure_singleline(ui_font_t f, const char* format, ...) {
    va_list vl;
    va_start(vl, format);
    ui_gdi_dtp_t p = { f, format, vl, {0, 0, 0, 0}, sl_measure };
    ui_wh_t mt = ui_gdi_text_measure(&p);
    va_end(vl);
    return mt;
}

static ui_wh_t ui_gdi_measure_multiline(ui_font_t f, int32_t w, const char* format, ...) {
    va_list vl;
    va_start(vl, format);
    uint32_t flags = w <= 0 ? ml_measure : ml_measure_break;
    ui_gdi_dtp_t p = { f, format, vl, {ui_gdi.x, ui_gdi.y, ui_gdi.x + (w <= 0 ? 1 : w), ui_gdi.y}, flags };
    ui_wh_t mt = ui_gdi_text_measure(&p);
    va_end(vl);
    return mt;
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
    not_null(ui_app.fonts.mono.font);
    ui_font_t f = ui_gdi.set_font(ui_app.fonts.mono.font);
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
    not_null(ui_app.fonts.mono.font);
    ui_font_t f = ui_gdi.set_font(ui_app.fonts.mono.font);
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
    .line_with                     = ui_gdi_line_with,
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
    .update_fm                     = ui_gdi_update_fm,
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

#pragma pop_macro("ui_gdi_hdc_with_font")
#pragma pop_macro("ui_gdi_with_hdc")
#pragma pop_macro("ui_app_canvas")
#pragma pop_macro("ui_app_window")
// ________________________________ ui_label.c ________________________________

#include "ut/ut.h"

static void ui_label_paint(ui_view_t* v) {
    assert(v->type == ui_view_label);
    assert(!v->hidden);
    ui_ltrb_t i = ui_view.gaps(v, &v->insets);
    ui_gdi.push(v->x + i.left, v->y + i.top);
    ui_font_t f = ui_gdi.set_font(v->fm->font);
//  traceln("%s h=%d dy=%d baseline=%d", v->text, v->h,
//          v->label_dy, v->baseline);
    ui_color_t c = v->hover && v->highlightable ?
        ui_app.get_color(ui_color_id_highlight) : v->color;
    ui_gdi.set_text_color(c);
    // paint for text also does lightweight re-layout
    // which is useful for simplifying dynamic text changes
    bool multiline = strchr(v->text, '\n') != null;
    if (!multiline) {
        ui_gdi.text("%s", ui_view.nls(v));
    } else {
        int32_t w = (int32_t)((fp64_t)v->min_w_em * (fp64_t)v->fm->em.w + 0.5);
        ui_gdi.multiline(w == 0 ? -1 : w, "%s", ui_view.nls(v));
    }
    if (v->hover && !v->flat && v->highlightable) {
        ui_gdi.set_colored_pen(ui_app.get_color(ui_color_id_highlight));
        ui_gdi.set_brush(ui_gdi.brush_hollow);
        int32_t cr = v->fm->em.h / 4; // corner radius
        int32_t h = multiline ? v->h : v->fm->baseline + v->fm->descent;
        ui_gdi.rounded(v->x - cr, v->y, v->w + 2 * cr, h, cr, cr);
    }
    ui_gdi.set_font(f);
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
    v->color_id      = ui_color_id_button_text;
    v->background_id = ui_color_id_button_face;
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
    int32_t mxw[1024]; // more than enough for sane humane UI
    swear(cols <= countof(mxw));
    memset(mxw, 0, (size_t)cols * sizeof(int32_t));
    ui_view_for_each(view, row, {
        if (!row->hidden) {
            row->h = 0;
            row->fm->baseline = 0;
            int32_t i = 0;
            ui_view_for_each(row, col, {
                if (!col->hidden) {
                    mxw[i] = ut_max(mxw[i], col->w);
                    row->h = ut_max(row->h, col->h);
//                  traceln("[%d] row.fm->baseline: %d col.fm->baseline: %d ",
//                          i, row->fm->baseline, col->fm->baseline);
                    row->fm->baseline = ut_max(row->fm->baseline, col->fm->baseline);
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
/*
                    // TODO: label_dy needs to be transferred to containers
                    //       rationale: labels and buttons baselines must align
                    if (c->type == ui_view_label) { // lineup text baselines
                        c->label_dy = r->fm->baseline - c->fm->baseline;
                    }
*/
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

static void ui_mbx_measured(ui_view_t* view) {
    ui_mbx_t* mx = (ui_mbx_t*)view;
    assert(view->type == ui_view_mbx);
    int32_t n = 0;
    ui_view_for_each(view, c, { n++; });
    n--; // number of buttons
    const int32_t em_x = mx->label.fm->em.w;
    const int32_t em_y = mx->label.fm->em.h;
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
    const int32_t em_y = mx->label.fm->em.h;
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
    view->measured = ui_mbx_measured;
    view->layout = ui_mbx_layout;
    mx->view.fm = &ui_app.fonts.H3;
    int32_t n = 0;
    while (mx->options[n] != null && n < countof(mx->button) - 1) {
        mx->button[n] = (ui_button_t)ui_button("", 6.0, ui_mbx_button);
        ut_str_printf(mx->button[n].text, "%s", mx->options[n]);
        n++;
    }
    swear(n <= countof(mx->button), "inhumane: %d buttons is too many", n);
    if (n > countof(mx->button)) { n = countof(mx->button); }
    mx->label = (ui_label_t)ui_label(0, "");
    ut_str_printf(mx->label.text, "%s", mx->view.text);
    ui_view.add_last(&mx->view, &mx->label);
    for (int32_t i = 0; i < n; i++) {
        ui_view.add_last(&mx->view, &mx->button[i]);
        mx->button[i].fm = mx->view.fm;
        ui_view.localize(&mx->button[i]);
        // TODO: remove assert below
        assert(mx->button[i].parent == &mx->view);
    }
    mx->label.fm = mx->view.fm;
    ui_view.localize(&mx->label);
    mx->view.text[0] = 0;
    mx->option = -1;
}

void ui_mbx_init(ui_mbx_t* mx, const char* options[],
        const char* format, ...) {
    mx->view.type = ui_view_mbx;
    mx->view.measured  = ui_mbx_measured;
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

static void ui_slider_measured(ui_view_t* v) {
    assert(v->type == ui_view_slider);
    ui_slider_t* s = (ui_slider_t*)v;
    s->inc.fm = v->fm;
    s->dec.fm = v->fm;
    ui_view.measure(&s->dec);
    ui_view.measure(&s->inc);
    assert(s->inc.w == s->dec.w && s->inc.h == s->dec.h);
    const int32_t em = v->fm->em.w;
    const int32_t w = (int32_t)((fp64_t)v->min_w_em * (fp64_t)v->fm->em.w + 0.5);
    s->mt = ui_gdi.measure_text(v->fm->font, ui_view.nls(v), s->value_max);
//  if (w > r->mt.x) { r->mt.x = w; }
    s->mt.w = w != 0 ? w : s->mt.w;
    v->w = s->dec.w + s->mt.w + s->inc.w + em * 2;
    v->h = s->inc.h;
}

static void ui_slider_layout(ui_view_t* v) {
    assert(v->type == ui_view_slider);
    ui_slider_t* s = (ui_slider_t*)v;
    assert(s->inc.w == s->dec.w && s->inc.h == s->dec.h);
    const int32_t em = v->fm->em.w;
    s->dec.x = v->x;
    s->dec.y = v->y;
    s->inc.x = v->x + s->dec.w + s->mt.w + em * 2;
    s->inc.y = v->y;
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
    ui_slider_t* s = (ui_slider_t*)v;
    ui_gdi.push(v->x, v->y);
    ui_gdi.set_clip(v->x, v->y, v->w, v->h);
    const int32_t em = v->fm->em.w;
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
    const int32_t x = v->x + s->dec.w + em2;
    const int32_t y = v->y;
    const int32_t w = s->mt.w + em;
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
    const fp64_t range = (fp64_t)s->value_max - (fp64_t)s->value_min;
    fp64_t vw = (fp64_t)(s->mt.w + em) * (s->value - s->value_min) / range;
    ui_gdi.rect(x, v->y, (int32_t)(vw + 0.5), v->h);
    ui_gdi.x += s->dec.w + em;
    ui_gdi.y = y;
    ui_color_t c = ui_gdi.set_text_color(v->color);
    ui_font_t f = ui_gdi.set_font(v->fm->font);
    const char* format = ut_nls.str(v->text);
    ui_gdi.text(format, s->value);
    ui_gdi.set_font(f);
    ui_gdi.set_text_color(c);
    ui_gdi.set_clip(0, 0, 0, 0);
    ui_gdi.delete_pen(pen_c1);
    ui_gdi.delete_pen(pen_c0);
    ui_gdi.pop();
}

static void ui_slider_mouse(ui_view_t* v, int32_t message, int64_t f) {
    if (!v->hidden && !v->disabled) {
        assert(v->type == ui_view_slider);
        ui_slider_t* s = (ui_slider_t*)v;
        bool drag = message == ui.message.mouse_move &&
            (f & (ui.mouse.button.left|ui.mouse.button.right)) != 0;
        if (message == ui.message.left_button_pressed ||
            message == ui.message.right_button_pressed || drag) {
            const int32_t x = ui_app.mouse.x - v->x - s->dec.w;
            const int32_t y = ui_app.mouse.y - v->y;
            const int32_t x0 = v->fm->em.w / 2;
            const int32_t x1 = s->mt.w + v->fm->em.w;
            if (x0 <= x && x < x1 && 0 <= y && y < v->h) {
                ui_app.focus = v;
                const fp64_t range = (fp64_t)s->value_max - (fp64_t)s->value_min;
                fp64_t val = ((fp64_t)x - x0) * range / (fp64_t)(x1 - x0 - 1);
                int32_t vw = (int32_t)(val + s->value_min + 0.5);
                s->value = ut_min(ut_max(vw, s->value_min), s->value_max);
                if (s->view.callback != null) { s->view.callback(&s->view); }
                ui_view.invalidate(v);
            }
        }
    }
}

static void ui_slider_inc_dec_value(ui_slider_t* s, int32_t sign, int32_t mul) {
    if (!ui_view.is_hidden(&s->view) && !ui_view.is_disabled(&s->view)) {
        // full 0x80000000..0x7FFFFFFF (-2147483648..2147483647) range
        int32_t v = s->value;
        if (v > s->value_min && sign < 0) {
            mul = ut_min(v - s->value_min, mul);
            v += mul * sign;
        } else if (v < s->value_max && sign > 0) {
            mul = ut_min(s->value_max - v, mul);
            v += mul * sign;
        }
        if (s->value != v) {
            s->value = v;
            if (s->view.callback != null) { s->view.callback(&s->view); }
            ui_view.invalidate(&s->view);
        }
    }
}

static void ui_slider_inc_dec(ui_button_t* b) {
    ui_slider_t* s = (ui_slider_t*)b->parent;
    if (!ui_view.is_hidden(&s->view) && !ui_view.is_disabled(&s->view)) {
        int32_t sign = b == &s->inc ? +1 : -1;
        int32_t mul = ui_app.shift && ui_app.ctrl ? 1000 :
            ui_app.shift ? 100 : ui_app.ctrl ? 10 : 1;
        ui_slider_inc_dec_value(s, sign, mul);
    }
}

static void ui_slider_every_100ms(ui_view_t* v) { // 100ms
    assert(v->type == ui_view_slider);
    ui_slider_t* s = (ui_slider_t*)v;
    if (ui_view.is_hidden(v) || ui_view.is_disabled(v)) {
        s->time = 0;
    } else if (!s->dec.armed && !s->inc.armed) {
        s->time = 0;
    } else {
        if (s->time == 0) {
            s->time = ui_app.now;
        } else if (ui_app.now - s->time > 1.0) {
            const int32_t sign = s->dec.armed ? -1 : +1;
            int32_t sec = (int32_t)(ui_app.now - s->time + 0.5);
            int32_t mul = sec >= 1 ? 1 << (sec - 1) : 1;
            const int64_t range = (int64_t)s->value_max - s->value_min;
            if (mul > range / 8) { mul = (int32_t)(range / 8); }
            ui_slider_inc_dec_value(s, sign, ut_max(mul, 1));
        }
    }
}

void ui_view_init_slider(ui_view_t* v) {
    assert(v->type == ui_view_slider);
    ui_view.set_text(v, v->text);
    v->measured = ui_slider_measured;
    v->layout      = ui_slider_layout;
    v->paint       = ui_slider_paint;
    v->mouse       = ui_slider_mouse;
    v->every_100ms = ui_slider_every_100ms;
    v->color_id    = ui_color_id_window_text;
    ui_slider_t* s = (ui_slider_t*)v;
    static const char* accel =
        " Hold key while clicking\n"
        " Ctrl: x 10 Shift: x 100 \n"
        " Ctrl+Shift: x 1000 \n for step multiplier.";
    s->dec = (ui_button_t)ui_button(ut_glyph_heavy_minus_sign, 0,
                                    ui_slider_inc_dec);
    s->dec.fm = v->fm;
    ut_str_printf(s->dec.hint, "%s", accel);
    s->inc = (ui_button_t)ui_button(ut_glyph_heavy_minus_sign, 0,
                                    ui_slider_inc_dec);
    s->inc.fm = v->fm;
    ut_str_printf(s->inc.hint, "%s", accel);
    ui_view.add(&s->view, &s->dec, &s->inc, null);
    ui_view.localize(&s->view);
}

void ui_slider_init(ui_slider_t* s, const char* label, fp32_t min_w_em,
        int32_t value_min, int32_t value_max,
        void (*callback)(ui_view_t* r)) {
    static_assert(offsetof(ui_slider_t, view) == 0, "offsetof(.view)");
    assert(min_w_em >= 3.0, "allow 1em for each of [-] and [+] buttons");
    s->view.type = ui_view_slider;
    ut_str_printf(s->view.text, "%s", label);
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
#include <Psapi.h>    // both ut_loader.c and ut_processes.c
#include <shellapi.h> // ut_processes.c
#include <winternl.h> // ut_processes.c
#include <initguid.h>     // for knownfolders
#include <KnownFolders.h> // ut_files.c
#include <AclAPI.h>       // ut_files.c
#include <ShlObj_core.h>  // ut_files.c
#include <Shlwapi.h>      // ut_files.c
// ui:
#include <windowsx.h>
#include <commdlg.h>
#include <dwmapi.h>
#include <ShellScalingApi.h>
#include <VersionHelpers.h>

#pragma warning(pop)

#include <fcntl.h>

#define ut_export __declspec(dllexport)

#define b2e(call) ((errno_t)(call ? 0 : GetLastError())) // BOOL -> errno_t



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
        traceln("RegGetValueA(%s\\%s) failed %s", path, key, strerr(r));
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
    if ((!ui_app.dark_mode && !ui_app.light_mode) ||
        ( ui_app.dark_mode &&  ui_app.light_mode)) {
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
                "failed %s", strerr(r));
    }
    ui_app.request_layout();
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

static int ui_toggle_paint_on_off(ui_view_t* v) {
    ui_gdi.push(v->x, v->y);
    ui_color_t b = v->background;
    if (!ui_theme.are_apps_dark()) { b = ui_colors.darken(b, 0.25f); }
    ui_color_t background = v->pressed ? ui_colors.tone_green : b;
    ui_gdi.set_text_color(background);
    int32_t x = v->x;
    int32_t x1 = v->x + v->fm->em.w * 3 / 4;
    while (x < x1) {
        ui_gdi.x = x;
        ui_gdi.text("%s", ut_glyph_black_large_circle);
        x++;
    }
    int32_t rx = ui_gdi.x;
    ui_gdi.x = v->pressed ? x : v->x;
    ui_color_t c = ui_gdi.set_text_color(v->color);
    ui_gdi.text("%s", ut_glyph_black_large_circle);
    ui_gdi.set_text_color(c);
    ui_gdi.pop();
    return rx;
}

static const char* ui_toggle_on_off_label(ui_view_t* v,
        char* label, int32_t count)  {
    ut_str.format(label, count, "%s", ui_view.nls(v));
    char* s = strstr(label, "___");
    if (s != null) {
        memcpy(s, v->pressed ? "On " : "Off", 3);
    }
    return ut_nls.str(label);
}

static void ui_toggle_measured(ui_view_t* v) {
    assert(v->type == ui_view_toggle);
    v->w += v->fm->em.w * 2;
}

static void ui_toggle_paint(ui_view_t* v) {
    assert(v->type == ui_view_toggle);
    char text[countof(v->text)];
    const char* label = ui_toggle_on_off_label(v, text, countof(text));
    ui_gdi.push(v->x, v->y);
    ui_font_t f = ui_gdi.set_font(v->fm->font);
    ui_gdi.x = ui_toggle_paint_on_off(v) + v->fm->em.w * 3 / 4;
    ui_color_t c = ui_gdi.set_text_color(v->color);
    ui_gdi.text("%s", label);
    ui_gdi.set_text_color(c);
    ui_gdi.set_font(f);
    ui_gdi.pop();
}

static void ui_toggle_flip(ui_toggle_t* t) {
    assert(t->type == ui_view_toggle);
    ui_app.request_redraw();
    t->pressed = !t->pressed;
    if (t->callback != null) { t->callback(t); }
}

static void ui_toggle_character(ui_view_t* v, const char* utf8) {
    assert(v->type == ui_view_toggle);
    assert(!v->hidden && !v->disabled);
    char ch = utf8[0];
    if (ui_view.is_shortcut_key(v, ch)) {
         ui_toggle_flip((ui_toggle_t*)v);
    }
}

static void ui_toggle_key_pressed(ui_view_t* v, int64_t key) {
    if (ui_app.alt && ui_view.is_shortcut_key(v, key)) {
//      traceln("key: 0x%02X shortcut: %d", key, ui_view.is_shortcut_key(view, key));
        ui_toggle_flip((ui_toggle_t*)v);
    }
}

static void ui_toggle_mouse(ui_view_t* v, int32_t message, int64_t unused(flags)) {
    assert(v->type == ui_view_toggle);
    assert(!v->hidden && !v->disabled);
    if (message == ui.message.left_button_pressed ||
        message == ui.message.right_button_pressed) {
        int32_t x = ui_app.mouse.x - v->x;
        int32_t y = ui_app.mouse.y - v->y;
        if (0 <= x && x < v->w && 0 <= y && y < v->h) {
            ui_app.focus = v;
            ui_toggle_flip((ui_toggle_t*)v);
        }
    }
}

void ui_view_init_toggle(ui_view_t* v) {
    assert(v->type == ui_view_toggle);
    ui_view.set_text(v, v->text);
    v->mouse         = ui_toggle_mouse;
    v->paint         = ui_toggle_paint;
    v->measured = ui_toggle_measured;
    v->character     = ui_toggle_character;
    v->key_pressed   = ui_toggle_key_pressed;
    v->color_id      = ui_color_id_button_text;
    v->background_id = ui_color_id_button_face;
    ui_view.localize(v);
}

void ui_toggle_init(ui_toggle_t* t, const char* label, fp32_t ems,
       void (*callback)(ui_toggle_t* b)) {
    ut_str_printf(t->text, "%s", label);
    t->min_w_em = ems;
    t->callback = callback;
    t->type = ui_view_toggle;
    ui_view_init_toggle(t);
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
    static_assertion(('vwXX' & 0xFFFF0000U) == ('vwZZ' & 0xFFFF0000U));
    static_assertion((ui_view_container & 0xFFFF0000U) == ('vwXX' & 0xFFFF0000U));
    swear(((uint32_t)v->type & 0xFFFF0000U) == ('vwXX'  & 0xFFFF0000U),
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
    ui_app.request_layout();
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
    ui_app.request_layout();
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
    ui_app.request_layout();
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
    ui_app.request_layout();
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
    ui_app.request_layout();
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
    ui_app.request_layout();
}

static void ui_view_remove_all(ui_view_t* p) {
    while (p->child != null) { ui_view.remove(p->child); }
    ui_app.request_layout();
}

static void ui_view_disband(ui_view_t* p) {
    // do not disband composite controls
    if (p->type != ui_view_mbx && p->type != ui_view_slider) {
        while (p->child != null) {
            ui_view_disband(p->child);
            ui_view.remove(p->child);
        }
    }
    ui_app.request_layout();
}

static void ui_view_invalidate(const ui_view_t* v) {
    ui_rect_t rc = { v->x, v->y, v->w, v->h};
    rc.x -= v->fm->em.w;
    rc.y -= v->fm->em.h;
    rc.w += v->fm->em.w * 2;
    rc.h += v->fm->em.h * 2;
    ui_app.invalidate(&rc);
}

static const char* ui_view_nls(ui_view_t* v) {
    return v->strid != 0 ? ut_nls.string(v->strid, v->text) : v->text;
}

static void ui_measure_view(ui_view_t* v) {
    ui_ltrb_t i = ui_view.gaps(v, &v->insets);
//  ui_ltrb_t p = ui_view.gaps(v, &v->padding);
//  traceln(">%s %d,%d %dx%d p: %d %d %d %d  i: %d %d %d %d",
//               v->text, v->x, v->y, v->w, v->h,
//               p.left, p.top, p.right, p.bottom,
//               i.left, i.top, i.right, i.bottom);
    if (v->text[0] != 0) {
        v->w = (int32_t)((fp64_t)v->fm->em.w * (fp64_t)v->min_w_em + 0.5);
        ui_wh_t mt = { 0 };
        bool multiline = strchr(v->text, '\n') != null;
        if (v->type == ui_view_label && multiline) {
            int32_t w = (int32_t)((fp64_t)v->min_w_em * (fp64_t)v->fm->em.w + 0.5);
            mt = ui_gdi.measure_multiline(v->fm->font, w == 0 ? -1 : w,
                                          ui_view.nls(v));
        } else {
            mt = ui_gdi.measure_text(v->fm->font, ui_view.nls(v));
        }
//      traceln(" mt: %dx%d", mt.x, mt.y);
        v->w = i.left + ut_max(v->w, mt.w) + i.right;
        v->h = i.top + mt.h + i.bottom;
    } else {
        // TODO: minimum view 1x1 em?
        v->w = i.left + v->fm->em.w + i.right;
        v->h = i.top + v->fm->em.h + i.bottom;
        v->w = ut_min(v->w, ui.gaps_em2px(v->fm->em.w, v->min_w_em));
        v->h = ut_min(v->h, ui.gaps_em2px(v->fm->em.h, v->min_h_em));
    }
//  traceln("<%s %d,%d %dx%d", v->text, v->x, v->y, v->w, v->h);
}

static void ui_view_measure(ui_view_t* v) {
    if (!ui_view.is_hidden(v)) {
        ui_view_for_each(v, c, { ui_view.measure(c); });
        if (v->prepare != null) { v->prepare(v); }
        if (v->measure != null && v->measure != ui_view_measure) {
            v->measure(v);
        } else {
            ui_measure_view(v);
        }
        if (v->measured != null) { v->measured(v); }
    }
}

static void ui_layout_view(ui_view_t* unused(v)) {
//  ui_ltrb_t i = ui_view.gaps(v, &v->insets);
//  ui_ltrb_t p = ui_view.gaps(v, &v->padding);
//  traceln(">%s %d,%d %dx%d p: %d %d %d %d  i: %d %d %d %d",
//               v->text, v->x, v->y, v->w, v->h,
//               p.left, p.top, p.right, p.bottom,
//               i.left, i.top, i.right, i.bottom);
//  traceln("<%s %d,%d %dx%d", v->text, v->x, v->y, v->w, v->h);
}

static void ui_view_layout(ui_view_t* v) {
//  traceln(">%s %d,%d %dx%d", v->text, v->x, v->y, v->w, v->h);
    if (!ui_view.is_hidden(v)) {
        if (v->layout != null && v->layout != ui_view_layout) {
            v->layout(v);
        } else {
            ui_layout_view(v);
        }
        if (v->composed != null) { v->composed(v); }
        ui_view_for_each(v, c, { ui_view.layout(c); });
    }
//  traceln("<%s %d,%d %dx%d", v->text, v->x, v->y, v->w, v->h);
}

static bool ui_view_inside(ui_view_t* v, const ui_point_t* pt) {
    const int32_t x = pt->x - v->x;
    const int32_t y = pt->y - v->y;
    return 0 <= x && x < v->w && 0 <= y && y < v->h;
}

static ui_ltrb_t ui_view_gaps(ui_view_t* v, const ui_gaps_t* g) {
    return (ui_ltrb_t) {
        .left   = ui.gaps_em2px(v->fm->em.w, g->left),
        .top    = ui.gaps_em2px(v->fm->em.h, g->top),
        .right  = ui.gaps_em2px(v->fm->em.w, g->right),
        .bottom = ui.gaps_em2px(v->fm->em.h, g->bottom)
    };
}

static void ui_view_inbox(ui_view_t* v, ui_rect_t* r, ui_ltrb_t* insets) {
    swear(r != null || insets != null);
    swear(v->max_w >= 0 && v->max_h >= 0);
    const ui_ltrb_t i = ui_view_gaps(v, &v->insets);
    if (insets != null) { *insets = i; }
    if (r != null) {
        *r = (ui_rect_t) {
            .x = v->x + i.left,
            .y = v->y + i.top,
            .w = v->w - i.left - i.right,
            .h = v->h - i.top  - i.bottom,
        };
    }
}

static void ui_view_outbox(ui_view_t* v, ui_rect_t* r, ui_ltrb_t* padding) {
    swear(r != null || padding != null);
    swear(v->max_w >= 0 && v->max_h >= 0);
    const ui_ltrb_t p = ui_view_gaps(v, &v->padding);
    if (padding != null) { *padding = p; }
    if (r != null) {
//      traceln("%s %d,%d %dx%d %.1f %.1f %.1f %.1f", v->text,
//          v->x, v->y, v->w, v->h,
//          v->padding.left, v->padding.top, v->padding.right, v->padding.bottom);
        *r = (ui_rect_t) {
            .x = v->x - p.left,
            .y = v->y - p.top,
            .w = v->w + p.left + p.right,
            .h = v->h + p.top  + p.bottom,
        };
//      traceln("%s %d,%d %dx%d", v->text,
//          r->x, r->y, r->w, r->h);
    }
}

static void ui_view_position_by_outbox(ui_view_t* v, const ui_rect_t* r,
            const ui_ltrb_t* padding) {
    v->x = r->x + padding->left;
    v->y = r->y + padding->top;
    v->w = r->w - padding->left - padding->right;
    v->h = r->h - padding->top  - padding->bottom;
}

static void ui_view_set_text(ui_view_t* v, const char* text) {
    int32_t n = (int32_t)strlen(text);
    ut_str_printf(v->text, "%s", text);
    v->strid = 0; // next call to nls() will localize this text
    for (int32_t i = 0; i < n; i++) {
        if (text[i] == '&' && i < n - 1 && text[i + 1] != '&') {
            v->shortcut = text[i + 1];
            break;
        }
    }
}

static void ui_view_localize(ui_view_t* v) {
    if (v->text[0] != 0) {
        v->strid = ut_nls.strid(v->text);
    }
}

static void ui_view_show_hint(ui_view_t* v, ui_view_t* hint) {
    ui_view_call_init(hint);
    ut_str_printf(hint->text, "%s", ut_nls.str(v->hint));
    ui_view.measure(hint);
    int32_t x = v->x + v->w / 2 - hint->w / 2 + hint->fm->em.w / 4;
    int32_t y = v->y + v->h + v->fm->em.h / 2 + hint->fm->em.h / 4;
    if (x + hint->w > ui_app.crc.w) { x = ui_app.crc.w - hint->w - hint->fm->em.w / 2; }
    if (x < 0) { x = hint->fm->em.w / 2; }
    if (y + hint->h > ui_app.crc.h) { y = ui_app.crc.h - hint->h - hint->fm->em.h / 2; }
    if (y < 0) { y = hint->fm->em.h / 2; }
    // show_tooltip will center horizontally
    ui_app.show_tooltip(hint, x + hint->w / 2, y, 0);
}

static void ui_view_hovering(ui_view_t* v, bool start) {
    static ui_label_t hint = ui_label(0.0, "");
    if (start && ui_app.animating.view == null && v->hint[0] != 0 &&
       !ui_view.is_hidden(v)) {
        ui_view_show_hint(v, &hint);
    } else if (!start && ui_app.animating.view == &hint) {
        ui_app.show_tooltip(null, -1, -1, 0);
    }
}

static bool ui_view_is_shortcut_key(ui_view_t* v, int64_t key) {
    // Supported keyboard shortcuts are ASCII characters only for now
    // If there is not focused UI control in Alt+key [Alt] is optional.
    // If there is focused control only Alt+Key is accepted as shortcut
    char ch = 0x20 <= key && key <= 0x7F ? (char)toupper((char)key) : 0x00;
    bool need_alt = ui_app.focus != null && ui_app.focus != v;
    bool keyboard_shortcut = ch != 0x00 && v->shortcut != 0x00 &&
         (ui_app.alt || !need_alt) && toupper(v->shortcut) == ch;
    return keyboard_shortcut;
}

static bool ui_view_is_hidden(const ui_view_t* v) {
    bool hidden = v->hidden;
    while (!hidden && v->parent != null) {
        v = v->parent;
        hidden = v->hidden;
    }
    return hidden;
}

static bool ui_view_is_disabled(const ui_view_t* v) {
    bool disabled = v->disabled;
    while (!disabled && v->parent != null) {
        v = v->parent;
        disabled = v->disabled;
    }
    return disabled;
}

static void ui_view_timer(ui_view_t* v, ui_timer_t id) {
    if (v->timer != null) { v->timer(v, id); }
    // timers are delivered even to hidden and disabled views:
    ui_view_for_each(v, c, { ui_view_timer(c, id); });
}

static void ui_view_every_sec(ui_view_t* v) {
    if (v->every_sec != null) { v->every_sec(v); }
    ui_view_for_each(v, c, { ui_view_every_sec(c); });
}

static void ui_view_every_100ms(ui_view_t* v) {
    if (v->every_100ms != null) { v->every_100ms(v); }
    ui_view_for_each(v, c, { ui_view_every_100ms(c); });
}

static void ui_view_key_pressed(ui_view_t* v, int64_t k) {
    if (!ui_view.is_hidden(v) && !ui_view.is_disabled(v)) {
        if (v->key_pressed != null) { v->key_pressed(v, k); }
        ui_view_for_each(v, c, { ui_view_key_pressed(c, k); });
    }
}

static void ui_view_key_released(ui_view_t* v, int64_t k) {
    if (!ui_view.is_hidden(v) && !ui_view.is_disabled(v)) {
        if (v->key_released != null) { v->key_released(v, k); }
        ui_view_for_each(v, c, { ui_view_key_released(c, k); });
    }
}

static void ui_view_character(ui_view_t* v, const char* utf8) {
    if (!ui_view.is_hidden(v) && !ui_view.is_disabled(v)) {
        if (v->character != null) { v->character(v, utf8); }
        ui_view_for_each(v, c, { ui_view_character(c, utf8); });
    }
}

static void ui_view_resolve_color_ids(ui_view_t* v) {
    if (v->color_id > 0) {
        v->color = ui_app.get_color(v->color_id);
    }
    if (v->background_id > 0) {
        v->background = ui_app.get_color(v->background_id);
    }
}

static void ui_view_paint(ui_view_t* v) {
    assert(ui_app.crc.w > 0 && ui_app.crc.h > 0);
    ui_view_resolve_color_ids(v);
    if (!v->hidden && ui_app.crc.w > 0 && ui_app.crc.h > 0) {
        if (v->paint != null) { v->paint(v); }
        if (v->painted != null) { v->painted(v); }
        if (v->debug_paint != null && v->debug) { v->debug_paint(v); }
        if (v->debug) { ui_view.debug_paint(v); }
        ui_view_for_each(v, c, { ui_view_paint(c); });
    }
}

static bool ui_view_set_focus(ui_view_t* v) {
    bool set = false;
   ui_view_for_each(v, c, {
        set = ui_view_set_focus(c);
        if (set) { break; }
    });
    if (!set && !ui_view.is_hidden(v) && !ui_view.is_disabled(v) &&
        v->focusable && v->set_focus != null &&
       (ui_app.focus == v || ui_app.focus == null)) {
        set = v->set_focus(v);
    }
    return set;
}

static void ui_view_kill_focus(ui_view_t* v) {
    ui_view_for_each(v, c, { ui_view_kill_focus(c); });
    if (v->kill_focus != null && v->focusable) {
        v->kill_focus(v);
    }
}

static int64_t ui_view_hit_test(ui_view_t* v, int32_t cx, int32_t cy) {
    int64_t ht = ui.hit_test.nowhere;
    if (!ui_view.is_hidden(v) && v->hit_test != null) {
         ht = v->hit_test(v, cx, cy);
    }
    if (ht == ui.hit_test.nowhere) {
        ui_view_for_each(v, c, {
            if (!c->hidden) {
                ht = ui_view_hit_test(c, cx, cy);
                if (ht != ui.hit_test.nowhere) { break; }
            }
        });
    }
    return ht;
}

static void ui_view_mouse(ui_view_t* v, int32_t m, int64_t f) {
    if (!ui_view.is_hidden(v) &&
       (m == ui.message.mouse_hover || m == ui.message.mouse_move)) {
        ui_rect_t r = { v->x, v->y, v->w, v->h};
        bool hover = v->hover;
        v->hover = ui.point_in_rect(&ui_app.mouse, &r);
        // inflate view rectangle:
        r.x -= v->w / 4;
        r.y -= v->h / 4;
        r.w += v->w / 2;
        r.h += v->h / 2;
        if (hover != v->hover) { ui_app.invalidate(&r); }
        if (hover != v->hover) {
            ui_view.hover_changed(v);
        }
    }
    if (!ui_view.is_hidden(v) && !ui_view.is_disabled(v)) {
        if (v->mouse != null) { v->mouse(v, m, f); }
        ui_view_for_each(v, c, { ui_view_mouse(c, m, f); });
    }
}

static void ui_view_mouse_wheel(ui_view_t* v, int32_t dx, int32_t dy) {
    if (!ui_view.is_hidden(v) && !ui_view.is_disabled(v)) {
        if (v->mouse_wheel != null) { v->mouse_wheel(v, dx, dy); }
        ui_view_for_each(v, c, { ui_view_mouse_wheel(c, dx, dy); });
    }
}

static void ui_view_hover_changed(ui_view_t* v) {
    if (!v->hidden) {
        if (!v->hover) {
            v->hover_when = 0;
            ui_view.hovering(v, false); // cancel hover
        } else {
            swear(ui_view_hover_delay >= 0);
            if (v->hover_when >= 0) {
                v->hover_when = ui_app.now + ui_view_hover_delay;
            }
        }
    }
}

static void ui_view_kill_hidden_focus(ui_view_t* v) {
    // removes focus from hidden or disabled ui controls
    if (ui_app.focus != null) {
        if (ui_app.focus == v && (v->disabled || v->hidden)) {
            ui_app.focus = null;
            // even for disabled or hidden view notify about kill_focus:
            v->kill_focus(v);
        } else {
            ui_view_for_each(v, c, { ui_view_kill_hidden_focus(c); });
        }
    }
}

static bool ui_view_tap(ui_view_t* v, int32_t ix) { // 0: left 1: middle 2: right
    bool done = false; // consumed
    if (!ui_view.is_hidden(v) && !ui_view.is_disabled(v) &&
        ui_view_inside(v, &ui_app.mouse)) {
        ui_view_for_each(v, c, {
            done = ui_view_tap(c, ix);
            if (done) { break; }
        });

        if (v->tap != null && !done) { done = v->tap(v, ix); }
    }
    return done;
}

static bool ui_view_press(ui_view_t* v, int32_t ix) { // 0: left 1: middle 2: right
    bool done = false; // consumed
    if (!ui_view.is_hidden(v) && !ui_view.is_disabled(v)) {
        ui_view_for_each(v, c, {
            done = ui_view_press(c, ix);
            if (done) { break; }
        });
        if (v->press != null && !done) { done = v->press(v, ix); }
    }
    return done;
}

static bool ui_view_context_menu(ui_view_t* v) {
    if (!ui_view.is_hidden(v) && !ui_view.is_disabled(v)) {
        ui_view_for_each(v, c, {
            if (ui_view_context_menu(c)) { return true; }
        });
        ui_rect_t r = { v->x, v->y, v->w, v->h};
        if (ui.point_in_rect(&ui_app.mouse, &r)) {
            if (!v->hidden && !v->disabled && v->context_menu != null) {
                v->context_menu(v);
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
    bool container = v->type == ui_view_container ||
                     v->type == ui_view_span ||
                     v->type == ui_view_list ||
                     v->type == ui_view_spacer;
    if (v->type == ui_view_spacer) {
        ui_gdi.fill_with(v->x, v->y, v->w, v->h, ui_rgb(128, 128, 128));
    } else if (container && v->color != ui_colors.transparent) {
//      traceln("%s 0x%08X", v->text, v->color);
        ui_gdi.fill_with(v->x, v->y, v->w, v->h, v->background);
    }
    ui_ltrb_t p = ui_view.gaps(v, &v->padding);
    ui_ltrb_t i = ui_view.gaps(v, &v->insets);
    if (p.left   > 0) { ui_gdi.frame_with(v->x - p.left, v->y, p.left, v->h, ui_colors.green); }
    if (p.right  > 0) { ui_gdi.frame_with(v->x + v->w, v->y, p.right, v->h, ui_colors.green); }
    if (p.top    > 0) { ui_gdi.frame_with(v->x, v->y - p.top, v->w, p.top, ui_colors.green); }
    if (p.bottom > 0) { ui_gdi.frame_with(v->x, v->y + v->h, v->w, p.bottom, ui_colors.green); }
    if (i.left  > 0)  { ui_gdi.frame_with(v->x, v->y,               i.left, v->h, ui_colors.orange); }
    if (i.right > 0)  { ui_gdi.frame_with(v->x + v->w - i.right, v->y, i.right, v->h, ui_colors.orange); }
    if (i.top   > 0)  { ui_gdi.frame_with(v->x, v->y,            v->w, i.top, ui_colors.orange); }
    if (i.bottom > 0) { ui_gdi.frame_with(v->x, v->y + v->h - i.bottom, v->w, i.bottom, ui_colors.orange); }
    if (container && v->w > 0 && v->h > 0 && v->color != ui_colors.transparent) {
        ui_gdi.set_text_color(ui_color_rgb(v->color) ^ 0xFFFFFF);
        ui_wh_t mt = ui_gdi.measure_text(v->fm->font, v->text);
        ui_gdi.x += (v->w - mt.w) / 2;
        ui_gdi.y += (v->h - mt.h) / 2;
        ui_font_t f = ui_gdi.set_font(v->fm->font);
        ui_gdi.text("%s", v->text);
        ui_gdi.set_font(f);
    }
    ui_gdi.pop();
}

#pragma push_macro("ui_view_no_siblings")

#define ui_view_no_siblings(v) do {                    \
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
    ui_view_no_siblings(&p0);
    ui_view_no_siblings(&c1);
    ui_view_no_siblings(&c4);
    ui_view.remove(&g1);                            ui_view_verify(&c2);
    ui_view.remove(&g2);                            ui_view_verify(&c2);
    ui_view.remove(&g3);                            ui_view_verify(&c3);
    ui_view.remove(&g4);                            ui_view_verify(&c3);
    ui_view_no_siblings(&c2); ui_view_no_siblings(&c3);
    ui_view_no_siblings(&g1); ui_view_no_siblings(&g2);
    ui_view_no_siblings(&g3); ui_view_no_siblings(&g4);
    // a bit more intuitive (for a human) nested way to initialize tree:
    ui_view.add(&p0,
        &c1,
        ui_view.add(&c2, &g1, &g2, null),
        ui_view.add(&c3, &g3, &g4, null),
        &c4);
    ui_view_verify(&p0);
    ui_view_disband(&p0);
    ui_view_no_siblings(&p0);
    ui_view_no_siblings(&c1); ui_view_no_siblings(&c2);
    ui_view_no_siblings(&c3); ui_view_no_siblings(&c4);
    ui_view_no_siblings(&g1); ui_view_no_siblings(&g2);
    ui_view_no_siblings(&g3); ui_view_no_siblings(&g4);
    if (ut_debug.verbosity.level > ut_debug.verbosity.quiet) { traceln("done"); }
}

#pragma pop_macro("ui_view_no_siblings")

ui_view_if ui_view = {
    .add                = ui_view_add,
    .add_first          = ui_view_add_first,
    .add_last           = ui_view_add_last,
    .add_after          = ui_view_add_after,
    .add_before         = ui_view_add_before,
    .remove             = ui_view_remove,
    .remove_all         = ui_view_remove_all,
    .disband            = ui_view_disband,
    .inside             = ui_view_inside,
    .gaps               = ui_view_gaps,
    .inbox              = ui_view_inbox,
    .outbox             = ui_view_outbox,
    .position_by_outbox = ui_view_position_by_outbox,
    .set_text           = ui_view_set_text,
    .invalidate         = ui_view_invalidate,
    .measure            = ui_view_measure,
    .layout             = ui_view_layout,
    .nls                = ui_view_nls,
    .localize           = ui_view_localize,
    .is_hidden          = ui_view_is_hidden,
    .is_disabled        = ui_view_is_disabled,
    .timer              = ui_view_timer,
    .every_sec          = ui_view_every_sec,
    .every_100ms        = ui_view_every_100ms,
    .hit_test           = ui_view_hit_test,
    .key_pressed        = ui_view_key_pressed,
    .key_released       = ui_view_key_released,
    .character          = ui_view_character,
    .paint              = ui_view_paint,
    .set_focus          = ui_view_set_focus,
    .kill_focus         = ui_view_kill_focus,
    .kill_hidden_focus  = ui_view_kill_hidden_focus,
    .mouse              = ui_view_mouse,
    .mouse_wheel        = ui_view_mouse_wheel,
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

