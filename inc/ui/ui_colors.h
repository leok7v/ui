#pragma once
#include "ut/ut_std.h"

begin_c

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

typedef struct ui_colors_s {
    ui_color_t (*get_color)(int32_t color_id); // ui.colors.*
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
    // darker shades of grey:
    ui_color_t const dkgray1; // 30 / 255 = 11.7%
    ui_color_t const dkgray2; // 38 / 255 = 15%
    ui_color_t const dkgray3; // 45 / 255 = 17.6%
    ui_color_t const dkgray4; // 63 / 255 = 24.0%
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
    ui_color_t const dkgreen;
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
    // highlights:
    ui_color_t const text_highlight; // bluish off-white
    ui_color_t const blue_highlight;
    ui_color_t const off_white;
    // button and other UI colors
    ui_color_t const btn_gradient_darker;
    ui_color_t const btn_gradient_dark;
    ui_color_t const btn_hover_highlight;
    ui_color_t const btn_disabled;
    ui_color_t const btn_armed;
    ui_color_t const btn_text;
    ui_color_t const toast; // toast background
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

end_c
