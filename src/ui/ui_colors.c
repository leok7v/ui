#include "ut/ut.h"
#include "ui/ui.h"

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
        default: rt_swear(false); break;
    }
    rt_assert(0 <= r && r <= 255);
    rt_assert(0 <= g && g <= 255);
    rt_assert(0 <= b && b <= 255);
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
    rt_assert(0.0f < multiplier && multiplier < 1.0f);
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
    rt_swear(0 < color_id && color_id < rt_countof(ui_theme_colors));
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

