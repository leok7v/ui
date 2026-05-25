#pragma once
#include "posix/posix.h"

posix_begin_c

// Graphic Device Interface (selected parts of Windows GDI)

enum {  // TODO: into gdi int32_t const
    ui_font_quality_default = 0,
    ui_font_quality_draft = 1,
    ui_font_quality_proof = 2, // anti-aliased w/o ClearType rainbows
    ui_font_quality_nonantialiased = 3,
    ui_font_quality_antialiased = 4,
    ui_font_quality_cleartype = 5,
    ui_font_quality_cleartype_natural = 6
};

struct ui_fm { // font metrics
    ui_font_t font;
    struct ui_wh em;        // "em" square point size expressed in pixels *)
    // https://learn.microsoft.com/en-us/windows/win32/gdi/string-widths-and-heights
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
    struct ui_wh subscript; // height
    struct ui_point subscript_offset;
    struct ui_wh superscript;    // height
    struct ui_point superscript_offset;
    int32_t underscore;     // height
    int32_t underscore_position;
    int32_t strike_through; // height
    int32_t strike_through_position;
    int32_t design_units_per_em; // aka EM square ~ 2048
    struct ui_rect box; // bounding box of the glyphs in design units
    bool mono;
};

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

struct ui_ta { // text attributes
    const struct ui_fm* fm; // font metrics
    int32_t color_id;  // <= 0 use color
    ui_color_t color;  // ui_colors.undefined() use color_id
    bool measure;      // measure only do not draw
};

struct ui_draw_if {
    struct {
        struct {
            struct ui_ta const normal;
            struct ui_ta const title;
            struct ui_ta const rubric;
            struct ui_ta const H1;
            struct ui_ta const H2;
            struct ui_ta const H3;
        } prop;
        struct {
            struct ui_ta const normal;
            struct ui_ta const title;
            struct ui_ta const rubric;
            struct ui_ta const H1;
            struct ui_ta const H2;
            struct ui_ta const H3;
        } mono;
    } const ta;
    void (*init)(void);
    void (*fini)(void);
    void (*begin)(struct ui_bitmap* bitmap_or_null);
    // all paint must be done in between
    void (*end)(void);
    // TODO: move to ui_colors
    uint32_t (*color_rgb)(ui_color_t c); // rgb color
    // bpp bytes (not bits!) per pixel. bpp = -3 or -4 does not swap RGB to BRG:
    void (*bitmap_init)(struct ui_bitmap* bitmap, int32_t w, int32_t h, int32_t bpp,
        const uint8_t* pixels);
    void (*bitmap_init_rgbx)(struct ui_bitmap* bitmap, int32_t w, int32_t h,
        int32_t bpp, const uint8_t* pixels); // sets all alphas to 0xFF
    void (*bitmap_dispose)(struct ui_bitmap* bitmap);
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
    void (*poly)(struct ui_point* points, int32_t count, ui_color_t c);
    void (*circle)(int32_t center_x, int32_t center_y, int32_t odd_radius,
        ui_color_t border, ui_color_t fill);
    void (*rounded)(int32_t x, int32_t y, int32_t w, int32_t h,
        int32_t odd_radius, ui_color_t border, ui_color_t fill);
    void (*gradient)(int32_t x, int32_t y, int32_t w, int32_t h,
        ui_color_t rgba_from, ui_color_t rgba_to, bool vertical);
    // dx, dy, dw, dh destination rectangle
    // ix, iy, iw, ih rectangle inside pixels[height][width]
    void (*pixels)(int32_t dx, int32_t dy, int32_t dw, int32_t dh,
        int32_t ix, int32_t iy, int32_t iw, int32_t ih,
        int32_t width, int32_t height, int32_t stride,
        int32_t bpp, const uint8_t* pixels); // bytes per pixel
    void (*greyscale)(int32_t dx, int32_t dy, int32_t dw, int32_t dh,
        int32_t ix, int32_t iy, int32_t iw, int32_t ih,
        int32_t width, int32_t height, int32_t stride, const uint8_t* pixels);
    void (*bgr)(int32_t dx, int32_t dy, int32_t dw, int32_t dh,
        int32_t ix, int32_t iy, int32_t iw, int32_t ih,
        int32_t width, int32_t height, int32_t stride, const uint8_t* pixels);
    void (*bgrx)(int32_t dx, int32_t dy, int32_t dw, int32_t dh,
        int32_t x, int32_t y, int32_t w, int32_t h,
        int32_t width, int32_t height, int32_t stride, const uint8_t* pixels);
    // alpha() blend only works with device allocated bitmaps
    void (*alpha)(int32_t dx, int32_t dy, int32_t dw, int32_t dh,
        int32_t ix, int32_t iy, int32_t iw, int32_t ih,
        struct ui_bitmap* bitmap, fp64_t alpha); // alpha blend
    // bitmap() only works with device allocated bitmaps
    void (*bitmap)(int32_t dx, int32_t dy, int32_t dw, int32_t dh,
        int32_t ix, int32_t iy, int32_t iw, int32_t ih,
        struct ui_bitmap* bitmap);
    void (*icon)(int32_t dx, int32_t dy, int32_t dw, int32_t dh,
        ui_icon_t icon);
    // text:
    void (*cleartype)(bool on); // system wide change: don't use
    void (*font_smoothing_contrast)(int32_t c); // [1000..2202] or -1 for 1400 default
    ui_font_t (*create_font)(const char* family, int32_t height, int32_t quality);
    // custom font, quality: -1 "as is"
    ui_font_t (*font)(ui_font_t f, int32_t height, int32_t quality);
    void      (*delete_font)(ui_font_t f);
    void (*dump_fm)(ui_font_t f); // dump font metrics
    void (*update_fm)(struct ui_fm* fm, ui_font_t f); // fills font metrics
    struct ui_wh (*text_va)(const struct ui_ta* ta, int32_t x, int32_t y,
        const char* format, va_list va);
    struct ui_wh (*text)(const struct ui_ta* ta, int32_t x, int32_t y,
        const char* format, ...);
    struct ui_wh (*multiline_va)(const struct ui_ta* ta, int32_t x, int32_t y,
        int32_t w, const char* format, va_list va); // "w" can be zero
    struct ui_wh (*multiline)(const struct ui_ta* ta, int32_t x, int32_t y,
        int32_t w, const char* format, ...);
    // x[posix_str.glyphs(utf8, bytes)] = {x0, x1, x2, ...}
    struct ui_wh (*glyphs_placement)(const struct ui_ta* ta, const char* utf8,
        int32_t bytes, int32_t x[/*glyphs + 1*/], int32_t glyphs);
};

extern struct ui_draw_if ui_draw;

posix_end_c
