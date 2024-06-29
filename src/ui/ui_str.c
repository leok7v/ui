/* Copyright (c) Dmitry "Leo" Kuznetsov 2021-24 see LICENSE for details */
#include "ut/ut.h"
#include "ui/ui.h"

static int32_t ui_str_g2b_ascii[1024]; // ui_str_g2b_ascii[i] == i for all "i"
static int8_t  ui_str_empty[1] = {0x00};

static bool    ui_str_init(ui_str_t* s, const uint8_t* u, int32_t b, bool heap);
static void    ui_str_swap(ui_str_t* s1, ui_str_t* s2);
static int32_t ui_str_utf8_bytes(const uint8_t* u, int32_t b);
static int32_t ui_str_glyphs(const uint8_t* utf8, int32_t bytes);
static int32_t ui_str_gp_to_bp(const uint8_t* s, int32_t bytes, int32_t gp);
static int32_t ui_str_bytes(ui_str_t* s, int32_t f, int32_t t);
static bool    ui_str_expand(ui_str_t* s, int32_t c);
static void    ui_str_shrink(ui_str_t* s);
static bool    ui_str_replace(ui_str_t* s, int32_t f, int32_t t,
                              const uint8_t* u, int32_t b);
static bool    ui_edit_str_concatenate(ui_str_t* d, const ui_str_t* s1,
                                       const ui_str_t* s2);
static bool    ui_edit_str_substring(ui_str_t* d, const ui_str_t* s,
                                     int32_t f, int32_t t);
static void    ui_str_test(void);
static void    ui_str_free(ui_str_t* s);

ui_str_if ui_str = {
    .init        = ui_str_init,
    .swap        = ui_str_swap,
    .utf8bytes   = ui_str_utf8_bytes,
    .glyphs      = ui_str_glyphs,
    .gp_to_bp    = ui_str_gp_to_bp,
    .bytes       = ui_str_bytes,
    .expand      = ui_str_expand,
    .shrink      = ui_str_shrink,
    .replace     = ui_str_replace,
    .concatenate = ui_edit_str_concatenate,
    .substring   = ui_edit_str_substring,
    .test        = ui_str_test,
    .free        = ui_str_free,
    .empty       = { .u = (uint8_t*)ui_str_empty,
                     .g2b = ui_str_g2b_ascii,
                     .c = 0, .b = 0, .g = 0 }
};

#pragma push_macro("ui_str_check")
#pragma push_macro("ui_str_check_from_to")
#pragma push_macro("ui_str_check_zeros")
#pragma push_macro("ui_str_check_empty")
#pragma push_macro("ui_str_parameters")

#ifdef DEBUG

#define ui_str_check(s) do {                                        \
    /* check the s struct constrains */                             \
    assert(s->b >= 0);                                              \
    assert(s->c == 0 || s->c >= s->b);                              \
    assert(s->g >= 0);                                              \
    /* s->g2b[] may be null (not heap allocated) when .b == 0 */    \
    if (s->g == 0) { assert(s->b == 0); }                           \
    if (s->g > 0) {                                                 \
        assert(s->g2b[0] == 0 && s->g2b[s->g] == s->b);             \
    }                                                               \
    for (int32_t i = 1; i < s->g; i++) {                            \
        assert(0 < s->g2b[i] - s->g2b[i - 1] &&                     \
                   s->g2b[i] - s->g2b[i - 1] <= 4);                 \
        assert(s->g2b[i] - s->g2b[i - 1] ==                         \
            ui_str_utf8_bytes(                               \
            s->u + s->g2b[i - 1], s->g2b[i] - s->g2b[i - 1]));      \
    }                                                               \
} while (0)

#define ui_str_check_from_to(s, f, t) do {                          \
    assert(0 <= f && f <= s->g);                                    \
    assert(0 <= t && t <= s->g);                                    \
    assert(f <= t);                                                 \
} while (0)

#define ui_str_check_empty(u, b) do {                               \
    if (b == 0) { assert(u != null && u[0] == 0x00); }              \
    if (u == null || u[0] == 0x00) { assert(b == 0); }              \
} while (0)

// ui_str_check_zeros only works for packed structs:

#define ui_str_check_zeros(a_, b_) do {                             \
    for (int32_t i_ = 0; i_ < (b_); i_++) {                         \
        assert(((const uint8_t*)(a_))[i_] == 0x00);                 \
    }                                                               \
} while (0)

#else

#define ui_str_check(s)               do { } while (0)
#define ui_str_check_from_to(s, f, t) do { } while (0)
#define ui_str_check_zeros(a, b)      do { } while (0)
#define ui_str_check_empty(u, b)      do { } while (0)

#endif

// ui_str_foo(*, "...", -1) treat as 0x00 terminated
// ui_str_foo(*, null, 0) treat as ("", 0)

#define ui_str_parameters(u, b) do {                        \
    if (u == null) { u = (const uint8_t*)ui_str_empty; }    \
    if (b < 0)  {                                           \
        assert(strlen((const char*)u) < INT32_MAX);         \
        b = (int32_t)strlen((const char*)u);                \
    }                                                       \
    ui_str_check_empty(u, b);                               \
} while (0)

static int32_t ui_str_utf8_bytes(const uint8_t* u, int32_t b) {
    swear(b >= 1, "should not be called with bytes < 1");
    // based on:
    // https://stackoverflow.com/questions/66715611/check-for-valid-utf-8-encoding-in-c
    if ((u[0] & 0x80u) == 0x00u) { return 1; }
    if (b > 1) {
        uint32_t c = (u[0] << 8) | u[1];
        // TODO: 0xC080 is a hack - consider removing
        if (c == 0xC080) { return 2; } // 0xC080 as not zero terminating '\0'
        if (0xC280 <= c && c <= 0xDFBF && (c & 0xE0C0) == 0xC080) { return 2; }
        if (b > 2) {
            c = (c << 8) | u[2];
            // reject utf16 surrogates:
            if (0xEDA080 <= c && c <= 0xEDBFBF) { return 0; }
            if (0xE0A080 <= c && c <= 0xEFBFBF && (c & 0xF0C0C0) == 0xE08080) {
                return 3;
            }
            if (b > 3) {
                c = (c << 8) | u[3];
                if (0xF0908080 <= c && c <= 0xF48FBFBF &&
                    (c & 0xF8C0C0C0) == 0xF0808080) {
                    return 4;
                }
            }
        }
    }
    return 0; // invalid utf8 sequence
}

static int32_t ui_str_glyphs(const uint8_t* utf8, int32_t bytes) {
    swear(bytes >= 0);
    bool ok = true;
    int32_t i = 0;
    int32_t k = 1;
    while (i < bytes && ok) {
        const int32_t b = ui_str_utf8_bytes(utf8 + i, bytes - i);
        ok = 0 < b && i + b <= bytes;
        if (ok) { i += b; k++; }
    }
    return ok ? k - 1 : -1;
}

static int32_t ui_str_gp_to_bp(const uint8_t* utf8, int32_t bytes, int32_t gp) {
    swear(bytes >= 0);
    bool ok = true;
    int32_t c = 0;
    int32_t i = 0;
    if (bytes > 0) {
        while (c < gp && ok) {
            assert(i < bytes);
            const int32_t b = ui_str_utf8_bytes(utf8 + i, bytes - i);
            ok = 0 < b && i + b <= bytes;
            if (ok) { i += b; c++; }
        }
    }
    assert(i <= bytes);
    return ok ? i : -1;
}

static void ui_str_free(ui_str_t* s) {
    if (s->g2b != null && s->g2b != ui_str_g2b_ascii) {
        ut_heap.free(s->g2b);
    } else {
        // check if overwritten by bugs in the code:
        for (int32_t i = 0; i < countof(ui_str_g2b_ascii); i++) {
            assert(ui_str_g2b_ascii[i] == i);
        }
    }
    s->g2b = null;
    s->g = 0;
    if (s->c > 0) {
        ut_heap.free(s->u);
        s->u = null;
        s->c = 0;
        s->b = 0;
    } else {
        s->u = null;
        s->b = 0;
    }
    ui_str_check_zeros(s, sizeof(*s));
}

static bool ui_str_init_g2b(ui_str_t* s) {
    const int64_t _4_bytes = (int64_t)sizeof(int32_t);
    // start with number of glyphs == number of bytes (ASCII text):
    bool ok = ut_heap.alloc(&s->g2b, (s->b + 1) * _4_bytes) == 0;
    int32_t i = 0; // index in u[] string
    int32_t k = 1; // glyph number
    // g2b[k] start postion in uint8_t offset from utf8 text of glyph[k]
    while (i < s->b && ok) {
        const int32_t b = ui_str_utf8_bytes(s->u + i, s->b - i);
        ok = b > 0 && i + b <= s->b;
        if (ok) {
            i += b;
            s->g2b[k] = i;
            k++;
        }
    }
    if (ok) {
        assert(0 < k && k <= s->b + 1);
        s->g2b[0] = 0;
        assert(s->g2b[k - 1] == s->b);
        s->g = k - 1;
        if (k < s->b + 1) {
            ok = ut_heap.realloc(&s->g2b, k * _4_bytes) == 0;
            assert(ok, "shrinking - should always be ok");
        }
    }
    return ok;
}

static bool ui_str_init(ui_str_t* s, const uint8_t* u, int32_t b,
        bool heap) {
    enum { n = countof(ui_str_g2b_ascii) };
    if (ui_str_g2b_ascii[n - 1] != n - 1) {
        for (int32_t i = 0; i < n; i++) { ui_str_g2b_ascii[i] = i; }
    }
    bool ok = true;
    ui_str_check_zeros(s, sizeof(*s)); // caller must zero out
    memset(s, 0x00, sizeof(*s));
    ui_str_parameters(u, b);
    if (b == 0) { // cast below intentionally removes "const" qualifier
        s->g2b = (int32_t*)ui_str_g2b_ascii; // simplifies use cases
        s->u = (uint8_t*)u;
        assert(s->c == 0 && u[0] == 0x00);
    } else if (b == 1 && u[0] <= 0x7F) {
        s->g2b = (int32_t*)ui_str_g2b_ascii; // simplifies use cases
        s->u = (uint8_t*)u;
        s->g = 1;
        s->b = 1;
    } else {
        if (heap) {
            ok = ut_heap.alloc((void**)&s->u, b) == 0;
            if (ok) { s->c = b; memmove(s->u, u, b); }
        } else {
            s->u = (uint8_t*)u;
        }
        if (ok) {
            s->b = b;
            ok = ui_str_init_g2b(s);
        }
    }
    if (!ok) { ui_str_free(s); }
    return ok;
}

static void ui_str_swap(ui_str_t* s1, ui_str_t* s2) {
    ui_str_t s = *s1; *s1 = *s2; *s2 = s;
}

static int32_t ui_str_bytes(ui_str_t* s,
        int32_t f, int32_t t) { // glyph positions
    ui_str_check_from_to(s, f, t);
    ui_str_check(s);
    return s->g2b[t] - s->g2b[f];
}

static bool ui_str_move_g2b_to_heap(ui_str_t* s) {
    bool ok = true;
    if (s->g2b == ui_str_g2b_ascii) { // even for s->g == 0
        const int32_t bytes = (s->g + 1) * (int32_t)sizeof(int32_t);
        ok = ut_heap.alloc(&s->g2b, bytes) == 0;
        if (ok) { memmove(s->g2b, ui_str_g2b_ascii, bytes); }
    }
    return ok;
}

static bool ui_str_move_to_heap(ui_str_t* s, int32_t c) {
    bool ok = true;
    assert(c >= s->b, "can expand cannot shrink");
    if (s->c == 0) { // s->u points outside of the heap
        const uint8_t* o = s->u;
        ok = ut_heap.alloc((void**)&s->u, c) == 0;
        if (ok) {
            memmove(s->u, o, s->b);
        }
    } else if (s->c < c) {
        ok = ut_heap.realloc((void**)&s->u, c) == 0;
    }
    if (ok) { s->c = c; }
    if (ok) { ok = ui_str_move_g2b_to_heap(s); }
    return ok;
}

static bool ui_str_expand(ui_str_t* s, int32_t c) {
    swear(c > 0);
    bool ok = ui_str_move_to_heap(s, c);
    if (ok && c > s->c) {
        if (ut_heap.realloc((void**)&s->u, c) == 0) {
            s->c = c;
        } else {
            ok = false;
        }
    }
    return ok;
}

static void ui_str_shrink(ui_str_t* s) {
    if (s->c > s->b) { // s->c == 0 for empty and single byte ASCII strings
        assert(s->u != (const uint8_t*)ui_str_empty);
        if (s->b == 0) {
            ut_heap.free(s->u);
            s->u = (uint8_t*)ui_str_empty;
        } else {
            bool ok = ut_heap.realloc((void**)&s->u, s->b) == 0;
            swear(ok, "smaller size is always expected to be ok");
        }
        s->c = s->b;
        // Optimize memory for short ASCII only strings:
        if (s->g < countof(ui_str_g2b_ascii) - 1 && s->g == s->b) {
            // If this is an ascii only utf8 string shorter than
            // ui_str_g2b_ascii it does not need .g2b[] allocated:
            if (s->g2b != ui_str_g2b_ascii) {
                ut_heap.free(s->g2b);
                s->g2b = (int32_t*)ui_str_g2b_ascii;
            }
        }
    }
}

static bool ui_str_remove(ui_str_t* s, int32_t f, int32_t t) {
    bool ok = true; // optimistic approach
    ui_str_check_from_to(s, f, t);
    ui_str_check(s);
    const int32_t bytes_to_remove = s->g2b[t] - s->g2b[f];
    assert(bytes_to_remove >= 0);
    if (bytes_to_remove > 0) {
        ok = ui_str_move_to_heap(s, s->b);
        if (ok) {
            const int32_t bytes_to_shift = s->b - s->g2b[t];
            assert(0 <= bytes_to_shift && bytes_to_shift <= s->b);
            memmove(s->u + s->g2b[f], s->u + s->g2b[t], bytes_to_shift);
            if (s->g2b != ui_str_g2b_ascii) {
                memmove(s->g2b + f, s->g2b + t, (s->g - t + 1) * sizeof(int32_t));
                for (int32_t i = f; i <= s->g; i++) {
                    s->g2b[i] -= bytes_to_remove;
                }
            } else {
                // no need to shrink g2b[] for ASCII only strings:
                for (int32_t i = 0; i <= s->g; i++) { assert(s->g2b[i] == i); }
            }
            s->b -= bytes_to_remove;
            s->g -= t - f;
        }
    }
    ui_str_check(s);
    return ok;
}

static bool ui_edit_str_concatenate(ui_str_t* d, const ui_str_t* s1,
        const ui_str_t* s2) {
    return ui_str.init(d, s1->u, s1->b, false) &&
           ui_str.replace(d, d->g, d->g, s2->u, s2->b);
}

static bool ui_edit_str_substring(ui_str_t* d, const ui_str_t* s,
        int32_t f, int32_t t) {
    swear(f <= t);
    const int32_t b = s->g2b[t] - s->g2b[f];
    const uint8_t* u = b == 0 ? null : s->u + s->g2b[f];
    return ui_str.init(d, u, b, false);
}

static bool ui_str_replace(ui_str_t* s,
        int32_t f, int32_t t, const uint8_t* u, int32_t b) {
    const int64_t _4_bytes = (int64_t)sizeof(int32_t);
    bool ok = true; // optimistic approach
    ui_str_check_from_to(s, f, t);
    ui_str_check(s);
    ui_str_parameters(u, b);
    // we are inserting "b" bytes and removing "t - f" glyphs
    const int32_t bytes_to_remove = s->g2b[t] - s->g2b[f];
    const int32_t bytes_to_insert = b; // only for readability
    if (b == 0) { // just remove glyphs
        ok = ui_str_remove(s, f, t);
    } else { // remove and insert
        ui_str_t ins = {0};
        // ui_str_init_ro() verifies utf-8 and calculates g2b[]:
        ok = ui_str_init(&ins, u, b, false);
        const int32_t glyphs_to_insert = ins.g; // only for readability
        const int32_t glyphs_to_remove = t - f; // only for readability
        if (ok) {
            assert(ins.g2b != null); // pacify code analysis
            assert(s->b + bytes_to_insert - bytes_to_remove > 0);
            const int32_t c = ut_max(s->b,
                s->b + bytes_to_insert - bytes_to_remove);
            ok = ui_str_move_to_heap(s, c);
            if (ok) {
                // insert ui_str_t "ins" at glyph position "f"
                // reusing ins.u[0..ins.b-1] and ins.g2b[0..ins.g]
                // moving memory using memmove() left to right:
                if (bytes_to_insert <= bytes_to_remove) {
                    memmove(s->u + s->g2b[f] + bytes_to_insert,
                           s->u + s->g2b[f] + bytes_to_remove,
                           s->b - s->g2b[f] - bytes_to_remove);
                    assert(s->g2b != ui_str_g2b_ascii);
                    memmove(s->g2b + f + glyphs_to_insert,
                           s->g2b + f + glyphs_to_remove,
                           (s->g - t + 1) * _4_bytes);
                    memmove(s->u + s->g2b[f], ins.u, ins.b);
                } else {
                    assert(s->g2b != ui_str_g2b_ascii);
                    const int32_t g = s->g + glyphs_to_insert -
                                             glyphs_to_remove;
                    assert(g > s->g);
                    ok = ut_heap.realloc(&s->g2b, (g + 1) * _4_bytes) == 0;
                    // need to shift bytes staring with s.g2b[t] toward the end
                    if (ok) {
                        assert(s->g2b != ui_str_g2b_ascii);
                        memmove(s->u + s->g2b[f] + bytes_to_insert,
                                s->u + s->g2b[f] + bytes_to_remove,
                                s->b - s->g2b[f] - bytes_to_remove);
                        memmove(s->g2b + f + glyphs_to_insert,
                                s->g2b + f + glyphs_to_remove,
                                (s->g - t + 1) * _4_bytes);
                        memmove(s->u + s->g2b[f], ins.u, ins.b);
                    }
                }
                if (ok) {
                    assert(s->g2b != ui_str_g2b_ascii);
                    for (int32_t i = f; i <= f + glyphs_to_insert; i++) {
                        s->g2b[i] = ins.g2b[i - f] + s->g2b[f];
                    }
                    s->b += bytes_to_insert - bytes_to_remove;
                    s->g += glyphs_to_insert - glyphs_to_remove;
                    for (int32_t i = f + glyphs_to_insert + 1; i <= s->g; i++) {
                        s->g2b[i] += bytes_to_insert - bytes_to_remove;
                    }
                    s->g2b[s->g] = s->b;
                }
            }
            ui_str_free(&ins);
        }
    }
    ui_str_shrink(s);
    ui_str_check(s);
    return ok;
}

#pragma push_macro("ui_edit_usd")
#pragma push_macro("ui_edit_gbp")
#pragma push_macro("ui_edit_euro")
#pragma push_macro("ui_edit_money_bag")
#pragma push_macro("ui_edit_pot_of_honey")
#pragma push_macro("ui_edit_gothic_hwair")

#define ui_edit_usd             "\x24"
#define ui_edit_gbp             "\xC2\xA3"
#define ui_edit_euro            "\xE2\x82\xAC"
// https://www.compart.com/en/unicode/U+1F4B0
#define ui_edit_money_bag       "\xF0\x9F\x92\xB0"
// https://www.compart.com/en/unicode/U+1F36F
#define ui_edit_pot_of_honey    "\xF0\x9F\x8D\xAF"
// https://www.compart.com/en/unicode/U+10348
#define ui_edit_gothic_hwair    "\xF0\x90\x8D\x88" // Gothic Letter Hwair

static void ui_str_test_replace(void) { // exhaustive permutations
    // Exhaustive 9,765,625 replace permutations may take
    // up to 5 minutes of CPU time in release.
    // Recommended to be invoked at least once after making any
    // changes to ui_str.replace and around.
    // Menu: Debug / Windows / Show Diagnostic Tools allows to watch
    //       memory pressure for whole 3 minutes making sure code is
    //       not leaking memory profusely.
    const char* gs[] = { // glyphs
        ui_str_empty, ui_edit_usd, ui_edit_gbp, ui_edit_euro, ui_edit_money_bag
    };
    const int32_t gb[] = {0, 1, 2, 3, 4}; // number of bytes per codepoint
    enum { n = countof(gs) };
    int32_t npn = 1; // n to the power of n
    for (int32_t i = 0; i < n; i++) { npn *= n; }
    int32_t gix_src[n] = {0};
    // 5^5 = 3,125   3,125 * 3,125 = 9,765,625
    for (int32_t i = 0; i < npn; i++) {
        int32_t vi = i;
        for (int32_t j = 0; j < n; j++) {
            gix_src[j] = vi % n;
            vi /= n;
        }
        int32_t g2p[n + 1] = {0};
        int32_t ngx = 1; // next glyph index
        char src[128] = {0};
        for (int32_t j = 0; j < n; j++) {
            if (gix_src[j] > 0) {
                strcat(src, gs[gix_src[j]]);
                assert(1 <= ngx && ngx <= n);
                g2p[ngx] = g2p[ngx - 1] + gb[gix_src[j]];
                ngx++;
            }
        }
        if (i % 100 == 99) {
            traceln("%2d%% [%d][%d][%d][%d][%d] "
                    "\"%s\",\"%s\",\"%s\",\"%s\",\"%s\": \"%s\"",
                (i * 100) / npn,
                gix_src[0], gix_src[1], gix_src[2], gix_src[3], gix_src[4],
                gs[gix_src[0]], gs[gix_src[1]], gs[gix_src[2]],
                gs[gix_src[3]], gs[gix_src[4]], src);
        }
        ui_str_t s = {0};
        // reference constructor does not copy to heap:
        bool ok = ui_str_init(&s, (const uint8_t*)src, -1, false);
        swear(ok);
        for (int32_t f = 0; f <= s.g; f++) { // from
            for (int32_t t = f; t <= s.g; t++) { // to
                int32_t gix_rep[n] = {0};
                // replace range [f, t] with all possible glyphs sequences:
                for (int32_t k = 0; k < npn; k++) {
                    int32_t vk = i;
                    for (int32_t j = 0; j < n; j++) {
                        gix_rep[j] = vk % n;
                        vk /= n;
                    }
                    char rep[128] = {0};
                    for (int32_t j = 0; j < n; j++) { strcat(rep, gs[gix_rep[j]]); }
                    char e1[128] = {0}; // expected based on s.g2b[]
                    snprintf(e1, countof(e1), "%.*s%s%.*s",
                        s.g2b[f], src,
                        rep,
                        s.b - s.g2b[t], src + s.g2b[t]
                    );
                    char e2[128] = {0}; // expected based on gs[]
                    snprintf(e2, countof(e1), "%.*s%s%.*s",
                        g2p[f], src,
                        rep,
                        (int32_t)strlen(src) - g2p[t], src + g2p[t]
                    );
                    swear(strcmp(e1, e2) == 0,
                        "s.u[%d:%d]: \"%.*s\" g:%d [%d:%d] rep=\"%s\" "
                        "e1: \"%s\" e2: \"%s\"",
                        s.b, s.c, s.b, s.u, s.g, f, t, rep, e1, e2);
                    ui_str_t c = {0}; // copy
                    ok = ui_str_init(&c, (const uint8_t*)src, -1, true);
                    swear(ok);
                    ok = ui_str_replace(&c, f, t, (const uint8_t*)rep, -1);
                    swear(ok);
                    swear(memcmp(c.u, e1, c.b) == 0,
                           "s.u[%d:%d]: \"%.*s\" g:%d [%d:%d] rep=\"%s\" "
                           "expected: \"%s\"",
                           s.b, s.c, s.b, s.u, s.g,
                           f, t, rep, e1);
                    ui_str_free(&c);
                }
            }
        }
        ui_str_free(&s);
    }
}

static void ui_str_test_glyph_bytes(void) {
    #pragma push_macro("glyph_bytes_test")
    #define glyph_bytes_test(s, b, expectancy) \
        swear(ui_str_utf8_bytes((const uint8_t*)s, b) == expectancy)
    // Valid Sequences
    glyph_bytes_test("a", 1, 1);
    glyph_bytes_test(ui_edit_gbp, 2, 2);
    glyph_bytes_test(ui_edit_euro, 3, 3);
    glyph_bytes_test(ui_edit_gothic_hwair, 4, 4);
    // Invalid Continuation Bytes
    glyph_bytes_test("\xC2\x00", 2, 0);
    glyph_bytes_test("\xE0\x80\x00", 3, 0);
    glyph_bytes_test("\xF0\x80\x80\x00", 4, 0);
    // Overlong Encodings
    glyph_bytes_test("\xC0\xAF", 2, 0); // '!'
    glyph_bytes_test("\xE0\x9F\xBF", 3, 0); // upside down '?'
    glyph_bytes_test("\xF0\x80\x80\xBF", 4, 0); // '~'
    // UTF-16 Surrogates
    glyph_bytes_test("\xED\xA0\x80", 3, 0); // High surrogate
    glyph_bytes_test("\xED\xBF\xBF", 3, 0); // Low surrogate
    // Code Points Outside Valid Range
    glyph_bytes_test("\xF4\x90\x80\x80", 4, 0); // U+110000
    // Invalid Initial Bytes
    glyph_bytes_test("\xC0", 1, 0);
    glyph_bytes_test("\xC1", 1, 0);
    glyph_bytes_test("\xF5", 1, 0);
    glyph_bytes_test("\xFF", 1, 0);
    // 5-byte sequence (always invalid)
    glyph_bytes_test("\xF8\x88\x80\x80\x80", 5, 0);
    #pragma pop_macro("glyph_bytes_test")
}

static void ui_str_test(void) {
    ui_str_test_glyph_bytes();
    {
        ui_str_t s = {0};
        bool ok = ui_str_init(&s, (const uint8_t*)"hello", -1, false);
        swear(ok);
        swear(s.b == 5 && s.c == 0 && memcmp(s.u, "hello", 5) == 0);
        swear(s.g == 5 && s.g2b != null);
        for (int32_t i = 0; i <= s.g; i++) {
            swear(s.g2b[i] == i);
        }
        ui_str_free(&s);
    }
    const char* currencies = ui_edit_usd  ui_edit_gbp
                             ui_edit_euro ui_edit_money_bag;
    const uint8_t* money = (const uint8_t*)currencies;
    {
        ui_str_t s = {0};
        const int32_t n = (int32_t)strlen(currencies);
        bool ok = ui_str_init(&s, money, n, true);
        swear(ok);
        swear(s.b == n && s.c == s.b && memcmp(s.u, money, s.b) == 0);
        swear(s.g == 4 && s.g2b != null);
        const int32_t g2b[] = {0, 1, 3, 6, 10};
        for (int32_t i = 0; i <= s.g; i++) {
            swear(s.g2b[i] == g2b[i]);
        }
        ui_str_free(&s);
    }
    {
        ui_str_t s = {0};
        bool ok = ui_str_init(&s, (const uint8_t*)"hello", -1, false);
        swear(ok);
        ok = ui_str_replace(&s, 1, 4, null, 0);
        swear(ok);
        swear(s.b == 2 && memcmp(s.u, "ho", 2) == 0);
        swear(s.g == 2 && s.g2b[0] == 0 && s.g2b[1] == 1 && s.g2b[2] == 2);
        ui_str_free(&s);
    }
    {
        ui_str_t s = {0};
        bool ok = ui_str_init(&s, (const uint8_t*)"Hello world", -1, false);
        swear(ok);
        ok = ui_str_replace(&s, 5, 6, (const uint8_t*)" cruel ", -1);
        swear(ok);
        ok = ui_str_replace(&s, 0, 5, (const uint8_t*)"Goodbye", -1);
        swear(ok);
        ok = ui_str_replace(&s, s.g - 5, s.g, (const uint8_t*)"Universe", -1);
        swear(ok);
        swear(s.g == 22 && s.g2b[0] == 0 && s.g2b[s.g] == s.b);
        for (int32_t i = 1; i < s.g; i++) {
            swear(s.g2b[i] == i); // because every glyph is ASCII
        }
        swear(memcmp(s.u, "Goodbye cruel Universe", 22) == 0);
        ui_str_free(&s);
    }
    #ifdef UI_STR_TEST_REPLACE_ALL_PERMUTATIONS
        ui_str_test_replace();
    #else
        (void)(void*)ui_str_test_replace; // mitigate unused warning
    #endif
}

#pragma push_macro("ui_edit_gothic_hwair")
#pragma push_macro("ui_edit_pot_of_honey")
#pragma push_macro("ui_edit_money_bag")
#pragma push_macro("ui_edit_euro")
#pragma push_macro("ui_edit_gbp")
#pragma push_macro("ui_edit_usd")

#pragma pop_macro("ui_str_parameters")
#pragma pop_macro("ui_str_check_empty")
#pragma pop_macro("ui_str_check_zeros")
#pragma pop_macro("ui_str_check_from_to")
#pragma pop_macro("ui_str_check")

// For quick and dirty test uncomment next line:
// ut_static_init(ui_str) { ui_str.test(); }
