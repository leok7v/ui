/* Copyright (c) Dmitry "Leo" Kuznetsov 2021-24 see LICENSE for details */
#include "ut/ut.h"
#include "ui/ui.h"

// TODO: undo/redo
// TODO: back/forward navigation
// TODO: exit/save keyboard shortcuts?
// TODO: iBeam cursor

// http://worrydream.com/refs/Tesler%20-%20A%20Personal%20History%20of%20Modeless%20Text%20Editing%20and%20Cut-Copy-Paste.pdf
// https://web.archive.org/web/20221216044359/http://worrydream.com/refs/Tesler%20-%20A%20Personal%20History%20of%20Modeless%20Text%20Editing%20and%20Cut-Copy-Paste.pdf

// Rich text options that are not addressed yet:
// * Color of ranges (useful for code editing)
// * Soft line breaks inside the paragraph (useful for e.g. bullet lists of options)
// * Bold/Italic/Underline (along with color ranges)
// * Multiple fonts (as long as run vertical size is the maximum of font)
// * Kerning (?! like in overhung "Fl")

// When implementation and header are amalgamated
// into a single file header library name_space is
// used to separate different modules namespaces.

typedef  struct ui_edit_glyph_s {
    const char* s;
    int32_t bytes;
} ui_edit_glyph_t;

static void ui_edit_layout(ui_view_t* v);

// Glyphs in monospaced Windows fonts may have different width for non-ASCII
// characters. Thus even if edit is monospaced glyph measurements are used
// in text layout.

static uint64_t ui_edit_uint64(int32_t high, int32_t low) {
    assert(high >= 0 && low >= 0);
    return ((uint64_t)high << 32) | (uint64_t)low;
}

// TODO:
// All allocate/free functions assume 'fail fast' semantics
// if underlying OS runs out of RAM it considered to be fatal.
// It is possible to implement and hold committed 'safety region'
// of RAM and free it to general pull or reuse it on alloc() or
// reallocate() returning null, try to notify user about low memory
// conditions and attempt to save edited work but all of the
// above may only work if there is no other run-away code that
// consumes system memory at a very high rate.

static void* ui_edit_alloc(int32_t bytes) {
    void* p = null;
    errno_t r = ut_heap.alloc(&p, bytes);
    swear(r == 0 && p != null); // fatal
    return p;
}

static void ui_edit_allocate(void** pp, int32_t count, size_t element) {
    not_null(pp);
    assert(count > 0 && (int64_t)count * (int64_t)element <= (int64_t)INT_MAX);
    *pp = ui_edit_alloc(count * (int32_t)element);
}

static void ui_edit_free(void** pp) {
    not_null(pp);
    // free(null) is acceptable but may indicate unbalanced caller logic
    not_null(*pp);
    ut_heap.free(*pp);
    *pp = null;
}

static void ui_edit_reallocate(void** pp, int32_t count, size_t element) {
    not_null(pp);
    assert(count > 0 && (int64_t)count * (int64_t)element <= (int64_t)INT_MAX);
    if (*pp == null) {
        ui_edit_allocate(pp, count, element);
    } else {
        errno_t r = ut_heap.realloc(pp, (int64_t)count * (int64_t)element);
        swear(r == 0 && *pp != null); // intentionally fatal
    }
}

static void ui_edit_invalidate(ui_edit_t* e) {
    ui_view.invalidate(&e->view);
}

static int32_t ui_edit_text_width(ui_edit_t* e, const char* s, int32_t n) {
//  fp64_t time = ut_clock.seconds();
    // average GDI measure_text() performance per character:
    // "ui_app.fonts.mono"    ~500us (microseconds)
    // "ui_app.fonts.regular" ~250us (microseconds)
    const ui_gdi_ta_t ta = { .fm = e->view.fm, .color = e->view.color,
                             .measure = true };
    int32_t x = n == 0 ? 0 : ui_gdi.text(&ta, 0, 0, "%.*s", n, s).w;
//  TODO: remove
//  int32_t x = n == 0 ? 0 : ui_gdi.measure_text(e->view.fm, "%.*s", n, s).w;

//  time = (ut_clock.seconds() - time) * 1000.0;
//  static fp64_t time_sum;
//  static fp64_t length_sum;
//  time_sum += time;
//  length_sum += n;
//  traceln("avg=%.6fms per char total %.3fms", time_sum / length_sum, time_sum);
    return x;
}

static int32_t ui_edit_glyph_bytes(char start_byte_value) { // utf-8
    // return 1-4 bytes glyph starting with `start_byte_value` character
    uint8_t uc = (uint8_t)start_byte_value;
    // 0xxxxxxx
    if ((uc & 0x80) == 0x00) { return 1; }
    // 110xxxxx 10xxxxxx 0b1100=0xE 0x1100=0xC
    if ((uc & 0xE0) == 0xC0) { return 2; }
    // 1110xxxx 10xxxxxx 10xxxxxx 0b1111=0xF 0x1110=0xE
    if ((uc & 0xF0) == 0xE0) { return 3; }
    // 11110xxx 10xxxxxx 10xxxxxx 10xxxxxx 0b1111,1000=0xF8 0x1111,0000=0xF0
    if ((uc & 0xF8) == 0xF0) { return 4; }
// TODO: should NOT be fatal: try editing .exe file to see the crash
    fatal_if(true, "incorrect UTF first byte 0%02X", uc);
    return -1;
}

// g2b() return number of glyphs in text and fills optional
// g2b[] array with glyphs positions.

static int32_t ui_edit_g2b(const char* utf8, int32_t bytes, int32_t g2b[]) {
    int32_t i = 0;
    int32_t k = 1;
    // g2b[k] start postion in uint8_t offset from utf8 text of glyph[k]
    if (g2b != null) { g2b[0] = 0; }
    while (i < bytes) {
        i += ui_edit_glyph_bytes(utf8[i]);
        if (g2b != null) { g2b[k] = i; }
        k++;
    }
    return k - 1;
}

static int32_t ui_edit_glyphs(const char* utf8, int32_t bytes) {
    return ui_edit_g2b(utf8, bytes, null);
}

static int32_t ui_edit_gp_to_bytes(const char* s, int32_t bytes, int32_t gp) {
    int32_t c = 0;
    int32_t i = 0;
    if (bytes > 0) {
        while (c < gp) {
            assert(i < bytes);
            i += ui_edit_glyph_bytes(s[i]);
            c++;
        }
    }
    assert(i <= bytes);
    return i;
}

static void ui_edit_paragraph_g2b(ui_edit_t* e, int32_t pn) {
    assert(0 <= pn && pn < e->paragraphs);
    ui_edit_para_t* p = &e->para[pn];
    if (p->glyphs < 0) {
        const int32_t bytes = p->bytes;
        const int32_t n = p->bytes + 1;
        const int32_t a = (n * (int32_t)sizeof(int32_t)) * 3 / 2; // heuristic
        if (p->g2b_capacity < a) {
            ui_edit_reallocate((void**)&p->g2b, n, sizeof(int32_t));
            p->g2b_capacity = a;
        }
        const char* utf8 = p->text;
        p->g2b[0] = 0; // first glyph starts at 0
        int32_t i = 0;
        int32_t k = 1;
        // g2b[k] start postion in uint8_t offset from utf8 text of glyph[k]
        while (i < bytes) {
            i += ui_edit_glyph_bytes(utf8[i]);
            p->g2b[k] = i;
            k++;
        }
        p->glyphs = k - 1;
    }
}

static int32_t ui_edit_word_break_at(ui_edit_t* e, int32_t pn, int32_t rn,
        const int32_t width, bool allow_zero) {
    ui_edit_para_t* p = &e->para[pn];
    int32_t k = 1; // at least 1 glyph
    // offsets inside a run in glyphs and bytes from start of the paragraph:
    int32_t gp = p->run[rn].gp;
    int32_t bp = p->run[rn].bp;
    if (gp < p->glyphs - 1) {
        const char* text = p->text + bp;
        const int32_t glyphs_in_this_run = p->glyphs - gp;
        int32_t* g2b = &p->g2b[gp];
        // 4 is maximum number of bytes in a UTF-8 sequence
        int32_t gc = ut_min(4, glyphs_in_this_run);
        int32_t w = ui_edit_text_width(e, text, g2b[gc] - bp);
        while (gc < glyphs_in_this_run && w < width) {
            gc = ut_min(gc * 4, glyphs_in_this_run);
            w = ui_edit_text_width(e, text, g2b[gc] - bp);
        }
        if (w < width) {
            k = gc;
            assert(1 <= k && k <= p->glyphs - gp);
        } else {
            int32_t i = 0;
            int32_t j = gc;
            k = (i + j) / 2;
            while (i < j) {
                assert(allow_zero || 1 <= k && k < gc + 1);
                const int32_t n = g2b[k + 1] - bp;
                int32_t px = ui_edit_text_width(e, text, n);
                if (px == width) { break; }
                if (px < width) { i = k + 1; } else { j = k; }
                if (!allow_zero && (i + j) / 2 == 0) { break; }
                k = (i + j) / 2;
                assert(allow_zero || 1 <= k && k <= p->glyphs - gp);
            }
        }
    }
    assert(allow_zero || 1 <= k && k <= p->glyphs - gp);
    return k;
}

static int32_t ui_edit_word_break(ui_edit_t* e, int32_t pn, int32_t rn) {
    return ui_edit_word_break_at(e, pn, rn, e->w, false);
}

static int32_t ui_edit_glyph_at_x(ui_edit_t* e, int32_t pn, int32_t rn,
        int32_t x) {
    if (x == 0 || e->para[pn].bytes == 0) {
        return 0;
    } else {
        return ui_edit_word_break_at(e, pn, rn, x + 1, true);
    }
}

static ui_edit_glyph_t ui_edit_glyph_at(ui_edit_t* e, ui_edit_pg_t p) {
    ui_edit_glyph_t g = { .s = "", .bytes = 0 };
    if (p.pn == e->paragraphs) {
        assert(p.gp == 0); // last empty paragraph
    } else {
        ui_edit_paragraph_g2b(e, p.pn);
        const int32_t bytes = e->para[p.pn].bytes;
        char* s = e->para[p.pn].text;
        const int32_t bp = e->para[p.pn].g2b[p.gp];
        if (bp < bytes) {
            g.s = s + bp;
            g.bytes = ui_edit_glyph_bytes(*g.s);
//          traceln("glyph: %.*s 0x%02X bytes: %d", g.bytes, g.s, *g.s, g.bytes);
        }
    }
    return g;
}

// paragraph_runs() breaks paragraph into `runs` according to `width`

static const ui_edit_run_t* ui_edit_paragraph_runs(ui_edit_t* e, int32_t pn,
        int32_t* runs) {
//  fp64_t time = ut_clock.seconds();
    assert(e->view.w > 0);
    const ui_edit_run_t* r = null;
    if (pn == e->paragraphs) {
        static const ui_edit_run_t eof_run = { 0 };
        *runs = 1;
        r = &eof_run;
    } else if (e->para[pn].run != null) {
        *runs = e->para[pn].runs;
        r = e->para[pn].run;
    } else {
        assert(0 <= pn && pn < e->paragraphs);
        ui_edit_paragraph_g2b(e, pn);
        ui_edit_para_t* p = &e->para[pn];
        if (p->run == null) {
            assert(p->runs == 0 && p->run == null);
            const int32_t max_runs = p->bytes + 1;
            ui_edit_allocate((void**)&p->run, max_runs, sizeof(ui_edit_run_t));
            ui_edit_run_t* run = p->run;
            run[0].bp = 0;
            run[0].gp = 0;
            int32_t gc = p->bytes == 0 ? 0 : ui_edit_word_break(e, pn, 0);
            if (gc == p->glyphs) { // whole paragraph fits into width
                p->runs = 1;
                run[0].bytes  = p->bytes;
                run[0].glyphs = p->glyphs;
                int32_t pixels = ui_edit_text_width(e, p->text, p->g2b[gc]);
                run[0].pixels = pixels;
            } else {
                assert(gc < p->glyphs);
                int32_t rc = 0; // runs count
                int32_t ix = 0; // glyph index from to start of paragraph
                char* text = p->text;
                int32_t bytes = p->bytes;
                while (bytes > 0) {
                    assert(rc < max_runs);
                    run[rc].bp = (int32_t)(text - p->text);
                    run[rc].gp = ix;
                    int32_t glyphs = ui_edit_word_break(e, pn, rc);
                    int32_t utf8bytes = p->g2b[ix + glyphs] - run[rc].bp;
                    int32_t pixels = ui_edit_text_width(e, text, utf8bytes);
                    if (glyphs > 1 && utf8bytes < bytes && text[utf8bytes - 1] != 0x20) {
                        // try to find word break SPACE character. utf8 space is 0x20
                        int32_t i = utf8bytes;
                        while (i > 0 && text[i - 1] != 0x20) { i--; }
                        if (i > 0 && i != utf8bytes) {
                            utf8bytes = i;
                            glyphs = ui_edit_glyphs(text, utf8bytes);
                            pixels = ui_edit_text_width(e, text, utf8bytes);
                        }
                    }
                    run[rc].bytes  = utf8bytes;
                    run[rc].glyphs = glyphs;
                    run[rc].pixels = pixels;
                    rc++;
                    text += utf8bytes;
                    assert(0 <= utf8bytes && utf8bytes <= bytes);
                    bytes -= utf8bytes;
                    ix += glyphs;
                }
                assert(rc > 0);
                p->runs = rc; // truncate heap capacity array:
                ui_edit_reallocate((void**)&p->run, rc, sizeof(ui_edit_run_t));
            }
        }
        *runs = p->runs;
        r = p->run;
    }
    assert(r != null && *runs >= 1);
//  time = ut_clock.seconds() - time;
//  traceln("%.3fms", time * 1000.0);
    return r;
}

static int32_t ui_edit_paragraph_run_count(ui_edit_t* e, int32_t pn) {
    swear(e->view.w > 0);
    int32_t runs = 0;
    if (e->view.w > 0 && 0 <= pn && pn < e->paragraphs) {
        (void)ui_edit_paragraph_runs(e, pn, &runs);
    }
    return runs;
}

static int32_t ui_edit_glyphs_in_paragraph(ui_edit_t* e, int32_t pn) {
    (void)ui_edit_paragraph_run_count(e, pn); // word break into runs
    return e->para[pn].glyphs;
}

static void ui_edit_create_caret(ui_edit_t* e) {
    fatal_if(e->focused);
    assert(ui_app.is_active());
    assert(ui_app.has_focus());
    int32_t caret_width = ut_min(2, ut_max(1, ui_app.dpi.monitor_effective / 100));
//  traceln("%d,%d", caret_width, e->view.fm->height);
    ui_app.create_caret(caret_width, e->view.fm->height);
    e->focused = true; // means caret was created
}

static void ui_edit_destroy_caret(ui_edit_t* e) {
    fatal_if(!e->focused);
    ui_app.destroy_caret();
    e->focused = false; // means caret was destroyed
//  traceln("");
}

static void ui_edit_show_caret(ui_edit_t* e) {
    if (e->focused) {
        assert(ui_app.is_active());
        assert(ui_app.has_focus());
        ui_app.move_caret(e->view.x + e->caret.x, e->view.y + e->caret.y);
        // TODO: it is possible to support unblinking caret if desired
        // do not set blink time - use global default
//      fatal_if_false(SetCaretBlinkTime(500));
        ui_app.show_caret();
        e->shown++;
//      traceln("shown=%d", e->shown);
        assert(e->shown == 1);
    }
}

static void ui_edit_hide_caret(ui_edit_t* e) {
    if (e->focused) {
        ui_app.hide_caret();
        e->shown--;
//      traceln("shown=%d", e->shown);
        assert(e->shown == 0);
    }
}

static void ui_edit_dispose_paragraphs_layout(ui_edit_t* e) {
    for (int32_t i = 0; i < e->paragraphs; i++) {
        ui_edit_para_t* p = &e->para[i];
        if (p->run != null) {
            ui_edit_free((void**)&p->run);
        }
        if (p->g2b != null) {
            ui_edit_free((void**)&p->g2b);
        }
        p->glyphs = -1;
        p->runs = 0;
        p->g2b_capacity = 0;
    }
}

static void ui_edit_layout_now(ui_edit_t* e) {
    if (e->view.measure != null && e->view.layout != null && e->view.w > 0) {
        ui_edit_dispose_paragraphs_layout(e);
        e->view.layout(&e->view);
        ui_edit_invalidate(e);
    }
}

static void ui_edit_if_sle_layout(ui_edit_t* e) {
    // only for single line edit controls that were already initialized
    // and measured horizontally at least once.
    if (e->sle && e->view.layout != null && e->view.w > 0) {
        ui_edit_layout_now(e);
    }
}

static void ui_edit_set_font(ui_edit_t* e, ui_fm_t* f) {
    ui_edit_dispose_paragraphs_layout(e);
    e->scroll.rn = 0;
    e->view.fm = f;
    ui_edit_layout_now(e);
    ui_app.request_layout();
}

// Paragraph number, glyph number -> run number

static ui_edit_pr_t ui_edit_pg_to_pr(ui_edit_t* e, const ui_edit_pg_t pg) {
    ui_edit_pr_t pr = { .pn = pg.pn, .rn = -1 };
    if (pg.pn == e->paragraphs || e->para[pg.pn].bytes == 0) { // last or empty
        assert(pg.gp == 0);
        pr.rn = 0;
    } else {
        assert(0 <= pg.pn && pg.pn < e->paragraphs);
        int32_t runs = 0;
        const ui_edit_run_t* run = ui_edit_paragraph_runs(e, pg.pn, &runs);
        if (pg.gp == e->para[pg.pn].glyphs + 1) {
            pr.rn = runs - 1; // TODO: past last glyph ??? is this correct?
        } else {
            assert(0 <= pg.gp && pg.gp <= e->para[pg.pn].glyphs);
            for (int32_t j = 0; j < runs && pr.rn < 0; j++) {
                const int32_t last_run = j == runs - 1;
                const int32_t start = run[j].gp;
                const int32_t end = run[j].gp + run[j].glyphs + last_run;
                if (start <= pg.gp && pg.gp < end) {
                    pr.rn = j;
                }
            }
            assert(pr.rn >= 0);
        }
    }
    return pr;
}

static int32_t ui_edit_runs_between(ui_edit_t* e, const ui_edit_pg_t pg0,
        const ui_edit_pg_t pg1) {
    assert(ui_edit_uint64(pg0.pn, pg0.gp) <= ui_edit_uint64(pg1.pn, pg1.gp));
    int32_t rn0 = ui_edit_pg_to_pr(e, pg0).rn;
    int32_t rn1 = ui_edit_pg_to_pr(e, pg1).rn;
    int32_t rc = 0;
    if (pg0.pn == pg1.pn) {
        assert(rn0 <= rn1);
        rc = rn1 - rn0;
    } else {
        assert(pg0.pn < pg1.pn);
        for (int32_t i = pg0.pn; i < pg1.pn; i++) {
            const int32_t runs = ui_edit_paragraph_run_count(e, i);
            if (i == pg0.pn) {
                rc += runs - rn0;
            } else { // i < pg1.pn
                rc += runs;
            }
        }
        rc += rn1;
    }
    return rc;
}

static ui_edit_pg_t ui_edit_scroll_pg(ui_edit_t* e) {
    int32_t runs = 0;
    const ui_edit_run_t* run = ui_edit_paragraph_runs(e, e->scroll.pn, &runs);
    assert(0 <= e->scroll.rn && e->scroll.rn < runs,
            "e->scroll.rn: %d runs: %d", e->scroll.rn, runs);
    return (ui_edit_pg_t) { .pn = e->scroll.pn, .gp = run[e->scroll.rn].gp };
}

static int32_t ui_edit_first_visible_run(ui_edit_t* e, int32_t pn) {
    return pn == e->scroll.pn ? e->scroll.rn : 0;
}

// ui_edit::pg_to_xy() paragraph # glyph # -> (x,y) in [0,0  width x height]

static ui_point_t ui_edit_pg_to_xy(ui_edit_t* e, const ui_edit_pg_t pg) {
    ui_point_t pt = { .x = -1, .y = 0 };
    for (int32_t i = e->scroll.pn; i < e->paragraphs && pt.x < 0; i++) {
        int32_t runs = 0;
        const ui_edit_run_t* run = ui_edit_paragraph_runs(e, i, &runs);
        for (int32_t j = ui_edit_first_visible_run(e, i); j < runs; j++) {
            const int32_t last_run = j == runs - 1;
            int32_t gc = run[j].glyphs;
            if (i == pg.pn) {
                // in the last `run` of a paragraph x after last glyph is OK
                if (run[j].gp <= pg.gp && pg.gp < run[j].gp + gc + last_run) {
                    const char* s = e->para[i].text + run[j].bp;
                    int32_t ofs = ui_edit_gp_to_bytes(s, run[j].bytes,
                        pg.gp - run[j].gp);
                    pt.x = ui_edit_text_width(e, s, ofs);
                    break;
                }
            }
            pt.y += e->view.fm->height;
        }
    }
    if (pg.pn == e->paragraphs) { pt.x = e->inside.left; }
    if (0 <= pt.x && pt.x < e->w && 0 <= pt.y && pt.y < e->h) {
        // all good, inside visible rectangle or right after it
    } else {
        traceln("(%d,%d) outside of %dx%d", pt.x, pt.y, e->w, e->h);
    }
    return pt;
}

static int32_t ui_edit_glyph_width_px(ui_edit_t* e, const ui_edit_pg_t pg) {
    char* text = e->para[pg.pn].text;
    int32_t gc = e->para[pg.pn].glyphs;
    if (pg.gp == 0 &&  gc == 0) {
        return 0; // empty paragraph
    } else if (pg.gp < gc) {
        char* s = text + ui_edit_gp_to_bytes(text, e->para[pg.pn].bytes, pg.gp);
        int32_t bytes_in_glyph = ui_edit_glyph_bytes(*s);
        int32_t x = ui_edit_text_width(e, s, bytes_in_glyph);
        return x;
    } else {
        assert(pg.gp == gc, "only next position past last glyph is allowed");
        return 0;
    }
}

// xy_to_pg() (x,y) (0,0, width x height) -> paragraph # glyph #

static ui_edit_pg_t ui_edit_xy_to_pg(ui_edit_t* e, int32_t x, int32_t y) {
    ui_edit_pg_t pg = {-1, -1};
    int32_t py = 0; // paragraph `y' coordinate
    for (int32_t i = e->scroll.pn; i < e->paragraphs && pg.pn < 0; i++) {
        int32_t runs = 0;
        const ui_edit_run_t* run = ui_edit_paragraph_runs(e, i, &runs);
        for (int32_t j = ui_edit_first_visible_run(e, i); j < runs && pg.pn < 0; j++) {
            const ui_edit_run_t* r = &run[j];
            char* s = e->para[i].text + run[j].bp;
            if (py <= y && y < py + e->view.fm->height) {
                int32_t w = ui_edit_text_width(e, s, r->bytes);
                pg.pn = i;
                if (x >= w) {
                    const int32_t last_run = j == runs - 1;
                    pg.gp = r->gp + ut_max(0, r->glyphs - 1 + last_run);
                } else {
                    pg.gp = r->gp + ui_edit_glyph_at_x(e, i, j, x);
                    if (pg.gp < r->glyphs - 1) {
                        ui_edit_pg_t right = {pg.pn, pg.gp + 1};
                        int32_t x0 = ui_edit_pg_to_xy(e, pg).x;
                        int32_t x1 = ui_edit_pg_to_xy(e, right).x;
                        if (x1 - x < x - x0) {
                            pg.gp++; // snap to closest glyph's 'x'
                        }
                    }
                }
            } else {
                py += e->view.fm->height;
            }
        }
        if (py > e->view.h) { break; }
    }
    if (pg.pn < 0 && pg.gp < 0) {
        pg.pn = e->paragraphs;
        pg.gp = 0;
    }
    return pg;
}

static void ui_edit_paint_selection(ui_edit_t* e, int32_t y, const ui_edit_run_t* r,
        const char* text, int32_t pn, int32_t c0, int32_t c1) {
    uint64_t s0 = ui_edit_uint64(e->selection[0].pn, e->selection[0].gp);
    uint64_t e0 = ui_edit_uint64(e->selection[1].pn, e->selection[1].gp);
    if (s0 > e0) {
        uint64_t swap = e0;
        e0 = s0;
        s0 = swap;
    }
    uint64_t s1 = ui_edit_uint64(pn, c0);
    uint64_t e1 = ui_edit_uint64(pn, c1);
    if (s0 <= e1 && s1 <= e0) {
        uint64_t start = ut_max(s0, s1) - (uint64_t)c0;
        uint64_t end = ut_min(e0, e1) - (uint64_t)c0;
        if (start < end) {
            int32_t fro = (int32_t)start;
            int32_t to  = (int32_t)end;
            int32_t ofs0 = ui_edit_gp_to_bytes(text, r->bytes, fro);
            int32_t ofs1 = ui_edit_gp_to_bytes(text, r->bytes, to);
            int32_t x0 = ui_edit_text_width(e, text, ofs0);
            int32_t x1 = ui_edit_text_width(e, text, ofs1);
            // selection_color is MSVC dark mode selection color
            // TODO: need light mode selection color tpp
            ui_color_t selection_color = ui_rgb(0x26, 0x4F, 0x78); // ui_rgb(64, 72, 96);
            if (!e->focused || !ui_app.has_focus()) {
                selection_color = ui_colors.darken(selection_color, 0.1f);
            }
            const ui_ltrb_t insets = ui_view.gaps(&e->view, &e->view.insets);
            int32_t x = e->view.x + insets.left;
            ui_gdi.fill(x + x0, y,
                             x1 - x0, e->view.fm->height, selection_color);
        }
    }
}

static int32_t ui_edit_paint_paragraph(ui_edit_t* e,
        const ui_gdi_ta_t* ta, int32_t x, int32_t y, int32_t pn) {
    int32_t runs = 0;
    const ui_edit_run_t* run = ui_edit_paragraph_runs(e, pn, &runs);
    for (int32_t j = ui_edit_first_visible_run(e, pn);
                 j < runs && y < e->view.y + e->inside.bottom; j++) {
        char* text = e->para[pn].text + run[j].bp;
        ui_edit_paint_selection(e, y, &run[j], text, pn,
                                run[j].gp, run[j].gp + run[j].glyphs);
        ui_gdi.text(ta, x, y, "%.*s", run[j].bytes, text);
        if (j < runs - 1 && !e->hide_word_wrap) {
            ui_gdi.text(ta, x + e->w, y, "%s",
                        ut_glyph_south_west_arrow_with_hook);
        }
        y += e->view.fm->height;
    }
    return y;
}

static void ui_edit_set_caret(ui_edit_t* e, int32_t x, int32_t y) {
    if (e->caret.x != x || e->caret.y != y) {
        if (e->focused && ui_app.has_focus()) {
            ui_app.move_caret(e->view.x + x, e->view.y + y);
//          traceln("%d,%d", e->view.x + x, e->view.y + y);
        }
        e->caret.x = x;
        e->caret.y = y;
    }
}

// scroll_up() text moves up (north) in the visible view,
// scroll position increments moves down (south)

static void ui_edit_scroll_up(ui_edit_t* e, int32_t run_count) {
    assert(0 < run_count, "does it make sense to have 0 scroll?");
    const ui_edit_pg_t eof = {.pn = e->paragraphs, .gp = 0};
    while (run_count > 0 && e->scroll.pn < e->paragraphs) {
        ui_edit_pg_t scroll = ui_edit_scroll_pg(e);
        int32_t between = ui_edit_runs_between(e, scroll, eof);
        if (between <= e->visible_runs - 1) {
            run_count = 0; // enough
        } else {
            int32_t runs = ui_edit_paragraph_run_count(e, e->scroll.pn);
            if (e->scroll.rn < runs - 1) {
                e->scroll.rn++;
            } else if (e->scroll.pn < e->paragraphs) {
                e->scroll.pn++;
                e->scroll.rn = 0;
            }
            run_count--;
            assert(e->scroll.pn >= 0 && e->scroll.rn >= 0);
        }
    }
    ui_edit_if_sle_layout(e);
    ui_edit_invalidate(e);
}

// scroll_dw() text moves down (south) in the visible view,
// scroll position decrements moves up (north)

static void ui_edit_scroll_down(ui_edit_t* e, int32_t run_count) {
    assert(0 < run_count, "does it make sense to have 0 scroll?");
    while (run_count > 0 && (e->scroll.pn > 0 || e->scroll.rn > 0)) {
        int32_t runs = ui_edit_paragraph_run_count(e, e->scroll.pn);
        e->scroll.rn = ut_min(e->scroll.rn, runs - 1);
        if (e->scroll.rn == 0 && e->scroll.pn > 0) {
            e->scroll.pn--;
            e->scroll.rn = ui_edit_paragraph_run_count(e, e->scroll.pn) - 1;
        } else if (e->scroll.rn > 0) {
            e->scroll.rn--;
        }
        assert(e->scroll.pn >= 0 && e->scroll.rn >= 0);
        assert(0 <= e->scroll.rn &&
                    e->scroll.rn < ui_edit_paragraph_run_count(e, e->scroll.pn));
        run_count--;
    }
    ui_edit_if_sle_layout(e);
}

static void ui_edit_scroll_into_view(ui_edit_t* e, const ui_edit_pg_t pg) {
    if (e->paragraphs > 0 && e->inside.bottom > 0) {
        if (e->sle) { assert(pg.pn == 0); }
        const int32_t rn = ui_edit_pg_to_pr(e, pg).rn;
        const uint64_t scroll = ui_edit_uint64(e->scroll.pn, e->scroll.rn);
        const uint64_t caret  = ui_edit_uint64(pg.pn, rn);
        uint64_t last = 0;
        int32_t py = 0;
        const int32_t pn = e->scroll.pn;
        const int32_t bottom = e->inside.bottom;
        for (int32_t i = pn; i < e->paragraphs && py < bottom; i++) {
            int32_t runs = ui_edit_paragraph_run_count(e, i);
            const int32_t fvr = ui_edit_first_visible_run(e, i);
            for (int32_t j = fvr; j < runs && py < bottom; j++) {
                last = ui_edit_uint64(i, j);
                py += e->view.fm->height;
            }
        }
        int32_t sle_runs = e->sle && e->view.w > 0 ?
            ui_edit_paragraph_run_count(e, 0) : 0;
        ui_edit_paragraph_g2b(e, e->paragraphs - 1);
        ui_edit_pg_t last_paragraph = {.pn = e->paragraphs - 1,
            .gp = e->para[e->paragraphs - 1].glyphs };
        ui_edit_pr_t lp = ui_edit_pg_to_pr(e, last_paragraph);
        uint64_t eof = ui_edit_uint64(e->paragraphs - 1, lp.rn);
        if (last == eof && py <= bottom - e->view.fm->height) {
            // vertical white space for EOF on the screen
            last = ui_edit_uint64(e->paragraphs, 0);
        }
        if (scroll <= caret && caret < last) {
            // no scroll
        } else if (caret < scroll) {
            e->scroll.pn = pg.pn;
            e->scroll.rn = rn;
        } else if (e->sle && sle_runs * e->view.fm->height <= e->view.h) {
            // single line edit control fits vertically - no scroll
        } else {
            assert(caret >= last);
            e->scroll.pn = pg.pn;
            e->scroll.rn = rn;
            while (e->scroll.pn > 0 || e->scroll.rn > 0) {
                ui_point_t pt = ui_edit_pg_to_xy(e, pg);
                if (pt.y + e->view.fm->height > bottom - e->view.fm->height) { break; }
                if (e->scroll.rn > 0) {
                    e->scroll.rn--;
                } else {
                    e->scroll.pn--;
                    e->scroll.rn = ui_edit_paragraph_run_count(e, e->scroll.pn) - 1;
                }
            }
        }
    }
}

static void ui_edit_move_caret(ui_edit_t* e, const ui_edit_pg_t pg) {
    // single line edit control cannot move caret past fist paragraph
    if (e->paragraphs == 0) {
        ui_edit_set_caret(e, e->inside.left, e->inside.top);
    } else {
        if (!e->sle || pg.pn < e->paragraphs) {
            ui_edit_scroll_into_view(e, pg);
            ui_point_t pt = e->view.w > 0 ? // width == 0 means no measure/layout yet
                ui_edit_pg_to_xy(e, pg) : (ui_point_t){0, 0};
            ui_edit_set_caret(e, pt.x + e->inside.left, pt.y + e->inside.top);
            e->selection[1] = pg;
            if (!ui_app.shift && e->mouse == 0) {
                e->selection[0] = e->selection[1];
            }
            ui_edit_invalidate(e);
        }
    }
}

static char* ui_edit_ensure(ui_edit_t* e, int32_t pn, int32_t bytes,
        int32_t preserve) {
    assert(bytes >= 0 && preserve <= bytes);
    if (bytes <= e->para[pn].capacity) {
        // enough memory already capacity - do nothing
    } else if (e->para[pn].capacity > 0) {
        assert(preserve <= e->para[pn].capacity);
        ui_edit_reallocate((void**)&e->para[pn].text, bytes, 1);
        fatal_if_null(e->para[pn].text);
        e->para[pn].capacity = bytes;
    } else {
        assert(e->para[pn].capacity == 0);
        char* text = ui_edit_alloc(bytes);
        e->para[pn].capacity = bytes;
        memcpy(text, e->para[pn].text, (size_t)preserve);
        e->para[pn].text = text;
        e->para[pn].bytes = preserve;
    }
    return e->para[pn].text;
}

static ui_edit_pg_t ui_edit_op(ui_edit_t* e, bool cut,
        ui_edit_pg_t from, ui_edit_pg_t to,
        char* text, int32_t* bytes) {
    #pragma push_macro("ui_edit_clip_append")
    #define ui_edit_clip_append(a, ab, mx, text, bytes) do {   \
        int32_t ba = (int32_t)(bytes); /* bytes to append */   \
        if (a != null) {                                       \
            assert(ab <= mx);                                  \
            memcpy(a, text, (size_t)ba);                       \
            a += ba;                                           \
        }                                                      \
        ab += ba;                                              \
    } while (0)
    char* a = text; // append
    int32_t ab = 0; // appended bytes
    int32_t limit = bytes != null ? *bytes : 0; // max byes in text
    uint64_t f = ui_edit_uint64(from.pn, from.gp);
    uint64_t t = ui_edit_uint64(to.pn, to.gp);
    if (f != t) {
        ui_edit_dispose_paragraphs_layout(e);
        if (f > t) { uint64_t swap = t; t = f; f = swap; }
        int32_t pn0 = (int32_t)(f >> 32);
        int32_t gp0 = (int32_t)(f);
        int32_t pn1 = (int32_t)(t >> 32);
        int32_t gp1 = (int32_t)(t);
        if (pn1 == e->paragraphs) { // last empty paragraph
            assert(gp1 == 0);
            pn1 = e->paragraphs - 1;
            gp1 = ui_edit_g2b(e->para[pn1].text, e->para[pn1].bytes, null);
        }
        const int32_t bytes0 = e->para[pn0].bytes;
        char* s0 = e->para[pn0].text;
        char* s1 = e->para[pn1].text;
        ui_edit_paragraph_g2b(e, pn0);
        const int32_t bp0 = e->para[pn0].g2b[gp0];
        if (pn0 == pn1) { // inside same paragraph
            const int32_t bp1 = e->para[pn0].g2b[gp1];
            ui_edit_clip_append(a, ab, limit, s0 + bp0, bp1 - bp0);
            if (cut) {
                if (e->para[pn0].capacity == 0) {
                    int32_t n = bytes0 - (bp1 - bp0);
                    s0 = ui_edit_alloc(n);
                    memcpy(s0, e->para[pn0].text, (size_t)bp0);
                    e->para[pn0].text = s0;
                    e->para[pn0].capacity = n;
                }
                assert(bytes0 - bp1 >= 0);
                memcpy(s0 + bp0, s1 + bp1, (size_t)(bytes0 - bp1));
                e->para[pn0].bytes -= (bp1 - bp0);
                e->para[pn0].glyphs = -1; // will relayout
            }
        } else {
            ui_edit_clip_append(a, ab, limit, s0 + bp0, bytes0 - bp0);
            ui_edit_clip_append(a, ab, limit, "\n", 1);
            for (int32_t i = pn0 + 1; i < pn1; i++) {
                ui_edit_clip_append(a, ab, limit, e->para[i].text,
                                                  e->para[i].bytes);
                ui_edit_clip_append(a, ab, limit, "\n", 1);
            }
            const int32_t bytes1 = e->para[pn1].bytes;
            ui_edit_paragraph_g2b(e, pn1);
            const int32_t bp1 = e->para[pn1].g2b[gp1];
            ui_edit_clip_append(a, ab, limit, s1, bp1);
            if (cut) {
                int32_t total = bp0 + bytes1 - bp1;
                s0 = ui_edit_ensure(e, pn0, total, bp0);
                assert(bytes1 - bp1 >= 0);
                memcpy(s0 + bp0, s1 + bp1, (size_t)(bytes1 - bp1));
                e->para[pn0].bytes = bp0 + bytes1 - bp1;
                e->para[pn0].glyphs = -1; // will relayout
            }
        }
        int32_t deleted = cut ? pn1 - pn0 : 0;
        if (deleted > 0) {
            assert(pn0 + deleted < e->paragraphs);
            for (int32_t i = pn0 + 1; i <= pn0 + deleted; i++) {
                if (e->para[i].capacity > 0) {
                    ui_edit_free((void**)&e->para[i].text);
                }
            }
            for (int32_t i = pn0 + 1; i < e->paragraphs - deleted; i++) {
                e->para[i] = e->para[i + deleted];
            }
            for (int32_t i = e->paragraphs - deleted; i < e->paragraphs; i++) {
                memset(&e->para[i], 0, sizeof(e->para[i]));
            }
        }
        if (t == ui_edit_uint64(e->paragraphs, 0)) {
            ui_edit_clip_append(a, ab, limit, "\n", 1);
        }
        if (a != null) { assert(a == text + limit); }
        e->paragraphs -= deleted;
        from.pn = pn0;
        from.gp = gp0;
        // traceln("from: %d.%d", from.pn, from.gp);
        ui_edit_scroll_into_view(e, from);
    } else {
        from.pn = -1;
        from.gp = -1;
    }
    if (bytes != null) { *bytes = ab; }
    (void)limit; // unused in release
    ui_edit_if_sle_layout(e);
    return from;
    #pragma pop_macro("ui_edit_clip_append")
}

static void ui_edit_insert_paragraph(ui_edit_t* e, int32_t pn) {
    ui_edit_dispose_paragraphs_layout(e);
    if (e->paragraphs + 1 > e->capacity / (int32_t)sizeof(ui_edit_para_t)) {
        int32_t n = (e->paragraphs + 1) * 3 / 2; // 1.5 times
        ui_edit_reallocate((void**)&e->para, n, sizeof(ui_edit_para_t));
        e->capacity = n * (int32_t)sizeof(ui_edit_para_t);
    }
    e->paragraphs++;
    for (int32_t i = e->paragraphs - 1; i > pn; i--) {
        e->para[i] = e->para[i - 1];
    }
    ui_edit_para_t* p = &e->para[pn];
    p->text = null;
    p->bytes = 0;
    p->glyphs = -1;
    p->capacity = 0;
    p->runs = 0;
    p->run = null;
    p->g2b = null;
    p->g2b_capacity = 0;
}

// insert_inline() inserts text (not containing \n paragraph
// break inside a paragraph)

static ui_edit_pg_t ui_edit_insert_inline(ui_edit_t* e, ui_edit_pg_t pg,
        const char* text, int32_t bytes) {
    assert(bytes > 0);
    for (int32_t i = 0; i < bytes; i++) {
        assert(text[i] != '\n',
           "text \"%s\" must not contain \\n character.", text);
    }
    if (pg.pn == e->paragraphs) {
        ui_edit_insert_paragraph(e, pg.pn);
    }
    const int32_t b = e->para[pg.pn].bytes;
    ui_edit_paragraph_g2b(e, pg.pn);
    char* s = e->para[pg.pn].text;
    const int32_t bp = e->para[pg.pn].g2b[pg.gp];
    int32_t n = (b + bytes) * 3 / 2; // heuristics 1.5 times of total
    if (e->para[pg.pn].capacity == 0) {
        s = ui_edit_alloc(n);
        memcpy(s, e->para[pg.pn].text, (size_t)b);
        e->para[pg.pn].text = s;
        e->para[pg.pn].capacity = n;
    } else if (e->para[pg.pn].capacity < b + bytes) {
        ui_edit_reallocate((void**)&s, n, 1);
        e->para[pg.pn].text = s;
        e->para[pg.pn].capacity = n;
    }
    s = e->para[pg.pn].text;
    assert(b - bp >= 0);
    memmove(s + bp + bytes, s + bp, (size_t)(b - bp)); // make space
    memcpy(s + bp, text, (size_t)bytes);
    e->para[pg.pn].bytes += bytes;
    ui_edit_dispose_paragraphs_layout(e);
    pg.gp = ui_edit_glyphs(s, bp + bytes);
    ui_edit_if_sle_layout(e);
    return pg;
}

static ui_edit_pg_t ui_edit_insert_paragraph_break(ui_edit_t* e,
        ui_edit_pg_t pg) {
    ui_edit_insert_paragraph(e, pg.pn + (pg.pn < e->paragraphs));
    const int32_t bytes = e->para[pg.pn].bytes;
    char* s = e->para[pg.pn].text;
    ui_edit_paragraph_g2b(e, pg.pn);
    const int32_t bp = e->para[pg.pn].g2b[pg.gp];
    ui_edit_pg_t next = {.pn = pg.pn + 1, .gp = 0};
    if (bp < bytes) {
        (void)ui_edit_insert_inline(e, next, s + bp, bytes - bp);
    } else {
        ui_edit_dispose_paragraphs_layout(e);
    }
    e->para[pg.pn].bytes = bp;
    return next;
}

static void ui_edit_key_left(ui_edit_t* e) {
    ui_edit_pg_t to = e->selection[1];
    if (to.pn > 0 || to.gp > 0) {
        ui_point_t pt = ui_edit_pg_to_xy(e, to);
        if (pt.x == 0 && pt.y == 0) {
            ui_edit_scroll_down(e, 1);
        }
        if (to.gp > 0) {
            to.gp--;
        } else if (to.pn > 0) {
            to.pn--;
            to.gp = ui_edit_glyphs_in_paragraph(e, to.pn);
        }
        ui_edit_move_caret(e, to);
        e->last_x = -1;
    }
}

static void ui_edit_key_right(ui_edit_t* e) {
    ui_edit_pg_t to = e->selection[1];
    if (to.pn < e->paragraphs) {
        int32_t glyphs = ui_edit_glyphs_in_paragraph(e, to.pn);
        if (to.gp < glyphs) {
            to.gp++;
            ui_edit_scroll_into_view(e, to);
        } else if (!e->sle) {
            to.pn++;
            to.gp = 0;
            ui_edit_scroll_into_view(e, to);
        }
        ui_edit_move_caret(e, to);
        e->last_x = -1;
    }
}

static void ui_edit_reuse_last_x(ui_edit_t* e, ui_point_t* pt) {
    // Vertical caret movement visually tend to move caret horizontally
    // in proportional font text. Remembering starting `x' value for vertical
    // movements alleviates this unpleasant UX experience to some degree.
    if (pt->x > 0) {
        if (e->last_x > 0) {
            int32_t prev = e->last_x - e->view.fm->em.w;
            int32_t next = e->last_x + e->view.fm->em.w;
            if (prev <= pt->x && pt->x <= next) {
                pt->x = e->last_x;
            }
        }
        e->last_x = pt->x;
    }
}

static void ui_edit_key_up(ui_edit_t* e) {
    const ui_edit_pg_t pg = e->selection[1];
    ui_edit_pg_t to = pg;
    if (to.pn == e->paragraphs) {
        assert(to.gp == 0); // positioned past EOF
        to.pn--;
        to.gp = e->para[to.pn].glyphs;
        ui_edit_scroll_into_view(e, to);
        ui_point_t pt = ui_edit_pg_to_xy(e, to);
        pt.x = 0;
        to.gp = ui_edit_xy_to_pg(e, pt.x, pt.y).gp;
    } else if (to.pn > 0 || ui_edit_pg_to_pr(e, to).rn > 0) {
        // top of the text
        ui_point_t pt = ui_edit_pg_to_xy(e, to);
        if (pt.y == 0) {
            ui_edit_scroll_down(e, 1);
        } else {
            pt.y -= 1;
        }
        ui_edit_reuse_last_x(e, &pt);
        assert(pt.y >= 0);
        to = ui_edit_xy_to_pg(e, pt.x, pt.y);
        assert(to.pn >= 0 && to.gp >= 0);
        int32_t rn0 = ui_edit_pg_to_pr(e, pg).rn;
        int32_t rn1 = ui_edit_pg_to_pr(e, to).rn;
        if (rn1 > 0 && rn0 == rn1) { // same run
            assert(to.gp > 0, "word break must not break on zero gp");
            int32_t runs = 0;
            const ui_edit_run_t* run = ui_edit_paragraph_runs(e, to.pn, &runs);
            to.gp = run[rn1].gp;
        }
    }
    ui_edit_move_caret(e, to);
}

static void ui_edit_key_down(ui_edit_t* e) {
    const ui_edit_pg_t pg = e->selection[1];
    ui_point_t pt = ui_edit_pg_to_xy(e, pg);
    ui_edit_reuse_last_x(e, &pt);
    // scroll runs guaranteed to be already layout for current state of view:
    ui_edit_pg_t scroll = ui_edit_scroll_pg(e);
    int32_t run_count = ui_edit_runs_between(e, scroll, pg);
    if (!e->sle && run_count >= e->visible_runs - 1) {
        ui_edit_scroll_up(e, 1);
    } else {
        pt.y += e->view.fm->height;
    }
    ui_edit_pg_t to = ui_edit_xy_to_pg(e, pt.x, pt.y);
    if (to.pn < 0 && to.gp < 0) {
        to.pn = e->paragraphs; // advance past EOF
        to.gp = 0;
    }
    ui_edit_move_caret(e, to);
}

static void ui_edit_key_home(ui_edit_t* e) {
    if (ui_app.ctrl) {
        e->scroll.pn = 0;
        e->scroll.rn = 0;
        e->selection[1].pn = 0;
        e->selection[1].gp = 0;
    }
    const int32_t pn = e->selection[1].pn;
    int32_t runs = ui_edit_paragraph_run_count(e, pn);
    const ui_edit_para_t* para = &e->para[pn];
    if (runs <= 1) {
        e->selection[1].gp = 0;
    } else {
        int32_t rn = ui_edit_pg_to_pr(e, e->selection[1]).rn;
        assert(0 <= rn && rn < runs);
        const int32_t gp = para->run[rn].gp;
        if (e->selection[1].gp != gp) {
            // first Home keystroke moves caret to start of run
            e->selection[1].gp = gp;
        } else {
            // second Home keystroke moves caret start of paragraph
            e->selection[1].gp = 0;
            if (e->scroll.pn >= e->selection[1].pn) { // scroll in
                e->scroll.pn = e->selection[1].pn;
                e->scroll.rn = 0;
            }
        }
    }
    if (!ui_app.shift) {
        e->selection[0] = e->selection[1];
    }
    ui_edit_move_caret(e, e->selection[1]);
}

static void ui_edit_key_end(ui_edit_t* e) {
    if (ui_app.ctrl) {
        int32_t py = e->inside.bottom;
        for (int32_t i = e->paragraphs - 1; i >= 0 && py >= e->view.fm->height; i--) {
            int32_t runs = ui_edit_paragraph_run_count(e, i);
            for (int32_t j = runs - 1; j >= 0 && py >= e->view.fm->height; j--) {
                py -= e->view.fm->height;
                if (py < e->view.fm->height) {
                    e->scroll.pn = i;
                    e->scroll.rn = j;
                }
            }
        }
        e->selection[1].pn = e->paragraphs;
        e->selection[1].gp = 0;
    } else if (e->selection[1].pn == e->paragraphs) {
        assert(e->selection[1].gp == 0);
    } else {
        int32_t pn = e->selection[1].pn;
        int32_t gp = e->selection[1].gp;
        int32_t runs = 0;
        const ui_edit_run_t* run = ui_edit_paragraph_runs(e, pn, &runs);
        int32_t rn = ui_edit_pg_to_pr(e, e->selection[1]).rn;
        assert(0 <= rn && rn < runs);
        if (rn == runs - 1) {
            e->selection[1].gp = e->para[pn].glyphs;
        } else if (e->selection[1].gp == e->para[pn].glyphs) {
            // at the end of paragraph do nothing (or move caret to EOF?)
        } else if (e->para[pn].glyphs > 0 && gp != run[rn].glyphs - 1) {
            e->selection[1].gp = run[rn].gp + run[rn].glyphs - 1;
        } else {
            e->selection[1].gp = e->para[pn].glyphs;
        }
    }
    if (!ui_app.shift) {
        e->selection[0] = e->selection[1];
    }
    ui_edit_move_caret(e, e->selection[1]);
}

static void ui_edit_key_page_up(ui_edit_t* e) {
    int32_t n = ut_max(1, e->visible_runs - 1);
    ui_edit_pg_t scr = ui_edit_scroll_pg(e);
    ui_edit_pg_t bof = {.pn = 0, .gp = 0};
    int32_t m = ui_edit_runs_between(e, bof, scr);
    if (m > n) {
        ui_point_t pt = ui_edit_pg_to_xy(e, e->selection[1]);
        ui_edit_pr_t scroll = e->scroll;
        ui_edit_scroll_down(e, n);
        if (scroll.pn != e->scroll.pn || scroll.rn != e->scroll.rn) {
            ui_edit_pg_t pg = ui_edit_xy_to_pg(e, pt.x, pt.y);
            ui_edit_move_caret(e, pg);
        }
    } else {
        ui_edit_move_caret(e, bof);
    }
}

static void ui_edit_key_page_down(ui_edit_t* e) {
    int32_t n = ut_max(1, e->visible_runs - 1);
    ui_edit_pg_t scr = ui_edit_scroll_pg(e);
    ui_edit_pg_t eof = {.pn = e->paragraphs, .gp = 0};
    int32_t m = ui_edit_runs_between(e, scr, eof);
    if (m > n) {
        ui_point_t pt = ui_edit_pg_to_xy(e, e->selection[1]);
        ui_edit_pr_t scroll = e->scroll;
        ui_edit_scroll_up(e, n);
        if (scroll.pn != e->scroll.pn || scroll.rn != e->scroll.rn) {
            ui_edit_pg_t pg = ui_edit_xy_to_pg(e, pt.x, pt.y);
            ui_edit_move_caret(e, pg);
        }
    } else {
        ui_edit_move_caret(e, eof);
    }
}

static void ui_edit_key_delete(ui_edit_t* e) {
    uint64_t f = ui_edit_uint64(e->selection[0].pn, e->selection[0].gp);
    uint64_t t = ui_edit_uint64(e->selection[1].pn, e->selection[1].gp);
    uint64_t eof = ui_edit_uint64(e->paragraphs, 0);
    if (f == t && t != eof) {
        ui_edit_pg_t s1 = e->selection[1];
        e->key_right(e);
        e->selection[1] = s1;
    }
    e->erase(e);
}

static void ui_edit_key_backspace(ui_edit_t* e) {
    uint64_t f = ui_edit_uint64(e->selection[0].pn, e->selection[0].gp);
    uint64_t t = ui_edit_uint64(e->selection[1].pn, e->selection[1].gp);
    if (t != 0 && f == t) {
        ui_edit_pg_t s1 = e->selection[1];
        e->key_left(e);
        e->selection[1] = s1;
    }
    e->erase(e);
}

static void ui_edit_key_enter(ui_edit_t* e) {
    assert(!e->ro);
    if (!e->sle) {
        e->erase(e);
        e->selection[1] = ui_edit_insert_paragraph_break(e, e->selection[1]);
        e->selection[0] = e->selection[1];
        ui_edit_move_caret(e, e->selection[1]);
    } else { // single line edit callback
        if (e->enter != null) { e->enter(e); }
    }
}

static void ui_edit_key_pressed(ui_view_t* v, int64_t key) {
    assert(v->type == ui_view_text);
    ui_edit_t* e = (ui_edit_t*)v;
    if (e->focused) {
        if (key == ui.key.down && e->selection[1].pn < e->paragraphs) {
            e->key_down(e);
        } else if (key == ui.key.up && e->paragraphs > 0) {
            e->key_up(e);
        } else if (key == ui.key.left) {
            e->key_left(e);
        } else if (key == ui.key.right) {
            e->key_right(e);
        } else if (key == ui.key.pageup) {
            e->key_page_up(e);
        } else if (key == ui.key.pagedw) {
            e->key_page_down(e);
        } else if (key == ui.key.home) {
            e->key_home(e);
        } else if (key == ui.key.end) {
            e->key_end(e);
        } else if (key == ui.key.del && !e->ro) {
            e->key_delete(e);
        } else if (key == ui.key.back && !e->ro) {
            e->key_backspace(e);
        } else if (key == ui.key.enter && !e->ro) {
            e->key_enter(e);
        } else {
            // ignore other keys
        }
    }
    if (e->fuzzer != null) { e->next_fuzz(e); }
}

static void ui_edit_character(ui_view_t* unused(view), const char* utf8) {
    assert(view->type == ui_view_text);
    assert(!view->hidden && !view->disabled);
    #pragma push_macro("ui_edit_ctl")
    #define ui_edit_ctl(c) ((char)((c) - 'a' + 1))
    ui_edit_t* e = (ui_edit_t*)view;
    if (e->focused) {
        char ch = utf8[0];
        if (ui_app.ctrl) {
            if (ch == ui_edit_ctl('a')) { e->select_all(e); }
            if (ch == ui_edit_ctl('c')) { e->copy_to_clipboard(e); }
            if (!e->ro) {
                if (ch == ui_edit_ctl('x')) { e->cut_to_clipboard(e); }
                if (ch == ui_edit_ctl('v')) { e->paste_from_clipboard(e); }
            }
        }
        if (0x20 <= ch && !e->ro) { // 0x20 space
            int32_t bytes = ui_edit_glyph_bytes(ch);
            e->erase(e); // remove selected text to be replaced by glyph
            e->selection[1] = ui_edit_insert_inline(e, e->selection[1], utf8, bytes);
            e->selection[0] = e->selection[1];
            ui_edit_move_caret(e, e->selection[1]);
        }
        ui_edit_invalidate(e);
        if (e->fuzzer != null) { e->next_fuzz(e); }
    }
    #pragma pop_macro("ui_edit_ctl")
}

static void ui_edit_select_word(ui_edit_t* e, int32_t x, int32_t y) {
    ui_edit_pg_t p = ui_edit_xy_to_pg(e, x, y);
    if (0 <= p.pn && 0 <= p.gp) {
        if (p.pn > e->paragraphs) { p.pn = ut_max(0, e->paragraphs); }
        int32_t glyphs = ui_edit_glyphs_in_paragraph(e, p.pn);
        if (p.gp > glyphs) { p.gp = ut_max(0, glyphs); }
        if (p.pn == e->paragraphs || glyphs == 0) {
            // last paragraph is empty - nothing to select on fp64_t click
        } else {
            ui_edit_glyph_t glyph = ui_edit_glyph_at(e, p);
            bool not_a_white_space = glyph.bytes > 0 &&
                *(const uint8_t*)glyph.s > 0x20;
            if (!not_a_white_space && p.gp > 0) {
                p.gp--;
                glyph = ui_edit_glyph_at(e, p);
                not_a_white_space = glyph.bytes > 0 &&
                    *(const uint8_t*)glyph.s > 0x20;
            }
            if (glyph.bytes > 0 && *(const uint8_t*)glyph.s > 0x20) {
                ui_edit_pg_t from = p;
                while (from.gp > 0) {
                    from.gp--;
                    ui_edit_glyph_t g = ui_edit_glyph_at(e, from);
                    if (g.bytes == 0 || *(const uint8_t*)g.s <= 0x20) {
                        from.gp++;
                        break;
                    }
                }
                e->selection[0] = from;
                ui_edit_pg_t to = p;
                while (to.gp < glyphs) {
                    to.gp++;
                    ui_edit_glyph_t g = ui_edit_glyph_at(e, to);
                    if (g.bytes == 0 || *(const uint8_t*)g.s <= 0x20) {
                        break;
                    }
                }
                e->selection[1] = to;
                ui_edit_invalidate(e);
                e->mouse = 0;
            }
        }
    }
}

static void ui_edit_select_paragraph(ui_edit_t* e, int32_t x, int32_t y) {
    ui_edit_pg_t p = ui_edit_xy_to_pg(e, x, y);
    if (0 <= p.pn && 0 <= p.gp) {
        if (p.pn > e->paragraphs) { p.pn = ut_max(0, e->paragraphs); }
        int32_t glyphs = ui_edit_glyphs_in_paragraph(e, p.pn);
        if (p.gp > glyphs) { p.gp = ut_max(0, glyphs); }
        if (p.pn == e->paragraphs || glyphs == 0) {
            // last paragraph is empty - nothing to select on fp64_t click
        } else if (p.pn == e->selection[0].pn &&
                ((e->selection[0].gp <= p.gp && p.gp <= e->selection[1].gp) ||
                 (e->selection[1].gp <= p.gp && p.gp <= e->selection[0].gp))) {
            e->selection[0].gp = 0;
            e->selection[1].gp = 0;
            e->selection[1].pn++;
        }
        ui_edit_invalidate(e);
        e->mouse = 0;
    }
}

static void ui_edit_double_click(ui_edit_t* e, int32_t x, int32_t y) {
    if (e->paragraphs == 0) {
        // do nothing
    } else if (e->selection[0].pn == e->selection[1].pn &&
        e->selection[0].gp == e->selection[1].gp) {
        ui_edit_select_word(e, x, y);
    } else {
        if (e->selection[0].pn == e->selection[1].pn &&
               e->selection[0].pn <= e->paragraphs) {
            ui_edit_select_paragraph(e, x, y);
        }
    }
}

static void ui_edit_click(ui_edit_t* e, int32_t x, int32_t y) {
    ui_edit_pg_t p = ui_edit_xy_to_pg(e, x, y);
    if (0 <= p.pn && 0 <= p.gp) {
        if (p.pn > e->paragraphs) { p.pn = ut_max(0, e->paragraphs); }
        int32_t glyphs = e->paragraphs == 0 ? 0 : ui_edit_glyphs_in_paragraph(e, p.pn);
        if (p.gp > glyphs) { p.gp = ut_max(0, glyphs); }
        ui_edit_move_caret(e, p);
    }
}

static void ui_edit_focus_on_click(ui_edit_t* e, int32_t x, int32_t y) {
    // was edit control focused before click arrives?
    const bool app_has_focus = ui_app.has_focus();
    bool focused = false;
    if (e->mouse != 0) {
        if (app_has_focus && !e->focused) {
            if (ui_app.focus != null && ui_app.focus->kill_focus != null) {
                ui_app.focus->kill_focus(ui_app.focus);
            }
            ui_app.focus = &e->view;
            bool set = e->view.set_focus(&e->view);
            fatal_if(!set);
            focused = true;
        }
        bool empty = memcmp(&e->selection[0], &e->selection[1],
                                sizeof(e->selection[0])) == 0;
        if (focused && !empty) {
            // first click on unfocused edit should set focus but
            // not set caret, because setting caret on click will
            // destroy selection and this is bad UX
        } else if (app_has_focus && e->focused) {
            e->mouse = 0;
            ui_edit_click(e, x, y);
        }
    }
}

static void ui_edit_mouse_button_down(ui_edit_t* e, int32_t m,
        int32_t x, int32_t y) {
    if (m == ui.message.left_button_pressed)  { e->mouse |= (1 << 0); }
    if (m == ui.message.right_button_pressed) { e->mouse |= (1 << 1); }
    ui_edit_focus_on_click(e, x, y);
}

static void ui_edit_mouse_button_up(ui_edit_t* e, int32_t m) {
    if (m == ui.message.left_button_released)  { e->mouse &= ~(1 << 0); }
    if (m == ui.message.right_button_released) { e->mouse &= ~(1 << 1); }
}

#ifdef EDIT_USE_TAP

static bool ui_edit_tap(ui_view_t* v, int32_t ix) {
    traceln("ix: %d", ix);
    if (ix == 0) {
        ui_edit_t* e = (ui_edit_t*)v;
        const int32_t x = ui_app.mouse.x - e->view.x;
        const int32_t y = ui_app.mouse.y - e->view.y - e->inside.top;
        bool inside = 0 <= x && x < v->w && 0 <= y && y < v->h;
        if (inside) {
            e->mouse = 0x1;
            ui_edit_focus_on_click(e, x, y);
            e->mouse = 0x0;
        }
        return inside;
    } else {
        return false; // do NOT consume event
    }
}

#endif // EDIT_USE_TAP

static bool ui_edit_press(ui_view_t* v, int32_t ix) {
//  traceln("ix: %d", ix);
    if (ix == 0) {
        ui_edit_t* e = (ui_edit_t*)v;
        const int32_t x = ui_app.mouse.x - e->view.x;
        const int32_t y = ui_app.mouse.y - e->view.y - e->inside.top;
        bool inside = 0 <= x && x < v->w && 0 <= y && y < v->h;
        if (inside) {
            e->mouse = 0x1;
            ui_edit_focus_on_click(e, x, y);
            ui_edit_double_click(e, x, y);
            e->mouse = 0x0;
        }
        return inside;
    } else {
        return false; // do NOT consume event
    }
}

static void ui_edit_mouse(ui_view_t* v, int32_t m, int64_t unused(flags)) {
//  if (m == ui.message.left_button_pressed) { traceln("%p", view); }
    assert(v->type == ui_view_text);
    assert(!v->hidden);
    assert(!v->disabled);
    ui_edit_t* e = (ui_edit_t*)v;
    const int32_t x = ui_app.mouse.x - e->view.x - e->inside.left;
    const int32_t y = ui_app.mouse.y - e->view.y - e->inside.top;
    bool inside = 0 <= x && x < v->w && 0 <= y && y < v->h;
    if (inside) {
        if (m == ui.message.left_button_pressed ||
            m == ui.message.right_button_pressed) {
            ui_edit_mouse_button_down(e, m, x, y);
        } else if (m == ui.message.left_button_released ||
                   m == ui.message.right_button_released) {
            ui_edit_mouse_button_up(e, m);
        } else if (m == ui.message.left_double_click ||
                   m == ui.message.right_double_click) {
            ui_edit_double_click(e, x, y);
        }
    }
}

static void ui_edit_mouse_wheel(ui_view_t* v, int32_t unused(dx), int32_t dy) {
    // TODO: maybe make a use of dx in single line no-word-break edit control?
    if (ui_app.focus == v) {
        assert(v->type == ui_view_text);
        ui_edit_t* e = (ui_edit_t*)v;
        int32_t lines = (abs(dy) + v->fm->height - 1) / v->fm->height;
        if (dy > 0) {
            ui_edit_scroll_down(e, lines);
        } else if (dy < 0) {
            ui_edit_scroll_up(e, lines);
        }
//  TODO: Ctrl UP/DW and caret of out of visible area scrolls are not
//        implemented. Not sure they are very good UX experience.
//        MacOS users may be used to scroll with touchpad, take a visual
//        peek, do NOT click and continue editing at last cursor position.
//        To me back forward stack navigation is much more intuitive and
//        much mode "modeless" in spirit of cut/copy/paste. But opinions
//        and editing habits vary. Easy to implement.
        ui_edit_pg_t pg = ui_edit_xy_to_pg(e, e->caret.x, e->caret.y);
        ui_edit_move_caret(e, pg);
    }
}

static bool ui_edit_set_focus(ui_view_t* v) {
    assert(v->type == ui_view_text);
    ui_edit_t* e = (ui_edit_t*)v;
//  traceln("active=%d has_focus=%d focused=%d",
//           ui_app.is_active(), ui_app.has_focus(), e->focused);
    assert(ui_app.focus == v || ui_app.focus == null);
    assert(v->focusable);
    ui_app.focus = v;
    if (ui_app.has_focus() && !e->focused) {
        ui_edit_create_caret(e);
        ui_edit_show_caret(e);
        ui_edit_if_sle_layout(e);
    }
    return true;
}

static void ui_edit_kill_focus(ui_view_t* v) {
    assert(v->type == ui_view_text);
    ui_edit_t* e = (ui_edit_t*)v;
//  traceln("active=%d has_focus=%d focused=%d",
//           ui_app.is_active(), ui_app.has_focus(), e->focused);
    if (e->focused) {
        ui_edit_hide_caret(e);
        ui_edit_destroy_caret(e);
        ui_edit_if_sle_layout(e);
    }
    if (ui_app.focus == v) { ui_app.focus = null; }
}

static void ui_edit_erase(ui_edit_t* e) {
    const ui_edit_pg_t from = e->selection[0];
    const ui_edit_pg_t to = e->selection[1];
    ui_edit_pg_t pg = ui_edit_op(e, true, from, to, null, null);
    if (pg.pn >= 0 && pg.gp >= 0) {
        e->selection[0] = pg;
        e->selection[1] = pg;
        ui_edit_move_caret(e, pg);
        ui_edit_invalidate(e);
    }
}

static void ui_edit_cut_copy(ui_edit_t* e, bool cut) {
    const ui_edit_pg_t from = e->selection[0];
    const ui_edit_pg_t to = e->selection[1];
    int32_t n = 0; // bytes between from..to
    ui_edit_op(e, false, from, to, null, &n);
    if (n > 0) {
        char* text = ui_edit_alloc(n + 1);
        ui_edit_pg_t pg = ui_edit_op(e, cut, from, to, text, &n);
        static ui_label_t hint = ui_label(0.0f, "copied to clipboard");
        int32_t x = e->view.x + e->view.w / 2;
        int32_t y = e->view.y + e->view.h / 2;
        ui_app.show_hint(&hint, x, y, 0.5);
        if (cut && pg.pn >= 0 && pg.gp >= 0) {
            e->selection[0] = pg;
            e->selection[1] = pg;
            ui_edit_move_caret(e, pg);
        }
        text[n] = 0; // make it zero terminated
        ut_clipboard.put_text(text);
        assert(n == (int32_t)strlen(text), "n=%d strlen(cb)=%d cb=\"%s\"",
               n, strlen(text), text);
        ui_edit_free((void**)&text);
    }
}

static void ui_edit_select_all(ui_edit_t* e) {
    e->selection[0] = (ui_edit_pg_t ){.pn = 0, .gp = 0};
    e->selection[1] = (ui_edit_pg_t ){.pn = e->paragraphs, .gp = 0};
    ui_edit_invalidate(e);
}

static int32_t ui_edit_copy(ui_edit_t* e, char* text, int32_t* bytes) {
    not_null(bytes);
    int32_t r = 0;
    const ui_edit_pg_t from = {.pn = 0, .gp = 0};
    const ui_edit_pg_t to = {.pn = e->paragraphs, .gp = 0};
    int32_t n = 0; // bytes between from..to
    ui_edit_op(e, false, from, to, null, &n);
    if (text != null) {
        int32_t m = ut_min(n, *bytes);
        enum { error_insufficient_buffer = 122 }; //  ERROR_INSUFFICIENT_BUFFER
        if (m < n) { r = error_insufficient_buffer; }
        ui_edit_op(e, false, from, to, text, &m);
    }
    *bytes = n;
    return r;
}

static void ui_edit_clipboard_cut(ui_edit_t* e) {
    if (!e->ro) { ui_edit_cut_copy(e, true); }
}

static void ui_edit_clipboard_copy(ui_edit_t* e) {
    ui_edit_cut_copy(e, false);
}

static ui_edit_pg_t ui_edit_paste_text(ui_edit_t* e,
        const char* s, int32_t n) {
    assert(!e->ro);
    ui_edit_pg_t pg = e->selection[1];
    int32_t i = 0;
    const char* text = s;
    while (i < n) {
        int32_t b = i;
        while (b < n && s[b] != '\n') { b++; }
        bool lf = b < n && s[b] == '\n';
        int32_t next = b + 1;
        if (b > i && s[b - 1] == '\r') { b--; } // CR LF
        if (b > i) {
            pg = ui_edit_insert_inline(e, pg, text, b - i);
        }
        if (lf && e->sle) {
            break;
        } else if (lf) {
            pg = ui_edit_insert_paragraph_break(e, pg);
        }
        text = s + next;
        i = next;
    }
    return pg;
}

static void ui_edit_paste(ui_edit_t* e, const char* s, int32_t n) {
    if (!e->ro) {
        if (n < 0) { n = (int32_t)strlen(s); }
        e->erase(e);
        e->selection[1] = ui_edit_paste_text(e, s, n);
        e->selection[0] = e->selection[1];
        if (e->view.w > 0) { ui_edit_move_caret(e, e->selection[1]); }
    }
}

static void ui_edit_clipboard_paste(ui_edit_t* e) {
    if (!e->ro) {
        ui_edit_pg_t pg = e->selection[1];
        int32_t bytes = 0;
        ut_clipboard.get_text(null, &bytes);
        if (bytes > 0) {
            char* text = ui_edit_alloc(bytes);
            int32_t r = ut_clipboard.get_text(text, &bytes);
            fatal_if_not_zero(r);
            if (bytes > 0 && text[bytes - 1] == 0) {
                bytes--; // clipboard includes zero terminator
            }
            if (bytes > 0) {
                e->erase(e);
                pg = ui_edit_paste_text(e, text, bytes);
                ui_edit_move_caret(e, pg);
            }
            ui_edit_free((void**)&text);
        }
    }
}

static void ui_edit_prepare_sle(ui_edit_t* e) {
    ui_view_t* v = &e->view;
    swear(e->sle && v->w > 0);
    // shingle line edit is capable of resizing itself to two
    // lines of text (and shrinking back) to avoid horizontal scroll
    int32_t runs = ut_max(1, ut_min(2, ui_edit_paragraph_run_count(e, 0)));
    const ui_ltrb_t insets = ui_view.gaps(v, &v->insets);
    int32_t h = insets.top + v->fm->height * runs + insets.bottom;
    fp32_t min_h_em = (fp32_t)h / v->fm->em.h;
    if (v->min_h_em != min_h_em) {
        v->min_h_em = min_h_em;
//      traceln("%p %-10s %dx%d runs: %d h: %d min_h_em := %.1f", v, v->p.text, v->w, v->h, runs, h, min_h_em);
    }
//  traceln("%p %-10s %dx%d runs: %d h: %d min_h_em: %.1f", v, v->p.text, v->w, v->h, runs, h, min_h_em);
}

static void ui_edit_insets(ui_edit_t* e) {
    ui_view_t* v = &e->view;
    const ui_ltrb_t insets = ui_view.gaps(v, &v->insets);
    e->inside = (ui_ltrb_t){
        .left   = insets.left,
        .top    = insets.top,
        .right  = v->w - insets.right,
        .bottom = v->h - insets.bottom
    };
    e->w = e->inside.right  - e->inside.left;
    e->h = e->inside.bottom - e->inside.top;
}

static void ui_edit_measure(ui_view_t* v) { // bottom up
    assert(v->type == ui_view_text);
    ui_edit_t* e = (ui_edit_t*)v;
    if (v->w > 0 && e->sle) { ui_edit_prepare_sle(e); }
    v->w = (int32_t)((fp64_t)v->fm->em.w * (fp64_t)v->min_w_em + 0.5);
    v->h = (int32_t)((fp64_t)v->fm->em.h * (fp64_t)v->min_h_em + 0.5);
    const ui_ltrb_t i = ui_view.gaps(v, &v->insets);
    // enforce minimum size - it makes it checking corner cases much simpler
    // and it's hard to edit anything in a smaller area - will result in bad UX
    if (v->w < v->fm->em.w * 4) { v->w = i.left + v->fm->em.w * 4 + i.right; }
    if (v->h < v->fm->height)   { v->h = i.top + v->fm->height + i.bottom; }
    ui_edit_insets(e);
//  if (e->sle) {
//      traceln("%p %10s %dx%d min_h_em: %.1f", v, v->p.text, v->w, v->h, v->min_h_em);
//  }
}

static void ui_edit_layout(ui_view_t* v) { // top down
//  traceln(">%d,%d %dx%d", v->y, v->y, v->w, v->h);
    assert(v->type == ui_view_text);
    assert(v->w > 0 && v->h > 0); // could be `if'
    ui_edit_t* e = (ui_edit_t*)v;
    // glyph position in scroll_pn paragraph:
    const ui_edit_pg_t scroll = v->w == 0 ?
        (ui_edit_pg_t){0, 0} : ui_edit_scroll_pg(e);
    // always dispose paragraphs layout:
    ui_edit_dispose_paragraphs_layout(e);
    ui_edit_insets(e);
    e->visible_runs = (e->inside.bottom - e->inside.top) / e->view.fm->height; // fully visible
    e->w = e->inside.right  - e->inside.left;
    e->h = e->inside.bottom - e->inside.top;
    // number of runs in e->scroll.pn may have changed with e->w change
    int32_t runs = ui_edit_paragraph_run_count(e, e->scroll.pn);
    if (e->paragraphs == 0) {
        e->selection[0] = (ui_edit_pg_t){0, 0};
        e->selection[1] = e->selection[0];
    } else {
        e->scroll.rn = ui_edit_pg_to_pr(e, scroll).rn;
        assert(0 <= e->scroll.rn && e->scroll.rn < runs); (void)runs;
        if (e->sle) { // single line edit (if changed on the fly):
            e->selection[0].pn = 0; // only has single paragraph
            e->selection[1].pn = 0;
            // scroll line on top of current cursor position into view
//          int32_t rn = ui_edit_pg_to_pr(e, e->selection[1]).rn;
//          traceln("scroll.rn: %d rn: %d", e->scroll.rn, rn);
            const ui_edit_run_t* run = ui_edit_paragraph_runs(e, 0, &runs);
            if (runs <= 2 && e->scroll.rn == 1) {
                ui_edit_pg_t top = scroll;
//              traceln("run[scroll.rn: %d].glyphs: %d", e->scroll.rn, run[e->scroll.rn].glyphs);
                top.gp = ut_max(0, top.gp - run[e->scroll.rn].glyphs - 1);
                ui_edit_scroll_into_view(e, top);
            }
        }
    }
    if (e->focused) {
        // recreate caret because fm->height may have changed
        ui_edit_hide_caret(e);
        ui_edit_destroy_caret(e);
        ui_edit_create_caret(e);
        ui_edit_show_caret(e);
    }
//  traceln("<%d,%d %dx%d", v->y, v->y, v->w, v->h);
}

static void ui_edit_paint(ui_view_t* v) {
    assert(v->type == ui_view_text);
    assert(!v->hidden);
    ui_edit_t* e = (ui_edit_t*)v;
    ui_gdi.fill(v->x, v->y, v->w, v->h, v->background);
    ui_gdi.set_clip(v->x + e->inside.left,  v->y + e->inside.top,
                    e->w + e->inside.right, e->h);
    const ui_ltrb_t insets = ui_view.gaps(v, &v->insets);
    int32_t x = v->x + insets.left;
    int32_t y = v->y + insets.top;
    const ui_gdi_ta_t ta = { .fm = v->fm, .color = v->color };
    const int32_t pn = e->scroll.pn;
    const int32_t bottom = v->y + e->inside.bottom;
    assert(pn <= e->paragraphs);
    for (int32_t i = pn; i < e->paragraphs && y < bottom; i++) {
        y = ui_edit_paint_paragraph(e, &ta, x, y, i);
    }
    ui_gdi.set_clip(0, 0, 0, 0);
}

static void ui_edit_move(ui_edit_t* e, ui_edit_pg_t pg) {
    if (e->view.w > 0) {
        ui_edit_move_caret(e, pg); // may select text on move
    } else {
        e->selection[1] = pg;
    }
    e->selection[0] = e->selection[1];
}

static bool ui_edit_message(ui_view_t* v, int32_t unused(m), int64_t unused(wp),
        int64_t unused(lp), int64_t* unused(rt)) {
    ui_edit_t* e = (ui_edit_t*)v;
    if (ui_app.is_active() && ui_app.has_focus() && !v->hidden) {
        if (e->focused != (ui_app.focus == v)) {
//          traceln("message: 0x%04X e->focused != (ui_app.focus == v)", m);
            if (e->focused) {
                v->kill_focus(v);
            } else {
                v->set_focus(v);
            }
        }
    } else {
        // do nothing: when app will become active and focused
        //             it will react on app->focus changes
    }
    return false;
}

void ui_edit_init(ui_edit_t* e) {
    memset(e, 0, sizeof(*e));
    e->view.color_id = ui_color_id_window_text;
    e->view.background_id = ui_color_id_window;
    e->view.fm = &ui_app.fonts.regular;
    e->view.insets  = (ui_gaps_t){ 0.25, 0.25, 0.50, 0.25 };
    e->view.padding = (ui_gaps_t){ 0.25, 0.25, 0.25, 0.25 };
    e->view.min_w_em = 1.0;
    e->view.min_h_em = 1.0;
    e->view.type = ui_view_text;
    e->view.focusable = true;
    e->fuzz_seed = 1; // client can seed it with (ut_clock.nanoseconds() | 1)
    e->last_x    = -1;
    e->focused   = false;
    e->sle       = false;
    e->ro        = false;
    e->caret                = (ui_point_t){-1, -1};
    e->view.message         = ui_edit_message;
    e->view.paint           = ui_edit_paint;
    e->view.measure         = ui_edit_measure;
    e->view.layout          = ui_edit_layout;
    #ifdef EDIT_USE_TAP
    e->view.tap             = ui_edit_tap;
    #else
    e->view.mouse           = ui_edit_mouse;
    #endif
    e->view.press           = ui_edit_press;
    e->view.character       = ui_edit_character;
    e->view.set_focus       = ui_edit_set_focus;
    e->view.kill_focus      = ui_edit_kill_focus;
    e->view.key_pressed     = ui_edit_key_pressed;
    e->view.mouse_wheel     = ui_edit_mouse_wheel;
    e->set_font             = ui_edit_set_font;
    e->move                 = ui_edit_move;
    e->paste                = ui_edit_paste;
    e->copy                 = ui_edit_copy;
    e->erase                = ui_edit_erase;
    e->cut_to_clipboard     = ui_edit_clipboard_cut;
    e->copy_to_clipboard    = ui_edit_clipboard_copy;
    e->paste_from_clipboard = ui_edit_clipboard_paste;
    e->select_all           = ui_edit_select_all;
    e->key_down             = ui_edit_key_down;
    e->key_up               = ui_edit_key_up;
    e->key_left             = ui_edit_key_left;
    e->key_right            = ui_edit_key_right;
    e->key_page_up          = ui_edit_key_page_up;
    e->key_page_down        = ui_edit_key_page_down;
    e->key_home             = ui_edit_key_home;
    e->key_end              = ui_edit_key_end;
    e->key_delete           = ui_edit_key_delete;
    e->key_backspace        = ui_edit_key_backspace;
    e->key_enter            = ui_edit_key_enter;
    e->fuzz                 = null;
}
