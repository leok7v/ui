/* Copyright (c) Dmitry "Leo" Kuznetsov 2021-24 see LICENSE for details */
#include "single_file_lib/ut/ut.h"
#include "single_file_lib/ui/ui.h"

static const char* lorem_ipsum_words[] = {
    "lorem", "ipsum", "dolor", "sit", "amet", "consectetur", "adipiscing",
    "elit", "quisque", "faucibus", "ex", "sapien", "vitae", "pellentesque",
    "sem", "placerat", "in", "id", "cursus", "mi", "pretium", "tellus",
    "duis", "convallis", "tempus", "leo", "eu", "aenean", "sed", "diam",
    "urna", "tempor", "pulvinar", "vivamus", "fringilla", "lacus", "nec",
    "metus", "bibendum", "egestas", "iaculis", "massa", "nisl",
    "malesuada", "lacinia", "integer", "nunc", "posuere", "ut", "hendrerit",
    "semper", "vel", "class", "aptent", "taciti", "sociosqu", "ad", "litora",
    "torquent", "per", "conubia", "nostra", "inceptos",
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

#define lorem_ipsum_canonique \
    "Lorem ipsum dolor sit amet, consectetur adipiscing elit, sed do "         \
    "eiusmod  tempor incididunt ut labore et dolore magna aliqua.Ut enim ad "  \
    "minim veniam, quis nostrud exercitation ullamco laboris nisi ut aliquip " \
    "ex ea commodo consequat. Duis aute irure dolor in reprehenderit in "      \
    "voluptate velit esse cillum dolore eu fugiat nulla pariatur. Excepteur "  \
    "sint occaecat cupidatat non proident, sunt in culpa qui officia "         \
    "deserunt mollit anim id est laborum."

#define lorem_ipsum_chinese \
    "\xE6\x88\x91\xE6\x98\xAF\xE6\x94\xBE\xE7\xBD\xAE\xE6\x96\x87\xE6\x9C\xAC\xE7\x9A\x84\xE4" \
    "\xBD\x8D\xE7\xBD\xAE\xE3\x80\x82\xE8\xBF\x99\xE9\x87\x8C\xE6\x94\xBE\xE7\xBD\xAE\xE4\xBA" \
    "\x86\xE5\x81\x87\xE6\x96\x87\xE5\x81\x87\xE5\xAD\x97\xE3\x80\x82\xE5\xB8\x8C\xE6\x9C\x9B" \
    "\xE8\xBF\x99\xE4\xBA\x9B\xE6\x96\x87\xE5\xAD\x97\xE5\x8F\xAF\xE4\xBB\xA5\xE5\xA1\xAB\xE5" \
    "\x85\x85\xE7\xA9\xBA\xE7\x99\xBD\xE3\x80\x82";

#define  lorem_ipsum_japanese \
    "\xE3\x81\x93\xE3\x82\x8C\xE3\x81\xAF\xE3\x83\x80\xE3\x83\x9F\xE3\x83\xBC\xE3\x83\x86\xE3" \
    "\x82\xAD\xE3\x82\xB9\xE3\x83\x88\xE3\x81\xA7\xE3\x81\x99\xE3\x80\x82\xE3\x81\x93\xE3\x81" \
    "\x93\xE3\x81\xAB\xE6\x96\x87\xE7\xAB\xA0\xE3\x81\x8C\xE5\x85\xA5\xE3\x82\x8A\xE3\x81\xBE" \
    "\xE3\x81\x99\xE3\x80\x82\xE8\xAA\xAD\xE3\x81\xBF\xE3\x82\x84\xE3\x81\x99\xE3\x81\x84\xE3" \
    "\x82\x88\xE3\x81\x86\xE3\x81\xAB\xE3\x83\x80\xE3\x83\x9F\xE3\x83\xBC\xE3\x83\x86\xE3\x82" \
    "\xAD\xE3\x82\xB9\xE3\x83\x88\xE3\x82\x92\xE4\xBD\xBF\xE7\x94\xA8\xE3\x81\x97\xE3\x81\xA6" \
    "\xE3\x81\x84\xE3\x81\xBE\xE3\x81\x99\xE3\x80\x82";


#define lorem_ipsum_korean \
    "\xEC\x9D\xB4\xEA\xB2\x83\xEC\x9D\x80\x20\xEB\x8D\x94\xEB\xAF\xB8\x20\xED\x85\x8D\xEC\x8A" \
    "\xA4\xED\x8A\xB8\xEC\x9E\x85\xEB\x8B\x88\xEB\x8B\xA4\x2E\x20\xEC\x97\xAC\xEA\xB8\xB0\xEC" \
    "\x97\x90\x20\xEB\xAC\xB8\xEC\x9E\x90\xEA\xB0\x80\x20\xEB\x93\x9C\xEC\x96\xB4\xEA\xB0\x80" \
    "\xEB\x8A\x94\x20\xEB\xAC\xB8\xEC\x9E\x90\xEA\xB0\x80\x20\xEC\x9E\x88\xEB\x8B\xA4\x2E\x20" \
    "\xEC\x9D\xBD\xEA\xB8\xB0\x20\xEC\x89\xBD\xEA\xB2\x8C\x20\xEB\x8D\x94\xEB\xAF\xB8\x20\xED" \
    "\x85\x8D\xEC\x8A\xA4\xED\x8A\xB8\xEB\xA5\xBC\x20\xEC\x82\xAC\xEC\x9A\xA9\xED\x95\xA9\xEB" \
    "\x8B\x88\xEB\x8B\xA4\x2E";

#define lorem_ipsum_emoji \
    "\xF0\x9F\x8D\x95\xF0\x9F\x9A\x80\xF0\x9F\xA6\x84\xF0\x9F\x92\xBB\xF0\x9F\x8E\x89\xF0\x9F" \
    "\x8C\x88\xF0\x9F\x90\xB1\xF0\x9F\x93\x9A\xF0\x9F\x8E\xA8\xF0\x9F\x8D\x94\xF0\x9F\x8D\xA6" \
    "\xF0\x9F\x8E\xB8\xF0\x9F\xA7\xA9\xF0\x9F\x8D\xBF\xF0\x9F\x93\xB7\xF0\x9F\x8E\xA4\xF0\x9F" \
    "\x91\xBE\xF0\x9F\x8C\xAE\xF0\x9F\x8E\x88\xF0\x9F\x9A\xB2\xF0\x9F\x8D\xA9\xF0\x9F\x8E\xAE" \
    "\xF0\x9F\x8D\x89\xF0\x9F\x8E\xAC\xF0\x9F\x90\xB6\xF0\x9F\x93\xB1\xF0\x9F\x8E\xB9\xF0\x9F" \
    "\xA6\x96\xF0\x9F\x8C\x9F\xF0\x9F\x8D\xAD\xF0\x9F\x8E\xA4\xF0\x9F\x8F\x96\xF0\x9F\xA6\x8B" \
    "\xF0\x9F\x8E\xB2\xF0\x9F\x8E\xAF\xF0\x9F\x8D\xA3\xF0\x9F\x9A\x81\xF0\x9F\x8E\xAD\xF0\x9F" \
    "\x91\x9F\xF0\x9F\x9A\x82\xF0\x9F\x8D\xAA\xF0\x9F\x8E\xBB\xF0\x9F\x9B\xB8\xF0\x9F\x8C\xBD" \
    "\xF0\x9F\x93\x80\xF0\x9F\x9A\x80\xF0\x9F\xA7\x81\xF0\x9F\x93\xAF\xF0\x9F\x8C\xAF\xF0\x9F" \
    "\x90\xA5\xF0\x9F\xA7\x83\xF0\x9F\x8D\xBB\xF0\x9F\x8E\xAE";


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
        "malesuada", "lacinia", "integer", "nunc", "posuere", "ut", "hendrerit",
        "semper", "vel", "class", "aptent", "taciti", "sociosqu", "ad", "litora",
        "torquent", "per", "conubia", "nostra", "inceptos",
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

