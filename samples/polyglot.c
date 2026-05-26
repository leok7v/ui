/* Copyright (c) Dmitry "Leo" Kuznetsov 2021-24 see LICENSE for details */
#include "posix/posix.h"
#include "ui/ui.h"

static char title[128] = "Polyglot"; // https://youtu.be/D36zd8yNTbQ

// "Hello" across a few Latin-script locales (the Segoe Script font below
// renders these cleanly); the runtime locale is switched to match.
static const struct greeting { const char* locale; const char* hello; }
greetings[] = {
    { "en-US", "Hello"      },
    { "es-ES", "Hola"       },
    { "fr-FR", "Bonjour"    },
    { "it-IT", "Ciao"       },
    { "de-DE", "Hallo"      },
    { "pt-PT", "Ol\xC3\xA1" }, // Ola with acute accent
    { "nl-NL", "Hoi"        },
    { "sv-SE", "Hej"        },
    { "fi-FI", "Hei"        },
    { "cs-CZ", "Ahoj"       },
    { "ro-RO", "Salut"      },
    { "tr-TR", "Merhaba"    },
};

static int32_t locale;
static ui_label_t label = ui_label(0.0, "Hello");

static void every_sec(struct ui_view* posix_unused(v)) {
    const struct greeting* g = &greetings[locale];
    posix_nls.set_locale(g->locale);
    ui_view.set_text(&label, "%s", g->hello);
    posix_str_printf(title, "Polyglot [%s]", g->locale);
    ui_app.set_title(title);
    ui_app.request_layout();
    locale = (locale + 1) % posix_countof(greetings);
}

static bool tap(struct ui_view* v, int32_t ix, bool pressed) {
    const bool inside = ui_view.inside(v, &ui_app.mouse);
    posix_println("ix: %d inside: %d %s", ix, inside, pressed ? "dw" : "up");
    return inside;
}

static bool long_press(struct ui_view* v, int32_t ix) {
    const bool inside = ui_view.inside(v, &ui_app.mouse);
    posix_println("ix: %d inside: %d", ix, inside);
    return inside;
}

static bool double_tap(struct ui_view* v, int32_t ix) {
    const bool inside = ui_view.inside(v, &ui_app.mouse);
    posix_println("ix: %d inside: %d", ix, inside);
    return inside;
}

static void opened(void) {
    static struct ui_fm fm;
    ui_draw.update_fm(&fm, ui_draw.create_font("Segoe Script", ui_app.in2px(0.5), -1));
    ui_app.content->every_sec = every_sec;
    label.fm = &fm;
    ui_view.add(ui_app.content, &label, null);
    locale = 0;
    label.tap = tap;
    label.long_press = long_press;
    label.double_tap = double_tap;
}
static void init(void) {
    ui_app.title = title;
    ui_app.opened = opened;
}

struct ui_app ui_app = {
    .class_name = "polyglot",
    .dark_mode = true,
    .init = init,
    .window_sizing = {
        .min_w = 4.0f, // 4.0x1.5 inches
        .min_h = 1.5f,
        .ini_w = 4.0f, // 4x2 inches
        .ini_h = 2.0f
    }
};
