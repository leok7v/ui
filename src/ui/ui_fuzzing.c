/* Copyright (c) Dmitry "Leo" Kuznetsov 2021-24 see LICENSE for details */
#include "ut/ut.h"
#include "ui/ui.h"

// TODO: Ctrl+A Ctrl+V Ctrl+C Ctrl+X Ctrl+Z Ctrl+Y

static bool     ui_fuzzing_debug = true;
static uint32_t ui_fuzzing_seed;
static bool     ui_fuzzing_running;
static bool     ui_fuzzing_inside;

static ui_fuzzing_t ui_fuzzing_work;

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

#define ui_fuzzing_lorem_ipsum_canonique \
    "Lorem ipsum dolor sit amet, consectetur adipiscing elit, sed do "         \
    "eiusmod  tempor incididunt ut labore et dolore magna aliqua.Ut enim ad "  \
    "minim veniam, quis nostrud exercitation ullamco laboris nisi ut aliquip " \
    "ex ea commodo consequat. Duis aute irure dolor in reprehenderit in "      \
    "voluptate velit esse cillum dolore eu fugiat nulla pariatur. Excepteur "  \
    "sint occaecat cupidatat non proident, sunt in culpa qui officia "         \
    "deserunt mollit anim id est laborum."

#define ui_fuzzing_lorem_ipsum_chinese \
    "\xE6\x88\x91\xE6\x98\xAF\xE6\x94\xBE\xE7\xBD\xAE\xE6\x96\x87\xE6\x9C\xAC\xE7\x9A\x84\xE4" \
    "\xBD\x8D\xE7\xBD\xAE\xE3\x80\x82\xE8\xBF\x99\xE9\x87\x8C\xE6\x94\xBE\xE7\xBD\xAE\xE4\xBA" \
    "\x86\xE5\x81\x87\xE6\x96\x87\xE5\x81\x87\xE5\xAD\x97\xE3\x80\x82\xE5\xB8\x8C\xE6\x9C\x9B" \
    "\xE8\xBF\x99\xE4\xBA\x9B\xE6\x96\x87\xE5\xAD\x97\xE5\x8F\xAF\xE4\xBB\xA5\xE5\xA1\xAB\xE5" \
    "\x85\x85\xE7\xA9\xBA\xE7\x99\xBD\xE3\x80\x82";

#define ui_fuzzing_lorem_ipsum_japanese \
    "\xE3\x81\x93\xE3\x82\x8C\xE3\x81\xAF\xE3\x83\x80\xE3\x83\x9F\xE3\x83\xBC\xE3\x83\x86\xE3" \
    "\x82\xAD\xE3\x82\xB9\xE3\x83\x88\xE3\x81\xA7\xE3\x81\x99\xE3\x80\x82\xE3\x81\x93\xE3\x81" \
    "\x93\xE3\x81\xAB\xE6\x96\x87\xE7\xAB\xA0\xE3\x81\x8C\xE5\x85\xA5\xE3\x82\x8A\xE3\x81\xBE" \
    "\xE3\x81\x99\xE3\x80\x82\xE8\xAA\xAD\xE3\x81\xBF\xE3\x82\x84\xE3\x81\x99\xE3\x81\x84\xE3" \
    "\x82\x88\xE3\x81\x86\xE3\x81\xAB\xE3\x83\x80\xE3\x83\x9F\xE3\x83\xBC\xE3\x83\x86\xE3\x82" \
    "\xAD\xE3\x82\xB9\xE3\x83\x88\xE3\x82\x92\xE4\xBD\xBF\xE7\x94\xA8\xE3\x81\x97\xE3\x81\xA6" \
    "\xE3\x81\x84\xE3\x81\xBE\xE3\x81\x99\xE3\x80\x82";


#define ui_fuzzing_lorem_ipsum_korean \
    "\xEC\x9D\xB4\xEA\xB2\x83\xEC\x9D\x80\x20\xEB\x8D\x94\xEB\xAF\xB8\x20\xED\x85\x8D\xEC\x8A" \
    "\xA4\xED\x8A\xB8\xEC\x9E\x85\xEB\x8B\x88\xEB\x8B\xA4\x2E\x20\xEC\x97\xAC\xEA\xB8\xB0\xEC" \
    "\x97\x90\x20\xEB\xAC\xB8\xEC\x9E\x90\xEA\xB0\x80\x20\xEB\x93\x9C\xEC\x96\xB4\xEA\xB0\x80" \
    "\xEB\x8A\x94\x20\xEB\xAC\xB8\xEC\x9E\x90\xEA\xB0\x80\x20\xEC\x9E\x88\xEB\x8B\xA4\x2E\x20" \
    "\xEC\x9D\xBD\xEA\xB8\xB0\x20\xEC\x89\xBD\xEA\xB2\x8C\x20\xEB\x8D\x94\xEB\xAF\xB8\x20\xED" \
    "\x85\x8D\xEC\x8A\xA4\xED\x8A\xB8\xEB\xA5\xBC\x20\xEC\x82\xAC\xEC\x9A\xA9\xED\x95\xA9\xEB" \
    "\x8B\x88\xEB\x8B\xA4\x2E";

#define ui_fuzzing_lorem_ipsum_emoji \
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
} ui_fuzzing_generator_params_t;

static uint32_t ui_fuzzing_random(void) {
    return ut_num.random32(&ui_fuzzing_seed);
}

static fp64_t ui_fuzzing_random_fp64(void) {
    uint32_t r = ui_fuzzing_random();
    return (fp64_t)r / (fp64_t)UINT32_MAX;
}

static void ui_fuzzing_generator(ui_fuzzing_generator_params_t p) {
    rt_fatal_if(p.count < 1024); // at least 1KB expected
    rt_fatal_if_not(0 < p.min_paragraphs && p.min_paragraphs <= p.max_paragraphs);
    rt_fatal_if_not(0 < p.min_sentences && p.min_sentences <= p.max_sentences);
    rt_fatal_if_not(2 < p.min_words && p.min_words <= p.max_words);
    char* s = p.text;
    // assume longest word is less than 128
    char* end = p.text + p.count - 128;
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

static void ui_fuzzing_next_gibberish(int32_t number_of_characters,
        char text[]) {
    static fp64_t freq[96] = {
        0.1716, 0.0023, 0.0027, 0.0002, 0.0001, 0.0005, 0.0013, 0.0012,
        0.0015, 0.0014, 0.0017, 0.0002, 0.0084, 0.0020, 0.0075, 0.0040,
        0.0135, 0.0045, 0.0053, 0.0053, 0.0047, 0.0047, 0.0043, 0.0047,
        0.0057, 0.0044, 0.0037, 0.0004, 0.0016, 0.0004, 0.0017, 0.0017,
        0.0020, 0.0045, 0.0026, 0.0020, 0.0027, 0.0021, 0.0025, 0.0026,
        0.0030, 0.0025, 0.0021, 0.0018, 0.0028, 0.0026, 0.0024, 0.0020,
        0.0025, 0.0026, 0.0030, 0.0022, 0.0027, 0.0022, 0.0020, 0.0023,
        0.0015, 0.0016, 0.0009, 0.0005, 0.0005, 0.0001, 0.0003, 0.0003,
        0.0078, 0.0013, 0.0012, 0.0008, 0.0012, 0.0007, 0.0006, 0.0011,
        0.0016, 0.0012, 0.0011, 0.0004, 0.0004, 0.0016, 0.0013, 0.0009,
        0.0009, 0.0008, 0.0013, 0.0011, 0.0013, 0.0012, 0.0006, 0.0007,
        0.0011, 0.0005, 0.0007, 0.0003, 0.0002, 0.0006, 0.0002, 0.0005
    };
    static fp64_t cumulative_freq[96];
    static bool initialized = 0;
    if (!initialized) {
        cumulative_freq[0] = freq[0];
        for (int i = 1; i < ut_countof(freq); i++) {
            cumulative_freq[i] = cumulative_freq[i - 1] + freq[i];
        }
        initialized = 1;
    }
    int32_t i = 0;
    while (i < number_of_characters) {
        text[i] = 0x00;
        fp64_t r = ui_fuzzing_random_fp64();
        for (int j = 0; j < 96 && text[i] == 0; j++) {
            if (r < cumulative_freq[j]) {
                text[i] = (char)(0x20 + j);
            }
        }
        if (text[i] != 0) { i++; }
    }
    text[number_of_characters] = 0x00;
}

static void ui_fuzzing_dispatch(ui_fuzzing_t* work) {
    rt_swear(work == &ui_fuzzing_work);
    ui_app.alt = work->alt;
    ui_app.ctrl = work->ctrl;
    ui_app.shift = work->shift;
    if (work->utf8 != null && work->utf8[0] != 0) {
        ui_view.character(ui_app.content, work->utf8);
        work->utf8 = work->utf8[1] == 0 ? null : work->utf8++;
    } else if (work->key != 0) {
        ui_view.key_pressed(ui_app.content, work->key);
        ui_view.key_released(ui_app.content, work->key);
        work->key = 0;
    } else if (work->pt != null) {
        const int32_t x = work->pt->x;
        const int32_t y = work->pt->y;
        ui_app.mouse.x = x;
        ui_app.mouse.y = y;
//      https://stackoverflow.com/questions/22259936/
//      https://stackoverflow.com/questions/65691101/
//      ut_println("%d,%d", x + ui_app.wrc.x, y + ui_app.wrc.y);
//      // next line works only when running as administrator:
//      ut_fatal_win32err(SetCursorPos(x + ui_app.wrc.x, y + ui_app.wrc.y));
        const bool l_button = ui_app.mouse_left  != work->left;
        const bool r_button = ui_app.mouse_right != work->right;
        ui_app.mouse_left  = work->left;
        ui_app.mouse_right = work->right;
        ui_view.mouse_move(ui_app.content);
        if (l_button) {
            ui_view.tap(ui_app.content, 0, work->left);
        }
        if (r_button) {
            ui_view.tap(ui_app.content, 2, work->right);
        }
        work->pt = null;
    } else {
        ut_assert(false, "TODO: ?");
    }
    if (ui_fuzzing_running) {
        if (ui_fuzzing.next == null) {
            ui_fuzzing.next_random(work);
        } else {
            ui_fuzzing.next(work);
        }
    }
}

static void ui_fuzzing_do_work(ut_work_t* p) {
    if (ui_fuzzing_running) {
        ui_fuzzing_inside = true;
        if (ui_fuzzing.custom != null) {
            ui_fuzzing.custom((ui_fuzzing_t*)p);
        } else {
            ui_fuzzing.dispatch((ui_fuzzing_t*)p);
        }
        ui_fuzzing_inside = false;
    } else {
        // fuzzing has been .stop()-ed drop it
    }
}

static void ui_fuzzing_post(void) {
    ui_app.post(&ui_fuzzing_work.base);
}

static void ui_fuzzing_alt_ctrl_shift(void) {
    ui_fuzzing_t* w = &ui_fuzzing_work;
    switch (ui_fuzzing_random() % 8) {
        case 0: w->alt = 0; w->ctrl = 0; w->shift = 0; break;
        case 1: w->alt = 1; w->ctrl = 0; w->shift = 0; break;
        case 2: w->alt = 0; w->ctrl = 1; w->shift = 0; break;
        case 3: w->alt = 1; w->ctrl = 1; w->shift = 0; break;
        case 4: w->alt = 0; w->ctrl = 0; w->shift = 1; break;
        case 5: w->alt = 1; w->ctrl = 0; w->shift = 1; break;
        case 6: w->alt = 0; w->ctrl = 1; w->shift = 1; break;
        case 7: w->alt = 1; w->ctrl = 1; w->shift = 1; break;
        default: ut_assert(false);
    }
}

static void ui_fuzzing_character(void) {
    static char utf8[4 * 1024];
    if (ui_fuzzing_work.utf8 == null) {
        fp64_t r = ui_fuzzing_random_fp64();
        if (r < 0.125) {
            uint32_t rnd = ui_fuzzing_random();
            int32_t n = (int32_t)rt_max(1, rnd % 32);
            ui_fuzzing_next_gibberish(n, utf8);
            ui_fuzzing_work.utf8 = utf8;
            if (ui_fuzzing_debug) {
    //          ut_println("%s", utf8);
            }
        } else if (r < 0.25) {
            ui_fuzzing_work.utf8 = ui_fuzzing_lorem_ipsum_chinese;
        } else if (r < 0.375) {
            ui_fuzzing_work.utf8 = ui_fuzzing_lorem_ipsum_japanese;
        } else if (r < 0.5) {
            ui_fuzzing_work.utf8 = ui_fuzzing_lorem_ipsum_korean;
        } else if (r < 0.5 + 0.125) {
            ui_fuzzing_work.utf8 = ui_fuzzing_lorem_ipsum_emoji;
        } else {
            ui_fuzzing_work.utf8 = ui_fuzzing_lorem_ipsum_canonique;
        }
    }
    ui_fuzzing_post();
}

static void ui_fuzzing_key(void) {
    struct {
        int32_t key;
        const char* name;
    } keys[] = {
        { ui.key.up,      "up",     },
        { ui.key.down,    "down",   },
        { ui.key.left,    "left",   },
        { ui.key.right,   "right",  },
        { ui.key.home,    "home",   },
        { ui.key.end,     "end",    },
        { ui.key.pageup,  "pgup",   },
        { ui.key.pagedw,  "pgdw",   },
        { ui.key.insert,  "insert"  },
        { ui.key.enter,   "enter"   },
        { ui.key.del,     "delete"  },
        { ui.key.back,    "back"   },
    };
    ui_fuzzing_alt_ctrl_shift();
    uint32_t ix = ui_fuzzing_random() % ut_countof(keys);
    if (ui_fuzzing_debug) {
//      ut_println("key(%s)", keys[ix].name);
    }
    ui_fuzzing_work.key = keys[ix].key;
    ui_fuzzing_post();
}

static void ui_fuzzing_mouse(void) {
    // mouse events only inside edit control otherwise
    // they will start clicking buttons around
    ui_view_t* v = ui_app.content;
    ui_fuzzing_t* w = &ui_fuzzing_work;
    int32_t x = ui_fuzzing_random() % v->w;
    int32_t y = ui_fuzzing_random() % v->h;
    static ui_point_t pt;
    pt = (ui_point_t){ x + v->x, y + v->y };
    if (ui_fuzzing_random() % 2) {
        w->left  = !w->left;
    }
    if (ui_fuzzing_random() % 2) {
        w->right = !w->right;
    }
    if (ui_fuzzing_debug) {
//      ut_println("mouse(%d,%d) %s%s", pt.x, pt.y,
//              w->left ? "L" : "_", w->right ? "R" : "_");
    }
    w->pt = &pt;
    ui_fuzzing_post();
}

static void ui_fuzzing_start(uint32_t seed) {
    ui_fuzzing_seed = seed | 0x1;
    ui_fuzzing_running = true;
    if (ui_fuzzing.next == null) {
        ui_fuzzing.next_random(&ui_fuzzing_work);
    } else {
        ui_fuzzing.next(&ui_fuzzing_work);
    }
}

static bool ui_fuzzing_is_running(void) {
    return ui_fuzzing_running;
}

static bool ui_fuzzing_from_inside(void) {
    return ui_fuzzing_inside;
}

static void ui_fuzzing_stop(void) {
    ui_fuzzing_running = false;
}

static void ui_fuzzing_next_random(ui_fuzzing_t* f) {
    rt_swear(f == &ui_fuzzing_work);
    ui_fuzzing_work = (ui_fuzzing_t){
        .base = { .when = rt_clock.seconds() + 0.001, // 1ms
                  .work = ui_fuzzing_do_work },
    };
    uint32_t rnd = ui_fuzzing_random() % 100;
    if (rnd < 80) {
        ui_fuzzing_character();
    } else if (rnd < 90) {
        ui_fuzzing_key();
    } else {
        ui_fuzzing_mouse();
    }
}

ui_fuzzing_if ui_fuzzing = {
    .start       = ui_fuzzing_start,
    .is_running  = ui_fuzzing_is_running,
    .from_inside = ui_fuzzing_from_inside,
    .next_random = ui_fuzzing_next_random,
    .dispatch    = ui_fuzzing_dispatch,
    .next        = null,
    .custom      = null,
    .stop        = ui_fuzzing_stop
};
