#include "ut/ut.h"
#include "ui/ui.h"

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

    .btn_gradient_darker = _colors_dkgray0,
    .btn_gradient_dark   = _colors_dkgray4,
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
