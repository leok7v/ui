/* Copyright (c) Dmitry "Leo" Kuznetsov 2021-24 see LICENSE for details */
#include "ut/ut.h"
#include "ui/ui.h"

static char title[128] = "Sample1: Hello";

static ui_font_t font;

static const char* locales[] = { // 123 languages
    "af-ZA", "am-ET", "ar-SA", "as-IN", "az-AZ", "ba-RU", "be-BY", "bg-BG",
    "bn-BD", "bo-CN", "br-FR", "bs-BA", "bs-Latn-BA", "ca-ES", "ca-ES-Valencia",
    "cs-CZ", "cy-GB", "da-DK", "de-DE", "dv-MV", "el-GR", "en-US",
    "es-ES", "et-EE", "eu-ES", "fa-IR", "ff-Latn-SN", "fi-FI", "fil-PH",
    "fo-FO", "fr-FR", "fy-NL", "ga-IE", "gd-GB", "gl-ES", "gsw-FR", "gu-IN",
    "ha-Latn-NG", "haw-US", "he-IL", "hi-IN", "hr-HR", "hsb-DE", "hu-HU",
    "hy-AM", "ig-NG", "id-ID", "is-IS", "it-IT", "iu-Latn-CA", "ja-JP", "kk-KZ",
    "kl-GL", "km-KH", "kn-IN", "kok-IN", "ko-KR", "ku-CK", "ky-KG", "lb-LU",
    "lo-LA", "lt-LT", "lv-LV", "mni-IN", "mk-MK", "ml-IN", "mn-MN", "moh-CA",
    "mr-IN", "ms-MY", "mt-MT", "mi-NZ", "ne-NP", "nb-NO", "nl-NL", "oc-FR",
    "or-IN", "pa-IN", "pl-PL", "prs-AF", "ps-AF", "pt-PT", "ro-RO", "rm-CH",
    "ru-RU", "rw-RW", "sa-IN", "sah-RU", "se-NO", "si-LK", "sk-SK", "sl-SI",
    "sr-RS", "sr-Latn-RS", "st-ZA", "sv-SE", "sw-KE", "ta-IN", "te-IN", "tg-TJ",
    "th-TH", "ti-ER", "tk-TM", "tn-ZA", "tr-TR", "tt-RU", "ug-CN", "uk-UA",
    "ur-PK", "uz-UZ", "vi-VN", "wo-SN", "xh-ZA", "yo-NG", "zu-ZA"
};


static const char* heavy_leftwards_arrow_with_equilateral_arrowhead =
                   "\xF0\x9F\xA0\x98";
static const char* heavy_rightwards_arrow_with_equilateral_arrowhead =
                   "\xF0\x9F\xA0\x96";

static ui_label_t label = ui_label(0.0, "Hello");

static void every_sec(ui_view_t* unused(view)) {
    static int32_t locale = 0;
    nls.set_locale(locales[locale]);
    ui_view.localize(&label.view);
    strprintf(title, "Hello %s%s %s [%s]",
              heavy_leftwards_arrow_with_equilateral_arrowhead,
              heavy_rightwards_arrow_with_equilateral_arrowhead,
              nls.str("Hello"), locales[locale]);
    app.set_title(title);
    app.layout();
    locale = (locale + 1) % countof(locales);
}

static void layout(ui_view_t* view) {
    // position single child at the center of the parent
    layouts.center(view); // only works for a single child view
}

// all views are transparent and expect parent to fill the background

static void paint(ui_view_t* view) {
    gdi.fill_with(0, 0, view->w, view->h, colors.black);
}

static void opened(void) {
    font = gdi.create_font("Segoe Script", app.in2px(0.5), -1);
    app.view->layout    = layout;
    app.view->paint     = paint;
    app.view->every_sec = every_sec;
    label.view.font = &font; // &app.fonts.H3;
    ui_view.add(app.view, &label, null);
}
static void init(void) {
    app.title = title;
    app.opened = opened;
}

app_t app = {
    .class_name = "sample1",
    .init = init,
    .window_sizing = {
        .min_w = 3.5f, // 3.5x1.5 inches
        .min_h = 1.5f,
        .ini_w = 4.0f, // 4x2 inches
        .ini_h = 2.0f
    }
};
