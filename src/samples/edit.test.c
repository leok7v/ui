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
    ut_glyph_lady_beetle "\n"
#if 1
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
#endif

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
                const int32_t ix = ut_num.random32(&p.seed) %
                                   ut_countof(lorem_ipsum_words);
                const char* word = lorem_ipsum_words[ix];
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
//  ut_println("%s\n", p.text);
}

void ui_edit_init_with_lorem_ipsum(ui_edit_text_t* t) {
    static char text[64 * 1024];
    ui_edit_lorem_ipsum_generator_params_t p = {
        .text = text,
        .count = ut_countof(text),
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
    ut_swear(ui_edit_text.replace_utf8(t, null, content, -1, null));
#if 1
    ui_edit_range_t end = ui_edit_text.end_range(t);
    ut_swear(ui_edit_text.replace_utf8(t, &end, "\n\n", -1, null));
    end = ui_edit_text.end_range(t);
    ut_swear(ui_edit_text.replace_utf8(t, &end, p.text, -1, null));
    // test bad UTF8
    static const char* th_bad_utf8 = "\xE0\xB8\x9A\xE0\xB8\xA3\xE0\xB8\xB4\xE0\xB9\x80\xE0\xB8\xA7\xE0\xB8\x93\xE0\xB8\x8A\xE0\xB8\xB8\xE0\xB8\x94\xE0\xB8\xA5\xE0\xB8\xB0\xE0\xB8\xA5\xE0\xB8\xB0\xE0\xB8\xAA\xE0\xB8\xB2\xE0\xB8\x87\xE0\xE0\xB9\x80\xE0\xB8\xA3\xE0\xB8\x87\xE0\xB9\x84\xE0\xB8\xA5\xE0\xB8\xA1\xE0\xB8\x95\xE0\xB8\xA3\xE0\xB8\xB4\xE0\xB8\xA1\xE0\xB8\xAD\xE0\xB9\x88\xE0\xB8\xB2\xE0\xB8\x94\xE0\xB8\xAD\xE0\xB8\x94\xE0\xB8\xAA\xE0\xB9\x80\xE0\xB8\xA3\xE0\xB8\xA2\xE0\xB8\xA2\xE0\xB8\xA7\xE0\xB8\xA5\xE0\xB8\xB1\xE0\xB8\x9A\xE0\xB8\x81\xE0\xB8\xB4\xE0\xB8\x9B\xE0\xB8\x81\xE0\xB8\xA7\xE0\xB8\xB1\xE0\xB8\x87\x2E";
    end = ui_edit_text.end_range(t);
//  ut_println("%s", th_bad_utf8);
    bool expected_false = ui_edit_text.replace_utf8(t, &end, th_bad_utf8, -1, null);
    ut_swear(expected_false == false);
    static const char* en_sentence_utf8 = "\x54\x68\x65\x20\x71\x75\x69\x63\x6b\x20\x62\x72\x6f\x77\x6e\x20\x66\x6f\x78\x20\x6a\x75\x6d\x70\x73\x20\x6f\x76\x65\x72\x20\x74\x68\x65\x20\x6c\x61\x7a\x79\x20\x64\x6f\x67\x2e";
    static const char* es_sentence_utf8 = "\x45\x6c\x20\x76\x65\x6c\x6f\x7a\x20\x6d\x75\x72\x63\x69\x65\xcc\x81\x6c\x61\x67\x6f\x20\x68\x69\x6e\x64\x75\xcc\x81\x20\x63\x6f\x6d\x69\xcc\x81\x61\x20\x66\x65\x6c\x69\x7a\x20\x63\x61\x72\x64\x69\x6c\x6c\x6f\x20\x79\x20\x6b\x69\x77\x69\x2e";
    static const char* fr_sentence_utf8 = "\x50\x6f\x72\x74\x65\x7a\x20\x63\x65\x20\x76\x69\x65\x75\x78\x20\x77\x68\x69\x73\x6b\x79\x20\x61\x75\x20\x6a\x75\x67\x65\x20\x62\x6c\x6f\x6e\x64\x20\x71\x75\x69\x20\x66\x75\x6d\x65\x2e";
    static const char* de_sentence_utf8 = "\x56\x69\x63\x74\x6f\x72\x20\x6a\x61\x67\x74\x20\x7a\x77\xc3\xb6\x6c\x66\x20\x42\x6f\x78\x6b\xc3\xa4\x6d\x70\x66\x65\x72\x20\x71\x75\x65\x72\x20\xc3\xbc\x62\x65\x72\x20\x64\x65\x6e\x20\x67\x72\x6f\xc3\x9f\x65\x6e\x20\x53\x79\x6c\x74\x65\x72\x20\x44\x65\x69\x63\x68\x2e";
    static const char* pl_sentence_utf8 = "\x50\x63\x68\x6e\xc4\x85\xc4\x87\x20\x77\x20\x74\xc4\x99\x20\xc5\x82\xc3\xb3\xc4\x87\x20\x6a\x65\xc5\xbc\x61\x20\x6c\x75\x62\x20\x6f\xc5\x9b\x6d\x20\x73\x6b\x72\x7a\x79\xc5\x84\x20\x66\x69\x67\x2e";
    static const char* cs_sentence_utf8 = "\x50\xc5\x99\xc3\xad\x6c\xc3\xad\xc5\xa1\x20\xc5\xbe\x6c\x75\xc5\xa5\x6f\x75\xc4\x8d\x6b\x79\x20\x6b\xc5\xaf\xc5\x88\x20\xc3\xba\x70\xc4\x9bl\x20\xc4\x8f\xc3\xa1\x62\x65\x6c\x73\x6b\xc3\xa9\x20\xc3\xb3\x64\x79\x2e";
    static const char* hu_sentence_utf8 = "\xc3\x81\x72\x76\xc3\xad\x7a\x74\xc5\xb1\x72\xc5\x91\x20\x74\xc3\xbc\x6b\xc3\xb6\x72\x66\xc3\xba\x72\xc3\xb3\x67\xc3\xa9\x70\x2e";
    static const char* ru_sentence_utf8 = "\xd0\xa1\xd1\x8a\xd0\xb5\xd1\x88\xd1\x8c\x20\xd0\xb6\xd0\xb5\x20\xd0\xb5\xd1\x89\xd1\x91\x20\xd1\x8d\xd1\x82\xd0\xb8\xd1\x85\x20\xd0\xbc\xd1\x8f\xd0\xb3\xd0\xba\xd0\xb8\xd1\x85\x20\xd1\x84\xd1\x80\xd0\xb0\xd0\xbd\xd1\x86\xd1\x83\xd0\xb7\xd1\x81\xd0\xba\xd0\xb8\xd1\x85\x20\xd0\xb1\xd1\x83\xd0\xbb\xd0\xbe\xd0\xba\x20\xd0\xb4\xd0\xb0\x20\xd0\xb2\xd1\x8b\xd0\xbf\xd0\xb5\xd0\xb9\x20\xd1\x87\xd0\xb0\xd1\x8e\x2e";
    static const char* pt_sentence_utf8 = "\x4f\x20\x76\x65\x6c\x6f\x7a\x20\x63\x61\x63\x68\x6f\x72\x72\x6f\x20\x6d\x61\x72\x72\x6f\x6d\x20\x7a\x61\x70\x61\x20\x72\x61\x70\x69\x64\x61\x6d\x65\x6e\x74\x65\x20\x75\x6d\x20\x6b\x69\x77\x69\x2e";
    static const char* it_sentence_utf8 = "\x51\x75\x65\x6c\x20\x66\x6f\x78\x20\x62\x72\x75\x6e\x6f\x20\x73\x61\x6c\x74\x61\x20\x69\x6c\x20\x63\x61\x6e\x65\x20\x70\x69\x67\x72\x6f\x20\x6c\x61\x7a\x7a\x6f\x2e";
    static const char* nl_sentence_utf8 = "\x44\x65\x20\x76\x6c\x75\x67\x20\x6a\x65\x20\x7a\x65\x76\x65\x6e\x20\x71\x75\x69\x73\x74\x20\x64\x65\x20\x73\x63\x68\x61\x74\x20\x62\x72\x75\x69\x6e\x20\x6c\x61\x7a\x65\x72\x2e";
    static const char* el_sentence_utf8 = "\xce\x9e\xce\xb5\xcf\x83\xce\xba\xce\xb5\xcf\x80\xce\xac\xce\xb6\xcf\x89\x20\xcf\x84\xce\xb7\xce\xbd\x20\xcf\x88\xcf\x85\xcf\x87\xce\xbf\xcf\x86\xce\xb8\xcf\x8c\xcf\x81\xce\xb1\x20\xce\xb2\xce\xb4\xce\xb5\xce\xbb\xcf\x85\xce\xb3\xce\xbc\xce\xaf\xce\xb1\x2e";
    static const char* tr_sentence_utf8 = "\xc3\x87\xc4\xb1\xc4\x9f\xc4\xb1\x6c\xc3\xb6\xc4\x9f\xc4\xb1\x6e\x6d\xc3\xa7\xc3\xa7\xc3\xa7";
    static const char* uk_sentence_utf8 = "\xd0\xa4\xd0\xb0\xd0\xbd\xd0\xba\xd1\x83\xd0\xb2\xd0\xb0\xd0\xbb\xd0\xb0\x20\xd0\xbd\xd0\xb0\x20\xd0\xb1\xd0\xb5\xd1\x80\xd0\xb5\xd0\xb7\xd1\x96\x20\xd0\xb2\xd0\xb5\xd0\xbb\xd0\xb8\xd0\xba\xd1\x83\xd1\x8e\x20\xd0\xbf\xd0\xbe\xd1\x80\xd1\x86\xd1\x96\xd1\x8e\x20\xd0\xbc\xd0\xb0\xd0\xbb\xd0\xb8\xd0\xbd\xd0\xbe\xd0\xb2\xd0\xbe\xd0\xb3\xd0\xbe\x20\xd0\xb2\xd0\xb0\xd1\x80\xd0\xb5\xd0\xbd\xd0\xbd\xd1\x8f\x2e";
    static const char* bg_sentence_utf8 = "\xd0\x9b\xd1\x8e\xd0\xb1\xd1\x8f\x20\xd1\x81\xd0\xb2\xd0\xbe\xd0\xb9\x20\xd0\xbc\xd0\xb5\xd0\xbb\xd1\x8a\xd0\xba\x20\xd1\x86\xd0\xb2\xd1\x8f\xd1\x82\x2c\x20\xd0\xb6\xd0\xb0\xd0\xba\x20\xd0\xb4\xd1\x8a\xd0\xb2\xd1\x87\xd0\xb5\x20\xd1\x88\xd1\x83\xd0\xbc\xd0\xb5\xd0\xbd\x20\xd1\x85\xd0\xb2\xd1\x8a\xd1\x80\xd1\x87\xd0\xb0\xd1\x89\x20\xd0\xbf\xd1\x83\xd0\xb4\xd0\xb5\xd0\xbb\x2e";
    static const char* sr_sentence_utf8 = "\xd0\x8f\xd0\xb5\xd0\xbf\x20\xd1\x98\xd0\xb5\x20\xd1\x88\xd1\x83\xd0\xbf\xd0\xb0\xd1\x99\x2c\x20\xd1\x84\xd0\xbe\xd0\xbb\xd0\xb8\xd1\x80\xd0\xb0\xd0\xbd\xd1\x82\xd1\x81\xd0\xba\xd0\xb0\x20\xd0\xb6\xd0\xb5\xd0\xbd\xd0\xb0\x20\xd1\x83\xd0\xb2\xd0\xb5\xd0\xba\x20\xd0\xbc\xd0\xb0\xd1\x81\xd0\xba\xd0\xb8\xd1\x80\xd0\xb0\x20\xd1\x99\xd1\x83\xd0\xbf\xd0\xba\xd0\xb5\x20\xd0\xb4\xd0\xb5\xd1\x87\xd0\xba\xd0\xb5\x2e";
    static const char* sq_sentence_utf8 = "\x5A\x68\x76\x69\x6C\x6F\x6A\x61\x20\xC3\x87\x64\x6F\x6B\x6F\x72\x20\x70\xC3\xAB\x72\x62\x65\x6C\x69\x6E\xC3\xAB\x20\x6E\xC3\xAB\x6E\x20\x64\x69\x6B\x75\x72\x20\x6E\x6F\x70\x61\x6C\x20\x74\x65\x20\x66\x6A\x61\x6C\xC3\xAB\x2E";
    static const char* sl_sentence_utf8 = "\x42\x6C\x61\x67\x6F\x76\x6F\x6C\x6A\x65\x6E\x20\xC5\xBE\x69\x6E\x6A\x61\x6A\x20\x70\x72\x61\x7A\x6E\x69\x20\xC5\xA1\x63\x72\x61\x6C\x20\x70\x6F\x64\x20\x76\x69\x73\x6F\x6B\x20\xC5\xBE\x65\x6C\x76\x65\x6C\x20\xC4\x8D\x75\x64\x6F\x76\x2E";
    static const char* lt_sentence_utf8 = "\xC4\x84\xC5\xBE\x75\x73\x69\x6E\xC4\x97\x20\xC5\xA1\x75\x6E\x79\x73\x74\x61\x20\x70\x6F\x20\x75\xC5\xB3\x73\x69\x75\x73\x20\xC5\xBE\x61\x6C\x6D\x61\x20\xC5\xBE\x69\x65\x6D\x6F\x6E\x69\x73\x20\x6D\x61\x6C\x65\x2E";
    static const char* lv_sentence_utf8 = "\xC4\x80\x72\x74\x61\x75\x72\x20\x6E\x65\x73\x65\x65\x64\x7A\xC4\xAB\x76\x73\x20\x6A\x61\x75\x20\xC5\xA1\x6B\x6F\x6C\x75\x20\xC5\xA1\x63\x65\x6E\xC5\xA1\x20\xC4\x8D\x69\x67\x61\x6E\x20\xC4\xBC\xC4\x81\x70\xC4\x81\x20\xC5\xA1\x6B\x61\x72\x62\x69\x6E\x73\x2E";
    static const char* et_sentence_utf8 = "\xC3\x9C\x68\x65\x74\x6F\x72\x75\x6E\x65\x20\x74\x6F\x68\x74\x75\x6D\x20\x6B\x61\x73\x6B\x75\x73\x20\xC3\xB5\x70\x65\x64\x20\x6B\xC3\xB5\x72\x67\x75\x6D\x20\xC3\xBC\x72\x69\x74\x75\x73\x20\xC3\xB5\x75\x20\x6A\xC3\xA4\xC3\xA4\x6E\x2E";
    static const char* ga_sentence_utf8 = "\x42\x68\x69\x20\x66\xC3\xA1\x6C\x74\x61\x20\x6D\x68\xC3\xA1\x69\x72\xC3\xAD\x2C\x20\x63\x68\x75\x69\x20\x66\xC3\xA9\x6E\x65\x20\x6D\x68\xC3\xA9\x69\x72\x20\xC3\xA1\x20\x63\x68\x6F\x69\x73\x20\xC3\xA9\x69\x6E\x2E";
    static const char* cy_sentence_utf8 = "\x59\x20\x66\x66\x6F\x72\x6B\x20\x67\x72\x65\x6E\x66\x6F\x72\x20\x20\x62\x6C\x61\x77\x64\x64\x20\x6C\x6C\x65\x20\x77\x61\x69\x74\x68\xC3\xAF\x73\x20\xC3\xA2\x73\x2E";
    static const char* is_sentence_utf8 = "\xC3\x86\x72\x69\x20\x6C\x65\x74\x6A\x61\x72\xC3\xAD\x20\x6E\xC3\xBD\x20\xC3\xB3\x76\x61\x72\x2E\x20\xC3\x9E\xC3\xA9\xC3\xB0\x20\x65\x72\x20\x61\xC3\xB0\x20\x76\x69\x6C\x6A\x61\x72\xC3\xAD\x20\x6D\xC3\xAD\x6E\x2E";
    static const char* mt_sentence_utf8 = "\xC4\xA0\x75\x73\x20\x6A\x6F\x62\x62\x61\x20\x6C\x69\x20\x6C\x61\x2E";
    static const char* he_sentence_utf8 = "\xD7\x90\xD7\x91\xD7\x92\xD7\x93\xD7\x94\xD7\x95\xD7\x96\xD7\x97\xD7\x98\xD7\x99\xD7\x9A\xD7\x9B\xD7\x9C\xD7\x9D\xD7\x9E\xD7\x9F\xD7\xA1\xD7\xA2\xD7\xA4\xD7\xA6\xD7\xA7\xD7\xA8\xD7\xA9\xD7\xAA";
    static const char* ar_sentence_utf8 = "\xD8\xB5\xD9\x90\xD9\x81\xD9\x92\x20\xD8\xAE\xD9\x90\xD9\x84\xD9\x92\xD8\xB9\xD9\x8E\xD8\xAA\xD9\x8E\xD9\x83\xD9\x8E\x20\xD8\xA7\xD9\x84\xD9\x8A\xD9\x8E\xD9\x88\xD9\x92\xD9\x85\xD9\x8E\x20\xD8\xBA\xD9\x8E\xD8\xB7\xD9\x91\xD9\x8E\xD8\xB3\xD9\x8E\x20\xD8\xAB\xD9\x8E\xD9\x88\xD8\xA8\xD9\x8E\xD9\x83\xD9\x8E\x20\xD9\x81\xD9\x90\xD9\x8A\xD9\x92\x20\xD8\xAD\xD9\x8E\xD9\x84\xD9\x8A\xD8\xA8\xD9\x8D\x20\xD8\xAC\xD9\x8E\xD8\xA7\xD9\x85\xD9\x90\xD8\xAF\xD9\x90\x2E";
    static const char* hi_sentence_utf8 = "\xE0\xA4\x97\xE0\xA4\xA3\xE0\xA5\x87\xE0\xA4\xB6\x20\xE0\xA4\xAA\xE0\xA5\x82\xE0\xA4\x9C\xE0\xA4\xA8\x20\xE0\xA4\x95\xE0\xA4\xB0\xE0\xA5\x8B\x20\xE0\xA4\x94\xE0\xA4\xB0\x20\xE0\xA4\xAA\xE0\xA5\x8D\xE0\xA4\xB0\xE0\xA4\xB8\xE0\xA4\xBE\xE0\xA4\xA6\x20\xE0\xA4\x9A\xE0\xA4\xA2\xE0\xA4\xBC\xE0\xA4\xBE\xE0\xA4\x93\xE0\xA5\xA4";
    static const char* bn_sentence_utf8 = "\xE0\xA6\x8F\xE0\xA6\x95\xE0\xA6\xAF\xE0\xA7\x81\xE0\xA6\x97\xE0\xA7\x87\x20\xE0\xA6\xAD\xE0\xA6\xBE\xE0\xA6\xB0\xE0\xA6\xA4\xE0\xA6\xAC\xE0\xA6\xB0\xE0\xA7\x8D\xE0\xA6\xB7\xE0\xA7\x87\xE0\xA6\xB0\x20\xE0\xA6\xAE\xE0\xA6\xBE\xE0\xA6\xA8\xE0\xA7\x81\xE0\xA6\xB7\x20\xE0\xA6\xB8\xE0\xA6\xBE\xE0\xA6\xB0\xE0\xA6\xBE\x20\xE0\xA6\x9C\xE0\xA6\x97\xE0\xA6\xA4\xE0\xA7\x87\x20\xE0\xA6\xB6\xE0\xA7\x8D\xE0\xA6\xB0\xE0\xA7\x87\xE0\xA6\xB7\xE0\xA7\x8D\xE0\xA6\xA0\x20\xE0\xA6\xB8\xE0\xA7\x8D\xE0\xA6\xA5\xE0\xA6\xBE\xE0\xA6\xA8\x20\xE0\xA6\x85\xE0\xA6\xA7\xE0\xA6\xBF\xE0\xA6\x95\xE0\xA6\xBE\xE0\xA6\xB0\x20\xE0\xA6\x95\xE0\xA6\xB0\xE0\xA6\xBF\xE0\xA6\xAC\xE0\xA7\x87\xE0\xA5\xA4";
    static const char* ta_sentence_utf8 = "\xe0\xae\x9a\xe0\xae\xbf\xe0\xae\xb5\xe0\xae\xaa\xe0\xaf\x8d\xe0\xae\xaa\xe0\xaf\x81\x20\xe0\xae\xa8\xe0\xae\xb0\xe0\xae\xbf\x20\xe0\xae\xae\xe0\xae\xa8\xe0\xaf\x8d\xe0\xae\xa4\x20\xe0\xae\xa8\xe0\xae\xbe\xe0\xae\xaf\xe0\xaf\x88\x20\xe0\xae\xa4\xe0\xae\xbe\xe0\xae\xa3\xe0\xaf\x8d\xe0\xae\x9f\xe0\xae\xbf\xe0\xae\xaf\xe0\xae\xa4\xe0\xaf\x81\x2e";
    static const char* te_sentence_utf8 = "\xE0\xB0\x8E\xE0\xB0\x97\xE0\xB0\xB8\xE0\xB0\xBF\xE0\xB0\xA8\x20\xE0\xB0\x95\xE0\xB0\x82\xE0\xB0\xA6\xE0\xB0\xAE\xE0\xB1\x81\xE0\xB0\xB2\xE0\xB1\x81\x20\xE0\xB0\xAF\xE0\xB0\xB5\xE0\xB1\x8D\xE0\xB0\xB5\xE0\xB0\xA8\xE0\xB0\xBE\xE0\xB0\xA8\xE0\xB1\x8D\xE0\xB0\xA8\xE0\xB0\xBF\x20\xE0\xB0\x9A\xE0\xB1\x86\xE0\xB0\xB0\xE0\xB0\xBF\xE0\xB0\xAA\xE0\xB1\x87\x20\xE0\xB0\x9A\xE0\xB0\x82\xE0\xB0\xA6\xE0\xB0\xBE\xE0\xB0\xB2\x20\xE0\xB0\xA6\xE0\xB0\xBF\xE0\xB0\xA8\xE0\xB0\x95\xE0\xB0\xB0\xE0\xB0\x82\x2E";
    static const char* kn_sentence_utf8 = "\xE0\xB2\x85\xE0\xB2\xA8\xE0\xB2\x95\xE0\xB3\x8D\xE0\xB2\x95\xE0\xB2\xA8\x20\xE0\xB2\x97\xE0\xB2\xB4\xE0\xB3\x81\xE0\xB2\xB8\xE0\xB2\xBF\x20\xE0\xB2\x8E\xE0\xB2\xA8\xE0\xB3\x8D\xE0\xB2\xA8\xE0\xB2\xA6\xE0\xB2\xBF\xE0\xB2\xA8\xE0\xB2\x95\xE0\xB2\x82\x20\xE0\xB2\xA6\xE0\xB2\xBF\xE0\xB2\x82\xE0\xB2\xA1\xE0\xB3\x81\xE0\xB2\x95\xE0\xB3\x86\xE0\xB2\xA6\x20\xE0\xB2\x9A\xE0\xB2\x82\xE0\xB2\xA6\xE0\xB2\x95\xE0\xB3\x86\xE0\xB2\x9F\xE0\xB2\xA1\xE0\xB3\x81\xE0\xB2\xB5\xE0\xB2\xBE\xE0\xB2\xB0\xE0\xB2\x93\x2E";
    static const char* ml_sentence_utf8 = "\xE0\xB4\x8F\xE0\xB4\x95\xE0\xB4\xB4\xE0\xB4\x82\x20\xE0\xB4\xA4\xE0\xB4\xB0\xE0\xB4\x82\x20\xE0\xB4\xAE\xE0\xB4\xA3\xE0\xB4\xB1\xE0\xB4\x96\xE0\xB4\xB7\xE0\xB4\x82\x20\xE0\xB4\x89\xE0\xB4\xA3\xE0\xB5\x8D\xE0\xB4\xAE\xE0\xB5\x82\x20\xE0\xB4\xA8\xE0\xB4\xA4\xE0\xB4\x95\xE0\xB5\x8D\xE0\xB4\x95\xE0\xB5\x8B\x20\xE0\xB4\x87\xE0\xB4\xA4\xE0\xB4\xBF\xE0\xB4\xB0\xE0\xB5\x81\xE0\xB4\xA8\xE0\xB5\x8D\xE0\xB4\xA8\xE0\xB5\x8B\x20\xE0\xB4\xA4\xE0\xB4\xA3\xE0\xB5\x8D\xE0\xB4\xA8\xE0\xB5\x81\x2E";
    static const char* si_sentence_utf8 = "\xE0\xB6\xB8\xE0\xB7\x9A\x20\xE0\xB6\xB6\xE0\xB7\x8F\xE0\xB6\xB1\xE0\xB6\xA7\xE0\xB7\x8A\xE0\xB6\xA7\x20\xE0\xB6\xB8\xE0\xB6\x9A\xE0\xB7\x8A\xE0\xB6\xA7\xE0\xB6\xA1\x20\xE0\xB6\xA2\xE0\xB6\xA7\xE0\xB6\xA2\xE0\xB7\x92\x20\xE0\xB6\xAD\xE0\xB6\xB1\xE0\xB7\x8A\x20\xE0\xB6\xBA\xE0\xB7\x8F\xE0\xB6\xA9\x20\xE0\xB6\xA7\xE0\xB6\xB1\x20\xE0\xB7\x84\xE0\xB7\x90\xE0\xB6\xB8\xE0\xB7\x9A\x2E";
    static const char* th_sentence_utf8 = "\xe0\xb8\x9a\xe0\xb8\xa3\xe0\xb8\xb4\xe0\xb9\x80\xe0\xb8\xa7\xe0\xb8\x93\xe0\xb8\xaa\xe0\xb8\xb8\xe0\xb8\x94\xe0\xb8\xa5\xe0\xb8\xb0\xe0\xb8\xa5\xe0\xb8\xb0\xe0\xb8\xaa\xe0\xb8\xb2\xe0\xb8\x87\xe0\xb8\x82\xe0\xb8\xad\xe0\xb8\x87\xe0\xb9\x84\xe0\xb8\xa5\xe0\xb8\xa1\xe0\xb9\x8c\xe0\xb8\x95\xe0\xb8\xa3\xe0\xb8\xb4\xe0\xb8\xa1\xe0\xb8\xad\xe0\xb9\x88\xe0\xb8\xb2\xe0\xb8\x94\xe0\xb8\xad\xe0\xb8\x94\xe0\xb9\x80\xe0\xb8\xaa\xe0\xb8\xa3\xe0\xb8\xa2\xe0\xb8\xa7\xe0\xb8\xa5\xe0\xb8\xb1\xe0\xb8\x9a\xe0\xb8\x81\xe0\xb8\xb4\xe0\xb8\x9b\xe0\xb8\x81\xe0\xb8\xa7\xe0\xb8\xb1\xe0\xb8\x87\x2e";
    static const char* lo_sentence_utf8 = "\xE0\xB8\x9A\xE0\xB8\xB3\xE0\xB8\xA7\xE0\xB8\x94\xE0\xB8\xA5\xE0\xB8\xB2\xE0\xB8\xA1\xE0\xB8\xAA\xE0\xB8\xB2\xE0\xB8\x87\xE0\xB8\x82\xE0\xB9\x80\xE0\xB8\xA7\xE0\xB8\x87\xE0\xB8\x9D\xE0\xB8\xB2\xE0\xB9\x80\xE0\xB8\xA1\xE0\xB8\xAA\xE0\xB9\x83\xE0\xB8\xB8\xE0\xB8\x88\xE0\xB8\xA7\xE0\xB8\xB2\xE0\xB8\x99\xE0\xB9\x88\xE0\xB8\xB2\xE0\xB8\x8A\xE0\xB9\x8C\xE0\xB9\x89\xE0\xB8\x8A\xE0\xB9\x89\xE0\xB8\xA7\xE0\xB8\xA2\xE0\xB8\xAA\xE0\xB8\xA7\xE0\xB8\xAA\xE0\xB8\xA2\xE0\xB8\xB2\xE0\xB8\x88\xE0\xB8\x9D\xE0\xB8\x81\xE0\xB8\x95\xE0\xB8\xAD\xE0\xB9\x80\xE0\xB8\xB1\xE0\xB9\x88\xE0\xB8\xA7\xE0\xB8\x81\xE0\xB8\xA3\xE0\xB8\xB2\xE0\xB9\x81\xE0\xB8\x94\xE0\xB8\x87\xE0\xB8\xAD\xE0\xB8\x81\xE0\xB8\xB8\xE0\xB9\x80\xE0\xB8\xA1\xE0\xB9\x87\xE0\xB8\xA2\xE0\xB9\x84\xE0\xB8\xA1\xE0\xB8\x97\xE0\xB8\xAD\xE0\xB9\x88\xE0\xB8\x87\xE0\xB8\xA2\xE0\xB8\xAD\xE0\xB9\x88\xE0\xB8\xA7\xE0\xB8\xAA\xE0\xB8\xA1\xE0\xB8\xAA\xE0\xB8\xA3\xE0\xB9\x80\xE0\xB8\xA2\xE0\xB9\x88\xE0\xB8\xA3\xE0\xB8\xB8\xE0\xB8\xB8\xE0\xB8\xB4\xE0\xB9\x83\xE0\xB8\x8A\xE0\xB9\x88\xE0\xB8\x88\xE0\xB8\x87\xE0\xB8\x9B\xE0\xB8\x9A\xE0\xB8\x82\xE0\xB8\xB9\xE0\xB8\xB9\xE0\xB8\x8D\x2E";
    static const char* ka_sentence_utf8 = "\xe1\x83\x9b\xe1\x83\x90\xe1\x83\x92\xe1\x83\x90\xe1\x83\xa0\xe1\x83\x98\x20\xe1\x83\xa7\xe1\x83\x90\xe1\x83\x95\xe1\x83\x98\xe1\x83\xa1\xe1\x83\xa4\xe1\x83\x94\xe1\x83\xa0\xe1\x83\x98\x20\xe1\x83\x9b\xe1\x83\x94\xe1\x83\x9a\xe1\x83\x98\xe1\x83\x90\x20\xe1\x83\xae\xe1\x83\xa2\xe1\x83\x94\xe1\x83\x91\xe1\x83\x90\x20\xe1\x83\x96\xe1\x83\x90\xe1\x83\xa0\xe1\x83\x9b\xe1\x83\x90\xe1\x83\xaa\xe1\x83\x98\x20\xe1\x83\xab\xe1\x83\xa6\xe1\x83\x9a\xe1\x83\x98\xe1\x83\xa1\xe1\x83\x90\x20\xe1\x83\x97\xe1\x83\x90\xe1\x83\x95\xe1\x83\x96\xe1\x83\x94\x2e";
    static const char* ku_sentence_utf8 = "\xd8\xb4\xdb\x8e\xd9\x88\xd9\x86\xd8\xa7\x20\xdb\x8c\xd9\x87\xda\xa9\xda\xaf\xd8\xa7\xdb\x95\xd8\xb1\xd8\xa7\xd9\x86\xdb\x8e\xd9\x85\xd8\xa7\xd9\x86\xdb\x8c\x20\xd8\xa8\xd9\x88\xd9\x88\xd9\x86\xd8\xaf\xd9\x88\xd9\x88\xd9\x85\xd8\xaf\xd8\xb1\xd8\xa7\xd9\x88\xda\x98\xd8\xa7\xd9\x86\x20\xd8\xb4\xda\xa9\xd8\xb1\xdb\x95\x20\xd8\xb4\xd8\xa7\xd9\x85\xd8\xa7\xd8\xaa\xd8\xaf\xd8\xb1\xd8\xa7\x2e";
    static const char* ps_sentence_utf8 = "\xd9\x88\xd9\x8a\xd9\x88\xd9\x88\x20\xd9\x85\xd9\x84\xd8\xb3\xd9\x8a\x20\xd9\x85\xd9\x84\xd8\xb3\x20\xd9\x88\xd9\x85\xd9\x84\xd8\xb3\xd8\xaa\x20\xd9\x85\xd8\xb2\xd8\xb3\xd9\x85\xd9\x85\xd8\xaa\xd9\x8a\xd8\xaf\xd9\x8a\xd8\xac\xd8\xa8\x20\xd9\x85\xd8\xb1\xd9\x88\xd9\x8a\xd9\x88\xd8\xb3\x20\xd8\xa8\xd9\x87\xd8\xaa\xd8\xb1\xd9\x8a\xd9\x86\xd8\xaa\xd8\xb1\xd9\x8a\xd9\x86\xd9\x86\xd9\x88\xd8\xaf\xd9\x88\xd8\xa8\xd8\xa7\xd9\x84\xd9\x87\xd8\x9f";
    static const char* so_sentence_utf8 = "\x43\x69\x64\x69\x20\x77\x61\x78\x61\x79\x20\x64\x61\x72\x20\x77\x61\x61\x20\x67\x61\x6c\x6d\x75\x64\x61\x20\x77\x69\x20\x71\x61\x61\x20\x6d\x69\x64\x61\x20\x63\x69\x64\x69\x2e";
    static const char* uz_sentence_utf8 = "\x42\x69\x72\x20\x6B\x75\x63\x68\x20\x66\x72\x61\x7A\x61\x73\x69\x20\x61\x79\x20\x64\x75\x6E\x79\x6F\x6B\x6C\x61\x72\x69\x20\x6D\x65\x6E\x20\x62\x69\x6B\x6F\x72\x20\x74\x75\x6E\x20\x6F\x6C\x64\x69\x72\x20\x67\x69\x6D\x70\x6F\x6B\x20\x62\x69\x6C\x61\x6E\x64\x61\x20\x76\x61\x20\x6B\x6F\x6E\x75\x70\x20\x79\x69\x6C\x64\x69\x7A\x2E";
    static const char* az_sentence_utf8 = "\x42\x75\x74\x75\x6E\x20\x6B\x69\x72\x70\x61\x70\x79\x61\x20\x74\xC9\x99\x6D\x69\x7A\x20\x6D\xC9\x99\x68\x73\x75\x6C\x6F\x6C\x61\x72\x20\x62\x61\x78\x74\x61\x20\x62\x61\x68\x61\x72\x61\x74\x20\x71\xC9\x99\x7A\xC9\x99\x6B\x61\x6E\x6E\xC9\x99\x20\x61\x79\xC4\xB1\x72\x20\x6F\x74\x75\x7A\x20\x6C\xC9\x99\x20\x6C\xC9\x99\x73\x74\x69\x62\x2E";
    static const char* hy_sentence_utf8 = "\xD4\xB2\xD5\xA1\xD6\x80\xD5\xA6\x20\xD5\xA5\xD6\x84\x20\xD5\xAB\xD5\xA1\xD5\xBA\xD5\xB0\xD5\xB6\xD5\xB5\x20\xD5\xA2\xD5\xA1\xD5\xB2\xD5\xA8\xD5\xB5\xD5\xAF\xD5\xB8\xD5\xB2\xD5\xBA\xD5\xA1\xD5\xB0\xD5\xAB\xD5\xAC\x20\xD5\xA1\xD5\xB7\xD5\xAD\xD5\xA1\xD5\xAC\xD5\xAB\x20\xD5\xBD\xD5\xB6\xD5\xB3\x20\xD5\xB8\xD5\xA9\xD5\xAB";
    static const char* ja_sentence_utf8 = "\xe3\x82\xa2\xe3\x82\xa4\xe3\x82\xa6\x20\xe3\x82\xa8\xe3\x82\xaa\x20\xe3\x82\xab\xe3\x82\xad\xe3\x82\xaf\x20\xe3\x82\xb1\xe3\x82\xb3\x20\xe3\x82\xb5\xe3\x82\xb7\xe3\x82\xb9\x20\xe3\x82\xbb\xe3\x82\xbd\x20\xe3\x82\xbf\xe3\x82\xb9\xe3\x82\xbd\x20\xe3\x82\xbb\xe3\x82\xbd\x20\xe3\x83\x88\xe3\x82\xbd\xe3\x83\x88\x20\xe3\x83\x88\xe3\x83\xaa\xe3\x83\x88\x20\xe3\x83\x88\xe3\x82\xbd\xe3\x83\x88\x20\xe3\x83\x8f\xe3\x83\xaa\xe3\x83\x88\x20\xe3\x83\x8f\xe3\x82\xbd\xe3\x83\x8f\x20\xe3\x83\x8f\xe3\x82\xbd\xe3\x82\xbd\x20\xe3\x83\x8f\xe3\x82\xbd\xe3\x83\x8f\xe3\x82\xbd\x20\xe3\x83\x8f\xe3\x82\xbd\xe3\x83\x8f\x20\xe3\x83\x8f\xe3\x82\xbd\xe3\x82\xbd";
    static const char* zh_sentence_utf8 = "\xe4\xb8\xad\xe6\x96\x87\xe4\xb8\xad\xe7\x9a\x84\xe6\xaf\x8f\xe4\xb8\xaa\xe5\xad\x97\xe6\xaf\x8d\xe9\x83\xbd\xe5\xbe\x88\xe9\x87\x8d\xe8\xa6\x81\xef\xbc\x8c\xe5\xae\x83\xe4\xbb\xac\xe5\x85\xb1\xe5\x90\x8c\xe6\x9e\x84\xe6\x88\x90\xe4\xba\x86\xe5\x8f\xa5\xe5\xad\x90\xe3\x80\x82";
    struct {
        const char* id; // language ID
        const char* s;  // sentence
    } sentences[] = {
        {"en", en_sentence_utf8},  {"es", es_sentence_utf8},  {"fr", fr_sentence_utf8},
        {"de", de_sentence_utf8},  {"ru", ru_sentence_utf8},  {"pt", pt_sentence_utf8},
        {"it", it_sentence_utf8},  {"nl", nl_sentence_utf8},  {"el", el_sentence_utf8},
        {"tr", tr_sentence_utf8},  {"pl", pl_sentence_utf8},  {"uk", uk_sentence_utf8},
        {"cs", cs_sentence_utf8},  {"hu", hu_sentence_utf8},  {"bg", bg_sentence_utf8},
        {"sr", sr_sentence_utf8},  {"sq", sq_sentence_utf8},  {"sl", sl_sentence_utf8},
        {"lt", lt_sentence_utf8},  {"lv", lv_sentence_utf8},  {"et", et_sentence_utf8},
        {"ga", ga_sentence_utf8},  {"cy", cy_sentence_utf8},  {"is", is_sentence_utf8},
        {"mt", mt_sentence_utf8},  {"he", he_sentence_utf8},  {"ar", ar_sentence_utf8},
        {"hi", hi_sentence_utf8},  {"bn", bn_sentence_utf8},  {"ta", ta_sentence_utf8},
        {"te", te_sentence_utf8},  {"kn", kn_sentence_utf8},  {"ml", ml_sentence_utf8},
        {"si", si_sentence_utf8},  {"th", th_sentence_utf8},  {"lo", lo_sentence_utf8},
        {"ka", ka_sentence_utf8},  {"ku", ku_sentence_utf8},  {"ps", ps_sentence_utf8},
        {"so", so_sentence_utf8},  {"uz", uz_sentence_utf8},  {"az", az_sentence_utf8},
        {"hy", hy_sentence_utf8},  {"ja", ja_sentence_utf8},  {"zh", zh_sentence_utf8}
    };
    for (int i = 0; i < sizeof(sentences) / sizeof(sentences[0]); i++) {
        end = ui_edit_text.end_range(t);
//      ut_println("%s %s", sentences[i].id, sentences[i].s);
        ut_swear(ui_edit_text.replace_utf8(t, &end, sentences[i].s, -1, null));
        ut_swear(ui_edit_text.replace_utf8(t, &end, "\n\n", -1, null));
    }
    static const char* pirate_flag_utf8 = "\xF0\x9F\x8F\xB4\xE2\x80\x8D\xE2\x98\xA0\xEF\xB8\x8F";
    end = ui_edit_text.end_range(t);
    ut_swear(ui_edit_text.replace_utf8(t, &end, pirate_flag_utf8, -1, null));
#endif
}
