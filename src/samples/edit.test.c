/* Copyright (c) Dmitry "Leo" Kuznetsov 2021-24 see LICENSE for details */
#include "single_file_lib/ut/ut.h"
#include "single_file_lib/ui/ui.h"

#define lorem_ipsum_canonique \
    "Lorem ipsum dolor sit amet, consectetur adipiscing elit, sed do "         \
    "eiusmod  tempor incididunt ut labore et dolore magna aliqua.Ut enim ad "  \
    "minim veniam, quis nostrud exercitation ullamco laboris nisi ut aliquip " \
    "ex ea commodo consequat.Duis aute irure dolor in reprehenderit in "       \
    "voluptate velit esse cillum dolore eu fugiat nulla pariatur.Excepteur "   \
    "sint occaecat cupidatat non proident, sunt in culpa qui officia "         \
    "deserunt mollit anim id est laborum."

static const char* content =
    "Good bye Universe...\n"
    "Hello World!\n"
    "\n"
    "Ctrl+Shift and F5 starts/stops FUZZING test.\n"
    "\n"
    "FUZZING use rapid mouse clicks thus UI Fuzz button is hard to press use keyboard shortcut F5 to stop.\n"
    "\n"
    "0         10        20        30        40        50        60        70        80        90\n"
    "01234567890123456789012345678901234567890abcdefghi01234567890123456789012345678901234567890123456789\n"
    "0         10        20        30        40        50        60        70        80        90\n"
    "01234567890123456789012345678901234567890abcdefghi01234567890123456789012345678901234567890123456789\n"
    "\n"
    "0" ut_glyph_chinese_jin4 ut_glyph_chinese_gong "3456789\n"
    "\n"
    ut_glyph_teddy_bear "\n"
    ut_glyph_teddy_bear ut_glyph_ice_cube ut_glyph_teddy_bear
    ut_glyph_ice_cube ut_glyph_teddy_bear ut_glyph_ice_cube "\n"
    ut_glyph_teddy_bear ut_glyph_ice_cube ut_glyph_teddy_bear " - "
    ut_glyph_ice_cube ut_glyph_teddy_bear ut_glyph_ice_cube "\n"
    "\n"
    lorem_ipsum_canonique "\n"
    lorem_ipsum_canonique;

// 566K angular2.min.js
// https://get.cdnpkg.com/angular.js/2.0.0-beta.17/angular2.min.js
// https://web.archive.org/web/20230104221014/https://get.cdnpkg.com/angular.js/2.0.0-beta.17/angular2.min.js

// https://en.wikipedia.org/wiki/Lorem_ipsum

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
} ui_edit_lorem_ipsum_generator_params_t;

static void ui_edit_lorem_ipsum_generator(ui_edit_lorem_ipsum_generator_params_t p) {
    ut_fatal_if(p.count < 1024); // at least 1KB expected
    ut_fatal_if_not(0 < p.min_paragraphs && p.min_paragraphs <= p.max_paragraphs);
    ut_fatal_if_not(0 < p.min_sentences && p.min_sentences <= p.max_sentences);
    ut_fatal_if_not(2 < p.min_words && p.min_words <= p.max_words);
    static const char* words[] = {
        "lorem", "ipsum", "dolor", "sit", "amet", "consectetur", "adipiscing",
        "elit", "quisque", "faucibus", "ex", "sapien", "vitae", "pellentesque",
        "sem", "placerat", "in", "id", "cursus", "mi", "pretium", "tellus",
        "duis", "convallis", "tempus", "leo", "eu", "aenean", "sed", "diam",
        "urna", "tempor", "pulvinar", "vivamus", "fringilla", "lacus", "nec",
        "metus", "bibendum", "egestas", "iaculis", "massa", "nisl",
        "malesuada", "lacinia", "integer", "nunc", "posuere", "ut",
        "hendrerit", "semper", "vel", "class", "aptent", "taciti", "sociosqu",
        "ad", "litora", "torquent", "per", "conubia", "nostra", "inceptos",
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
    char* s = p.text;
    char* end = p.text + p.count - 64;
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
                const char* word = words[ut_num.random32(&p.seed) % ut_count_of(words)];
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
//  traceln("%s\n", p.text);
}

void ui_edit_init_with_lorem_ipsum(ui_edit_text_t* t) {
    static char text[64 * 1024];
    ui_edit_lorem_ipsum_generator_params_t p = {
        .text = text,
        .count = ut_count_of(text),
        .min_paragraphs = 4,
        .max_paragraphs = 15,
        .min_sentences  = 4,
        .max_sentences  = 20,
        .min_words      = 8,
        .max_words      = 16,
        .append         = "\n"
    };
    #ifdef DEBUG
        p.seed = 1; // repeatable sequence of pseudo random numbers
    #else
        p.seed = (int32_t)ut_clock.nanoseconds() | 0x1; // must be odd
    #endif
    ui_edit_lorem_ipsum_generator(p);
    swear(ui_edit_text.replace_utf8(t, null, content, -1, null));
    ui_edit_range_t end = ui_edit_text.end_range(t);
    swear(ui_edit_text.replace_utf8(t, &end, "\n\n", -1, null));
    end = ui_edit_text.end_range(t);
    swear(ui_edit_text.replace_utf8(t, &end, p.text, -1, null));
}

