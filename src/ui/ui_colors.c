#include "ut/ut.h"
#include "ui/ui.h"

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
    .dkgray1 = rgb(30, 30, 30),
    .dkgray2 = rgb(37, 38, 38),
    .dkgray3 = rgb(45, 45, 48),
    .dkgray4 = _colors_dkgray4,
    // tone down RGB colors:
    .tone_white   = rgb(164, 164, 164),
    .tone_red     = rgb(192,  64,  64),
    .tone_green   = rgb(64,  192,  64),
    .tone_blue    = rgb(64,   64, 192),
    .tone_yellow  = rgb(192, 192,  64),
    .tone_cyan    = rgb(64,  192, 192),
    .tone_magenta = rgb(192,  64, 192),
    // miscelaneous:
    .orange           = rgb(255, 165, 0), // 0xFFA500
    .dkgreen          = rgb(1, 50, 32),   // 0x013220
    .pink             = 0xFFC0CB,
    .ochre            = 0xCC7722,
    .gold             = 0xFFD700,
    .teal             = 0x008080,
    .wheat            = 0xF5DEB3,
    .tan              = 0xD2B48C,
    .brown            = 0xA52A2A,
    .maroon           = 0x800000,
    .barbie_pink      = 0xE0218A,
    .steel_pink       = 0xCC33CC,
    .salmon_pink      = 0xFF91A4,
    .gainsboro        = 0xDCDCDC, // rgb(220, 220, 220)
    .light_gray       = 0xD3D3D3, // rgb(211, 211, 211)
    .silver	          = 0xC0C0C0, // rgb(192, 192, 192)
    .dark_gray        = 0xA9A9A9, // rgb(169, 169, 169)
    .dim_gray         = 0x696969, // rgb(105, 105, 105)
    .light_slate_gray = 0x778899, // rgb(119, 136, 153)
    .slate_gray	      = 0x708090, // rgb(112, 128, 144)

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
    .toast = rgb(8, 40, 24), // toast background

    /* Main Panel Backgrounds */
    .charcoal                   = 0x36454F,
    .onyx                       = 0x353839,
    .gunmetal                   = 0x2A3439,
    .jet_black                  = 0x343434,
    .outer_space                = 0x414A4C,
    .eerie_black                = 0x1B1B1B,
    .oil                        = 0x3B3C36,
    .black_coral                = 0x54626F,

    /* Secondary Panels or Sidebars */
    .raisin_black               = 0x272635,
    .dark_charcoal              = 0x303030,
    .dark_jungle_green          = 0x1A2421,
    .pine_tree                  = 0x2A2F23,
    .rich_black                 = 0x004040,
    .eclipse                    = 0x3F3939,
    .cafe_noir                  = 0x4B3621,

    /* Flat Buttons */
    .prussian_blue              = 0x003153,
    .midnight_green             = 0x004953,
    .charleston_green           = 0x232B2B,
    .rich_black_fogra           = 0x0A0F0D,
    .dark_liver                 = 0x534B4F,
    .dark_slate_gray            = 0x2F4F4F,
    .black_olive                = 0x3B3C36,
    .cadet                      = 0x536872,

    /* Button highlights (hover) */
    .dark_sienna                = 0x3C1414,
    .bistre_brown               = 0x967117,
    .dark_puce                  = 0x4F3A3C,
    .wenge                      = 0x645452,

    /* Raised button effects */
    .dark_scarlet               = 0x560319,
    .burnt_umber                = 0x8A3324,
    .caput_mortuum              = 0x592720,
    .barn_red                   = 0x7C0A02,
};
