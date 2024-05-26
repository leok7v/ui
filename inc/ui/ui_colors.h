#pragma once
#include "ut/ut_std.h"

begin_c

typedef uint64_t ui_color_t; // top 2 bits determine color format

/* TODO: make ui_color_t uint64_t RGBA remove pens and brushes
         support upto 16-16-16-15(A)bit per pixel color
         components with 'transparent/hollow' bit
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

end_c
