#ifndef ui_definition
#define ui_definition

// ___________________________________ ui.h ___________________________________

// alphabetical order is not possible because of headers interdependencies
// _________________________________ ut_std.h _________________________________

#include <ctype.h>
#include <errno.h>
#include <float.h>
#include <limits.h>
#include <locale.h>
#include <malloc.h>
#include <signal.h>
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

#ifdef _MSC_VER
    #define ut_suppress_constant_cond_exp _Pragma("warning(suppress: 4127)")
#else
    #define ut_suppress_constant_cond_exp
#endif

// Type aliases for floating-point types similar to <stdint.h>
typedef float  fp32_t;
typedef double fp64_t;
// "long fp64_t" is required by C standard but the bitness
// of it is not specified.

#ifdef __cplusplus
    #define ut_begin_c extern "C" {
    #define ut_end_c } // extern "C"
#else
    #define ut_begin_c // C headers compiled as C++
    #define ut_end_c
#endif

// ut_countof() and ut_countof() are suitable for
// small < 2^31 element arrays

#define ut_countof(a) ((int32_t)((int)(sizeof(a) / sizeof((a)[0]))))

#if defined(__GNUC__) || defined(__clang__)
    #define ut_force_inline __attribute__((always_inline))
#elif defined(_MSC_VER)
    #define ut_force_inline __forceinline
#endif

#ifndef __cplusplus
    #define null ((void*)0) // better than NULL which is zero
#else
    #define null nullptr
#endif

#if defined(_MSC_VER)
    #define ut_thread_local __declspec(thread)
#else
    #ifndef __cplusplus
        #define ut_thread_local _Thread_local // C99
    #else
        // C++ supports ut_thread_local keyword
    #endif
#endif

// ut_begin_packed ut_end_packed
// usage: typedef ut_begin_packed struct foo_s { ... } ut_end_packed foo_t;

#if defined(__GNUC__) || defined(__clang__)
#define ut_attribute_packed __attribute__((packed))
#define ut_begin_packed
#define ut_end_packed ut_attribute_packed
#else
#define ut_begin_packed ut_pragma( pack(push, 1) )
#define ut_end_packed ut_pragma( pack(pop) )
#define ut_attribute_packed
#endif

// usage: typedef struct ut_aligned_8 foo_s { ... } foo_t;

#if defined(__GNUC__) || defined(__clang__)
#define ut_aligned_8 __attribute__((aligned(8)))
#elif defined(_MSC_VER)
#define ut_aligned_8 __declspec(align(8))
#else
#define ut_aligned_8
#endif


// In callbacks the formal parameters are
// frequently unused. Also sometimes parameters
// are used in debug configuration only (e.g. ut_assert() checks)
// but not in release.
// C does not have anonymous parameters like C++
// Instead of:
//      void foo(param_type_t param) { (void)param; / *unused */ }
// use:
//      vod foo(param_type_t ut_unused(param)) { }

#if defined(__GNUC__) || defined(__clang__)
#define ut_unused(name) name __attribute__((unused))
#elif defined(_MSC_VER)
#define ut_unused(name) _Pragma("warning(suppress:  4100)") name
#else
#define ut_unused(name) name
#endif

// Because MS C compiler is unhappy about alloca() and
// does not implement (C99 optional) dynamic arrays on the stack:

#define ut_stackalloc(n) (_Pragma("warning(suppress: 6255 6263)") alloca(n))

// alloca() is messy and in general is a not a good idea.
// try to avoid if possible. Stack sizes vary from 64KB to 8MB in 2024.
// ________________________________ ui_core.h _________________________________

#include "ut/ut_std.h"

ut_begin_c

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

// *)  .em square pixel size of glyph "m"
//     https://en.wikipedia.org/wiki/Em_(typography)

typedef uintptr_t ui_timer_t; // timer not the same as "id" in set_timer()!

typedef struct ui_image_s { // TODO: ui_ namespace
    int32_t w; // width
    int32_t h; // height
    int32_t bpp;    // "components" bytes per pixel
    int32_t stride; // bytes per scanline rounded up to: (w * bpp + 3) & ~3
    ui_bitmap_t bitmap;
    void* pixels;
} ui_image_t;

// ui_margins_t are used for padding and insets and expressed
// in partial "em"s not in pixels, inches or points.
// Pay attention that "em" is not square. "M" measurement
// for most fonts are em.w = 0.5 * em.h

typedef struct ui_gaps_s { // in partial "em"s
    fp32_t left;
    fp32_t top;
    fp32_t right;
    fp32_t bottom;
} ui_margins_t;

typedef struct ui_s {
    bool (*point_in_rect)(const ui_point_t* p, const ui_rect_t* r);
    // intersect_rect(null, r0, r1) and intersect_rect(r0, r0, r1) supported.
    bool (*intersect_rect)(ui_rect_t* destination, const ui_rect_t* r0,
                                                   const ui_rect_t* r1);
    ui_rect_t (*combine_rect)(const ui_rect_t* r0, const ui_rect_t* r1);
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
    // TODO: remove or move inside app
    struct { // message:
        int32_t const animate;
        int32_t const opening;
        int32_t const closing;
   } const message;
   // TODO: remove or move inside app
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
    struct {
        int32_t const ok;
        int32_t const info;
        int32_t const question;
        int32_t const warning;
        int32_t const error;
    } beep;
} ui_if;

extern ui_if ui;

// ui_margins_t in "em"s:
//
// The reason is that UI fonts may become larger smaller
// for accessibility reasons with the same display
// density in DPIs. Humanoid would expect the margins around
// larger font text to grow with font size increase.
// SwingUI and MacOS is using "pt" for padding which does
// not account to font size changes. MacOS does weird stuff
// with font increase - it actually decreases GPU resolution.
// Android uses "dp" which is pretty much the same as scaled
// "pixels" on MacOS. Windows used to use "dialog units" which
// is font size based and this is where the idea is inherited from.

ut_end_c


// _______________________________ ui_colors.h ________________________________

ut_begin_c

typedef uint64_t ui_color_t; // top 2 bits determine color format

/* TODO: make ui_color_t uint64_t RGBA or better yet fp32_t RGBA
         support upto 16-16-16-14(A)bit per pixel color
         components with 'transparent' aka 'hollow' bit
*/

#define ui_color_mask        ((ui_color_t)0xC000000000000000ULL)
#define ui_color_undefined   ((ui_color_t)0x8000000000000000ULL)
#define ui_color_transparent ((ui_color_t)0x4000000000000000ULL)
#define ui_color_hdr         ((ui_color_t)0xC000000000000000ULL)

#define ui_color_is_8bit(c)        ( ((c) &  ui_color_mask) == 0)
#define ui_color_is_hdr(c)         ( ((c) &  ui_color_mask) == ui_color_hdr)
#define ui_color_is_undefined(c)   ( ((c) &  ui_color_mask) == ui_color_undefined)
#define ui_color_is_transparent(c) ((((c) &  ui_color_mask) == ui_color_transparent) && \
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

#define ui_color_is_rgb(c)   ((uint32_t)( (c) & 0x00FFFFFFU))
#define ui_color_is_rgba(c)  ((uint32_t)( (c) & 0xFFFFFFFFU))
#define ui_color_is_rgbFF(c) ((uint32_t)(((c) & 0x00FFFFFFU)) | 0xFF000000U)

#define ui_color_rgb(r, g, b) ((ui_color_t)(                     \
                              (((uint32_t)(uint8_t)(r))      ) | \
                              (((uint32_t)(uint8_t)(g)) <<  8) | \
                              (((uint32_t)(uint8_t)(b)) << 16)))


#define ui_color_rgba(r, g, b, a)                     \
    ( (ui_color_t)(                                   \
      (ui_color_rgb(r, g, b)) |                       \
      ((ui_color_t)((uint32_t)((uint8_t)(a))) << 24)) \
    )

enum {
    ui_color_id_undefined           =  0,
    ui_color_id_active_title        =  1,
    ui_color_id_button_face         =  2,
    ui_color_id_button_text         =  3,
    ui_color_id_gray_text           =  4,
    ui_color_id_highlight           =  5,
    ui_color_id_highlight_text      =  6,
    ui_color_id_hot_tracking        =  7,
    ui_color_id_inactive_title      =  8,
    ui_color_id_inactive_title_text =  9,
    ui_color_id_menu_highlight      = 10,
    ui_color_id_title_text          = 11,
    ui_color_id_window              = 12,
    ui_color_id_window_text         = 13,
    ui_color_id_accent              = 14
};

typedef struct ui_control_colors_s {
    ui_color_t text;
    ui_color_t background;
    ui_color_t border;
    ui_color_t accent; // aka highlight
    ui_color_t gradient_top;
    ui_color_t gradient_bottom;
} control_colors_t;

typedef struct ui_control_state_colors_s {
    control_colors_t disabled;
    control_colors_t enabled;
    control_colors_t hover;
    control_colors_t armed;
    control_colors_t pressed;
} ui_control_state_colors_t;

typedef struct ui_colors_s {
    ui_color_t (*get_color)(int32_t color_id); // ui.colors.*
    void       (*rgb_to_hsi)(fp64_t r, fp64_t g, fp64_t b, fp64_t *h, fp64_t *s, fp64_t *i);
    ui_color_t (*hsi_to_rgb)(fp64_t h, fp64_t s, fp64_t i,  uint8_t a);
    // interpolate():
    //    0.0 < multiplier < 1.0 excluding boundaries
    //    alpha is interpolated as well
    ui_color_t (*interpolate)(ui_color_t c0, ui_color_t c1, fp32_t multiplier);
    ui_color_t (*gray_with_same_intensity)(ui_color_t c);
    // multiplier ]0.0..1.0] excluding zero
    // lighten() and darken() ignore alpha (use interpolate for alpha colors)
    ui_color_t (*lighten)(ui_color_t rgb, fp32_t multiplier); // interpolate toward white
    ui_color_t (*darken)(ui_color_t  rgb, fp32_t multiplier); // interpolate toward black
    ui_color_t (*adjust_saturation)(ui_color_t c,   fp32_t multiplier);
    ui_color_t (*multiply_brightness)(ui_color_t c, fp32_t multiplier);
    ui_color_t (*multiply_saturation)(ui_color_t c, fp32_t multiplier);
    ui_control_state_colors_t* controls; // colors for UI controls
    ui_color_t const transparent;
    ui_color_t const none; // aka CLR_INVALID in wingdi.h
    ui_color_t const text;
    ui_color_t const white;
    ui_color_t const black;
    ui_color_t const red;
    ui_color_t const green;
    ui_color_t const blue;
    ui_color_t const yellow;
    ui_color_t const cyan;
    ui_color_t const magenta;
    ui_color_t const gray;
    // tone down RGB colors:
    ui_color_t const tone_white;
    ui_color_t const tone_red;
    ui_color_t const tone_green;
    ui_color_t const tone_blue;
    ui_color_t const tone_yellow;
    ui_color_t const tone_cyan;
    ui_color_t const tone_magenta;
    // miscellaneous:
    ui_color_t const orange;
    ui_color_t const dark_green;
    ui_color_t const pink;
    ui_color_t const ochre;
    ui_color_t const gold;
    ui_color_t const teal;
    ui_color_t const wheat;
    ui_color_t const tan;
    ui_color_t const brown;
    ui_color_t const maroon;
    ui_color_t const barbie_pink;
    ui_color_t const steel_pink;
    ui_color_t const salmon_pink;
    ui_color_t const gainsboro;
    ui_color_t const light_gray;
    ui_color_t const silver;
    ui_color_t const dark_gray;
    ui_color_t const dim_gray;
    ui_color_t const light_slate_gray;
    ui_color_t const slate_gray;
    /* Named colors */
    /* Main Panel Backgrounds */
    ui_color_t const ennui_black; // rgb(18, 18, 18) 0x121212
    ui_color_t const charcoal;
    ui_color_t const onyx;
    ui_color_t const gunmetal;
    ui_color_t const jet_black;
    ui_color_t const outer_space;
    ui_color_t const eerie_black;
    ui_color_t const oil;
    ui_color_t const black_coral;
    ui_color_t const obsidian;
    /* Secondary Panels or Sidebars */
    ui_color_t const raisin_black;
    ui_color_t const dark_charcoal;
    ui_color_t const dark_jungle_green;
    ui_color_t const pine_tree;
    ui_color_t const rich_black;
    ui_color_t const eclipse;
    ui_color_t const cafe_noir;
    /* Flat Buttons */
    ui_color_t const prussian_blue;
    ui_color_t const midnight_green;
    ui_color_t const charleston_green;
    ui_color_t const rich_black_fogra;
    ui_color_t const dark_liver;
    ui_color_t const dark_slate_gray;
    ui_color_t const black_olive;
    ui_color_t const cadet;
    /* Button highlights (hover) */
    ui_color_t const dark_sienna;
    ui_color_t const bistre_brown;
    ui_color_t const dark_puce;
    ui_color_t const wenge;
    /* Raised button effects */
    ui_color_t const dark_scarlet;
    ui_color_t const burnt_umber;
    ui_color_t const caput_mortuum;
    ui_color_t const barn_red;
    /* Text and Icons */
    ui_color_t const platinum;
    ui_color_t const anti_flash_white;
    ui_color_t const silver_sand;
    ui_color_t const quick_silver;
    /* Links and Selections */
    ui_color_t const dark_powder_blue;
    ui_color_t const sapphire_blue;
    ui_color_t const international_klein_blue;
    ui_color_t const zaffre;
    /* Additional Colors */
    ui_color_t const fish_belly;
    ui_color_t const rusty_red;
    ui_color_t const falu_red;
    ui_color_t const cordovan;
    ui_color_t const dark_raspberry;
    ui_color_t const deep_magenta;
    ui_color_t const byzantium;
    ui_color_t const amethyst;
    ui_color_t const wisteria;
    ui_color_t const lavender_purple;
    ui_color_t const opera_mauve;
    ui_color_t const mauve_taupe;
    ui_color_t const rich_lavender;
    ui_color_t const pansy_purple;
    ui_color_t const violet_eggplant;
    ui_color_t const jazzberry_jam;
    ui_color_t const dark_orchid;
    ui_color_t const electric_purple;
    ui_color_t const sky_magenta;
    ui_color_t const brilliant_rose;
    ui_color_t const fuchsia_purple;
    ui_color_t const french_raspberry;
    ui_color_t const wild_watermelon;
    ui_color_t const neon_carrot;
    ui_color_t const burnt_orange;
    ui_color_t const carrot_orange;
    ui_color_t const tiger_orange;
    ui_color_t const giant_onion;
    ui_color_t const rust;
    ui_color_t const copper_red;
    ui_color_t const dark_tangerine;
    ui_color_t const bright_marigold;
    ui_color_t const bone;
    /* Earthy Tones */
    ui_color_t const sienna;
    ui_color_t const sandy_brown;
    ui_color_t const golden_brown;
    ui_color_t const camel;
    ui_color_t const burnt_sienna;
    ui_color_t const khaki;
    ui_color_t const dark_khaki;
    /* Greens */
    ui_color_t const fern_green;
    ui_color_t const moss_green;
    ui_color_t const myrtle_green;
    ui_color_t const pine_green;
    ui_color_t const jungle_green;
    ui_color_t const sacramento_green;
    /* Blues */
    ui_color_t const yale_blue;
    ui_color_t const cobalt_blue;
    ui_color_t const persian_blue;
    ui_color_t const royal_blue;
    ui_color_t const iceberg;
    ui_color_t const blue_yonder;
    /* Miscellaneous */
    ui_color_t const cocoa_brown;
    ui_color_t const cinnamon_satin;
    ui_color_t const fallow;
    ui_color_t const cafe_au_lait;
    ui_color_t const liver;
    ui_color_t const shadow;
    ui_color_t const cool_grey;
    ui_color_t const payne_grey;
    /* Lighter Tones for Contrast */
    ui_color_t const timberwolf;
    ui_color_t const silver_chalice;
    ui_color_t const roman_silver;
    /* Dark Mode Specific Highlights */
    ui_color_t const electric_lavender;
    ui_color_t const magenta_haze;
    ui_color_t const cyber_grape;
    ui_color_t const purple_navy;
    ui_color_t const liberty;
    ui_color_t const purple_mountain_majesty;
    ui_color_t const ceil;
    ui_color_t const moonstone_blue;
    ui_color_t const independence;
} ui_colors_if;

extern ui_colors_if ui_colors;

// TODO:
// https://ankiewicz.com/colors/
// https://htmlcolorcodes.com/color-names/
// it would be super cool to implement a plethora of palettes
// with named colors and app "themes" that can be switched

ut_end_c
// _______________________________ ui_fuzzing.h _______________________________

/* Copyright (c) Dmitry "Leo" Kuznetsov 2021-24 see LICENSE for details */
#include "ut/ut.h"
// ___________________________________ ui.h ___________________________________

// alphabetical order is not possible because of headers interdependencies

// _________________________________ ui_gdi.h _________________________________

ut_begin_c

// Graphic Device Interface (selected parts of Windows GDI)

enum {  // TODO: into gdi int32_t const
    ui_gdi_font_quality_default = 0,
    ui_gdi_font_quality_draft = 1,
    ui_gdi_font_quality_proof = 2, // anti-aliased w/o ClearType rainbows
    ui_gdi_font_quality_nonantialiased = 3,
    ui_gdi_font_quality_antialiased = 4,
    ui_gdi_font_quality_cleartype = 5,
    ui_gdi_font_quality_cleartype_natural = 6
};

typedef struct ui_fm_s { // font metrics
    ui_font_t font;
    ui_wh_t em;        // "em" square point size expressed in pixels *)
    // https://learn.microsoft.com/en-us/windows/win32/gdi/string-widths-and-heights
    ui_rect_t box;     // bounding box of the glyphs (doesn't look good in Win32)
    int32_t height;    // font height in pixels
    int32_t baseline;  // bottom of the glyphs sans descenders (align of multi-font text)
    int32_t ascent;    // the maximum glyphs extend above the baseline
    int32_t descent;   // maximum height of descenders
    int32_t x_height;  // small letters height
    int32_t cap_em_height;    // Capital letter "M" height
    int32_t internal_leading; // accents and diacritical marks goes there
    int32_t external_leading;
    int32_t average_char_width;
    int32_t max_char_width;
    int32_t line_gap;  // gap between lines of text
    ui_wh_t subscript; // height
    ui_point_t subscript_offset;
    ui_wh_t superscript;    // height
    ui_point_t superscript_offset;
    int32_t underscore;     // height
    int32_t underscore_position;
    int32_t strike_through; // height
    int32_t strike_through_position;
    bool mono;
} ui_fm_t;

/* see: https://github.com/leok7v/ui/wiki/Typography-Line-Terms
   https://en.wikipedia.org/wiki/Typeface#Font_metrics

   Example em55x55 H1 font @ 192dpi:
    _   _                   _              ___    <- y:0
   (_)_(_)                 | |             ___ /\    "diacritics circumflex"
     / \   __ _ _   _ _ __ | |_ ___ _ __       ||
    / _ \ / _` | | | | '_ \| __/ _ \ '_ \      ||    .ascend:30
   / ___ \ (_| | |_| | |_) | ||  __/ | | |     ||     max extend above baseline
  /_/   \_\__, |\__, | .__/ \__\___|_| |_| ___ || <- .baseline:44
           __/ | __/ | |                       ||    .descend:11
          |___/ |___/|_|                   ___ \/     max height of descenders
                                                  <- .height:55
  em: 55x55
  ascender for "diacritics circumflex" is (h:55 - a:30 - d:11) = 14
*/

typedef struct ui_fms_s {
    // when font handles are re-created on system scaling change
    // metrics "em" and font geometry filled
    ui_fm_t regular; // proportional UI font
    ui_fm_t mono; // monospaced  UI font
    ui_fm_t H1; // bold header font
    ui_fm_t H2;
    ui_fm_t H3;
} ui_fms_t;

// TODO: fonts
#if 0

typedef struct ui_fms_s {
    struct {
        ui_fm_t title;    // largest font
        ui_fm_t subtitle; // larger then `H1`
        ui_fm_t H1;       // largest header font
        ui_fm_t H2;
        ui_fm_t H3;       // larger then `normal`
        ui_fm_t normal;   // larger then `normal`
    } prop; // proportional
    struct {
        ui_fm_t title;    // largest font
        ui_fm_t subtitle; // larger then `H1`
        ui_fm_t H1;       // largest header font
        ui_fm_t H2;
        ui_fm_t H3;       // larger then `normal`
        ui_fm_t normal;   // larger then `normal`
    } mono; // monospaced
} ui_fms_t;

#endif

typedef struct ui_gdi_ta_s { // text attributes
    const ui_fm_t* fm; // font metrics
    int32_t color_id;  // <= 0 use color
    ui_color_t color;  // ui_colors.undefined() use color_id
    bool measure;      // measure only do not draw
} ui_gdi_ta_t;

typedef struct {
    struct {
        ui_gdi_ta_t const regular;
        ui_gdi_ta_t const mono;
        ui_gdi_ta_t const H1;
        ui_gdi_ta_t const H2;
        ui_gdi_ta_t const H3;
    } const ta;
    void (*init)(void);
    void (*fini)(void);
    void (*begin)(ui_image_t* image_or_null);
    // all paint must be done in between
    void (*end)(void);
    // TODO: move to ui_colors
    uint32_t (*color_rgb)(ui_color_t c); // rgb color
    // bpp bytes (not bits!) per pixel. bpp = -3 or -4 does not swap RGB to BRG:
    void (*image_init)(ui_image_t* image, int32_t w, int32_t h, int32_t bpp,
        const uint8_t* pixels);
    void (*image_init_rgbx)(ui_image_t* image, int32_t w, int32_t h,
        int32_t bpp, const uint8_t* pixels); // sets all alphas to 0xFF
    void (*image_dispose)(ui_image_t* image);
    void (*set_clip)(int32_t x, int32_t y, int32_t w, int32_t h);
    // use set_clip(0, 0, 0, 0) to clear clip region
    void (*pixel)(int32_t x, int32_t y, ui_color_t c);
    void (*line)(int32_t x0, int32_t y1, int32_t x2, int32_t y2,
                      ui_color_t c);
    void (*frame)(int32_t x, int32_t y, int32_t w, int32_t h,
                      ui_color_t c);
    void (*rect)(int32_t x, int32_t y, int32_t w, int32_t h,
                      ui_color_t border, ui_color_t fill);
    void (*fill)(int32_t x, int32_t y, int32_t w, int32_t h, ui_color_t c);
    void (*poly)(ui_point_t* points, int32_t count, ui_color_t c);
    void (*circle)(int32_t center_x, int32_t center_y, int32_t odd_radius,
        ui_color_t border, ui_color_t fill);
    void (*rounded)(int32_t x, int32_t y, int32_t w, int32_t h,
        int32_t odd_radius, ui_color_t border, ui_color_t fill);
    void (*gradient)(int32_t x, int32_t y, int32_t w, int32_t h,
        ui_color_t rgba_from, ui_color_t rgba_to, bool vertical);
    // greyscale() sx, sy, sw, sh screen rectangle
    // x, y, w, h rectangle inside pixels[ih][iw] uint8_t array
    void (*greyscale)(int32_t sx, int32_t sy, int32_t sw, int32_t sh,
        int32_t x, int32_t y, int32_t w, int32_t h,
        int32_t iw, int32_t ih, int32_t stride, const uint8_t* pixels);
    void (*bgr)(int32_t sx, int32_t sy, int32_t sw, int32_t sh,
        int32_t x, int32_t y, int32_t w, int32_t h,
        int32_t iw, int32_t ih, int32_t stride, const uint8_t* pixels);
    void (*bgrx)(int32_t sx, int32_t sy, int32_t sw, int32_t sh,
        int32_t x, int32_t y, int32_t w, int32_t h,
        int32_t iw, int32_t ih, int32_t stride, const uint8_t* pixels);
    void (*alpha)(int32_t x, int32_t y, int32_t w, int32_t h,
        ui_image_t* image, fp64_t alpha); // alpha blend image
    void (*image)(int32_t x, int32_t y, int32_t w, int32_t h,
        ui_image_t* image);
    void (*icon)(int32_t x, int32_t y, int32_t w, int32_t h,
        ui_icon_t icon);
    // text:
    void (*cleartype)(bool on); // system wide change: don't use
    void (*font_smoothing_contrast)(int32_t c); // [1000..2202] or -1 for 1400 default
    ui_font_t (*create_font)(const char* family, int32_t height, int32_t quality);
    // custom font, quality: -1 "as is"
    ui_font_t (*font)(ui_font_t f, int32_t height, int32_t quality);
    void      (*delete_font)(ui_font_t f);
    void (*dump_fm)(ui_font_t f); // dump font metrics
    void (*update_fm)(ui_fm_t* fm, ui_font_t f); // fills font metrics
    ui_wh_t (*text_va)(const ui_gdi_ta_t* ta, int32_t x, int32_t y,
        const char* format, va_list va);
    ui_wh_t (*text)(const ui_gdi_ta_t* ta, int32_t x, int32_t y,
        const char* format, ...);
    ui_wh_t (*multiline_va)(const ui_gdi_ta_t* ta, int32_t x, int32_t y,
        int32_t w, const char* format, va_list va); // "w" can be zero
    ui_wh_t (*multiline)(const ui_gdi_ta_t* ta, int32_t x, int32_t y,
        int32_t w, const char* format, ...);
    // x[ut_str.glyphs(utf8, bytes)] = {x0, x1, x2, ...}
    ui_wh_t (*glyphs_placement)(const ui_gdi_ta_t* ta, const char* utf8,
        int32_t bytes, int32_t x[/*glyphs + 1*/], int32_t glyphs);
} ui_gdi_if;

extern ui_gdi_if ui_gdi;

ut_end_c

// ________________________________ ui_view.h _________________________________

ut_begin_c

enum ui_view_type_t {
    ui_view_stack     = 'vwst',
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

typedef struct ui_view_private_s { // do not access directly
    char text[1024]; // utf8 zero terminated
    int32_t strid;    // 0 for not yet localized, -1 no localization
    fp64_t armed_until; // ut_clock.seconds() - when to release
    fp64_t hover_when;  // time in seconds when to call hovered()
    // use: ui_view.string(v) and ui_view.set_string()
} ui_view_private_t;

typedef struct ui_view_text_metrics_s { // ui_view.measure_text() fills these attributes:
    ui_wh_t    mt; // text width and height
    ui_point_t xy; // text offset inside view
    bool multiline; // text contains "\n"
} ui_view_text_metrics_t;

typedef struct ui_view_s {
    enum ui_view_type_t type;
    ui_view_private_t p; // private
    void (*init)(ui_view_t* v); // called once before first layout
    ui_view_t* parent;
    ui_view_t* child; // first child, circular doubly linked list
    ui_view_t* prev;  // left or top sibling
    ui_view_t* next;  // right or top sibling
    int32_t x;
    int32_t y;
    int32_t w;
    int32_t h;
    ui_margins_t insets;
    ui_margins_t padding;
    ui_view_text_metrics_t text;
    // see ui.alignment values
    int32_t align; // align inside parent
    int32_t text_align; // align of the text inside control
    int32_t max_w; // > 0 maximum width in pixels the view agrees to
    int32_t max_h; // > 0 maximum height in pixels
    fp32_t  min_w_em; // > 0 minimum width  of a view in "em"s
    fp32_t  min_h_em; // > 0 minimum height of a view in "em"s
    ui_icon_t icon; // used instead of text if != null
    // updated on layout() call
    const ui_fm_t* fm; // font metrics
    int32_t  shortcut; // keyboard shortcut
    void* that;  // for the application use
    void (*notify)(ui_view_t* v, void* p); // for the application use
    // two pass layout: measure() .w, .h layout() .x .y
    // first  measure() bottom up - children.layout before parent.layout
    // second layout() top down - parent.layout before children.layout
    // before methods: called before measure()/layout()/paint()
    void (*prepare)(ui_view_t* v);  // called before measure()
    void (*measure)(ui_view_t* v);  // determine w, h (bottom up)
    void (*measured)(ui_view_t* v); // called after measure()
    void (*layout)(ui_view_t* v);   // set x, y possibly adjust w, h (top down)
    void (*composed)(ui_view_t* v); // after layout() is done (laid out)
    void (*paint)(ui_view_t* v);
    void (*painted)(ui_view_t* v);  // called after paint()
    // composed() is effectively called right before paint() and
    // can be used to prepare for painting w/o need to override paint()
    void (*debug_paint)(ui_view_t* v); // called if .debug is set to true
    // any message:
    bool (*message)(ui_view_t* v, int32_t message, int64_t wp, int64_t lp,
        int64_t* rt); // return true and value in rt to stop processing
    void (*click)(ui_view_t* v);    // ui click callback - view action
    void (*format)(ui_view_t* v);   // format a value to text (e.g. slider)
    void (*callback)(ui_view_t* v); // state change callback
    void (*mouse_scroll)(ui_view_t* v, ui_point_t dx_dy); // touchpad scroll
    void (*mouse_hover)(ui_view_t* v); // hover over
    void (*mouse_move)(ui_view_t* v);
    void (*mouse_click)(ui_view_t* v,  int32_t ix, bool pressed);
    void (*double_click)(ui_view_t* v, int32_t ix);
    // tap(ui, button_index) press(ui, button_index) see note below
    // button index 0: left, 1: middle, 2: right
    // bottom up (leaves to root or children to parent)
    // return true if consumed (halts further calls up the tree)
    bool (*tap)(ui_view_t* v, int32_t ix, bool pressed); // single click/tap inside ui
    bool (*long_press)(ui_view_t* v, int32_t ix); // two finger click/tap or long press
    bool (*double_tap)(ui_view_t* v, int32_t ix); // legacy double click
    bool (*context_menu)(ui_view_t* v); // right mouse click or long press
    void (*focus_gained)(ui_view_t* v);
    void (*focus_lost)(ui_view_t* v);
    // translated from key pressed/released to utf8:
    void (*character)(ui_view_t* v, const char* utf8);
    bool (*key_pressed)(ui_view_t* v, int64_t key);  // return true to stop
    bool (*key_released)(ui_view_t* v, int64_t key); // processing
    // timer() every_100ms() and every_sec() called
    // even for hidden and disabled views
    void (*timer)(ui_view_t* v, ui_timer_t id);
    void (*every_100ms)(ui_view_t* v); // ~10 x times per second
    void (*every_sec)(ui_view_t* v); // ~once a second
    int64_t (*hit_test)(const ui_view_t* v, ui_point_t pt);
    struct {
        bool hidden;    // measure()/ layout() paint() is not called on
        bool disabled;  // mouse, keyboard, key_up/down not called on
        bool armed;     // button is pressed but not yet released
        bool hover;     // cursor is hovering over the control
        bool pressed;   // for ui_button_t and ui_toggle_t
    } state;
    // TODO: instead of flat color scheme: undefined colors for
    // border rounded gradient etc.
    bool flat;                // no-border appearance of controls
    bool flip;                // flip button pressed / released
    bool focusable;           // can be target for keyboard focus
    bool highlightable;       // paint highlight rectangle when hover over label
    ui_color_t color;         // interpretation depends on view type
    int32_t    color_id;      // 0 is default meaning use color
    ui_color_t background;    // interpretation depends on view type
    int32_t    background_id; // 0 is default meaning use background
    char hint[256]; // tooltip hint text (to be shown while hovering over view)
    struct {
        struct {
            bool prc; // paint rect
            bool mt;  // measure text
        } trace;
        struct { // after painted():
            bool call;    // v->debug_paint()
            bool margins; // call debug_paint_margins()
            bool fm;      // paint font metrics
        } paint;
        const char* id; // for debugging purposes
    } debug; // debug flags
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
    void (*remove)(ui_view_t* v); // removes view from it`s parent
    void (*remove_all)(ui_view_t* parent); // removes all children
    void (*disband)(ui_view_t* parent); // removes all children recursively
    bool (*is_parent_of)(const ui_view_t* p, const ui_view_t* c);
    bool (*inside)(const ui_view_t* v, const ui_point_t* pt);
    ui_ltrb_t (*margins)(const ui_view_t* v, const ui_margins_t* g); // to pixels
    void (*inbox)(const ui_view_t* v, ui_rect_t* r, ui_ltrb_t* insets);
    void (*outbox)(const ui_view_t* v, ui_rect_t* r, ui_ltrb_t* padding);
    void (*set_text)(ui_view_t* v, const char* format, ...);
    void (*set_text_va)(ui_view_t* v, const char* format, va_list va);
    // ui_view.invalidate() prone to 30ms delays don't use in r/t video code
    // ui_view.invalidate(v, ui_app.crc) invalidates whole client rect but
    // ui_view.redraw() (fast non blocking) is much better instead
    void (*invalidate)(const ui_view_t* v, const ui_rect_t* rect_or_null);
    bool (*is_orphan)(const ui_view_t* v);   // view parent chain has null
    bool (*is_hidden)(const ui_view_t* v);   // view or any parent is hidden
    bool (*is_disabled)(const ui_view_t* v); // view or any parent is disabled
    bool (*is_control)(const ui_view_t* v);
    bool (*is_container)(const ui_view_t* v);
    bool (*is_spacer)(const ui_view_t* v);
    const char* (*string)(ui_view_t* v);  // returns localized text
    void (*timer)(ui_view_t* v, ui_timer_t id);
    void (*every_sec)(ui_view_t* v);
    void (*every_100ms)(ui_view_t* v);
    int64_t (*hit_test)(const ui_view_t* v, ui_point_t pt);
    // key_pressed() key_released() return true to stop further processing
    bool (*key_pressed)(ui_view_t* v, int64_t v_key);
    bool (*key_released)(ui_view_t* v, int64_t v_key);
    void (*character)(ui_view_t* v, const char* utf8);
    void (*paint)(ui_view_t* v);
    bool (*has_focus)(const ui_view_t* v); // ui_app.focused() && ui_app.focus == v
    void (*set_focus)(ui_view_t* view_or_null);
    void (*lose_hidden_focus)(ui_view_t* v);
    void (*hovering)(ui_view_t* v, bool start);
    void (*mouse_hover)(ui_view_t* v); // hover over
    void (*mouse_move)(ui_view_t* v);
    // TODO: remove
    void (*mouse_click)(ui_view_t* v,  int32_t ix, bool pressed);
    void (*double_click)(ui_view_t* v, int32_t ix);
    // TODO: remove ^^^
    void (*mouse_scroll)(ui_view_t* v, ui_point_t dx_dy); // touchpad scroll
    ui_wh_t (*text_metrics_va)(int32_t x, int32_t y, bool multiline, int32_t w,
        const ui_fm_t* fm, const char* format, va_list va);
    ui_wh_t (*text_metrics)(int32_t x, int32_t y, bool multiline, int32_t w,
        const ui_fm_t* fm, const char* format, ...);
    void (*text_measure)(ui_view_t* v, const char* s,
        ui_view_text_metrics_t* tm);
    void (*text_align)(ui_view_t* v, ui_view_text_metrics_t* tm);
    void (*measure_text)(ui_view_t* v); // fills v->text.mt and .xy
    // measure_control(): control is special case with v->text.mt and .xy
    void (*measure_control)(ui_view_t* v);
    void (*measure_children)(ui_view_t* v);
    void (*layout_children)(ui_view_t* v);
    void (*measure)(ui_view_t* v);
    void (*layout)(ui_view_t* v);
    void (*hover_changed)(ui_view_t* v);
    bool (*is_shortcut_key)(ui_view_t* v, int64_t key);
    bool (*context_menu)(ui_view_t* v);
    // preferred to mouse_click() because of possibility of touch devices
    // `ix` 0: left 1: middle 2: right
    bool (*tap)(ui_view_t* v, int32_t ix, bool pressed);
    bool (*long_press)(ui_view_t* v, int32_t ix);
    bool (*double_tap)(ui_view_t* v, int32_t ix);
    bool (*message)(ui_view_t* v, int32_t m, int64_t wp, int64_t lp, int64_t* ret);
    void (*debug_paint_margins)(ui_view_t* v); // insets padding
    void (*debug_paint_fm)(ui_view_t* v);   // text font metrics
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

#define ui_view_debug_id(v) \
    ((v)->debug.id != null ? (v)->debug.id : (v)->p.text)

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

// ui_view_insets (fractions of 1/2 to keep float calculations precise):
#define ui_view_i_lr (0.750f) // 3/4 of "em.w" on left and right
#define ui_view_i_tb (0.125f) // 1/8 em

// ui_view_padding
#define ui_view_p_lr (0.375f)
#define ui_view_p_tb (0.250f)

#define ui_view_call_init(v) do {                   \
    if ((v)->init != null) {                        \
        void (*_init_)(ui_view_t* _v_) = (v)->init; \
        (v)->init = null; /* before! call */        \
        _init_((v));                                \
    }                                               \
} while (0)


ut_end_c
// _____________________________ ui_containers.h ______________________________

/* Copyright (c) Dmitry "Leo" Kuznetsov 2021-24 see LICENSE for details */

ut_begin_c

typedef struct ui_view_s ui_view_t;

// Usage:
//
// ui_view_t* stack  = ui_view(stack);
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
// Except ui_app.root,caption,content which are also containers
// but are not inset or padded and have default background color.
//
// Application implementer can override this after
//
// void opened(void) {
//     ui_view.add(ui_app.view, ..., null);
//     ui_app.view->insets = (ui_margins_t) {
//         .left  = 0.25, .top    = 0.25,
//         .right = 0.25, .bottom = 0.25 };
//     ui_app.view->color = ui_colors.dark_scarlet;
// }

typedef struct ui_view_s ui_view_t;

#define ui_view(view_type) {            \
    .type = (ui_view_ ## view_type),    \
    .init = ui_view_init_ ## view_type, \
    .fm   = &ui_app.fm.regular,         \
    .color = ui_color_transparent,      \
    .color_id = 0                       \
}

void ui_view_init_stack(ui_view_t* v);
void ui_view_init_span(ui_view_t* v);
void ui_view_init_list(ui_view_t* v);
void ui_view_init_spacer(ui_view_t* v);

ut_end_c
// ______________________________ ui_edit_doc.h _______________________________

/* Copyright (c) Dmitry "Leo" Kuznetsov 2021-24 see LICENSE for details */

ut_begin_c

typedef struct ui_edit_str_s ui_edit_str_t;

typedef struct ui_edit_doc_s ui_edit_doc_t;

typedef struct ui_edit_notify_s ui_edit_notify_t;

typedef struct ui_edit_to_do_s ui_edit_to_do_t;

typedef struct ui_edit_pg_s { // page/glyph coordinates
    // humans used to line:column coordinates in text
    int32_t pn; // zero based paragraph number ("line number")
    int32_t gp; // zero based glyph position ("column")
} ui_edit_pg_t;

typedef union ut_begin_packed ui_edit_range_s {
    struct { ui_edit_pg_t from; ui_edit_pg_t to; };
    ui_edit_pg_t a[2];
} ut_end_packed ui_edit_range_t; // "from"[0] "to"[1]

typedef struct ui_edit_text_s {
    int32_t np;   // number of paragraphs
    ui_edit_str_t* ps; // ps[np] paragraphs
} ui_edit_text_t;

typedef struct ui_edit_notify_info_s {
    bool ok; // false if ui_edit.replace() failed (bad utf8 or no memory)
    const ui_edit_doc_t*   const d;
    const ui_edit_range_t* const r; // range to be replaced
    const ui_edit_range_t* const x; // extended range (replacement)
    const ui_edit_text_t*  const t; // replacement text
    // d->text.np number of paragraphs may change after replace
    // before/after: [pnf..pnt] is inside [0..d->text.np-1]
    int32_t const pnf; // paragraph number from
    int32_t const pnt; // paragraph number to. (inclusive)
    // one can safely assume that ps[pnf] was modified
    // except empty range replace with empty text (which shouldn't be)
    // d->text.ps[pnf..pnf + deleted] were deleted
    // d->text.ps[pnf..pnf + inserted] were inserted
    int32_t const deleted;  // number of deleted  paragraphs (before: 0)
    int32_t const inserted; // paragraph inserted paragraphs (before: 0)
} ui_edit_notify_info_t;

typedef struct ui_edit_notify_s { // called before and after replace()
    void (*before)(ui_edit_notify_t* notify, const ui_edit_notify_info_t* ni);
    // after() is called even if replace() failed with ok: false
    void (*after)(ui_edit_notify_t* notify, const ui_edit_notify_info_t* ni);
} ui_edit_notify_t;

typedef struct ui_edit_listener_s ui_edit_listener_t;

typedef struct ui_edit_listener_s {
    ui_edit_notify_t* notify;
    ui_edit_listener_t* prev;
    ui_edit_listener_t* next;
} ui_edit_listener_t;

typedef struct ui_edit_to_do_s { // undo/redo action
    ui_edit_range_t  range;
    ui_edit_text_t   text;
    ui_edit_to_do_t* next; // inside undo or redo list
} ui_edit_to_do_t;

typedef struct ui_edit_doc_s {
    ui_edit_text_t   text;
    ui_edit_to_do_t* undo; // undo stack
    ui_edit_to_do_t* redo; // redo stack
    ui_edit_listener_t* listeners;
} ui_edit_doc_t;

typedef struct ui_edit_doc_if {
    // init(utf8, bytes, heap:false) must have longer lifetime
    // than document, otherwise use heap: true to copy
    bool    (*init)(ui_edit_doc_t* d, const char* utf8_or_null,
                    int32_t bytes, bool heap);
    bool    (*replace)(ui_edit_doc_t* d, const ui_edit_range_t* r,
                const char* utf8, int32_t bytes);
    int32_t (*bytes)(const ui_edit_doc_t* d, const ui_edit_range_t* range);
    bool    (*copy_text)(ui_edit_doc_t* d, const ui_edit_range_t* range,
                ui_edit_text_t* text); // retrieves range into string
    int32_t (*utf8bytes)(const ui_edit_doc_t* d, const ui_edit_range_t* range);
    // utf8 must be at least ui_edit_doc.utf8bytes()
    void    (*copy)(ui_edit_doc_t* d, const ui_edit_range_t* range,
                char* utf8, int32_t bytes);
    // undo() and push reverse into redo stack
    bool (*undo)(ui_edit_doc_t* d); // false if there is nothing to redo
    // redo() and push reverse into undo stack
    bool (*redo)(ui_edit_doc_t* d); // false if there is nothing to undo
    bool (*subscribe)(ui_edit_doc_t* d, ui_edit_notify_t* notify);
    void (*unsubscribe)(ui_edit_doc_t* d, ui_edit_notify_t* notify);
    void (*dispose_to_do)(ui_edit_to_do_t* to_do);
    void (*dispose)(ui_edit_doc_t* d);
    void (*test)(void);
} ui_edit_doc_if;

extern ui_edit_doc_if ui_edit_doc;

typedef struct ui_edit_range_if {
    int (*compare)(const ui_edit_pg_t pg1, const ui_edit_pg_t pg2);
    ui_edit_range_t (*order)(const ui_edit_range_t r);
    bool            (*is_valid)(const ui_edit_range_t r);
    bool            (*is_empty)(const ui_edit_range_t r);
    uint64_t        (*uint64)(const ui_edit_pg_t pg); // (p << 32 | g)
    ui_edit_pg_t    (*pg)(uint64_t ui64); // p: (ui64 >> 32) g: (int32_t)ui64
    bool            (*inside)(const ui_edit_text_t* t,
                              const ui_edit_range_t r);
    ui_edit_range_t (*intersect)(const ui_edit_range_t r1,
                                 const ui_edit_range_t r2);
    const ui_edit_range_t* const invalid_range; // {{-1,-1},{-1,-1}}
} ui_edit_range_if;

extern ui_edit_range_if ui_edit_range;

typedef struct ui_edit_text_if {
    bool    (*init)(ui_edit_text_t* t, const char* utf, int32_t b, bool heap);

    int32_t (*bytes)(const ui_edit_text_t* t, const ui_edit_range_t* r);
    // end() last paragraph, last glyph in text
    ui_edit_pg_t    (*end)(const ui_edit_text_t* t);
    ui_edit_range_t (*end_range)(const ui_edit_text_t* t);
    ui_edit_range_t (*all_on_null)(const ui_edit_text_t* t,
                                   const ui_edit_range_t* r);
    ui_edit_range_t (*ordered)(const ui_edit_text_t* t,
                               const ui_edit_range_t* r);
    bool    (*dup)(ui_edit_text_t* t, const ui_edit_text_t* s);
    bool    (*equal)(const ui_edit_text_t* t1, const ui_edit_text_t* t2);
    bool    (*copy_text)(const ui_edit_text_t* t, const ui_edit_range_t* range,
                ui_edit_text_t* to);
    void    (*copy)(const ui_edit_text_t* t, const ui_edit_range_t* range,
                char* to, int32_t bytes);
    bool    (*replace)(ui_edit_text_t* t, const ui_edit_range_t* r,
                const ui_edit_text_t* text, ui_edit_to_do_t* undo_or_null);
    bool    (*replace_utf8)(ui_edit_text_t* t, const ui_edit_range_t* r,
                const char* utf8, int32_t bytes, ui_edit_to_do_t* undo_or_null);
    void    (*dispose)(ui_edit_text_t* t);
} ui_edit_text_if;

extern ui_edit_text_if ui_edit_text;

typedef struct ut_begin_packed ui_edit_str_s {
    char* u;    // always correct utf8 bytes not zero terminated(!) sequence
    // s.g2b[s.g + 1] glyph to byte position inside s.u[]
    // s.g2b[0] == 0, s.g2b[s.glyphs] == s.bytes
    int32_t* g2b;  // g2b_0 or heap allocated glyphs to bytes indices
    int32_t  b;    // number of bytes
    int32_t  c;    // when capacity is zero .u is not heap allocated
    int32_t  g;    // number of glyphs
} ut_end_packed ui_edit_str_t;

typedef struct ui_edit_str_if {
    bool (*init)(ui_edit_str_t* s, const char* utf8, int32_t bytes, bool heap);
    void (*swap)(ui_edit_str_t* s1, ui_edit_str_t* s2);
    int32_t (*gp_to_bp)(const char* s, int32_t bytes, int32_t gp); // or -1
    int32_t (*bytes)(ui_edit_str_t* s, int32_t from, int32_t to); // glyphs
    bool (*expand)(ui_edit_str_t* s, int32_t capacity); // reallocate
    void (*shrink)(ui_edit_str_t* s); // get rid of extra heap memory
    bool (*replace)(ui_edit_str_t* s, int32_t from, int32_t to, // glyphs
                    const char* utf8, int32_t bytes); // [from..to[ exclusive
    bool (*is_zwj)(uint32_t utf32); // zero width joiner
    bool (*is_letter)(uint32_t utf32); // in European Alphabets
    bool (*is_digit)(uint32_t utf32);
    bool (*is_symbol)(uint32_t utf32);
    bool (*is_alphanumeric)(uint32_t utf32);
    bool (*is_blank)(uint32_t utf32); // white space
    bool (*is_punctuation)(uint32_t utf32);
    bool (*is_combining)(uint32_t utf32);
    bool (*is_spacing)(uint32_t utf32); // spacing modifiers
    bool (*is_cjk_or_emoji)(uint32_t utf32);
    bool (*can_break)(uint32_t cp1, uint32_t cp2);
    void (*test)(void);
    void (*free)(ui_edit_str_t* s);
    const ui_edit_str_t* const empty;
} ui_edit_str_if;

extern ui_edit_str_if ui_edit_str;

/*
    For caller convenience the bytes parameter in all calls can be set
    to -1 for zero terminated utf8 strings which results in treating
    strlen(utf8) as number of bytes.

    ui_edit_str.init()
            initializes not zero terminated utf8 string that may be
            allocated on the heap or point out to an outside memory
            location that should have longer lifetime and will be
            treated as read only. init() may return false if
            heap.alloc() returns null or the utf8 bytes sequence
            is invalid.
            s.b is number of bytes in the initialized string;
            s.c is set to heap allocated capacity is set to zero
            for strings that are not allocated on the heap;
            s.g is number of the utf8 glyphs (aka Unicode codepoints)
            in the string;
            s.g2b[] is an array of s.g + 1 integers that maps glyph
            positions to byte positions in the utf8 string. The last
            element is number of bytes in the s.u memory.
            Called must zero out the string struct before calling init().

    ui_edit_str.bytes()
            returns number of bytes in utf8 string in the exclusive
            range [from..to[ between string glyphs.

    ui_edit_str.replace()
            replaces utf8 string in the exclusive range [from..to[
            with the new utf8 string. The new string may be longer
            or shorter than the replaced string. The function returns
            false if the new string is invalid utf8 sequence or
            heap allocation fails. The called must ensure that the
            range [from..to[ is valid, failure to do so is a fatal
            error. ui_edit_str.replace() moves string content to the heap.

    ui_edit_str.free()
            deallocates all heap allocated memory and zero out string
            struct. It is incorrect to call free() on the string that
            was not initialized or already freed.

    All ui_edit_str_t keep "precise" number of utf8 bytes.
    Caller may allocate extra byte and set it to 0x00
    after retrieving and copying data from ui_edit_str if
    the string content is intended to be used by any
    other API that expects zero terminated strings.
*/


ut_end_c
// ______________________________ ui_edit_view.h ______________________________

/* Copyright (c) Dmitry "Leo" Kuznetsov 2021-24 see LICENSE for details */

ut_begin_c

// important ui_edit_t will refuse to layout into a box smaller than
// width 3 x fm->em.w height 1 x fm->em.h

typedef struct ui_edit_s ui_edit_t;

typedef struct ui_edit_str_s ui_edit_str_t;

typedef struct ui_edit_doc_s ui_edit_doc_t;

typedef struct ui_edit_notify_s ui_edit_notify_t;

typedef struct ui_edit_to_do_s ui_edit_to_do_t;

typedef struct ui_edit_pr_s { // page/run coordinates
    int32_t pn; // paragraph number
    int32_t rn; // run number inside paragraph
} ui_edit_pr_t;

typedef struct ui_edit_run_s {
    int32_t bp;     // position in bytes  since start of the paragraph
    int32_t gp;     // position in glyphs since start of the paragraph
    int32_t bytes;  // number of bytes in this `run`
    int32_t glyphs; // number of glyphs in this `run`
    int32_t pixels; // width in pixels
} ui_edit_run_t;

// ui_edit_paragraph_t.initially text will point to readonly memory
// with .allocated == 0; as text is modified it is copied to
// heap and reallocated there.

typedef struct ui_edit_paragraph_s { // "paragraph" view consists of wrapped runs
    int32_t runs;       // number of runs in this paragraph
    ui_edit_run_t* run; // heap allocated array[runs]
} ui_edit_paragraph_t;

typedef struct ui_edit_notify_view_s {
    ui_edit_notify_t notify;
    void*            that; // specific for listener
    uintptr_t        data; // before -> after listener data
} ui_edit_notify_view_t;

typedef struct ui_edit_s {
    union {
        ui_view_t view;
        struct ui_view_s;
    };
    ui_edit_doc_t* doc; // document
    ui_edit_notify_view_t listener;
    ui_edit_range_t selection; // "from" selection[0] "to" selection[1]
    ui_point_t caret; // (-1, -1) off
    int32_t caret_width; // in pixels
    ui_edit_pr_t scroll; // left top corner paragraph/run coordinates
    int32_t last_x;    // last_x for up/down caret movement
    ui_ltrb_t inside;  // inside insets space
    struct {
        int32_t w;       // inside.right - inside.left
        int32_t h;       // inside.bottom - inside.top
        int32_t buttons; // bit 0 and bit 1 for LEFT and RIGHT mouse buttons down
    } edit;
    // number of fully (not partially clipped) visible `runs' from top to bottom:
    int32_t visible_runs;
    // TODO: remove focused because it is the same as caret != (-1, -1)
    bool focused;     // is focused and created caret
    bool ro;          // Read Only
    bool sle;         // Single Line Edit
    bool hide_word_wrap; // do not paint word wrap
    int32_t shown;    // debug: caret show/hide counter 0|1
    // paragraphs memory:
    ui_edit_paragraph_t* para; // para[e->doc->text.np]
} ui_edit_t;

typedef struct ui_edit_if {
    void (*init)(ui_edit_t* e, ui_edit_doc_t* d);
    void (*set_font)(ui_edit_t* e, ui_fm_t* fm); // see notes below (*)
    void (*move)(ui_edit_t* e, ui_edit_pg_t pg); // move caret clear selection
    // replace selected text. If bytes < 0 text is treated as zero terminated
    void (*paste)(ui_edit_t* e, const char* text, int32_t bytes);
    // call save(e, null, &bytes) to retrieve number of utf8
    // bytes required to save whole text including 0x00 terminating bytes
    errno_t (*save)(ui_edit_t* e, char* text, int32_t* bytes);
    void (*copy_to_clipboard)(ui_edit_t* e);
    void (*cut_to_clipboard)(ui_edit_t* e);
    // replace selected text with content of clipboard:
    void (*paste_from_clipboard)(ui_edit_t* e);
    void (*select_all)(ui_edit_t* e); // select whole text
    void (*erase)(ui_edit_t* e); // delete selected text
    // keyboard actions dispatcher:
    void (*key_down)(ui_edit_t* e);
    void (*key_up)(ui_edit_t* e);
    void (*key_left)(ui_edit_t* e);
    void (*key_right)(ui_edit_t* e);
    void (*key_page_up)(ui_edit_t* e);
    void (*key_page_down)(ui_edit_t* e);
    void (*key_home)(ui_edit_t* e);
    void (*key_end)(ui_edit_t* e);
    void (*key_delete)(ui_edit_t* e);
    void (*key_backspace)(ui_edit_t* e);
    void (*key_enter)(ui_edit_t* e);
    // called when ENTER keyboard key is pressed in single line mode
    void (*enter)(ui_edit_t* e);
    // fuzzer test:
    void (*fuzz)(ui_edit_t* e);      // start/stop fuzzing test
    void (*dispose)(ui_edit_t* e);
} ui_edit_if;

extern ui_edit_if ui_edit;

/*
    Notes:
    set_font()
        neither edit.view.font = font nor measure()/layout() functions
        do NOT dispose paragraphs layout unless geometry changed because
        it is quite expensive operation. But choosing different font
        on the fly needs to re-layout all paragraphs. Thus caller needs
        to set font via this function instead which also requests
        edit UI element re-layout.

    .ro
        readonly edit->ro is used to control readonly mode.
        If edit control is readonly its appearance does not change but it
        refuses to accept any changes to the rendered text.

    .wb
        wordbreak this attribute was removed as poor UX human experience
        along with single line scroll editing. See note below about .sle.

    .sle
        single line edit control.
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
        is distributed evenly between single line of text and top bottom
        margins.
        IMPORTANT: SLE resizes itself vertically to accommodate for
        input that is too wide. If caller wants to limit vertical space it
        will need to hook .measure() function of SLE and do the math there.
*/

/*
    For caller convenience the bytes parameter in all calls can be set
    to -1 for zero terminated utf8 strings which results in treating
    strlen(utf8) as number of bytes.

    ui_edit_str.init()
            initializes not zero terminated utf8 string that may be
            allocated on the heap or point out to an outside memory
            location that should have longer lifetime and will be
            treated as read only. init() may return false if
            heap.alloc() returns null or the utf8 bytes sequence
            is invalid.
            s.b is number of bytes in the initialized string;
            s.c is set to heap allocated capacity is set to zero
            for strings that are not allocated on the heap;
            s.g is number of the utf8 glyphs (aka Unicode codepoints)
            in the string;
            s.g2b[] is an array of s.g + 1 integers that maps glyph
            positions to byte positions in the utf8 string. The last
            element is number of bytes in the s.u memory.
            Called must zero out the string struct before calling init().

    ui_edit_str.bytes()
            returns number of bytes in utf8 string in the exclusive
            range [from..to[ between string glyphs.

    ui_edit_str.replace()
            replaces utf8 string in the exclusive range [from..to[
            with the new utf8 string. The new string may be longer
            or shorter than the replaced string. The function returns
            false if the new string is invalid utf8 sequence or
            heap allocation fails. The called must ensure that the
            range [from..to[ is valid, failure to do so is a fatal
            error. ui_edit_str.replace() moves string content to the heap.

    ui_edit_str.free()
            deallocates all heap allocated memory and zero out string
            struct. It is incorrect to call free() on the string that
            was not initialized or already freed.

    All ui_edit_str_t keep "precise" number of utf8 bytes.
    Caller may allocate extra byte and set it to 0x00
    after retrieving and copying data from ui_edit_str if
    the string content is intended to be used by any
    other API that expects zero terminated strings.
*/

ut_end_c

// _______________________________ ui_layout.h ________________________________

ut_begin_c

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

ut_end_c

// ________________________________ ui_label.h ________________________________

ut_begin_c

typedef ui_view_t ui_label_t;

void ui_view_init_label(ui_view_t* v);

// label insets and padding left/right are intentionally
// smaller than button/slider/toggle controls

#define ui_label(min_width_em, s) {                    \
    .type = ui_view_label, .init = ui_view_init_label, \
    .fm = &ui_app.fm.regular,                          \
    .p.text = s,                                       \
    .min_w_em = min_width_em, .min_h_em = 1.25f,       \
    .insets  = {                                       \
        .left  = ui_view_i_lr, .top    = ui_view_i_tb, \
        .right = ui_view_i_lr, .bottom = ui_view_i_tb  \
    },                                                 \
    .padding = {                                       \
        .left  = ui_view_p_lr, .top    = ui_view_p_tb, \
        .right = ui_view_p_lr, .bottom = ui_view_p_tb, \
    }                                                  \
}

// text with "&" keyboard shortcuts:

void ui_label_init(ui_label_t* t, fp32_t min_w_em, const char* format, ...);
void ui_label_init_va(ui_label_t* t, fp32_t min_w_em, const char* format, va_list va);

// use this macro for initialization:
//    ui_label_t label = ui_label(min_width_em, s);
// or:
//    label = (ui_label_t)ui_label(min_width_em, s);
// which is subtle C difference of constant and
// variable initialization and I did not find universal way

ut_end_c

// _______________________________ ui_button.h ________________________________

ut_begin_c

typedef ui_view_t ui_button_t;

void ui_view_init_button(ui_view_t* v);

void ui_button_init(ui_button_t* b, const char* label, fp32_t min_width_em,
    void (*callback)(ui_button_t* b));

// ui_button_clicked can only be used on static button variables

#define ui_button_clicked(name, s, min_width_em, ...)       \
    static void name ## _clicked(ui_button_t* name) {       \
        (void)name; /* no warning if unused */              \
        { __VA_ARGS__ }                                     \
    }                                                       \
    static                                                  \
    ui_button_t name = {                                    \
        .type = ui_view_button,                             \
        .init = ui_view_init_button,                        \
        .fm = &ui_app.fm.regular,                           \
        .p.text = s,                                        \
        .callback = name ## _clicked,                       \
        .color_id = ui_color_id_button_text,                \
        .min_w_em = min_width_em, .min_h_em = 1.25f,        \
        .insets  = {                                        \
            .left  = ui_view_i_lr, .top    = ui_view_i_tb,  \
            .right = ui_view_i_lr, .bottom = ui_view_i_tb   \
        },                                                  \
        .padding = {                                        \
            .left  = ui_view_p_lr, .top    = ui_view_p_tb,  \
            .right = ui_view_p_lr, .bottom = ui_view_p_tb,  \
        }                                                   \
    }

#define ui_button(s, min_width_em, clicked) {               \
    .type = ui_view_button,                                 \
    .init = ui_view_init_button,                            \
    .fm = &ui_app.fm.regular,                               \
    .p.text = s,                                            \
    .callback = clicked,                                    \
    .color_id = ui_color_id_button_text,                    \
    .min_w_em = min_width_em, .min_h_em = 1.25f,            \
    .insets  = {                                            \
        .left  = ui_view_i_lr, .top    = ui_view_i_tb,      \
        .right = ui_view_i_lr, .bottom = ui_view_i_tb       \
    },                                                      \
    .padding = {                                            \
        .left  = ui_view_p_lr, .top    = ui_view_p_tb,      \
        .right = ui_view_p_lr, .bottom = ui_view_p_tb,      \
    }                                                       \
}

// usage:
//
// ui_button_clicked(button, "&Button", 7.0, {
//      if (button->state.pressed) {
//          // do something on click that happens on release mouse button
//      }
// })
//
// or:
//
// static void button_flipped(ui_button_t* b) {
//      swear(b->flip == true); // 2 state button, clicked on mouse press button
//      if (b->state.pressed) {
//          // show something:
//      } else {
//          // show something else:
//      }
// }
//
// ui_button_t button = ui_button(7.0, "&Button", button_flipped);
//
// or
//
// ui_button_t button = ui_view)button(button);
// ui_view.set_text(button.text, "&Button");
// button.min_w_em = 7.0;
// button.callback = button_flipped;
//
// Note:
// ui_button_clicked(button, "&Button", 7.0, {
//      button->state.pressed = !button->state.pressed;
//      // is similar to: button.flip = true but it leads thru
//      // multiple button paint and click happens on mouse button
//      // release not press
// }


ut_end_c

// _______________________________ ui_slider.h ________________________________

ut_begin_c

typedef struct ui_slider_s ui_slider_t;

typedef struct ui_slider_s {
    union {
        ui_view_t view;
        struct ui_view_s;
    };
    int32_t step;
    fp64_t time; // time last button was pressed
    ui_wh_t mt;  // text measurement (special case for %0*d)
    ui_button_t inc; // can be hidden
    ui_button_t dec; // can be hidden
    int32_t value;  // for ui_slider_t range slider control
    int32_t value_min;
    int32_t value_max;
    // style:
    bool notched; // true if marked with a notches and has a thumb
} ui_slider_t;

void ui_view_init_slider(ui_view_t* v);

void ui_slider_init(ui_slider_t* r, const char* label, fp32_t min_w_em,
    int32_t value_min, int32_t value_max, void (*callback)(ui_view_t* r));

// ui_slider_changed can only be used on static slider variables

#define ui_slider_changed(name, s, min_width_em, mn,  mx, fmt, ...) \
    static void name ## _changed(ui_slider_t* name) {               \
        (void)name; /* no warning if unused */                      \
        { __VA_ARGS__ }                                             \
    }                                                               \
    static                                                          \
    ui_slider_t name = {                                            \
        .view = {                                                   \
            .type = ui_view_slider,                                 \
            .init = ui_view_init_slider,                            \
            .fm = &ui_app.fm.regular,                               \
            .p.text = s,                                            \
            .format = fmt,                                          \
            .callback = name ## _changed,                           \
            .min_w_em = min_width_em, .min_h_em = 1.25f,            \
            .insets  = {                                            \
                .left  = ui_view_i_lr, .top    = ui_view_i_tb,      \
                .right = ui_view_i_lr, .bottom = ui_view_i_tb       \
            },                                                      \
            .padding = {                                            \
                .left  = ui_view_p_lr, .top    = ui_view_p_tb,      \
                .right = ui_view_p_lr, .bottom = ui_view_p_tb,      \
            }                                                       \
        },                                                          \
        .value_min = mn, .value_max = mx, .value = mn,              \
    }

#define ui_slider(s, min_width_em, mn, mx, fmt, changed) {          \
    .view = {                                                       \
        .type = ui_view_slider,                                     \
        .init = ui_view_init_slider,                                \
        .fm = &ui_app.fm.regular,                                   \
        .p.text = s,                                                \
        .callback = changed,                                        \
        .format = fmt,                                              \
        .min_w_em = min_width_em, .min_h_em = 1.25f,                \
            .insets  = {                                            \
                .left  = ui_view_i_lr, .top    = ui_view_i_tb,      \
                .right = ui_view_i_lr, .bottom = ui_view_i_tb       \
            },                                                      \
            .padding = {                                            \
                .left  = ui_view_p_lr, .top    = ui_view_p_tb,      \
                .right = ui_view_p_lr, .bottom = ui_view_p_tb,      \
            }                                                       \
    },                                                              \
    .value_min = mn, .value_max = mx, .value = mn,                  \
}

ut_end_c
// ________________________________ ui_theme.h ________________________________

/* Copyright (c) Dmitry "Leo" Kuznetsov 2021-24 see LICENSE for details */

ut_begin_c

enum {
    ui_theme_app_mode_default     = 0,
    ui_theme_app_mode_allow_dark  = 1,
    ui_theme_app_mode_force_dark  = 2,
    ui_theme_app_mode_force_light = 3
};

typedef struct  {
    bool (*is_app_dark)(void);
    bool (*is_system_dark)(void);
    bool (*are_apps_dark)(void);
    void (*set_preferred_app_mode)(int32_t mode);
    void (*flush_menu_themes)(void);
    void (*allow_dark_mode_for_app)(bool allow);
    void (*allow_dark_mode_for_window)(bool allow);
    void (*refresh)(void);
    void (*test)(void);
} ui_theme_if;

extern ui_theme_if ui_theme;

ut_end_c

// _______________________________ ui_toggle.h ________________________________

ut_begin_c

typedef ui_view_t ui_toggle_t;

// label may contain "___" which will be replaced with "On" / "Off"
void ui_toggle_init(ui_toggle_t* b, const char* label, fp32_t ems,
    void (*callback)(ui_toggle_t* b));

void ui_view_init_toggle(ui_view_t* v);

// ui_toggle_on_off can only be used on static toggle variables

#define ui_toggle_on_off(name, s, min_width_em, ...)        \
    static void name ## _on_off(ui_toggle_t* name) {        \
        (void)name; /* no warning if unused */              \
        { __VA_ARGS__ }                                     \
    }                                                       \
    static                                                  \
    ui_toggle_t name = {                                    \
        .type = ui_view_toggle,                             \
        .init = ui_view_init_toggle,                        \
        .fm = &ui_app.fm.regular,                           \
        .min_w_em = min_width_em,  .min_h_em = 1.25f,       \
        .p.text = s,                                        \
        .callback = name ## _on_off,                        \
        .insets  = {                                        \
            .left  = 1.75f,        .top    = ui_view_i_tb,  \
            .right = ui_view_i_lr, .bottom = ui_view_i_tb   \
        },                                                  \
        .padding = {                                        \
            .left  = ui_view_p_lr, .top    = ui_view_p_tb,  \
            .right = ui_view_p_lr, .bottom = ui_view_p_tb,  \
        }                                                   \
    }

#define ui_toggle(s, min_width_em, on_off) {                \
    .type = ui_view_toggle,                                 \
    .init = ui_view_init_toggle,                            \
    .fm = &ui_app.fm.regular,                               \
    .p.text = s,                                            \
    .callback = on_off,                                     \
    .min_w_em = min_width_em,  .min_h_em = 1.25f,           \
    .insets  = {                                            \
        .left  = 1.75f,        .top    = ui_view_i_tb,      \
        .right = ui_view_i_lr, .bottom = ui_view_i_tb       \
    },                                                      \
    .padding = {                                            \
        .left  = ui_view_p_lr, .top    = ui_view_p_tb,      \
        .right = ui_view_p_lr, .bottom = ui_view_p_tb,      \
    }                                                       \
}

ut_end_c

// _________________________________ ui_mbx.h _________________________________

ut_begin_c

// Options like:
//   "Yes"|"No"|"Abort"|"Retry"|"Ignore"|"Cancel"|"Try"|"Continue"
// maximum number of choices presentable to human is 4.

typedef struct {
    union {
        ui_view_t view;
        struct ui_view_s;
    };
    ui_label_t   label;
    ui_button_t  button[4];
    int32_t      option; // -1 or option chosen by user
    const char** options;
} ui_mbx_t;

void ui_view_init_mbx(ui_view_t* v);

void ui_mbx_init(ui_mbx_t* mx, const char* option[], const char* format, ...);

// ui_mbx_on_choice can only be used on static mbx variables


#define ui_mbx_chosen(name, s, code, ...)                        \
                                                                 \
    static char* name ## _options[] = { __VA_ARGS__, null };     \
                                                                 \
    static void name ## _chosen(ui_mbx_t* m, int32_t option) {   \
        (void)m; (void)option; /* no warnings if unused */       \
        code                                                     \
    }                                                            \
    static                                                       \
    ui_mbx_t name = {                                            \
        .view = {                                                \
            .type = ui_view_mbx,                                 \
            .init = ui_view_init_mbx,                            \
            .fm = &ui_app.fm.regular,                            \
            .p.text = s,                                         \
            .callback = name ## _chosen,                         \
            .padding = { .left  = 0.125, .top    = 0.25,         \
                         .right = 0.125, .bottom = 0.25 },       \
            .insets  = { .left  = 0.125, .top    = 0.25,         \
                         .right = 0.125, .bottom = 0.25 }        \
        },                                                       \
        .options = name ## _options                              \
    }

#define ui_mbx(s, chosen, ...) {                            \
    .view = {                                               \
        .type = ui_view_mbx, .init = ui_view_init_mbx,      \
        .fm = &ui_app.fm.regular,                           \
        .p.text = s,                                        \
        .callback = chosen,                                 \
        .padding = { .left  = 0.125, .top    = 0.25,        \
                     .right = 0.125, .bottom = 0.25 },      \
        .insets  = { .left  = 0.125, .top    = 0.25,        \
                     .right = 0.125, .bottom = 0.25 }       \
    },                                                      \
    .options = (const char*[]){ __VA_ARGS__, null },        \
}

ut_end_c
// _______________________________ ui_caption.h _______________________________

/* Copyright (c) Dmitry "Leo" Kuznetsov 2021-24 see LICENSE for details */

ut_begin_c

typedef struct ui_caption_s {
    ui_view_t view;
    // caption`s children:
    ui_button_t icon;
    ui_label_t title;
    ui_view_t spacer;
    ui_button_t menu; // use: ui_caption.button_menu.cb := your callback
    ui_button_t mode; // switch between dark/light mode
    ui_button_t mini;
    ui_button_t maxi;
    ui_button_t full;
    ui_button_t quit;
} ui_caption_t;

extern ui_caption_t ui_caption;

ut_end_c

// _________________________________ ui_app.h _________________________________

ut_begin_c

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
    ui_rect_t prc;  // previously invalidated paint rectangle inside crc
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
    void (*post)(ut_work_t* work); // work.when == 0 meaning ASAP
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
    // const char* fn = ui_app.open_filename("C:\\", filter, ut_countof(filter));
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

ut_end_c

ut_begin_c

// https://en.wikipedia.org/wiki/Fuzzing
// aka "Monkey" testing

typedef struct ui_fuzzing_s {
    ut_work_t    base;
    const char*  utf8; // .character(utf8)
    int32_t      key;  // .key_pressed(key)/.key_released(key)
    ui_point_t*  pt;   // .move_move()
    // key_press and character
    bool         alt;
    bool         ctrl;
    bool         shift;
    // mouse modifiers
    bool         left; // tap() buttons:
    bool         right;
    bool         double_tap;
    bool         long_press;
    // custom
    int32_t      op;
    void*        data;
} ui_fuzzing_t;

typedef struct ui_fuzzing_if {
    void (*start)(uint32_t seed);
    bool (*is_running)(void);
    bool (*from_inside)(void); // true if called originated inside fuzzing
    void (*next_random)(ui_fuzzing_t* f); // called if `next` is null
    void (*dispatch)(ui_fuzzing_t* f);    // dispatch work
    // next() called instead of random if not null
    void (*next)(ui_fuzzing_t* f);
    // custom() called instead of dispatch() if not null
    void (*custom)(ui_fuzzing_t* f);
    void (*stop)(void);
} ui_fuzzing_if;

extern ui_fuzzing_if ui_fuzzing;

ut_end_c


#endif // ui_definition

#ifdef ui_implementation
// _________________________________ ui_app.c _________________________________

#include "ut/ut.h"
#include "ut/ut_win32.h"

#pragma push_macro("ui_app_window")
#pragma push_macro("ui_app_canvas")

static bool ui_app_trace_utf16_keyboard_input;

#define ui_app_window() ((HWND)ui_app.window)
#define ui_app_canvas() ((HDC)ui_app.canvas)

static WNDCLASSW ui_app_wc; // window class

static NONCLIENTMETRICSW ui_app_ncm = { sizeof(NONCLIENTMETRICSW) };
static MONITORINFO ui_app_mi = {sizeof(MONITORINFO)};

static ut_event_t ui_app_event_quit;
static ut_event_t ui_app_event_invalidate;
static ut_event_t ui_app_wt; // waitable timer;

static ut_work_queue_t ui_app_queue;

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
    ut_fatal_win32err(PostMessageA(ui_app_window(), (UINT)m,
            (WPARAM)wp, (LPARAM)lp));
}

static void ui_app_update_wt_timeout(void) {
    fp64_t next_due_at = -1.0;
    ut_atomics.spinlock_acquire(&ui_app_queue.lock);
    if (ui_app_queue.head != null) {
        next_due_at = ui_app_queue.head->when;
    }
    ut_atomics.spinlock_release(&ui_app_queue.lock);
    if (next_due_at >= 0) {
        static fp64_t last_next_due_at;
        fp64_t dt = next_due_at - ut_clock.seconds();
        if (dt <= 0) {
// TODO: remove
//          ut_println("post(WM_NULL) dt: %.6f", dt);
            ui_app_post_message(WM_NULL, 0, 0);
        } else if (last_next_due_at != next_due_at) {
            // Negative values indicate relative time in 100ns intervals
            LARGE_INTEGER rt = {0}; // relative negative time
            rt.QuadPart = (LONGLONG)(-dt * 1.0E+7);
// TODO: remove
//          ut_println("dt: %.6f %lld", dt, rt.QuadPart);
            ut_swear(rt.QuadPart < 0, "dt: %.6f %lld", dt, rt.QuadPart);
            ut_fatal_win32err(
                SetWaitableTimer(ui_app_wt, &rt, 0, null, null, 0)
            );
        }
        last_next_due_at = next_due_at;
    }
}

static void ui_app_post(ut_work_t* w) {
    if (w->queue == null) { w->queue = &ui_app_queue; }
    // work item can be reused but only with the same queue
    ut_assert(w->queue == &ui_app_queue);
    ut_work_queue.post(w);
    ui_app_update_wt_timeout();
}

static void ui_app_alarm_thread(void* ut_unused(p)) {
    ut_thread.realtime();
    ut_thread.name("ui_app.alarm");
    for (;;) {
        ut_event_t es[] = { ui_app_wt, ui_app_event_quit };
        int32_t ix = ut_event.wait_any(ut_countof(es), es);
        if (ix == 0) {
// TODO: remove
//          ut_println("post(WM_NULL)");
            ui_app_post_message(WM_NULL, 0, 0);
        } else {
            break;
        }
    }
}


// InvalidateRect() may wait for up to 30 milliseconds
// which is unacceptable for video drawing at monitor
// refresh rate

static void ui_app_redraw_thread(void* ut_unused(p)) {
    ut_thread.realtime();
    ut_thread.name("ui_app.redraw");
    for (;;) {
        ut_event_t es[] = { ui_app_event_invalidate, ui_app_event_quit };
        int32_t ix = ut_event.wait_any(ut_countof(es), es);
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
    ut_fatal_win32err(SystemParametersInfoForDpi(SPI_GETNONCLIENTMETRICS,
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
            ut_thread.sleep_for(1.0 / 32); // and retry:
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
            const int32_t max_xy = (int32_t)ut_max(dpi_x, dpi_y);
            switch (mtd) {
                case MDT_EFFECTIVE_DPI:
                    dpi->monitor_effective = max_xy;
//                  ut_println("ui_app.dpi.monitor_effective := max(%d,%d)", dpi_x, dpi_y);
                    break;
                case MDT_ANGULAR_DPI:
                    dpi->monitor_angular = max_xy;
//                  ut_println("ui_app.dpi.monitor_angular := max(%d,%d)", dpi_x, dpi_y);
                    break;
                case MDT_RAW_DPI:
                    dpi->monitor_raw = max_xy;
//                  ut_println("ui_app.dpi.monitor_raw := max(%d,%d)", dpi_x, dpi_y);
                    break;
                default: ut_assert(false);
            }
            dpi->monitor_max = ut_max(dpi->monitor_max, max_xy);
        }
    }
//  ut_println("ui_app.dpi.monitor_max := %d", dpi->monitor_max);
}

#ifndef UI_APP_DEBUG

static void ui_app_dump_dpi(void) {
    ut_println("ui_app.dpi.monitor_effective: %d", ui_app.dpi.monitor_effective  );
    ut_println("ui_app.dpi.monitor_angular  : %d", ui_app.dpi.monitor_angular    );
    ut_println("ui_app.dpi.monitor_raw      : %d", ui_app.dpi.monitor_raw        );
    ut_println("ui_app.dpi.monitor_max      : %d", ui_app.dpi.monitor_max        );
    ut_println("ui_app.dpi.window           : %d", ui_app.dpi.window             );
    ut_println("ui_app.dpi.system           : %d", ui_app.dpi.system             );
    ut_println("ui_app.dpi.process          : %d", ui_app.dpi.process            );
    ut_println("ui_app.mrc      : %d,%d %dx%d", ui_app.mrc.x, ui_app.mrc.y,
                                             ui_app.mrc.w, ui_app.mrc.h);
    ut_println("ui_app.wrc      : %d,%d %dx%d", ui_app.wrc.x, ui_app.wrc.y,
                                             ui_app.wrc.w, ui_app.wrc.h);
    ut_println("ui_app.crc      : %d,%d %dx%d", ui_app.crc.x, ui_app.crc.y,
                                             ui_app.crc.w, ui_app.crc.h);
    ut_println("ui_app.work_area: %d,%d %dx%d", ui_app.work_area.x, ui_app.work_area.y,
                                             ui_app.work_area.w, ui_app.work_area.h);
    int32_t mxt_x = GetSystemMetrics(SM_CXMAXTRACK);
    int32_t mxt_y = GetSystemMetrics(SM_CYMAXTRACK);
    ut_println("MAXTRACK: %d, %d", mxt_x, mxt_y);
    int32_t scr_x = GetSystemMetrics(SM_CXSCREEN);
    int32_t scr_y = GetSystemMetrics(SM_CYSCREEN);
    fp64_t monitor_x = (fp64_t)scr_x / (fp64_t)ui_app.dpi.monitor_max;
    fp64_t monitor_y = (fp64_t)scr_y / (fp64_t)ui_app.dpi.monitor_max;
    ut_println("SCREEN: %d, %d %.1fx%.1f\"", scr_x, scr_y, monitor_x, monitor_y);
}

#endif

static bool ui_app_update_mi(const ui_rect_t* r, uint32_t flags) {
    RECT rc = ui_app_ui2rect(r);
    HMONITOR monitor = MonitorFromRect(&rc, flags);
//  TODO: moving between monitors with different DPIs
//  HMONITOR mw = MonitorFromWindow(ui_app_window(), flags);
    if (monitor != null) {
        ui_app_update_monitor_dpi(monitor, &ui_app.dpi);
        ut_fatal_win32err(GetMonitorInfoA(monitor, &ui_app_mi));
        ui_app.work_area = ui_app_rect2ui(&ui_app_mi.rcWork);
        ui_app.mrc = ui_app_rect2ui(&ui_app_mi.rcMonitor);
//      ui_app_dump_dpi();
    }
    return monitor != null;
}

static void ui_app_update_crc(void) {
    RECT rc = {0};
    ut_fatal_win32err(GetClientRect(ui_app_window(), &rc));
    ui_app.crc = ui_app_rect2ui(&rc);
}

static void ui_app_dispose_fonts(void) {
    ui_gdi.delete_font(ui_app.fm.regular.font);
    ui_gdi.delete_font(ui_app.fm.H1.font);
    ui_gdi.delete_font(ui_app.fm.H2.font);
    ui_gdi.delete_font(ui_app.fm.H3.font);
    ui_gdi.delete_font(ui_app.fm.mono.font);
}

static int32_t ui_app_px2pt(fp64_t px) {
    return (int32_t)(px * 72.0 / (fp64_t)ui_app.dpi.window + 0.5);
}

static int32_t ui_app_pt2px(fp64_t pt) {
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

static void ui_app_init_fonts(int32_t dpi) {
    ui_app_update_ncm(dpi);
    if (ui_app.fm.regular.font != null) { ui_app_dispose_fonts(); }
    LOGFONTW lf = ui_app_ncm.lfMessageFont;
    // lf.lfQuality is CLEARTYPE_QUALITY which looks bad on 4K monitors
    // Windows UI uses PROOF_QUALITY which is aliased w/o ClearType rainbows
    lf.lfQuality = ANTIALIASED_QUALITY; // PROOF_QUALITY;
    ui_gdi.update_fm(&ui_app.fm.regular, (ui_font_t)CreateFontIndirectW(&lf));
//  ui_gdi.dump_fm(ui_app.fm.regular.font);
    const fp64_t fh = ui_app_ncm.lfMessageFont.lfHeight;
//  ut_println("lfHeight=%.1f", fh);
    ut_assert(fh != 0);
    lf.lfWeight = FW_SEMIBOLD;
    lf.lfHeight = (int32_t)(fh * 1.75 + 0.5);
    ui_gdi.update_fm(&ui_app.fm.H1, (ui_font_t)CreateFontIndirectW(&lf));
    lf.lfWeight = FW_SEMIBOLD;
    lf.lfHeight = (int32_t)(fh * 1.4 + 0.5);
    ui_gdi.update_fm(&ui_app.fm.H2, (ui_font_t)CreateFontIndirectW(&lf));
    lf.lfWeight = FW_SEMIBOLD;
    lf.lfHeight = (int32_t)(fh * 1.15 + 0.5);
    ui_gdi.update_fm(&ui_app.fm.H3, (ui_font_t)CreateFontIndirectW(&lf));
    lf = ui_app_ncm.lfMessageFont;
    lf.lfPitchAndFamily &= FIXED_PITCH;
    // TODO: how to get monospaced from Win32 API?
    ut_str.utf8to16(lf.lfFaceName, ut_countof(lf.lfFaceName),
                    "Cascadia Mono", -1);
    ui_gdi.update_fm(&ui_app.fm.mono, (ui_font_t)CreateFontIndirectW(&lf));
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

typedef ut_begin_packed struct ui_app_wiw_s { // "where is window"
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
} ut_end_packed ui_app_wiw_t;

static BOOL CALLBACK ui_app_monitor_enum_proc(HMONITOR monitor,
        HDC ut_unused(hdc), RECT* ut_unused(rc1), LPARAM that) {
    ui_app_wiw_t* wiw = (ui_app_wiw_t*)(uintptr_t)that;
    MONITORINFOEXA mi = { .cbSize = sizeof(MONITORINFOEXA) };
    ut_fatal_win32err(GetMonitorInfoA(monitor, (MONITORINFO*)&mi));
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
    ut_fatal_win32err(GetWindowRect((HWND)wnd, &wr));
    ui_rect_t wrc = ui_app_rect2ui(&wr);
    ui_app_update_mi(&wrc, MONITOR_DEFAULTTONEAREST);
    WINDOWPLACEMENT wpl = { .length = sizeof(wpl) };
    ut_fatal_win32err(GetWindowPlacement((HWND)wnd, &wpl));
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
        ut_println("wiw.space: %d,%d %dx%d",
              wiw.space.x, wiw.space.y, wiw.space.w, wiw.space.h);
        ut_println("MAXTRACK: %d, %d", wiw.max_track.x, wiw.max_track.y);
        ut_println("wpl.rcNormalPosition: %d,%d %dx%d",
            wpl.rcNormalPosition.left, wpl.rcNormalPosition.top,
            wpl.rcNormalPosition.right - wpl.rcNormalPosition.left,
            wpl.rcNormalPosition.bottom - wpl.rcNormalPosition.top);
        ut_println("wpl.ptMinPosition: %d,%d",
            wpl.ptMinPosition.x, wpl.ptMinPosition.y);
        ut_println("wpl.ptMaxPosition: %d,%d",
            wpl.ptMaxPosition.x, wpl.ptMaxPosition.y);
        ut_println("wpl.showCmd: %d", wpl.showCmd);
        // WPF_SETMINPOSITION. WPF_RESTORETOMAXIMIZED WPF_ASYNCWINDOWPLACEMENT
        ut_println("wpl.flags: %d", wpl.flags);
    }
//  ut_println("%d,%d %dx%d show=%d", wiw.placement.x, wiw.placement.y,
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
            ut_println("GetConsoleScreenBufferInfoEx() %s", ut_strerr(r));
        } else {
            ut_config.save(ui_app.class_name, "console_screen_buffer_infoex",
                            &info, (int32_t)sizeof(info));
//          ut_println("info: %dx%d", info.dwSize.X, info.dwSize.Y);
//          ut_println("%d,%d %dx%d", info.srWindow.Left, info.srWindow.Top,
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
    ut_assert(mrc->w > 0 && mrc->h > 0);
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
        #ifdef UI_APP_DEBUG
            ut_println("wiw.placement: %d,%d %dx%d", wiw.placement.x, wiw.placement.y,
                wiw.placement.w, wiw.placement.h);
            ut_println("wiw.mrc: %d,%d %dx%d", wiw.mrc.x, wiw.mrc.y, wiw.mrc.w, wiw.mrc.h);
            ut_println("wiw.work_area: %d,%d %dx%d", wiw.work_area.x, wiw.work_area.y,
                                                  wiw.work_area.w, wiw.work_area.h);
            ut_println("wiw.min_position: %d,%d", wiw.min_position.x, wiw.min_position.y);
            ut_println("wiw.max_position: %d,%d", wiw.max_position.x, wiw.max_position.y);
            ut_println("wiw.max_track: %d,%d", wiw.max_track.x, wiw.max_track.y);
            ut_println("wiw.dpi: %d", wiw.dpi);
            ut_println("wiw.flags: %d", wiw.flags);
            ut_println("wiw.show: %d", wiw.show);
        #endif
        ui_app_update_mi(&wiw.placement, MONITOR_DEFAULTTONEAREST);
        bool same_monitor = memcmp(&wiw.mrc, &ui_app.mrc, sizeof(wiw.mrc)) == 0;
//      ut_println("%d,%d %dx%d", p->x, p->y, p->w, p->h);
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
//  ut_println("%d,%d %dx%d show=%d", rect->x, rect->y, rect->w, rect->h, *visibility);
    ui_app_bring_window_inside_monitor(&ui_app.mrc, rect);
//  ut_println("%d,%d %dx%d show=%d", rect->x, rect->y, rect->w, rect->h, *visibility);
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
//      ut_println("%d,%d %dx%d", p->x, p->y, p->w, p->h);
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
    ut_fatal_win32err(KillTimer(ui_app_window(), timer));
}

static ui_timer_t ui_app_timer_set(uintptr_t id, int32_t ms) {
    ut_not_null(ui_app_window());
    ut_assert(10 <= ms && ms < 0x7FFFFFFF);
    ui_timer_t tid = (ui_timer_t)SetTimer(ui_app_window(), id, (uint32_t)ms, null);
    ut_fatal_if(tid == 0);
    ut_assert(tid == id);
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
    ut_assert(ui_app.cursors.arrow != null);
    ui_app.set_cursor(ui_app.cursors.arrow);
    ui_app.canvas = (ui_canvas_t)GetDC(ui_app_window());
    ut_not_null(ui_app.canvas);
    if (ui_app.opened != null) { ui_app.opened(); }
    ui_view.set_text(ui_app.root, "ui_app.root"); // debugging
    ui_app_wm_timer(ui_app_timer_100ms_id);
    ui_app_wm_timer(ui_app_timer_1s_id);
    ut_fatal_if(ReleaseDC(ui_app_window(), ui_app_canvas()) == 0);
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
        mmi->ptMaxTrackSize.x = ut_min(max_w, wa->w);
        mmi->ptMaxTrackSize.y = ut_min(max_h, wa->h);
    }
    mmi->ptMaxSize.x = mmi->ptMaxTrackSize.x;
    mmi->ptMaxSize.y = mmi->ptMaxTrackSize.y;
}

static void ui_app_paint(ui_view_t* view) {
    ut_assert(ui_app_window() != null);
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
static void ui_app_toast_mouse_click(ui_view_t* v, bool left, bool pressed);

static void ui_app_dispatch_wm_char(ui_view_t* view, const uint16_t* utf16) {
    char utf8[32 + 1];
    int32_t utf8bytes = ut_str.utf8_bytes(utf16, -1);
    ut_swear(utf8bytes < ut_countof(utf8) - 1); // 32 bytes + 0x00
    ut_str.utf16to8(utf8, ut_countof(utf8), utf16, -1);
    utf8[utf8bytes] = 0x00;
    if (ui_app.animating.view != null) {
        ui_app_toast_character(utf8);
    } else {
        ui_view.character(view, utf8);
    }
    ui_app_high_surrogate = 0x0000;
}

static void ui_app_wm_char(ui_view_t* view, const uint16_t* utf16) {
    int32_t utf16chars = ut_str.len16(utf16);
    ut_swear(0 < utf16chars && utf16chars < 4); // wParam is 64bits
    const uint16_t utf16char = utf16[0];
    if (utf16chars == 1 && ut_str.utf16_is_high_surrogate(utf16char)) {
        ui_app_high_surrogate = utf16char;
    } else if (utf16chars == 1 && ut_str.utf16_is_low_surrogate(utf16char)) {
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

static void ui_app_mouse(ui_view_t* v, int32_t m, int64_t f) {
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
        ut_swear(i >= 0);
        const int32_t ix = ui_app.mouse_swapped ? 2 - i : i;
        const bool pressed =
            m == WM_LBUTTONDOWN ||
            m == WM_MBUTTONDOWN ||
            m == WM_RBUTTONDOWN;
        if (av != null) {
            // because of "micro" close button:
            ui_app_toast_mouse_click(ui_app.animating.view, ix, pressed);
        } else {
            ui_view.mouse_click(v, ix, pressed);
        }
    } else if (m == WM_LBUTTONDBLCLK ||
               m == WM_MBUTTONDBLCLK ||
               m == WM_RBUTTONDBLCLK) {
        const int i =
             (m == WM_LBUTTONDBLCLK) ? 0 :
            ((m == WM_MBUTTONDBLCLK) ? 1 :
            ((m == WM_RBUTTONDBLCLK) ? 2 : -1));
        ut_swear(i >= 0);
        const int32_t ix = ui_app.mouse_swapped ? 2 - i : i;
        ui_view.double_click(av != null && av->double_click != null ? av : v, ix);
    } else {
        ut_assert(false, "m: 0x%04X", m);
    }
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
        default: ut_swear(false, "fix me m: %d", m);
    }
    return -1;
}

static void ui_app_nc_mouse_buttons(int32_t m, int64_t wp, int64_t lp) {
    POINT screen = {GET_X_LPARAM(lp), GET_Y_LPARAM(lp)};
    POINT client = screen;
    ScreenToClient(ui_app_window(), &client);
    ui_app.mouse = ui_app_point2ui(&client);
    const bool inside = ui_view.inside(ui_app.caption, &ui_app.mouse);
    if (!ui_view.is_hidden(ui_app.caption) && inside) {
        uint16_t lr = ui_app.mouse_swapped ? WM_NCLBUTTONDOWN : WM_NCRBUTTONDOWN;
        if (m == lr) {
//          ut_println("WM_NC*BUTTONDOWN %d %d", ui_app.mouse.x, ui_app.mouse.y);
            ui_app_show_sys_menu(screen.x, screen.y);
        }
    } else {
        ui_app_mouse(ui_app.root, ui_app_nc_mouse_message(m), wp);
    }
}

enum { ui_app_animation_steps = 63 };

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
    ui_view_t* av = ui_app.animating.view;
    if (av != null) {
        ui_view.measure(av);
        bool hint = ui_app.animating.x >= 0 && ui_app.animating.y >= 0;
        const int32_t em_w = av->fm->em.w;
        const int32_t em_h = av->fm->em.h;
        if (!hint) {
            ut_assert(0 <= ui_app.animating.step && ui_app.animating.step < ui_app_animation_steps);
            int32_t step = ui_app.animating.step - (ui_app_animation_steps - 1);
            av->y = av->h * step / (ui_app_animation_steps - 1);
//          ut_println("step=%d of %d y=%d", ui_app.animating.step,
//                  ui_app_toast_steps, av->y);
            ui_app_measure_and_layout(av);
            // dim main window (as `disabled`):
            fp64_t alpha = ut_min(0.40, 0.40 * ui_app.animating.step / (fp64_t)ui_app_animation_steps);
            ui_gdi.alpha(0, 0, ui_app.crc.w, ui_app.crc.h, &image_dark, alpha);
            av->x = (ui_app.root->w - av->w) / 2;
//          ut_println("ui_app.animating.y: %d av->y: %d",
//                  ui_app.animating.y, av->y);
        } else {
            av->x = ui_app.animating.x;
            av->y = ui_app.animating.y;
            ui_app_measure_and_layout(av);
            int32_t mx = ui_app.root->w - av->w - em_w;
            int32_t cx = ui_app.animating.x - av->w / 2;
            av->x = ut_min(mx, ut_max(0, cx));
            av->y = ut_min(
                ui_app.root->h - em_h, ut_max(0, ui_app.animating.y));
//          ut_println("ui_app.animating.y: %d av->y: %d",
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
                    .fm = &ui_app.fm.regular,
                    .color = ui_color_undefined,
                    .color_id = ui_color_id_window_text
                };
                ui_gdi.text(&ta, tx, ty, "%s",
                                 ut_glyph_multiplication_sign);
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

static void ui_app_toast_mouse_click(ui_view_t* v, bool left, bool pressed) {
    ut_swear(v == ui_app.animating.view);
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
        ui_view.mouse_click(v, left, pressed);
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
    ut_not_null(ui_app.window);
    ut_not_null(ui_app.canvas);
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
    ut_assert(ui_app.border.w == ui_app.border.h);
    const int32_t w = ui_app.wrc.w;
    const int32_t h = ui_app.wrc.h;
    for (int32_t i = 0; i < ui_app.border.w; i++) {
        ui_gdi.frame(i, i, w - i * 2, h - i * 2, c);
    }
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
    if (ui_app.paint_last == 0) {
        ui_app.paint_dt_min = 1.0 / 60.0; // 60Hz monitor
    } else {
        fp64_t since_last = ui_app.now - ui_app.paint_last;
        if (since_last > 1.0 / 120.0) { // 240Hz monitor
            ui_app.paint_dt_min = ut_min(ui_app.paint_dt_min, since_last);
        }
//      ut_println("paint_dt_min: %.6f since_last: %.6f",
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
//      ut_println("%d,%d %dx%d", ui_app.prc.x, ui_app.prc.y, ui_app.prc.w, ui_app.prc.h);
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
        ut_fatal_win32err(GetWindowRect(ui_app_window(), &wrc));
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
//      ut_println("wp: 0x%08X", wp);
        // actual wp == 0x0000
        ui_theme.refresh();
    } else if (wp == 0 && lp != 0 && strcmp((const char*)lp, "intl") == 0) {
        ut_println("wp: 0x%04X", wp); // SPI_SETLOCALEINFO 0x24 ?
        uint16_t ln[LOCALE_NAME_MAX_LENGTH + 1];
        int32_t n = GetUserDefaultLocaleName(ln, ut_countof(ln));
        ut_fatal_if(n <= 0);
        uint16_t rln[LOCALE_NAME_MAX_LENGTH + 1];
        n = ResolveLocaleName(ln, rln, ut_countof(rln));
        ut_fatal_if(n <= 0);
        LCID lc_id = LocaleNameToLCID(rln, LOCALE_ALLOW_NEUTRAL_NAMES);
        ut_fatal_win32err(SetThreadLocale(lc_id));
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
    enum { tap = 1, long_press = 2, double_tap = 3 };
    // TODO: click detector does not handle WM_NCLBUTTONDOWN, ...
    //       it can be modified to do so if needed
    #pragma push_macro("ui_set_timer")
    #pragma push_macro("ui_kill_timer")
    #pragma push_macro("ui_timers_done")

    #define ui_set_timer(t, ms) do {                 \
        ut_assert(t == 0);                              \
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
                ui_timers_done(i);
            }
            if (wp == timer_d[i]) {
                ui_timers_done(i);
            }
        }
    }
    if (ix != -1) {
        const int32_t double_click_msec = (int32_t)GetDoubleClickTime();
        const fp64_t  double_click_dt = double_click_msec / 1000.0; // seconds
//      ut_println("double_click_msec: %d double_click_dt: %.3fs",
//               double_click_msec, double_click_dt);
        const int double_click_x = GetSystemMetrics(SM_CXDOUBLECLK) / 2;
        const int double_click_y = GetSystemMetrics(SM_CYDOUBLECLK) / 2;
        ui_point_t pt = { GET_X_LPARAM(lp), GET_Y_LPARAM(lp) };
        if (m == tap && !up) {
            ui_view.tap(ui_app.root, ix, !up);
            if (ui_app.now  - clicked[ix]  <= double_click_dt &&
                abs(pt.x - click_at[ix].x) <= double_click_x &&
                abs(pt.y - click_at[ix].y) <= double_click_y) {
                ui_app.mouse = (ui_point_t){ click_at[ix].x, click_at[ix].y };
                ui_view.double_tap(ui_app.root, ix);
                ui_timers_done(ix);
            } else {
                ui_timers_done(ix); // clear timers
                clicked[ix]  = ui_app.now;
                click_at[ix] = pt;
                pressed[ix]  = true;
                if ((ui_app_wc.style & CS_DBLCLKS) == 0) {
                    // only if Windows are not detecting DLBCLKs
                    ui_set_timer(timer_d[ix], double_click_msec);  // 0.5s
                }
                ui_set_timer(timer_p[ix], double_click_msec * 3 / 4); // 0.375s
            }
        } else if (up) {
            fp64_t since_clicked = ui_app.now - clicked[ix];
//          ut_println("pressed[%d]: %d %.3f", ix, pressed[ix], since_clicked);
            ui_view.tap(ui_app.root, ix, !up);
            // only if Windows are not detecting DLBCLKs
            if ((ui_app_wc.style & CS_DBLCLKS) == 0 &&
                 pressed[ix] && since_clicked > double_click_dt) {
                ui_view.double_tap(ui_app.root, ix);
                ui_timers_done(ix);
            }
            ui_kill_timer(timer_p[ix]); // long press is not the case
        } else if (m == double_tap) {
            ut_assert((ui_app_wc.style & CS_DBLCLKS) != 0);
            ui_view.double_tap(ui_app.root, ix);
            ui_timers_done(ix);
        }
    }
    #pragma pop_macro("ui_timers_done")
    #pragma pop_macro("ui_kill_timer")
    #pragma pop_macro("ui_set_timer")
}

static int64_t ui_app_root_hit_test(const ui_view_t* v, ui_point_t pt) {
    ut_swear(v == ui_app.root);
    if (ui_app.no_decor) {
        ut_assert(ui_app.border.w == ui_app.border.h);
        // on 96dpi monitors ui_app.border is 1x1
        // make it easier for the user to resize window
        int32_t border = ut_max(4, ui_app.border.w * 2);
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
        ut_assert(GetActiveWindow() == ui_app_window());
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
//  ut_println("WM_NCCALCSIZE wp: %lld is_max: %d (%d %d %d %d) (%d %d %d %d) (%d %d %d %d)",
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
        ut_println("WM_GETDPISCALEDSIZE dpi %d := %d "
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
//  ut_println("WM_SYSCOMMAND wp: 0x%08llX lp: 0x%016llX %lld sys: 0x%04X",
//          wp, lp, lp, sys_cmd);
    if (sys_cmd == SC_MINIMIZE && ui_app.hide_on_minimize) {
        ui_app.show_window(ui.visibility.min_na);
        ui_app.show_window(ui.visibility.hide);
    } else  if (sys_cmd == SC_MINIMIZE && ui_app.no_decor) {
        ui_app.show_window(ui.visibility.min_na);
    }
//  if (sys_cmd == SC_KEYMENU) { ut_println("SC_KEYMENU lp: %lld", lp); }
    // If the selection is in menu handle the key event
    if (sys_cmd == SC_KEYMENU && lp != 0x20) {
        return true; // handled: This prevents the error/beep sound
    }
    if (sys_cmd == SC_MAXIMIZE && ui_app.no_decor) {
        return true; // handled: prevent maximizing no decorations window
    }
//  if (sys_cmd == SC_MOUSEMENU) {
//      ut_println("SC_KEYMENU.SC_MOUSEMENU 0x%00llX %lld", wp, lp);
//  }
    return false; // drop down to to DefWindowProc
}

static void ui_app_wm_window_position_changing(int64_t wp, int64_t lp) {
    #ifdef UI_APP_DEBUG // TODO: ui_app.debug.trace.window_position?
        WINDOWPOS* pos = (WINDOWPOS*)lp;
        ut_println("WM_WINDOWPOSCHANGING flags: 0x%08X", pos->flags);
        if (pos->flags & SWP_SHOWWINDOW) {
            ut_println("SWP_SHOWWINDOW");
        } else if (pos->flags & SWP_HIDEWINDOW) {
            ut_println("SWP_HIDEWINDOW");
        }
    #else
        (void)wp; // unused
        (void)lp; // unused
    #endif
}

static void ui_app_wm_mouse(int32_t m, int64_t wp, int64_t lp) {
    // note: x, y is already in client coordinates
    ui_app.mouse.x = GET_X_LPARAM(lp);
    ui_app.mouse.y = GET_Y_LPARAM(lp);
    ui_app_mouse(ui_app.root, m, wp);
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
    for (int32_t i = 0; i < ut_countof(cs); i++) {
        if (cs[i].charset == wp) {
            ut_println("WM_INPUTLANGCHANGE: 0x%08X %s", wp, cs[i].name);
            break;
        }
    }
    #else
        (void)wp; // unused
    #endif
}

static void ui_app_decode_keyboard(int32_t m, int64_t wp, int64_t lp) {
    // https://learn.microsoft.com/en-us/windows/win32/inputdev/about-keyboard-input#keystroke-message-flags
    ut_swear(m == WM_KEYDOWN || m == WM_SYSKEYDOWN ||
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
    ut_fatal_win32err(GetKeyboardState(keyboard_state));
    // HKL low word Language Identifier
    //     high word device handle to the physical layout of the keyboard
    const HKL kl = GetKeyboardLayout(0);
    // Map virtual key to scan code
    UINT vk = MapVirtualKeyEx(scan_code, MAPVK_VSC_TO_VK_EX, kl);
//  ut_println("virtual_key: %02X keyboard layout: %08X",
//              virtual_key, kl);
    memset(ui_app_decoded_released, 0x00, sizeof(ui_app_decoded_released));
    memset(ui_app_decoded_pressed,  0x00, sizeof(ui_app_decoded_pressed));
    // Translate scan code to character
    int32_t r = ToUnicodeEx(vk, scan_code, keyboard_state,
                            utf16, ut_countof(utf16), 0, kl);
    if (r > 0) {
        ut_static_assertion(ut_countof(ui_app_decoded_pressed) ==
                            ut_countof(ui_app_decoded_released));
        enum { capacity = (int32_t)ut_countof(ui_app_decoded_released) };
        char* utf8 = is_key_released ?
            ui_app_decoded_released : ui_app_decoded_pressed;
        ut_str.utf16to8(utf8, capacity, utf16, -1);
        if (ui_app_trace_utf16_keyboard_input) {
            ut_println("0x%04X%04X released: %d down: %d repeat: %d \"%s\"",
                    utf16[0], utf16[1], is_key_released, was_key_down,
                    repeat_count, utf8);
        }
    } else if (r == 0) {
        // The specified virtual key has no translation for the
        // current state of the keyboard. (E.g. arrows, enter etc)
    } else {
        ut_assert(r < 0);
        // The specified virtual key is a dead key character (accent or diacritic).
        if (ui_app_trace_utf16_keyboard_input) { ut_println("dead key"); }
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
            if (0 < count && count < ut_countof(utf16) - 1) {
                ImmGetCompositionStringW(imc, GCS_RESULTSTR, utf16, bytes);
                utf16[count] = 0x00;
                ut_str.utf16to8(utf8, ut_countof(utf8), utf16, -1);
                ut_println("bytes: %d 0x%04X 0x%04X %s", bytes, utf16[0], utf16[1], utf8);
            }
            ut_fatal_win32err(ImmReleaseContext(ui_app_window(), imc));
        }
    }
}

static LRESULT CALLBACK ui_app_window_proc(HWND window, UINT message,
        WPARAM w_param, LPARAM l_param) {
// TODO: remove
//  if (message == WM_NULL) { ut_println("got(WM_NULL)"); }
    ui_app.now = ut_clock.seconds();
    if (ui_app.window == null) {
        ui_app.window = (ui_window_t)window;
    } else {
        ut_assert(ui_app_window() == window);
    }
    ut_work_queue.dispatch(&ui_app_queue);
    ui_app_update_wt_timeout(); // because head might have changed
    const int32_t m  = (int32_t)message;
    const int64_t wp = (int64_t)w_param;
    const int64_t lp = (int64_t)l_param;
    int64_t ret = 0;
    ui_app_update_mouse_buttons_state();
    ui_view.lose_hidden_focus(ui_app.root);
    ui_app_click_detector((uint32_t)m, (WPARAM)wp, (LPARAM)lp);
    if (ui_view.message(ui_app.root, m, wp, lp, &ret)) {
        return (LRESULT)ret;
    }
    if (m == ui.message.opening) { ui_app_window_opening(); return 0; }
    if (m == ui.message.closing) { ui_app_window_closing(); return 0; }
    if (m == ui.message.animate) {
        ui_app_animate_step((ui_app_animate_function_t)lp, (int32_t)wp, -1);
        return 0;
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
//          if (m == WM_LBUTTONDOWN)   { ut_println("WM_LBUTTONDOWN"); }
//          if (m == WM_LBUTTONUP)     { ut_println("WM_LBUTTONUP"); }
//          if (m == WM_LBUTTONDBLCLK) { ut_println("WM_LBUTTONDBLCLK"); }
            ui_app_wm_mouse(m, wp, lp);
            break;
        case WM_MOUSEHOVER      :
        case WM_MOUSEMOVE       :
            ui_app_wm_mouse(m, wp, lp);
            break;
        case WM_MOUSEWHEEL   :
            ui_app_wm_mouse_wheel(true, wp);
            break;
        case WM_MOUSEHWHEEL  :
            ui_app_wm_mouse_wheel(false, wp);
            break;
        // debugging:
        #ifdef UI_APP_DEBUGING_ALT_KEYBOARD_SHORTCUTS
        case WM_PARENTNOTIFY  : ut_println("WM_PARENTNOTIFY");     break;
        case WM_ENTERMENULOOP : ut_println("WM_ENTERMENULOOP");    return 0;
        case WM_EXITMENULOOP  : ut_println("WM_EXITMENULOOP");     return 0;
        case WM_INITMENU      : ut_println("WM_INITMENU");         return 0;
        case WM_MENUCHAR      : ut_println("WM_MENUCHAR");         return MNC_CLOSE << 16;
        case WM_CAPTURECHANGED: ut_println("WM_CAPTURECHANGED");   break;
        case WM_MENUSELECT    : ut_println("WM_MENUSELECT");       return 0;
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
            ut_println("WM_IME_CHAR: 0x%04X", wp);
            break;
        case WM_IME_NOTIFY:
            ut_println("WM_IME_NOTIFY");
            break;
        case WM_IME_REQUEST:
            ut_println("WM_IME_REQUEST");
            break;
        case WM_IME_STARTCOMPOSITION:
            ut_println("WM_IME_STARTCOMPOSITION");
            break;
        case WM_IME_ENDCOMPOSITION:
            ut_println("WM_IME_ENDCOMPOSITION");
            break;
        case WM_IME_COMPOSITION:
            ut_println("WM_IME_COMPOSITION");
            ui_app_ime_composition(lp);
            break;
#endif  // UI_APP_USE_WM_IME
        // TODO:
        case WM_UNICHAR       : // only UTF-32 via PostMessage?
            ut_println("???");
            // see: https://learn.microsoft.com/en-us/windows/win32/inputdev/about-keyboard-input
            // https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-tounicode
            break;
        default:
            break;
    }
    return DefWindowProcW(ui_app_window(), (UINT)m, (WPARAM)wp, lp);
}

static long ui_app_get_window_long(int32_t index) {
    ut_runtime.set_err(0);
    long v = GetWindowLongA(ui_app_window(), index);
    ut_fatal_if_error(ut_runtime.err());
    return v;
}

static long ui_app_set_window_long(int32_t index, long value) {
    ut_runtime.set_err(0);
    long r = SetWindowLongA(ui_app_window(), index, value); // r previous value
    ut_fatal_if_error(ut_runtime.err());
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
        ut_assert(ui_color_is_8bit(color));
        c = ui_gdi.color_rgb(color);
    }
    return ut_b2e(SetLayeredWindowAttributes(ui_app_window(), c, a, mask));
}

static void ui_app_set_dwm_attribute(uint32_t mode, void* a, DWORD bytes) {
    ut_fatal_if_error(DwmSetWindowAttribute(ui_app_window(), mode, a, bytes));
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
        ut_fatal_if_error(
            DwmExtendFrameIntoClientArea(ui_app_window(), &margins)
        );
    }
}

static void ui_app_swp(HWND top, int32_t x, int32_t y, int32_t w, int32_t h,
        uint32_t f) {
    ut_fatal_win32err(SetWindowPos(ui_app_window(), top, x, y, w, h, f));
}

static void ui_app_swp_flags(uint32_t f) {
    ut_fatal_win32err(SetWindowPos(ui_app_window(), null, 0, 0, 0, 0, f));
}

static void ui_app_disable_sys_menu_item(HMENU sys_menu, uint32_t item) {
    const uint32_t f = MF_BYCOMMAND | MF_DISABLED;
    ut_fatal_win32err(EnableMenuItem(sys_menu, item, f));
}

static void ui_app_init_sys_menu(void) {
    // tried to remove unused items from system menu which leads to
    // AllowDarkModeForWindow() failed 0x000005B0(1456) "A menu item was not found."
    // SetPreferredAppMode() failed 0x000005B0(1456) "A menu item was not found."
    // this is why they just disabled instead.
    HMENU sys_menu = GetSystemMenu(ui_app_window(), false);
    ut_not_null(sys_menu);
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
    ut_str.utf8to16(class_name, ut_countof(class_name), ui_app.class_name, -1);
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
    ut_fatal_if(atom == 0);
    uint16_t title[256];
    ut_str.utf8to16(title, ut_countof(title), ui_app.title, -1);
    HWND window = CreateWindowExW(WS_EX_COMPOSITED | WS_EX_LAYERED,
        class_name, title, ui_app_window_style(),
        r.x, r.y, r.w, r.h, null, null, wc->hInstance, null);
    ut_not_null(ui_app.window);
    ut_swear(window == ui_app_window());
    ui_app.show_window(ui.visibility.hide);
    ui_view.set_text(&ui_caption.title, "%s", ui_app.title);
    ui_app.dpi.window = (int32_t)GetDpiForWindow(ui_app_window());
    RECT wrc = ui_app_ui2rect(&r);
    ut_fatal_win32err(GetWindowRect(ui_app_window(), &wrc));
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
            ut_fatal_win32err(GetWindowPlacement(ui_app_window(), &wp));
            WINDOWPLACEMENT nwp = wp;
            nwp.showCmd = SW_SHOWNORMAL;
            nwp.rcNormalPosition = (RECT){ui_app.mrc.x, ui_app.mrc.y,
                ui_app.mrc.x + ui_app.mrc.w, ui_app.mrc.y + ui_app.mrc.h};
            ut_fatal_win32err(SetWindowPlacement(ui_app_window(), &nwp));
        } else {
            ut_fatal_win32err(SetWindowPlacement(ui_app_window(), &wp));
            ui_app_set_window_long(GWL_STYLE, ui_app_window_style());
            enum { flags = SWP_FRAMECHANGED | SWP_NOMOVE | SWP_NOSIZE |
                           SWP_NOZORDER | SWP_NOOWNERZORDER };
            ui_app_swp_flags(flags);
        }
        ui_app.is_full_screen = on;
    }
}

static bool ui_app_set_focus(ui_view_t* ut_unused(v)) { return false; }

static void ui_app_request_redraw(void) {  // < 2us
    SetEvent(ui_app_event_invalidate);
}

static void ui_app_draw(void) {
    ut_println("avoid at all cost. bad performance, bad UX");
    UpdateWindow(ui_app_window());
}

static void ui_app_invalidate_rect(const ui_rect_t* r) {
    RECT rc = ui_app_ui2rect(r);
    InvalidateRect(ui_app_window(), &rc, false);
//  ut_bt_here();
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
    ut_work_queue.flush(&ui_app_queue);
    ut_assert(msg.message == WM_QUIT);
    return (int32_t)msg.wParam;
}

static void ui_app_dispose(void) {
    ui_app_dispose_fonts();
    ut_event.dispose(ui_app_event_invalidate);
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
    ut_fatal_win32err(CreateCaret(ui_app_window(), null, w, h));
    ut_assert(GetSystemMetrics(SM_CARETBLINKINGENABLED));
}

static void ui_app_invalidate_caret(void) {
    if (ui_app_caret_w >  0 && ui_app_caret_h >  0 &&
        ui_app_caret_x >= 0 && ui_app_caret_y >= 0 &&
        ui_app_caret_shown) {
        RECT rc = { ui_app_caret_x, ui_app_caret_y,
                    ui_app_caret_x + ui_app_caret_w,
                    ui_app_caret_y + ui_app_caret_h };
        ut_fatal_win32err(InvalidateRect(ui_app_window(), &rc, false));
    }
}

static void ui_app_show_caret(void) {
    ut_assert(!ui_app_caret_shown);
    ut_fatal_win32err(ShowCaret(ui_app_window()));
    ui_app_caret_shown = true;
    ui_app_invalidate_caret();
}

static void ui_app_move_caret(int32_t x, int32_t y) {
    ui_app_invalidate_caret(); // where is was
    ui_app_caret_x = x;
    ui_app_caret_y = y;
    ut_fatal_win32err(SetCaretPos(x, y));
    ui_app_invalidate_caret(); // where it is now
}

static void ui_app_hide_caret(void) {
    ut_assert(ui_app_caret_shown);
    ut_fatal_win32err(HideCaret(ui_app_window()));
    ui_app_invalidate_caret();
    ui_app_caret_shown = false;
}

static void ui_app_destroy_caret(void) {
    ui_app_caret_w = 0;
    ui_app_caret_h = 0;
    ut_fatal_win32err(DestroyCaret());
}

static void ui_app_beep(int32_t kind) {
    static int32_t beep_id[] = { MB_OK, MB_ICONINFORMATION, MB_ICONQUESTION,
                          MB_ICONWARNING, MB_ICONERROR};
    ut_swear(0 <= kind && kind < ut_countof(beep_id));
    ut_fatal_win32err(MessageBeep(beep_id[kind]));
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
        ut_println("GetConsoleScreenBufferInfoEx() %s", ut_strerr(r));
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
        int r0 = SetConsoleWindowInfo(console, true, &min_win) ? 0 : ut_runtime.err();
//      if (r0 != 0) { ut_println("SetConsoleWindowInfo() %s", ut_strerr(r0)); }
        int r1 = SetConsoleScreenBufferSize(console, c) ? 0 : ut_runtime.err();
//      if (r1 != 0) { ut_println("SetConsoleScreenBufferSize() %s", ut_strerr(r1)); }
        if (r0 != 0 || r1 != 0) { // try in reverse order (which expected to work):
            r0 = SetConsoleScreenBufferSize(console, c) ? 0 : ut_runtime.err();
            if (r0 != 0) { ut_println("SetConsoleScreenBufferSize() %s", ut_strerr(r0)); }
            r1 = SetConsoleWindowInfo(console, true, &min_win) ? 0 : ut_runtime.err();
            if (r1 != 0) { ut_println("SetConsoleWindowInfo() %s", ut_strerr(r1)); }
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
    ut_fatal_if_error(r, "GetConsoleMode() %s", ut_strerr(r));
    mode &= ~ENABLE_AUTO_POSITION;
    r = SetConsoleMode(console, &mode) ? 0 : ut_runtime.err();
    ut_fatal_if_error(r, "SetConsoleMode() %s", ut_strerr(r));
    */
    CONSOLE_SCREEN_BUFFER_INFOEX info = { sizeof(CONSOLE_SCREEN_BUFFER_INFOEX) };
    int r = GetConsoleScreenBufferInfoEx(console, &info) ? 0 : ut_runtime.err();
    ut_fatal_if_error(r, "GetConsoleScreenBufferInfoEx() %s", ut_strerr(r));
    COORD c = GetLargestConsoleWindowSize(console);
    if (c.X > 80) { c.X &= ~0x7; }
    if (c.Y > 24) { c.Y &= ~0x3; }
    if (c.X > 80) { c.X -= 8; }
    if (c.Y > 24) { c.Y -= 4; }
    ui_app_set_console_size(c.X, c.Y);
    r = GetConsoleScreenBufferInfoEx(console, &info) ? 0 : ut_runtime.err();
    ut_fatal_if_error(r, "GetConsoleScreenBufferInfoEx() %s", ut_strerr(r));
    info.dwSize.Y = 9999; // maximum value at the moment of implementation
    r = SetConsoleScreenBufferInfoEx(console, &info) ? 0 : ut_runtime.err();
    ut_fatal_if_error(r, "SetConsoleScreenBufferInfoEx() %s", ut_strerr(r));
    ui_app_save_console_pos();
}

static void ui_app_make_topmost(void) {
    //  Places the window above all non-topmost windows.
    // The window maintains its topmost position even when it is deactivated.
    enum { swp = SWP_SHOWWINDOW | SWP_NOREPOSITION | SWP_NOMOVE | SWP_NOSIZE };
    ui_app_swp(HWND_TOPMOST, 0, 0, 0, 0, swp);
}

static void ui_app_activate(void) {
    ut_runtime.set_err(0);
    HWND previous = SetActiveWindow(ui_app_window());
    if (previous == null) { ut_fatal_if_error(ut_runtime.err()); }
}

static void ui_app_bring_to_foreground(void) {
    // SetForegroundWindow() does not activate window:
    ut_fatal_win32err(SetForegroundWindow(ui_app_window()));
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
    ut_fatal_win32err(SetWindowTextA(ui_app_window(), ut_nls.str(title)));
}

static void ui_app_capture_mouse(bool on) {
    static int32_t mouse_capture;
    if (on) {
        ut_swear(mouse_capture == 0);
        mouse_capture++;
        SetCapture(ui_app_window());
    } else {
        ut_swear(mouse_capture == 1);
        mouse_capture--;
        ReleaseCapture();
    }
}

static void ui_app_move_and_resize(const ui_rect_t* rc) {
    enum { swp = SWP_NOZORDER | SWP_NOACTIVATE };
    ui_app_swp(null, rc->x, rc->y, rc->w, rc->h, swp);
}

static void ui_app_set_console_title(HWND cw) {
    ut_swear(ut_thread.id() == ui_app.tid);
    static char text[256];
    text[0] = 0;
    GetWindowTextA((HWND)ui_app.window, text, ut_countof(text));
    text[ut_countof(text) - 1] = 0;
    char title[256];
    ut_str_printf(title, "%s - Console", text);
    ut_fatal_win32err(SetWindowTextA(cw, title));
}

static void ui_app_restore_console(int32_t *visibility) {
    HWND cw = GetConsoleWindow();
    if (cw != null) {
        RECT wr = {0};
        GetWindowRect(cw, &wr);
        ui_rect_t rc = ui_app_rect2ui(&wr);
        ui_app_load_console_pos(&rc, visibility);
        if (rc.w > 0 && rc.h > 0) {
//          ut_println("%d,%d %dx%d px", rc.x, rc.y, rc.w, rc.h);
            CONSOLE_SCREEN_BUFFER_INFOEX info = {
                sizeof(CONSOLE_SCREEN_BUFFER_INFOEX)
            };
            int32_t r = ut_config.load(ui_app.class_name,
                "console_screen_buffer_infoex", &info, (int32_t)sizeof(info));
            if (r == sizeof(info)) { // 24x80
                SMALL_RECT sr = info.srWindow;
                int16_t w = (int16_t)ut_max(sr.Right - sr.Left + 1, 80);
                int16_t h = (int16_t)ut_max(sr.Bottom - sr.Top + 1, 24);
//              ut_println("info: %dx%d", info.dwSize.X, info.dwSize.Y);
//              ut_println("%d,%d %dx%d", sr.Left, sr.Top, w, h);
                if (w > 0 && h > 0) { ui_app_set_console_size(w, h); }
    	    }
            // do not resize console window just restore it's position
            enum { flags = SWP_NOZORDER | SWP_NOACTIVATE | SWP_NOSIZE };
            ut_fatal_win32err(SetWindowPos(cw, null,
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
    ut_assert(ui_app.dpi.monitor_max > 0);
//  ut_println("ui_app.dpi.monitor_raw: %d", ui_app.dpi.monitor_max);
    return ui_app.dpi.monitor_max > 0 ?
           (fp32_t)pixels / (fp32_t)ui_app.dpi.monitor_max : 0;
}

static int32_t ui_app_in2px(fp32_t inches) {
    ut_assert(ui_app.dpi.monitor_max > 0);
//  ut_println("ui_app.dpi.monitor_raw: %d", ui_app.dpi.monitor_max);
    return (int32_t)(inches * (fp64_t)ui_app.dpi.monitor_max + 0.5);
}

static void ui_app_request_layout(void) {
    ui_app_layout_dirty = true;
    ui_app.request_redraw();
}

static void ui_app_show_window(int32_t show) {
    ut_assert(ui.visibility.hide <= show &&
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
    ut_swear(ut_thread.id() == ui_app.tid);
    ut_assert(pairs == null && n == 0 || n >= 2 && n % 2 == 0);
    static uint16_t memory[4 * 1024];
    uint16_t* filter = memory;
    if (pairs == null || n == 0) {
        filter = L"All Files\0*\0\0";
    } else {
        int32_t left = ut_countof(memory) - 2;
        uint16_t* s = memory;
        for (int32_t i = 0; i < n; i+= 2) {
            uint16_t* s0 = s;
            ut_str.utf8to16(s0, left, pairs[i + 0], -1);
            int32_t n0 = (int32_t)ut_str.len16(s0);
            ut_assert(n0 > 0);
            s += n0 + 1;
            left -= n0 + 1;
            uint16_t* s1 = s;
            ut_str.utf8to16(s1, left, pairs[i + 1], -1);
            int32_t n1 = (int32_t)ut_str.len16(s1);
            ut_assert(n1 > 0);
            s[n1] = 0;
            s += n1 + 1;
            left -= n1 + 1;
        }
        *s++ = 0;
    }
    static uint16_t dir[ut_files_max_path];
    dir[0] = 0;
    ut_str.utf8to16(dir, ut_countof(dir), folder, -1);
    static uint16_t path[ut_files_max_path];
    path[0] = 0;
    OPENFILENAMEW ofn = { sizeof(ofn) };
    ofn.hwndOwner = (HWND)ui_app.window;
    ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;
    ofn.lpstrFilter = filter;
    ofn.lpstrInitialDir = dir;
    ofn.lpstrFile = path;
    ofn.nMaxFile = sizeof(path);
    static ut_file_name_t fn;
    fn.s[0] = 0;
    if (GetOpenFileNameW(&ofn) && path[0] != 0) {
        ut_str.utf16to8(fn.s, ut_countof(fn.s), path, -1);
    } else {
        fn.s[0] = 0;
    }
    return fn.s;
}

// TODO: use clipboard instead?

static errno_t ui_app_clipboard_put_image(ui_image_t* im) {
    HDC canvas = GetDC(null);
    ut_not_null(canvas);
    HDC src = CreateCompatibleDC(canvas); ut_not_null(src);
    HDC dst = CreateCompatibleDC(canvas); ut_not_null(dst);
    // CreateCompatibleBitmap(dst) will create monochrome bitmap!
    // CreateCompatibleBitmap(canvas) will create display compatible
    HBITMAP bitmap = CreateCompatibleBitmap(canvas, im->w, im->h);
//  HBITMAP bitmap = CreateBitmap(image.w, image.h, 1, 32, null);
    ut_not_null(bitmap);
    HBITMAP s = SelectBitmap(src, im->bitmap); ut_not_null(s);
    HBITMAP d = SelectBitmap(dst, bitmap);     ut_not_null(d);
    POINT pt = { 0 };
    ut_fatal_win32err(SetBrushOrgEx(dst, 0, 0, &pt));
    ut_fatal_win32err(StretchBlt(dst, 0, 0, im->w, im->h, src, 0, 0,
        im->w, im->h, SRCCOPY));
    errno_t r = ut_b2e(OpenClipboard(GetDesktopWindow()));
    if (r != 0) { ut_println("OpenClipboard() failed %s", ut_strerr(r)); }
    if (r == 0) {
        r = ut_b2e(EmptyClipboard());
        if (r != 0) { ut_println("EmptyClipboard() failed %s", ut_strerr(r)); }
    }
    if (r == 0) {
        r = ut_b2e(SetClipboardData(CF_BITMAP, bitmap));
        if (r != 0) {
            ut_println("SetClipboardData() failed %s", ut_strerr(r));
        }
    }
    if (r == 0) {
        r = ut_b2e(CloseClipboard());
        if (r != 0) {
            ut_println("CloseClipboard() failed %s", ut_strerr(r));
        }
    }
    ut_not_null(SelectBitmap(dst, d));
    ut_not_null(SelectBitmap(src, s));
    ut_fatal_win32err(DeleteBitmap(bitmap));
    ut_fatal_win32err(DeleteDC(dst));
    ut_fatal_win32err(DeleteDC(src));
    ut_fatal_win32err(ReleaseDC(null, canvas));
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
    ut_assert(ut_thread.id() == ui_app.tid, "cannot be called from background thread");
    ut_runtime.set_err(0);
    w = SetFocus((HWND)w); // w previous focused window
    if (w == null) { ut_fatal_if_error(ut_runtime.err()); }
}

static void ui_app_request_focus(void) {
    window_request_focus(ui_app.window);
}

static void ui_app_init(void) {
    ui_app_event_quit           = ut_event.create_manual();
    ui_app_event_invalidate     = ut_event.create();
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
    ut_assert(ui_app.content->type == ui_view_stack);
    ut_assert(ui_app.content->background == ui_colors.transparent);
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
    ui_app.init();
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
    errno_t error = ut_b2e(SetProcessDpiAwarenessContext(
            DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2));
    if (error == ERROR_ACCESS_DENIED) {
        ut_println("Warning: SetProcessDpiAwarenessContext(): ERROR_ACCESS_DENIED");
        // dpi awareness already set, manifest, registry, windows policy
        // Try via Shell:
        HRESULT hr = SetProcessDpiAwareness(PROCESS_PER_MONITOR_DPI_AWARE);
        if (hr == E_ACCESSDENIED) {
            ut_println("Warning: SetProcessDpiAwareness(): E_ACCESSDENIED");
        }
    }
    DPI_AWARENESS_CONTEXT dpi_awareness_context_2 =
        GetThreadDpiAwarenessContext();
    ut_swear(dpi_awareness_context_1 != dpi_awareness_context_2);
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
//  ut_println("ui_app.dpi.monitor_max := %d", ui_app.dpi.system);
    static const RECT nowhere = {0x7FFFFFFF, 0x7FFFFFFF, 0x7FFFFFFF, 0x7FFFFFFF};
    ui_rect_t r = ui_app_rect2ui(&nowhere);
    ui_app_update_mi(&r, MONITOR_DEFAULTTOPRIMARY);
    ui_app.dpi.window = ui_app.dpi.monitor_effective;
}

static ui_rect_t ui_app_window_initial_rectangle(void) {
    const ui_window_sizing_t* ws = &ui_app.window_sizing;
    // it is not practical and thus not implemented handling
    // == (0, 0) and != (0, 0) for sizing half dimension (only w or only h)
    ut_swear((ws->min_w != 0) == (ws->min_h != 0) &&
           ws->min_w >= 0 && ws->min_h >= 0,
          "ui_app.window_sizing .min_w=%.1f .min_h=%.1f", ws->min_w, ws->min_h);
    ut_swear((ws->ini_w != 0) == (ws->ini_h != 0) &&
           ws->ini_w >= 0 && ws->ini_h >= 0,
          "ui_app.window_sizing .ini_w=%.1f .ini_h=%.1f", ws->ini_w, ws->ini_h);
    ut_swear((ws->max_w != 0) == (ws->max_h != 0) &&
           ws->max_w >= 0 && ws->max_h >= 0,
          "ui_app.window_sizing .max_w=%.1f .max_h=%.1f", ws->max_w, ws->max_h);
    // if max is set then min and ini must be less than max
    if (ws->max_w != 0 || ws->max_h != 0) {
        ut_swear(ws->min_w <= ws->max_w && ws->min_h <= ws->max_h,
            "ui_app.window_sizing .min_w=%.1f .min_h=%.1f .max_w=%1.f .max_h=%.1f",
             ws->min_w, ws->min_h, ws->max_w, ws->max_h);
        ut_swear(ws->ini_w <= ws->max_w && ws->ini_h <= ws->max_h,
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
    // T-connector for intercepting ut_debug.output:
    bool (*tee)(const char* s, int32_t n) = ut_debug.tee;
    ut_debug.tee = ui_app_write_backtrace;
    const char* home = ut_files.known_folder(ut_files.folder.home);
    if (home != null) {
        const char* name = ui_app.class_name  != null ?
                           ui_app.class_name : "ui_app";
        ut_str_printf(fn, "%s\\%s_crash_log.txt", home, name);
        ui_app_crash_log = fopen(fn, "w");
    }
    ut_debug.println(null, 0, null,
        "To file and issue report copy this log and");
    ut_debug.println(null, 0, null,
        "paste it here: https://github.com/leok7v/ui/discussions/4");
    ut_debug.println(null, 0, null,
        "%s exception: %s", ut_args.basename(), ut_str.error(ex));
    ut_bt_t bt = {{0}};
    ut_bt.context(ut_thread.self(), ep->ContextRecord, &bt);
    ut_bt.trace(&bt, "*");
    ut_bt.trace_all_but_self();
    ut_debug.tee = tee;
    if (ui_app_crash_log != null) {
        fclose(ui_app_crash_log);
        char cmd[1024];
        ut_str_printf(cmd, "cmd.exe /c start notepad \"%s\"", fn);
        system(cmd);
    }
    return EXCEPTION_CONTINUE_SEARCH;
}

#undef UI_APP_TEST_POST

#ifdef UI_APP_TEST_POST

// The dispatch_until() is just for testing purposes.
// Usually ut_work_queue.dispatch(q) will be called inside each
// iteration of message loop of a dispatch [UI] thread.

static void ui_app_test_dispatch_until(ut_work_queue_t* q, int32_t* i,
        const int32_t n) {
    while (q->head != null && *i < n) {
        ut_thread.sleep_for(0.0001); // 100 microseconds
        ut_work_queue.dispatch(q);
    }
    ut_work_queue.flush(q);
}

// simple way of passing a single pointer to call_later

static void ui_app_test_every_100ms(ut_work_t* w) {
    int32_t* i = (int32_t*)w->data;
    ut_println("i: %d", *i);
    (*i)++;
    w->when = ut_clock.seconds() + 0.100;
    ut_work_queue.post(w);
}

static void ui_app_test_work_queue_1(void) {
    ut_work_queue_t queue = {0};
    // if a single pointer will suffice
    int32_t i = 0;
    ut_work_t work = {
        .queue = &queue,
        .when  = ut_clock.seconds() + 0.100,
        .work  = ui_app_test_every_100ms,
        .data  = &i
    };
    ut_work_queue.post(&work);
    ui_app_test_dispatch_until(&queue, &i, 4);
}

// extending ut_work_t with extra data:

typedef struct ut_work_ex_s {
    union {
        ut_work_t base;
        struct ut_work_s;
    };
    struct { int32_t a; int32_t b; } s;
    int32_t i;
} ut_work_ex_t;

static void ui_app_test_every_200ms(ut_work_t* w) {
    ut_work_ex_t* ex = (ut_work_ex_t*)w;
    ut_println("ex { .i: %d, .s.a: %d .s.b: %d}", ex->i, ex->s.a, ex->s.b);
    ex->i++;
    const int32_t swap = ex->s.a; ex->s.a = ex->s.b; ex->s.b = swap;
    w->when = ut_clock.seconds() + 0.200;
    ut_work_queue.post(w);
}

static void ui_app_test_work_queue_2(void) {
    ut_work_queue_t queue = {0};
    ut_work_ex_t work = {
        .queue = &queue,
        .when  = ut_clock.seconds() + 0.200,
        .work  = ui_app_test_every_200ms,
        .data  = null,
        .s = { .a = 1, .b = 2 },
        .i = 0
    };
    ut_work_queue.post(&work.base);
    ui_app_test_dispatch_until(&queue, &work.i, 4);
}

static fp64_t ui_app_test_timestamp_0;
static fp64_t ui_app_test_timestamp_2;
static fp64_t ui_app_test_timestamp_3;
static fp64_t ui_app_test_timestamp_4;

static void ui_app_test_in_1_second(ut_work_t* ut_unused(work)) {
    ui_app_test_timestamp_3 = ut_clock.seconds();
    ut_println("ETA 3 seconds");
}

static void ui_app_test_in_2_seconds(ut_work_t* ut_unused(work)) {
    ui_app_test_timestamp_2 = ut_clock.seconds();
    ut_println("ETA 2 seconds");
    static ut_work_t invoke_in_1_seconds;
    invoke_in_1_seconds = (ut_work_t){
        .queue = null, // &ui_app_queue will be used
        .when = ut_clock.seconds() + 1.0, // seconds
        .work = ui_app_test_in_1_second
    };
    ui_app.post(&invoke_in_1_seconds);
}

static void ui_app_test_in_4_seconds(ut_work_t* ut_unused(work)) {
    ui_app_test_timestamp_4 = ut_clock.seconds();
    ut_println("ETA 4 seconds");
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
    ut_println("see Output/Timestamps");
    static ut_work_t invoke_in_2_seconds;
    static ut_work_t invoke_in_4_seconds;
    ui_app_test_timestamp_0 = ut_clock.seconds();
    invoke_in_2_seconds = (ut_work_t){
        .queue = null, // &ui_app_queue will be used
        .when = ut_clock.seconds() + 2.0, // seconds
        .work = ui_app_test_in_2_seconds
    };
    invoke_in_4_seconds = (ut_work_t){
        .queue = null, // &ui_app_queue will be used
        .when = ut_clock.seconds() + 4.0, // seconds
        .work = ui_app_test_in_4_seconds
    };
    ui_app.post(&invoke_in_4_seconds);
    ui_app.post(&invoke_in_2_seconds);
}

#endif

static int ui_app_win_main(HINSTANCE instance) {
    // IDI_ICON 101:
    ui_app.icon = (ui_icon_t)LoadIconW(instance, MAKEINTRESOURCE(101));
    ut_not_null(ui_app.init);
    ui_app_init_windows();
    ui_gdi.init();
    ut_clipboard.put_image = ui_app_clipboard_put_image;
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
        ui_app.border.w = ut_min(max_border, ui_app.border.w);
        ui_app.border.h = ut_min(max_border, ui_app.border.h);
    }
//  ut_println("frame: %d,%d caption_height: %d", ui_app.border.w, ui_app.border.h, ui_app.caption_height);
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
    ui_app.root->fm = &ui_app.fm.regular;
    ui_app.root->w = wr.w - ui_app.border.w * 2;
    ui_app.root->h = wr.h - ui_app.border.h * 2 - ui_app.caption_height;
    ui_app_layout_dirty = true; // layout will be done before first paint
    ut_not_null(ui_app.class_name);
    ui_app_wt = (ut_event_t)CreateWaitableTimerA(null, false, null);
    ut_thread_t alarm  = ut_thread.start(ui_app_alarm_thread, null);
    if (!ui_app.no_ui) {
        ui_app_create_window(wr);
        ui_app_init_fonts(ui_app.dpi.window);
        ut_thread_t redraw = ut_thread.start(ui_app_redraw_thread, null);
        #ifdef UI_APP_TEST_POST
            ui_app_test_post();
        #endif
        r = ui_app_message_loop();
        // ui_app.fini() must be called before ui_app_dispose()
        if (ui_app.fini != null) { ui_app.fini(); }
        ut_event.set(ui_app_event_quit);
        ut_thread.join(redraw, -1);
        ui_app_dispose();
        if (r == 0 && ui_app.exit_code != 0) { r = ui_app.exit_code; }
    } else {
        r = ui_app.main();
        if (ui_app.fini != null) { ui_app.fini(); }
    }
    ut_event.set(ui_app_event_quit);
    ut_thread.join(alarm, -1);
    ut_event.dispose(ui_app_event_quit);
    ui_app_event_quit = null;
    ut_event.dispose(ui_app_wt);
    ui_app_wt = null;
    ui_gdi.fini();
    return r;
}

#pragma warning(disable: 28251) // inconsistent annotations

int WINAPI WinMain(HINSTANCE instance, HINSTANCE ut_unused(previous),
        char* ut_unused(command), int show) {
    SetUnhandledExceptionFilter(ui_app_exception_filter);
    const COINIT co_init = COINIT_MULTITHREADED | COINIT_SPEED_OVER_MEMORY;
    ut_fatal_if_error(CoInitializeEx(0, co_init));
    SetConsoleCP(CP_UTF8);
    // Expected manifest.xml containing UTF-8 code page
    // for Translate message and WM_CHAR to deliver UTF-8 characters
    // see: https://learn.microsoft.com/en-us/windows/apps/design/globalizing/use-utf8-code-page
    if (GetACP() != 65001) {
        ut_println("codepage: %d UTF-8 will not be supported", GetACP());
    }
    // at the moment of writing there is no API call to inform Windows about process
    // preferred codepage except manifest.xml file in resource #1.
    // Absence of manifest.xml will result to ancient and useless ANSI 1252 codepage
    // TODO: may need to change CreateWindowA() to CreateWindowW() and
    // translate UTF16 to UTF8
    ui_app.tid = ut_thread.id();
    ut_nls.init();
    ui_app.visibility = show;
    ut_args.WinMain();
    int32_t r = ui_app_win_main(instance);
    ut_args.fini();
    return r;
}

int main(int argc, const char* argv[], const char** envp) {
    SetUnhandledExceptionFilter(ui_app_exception_filter);
    ut_fatal_if_error(CoInitializeEx(0, COINIT_MULTITHREADED | COINIT_SPEED_OVER_MEMORY));
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
#pragma comment(lib, "shcore")
#pragma comment(lib, "uxtheme")
// _______________________________ ui_button.c ________________________________

#include "ut/ut.h"

static void ui_button_every_100ms(ui_view_t* v) { // every 100ms
    if (v->p.armed_until != 0 && ui_app.now > v->p.armed_until) {
        v->p.armed_until = 0;
        v->state.armed = false;
        ui_view.invalidate(v, null);
    }
    if (v->p.armed_until != 0) { ui_app.show_hint(null, -1, -1, 0); }
}

static void ui_button_paint(ui_view_t* v) {
    bool pressed = (v->state.armed ^ v->state.pressed) == 0;
    if (v->p.armed_until != 0) { pressed = true; }
    const int32_t w = v->w;
    const int32_t h = v->h;
    const int32_t x = v->x;
    const int32_t y = v->y;
    const int32_t r = (0x1 | ut_max(3, v->fm->em.h / 4));  // odd radius
    const fp32_t d = ui_theme.is_app_dark() ? 0.50f : 0.25f;
    ui_color_t d0 = ui_colors.darken(v->background, d);
    const fp32_t d2 = d / 2;
    if (v->flat) {
        if (v->state.hover) {
            ui_color_t d1 = ui_theme.is_app_dark() ?
                    ui_colors.lighten(v->background, d2) :
                    ui_colors.darken(v->background,  d2);
            if (!pressed) {
                ui_gdi.gradient(x, y, w, h, d0, d1, true);
            } else {
                ui_gdi.gradient(x, y, w, h, d1, d0, true);
            }
        }
    } else {
        // `bc` border color
        ui_color_t bc = ui_colors.get_color(ui_color_id_gray_text);
        if (v->state.armed) { bc = ui_colors.lighten(bc, 0.125f); }
        if (ui_view.is_disabled(v)) { bc = ui_color_rgb(30, 30, 30); } // TODO: hardcoded
        if (v->state.hover && !v->state.armed) {
            bc = ui_colors.get_color(ui_color_id_hot_tracking);
        }
        ui_color_t d1 = ui_colors.darken(v->background, d2);
        ui_color_t fc = ui_colors.interpolate(d0, d1, 0.5f); // fill color
        if (v->state.armed) {
            fc = ui_colors.lighten(fc, 0.250f);
        } else if (v->state.hover) {
            fc = ui_colors.darken(fc, 0.250f);
        }
        ui_gdi.rounded(v->x, v->y, v->w, v->h, r, bc, fc);
    }
    const int32_t tx = v->x + v->text.xy.x;
    const int32_t ty = v->y + v->text.xy.y;
    if (v->icon == null) {
        ui_color_t c = v->color;
        if (v->state.hover && !v->state.armed) {
            c = ui_theme.is_app_dark() ? ui_color_rgb(0xFF, 0xE0, 0xE0) :
                                         ui_color_rgb(0x00, 0x40, 0xFF);
        }
        if (ui_view.is_disabled(v)) { c = ui_colors.get_color(ui_color_id_gray_text); }
        if (v->debug.paint.fm) {
            ui_view.debug_paint_fm(v);
        }
        const ui_gdi_ta_t ta = { .fm = v->fm, .color = c };
        ui_gdi.text(&ta, tx, ty, "%s", ui_view.string(v));
    } else {
        const ui_ltrb_t i = ui_view.margins(v, &v->insets);
        const ui_wh_t i_wh = { .w = v->w - i.left - i.right,
                               .h = v->h - i.top - i.bottom };
        // TODO: icon text alignment
        ui_gdi.icon(tx, ty + v->text.xy.y, i_wh.w, i_wh.h, v->icon);
    }
}

static void ui_button_callback(ui_button_t* b) {
    ui_app.show_hint(null, -1, -1, 0);
    // for flip buttons the state of the button flips
    // *before* callback.
    if (b->flip) { b->state.pressed = !b->state.pressed; }
    const bool pressed = b->state.pressed;
    if (b->callback != null) { b->callback(b); }
    if (pressed != b->state.pressed) {
        if (b->flip) { // warn the client of strange logic:
            ut_println("strange flip the button with button.flip: true");
            // if client wants to flip pressed state manually it
            // should do it for the button.flip = false
        }
//      ut_println("disarmed immediately");
        b->p.armed_until = 0;
        b->state.armed = false;
    } else {
        if (b->flip) {
//          ut_println("disarmed immediately");
            b->p.armed_until = 0;
            b->state.armed = false;
        } else {
//          ut_println("will disarm in 1/4 seconds");
            b->p.armed_until = ui_app.now + 0.250;
        }
    }
}

static void ui_button_trigger(ui_view_t* v) {
    ui_button_t* b = (ui_button_t*)v;
    v->state.armed = true;
    ui_view.invalidate(v, null);
    ui_button_callback(b);
}

static void ui_button_character(ui_view_t* v, const char* utf8) {
    char ch = utf8[0]; // TODO: multibyte utf8 shortcuts?
    if (ui_view.is_shortcut_key(v, ch)) {
        ui_button_trigger(v);
    }
}

static bool ui_button_key_pressed(ui_view_t* v, int64_t key) {
    ut_assert(!ui_view.is_hidden(v) && !ui_view.is_disabled(v));
    const bool trigger = ui_app.alt && ui_view.is_shortcut_key(v, key);
    if (trigger) { ui_button_trigger(v); }
    return trigger; // swallow if true
}

static void ui_button_mouse_click(ui_view_t* v, int32_t ut_unused(ix),
        bool pressed) {
    // 'ix' ignored - button index acts on any mouse button
    ui_button_t* b = (ui_button_t*)v;
    ut_assert(ui_view.inside(b, &ui_app.mouse));
    ui_view.invalidate(v, null); // always on any press/release inside
    if (pressed && b->flip) {
        if (b->flip) { ui_button_callback(b); }
    } else if (pressed) {
        if (!v->state.armed) { ui_app.show_hint(null, -1, -1, 0); }
        v->state.armed = true;
    } else { // released
        if (!b->flip) { ui_button_callback(b); }
    }
}

void ui_view_init_button(ui_view_t* v) {
    ut_assert(v->type == ui_view_button);
    v->mouse_click   = ui_button_mouse_click;
    v->paint         = ui_button_paint;
    v->character     = ui_button_character;
    v->every_100ms   = ui_button_every_100ms;
    v->key_pressed   = ui_button_key_pressed;
    v->color_id      = ui_color_id_button_text;
    v->background_id = ui_color_id_button_face;
    if (v->debug.id == null) { v->debug.id = "#button"; }
}

void ui_button_init(ui_button_t* b, const char* label, fp32_t ems,
        void (*callback)(ui_button_t* b)) {
    b->type = ui_view_button;
    ui_view.set_text(b, "%s", label);
    b->callback = callback;
    b->min_w_em = ems;
    ui_view_init_button(b);
}
// _______________________________ ui_caption.c _______________________________

/* Copyright (c) Dmitry "Leo" Kuznetsov 2021-24 see LICENSE for details */
#include "ut/ut.h"

#pragma push_macro("ui_caption_glyph_rest")
#pragma push_macro("ui_caption_glyph_menu")
#pragma push_macro("ui_caption_glyph_dark")
#pragma push_macro("ui_caption_glyph_light")
#pragma push_macro("ui_caption_glyph_mini")
#pragma push_macro("ui_caption_glyph_maxi")
#pragma push_macro("ui_caption_glyph_full")
#pragma push_macro("ui_caption_glyph_quit")

#define ui_caption_glyph_rest  ut_glyph_white_square_with_upper_right_quadrant // instead of ut_glyph_desktop_window
#define ui_caption_glyph_menu  ut_glyph_trigram_for_heaven
#define ui_caption_glyph_dark  ut_glyph_crescent_moon
#define ui_caption_glyph_light ut_glyph_white_sun_with_rays
#define ui_caption_glyph_mini  ut_glyph_minimize
#define ui_caption_glyph_maxi  ut_glyph_white_square_with_lower_left_quadrant // instead of ut_glyph_maximize
#define ui_caption_glyph_full  ut_glyph_square_four_corners
#define ui_caption_glyph_quit  ut_glyph_cancellation_x

static void ui_caption_toggle_full(void) {
    ui_app.full_screen(!ui_app.is_full_screen);
    ui_caption.view.state.hidden = ui_app.is_full_screen;
    ui_app.request_layout();
}

static void ui_caption_esc_full_screen(ui_view_t* v, const char utf8[]) {
    ut_swear(v == ui_caption.view.parent);
    // TODO: inside ui_app.c instead of here?
    if (utf8[0] == 033 && ui_app.is_full_screen) { ui_caption_toggle_full(); }
}

static void ui_caption_quit(ui_button_t* ut_unused(b)) {
    ui_app.close();
}

static void ui_caption_mini(ui_button_t* ut_unused(b)) {
    ui_app.show_window(ui.visibility.minimize);
}

static void ui_caption_mode_appearance(void) {
    if (ui_theme.is_app_dark()) {
        ui_view.set_text(&ui_caption.mode, "%s", ui_caption_glyph_light);
        ut_str_printf(ui_caption.mode.hint, "%s", ut_nls.str("Switch to Light Mode"));
    } else {
        ui_view.set_text(&ui_caption.mode, "%s", ui_caption_glyph_dark);
        ut_str_printf(ui_caption.mode.hint, "%s", ut_nls.str("Switch to Dark Mode"));
    }
}

static void ui_caption_mode(ui_button_t* ut_unused(b)) {
    bool was_dark = ui_theme.is_app_dark();
    ui_app.light_mode =  was_dark;
    ui_app.dark_mode  = !was_dark;
    ui_theme.refresh();
    ui_caption_mode_appearance();
}

static void ui_caption_maximize_or_restore(void) {
    ui_view.set_text(&ui_caption.maxi, "%s",
        ui_app.is_maximized() ?
        ui_caption_glyph_rest : ui_caption_glyph_maxi);
    ut_str_printf(ui_caption.maxi.hint, "%s",
        ui_app.is_maximized() ?
        ut_nls.str("Restore") : ut_nls.str("Maximize"));
    // non-decorated windows on Win32 are "popup" style
    // that cannot be maximized. Full screen will serve
    // the purpose of maximization.
    ui_caption.maxi.state.hidden = ui_app.no_decor;
}

static void ui_caption_maxi(ui_button_t* ut_unused(b)) {
    if (!ui_app.is_maximized()) {
        ui_app.show_window(ui.visibility.maximize);
    } else if (ui_app.is_maximized() || ui_app.is_minimized()) {
        ui_app.show_window(ui.visibility.restore);
    }
    ui_caption_maximize_or_restore();
}

static void ui_caption_full(ui_button_t* ut_unused(b)) {
    ui_caption_toggle_full();
}

static int64_t ui_caption_hit_test(const ui_view_t* v, ui_point_t pt) {
    ut_swear(v == &ui_caption.view);
    ut_assert(ui_view.inside(v, &pt));
//  ut_println("%d,%d ui_caption.icon: %d,%d %dx%d inside: %d",
//      x, y,
//      ui_caption.icon.x, ui_caption.icon.y,
//      ui_caption.icon.w, ui_caption.icon.h,
//      ui_view.inside(&ui_caption.icon, &pt));
    if (ui_app.is_full_screen) {
        return ui.hit_test.client;
    } else if (!ui_caption.icon.state.hidden &&
                ui_view.inside(&ui_caption.icon, &pt)) {
        return ui.hit_test.system_menu;
    } else {
        ui_view_for_each(&ui_caption.view, c, {
            bool ignore = c->type == ui_view_stack ||
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
        ui_colors.get_color(ui_color_id_active_title) :
        ui_colors.get_color(ui_color_id_inactive_title);
    return c;
}

static const ui_margins_t ui_caption_button_button_padding =
    { .left  = 0.25,  .top    = 0.0,
      .right = 0.25,  .bottom = 0.0};

static void ui_caption_button_measure(ui_view_t* v) {
    ut_assert(v->type == ui_view_button);
    ui_view.measure_control(v);
    const int32_t dx = ui_app.caption_height - v->w;
    const int32_t dy = ui_app.caption_height - v->h;
    v->w += dx;
    v->h += dy;
    v->text.xy.x += dx / 2;
    v->text.xy.y += dy / 2;
    v->padding = ui_caption_button_button_padding;
}

static void ui_caption_button_icon_paint(ui_view_t* v) {
    int32_t w = v->w;
    int32_t h = v->h;
    while (h > 16 && (h & (h - 1)) != 0) { h--; }
    w = h;
    int32_t dx = (v->w - w) / 2;
    int32_t dy = (v->h - h) / 2;
    ui_gdi.icon(v->x + dx, v->y + dy, w, h, v->icon);
}

static void ui_caption_prepare(ui_view_t* ut_unused(v)) {
    ui_caption.title.state.hidden = false;
}

static void ui_caption_measured(ui_view_t* v) {
    // remeasure all child buttons with hard override:
    int32_t w = 0;
    ui_view_for_each(v, it, {
        if (it->type == ui_view_button) {
            it->fm = &ui_app.fm.mono;
            it->flat = true;
            ui_caption_button_measure(it);
        }
        if (!it->state.hidden) {
            const ui_ltrb_t p = ui_view.margins(it, &it->padding);
            w += it->w + p.left + p.right;
        }
    });
    const ui_ltrb_t p = ui_view.margins(v, &v->padding);
    w += p.left + p.right;
    // do not show title if there is not enough space
    ui_caption.title.state.hidden = w > ui_app.root->w;
    v->w = ui_app.root->w;
    const ui_ltrb_t insets = ui_view.margins(v, &v->insets);
    v->h = insets.top + ui_app.caption_height + insets.bottom;
}

static void ui_caption_composed(ui_view_t* v) {
    v->x = ui_app.root->x;
    v->y = ui_app.root->y;
}

static void ui_caption_paint(ui_view_t* v) {
    ui_color_t background = ui_caption_color();
    ui_gdi.fill(v->x, v->y, v->w, v->h, background);
}

static void ui_caption_init(ui_view_t* v) {
    ut_swear(v == &ui_caption.view, "caption is a singleton");
    ui_view_init_span(v);
    ui_caption.view.insets = (ui_margins_t){ 0.125, 0.0, 0.125, 0.0 };
    ui_caption.view.state.hidden = false;
    v->parent->character = ui_caption_esc_full_screen; // ESC for full screen
    ui_view.add(&ui_caption.view,
        &ui_caption.icon,
        &ui_caption.menu,
        &ui_caption.title,
        &ui_caption.spacer,
        &ui_caption.mode,
        &ui_caption.mini,
        &ui_caption.maxi,
        &ui_caption.full,
        &ui_caption.quit,
        null);
    ui_caption.view.color_id = ui_color_id_window_text;
    static const ui_margins_t p0 = { .left  = 0.0,   .top    = 0.0,
                                  .right = 0.0,   .bottom = 0.0};
    static const ui_margins_t pd = { .left  = 0.25,  .top    = 0.0,
                                  .right = 0.25,  .bottom = 0.0};
    static const ui_margins_t in = { .left  = 0.0,   .top    = 0.0,
                                  .right = 0.0,   .bottom = 0.0};
    ui_view_for_each(&ui_caption.view, c, {
        c->fm = &ui_app.fm.regular;
        c->color_id = ui_caption.view.color_id;
        if (c->type != ui_view_button) {
            c->padding = pd;
        }
        c->insets  = in;
        c->h = ui_app.caption_height;
        c->min_w_em = 0.5f;
        c->min_h_em = 0.5f;
    });
    ut_str_printf(ui_caption.menu.hint, "%s", ut_nls.str("Menu"));
    ut_str_printf(ui_caption.mode.hint, "%s", ut_nls.str("Switch to Light Mode"));
    ut_str_printf(ui_caption.mini.hint, "%s", ut_nls.str("Minimize"));
    ut_str_printf(ui_caption.maxi.hint, "%s", ut_nls.str("Maximize"));
    ut_str_printf(ui_caption.full.hint, "%s", ut_nls.str("Full Screen (ESC to restore)"));
    ut_str_printf(ui_caption.quit.hint, "%s", ut_nls.str("Close"));
    ui_caption.icon.icon     = ui_app.icon;
    ui_caption.icon.padding  = p0;
    ui_caption.icon.paint    = ui_caption_button_icon_paint;
    ui_caption.view.align    = ui.align.left;
    ui_caption.view.prepare  = ui_caption_prepare;
    ui_caption.view.measured = ui_caption_measured;
    ui_caption.view.composed = ui_caption_composed;
    ui_view.set_text(&ui_caption.view, "#ui_caption"); // for debugging
    ui_caption_maximize_or_restore();
    ui_caption.view.paint = ui_caption_paint;
    ui_caption_mode_appearance();
    ui_caption.icon.debug.id = "#caption.icon";
    ui_caption.menu.debug.id = "#caption.menu";
    ui_caption.mode.debug.id = "#caption.mode";
    ui_caption.mini.debug.id = "#caption.mini";
    ui_caption.maxi.debug.id = "#caption.maxi";
    ui_caption.full.debug.id = "#caption.full";
    ui_caption.quit.debug.id = "#caption.quit";
    ui_caption.title.debug.id  = "#caption.title";
    ui_caption.spacer.debug.id = "#caption.spacer";

}

ui_caption_t ui_caption =  {
    .view = {
        .type     = ui_view_span,
        .fm       = &ui_app.fm.regular,
        .init     = ui_caption_init,
        .hit_test = ui_caption_hit_test,
        .state.hidden = true
    },
    .icon   = ui_button(ut_glyph_nbsp, 0.0, null),
    .title  = ui_label(0, ""),
    .spacer = ui_view(spacer),
    .menu   = ui_button(ui_caption_glyph_menu, 0.0, null),
    .mode   = ui_button(ui_caption_glyph_mini, 0.0, ui_caption_mode),
    .mini   = ui_button(ui_caption_glyph_mini, 0.0, ui_caption_mini),
    .maxi   = ui_button(ui_caption_glyph_maxi, 0.0, ui_caption_maxi),
    .full   = ui_button(ui_caption_glyph_full, 0.0, ui_caption_full),
    .quit   = ui_button(ui_caption_glyph_quit, 0.0, ui_caption_quit),
};

#pragma pop_macro("ui_caption_glyph_rest")
#pragma pop_macro("ui_caption_glyph_menu")
#pragma pop_macro("ui_caption_glyph_dark")
#pragma pop_macro("ui_caption_glyph_light")
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
        default: ut_swear(false); break;
    }
    ut_assert(0 <= r && r <= 255);
    ut_assert(0 <= g && g <= 255);
    ut_assert(0 <= b && b <= 255);
    return ui_color_rgba((uint8_t)r, (uint8_t)g, (uint8_t)b, a);
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
    ut_assert(0.0f < multiplier && multiplier < 1.0f);
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
    return ui_color_rgba(intensity, intensity, intensity, ui_color_a(c));
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
        ui_color_rgba(255, 255, 255, ui_color_a(c)) :
        ui_color_rgba(  0,   0,   0, ui_color_a(c));
    return ui_color_interpolate(c, target, multiplier);
}

static ui_color_t ui_color_lighten(ui_color_t c, fp32_t multiplier) {
    const ui_color_t target = ui_color_rgba(255, 255, 255, ui_color_a(c));
    return ui_color_interpolate(c, target, multiplier);
}
static ui_color_t ui_color_darken(ui_color_t c, fp32_t multiplier) {
    const ui_color_t target = ui_color_rgba(0, 0, 0, ui_color_a(c));
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

static struct {
    const char* name;
    ui_color_t  dark;
    ui_color_t  light;
} ui_theme_colors[] = { // empirical
    { .name = "Undefiled"        ,.dark = ui_color_undefined, .light = ui_color_undefined },
    { .name = "ActiveTitle"      ,.dark = 0x001F1F1F, .light = 0x00D1B499 },
    { .name = "ButtonFace"       ,.dark = 0x00333333, .light = 0x00F0F0F0 },
    { .name = "ButtonText"       ,.dark = 0x00C8C8C8, .light = 0x00161616 },
//  { .name = "ButtonText"       ,.dark = 0x00F6F3EE, .light = 0x00000000 },
    { .name = "GrayText"         ,.dark = 0x00666666, .light = 0x006D6D6D },
    { .name = "Hilight"          ,.dark = 0x00626262, .light = 0x00D77800 },
    { .name = "HilightText"      ,.dark = 0x00000000, .light = 0x00FFFFFF },
    { .name = "HotTrackingColor" ,.dark = 0x00B16300, .light = 0x00FF0000 }, // automatic Win11 "accent" ABRG: 0xFFB16300
//  { .name = "HotTrackingColor" ,.dark = 0x00B77878, .light = 0x00CC6600 },
    { .name = "InactiveTitle"    ,.dark = 0x002B2B2B, .light = 0x00DBCDBF },
    { .name = "InactiveTitleText",.dark = 0x00969696, .light = 0x00000000 },
    { .name = "MenuHilight"      ,.dark = 0x00002642, .light = 0x00FF9933 },
    { .name = "TitleText"        ,.dark = 0x00FFFFFF, .light = 0x00000000 },
//  { .name = "Window"           ,.dark = 0x00000000, .light = 0x00FFFFFF }, // too contrast
//  { .name = "Window"           ,.dark = 0x00121212, .light = 0x00E0E0E0 },
    { .name = "Window"           ,.dark = 0x002E2E2E, .light = 0x00E0E0E0 },
    { .name = "WindowText"       ,.dark = 0x00FFFFFF, .light = 0x00000000 },
};

// TODO: add
// Accent Color BGR: B16300  RGB: 0063B1 light blue
// [HKEY_CURRENT_USER\Software\Microsoft\Windows\DWM]
// "AccentColor"=dword:ffb16300
// Windows used as accent almost on everything
// see here: https://github.com/leok7v/ui/discussions/5


static ui_color_t ui_colors_get_color(int32_t color_id) {
    // SysGetColor() does not work on Win10
    ut_swear(0 < color_id && color_id < ut_countof(ui_theme_colors));
    return ui_theme.is_app_dark() ?
           ui_theme_colors[color_id].dark :
           ui_theme_colors[color_id].light;
}

ui_colors_if ui_colors = {
    .get_color                = ui_colors_get_color,
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
    .text             = ui_color_rgb(240, 231, 220),
    .white            = ui_color_rgb(255, 255, 255),
    .black            = ui_color_rgb(0,     0,   0),
    .red              = ui_color_rgb(255,   0,   0),
    .green            = ui_color_rgb(0,   255,   0),
    .blue             = ui_color_rgb(0,   0,   255),
    .yellow           = ui_color_rgb(255, 255,   0),
    .cyan             = ui_color_rgb(0,   255, 255),
    .magenta          = ui_color_rgb(255,   0, 255),
    .gray             = ui_color_rgb(128, 128, 128),
    // tone down RGB colors:
    .tone_white       = ui_color_rgb(164, 164, 164),
    .tone_red         = ui_color_rgb(192,  64,  64),
    .tone_green       = ui_color_rgb(64,  192,  64),
    .tone_blue        = ui_color_rgb(64,   64, 192),
    .tone_yellow      = ui_color_rgb(192, 192,  64),
    .tone_cyan        = ui_color_rgb(64,  192, 192),
    .tone_magenta     = ui_color_rgb(192,  64, 192),
    // miscellaneous:
    .orange           = ui_color_rgb(255, 165,   0), // 0xFFA500
    .dark_green          = ui_color_rgb(  1,  50,  32), // 0x013220
    .pink             = ui_color_rgb(255, 192, 203), // 0xFFC0CB
    .ochre            = ui_color_rgb(204, 119,  34), // 0xCC7722
    .gold             = ui_color_rgb(255, 215,   0), // 0xFFD700
    .teal             = ui_color_rgb(  0, 128, 128), // 0x008080
    .wheat            = ui_color_rgb(245, 222, 179), // 0xF5DEB3
    .tan              = ui_color_rgb(210, 180, 140), // 0xD2B48C
    .brown            = ui_color_rgb(165,  42,  42), // 0xA52A2A
    .maroon           = ui_color_rgb(128,   0,   0), // 0x800000
    .barbie_pink      = ui_color_rgb(224,  33, 138), // 0xE0218A
    .steel_pink       = ui_color_rgb(204,  51, 204), // 0xCC33CC
    .salmon_pink      = ui_color_rgb(255, 145, 164), // 0xFF91A4
    .gainsboro        = ui_color_rgb(220, 220, 220), // 0xDCDCDC
    .light_gray       = ui_color_rgb(211, 211, 211), // 0xD3D3D3
    .silver           = ui_color_rgb(192, 192, 192), // 0xC0C0C0
    .dark_gray        = ui_color_rgb(169, 169, 169), // 0xA9A9A9
    .dim_gray         = ui_color_rgb(105, 105, 105), // 0x696969
    .light_slate_gray = ui_color_rgb(119, 136, 153), // 0x778899
    .slate_gray       = ui_color_rgb(112, 128, 144), // 0x708090
    /* Main Panel Backgrounds */
    .ennui_black                = ui_color_rgb( 18,  18,  18), // 0x1212121
    .charcoal                   = ui_color_rgb( 54,  69,  79), // 0x36454F
    .onyx                       = ui_color_rgb( 53,  56,  57), // 0x353839
    .gunmetal                   = ui_color_rgb( 42,  52,  57), // 0x2A3439
    .jet_black                  = ui_color_rgb( 52,  52,  52), // 0x343434
    .outer_space                = ui_color_rgb( 65,  74,  76), // 0x414A4C
    .eerie_black                = ui_color_rgb( 27,  27,  27), // 0x1B1B1B
    .oil                        = ui_color_rgb( 59,  60,  54), // 0x3B3C36
    .black_coral                = ui_color_rgb( 84,  98, 111), // 0x54626F
    .obsidian                   = ui_color_rgb( 58,  50,  45), // 0x3A322D
    /* Secondary Panels or Sidebars */
    .raisin_black               = ui_color_rgb( 39,  38,  53), // 0x272635
    .dark_charcoal              = ui_color_rgb( 48,  48,  48), // 0x303030
    .dark_jungle_green          = ui_color_rgb( 26,  36,  33), // 0x1A2421
    .pine_tree                  = ui_color_rgb( 42,  47,  35), // 0x2A2F23
    .rich_black                 = ui_color_rgb(  0,  64,  64), // 0x004040
    .eclipse                    = ui_color_rgb( 63,  57,  57), // 0x3F3939
    .cafe_noir                  = ui_color_rgb( 75,  54,  33), // 0x4B3621

    /* Flat Buttons */
    .prussian_blue              = ui_color_rgb(  0,  49,  83), // 0x003153
    .midnight_green             = ui_color_rgb(  0,  73,  83), // 0x004953
    .charleston_green           = ui_color_rgb( 35,  43,  43), // 0x232B2B
    .rich_black_fogra           = ui_color_rgb( 10,  15,  13), // 0x0A0F0D
    .dark_liver                 = ui_color_rgb( 83,  75,  79), // 0x534B4F
    .dark_slate_gray            = ui_color_rgb( 47,  79,  79), // 0x2F4F4F
    .black_olive                = ui_color_rgb( 59,  60,  54), // 0x3B3C36
    .cadet                      = ui_color_rgb( 83, 104, 114), // 0x536872

    /* Button highlights (hover) */
    .dark_sienna                = ui_color_rgb( 60,  20,  20), // 0x3C1414
    .bistre_brown               = ui_color_rgb(150, 113,  23), // 0x967117
    .dark_puce                  = ui_color_rgb( 79,  58,  60), // 0x4F3A3C
    .wenge                      = ui_color_rgb(100,  84,  82), // 0x645452

    /* Raised button effects */
    .dark_scarlet               = ui_color_rgb( 86,   3,  25), // 0x560319
    .burnt_umber                = ui_color_rgb(138,  51,  36), // 0x8A3324
    .caput_mortuum              = ui_color_rgb( 89,  39,  32), // 0x592720
    .barn_red                   = ui_color_rgb(124,  10,   2), // 0x7C0A02

    /* Text and Icons */
    .platinum                   = ui_color_rgb(229, 228, 226), // 0xE5E4E2
    .anti_flash_white           = ui_color_rgb(242, 243, 244), // 0xF2F3F4
    .silver_sand                = ui_color_rgb(191, 193, 194), // 0xBFC1C2
    .quick_silver               = ui_color_rgb(166, 166, 166), // 0xA6A6A6

    /* Links and Selections */
    .dark_powder_blue           = ui_color_rgb(  0,  51, 153), // 0x003399
    .sapphire_blue              = ui_color_rgb( 15,  82, 186), // 0x0F52BA
    .international_klein_blue   = ui_color_rgb(  0,  47, 167), // 0x002FA7
    .zaffre                     = ui_color_rgb(  0,  20, 168), // 0x0014A8

    /* Additional Colors */
    .fish_belly                 = ui_color_rgb(232, 241, 212), // 0xE8F1D4
    .rusty_red                  = ui_color_rgb(218,  44,  67), // 0xDA2C43
    .falu_red                   = ui_color_rgb(128,  24,  24), // 0x801818
    .cordovan                   = ui_color_rgb(137,  63,  69), // 0x893F45
    .dark_raspberry             = ui_color_rgb(135,  38,  87), // 0x872657
    .deep_magenta               = ui_color_rgb(204,   0, 204), // 0xCC00CC
    .byzantium                  = ui_color_rgb(112,  41,  99), // 0x702963
    .amethyst                   = ui_color_rgb(153, 102, 204), // 0x9966CC
    .wisteria                   = ui_color_rgb(201, 160, 220), // 0xC9A0DC
    .lavender_purple            = ui_color_rgb(150, 123, 182), // 0x967BB6
    .opera_mauve                = ui_color_rgb(183, 132, 167), // 0xB784A7
    .mauve_taupe                = ui_color_rgb(145,  95, 109), // 0x915F6D
    .rich_lavender              = ui_color_rgb(167, 107, 207), // 0xA76BCF
    .pansy_purple               = ui_color_rgb(120,  24,  74), // 0x78184A
    .violet_eggplant            = ui_color_rgb(153,  17, 153), // 0x991199
    .jazzberry_jam              = ui_color_rgb(165,  11,  94), // 0xA50B5E
    .dark_orchid                = ui_color_rgb(153,  50, 204), // 0x9932CC
    .electric_purple            = ui_color_rgb(191,   0, 255), // 0xBF00FF
    .sky_magenta                = ui_color_rgb(207, 113, 175), // 0xCF71AF
    .brilliant_rose             = ui_color_rgb(230, 103, 206), // 0xE667CE
    .fuchsia_purple             = ui_color_rgb(204,  57, 123), // 0xCC397B
    .french_raspberry           = ui_color_rgb(199,  44,  72), // 0xC72C48
    .wild_watermelon            = ui_color_rgb(252, 108, 133), // 0xFC6C85
    .neon_carrot                = ui_color_rgb(255, 163,  67), // 0xFFA343
    .burnt_orange               = ui_color_rgb(204,  85,   0), // 0xCC5500
    .carrot_orange              = ui_color_rgb(237, 145,  33), // 0xED9121
    .tiger_orange               = ui_color_rgb(253, 106,   2), // 0xFD6A02
    .giant_onion                = ui_color_rgb(176, 181, 137), // 0xB0B589
    .rust                       = ui_color_rgb(183,  65,  14), // 0xB7410E
    .copper_red                 = ui_color_rgb(203, 109,  81), // 0xCB6D51
    .dark_tangerine             = ui_color_rgb(255, 168,  18), // 0xFFA812
    .bright_marigold            = ui_color_rgb(252, 192,   6), // 0xFCC006
    .bone                       = ui_color_rgb(227, 218, 201), // 0xE3DAC9

    /* Earthy Tones */
    .sienna                     = ui_color_rgb(160,  82,  45), // 0xA0522D
    .sandy_brown                = ui_color_rgb(244, 164,  96), // 0xF4A460
    .golden_brown               = ui_color_rgb(153, 101,  21), // 0x996515
    .camel                      = ui_color_rgb(193, 154, 107), // 0xC19A6B
    .burnt_sienna               = ui_color_rgb(238, 124,  88), // 0xEE7C58
    .khaki                      = ui_color_rgb(195, 176, 145), // 0xC3B091
    .dark_khaki                 = ui_color_rgb(189, 183, 107), // 0xBDB76B

    /* Greens */
    .fern_green                 = ui_color_rgb( 79, 121,  66), // 0x4F7942
    .moss_green                 = ui_color_rgb(138, 154,  91), // 0x8A9A5B
    .myrtle_green               = ui_color_rgb( 49, 120, 115), // 0x317873
    .pine_green                 = ui_color_rgb(  1, 121, 111), // 0x01796F
    .jungle_green               = ui_color_rgb( 41, 171, 135), // 0x29AB87
    .sacramento_green           = ui_color_rgb(  4,  57,  39), // 0x043927

    /* Blues */
    .yale_blue                  = ui_color_rgb( 15,  77, 146), // 0x0F4D92
    .cobalt_blue                = ui_color_rgb(  0,  71, 171), // 0x0047AB
    .persian_blue               = ui_color_rgb( 28,  57, 187), // 0x1C39BB
    .royal_blue                 = ui_color_rgb( 65, 105, 225), // 0x4169E1
    .iceberg                    = ui_color_rgb(113, 166, 210), // 0x71A6D2
    .blue_yonder                = ui_color_rgb( 80, 114, 167), // 0x5072A7

    /* Miscellaneous */
    .cocoa_brown                = ui_color_rgb(210, 105,  30), // 0xD2691E
    .cinnamon_satin             = ui_color_rgb(205,  96, 126), // 0xCD607E
    .fallow                     = ui_color_rgb(193, 154, 107), // 0xC19A6B
    .cafe_au_lait               = ui_color_rgb(166, 123,  91), // 0xA67B5B
    .liver                      = ui_color_rgb(103,  76,  71), // 0x674C47
    .shadow                     = ui_color_rgb(138, 121,  93), // 0x8A795D
    .cool_grey                  = ui_color_rgb(140, 146, 172), // 0x8C92AC
    .payne_grey                 = ui_color_rgb( 83, 104, 120), // 0x536878

    /* Lighter Tones for Contrast */
    .timberwolf                 = ui_color_rgb(219, 215, 210), // 0xDBD7D2
    .silver_chalice             = ui_color_rgb(172, 172, 172), // 0xACACAC
    .roman_silver               = ui_color_rgb(131, 137, 150), // 0x838996

    /* Dark Mode Specific Highlights */
    .electric_lavender          = ui_color_rgb(244, 191, 255), // 0xF4BFFF
    .magenta_haze               = ui_color_rgb(159,  69, 118), // 0x9F4576
    .cyber_grape                = ui_color_rgb( 88,  66, 124), // 0x58427C
    .purple_navy                = ui_color_rgb( 78,  81, 128), // 0x4E5180
    .liberty                    = ui_color_rgb( 84,  90, 167), // 0x545AA7
    .purple_mountain_majesty    = ui_color_rgb(150, 120, 182), // 0x9678B6
    .ceil                       = ui_color_rgb(146, 161, 207), // 0x92A1CF
    .moonstone_blue             = ui_color_rgb(115, 169, 194), // 0x73A9C2
    .independence               = ui_color_rgb( 76,  81, 109)  // 0x4C516D
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
    if (ui_containers_debug) {  ut_println(__VA_ARGS__); }  \
} while (0)

static int32_t ui_layout_nesting;

#define ui_layout_enter(v) do {                                  \
    ui_ltrb_t i_ = ui_view.margins(v, &v->insets);                  \
    ui_ltrb_t p_ = ui_view.margins(v, &v->padding);                 \
    debugln("%*c>%s %d,%d %dx%d p: %d %d %d %d  i: %d %d %d %d", \
            ui_layout_nesting, 0x20,                             \
            ui_view.string(v), v->x, v->y, v->w, v->h,           \
            p_.left, p_.top, p_.right, p_.bottom,                \
            i_.left, i_.top, i_.right, i_.bottom);               \
    ui_layout_nesting += 4;                                      \
} while (0)

#define ui_layout_exit(v) do {                                   \
    ui_layout_nesting -= 4;                                      \
    debugln("%*c<%s %d,%d %dx%d",                                \
            ui_layout_nesting, 0x20,                             \
            ui_view.string(v), v->x, v->y, v->w, v->h);          \
} while (0)

#define ui_layout_clild(v) do {                                  \
    debugln("%*c %s %d,%d %dx%d", ui_layout_nesting, 0x20,       \
            ui_view.string(c), c->x, c->y, c->w, c->h);          \
} while (0)

static const char* ui_stack_finite_int(int32_t v, char* text, int32_t count) {
    ut_swear(v >= 0);
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
        ui_view_debug_id(v),                                                  \
        &v->type, v->x, v->y, v->w, v->h,                                     \
        ui_stack_finite_int(v->max_w, maxw, ut_countof(maxw)),               \
        ui_stack_finite_int(v->max_h, maxh, ut_countof(maxh)),               \
        v->padding.left, v->padding.top, v->padding.right, v->padding.bottom, \
        v->insets.left, v->insets.top, v->insets.right, v->insets.bottom,     \
        v->align);                                                            \
} while (0)

static void ui_span_measure(ui_view_t* p) {
    ui_layout_enter(p);
    ut_swear(p->type == ui_view_span, "type %4.4s 0x%08X", &p->type, p->type);
    ui_ltrb_t insets;
    ui_view.inbox(p, null, &insets);
    int32_t w = insets.left;
    int32_t h = 0;
    int32_t max_w = w;
    ui_view_for_each_begin(p, c) {
        ut_swear(c->max_w == 0 || c->max_w >= c->w,
              "max_w: %d w: %d", c->max_w, c->w);
        if (ui_view.is_hidden(c)) {
            // nothing
        } else if (c->type == ui_view_spacer) {
            c->padding = (ui_margins_t){ 0, 0, 0, 0 };
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
                ut_swear(c->max_w >= c->w, "c->max_w %d < c->w %d ",
                      c->max_w, c->w);
                max_w += c->max_w;
            } else if (max_w < ui.infinity) {
                ut_swear(0 <= max_w + cbx.w &&
                      (int64_t)max_w + (int64_t)cbx.w < (int64_t)ui.infinity,
                      "max_w:%d + cbx.w:%d = %d", max_w, cbx.w, max_w + cbx.w);
                max_w += cbx.w;
            }
            w += cbx.w;
        }
        ui_layout_clild(c);
    } ui_view_for_each_end(p, c);
    if (0 < max_w && max_w < ui.infinity) {
        ut_swear(0 <= max_w + insets.right &&
              (int64_t)max_w + (int64_t)insets.right < (int64_t)ui.infinity,
             "max_w:%d + right:%d = %d", max_w, insets.right, max_w + insets.right);
        max_w += insets.right;
    }
    ut_swear(max_w == 0 || max_w >= w, "max_w: %d w: %d", max_w, w);
    if (ui_view.is_hidden(p)) {
        p->w = 0;
        p->h = 0;
    } else {
        p->w = w + insets.right;
        p->h = insets.top + h + insets.bottom;
        ut_swear(p->max_w == 0 || p->max_w >= p->w,
              "max_w: %d is less than actual width: %d", p->max_w, p->w);
    }
    ui_layout_exit(p);
}

// after measure of the subtree is concluded the parent ui_span
// may adjust span_w wider number depending on it's own width
// and ui_span.max_w agreement

static int32_t ui_span_place_child(ui_view_t* c, ui_rect_t pbx, int32_t x) {
    ui_ltrb_t padding = ui_view.margins(c, &c->padding);
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
        ut_assert(c->align == ui.align.top);
        c->y = min_y;
    } else if ((c->align & ui.align.bottom) != 0) {
        ut_assert(c->align == ui.align.bottom);
        c->y = ut_max(min_y, pbx.y + pbx.h - c->h - padding.bottom);
    } else { // effective height (c->h might have been changed)
        ut_assert(c->align == ui.align.center);
        const int32_t ch = padding.top + c->h + padding.bottom;
        c->y = ut_max(min_y, pbx.y + (pbx.h - ch) / 2 + padding.top);
    }
    c->x = x + padding.left;
    return c->x + c->w + padding.right;
}

static void ui_span_layout(ui_view_t* p) {
    ui_layout_enter(p);
    ut_swear(p->type == ui_view_span, "type %4.4s 0x%08X", &p->type, p->type);
    ui_rect_t pbx; // parent "in" box (sans insets)
    ui_ltrb_t insets;
    ui_view.inbox(p, &pbx, &insets);
    int32_t spacers = 0; // Number of spacers
    int32_t max_w_count = 0;
    int32_t x = p->x + insets.left;
    ui_view_for_each_begin(p, c) {
        if (!ui_view.is_hidden(c)) {
            if (c->type == ui_view_spacer) {
                c->x = x;
                c->y = pbx.y;
                c->h = pbx.h;
                c->w = 0;
                spacers++;
            } else {
                x = ui_span_place_child(c, pbx, x);
                ut_swear(c->max_w == 0 || c->max_w >= c->w,
                      "max_w:%d < w:%d", c->max_w, c->w);
                if (c->max_w > 0) {
                    max_w_count++;
                }
            }
            ui_layout_clild(c);
        }
    } ui_view_for_each_end(p, c);
    int32_t xw = ut_max(0, pbx.x + pbx.w - x); // excess width
    int32_t max_w_sum = 0;
    if (xw > 0 && max_w_count > 0) {
        ui_view_for_each_begin(p, c) {
            if (!ui_view.is_hidden(c) && c->type != ui_view_spacer &&
                 c->max_w > 0) {
                max_w_sum += ut_min(c->max_w, xw);
                ui_layout_clild(c);
            }
        } ui_view_for_each_end(p, c);
    }
    if (xw > 0 && max_w_count > 0) {
        x = p->x + insets.left;
        int32_t k = 0;
        ui_view_for_each_begin(p, c) {
            if (!ui_view.is_hidden(c)) {
                ui_rect_t cbx; // child "out" box expanded by padding
                ui_ltrb_t padding;
                ui_view.outbox(c, &cbx, &padding);
                if (c->type == ui_view_spacer) {
                    ut_swear(padding.left == 0 && padding.right == 0);
                } else if (c->max_w > 0) {
                    const int32_t max_w = ut_min(c->max_w, xw);
                    int64_t proportional = (xw * (int64_t)max_w) / max_w_sum;
                    ut_assert(proportional <= (int64_t)INT32_MAX);
                    int32_t cw = (int32_t)proportional;
                    c->w = ut_min(c->max_w, c->w + cw);
                    k++;
                }
                // TODO: take into account .align of a child and adjust x
                //       depending on ui.align.left/right/center
                //       distributing excess width on the left and right of a child
                c->x = padding.left + x;
                x = c->x + padding.left + c->w + padding.right;
                ui_layout_clild(c);
            }
        } ui_view_for_each_end(p, c);
        ut_swear(k == max_w_count);
    }
    // excess width after max_w of non-spacers taken into account
    xw = ut_max(0, pbx.x + pbx.w - x);
    if (xw > 0 && spacers > 0) {
        // evenly distribute excess among spacers
        int32_t partial = xw / spacers;
        x = p->x + insets.left;
        ui_view_for_each_begin(p, c) {
            if (!ui_view.is_hidden(c)) {
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
                ui_layout_clild(c);
            }
        } ui_view_for_each_end(p, c);
    }
    ui_layout_exit(p);
}

static void ui_list_measure(ui_view_t* p) {
    ui_layout_enter(p);
    ut_swear(p->type == ui_view_list, "type %4.4s 0x%08X", &p->type, p->type);
    ui_rect_t pbx; // parent "in" box (sans insets)
    ui_ltrb_t insets;
    ui_view.inbox(p, &pbx, &insets);
    int32_t max_h = insets.top;
    int32_t h = insets.top;
    int32_t w = 0;
    ui_view_for_each_begin(p, c) {
        ut_swear(c->max_h == 0 || c->max_h >= c->h, "max_h: %d h: %d",
              c->max_h, c->h);
        if (!ui_view.is_hidden(c)) {
            if (c->type == ui_view_spacer) {
                c->padding = (ui_margins_t){ 0, 0, 0, 0 };
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
                    ut_swear(c->max_h >= c->h, "c->max_h:%d < c->h: %d",
                          c->max_h, c->h);
                    max_h += c->max_h;
                } else if (max_h < ui.infinity) {
                    ut_swear(0 <= max_h + cbx.h &&
                          (int64_t)max_h + (int64_t)cbx.h < (int64_t)ui.infinity,
                          "max_h:%d + ch:%d = %d", max_h, cbx.h, max_h + cbx.h);
                    max_h += cbx.h;
                }
                h += cbx.h;
            }
            ui_layout_clild(c);
        }
    } ui_view_for_each_end(p, c);
    if (max_h < ui.infinity) {
        ut_swear(0 <= max_h + insets.bottom &&
              (int64_t)max_h + (int64_t)insets.bottom < (int64_t)ui.infinity,
             "max_h:%d + bottom:%d = %d",
              max_h, insets.bottom, max_h + insets.bottom);
        max_h += insets.bottom;
    }
    if (ui_view.is_hidden(p)) {
        p->w = 0;
        p->h = 0;
    } else if (p == ui_app.root) {
        // ui_app.root is special occupying whole window client rectangle
        // sans borders and caption thus it should not be re-measured
    } else {
        p->h = h + insets.bottom;
        p->w = insets.left + w + insets.right;
    }
    ui_layout_exit(p);
}

static int32_t ui_list_place_child(ui_view_t* c, ui_rect_t pbx, int32_t y) {
    ui_ltrb_t padding = ui_view.margins(c, &c->padding);
    // setting child`s max_w to infinity means that child`s height is
    // *always* fill vertical view size of the parent
    // childs.w can exceed parent.w (horizontal overflow) - not encouraged but allowed
    if (c->max_w == ui.infinity) {
        c->w = ut_max(c->w, pbx.w - padding.left - padding.right);
    }
    int32_t min_x = pbx.x + padding.left;
    if ((c->align & ui.align.left) != 0) {
        ut_assert(c->align == ui.align.left);
        c->x = min_x;
    } else if ((c->align & ui.align.right) != 0) {
        ut_assert(c->align == ui.align.right);
        c->x = ut_max(min_x, pbx.x + pbx.w - c->w - padding.right);
    } else {
        ut_assert(c->align == ui.align.center);
        const int32_t cw = padding.left + c->w + padding.right;
        c->x = ut_max(min_x, pbx.x + (pbx.w - cw) / 2 + padding.left);
    }
    c->y = y + padding.top;
    return c->y + c->h + padding.bottom;
}

static void ui_list_layout(ui_view_t* p) {
    ui_layout_enter(p);
    ut_swear(p->type == ui_view_list, "type %4.4s 0x%08X", &p->type, p->type);
    ui_rect_t pbx; // parent "in" box (sans insets)
    ui_ltrb_t insets;
    ui_view.inbox(p, &pbx, &insets);
    int32_t spacers = 0; // Number of spacers
    int32_t max_h_sum = 0;
    int32_t max_h_count = 0;
    int32_t y = pbx.y;
    ui_view_for_each_begin(p, c) {
        if (ui_view.is_hidden(c)) {
            // nothing
        } else if (c->type == ui_view_spacer) {
            c->x = pbx.x;
            c->y = y;
            c->w = pbx.w;
            c->h = 0;
            spacers++;
        } else {
            y = ui_list_place_child(c, pbx, y);
            ut_swear(c->max_h == 0 || c->max_h >= c->h,
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
            if (!ui_view.is_hidden(c) && c->type != ui_view_spacer &&
                 c->max_h > 0) {
                max_h_sum += ut_min(c->max_h, xh);
            }
        } ui_view_for_each_end(p, c);
    }
    if (xh > 0 && max_h_count > 0) {
        y = pbx.y;
        int32_t k = 0;
        ui_view_for_each_begin(p, c) {
            if (!ui_view.is_hidden(c)) {
                ui_rect_t cbx; // child "out" box expanded by padding
                ui_ltrb_t padding;
                ui_view.outbox(c, &cbx, &padding);
                if (c->type != ui_view_spacer && c->max_h > 0) {
                    const int32_t max_h = ut_min(c->max_h, xh);
                    int64_t proportional = (xh * (int64_t)max_h) / max_h_sum;
                    ut_assert(proportional <= (int64_t)INT32_MAX);
                    int32_t ch = (int32_t)proportional;
                    c->h = ut_min(c->max_h, c->h + ch);
                    k++;
                }
                int32_t ch = padding.top + c->h + padding.bottom;
                c->y = y + padding.top;
                y += ch;
                ui_layout_clild(c);
            }
        } ui_view_for_each_end(p, c);
        ut_swear(k == max_h_count);
    }
    // excess height after max_h of non-spacers taken into account
    xh = ut_max(0, pbx.y + pbx.h - y); // excess height
    if (xh > 0 && spacers > 0) {
        // evenly distribute excess among spacers
        int32_t partial = xh / spacers;
        y = pbx.y;
        ui_view_for_each_begin(p, c) {
            if (!ui_view.is_hidden(c)) {
                ui_rect_t cbx; // child "out" box expanded by padding
                ui_ltrb_t padding;
                ui_view.outbox(c, &cbx, &padding);
                if (c->type == ui_view_spacer) {
                    c->x = pbx.x;
                    c->w = pbx.x + pbx.w - pbx.x;
                    c->h = partial; // TODO: last?
                    spacers--;
                }
                int32_t ch = padding.top + c->h + padding.bottom;
                c->y = y + padding.top;
                y += ch;
                ui_layout_clild(c);
            }
        } ui_view_for_each_end(p, c);
    }
    ui_layout_exit(p);
}

static void ui_stack_child_3x3(ui_view_t* c, int32_t *row, int32_t *col) {
    *row = 0; *col = 0; // makes code analysis happier
    if (c->align == (ui.align.left|ui.align.top)) {
        *row = 0; *col = 0;
    } else if (c->align == ui.align.top) {
        *row = 0; *col = 1;
    } else if (c->align == (ui.align.right|ui.align.top)) {
        *row = 0; *col = 2;
    } else if (c->align == ui.align.left) {
        *row = 1; *col = 0;
    } else if (c->align == ui.align.center) {
        *row = 1; *col = 1;
    } else if (c->align == ui.align.right) {
        *row = 1; *col = 2;
    } else if (c->align == (ui.align.left|ui.align.bottom)) {
        *row = 2; *col = 0;
    } else if (c->align == ui.align.bottom) {
        *row = 2; *col = 1;
    } else if (c->align == (ui.align.right|ui.align.bottom)) {
        *row = 2; *col = 2;
    } else {
        ut_swear(false, "invalid child align: 0x%02X", c->align);
    }
}

static void ui_stack_measure(ui_view_t* p) {
    ui_layout_enter(p);
    ut_swear(p->type == ui_view_stack, "type %4.4s 0x%08X", &p->type, p->type);
    ui_rect_t pbx; // parent "in" box (sans insets)
    ui_ltrb_t insets;
    ui_view.inbox(p, &pbx, &insets);
    ui_wh_t sides[3][3] = { {0, 0} };
    ui_view_for_each_begin(p, c) {
        if (!ui_view.is_hidden(c)) {
            ui_rect_t cbx; // child "out" box expanded by padding
            ui_ltrb_t padding;
            ui_view.outbox(c, &cbx, &padding);
            int32_t row = 0;
            int32_t col = 0;
            ui_stack_child_3x3(c, &row, &col);
            sides[row][col].w = ut_max(sides[row][col].w, cbx.w);
            sides[row][col].h = ut_max(sides[row][col].h, cbx.h);
            ui_layout_clild(c);
        }
    } ui_view_for_each_end(p, c);
    if (ui_containers_debug) {
        for (int32_t r = 0; r < ut_countof(sides); r++) {
            char text[1024];
            text[0] = 0;
            for (int32_t c = 0; c < ut_countof(sides[r]); c++) {
                char line[128];
                ut_str_printf(line, " %4dx%-4d", sides[r][c].w, sides[r][c].h);
                strcat(text, line);
            }
            debugln("%*c sides[%d] %s", ui_layout_nesting, 0x20, r, text);
        }
    }
    ui_wh_t wh = {0, 0};
    for (int32_t r = 0; r < 3; r++) {
        int32_t sum_w = 0;
        for (int32_t c = 0; c < 3; c++) {
            sum_w += sides[r][c].w;
        }
        wh.w = ut_max(wh.w, sum_w);
    }
    for (int32_t c = 0; c < 3; c++) {
        int32_t sum_h = 0;
        for (int32_t r = 0; r < 3; r++) {
            sum_h += sides[r][c].h;
        }
        wh.h = ut_max(wh.h, sum_h);
    }
    debugln("%*c wh %dx%d", ui_layout_nesting, 0x20, wh.w, wh.h);
    p->w = insets.left + wh.w + insets.right;
    p->h = insets.top  + wh.h + insets.bottom;
    ui_layout_exit(p);
}

static void ui_stack_layout(ui_view_t* p) {
    ui_layout_enter(p);
    ut_swear(p->type == ui_view_stack, "type %4.4s 0x%08X", &p->type, p->type);
    ui_rect_t pbx; // parent "in" box (sans insets)
    ui_ltrb_t insets;
    ui_view.inbox(p, &pbx, &insets);
    ui_view_for_each_begin(p, c) {
        if (c->type != ui_view_spacer && !ui_view.is_hidden(c)) {
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
            ut_swear((c->align & (ui.align.left|ui.align.right)) !=
                               (ui.align.left|ui.align.right),
                   "align: left|right 0x%02X", c->align);
            ut_swear((c->align & (ui.align.top|ui.align.bottom)) !=
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
                c->y = ut_max(min_y, pbx.y + pbx.h - c->h - padding.bottom);
            } else {
                c->y = ut_max(min_y, min_y + (pbx.h - (padding.top + c->h + padding.bottom)) / 2);
            }
            ui_layout_clild(c);
        }
    } ui_view_for_each_end(p, c);
    ui_layout_exit(p);
}

static void ui_container_paint(ui_view_t* v) {
    if (!ui_color_is_undefined(v->background) &&
        !ui_color_is_transparent(v->background)) {
        ui_gdi.fill(v->x, v->y, v->w, v->h, v->background);
    } else {
//      ut_println("%s undefined", ui_view_debug_id(v));
    }
}

static void ui_view_container_init(ui_view_t* v) {
    v->background = ui_colors.transparent;
    v->insets  = (ui_margins_t){
       .left  = 0.25, .top    = 0.125,
        .right = 0.25, .bottom = 0.125
//      .left  = 0.25, .top    = 0.0625,  // TODO: why?
//      .right = 0.25, .bottom = 0.1875
    };
}

void ui_view_init_span(ui_view_t* v) {
    ut_swear(v->type == ui_view_span, "type %4.4s 0x%08X", &v->type, v->type);
    ui_view_container_init(v);
    if (v->measure == null) { v->measure = ui_span_measure; }
    if (v->layout  == null) { v->layout  = ui_span_layout; }
    if (v->paint   == null) { v->paint   = ui_container_paint; }
    if (ui_view.string(v)[0] == 0) { ui_view.set_text(v, "ui_span"); }
    if (v->debug.id == null) { v->debug.id = "#ui_span"; }
}

void ui_view_init_list(ui_view_t* v) {
    ut_swear(v->type == ui_view_list, "type %4.4s 0x%08X", &v->type, v->type);
    ui_view_container_init(v);
    if (v->measure == null) { v->measure = ui_list_measure; }
    if (v->layout  == null) { v->layout  = ui_list_layout; }
    if (v->paint   == null) { v->paint   = ui_container_paint; }
    if (ui_view.string(v)[0] == 0) { ui_view.set_text(v, "ui_list"); }
    if (v->debug.id == null) { v->debug.id = "#ui_list"; }
}

void ui_view_init_spacer(ui_view_t* v) {
    ut_swear(v->type == ui_view_spacer, "type %4.4s 0x%08X", &v->type, v->type);
    v->w = 0;
    v->h = 0;
    v->max_w = ui.infinity;
    v->max_h = ui.infinity;
    if (ui_view.string(v)[0] == 0) { ui_view.set_text(v, "ui_spacer"); }
    if (v->debug.id == null) { v->debug.id = "#ui_spacer"; }

}

void ui_view_init_stack(ui_view_t* v) {
    ui_view_container_init(v);
    if (v->measure == null) { v->measure = ui_stack_measure; }
    if (v->layout  == null) { v->layout  = ui_stack_layout; }
    if (v->paint   == null) { v->paint   = ui_container_paint; }
    if (ui_view.string(v)[0] == 0) { ui_view.set_text(v, "ui_stack"); }
    if (v->debug.id == null) { v->debug.id = "#ui_stack"; }
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
#define UI_WM_DTAP     (WM_APP + 0x7FFB) // double tap (aka click)
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

static ui_rect_t ui_combine_rect(const ui_rect_t* r0, const ui_rect_t* r1) {
    return (ui_rect_t) {
        .x = ut_min(r0->x, r1->x),
        .y = ut_min(r0->y, r1->y),
        .w = ut_max(r0->x + r0->w, r1->x + r1->w) - ut_min(r0->x, r1->x),
        .h = ut_max(r0->y + r0->h, r1->y + r1->h) - ut_min(r0->y, r1->y)
    };
}

ui_if ui = {
    .point_in_rect  = ui_point_in_rect,
    .intersect_rect = ui_intersect_rect,
    .combine_rect   = ui_combine_rect,
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
        .animate               = UI_WM_ANIMATE,
        .opening               = UI_WM_OPENING,
        .closing               = UI_WM_CLOSING
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
    .beep = {
        .ok         = 0,
        .info       = 1,
        .question   = 2,
        .warning    = 3,
        .error      = 4
    }
};

// https://docs.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-showwindow
// ______________________________ ui_edit_doc.c _______________________________

/* Copyright (c) Dmitry "Leo" Kuznetsov 2021-24 see LICENSE for details */
#include "ut/ut.h"

#undef UI_EDIT_STR_TEST
#undef UI_EDIT_DOC_TEST
#undef UI_STR_TEST_REPLACE_ALL_PERMUTATIONS
#undef UI_EDIT_DOC_TEST_PARAGRAPHS

#if 0 // flip to 1 to run tests

#define UI_EDIT_STR_TEST
#define UI_EDIT_DOC_TEST

#if 0 // flip to 1 to run exhausting lengthy tests
#define UI_STR_TEST_REPLACE_ALL_PERMUTATIONS
#define UI_EDIT_DOC_TEST_PARAGRAPHS
#endif

#endif

#pragma push_macro("ui_edit_check_zeros")
#pragma push_macro("ui_edit_check_pg_inside_text")
#pragma push_macro("ui_edit_check_range_inside_text")
#pragma push_macro("ui_edit_pg_dump")
#pragma push_macro("ui_edit_range_dump")
#pragma push_macro("ui_edit_text_dump")
#pragma push_macro("ui_edit_doc_dump")

#define ui_edit_pg_dump(pg)                              \
    ut_debug.println(__FILE__, __LINE__, __func__,       \
                    "pn:%d gp:%d", (pg)->pn, (pg)->gp)

#define ui_edit_range_dump(r)                            \
    ut_debug.println(__FILE__, __LINE__, __func__,       \
            "from {pn:%d gp:%d} to {pn:%d gp:%d}",       \
    (r)->from.pn, (r)->from.gp, (r)->to.pn, (r)->to.gp);

#define ui_edit_text_dump(t) do {                        \
    for (int32_t i_ = 0; i_ < (t)->np; i_++) {           \
        const ui_edit_str_t* p_ = &t->ps[i_];            \
        ut_debug.println(__FILE__, __LINE__, __func__,   \
            "ps[%d].%d: %.*s", i_, p_->b, p_->b, p_->u); \
    }                                                    \
} while (0)

// TODO: undo/redo stacks and listeners
#define ui_edit_doc_dump(d) do {                                \
    for (int32_t i_ = 0; i_ < (d)->text.np; i_++) {             \
        const ui_edit_str_t* p_ = &(d)->text.ps[i_];            \
        ut_debug.println(__FILE__, __LINE__, __func__,          \
            "ps[%d].b:%d.c:%d: %p %.*s", i_, p_->b, p_->c,      \
            p_, p_->b, p_->u);                                  \
    }                                                           \
} while (0)


#ifdef DEBUG

// ui_edit_check_zeros only works for packed structs:

#define ui_edit_check_zeros(a_, b_) do {                                    \
    for (int32_t i_ = 0; i_ < (b_); i_++) {                                 \
        ut_assert(((const uint8_t*)(a_))[i_] == 0x00);                         \
    }                                                                       \
} while (0)

#define ui_edit_check_pg_inside_text(t_, pg_)                               \
    ut_assert(0 <= (pg_)->pn && (pg_)->pn < (t_)->np &&                        \
           0 <= (pg_)->gp && (pg_)->gp <= (t_)->ps[(pg_)->pn].g)

#define ui_edit_check_range_inside_text(t_, r_) do {                        \
    ut_assert((r_)->from.pn <= (r_)->to.pn);                                   \
    ut_assert((r_)->from.pn <  (r_)->to.pn || (r_)->from.gp <= (r_)->to.gp);   \
    ui_edit_check_pg_inside_text(t_, (&(r_)->from));                        \
    ui_edit_check_pg_inside_text(t_, (&(r_)->to));                          \
} while (0)

#else

#define ui_edit_check_zeros(a, b)             do { } while (0)
#define ui_edit_check_pg_inside_text(t, pg)   do { } while (0)
#define ui_edit_check_range_inside_text(t, r) do { } while (0)

#endif

static ui_edit_range_t ui_edit_text_all_on_null(const ui_edit_text_t* t,
        const ui_edit_range_t* range) {
    ui_edit_range_t r;
    if (range != null) {
        r = *range;
    } else {
        ut_assert(t->np >= 1);
        r.from.pn = 0;
        r.from.gp = 0;
        r.to.pn = t->np - 1;
        r.to.gp = t->ps[r.to.pn].g;
    }
    return r;
}

static int ui_edit_range_compare(const ui_edit_pg_t pg1, const ui_edit_pg_t pg2) {
    int64_t d = (((int64_t)pg1.pn << 32) | pg1.gp) -
                (((int64_t)pg2.pn << 32) | pg2.gp);
    return d < 0 ? -1 : d > 0 ? 1 : 0;
}

static ui_edit_range_t ui_edit_range_order(const ui_edit_range_t range) {
    ui_edit_range_t r = range;
    uint64_t f = ((uint64_t)r.from.pn << 32) | r.from.gp;
    uint64_t t = ((uint64_t)r.to.pn   << 32) | r.to.gp;
    if (ui_edit_range.compare(r.from, r.to) > 0) {
        uint64_t swap = t; t = f; f = swap;
        r.from.pn = (int32_t)(f >> 32);
        r.from.gp = (int32_t)(f);
        r.to.pn   = (int32_t)(t >> 32);
        r.to.gp   = (int32_t)(t);
    }
    return r;
}

static ui_edit_range_t ui_edit_text_ordered(const ui_edit_text_t* t,
        const ui_edit_range_t* r) {
    return ui_edit_range.order(ui_edit_text.all_on_null(t, r));
}

static bool ui_edit_range_is_valid(const ui_edit_range_t r) {
    if (0 <= r.from.pn && 0 <= r.to.pn &&
        0 <= r.from.gp && 0 <= r.to.gp) {
        ui_edit_range_t o = ui_edit_range.order(r);
        return ui_edit_range.compare(o.from, o.to) <= 0;
    } else {
        return false;
    }
}

static bool ui_edit_range_is_empty(const ui_edit_range_t r) {
    return r.from.pn == r.to.pn && r.from.gp == r.to.gp;
}

static ui_edit_pg_t ui_edit_text_end(const ui_edit_text_t* t) {
    return (ui_edit_pg_t){ .pn = t->np - 1, .gp = t->ps[t->np - 1].g };
}

static ui_edit_range_t ui_edit_text_end_range(const ui_edit_text_t* t) {
    ui_edit_pg_t e = (ui_edit_pg_t){ .pn = t->np - 1,
                                     .gp = t->ps[t->np - 1].g };
    return (ui_edit_range_t){ .from = e, .to = e };
}

static uint64_t ui_edit_range_uint64(const ui_edit_pg_t pg) {
    ut_assert(pg.pn >= 0 && pg.gp >= 0);
    return ((uint64_t)pg.pn << 32) | (uint64_t)pg.gp;
}

static ui_edit_pg_t ui_edit_range_pg(uint64_t uint64) {
    ut_assert((int32_t)(uint64 >> 32) >= 0 && (int32_t)uint64 >= 0);
    return (ui_edit_pg_t){ .pn = (int32_t)(uint64 >> 32), .gp = (int32_t)uint64 };
}

static bool ui_edit_range_inside_text(const ui_edit_text_t* t,
        const ui_edit_range_t r) {
    return ui_edit_range.is_valid(r) &&
            0 <= r.from.pn && r.from.pn <= r.to.pn && r.to.pn < t->np &&
            0 <= r.from.gp && r.from.gp <= r.to.gp &&
            r.to.gp <= t->ps[r.to.pn - 1].g;
}

static ui_edit_range_t ui_edit_range_intersect(const ui_edit_range_t r1,
    const ui_edit_range_t r2) {
    if (ui_edit_range.is_valid(r1) && ui_edit_range.is_valid(r2)) {
        ui_edit_range_t o1 = ui_edit_range.order(r1);
        ui_edit_range_t o2 = ui_edit_range.order(r1);
        uint64_t f1 = ((uint64_t)o1.from.pn << 32) | o1.from.gp;
        uint64_t t1 = ((uint64_t)o1.to.pn   << 32) | o1.to.gp;
        uint64_t f2 = ((uint64_t)o2.from.pn << 32) | o2.from.gp;
        uint64_t t2 = ((uint64_t)o2.to.pn   << 32) | o2.to.gp;
        if (f1 <= f2 && f2 <= t1) { // f2 is inside r1
            if (t2 <= t1) { // r2 is fully inside r1
                return r2;
            } else { // r2 is partially inside r1
                ui_edit_range_t r = {0};
                r.from.pn = (int32_t)(f2 >> 32);
                r.from.gp = (int32_t)(f2);
                r.to.pn   = (int32_t)(t1 >> 32);
                r.to.gp   = (int32_t)(t1);
                return r;
            }
        } else if (f2 <= f1 && f1 <= t2) { // f1 is inside r2
            if (t1 <= t2) { // r1 is fully inside r2
                return r1;
            } else { // r1 is partially inside r2
                ui_edit_range_t r = {0};
                r.from.pn = (int32_t)(f1 >> 32);
                r.from.gp = (int32_t)(f1);
                r.to.pn   = (int32_t)(t2 >> 32);
                r.to.gp   = (int32_t)(t2);
                return r;
            }
        } else {
            return *ui_edit_range.invalid_range;
        }
    } else {
        return *ui_edit_range.invalid_range;
    }
}

static bool ui_edit_doc_realloc_ps_no_init(ui_edit_str_t* *ps,
        int32_t old_np, int32_t new_np) { // reallocate paragraphs
    for (int32_t i = new_np; i < old_np; i++) { ui_edit_str.free(&(*ps)[i]); }
    bool ok = true;
    if (new_np == 0) {
        ut_heap.free(*ps);
        *ps = null;
    } else {
        ok = ut_heap.realloc_zero((void**)ps, new_np * sizeof(ui_edit_str_t)) == 0;
    }
    return ok;
}

static bool ui_edit_doc_realloc_ps(ui_edit_str_t* *ps,
        int32_t old_np, int32_t new_np) { // reallocate paragraphs
    bool ok = ui_edit_doc_realloc_ps_no_init(ps, old_np, new_np);
    if (ok) {
        for (int32_t i = old_np; i < new_np; i++) {
            ok = ui_edit_str.init(&(*ps)[i], null, 0, false);
            ut_swear(ok, "because .init(\"\", 0) does NOT allocate memory");
        }
    }
    return ok;
}

static bool ui_edit_text_init(ui_edit_text_t* t,
        const char* s, int32_t b, bool heap) {
    // When text comes from the source that lifetime is shorter
    // than text itself (e.g. paste from clipboard) the parameter
    // heap: true allows to make a copy of data on the heap
    ui_edit_check_zeros(t, sizeof(*t));
    memset(t, 0x00, sizeof(*t));
    if (b < 0) { b = (int32_t)strlen(s); }
    // if caller is concerned with best performance - it should pass b >= 0
    int32_t np = 0; // number of paragraphs
    int32_t n = ut_max(b / 64, 2); // initial number of allocated paragraphs
    ui_edit_str_t* ps = null; // ps[n]
    bool ok = ui_edit_doc_realloc_ps(&ps, 0, n);
    if (ok) {
        bool lf = false;
        int32_t i = 0;
        while (ok && i < b) {
            int32_t k = i;
            while (k < b && s[k] != '\n') { k++; }
            lf = k < b && s[k] == '\n';
            if (np >= n) {
                int32_t n1_5 = n * 3 / 2; // n * 1.5
                ut_assert(n1_5 > n);
                ok = ui_edit_doc_realloc_ps(&ps, n, n1_5);
                if (ok) { n = n1_5; }
            }
            if (ok) {
                // insider knowledge about ui_edit_str allocation behaviour:
                ut_assert(ps[np].c == 0 && ps[np].b == 0 &&
                       ps[np].g2b[0] == 0);
                ui_edit_str.free(&ps[np]);
                // process "\r\n" strings
                const int32_t e = k > i && s[k - 1] == '\r' ? k - 1 : k;
                const int32_t bytes = e - i; ut_assert(bytes >= 0);
                const char* u = bytes == 0 ? null : s + i;
                // str.init may allocate str.g2b[] on the heap and may fail
                ok = ui_edit_str.init(&ps[np], u, bytes, heap && bytes > 0);
                if (ok) { np++; }
            }
            i = k + lf;
        }
        if (ok && lf) { // last paragraph ended with line feed
            if (np + 1 >= n) {
                ok = ui_edit_doc_realloc_ps(&ps, n, n + 1);
                if (ok) { n = n + 1; }
            }
            if (ok) { np++; }
        }
    }
    if (ok && np == 0) { // special case empty string to a single paragraph
        ut_assert(b <= 0 && (b == 0 || s[0] == 0x00));
        np = 1; // ps[0] is already initialized as empty str
        ok = ui_edit_doc_realloc_ps(&ps, n, 1);
        ut_swear(ok, "shrinking ps[] above");
    }
    if (ok) {
        ut_assert(np > 0);
        t->np = np;
        t->ps = ps;
    } else if (ps != null) {
        bool shrink = ui_edit_doc_realloc_ps(&ps, n, 0); // free()
        ut_swear(shrink);
        ut_heap.free(ps);
        t->np = 0;
        t->ps = null;
    }
    return ok;
}

static void ui_edit_text_dispose(ui_edit_text_t* t) {
    if (t->np != 0) {
        ui_edit_doc_realloc_ps(&t->ps, t->np, 0);
        ut_assert(t->ps == null);
        t->np = 0;
    } else {
        ut_assert(t->np == 0 && t->ps == null);
    }
}

static void ui_edit_doc_dispose_to_do(ui_edit_to_do_t* to_do) {
    if (to_do->text.np > 0) {
        ui_edit_text_dispose(&to_do->text);
    }
    memset(&to_do->range, 0x00, sizeof(to_do->range));
    ui_edit_check_zeros(to_do, sizeof(*to_do));
}

static int32_t ui_edit_text_bytes(const ui_edit_text_t* t,
        const ui_edit_range_t* range) {
    const ui_edit_range_t r = ui_edit_text.ordered(t, range);
    ui_edit_check_range_inside_text(t, &r);
    int32_t bytes = 0;
    for (int32_t pn = r.from.pn; pn <= r.to.pn; pn++) {
        const ui_edit_str_t* p = &t->ps[pn];
        if (pn == r.from.pn && pn == r.to.pn) {
            bytes += p->g2b[r.to.gp] - p->g2b[r.from.gp];
        } else if (pn == r.from.pn) {
            bytes += p->b - p->g2b[r.from.gp];
        } else if (pn == r.to.pn) {
            bytes += p->g2b[r.to.gp];
        } else {
            bytes += p->b;
        }
    }
    return bytes;
}

static int32_t ui_edit_doc_bytes(const ui_edit_doc_t* d,
        const ui_edit_range_t* r) {
    return ui_edit_text.bytes(&d->text, r);
}

static int32_t ui_edit_doc_utf8bytes(const ui_edit_doc_t* d,
        const ui_edit_range_t* range) {
    const ui_edit_range_t r = ui_edit_text.ordered(&d->text, range);
    int32_t bytes = ui_edit_text.bytes(&d->text, &r);
    // "\n" after each paragraph and 0x00
    return bytes + r.to.pn - r.from.pn + 1;
}

static void ui_edit_notify_before(ui_edit_doc_t* d,
        const ui_edit_notify_info_t* ni) {
    ui_edit_listener_t* o = d->listeners;
    while (o != null) {
        if (o->notify != null && o->notify->before != null) {
            o->notify->before(o->notify, ni);
        }
        o = o->next;
    }
}

static void ui_edit_notify_after(ui_edit_doc_t* d,
        const ui_edit_notify_info_t* ni) {
    ui_edit_listener_t* o = d->listeners;
    while (o != null) {
        if (o->notify != null && o->notify->after != null) {
            o->notify->after(o->notify, ni);
        }
        o = o->next;
    }
}

static bool ui_edit_doc_subscribe(ui_edit_doc_t* t, ui_edit_notify_t* notify) {
    // TODO: not sure about double linked list.
    // heap allocated resizable array may serve better and may be easier to maintain
    bool ok = true;
    ui_edit_listener_t* o = t->listeners;
    if (o == null) {
        ok = ut_heap.alloc_zero((void**)&t->listeners, sizeof(*o)) == 0;
        if (ok) { o = t->listeners; }
    } else {
        while (o->next != null) { ut_swear(o->notify != notify); o = o->next; }
        ok = ut_heap.alloc_zero((void**)&o->next, sizeof(*o)) == 0;
        if (ok) { o->next->prev = o; o = o->next; }
    }
    if (ok) { o->notify = notify; }
    return ok;
}

static void ui_edit_doc_unsubscribe(ui_edit_doc_t* t, ui_edit_notify_t* notify) {
    ui_edit_listener_t* o = t->listeners;
    bool removed = false;
    while (o != null) {
        ui_edit_listener_t* n = o->next;
        if (o->notify == notify) {
            ut_assert(!removed);
            if (o->prev != null) { o->prev->next = n; }
            if (o->next != null) { o->next->prev = o->prev; }
            if (o == t->listeners) { t->listeners = n; }
            ut_heap.free(o);
            removed = true;
        }
        o = n;
    }
    ut_swear(removed);
}

static bool ui_edit_doc_copy_text(const ui_edit_doc_t* d,
        const ui_edit_range_t* range, ui_edit_text_t* t) {
    ui_edit_check_zeros(t, sizeof(*t));
    memset(t, 0x00, sizeof(*t));
    const ui_edit_range_t r = ui_edit_text.ordered(&d->text, range);
    ui_edit_check_range_inside_text(&d->text, &r);
    int32_t np = r.to.pn - r.from.pn + 1;
    bool ok = ui_edit_doc_realloc_ps(&t->ps, 0, np);
    if (ok) { t->np = np; }
    for (int32_t pn = r.from.pn; ok && pn <= r.to.pn; pn++) {
        const ui_edit_str_t* p = &d->text.ps[pn];
        const char* u = p->u;
        int32_t bytes = 0;
        if (pn == r.from.pn && pn == r.to.pn) {
            bytes = p->g2b[r.to.gp] - p->g2b[r.from.gp];
            u += p->g2b[r.from.gp];
        } else if (pn == r.from.pn) {
            bytes = p->b - p->g2b[r.from.gp];
            u += p->g2b[r.from.gp];
        } else if (pn == r.to.pn) {
            bytes = p->g2b[r.to.gp];
        } else {
            bytes = p->b;
        }
        ut_assert(t->ps[pn - r.from.pn].g == 0);
        const char* u_or_null = bytes == 0 ? null : u;
        ui_edit_str.replace(&t->ps[pn - r.from.pn], 0, 0, u_or_null, bytes);
    }
    if (!ok) {
        ui_edit_text.dispose(t);
        ui_edit_check_zeros(t, sizeof(*t));
    }
    return ok;
}

static void ui_edit_doc_copy(const ui_edit_doc_t* d,
        const ui_edit_range_t* range, char* text, int32_t b) {
    const ui_edit_range_t r = ui_edit_text.ordered(&d->text, range);
    ui_edit_check_range_inside_text(&d->text, &r);
    char* to = text;
    for (int32_t pn = r.from.pn; pn <= r.to.pn; pn++) {
        const ui_edit_str_t* p = &d->text.ps[pn];
        const char* u = p->u;
        int32_t bytes = 0;
        if (pn == r.from.pn && pn == r.to.pn) {
            bytes = p->g2b[r.to.gp] - p->g2b[r.from.gp];
            u += p->g2b[r.from.gp];
        } else if (pn == r.from.pn) {
            bytes = p->b - p->g2b[r.from.gp];
            u += p->g2b[r.from.gp];
        } else if (pn == r.to.pn) {
            bytes = p->g2b[r.to.gp];
        } else {
            bytes = p->b;
        }
        const int32_t c = (int32_t)(uintptr_t)(to - text);
        if (bytes > 0) {
            ut_swear(c + bytes < b, "c: %d bytes: %d b: %d", c, bytes, b);
            memmove(to, u, (size_t)bytes);
            to += bytes;
        }
        if (pn < r.to.pn) {
            ut_swear(c + bytes < b, "c: %d bytes: %d b: %d", c, bytes, b);
            *to++ = '\n';
        }
    }
    const int32_t c = (int32_t)(uintptr_t)(to - text);
    ut_swear(c + 1 == b, "c: %d b: %d", c, b);
    *to++ = 0x00;
}

static bool ui_edit_text_insert_2_or_more(ui_edit_text_t* t, int32_t pn,
        const ui_edit_str_t* s, const ui_edit_text_t* insert,
        const ui_edit_str_t* e) {
    // insert 2 or more paragraphs
    ut_assert(0 <= pn && pn < t->np);
    const int32_t np = t->np + insert->np - 1;
    ut_assert(np > 0);
    ui_edit_str_t* ps = null; // ps[np]
    bool ok = ui_edit_doc_realloc_ps_no_init(&ps, 0, np);
    if (ok) {
        memmove(ps, t->ps, (size_t)pn * sizeof(ui_edit_str_t));
        // `s` first line of `insert`
        ok = ui_edit_str.init(&ps[pn], s->u, s->b, true);
        // lines of `insert` between `s` and `e`
        for (int32_t i = 1; ok && i < insert->np - 1; i++) {
            ok = ui_edit_str.init(&ps[pn + i], insert->ps[i].u,
                                               insert->ps[i].b, true);
        }
        // `e` last line of `insert`
        if (ok) {
            const int32_t ix = pn + insert->np - 1; // last `insert` index
            ok = ui_edit_str.init(&ps[ix], e->u, e->b, true);
        }
        ut_assert(t->np - pn - 1 >= 0);
        memmove(ps + pn + insert->np, t->ps + pn + 1,
               (size_t)(t->np - pn - 1) * sizeof(ui_edit_str_t));
        if (ok) {
            // this two regions where moved to `ps`
            memset(t->ps, 0x00, pn * sizeof(ui_edit_str_t));
            memset(t->ps + pn + 1, 0x00,
                   (size_t)(t->np - pn - 1) * sizeof(ui_edit_str_t));
            // deallocate what was copied from `insert`
            ui_edit_doc_realloc_ps_no_init(&t->ps, t->np, 0);
            t->np = np;
            t->ps = ps;
        } else { // free allocated memory:
            ui_edit_doc_realloc_ps_no_init(&ps, np, 0);
        }
    }
    return ok;
}

static bool ui_edit_text_insert_1(ui_edit_text_t* t,
        const ui_edit_pg_t ip, // insertion point
        const ui_edit_text_t* insert) {
    ut_assert(0 <= ip.pn && ip.pn < t->np);
    ui_edit_str_t* str = &t->ps[ip.pn]; // string in document text
    ut_assert(insert->np == 1);
    ui_edit_str_t* ins = &insert->ps[0]; // string to insert
    ut_assert(0 <= ip.gp && ip.gp <= str->g);
    // ui_edit_str.replace() is all or nothing:
    return ui_edit_str.replace(str, ip.gp, ip.gp, ins->u, ins->b);
}

static bool ui_edit_substr_append(ui_edit_str_t* d, const ui_edit_str_t* s1,
    int32_t gp1, const ui_edit_str_t* s2) { // s1[0:gp1] + s2
    ut_assert(d != s1 && d != s2);
    const int32_t b = s1->g2b[gp1];
    bool ok = ui_edit_str.init(d, b == 0 ? null : s1->u, b, true);
    if (ok) {
        ok = ui_edit_str.replace(d, d->g, d->g, s2->u, s2->b);
    } else {
        *d = *ui_edit_str.empty;
    }
    return ok;
}

static bool ui_edit_append_substr(ui_edit_str_t* d, const ui_edit_str_t* s1,
    const ui_edit_str_t* s2, int32_t gp2) {  // s1 + s2[gp1:*]
    ut_assert(d != s1 && d != s2);
    bool ok = ui_edit_str.init(d, s1->b == 0 ? null : s1->u, s1->b, true);
    if (ok) {
        const int32_t o = s2->g2b[gp2]; // offset (bytes)
        const int32_t b = s2->b - o;
        ok = ui_edit_str.replace(d, d->g, d->g, b == 0 ? null : s2->u + o, b);
    } else {
        *d = *ui_edit_str.empty;
    }
    return ok;
}

static bool ui_edit_text_insert(ui_edit_text_t* t, const ui_edit_pg_t ip,
        const ui_edit_text_t* i) {
    bool ok = true;
    if (ok) {
        if (i->np == 1) {
            ok = ui_edit_text_insert_1(t, ip, i);
        } else {
            ui_edit_str_t* str = &t->ps[ip.pn];
            ui_edit_str_t s = {0}; // start line of insert text `i`
            ui_edit_str_t e = {0}; // end   line
            if (ui_edit_substr_append(&s, str, ip.gp, &i->ps[0])) {
                if (ui_edit_append_substr(&e, &i->ps[i->np - 1], str, ip.gp)) {
                    ok = ui_edit_text_insert_2_or_more(t, ip.pn, &s, i, &e);
                    ui_edit_str.free(&e);
                }
                ui_edit_str.free(&s);
            }
        }
    }
    return ok;
}

static bool ui_edit_text_remove_lines(ui_edit_text_t* t,
    ui_edit_str_t* merge, int32_t from, int32_t to) {
    bool ok = true;
    for (int32_t pn = from + 1; pn <= to; pn++) {
        ui_edit_str.free(&t->ps[pn]);
    }
    if (t->np - to - 1 > 0) {
        memmove(&t->ps[from + 1], &t->ps[to + 1],
                (size_t)(t->np - to - 1) * sizeof(ui_edit_str_t));
    }
    t->np -= to - from;
    if (ok) {
        ui_edit_str.swap(&t->ps[from], merge);
    }
    return ok;
}

static bool ui_edit_text_insert_remove(ui_edit_text_t* t,
        const ui_edit_range_t r, const ui_edit_text_t* i) {
    bool ok = true;
    ui_edit_str_t merge = {0};
    const ui_edit_str_t* s = &t->ps[r.from.pn];
    const ui_edit_str_t* e = &t->ps[r.to.pn];
    const int32_t o = e->g2b[r.to.gp];
    const int32_t b = e->b - o;
    const char* u = b == 0 ? null : e->u + o;
    ok = ui_edit_substr_append(&merge, s, r.from.gp, &i->ps[i->np - 1]) &&
         ui_edit_str.replace(&merge, merge.g, merge.g, u, b);
    if (ok) {
        const bool empty_text = i->np == 1 && i->ps[0].g == 0;
        if (!empty_text) {
            ok = ui_edit_text_insert(t, r.to, i);
        }
        if (ok) {
            ok = ui_edit_text_remove_lines(t, &merge, r.from.pn, r.to.pn);
        }
    }
    if (merge.c > 0 || merge.g > 0) { ui_edit_str.free(&merge); }
    return ok;
}

static bool ui_edit_text_copy_text(const ui_edit_text_t* t,
        const ui_edit_range_t* range, ui_edit_text_t* to) {
    ui_edit_check_zeros(to, sizeof(*to));
    memset(to, 0x00, sizeof(*to));
    const ui_edit_range_t r = ui_edit_text.ordered(t, range);
    ui_edit_check_range_inside_text(t, &r);
    int32_t np = r.to.pn - r.from.pn + 1;
    bool ok = ui_edit_doc_realloc_ps(&to->ps, 0, np);
    if (ok) { to->np = np; }
    for (int32_t pn = r.from.pn; ok && pn <= r.to.pn; pn++) {
        const ui_edit_str_t* p = &t->ps[pn];
        const char* u = p->u;
        int32_t bytes = 0;
        if (pn == r.from.pn && pn == r.to.pn) {
            bytes = p->g2b[r.to.gp] - p->g2b[r.from.gp];
            u += p->g2b[r.from.gp];
        } else if (pn == r.from.pn) {
            bytes = p->b - p->g2b[r.from.gp];
            u += p->g2b[r.from.gp];
        } else if (pn == r.to.pn) {
            bytes = p->g2b[r.to.gp];
        } else {
            bytes = p->b;
        }
        ut_assert(to->ps[pn - r.from.pn].g == 0);
        const char* u_or_null = bytes == 0 ? null : u;
        ui_edit_str.replace(&to->ps[pn - r.from.pn], 0, 0, u_or_null, bytes);
    }
    if (!ok) {
        ui_edit_text.dispose(to);
        ui_edit_check_zeros(to, sizeof(*to));
    }
    return ok;
}

static void ui_edit_text_copy(const ui_edit_text_t* t,
        const ui_edit_range_t* range, char* text, int32_t b) {
    const ui_edit_range_t r = ui_edit_text.ordered(t, range);
    ui_edit_check_range_inside_text(t, &r);
    char* to = text;
    for (int32_t pn = r.from.pn; pn <= r.to.pn; pn++) {
        const ui_edit_str_t* p = &t->ps[pn];
        const char* u = p->u;
        int32_t bytes = 0;
        if (pn == r.from.pn && pn == r.to.pn) {
            bytes = p->g2b[r.to.gp] - p->g2b[r.from.gp];
            u += p->g2b[r.from.gp];
        } else if (pn == r.from.pn) {
            bytes = p->b - p->g2b[r.from.gp];
            u += p->g2b[r.from.gp];
        } else if (pn == r.to.pn) {
            bytes = p->g2b[r.to.gp];
        } else {
            bytes = p->b;
        }
        const int32_t c = (int32_t)(uintptr_t)(to - text);
        ut_swear(c + bytes < b, "d: %d bytes:%d b: %d", c, bytes, b);
        if (bytes > 0) {
            memmove(to, u, (size_t)bytes);
            to += bytes;
        }
        if (pn < r.to.pn) {
            ut_swear(c + bytes + 1 < b, "d: %d bytes:%d b: %d", c, bytes, b);
            *to++ = '\n';
        }
    }
    const int32_t c = (int32_t)(uintptr_t)(to - text);
    ut_swear(c + 1 == b, "d: %d b: %d", c, b);
    *to++ = 0x00;
}

static bool ui_edit_text_replace(ui_edit_text_t* t,
        const ui_edit_range_t* range, const ui_edit_text_t* i,
        ui_edit_to_do_t* undo) {
    const ui_edit_range_t r = ui_edit_text.ordered(t, range);
    bool ok = undo == null ? true : ui_edit_text.copy_text(t, &r, &undo->text);
    ui_edit_range_t x = r;
    if (ok) {
        if (ui_edit_range.is_empty(r)) {
            x.to.pn = r.from.pn + i->np - 1;
            x.to.gp = i->np == 1 ? r.from.gp + i->ps[0].g : i->ps[i->np - 1].g;
            ok = ui_edit_text_insert(t, r.from, i);
        } else if (i->np == 1 && r.from.pn == r.to.pn) {
            x.to.pn = r.from.pn + i->np - 1;
            x.to.gp = r.from.gp + i->ps[0].g;
            ok = ui_edit_str.replace(&t->ps[r.from.pn],
                    r.from.gp, r.to.gp, i->ps[0].u, i->ps[0].b);
        } else {
            x.to.pn = r.from.pn + i->np - 1;
            x.to.gp = i->np == 1 ? r.from.gp + i->ps[0].g : i->ps[0].g;
            ok = ui_edit_text_insert_remove(t, r, i);
        }
    }
    if (undo != null) { undo->range = x; }
    return ok;
}

static bool ui_edit_text_replace_utf8(ui_edit_text_t* t,
        const ui_edit_range_t* range,
        const char* utf8, int32_t b,
        ui_edit_to_do_t* undo) {
    if (b < 0) { b = (int32_t)strlen(utf8); }
    ui_edit_text_t i = {0};
    bool ok = ui_edit_text.init(&i, utf8, b, false);
    if (ok) {
        ok = ui_edit_text.replace(t, range, &i, undo);
        ui_edit_text.dispose(&i);
    }
    return ok;
}

static bool ui_edit_text_dup(ui_edit_text_t* t, const ui_edit_text_t* s) {
    ui_edit_check_zeros(t, sizeof(*t));
    memset(t, 0x00, sizeof(*t));
    bool ok = ui_edit_doc_realloc_ps(&t->ps, 0, s->np);
    if (ok) {
        t->np = s->np;
        for (int32_t i = 0; ok && i < s->np; i++) {
            const ui_edit_str_t* p = &s->ps[i];
            ok = ui_edit_str.replace(&t->ps[i], 0, 0, p->u, p->b);
        }
    }
    if (!ok) {
        ui_edit_text.dispose(t);
    }
    return ok;
}

static bool ui_edit_text_equal(const ui_edit_text_t* t1,
        const ui_edit_text_t* t2) {
    bool equal =  t1->np != t2->np;
    for (int32_t i = 0; equal && i < t1->np; i++) {
        const ui_edit_str_t* p1 = &t1->ps[i];
        const ui_edit_str_t* p2 = &t2->ps[i];
        equal = p1->b == p2->b &&
                memcmp(p1->u, p2->u, p1->b) == 0;
    }
    return equal;
}

static void ui_edit_doc_before_replace_text(ui_edit_doc_t* d,
        const ui_edit_range_t r, const ui_edit_text_t* t) {
    ui_edit_check_range_inside_text(&d->text, &r);
    ui_edit_range_t x = r;
    x.to.pn = r.from.pn + t->np - 1;
    if (r.from.pn == r.to.pn && t->np == 1) {
        x.to.gp = r.from.gp + t->ps[0].g;
    } else {
        x.to.gp = t->ps[t->np - 1].g;
    }
    const ui_edit_notify_info_t ni_before = {
        .ok = true, .d = d, .r = &r, .x = &x, .t = t,
        .pnf = r.from.pn, .pnt = r.to.pn,
        .deleted = 0, .inserted = 0
    };
    ui_edit_notify_before(d, &ni_before);
}

static void ui_edit_doc_after_replace_text(ui_edit_doc_t* d,
        bool ok,
        const ui_edit_range_t r,
        const ui_edit_range_t x,
        const ui_edit_text_t* t) {
    const ui_edit_notify_info_t ni_after = {
        .ok = ok, .d = d, .r = &r, .x = &x, .t = t,
        .pnf = r.from.pn, .pnt = x.to.pn,
        .deleted = r.to.pn - r.from.pn,
        .inserted = t->np - 1
    };
    ui_edit_notify_after(d, &ni_after);
}

static bool ui_edit_doc_replace_text(ui_edit_doc_t* d,
        const ui_edit_range_t* range, const ui_edit_text_t* i,
        ui_edit_to_do_t* undo) {
    ui_edit_text_t* t = &d->text;
    const ui_edit_range_t r = ui_edit_text.ordered(t, range);
    ui_edit_doc_before_replace_text(d, r, i);
    bool ok = ui_edit_text.replace(t, &r, i, undo);
    ui_edit_doc_after_replace_text(d, ok, r, undo->range, i);
    return ok;
}

static bool ui_edit_doc_replace_undoable(ui_edit_doc_t* d,
        const ui_edit_range_t* r, const ui_edit_text_t* t,
        ui_edit_to_do_t* undo) {
    bool ok = ui_edit_doc_replace_text(d, r, t, undo);
    if (ok && undo != null) {
        undo->next = d->undo;
        d->undo = undo;
        // redo stack is not valid after new replace, empty it:
        while (d->redo != null) {
            ui_edit_to_do_t* next = d->redo->next;
            d->redo->next = null;
            ui_edit_doc.dispose_to_do(d->redo);
            ut_heap.free(d->redo);
            d->redo = next;
        }
    }
    return ok;
}

static bool ui_edit_utf8_to_heap_text(const char* u, int32_t b,
        ui_edit_text_t* it) {
    ut_assert((b == 0) == (u == null || u[0] == 0x00));
    return ui_edit_text.init(it, b != 0 ? u : null, b, true);
}


static bool ui_edit_doc_coalesce_undo(ui_edit_doc_t* d, ui_edit_text_t* i) {
    ui_edit_to_do_t* undo = d->undo;
    ui_edit_to_do_t* next = undo->next;
//  ut_println("i: %.*s", i->ps[0].b, i->ps[0].u);
//  if (i->np == 1 && i->ps[0].g == 1) {
//      ut_println("an: %d", ui_edit_str.is_letter(ut_str.utf32(i->ps[0].u, i->ps[0].b)));
//  }
    bool coalesced = false;
    const bool alpha_numeric = i->np == 1 && i->ps[0].g == 1 &&
        ui_edit_str.is_letter(ut_str.utf32(i->ps[0].u, i->ps[0].b));
    if (alpha_numeric && next != null) {
        const ui_edit_range_t ur = undo->range;
        const ui_edit_text_t* ut = &undo->text;
        const ui_edit_range_t nr = next->range;
        const ui_edit_text_t* nt = &next->text;
//      ut_println("next: \"%.*s\" %d:%d..%d:%d undo: \"%.*s\" %d:%d..%d:%d",
//          nt->ps[0].b, nt->ps[0].u, nr.from.pn, nr.from.gp, nr.to.pn, nr.to.gp,
//          ut->ps[0].b, ut->ps[0].u, ur.from.pn, ur.from.gp, ur.to.pn, ur.to.gp);
        const bool c =
            nr.from.pn == nr.to.pn && ur.from.pn == ur.to.pn &&
            nr.from.pn == ur.from.pn &&
            ut->np == 1 && ut->ps[0].g == 0 &&
            nt->np == 1 && nt->ps[0].g == 0 &&
            nr.to.gp == ur.from.gp && nr.to.gp > 0;
        if (c) {
            const ui_edit_str_t* str = &d->text.ps[nr.from.pn];
            const int32_t* g2b = str->g2b;
            const char* utf8 = str->u + g2b[nr.to.gp - 1];
            uint32_t utf32 = ut_str.utf32(utf8, g2b[nr.to.gp] - g2b[nr.to.gp - 1]);
            coalesced = ui_edit_str.is_letter(utf32);
        }
        if (coalesced) {
//          ut_println("coalesced");
            next->range.to.gp++;
            d->undo = next;
            undo->next = null;
            coalesced = true;
        }
    }
    return coalesced;
}

static bool ui_edit_doc_replace(ui_edit_doc_t* d,
        const ui_edit_range_t* range, const char* u, int32_t b) {
    ui_edit_text_t* t = &d->text;
    const ui_edit_range_t r = ui_edit_text.ordered(t, range);
    ui_edit_to_do_t* undo = null;
    bool ok = ut_heap.alloc_zero((void**)&undo, sizeof(ui_edit_to_do_t)) == 0;
    if (ok) {
        ui_edit_text_t i = {0};
        ok = ui_edit_utf8_to_heap_text(u, b, &i);
        if (ok) {
            ok = ui_edit_doc_replace_undoable(d, &r, &i, undo);
            if (ok) {
                if (ui_edit_doc_coalesce_undo(d, &i)) {
                    ui_edit_doc.dispose_to_do(undo);
                    ut_heap.free(undo);
                    undo = null;
                }
            }
            ui_edit_text.dispose(&i);
        }
        if (!ok) {
            ui_edit_doc.dispose_to_do(undo);
            ut_heap.free(undo);
            undo = null;
        }
    }
    return ok;
}

static bool ui_edit_doc_do(ui_edit_doc_t* d, ui_edit_to_do_t* to_do,
        ui_edit_to_do_t* *stack) {
    const ui_edit_range_t* r = &to_do->range;
    ui_edit_to_do_t* redo = null;
    bool ok = ut_heap.alloc_zero((void**)&redo, sizeof(ui_edit_to_do_t)) == 0;
    if (ok) {
        ok = ui_edit_doc_replace_text(d, r, &to_do->text, redo);
        if (ok) {
            ui_edit_doc.dispose_to_do(to_do);
            ut_heap.free(to_do);
        }
        if (ok) {
            redo->next = *stack;
            *stack = redo;
        } else {
            if (redo != null) {
                ui_edit_doc.dispose_to_do(redo);
                ut_heap.free(redo);
            }
        }
    }
    return ok;
}

static bool ui_edit_doc_redo(ui_edit_doc_t* d) {
    ui_edit_to_do_t* to_do = d->redo;
    if (to_do == null) {
        return false;
    } else {
        d->redo = d->redo->next;
        to_do->next = null;
        return ui_edit_doc_do(d, to_do, &d->undo);
    }
}

static bool ui_edit_doc_undo(ui_edit_doc_t* d) {
    ui_edit_to_do_t* to_do = d->undo;
    if (to_do == null) {
        return false;
    } else {
        d->undo = d->undo->next;
        to_do->next = null;
        return ui_edit_doc_do(d, to_do, &d->redo);
    }
}

static bool ui_edit_doc_init(ui_edit_doc_t* d, const char* utf8,
        int32_t bytes, bool heap) {
    bool ok = true;
    ui_edit_check_zeros(d, sizeof(*d));
    memset(d, 0x00, sizeof(d));
    ut_assert(bytes >= 0);
    ut_assert((utf8 == null) == (bytes == 0));
    if (ok) {
        if (bytes == 0) { // empty string
            ok = ut_heap.alloc_zero((void**)&d->text.ps, sizeof(ui_edit_str_t)) == 0;
            if (ok) {
                d->text.np = 1;
                ok = ui_edit_str.init(&d->text.ps[0], null, 0, false);
            }
        } else {
            ok = ui_edit_text.init(&d->text, utf8, bytes, heap);
        }
    }
    return ok;
}

static void ui_edit_doc_dispose(ui_edit_doc_t* d) {
    for (int32_t i = 0; i < d->text.np; i++) {
        ui_edit_str.free(&d->text.ps[i]);
    }
    if (d->text.ps != null) {
        ut_heap.free(d->text.ps);
        d->text.ps = null;
    }
    d->text.np  = 0;
    while (d->undo != null) {
        ui_edit_to_do_t* next = d->undo->next;
        d->undo->next = null;
        ui_edit_doc.dispose_to_do(d->undo);
        ut_heap.free(d->undo);
        d->undo = next;
    }
    while (d->redo != null) {
        ui_edit_to_do_t* next = d->redo->next;
        d->redo->next = null;
        ui_edit_doc.dispose_to_do(d->redo);
        ut_heap.free(d->redo);
        d->redo = next;
    }
    ut_assert(d->listeners == null, "unsubscribe listeners?");
    while (d->listeners != null) {
        ui_edit_listener_t* next = d->listeners->next;
        d->listeners->next = null;
        ut_heap.free(d->listeners->next);
        d->listeners = next;
    }
    ui_edit_check_zeros(d, sizeof(*d));
}

// ui_edit_str

static int32_t ui_edit_str_g2b_ascii[1024]; // ui_edit_str_g2b_ascii[i] == i for all "i"
static int8_t  ui_edit_str_empty_utf8[1] = {0x00};

static const ui_edit_str_t ui_edit_str_empty = {
    .u = ui_edit_str_empty_utf8,
    .g2b = ui_edit_str_g2b_ascii,
    .c = 0, .b = 0, .g = 0
};

static bool    ui_edit_str_init(ui_edit_str_t* s, const char* u, int32_t b, bool heap);
static void    ui_edit_str_swap(ui_edit_str_t* s1, ui_edit_str_t* s2);
static int32_t ui_edit_str_gp_to_bp(const char* s, int32_t bytes, int32_t gp);
static int32_t ui_edit_str_bytes(ui_edit_str_t* s, int32_t f, int32_t t);
static bool    ui_edit_str_expand(ui_edit_str_t* s, int32_t c);
static void    ui_edit_str_shrink(ui_edit_str_t* s);
static bool    ui_edit_str_replace(ui_edit_str_t* s, int32_t f, int32_t t,
                                   const char* u, int32_t b);

//  bool (*is_zwj)(uint32_t utf32); // zero width joiner
//  bool (*is_letter)(uint32_t utf32); // in European Alphabets
//  bool (*is_digit)(uint32_t utf32);
//  bool (*is_symbol)(uint32_t utf32);
//  bool (*is_alphanumeric)(uint32_t utf32);
//  bool (*is_blank)(uint32_t utf32); // white space
//  bool (*is_punctuation)(uint32_t utf32);
//  bool (*is_combining)(uint32_t utf32);
//  bool (*is_spacing)(uint32_t utf32); // spacing modifiers
//  bool (*is_cjk_or_emoji)(uint32_t utf32);

static bool ui_edit_str_is_zwj(uint32_t utf32);
static bool ui_edit_str_is_letter(uint32_t utf32);
static bool ui_edit_str_is_digit(uint32_t utf32);
static bool ui_edit_str_is_symbol(uint32_t utf32);
static bool ui_edit_str_is_alphanumeric(uint32_t utf32);
static bool ui_edit_str_is_blank(uint32_t utf32);
static bool ui_edit_str_is_punctuation(uint32_t utf32);
static bool ui_edit_str_is_combining(uint32_t utf32);
static bool ui_edit_str_is_spacing(uint32_t utf32);
static bool ui_edit_str_is_blank(uint32_t utf32);
static bool ui_edit_str_is_cjk_or_emoji(uint32_t utf32);
static bool ui_edit_str_can_break(uint32_t cp1, uint32_t cp2);

static void    ui_edit_str_test(void);
static void    ui_edit_str_free(ui_edit_str_t* s);

ui_edit_str_if ui_edit_str = {
    .init            = ui_edit_str_init,
    .swap            = ui_edit_str_swap,
    .gp_to_bp        = ui_edit_str_gp_to_bp,
    .bytes           = ui_edit_str_bytes,
    .expand          = ui_edit_str_expand,
    .shrink          = ui_edit_str_shrink,
    .replace         = ui_edit_str_replace,
    .is_zwj          = ui_edit_str_is_zwj,
    .is_letter       = ui_edit_str_is_letter,
    .is_digit        = ui_edit_str_is_digit,
    .is_symbol       = ui_edit_str_is_symbol,
    .is_alphanumeric = ui_edit_str_is_alphanumeric,
    .is_blank        = ui_edit_str_is_blank,
    .is_punctuation  = ui_edit_str_is_punctuation,
    .is_combining    = ui_edit_str_is_combining,
    .is_spacing      = ui_edit_str_is_spacing,
    .is_punctuation  = ui_edit_str_is_punctuation,
    .is_cjk_or_emoji = ui_edit_str_is_cjk_or_emoji,
    .can_break       = ui_edit_str_can_break,
    .test            = ui_edit_str_test,
    .free            = ui_edit_str_free,
    .empty           = &ui_edit_str_empty
};

#pragma push_macro("ui_edit_str_check")
#pragma push_macro("ui_edit_str_check_from_to")
#pragma push_macro("ui_edit_check_zeros")
#pragma push_macro("ui_edit_str_check_empty")
#pragma push_macro("ui_edit_str_parameters")

#ifdef DEBUG

#define ui_edit_str_check(s) do {                                   \
    /* check the s struct constrains */                             \
    ut_assert(s->b >= 0);                                              \
    ut_assert(s->c == 0 || s->c >= s->b);                              \
    ut_assert(s->g >= 0);                                              \
    /* s->g2b[] may be null (not heap allocated) when .b == 0 */    \
    if (s->g == 0) { ut_assert(s->b == 0); }                           \
    if (s->g > 0) {                                                 \
        ut_assert(s->g2b[0] == 0 && s->g2b[s->g] == s->b);             \
    }                                                               \
    for (int32_t i = 1; i < s->g; i++) {                            \
        ut_assert(0 < s->g2b[i] - s->g2b[i - 1] &&                     \
                   s->g2b[i] - s->g2b[i - 1] <= 4);                 \
        ut_assert(s->g2b[i] - s->g2b[i - 1] ==                         \
            ut_str.utf8bytes(                                 \
            s->u + s->g2b[i - 1], s->g2b[i] - s->g2b[i - 1]));      \
    }                                                               \
} while (0)

#define ui_edit_str_check_from_to(s, f, t) do {                     \
    ut_assert(0 <= f && f <= s->g);                                    \
    ut_assert(0 <= t && t <= s->g);                                    \
    ut_assert(f <= t);                                                 \
} while (0)

#define ui_edit_str_check_empty(u, b) do {                          \
    if (b == 0) { ut_assert(u != null && u[0] == 0x00); }              \
    if (u == null || u[0] == 0x00) { ut_assert(b == 0); }              \
} while (0)



#else

#define ui_edit_str_check(s)               do { } while (0)
#define ui_edit_str_check_from_to(s, f, t) do { } while (0)
#define ui_edit_str_check_empty(u, b)      do { } while (0)

#endif

// ui_edit_str_foo(*, "...", -1) treat as 0x00 terminated
// ui_edit_str_foo(*, null, 0) treat as ("", 0)

#define ui_edit_str_parameters(u, b) do {                           \
    if (u == null) { u = ui_edit_str_empty_utf8; }                  \
    if (b < 0)  {                                                   \
        ut_assert(strlen(u) < INT32_MAX);                              \
        b = (int32_t)strlen(u);                                     \
    }                                                               \
    ui_edit_str_check_empty(u, b);                                  \
} while (0)

static int32_t ui_edit_str_gp_to_bp(const char* utf8, int32_t bytes, int32_t gp) {
    ut_swear(bytes >= 0);
    bool ok = true;
    int32_t c = 0;
    int32_t i = 0;
    if (bytes > 0) {
        while (c < gp && ok) {
            ut_assert(i < bytes);
            const int32_t b = ut_str.utf8bytes(utf8 + i, bytes - i);
            ok = 0 < b && i + b <= bytes;
            if (ok) { i += b; c++; }
        }
    }
    ut_assert(i <= bytes);
    return ok ? i : -1;
}

static void ui_edit_str_free(ui_edit_str_t* s) {
    if (s->g2b != null && s->g2b != ui_edit_str_g2b_ascii) {
        ut_heap.free(s->g2b);
    } else {
        #ifdef UI_EDIT_STR_TEST // check ui_edit_str_g2b_ascii integrity
            for (int32_t i = 0; i < ut_countof(ui_edit_str_g2b_ascii); i++) {
                ut_assert(ui_edit_str_g2b_ascii[i] == i);
            }
        #endif
    }
    s->g2b = null;
    s->g = 0;
    if (s->c > 0) {
        ut_heap.free(s->u);
        s->u = null;
        s->c = 0;
        s->b = 0;
    } else {
        s->u = null;
        s->b = 0;
    }
    ui_edit_check_zeros(s, sizeof(*s));
}

static bool ui_edit_str_init_g2b(ui_edit_str_t* s) {
    const int64_t _4_bytes = (int64_t)sizeof(int32_t);
    // start with number of glyphs == number of bytes (ASCII text):
    bool ok = ut_heap.alloc(&s->g2b, (size_t)(s->b + 1) * _4_bytes) == 0;
    int32_t i = 0; // index in u[] string
    int32_t k = 1; // glyph number
    // g2b[k] start postion in uint8_t offset from utf8 text of glyph[k]
    while (i < s->b && ok) {
        const int32_t b = ut_str.utf8bytes(s->u + i, s->b - i);
        ok = b > 0 && i + b <= s->b;
        if (ok) {
            i += b;
            s->g2b[k] = i;
            k++;
        }
    }
    if (ok) {
        ut_assert(0 < k && k <= s->b + 1);
        s->g2b[0] = 0;
        ut_assert(s->g2b[k - 1] == s->b);
        s->g = k - 1;
        if (k < s->b + 1) {
            ok = ut_heap.realloc(&s->g2b, k * _4_bytes) == 0;
            ut_assert(ok, "shrinking - should always be ok");
        }
    }
    return ok;
}

static bool ui_edit_str_init(ui_edit_str_t* s, const char* u, int32_t b,
        bool heap) {
    enum { n = ut_countof(ui_edit_str_g2b_ascii) };
    if (ui_edit_str_g2b_ascii[n - 1] != n - 1) {
        for (int32_t i = 0; i < n; i++) { ui_edit_str_g2b_ascii[i] = i; }
    }
    bool ok = true;
    ui_edit_check_zeros(s, sizeof(*s)); // caller must zero out
    memset(s, 0x00, sizeof(*s));
    ui_edit_str_parameters(u, b);
    if (b == 0) { // cast below intentionally removes "const" qualifier
        s->g2b = (int32_t*)ui_edit_str_g2b_ascii;
        s->u = (char*)u;
        ut_assert(s->c == 0 && u[0] == 0x00);
    } else {
        if (heap) {
            ok = ut_heap.alloc((void**)&s->u, b) == 0;
            if (ok) { s->c = b; memmove(s->u, u, (size_t)b); }
        } else {
            s->u = (char*)u;
        }
        if (ok) {
            s->b = b;
            if (b == 1 && u[0] <= 0x7F) {
                s->g2b = (int32_t*)ui_edit_str_g2b_ascii;
                s->g = 1;
            } else {
                ok = ui_edit_str_init_g2b(s);
            }
        }
    }
    if (ok) { ui_edit_str.shrink(s); } else { ui_edit_str.free(s); }
    return ok;
}

static void ui_edit_str_swap(ui_edit_str_t* s1, ui_edit_str_t* s2) {
    ui_edit_str_t s = *s1; *s1 = *s2; *s2 = s;
}

static int32_t ui_edit_str_bytes(ui_edit_str_t* s,
        int32_t f, int32_t t) { // glyph positions
    ui_edit_str_check_from_to(s, f, t);
    ui_edit_str_check(s);
    return s->g2b[t] - s->g2b[f];
}

static bool ui_edit_str_move_g2b_to_heap(ui_edit_str_t* s) {
    bool ok = true;
    if (s->g2b == ui_edit_str_g2b_ascii) { // even for s->g == 0
        if (s->b == s->g && s->g < ut_countof(ui_edit_str_g2b_ascii) - 1) {
//          ut_println("forcefully moving to heap");
            // this is usually done in the process of concatenation
            // of 2 ascii strings when result is known to be longer
            // than ut_countof(ui_edit_str_g2b_ascii) - 1 but the
            // first string in concatenation is short. It's OK.
        }
        const int32_t bytes = (s->g + 1) * (int32_t)sizeof(int32_t);
        ok = ut_heap.alloc(&s->g2b, bytes) == 0;
        if (ok) { memmove(s->g2b, ui_edit_str_g2b_ascii, (size_t)bytes); }
    }
    return ok;
}

static bool ui_edit_str_move_to_heap(ui_edit_str_t* s, int32_t c) {
    bool ok = true;
    ut_assert(c >= s->b, "can expand cannot shrink");
    if (s->c == 0) { // s->u points outside of the heap
        const char* o = s->u;
        ok = ut_heap.alloc((void**)&s->u, c) == 0;
        if (ok) { memmove(s->u, o, (size_t)s->b); }
    } else if (s->c < c) {
        ok = ut_heap.realloc((void**)&s->u, c) == 0;
    }
    if (ok) { s->c = c; }
    return ok;
}

static bool ui_edit_str_expand(ui_edit_str_t* s, int32_t c) {
    ut_swear(c > 0);
    bool ok = ui_edit_str_move_to_heap(s, c);
    if (ok && c > s->c) {
        if (ut_heap.realloc((void**)&s->u, c) == 0) {
            s->c = c;
        } else {
            ok = false;
        }
    }
    return ok;
}

static void ui_edit_str_shrink(ui_edit_str_t* s) {
    if (s->c > s->b) { // s->c == 0 for empty and single byte ASCII strings
        ut_assert(s->u != ui_edit_str_empty_utf8);
        if (s->b == 0) {
            ut_heap.free(s->u);
            s->u = ui_edit_str_empty_utf8;
        } else {
            bool ok = ut_heap.realloc((void**)&s->u, s->b) == 0;
            ut_swear(ok, "smaller size is always expected to be ok");
        }
        s->c = s->b;
    }
    // Optimize memory for short ASCII only strings:
    if (s->g2b != ui_edit_str_g2b_ascii) {
        if (s->g == s->b && s->g < ut_countof(ui_edit_str_g2b_ascii) - 1) {
            // If this is an ascii only utf8 string shorter than
            // ui_edit_str_g2b_ascii it does not need .g2b[] allocated:
            if (s->g2b != ui_edit_str_g2b_ascii) {
                ut_heap.free(s->g2b);
                s->g2b = ui_edit_str_g2b_ascii;
            }
        } else {
//          const int32_t b64 = ut_min(s->b, 64);
//          ut_println("none ASCII: .b:%d .g:%d %*.*s", s->b, s->g, b64, b64, s->u);
        }
    }
}

static bool ui_edit_str_remove(ui_edit_str_t* s, int32_t f, int32_t t) {
    bool ok = true; // optimistic approach
    ui_edit_str_check_from_to(s, f, t);
    ui_edit_str_check(s);
    const int32_t bytes_to_remove = s->g2b[t] - s->g2b[f];
    ut_assert(bytes_to_remove >= 0);
    if (bytes_to_remove > 0) {
        ok = ui_edit_str_move_to_heap(s, s->b);
        if (ok) {
            const int32_t bytes_to_shift = s->b - s->g2b[t];
            ut_assert(0 <= bytes_to_shift && bytes_to_shift <= s->b);
            memmove(s->u + s->g2b[f], s->u + s->g2b[t], (size_t)bytes_to_shift);
            if (s->g2b != ui_edit_str_g2b_ascii) {
                memmove(s->g2b + f, s->g2b + t,
                        (size_t)(s->g - t + 1) * sizeof(int32_t));
                for (int32_t i = f; i <= s->g; i++) {
                    s->g2b[i] -= bytes_to_remove;
                }
            } else {
                // no need to shrink g2b[] for ASCII only strings:
                for (int32_t i = 0; i <= s->g; i++) { ut_assert(s->g2b[i] == i); }
            }
            s->b -= bytes_to_remove;
            s->g -= t - f;
        }
    }
    ui_edit_str_check(s);
    return ok;
}

static bool ui_edit_str_replace(ui_edit_str_t* s,
        int32_t f, int32_t t, const char* u, int32_t b) {
    const int64_t _4_bytes = (int64_t)sizeof(int32_t);
    bool ok = true; // optimistic approach
    ui_edit_str_check_from_to(s, f, t);
    ui_edit_str_check(s);
    ui_edit_str_parameters(u, b);
    // we are inserting "b" bytes and removing "t - f" glyphs
    const int32_t bytes_to_remove = s->g2b[t] - s->g2b[f];
    const int32_t bytes_to_insert = b; // only for readability
    if (b == 0) { // just remove glyphs
        ok = ui_edit_str_remove(s, f, t);
    } else { // remove and insert
        ui_edit_str_t ins = {0};
        // ui_edit_str_init_ro() verifies utf-8 and calculates g2b[]:
        ok = ui_edit_str_init(&ins, u, b, false);
        const int32_t glyphs_to_insert = ins.g; // only for readability
        const int32_t glyphs_to_remove = t - f; // only for readability
        if (ok) {
            const int32_t bytes = s->b + bytes_to_insert - bytes_to_remove;
            ut_assert(ins.g2b != null); // pacify code analysis
            ut_assert(bytes > 0);
            const int32_t c = ut_max(s->b, bytes);
            // keep g2b == ui_edit_str_g2b_ascii as much as possible
            const bool all_ascii = s->g2b == ui_edit_str_g2b_ascii &&
                                   ins.g2b == ui_edit_str_g2b_ascii &&
                                   bytes < ut_countof(ui_edit_str_g2b_ascii) - 1;
            ok = ui_edit_str_move_to_heap(s, c);
            if (ok) {
                if (!all_ascii) {
                    ui_edit_str_move_g2b_to_heap(s);
                }
                // insert ui_edit_str_t "ins" at glyph position "f"
                // reusing ins.u[0..ins.b-1] and ins.g2b[0..ins.g]
                // moving memory using memmove() left to right:
                if (bytes_to_insert <= bytes_to_remove) {
                    memmove(s->u + s->g2b[f] + bytes_to_insert,
                           s->u + s->g2b[f] + bytes_to_remove,
                           (size_t)(s->b - s->g2b[f] - bytes_to_remove));
                    if (all_ascii) {
                        ut_assert(s->g2b == ui_edit_str_g2b_ascii);
                    } else {
                        ut_assert(s->g2b != ui_edit_str_g2b_ascii);
                        memmove(s->g2b + f + glyphs_to_insert,
                               s->g2b + f + glyphs_to_remove,
                               (size_t)(s->g - t + 1) * _4_bytes);
                    }
                    memmove(s->u + s->g2b[f], ins.u, (size_t)ins.b);
                } else {
                    if (all_ascii) {
                        ut_assert(s->g2b == ui_edit_str_g2b_ascii);
                    } else {
                        ut_assert(s->g2b != ui_edit_str_g2b_ascii);
                        const int32_t g = s->g + glyphs_to_insert -
                                                 glyphs_to_remove;
                        ut_assert(g > s->g);
                        ok = ut_heap.realloc(&s->g2b,
                                             (size_t)(g + 1) * _4_bytes) == 0;
                    }
                    // need to shift bytes staring with s.g2b[t] toward the end
                    if (ok) {
                        memmove(s->u + s->g2b[f] + bytes_to_insert,
                                s->u + s->g2b[f] + bytes_to_remove,
                                (size_t)(s->b - s->g2b[f] - bytes_to_remove));
                        if (all_ascii) {
                            ut_assert(s->g2b == ui_edit_str_g2b_ascii);
                        } else {
                            ut_assert(s->g2b != ui_edit_str_g2b_ascii);
                            memmove(s->g2b + f + glyphs_to_insert,
                                    s->g2b + f + glyphs_to_remove,
                                    (size_t)(s->g - t + 1) * _4_bytes);
                        }
                        memmove(s->u + s->g2b[f], ins.u, (size_t)ins.b);
                    }
                }
                if (ok) {
                    if (!all_ascii) {
                        ut_assert(s->g2b != null && s->g2b != ui_edit_str_g2b_ascii);
                        for (int32_t i = f; i <= f + glyphs_to_insert; i++) {
                            s->g2b[i] = ins.g2b[i - f] + s->g2b[f];
                        }
                    } else {
                        ut_assert(s->g2b == ui_edit_str_g2b_ascii);
                        for (int32_t i = f; i <= f + glyphs_to_insert; i++) {
                            ut_assert(ui_edit_str_g2b_ascii[i] == i);
                            ut_assert(ins.g2b[i - f] + s->g2b[f] == i);
                        }
                    }
                    s->b += bytes_to_insert - bytes_to_remove;
                    s->g += glyphs_to_insert - glyphs_to_remove;
                    ut_assert(s->b == bytes);
                    if (!all_ascii) {
                        ut_assert(s->g2b != ui_edit_str_g2b_ascii);
                        for (int32_t i = f + glyphs_to_insert + 1; i <= s->g; i++) {
                            s->g2b[i] += bytes_to_insert - bytes_to_remove;
                        }
                        s->g2b[s->g] = s->b;
                    } else {
                        ut_assert(s->g2b == ui_edit_str_g2b_ascii);
                        for (int32_t i = f + glyphs_to_insert + 1; i <= s->g; i++) {
                            ut_assert(s->g2b[i] == i);
                            ut_assert(ui_edit_str_g2b_ascii[i] == i);
                        }
                        ut_assert(s->g2b[s->g] == s->b);
                    }
                }
            }
            ui_edit_str_free(&ins);
        }
    }
    ui_edit_str_shrink(s);
    ui_edit_str_check(s);
    return ok;
}

static bool ui_edit_str_is_zwj(uint32_t utf32) {
    return utf32 == 0x200D;
}

static bool ui_edit_str_is_punctuation(uint32_t utf32) {
    return
        (utf32 >= 0x0021 && utf32 <= 0x0023) ||  // !"#
        (utf32 >= 0x0025 && utf32 <= 0x002A) ||  // %&'()*+
        (utf32 >= 0x002C && utf32 <= 0x002F) ||  // ,-./
        (utf32 >= 0x003A && utf32 <= 0x003B) ||  //:;
        (utf32 >= 0x003F && utf32 <= 0x0040) ||  // ?@
        (utf32 >= 0x005B && utf32 <= 0x005D) ||  // [\]
        (utf32 == 0x005F) ||                     // _
        (utf32 == 0x007B) ||                     // {
        (utf32 == 0x007D) ||                     // }
        (utf32 == 0x007E) ||                     // ~
        (utf32 >= 0x2000 && utf32 <= 0x206F) ||  // General Punctuation
        (utf32 >= 0x3000 && utf32 <= 0x303F) ||  // CJK Symbols and Punctuation
        (utf32 >= 0xFE30 && utf32 <= 0xFE4F) ||  // CJK Compatibility Forms
        (utf32 >= 0xFE50 && utf32 <= 0xFE6F) ||  // Small Form Variants
        (utf32 >= 0xFF01 && utf32 <= 0xFF0F) ||  // Fullwidth ASCII variants
        (utf32 >= 0xFF1A && utf32 <= 0xFF1F) ||  // Fullwidth ASCII variants
        (utf32 >= 0xFF3B && utf32 <= 0xFF3D) ||  // Fullwidth ASCII variants
        (utf32 == 0xFF3F) ||                     // Fullwidth _
        (utf32 >= 0xFF5B && utf32 <= 0xFF65);    // Fullwidth ASCII variants and halfwidth forms
}

static bool ui_edit_str_is_letter(uint32_t utf32) {
    return
        (utf32 >= 0x0041 && utf32 <= 0x005A) ||  // Latin uppercase
        (utf32 >= 0x0061 && utf32 <= 0x007A) ||  // Latin lowercase
        (utf32 >= 0x00C0 && utf32 <= 0x00D6) ||  // Latin-1 uppercase
        (utf32 >= 0x00D8 && utf32 <= 0x00F6) ||  // Latin-1 lowercase
        (utf32 >= 0x00F8 && utf32 <= 0x00FF) ||  // Latin-1 lowercase
        (utf32 >= 0x0100 && utf32 <= 0x017F) ||  // Latin Extended-A
        (utf32 >= 0x0180 && utf32 <= 0x024F) ||  // Latin Extended-B
        (utf32 >= 0x0250 && utf32 <= 0x02AF) ||  // IPA Extensions
        (utf32 >= 0x0370 && utf32 <= 0x03FF) ||  // Greek and Coptic
        (utf32 >= 0x0400 && utf32 <= 0x04FF) ||  // Cyrillic
        (utf32 >= 0x0500 && utf32 <= 0x052F) ||  // Cyrillic Supplement
        (utf32 >= 0x0530 && utf32 <= 0x058F) ||  // Armenian
        (utf32 >= 0x10A0 && utf32 <= 0x10FF) ||  // Georgian
        (utf32 >= 0x0600 && utf32 <= 0x06FF) ||  // Arabic (covers Arabic, Kurdish, and Pashto)
        (utf32 >= 0x0900 && utf32 <= 0x097F) ||  // Devanagari (covers Hindi)
        (utf32 >= 0x0980 && utf32 <= 0x09FF) ||  // Bengali
        (utf32 >= 0x0A00 && utf32 <= 0x0A7F) ||  // Gurmukhi (common in Northern India, related to Punjabi)
        (utf32 >= 0x0B80 && utf32 <= 0x0BFF) ||  // Tamil
        (utf32 >= 0x0C00 && utf32 <= 0x0C7F) ||  // Telugu
        (utf32 >= 0x0C80 && utf32 <= 0x0CFF) ||  // Kannada
        (utf32 >= 0x0D00 && utf32 <= 0x0D7F) ||  // Malayalam
        (utf32 >= 0x0D80 && utf32 <= 0x0DFF) ||  // Sinhala
        (utf32 >= 0x3040 && utf32 <= 0x309F) ||  // Hiragana (because it is syllabic)
        (utf32 >= 0x30A0 && utf32 <= 0x30FF) ||  // Katakana
        (utf32 >= 0x1E00 && utf32 <= 0x1EFF);    // Latin Extended Additional
}

static bool ui_edit_str_is_spacing(uint32_t utf32) {
    return
        (utf32 >= 0x02B0 && utf32 <= 0x02FF) ||  // Spacing Modifier Letters
        (utf32 >= 0xA700 && utf32 <= 0xA71F);    // Modifier Tone Letters
}

static bool ui_edit_str_is_combining(uint32_t utf32) {
    return
        (utf32 >= 0x0300 && utf32 <= 0x036F) ||  // Combining Diacritical Marks
        (utf32 >= 0x1AB0 && utf32 <= 0x1AFF) ||  // Combining Diacritical Marks Extended
        (utf32 >= 0x1DC0 && utf32 <= 0x1DFF) ||  // Combining Diacritical Marks Supplement
        (utf32 >= 0x20D0 && utf32 <= 0x20FF) ||  // Combining Diacritical Marks for Symbols
        (utf32 >= 0xFE20 && utf32 <= 0xFE2F);    // Combining Half Marks
}

static bool ui_edit_str_is_blank(uint32_t utf32) {
    return
        (utf32 == 0x0009) ||  // Horizontal Tab
        (utf32 == 0x000A) ||  // Line Feed
        (utf32 == 0x000B) ||  // Vertical Tab
        (utf32 == 0x000C) ||  // Form Feed
        (utf32 == 0x000D) ||  // Carriage Return
        (utf32 == 0x0020) ||  // Space
        (utf32 == 0x0085) ||  // Next Line
        (utf32 == 0x00A0) ||  // Non-breaking Space
        (utf32 == 0x1680) ||  // Ogham Space Mark
        (utf32 >= 0x2000 && utf32 <= 0x200A) ||  // En Quad to Hair Space
        (utf32 == 0x2028) ||  // Line Separator
        (utf32 == 0x2029) ||  // Paragraph Separator
        (utf32 == 0x202F) ||  // Narrow No-Break Space
        (utf32 == 0x205F) ||  // Medium Mathematical Space
        (utf32 == 0x3000);    // Ideographic Space
}

static bool ui_edit_str_is_symbol(uint32_t utf32) {
    return
        (utf32 >= 0x0024 && utf32 <= 0x0024) ||  // Dollar sign
        (utf32 >= 0x00A2 && utf32 <= 0x00A5) ||  // Cent sign to Yen sign
        (utf32 >= 0x20A0 && utf32 <= 0x20CF) ||  // Currency Symbols
        (utf32 >= 0x2100 && utf32 <= 0x214F) ||  // Letter like Symbols
        (utf32 >= 0x2190 && utf32 <= 0x21FF) ||  // Arrows
        (utf32 >= 0x2200 && utf32 <= 0x22FF) ||  // Mathematical Operators
        (utf32 >= 0x2300 && utf32 <= 0x23FF) ||  // Miscellaneous Technical
        (utf32 >= 0x2400 && utf32 <= 0x243F) ||  // Control Pictures
        (utf32 >= 0x2440 && utf32 <= 0x245F) ||  // Optical Character Recognition
        (utf32 >= 0x2460 && utf32 <= 0x24FF) ||  // Enclosed Alphanumeric
        (utf32 >= 0x2500 && utf32 <= 0x257F) ||  // Box Drawing
        (utf32 >= 0x2580 && utf32 <= 0x259F) ||  // Block Elements
        (utf32 >= 0x25A0 && utf32 <= 0x25FF) ||  // Geometric Shapes
        (utf32 >= 0x2600 && utf32 <= 0x26FF) ||  // Miscellaneous Symbols
        (utf32 >= 0x2700 && utf32 <= 0x27BF) ||  // Dingbats
        (utf32 >= 0x2900 && utf32 <= 0x297F) ||  // Supplemental Arrows-B
        (utf32 >= 0x2B00 && utf32 <= 0x2BFF) ||  // Miscellaneous Symbols and Arrows
        (utf32 >= 0xFB00 && utf32 <= 0xFB4F) ||  // Alphabetic Presentation Forms
        (utf32 >= 0xFE50 && utf32 <= 0xFE6F) ||  // Small Form Variants
        (utf32 >= 0xFF01 && utf32 <= 0xFF20) ||  // Fullwidth ASCII variants
        (utf32 >= 0xFF3B && utf32 <= 0xFF40) ||  // Fullwidth ASCII variants
        (utf32 >= 0xFF5B && utf32 <= 0xFF65);    // Fullwidth ASCII variants
}

static bool ui_edit_str_is_digit(uint32_t utf32) {
    return
        (utf32 >= 0x0030 && utf32 <= 0x0039) ||  // ASCII digits 0-9
        (utf32 >= 0x0660 && utf32 <= 0x0669) ||  // Arabic-Indic digits
        (utf32 >= 0x06F0 && utf32 <= 0x06F9) ||  // Extended Arabic-Indic digits
        (utf32 >= 0x07C0 && utf32 <= 0x07C9) ||  // N'Ko digits
        (utf32 >= 0x0966 && utf32 <= 0x096F) ||  // Devanagari digits
        (utf32 >= 0x09E6 && utf32 <= 0x09EF) ||  // Bengali digits
        (utf32 >= 0x0A66 && utf32 <= 0x0A6F) ||  // Gurmukhi digits
        (utf32 >= 0x0AE6 && utf32 <= 0x0AEF) ||  // Gujarati digits
        (utf32 >= 0x0B66 && utf32 <= 0x0B6F) ||  // Oriya digits
        (utf32 >= 0x0BE6 && utf32 <= 0x0BEF) ||  // Tamil digits
        (utf32 >= 0x0C66 && utf32 <= 0x0C6F) ||  // Telugu digits
        (utf32 >= 0x0CE6 && utf32 <= 0x0CEF) ||  // Kannada digits
        (utf32 >= 0x0D66 && utf32 <= 0x0D6F) ||  // Malayalam digits
        (utf32 >= 0x0E50 && utf32 <= 0x0E59) ||  // Thai digits
        (utf32 >= 0x0ED0 && utf32 <= 0x0ED9) ||  // Lao digits
        (utf32 >= 0x0F20 && utf32 <= 0x0F29) ||  // Tibetan digits
        (utf32 >= 0x1040 && utf32 <= 0x1049) ||  // Myanmar digits
        (utf32 >= 0x17E0 && utf32 <= 0x17E9) ||  // Khmer digits
        (utf32 >= 0x1810 && utf32 <= 0x1819) ||  // Mongolian digits
        (utf32 >= 0xFF10 && utf32 <= 0xFF19);    // Fullwidth digits
}

static bool ui_edit_str_is_alphanumeric(uint32_t utf32) {
    return ui_edit_str.is_letter(utf32) || ui_edit_str.is_digit(utf32);
}

static bool ui_edit_str_is_cjk_or_emoji(uint32_t utf32) {
    return !ui_edit_str_is_letter(utf32) &&
       ((utf32 >=  0x4E00 && utf32 <=  0x9FFF) || // CJK Unified Ideographs
        (utf32 >=  0x3400 && utf32 <=  0x4DBF) || // CJK Unified Ideographs Extension A
        (utf32 >= 0x20000 && utf32 <= 0x2A6DF) || // CJK Unified Ideographs Extension B
        (utf32 >= 0x2A700 && utf32 <= 0x2B73F) || // CJK Unified Ideographs Extension C
        (utf32 >= 0x2B740 && utf32 <= 0x2B81F) || // CJK Unified Ideographs Extension D
        (utf32 >= 0x2B820 && utf32 <= 0x2CEAF) || // CJK Unified Ideographs Extension E
        (utf32 >= 0x2CEB0 && utf32 <= 0x2EBEF) || // CJK Unified Ideographs Extension F
        (utf32 >=  0xF900 && utf32 <=  0xFAFF) || // CJK Compatibility Ideographs
        (utf32 >= 0x2F800 && utf32 <= 0x2FA1F) || // CJK Compatibility Ideographs Supplement
        (utf32 >= 0x1F600 && utf32 <= 0x1F64F) || // Emoticons
        (utf32 >= 0x1F300 && utf32 <= 0x1F5FF) || // Misc Symbols and Pictographs
        (utf32 >= 0x1F680 && utf32 <= 0x1F6FF) || // Transport and Map
        (utf32 >= 0x1F700 && utf32 <= 0x1F77F) || // Alchemical Symbols
        (utf32 >= 0x1F780 && utf32 <= 0x1F7FF) || // Geometric Shapes Extended
        (utf32 >= 0x1F800 && utf32 <= 0x1F8FF) || // Supplemental Arrows-C
        (utf32 >= 0x1F900 && utf32 <= 0x1F9FF) || // Supplemental Symbols and Pictographs
        (utf32 >= 0x1FA00 && utf32 <= 0x1FA6F) || // Chess Symbols
        (utf32 >= 0x1FA70 && utf32 <= 0x1FAFF) || // Symbols and Pictographs Extended-A
        (utf32 >= 0x1FB00 && utf32 <= 0x1FBFF));  // Symbols for Legacy Computing
}

static bool ui_edit_str_can_break(uint32_t cp1, uint32_t cp2) {
    return !ui_edit_str.is_zwj(cp2) &&
       (ui_edit_str.is_cjk_or_emoji(cp1) || ui_edit_str.is_cjk_or_emoji(cp2) ||
        ui_edit_str.is_punctuation(cp1)  || ui_edit_str.is_punctuation(cp2)  ||
        ui_edit_str.is_blank(cp1)        || ui_edit_str.is_blank(cp2)        ||
        ui_edit_str.is_combining(cp1)    || ui_edit_str.is_combining(cp2)    ||
        ui_edit_str.is_spacing(cp1)      || ui_edit_str.is_spacing(cp2));
}

#pragma push_macro("ui_edit_usd")
#pragma push_macro("ui_edit_gbp")
#pragma push_macro("ui_edit_euro")
#pragma push_macro("ui_edit_money_bag")
#pragma push_macro("ui_edit_pot_of_honey")
#pragma push_macro("ui_edit_gothic_hwair")

#define ui_edit_usd             "\x24"
#define ui_edit_gbp             "\xC2\xA3"
#define ui_edit_euro            "\xE2\x82\xAC"
// https://www.compart.com/en/unicode/U+1F4B0
#define ui_edit_money_bag       "\xF0\x9F\x92\xB0"
// https://www.compart.com/en/unicode/U+1F36F
#define ui_edit_pot_of_honey    "\xF0\x9F\x8D\xAF"
// https://www.compart.com/en/unicode/U+10348
#define ui_edit_gothic_hwair    "\xF0\x90\x8D\x88" // Gothic Letter Hwair

static void ui_edit_str_test_replace(void) { // exhaustive permutations
    // Exhaustive 9,765,625 replace permutations may take
    // up to 5 minutes of CPU time in release.
    // Recommended to be invoked at least once after making any
    // changes to ui_edit_str.replace and around.
    // Menu: Debug / Windows / Show Diagnostic Tools allows to watch
    //       memory pressure for whole 3 minutes making sure code is
    //       not leaking memory profusely.
    const char* gs[] = { // glyphs
        "", ui_edit_usd, ui_edit_gbp, ui_edit_euro, ui_edit_money_bag
    };
    const int32_t gb[] = {0, 1, 2, 3, 4}; // number of bytes per codepoint
    enum { n = ut_countof(gs) };
    int32_t npn = 1; // n to the power of n
    for (int32_t i = 0; i < n; i++) { npn *= n; }
    int32_t gix_src[n] = {0};
    // 5^5 = 3,125   3,125 * 3,125 = 9,765,625
    for (int32_t i = 0; i < npn; i++) {
        int32_t vi = i;
        for (int32_t j = 0; j < n; j++) {
            gix_src[j] = vi % n;
            vi /= n;
        }
        int32_t g2p[n + 1] = {0};
        int32_t ngx = 1; // next glyph index
        char src[128] = {0};
        for (int32_t j = 0; j < n; j++) {
            if (gix_src[j] > 0) {
                strcat(src, gs[gix_src[j]]);
                ut_assert(1 <= ngx && ngx <= n);
                g2p[ngx] = g2p[ngx - 1] + gb[gix_src[j]];
                ngx++;
            }
        }
        if (i % 100 == 99) {
            ut_println("%2d%% [%d][%d][%d][%d][%d] "
                    "\"%s\",\"%s\",\"%s\",\"%s\",\"%s\": \"%s\"",
                (i * 100) / npn,
                gix_src[0], gix_src[1], gix_src[2], gix_src[3], gix_src[4],
                gs[gix_src[0]], gs[gix_src[1]], gs[gix_src[2]],
                gs[gix_src[3]], gs[gix_src[4]], src);
        }
        ui_edit_str_t s = {0};
        // reference constructor does not copy to heap:
        bool ok = ui_edit_str_init(&s, src, -1, false);
        ut_swear(ok);
        for (int32_t f = 0; f <= s.g; f++) { // from
            for (int32_t t = f; t <= s.g; t++) { // to
                int32_t gix_rep[n] = {0};
                // replace range [f, t] with all possible glyphs sequences:
                for (int32_t k = 0; k < npn; k++) {
                    int32_t vk = i;
                    for (int32_t j = 0; j < n; j++) {
                        gix_rep[j] = vk % n;
                        vk /= n;
                    }
                    char rep[128] = {0};
                    for (int32_t j = 0; j < n; j++) { strcat(rep, gs[gix_rep[j]]); }
                    char e1[128] = {0}; // expected based on s.g2b[]
                    snprintf(e1, ut_countof(e1), "%.*s%s%.*s",
                        s.g2b[f], src,
                        rep,
                        s.b - s.g2b[t], src + s.g2b[t]
                    );
                    char e2[128] = {0}; // expected based on gs[]
                    snprintf(e2, ut_countof(e1), "%.*s%s%.*s",
                        g2p[f], src,
                        rep,
                        (int32_t)strlen(src) - g2p[t], src + g2p[t]
                    );
                    ut_swear(strcmp(e1, e2) == 0,
                        "s.u[%d:%d]: \"%.*s\" g:%d [%d:%d] rep=\"%s\" "
                        "e1: \"%s\" e2: \"%s\"",
                        s.b, s.c, s.b, s.u, s.g, f, t, rep, e1, e2);
                    ui_edit_str_t c = {0}; // copy
                    ok = ui_edit_str_init(&c, src, -1, true);
                    ut_swear(ok);
                    ok = ui_edit_str_replace(&c, f, t, rep, -1);
                    ut_swear(ok);
                    ut_swear(memcmp(c.u, e1, c.b) == 0,
                           "s.u[%d:%d]: \"%.*s\" g:%d [%d:%d] rep=\"%s\" "
                           "expected: \"%s\"",
                           s.b, s.c, s.b, s.u, s.g,
                           f, t, rep, e1);
                    ui_edit_str_free(&c);
                }
            }
        }
        ui_edit_str_free(&s);
    }
}

static void ui_edit_str_test_glyph_bytes(void) {
    #pragma push_macro("glyph_bytes_test")
    #define glyph_bytes_test(s, b, expectancy) \
        ut_swear(ut_str.utf8bytes(s, b) == expectancy)
    // Valid Sequences
    glyph_bytes_test("a", 1, 1);
    glyph_bytes_test(ui_edit_gbp, 2, 2);
    glyph_bytes_test(ui_edit_euro, 3, 3);
    glyph_bytes_test(ui_edit_gothic_hwair, 4, 4);
    // Invalid Continuation Bytes
    glyph_bytes_test("\xC2\x00", 2, 0);
    glyph_bytes_test("\xE0\x80\x00", 3, 0);
    glyph_bytes_test("\xF0\x80\x80\x00", 4, 0);
    // Overlong Encodings
    glyph_bytes_test("\xC0\xAF", 2, 0); // '!'
    glyph_bytes_test("\xE0\x9F\xBF", 3, 0); // upside down '?'
    glyph_bytes_test("\xF0\x80\x80\xBF", 4, 0); // '~'
    // UTF-16 Surrogates
    glyph_bytes_test("\xED\xA0\x80", 3, 0); // High surrogate
    glyph_bytes_test("\xED\xBF\xBF", 3, 0); // Low surrogate
    // Code Points Outside Valid Range
    glyph_bytes_test("\xF4\x90\x80\x80", 4, 0); // U+110000
    // Invalid Initial Bytes
    glyph_bytes_test("\xC0", 1, 0);
    glyph_bytes_test("\xC1", 1, 0);
    glyph_bytes_test("\xF5", 1, 0);
    glyph_bytes_test("\xFF", 1, 0);
    // 5-byte sequence (always invalid)
    glyph_bytes_test("\xF8\x88\x80\x80\x80", 5, 0);
    #pragma pop_macro("glyph_bytes_test")
}

static void ui_edit_str_test(void) {
    ui_edit_str_test_glyph_bytes();
    {
        ui_edit_str_t s = {0};
        bool ok = ui_edit_str_init(&s, "hello", -1, false);
        ut_swear(ok);
        ut_swear(s.b == 5 && s.c == 0 && memcmp(s.u, "hello", 5) == 0);
        ut_swear(s.g == 5 && s.g2b != null);
        for (int32_t i = 0; i <= s.g; i++) {
            ut_swear(s.g2b[i] == i);
        }
        ui_edit_str_free(&s);
    }
    const char* currencies = ui_edit_usd  ui_edit_gbp
                             ui_edit_euro ui_edit_money_bag;
    const char* money = currencies;
    {
        ui_edit_str_t s = {0};
        const int32_t n = (int32_t)strlen(currencies);
        bool ok = ui_edit_str_init(&s, money, n, true);
        ut_swear(ok);
        ut_swear(s.b == n && s.c == s.b && memcmp(s.u, money, s.b) == 0);
        ut_swear(s.g == 4 && s.g2b != null);
        const int32_t g2b[] = {0, 1, 3, 6, 10};
        for (int32_t i = 0; i <= s.g; i++) {
            ut_swear(s.g2b[i] == g2b[i]);
        }
        ui_edit_str_free(&s);
    }
    {
        ui_edit_str_t s = {0};
        bool ok = ui_edit_str_init(&s, "hello", -1, false);
        ut_swear(ok);
        ok = ui_edit_str_replace(&s, 1, 4, null, 0);
        ut_swear(ok);
        ut_swear(s.b == 2 && memcmp(s.u, "ho", 2) == 0);
        ut_swear(s.g == 2 && s.g2b[0] == 0 && s.g2b[1] == 1 && s.g2b[2] == 2);
        ui_edit_str_free(&s);
    }
    {
        ui_edit_str_t s = {0};
        bool ok = ui_edit_str_init(&s, "Hello world", -1, false);
        ut_swear(ok);
        ok = ui_edit_str_replace(&s, 5, 6, " cruel ", -1);
        ut_swear(ok);
        ok = ui_edit_str_replace(&s, 0, 5, "Goodbye", -1);
        ut_swear(ok);
        ok = ui_edit_str_replace(&s, s.g - 5, s.g, "Universe", -1);
        ut_swear(ok);
        ut_swear(s.g == 22 && s.g2b[0] == 0 && s.g2b[s.g] == s.b);
        for (int32_t i = 1; i < s.g; i++) {
            ut_swear(s.g2b[i] == i); // because every glyph is ASCII
        }
        ut_swear(memcmp(s.u, "Goodbye cruel Universe", 22) == 0);
        ui_edit_str_free(&s);
    }
    #ifdef UI_STR_TEST_REPLACE_ALL_PERMUTATIONS
        ui_edit_str_test_replace();
    #else
        (void)(void*)ui_edit_str_test_replace; // mitigate unused warning
    #endif
}

#pragma push_macro("ui_edit_gothic_hwair")
#pragma push_macro("ui_edit_pot_of_honey")
#pragma push_macro("ui_edit_money_bag")
#pragma push_macro("ui_edit_euro")
#pragma push_macro("ui_edit_gbp")
#pragma push_macro("ui_edit_usd")

#pragma pop_macro("ui_edit_str_parameters")
#pragma pop_macro("ui_edit_str_check_empty")
#pragma pop_macro("ui_edit_check_zeros")
#pragma pop_macro("ui_edit_str_check_from_to")
#pragma pop_macro("ui_edit_str_check")

#ifdef UI_EDIT_STR_TEST
    ut_static_init(ui_edit_str) { ui_edit_str.test(); }
#endif

// tests:

static void ui_edit_doc_test_big_text(void) {
    enum { MB10 = 10 * 1000 * 1000 };
    char* text = null;
    ut_heap.alloc(&text, MB10);
    memset(text, 'a', (size_t)MB10 - 1);
    char* p = text;
    uint32_t seed = 0x1;
    for (;;) {
        int32_t n = ut_num.random32(&seed) % 40 + 40;
        if (p + n >= text + MB10) { break; }
        p += n;
        *p = '\n';
    }
    text[MB10 - 1] = 0x00;
    ui_edit_text_t t = {0};
    bool ok = ui_edit_text.init(&t, text, MB10, false);
    ut_swear(ok);
    ui_edit_text.dispose(&t);
    ut_heap.free(text);
}

static void ui_edit_doc_test_paragraphs(void) {
    // ui_edit_doc_to_paragraphs() is about 1 microsecond
    for (int i = 0; i < 100; i++)
    {
        {   // empty string to paragraphs:
            ui_edit_text_t t = {0};
            bool ok = ui_edit_text.init(&t, null, 0, false);
            ut_swear(ok);
            ut_swear(t.ps != null && t.np == 1);
            ut_swear(t.ps[0].u[0] == 0 &&
                  t.ps[0].c == 0);
            ut_swear(t.ps[0].b == 0 &&
                  t.ps[0].g == 0);
            ui_edit_text.dispose(&t);
        }
        {   // string without "\n"
            const char* hello = "hello";
            const int32_t n = (int32_t)strlen(hello);
            ui_edit_text_t t = {0};
            bool ok = ui_edit_text.init(&t, hello, n, false);
            ut_swear(ok);
            ut_swear(t.ps != null && t.np == 1);
            ut_swear(t.ps[0].u == hello);
            ut_swear(t.ps[0].c == 0);
            ut_swear(t.ps[0].b == n);
            ut_swear(t.ps[0].g == n);
            ui_edit_text.dispose(&t);
        }
        {   // string with "\n" at the end
            const char* hello = "hello\n";
            ui_edit_text_t t = {0};
            bool ok = ui_edit_text.init(&t, hello, -1, false);
            ut_swear(ok);
            ut_swear(t.ps != null && t.np == 2);
            ut_swear(t.ps[0].u == hello);
            ut_swear(t.ps[0].c == 0);
            ut_swear(t.ps[0].b == 5);
            ut_swear(t.ps[0].g == 5);
            ut_swear(t.ps[1].u[0] == 0x00);
            ut_swear(t.ps[0].c == 0);
            ut_swear(t.ps[1].b == 0);
            ut_swear(t.ps[1].g == 0);
            ui_edit_text.dispose(&t);
        }
        {   // two string separated by "\n"
            const char* hello = "hello\nworld";
            const char* world = hello + 6;
            ui_edit_text_t t = {0};
            bool ok = ui_edit_text.init(&t, hello, -1, false);
            ut_swear(ok);
            ut_swear(t.ps != null && t.np == 2);
            ut_swear(t.ps[0].u == hello);
            ut_swear(t.ps[0].c == 0);
            ut_swear(t.ps[0].b == 5);
            ut_swear(t.ps[0].g == 5);
            ut_swear(t.ps[1].u == world);
            ut_swear(t.ps[0].c == 0);
            ut_swear(t.ps[1].b == 5);
            ut_swear(t.ps[1].g == 5);
            ui_edit_text.dispose(&t);
        }
    }
    for (int i = 0; i < 10; i++) {
        ui_edit_doc_test_big_text();
    }
}

typedef struct ui_edit_doc_test_notify_s {
    ui_edit_notify_t notify;
    int32_t count_before;
    int32_t count_after;
} ui_edit_doc_test_notify_t;

static void ui_edit_doc_test_before(ui_edit_notify_t* n,
        const ui_edit_notify_info_t* ut_unused(ni)) {
    ui_edit_doc_test_notify_t* notify = (ui_edit_doc_test_notify_t*)n;
    notify->count_before++;
}

static void ui_edit_doc_test_after(ui_edit_notify_t* n,
        const ui_edit_notify_info_t* ut_unused(ni)) {
    ui_edit_doc_test_notify_t* notify = (ui_edit_doc_test_notify_t*)n;
    notify->count_after++;
}

static struct {
    ui_edit_notify_t notify;
} ui_edit_doc_test_notify;


static void ui_edit_doc_test_0(void) {
    ui_edit_doc_t edit_doc = {0};
    ui_edit_doc_t* d = &edit_doc;
    ut_swear(ui_edit_doc.init(d, null, 0, false));
    ui_edit_text_t ins_text = {0};
    ut_swear(ui_edit_text.init(&ins_text, "a", 1, false));
    ui_edit_to_do_t undo = {0};
    ut_swear(ui_edit_text.replace(&d->text, null, &ins_text, &undo));
    ui_edit_doc.dispose_to_do(&undo);
    ui_edit_text.dispose(&ins_text);
    ui_edit_doc.dispose(d);
}

static void ui_edit_doc_test_1(void) {
    ui_edit_doc_t edit_doc = {0};
    ui_edit_doc_t* d = &edit_doc;
    ut_swear(ui_edit_doc.init(d, null, 0, false));
    ui_edit_text_t ins_text = {0};
    ut_swear(ui_edit_text.init(&ins_text, "a", 1, false));
    ui_edit_to_do_t undo = {0};
    ut_swear(ui_edit_text.replace(&d->text, null, &ins_text, &undo));
    ui_edit_doc.dispose_to_do(&undo);
    ui_edit_text.dispose(&ins_text);
    ui_edit_doc.dispose(d);
}

static void ui_edit_doc_test_2(void) {
    {   // two string separated by "\n"
        ui_edit_doc_t edit_doc = {0};
        ui_edit_doc_t* d = &edit_doc;
        ut_swear(ui_edit_doc.init(d, null, 0, false));
        ui_edit_notify_t notify1 = {0};
        ui_edit_notify_t notify2 = {0};
        ui_edit_doc_test_notify_t before_and_after = {0};
        before_and_after.notify.before = ui_edit_doc_test_before;
        before_and_after.notify.after  = ui_edit_doc_test_after;
        ui_edit_doc.subscribe(d, &notify1);
        ui_edit_doc.subscribe(d, &before_and_after.notify);
        ui_edit_doc.subscribe(d, &notify2);
        ut_swear(ui_edit_doc.bytes(d, null) == 0, "expected empty");
        const char* hello = "hello\nworld";
        ut_swear(ui_edit_doc.replace(d, null, hello, -1));
        ui_edit_text_t t = {0};
        ut_swear(ui_edit_doc.copy_text(d, null, &t));
        ut_swear(t.np == 2);
        ut_swear(t.ps[0].b == 5);
        ut_swear(t.ps[0].g == 5);
        ut_swear(memcmp(t.ps[0].u, "hello", 5) == 0);
        ut_swear(t.ps[1].b == 5);
        ut_swear(t.ps[1].g == 5);
        ut_swear(memcmp(t.ps[1].u, "world", 5) == 0);
        ui_edit_text.dispose(&t);
        ui_edit_doc.unsubscribe(d, &notify1);
        ui_edit_doc.unsubscribe(d, &before_and_after.notify);
        ui_edit_doc.unsubscribe(d, &notify2);
        ui_edit_doc.dispose(d);
    }
    // TODO: "GoodbyeCruelUniverse" insert 2x"\n" splitting in 3 paragraphs
    {   // three string separated by "\n"
        ui_edit_doc_t edit_doc = {0};
        ui_edit_doc_t* d = &edit_doc;
        ut_swear(ui_edit_doc.init(d, null, 0, false));
        const char* s = "Goodbye" "\n" "Cruel" "\n" "Universe";
        ut_swear(ui_edit_doc.replace(d, null, s, -1));
        ui_edit_text_t t = {0};
        ut_swear(ui_edit_doc.copy_text(d, null, &t));
        ui_edit_text.dispose(&t);
        ui_edit_range_t r = { .from = {.pn = 0, .gp = 4},
                              .to   = {.pn = 2, .gp = 3} };
        ut_swear(ui_edit_doc.replace(d, &r, null, 0));
        ut_swear(d->text.np == 1);
        ut_swear(d->text.ps[0].b == 9);
        ut_swear(d->text.ps[0].g == 9);
        ut_swear(memcmp(d->text.ps[0].u, "Goodverse", 9) == 0);
        ut_swear(ui_edit_doc.replace(d, null, null, 0)); // remove all
        ut_swear(d->text.np == 1);
        ut_swear(d->text.ps[0].b == 0);
        ut_swear(d->text.ps[0].g == 0);
        ui_edit_doc.dispose(d);
    }
    // TODO: "GoodbyeCruelUniverse" insert 2x"\n" splitting in 3 paragraphs
    {
        ui_edit_doc_t edit_doc = {0};
        ui_edit_doc_t* d = &edit_doc;
        const char* ins[] = { "X\nY", "X\n", "\nY", "\n", "X\nY\nZ" };
        for (int32_t i = 0; i < ut_countof(ins); i++) {
            ut_swear(ui_edit_doc.init(d, null, 0, false));
            const char* s = "GoodbyeCruelUniverse";
            ut_swear(ui_edit_doc.replace(d, null, s, -1));
            ui_edit_range_t r = { .from = {.pn = 0, .gp =  7},
                                  .to   = {.pn = 0, .gp = 12} };
            ui_edit_text_t ins_text = {0};
            ui_edit_text.init(&ins_text, ins[i], -1, false);
            ui_edit_to_do_t undo = {0};
            ut_swear(ui_edit_text.replace(&d->text, &r, &ins_text, &undo));
            ui_edit_to_do_t redo = {0};
            ut_swear(ui_edit_text.replace(&d->text, &undo.range, &undo.text, &redo));
            ui_edit_doc.dispose_to_do(&undo);
            undo.range = (ui_edit_range_t){0};
            ut_swear(ui_edit_text.replace(&d->text, &redo.range, &redo.text, &undo));
            ui_edit_doc.dispose_to_do(&redo);
            ui_edit_doc.dispose_to_do(&undo);
            ui_edit_text.dispose(&ins_text);
            ui_edit_doc.dispose(d);
        }
    }
}

static void ui_edit_doc_test_3(void) {
    {
        ui_edit_doc_t edit_doc = {0};
        ui_edit_doc_t* d = &edit_doc;
        ui_edit_doc_test_notify_t before_and_after = {0};
        before_and_after.notify.before = ui_edit_doc_test_before;
        before_and_after.notify.after  = ui_edit_doc_test_after;
        ut_swear(ui_edit_doc.init(d, null, 0, false));
        ut_swear(ui_edit_doc.subscribe(d, &before_and_after.notify));
        const char* s = "Goodbye Cruel Universe";
        const int32_t before = before_and_after.count_before;
        const int32_t after  = before_and_after.count_after;
        ut_swear(ui_edit_doc.replace(d, null, s, -1));
        const int32_t bytes = (int32_t)strlen(s);
        ut_swear(before + 1 == before_and_after.count_before);
        ut_swear(after  + 1 == before_and_after.count_after);
        ut_swear(d->text.np == 1);
        ut_swear(ui_edit_doc.bytes(d, null) == bytes);
        ui_edit_text_t t = {0};
        ut_swear(ui_edit_doc.copy_text(d, null, &t));
        ut_swear(t.np == 1);
        ut_swear(t.ps[0].b == bytes);
        ut_swear(t.ps[0].g == bytes);
        ut_swear(memcmp(t.ps[0].u, s, t.ps[0].b) == 0);
        // with "\n" and 0x00 at the end:
        int32_t utf8bytes = ui_edit_doc.utf8bytes(d, null);
        char* p = null;
        ut_swear(ut_heap.alloc((void**)&p, utf8bytes) == 0);
        p[utf8bytes - 1] = 0xFF;
        ui_edit_doc.copy(d, null, p, utf8bytes);
        ut_swear(p[utf8bytes - 1] == 0x00);
        ut_swear(memcmp(p, s, bytes) == 0);
        ut_heap.free(p);
        ui_edit_text.dispose(&t);
        ui_edit_doc.unsubscribe(d, &before_and_after.notify);
        ui_edit_doc.dispose(d);
    }
    {
        ui_edit_doc_t edit_doc = {0};
        ui_edit_doc_t* d = &edit_doc;
        ut_swear(ui_edit_doc.init(d, null, 0, false));
        const char* s =
            "Hello World"
            "\n"
            "Goodbye Cruel Universe";
        ut_swear(ui_edit_doc.replace(d, null, s, -1));
        ut_swear(ui_edit_doc.undo(d));
        ut_swear(ui_edit_doc.bytes(d, null) == 0);
        ut_swear(ui_edit_doc.utf8bytes(d, null) == 1);
        ut_swear(ui_edit_doc.redo(d));
        {
            int32_t utf8bytes = ui_edit_doc.utf8bytes(d, null);
            char* p = null;
            ut_swear(ut_heap.alloc((void**)&p, utf8bytes) == 0);
            p[utf8bytes - 1] = 0xFF;
            ui_edit_doc.copy(d, null, p, utf8bytes);
            ut_swear(p[utf8bytes - 1] == 0x00);
            ut_swear(memcmp(p, s, utf8bytes) == 0);
            ut_heap.free(p);
        }
        ui_edit_doc.dispose(d);
    }
}

static void ui_edit_doc_test_4(void) {
    {
        ui_edit_doc_t edit_doc = {0};
        ui_edit_doc_t* d = &edit_doc;
        ut_swear(ui_edit_doc.init(d, null, 0, false));
        ui_edit_range_t r = {0};
        r = ui_edit_text.end_range(&d->text);
        ut_swear(ui_edit_doc.replace(d, &r, "a", -1));
        r = ui_edit_text.end_range(&d->text);
        ut_swear(ui_edit_doc.replace(d, &r, "\n", -1));
        r = ui_edit_text.end_range(&d->text);
        ut_swear(ui_edit_doc.replace(d, &r, "b", -1));
        r = ui_edit_text.end_range(&d->text);
        ut_swear(ui_edit_doc.replace(d, &r, "\n", -1));
        r = ui_edit_text.end_range(&d->text);
        ut_swear(ui_edit_doc.replace(d, &r, "c", -1));
        r = ui_edit_text.end_range(&d->text);
        ut_swear(ui_edit_doc.replace(d, &r, "\n", -1));
        ui_edit_doc.dispose(d);
    }
}

static void ui_edit_doc_test(void) {
    {
        ui_edit_range_t r = { .from = {0,0}, .to = {0,0} };
        ut_static_assertion(sizeof(r.from) + sizeof(r.from) == sizeof(r.a));
        ut_swear(&r.from == &r.a[0] && &r.to == &r.a[1]);
    }
    #ifdef UI_EDIT_DOC_TEST_PARAGRAPHS
        ui_edit_doc_test_paragraphs();
    #else
        (void)(void*)ui_edit_doc_test_paragraphs; // unused
    #endif
    // use n = 10,000,000 and Diagnostic Tools to watch for memory leaks
    enum { n = 1000 };
//  enum { n = 10 * 1000 * 1000 };
    for (int32_t i = 0; i < n; i++) {
        ui_edit_doc_test_0();
        ui_edit_doc_test_1();
        ui_edit_doc_test_2();
        ui_edit_doc_test_3();
        ui_edit_doc_test_4();
    }
}

static const ui_edit_range_t ui_edit_invalid_range = {
    .from = { .pn = -1, .gp = -1},
    .to   = { .pn = -1, .gp = -1}
};

ui_edit_range_if ui_edit_range = {
    .compare       = ui_edit_range_compare,
    .order         = ui_edit_range_order,
    .is_valid      = ui_edit_range_is_valid,
    .is_empty      = ui_edit_range_is_empty,
    .uint64        = ui_edit_range_uint64,
    .pg            = ui_edit_range_pg,
    .inside        = ui_edit_range_inside_text,
    .intersect     = ui_edit_range_intersect,
    .invalid_range = &ui_edit_invalid_range
};

ui_edit_text_if ui_edit_text = {
    .init          = ui_edit_text_init,
    .bytes         = ui_edit_text_bytes,
    .all_on_null   = ui_edit_text_all_on_null,
    .ordered       = ui_edit_text_ordered,
    .end           = ui_edit_text_end,
    .end_range     = ui_edit_text_end_range,
    .dup           = ui_edit_text_dup,
    .equal         = ui_edit_text_equal,
    .copy_text     = ui_edit_text_copy_text,
    .copy          = ui_edit_text_copy,
    .replace       = ui_edit_text_replace,
    .replace_utf8  = ui_edit_text_replace_utf8,
    .dispose       = ui_edit_text_dispose
};

ui_edit_doc_if ui_edit_doc = {
    .init               = ui_edit_doc_init,
    .replace            = ui_edit_doc_replace,
    .bytes              = ui_edit_doc_bytes,
    .copy_text          = ui_edit_doc_copy_text,
    .utf8bytes          = ui_edit_doc_utf8bytes,
    .copy               = ui_edit_doc_copy,
    .redo               = ui_edit_doc_redo,
    .undo               = ui_edit_doc_undo,
    .subscribe          = ui_edit_doc_subscribe,
    .unsubscribe        = ui_edit_doc_unsubscribe,
    .dispose_to_do      = ui_edit_doc_dispose_to_do,
    .dispose            = ui_edit_doc_dispose,
    .test               = ui_edit_doc_test
};

#pragma push_macro("ui_edit_doc_dump")
#pragma push_macro("ui_edit_text_dump")
#pragma push_macro("ui_edit_range_dump")
#pragma push_macro("ui_edit_pg_dump")
#pragma push_macro("ui_edit_check_range_inside_text")
#pragma push_macro("ui_edit_check_pg_inside_text")
#pragma push_macro("ui_edit_check_zeros")

#ifdef UI_EDIT_DOC_TEST
    ut_static_init(ui_edit_doc) { ui_edit_doc.test(); }
#endif

// ______________________________ ui_edit_view.c ______________________________

/* Copyright (c) Dmitry "Leo" Kuznetsov 2021-24 see LICENSE for details */
#include "ut/ut.h"

// TODO: find all "== dt->np" it is wrong pn < dt->np fix them all
// TODO: undo/redo coalescing
// TODO: back/forward navigation
// TODO: exit (Ctrl+W?)/save(Ctrl+S, Ctrl+Shift+S) keyboard shortcuts?
// TODO: ctrl left, ctrl right jump word ctrl+shift left/right select word?
// TODO: iBeam cursor (definitely yes - see how MSVC does it)
// TODO: vertical scrollbar ui
// TODO: horizontal scroll: trivial to implement:
//       add horizontal_scroll to e->w and paint
//       paragraphs in a horizontally shifted clip

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

static void ui_edit_layout(ui_view_t* v);
static ui_point_t ui_edit_pg_to_xy(ui_edit_t* e, const ui_edit_pg_t pg);

// Glyphs in monospaced Windows fonts may have different width for non-ASCII
// characters. Thus even if edit is monospaced glyph measurements are used
// in text layout.

static void ui_edit_invalidate_view(const ui_edit_t* e) {
    ui_view.invalidate(&e->view, null);
}

static void ui_edit_invalidate_rect(const ui_edit_t* e, const ui_rect_t rc) {
    ut_assert(rc.w > 0 && rc.h > 0);
    ui_view.invalidate(&e->view, &rc);
}

static ui_rect_t ui_edit_selection_rect(ui_edit_t* e) {
    const ui_edit_range_t r = ui_edit_range.order(e->selection);
    const ui_ltrb_t i = ui_view.margins(&e->view, &e->insets);
    const ui_point_t p0 = ui_edit_pg_to_xy(e, r.from);
    const ui_point_t p1 = ui_edit_pg_to_xy(e, r.to);
    if (p0.x < 0 || p1.x < 0) { // selection outside of visible area
        return (ui_rect_t) { .x = 0, .y = 0, .w = e->w, .h = e->h };
    } else if (p0.y == p1.y) {
        const int32_t max_w = ut_max(e->fm->max_char_width, e->fm->em.w);
        int32_t w = p1.x - p0.x != 0 ?
                p1.x - p0.x + max_w : e->caret_width;
        return (ui_rect_t) { .x = p0.x, .y = i.top + p0.y,
                             .w = w, .h = e->fm->height };
    } else {
        const int32_t h = p1.y - p0.y + e->fm->height;
        return (ui_rect_t) { .x = 0, .y = i.top + p0.y,
                             .w = e->w, .h = h };
    }
}

#if 0
static void ui_edit_text_width_gp(ui_edit_t* e, const char* utf8, int32_t bytes) {
    const int32_t glyphs = ut_str.glyphs(utf8, bytes);
    ut_println("\"%.*s\" bytes:%d glyphs:%d", bytes, utf8, bytes, glyphs);
    int32_t* x = (int32_t*)ut_stackalloc((glyphs + 1) * sizeof(int32_t));
    const ui_gdi_ta_t ta = { .fm = e->fm };
    ui_wh_t wh = ui_gdi.glyphs_placement(&ta, utf8,  bytes, x, glyphs);
//  ut_println("wh: %dx%d", wh.w, wh.h);
}
#endif

static int32_t ui_edit_text_width(ui_edit_t* e, const char* s, int32_t n) {
//  if (n > 0) {
//      ui_edit_text_width_gp(e, s, n);
//  }
//  fp64_t time = ut_clock.seconds();
    // average GDI measure_text() performance per character:
    // "ui_app.fm.mono"    ~500us (microseconds)
    // "ui_app.fm.regular" ~250us (microseconds) DirectWrite ~100us
    const ui_gdi_ta_t ta = { .fm = e->fm, .color = e->color,
                             .measure = true };
    int32_t x = n == 0 ? 0 : ui_gdi.text(&ta, 0, 0, "%.*s", n, s).w;
//  TODO: remove
//  int32_t x = n == 0 ? 0 : ui_gdi.measure_text(e->fm, "%.*s", n, s).w;

//  time = (ut_clock.seconds() - time) * 1000.0;
//  static fp64_t time_sum;
//  static fp64_t length_sum;
//  time_sum += time;
//  length_sum += n;
//  ut_println("avg=%.6fms per char total %.3fms", time_sum / length_sum, time_sum);
    return x;
}

static int32_t ui_edit_word_break_at(ui_edit_t* e, int32_t pn, int32_t rn,
        const int32_t width, bool allow_zero) {
    // TODO: in sqlite.c 257,674 lines it takes 11 seconds to get all runs()
    //       on average ui_edit_word_break_at() takes 4 x ui_edit_text_width()
    //       measurements and they are slow. If we can reduce this amount
    //       (not clear how) at least 2 times it will be a win.
    //       Another way is background thread runs() processing but this is
    //       involving a lot of complexity.
    //       MSVC devenv.exe edits sqlite3.c w/o any visible delays
    int32_t count = 0; // stats logging
    int32_t chars = 0;
    ui_edit_text_t* dt = &e->doc->text; // document text
    ut_assert(0 <= pn && pn < dt->np);
    ui_edit_paragraph_t* p = &e->para[pn];
    const ui_edit_str_t* str = &dt->ps[pn];
    int32_t k = 1; // at least 1 glyph
    // offsets inside a run in glyphs and bytes from start of the paragraph:
    int32_t gp = p->run[rn].gp;
    int32_t bp = p->run[rn].bp;
    if (gp < str->g - 1) {
        const char* text = str->u + bp;
        const int32_t glyphs_in_this_run = str->g - gp;
        int32_t* g2b = &str->g2b[gp];
        // 4 is maximum number of bytes in a UTF-8 sequence
        int32_t gc = ut_min(4, glyphs_in_this_run);
        int32_t w = ui_edit_text_width(e, text, g2b[gc] - bp);
        count++;
        chars += g2b[gc] - bp;
        while (gc < glyphs_in_this_run && w < width) {
            gc = ut_min(gc * 4, glyphs_in_this_run);
            w = ui_edit_text_width(e, text, g2b[gc] - bp);
            count++;
            chars += g2b[gc] - bp;
        }
        if (w < width) {
            k = gc;
            ut_assert(1 <= k && k <= str->g - gp);
        } else {
            int32_t i = 0;
            int32_t j = gc;
            k = (i + j) / 2;
            while (i < j) {
                ut_assert(allow_zero || 1 <= k && k < gc + 1);
                const int32_t n = g2b[k + 1] - bp;
                int32_t px = ui_edit_text_width(e, text, n);
                count++;
                chars += n;
                if (px == width) { break; }
                if (px < width) { i = k + 1; } else { j = k; }
                if (!allow_zero && (i + j) / 2 == 0) { break; }
                k = (i + j) / 2;
                ut_assert(allow_zero || 1 <= k && k <= str->g - gp);
            }
        }
    }
    ut_assert(allow_zero || 1 <= k && k <= str->g - gp);
    return k;
}

static int32_t ui_edit_word_break(ui_edit_t* e, int32_t pn, int32_t rn) {
    return ui_edit_word_break_at(e, pn, rn, e->edit.w, false);
}

static int32_t ui_edit_glyph_at_x(ui_edit_t* e, int32_t pn, int32_t rn,
        int32_t x) {
    ui_edit_text_t* dt = &e->doc->text; // document text
    ut_assert(0 <= pn && pn < dt->np);
    if (x == 0 || dt->ps[pn].b == 0) {
        return 0;
    } else {
        return ui_edit_word_break_at(e, pn, rn, x + 1, true);
    }
}

static ui_edit_glyph_t ui_edit_glyph_at(ui_edit_t* e, ui_edit_pg_t p) {
    ui_edit_text_t* dt = &e->doc->text; // document text
    ui_edit_glyph_t g = { .s = "", .bytes = 0 };
    ut_assert(0 <= p.pn && p.pn < dt->np);
    const ui_edit_str_t* str = &dt->ps[p.pn];
    const int32_t bytes = str->b;
    const char* s = str->u;
    const int32_t bp = str->g2b[p.gp];
    if (bp < bytes) {
        g.s = s + bp;
        g.bytes = ut_str.utf8bytes(g.s, bytes - bp);
        ut_swear(g.bytes > 0);
    }
    return g;
}

// paragraph_runs() breaks paragraph into `runs` according to `width`

static const ui_edit_run_t* ui_edit_paragraph_runs(ui_edit_t* e, int32_t pn,
        int32_t* runs) {
//  fp64_t time = ut_clock.seconds();
    ut_assert(e->w > 0);
    ui_edit_text_t* dt = &e->doc->text; // document text
    ut_assert(0 <= pn && pn < dt->np);
    const ui_edit_run_t* r = null;
    if (e->para[pn].run != null) {
        *runs = e->para[pn].runs;
        r = e->para[pn].run;
    } else {
        ut_assert(0 <= pn && pn < dt->np);
        ui_edit_paragraph_t* p = &e->para[pn];
        const ui_edit_str_t* str = &dt->ps[pn];
        if (p->run == null) {
            ut_assert(p->runs == 0 && p->run == null);
            const int32_t max_runs = str->b + 1;
            bool ok = ut_heap.alloc((void**)&p->run, max_runs *
                                    sizeof(ui_edit_run_t)) == 0;
            ut_swear(ok);
            ui_edit_run_t* run = p->run;
            run[0].bp = 0;
            run[0].gp = 0;
            int32_t gc = str->b == 0 ? 0 : ui_edit_word_break(e, pn, 0);
            if (gc == str->g) { // whole paragraph fits into width
                p->runs = 1;
                run[0].bytes  = str->b;
                run[0].glyphs = str->g;
                int32_t pixels = ui_edit_text_width(e, str->u, str->g2b[gc]);
                run[0].pixels = pixels;
            } else {
                ut_assert(gc < str->g);
                int32_t rc = 0; // runs count
                int32_t ix = 0; // glyph index from to start of paragraph
                const char* text = str->u;
                int32_t bytes = str->b;
                while (bytes > 0) {
                    ut_assert(rc < max_runs);
                    run[rc].bp = (int32_t)(text - str->u);
                    run[rc].gp = ix;
                    int32_t glyphs = ui_edit_word_break(e, pn, rc);
                    int32_t utf8bytes = str->g2b[ix + glyphs] - run[rc].bp;
                    int32_t pixels = ui_edit_text_width(e, text, utf8bytes);
                    if (glyphs > 1 && utf8bytes < bytes && text[utf8bytes - 1] != 0x20) {
                        // try to find word break SPACE character. utf8 space is 0x20
                        int32_t i = utf8bytes;
                        while (i > 0 && text[i - 1] != 0x20) { i--; }
                        if (i > 0 && i != utf8bytes) {
                            utf8bytes = i;
                            glyphs = ut_str.glyphs(text, utf8bytes);
                            ut_assert(glyphs >= 0);
                            pixels = ui_edit_text_width(e, text, utf8bytes);
                        }
                    }
                    run[rc].bytes  = utf8bytes;
                    run[rc].glyphs = glyphs;
                    run[rc].pixels = pixels;
                    rc++;
                    text += utf8bytes;
                    ut_assert(0 <= utf8bytes && utf8bytes <= bytes);
                    bytes -= utf8bytes;
                    ix += glyphs;
                }
                ut_assert(rc > 0);
                p->runs = rc; // truncate heap capacity array:
                ok = ut_heap.realloc((void**)&p->run, rc * sizeof(ui_edit_run_t)) == 0;
                ut_swear(ok);
            }
        }
        *runs = p->runs;
        r = p->run;
    }
    ut_assert(r != null && *runs >= 1);
    return r;
}

static int32_t ui_edit_paragraph_run_count(ui_edit_t* e, int32_t pn) {
    ut_swear(e->w > 0);
    ui_edit_text_t* dt = &e->doc->text; // document text
    int32_t runs = 0;
    if (e->w > 0 && 0 <= pn && pn < dt->np) {
        (void)ui_edit_paragraph_runs(e, pn, &runs);
    }
    return runs;
}

static int32_t ui_edit_glyphs_in_paragraph(ui_edit_t* e, int32_t pn) {
    ui_edit_text_t* dt = &e->doc->text; // document text
    ut_assert(0 <= pn && pn < dt->np);
    (void)ui_edit_paragraph_run_count(e, pn); // word break into runs
    return dt->ps[pn].g;
}

static void ui_edit_create_caret(ui_edit_t* e) {
    ut_fatal_if(e->focused);
    ut_assert(ui_app.is_active());
    ut_assert(ui_app.focused());
    fp64_t px = ui_app.dpi.monitor_raw / 100.0 + 0.5;
    e->caret_width = ut_min(3, ut_max(1, (int32_t)px));
    ui_app.create_caret(e->caret_width, e->fm->height);
    e->focused = true; // means caret was created
//  ut_println("e->focused := true %s", ui_view_debug_id(&e->view));
}

static void ui_edit_destroy_caret(ui_edit_t* e) {
    ut_fatal_if(!e->focused);
    ui_app.destroy_caret();
    e->focused = false; // means caret was destroyed
//  ut_println("e->focused := false %s", ui_view_debug_id(&e->view));
}

static void ui_edit_show_caret(ui_edit_t* e) {
    if (e->focused) {
        ut_assert(ui_app.is_active());
        ut_assert(ui_app.focused());
        ut_assert((e->caret.x < 0) == (e->caret.y < 0));
        const ui_ltrb_t insets = ui_view.margins(&e->view, &e->insets);
        int32_t x = e->caret.x < 0 ? insets.left : e->caret.x;
        int32_t y = e->caret.y < 0 ? insets.top  : e->caret.y;
        ui_app.move_caret(e->x + x, e->y + y);
        // TODO: it is possible to support unblinking caret if desired
        // do not set blink time - use global default
//      fatal_if_false(SetCaretBlinkTime(500));
        ui_app.show_caret();
        e->shown++;
        ut_assert(e->shown == 1);
    }
}

static void ui_edit_hide_caret(ui_edit_t* e) {
    if (e->focused) {
        ui_app.hide_caret();
        e->shown--;
        ut_assert(e->shown == 0);
    }
}

static void ui_edit_allocate_runs(ui_edit_t* e) {
    ui_edit_text_t* dt = &e->doc->text; // document text
    ut_assert(e->para == null);
    ut_assert(dt->np > 0);
    ut_assert(e->para == null);
    bool done = ut_heap.alloc_zero((void**)&e->para,
                dt->np * sizeof(e->para[0])) == 0;
    ut_swear(done, "out of memory - cannot continue");
}

static void ui_edit_invalidate_run(ui_edit_t* e, int32_t i) {
    if (e->para[i].run != null) {
        ut_assert(e->para[i].runs > 0);
        ut_heap.free(e->para[i].run);
        e->para[i].run = null;
        e->para[i].runs = 0;
    } else {
        ut_assert(e->para[i].runs == 0);
    }
}

static void ui_edit_invalidate_runs(ui_edit_t* e, int32_t f, int32_t t,
        int32_t np) { // [from..to] inclusive inside [0..np - 1]
    ut_swear(e->para != null && f <= t && 0 <= f && t < np);
    for (int32_t i = f; i <= t; i++) { ui_edit_invalidate_run(e, i); }
}

static void ui_edit_invalidate_all_runs(ui_edit_t* e) {
    ui_edit_text_t* dt = &e->doc->text; // document text
    ui_edit_invalidate_runs(e, 0, dt->np - 1, dt->np);
}

static void ui_edit_dispose_runs(ui_edit_t* e, int32_t np) {
    ut_assert(e->para != null);
    ui_edit_invalidate_runs(e, 0, np - 1, np);
    ut_heap.free(e->para);
    e->para = null;
}

static void ui_edit_dispose_all_runs(ui_edit_t* e) {
    ui_edit_dispose_runs(e, e->doc->text.np);
}

static void ui_edit_layout_now(ui_edit_t* e) {
    if (e->measure != null && e->layout != null && e->w > 0) {
        e->layout(&e->view);
        ui_edit_invalidate_view(e);
    }
}

static void ui_edit_if_sle_layout(ui_edit_t* e) {
    // only for single line edit controls that were already initialized
    // and measured horizontally at least once.
    if (e->sle && e->layout != null && e->w > 0) {
        ui_edit_layout_now(e);
    }
}

static void ui_edit_set_font(ui_edit_t* e, ui_fm_t* f) {
    ui_edit_invalidate_all_runs(e);
    e->scroll.rn = 0;
    e->fm = f;
    ui_edit_layout_now(e);
    ui_app.request_layout();
}

// Paragraph number, glyph number -> run number

static ui_edit_pr_t ui_edit_pg_to_pr(ui_edit_t* e, const ui_edit_pg_t pg) {
    ui_edit_text_t* dt = &e->doc->text; // document text
    ut_assert(0 <= pg.pn && pg.pn < dt->np);
    const ui_edit_str_t* str = &dt->ps[pg.pn];
    ui_edit_pr_t pr = { .pn = pg.pn, .rn = -1 };
    if (str->b == 0) { // empty
        ut_assert(pg.gp == 0);
        pr.rn = 0;
    } else {
        ut_assert(0 <= pg.pn && pg.pn < dt->np);
        int32_t runs = 0;
        const ui_edit_run_t* run = ui_edit_paragraph_runs(e, pg.pn, &runs);
        if (pg.gp == str->g + 1) {
            pr.rn = runs - 1; // TODO: past last glyph ??? is this correct?
        } else {
            ut_assert(0 <= pg.gp && pg.gp <= str->g);
            for (int32_t j = 0; j < runs && pr.rn < 0; j++) {
                const int32_t last_run = j == runs - 1;
                const int32_t start = run[j].gp;
                const int32_t end = run[j].gp + run[j].glyphs + last_run;
                if (start <= pg.gp && pg.gp < end) {
                    pr.rn = j;
                }
            }
            ut_assert(pr.rn >= 0);
        }
    }
    return pr;
}

static int32_t ui_edit_runs_between(ui_edit_t* e, const ui_edit_pg_t pg0,
        const ui_edit_pg_t pg1) {
    ut_assert(ui_edit_range.uint64(pg0) <= ui_edit_range.uint64(pg1));
    int32_t rn0 = ui_edit_pg_to_pr(e, pg0).rn;
    int32_t rn1 = ui_edit_pg_to_pr(e, pg1).rn;
    int32_t rc = 0;
    if (pg0.pn == pg1.pn) {
        ut_assert(rn0 <= rn1);
        rc = rn1 - rn0;
    } else {
        ut_assert(pg0.pn < pg1.pn);
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
    // layout may decrease number of runs when view is growing:
    if (e->scroll.rn >= runs) { e->scroll.rn = runs - 1; }
    ut_assert(0 <= e->scroll.rn && e->scroll.rn < runs,
            "e->scroll.rn: %d runs: %d", e->scroll.rn, runs);
    return (ui_edit_pg_t) { .pn = e->scroll.pn, .gp = run[e->scroll.rn].gp };
}

static int32_t ui_edit_first_visible_run(ui_edit_t* e, int32_t pn) {
    return pn == e->scroll.pn ? e->scroll.rn : 0;
}

// ui_edit::pg_to_xy() paragraph # glyph # -> (x,y) in [0,0  width x height]

static ui_point_t ui_edit_pg_to_xy(ui_edit_t* e, const ui_edit_pg_t pg) {
    ui_edit_text_t* dt = &e->doc->text; // document text
    ut_assert(0 <= pg.pn && pg.pn < dt->np);
    ui_point_t pt = { .x = -1, .y = 0 };
    const int32_t spn = e->scroll.pn + e->visible_runs + 1;
    const int32_t pn = ut_min(ut_min(spn, pg.pn + 1), dt->np - 1);
    for (int32_t i = e->scroll.pn; i <= pn && pt.x < 0; i++) {
        ut_assert(0 <= i && i < dt->np);
        const ui_edit_str_t* str = &dt->ps[i];
        int32_t runs = 0;
        const ui_edit_run_t* run = ui_edit_paragraph_runs(e, i, &runs);
        for (int32_t j = ui_edit_first_visible_run(e, i); j < runs; j++) {
            const int32_t last_run = j == runs - 1;
            int32_t gc = run[j].glyphs;
            if (i == pg.pn) {
                // in the last `run` of a paragraph x after last glyph is OK
                if (run[j].gp <= pg.gp && pg.gp < run[j].gp + gc + last_run) {
                    const char* s = str->u + run[j].bp;
                    const uint32_t bp2e = str->b - run[j].bp; // to end of str
                    int32_t ofs = ui_edit_str.gp_to_bp(s, bp2e, pg.gp - run[j].gp);
                    ut_swear(ofs >= 0);
                    pt.x = ui_edit_text_width(e, s, ofs);
                    break;
                }
            }
            pt.y += e->fm->height;
        }
    }
    if (0 <= pt.x && pt.x < e->edit.w && 0 <= pt.y && pt.y < e->edit.h) {
        // all good, inside visible rectangle or right after it
    } else {
//      ut_println("%d:%d (%d,%d) outside of %dx%d", pg.pn, pg.gp,
//          pt.x, pt.y, e->edit.w, e->edit.h);
        pt = (ui_point_t){-1, -1};
    }
    return pt;
}

static int32_t ui_edit_glyph_width_px(ui_edit_t* e, const ui_edit_pg_t pg) {
    ui_edit_text_t* dt = &e->doc->text; // document text
    ut_assert(0 <= pg.pn && pg.pn < dt->np);
    const ui_edit_str_t* str = &dt->ps[pg.pn];
    const char* text = str->u;
    int32_t gc = str->g;
    if (pg.gp == 0 &&  gc == 0) {
        return 0; // empty paragraph
    } else if (pg.gp < gc) {
        const int32_t bp = ui_edit_str.gp_to_bp(text, str->b, pg.gp);
        ut_swear(bp >= 0);
        const char* s = text + bp;
        int32_t bytes_in_glyph = ut_str.utf8bytes(s, str->b - bp);
        ut_swear(bytes_in_glyph > 0);
        int32_t x = ui_edit_text_width(e, s, bytes_in_glyph);
        return x;
    } else {
        ut_assert(pg.gp == gc, "only next position past last glyph is allowed");
        return 0;
    }
}

// xy_to_pg() (x,y) (0,0, width x height) -> paragraph # glyph #

static ui_edit_pg_t ui_edit_xy_to_pg(ui_edit_t* e, int32_t x, int32_t y) {
// TODO: remove
//  const ui_ltrb_t i = ui_view.margins(&e->view, &e->view.insets);
//  ut_println("x,y: %d,%d insets left:%d right:%d", x, y, i.left, i.right);
    ui_edit_text_t* dt = &e->doc->text; // document text
    ui_edit_pg_t pg = {-1, -1};
    int32_t py = 0; // paragraph `y' coordinate
    for (int32_t i = e->scroll.pn; i < dt->np && pg.pn < 0; i++) {
        ut_assert(0 <= i && i < dt->np);
        const ui_edit_str_t* str = &dt->ps[i];
        int32_t runs = 0;
        const ui_edit_run_t* run = ui_edit_paragraph_runs(e, i, &runs);
        for (int32_t j = ui_edit_first_visible_run(e, i); j < runs && pg.pn < 0; j++) {
            const ui_edit_run_t* r = &run[j];
            const char* s = str->u + run[j].bp;
            if (py <= y && y < py + e->fm->height) {
                int32_t w = ui_edit_text_width(e, s, r->bytes);
                pg.pn = i;
                if (x >= w) {
                    pg.gp = r->gp + r->glyphs;
                } else {
                    pg.gp = r->gp + ui_edit_glyph_at_x(e, i, j, x);
// TODO: remove
//                  ut_println("pg.gp: %d r->gp: %d ui_edit_glyph_at_x(%d, %d, x:%d)",
//                          pg.gp, r->gp, i, j, x, ui_edit_glyph_at_x(e, i, j, x));
                    if (pg.gp < r->glyphs - 1) {
                        ui_edit_pg_t right = {pg.pn, pg.gp + 1};
                        int32_t x0 = ui_edit_pg_to_xy(e, pg).x;
                        int32_t x1 = ui_edit_pg_to_xy(e, right).x;
// TODO: remove
//                      ut_println("x0: %d x1: %d", x0, x1);
                        if (x1 - x < x - x0) {
                            pg.gp++; // snap to closest glyph's 'x'
                        }
                    }
                }
            } else {
                py += e->fm->height;
            }
        }
        if (py > e->h) { break; }
    }
// TODO: remove
//  ut_println("x,y: %d,%d p:d %d:%d", x, y, pg.pn, pg.gp);
    return pg;
}

static void ui_edit_set_caret(ui_edit_t* e, int32_t x, int32_t y) {
    if (e->caret.x != x || e->caret.y != y) {
        if (e->focused && ui_app.focused()) {
            ui_app.move_caret(e->x + x, e->y + y);
        }
        const ui_ltrb_t i = ui_view.margins(&e->view, &e->insets);
        // caret in i.left .. e->view.w - i.right
        //          i.top  .. e->view.h - i.bottom
        // coordinate space
        ut_swear(i.left <= x && x < e->w && i.top <= y && y < e->h);
        e->caret.x = x;
        e->caret.y = y;
// TODO: remove
//      ut_println("caret: %d, %d", x, y);
    }
}

// scroll_up() text moves up (north) in the visible view,
// scroll position increments moves down (south)

static void ui_edit_scroll_up(ui_edit_t* e, int32_t run_count) {
    ui_edit_text_t* dt = &e->doc->text; // document text
    ut_assert(0 < run_count, "does it make sense to have 0 scroll?");
    while (run_count > 0 && e->scroll.pn < dt->np - 1) {
        const ui_edit_pg_t scroll = ui_edit_scroll_pg(e);
        const ui_edit_pg_t next = (ui_edit_pg_t){
            .pn = ut_min(scroll.pn + e->visible_runs + 1, dt->np - 1),
            .gp = 0
        };
        const int32_t between = ui_edit_runs_between(e, scroll, next);
        if (between <= e->visible_runs - 1) {
            run_count = 0; // enough
        } else {
            const int32_t runs = ui_edit_paragraph_run_count(e, e->scroll.pn);
            if (e->scroll.rn < runs - 1) {
                e->scroll.rn++;
            } else if (e->scroll.pn < dt->np - 1) {
                e->scroll.pn++;
                e->scroll.rn = 0;
            }
            run_count--;
            ut_assert(e->scroll.pn >= 0 && e->scroll.rn >= 0);
        }
    }
    ui_edit_if_sle_layout(e);
    ui_edit_invalidate_view(e);
}

// scroll_dw() text moves down (south) in the visible view,
// scroll position decrements moves up (north)

static void ui_edit_scroll_down(ui_edit_t* e, int32_t run_count) {
    ut_assert(0 < run_count, "does it make sense to have 0 scroll?");
    while (run_count > 0 && (e->scroll.pn > 0 || e->scroll.rn > 0)) {
        int32_t runs = ui_edit_paragraph_run_count(e, e->scroll.pn);
        e->scroll.rn = ut_min(e->scroll.rn, runs - 1);
        if (e->scroll.rn == 0 && e->scroll.pn > 0) {
            e->scroll.pn--;
            e->scroll.rn = ui_edit_paragraph_run_count(e, e->scroll.pn) - 1;
        } else if (e->scroll.rn > 0) {
            e->scroll.rn--;
        }
        ut_assert(e->scroll.pn >= 0 && e->scroll.rn >= 0);
        ut_assert(0 <= e->scroll.rn &&
                    e->scroll.rn < ui_edit_paragraph_run_count(e, e->scroll.pn));
        run_count--;
    }
    ui_edit_if_sle_layout(e);
    ui_edit_invalidate_view(e);
}

static void ui_edit_scroll_into_view(ui_edit_t* e, const ui_edit_pg_t pg) {
    ui_edit_text_t* dt = &e->doc->text; // document text
    ut_assert(0 <= pg.pn && pg.pn < dt->np && dt->np > 0);
    if (e->inside.bottom > 0) {
        if (e->sle) { ut_assert(pg.pn == 0); }
        const int32_t rn = ui_edit_pg_to_pr(e, pg).rn;
        const uint64_t scroll = (uint64_t)e->scroll.pn << 32 | e->scroll.rn;
        const uint64_t caret  = (uint64_t)pg.pn << 32 | rn;
        uint64_t last = 0;
        int32_t py = 0;
        const int32_t pn = e->scroll.pn;
        const int32_t bottom = e->inside.bottom;
        for (int32_t i = pn; i < dt->np && py < bottom; i++) {
            int32_t runs = ui_edit_paragraph_run_count(e, i);
            const int32_t fvr = ui_edit_first_visible_run(e, i);
            for (int32_t j = fvr; j < runs && py < bottom; j++) {
                last = (uint64_t)i << 32 | j;
                py += e->fm->height;
            }
        }
        int32_t sle_runs = e->sle && e->w > 0 ?
            ui_edit_paragraph_run_count(e, 0) : 0;
        ui_edit_pg_t end = ui_edit_text.end(dt);
        ui_edit_pr_t lp = ui_edit_pg_to_pr(e, end);
        uint64_t eof = (uint64_t)(dt->np - 1) << 32 | lp.rn;
        if (last == eof && py <= bottom - e->fm->height) {
            // vertical white space for EOF on the screen
            last = (uint64_t)dt->np << 32 | 0;
        }
        if (scroll <= caret && caret < last) {
            // no scroll
        } else if (caret < scroll) {
            ui_edit_invalidate_view(e);
            e->scroll.pn = pg.pn;
            e->scroll.rn = rn;
        } else if (e->sle && sle_runs * e->fm->height <= e->h) {
            // single line edit control fits vertically - no scroll
        } else {
            ui_edit_invalidate_view(e);
            ut_assert(caret >= last);
            e->scroll.pn = pg.pn;
            e->scroll.rn = rn;
            while (e->scroll.pn > 0 || e->scroll.rn > 0) {
                ui_point_t pt = ui_edit_pg_to_xy(e, pg);
                if (pt.y + e->fm->height > bottom - e->fm->height) { break; }
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

static void ui_edit_caret_to(ui_edit_t* e, const ui_edit_pg_t to) {
    ui_edit_scroll_into_view(e, to);
    ui_point_t pt =  ui_edit_pg_to_xy(e, to);
    ui_edit_set_caret(e, pt.x + e->inside.left, pt.y + e->inside.top);
}

static void ui_edit_move_caret(ui_edit_t* e, const ui_edit_pg_t pg) {
    if (e->w > 0) { // width == 0 means no measure/layout yet
        ui_rect_t before = ui_edit_selection_rect(e);
        ui_edit_text_t* dt = &e->doc->text; // document text
        ut_assert(0 <= pg.pn && pg.pn < dt->np);
        // single line edit control cannot move caret past fist paragraph
        if (!e->sle || pg.pn < dt->np) {
            e->selection.a[1] = pg;
            ui_edit_caret_to(e, pg);
            if (!ui_app.shift && e->edit.buttons == 0) {
                e->selection.a[0] = e->selection.a[1];
            }
        }
        ui_rect_t after = ui_edit_selection_rect(e);
        ui_edit_invalidate_rect(e, ui.combine_rect(&before, &after));
    }
}

static ui_edit_pg_t ui_edit_insert_inline(ui_edit_t* e, ui_edit_pg_t pg,
        const char* text, int32_t bytes) {
    // insert_inline() inserts text (not containing '\n' in it)
    ut_assert(bytes > 0);
    for (int32_t i = 0; i < bytes; i++) { ut_assert(text[i] != '\n'); }
    ui_edit_range_t r = { .from = pg, .to = pg };
    int32_t g = 0;
    if (ui_edit_doc.replace(e->doc, &r, text, bytes)) {
        ui_edit_text_t t = {0};
        if (ui_edit_text.init(&t, text, bytes, false)) {
            ut_assert(t.ps != null && t.np == 1);
            g = t.np == 1 && t.ps != null ? t.ps[0].g : 0;
            ui_edit_text.dispose(&t);
        }
    }
    r.from.gp += g;
    r.to.gp += g;
    e->selection = r;
    ui_edit_move_caret(e, e->selection.from);
    return r.to;
}

static ui_edit_pg_t ui_edit_insert_paragraph_break(ui_edit_t* e,
        ui_edit_pg_t pg) {
    ui_edit_range_t r = { .from = pg, .to = pg };
    bool ok = ui_edit_doc.replace(e->doc, &r, "\n", 1);
    ui_edit_pg_t next = {.pn = pg.pn + 1, .gp = 0};
    return ok ? next : pg;
}

static bool ui_edit_is_blank(ui_edit_glyph_t g) {
    return g.bytes == 0 || ui_edit_str.is_blank(ut_str.utf32(g.s, g.bytes));
}

static bool ui_edit_is_punctuation(ui_edit_glyph_t g) {
    uint32_t utf32 = g.bytes > 0 ? ut_str.utf32(g.s, g.bytes) : 0;
    return utf32 != 0 && ui_edit_str.is_punctuation(utf32);
}

static bool ui_edit_is_alphanumeric(ui_edit_glyph_t g) {
    return g.bytes > 0 &&
        ui_edit_str.is_alphanumeric(ut_str.utf32(g.s, g.bytes));
}

static bool ui_edit_is_cjk_or_emoji_or_symbol(ui_edit_glyph_t g) {
    uint32_t utf32 = g.bytes > 0 ? ut_str.utf32(g.s, g.bytes) : 0;
    return utf32 != 0 &&
        (ui_edit_str.is_cjk_or_emoji(utf32) || ui_edit_str.is_symbol(utf32));
}

static bool ui_edit_is_break(ui_edit_glyph_t g) {
    uint32_t utf32 = g.bytes > 0 ? ut_str.utf32(g.s, g.bytes) : 0;
    return utf32 != 0 &&
       (ui_edit_str.is_blank(utf32) ||
        ui_edit_str.is_punctuation(utf32) ||
        ui_edit_str.is_symbol(utf32) ||
        ui_edit_str.is_cjk_or_emoji(utf32));
}

static ui_edit_glyph_t ui_edit_left_of(ui_edit_t* e, ui_edit_pg_t pg) {
    if (pg.gp > 0) {
        pg.gp--;
        return ui_edit_glyph_at(e, pg);
    } else {
        return (ui_edit_glyph_t){ null, 0 };
    }
}

static ui_edit_glyph_t ui_edit_right_of(ui_edit_t* e, ui_edit_pg_t pg) {
    ui_edit_text_t* dt = &e->doc->text; // document text
    if (pg.gp < dt->ps[pg.pn].g - 1) {
        pg.gp++;
        return ui_edit_glyph_at(e, pg);
    } else {
        return (ui_edit_glyph_t){ null, 0 };
    }
}

static ui_edit_pg_t ui_edit_skip_left_blanks(ui_edit_t* e,
    ui_edit_pg_t pg) {
    ui_edit_text_t* dt = &e->doc->text; // document text
    ut_swear(pg.pn <= dt->np - 1);
    while (pg.gp > 0) {
        pg.gp--;
        ui_edit_glyph_t glyph = ui_edit_glyph_at(e, pg);
        if (glyph.bytes > 0 && !ui_edit_is_blank(glyph)) {
            pg.gp++;
            break;
        }
    }
    return pg;
}

static ui_edit_pg_t ui_edit_skip_right_blanks(ui_edit_t* e,
    ui_edit_pg_t pg) {
    ui_edit_text_t* dt = &e->doc->text; // document text
    ut_swear(pg.pn <= dt->np - 1);
    int32_t glyphs = ui_edit_glyphs_in_paragraph(e, pg.pn);
    ui_edit_glyph_t glyph = ui_edit_glyph_at(e, pg);
    while (pg.gp < glyphs && glyph.bytes > 0 && ui_edit_is_blank(glyph)) {
        pg.gp++;
        glyph = ui_edit_glyph_at(e, pg);
    }
    return pg;
}

static ui_edit_range_t ui_edit_word_range(ui_edit_t* e, ui_edit_pg_t pg) {
    ui_edit_range_t r = { .from = pg, .to = pg };
    ui_edit_text_t* dt = &e->doc->text; // document text
    if (0 <= pg.pn && 0 <= pg.gp) {
        ut_swear(pg.pn <= dt->np - 1);
        // number of glyphs in paragraph:
        int32_t ng = ui_edit_glyphs_in_paragraph(e, pg.pn);
        if (pg.gp > ng) { pg.gp = ut_max(0, ng); }
        ui_edit_glyph_t g = ui_edit_glyph_at(e, pg);
        if (ng <= 1) {
            r.to.gp = ng;
        } else if (ui_edit_is_cjk_or_emoji_or_symbol(g)) {
            // r == {pg,pg}
        } else {
            ui_edit_pg_t from = pg;
            ui_edit_pg_t to   = pg;
            if (pg.gp > 0 && ui_edit_is_punctuation(g)) {
                from.gp--;
                g = ui_edit_glyph_at(e, from);
            } else if (pg.gp > 0 && ui_edit_is_blank(g)) {
                from.gp--;
                to.gp--;
                g = ui_edit_glyph_at(e, from);
            }
            if (ui_edit_is_blank(g)) {
                while (from.gp > 0 &&
                       ui_edit_is_blank(ui_edit_left_of(e, from))) {
                    from.gp--;
                }
                r.from = from;
                while (to.gp < ng && ui_edit_is_blank(g)) {
                    to.gp++;
                    g = ui_edit_glyph_at(e, to);
                }
                r.to = to;
            } else if (ui_edit_is_alphanumeric(g)) {
                while (from.gp > 0 &&
                       ui_edit_is_alphanumeric(ui_edit_left_of(e, from))) {
                    from.gp--;
                }
                r.from = from;
                while (to.gp < ng && ui_edit_is_alphanumeric(g)) {
                    to.gp++;
                    g = ui_edit_glyph_at(e, to);
                }
                r.to = to;
            } else {
                while (from.gp > 0 &&
                        ui_edit_is_break(ui_edit_left_of(e, from))) {
                    from.gp--;
                }
                r.from = from;
                while (to.gp < ng && ui_edit_is_break(g)) {
                    to.gp++;
                    g = ui_edit_glyph_at(e, to);
                }
                r.to = to;
            }
        }
    }
    return r;
}

static void ui_edit_ctrl_left(ui_edit_t* e) {
    ui_edit_invalidate_rect(e, ui_edit_selection_rect(e));
    const ui_edit_range_t s = e->selection;
    ui_edit_pg_t to = e->selection.to;
    if (to.gp == 0) {
        if (to.pn > 0) {
            to.pn--;
            int32_t runs = 0;
            const ui_edit_run_t* run = ui_edit_paragraph_runs(e, to.pn, &runs);
            to.gp = run[runs - 1].gp + run[runs - 1].glyphs;
        }
    } else {
        to.gp--;
    }
    const ui_edit_pg_t lf = ui_edit_skip_left_blanks(e, to);
    const ui_edit_range_t w = ui_edit_word_range(e, lf);
    e->selection.to = w.from;
    if (ui_app.shift) {
        e->selection.from = s.from;
    } else {
        e->selection.from = w.from;
    }
    ui_edit_invalidate_rect(e, ui_edit_selection_rect(e));
}

static void ui_edit_key_left(ui_edit_t* e) {
    ui_edit_pg_t to = e->selection.a[1];
    if (to.pn > 0 || to.gp > 0) {
        if (ui_app.ctrl) {
            ui_edit_ctrl_left(e);
        } else {
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
}

static void ui_edit_ctrl_right(ui_edit_t* e) {
    const ui_edit_text_t* dt = &e->doc->text; // document text
    ui_edit_range_t s = e->selection;
    ui_edit_pg_t to = e->selection.to;
    int32_t glyphs = ui_edit_glyphs_in_paragraph(e, to.pn);
    if (to.pn < dt->np - 1 || to.gp < glyphs) {
        ui_edit_invalidate_rect(e, ui_edit_selection_rect(e));
        if (to.gp == glyphs) {
            to.pn++;
            to.gp = 0;
        } else {
            to.gp++;
        }
        ui_edit_pg_t rt = ui_edit_skip_right_blanks(e, to);
        ui_edit_range_t w = ui_edit_word_range(e, rt);
        e->selection.to = w.to;
        if (ui_app.shift) {
            e->selection.from = s.from;
        } else {
            e->selection.from = w.to;
        }
        ui_edit_invalidate_rect(e, ui_edit_selection_rect(e));
    }
}

static void ui_edit_key_right(ui_edit_t* e) {
    ui_edit_text_t* dt = &e->doc->text; // document text
    ui_edit_pg_t to = e->selection.a[1];
    if (to.pn < dt->np) {
        if (ui_app.ctrl) {
            ui_edit_ctrl_right(e);
        } else {
            int32_t glyphs = ui_edit_glyphs_in_paragraph(e, to.pn);
            if (to.gp < glyphs) {
                to.gp++;
                ui_edit_scroll_into_view(e, to);
            } else if (!e->sle && to.pn < dt->np - 1) {
                to.pn++;
                to.gp = 0;
                ui_edit_scroll_into_view(e, to);
            }
            ui_edit_move_caret(e, to);
// TODO: last_x does not work!
            e->last_x = -1;
        }
    }
}

static void ui_edit_reuse_last_x(ui_edit_t* e, ui_point_t* pt) {
    // Vertical caret movement visually tend to move caret horizontally
    // in proportional font text. Remembering starting `x' value for vertical
    // movements alleviates this unpleasant UX experience to some degree.
    if (pt->x > 0) {
        if (e->last_x > 0) {
            int32_t prev = e->last_x - e->fm->em.w;
            int32_t next = e->last_x + e->fm->em.w;
            if (prev <= pt->x && pt->x <= next) {
                pt->x = e->last_x;
            }
        }
        e->last_x = pt->x;
    }
}

static void ui_edit_key_up(ui_edit_t* e) {
    const ui_edit_pg_t pg = e->selection.a[1];
    ui_edit_pg_t to = pg;
    if (to.pn > 0 || ui_edit_pg_to_pr(e, to).rn > 0) {
        // top of the text
        ui_point_t pt = ui_edit_pg_to_xy(e, to);
        if (pt.y == 0) {
            ui_edit_scroll_down(e, 1);
        } else {
            pt.y -= 1;
        }
        ui_edit_reuse_last_x(e, &pt);
        ut_assert(pt.y >= 0);
        to = ui_edit_xy_to_pg(e, pt.x, pt.y);
        if (to.pn >= 0 && to.gp >= 0) {
            int32_t rn0 = ui_edit_pg_to_pr(e, pg).rn;
            int32_t rn1 = ui_edit_pg_to_pr(e, to).rn;
            if (rn1 > 0 && rn0 == rn1) { // same run
                ut_assert(to.gp > 0, "word break must not break on zero gp");
                int32_t runs = 0;
                const ui_edit_run_t* run = ui_edit_paragraph_runs(e, to.pn, &runs);
                to.gp = run[rn1].gp;
            }
        }
    }
    if (to.pn >= 0 && to.gp >= 0) {
        ui_edit_move_caret(e, to);
    }
}

static void ui_edit_key_down(ui_edit_t* e) {
    const ui_edit_pg_t pg = e->selection.a[1];
    ui_point_t pt = ui_edit_pg_to_xy(e, pg);
    ui_edit_reuse_last_x(e, &pt);
    // scroll runs guaranteed to be already layout for current state of view:
    ui_edit_pg_t scroll = ui_edit_scroll_pg(e);
    const int32_t run_count = ui_edit_runs_between(e, scroll, pg);
    if (!e->sle && run_count >= e->visible_runs - 1) {
        ui_edit_scroll_up(e, 1);
    } else {
        pt.y += e->fm->height;
    }
    ui_edit_pg_t to = ui_edit_xy_to_pg(e, pt.x, pt.y);
    if (to.pn >= 0 && to.gp >= 0) {
        ui_edit_move_caret(e, to);
    }
}

static void ui_edit_key_home(ui_edit_t* e) {
    if (ui_app.ctrl) {
        e->scroll.pn = 0;
        e->scroll.rn = 0;
        e->selection.a[1].pn = 0;
        e->selection.a[1].gp = 0;
        ui_edit_invalidate_view(e);
    }
    const int32_t pn = e->selection.a[1].pn;
    int32_t runs = ui_edit_paragraph_run_count(e, pn);
    const ui_edit_paragraph_t* para = &e->para[pn];
    if (runs <= 1) {
        e->selection.a[1].gp = 0;
    } else {
        int32_t rn = ui_edit_pg_to_pr(e, e->selection.a[1]).rn;
        ut_assert(0 <= rn && rn < runs);
        const int32_t gp = para->run[rn].gp;
        if (e->selection.a[1].gp != gp) {
            // first Home keystroke moves caret to start of run
            e->selection.a[1].gp = gp;
        } else {
            // second Home keystroke moves caret start of paragraph
            e->selection.a[1].gp = 0;
            if (e->scroll.pn >= e->selection.a[1].pn) { // scroll in
                e->scroll.pn = e->selection.a[1].pn;
                e->scroll.rn = 0;
                ui_edit_invalidate_view(e);
            }
        }
    }
    if (!ui_app.shift) {
        e->selection.a[0] = e->selection.a[1];
    }
    ui_edit_move_caret(e, e->selection.a[1]);
}

static void ui_edit_key_eol(ui_edit_t* e) {
    const ui_edit_text_t* dt = &e->doc->text; // document text
    int32_t pn = e->selection.a[1].pn;
    int32_t gp = e->selection.a[1].gp;
    ut_assert(0 <= pn && pn < dt->np);
    const ui_edit_str_t* str = &dt->ps[pn];
    int32_t runs = 0;
    const ui_edit_run_t* run = ui_edit_paragraph_runs(e, pn, &runs);
    int32_t rn = ui_edit_pg_to_pr(e, e->selection.a[1]).rn;
    ut_assert(0 <= rn && rn < runs);
    if (rn == runs - 1) {
        e->selection.a[1].gp = str->g;
    } else if (e->selection.a[1].gp == str->g) {
        // at the end of paragraph do nothing (or move caret to EOF?)
    } else if (str->g > 0 && gp != run[rn].glyphs - 1) {
        e->selection.a[1].gp = run[rn].gp + run[rn].glyphs - 1;
    } else {
        e->selection.a[1].gp = str->g;
    }
}

static void ui_edit_key_end(ui_edit_t* e) {
    const ui_edit_text_t* dt = &e->doc->text; // document text
    if (ui_app.ctrl) {
        int32_t py = e->inside.bottom;
        for (int32_t i = dt->np - 1; i >= 0 && py >= e->fm->height; i--) {
            int32_t runs = ui_edit_paragraph_run_count(e, i);
            for (int32_t j = runs - 1; j >= 0 && py >= e->fm->height; j--) {
                py -= e->fm->height;
                if (py < e->fm->height) {
                    e->scroll.pn = i;
                    e->scroll.rn = j;
                }
            }
        }
        e->selection.a[1] = ui_edit_text.end(dt);
        ui_edit_invalidate_view(e);
    } else {
        ui_edit_key_eol(e);
    }
    if (!ui_app.shift) {
        e->selection.a[0] = e->selection.a[1];
    }
    ui_edit_move_caret(e, e->selection.a[1]);
}

static void ui_edit_key_page_up(ui_edit_t* e) {
    int32_t n = ut_max(1, e->visible_runs - 1);
    ui_edit_pg_t scr = ui_edit_scroll_pg(e);
    const ui_edit_pg_t prev = (ui_edit_pg_t){
        .pn = ut_max(scr.pn - e->visible_runs - 1, 0),
        .gp = 0
    };
    const int32_t m = ui_edit_runs_between(e, prev, scr);
    if (m > n) {
        ui_point_t pt = ui_edit_pg_to_xy(e, e->selection.a[1]);
        ui_edit_pr_t scroll = e->scroll;
        ui_edit_scroll_down(e, n);
        if (scroll.pn != e->scroll.pn || scroll.rn != e->scroll.rn) {
            ui_edit_pg_t pg = ui_edit_xy_to_pg(e, pt.x, pt.y);
            ui_edit_move_caret(e, pg);
        }
    } else {
        const ui_edit_pg_t bof = {.pn = 0, .gp = 0};
        ui_edit_move_caret(e, bof);
    }
}

static void ui_edit_key_page_down(ui_edit_t* e) {
    const ui_edit_text_t* dt = &e->doc->text; // document text
    const int32_t n = ut_max(1, e->visible_runs - 1);
    const ui_edit_pg_t scr = ui_edit_scroll_pg(e);
    const ui_edit_pg_t next = (ui_edit_pg_t){
        .pn = ut_min(scr.pn + e->visible_runs + 1, dt->np - 1),
        .gp = 0
    };
    const int32_t m = ui_edit_runs_between(e, scr, next);
    if (m > n) {
        const ui_point_t pt = ui_edit_pg_to_xy(e, e->selection.a[1]);
        const ui_edit_pr_t scroll = e->scroll;
        ui_edit_scroll_up(e, n);
        if (scroll.pn != e->scroll.pn || scroll.rn != e->scroll.rn) {
            ui_edit_pg_t pg = ui_edit_xy_to_pg(e, pt.x, pt.y);
            ui_edit_move_caret(e, pg);
        }
    } else {
        const ui_edit_pg_t end = ui_edit_text.end(dt);
        ui_edit_move_caret(e, end);
    }
}

static void ui_edit_key_delete(ui_edit_t* e) {
    ui_edit_text_t* dt = &e->doc->text; // document text
    uint64_t f = ui_edit_range.uint64(e->selection.a[0]);
    uint64_t t = ui_edit_range.uint64(e->selection.a[1]);
    uint64_t end = ui_edit_range.uint64(ui_edit_text.end(dt));
    if (f == t && t != end) {
        ui_edit_pg_t s1 = e->selection.a[1];
        ui_edit.key_right(e);
        e->selection.a[1] = s1;
    }
    ui_edit.erase(e);
}

static void ui_edit_key_backspace(ui_edit_t* e) {
    uint64_t f = ui_edit_range.uint64(e->selection.a[0]);
    uint64_t t = ui_edit_range.uint64(e->selection.a[1]);
    if (t != 0 && f == t) {
        ui_edit_pg_t s1 = e->selection.a[1];
        ui_edit.key_left(e);
        e->selection.a[1] = s1;
    }
    ui_edit.erase(e);
}

static void ui_edit_key_enter(ui_edit_t* e) {
    ut_assert(!e->ro);
    if (!e->sle) {
        ui_edit.erase(e);
        e->selection.a[1] = ui_edit_insert_paragraph_break(e, e->selection.a[1]);
        e->selection.a[0] = e->selection.a[1];
        ui_edit_move_caret(e, e->selection.a[1]);
    } else { // single line edit callback
        if (ui_edit.enter != null) { ui_edit.enter(e); }
    }
}

static bool ui_edit_key_pressed(ui_view_t* v, int64_t key) {
    bool swallow = false;
    ut_assert(v->type == ui_view_text);
    ui_edit_t* e = (ui_edit_t*)v;
    ui_edit_text_t* dt = &e->doc->text; // document text
    if (e->focused) {
        swallow = true;
        if (key == ui.key.down && e->selection.a[1].pn < dt->np) {
            ui_edit.key_down(e);
        } else if (key == ui.key.up && dt->np > 1) {
            ui_edit.key_up(e);
        } else if (key == ui.key.left) {
            ui_edit.key_left(e);
        } else if (key == ui.key.right) {
            ui_edit.key_right(e);
        } else if (key == ui.key.pageup) {
            ui_edit.key_page_up(e);
        } else if (key == ui.key.pagedw) {
            ui_edit.key_page_down(e);
        } else if (key == ui.key.home) {
            ui_edit.key_home(e);
        } else if (key == ui.key.end) {
            ui_edit.key_end(e);
        } else if (key == ui.key.del && !e->ro) {
            ui_edit.key_delete(e);
        } else if (key == ui.key.back && !e->ro) {
            ui_edit.key_backspace(e);
        } else if (key == ui.key.enter && !e->ro) {
            ui_edit.key_enter(e);
        } else {
            swallow = false; // ignore other keys
        }
    }
    return swallow;
}

static void ui_edit_undo(ui_edit_t* e) {
    if (e->doc->undo != null) {
        ui_edit_doc.undo(e->doc);
    } else {
        ui_app.beep(ui.beep.error);
    }
}
static void ui_edit_redo(ui_edit_t* e) {
    if (e->doc->redo != null) {
        ui_edit_doc.redo(e->doc);
    } else {
        ui_app.beep(ui.beep.error);
    }
}

static void ui_edit_character(ui_view_t* v, const char* utf8) {
    ut_assert(v->type == ui_view_text);
    ut_assert(!ui_view.is_hidden(v) && !ui_view.is_disabled(v));
    #pragma push_macro("ui_edit_ctrl")
    #define ui_edit_ctrl(c) ((char)((c) - 'a' + 1))
    ui_edit_t* e = (ui_edit_t*)v;
    if (e->focused) {
        char ch = utf8[0];
        if (ui_app.ctrl) {
            if (ch == ui_edit_ctrl('a')) { ui_edit.select_all(e); }
            if (ch == ui_edit_ctrl('c')) { ui_edit.copy_to_clipboard(e); }
            if (!e->ro) {
                if (ch == ui_edit_ctrl('x')) { ui_edit.cut_to_clipboard(e); }
                if (ch == ui_edit_ctrl('v')) { ui_edit.paste_from_clipboard(e); }
                if (ch == ui_edit_ctrl('y')) { ui_edit_redo(e); }
                if (ch == ui_edit_ctrl('z') || ch == ui_edit_ctrl('Z')) {
                    if (ui_app.shift) { // Ctrl+Shift+Z
                        ui_edit_redo(e);
                    } else { // Ctrl+Z
                        ui_edit_undo(e);
                    }
                }
            }
        }
        if (0x20u <= (uint8_t)ch && !e->ro) { // 0x20 space
            int32_t len = (int32_t)strlen(utf8);
            int32_t bytes = ut_str.utf8bytes(utf8, len);
            if (bytes > 0) {
                ui_edit.erase(e); // remove selected text to be replaced by glyph
                e->selection.a[1] = ui_edit_insert_inline(e,
                    e->selection.a[1], utf8, bytes);
                e->selection.a[0] = e->selection.a[1];
                ui_edit_move_caret(e, e->selection.a[1]);
            } else {
                ut_println("invalid UTF8: 0x%02X%02X%02X%02X",
                        utf8[0], utf8[1], utf8[2], utf8[3]);
            }
        }
    }
    #pragma pop_macro("ui_edit_ctrl")
}

static void ui_edit_select_word(ui_edit_t* e, int32_t x, int32_t y) {
    ui_edit_invalidate_rect(e, ui_edit_selection_rect(e));
    ui_edit_pg_t pg = ui_edit_xy_to_pg(e, x, y);
    if (0 <= pg.pn && 0 <= pg.gp) {
        ui_edit_range_t r = ui_edit_word_range(e, pg);
        int32_t glyphs = ui_edit_glyphs_in_paragraph(e, r.to.pn);
        if (r.to.pn == r.from.pn && r.to.gp == r.from.gp && r.to.gp < glyphs) {
            r.to.gp++; // at least one glyph to the right
        }
        if (ui_edit_range.compare(r.from, pg) != 0 ||
            ui_edit_range.compare(r.to, pg) != 0) {
            e->selection = r;
            ui_edit_caret_to(e, r.to);
//          ut_println("e->selection.a[1] = %d.%d", to.pn, to.gp);
            ui_edit_invalidate_rect(e, ui_edit_selection_rect(e));
            e->edit.buttons = 0;
        }
    }
}

static void ui_edit_select_paragraph(ui_edit_t* e, int32_t x, int32_t y) {
    ui_edit_invalidate_rect(e, ui_edit_selection_rect(e));
    ui_edit_text_t* dt = &e->doc->text; // document text
    ui_edit_pg_t p = ui_edit_xy_to_pg(e, x, y);
    if (0 <= p.pn && 0 <= p.gp) {
        ui_edit_range_t r = ui_edit_text.ordered(dt, &e->selection);
        int32_t glyphs = ui_edit_glyphs_in_paragraph(e, p.pn);
        if (p.gp > glyphs) { p.gp = ut_max(0, glyphs); }
        if (p.pn == r.a[0].pn && r.a[0].pn == r.a[1].pn &&
            r.a[0].gp <= p.gp && p.gp <= r.a[1].gp) {
            r.a[0].gp = 0;
            if (p.pn < dt->np - 1) {
                r.a[1].pn = p.pn + 1;
                r.a[1].gp = 0;
            } else {
                r.a[1].gp = dt->ps[p.pn].g;
            }
            e->selection = r;
            ui_edit_caret_to(e, r.to);
        }
        ui_edit_invalidate_rect(e, ui_edit_selection_rect(e));
        e->edit.buttons = 0;
    }
}

static void ui_edit_click(ui_edit_t* e, int32_t x, int32_t y) {
    // x, y in 0..e->w, 0->e.h coordinate space
    ut_assert(0 <= x && x < e->w && 0 <= y && y < e->h);
    ui_edit_text_t* dt = &e->doc->text; // document text
    ui_edit_pg_t pg = ui_edit_xy_to_pg(e, x, y);
//  TODO: remove
//  ut_println("x,y: %d,%d p:d %d:%d", e->caret.x, e->caret.y, pg.pn, pg.gp);
    if (0 <= pg.pn && 0 <= pg.gp && ui_view.has_focus(&e->view)) {
        ut_swear(dt->np > 0 && pg.pn < dt->np);
        int32_t glyphs = ui_edit_glyphs_in_paragraph(e, pg.pn);
        if (pg.gp > glyphs) { pg.gp = ut_max(0, glyphs); }
//      ut_println("move_caret: %d.%d", p.pn, p.gp);
        ui_edit_move_caret(e, pg);
    }
}

static void ui_edit_mouse_button_down(ui_edit_t* e, int32_t ix) {
    e->edit.buttons |= (1 << ix);
}

static void ui_edit_mouse_button_up(ui_edit_t* e, int32_t ix) {
    e->edit.buttons &= ~(1 << ix);
}

static bool ui_edit_tap(ui_view_t* v, int32_t ut_unused(ix), bool pressed) {
    // `ix` ignored for now till context menu (copy/paste/select...)
    ui_edit_t* e = (ui_edit_t*)v;
    ut_assert(ui_view.inside(v, &ui_app.mouse));
    const int32_t x = ui_app.mouse.x - (v->x + e->inside.left);
    const int32_t y = ui_app.mouse.y - (v->y + e->inside.top);
    bool inside = 0 <= x && x < e->w && 0 <= y && y < e->h;
//  TODO: remove
//  ut_println("mouse: %d,%d x,y: %d %d inside: %d", ui_app.mouse.x, ui_app.mouse.y, x, y, inside);
    if (inside) {
        if (pressed) {
            e->edit.buttons = 0;
            ui_edit_click(e, x, y);
            ui_edit_mouse_button_down(e, ix);
        } else if (!pressed) {
            ui_edit_mouse_button_up(e, ix);
        }
    }
    return true;
}

static bool ui_edit_long_press(ui_view_t* v, int32_t ut_unused(ix)) {
    ui_edit_t* e = (ui_edit_t*)v;
    const int32_t x = ui_app.mouse.x - (v->x + e->inside.left);
    const int32_t y = ui_app.mouse.y - (v->y + e->inside.top);
    bool inside = 0 <= x && x < e->w && 0 <= y && y < e->h;
    if (inside && ui_edit_range.is_empty(e->selection)) {
        ui_edit_select_paragraph(e, x, y);
    }
    return true;
}

static bool ui_edit_double_tap(ui_view_t* v, int32_t ut_unused(ix)) {
    ui_edit_t* e = (ui_edit_t*)v;
    const int32_t x = ui_app.mouse.x - (v->x + e->inside.left);
    const int32_t y = ui_app.mouse.y - (v->y + e->inside.top);
    bool inside = 0 <= x && x < e->w && 0 <= y && y < e->h;
    if (inside && e->selection.a[0].pn == e->selection.a[1].pn) {
        ui_edit_select_word(e, x, y);
    }
    return false;
}

static void ui_edit_mouse_scroll(ui_view_t* v, ui_point_t dx_dy) {
    if (v->w > 0 && v->h > 0) {
        const int32_t dy = dx_dy.y;
        // TODO: maybe make a use of dx in single line no-word-break edit control?
        if (ui_app.focus == v) {
            ut_assert(v->type == ui_view_text);
            ui_edit_t* e = (ui_edit_t*)v;
            int32_t lines = (abs(dy) + v->fm->height - 1) / v->fm->height;
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
            const int32_t x = e->caret.x - e->inside.left;
            const int32_t y = e->caret.y - e->inside.top;
            ui_edit_pg_t pg = ui_edit_xy_to_pg(e, x, y);
// TODO: remove
//          ut_println("x,y: %d,%d caret: %d,%d p:d %d:%d", x, y, e->caret.x, e->caret.y, pg.pn, pg.gp);
            if (pg.pn >= 0 && pg.gp >= 0) {
                ut_assert(pg.gp <= e->doc->text.ps[pg.pn].g);
                ui_edit_move_caret(e, pg);
            } else {
                ui_edit_click(e, x, y);
            }
        }
    }
}

static bool ui_edit_focus_gained(ui_view_t* v) {
    ut_assert(v->type == ui_view_text);
    ui_edit_t* e = (ui_edit_t*)v;
    ut_assert(v->focusable);
    if (ui_app.focused() && !e->focused) {
        ui_edit_create_caret(e);
        ui_edit_show_caret(e);
        ui_edit_if_sle_layout(e);
    }
    e->edit.buttons = 0;
    ui_app.request_redraw();
    return true;
}

static void ui_edit_focus_lost(ui_view_t* v) {
    ut_assert(v->type == ui_view_text);
    ui_edit_t* e = (ui_edit_t*)v;
    if (e->focused) {
        ui_edit_hide_caret(e);
        ui_edit_destroy_caret(e);
        ui_edit_if_sle_layout(e);
    }
    e->edit.buttons = 0;
    ui_app.request_redraw();
}

static void ui_edit_erase(ui_edit_t* e) {
    if (e->selection.from.pn != e->selection.to.pn) {
        ui_edit_invalidate_view(e);
    } else {
        ui_edit_invalidate_rect(e, ui_edit_selection_rect(e));
    }
    ui_edit_range_t r = ui_edit_range.order(e->selection);
    if (!ui_edit_range.is_empty(r) && ui_edit_doc.replace(e->doc, &r, null, 0)) {
        e->selection = r;
        e->selection.to = e->selection.from;
        ui_edit_move_caret(e, e->selection.from);
    }
}

static void ui_edit_select_all(ui_edit_t* e) {
    e->selection = ui_edit_text.all_on_null(&e->doc->text, null);
    ui_edit_invalidate_view(e);
}

static int32_t ui_edit_save(ui_edit_t* e, char* text, int32_t* bytes) {
    ut_not_null(bytes);
    enum {
        error_insufficient_buffer = 122, // ERROR_INSUFFICIENT_BUFFER
        error_more_data = 234            // ERROR_MORE_DATA
    };
    int32_t r = 0;
    const int32_t utf8bytes = ui_edit_doc.utf8bytes(e->doc, null);
    if (text == null) {
        *bytes = utf8bytes;
        r = ut_runtime.error.more_data;
    } else if (*bytes < utf8bytes) {
        r = ut_runtime.error.insufficient_buffer;
    } else {
        ui_edit_doc.copy(e->doc, null, text, utf8bytes);
        ut_assert(text[utf8bytes - 1] == 0x00);
    }
    return r;
}

static void ui_edit_clipboard_copy(ui_edit_t* e) {
    int32_t utf8bytes = ui_edit_doc.utf8bytes(e->doc, &e->selection);
    if (utf8bytes > 0) {
        char* text = null;
        bool ok = ut_heap.alloc((void**)&text, utf8bytes) == 0;
        ut_swear(ok);
        ui_edit_doc.copy(e->doc, &e->selection, text, utf8bytes);
        ut_assert(text[utf8bytes - 1] == 0x00); // verify zero termination
        ut_clipboard.put_text(text);
        ut_heap.free(text);
        static ui_label_t hint = ui_label(0.0f, "copied to clipboard");
        int32_t x = e->x + e->caret.x;
        int32_t y = e->y + e->caret.y - e->fm->height;
        if (y < ui_app.content->y) {
            y += e->fm->height * 2;
        }
        if (y > ui_app.content->y + ui_app.content->h - e->fm->height) {
            y = e->caret.y;
        }
        ui_app.show_hint(&hint, x, y, 0.5);
    }
}

static void ui_edit_clipboard_cut(ui_edit_t* e) {
    int32_t utf8bytes = ui_edit_doc.utf8bytes(e->doc, &e->selection);
    if (utf8bytes > 0) { ui_edit_clipboard_copy(e); }
    if (!e->ro) { ui_edit.erase(e); }
}

static ui_edit_pg_t ui_edit_paste_text(ui_edit_t* e,
        const char* text, int32_t bytes) {
    ut_assert(!e->ro);
    ui_edit_text_t t = {0};
    ui_edit_text.init(&t, text, bytes, false);
    ui_edit_range_t r = ui_edit_text.all_on_null(&t, null);
    ui_edit_doc.replace(e->doc, &e->selection, text, bytes);
    ui_edit_pg_t pg = e->selection.from;
    pg.pn += r.to.pn;
    if (e->selection.from.pn == e->selection.to.pn && r.to.pn == 0) {
        pg.gp = e->selection.from.gp + r.to.gp;
    } else {
        pg.gp = r.to.gp;
    }
    ui_edit_text.dispose(&t);
    return pg;
}

static void ui_edit_paste(ui_edit_t* e, const char* s, int32_t n) {
    if (!e->ro) {
        if (n < 0) { n = (int32_t)strlen(s); }
        ui_edit.erase(e);
        e->selection.a[1] = ui_edit_paste_text(e, s, n);
        e->selection.a[0] = e->selection.a[1];
        if (e->w > 0) { ui_edit_move_caret(e, e->selection.a[1]); }
    }
}

static void ui_edit_clipboard_paste(ui_edit_t* e) {
    if (!e->ro) {
        ui_edit_pg_t pg = e->selection.a[1];
        int32_t bytes = 0;
        ut_clipboard.get_text(null, &bytes);
        if (bytes > 0) {
            char* text = null;
            bool ok = ut_heap.alloc((void**)&text, bytes) == 0;
            ut_swear(ok);
            int32_t r = ut_clipboard.get_text(text, &bytes);
            ut_fatal_if_error(r);
            if (bytes > 0 && text[bytes - 1] == 0) {
                bytes--; // clipboard includes zero terminator
            }
            if (bytes > 0) {
                ui_edit.erase(e);
                pg = ui_edit_paste_text(e, text, bytes);
                ui_edit_move_caret(e, pg);
            }
            ut_heap.free(text);
        }
    }
}

static void ui_edit_prepare_sle(ui_edit_t* e) {
    ui_view_t* v = &e->view;
    ut_swear(e->sle && v->w > 0);
    // shingle line edit is capable of resizing itself to two
    // lines of text (and shrinking back) to avoid horizontal scroll
    int32_t runs = ut_max(1, ut_min(2, ui_edit_paragraph_run_count(e, 0)));
    const ui_ltrb_t insets = ui_view.margins(v, &v->insets);
    int32_t h = insets.top + v->fm->height * runs + insets.bottom;
    fp32_t min_h_em = (fp32_t)h / v->fm->em.h;
    if (v->min_h_em != min_h_em) {
        v->min_h_em = min_h_em;
    }
}

static void ui_edit_insets(ui_edit_t* e) {
    ui_view_t* v = &e->view;
    const ui_ltrb_t insets = ui_view.margins(v, &v->insets);
    e->inside = (ui_ltrb_t){
        .left   = insets.left,
        .top    = insets.top,
        .right  = v->w - insets.right,
        .bottom = v->h - insets.bottom
    };
    const int32_t width = e->edit.w; // previous width
    e->edit.w = e->inside.right  - e->inside.left;
    e->edit.h = e->inside.bottom - e->inside.top;
    if (e->edit.w != width) { ui_edit_invalidate_all_runs(e); }
}

static void ui_edit_measure(ui_view_t* v) { // bottom up
    ut_assert(v->type == ui_view_text);
    ui_edit_t* e = (ui_edit_t*)v;
    if (v->w > 0 && e->sle) { ui_edit_prepare_sle(e); }
    v->w = (int32_t)((fp64_t)v->fm->em.w * (fp64_t)v->min_w_em + 0.5);
    v->h = (int32_t)((fp64_t)v->fm->em.h * (fp64_t)v->min_h_em + 0.5);
    const ui_ltrb_t i = ui_view.margins(v, &v->insets);
    // enforce minimum size - it makes it checking corner cases much simpler
    // and it's hard to edit anything in a smaller area - will result in bad UX
    if (v->w < v->fm->em.w * 4) { v->w = i.left + v->fm->em.w * 4 + i.right; }
    if (v->h < v->fm->height)   { v->h = i.top + v->fm->height + i.bottom; }
}

static void ui_edit_layout(ui_view_t* v) { // top down
    ut_assert(v->type == ui_view_text);
    ut_assert(v->w > 0 && v->h > 0); // could be `if'
    ui_edit_t* e = (ui_edit_t*)v;
    ui_edit_insets(e);
    // fully visible runs
    e->visible_runs = e->h / e->fm->height;
    ui_edit_invalidate_run(e, e->scroll.pn);
    // number of runs in e->scroll.pn may have changed with e->w change
    int32_t runs = ui_edit_paragraph_run_count(e, e->scroll.pn);
    // glyph position in scroll_pn paragraph:
    const ui_edit_pg_t scroll = v->w == 0 ? (ui_edit_pg_t){0, 0} :
                                            ui_edit_scroll_pg(e);
    e->scroll.rn = ui_edit_pg_to_pr(e, scroll).rn;
    ut_assert(0 <= e->scroll.rn && e->scroll.rn < runs); (void)runs;
    if (e->sle) { // single line edit (if changed on the fly):
        e->selection.a[0].pn = 0; // only has single paragraph
        e->selection.a[1].pn = 0;
        // scroll line on top of current cursor position into view
        const ui_edit_run_t* run = ui_edit_paragraph_runs(e, 0, &runs);
        if (runs <= 2 && e->scroll.rn == 1) {
            ui_edit_pg_t top = scroll;
            top.gp = ut_max(0, top.gp - run[e->scroll.rn].glyphs - 1);
            ui_edit_scroll_into_view(e, top);
        }
    }
    ui_edit_scroll_into_view(e, e->selection.a[1]);
    ui_edit_caret_to(e, e->selection.a[1]);
    if (e->focused) {
        // recreate caret because fm->height may have changed
        ui_edit_hide_caret(e);
        ui_edit_destroy_caret(e);
        ui_edit_create_caret(e);
        ui_edit_show_caret(e);
        ut_assert(e->focused);
    }
}

static void ui_edit_paint_selection(ui_edit_t* e, int32_t y, const ui_edit_run_t* r,
        const char* text, int32_t pn, int32_t c0, int32_t c1) {
    uint64_t s0 = ui_edit_range.uint64(e->selection.a[0]);
    uint64_t e0 = ui_edit_range.uint64(e->selection.a[1]);
    if (s0 > e0) {
        uint64_t swap = e0;
        e0 = s0;
        s0 = swap;
    }
    const ui_edit_pg_t pnc0 = {.pn = pn, .gp = c0};
    const ui_edit_pg_t pnc1 = {.pn = pn, .gp = c1};
    uint64_t s1 = ui_edit_range.uint64(pnc0);
    uint64_t e1 = ui_edit_range.uint64(pnc1);
    if (s0 <= e1 && s1 <= e0) {
        uint64_t start = ut_max(s0, s1) - (uint64_t)c0;
        uint64_t end = ut_min(e0, e1) - (uint64_t)c0;
        if (start < end) {
            int32_t fro = (int32_t)start;
            int32_t to  = (int32_t)end;
            int32_t ofs0 = ui_edit_str.gp_to_bp(text, r->bytes, fro);
            int32_t ofs1 = ui_edit_str.gp_to_bp(text, r->bytes, to);
            ut_swear(ofs0 >= 0 && ofs1 >= 0);
            int32_t x0 = ui_edit_text_width(e, text, ofs0);
            int32_t x1 = ui_edit_text_width(e, text, ofs1);
            // selection_color is MSVC dark mode selection color
            // TODO: need light mode selection color tpp
            ui_color_t selection_color = ui_color_rgb(0x26, 0x4F, 0x78); // ui_color_rgb(64, 72, 96);
            if (!e->focused || !ui_app.focused()) {
                selection_color = ui_colors.darken(selection_color, 0.1f);
            }
            const ui_ltrb_t insets = ui_view.margins(&e->view, &e->insets);
            int32_t x = e->x + insets.left;
            ui_gdi.fill(x + x0, y, x1 - x0, e->fm->height, selection_color);
        }
    }
}

static int32_t ui_edit_paint_paragraph(ui_edit_t* e,
        const ui_gdi_ta_t* ta, int32_t x, int32_t y, int32_t pn,
        ui_rect_t rc) {
    static const char* ww = ut_glyph_south_west_arrow_with_hook;
    ui_edit_text_t* dt = &e->doc->text; // document text
    ut_assert(0 <= pn && pn < dt->np);
    const ui_edit_str_t* str = &dt->ps[pn];
    int32_t runs = 0;
    const ui_edit_run_t* run = ui_edit_paragraph_runs(e, pn, &runs);
    for (int32_t j = ui_edit_first_visible_run(e, pn);
                 j < runs && y < e->y + e->inside.bottom; j++) {
//      ut_println("[%d.%d] @%d,%d bytes: %d", pn, j, x, y, run[j].bytes);
        if (rc.y - e->fm->height <= y && y < rc.y + rc.h) {
            const char* text = str->u + run[j].bp;
            ui_edit_paint_selection(e, y, &run[j], text, pn,
                                    run[j].gp, run[j].gp + run[j].glyphs);
            ui_gdi.text(ta, x, y, "%.*s", run[j].bytes, text);
            if (j < runs - 1 && !e->hide_word_wrap) {
                ui_gdi.text(ta, x + e->edit.w, y, "%s", ww);
            }
        }
        y += e->fm->height;
    }
    return y;
}

static void ui_edit_paint(ui_view_t* v) {
    ut_assert(v->type == ui_view_text);
    ut_assert(!ui_view.is_hidden(v));
    ui_edit_t* e = (ui_edit_t*)v;
    ui_edit_text_t* dt = &e->doc->text; // document text
    // drawing text is really expensive, only paint what's needed:
    ui_rect_t vrc = (ui_rect_t){v->x, v->y, v->w, v->h};
    ui_rect_t rc;
    if (ui.intersect_rect(&rc, &vrc, &ui_app.prc)) {
        // because last line of the view may extend over the bottom
        ui_gdi.set_clip(v->x, v->y, v->w, v->h);
        ui_gdi.fill(rc.x, rc.y, rc.w, rc.h, v->background);
        const ui_ltrb_t insets = ui_view.margins(v, &v->insets);
        int32_t x = v->x + insets.left;
        int32_t y = v->y + insets.top;
        const ui_gdi_ta_t ta = { .fm = v->fm, .color = v->color };
        const int32_t pn = e->scroll.pn;
        const int32_t bottom = v->y + e->inside.bottom;
        ut_assert(pn < dt->np);
        for (int32_t i = pn; i < dt->np && y < bottom; i++) {
            y = ui_edit_paint_paragraph(e, &ta, x, y, i, rc);
        }
        ui_gdi.set_clip(0, 0, 0, 0);
    }
}

static void ui_edit_move(ui_edit_t* e, ui_edit_pg_t pg) {
    if (e->w > 0) {
        ui_edit_move_caret(e, pg); // may select text on move
    } else {
        e->selection.a[1] = pg;
    }
    e->selection.a[0] = e->selection.a[1];
}

static bool ui_edit_reallocate_runs(ui_edit_t* e, int32_t p, int32_t np) {
    // This function is called in after() callback when
    // d->text.np already changed to `new_np`.
    // It has to manipulate e->para[] array w/o calling
    // ui_edit_invalidate_runs() ui_edit_dispose_all_runs()
    // because they assume that e->para[] array is in sync
    // d->text.np.
    ui_edit_text_t* dt = &e->doc->text; // document text
    bool ok = true;
    int32_t old_np = np;     // old (before) number of paragraphs
    int32_t new_np = dt->np; // new (after)  number of paragraphs
    ut_assert(old_np > 0 && new_np > 0 && e->para != null);
    ut_assert(0 <= p && p < old_np);
    if (old_np == new_np) {
        ui_edit_invalidate_run(e, p);
    } else if (new_np < old_np) { // shrinking - delete runs
        const int32_t d = old_np - new_np; // `d` delta > 0
        if (p + d < old_np - 1) {
            const int32_t n = ut_max(0, old_np - p - d - 1);
            memcpy(e->para + p + 1, e->para + p + 1 + d, n * sizeof(e->para[0]));
        }
        if (p < new_np) { ui_edit_invalidate_run(e, p); }
        ok = ut_heap.realloc((void**)&e->para, new_np * sizeof(e->para[0])) == 0;
        ut_swear(ok, "shrinking");
    } else { // growing - insert runs
        ui_edit_invalidate_run(e, p);
        int32_t d = new_np - old_np;  // `d` delta > 0
        ok = ut_heap.realloc_zero((void**)&e->para, new_np * sizeof(e->para[0])) == 0;
        if (ok) {
            const int32_t n = ut_max(0, new_np - p - d - 1);
            memmove(e->para + p + 1 + d, e->para + p + 1,
                    (size_t)n * sizeof(e->para[0]));
            const int32_t m = ut_min(new_np, p + 1 + d);
            for (int32_t i = p + 1; i < m; i++) {
                e->para[i].run = null;
                e->para[i].runs = 0;
            }
        }
    }
    return ok;
}

static void ui_edit_before(ui_edit_notify_t* notify,
         const ui_edit_notify_info_t* ni) {
    ui_edit_notify_view_t* n = (ui_edit_notify_view_t*)notify;
    ui_edit_t* e = (ui_edit_t*)n->that;
    ut_swear(e->doc == ni->d);
    if (e->w > 0 && e->h > 0) {
        const ui_edit_text_t* dt = &e->doc->text; // document text
        ut_assert(dt->np > 0);
        // `n->data` is number of paragraphs before replace():
        n->data = (uintptr_t)dt->np;
        if (e->selection.from.pn != e->selection.to.pn) {
            ui_edit_invalidate_view(e);
        } else {
            ui_edit_invalidate_rect(e, ui_edit_selection_rect(e));
        }
    }
}

static void ui_edit_after(ui_edit_notify_t* notify,
         const ui_edit_notify_info_t* ni) {
    ui_edit_notify_view_t* n = (ui_edit_notify_view_t*)notify;
    ui_edit_t* e = (ui_edit_t*)n->that;
    const ui_edit_text_t* dt = &ni->d->text; // document text
    ut_assert(ni->d == e->doc && dt->np > 0);
    if (e->w > 0 && e->h > 0) {
        // number of paragraphs before replace():
        const int32_t np = (int32_t)n->data;
        ut_swear(dt->np == np - ni->deleted + ni->inserted);
        ui_edit_reallocate_runs(e, ni->r->from.pn, np);
        e->selection = *ni->x;
        // this is needed by undo/redo: trim selection
        ui_edit_pg_t* pg = e->selection.a;
        for (int32_t i = 0; i < ut_countof(e->selection.a); i++) {
            pg[i].pn = ut_max(0, ut_min(dt->np - 1, pg[i].pn));
            pg[i].gp = ut_max(0, ut_min(dt->ps[pg[i].pn].g, pg[i].gp));
        }
        if (ni->r->from.pn != ni->r->to.pn &&
            ni->x->from.pn != ni->x->to.pn &&
            ni->r->from.pn == ni->x->from.pn) {
            ui_edit_invalidate_rect(e, ui_edit_selection_rect(e));
        } else {
            ui_edit_invalidate_view(e);
        }
        ui_edit_scroll_into_view(e, e->selection.to);
    }
}

static void ui_edit_init(ui_edit_t* e, ui_edit_doc_t* d) {
    memset(e, 0, sizeof(*e));
    ut_assert(d != null && d->text.np > 0);
    e->doc = d;
    ut_assert(d->text.np > 0);
    e->listener.that = (void*)e;
    e->listener.data = 0;
    e->listener.notify.before = ui_edit_before;
    e->listener.notify.after  = ui_edit_after;
    ut_static_assertion(offsetof(ui_edit_notify_view_t, notify) == 0);
    ui_edit_doc.subscribe(d, &e->listener.notify);
    e->color_id = ui_color_id_window_text;
    e->background_id = ui_color_id_window;
    e->fm = &ui_app.fm.regular;
    e->insets  = (ui_margins_t){ 0.25, 0.25, 0.50, 0.25 };
    e->padding = (ui_margins_t){ 0.25, 0.25, 0.25, 0.25 };
    e->min_w_em = 1.0;
    e->min_h_em = 1.0;
    e->type = ui_view_text;
    e->focusable = true;
    e->last_x    = -1;
    e->focused   = false;
    e->sle       = false;
    e->ro        = false;
    e->caret        = (ui_point_t){-1, -1};
    e->paint        = ui_edit_paint;
    e->measure      = ui_edit_measure;
    e->layout       = ui_edit_layout;
    e->tap          = ui_edit_tap;
    e->long_press   = ui_edit_long_press;
    e->double_tap   = ui_edit_double_tap;
    e->character    = ui_edit_character;
    e->focus_gained = ui_edit_focus_gained;
    e->focus_lost   = ui_edit_focus_lost;
    e->key_pressed  = ui_edit_key_pressed;
    e->mouse_scroll = ui_edit_mouse_scroll;
    ui_edit_allocate_runs(e);
    if (e->debug.id == null) { e->debug.id = "#edit"; }
}

static void ui_edit_dispose(ui_edit_t* e) {
    ui_edit_doc.unsubscribe(e->doc, &e->listener.notify);
    ui_edit_dispose_all_runs(e);
    memset(e, 0, sizeof(*e));
}

ui_edit_if ui_edit = {
    .init                 = ui_edit_init,
    .set_font             = ui_edit_set_font,
    .move                 = ui_edit_move,
    .paste                = ui_edit_paste,
    .save                 = ui_edit_save,
    .erase                = ui_edit_erase,
    .cut_to_clipboard     = ui_edit_clipboard_cut,
    .copy_to_clipboard    = ui_edit_clipboard_copy,
    .paste_from_clipboard = ui_edit_clipboard_paste,
    .select_all           = ui_edit_select_all,
    .key_down             = ui_edit_key_down,
    .key_up               = ui_edit_key_up,
    .key_left             = ui_edit_key_left,
    .key_right            = ui_edit_key_right,
    .key_page_up          = ui_edit_key_page_up,
    .key_page_down        = ui_edit_key_page_down,
    .key_home             = ui_edit_key_home,
    .key_end              = ui_edit_key_end,
    .key_delete           = ui_edit_key_delete,
    .key_backspace        = ui_edit_key_backspace,
    .key_enter            = ui_edit_key_enter,
    .fuzz                 = null,
    .dispose              = ui_edit_dispose
};
// _______________________________ ui_fuzzing.c _______________________________

/* Copyright (c) Dmitry "Leo" Kuznetsov 2021-24 see LICENSE for details */
#include "ut/ut.h"

// TODO: Ctrl+A Ctrl+V Ctrl+C Ctrl+X Ctrl+Z Ctrl+Y

static bool     ui_fuzzing_debug = true;
static uint32_t ui_fuzzing_seed;
static bool     ui_fuzzing_running;
static bool     ui_fuzzing_inside;

static ui_fuzzing_t ui_fuzzing_work;

static const char* lorem_ipsum_words[] = {
    "lorem", "ipsum", "dolor", "sit", "amet", "consectetur", "adipiscing",
    "elit", "quisque", "faucibus", "ex", "sapien", "vitae", "pellentesque",
    "sem", "placerat", "in", "id", "cursus", "mi", "pretium", "tellus",
    "duis", "convallis", "tempus", "leo", "eu", "aenean", "sed", "diam",
    "urna", "tempor", "pulvinar", "vivamus", "fringilla", "lacus", "nec",
    "metus", "bibendum", "egestas", "iaculis", "massa", "nisl",
    "malesuada", "lacinia", "integer", "nunc", "posuere", "ut", "hendrerit",
    "semper", "vel", "class", "aptent", "taciti", "sociosqu", "ad", "litora",
    "torquent", "per", "conubia", "nostra", "inceptos",
    "himenaeos", "orci", "varius", "natoque", "penatibus", "et", "magnis",
    "dis", "parturient", "montes", "nascetur", "ridiculus", "mus", "donec",
    "rhoncus", "eros", "lobortis", "nulla", "molestie", "mattis",
    "scelerisque", "maximus", "eget", "fermentum", "odio", "phasellus",
    "non", "purus", "est", "efficitur", "laoreet", "mauris", "pharetra",
    "vestibulum", "fusce", "dictum", "risus", "blandit", "quis",
    "suspendisse", "aliquet", "nisi", "sodales", "consequat", "magna",
    "ante", "condimentum", "neque", "at", "luctus", "nibh", "finibus",
    "facilisis", "dapibus", "etiam", "interdum", "tortor", "ligula",
    "congue", "sollicitudin", "erat", "viverra", "ac", "tincidunt", "nam",
    "porta", "elementum", "a", "enim", "euismod", "quam", "justo",
    "lectus", "commodo", "augue", "arcu", "dignissim", "velit", "aliquam",
    "imperdiet", "mollis", "nullam", "volutpat", "porttitor",
    "ullamcorper", "rutrum", "gravida", "cras", "eleifend", "turpis",
    "fames", "primis", "vulputate", "ornare", "sagittis", "vehicula",
    "praesent", "dui", "felis", "venenatis", "ultrices", "proin", "libero",
    "feugiat", "tristique", "accumsan", "maecenas", "potenti", "ultricies",
    "habitant", "morbi", "senectus", "netus", "suscipit", "auctor",
    "curabitur", "facilisi", "cubilia", "curae", "hac", "habitasse",
    "platea", "dictumst"
};

#define ui_fuzzing_lorem_ipsum_canonique \
    "Lorem ipsum dolor sit amet, consectetur adipiscing elit, sed do "         \
    "eiusmod  tempor incididunt ut labore et dolore magna aliqua.Ut enim ad "  \
    "minim veniam, quis nostrud exercitation ullamco laboris nisi ut aliquip " \
    "ex ea commodo consequat. Duis aute irure dolor in reprehenderit in "      \
    "voluptate velit esse cillum dolore eu fugiat nulla pariatur. Excepteur "  \
    "sint occaecat cupidatat non proident, sunt in culpa qui officia "         \
    "deserunt mollit anim id est laborum."

#define ui_fuzzing_lorem_ipsum_chinese \
    "\xE6\x88\x91\xE6\x98\xAF\xE6\x94\xBE\xE7\xBD\xAE\xE6\x96\x87\xE6\x9C\xAC\xE7\x9A\x84\xE4" \
    "\xBD\x8D\xE7\xBD\xAE\xE3\x80\x82\xE8\xBF\x99\xE9\x87\x8C\xE6\x94\xBE\xE7\xBD\xAE\xE4\xBA" \
    "\x86\xE5\x81\x87\xE6\x96\x87\xE5\x81\x87\xE5\xAD\x97\xE3\x80\x82\xE5\xB8\x8C\xE6\x9C\x9B" \
    "\xE8\xBF\x99\xE4\xBA\x9B\xE6\x96\x87\xE5\xAD\x97\xE5\x8F\xAF\xE4\xBB\xA5\xE5\xA1\xAB\xE5" \
    "\x85\x85\xE7\xA9\xBA\xE7\x99\xBD\xE3\x80\x82";

#define ui_fuzzing_lorem_ipsum_japanese \
    "\xE3\x81\x93\xE3\x82\x8C\xE3\x81\xAF\xE3\x83\x80\xE3\x83\x9F\xE3\x83\xBC\xE3\x83\x86\xE3" \
    "\x82\xAD\xE3\x82\xB9\xE3\x83\x88\xE3\x81\xA7\xE3\x81\x99\xE3\x80\x82\xE3\x81\x93\xE3\x81" \
    "\x93\xE3\x81\xAB\xE6\x96\x87\xE7\xAB\xA0\xE3\x81\x8C\xE5\x85\xA5\xE3\x82\x8A\xE3\x81\xBE" \
    "\xE3\x81\x99\xE3\x80\x82\xE8\xAA\xAD\xE3\x81\xBF\xE3\x82\x84\xE3\x81\x99\xE3\x81\x84\xE3" \
    "\x82\x88\xE3\x81\x86\xE3\x81\xAB\xE3\x83\x80\xE3\x83\x9F\xE3\x83\xBC\xE3\x83\x86\xE3\x82" \
    "\xAD\xE3\x82\xB9\xE3\x83\x88\xE3\x82\x92\xE4\xBD\xBF\xE7\x94\xA8\xE3\x81\x97\xE3\x81\xA6" \
    "\xE3\x81\x84\xE3\x81\xBE\xE3\x81\x99\xE3\x80\x82";


#define ui_fuzzing_lorem_ipsum_korean \
    "\xEC\x9D\xB4\xEA\xB2\x83\xEC\x9D\x80\x20\xEB\x8D\x94\xEB\xAF\xB8\x20\xED\x85\x8D\xEC\x8A" \
    "\xA4\xED\x8A\xB8\xEC\x9E\x85\xEB\x8B\x88\xEB\x8B\xA4\x2E\x20\xEC\x97\xAC\xEA\xB8\xB0\xEC" \
    "\x97\x90\x20\xEB\xAC\xB8\xEC\x9E\x90\xEA\xB0\x80\x20\xEB\x93\x9C\xEC\x96\xB4\xEA\xB0\x80" \
    "\xEB\x8A\x94\x20\xEB\xAC\xB8\xEC\x9E\x90\xEA\xB0\x80\x20\xEC\x9E\x88\xEB\x8B\xA4\x2E\x20" \
    "\xEC\x9D\xBD\xEA\xB8\xB0\x20\xEC\x89\xBD\xEA\xB2\x8C\x20\xEB\x8D\x94\xEB\xAF\xB8\x20\xED" \
    "\x85\x8D\xEC\x8A\xA4\xED\x8A\xB8\xEB\xA5\xBC\x20\xEC\x82\xAC\xEC\x9A\xA9\xED\x95\xA9\xEB" \
    "\x8B\x88\xEB\x8B\xA4\x2E";

#define ui_fuzzing_lorem_ipsum_emoji \
    "\xF0\x9F\x8D\x95\xF0\x9F\x9A\x80\xF0\x9F\xA6\x84\xF0\x9F\x92\xBB\xF0\x9F\x8E\x89\xF0\x9F" \
    "\x8C\x88\xF0\x9F\x90\xB1\xF0\x9F\x93\x9A\xF0\x9F\x8E\xA8\xF0\x9F\x8D\x94\xF0\x9F\x8D\xA6" \
    "\xF0\x9F\x8E\xB8\xF0\x9F\xA7\xA9\xF0\x9F\x8D\xBF\xF0\x9F\x93\xB7\xF0\x9F\x8E\xA4\xF0\x9F" \
    "\x91\xBE\xF0\x9F\x8C\xAE\xF0\x9F\x8E\x88\xF0\x9F\x9A\xB2\xF0\x9F\x8D\xA9\xF0\x9F\x8E\xAE" \
    "\xF0\x9F\x8D\x89\xF0\x9F\x8E\xAC\xF0\x9F\x90\xB6\xF0\x9F\x93\xB1\xF0\x9F\x8E\xB9\xF0\x9F" \
    "\xA6\x96\xF0\x9F\x8C\x9F\xF0\x9F\x8D\xAD\xF0\x9F\x8E\xA4\xF0\x9F\x8F\x96\xF0\x9F\xA6\x8B" \
    "\xF0\x9F\x8E\xB2\xF0\x9F\x8E\xAF\xF0\x9F\x8D\xA3\xF0\x9F\x9A\x81\xF0\x9F\x8E\xAD\xF0\x9F" \
    "\x91\x9F\xF0\x9F\x9A\x82\xF0\x9F\x8D\xAA\xF0\x9F\x8E\xBB\xF0\x9F\x9B\xB8\xF0\x9F\x8C\xBD" \
    "\xF0\x9F\x93\x80\xF0\x9F\x9A\x80\xF0\x9F\xA7\x81\xF0\x9F\x93\xAF\xF0\x9F\x8C\xAF\xF0\x9F" \
    "\x90\xA5\xF0\x9F\xA7\x83\xF0\x9F\x8D\xBB\xF0\x9F\x8E\xAE";

typedef struct {
    char* text;
    int32_t count; // at least 1KB
    uint32_t seed; // seed for random generator
    int32_t min_paragraphs; // at least 1
    int32_t max_paragraphs;
    int32_t min_sentences; // at least 1
    int32_t max_sentences;
    int32_t min_words; // at least 2
    int32_t max_words;
    const char* append; // append after each paragraph (e.g. extra "\n")
} ui_fuzzing_generator_params_t;

static uint32_t ui_fuzzing_random(void) {
    return ut_num.random32(&ui_fuzzing_seed);
}

static fp64_t ui_fuzzing_random_fp64(void) {
    uint32_t r = ui_fuzzing_random();
    return (fp64_t)r / (fp64_t)UINT32_MAX;
}

static void ui_fuzzing_generator(ui_fuzzing_generator_params_t p) {
    ut_fatal_if(p.count < 1024); // at least 1KB expected
    ut_fatal_if_not(0 < p.min_paragraphs && p.min_paragraphs <= p.max_paragraphs);
    ut_fatal_if_not(0 < p.min_sentences && p.min_sentences <= p.max_sentences);
    ut_fatal_if_not(2 < p.min_words && p.min_words <= p.max_words);
    char* s = p.text;
    // assume longest word is less than 128
    char* end = p.text + p.count - 128;
    uint32_t paragraphs = p.min_paragraphs +
        (p.min_paragraphs == p.max_paragraphs ? 0 :
         ut_num.random32(&p.seed) % (p.max_paragraphs - p.min_paragraphs + 1));
    while (paragraphs > 0 && s < end) {
        uint32_t sentences_in_paragraph = p.min_sentences +
            (p.min_sentences == p.max_sentences ? 0 :
             ut_num.random32(&p.seed) % (p.max_sentences - p.min_sentences + 1));
        while (sentences_in_paragraph > 0 && s < end) {
            const uint32_t words_in_sentence = p.min_words +
                (p.min_words == p.max_words ? 0 :
                 ut_num.random32(&p.seed) % (p.max_words - p.min_words + 1));
            for (uint32_t i = 0; i < words_in_sentence && s < end; i++) {
                const int32_t ix = ut_num.random32(&p.seed) %
                                   ut_countof(lorem_ipsum_words);
                const char* word = lorem_ipsum_words[ix];
                memcpy(s, word, strlen(word));
                if (i == 0) { *s = (char)toupper(*s); }
                s += strlen(word);
                if (i < words_in_sentence - 1 && s < end) {
                    const char* delimiter = "\x20";
                    int32_t punctuation = ut_num.random32(&p.seed) % 128;
                    switch (punctuation) {
                        case 0:
                        case 1:
                        case 2: delimiter = ", "; break;
                        case 3:
                        case 4: delimiter = "; "; break;
                        case 6: delimiter = ": "; break;
                        case 7: delimiter = " - "; break;
                        default: break;
                    }
                    memcpy(s, delimiter, strlen(delimiter));
                    s += strlen(delimiter);
                }
            }
            if (sentences_in_paragraph > 1 && s < end) {
                memcpy(s, ".\x20", 2);
                s += 2;
            } else {
                *s++ = '.';
            }
            sentences_in_paragraph--;
        }
        if (paragraphs > 1 && s < end) {
            *s++ = '\n';
        }
        if (p.append != null && p.append[0] != 0) {
            memcpy(s, p.append, strlen(p.append));
            s += strlen(p.append);
        }
        paragraphs--;
    }
    *s = 0;
//  ut_println("%s\n", p.text);
}

static void ui_fuzzing_next_gibberish(int32_t number_of_characters,
        char text[]) {
    static fp64_t freq[96] = {
        0.1716, 0.0023, 0.0027, 0.0002, 0.0001, 0.0005, 0.0013, 0.0012,
        0.0015, 0.0014, 0.0017, 0.0002, 0.0084, 0.0020, 0.0075, 0.0040,
        0.0135, 0.0045, 0.0053, 0.0053, 0.0047, 0.0047, 0.0043, 0.0047,
        0.0057, 0.0044, 0.0037, 0.0004, 0.0016, 0.0004, 0.0017, 0.0017,
        0.0020, 0.0045, 0.0026, 0.0020, 0.0027, 0.0021, 0.0025, 0.0026,
        0.0030, 0.0025, 0.0021, 0.0018, 0.0028, 0.0026, 0.0024, 0.0020,
        0.0025, 0.0026, 0.0030, 0.0022, 0.0027, 0.0022, 0.0020, 0.0023,
        0.0015, 0.0016, 0.0009, 0.0005, 0.0005, 0.0001, 0.0003, 0.0003,
        0.0078, 0.0013, 0.0012, 0.0008, 0.0012, 0.0007, 0.0006, 0.0011,
        0.0016, 0.0012, 0.0011, 0.0004, 0.0004, 0.0016, 0.0013, 0.0009,
        0.0009, 0.0008, 0.0013, 0.0011, 0.0013, 0.0012, 0.0006, 0.0007,
        0.0011, 0.0005, 0.0007, 0.0003, 0.0002, 0.0006, 0.0002, 0.0005
    };
    static fp64_t cumulative_freq[96];
    static bool initialized = 0;
    if (!initialized) {
        cumulative_freq[0] = freq[0];
        for (int i = 1; i < ut_countof(freq); i++) {
            cumulative_freq[i] = cumulative_freq[i - 1] + freq[i];
        }
        initialized = 1;
    }
    int32_t i = 0;
    while (i < number_of_characters) {
        text[i] = 0x00;
        fp64_t r = ui_fuzzing_random_fp64();
        for (int j = 0; j < 96 && text[i] == 0; j++) {
            if (r < cumulative_freq[j]) {
                text[i] = (char)(0x20 + j);
            }
        }
        if (text[i] != 0) { i++; }
    }
    text[number_of_characters] = 0x00;
}

static void ui_fuzzing_dispatch(ui_fuzzing_t* work) {
    ut_swear(work == &ui_fuzzing_work);
    ui_app.alt = work->alt;
    ui_app.ctrl = work->ctrl;
    ui_app.shift = work->shift;
    if (work->utf8 != null && work->utf8[0] != 0) {
        ui_view.character(ui_app.content, work->utf8);
        work->utf8 = work->utf8[1] == 0 ? null : work->utf8++;
    } else if (work->key != 0) {
        ui_view.key_pressed(ui_app.content, work->key);
        ui_view.key_released(ui_app.content, work->key);
        work->key = 0;
    } else if (work->pt != null) {
        const int32_t x = work->pt->x;
        const int32_t y = work->pt->y;
        ui_app.mouse.x = x;
        ui_app.mouse.y = y;
//      https://stackoverflow.com/questions/22259936/
//      https://stackoverflow.com/questions/65691101/
//      ut_println("%d,%d", x + ui_app.wrc.x, y + ui_app.wrc.y);
//      // next line works only when running as administrator:
//      ut_fatal_win32err(SetCursorPos(x + ui_app.wrc.x, y + ui_app.wrc.y));
        const bool l_button = ui_app.mouse_left  != work->left;
        const bool r_button = ui_app.mouse_right != work->right;
        ui_app.mouse_left  = work->left;
        ui_app.mouse_right = work->right;
        ui_view.mouse_move(ui_app.content);
        if (l_button) {
            ui_view.tap(ui_app.content, 0, work->left);
        }
        if (r_button) {
            ui_view.tap(ui_app.content, 2, work->right);
        }
        work->pt = null;
    } else {
        ut_assert(false, "TODO: ?");
    }
    if (ui_fuzzing_running) {
        if (ui_fuzzing.next == null) {
            ui_fuzzing.next_random(work);
        } else {
            ui_fuzzing.next(work);
        }
    }
}

static void ui_fuzzing_do_work(ut_work_t* p) {
    if (ui_fuzzing_running) {
        ui_fuzzing_inside = true;
        if (ui_fuzzing.custom != null) {
            ui_fuzzing.custom((ui_fuzzing_t*)p);
        } else {
            ui_fuzzing.dispatch((ui_fuzzing_t*)p);
        }
        ui_fuzzing_inside = false;
    } else {
        // fuzzing has been .stop()-ed drop it
    }
}

static void ui_fuzzing_post(void) {
    ui_app.post(&ui_fuzzing_work.base);
}

static void ui_fuzzing_alt_ctrl_shift(void) {
    ui_fuzzing_t* w = &ui_fuzzing_work;
    switch (ui_fuzzing_random() % 8) {
        case 0: w->alt = 0; w->ctrl = 0; w->shift = 0; break;
        case 1: w->alt = 1; w->ctrl = 0; w->shift = 0; break;
        case 2: w->alt = 0; w->ctrl = 1; w->shift = 0; break;
        case 3: w->alt = 1; w->ctrl = 1; w->shift = 0; break;
        case 4: w->alt = 0; w->ctrl = 0; w->shift = 1; break;
        case 5: w->alt = 1; w->ctrl = 0; w->shift = 1; break;
        case 6: w->alt = 0; w->ctrl = 1; w->shift = 1; break;
        case 7: w->alt = 1; w->ctrl = 1; w->shift = 1; break;
        default: ut_assert(false);
    }
}

static void ui_fuzzing_character(void) {
    static char utf8[4 * 1024];
    if (ui_fuzzing_work.utf8 == null) {
        fp64_t r = ui_fuzzing_random_fp64();
        if (r < 0.125) {
            uint32_t rnd = ui_fuzzing_random();
            int32_t n = (int32_t)ut_max(1, rnd % 32);
            ui_fuzzing_next_gibberish(n, utf8);
            ui_fuzzing_work.utf8 = utf8;
            if (ui_fuzzing_debug) {
    //          ut_println("%s", utf8);
            }
        } else if (r < 0.25) {
            ui_fuzzing_work.utf8 = ui_fuzzing_lorem_ipsum_chinese;
        } else if (r < 0.375) {
            ui_fuzzing_work.utf8 = ui_fuzzing_lorem_ipsum_japanese;
        } else if (r < 0.5) {
            ui_fuzzing_work.utf8 = ui_fuzzing_lorem_ipsum_korean;
        } else if (r < 0.5 + 0.125) {
            ui_fuzzing_work.utf8 = ui_fuzzing_lorem_ipsum_emoji;
        } else {
            ui_fuzzing_work.utf8 = ui_fuzzing_lorem_ipsum_canonique;
        }
    }
    ui_fuzzing_post();
}

static void ui_fuzzing_key(void) {
    struct {
        int32_t key;
        const char* name;
    } keys[] = {
        { ui.key.up,      "up",     },
        { ui.key.down,    "down",   },
        { ui.key.left,    "left",   },
        { ui.key.right,   "right",  },
        { ui.key.home,    "home",   },
        { ui.key.end,     "end",    },
        { ui.key.pageup,  "pgup",   },
        { ui.key.pagedw,  "pgdw",   },
        { ui.key.insert,  "insert"  },
        { ui.key.enter,   "enter"   },
        { ui.key.del,     "delete"  },
        { ui.key.back,    "back"   },
    };
    ui_fuzzing_alt_ctrl_shift();
    uint32_t ix = ui_fuzzing_random() % ut_countof(keys);
    if (ui_fuzzing_debug) {
//      ut_println("key(%s)", keys[ix].name);
    }
    ui_fuzzing_work.key = keys[ix].key;
    ui_fuzzing_post();
}

static void ui_fuzzing_mouse(void) {
    // mouse events only inside edit control otherwise
    // they will start clicking buttons around
    ui_view_t* v = ui_app.content;
    ui_fuzzing_t* w = &ui_fuzzing_work;
    int32_t x = ui_fuzzing_random() % v->w;
    int32_t y = ui_fuzzing_random() % v->h;
    static ui_point_t pt;
    pt = (ui_point_t){ x + v->x, y + v->y };
    if (ui_fuzzing_random() % 2) {
        w->left  = !w->left;
    }
    if (ui_fuzzing_random() % 2) {
        w->right = !w->right;
    }
    if (ui_fuzzing_debug) {
//      ut_println("mouse(%d,%d) %s%s", pt.x, pt.y,
//              w->left ? "L" : "_", w->right ? "R" : "_");
    }
    w->pt = &pt;
    ui_fuzzing_post();
}

static void ui_fuzzing_start(uint32_t seed) {
    ui_fuzzing_seed = seed | 0x1;
    ui_fuzzing_running = true;
    if (ui_fuzzing.next == null) {
        ui_fuzzing.next_random(&ui_fuzzing_work);
    } else {
        ui_fuzzing.next(&ui_fuzzing_work);
    }
}

static bool ui_fuzzing_is_running(void) {
    return ui_fuzzing_running;
}

static bool ui_fuzzing_from_inside(void) {
    return ui_fuzzing_inside;
}

static void ui_fuzzing_stop(void) {
    ui_fuzzing_running = false;
}

static void ui_fuzzing_next_random(ui_fuzzing_t* f) {
    ut_swear(f == &ui_fuzzing_work);
    ui_fuzzing_work = (ui_fuzzing_t){
        .base = { .when = ut_clock.seconds() + 0.001, // 1ms
                  .work = ui_fuzzing_do_work },
    };
    uint32_t rnd = ui_fuzzing_random() % 100;
    if (rnd < 80) {
        ui_fuzzing_character();
    } else if (rnd < 90) {
        ui_fuzzing_key();
    } else {
        ui_fuzzing_mouse();
    }
}

ui_fuzzing_if ui_fuzzing = {
    .start       = ui_fuzzing_start,
    .is_running  = ui_fuzzing_is_running,
    .from_inside = ui_fuzzing_from_inside,
    .next_random = ui_fuzzing_next_random,
    .dispatch    = ui_fuzzing_dispatch,
    .next        = null,
    .custom      = null,
    .stop        = ui_fuzzing_stop
};
// _________________________________ ui_gdi.c _________________________________

#include "ut/ut.h"
#include "ut/ut_win32.h"

#pragma push_macro("ui_gdi_with_hdc")
#pragma push_macro("ui_gdi_hdc_with_font")

static ui_brush_t  ui_gdi_brush_hollow;
static ui_brush_t  ui_gdi_brush_color;
static ui_pen_t    ui_gdi_pen_hollow;
static ui_region_t ui_gdi_clip;

typedef struct ui_gdi_context_s {
    HDC hdc; // window canvas() or memory DC
    int32_t background_mode;
    int32_t stretch_mode;
    ui_pen_t pen;
    ui_font_t font;
    ui_color_t text_color;
    POINT brush_origin;
    ui_brush_t brush;
    HBITMAP bitmap;
} ui_gdi_context_t;

static ui_gdi_context_t ui_gdi_context;

#define ui_gdi_hdc() (ui_gdi_context.hdc)

static void ui_gdi_init(void) {
    ui_gdi_brush_hollow = (ui_brush_t)GetStockBrush(HOLLOW_BRUSH);
    ui_gdi_brush_color  = (ui_brush_t)GetStockBrush(DC_BRUSH);
    ui_gdi_pen_hollow = (ui_pen_t)GetStockPen(NULL_PEN);
}

static void ui_gdi_fini(void) {
    if (ui_gdi_clip != null) {
        ut_fatal_win32err(DeleteRgn(ui_gdi_clip));
    }
    ui_gdi_clip = null;
}

static ui_pen_t ui_gdi_set_pen(ui_pen_t p) {
    ut_not_null(p);
    return (ui_pen_t)SelectPen(ui_gdi_hdc(), (HPEN)p);
}

static ui_brush_t ui_gdi_set_brush(ui_brush_t b) {
    ut_not_null(b);
    return (ui_brush_t)SelectBrush(ui_gdi_hdc(), b);
}

static uint32_t ui_gdi_color_rgb(ui_color_t c) {
    ut_assert(ui_color_is_8bit(c));
    return (COLORREF)(c & 0xFFFFFFFF);
}

static COLORREF ui_gdi_color_ref(ui_color_t c) {
    return ui_gdi.color_rgb(c);
}

static ui_color_t ui_gdi_set_text_color(ui_color_t c) {
    return SetTextColor(ui_gdi_hdc(), ui_gdi_color_ref(c));
}

static ui_font_t ui_gdi_set_font(ui_font_t f) {
    ut_not_null(f);
    return (ui_font_t)SelectFont(ui_gdi_hdc(), (HFONT)f);
}

static void ui_gdi_begin(ui_image_t* image) {
    ut_swear(ui_gdi_context.hdc == null, "no nested begin()/end()");
    if (image != null) {
        ut_swear(image->bitmap != null);
        ui_gdi_context.hdc = CreateCompatibleDC((HDC)ui_app.canvas);
        ui_gdi_context.bitmap = SelectBitmap(ui_gdi_hdc(),
                                             (HBITMAP)image->bitmap);
    } else {
        ui_gdi_context.hdc = (HDC)ui_app.canvas;
        ut_swear(ui_gdi_context.bitmap == null);
    }
    ui_gdi_context.font  = ui_gdi_set_font(ui_app.fm.regular.font);
    ui_gdi_context.pen   = ui_gdi_set_pen(ui_gdi_pen_hollow);
    ui_gdi_context.brush = ui_gdi_set_brush(ui_gdi_brush_hollow);
    ut_fatal_win32err(SetBrushOrgEx(ui_gdi_hdc(), 0, 0,
        &ui_gdi_context.brush_origin));
    ui_color_t tc = ui_colors.get_color(ui_color_id_window_text);
    ui_gdi_context.text_color = ui_gdi_set_text_color(tc);
    ui_gdi_context.background_mode = SetBkMode(ui_gdi_hdc(), TRANSPARENT);
    ui_gdi_context.stretch_mode = SetStretchBltMode(ui_gdi_hdc(), HALFTONE);
}

static void ui_gdi_end(void) {
    ut_fatal_win32err(SetBrushOrgEx(ui_gdi_hdc(),
                   ui_gdi_context.brush_origin.x,
                   ui_gdi_context.brush_origin.y, null));
    ui_gdi_set_brush(ui_gdi_context.brush);
    ui_gdi_set_pen(ui_gdi_context.pen);
    ui_gdi_set_text_color(ui_gdi_context.text_color);
    SetBkMode(ui_gdi_hdc(), ui_gdi_context.background_mode);
    SetStretchBltMode(ui_gdi_hdc(), ui_gdi_context.stretch_mode);
    if (ui_gdi_context.hdc != (HDC)ui_app.canvas) {
        ut_swear(ui_gdi_context.bitmap != null); // 1x1 bitmap
        SelectBitmap(ui_gdi_context.hdc, (HBITMAP)ui_gdi_context.bitmap);
        ut_fatal_win32err(DeleteDC(ui_gdi_context.hdc));
    }
    memset(&ui_gdi_context, 0x00, sizeof(ui_gdi_context));
}

static ui_pen_t ui_gdi_set_colored_pen(ui_color_t c) {
    ui_pen_t p = (ui_pen_t)SelectPen(ui_gdi_hdc(), GetStockPen(DC_PEN));
    SetDCPenColor(ui_gdi_hdc(), ui_gdi_color_ref(c));
    return p;
}

static ui_pen_t ui_gdi_create_pen(ui_color_t c, int32_t width) {
    ut_assert(width >= 1);
    ui_pen_t pen = (ui_pen_t)CreatePen(PS_SOLID, width, ui_gdi_color_ref(c));
    ut_not_null(pen);
    return pen;
}

static void ui_gdi_delete_pen(ui_pen_t p) {
    ut_fatal_win32err(DeletePen(p));
}

static ui_brush_t ui_gdi_create_brush(ui_color_t c) {
    return (ui_brush_t)CreateSolidBrush(ui_gdi_color_ref(c));
}

static void ui_gdi_delete_brush(ui_brush_t b) {
    DeleteBrush((HBRUSH)b);
}

static ui_color_t ui_gdi_set_brush_color(ui_color_t c) {
    return SetDCBrushColor(ui_gdi_hdc(), ui_gdi_color_ref(c));
}

static void ui_gdi_set_clip(int32_t x, int32_t y, int32_t w, int32_t h) {
    if (ui_gdi_clip != null) { DeleteRgn(ui_gdi_clip); ui_gdi_clip = null; }
    if (w > 0 && h > 0) {
        ui_gdi_clip = (ui_region_t)CreateRectRgn(x, y, x + w, y + h);
        ut_not_null(ui_gdi_clip);
    }
    ut_fatal_if(SelectClipRgn(ui_gdi_hdc(), (HRGN)ui_gdi_clip) == ERROR);
}

static void ui_gdi_pixel(int32_t x, int32_t y, ui_color_t c) {
    ut_not_null(ui_app.canvas);
    ut_fatal_win32err(SetPixel(ui_gdi_hdc(), x, y, ui_gdi_color_ref(c)));
}

static void ui_gdi_rectangle(int32_t x, int32_t y, int32_t w, int32_t h) {
    ut_fatal_win32err(Rectangle(ui_gdi_hdc(), x, y, x + w, y + h));
}

static void ui_gdi_line(int32_t x0, int32_t y0, int32_t x1, int32_t y1,
        ui_color_t c) {
    POINT pt;
    ut_fatal_win32err(MoveToEx(ui_gdi_hdc(), x0, y0, &pt));
    ui_pen_t p = ui_gdi_set_colored_pen(c);
    ut_fatal_win32err(LineTo(ui_gdi_hdc(), x1, y1));
    ui_gdi_set_pen(p);
    ut_fatal_win32err(MoveToEx(ui_gdi_hdc(), pt.x, pt.y, null));
}

static void ui_gdi_frame(int32_t x, int32_t y, int32_t w, int32_t h,
        ui_color_t c) {
    ui_brush_t b = ui_gdi_set_brush(ui_gdi_brush_hollow);
    ui_pen_t p = ui_gdi_set_colored_pen(c);
    ui_gdi_rectangle(x, y, w, h);
    ui_gdi_set_pen(p);
    ui_gdi_set_brush(b);
}

static void ui_gdi_rect(int32_t x, int32_t y, int32_t w, int32_t h,
        ui_color_t border, ui_color_t fill) {
    const bool tf = ui_color_is_transparent(fill);   // transparent fill
    const bool tb = ui_color_is_transparent(border); // transparent border
    ui_brush_t b = tf ? ui_gdi_brush_hollow : ui_gdi_brush_color;
    b = ui_gdi_set_brush(b);
    ui_color_t c = tf ? ui_colors.transparent : ui_gdi_set_brush_color(fill);
    ui_pen_t p = tb ? ui_gdi_set_pen(ui_gdi_pen_hollow) :
                      ui_gdi_set_colored_pen(border);
    ui_gdi_rectangle(x, y, w, h);
    if (!tf) { ui_gdi_set_brush_color(c); }
    ui_gdi_set_pen(p);
    ui_gdi_set_brush(b);
}

static void ui_gdi_fill(int32_t x, int32_t y, int32_t w, int32_t h,
        ui_color_t c) {
//  ut_println("%d,%d %dx%d 0x%08X", x, y, w, h, (uint32_t)c);
    ui_brush_t b = ui_gdi_set_brush(ui_gdi_brush_color);
    c = ui_gdi_set_brush_color(c);
    RECT rc = { x, y, x + w, y + h };
    HBRUSH brush = (HBRUSH)GetCurrentObject(ui_gdi_hdc(), OBJ_BRUSH);
    ut_fatal_win32err(FillRect(ui_gdi_hdc(), &rc, brush));
    ui_gdi_set_brush_color(c);
    ui_gdi_set_brush(b);
}

static void ui_gdi_poly(ui_point_t* points, int32_t count, ui_color_t c) {
    // make sure ui_point_t and POINT have the same memory layout:
    static_assert(sizeof(points->x) == sizeof(((POINT*)0)->x), "ui_point_t");
    static_assert(sizeof(points->y) == sizeof(((POINT*)0)->y), "ui_point_t");
    static_assert(sizeof(points[0]) == sizeof(*((POINT*)0)), "ui_point_t");
    ut_assert(ui_gdi_hdc() != null && count > 1);
    ui_pen_t pen = ui_gdi_set_colored_pen(c);
    ut_fatal_win32err(Polyline(ui_gdi_hdc(), (POINT*)points, count));
    ui_gdi_set_pen(pen);
}

static void ui_gdi_circle(int32_t x, int32_t y, int32_t radius,
        ui_color_t border, ui_color_t fill) {
    ut_swear(!ui_color_is_transparent(border) || ui_color_is_transparent(fill));
    // Win32 GDI even radius drawing looks ugly squarish and asymmetrical.
    ut_swear(radius % 2 == 1, "radius: %d must be odd");
    if (ui_color_is_transparent(border)) {
        ut_assert(!ui_color_is_transparent(fill));
        border = fill;
    }
    ut_assert(!ui_color_is_transparent(border));
    const bool tf = ui_color_is_transparent(fill);   // transparent fill
    ui_brush_t brush = tf ? ui_gdi_set_brush(ui_gdi_brush_hollow) :
                        ui_gdi_set_brush(ui_gdi_brush_color);
    ui_color_t c = tf ? ui_colors.transparent : ui_gdi_set_brush_color(fill);
    ui_pen_t p = ui_gdi_set_colored_pen(border);
    HDC hdc = ui_gdi_context.hdc;
    int32_t l = x - radius;
    int32_t t = y - radius;
    int32_t r = x + radius + 1;
    int32_t b = y + radius + 1;
    Ellipse(hdc, l, t, r, b);
//  SetPixel(hdc, x, y, RGB(255, 255, 255));
    ui_gdi_set_pen(p);
    if (!tf) { ui_gdi_set_brush_color(c); }
    ui_gdi_set_brush(brush);
}

static void ui_gdi_fill_rounded(int32_t x, int32_t y, int32_t w, int32_t h,
        int32_t radius, ui_color_t fill) {
    int32_t r = x + w - 1; // right
    int32_t b = y + h - 1; // bottom
    ui_gdi_circle(x + radius, y + radius, radius, fill, fill);
    ui_gdi_circle(r - radius, y + radius, radius, fill, fill);
    ui_gdi_circle(x + radius, b - radius, radius, fill, fill);
    ui_gdi_circle(r - radius, b - radius, radius, fill, fill);
    // rectangles
    ui_gdi.fill(x + radius, y, w - radius * 2, h, fill);
    r = x + w - radius;
    ui_gdi.fill(x, y + radius, radius, h - radius * 2, fill);
    ui_gdi.fill(r, y + radius, radius, h - radius * 2, fill);
}

static void ui_gdi_rounded_border(int32_t x, int32_t y, int32_t w, int32_t h,
        int32_t radius, ui_color_t border) {
    {
        int32_t r = x + w - 1; // right
        int32_t b = y + h - 1; // bottom
        ui_gdi.set_clip(x, y, radius + 1, radius + 1);
        ui_gdi_circle(x + radius, y + radius, radius, border, ui_colors.transparent);
        ui_gdi.set_clip(r - radius, y, radius + 1, radius + 1);
        ui_gdi_circle(r - radius, y + radius, radius, border, ui_colors.transparent);
        ui_gdi.set_clip(x, b - radius, radius + 1, radius + 1);
        ui_gdi_circle(x + radius, b - radius, radius, border, ui_colors.transparent);
        ui_gdi.set_clip(r - radius, b - radius, radius + 1, radius + 1);
        ui_gdi_circle(r - radius, b - radius, radius, border, ui_colors.transparent);
        ui_gdi.set_clip(0, 0, 0, 0);
    }
    {
        int32_t r = x + w - 1; // right
        int32_t b = y + h - 1; // bottom
        ui_gdi.line(x + radius, y, r - radius + 1, y, border);
        ui_gdi.line(x + radius, b, r - radius + 1, b, border);
        ui_gdi.line(x - 1, y + radius, x - 1, b - radius + 1, border);
        ui_gdi.line(r + 1, y + radius, r + 1, b - radius + 1, border);
    }
}

static void ui_gdi_rounded(int32_t x, int32_t y, int32_t w, int32_t h,
        int32_t radius, ui_color_t border, ui_color_t fill) {
    ut_swear(!ui_color_is_transparent(border) || !ui_color_is_transparent(fill));
    if (!ui_color_is_transparent(fill)) {
        ui_gdi_fill_rounded(x, y, w, h, radius, fill);
    }
    if (!ui_color_is_transparent(border)) {
        ui_gdi_rounded_border(x, y, w, h, radius, border);
    }
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
    GradientFill(ui_gdi_hdc(), vertex, 2, &gRect, 1, mode);
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

static void ui_gdi_greyscale(int32_t sx, int32_t sy, int32_t sw, int32_t sh,
        int32_t x, int32_t y, int32_t w, int32_t h,
        int32_t iw, int32_t ih, int32_t stride, const uint8_t* pixels) {
    ut_fatal_if(stride != ((iw + 3) & ~0x3));
    ut_assert(w > 0 && h != 0); // h can be negative
    if (w > 0 && h != 0) {
        BITMAPINFO *bi = ui_gdi_greyscale_bitmap_info(); // global! not thread safe
        BITMAPINFOHEADER* bih = &bi->bmiHeader;
        bih->biWidth = iw;
        bih->biHeight = -ih; // top down image
        bih->biSizeImage = (DWORD)(w * abs(h));
        POINT pt = { 0 };
        ut_fatal_win32err(SetBrushOrgEx(ui_gdi_hdc(), 0, 0, &pt));
        ut_fatal_if(StretchDIBits(ui_gdi_hdc(), sx, sy, sw, sh, x, y, w, h,
            pixels, bi, DIB_RGB_COLORS, SRCCOPY) == 0);
        ut_fatal_win32err(SetBrushOrgEx(ui_gdi_hdc(), pt.x, pt.y, &pt));
    }
}

static BITMAPINFOHEADER ui_gdi_bgrx_init_bi(int32_t w, int32_t h, int32_t bpp) {
    ut_assert(w > 0 && h >= 0); // h cannot be negative?
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

// bgr(iw) assumes strides are padded and rounded up to 4 bytes
// if this is not the case use ui_gdi.image_init() that will unpack
// and align scanlines prior to draw

static void ui_gdi_bgr(int32_t sx, int32_t sy, int32_t sw, int32_t sh,
        int32_t x, int32_t y, int32_t w, int32_t h,
        int32_t iw, int32_t ih, int32_t stride,
        const uint8_t* pixels) {
    ut_fatal_if(stride != ((iw * 3 + 3) & ~0x3));
    ut_assert(w > 0 && h != 0); // h can be negative
    if (w > 0 && h != 0) {
        BITMAPINFOHEADER bi = ui_gdi_bgrx_init_bi(iw, ih, 3);
        POINT pt = { 0 };
        ut_fatal_win32err(SetBrushOrgEx(ui_gdi_hdc(), 0, 0, &pt));
        ut_fatal_if(StretchDIBits(ui_gdi_hdc(), sx, sy, sw, sh, x, y, w, h,
            pixels, (BITMAPINFO*)&bi, DIB_RGB_COLORS, SRCCOPY) == 0);
        ut_fatal_win32err(SetBrushOrgEx(ui_gdi_hdc(), pt.x, pt.y, &pt));
    }
}

static void ui_gdi_bgrx(int32_t sx, int32_t sy, int32_t sw, int32_t sh,
        int32_t x, int32_t y, int32_t w, int32_t h,
        int32_t iw, int32_t ih, int32_t stride,
        const uint8_t* pixels) {
    ut_fatal_if(stride != ((iw * 4 + 3) & ~0x3));
    ut_assert(w > 0 && h != 0); // h can be negative
    if (w > 0 && h != 0) {
        BITMAPINFOHEADER bi = ui_gdi_bgrx_init_bi(iw, ih, 4);
        POINT pt = { 0 };
        ut_fatal_win32err(SetBrushOrgEx(ui_gdi_hdc(), 0, 0, &pt));
        ut_fatal_if(StretchDIBits(ui_gdi_hdc(), sx, sy, sw, sh, x, y, w, h,
            pixels, (BITMAPINFO*)&bi, DIB_RGB_COLORS, SRCCOPY) == 0);
        ut_fatal_win32err(SetBrushOrgEx(ui_gdi_hdc(), pt.x, pt.y, &pt));
    }
}

static BITMAPINFO* ui_gdi_init_bitmap_info(int32_t w, int32_t h, int32_t bpp,
        BITMAPINFO* bi) {
    ut_assert(w > 0 && h >= 0); // h cannot be negative?
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
    ut_fatal_if(image->bitmap != null, "image_dispose() not called?");
    // not using GetWindowDC(ui_app.window) will allow to initialize images
    // before window is created
    HDC c = CreateCompatibleDC(null); // GetWindowDC(ui_app.window);
    BITMAPINFO local = { {sizeof(BITMAPINFOHEADER)} };
    BITMAPINFO* bi = bpp == 1 ? ui_gdi_greyscale_bitmap_info() : &local;
    image->bitmap = (ui_bitmap_t)CreateDIBSection(c, ui_gdi_init_bitmap_info(w, h, bpp, bi),
                                               DIB_RGB_COLORS, &image->pixels, null, 0x0);
    ut_fatal_if(image->bitmap == null || image->pixels == null);
    ut_fatal_win32err(DeleteDC(c));
}

static void ui_gdi_image_init_rgbx(ui_image_t* image, int32_t w, int32_t h,
        int32_t bpp, const uint8_t* pixels) {
    bool swapped = bpp < 0;
    bpp = abs(bpp);
    ut_fatal_if(bpp != 4, "bpp: %d", bpp);
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
    ut_fatal_if(bpp < 0 || bpp == 2 || bpp > 4, "bpp=%d not {1, 3, 4}", bpp);
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

static void ui_gdi_alpha(int32_t x, int32_t y, int32_t w, int32_t h,
        ui_image_t* image, fp64_t alpha) {
    ut_assert(image->bpp > 0);
    ut_assert(0 <= alpha && alpha <= 1);
    ut_not_null(ui_gdi_hdc());
    HDC c = CreateCompatibleDC(ui_gdi_hdc());
    ut_not_null(c);
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
    ut_fatal_win32err(AlphaBlend(ui_gdi_hdc(), x, y, w, h,
        c, 0, 0, image->w, image->h, bf));
    SelectBitmap((HDC)c, zero1x1);
    ut_fatal_win32err(DeleteDC(c));
}

static void ui_gdi_image(int32_t x, int32_t y, int32_t w, int32_t h,
        ui_image_t* image) {
    ut_assert(image->bpp == 1 || image->bpp == 3 || image->bpp == 4);
    ut_not_null(ui_gdi_hdc());
    if (image->bpp == 1) { // StretchBlt() is bad for greyscale
        BITMAPINFO* bi = ui_gdi_greyscale_bitmap_info();
        ut_fatal_if(StretchDIBits(ui_gdi_hdc(), x, y, w, h, 0, 0, image->w, image->h,
            image->pixels, ui_gdi_init_bitmap_info(image->w, image->h, 1, bi),
            DIB_RGB_COLORS, SRCCOPY) == 0);
    } else {
        HDC c = CreateCompatibleDC(ui_gdi_hdc());
        ut_not_null(c);
        HBITMAP zero1x1 = SelectBitmap(c, image->bitmap);
        ut_fatal_win32err(StretchBlt(ui_gdi_hdc(), x, y, w, h,
            c, 0, 0, image->w, image->h, SRCCOPY));
        SelectBitmap(c, zero1x1);
        ut_fatal_win32err(DeleteDC(c));
    }
}

static void ui_gdi_icon(int32_t x, int32_t y, int32_t w, int32_t h,
        ui_icon_t icon) {
    DrawIconEx(ui_gdi_hdc(), x, y, (HICON)icon, w, h, 0, NULL, DI_NORMAL | DI_COMPAT);
}

static void ui_gdi_cleartype(bool on) {
    enum { spif = SPIF_UPDATEINIFILE | SPIF_SENDCHANGE };
    ut_fatal_win32err(SystemParametersInfoA(SPI_SETFONTSMOOTHING,
                                                   true, 0, spif));
    uintptr_t s = on ? FE_FONTSMOOTHINGCLEARTYPE : FE_FONTSMOOTHINGSTANDARD;
    ut_fatal_win32err(SystemParametersInfoA(SPI_SETFONTSMOOTHINGTYPE,
        0, (void*)s, spif));
}

static void ui_gdi_font_smoothing_contrast(int32_t c) {
    ut_fatal_if(!(c == -1 || 1000 <= c && c <= 2200), "contrast: %d", c);
    if (c == -1) { c = 1400; }
    ut_fatal_win32err(SystemParametersInfoA(SPI_SETFONTSMOOTHINGCONTRAST,
        0, (void*)(uintptr_t)c, SPIF_UPDATEINIFILE | SPIF_SENDCHANGE));
}

ut_static_assertion(ui_gdi_font_quality_default == DEFAULT_QUALITY);
ut_static_assertion(ui_gdi_font_quality_draft == DRAFT_QUALITY);
ut_static_assertion(ui_gdi_font_quality_proof == PROOF_QUALITY);
ut_static_assertion(ui_gdi_font_quality_nonantialiased == NONANTIALIASED_QUALITY);
ut_static_assertion(ui_gdi_font_quality_antialiased == ANTIALIASED_QUALITY);
ut_static_assertion(ui_gdi_font_quality_cleartype == CLEARTYPE_QUALITY);
ut_static_assertion(ui_gdi_font_quality_cleartype_natural == CLEARTYPE_NATURAL_QUALITY);

static ui_font_t ui_gdi_create_font(const char* family, int32_t h, int32_t q) {
    ut_assert(h > 0);
    LOGFONTA lf = {0};
    int32_t n = GetObjectA(ui_app.fm.regular.font, sizeof(lf), &lf);
    ut_fatal_if(n != (int32_t)sizeof(lf));
    lf.lfHeight = -h;
    ut_str_printf(lf.lfFaceName, "%s", family);
    if (ui_gdi_font_quality_default <= q &&
        q <= ui_gdi_font_quality_cleartype_natural) {
        lf.lfQuality = (uint8_t)q;
    } else {
        ut_fatal_if(q != -1, "use -1 for do not care quality");
    }
    return (ui_font_t)CreateFontIndirectA(&lf);
}

static ui_font_t ui_gdi_font(ui_font_t f, int32_t h, int32_t q) {
    ut_assert(f != null && h > 0);
    LOGFONTA lf = {0};
    int32_t n = GetObjectA(f, sizeof(lf), &lf);
    ut_fatal_if(n != (int32_t)sizeof(lf));
    lf.lfHeight = -h;
    if (ui_gdi_font_quality_default <= q &&
        q <= ui_gdi_font_quality_cleartype_natural) {
        lf.lfQuality = (uint8_t)q;
    } else {
        ut_fatal_if(q != -1, "use -1 for do not care quality");
    }
    return (ui_font_t)CreateFontIndirectA(&lf);
}

static void ui_gdi_delete_font(ui_font_t f) {
    ut_fatal_win32err(DeleteFont(f));
}

// guaranteed to return dc != null even if not painting

static HDC ui_gdi_get_dc(void) {
    ut_not_null(ui_app.window);
    HDC hdc = ui_gdi_hdc() != null ?
              ui_gdi_hdc() : GetDC((HWND)ui_app.window);
    ut_not_null(hdc);
    return hdc;
}

static void ui_gdi_release_dc(HDC hdc) {
    if (ui_gdi_hdc() == null) {
        ReleaseDC((HWND)ui_app.window, hdc);
    }
}

#define ui_gdi_with_hdc(code) do {           \
    HDC hdc = ui_gdi_get_dc();               \
    code                                     \
    ui_gdi_release_dc(hdc);                  \
} while (0)

#define ui_gdi_hdc_with_font(f, ...) do {    \
    ut_not_null(f);                          \
    HDC hdc = ui_gdi_get_dc();               \
    HFONT font_ = SelectFont(hdc, (HFONT)f); \
    { __VA_ARGS__ }                          \
    SelectFont(hdc, font_);                  \
    ui_gdi_release_dc(hdc);                  \
} while (0)

static void ui_gdi_dump_hdc_fm(HDC hdc) {
    // https://en.wikipedia.org/wiki/Quad_(typography)
    // https://learn.microsoft.com/en-us/windows/win32/gdi/string-widths-and-heights
    // https://stackoverflow.com/questions/27631736/meaning-of-top-ascent-baseline-descent-bottom-and-leading-in-androids-font
    // Amazingly same since Windows 3.1 1992
    // https://learn.microsoft.com/en-us/windows/win32/api/wingdi/ns-wingdi-textmetrica
    // https://learn.microsoft.com/en-us/windows/win32/api/wingdi/ns-wingdi-outlinetextmetrica
    TEXTMETRICA tm = {0};
    ut_fatal_win32err(GetTextMetricsA(hdc, &tm));
    char pitch[64] = { 0 };
    if (tm.tmPitchAndFamily & TMPF_FIXED_PITCH) { strcat(pitch, "FIXED_PITCH "); }
    if (tm.tmPitchAndFamily & TMPF_VECTOR)      { strcat(pitch, "VECTOR "); }
    if (tm.tmPitchAndFamily & TMPF_DEVICE)      { strcat(pitch, "DEVICE "); }
    if (tm.tmPitchAndFamily & TMPF_TRUETYPE)    { strcat(pitch, "TRUETYPE "); }
    ut_println("tm: .pitch_and_family: %s", pitch);
    ut_println(".height            : %2d   .ascent (baseline) : %2d  .descent: %2d",
            tm.tmHeight, tm.tmAscent, tm.tmDescent);
    ut_println(".internal_leading  : %2d   .external_leading  : %2d  .ave_char_width: %2d",
            tm.tmInternalLeading, tm.tmExternalLeading, tm.tmAveCharWidth);
    ut_println(".max_char_width    : %2d   .weight            : %2d .overhang: %2d",
            tm.tmMaxCharWidth, tm.tmWeight, tm.tmOverhang);
    ut_println(".digitized_aspect_x: %2d   .digitized_aspect_y: %2d",
            tm.tmDigitizedAspectX, tm.tmDigitizedAspectY);
    ut_swear(tm.tmPitchAndFamily & TMPF_TRUETYPE);
    OUTLINETEXTMETRICA otm = { .otmSize = sizeof(OUTLINETEXTMETRICA) };
    uint32_t bytes = GetOutlineTextMetricsA(hdc, otm.otmSize, &otm);
    ut_swear(bytes == sizeof(OUTLINETEXTMETRICA));
    // unsupported XHeight CapEmHeight
    // ignored:    MacDescent, MacLineGap, EMSquare, ItalicAngle
    //             CharSlopeRise, CharSlopeRun, ItalicAngle
    ut_println("otm: .Ascent       : %2d   .Descent        : %2d",
            otm.otmAscent, otm.otmDescent);
    ut_println(".otmLineGap        : %2u", otm.otmLineGap);
    ut_println(".FontBox.ltrb      :  %d,%d %2d,%2d",
            otm.otmrcFontBox.left, otm.otmrcFontBox.top,
            otm.otmrcFontBox.right, otm.otmrcFontBox.bottom);
    ut_println(".MinimumPPEM       : %2u    (minimum height in pixels)",
            otm.otmusMinimumPPEM);
    ut_println(".SubscriptOffset   : %d,%d  .SubscriptSize.x   : %dx%d",
            otm.otmptSubscriptOffset.x, otm.otmptSubscriptOffset.y,
            otm.otmptSubscriptSize.x, otm.otmptSubscriptSize.y);
    ut_println(".SuperscriptOffset : %d,%d  .SuperscriptSize.x : %dx%d",
            otm.otmptSuperscriptOffset.x, otm.otmptSuperscriptOffset.y,
            otm.otmptSuperscriptSize.x,   otm.otmptSuperscriptSize.y);
    ut_println(".UnderscoreSize    : %2d   .UnderscorePosition: %2d",
            otm.otmsUnderscoreSize, otm.otmsUnderscorePosition);
    ut_println(".StrikeoutSize     : %2u   .StrikeoutPosition : %2d ",
            otm.otmsStrikeoutSize,  otm.otmsStrikeoutPosition);
    int32_t h = otm.otmAscent + abs(tm.tmDescent); // without diacritical space above
    fp32_t pts = (h * 72.0f)  / GetDeviceCaps(hdc, LOGPIXELSY);
    ut_println("height: %.1fpt", pts);
}

static void ui_gdi_dump_fm(ui_font_t f) {
    ut_not_null(f);
    ui_gdi_hdc_with_font(f, { ui_gdi_dump_hdc_fm(hdc); });
}

static void ui_gdi_get_fm(HDC hdc, ui_fm_t* fm) {
    TEXTMETRICA tm = {0};
    ut_fatal_win32err(GetTextMetricsA(hdc, &tm));
    ut_swear(tm.tmPitchAndFamily & TMPF_TRUETYPE);
    OUTLINETEXTMETRICA otm = { .otmSize = sizeof(OUTLINETEXTMETRICA) };
    uint32_t bytes = GetOutlineTextMetricsA(hdc, otm.otmSize, &otm);
    ut_swear(bytes == sizeof(OUTLINETEXTMETRICA));
    // "tm.tmAscent" The ascent (units above the base line) of characters
    // and actually is "baseline" in other terminology
    // "otm.otmAscent" The maximum distance characters in this font extend
    // above the base line. This is the typographic ascent for the font.
    // otm.otmEMSquare usually is 2048 which is size of rasterizer
    fm->height   = tm.tmHeight;
    fm->baseline = tm.tmAscent;
    fm->ascent   = otm.otmAscent;
    fm->descent  = tm.tmDescent;
    fm->baseline = tm.tmAscent;
    fm->x_height = otm.otmsXHeight;
    fm->cap_em_height = otm.otmsCapEmHeight;
    fm->internal_leading = tm.tmInternalLeading;
    fm->external_leading = tm.tmExternalLeading;
    fm->average_char_width = tm.tmAveCharWidth;
    fm->max_char_width = tm.tmMaxCharWidth;
    fm->line_gap = otm.otmLineGap;
    fm->subscript.w = otm.otmptSubscriptSize.x;
    fm->subscript.h = otm.otmptSubscriptSize.y;
    fm->subscript_offset.x = otm.otmptSubscriptOffset.x;
    fm->subscript_offset.y = otm.otmptSubscriptOffset.y;
    fm->superscript.w = otm.otmptSuperscriptSize.x;
    fm->superscript.h = otm.otmptSuperscriptSize.y;
    fm->superscript_offset.x = otm.otmptSuperscriptOffset.x;
    fm->superscript_offset.y = otm.otmptSuperscriptOffset.y;
    fm->underscore = otm.otmsUnderscoreSize;
    fm->underscore_position = otm.otmsUnderscorePosition;
    fm->strike_through = otm.otmsStrikeoutSize;
    fm->strike_through_position = otm.otmsStrikeoutPosition;
    fm->box = (ui_rect_t){
                otm.otmrcFontBox.left, otm.otmrcFontBox.top,
                otm.otmrcFontBox.right - otm.otmrcFontBox.left,
                otm.otmrcFontBox.bottom - otm.otmrcFontBox.top
    };
    // otm.Descent: The maximum distance characters in this font extend below
    // the base line. This is the typographic descent for the font.
    // Negative from the bottom (font.height)
    // tm.Descent: The descent (units below the base line) of characters.
    // Positive from the baseline down
    ut_assert(tm.tmDescent >= 0 && otm.otmDescent <= 0 &&
           -otm.otmDescent <= tm.tmDescent,
           "tm.tmDescent: %d otm.otmDescent: %d", tm.tmDescent, otm.otmDescent);
    // "Mac" typography is ignored because it's usefulness is unclear.
    // Italic angle/slant/run is ignored because at the moment edit
    // view implementation does not support italics and thus does not
    // need it. Easy to add if necessary.
};

static void ui_gdi_update_fm(ui_fm_t* fm, ui_font_t f) {
    ut_not_null(f);
    SIZE em = {0, 0}; // "m"
    *fm = (ui_fm_t){ .font = f };
//  ui_gdi.dump_fm(f);
    ui_gdi_hdc_with_font(f, {
        ui_gdi_get_fm(hdc, fm);
        // ut_glyph_nbsp and "M" have the same result
        ut_fatal_win32err(GetTextExtentPoint32A(hdc, "m", 1, &em));
        SIZE vl = {0}; // "|" Vertical Line https://www.compart.com/en/unicode/U+007C
        ut_fatal_win32err(GetTextExtentPoint32A(hdc, "|", 1, &vl));
        SIZE e3 = {0}; // Three-Em Dash
        ut_fatal_win32err(GetTextExtentPoint32A(hdc,
            ut_glyph_three_em_dash, 1, &e3));
        fm->mono = em.cx == vl.cx && vl.cx == e3.cx;
//      ut_println("vl: %d %d", vl.cx, vl.cy);
//      ut_println("e3: %d %d", e3.cx, e3.cy);
//      ut_println("fm->mono: %d height: %d baseline: %d ascent: %d descent: %d",
//              fm->mono, fm->height, fm->baseline, fm->ascent, fm->descent);
    });
    ut_assert(fm->baseline <= fm->height);
    fm->em = (ui_wh_t){ .w = fm->height, .h = fm->height };
//  ut_println("fm.em: %dx%d", fm->em.w, fm->em.h);
}

static int32_t ui_gdi_draw_utf16(ui_font_t font, const char* s, int32_t n,
        RECT* r, uint32_t format) { // ~70 microsecond Core i-7 3667U 2.0 GHz (2012)
    // if font == null, draws on HDC with selected font
if (0) {
    HDC hdc = ui_gdi_hdc();
    if (hdc != null) {
        SIZE em = {0, 0}; // "M"
        ut_fatal_win32err(GetTextExtentPoint32A(hdc, "M", 1, &em));
        ut_println("em: %d %d", em.cx, em.cy);
        ut_fatal_win32err(GetTextExtentPoint32A(hdc, ut_glyph_em_quad, 1, &em));
        ut_println("em: %d %d", em.cx, em.cy);
        SIZE vl = {0}; // "|" Vertical Line https://www.compart.com/en/unicode/U+007C
        SIZE e3 = {0}; // Three-Em Dash
        ut_fatal_win32err(GetTextExtentPoint32A(hdc, "|", 1, &vl));
        ut_println("vl: %d %d", vl.cx, vl.cy);
        ut_fatal_win32err(GetTextExtentPoint32A(hdc, ut_glyph_three_em_dash, 1, &e3));
        ut_println("e3: %d %d", e3.cx, e3.cy);
    }
}
    int32_t count = ut_str.utf16_chars(s, -1);
    ut_assert(0 < count && count < 4096, "be reasonable count: %d?", count);
    uint16_t ws[4096];
    ut_swear(count <= ut_countof(ws), "find another way to draw!");
    ut_str.utf8to16(ws, count, s, -1);
    int32_t h = 0; // return value is the height of the text
    if (font != null) {
        ui_gdi_hdc_with_font(font, { h = DrawTextW(hdc, ws, n, r, format); });
    } else { // with already selected font
        ui_gdi_with_hdc({ h = DrawTextW(hdc, ws, n, r, format); });
    }
    return h;
}

typedef struct { // draw text parameters
    const ui_fm_t* fm;
    const char* format; // format string
    va_list va;
    RECT rc;
    uint32_t flags; // flags:
    // https://docs.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-drawtextw
    // DT_CALCRECT DT_NOCLIP useful for measure
    // DT_END_ELLIPSIS useful for clipping
    // DT_LEFT, DT_RIGHT, DT_CENTER useful for paragraphs
    // DT_WORDBREAK is not good (GDI does not break nicely)
    // DT_BOTTOM, DT_VCENTER limited usability in weird cases (layout is better)
    // DT_NOPREFIX not to draw underline at "&Keyboard shortcuts
    // DT_SINGLELINE versus multiline
} ui_gdi_dtp_t;

static void ui_gdi_text_draw(ui_gdi_dtp_t* p) {
    ut_not_null(p);
    char text[4096]; // expected to be enough for single text draw
    text[0] = 0;
    ut_str.format_va(text, ut_countof(text), p->format, p->va);
    text[ut_countof(text) - 1] = 0;
    int32_t k = (int32_t)ut_str.len(text);
    if (k > 0) {
        ut_swear(k > 0 && k < ut_countof(text), "k=%d n=%d fmt=%s", k, p->format);
        // rectangle is always calculated - it makes draw text
        // much slower but UI layer is mostly uses bitmap caching:
        if ((p->flags & DT_CALCRECT) == 0) {
            // no actual drawing just calculate rectangle
            bool b = ui_gdi_draw_utf16(p->fm->font, text, -1, &p->rc, p->flags | DT_CALCRECT);
            ut_assert(b, "text_utf16(%s) failed", text); (void)b;
        }
        bool b = ui_gdi_draw_utf16(p->fm->font, text, -1, &p->rc, p->flags);
        ut_assert(b, "text_utf16(%s) failed", text); (void)b;
    } else {
        p->rc.right = p->rc.left;
        p->rc.bottom = p->rc.top + p->fm->height;
    }
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

static ui_wh_t ui_gdi_text_with_flags(const ui_gdi_ta_t* ta,
        int32_t x, int32_t y, int32_t w,
        const char* format, va_list va, uint32_t flags) {
    const int32_t right = w == 0 ? 0 : x + w;
    ui_gdi_dtp_t p = {
        .fm = ta->fm,
        .format = format,
        .va = va,
        .rc = {.left = x, .top = y, .right = right, .bottom = 0 },
        .flags = flags
    };
    ui_color_t c = ta->color;
    if (!ta->measure) {
        if (ui_color_is_undefined(c)) {
            ut_swear(ta->color_id > 0);
            c = ui_colors.get_color(ta->color_id);
        } else {
            ut_swear(ta->color_id == 0);
        }
        c = ui_gdi_set_text_color(c);
    }
    ui_gdi_text_draw(&p);
    if (!ta->measure) { ui_gdi_set_text_color(c); } // restore color
    return (ui_wh_t){ p.rc.right - p.rc.left, p.rc.bottom - p.rc.top };
}

static ui_wh_t ui_gdi_text_va(const ui_gdi_ta_t* ta,
        int32_t x, int32_t y,  const char* format, va_list va) {
    const uint32_t flags = sl_draw | (ta->measure ? sl_measure : 0);
    return ui_gdi_text_with_flags(ta, x, y, 0, format, va, flags);
}

static ui_wh_t ui_gdi_text(const ui_gdi_ta_t* ta,
        int32_t x, int32_t y, const char* format, ...) {
    const uint32_t flags = sl_draw | (ta->measure ? sl_measure : 0);
    va_list va;
    va_start(va, format);
    ui_wh_t wh = ui_gdi_text_with_flags(ta, x, y, 0, format, va, flags);
    va_end(va);
    return wh;
}

static ui_wh_t ui_gdi_multiline_va(const ui_gdi_ta_t* ta,
        int32_t x, int32_t y, int32_t w, const char* format, va_list va) {
    const uint32_t flags = ta->measure ?
                            (w <= 0 ? ml_measure : ml_measure_break) :
                            (w <= 0 ? ml_draw    : ml_draw_break);
    return ui_gdi_text_with_flags(ta, x, y, w, format, va, flags);
}

static ui_wh_t ui_gdi_multiline(const ui_gdi_ta_t* ta,
        int32_t x, int32_t y, int32_t w, const char* format, ...) {
    va_list va;
    va_start(va, format);
    ui_wh_t wh = ui_gdi_multiline_va(ta, x, y, w, format, va);
    va_end(va);
    return wh;
}

static ui_wh_t ui_gdi_glyphs_placement(const ui_gdi_ta_t* ta,
        const char* utf8, int32_t bytes, int32_t x[], int32_t glyphs) {
    ut_swear(bytes >= 0 && glyphs >= 0 && glyphs <= bytes);
    ut_assert(false, "Does not work for Tamil simplest utf8: \xe0\xae\x9a utf16: 0x0B9A");
    x[0] = 0;
    ui_wh_t wh = { .w = 0, .h = 0 };
    if (bytes > 0) {
        const int32_t chars = ut_str.utf16_chars(utf8, bytes);
        uint16_t* utf16 = ut_stackalloc((chars + 1) * sizeof(uint16_t));
        uint16_t* output = ut_stackalloc((chars + 1) * sizeof(uint16_t));
        const errno_t r = ut_str.utf8to16(utf16, chars, utf8, bytes);
        ut_swear(r == 0);
// TODO: remove
#if 1
        char str[16 * 1024] = {0};
        char hex[16 * 1024] = {0};
        for (int i = 0; i < chars; i++) {
            ut_str_printf(hex, "%04X ", utf16[i]);
            strcat(str, hex);
        }
ut_println("%.*s %s %p bytes:%d glyphs:%d font:%p hdc:%p", bytes, utf8, str, utf8, bytes, glyphs, ta->fm->font, ui_gdi_context.hdc);
#endif
        GCP_RESULTSW gcp = {
            .lStructSize = sizeof(GCP_RESULTSW),
            .lpOutString = output,
            .nGlyphs = glyphs
        };
        gcp.lpDx = (int*)ut_stackalloc((chars + 1) * sizeof(int));
        DWORD n = 0;
        const int mx = INT32_MAX; // max extent
        const DWORD f = GCP_MAXEXTENT; // |GCP_GLYPHSHAPE|GCP_DIACRITIC|GCP_LIGATE
        if (ta->fm->font != null) {
            ui_gdi_hdc_with_font(ta->fm->font, {
                n = GetCharacterPlacementW(hdc, utf16, chars, mx, &gcp, f);
            });
        } else { // with already selected font
            ui_gdi_with_hdc({
                n = GetCharacterPlacementW(hdc, utf16, chars, mx, &gcp, f);
            });
        }
        wh = (ui_wh_t){ .w = LOWORD(n), .h = HIWORD(n) };
        if (n != 0) {
            // IS_HIGH_SURROGATE(wch)
            // IS_LOW_SURROGATE(wch)
            // IS_SURROGATE_PAIR(hs, ls)
            int32_t i = 0;
            int32_t k = 1;
            while (i < chars) {
                x[k] = x[k - 1] + gcp.lpDx[i];
//              ut_println("%d", x[i]);
                k++;
                if (i < chars - 1 && ut_str.utf16_is_high_surrogate(utf16[i]) &&
                                     ut_str.utf16_is_low_surrogate(utf16[i + 1])) {
                    i += 2;
                } else {
                    i++;
                }
            }
            ut_assert(k == glyphs + 1);
        } else {
//          ut_assert(false, "GetCharacterPlacementW() failed");
            ut_println("GetCharacterPlacementW() failed");
        }
    }
    return wh;
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
        ut_fatal_if(true, "curl.exe --silent --fail --create-dirs "
            "https://raw.githubusercontent.com/nothings/stb/master/stb_image.h "
            "--output ext/stb_image.h");
        return null;
    #endif
}

static void ui_gdi_image_dispose(ui_image_t* image) {
    ut_fatal_win32err(DeleteBitmap(image->bitmap));
    memset(image, 0, sizeof(ui_image_t));
}

ui_gdi_if ui_gdi = {
    .ta = {
        .regular = {
            .color_id = ui_color_id_window_text,
            .color    = ui_color_undefined,
            .fm       = &ui_app.fm.regular,
            .measure  = false
        },
        .mono = {
            .color_id = ui_color_id_window_text,
            .color    = ui_color_undefined,
            .fm       = &ui_app.fm.mono,
            .measure  = false
        },
        .H1 = {
            .color_id = ui_color_id_window_text,
            .color    = ui_color_undefined,
            .fm       = &ui_app.fm.H1,
            .measure  = false
        },
        .H2 = {
            .color_id = ui_color_id_window_text,
            .color    = ui_color_undefined,
            .fm       = &ui_app.fm.H2,
            .measure  = false
        },
        .H3 = {
            .color_id = ui_color_id_window_text,
            .color    = ui_color_undefined,
            .fm       = &ui_app.fm.H3,
            .measure  = false
        }
    },
    .init                     = ui_gdi_init,
    .begin                    = ui_gdi_begin,
    .end                      = ui_gdi_end,
    .color_rgb                = ui_gdi_color_rgb,
    .image_init               = ui_gdi_image_init,
    .image_init_rgbx          = ui_gdi_image_init_rgbx,
    .image_dispose            = ui_gdi_image_dispose,
    .alpha                    = ui_gdi_alpha,
    .image                    = ui_gdi_image,
    .icon                     = ui_gdi_icon,
    .set_clip                 = ui_gdi_set_clip,
    .pixel                    = ui_gdi_pixel,
    .line                     = ui_gdi_line,
    .frame                    = ui_gdi_frame,
    .rect                     = ui_gdi_rect,
    .fill                     = ui_gdi_fill,
    .poly                     = ui_gdi_poly,
    .circle                   = ui_gdi_circle,
    .rounded                  = ui_gdi_rounded,
    .gradient                 = ui_gdi_gradient,
    .greyscale                = ui_gdi_greyscale,
    .bgr                      = ui_gdi_bgr,
    .bgrx                     = ui_gdi_bgrx,
    .cleartype                = ui_gdi_cleartype,
    .font_smoothing_contrast  = ui_gdi_font_smoothing_contrast,
    .create_font              = ui_gdi_create_font,
    .font                     = ui_gdi_font,
    .delete_font              = ui_gdi_delete_font,
    .dump_fm                  = ui_gdi_dump_fm,
    .update_fm                = ui_gdi_update_fm,
    .text_va                  = ui_gdi_text_va,
    .text                     = ui_gdi_text,
    .multiline_va             = ui_gdi_multiline_va,
    .multiline                = ui_gdi_multiline,
    .glyphs_placement         = ui_gdi_glyphs_placement,
    .fini                     = ui_gdi_fini
};

#pragma pop_macro("ui_gdi_hdc_with_font")
#pragma pop_macro("ui_gdi_with_hdc")
// ________________________________ ui_label.c ________________________________

#include "ut/ut.h"

static void ui_label_paint(ui_view_t* v) {
    ut_assert(v->type == ui_view_label);
    ut_assert(!ui_view.is_hidden(v));
    const char* s = ui_view.string(v);
    ui_color_t c = v->state.hover && v->highlightable ?
        ui_colors.interpolate(v->color, ui_colors.blue, 1.0f / 8.0f) :
        v->color;
    const int32_t tx = v->x + v->text.xy.x;
    const int32_t ty = v->y + v->text.xy.y;
    const ui_gdi_ta_t ta = { .fm = v->fm, .color = c };
    const bool multiline = strchr(s, '\n') != null;
    if (multiline) {
        int32_t w = (int32_t)((fp64_t)v->min_w_em * (fp64_t)v->fm->em.w + 0.5);
        ui_gdi.multiline(&ta, tx, ty, w, "%s", ui_view.string(v));
    } else {
        ui_gdi.text(&ta, tx, ty, "%s", ui_view.string(v));
    }
    if (v->state.hover && !v->flat && v->highlightable) {
        ui_color_t highlight = ui_colors.get_color(ui_color_id_highlight);
        int32_t radius = (v->fm->em.h / 4) | 0x1; // corner radius
        int32_t h = multiline ? v->h : v->fm->baseline + v->fm->descent;
        ui_gdi.rounded(v->x - radius, v->y, v->w + 2 * radius, h,
                       radius, highlight, ui_colors.transparent);
    }
}

static bool ui_label_context_menu(ui_view_t* v) {
    ut_assert(!ui_view.is_hidden(v) && !ui_view.is_disabled(v));
    const bool inside = ui_view.inside(v, &ui_app.mouse);
    if (inside) {
        ut_clipboard.put_text(ui_view.string(v));
        static ui_label_t hint = ui_label(0.0f, "copied to clipboard");
        int32_t x = v->x + v->w / 2;
        int32_t y = v->y + v->h;
        ui_app.show_hint(&hint, x, y, 0.75);
    }
    return inside;
}

static void ui_label_character(ui_view_t* v, const char* utf8) {
    ut_assert(v->type == ui_view_label);
    if (v->state.hover && !ui_view.is_hidden(v)) {
        char ch = utf8[0];
        // Copy to clipboard works for hover over text
        if ((ch == 3 || ch == 'c' || ch == 'C') && ui_app.ctrl) {
            ut_clipboard.put_text(ui_view.string(v)); // 3 is ASCII for Ctrl+C
        }
    }
}

void ui_view_init_label(ui_view_t* v) {
    ut_assert(v->type == ui_view_label);
    v->paint         = ui_label_paint;
    v->character     = ui_label_character;
    v->context_menu  = ui_label_context_menu;
    v->color_id      = ui_color_id_button_text;
    v->background_id = ui_color_id_button_face;
    v->text_align    = ui.align.left;
}

void ui_label_init_va(ui_label_t* v, fp32_t min_w_em,
        const char* format, va_list va) {
    ui_view.set_text(v, format, va);
    v->min_w_em = min_w_em;
    v->type = ui_view_label;
    ui_view_init_label(v);
}

void ui_label_init(ui_label_t* v, fp32_t min_w_em, const char* format, ...) {
    va_list va;
    va_start(va, format);
    ui_label_init_va(v, min_w_em, format, va);
    va_end(va);
}
// _______________________________ ui_layout.c ________________________________

#include "ut/ut.h"

static void measurements_center(ui_view_t* view) {
    ut_assert(view->child != null && view->child->next == view->child,
        "must be a single child parent");
    ui_view_t* c = view->child; // even if hidden measure it
    c->w = view->w;
    c->h = view->h;
}

static void measurements_horizontal(ui_view_t* view, int32_t gap) {
    ut_assert(view->child != null, "not a single child?");
    view->w = 0;
    view->h = 0;
    bool seen = false;
    ui_view_for_each(view, c, {
        if (!ui_view.is_hidden(c)) {
            if (seen) { view->w += gap; }
            view->w += c->w;
            view->h = ut_max(view->h, c->h);
            seen = true;
        }
    });
}

static void measurements_vertical(ui_view_t* view, int32_t gap) {
    ut_assert(view->child != null, "not a single child?");
    view->h = 0;
    bool seen = false;
    ui_view_for_each(view, c, {
        if (!ui_view.is_hidden(c)) {
            if (seen) { view->h += gap; }
            view->h += c->h;
            view->w = ut_max(view->w, c->w);
            seen = true;
        }
    });
}

static void measurements_grid(ui_view_t* v, int32_t gap_h, int32_t gap_v) {
    int32_t cols = 0;
    ui_view_for_each(v, r, {
        int32_t n = 0;
        ui_view_for_each(r, c, { n++; });
        if (cols == 0) { cols = n; }
        ut_assert(n > 0 && cols == n);
    });
    #pragma warning(push) // mxw[] IntelliSense confusion
    #pragma warning(disable: 6385)
    #pragma warning(disable: 6386)
    int32_t mxw[1024]; // more than enough for sane humane UI
    ut_swear(cols <= ut_countof(mxw));
    memset(mxw, 0, (size_t)cols * sizeof(int32_t));
    ui_view_for_each(v, r, {
        if (!ui_view.is_hidden(r)) {
            r->h = 0;
//          r->fm->baseline = 0;
            int32_t i = 0;
            ui_view_for_each(r, c, {
                if (!ui_view.is_hidden(c)) {
                    mxw[i] = ut_max(mxw[i], c->w);
                    r->h = ut_max(r->h, c->h);
//                  ut_println("[%d] r.fm->baseline: %d c.fm->baseline: %d ",
//                          i, r->fm->baseline, c->fm->baseline);
//                  r->fm->baseline = ut_max(r->fm->baseline, c->fm->baseline);
                }
                i++;
            });
        }
    });
    v->h = 0;
    v->w = 0;
    int32_t rows_seen = 0; // number of visible rows so far
    ui_view_for_each(v, r, {
        if (!ui_view.is_hidden(r)) {
            r->w = 0;
            int32_t i = 0;
            int32_t cols_seen = 0; // number of visible columns so far
            ui_view_for_each(v, c, {
                if (!ui_view.is_hidden(c)) {
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
                    v->w = ut_max(v->w, r->w);
                    cols_seen++;
                }
            });
            v->h += r->h;
            if (rows_seen > 0) { v->h += gap_v; }
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
    ut_assert(view->child != null && view->child->next == view->child,
        "must be a single child parent");
    ui_view_t* c = view->child;
    c->x = (view->w - c->w) / 2;
    c->y = (view->h - c->h) / 2;
}

static void layouts_horizontal(ui_view_t* view, int32_t x, int32_t y,
        int32_t gap) {
    ut_assert(view->child != null, "not a single child?");
    bool seen = false;
    ui_view_for_each(view, c, {
        if (!ui_view.is_hidden(c)) {
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
    ut_assert(view->child != null, "not a single child?");
    bool seen = false;
    ui_view_for_each(view, c, {
        if (!ui_view.is_hidden(c)) {
            if (seen) { y += gap; }
            c->x = x;
            c->y = y;
            y += c->h;
            seen = true;
        }
    });
}

static void layouts_grid(ui_view_t* view, int32_t gap_h, int32_t gap_v) {
    ut_assert(view->child != null, "not a single child?");
    int32_t x = view->x;
    int32_t y = view->y;
    bool row_seen = false;
    ui_view_for_each(view, r, {
        if (!ui_view.is_hidden(r)) {
            if (row_seen) { y += gap_v; }
            int32_t xc = x;
            bool col_seen = false;
            ui_view_for_each(r, c, {
                if (!ui_view.is_hidden(c)) {
                    if (col_seen) { xc += gap_h; }
                    c->x = xc;
                    c->y = y;
                    xc += c->w;
                    col_seen = true;
                }
            });
            y += r->h;
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
    ui_mbx_t* m = (ui_mbx_t*)b->parent;
    ut_assert(m->type == ui_view_mbx);
    m->option = -1;
    for (int32_t i = 0; i < ut_countof(m->button) && m->option < 0; i++) {
        if (b == &m->button[i]) {
            m->option = i;
            if (m->callback != null) { m->callback(&m->view); }
        }
    }
    ui_app.show_toast(null, 0);
}

static void ui_mbx_measured(ui_view_t* v) {
    ui_mbx_t* m = (ui_mbx_t*)v;
    int32_t n = 0;
    ui_view_for_each(v, c, { n++; });
    n--; // number of buttons
    const int32_t em_x = m->label.fm->em.w;
    const int32_t em_y = m->label.fm->em.h;
    const int32_t tw = m->label.w;
    const int32_t th = m->label.h;
    if (n > 0) {
        int32_t bw = 0;
        for (int32_t i = 0; i < n; i++) {
            bw += m->button[i].w;
        }
        v->w = ut_max(tw, bw + em_x * 2);
        v->h = th + m->button[0].h + em_y + em_y / 2;
    } else {
        v->h = th + em_y / 2;
        v->w = tw;
    }
}

static void ui_mbx_layout(ui_view_t* v) {
    ui_mbx_t* m = (ui_mbx_t*)v;
    int32_t n = 0;
    ui_view_for_each(v, c, { n++; });
    n--; // number of buttons
    const int32_t em_y = m->label.fm->em.h;
    m->label.x = v->x;
    m->label.y = v->y + em_y * 2 / 3;
    const int32_t tw = m->label.w;
    const int32_t th = m->label.h;
    if (n > 0) {
        int32_t bw = 0;
        for (int32_t i = 0; i < n; i++) {
            bw += m->button[i].w;
        }
        // center text:
        m->label.x = v->x + (v->w - tw) / 2;
        // spacing between buttons:
        int32_t sp = (v->w - bw) / (n + 1);
        int32_t x = sp;
        for (int32_t i = 0; i < n; i++) {
            m->button[i].x = v->x + x;
            m->button[i].y = v->y + th + em_y * 3 / 2;
            x += m->button[i].w + sp;
        }
    }
}

void ui_view_init_mbx(ui_view_t* v) {
    ui_mbx_t* m = (ui_mbx_t*)v;
    v->measured = ui_mbx_measured;
    v->layout = ui_mbx_layout;
    m->fm = &ui_app.fm.regular;
    int32_t n = 0;
    while (m->options[n] != null && n < ut_countof(m->button) - 1) {
        m->button[n] = (ui_button_t)ui_button("", 6.0, ui_mbx_button);
        ui_view.set_text(&m->button[n], "%s", m->options[n]);
        n++;
    }
    ut_swear(n <= ut_countof(m->button), "inhumane: %d buttons is too many", n);
    if (n > ut_countof(m->button)) { n = ut_countof(m->button); }
    m->label = (ui_label_t)ui_label(0, "");
    ui_view.set_text(&m->label, "%s", ui_view.string(&m->view));
    ui_view.add_last(&m->view, &m->label);
    for (int32_t i = 0; i < n; i++) {
        ui_view.add_last(&m->view, &m->button[i]);
        m->button[i].fm = m->fm;
    }
    m->label.fm = m->fm;
    ui_view.set_text(&m->view, "");
    m->option = -1;
    if (m->debug.id == null) { m->debug.id = "#mbx"; }
}

void ui_mbx_init(ui_mbx_t* m, const char* options[],
        const char* format, ...) {
    m->type = ui_view_mbx;
    m->measured  = ui_mbx_measured;
    m->layout    = ui_mbx_layout;
    m->color_id  = ui_color_id_window;
    m->options = options;
    va_list va;
    va_start(va, format);
    ui_view.set_text_va(&m->view, format, va);
    ui_label_init(&m->label, 0.0, ui_view.string(&m->view));
    va_end(va);
    ui_view_init_mbx(&m->view);
}
// _______________________________ ui_slider.c ________________________________

#include "ut/ut.h"

static void ui_slider_invalidate(const ui_slider_t* s) {
    const ui_view_t* v = &s->view;
    ui_view.invalidate(v, null);
    if (!s->dec.state.hidden) { ui_view.invalidate(&s->dec, null); }
    if (!s->inc.state.hidden) { ui_view.invalidate(&s->dec, null); }
}

static int32_t ui_slider_width(const ui_slider_t* s) {
    const ui_ltrb_t i = ui_view.margins(&s->view, &s->insets);
    int32_t w = s->w - i.left - i.right;
    if (!s->dec.state.hidden) {
        const ui_ltrb_t dec_p = ui_view.margins(&s->dec, &s->dec.padding);
        const ui_ltrb_t inc_p = ui_view.margins(&s->inc, &s->inc.padding);
        w -= s->dec.w + s->inc.w + dec_p.right + inc_p.left;
    }
    return w;
}

static ui_wh_t measure_text(const ui_fm_t* fm, const char* format, ...) {
    va_list va;
    va_start(va, format);
    const ui_gdi_ta_t ta = { .fm = fm, .color = ui_colors.white, .measure = true };
    ui_wh_t wh = ui_gdi.text_va(&ta, 0, 0, format, va);
    va_end(va);
    return wh;
}

static ui_wh_t ui_slider_measure_text(ui_slider_t* s) {
    char formatted[ut_countof(s->p.text)];
    const ui_fm_t* fm = s->fm;
    const char* text = ui_view.string(&s->view);
    const ui_ltrb_t i = ui_view.margins(&s->view, &s->insets);
    ui_wh_t mt = s->fm->em;
    if (s->debug.trace.mt) {
        const ui_ltrb_t p = ui_view.margins(&s->view, &s->padding);
        ut_println(">%dx%d em: %dx%d min: %.1fx%.1f "
                "i: %d %d %d %d p: %d %d %d %d \"%.*s\"",
            s->w, s->h, fm->em.w, fm->em.h, s->min_w_em, s->min_h_em,
            i.left, i.top, i.right, i.bottom,
            p.left, p.top, p.right, p.bottom,
            ut_min(64, strlen(text)), text);
        const ui_margins_t in = s->insets;
        const ui_margins_t pd = s->padding;
        ut_println(" i: %.3f %.3f %.3f %.3f l+r: %.3f t+b: %.3f"
                " p: %.3f %.3f %.3f %.3f l+r: %.3f t+b: %.3f",
            in.left, in.top, in.right, in.bottom,
            in.left + in.right, in.top + in.bottom,
            pd.left, pd.top, pd.right, pd.bottom,
            pd.left + pd.right, pd.top + pd.bottom);
    }
    if (s->format != null) {
        s->format(&s->view);
        ut_str_printf(formatted, "%s", text);
        mt = measure_text(s->fm, "%s", formatted);
        // TODO: format string 0x08X?
    } else if (text != null && (strstr(text, "%d") != null ||
                                strstr(text, "%u") != null)) {
        ui_wh_t mt_min = measure_text(s->fm, text, s->value_min);
        ui_wh_t mt_max = measure_text(s->fm, text, s->value_max);
        ui_wh_t mt_val = measure_text(s->fm, text, s->value);
        mt.h = ut_max(mt_val.h, ut_max(mt_min.h, mt_max.h));
        mt.w = ut_max(mt_val.w, ut_max(mt_min.w, mt_max.w));
    } else if (text != null && text[0] != 0) {
        mt = measure_text(s->fm, "%s", text);
    }
    if (s->debug.trace.mt) {
        ut_println(" mt: %dx%d", mt.w, mt.h);
    }
    return mt;
}

static void ui_slider_measure(ui_view_t* v) {
    ut_assert(v->type == ui_view_slider);
    ui_slider_t* s = (ui_slider_t*)v;
    const ui_fm_t* fm = v->fm;
    const ui_ltrb_t i = ui_view.margins(v, &v->insets);
    // slider cannot be smaller than 2*em
    const fp32_t min_w_em = ut_max(2.0f, v->min_w_em);
    v->w = (int32_t)((fp64_t)fm->em.w * (fp64_t)   min_w_em + 0.5);
    v->h = (int32_t)((fp64_t)fm->em.h * (fp64_t)v->min_h_em + 0.5);
    // dec and inc have same font metrics as a slider:
    s->dec.fm = fm;
    s->inc.fm = fm;
    ut_assert(s->dec.state.hidden == s->inc.state.hidden, "not the same");
    ui_view.measure_control(v);
//  s->text.mt = ui_slider_measure_text(s);
    if (s->dec.state.hidden) {
        v->w = ut_max(v->w, i.left + s->mt.w + i.right);
    } else {
        ui_view.measure(&s->dec); // remeasure with inherited metrics
        ui_view.measure(&s->inc);
        const ui_ltrb_t dec_p = ui_view.margins(&s->dec, &s->dec.padding);
        const ui_ltrb_t inc_p = ui_view.margins(&s->inc, &s->inc.padding);
        v->w = ut_max(v->w, s->dec.w + dec_p.right + s->mt.w + inc_p.left + s->inc.w);
    }
    v->h = ut_max(v->h, i.top + fm->em.h + i.bottom);
    if (s->debug.trace.mt) {
        ut_println("<%dx%d", s->w, s->h);
    }
}

static void ui_slider_layout(ui_view_t* v) {
    ut_assert(v->type == ui_view_slider);
    ui_slider_t* s = (ui_slider_t*)v;
    // disregard inc/dec .state.hidden bit for layout:
    const ui_ltrb_t i = ui_view.margins(v, &v->insets);
    s->dec.x = v->x + i.left;
    s->dec.y = v->y;
    s->inc.x = v->x + v->w - i.right - s->inc.w;
    s->inc.y = v->y;
}

static void ui_slider_paint(ui_view_t* v) {
    ut_assert(v->type == ui_view_slider);
    ui_slider_t* s = (ui_slider_t*)v;
    const ui_fm_t* fm = v->fm;
    const ui_ltrb_t i = ui_view.margins(v, &v->insets);
    const ui_ltrb_t dec_p = ui_view.margins(&s->dec, &s->dec.padding);
    // dec button is sticking to the left into slider padding
    const int32_t dec_w = s->dec.w + dec_p.right;
    ut_assert(s->dec.state.hidden == s->inc.state.hidden, "hidden or not together");
    const int32_t dx = s->dec.state.hidden ? 0 : dec_w;
    const int32_t x = v->x + dx + i.left;
    const int32_t w = ui_slider_width(s);
    // draw background:
    fp32_t d = ui_theme.is_app_dark() ? 0.50f : 0.25f;
    ui_color_t d0 = ui_colors.darken(v->background, d);
    d /= 4;
    ui_color_t d1 = ui_colors.darken(v->background, d);
    ui_gdi.gradient(x, v->y, w, v->h, d1, d0, true);
    // draw value:
    ui_color_t c = ui_theme.is_app_dark() ?
        ui_colors.darken(ui_colors.green, 1.0f / 128.0f) :
        ui_colors.jungle_green;
    d1 = c;
    d0 = ui_colors.darken(c, 1.0f / 64.0f);
    const fp64_t range = (fp64_t)s->value_max - (fp64_t)s->value_min;
    ut_assert(range > 0, "range: %.6f", range);
    const fp64_t  vw = (fp64_t)w * (s->value - s->value_min) / range;
    const int32_t wi = (int32_t)(vw + 0.5);
    ui_gdi.gradient(x, v->y, wi, v->h, d1, d0, true);
    if (!v->flat) {
        ui_color_t color = v->state.hover ?
            ui_colors.get_color(ui_color_id_hot_tracking) :
            ui_colors.get_color(ui_color_id_gray_text);
        if (ui_view.is_disabled(v)) { color = ui_color_rgb(30, 30, 30); } // TODO: hardcoded
        ui_gdi.frame(x, v->y, w, v->h, color);
    }
    // text:
    const char* text = ui_view.string(v);
    char formatted[ut_countof(v->p.text)];
    if (s->format != null) {
        s->format(v);
        s->p.strid = 0; // nls again
        text = ui_view.string(v);
    } else if (text != null &&
        (strstr(text, "%d") != null || strstr(text, "%u") != null)) {
        ut_str.format(formatted, ut_countof(formatted), text, s->value);
        s->p.strid = 0; // nls again
        text = ut_nls.str(formatted);
    }
    // because current value was formatted into `text` need to
    // remeasure and align text again:
    ui_view.text_measure(v, text, &v->text);
    ui_view.text_align(v, &v->text);
    const ui_color_t text_color = !v->state.hover ? v->color :
            (ui_theme.is_app_dark() ? ui_colors.white : ui_colors.black);
    const ui_gdi_ta_t ta = { .fm = fm, .color = text_color };
    ui_gdi.text(&ta, v->x + v->text.xy.x, v->y + v->text.xy.y, "%s", text);
}

static void ui_slider_mouse_click(ui_view_t* v, int32_t ut_unused(ix),
        bool pressed) {
    ui_slider_t* s = (ui_slider_t*)v;
    if (pressed) {
        const ui_ltrb_t i = ui_view.margins(v, &v->insets);
        const ui_ltrb_t dec_p = ui_view.margins(&s->dec, &s->dec.padding);
        const int32_t dec_w = s->dec.w + dec_p.right;
        ut_assert(s->dec.state.hidden == s->inc.state.hidden, "hidden or not together");
        const int32_t sw = ui_slider_width(s); // slider width
        const int32_t dx = s->dec.state.hidden ? 0 : dec_w + dec_p.right;
        const int32_t vx = v->x + i.left + dx;
        const int32_t x = ui_app.mouse.x - vx;
        const int32_t y = ui_app.mouse.y - (v->y + i.top);
        if (0 <= x && x < sw && 0 <= y && y < v->h) {
            const fp64_t range = (fp64_t)s->value_max - (fp64_t)s->value_min;
            fp64_t val = (fp64_t)x * range / (fp64_t)(sw - 1);
            int32_t vw = (int32_t)(val + s->value_min + 0.5);
            s->value = ut_min(ut_max(vw, s->value_min), s->value_max);
            if (s->callback != null) { s->callback(&s->view); }
            ui_slider_invalidate(s);
        }
    }
}

static void ui_slider_mouse_move(ui_view_t* v) {
    const ui_ltrb_t i = ui_view.margins(v, &v->insets);
    ui_slider_t* s = (ui_slider_t*)v;
    bool drag = ui_app.mouse_left || ui_app.mouse_right;
    if (drag) {
        const ui_ltrb_t dec_p = ui_view.margins(&s->dec, &s->dec.padding);
        const int32_t dec_w = s->dec.w + dec_p.right;
        ut_assert(s->dec.state.hidden == s->inc.state.hidden, "hidden or not together");
        const int32_t sw = ui_slider_width(s); // slider width
        const int32_t dx = s->dec.state.hidden ? 0 : dec_w + dec_p.right;
        const int32_t vx = v->x + i.left + dx;
        const int32_t x = ui_app.mouse.x - vx;
        const int32_t y = ui_app.mouse.y - (v->y + i.top);
        if (0 <= x && x < sw && 0 <= y && y < v->h) {
            const fp64_t range = (fp64_t)s->value_max - (fp64_t)s->value_min;
            fp64_t val = (fp64_t)x * range / (fp64_t)(sw - 1);
            int32_t vw = (int32_t)(val + s->value_min + 0.5);
            s->value = ut_min(ut_max(vw, s->value_min), s->value_max);
            if (s->callback != null) { s->callback(&s->view); }
            ui_slider_invalidate(s);
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
            if (s->callback != null) { s->callback(&s->view); }
            ui_slider_invalidate(s);
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
    ut_assert(v->type == ui_view_slider);
    ui_slider_t* s = (ui_slider_t*)v;
    if (ui_view.is_hidden(v) || ui_view.is_disabled(v)) {
        s->time = 0;
    } else if (!s->dec.state.armed && !s->inc.state.armed) {
        s->time = 0;
    } else {
        if (s->time == 0) {
            s->time = ui_app.now;
        } else if (ui_app.now - s->time > 1.0) {
            const int32_t sign = s->dec.state.armed ? -1 : +1;
            const int32_t sec = (int32_t)(ui_app.now - s->time + 0.5);
            int32_t initial = ui_app.shift && ui_app.ctrl ? 1000 :
                ui_app.shift ? 100 : ui_app.ctrl ? 10 : 1;
            int32_t mul = sec >= 1 ? initial << (sec - 1) : initial;
            const int64_t range = (int64_t)s->value_max - (int64_t)s->value_min;
            if (mul > range / 8) { mul = (int32_t)(range / 8); }
            ui_slider_inc_dec_value(s, sign, ut_max(mul, 1));
        }
    }
}

void ui_view_init_slider(ui_view_t* v) {
    ut_assert(v->type == ui_view_slider);
    v->measure       = ui_slider_measure;
    v->layout        = ui_slider_layout;
    v->paint         = ui_slider_paint;
    v->mouse_click   = ui_slider_mouse_click;
    v->mouse_move    = ui_slider_mouse_move;
    v->every_100ms   = ui_slider_every_100ms;
    v->color_id      = ui_color_id_window_text;
    v->background_id = ui_color_id_button_face;
    ui_slider_t* s = (ui_slider_t*)v;
    static const char* accel =
        " Hold key while clicking\n"
        " Ctrl: x 10 Shift: x 100 \n"
        " Ctrl+Shift: x 1000 \n for step multiplier.";
    s->dec = (ui_button_t)ui_button(ut_glyph_fullwidth_hyphen_minus, 0, // ut_glyph_heavy_minus_sign
                                    ui_slider_inc_dec);
    s->dec.fm = v->fm;
    ut_str_printf(s->dec.hint, "%s", accel);
    s->inc = (ui_button_t)ui_button(ut_glyph_fullwidth_plus_sign, 0, // ut_glyph_heavy_plus_sign
                                    ui_slider_inc_dec);
    s->inc.fm = v->fm;
    ui_view.add(&s->view, &s->dec, &s->inc, null);
    // single glyph buttons less insets look better:
    ui_view_for_each(&s->view, it, {
        it->insets.left   = 0.125f;
        it->insets.right  = 0.125f;
    });
    // inherit initial padding and insets from buttons.
    // caller may change those later and it should be accounted to
    // in measure() and layout()
    v->insets  = s->dec.insets;
    v->padding = s->dec.padding;
    s->dec.padding.right = 0.125f;
    s->dec.padding.left  = 0;
    s->inc.padding.left  = 0.125f;
    s->inc.padding.right = 0;
    s->dec.min_h_em = 1.0f + ui_view_i_tb * 2;
    s->dec.min_w_em = 1.0f + ui_view_i_tb * 2;
    s->inc.min_h_em = 1.0f + ui_view_i_tb * 2;
    s->inc.min_w_em = 1.0f + ui_view_i_tb * 2;
    ut_str_printf(s->inc.hint, "%s", accel);
    v->color_id      = ui_color_id_button_text;
    v->background_id = ui_color_id_button_face;
    if (v->debug.id == null) { v->debug.id = "#slider"; }
}

void ui_slider_init(ui_slider_t* s, const char* label, fp32_t min_w_em,
        int32_t value_min, int32_t value_max,
        void (*callback)(ui_view_t* r)) {
    static_assert(offsetof(ui_slider_t, view) == 0, "offsetof(.view)");
    if (min_w_em < 6.0) { ut_println("6.0 em minimum"); }
    s->type = ui_view_slider;
    ui_view.set_text(&s->view, "%s", label);
    s->callback = callback;
    s->min_w_em = ut_max(6.0f, min_w_em);
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

#pragma push_macro("UNICODE")
#define UNICODE // always because otherwise IME does not work

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
#include <commdlg.h>
#include <dbghelp.h>
#include <dwmapi.h>
#include <imm.h>
#include <ShellScalingApi.h>
#include <tlhelp32.h>
#include <VersionHelpers.h>
#include <windowsx.h>
#include <winnt.h>

#pragma pop_macro("UNICODE")

#pragma warning(pop)

#include <fcntl.h>

#define ut_export __declspec(dllexport)

// Win32 API BOOL -> errno_t translation

#define ut_b2e(call) ((errno_t)(call ? 0 : GetLastError()))

void ut_win32_close_handle(void* h);
/* translate ix to error */
errno_t ut_wait_ix2e(uint32_t r);


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
        ut_println("RegGetValueA(%s\\%s) failed %s", path, key, ut_strerr(r));
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
        ut_assert(ui_app.dark_mode);
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
    ut_not_null(uxtheme);
    return uxtheme;
}

static void* ui_theme_uxtheme_func(uint16_t ordinal) {
    HMODULE uxtheme = ui_theme_uxtheme();
    void* proc = (void*)GetProcAddress(uxtheme, MAKEINTRESOURCEA(ordinal));
    ut_not_null(proc);
    return proc;
}

static void ui_theme_set_preferred_app_mode(int32_t mode) {
    typedef BOOL (__stdcall *SetPreferredAppMode_t)(int32_t mode);
    SetPreferredAppMode_t SetPreferredAppMode = (SetPreferredAppMode_t)
            (SetPreferredAppMode_t)ui_theme_uxtheme_func(135);
    errno_t r = ut_b2e(SetPreferredAppMode(mode));
    // On Win11: 10.0.22631
    // SetPreferredAppMode(true) failed 0x0000047E(1150) ERROR_OLD_WIN_VERSION
    // "The specified program requires a newer version of Windows."
    if (r != 0 && r != ERROR_PROC_NOT_FOUND && r != ERROR_OLD_WIN_VERSION) {
        ut_println("SetPreferredAppMode(AllowDark) failed %s", ut_strerr(r));
    }
}

// https://stackoverflow.com/questions/75835069/dark-system-contextmenu-in-window

static void ui_theme_flush_menu_themes(void) {
    typedef BOOL (__stdcall *FlushMenuThemes_t)(void);
    FlushMenuThemes_t FlushMenuThemes = (FlushMenuThemes_t)
            (FlushMenuThemes_t)ui_theme_uxtheme_func(136);
    errno_t r = ut_b2e(FlushMenuThemes());
    // FlushMenuThemes() works but returns ERROR_OLD_WIN_VERSION
    // on newest Windows 11 but it is not documented thus no complains.
    if (r != 0 && r != ERROR_PROC_NOT_FOUND && r != ERROR_OLD_WIN_VERSION) {
        ut_println("FlushMenuThemes(AllowDark) failed %s", ut_strerr(r));
    }
}

static void ui_theme_allow_dark_mode_for_app(bool allow) {
    // https://github.com/rizonesoft/Notepad3/tree/96a48bd829a3f3192bbc93cd6944cafb3228b96d/src/DarkMode
    typedef BOOL (__stdcall *AllowDarkModeForApp_t)(bool allow);
    AllowDarkModeForApp_t AllowDarkModeForApp =
            (AllowDarkModeForApp_t)ui_theme_uxtheme_func(132);
    if (AllowDarkModeForApp != null) {
        errno_t r = ut_b2e(AllowDarkModeForApp(allow));
        if (r != 0 && r != ERROR_PROC_NOT_FOUND) {
            ut_println("AllowDarkModeForApp(true) failed %s", ut_strerr(r));
        }
    }
}

static void ui_theme_allow_dark_mode_for_window(bool allow) {
    typedef BOOL (__stdcall *AllowDarkModeForWindow_t)(HWND hWnd, bool allow);
    AllowDarkModeForWindow_t AllowDarkModeForWindow =
        (AllowDarkModeForWindow_t)ui_theme_uxtheme_func(133);
    if (AllowDarkModeForWindow != null) {
        int r = ut_b2e(AllowDarkModeForWindow((HWND)ui_app.window, allow));
        // On Win11: 10.0.22631
        // AllowDarkModeForWindow(true) failed 0x0000047E(1150) ERROR_OLD_WIN_VERSION
        // "The specified program requires a newer version of Windows."
        if (r != 0 && r != ERROR_PROC_NOT_FOUND && r != ERROR_OLD_WIN_VERSION) {
            ut_println("AllowDarkModeForWindow(true) failed %s", ut_strerr(r));
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
    ut_swear(ui_app.window != null);
    ui_theme_dark = -1;
    BOOL dark_mode = ui_theme_is_app_dark(); // must be 32-bit "BOOL"
    static const DWORD DWMWA_USE_IMMERSIVE_DARK_MODE = 20;
    /* 20 == DWMWA_USE_IMMERSIVE_DARK_MODE in Windows 11 SDK.
       This value was undocumented for Windows 10 versions 2004
       and later, supported for Windows 11 Build 22000 and later. */
    errno_t r = DwmSetWindowAttribute((HWND)ui_app.window,
        DWMWA_USE_IMMERSIVE_DARK_MODE, &dark_mode, sizeof(dark_mode));
    if (r != 0) {
        ut_println("DwmSetWindowAttribute(DWMWA_USE_IMMERSIVE_DARK_MODE) "
                "failed %s", ut_strerr(r));
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


// _______________________________ ui_toggle.c ________________________________

#include "ut/ut.h"

static void ui_toggle_paint_on_off(ui_view_t* v) {
    const ui_ltrb_t i = ui_view.margins(v, &v->insets);
    int32_t x = v->x;
    int32_t y = v->y + i.top;
    ui_color_t c = ui_colors.darken(v->background,
        !ui_theme.is_app_dark() ? 0.125f : 0.5f);
    ui_color_t b = v->state.pressed ? ui_colors.tone_green : c;
    const int32_t a = v->fm->ascent;
    const int32_t d = v->fm->descent;
    const int32_t w = v->fm->em.w;
    int32_t h = a + d;
    int32_t r = (h / 2) | 0x1; // must be odd
    h = r * 2 + 1;
    y += (v->h - i.top - i.bottom - h + 1) / 2;
    y += r;
    x += r;
    ui_color_t border = ui_theme.is_app_dark() ?
        ui_colors.darken(v->color, 0.5) :
        ui_colors.lighten(v->color, 0.5);
    if (v->state.hover) {
        border = ui_colors.get_color(ui_color_id_hot_tracking);
    }
    ui_gdi.circle(x, y, r, border, b);
    ui_gdi.circle(x + w - r, y, r, border, b);
    ui_gdi.fill(x, y - r, w - r + 1, h, b);
    ui_gdi.line(x, y - r, x + w - r + 1, y - r, border);
    ui_gdi.line(x, y + r, x + w - r + 1, y + r, border);
    int32_t x1 = v->state.pressed ? x + w - r : x;
    // circle is too bold in control color - water it down
    ui_color_t fill = ui_theme.is_app_dark() ?
        ui_colors.darken(v->color, 0.5f) : ui_colors.lighten(v->color, 0.5f);
    border = ui_theme.is_app_dark() ?
        ui_colors.darken(fill, 0.0625f) : ui_colors.lighten(fill, 0.0625f);
    ui_gdi.circle(x1, y, r - 2, border, fill);
}

static const char* ui_toggle_on_off_label(ui_view_t* v,
        char* label, int32_t count)  {
    ut_str.format(label, count, "%s", ui_view.string(v));
    char* s = strstr(label, "___");
    if (s != null) {
        memcpy(s, v->state.pressed ? "On " : "Off", 3);
    }
    return ut_nls.str(label);
}

static void ui_toggle_measure(ui_view_t* v) {
    if (v->min_w_em < 3.0f) {
        ut_println("3.0f em minimum width");
        v->min_w_em = 4.0f;
    }
    ui_view.measure_control(v);
    ut_assert(v->type == ui_view_toggle);
}

static void ui_toggle_paint(ui_view_t* v) {
    ut_assert(v->type == ui_view_toggle);
    char txt[ut_countof(v->p.text)];
    const char* label = ui_toggle_on_off_label(v, txt, ut_countof(txt));
    const char* text = ut_nls.str(label);
    ui_view.text_measure(v, text, &v->text);
    ui_view.text_align(v, &v->text);
    ui_toggle_paint_on_off(v);
    const ui_color_t text_color = !v->state.hover ? v->color :
            (ui_theme.is_app_dark() ? ui_colors.white : ui_colors.black);
    const ui_gdi_ta_t ta = { .fm = v->fm, .color = text_color };
    ui_gdi.text(&ta, v->x + v->text.xy.x, v->y + v->text.xy.y, "%s", text);
}

static void ui_toggle_flip(ui_toggle_t* t) {
    ui_view.invalidate((ui_view_t*)t, null);
    t->state.pressed = !t->state.pressed;
    if (t->callback != null) { t->callback(t); }
}

static void ui_toggle_character(ui_view_t* v, const char* utf8) {
    char ch = utf8[0];
    if (ui_view.is_shortcut_key(v, ch)) {
         ui_toggle_flip((ui_toggle_t*)v);
    }
}

static bool ui_toggle_key_pressed(ui_view_t* v, int64_t key) {
    const bool trigger = ui_app.alt && ui_view.is_shortcut_key(v, key);
    if (trigger) { ui_toggle_flip((ui_toggle_t*)v); }
    return trigger; // swallow if true
}

static void ui_toggle_mouse_click(ui_view_t* v, int32_t ut_unused(ix),
        bool pressed) {
    if (pressed && ui_view.inside(v, &ui_app.mouse)) {
        ui_toggle_flip((ui_toggle_t*)v);
    }
}

void ui_view_init_toggle(ui_view_t* v) {
    ut_assert(v->type == ui_view_toggle);
    v->mouse_click   = ui_toggle_mouse_click;
    v->paint         = ui_toggle_paint;
    v->measure       = ui_toggle_measure;
    v->character     = ui_toggle_character;
    v->key_pressed   = ui_toggle_key_pressed;
    v->color_id      = ui_color_id_button_text;
    v->background_id = ui_color_id_button_face;
    v->text_align    = ui.align.left;
    if (v->debug.id == null) { v->debug.id = "#toggle"; }
}

void ui_toggle_init(ui_toggle_t* t, const char* label, fp32_t ems,
       void (*callback)(ui_toggle_t* b)) {
    ui_view.set_text(t, "%s", label);
    t->min_w_em = ems;
    t->callback = callback;
    t->type = ui_view_toggle;
    ui_view_init_toggle(t);
}
// ________________________________ ui_view.c _________________________________

#include "ut/ut.h"

static const fp64_t ui_view_hover_delay = 1.5; // seconds

#pragma push_macro("ui_view_for_each")

static void ui_view_update_shortcut(ui_view_t* v);

// adding and removing views is not expected to be frequent
// actions by application code (human factor - UI design)
// thus extra checks and verifications are there even in
// release code because C is not type safety champion language.

static inline void ui_view_check_type(ui_view_t* v) {
    // little endian:
    ut_static_assertion(('vwXX' & 0xFFFF0000U) == ('vwZZ' & 0xFFFF0000U));
    ut_static_assertion((ui_view_stack & 0xFFFF0000U) == ('vwXX' & 0xFFFF0000U));
    ut_swear(((uint32_t)v->type & 0xFFFF0000U) == ('vwXX'  & 0xFFFF0000U),
          "not a view: %4.4s 0x%08X (forgotten &static_view?)",
          &v->type, v->type);
}

static void ui_view_verify(ui_view_t* p) {
    ui_view_check_type(p);
    ui_view_for_each(p, c, {
        ui_view_check_type(c);
        ui_view_update_shortcut(c);
        ut_swear(c->parent == p);
        ut_swear(c == c->next->prev);
        ut_swear(c == c->prev->next);
    });
}

static ui_view_t* ui_view_add(ui_view_t* p, ...) {
    va_list va;
    va_start(va, p);
    ui_view_t* c = va_arg(va, ui_view_t*);
    while (c != null) {
        ut_swear(c->parent == null && c->prev == null && c->next == null);
        ui_view.add_last(p, c);
        c = va_arg(va, ui_view_t*);
    }
    va_end(va);
    ui_view_call_init(p);
    ui_app.request_layout();
    return p;
}

static void ui_view_add_first(ui_view_t* p, ui_view_t* c) {
    ut_swear(c->parent == null && c->prev == null && c->next == null);
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
    ut_swear(c->parent == null && c->prev == null && c->next == null);
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
    ut_swear(c->parent == null && c->prev == null && c->next == null);
    ut_not_null(a->parent);
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
    ut_swear(c->parent == null && c->prev == null && c->next == null);
    ut_not_null(b->parent);
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
    ut_not_null(c->parent);
    ut_not_null(c->parent->child);
    // if a view that has focus is removed from parent:
    if (c == ui_app.focus) { ui_view.set_focus(null); }
    if (c->prev == c) {
        ut_swear(c->next == c);
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

static void ui_view_invalidate(const ui_view_t* v, const ui_rect_t* r) {
    if (ui_view.is_hidden(v)) {
        ut_println("hidden: %s", ui_view_debug_id(v));
    } else {
        ui_rect_t rc = {0};
        if (r != null) {
            rc = (ui_rect_t){
                .x = v->x + r->x,
                .y = v->y + r->y,
                .w = r->w,
                .h = r->h
            };
        } else {
            rc = (ui_rect_t){ v->x, v->y, v->w, v->h};
            // expand view rectangle by padding
            const ui_ltrb_t p = ui_view.margins(v, &v->padding);
            rc.x -= p.left;
            rc.y -= p.top;
            rc.w += p.left + p.right;
            rc.h += p.top + p.bottom;
        }
        if (v->debug.trace.prc) {
            ut_println("%d,%d %dx%d", rc.x, rc.y, rc.w, rc.h);
        }
        ui_app.invalidate(&rc);
    }
}

static const char* ui_view_string(ui_view_t* v) {
    if (v->p.strid == 0) {
        int32_t id = ut_nls.strid(v->p.text);
        v->p.strid = id > 0 ? id : -1;
    }
    return v->p.strid < 0 ? v->p.text : // not localized
        ut_nls.string(v->p.strid, v->p.text);
}

static ui_wh_t ui_view_text_metrics_va(int32_t x, int32_t y,
        bool multiline, int32_t w, const ui_fm_t* fm,
        const char* format, va_list va) {
    const ui_gdi_ta_t ta = { .fm = fm, .color = ui_colors.transparent,
                             .measure = true };
    return multiline ?
        ui_gdi.multiline_va(&ta, x, y, w, format, va) :
        ui_gdi.text_va(&ta, x, y, format, va);
}

static ui_wh_t ui_view_text_metrics(int32_t x, int32_t y,
        bool multiline, int32_t w, const ui_fm_t* fm,
        const char* format, ...) {
    va_list va;
    va_start(va, format);
    ui_wh_t wh = ui_view_text_metrics_va(x, y, multiline, w, fm, format, va);
    va_end(va);
    return wh;
}

static void ui_view_text_measure(ui_view_t* v, const char* s,
        ui_view_text_metrics_t* tm) {
    const ui_fm_t* fm = v->fm;
    tm->mt = (ui_wh_t){ .w = 0, .h = fm->height };
    if (s[0] == 0) {
        tm->multiline = false;
    } else {
        tm->multiline = strchr(s, '\n') != null;
        if (v->type == ui_view_label && tm->multiline) {
            int32_t w = (int32_t)((fp64_t)v->min_w_em * (fp64_t)fm->em.w + 0.5);
            tm->mt = ui_view.text_metrics(v->x, v->y, true,  w, fm, "%s", s);
        } else {
            tm->mt = ui_view.text_metrics(v->x, v->y, false, 0, fm, "%s", s);
        }
    }
}

static void ui_view_text_align(ui_view_t* v, ui_view_text_metrics_t* tm) {
    const ui_fm_t* fm = v->fm;
    tm->xy = (ui_point_t){ .x = -1, .y = -1 };
    const ui_ltrb_t i = ui_view.margins(v, &v->insets);
    // i_wh the inside insets w x h:
    const ui_wh_t i_wh = { .w = v->w - i.left - i.right,
                           .h = v->h - i.top - i.bottom };
    const int32_t h_align = v->text_align & ~(ui.align.top|ui.align.bottom);
    const int32_t v_align = v->text_align & ~(ui.align.left|ui.align.right);
    tm->xy.x = i.left + (i_wh.w - tm->mt.w) / 2;
    if (h_align & ui.align.left) {
        tm->xy.x = i.left;
    } else if (h_align & ui.align.right) {
        tm->xy.x = i_wh.w - tm->mt.w - i.right;
    }
    // vertical centering is trickier.
    // mt.h is height of all measured lines of text
    tm->xy.y = i.top + (i_wh.h - tm->mt.h) / 2;
    if (v_align & ui.align.top) {
        tm->xy.y = i.top;
    } else if (v_align & ui.align.bottom) {
        tm->xy.y = i_wh.h - tm->mt.h - i.bottom;
    } else if (!tm->multiline) {
        // UI controls should have x-height line in the dead center
        // of the control to be visually balanced.
        // y offset of "x-line" of the glyph:
        const int32_t y_of_x_line = fm->baseline - fm->x_height;
        // `dy` offset of the center to x-line (middle of glyph cell)
        const int32_t dy = tm->mt.h / 2 - y_of_x_line;
        tm->xy.y += dy / 2;
        if (v->debug.trace.mt) {
            ut_println(" x-line: %d mt.h: %d mt.h / 2 - x_line: %d",
                      y_of_x_line, tm->mt.h, dy);
        }
    }
}

static void ui_view_measure_control(ui_view_t* v) {
    v->p.strid = 0;
    const char* s = ui_view.string(v);
    const ui_fm_t* fm = v->fm;
    const ui_ltrb_t i = ui_view.margins(v, &v->insets);
    v->w = (int32_t)((fp64_t)fm->em.w * (fp64_t)v->min_w_em + 0.5);
    v->h = (int32_t)((fp64_t)fm->em.h * (fp64_t)v->min_h_em + 0.5);
    if (v->debug.trace.mt) {
        const ui_ltrb_t p = ui_view.margins(v, &v->padding);
        ut_println(">%dx%d em: %dx%d min: %.1fx%.1f "
                "i: %d %d %d %d p: %d %d %d %d \"%.*s\"",
            v->w, v->h, fm->em.w, fm->em.h, v->min_w_em, v->min_h_em,
            i.left, i.top, i.right, i.bottom,
            p.left, p.top, p.right, p.bottom,
            ut_min(64, strlen(s)), s);
        const ui_margins_t in = v->insets;
        const ui_margins_t pd = v->padding;
        ut_println(" i: %.3f %.3f %.3f %.3f l+r: %.3f t+b: %.3f"
                " p: %.3f %.3f %.3f %.3f l+r: %.3f t+b: %.3f",
            in.left, in.top, in.right, in.bottom,
            in.left + in.right, in.top + in.bottom,
            pd.left, pd.top, pd.right, pd.bottom,
            pd.left + pd.right, pd.top + pd.bottom);
    }
    ui_view_text_measure(v, s, &v->text);
    if (v->debug.trace.mt) {
        ut_println(" mt: %d %d", v->text.mt.w, v->text.mt.h);
    }
    v->w = ut_max(v->w, i.left + v->text.mt.w + i.right);
    v->h = ut_max(v->h, i.top  + v->text.mt.h + i.bottom);
    ui_view_text_align(v, &v->text);
    if (v->debug.trace.mt) {
        ut_println("<%dx%d text_align x,y: %d,%d",
                v->w, v->h, v->text.xy.x, v->text.xy.y);
    }
}

static void ui_view_measure_children(ui_view_t* v) {
    if (!ui_view.is_hidden(v)) {
        ui_view_for_each(v, c, { ui_view.measure(c); });
    }
}

static void ui_view_measure(ui_view_t* v) {
    if (!ui_view.is_hidden(v)) {
        ui_view_measure_children(v);
        if (v->prepare != null) { v->prepare(v); }
        if (v->measure != null && v->measure != ui_view_measure) {
            v->measure(v);
        } else {
            ui_view.measure_control(v);
        }
        if (v->measured != null) { v->measured(v); }
    }
}

static void ui_layout_view(ui_view_t* ut_unused(v)) {
//  ui_ltrb_t i = ui_view.margins(v, &v->insets);
//  ui_ltrb_t p = ui_view.margins(v, &v->padding);
//  ut_println(">%s %d,%d %dx%d p: %d %d %d %d  i: %d %d %d %d",
//               v->p.text, v->x, v->y, v->w, v->h,
//               p.left, p.top, p.right, p.bottom,
//               i.left, i.top, i.right, i.bottom);
//  ut_println("<%s %d,%d %dx%d", v->p.text, v->x, v->y, v->w, v->h);
}

static void ui_view_layout_children(ui_view_t* v) {
    if (!ui_view.is_hidden(v)) {
        ui_view_for_each(v, c, { ui_view.layout(c); });
    }
}

static void ui_view_layout(ui_view_t* v) {
//  ut_println(">%s %d,%d %dx%d", v->p.text, v->x, v->y, v->w, v->h);
    if (!ui_view.is_hidden(v)) {
        if (v->layout != null && v->layout != ui_view_layout) {
            v->layout(v);
        } else {
            ui_layout_view(v);
        }
        if (v->composed != null) { v->composed(v); }
        ui_view_layout_children(v);
    }
//  ut_println("<%s %d,%d %dx%d", v->p.text, v->x, v->y, v->w, v->h);
}

static bool ui_view_inside(const ui_view_t* v, const ui_point_t* pt) {
    const int32_t x = pt->x - v->x;
    const int32_t y = pt->y - v->y;
    return 0 <= x && x < v->w && 0 <= y && y < v->h;
}

static bool ui_view_is_parent_of(const ui_view_t* parent,
        const ui_view_t* child) {
    ut_swear(parent != null && child != null);
    const ui_view_t* p = child->parent;
    while (p != null) {
        if (parent == p) { return true; }
        p = p->parent;
    }
    return false;
}

static ui_ltrb_t ui_view_margins(const ui_view_t* v, const ui_margins_t* g) {
    const fp64_t gw = (fp64_t)g->left + (fp64_t)g->right;
    const fp64_t gh = (fp64_t)g->top  + (fp64_t)g->bottom;
    const ui_wh_t* em = &v->fm->em;
    const int32_t em_w = (int32_t)(em->w * gw + 0.5);
    const int32_t em_h = (int32_t)(em->h * gh + 0.5);
    const int32_t left = (int32_t)((fp64_t)em->w * (fp64_t)g->left + 0.5);
    const int32_t top  = (int32_t)((fp64_t)em->h * (fp64_t)g->top  + 0.5);
    return (ui_ltrb_t) {
        .left   = left,         .top    = top,
        .right  = em_w - left,  .bottom = em_h - top
    };
}

static void ui_view_inbox(const ui_view_t* v, ui_rect_t* r, ui_ltrb_t* insets) {
    ut_swear(r != null || insets != null);
    ut_swear(v->max_w >= 0 && v->max_h >= 0);
    const ui_ltrb_t i = ui_view_margins(v, &v->insets);
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

static void ui_view_outbox(const ui_view_t* v, ui_rect_t* r, ui_ltrb_t* padding) {
    ut_swear(r != null || padding != null);
    ut_swear(v->max_w >= 0 && v->max_h >= 0);
    const ui_ltrb_t p = ui_view_margins(v, &v->padding);
    if (padding != null) { *padding = p; }
    if (r != null) {
//      ut_println("%s %d,%d %dx%d %.1f %.1f %.1f %.1f", v->p.text,
//          v->x, v->y, v->w, v->h,
//          v->padding.left, v->padding.top, v->padding.right, v->padding.bottom);
        *r = (ui_rect_t) {
            .x = v->x - p.left,
            .y = v->y - p.top,
            .w = v->w + p.left + p.right,
            .h = v->h + p.top  + p.bottom,
        };
//      ut_println("%s %d,%d %dx%d", v->p.text,
//          r->x, r->y, r->w, r->h);
    }
}

static void ui_view_update_shortcut(ui_view_t* v) {
    if (ui_view.is_control(v) && v->type != ui_view_text &&
        v->shortcut == 0x00) {
        const char* s = ui_view.string(v);
        const char* a = strchr(s, '&');
        if (a != null && a[1] != 0 && a[1] != '&') {
            // TODO: utf-8 shortcuts? possible
            v->shortcut = a[1];
        }
    }
}

static void ui_view_set_text_va(ui_view_t* v, const char* format, va_list va) {
    char t[ut_countof(v->p.text)];
    ut_str.format_va(t, ut_countof(t), format, va);
    char* s = v->p.text;
    if (strcmp(s, t) != 0) {
        int32_t n = (int32_t)strlen(t);
        memcpy(s, t, (size_t)n + 1);
        v->p.strid = 0;  // next call to nls() will localize it
        ui_view_update_shortcut(v);
        ui_app.request_layout();
    }
}

static void ui_view_set_text(ui_view_t* v, const char* format, ...) {
    va_list va;
    va_start(va, format);
    ui_view.set_text_va(v, format, va);
    va_end(va);
}

static void ui_view_show_hint(ui_view_t* v, ui_view_t* hint) {
    ui_view_call_init(hint);
    ui_view.set_text(hint, v->hint);
    ui_view.measure(hint);
    int32_t x = v->x + v->w / 2 - hint->w / 2 + hint->fm->em.w / 4;
    int32_t y = v->y + v->h + hint->fm->em.h / 4;
    if (x + hint->w > ui_app.crc.w) {
        x = ui_app.crc.w - hint->w - hint->fm->em.w / 2;
    }
    if (x < 0) { x = hint->fm->em.w / 2; }
    if (y + hint->h > ui_app.crc.h) {
        y = ui_app.crc.h - hint->h - hint->fm->em.h / 2;
    }
    if (y < 0) { y = hint->fm->em.h / 2; }
    // show_tooltip will center horizontally
    ui_app.show_hint(hint, x + hint->w / 2, y, 0);
}

static void ui_view_hovering(ui_view_t* v, bool start) {
    static ui_label_t hint = ui_label(0.0, "");
    if (start && ui_app.animating.view == null && v->hint[0] != 0 &&
       !ui_view.is_hidden(v)) {
        hint.padding = (ui_margins_t){0, 0, 0, 0};
        ui_view_show_hint(v, &hint);
    } else if (!start && ui_app.animating.view == &hint) {
        ui_app.show_hint(null, -1, -1, 0);
    }
}

static bool ui_view_is_shortcut_key(ui_view_t* v, int64_t key) {
    // Supported keyboard shortcuts are ASCII characters only for now
    // If there is not focused UI control in Alt+key [Alt] is optional.
    // If there is focused control only Alt+Key is accepted as shortcut
    char ch = 0x20 <= key && key <= 0x7F ? (char)toupper((char)key) : 0x00;
    bool needs_alt = ui_app.focus != null && ui_app.focus != v &&
         !ui_view.is_parent_of(ui_app.focus, v);
    bool keyboard_shortcut = ch != 0x00 && v->shortcut != 0x00 &&
         (ui_app.alt || ui_app.ctrl || !needs_alt) && toupper(v->shortcut) == ch;
    return keyboard_shortcut;
}

static bool ui_view_is_orphan(const ui_view_t* v) {
    while (v != ui_app.root && v != null) { v = v->parent; }
    return v == null;
}

static bool ui_view_is_hidden(const ui_view_t* v) {
    bool hidden = v->state.hidden || ui_view.is_orphan(v);
    while (!hidden && v->parent != null) {
        v = v->parent;
        hidden = v->state.hidden;
    }
    return hidden;
}

static bool ui_view_is_disabled(const ui_view_t* v) {
    bool disabled = v->state.disabled;
    while (!disabled && v->parent != null) {
        v = v->parent;
        disabled = v->state.disabled;
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

static bool ui_view_key_pressed(ui_view_t* v, int64_t k) {
    bool done = false;
    if (!ui_view.is_hidden(v) && !ui_view.is_disabled(v)) {
        if (v->key_pressed != null) {
            ui_view_update_shortcut(v);
            done = v->key_pressed(v, k);
        }
        if (!done) {
            ui_view_for_each(v, c, {
                done = ui_view_key_pressed(c, k);
                if (done) { break; }
            });
        }
    }
    return done;
}

static bool ui_view_key_released(ui_view_t* v, int64_t k) {
    bool done = false;
    if (!ui_view.is_hidden(v) && !ui_view.is_disabled(v)) {
        if (v->key_released != null) {
            done = v->key_released(v, k);
        }
        if (!done) {
            ui_view_for_each(v, c, {
                done = ui_view_key_released(c, k);
                if (done) { break; }
            });
        }
    }
    return done;
}

static void ui_view_character(ui_view_t* v, const char* utf8) {
    if (!ui_view.is_hidden(v) && !ui_view.is_disabled(v)) {
        if (v->character != null) {
            ui_view_update_shortcut(v);
            v->character(v, utf8);
        }
        ui_view_for_each(v, c, { ui_view_character(c, utf8); });
    }
}

static void ui_view_resolve_color_ids(ui_view_t* v) {
    if (v->color_id > 0) {
        v->color = ui_colors.get_color(v->color_id);
    }
    if (v->background_id > 0) {
        v->background = ui_colors.get_color(v->background_id);
    }
}

static void ui_view_paint(ui_view_t* v) {
    ut_assert(ui_app.crc.w > 0 && ui_app.crc.h > 0);
    ui_view_resolve_color_ids(v);
    if (v->debug.trace.prc) {
        const char* s = ui_view.string(v);
        ut_println("%d,%d %dx%d prc: %d,%d %dx%d \"%.*s\"", v->x, v->y, v->w, v->h,
                ui_app.prc.x, ui_app.prc.y, ui_app.prc.w, ui_app.prc.h,
                ut_min(64, strlen(s)), s);
    }
    if (!v->state.hidden && ui_app.crc.w > 0 && ui_app.crc.h > 0) {
        if (v->paint != null) { v->paint(v); }
        if (v->painted != null) { v->painted(v); }
        if (v->debug.paint.margins) { ui_view.debug_paint_margins(v); }
        if (v->debug.paint.fm)   { ui_view.debug_paint_fm(v); }
        if (v->debug.paint.call && v->debug_paint != null) { v->debug_paint(v); }
        ui_view_for_each(v, c, { ui_view_paint(c); });
    }
}

static bool ui_view_has_focus(const ui_view_t* v) {
    return ui_app.focused() && ui_app.focus == v;
}

static void ui_view_set_focus(ui_view_t* v) {
    if (ui_app.focus != v) {
        ui_view_t* loosing = ui_app.focus;
        ui_view_t* gaining = v;
        if (gaining != null) {
            ut_swear(gaining->focusable && !ui_view.is_hidden(gaining) &&
                                        !ui_view.is_disabled(gaining));
        }
        if (loosing != null) { ut_swear(loosing->focusable); }
        ui_app.focus = v;
        if (loosing != null && loosing->focus_lost != null) {
            loosing->focus_lost(loosing);
        }
        if (gaining != null && gaining->focus_gained != null) {
            gaining->focus_gained(gaining);
        }
    }
}

static int64_t ui_view_hit_test(const ui_view_t* v, ui_point_t pt) {
    int64_t ht = ui.hit_test.nowhere;
    if (!ui_view.is_hidden(v) && v->hit_test != null) {
         ht = v->hit_test(v, pt);
    }
    if (ht == ui.hit_test.nowhere) {
        ui_view_for_each(v, c, {
            if (!c->state.hidden && ui_view.inside(c, &pt)) {
                ht = ui_view_hit_test(c, pt);
                if (ht != ui.hit_test.nowhere) { break; }
            }
        });
    }
    return ht;
}

static void ui_view_update_hover(ui_view_t* v, bool hidden) {
    const bool hover  = v->state.hover;
    const bool inside = ui_view.inside(v, &ui_app.mouse);
    v->state.hover = !ui_view.is_hidden(v) && inside;
    if (hover != v->state.hover) {
//      ut_println("hover := %d %p %s", v->state.hover, v, ui_view_debug_id(v));
        ui_view.hover_changed(v); // even for hidden
        if (!hidden) { ui_view.invalidate(v, null); }
    }
}

static void ui_view_mouse_hover(ui_view_t* v) {
//  ut_println("%d,%d %s", ui_app.mouse.x, ui_app.mouse.y,
//          ui_app.mouse_left  ? "L" : "_",
//          ui_app.mouse_right ? "R" : "_");
    // mouse hover over is dispatched even to disabled views
    const bool hidden = ui_view.is_hidden(v);
    ui_view_update_hover(v, hidden);
    if (!hidden && v->mouse_hover != null) { v->mouse_hover(v); }
    ui_view_for_each(v, c, { ui_view_mouse_hover(c); });
}

static void ui_view_mouse_move(ui_view_t* v) {
//  ut_println("%d,%d %s", ui_app.mouse.x, ui_app.mouse.y,
//          ui_app.mouse_left  ? "L" : "_",
//          ui_app.mouse_right ? "R" : "_");
    // mouse move is dispatched even to disabled views
    const bool hidden = ui_view.is_hidden(v);
    ui_view_update_hover(v, hidden);
    if (!hidden && v->mouse_move != null) { v->mouse_move(v); }
    ui_view_for_each(v, c, { ui_view_mouse_move(c); });
}

static void ui_view_double_click(ui_view_t* v, int32_t ix) {
    if (!ui_view.is_hidden(v) && !ui_view.is_disabled(v)) {
        const bool inside = ui_view.inside(v, &ui_app.mouse);
        if (inside) {
            if (v->focusable) { ui_view.set_focus(v); }
            if (v->double_click != null) { v->double_click(v, ix); }
        }
        ui_view_for_each(v, c, { ui_view_double_click(c, ix); });
    }
}

static void ui_view_mouse_click(ui_view_t* v, int32_t ix, bool pressed) {
    if (!ui_view.is_hidden(v) && !ui_view.is_disabled(v)) {
        ui_view_for_each(v, c, { ui_view_mouse_click(c, ix, pressed); });
        const bool inside = ui_view.inside(v, &ui_app.mouse);
        if (inside) {
            if (v->focusable) { ui_view.set_focus(v); }
            if (v->mouse_click != null) { v->mouse_click(v, ix, pressed); }
        }
    }
}

static void ui_view_mouse_scroll(ui_view_t* v, ui_point_t dx_dy) {
    if (!ui_view.is_hidden(v) && !ui_view.is_disabled(v)) {
        if (v->mouse_scroll != null) { v->mouse_scroll(v, dx_dy); }
        ui_view_for_each(v, c, { ui_view_mouse_scroll(c, dx_dy); });
    }
}

static void ui_view_hover_changed(ui_view_t* v) {
    if (!v->state.hidden) {
        if (!v->state.hover) {
            v->p.hover_when = 0;
            ui_view.hovering(v, false); // cancel hover
        } else {
            ut_swear(ui_view_hover_delay >= 0);
            if (v->p.hover_when >= 0) {
                v->p.hover_when = ui_app.now + ui_view_hover_delay;
            }
        }
    }
}

static void ui_view_lose_hidden_focus(ui_view_t* v) {
    // removes focus from hidden or disabled ui controls
    if (ui_app.focus != null) {
        if (ui_app.focus == v && (v->state.disabled || v->state.hidden)) {
            ui_view.set_focus(null);
        } else {
            ui_view_for_each(v, c, {
                if (ui_app.focus != null) { ui_view_lose_hidden_focus(c); }
            });
        }
    }
}

static bool ui_view_tap(ui_view_t* v, int32_t ix, bool pressed) {
    bool swallow = false; // consumed
    if (!ui_view.is_hidden(v) && !ui_view.is_disabled(v)) {
        ui_view_for_each(v, c, {
            swallow = ui_view_tap(c, ix, pressed);
            if (swallow) { break; }
        });
        const bool inside = ui_view.inside(v, &ui_app.mouse);
        if (!swallow && inside) {
            if (v->focusable) { ui_view.set_focus(v); }
            if (v->tap != null) { swallow = v->tap(v, ix, pressed); }
        }
    }
    return swallow;
}

static bool ui_view_long_press(ui_view_t* v, int32_t ix) {
    bool swallow = false; // consumed
    if (!ui_view.is_hidden(v) && !ui_view.is_disabled(v)) {
        ui_view_for_each(v, c, {
            swallow = ui_view_long_press(c, ix);
            if (swallow) { break; }
        });
        const bool inside = ui_view.inside(v, &ui_app.mouse);
        if (!swallow && inside && v->long_press != null) {
            swallow = v->long_press(v, ix);
        }
    }
    return swallow;
}

static bool ui_view_double_tap(ui_view_t* v, int32_t ix) { // 0: left 1: middle 2: right
    bool swallow = false; // consumed
    if (!ui_view.is_hidden(v) && !ui_view.is_disabled(v)) {
        ui_view_for_each(v, c, {
            swallow = ui_view_double_tap(c, ix);
            if (swallow) { break; }
        });
        const bool inside = ui_view.inside(v, &ui_app.mouse);
        if (!swallow && inside && v->double_tap != null) {
            swallow = v->double_tap(v, ix);
        }
    }
    return swallow;
}

static bool ui_view_context_menu(ui_view_t* v) {
    bool swallow = false;
    if (!ui_view.is_hidden(v) && !ui_view.is_disabled(v)) {
        ui_view_for_each(v, c, {
            swallow = ui_view_context_menu(c);
            if (swallow) { break; }
        });
        const bool inside = ui_view.inside(v, &ui_app.mouse);
        if (!swallow && inside && v->context_menu != null) {
            swallow = v->context_menu(v);
        }
    }
    return swallow;
}

static bool ui_view_message(ui_view_t* view, int32_t m, int64_t wp, int64_t lp,
        int64_t* ret) {
    if (!view->state.hidden) {
        if (view->p.hover_when > 0 && ui_app.now > view->p.hover_when) {
            view->p.hover_when = -1; // "already called"
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

static bool ui_view_is_container(const ui_view_t* v) {
    return  v->type == ui_view_stack ||
            v->type == ui_view_span  ||
            v->type == ui_view_list;
}

static bool ui_view_is_spacer(const ui_view_t* v) {
    return  v->type == ui_view_spacer;
}

static bool ui_view_is_control(const ui_view_t* v) {
    return  v->type == ui_view_text   ||
            v->type == ui_view_label  ||
            v->type == ui_view_toggle ||
            v->type == ui_view_button ||
            v->type == ui_view_slider ||
            v->type == ui_view_mbx;
}

static void ui_view_debug_paint_margins(ui_view_t* v) {
    if (v->debug.paint.margins) {
        if (v->type == ui_view_spacer) {
            ui_gdi.fill(v->x, v->y, v->w, v->h, ui_color_rgb(128, 128, 128));
        }
        const ui_ltrb_t p = ui_view.margins(v, &v->padding);
        const ui_ltrb_t i = ui_view.margins(v, &v->insets);
        ui_color_t c = ui_colors.green;
        const int32_t pl = p.left;
        const int32_t pr = p.right;
        const int32_t pt = p.top;
        const int32_t pb = p.bottom;
        if (pl > 0) { ui_gdi.frame(v->x - pl, v->y, pl, v->h, c); }
        if (pr > 0) { ui_gdi.frame(v->x + v->w, v->y, pr, v->h, c); }
        if (pt > 0) { ui_gdi.frame(v->x, v->y - pt, v->w, pt, c); }
        if (p.bottom > 0) {
            ui_gdi.frame(v->x, v->y + v->h, v->w, pb, c);
        }
        c = ui_colors.orange;
        const int32_t il = i.left;
        const int32_t ir = i.right;
        const int32_t it = i.top;
        const int32_t ib = i.bottom;
        if (il > 0) { ui_gdi.frame(v->x, v->y, il, v->h, c); }
        if (ir > 0) { ui_gdi.frame(v->x + v->w - ir, v->y, ir, v->h, c); }
        if (it > 0) { ui_gdi.frame(v->x, v->y, v->w, it, c); }
        if (ib > 0) { ui_gdi.frame(v->x, v->y + v->h - ib, v->w, ib, c); }
        if ((ui_view.is_container(v) || ui_view.is_spacer(v)) &&
            v->w > 0 && v->h > 0 && v->color != ui_colors.transparent) {
            ui_wh_t mt = ui_view_text_metrics(v->x, v->y, false, 0,
                                              v->fm, "%s", ui_view.string(v));
            const int32_t tx = v->x + (v->w - mt.w) / 2;
            const int32_t ty = v->y + (v->h - mt.h) / 2;
            c = ui_color_is_rgb(v->color) ^ 0xFFFFFF;
            const ui_gdi_ta_t ta = { .fm = v->fm, .color = c };
            ui_gdi.text(&ta, tx, ty, "%s", ui_view.string(v));
        }
    }
}

static void ui_view_debug_paint_fm(ui_view_t* v) {
    if (v->debug.paint.fm && v->p.text[0] != 0 &&
       !ui_view_is_container(v) && !ui_view_is_spacer(v)) {
        const ui_point_t t = v->text.xy;
        const int32_t x = v->x;
        const int32_t y = v->y;
        const int32_t w = v->w;
        const int32_t y_0 = y + t.y;
        const int32_t y_b = y_0 + v->fm->baseline;
        const int32_t y_a = y_b - v->fm->ascent;
        const int32_t y_h = y_0 + v->fm->height;
        const int32_t y_x = y_b - v->fm->x_height;
        const int32_t y_d = y_b + v->fm->descent;
        // fm.height y == 0 line is painted one pixel higher:
        ui_gdi.line(x, y_0 - 1, x + w, y_0 - 1, ui_colors.red);
        ui_gdi.line(x, y_a, x + w, y_a, ui_colors.green);
        ui_gdi.line(x, y_x, x + w, y_x, ui_colors.orange);
        ui_gdi.line(x, y_b, x + w, y_b, ui_colors.red);
        ui_gdi.line(x, y_d, x + w, y_d, ui_colors.green);
        if (y_h != y_d) {
            ui_gdi.line(x, y_d, x + w, y_d, ui_colors.green);
            ui_gdi.line(x, y_h, x + w, y_h, ui_colors.red);
        } else {
            ui_gdi.line(x, y_h, x + w, y_h, ui_colors.orange);
        }
        // fm.height line painted _under_ the actual height
    }
}

#pragma push_macro("ui_view_no_siblings")

#define ui_view_no_siblings(v) do {                    \
    ut_swear((v)->parent == null && (v)->child == null && \
          (v)->prev == null && (v)->next == null);     \
} while (0)

static void ui_view_test(void) {
    ui_view_t p0 = ui_view(stack);
    ui_view_t c1 = ui_view(stack);
    ui_view_t c2 = ui_view(stack);
    ui_view_t c3 = ui_view(stack);
    ui_view_t c4 = ui_view(stack);
    ui_view_t g1 = ui_view(stack);
    ui_view_t g2 = ui_view(stack);
    ui_view_t g3 = ui_view(stack);
    ui_view_t g4 = ui_view(stack);
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
    ut_swear(p0.child == &c1);
    ui_view.add_last(&p0, &c4);                     ui_view_verify(&p0);
    ut_swear(p0.child == &c1 && p0.child->prev == &c4);
    ui_view.add_after(&c2, &c1);                    ui_view_verify(&p0);
    ut_swear(p0.child == &c1);
    ut_swear(c1.next == &c2);
    ui_view.add_before(&c3, &c4);                   ui_view_verify(&p0);
    ut_swear(p0.child == &c1);
    ut_swear(c4.prev == &c3);
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
    if (ut_debug.verbosity.level > ut_debug.verbosity.quiet) { ut_println("done"); }
}

#pragma pop_macro("ui_view_no_siblings")

ui_view_if ui_view = {
    .add                 = ui_view_add,
    .add_first           = ui_view_add_first,
    .add_last            = ui_view_add_last,
    .add_after           = ui_view_add_after,
    .add_before          = ui_view_add_before,
    .remove              = ui_view_remove,
    .remove_all          = ui_view_remove_all,
    .disband             = ui_view_disband,
    .inside              = ui_view_inside,
    .is_parent_of        = ui_view_is_parent_of,
    .margins             = ui_view_margins,
    .inbox               = ui_view_inbox,
    .outbox              = ui_view_outbox,
    .set_text            = ui_view_set_text,
    .set_text_va         = ui_view_set_text_va,
    .invalidate          = ui_view_invalidate,
    .text_metrics_va     = ui_view_text_metrics_va,
    .text_metrics        = ui_view_text_metrics,
    .text_measure        = ui_view_text_measure,
    .text_align          = ui_view_text_align,
    .measure_control     = ui_view_measure_control,
    .measure_children    = ui_view_measure_children,
    .layout_children     = ui_view_layout_children,
    .measure             = ui_view_measure,
    .layout              = ui_view_layout,
    .string              = ui_view_string,
    .is_orphan           = ui_view_is_orphan,
    .is_hidden           = ui_view_is_hidden,
    .is_disabled         = ui_view_is_disabled,
    .is_control          = ui_view_is_control,
    .is_container        = ui_view_is_container,
    .is_spacer           = ui_view_is_spacer,
    .timer               = ui_view_timer,
    .every_sec           = ui_view_every_sec,
    .every_100ms         = ui_view_every_100ms,
    .hit_test            = ui_view_hit_test,
    .key_pressed         = ui_view_key_pressed,
    .key_released        = ui_view_key_released,
    .character           = ui_view_character,
    .paint               = ui_view_paint,
    .has_focus           = ui_view_has_focus,
    .set_focus           = ui_view_set_focus,
    .lose_hidden_focus   = ui_view_lose_hidden_focus,
    .mouse_hover         = ui_view_mouse_hover,
    .mouse_move          = ui_view_mouse_move,
    .mouse_click         = ui_view_mouse_click,
    .double_click        = ui_view_double_click,
    .mouse_scroll        = ui_view_mouse_scroll,
    .hovering            = ui_view_hovering,
    .hover_changed       = ui_view_hover_changed,
    .is_shortcut_key     = ui_view_is_shortcut_key,
    .context_menu        = ui_view_context_menu,
    .tap                 = ui_view_tap,
    .long_press          = ui_view_long_press,
    .double_tap          = ui_view_double_tap,
    .message             = ui_view_message,
    .debug_paint_margins = ui_view_debug_paint_margins,
    .debug_paint_fm      = ui_view_debug_paint_fm,
    .test                = ui_view_test
};

#ifdef UI_VIEW_TEST

ut_static_init(ui_view) {
    ui_view.test();
}

#endif

#pragma pop_macro("ui_view_for_each")

#endif // ui_implementation

