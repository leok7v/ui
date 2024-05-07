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

end_c
