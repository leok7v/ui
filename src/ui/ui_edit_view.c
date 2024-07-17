/* Copyright (c) Dmitry "Leo" Kuznetsov 2021-24 see LICENSE for details */
#include "ut/ut.h"
#include "ui/ui.h"
#include "ui/ui_edit_doc.h"

// TODO: find all "== dt->np" it is wrong pn < dt->np fix them all
// TODO: undo/redo coalescing
// TODO: back/forward navigation
// TODO: exit (Ctrl+W?)/save(Ctrl+S, Ctrl+Shift+S) keyboard shortcuts?
// TODO: ctrl left, ctrl right jump word ctrl+shift left/right select word?
// TODO: iBeam cursor (definitely yes - see how MSVC does it)
// TODO: vertical scrollbar ui
// TODO: horizontal scroll: trivial to implement:
//       add horizontal_scroll to e->w and paint
//       paragraphs in a horizontally shifted clip

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
static ui_point_t ui_edit_pg_to_xy(ui_edit_t* e, const ui_edit_pg_t pg);

// Glyphs in monospaced Windows fonts may have different width for non-ASCII
// characters. Thus even if edit is monospaced glyph measurements are used
// in text layout.

static void ui_edit_invalidate_view(const ui_edit_t* e) {
    ui_view.invalidate(&e->view, null);
}

static void ui_edit_invalidate_rect(const ui_edit_t* e, const ui_rect_t rc) {
    assert(rc.w > 0 && rc.h > 0);
    ui_view.invalidate(&e->view, &rc);
}

static ui_rect_t ui_edit_selection_rect(ui_edit_t* e) {
    const ui_edit_range_t r = ui_edit_range.order(e->selection);
    const ui_ltrb_t i = ui_view.margins(&e->view, &e->insets);
    const ui_point_t p0 = ui_edit_pg_to_xy(e, r.from);
    const ui_point_t p1 = ui_edit_pg_to_xy(e, r.to);
    if (p0.x < 0 || p1.x < 0) { // selection outside of visible area
        return (ui_rect_t) { .x = 0, .y = 0, .w = e->w, .h = e->h };
    } else if (p0.y == p1.y) {
        const int32_t max_w = ut_max(e->fm->max_char_width, e->fm->em.w);
        int32_t w = p1.x - p0.x != 0 ?
                p1.x - p0.x + max_w : e->caret_width;
        return (ui_rect_t) { .x = p0.x, .y = i.top + p0.y,
                             .w = w, .h = e->fm->height };
    } else {
        const int32_t h = p1.y - p0.y + e->fm->height;
        return (ui_rect_t) { .x = 0, .y = i.top + p0.y,
                             .w = e->w, .h = h };
    }
}

static int32_t ui_edit_text_width(ui_edit_t* e, const char* s, int32_t n) {
//  fp64_t time = ut_clock.seconds();
    // average GDI measure_text() performance per character:
    // "ui_app.fm.mono"    ~500us (microseconds)
    // "ui_app.fm.regular" ~250us (microseconds) DirectWrite ~100us
    const ui_gdi_ta_t ta = { .fm = e->fm, .color = e->color,
                             .measure = true };
    int32_t x = n == 0 ? 0 : ui_gdi.text(&ta, 0, 0, "%.*s", n, s).w;
//  TODO: remove
//  int32_t x = n == 0 ? 0 : ui_gdi.measure_text(e->fm, "%.*s", n, s).w;

//  time = (ut_clock.seconds() - time) * 1000.0;
//  static fp64_t time_sum;
//  static fp64_t length_sum;
//  time_sum += time;
//  length_sum += n;
//  ut_traceln("avg=%.6fms per char total %.3fms", time_sum / length_sum, time_sum);
    return x;
}

static int32_t ui_edit_word_break_at(ui_edit_t* e, int32_t pn, int32_t rn,
        const int32_t width, bool allow_zero) {
    // TODO: in sqlite.c 257,674 lines it takes 11 seconds to get all runs()
    //       on average ui_edit_word_break_at() takes 4 x ui_edit_text_width()
    //       measurements and they are slow. If we can reduce this amount
    //       (not clear how) at least 2 times it will be a win.
    //       Another way is background thread runs() processing but this is
    //       involving a lot of complexity.
    //       MSVC devenv.exe edits sqlite3.c w/o any visible delays
    int32_t count = 0; // stats logging
    int32_t chars = 0;
    ui_edit_text_t* dt = &e->doc->text; // document text
    assert(0 <= pn && pn < dt->np);
    ui_edit_paragraph_t* p = &e->para[pn];
    const ui_edit_str_t* str = &dt->ps[pn];
    int32_t k = 1; // at least 1 glyph
    // offsets inside a run in glyphs and bytes from start of the paragraph:
    int32_t gp = p->run[rn].gp;
    int32_t bp = p->run[rn].bp;
    if (gp < str->g - 1) {
        const char* text = str->u + bp;
        const int32_t glyphs_in_this_run = str->g - gp;
        int32_t* g2b = &str->g2b[gp];
        // 4 is maximum number of bytes in a UTF-8 sequence
        int32_t gc = ut_min(4, glyphs_in_this_run);
        int32_t w = ui_edit_text_width(e, text, g2b[gc] - bp);
        count++;
        chars += g2b[gc] - bp;
        while (gc < glyphs_in_this_run && w < width) {
            gc = ut_min(gc * 4, glyphs_in_this_run);
            w = ui_edit_text_width(e, text, g2b[gc] - bp);
            count++;
            chars += g2b[gc] - bp;
        }
        if (w < width) {
            k = gc;
            assert(1 <= k && k <= str->g - gp);
        } else {
            int32_t i = 0;
            int32_t j = gc;
            k = (i + j) / 2;
            while (i < j) {
                assert(allow_zero || 1 <= k && k < gc + 1);
                const int32_t n = g2b[k + 1] - bp;
                int32_t px = ui_edit_text_width(e, text, n);
                count++;
                chars += n;
                if (px == width) { break; }
                if (px < width) { i = k + 1; } else { j = k; }
                if (!allow_zero && (i + j) / 2 == 0) { break; }
                k = (i + j) / 2;
                assert(allow_zero || 1 <= k && k <= str->g - gp);
            }
        }
    }
    assert(allow_zero || 1 <= k && k <= str->g - gp);
    return k;
}

static int32_t ui_edit_word_break(ui_edit_t* e, int32_t pn, int32_t rn) {
    return ui_edit_word_break_at(e, pn, rn, e->edit.w, false);
}

static int32_t ui_edit_glyph_at_x(ui_edit_t* e, int32_t pn, int32_t rn,
        int32_t x) {
    ui_edit_text_t* dt = &e->doc->text; // document text
    assert(0 <= pn && pn < dt->np);
    if (x == 0 || dt->ps[pn].b == 0) {
        return 0;
    } else {
        return ui_edit_word_break_at(e, pn, rn, x + 1, true);
    }
}

static ui_edit_glyph_t ui_edit_glyph_at(ui_edit_t* e, ui_edit_pg_t p) {
    ui_edit_text_t* dt = &e->doc->text; // document text
    ui_edit_glyph_t g = { .s = "", .bytes = 0 };
    assert(0 <= p.pn && p.pn < dt->np);
    const ui_edit_str_t* str = &dt->ps[p.pn];
    const int32_t bytes = str->b;
    const char* s = str->u;
    const int32_t bp = str->g2b[p.gp];
    if (bp < bytes) {
        g.s = s + bp;
        g.bytes = ut_str.utf8bytes(g.s, bytes - bp);
        swear(g.bytes > 0);
    }
    return g;
}

// paragraph_runs() breaks paragraph into `runs` according to `width`

static const ui_edit_run_t* ui_edit_paragraph_runs(ui_edit_t* e, int32_t pn,
        int32_t* runs) {
//  fp64_t time = ut_clock.seconds();
    assert(e->w > 0);
    ui_edit_text_t* dt = &e->doc->text; // document text
    assert(0 <= pn && pn < dt->np);
    const ui_edit_run_t* r = null;
    if (e->para[pn].run != null) {
        *runs = e->para[pn].runs;
        r = e->para[pn].run;
    } else {
        assert(0 <= pn && pn < dt->np);
        ui_edit_paragraph_t* p = &e->para[pn];
        const ui_edit_str_t* str = &dt->ps[pn];
        if (p->run == null) {
            assert(p->runs == 0 && p->run == null);
            const int32_t max_runs = str->b + 1;
            bool ok = ut_heap.alloc((void**)&p->run, max_runs *
                                    sizeof(ui_edit_run_t)) == 0;
            swear(ok);
            ui_edit_run_t* run = p->run;
            run[0].bp = 0;
            run[0].gp = 0;
            int32_t gc = str->b == 0 ? 0 : ui_edit_word_break(e, pn, 0);
            if (gc == str->g) { // whole paragraph fits into width
                p->runs = 1;
                run[0].bytes  = str->b;
                run[0].glyphs = str->g;
                int32_t pixels = ui_edit_text_width(e, str->u, str->g2b[gc]);
                run[0].pixels = pixels;
            } else {
                assert(gc < str->g);
                int32_t rc = 0; // runs count
                int32_t ix = 0; // glyph index from to start of paragraph
                const char* text = str->u;
                int32_t bytes = str->b;
                while (bytes > 0) {
                    assert(rc < max_runs);
                    run[rc].bp = (int32_t)(text - str->u);
                    run[rc].gp = ix;
                    int32_t glyphs = ui_edit_word_break(e, pn, rc);
                    int32_t utf8bytes = str->g2b[ix + glyphs] - run[rc].bp;
                    int32_t pixels = ui_edit_text_width(e, text, utf8bytes);
                    if (glyphs > 1 && utf8bytes < bytes && text[utf8bytes - 1] != 0x20) {
                        // try to find word break SPACE character. utf8 space is 0x20
                        int32_t i = utf8bytes;
                        while (i > 0 && text[i - 1] != 0x20) { i--; }
                        if (i > 0 && i != utf8bytes) {
                            utf8bytes = i;
                            glyphs = ut_str.glyphs(text, utf8bytes);
                            assert(glyphs >= 0);
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
                ok = ut_heap.realloc((void**)&p->run, rc * sizeof(ui_edit_run_t)) == 0;
                swear(ok);
            }
        }
        *runs = p->runs;
        r = p->run;
    }
    assert(r != null && *runs >= 1);
    return r;
}

static int32_t ui_edit_paragraph_run_count(ui_edit_t* e, int32_t pn) {
    swear(e->w > 0);
    ui_edit_text_t* dt = &e->doc->text; // document text
    int32_t runs = 0;
    if (e->w > 0 && 0 <= pn && pn < dt->np) {
        (void)ui_edit_paragraph_runs(e, pn, &runs);
    }
    return runs;
}

static int32_t ui_edit_glyphs_in_paragraph(ui_edit_t* e, int32_t pn) {
    ui_edit_text_t* dt = &e->doc->text; // document text
    assert(0 <= pn && pn < dt->np);
    (void)ui_edit_paragraph_run_count(e, pn); // word break into runs
    return dt->ps[pn].g;
}

static void ui_edit_create_caret(ui_edit_t* e) {
    ut_fatal_if(e->focused);
    assert(ui_app.is_active());
    assert(ui_app.focused());
    fp64_t px = ui_app.dpi.monitor_raw / 100.0 + 0.5;
    e->caret_width = ut_min(3, ut_max(1, (int32_t)px));
    ui_app.create_caret(e->caret_width, e->fm->height);
    e->focused = true; // means caret was created
//  ut_traceln("e->focused := true %s", ui_view_debug_id(&e->view));
}

static void ui_edit_destroy_caret(ui_edit_t* e) {
    ut_fatal_if(!e->focused);
    ui_app.destroy_caret();
    e->focused = false; // means caret was destroyed
//  ut_traceln("e->focused := false %s", ui_view_debug_id(&e->view));
}

static void ui_edit_show_caret(ui_edit_t* e) {
    if (e->focused) {
        assert(ui_app.is_active());
        assert(ui_app.focused());
        assert((e->caret.x < 0) == (e->caret.y < 0));
        const ui_ltrb_t insets = ui_view.margins(&e->view, &e->insets);
        int32_t x = e->caret.x < 0 ? insets.left : e->caret.x;
        int32_t y = e->caret.y < 0 ? insets.top  : e->caret.y;
        ui_app.move_caret(e->x + x, e->y + y);
        // TODO: it is possible to support unblinking caret if desired
        // do not set blink time - use global default
//      fatal_if_false(SetCaretBlinkTime(500));
        ui_app.show_caret();
        e->shown++;
        assert(e->shown == 1);
    }
}

static void ui_edit_hide_caret(ui_edit_t* e) {
    if (e->focused) {
        ui_app.hide_caret();
        e->shown--;
        assert(e->shown == 0);
    }
}

static void ui_edit_allocate_runs(ui_edit_t* e) {
    ui_edit_text_t* dt = &e->doc->text; // document text
    assert(e->para == null);
    assert(dt->np > 0);
    assert(e->para == null);
    bool done = ut_heap.alloc_zero((void**)&e->para,
                dt->np * sizeof(e->para[0])) == 0;
    swear(done, "out of memory - cannot continue");
}

static void ui_edit_invalidate_run(ui_edit_t* e, int32_t i) {
    if (e->para[i].run != null) {
        assert(e->para[i].runs > 0);
        ut_heap.free(e->para[i].run);
        e->para[i].run = null;
        e->para[i].runs = 0;
    } else {
        assert(e->para[i].runs == 0);
    }
}

static void ui_edit_invalidate_runs(ui_edit_t* e, int32_t f, int32_t t,
        int32_t np) { // [from..to] inclusive inside [0..np - 1]
    swear(e->para != null && f <= t && 0 <= f && t < np);
    for (int32_t i = f; i <= t; i++) { ui_edit_invalidate_run(e, i); }
}

static void ui_edit_invalidate_all_runs(ui_edit_t* e) {
    ui_edit_text_t* dt = &e->doc->text; // document text
    ui_edit_invalidate_runs(e, 0, dt->np - 1, dt->np);
}

static void ui_edit_dispose_runs(ui_edit_t* e, int32_t np) {
    assert(e->para != null);
    ui_edit_invalidate_runs(e, 0, np - 1, np);
    ut_heap.free(e->para);
    e->para = null;
}

static void ui_edit_dispose_all_runs(ui_edit_t* e) {
    ui_edit_dispose_runs(e, e->doc->text.np);
}

static void ui_edit_layout_now(ui_edit_t* e) {
    if (e->measure != null && e->layout != null && e->w > 0) {
        e->layout(&e->view);
        ui_edit_invalidate_view(e);
    }
}

static void ui_edit_if_sle_layout(ui_edit_t* e) {
    // only for single line edit controls that were already initialized
    // and measured horizontally at least once.
    if (e->sle && e->layout != null && e->w > 0) {
        ui_edit_layout_now(e);
    }
}

static void ui_edit_set_font(ui_edit_t* e, ui_fm_t* f) {
    ui_edit_invalidate_all_runs(e);
    e->scroll.rn = 0;
    e->fm = f;
    ui_edit_layout_now(e);
    ui_app.request_layout();
}

// Paragraph number, glyph number -> run number

static ui_edit_pr_t ui_edit_pg_to_pr(ui_edit_t* e, const ui_edit_pg_t pg) {
    ui_edit_text_t* dt = &e->doc->text; // document text
    assert(0 <= pg.pn && pg.pn < dt->np);
    const ui_edit_str_t* str = &dt->ps[pg.pn];
    ui_edit_pr_t pr = { .pn = pg.pn, .rn = -1 };
    if (str->b == 0) { // empty
        assert(pg.gp == 0);
        pr.rn = 0;
    } else {
        assert(0 <= pg.pn && pg.pn < dt->np);
        int32_t runs = 0;
        const ui_edit_run_t* run = ui_edit_paragraph_runs(e, pg.pn, &runs);
        if (pg.gp == str->g + 1) {
            pr.rn = runs - 1; // TODO: past last glyph ??? is this correct?
        } else {
            assert(0 <= pg.gp && pg.gp <= str->g);
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
    assert(ui_edit_range.uint64(pg0) <= ui_edit_range.uint64(pg1));
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
    // layout may decrease number of runs when view is growing:
    if (e->scroll.rn >= runs) { e->scroll.rn = runs - 1; }
    assert(0 <= e->scroll.rn && e->scroll.rn < runs,
            "e->scroll.rn: %d runs: %d", e->scroll.rn, runs);
    return (ui_edit_pg_t) { .pn = e->scroll.pn, .gp = run[e->scroll.rn].gp };
}

static int32_t ui_edit_first_visible_run(ui_edit_t* e, int32_t pn) {
    return pn == e->scroll.pn ? e->scroll.rn : 0;
}

// ui_edit::pg_to_xy() paragraph # glyph # -> (x,y) in [0,0  width x height]

static ui_point_t ui_edit_pg_to_xy(ui_edit_t* e, const ui_edit_pg_t pg) {
    ui_edit_text_t* dt = &e->doc->text; // document text
    assert(0 <= pg.pn && pg.pn < dt->np);
    ui_point_t pt = { .x = -1, .y = 0 };
    const int32_t spn = e->scroll.pn + e->visible_runs + 1;
    const int32_t pn = ut_min(ut_min(spn, pg.pn + 1), dt->np - 1);
    for (int32_t i = e->scroll.pn; i <= pn && pt.x < 0; i++) {
        assert(0 <= i && i < dt->np);
        const ui_edit_str_t* str = &dt->ps[i];
        int32_t runs = 0;
        const ui_edit_run_t* run = ui_edit_paragraph_runs(e, i, &runs);
        for (int32_t j = ui_edit_first_visible_run(e, i); j < runs; j++) {
            const int32_t last_run = j == runs - 1;
            int32_t gc = run[j].glyphs;
            if (i == pg.pn) {
                // in the last `run` of a paragraph x after last glyph is OK
                if (run[j].gp <= pg.gp && pg.gp < run[j].gp + gc + last_run) {
                    const char* s = str->u + run[j].bp;
                    const uint32_t bp2e = str->b - run[j].bp; // to end of str
                    int32_t ofs = ui_edit_str.gp_to_bp(s, bp2e, pg.gp - run[j].gp);
                    swear(ofs >= 0);
                    pt.x = ui_edit_text_width(e, s, ofs);
                    break;
                }
            }
            pt.y += e->fm->height;
        }
    }
    if (0 <= pt.x && pt.x < e->edit.w && 0 <= pt.y && pt.y < e->edit.h) {
        // all good, inside visible rectangle or right after it
    } else {
//      ut_traceln("%d:%d (%d,%d) outside of %dx%d", pg.pn, pg.gp,
//          pt.x, pt.y, e->edit.w, e->edit.h);
        pt = (ui_point_t){-1, -1};
    }
    return pt;
}

static int32_t ui_edit_glyph_width_px(ui_edit_t* e, const ui_edit_pg_t pg) {
    ui_edit_text_t* dt = &e->doc->text; // document text
    assert(0 <= pg.pn && pg.pn < dt->np);
    const ui_edit_str_t* str = &dt->ps[pg.pn];
    const char* text = str->u;
    int32_t gc = str->g;
    if (pg.gp == 0 &&  gc == 0) {
        return 0; // empty paragraph
    } else if (pg.gp < gc) {
        const int32_t bp = ui_edit_str.gp_to_bp(text, str->b, pg.gp);
        swear(bp >= 0);
        const char* s = text + bp;
        int32_t bytes_in_glyph = ut_str.utf8bytes(s, str->b - bp);
        swear(bytes_in_glyph > 0);
        int32_t x = ui_edit_text_width(e, s, bytes_in_glyph);
        return x;
    } else {
        assert(pg.gp == gc, "only next position past last glyph is allowed");
        return 0;
    }
}

// xy_to_pg() (x,y) (0,0, width x height) -> paragraph # glyph #

static ui_edit_pg_t ui_edit_xy_to_pg(ui_edit_t* e, int32_t x, int32_t y) {
// TODO: remove
//  const ui_ltrb_t i = ui_view.margins(&e->view, &e->view.insets);
//  ut_traceln("x,y: %d,%d insets left:%d right:%d", x, y, i.left, i.right);
    ui_edit_text_t* dt = &e->doc->text; // document text
    ui_edit_pg_t pg = {-1, -1};
    int32_t py = 0; // paragraph `y' coordinate
    for (int32_t i = e->scroll.pn; i < dt->np && pg.pn < 0; i++) {
        assert(0 <= i && i < dt->np);
        const ui_edit_str_t* str = &dt->ps[i];
        int32_t runs = 0;
        const ui_edit_run_t* run = ui_edit_paragraph_runs(e, i, &runs);
        for (int32_t j = ui_edit_first_visible_run(e, i); j < runs && pg.pn < 0; j++) {
            const ui_edit_run_t* r = &run[j];
            const char* s = str->u + run[j].bp;
            if (py <= y && y < py + e->fm->height) {
                int32_t w = ui_edit_text_width(e, s, r->bytes);
                pg.pn = i;
                if (x >= w) {
                    pg.gp = r->gp + r->glyphs;
                } else {
                    pg.gp = r->gp + ui_edit_glyph_at_x(e, i, j, x);
// TODO: remove
//                  ut_traceln("pg.gp: %d r->gp: %d ui_edit_glyph_at_x(%d, %d, x:%d)",
//                          pg.gp, r->gp, i, j, x, ui_edit_glyph_at_x(e, i, j, x));
                    if (pg.gp < r->glyphs - 1) {
                        ui_edit_pg_t right = {pg.pn, pg.gp + 1};
                        int32_t x0 = ui_edit_pg_to_xy(e, pg).x;
                        int32_t x1 = ui_edit_pg_to_xy(e, right).x;
// TODO: remove
//                      ut_traceln("x0: %d x1: %d", x0, x1);
                        if (x1 - x < x - x0) {
                            pg.gp++; // snap to closest glyph's 'x'
                        }
                    }
                }
            } else {
                py += e->fm->height;
            }
        }
        if (py > e->h) { break; }
    }
// TODO: remove
//  ut_traceln("x,y: %d,%d p:d %d:%d", x, y, pg.pn, pg.gp);
    return pg;
}

static void ui_edit_set_caret(ui_edit_t* e, int32_t x, int32_t y) {
    if (e->caret.x != x || e->caret.y != y) {
        if (e->focused && ui_app.focused()) {
            ui_app.move_caret(e->x + x, e->y + y);
        }
        const ui_ltrb_t i = ui_view.margins(&e->view, &e->insets);
        // caret in i.left .. e->view.w - i.right
        //          i.top  .. e->view.h - i.bottom
        // coordinate space
        swear(i.left <= x && x < e->w && i.top <= y && y < e->h);
        e->caret.x = x;
        e->caret.y = y;
// TODO: remove
//      ut_traceln("caret: %d, %d", x, y);
    }
}

// scroll_up() text moves up (north) in the visible view,
// scroll position increments moves down (south)

static void ui_edit_scroll_up(ui_edit_t* e, int32_t run_count) {
    ui_edit_text_t* dt = &e->doc->text; // document text
    assert(0 < run_count, "does it make sense to have 0 scroll?");
    while (run_count > 0 && e->scroll.pn < dt->np - 1) {
        const ui_edit_pg_t scroll = ui_edit_scroll_pg(e);
        const ui_edit_pg_t next = (ui_edit_pg_t){
            .pn = ut_min(scroll.pn + e->visible_runs + 1, dt->np - 1),
            .gp = 0
        };
        const int32_t between = ui_edit_runs_between(e, scroll, next);
        if (between <= e->visible_runs - 1) {
            run_count = 0; // enough
        } else {
            const int32_t runs = ui_edit_paragraph_run_count(e, e->scroll.pn);
            if (e->scroll.rn < runs - 1) {
                e->scroll.rn++;
            } else if (e->scroll.pn < dt->np - 1) {
                e->scroll.pn++;
                e->scroll.rn = 0;
            }
            run_count--;
            assert(e->scroll.pn >= 0 && e->scroll.rn >= 0);
        }
    }
    ui_edit_if_sle_layout(e);
    ui_edit_invalidate_view(e);
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
    ui_edit_invalidate_view(e);
}

static void ui_edit_scroll_into_view(ui_edit_t* e, const ui_edit_pg_t pg) {
    ui_edit_text_t* dt = &e->doc->text; // document text
    assert(0 <= pg.pn && pg.pn < dt->np && dt->np > 0);
    if (e->inside.bottom > 0) {
        if (e->sle) { assert(pg.pn == 0); }
        const int32_t rn = ui_edit_pg_to_pr(e, pg).rn;
        const uint64_t scroll = (uint64_t)e->scroll.pn << 32 | e->scroll.rn;
        const uint64_t caret  = (uint64_t)pg.pn << 32 | rn;
        uint64_t last = 0;
        int32_t py = 0;
        const int32_t pn = e->scroll.pn;
        const int32_t bottom = e->inside.bottom;
        for (int32_t i = pn; i < dt->np && py < bottom; i++) {
            int32_t runs = ui_edit_paragraph_run_count(e, i);
            const int32_t fvr = ui_edit_first_visible_run(e, i);
            for (int32_t j = fvr; j < runs && py < bottom; j++) {
                last = (uint64_t)i << 32 | j;
                py += e->fm->height;
            }
        }
        int32_t sle_runs = e->sle && e->w > 0 ?
            ui_edit_paragraph_run_count(e, 0) : 0;
        ui_edit_pg_t end = ui_edit_text.end(dt);
        ui_edit_pr_t lp = ui_edit_pg_to_pr(e, end);
        uint64_t eof = (uint64_t)(dt->np - 1) << 32 | lp.rn;
        if (last == eof && py <= bottom - e->fm->height) {
            // vertical white space for EOF on the screen
            last = (uint64_t)dt->np << 32 | 0;
        }
        if (scroll <= caret && caret < last) {
            // no scroll
        } else if (caret < scroll) {
            ui_edit_invalidate_view(e);
            e->scroll.pn = pg.pn;
            e->scroll.rn = rn;
        } else if (e->sle && sle_runs * e->fm->height <= e->h) {
            // single line edit control fits vertically - no scroll
        } else {
            ui_edit_invalidate_view(e);
            assert(caret >= last);
            e->scroll.pn = pg.pn;
            e->scroll.rn = rn;
            while (e->scroll.pn > 0 || e->scroll.rn > 0) {
                ui_point_t pt = ui_edit_pg_to_xy(e, pg);
                if (pt.y + e->fm->height > bottom - e->fm->height) { break; }
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

static void ui_edit_caret_to(ui_edit_t* e, const ui_edit_pg_t to) {
    ui_edit_scroll_into_view(e, to);
    ui_point_t pt =  ui_edit_pg_to_xy(e, to);
    ui_edit_set_caret(e, pt.x + e->inside.left, pt.y + e->inside.top);
}

static void ui_edit_move_caret(ui_edit_t* e, const ui_edit_pg_t pg) {
    if (e->w > 0) { // width == 0 means no measure/layout yet
        ui_rect_t before = ui_edit_selection_rect(e);
        ui_edit_text_t* dt = &e->doc->text; // document text
        assert(0 <= pg.pn && pg.pn < dt->np);
        // single line edit control cannot move caret past fist paragraph
        if (!e->sle || pg.pn < dt->np) {
            e->selection.a[1] = pg;
            ui_edit_caret_to(e, pg);
            if (!ui_app.shift && e->edit.buttons == 0) {
                e->selection.a[0] = e->selection.a[1];
            }
        }
        ui_rect_t after = ui_edit_selection_rect(e);
        ui_edit_invalidate_rect(e, ui.combine_rect(&before, &after));
    }
}

static ui_edit_pg_t ui_edit_insert_inline(ui_edit_t* e, ui_edit_pg_t pg,
        const char* text, int32_t bytes) {
    // insert_inline() inserts text (not containing '\n' in it)
    assert(bytes > 0);
    for (int32_t i = 0; i < bytes; i++) { assert(text[i] != '\n'); }
    ui_edit_range_t r = { .from = pg, .to = pg };
    int32_t g = 0;
    if (ui_edit_doc.replace(e->doc, &r, text, bytes)) {
        ui_edit_text_t t = {0};
        if (ui_edit_text.init(&t, text, bytes, false)) {
            assert(t.ps != null && t.np == 1);
            g = t.np == 1 && t.ps != null ? t.ps[0].g : 0;
            ui_edit_text.dispose(&t);
        }
    }
    r.from.gp += g;
    r.to.gp += g;
    e->selection = r;
    ui_edit_move_caret(e, e->selection.from);
    return r.to;
}

static ui_edit_pg_t ui_edit_insert_paragraph_break(ui_edit_t* e,
        ui_edit_pg_t pg) {
    ui_edit_range_t r = { .from = pg, .to = pg };
    bool ok = ui_edit_doc.replace(e->doc, &r, "\n", 1);
    ui_edit_pg_t next = {.pn = pg.pn + 1, .gp = 0};
    return ok ? next : pg;
}

static ui_edit_pg_t ui_edit_skip_left_blanks(ui_edit_t* e,
    ui_edit_pg_t pg) {
    ui_edit_text_t* dt = &e->doc->text; // document text
    swear(pg.pn <= dt->np - 1);
    while (pg.gp > 0) {
        pg.gp--;
        ui_edit_glyph_t glyph = ui_edit_glyph_at(e, pg);
        if (glyph.bytes > 0 && (uint8_t)*glyph.s != 0x20u) {
            pg.gp++;
            break;
        }
    }
    return pg;
}

static ui_edit_pg_t ui_edit_skip_right_blanks(ui_edit_t* e,
    ui_edit_pg_t pg) {
    ui_edit_text_t* dt = &e->doc->text; // document text
    swear(pg.pn <= dt->np - 1);
    int32_t glyphs = ui_edit_glyphs_in_paragraph(e, pg.pn);
    ui_edit_glyph_t glyph = ui_edit_glyph_at(e, pg);
    while (pg.gp < glyphs && glyph.bytes > 0 && (uint8_t)*glyph.s == 0x20u) {
        pg.gp++;
        glyph = ui_edit_glyph_at(e, pg);
    }
    return pg;
}

static bool ui_edit_is_cjk_or_emoji(uint32_t utf32) {
    // Simplified range checks
    return
        // CJK Unified Ideographs and Extensions
        (utf32 >=  0x3400 && utf32 <=  0x4DBF) ||
        (utf32 >=  0x4E00 && utf32 <=  0x9FFF) ||
        (utf32 >= 0x20000 && utf32 <= 0x2A6DF) ||
        (utf32 >= 0x2A700 && utf32 <= 0x2B73F) ||
        (utf32 >= 0x2B740 && utf32 <= 0x2B81F) ||
        (utf32 >= 0x2B820 && utf32 <= 0x2CEAF) ||
        (utf32 >= 0x2CEB0 && utf32 <= 0x2EBEF) ||
        // CJK Compatibility Ideographs
        (utf32 >= 0xF900 && utf32 <= 0xFAFF)   ||
        (utf32 >= 0x2F800 && utf32 <= 0x2FA1F) ||
        // Emojis and Symbols
        (utf32 >= 0x1F300 && utf32 <= 0x1F5FF) ||
        (utf32 >= 0x1F600 && utf32 <= 0x1F64F) ||
        (utf32 >= 0x1F680 && utf32 <= 0x1F6FF) ||
        (utf32 >= 0x1F700 && utf32 <= 0x1F77F) ||
        (utf32 >= 0x1F780 && utf32 <= 0x1F7FF) ||
        (utf32 >= 0x1F800 && utf32 <= 0x1F8FF) ||
        (utf32 >= 0x1F900 && utf32 <= 0x1F9FF) ||
        (utf32 >= 0x1FA00 && utf32 <= 0x1FA6F) ||
        (utf32 >= 0x1FA70 && utf32 <= 0x1FAFF) ||
        (utf32 >= 0x1FB00 && utf32 <= 0x1FBFF);
}

static bool ui_edit_can_break(const char* utf8_1, int32_t b1,
                              const char* utf8_2, int32_t b2) {
    uint32_t cp1 = ut_str.utf32(utf8_1, b1);
    uint32_t cp2 = ut_str.utf32(utf8_2, b2);
    return ui_edit_is_cjk_or_emoji(cp1) || ui_edit_is_cjk_or_emoji(cp2);
}

static ui_edit_range_t ui_edit_word_range(ui_edit_t* e, ui_edit_pg_t pg) {
    ui_edit_range_t r = { .from = pg, .to = pg };
    ui_edit_text_t* dt = &e->doc->text; // document text
    if (0 <= pg.pn && 0 <= pg.gp) {
        swear(pg.pn <= dt->np - 1);
        int32_t glyphs = ui_edit_glyphs_in_paragraph(e, pg.pn);
        if (pg.gp > glyphs) { pg.gp = ut_max(0, glyphs); }
        ui_edit_glyph_t glyph = ui_edit_glyph_at(e, pg);
        uint8_t b = (uint8_t)*glyph.s; // byte unsigned
        bool not_a_white_space = glyph.bytes > 0 && b > 0x20u;
        if (!not_a_white_space && pg.gp > 0) {
            pg.gp--;
            glyph = ui_edit_glyph_at(e, pg);
            b = (uint8_t)*glyph.s; // byte unsigned
            not_a_white_space = glyph.bytes > 0 && b > 0x20u;
        }
        b = (uint8_t)*glyph.s; // byte unsigned
        if (glyph.bytes > 0 && 0x20u < b) {
            ui_edit_pg_t from = pg;
            ui_edit_glyph_t g = ui_edit_glyph_at(e, from);
            b = (uint8_t)*glyph.s; // byte unsigned
            char first_ascii = from.gp == 0 && glyphs > 0 &&
                               glyph.bytes == 1 && b <= 0x7Fu ? *glyph.s : 0x00;
            while (from.gp > 0) {
                from.gp--;
                g = ui_edit_glyph_at(e, from);
                b = (uint8_t)*glyph.s; // byte unsigned
                first_ascii = glyph.bytes == 1 && b <= 0x7Fu ? *glyph.s : first_ascii;
                const bool starts_with_alnum = isalnum(first_ascii);
                bool stop = starts_with_alnum &&
                            (g.bytes != 1 || b > 0x7Fu || !isalnum(*g.s));
                if (g.bytes != 1 || b <= 0x20u || b > 0x7Fu || stop) {
                    from.gp++;
//                  ut_traceln("left while space @%d 0x%02X", from.gp, *g.s);
                    break;
                }
            }
            r.from = from;
            ui_edit_pg_t to = pg;
            while (to.gp < glyphs) {
                to.gp++;
                g = ui_edit_glyph_at(e, to);
                const bool starts_with_alnum = isalnum(first_ascii);
                b = (uint8_t)*glyph.s; // byte unsigned
                bool stop = starts_with_alnum &&
                            (g.bytes != 1 || b > 0x7Fu || !isalnum(*g.s));
                if (g.bytes != 1 || b <= 0x20u || b > 0x7Fu || stop) {
//                  ut_traceln("right while space @%d 0x%02X", to.gp, *g.s);
                    break;
                }
            }
            r.to = to;
        }
    }
    return r;
}

static void ui_edit_ctrl_left(ui_edit_t* e) {
    ui_edit_invalidate_rect(e, ui_edit_selection_rect(e));
    const ui_edit_range_t s = e->selection;
    ui_edit_pg_t to = e->selection.to;
    if (to.gp == 0) {
        if (to.pn > 0) {
            to.pn--;
            int32_t runs = 0;
            const ui_edit_run_t* run = ui_edit_paragraph_runs(e, to.pn, &runs);
            to.gp = run[runs - 1].gp + run[runs - 1].glyphs;
        }
    } else {
        to.gp--;
    }
    const ui_edit_pg_t lf = ui_edit_skip_left_blanks(e, to);
    const ui_edit_range_t w = ui_edit_word_range(e, lf);
    e->selection.to = w.from;
    if (ui_app.shift) {
        e->selection.from = s.from;
    } else {
        e->selection.from = w.from;
    }
    ui_edit_invalidate_rect(e, ui_edit_selection_rect(e));
}

static void ui_edit_key_left(ui_edit_t* e) {
    ui_edit_pg_t to = e->selection.a[1];
    if (to.pn > 0 || to.gp > 0) {
        if (ui_app.ctrl) {
            ui_edit_ctrl_left(e);
        } else {
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
}

static void ui_edit_ctrl_right(ui_edit_t* e) {
    const ui_edit_text_t* dt = &e->doc->text; // document text
    ui_edit_range_t s = e->selection;
    ui_edit_pg_t to = e->selection.to;
    int32_t glyphs = ui_edit_glyphs_in_paragraph(e, to.pn);
    if (to.pn < dt->np - 1 || to.gp < glyphs) {
        ui_edit_invalidate_rect(e, ui_edit_selection_rect(e));
        if (to.gp == glyphs) {
            to.pn++;
            to.gp = 0;
        } else {
            to.gp++;
        }
        ui_edit_pg_t rt = ui_edit_skip_right_blanks(e, to);
        ui_edit_range_t w = ui_edit_word_range(e, rt);
        e->selection.to = w.to;
        if (ui_app.shift) {
            e->selection.from = s.from;
        } else {
            e->selection.from = w.to;
        }
        ui_edit_invalidate_rect(e, ui_edit_selection_rect(e));
    }
}

static void ui_edit_key_right(ui_edit_t* e) {
    ui_edit_text_t* dt = &e->doc->text; // document text
    ui_edit_pg_t to = e->selection.a[1];
    if (to.pn < dt->np) {
        if (ui_app.ctrl) {
            ui_edit_ctrl_right(e);
        } else {
            int32_t glyphs = ui_edit_glyphs_in_paragraph(e, to.pn);
            if (to.gp < glyphs) {
                to.gp++;
                ui_edit_scroll_into_view(e, to);
            } else if (!e->sle && to.pn < dt->np - 1) {
                to.pn++;
                to.gp = 0;
                ui_edit_scroll_into_view(e, to);
            }
            ui_edit_move_caret(e, to);
            e->last_x = -1;
        }
    }
}

static void ui_edit_reuse_last_x(ui_edit_t* e, ui_point_t* pt) {
    // Vertical caret movement visually tend to move caret horizontally
    // in proportional font text. Remembering starting `x' value for vertical
    // movements alleviates this unpleasant UX experience to some degree.
    if (pt->x > 0) {
        if (e->last_x > 0) {
            int32_t prev = e->last_x - e->fm->em.w;
            int32_t next = e->last_x + e->fm->em.w;
            if (prev <= pt->x && pt->x <= next) {
                pt->x = e->last_x;
            }
        }
        e->last_x = pt->x;
    }
}

static void ui_edit_key_up(ui_edit_t* e) {
    const ui_edit_pg_t pg = e->selection.a[1];
    ui_edit_pg_t to = pg;
    if (to.pn > 0 || ui_edit_pg_to_pr(e, to).rn > 0) {
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
        if (to.pn >= 0 && to.gp >= 0) {
            int32_t rn0 = ui_edit_pg_to_pr(e, pg).rn;
            int32_t rn1 = ui_edit_pg_to_pr(e, to).rn;
            if (rn1 > 0 && rn0 == rn1) { // same run
                assert(to.gp > 0, "word break must not break on zero gp");
                int32_t runs = 0;
                const ui_edit_run_t* run = ui_edit_paragraph_runs(e, to.pn, &runs);
                to.gp = run[rn1].gp;
            }
        }
    }
    if (to.pn >= 0 && to.gp >= 0) {
        ui_edit_move_caret(e, to);
    }
}

static void ui_edit_key_down(ui_edit_t* e) {
    const ui_edit_pg_t pg = e->selection.a[1];
    ui_point_t pt = ui_edit_pg_to_xy(e, pg);
    ui_edit_reuse_last_x(e, &pt);
    // scroll runs guaranteed to be already layout for current state of view:
    ui_edit_pg_t scroll = ui_edit_scroll_pg(e);
    const int32_t run_count = ui_edit_runs_between(e, scroll, pg);
    if (!e->sle && run_count >= e->visible_runs - 1) {
        ui_edit_scroll_up(e, 1);
    } else {
        pt.y += e->fm->height;
    }
    ui_edit_pg_t to = ui_edit_xy_to_pg(e, pt.x, pt.y);
    if (to.pn >= 0 && to.gp >= 0) {
        ui_edit_move_caret(e, to);
    }
}

static void ui_edit_key_home(ui_edit_t* e) {
    if (ui_app.ctrl) {
        e->scroll.pn = 0;
        e->scroll.rn = 0;
        e->selection.a[1].pn = 0;
        e->selection.a[1].gp = 0;
        ui_edit_invalidate_view(e);
    }
    const int32_t pn = e->selection.a[1].pn;
    int32_t runs = ui_edit_paragraph_run_count(e, pn);
    const ui_edit_paragraph_t* para = &e->para[pn];
    if (runs <= 1) {
        e->selection.a[1].gp = 0;
    } else {
        int32_t rn = ui_edit_pg_to_pr(e, e->selection.a[1]).rn;
        assert(0 <= rn && rn < runs);
        const int32_t gp = para->run[rn].gp;
        if (e->selection.a[1].gp != gp) {
            // first Home keystroke moves caret to start of run
            e->selection.a[1].gp = gp;
        } else {
            // second Home keystroke moves caret start of paragraph
            e->selection.a[1].gp = 0;
            if (e->scroll.pn >= e->selection.a[1].pn) { // scroll in
                e->scroll.pn = e->selection.a[1].pn;
                e->scroll.rn = 0;
                ui_edit_invalidate_view(e);
            }
        }
    }
    if (!ui_app.shift) {
        e->selection.a[0] = e->selection.a[1];
    }
    ui_edit_move_caret(e, e->selection.a[1]);
}

static void ui_edit_key_eol(ui_edit_t* e) {
    const ui_edit_text_t* dt = &e->doc->text; // document text
    int32_t pn = e->selection.a[1].pn;
    int32_t gp = e->selection.a[1].gp;
    assert(0 <= pn && pn < dt->np);
    const ui_edit_str_t* str = &dt->ps[pn];
    int32_t runs = 0;
    const ui_edit_run_t* run = ui_edit_paragraph_runs(e, pn, &runs);
    int32_t rn = ui_edit_pg_to_pr(e, e->selection.a[1]).rn;
    assert(0 <= rn && rn < runs);
    if (rn == runs - 1) {
        e->selection.a[1].gp = str->g;
    } else if (e->selection.a[1].gp == str->g) {
        // at the end of paragraph do nothing (or move caret to EOF?)
    } else if (str->g > 0 && gp != run[rn].glyphs - 1) {
        e->selection.a[1].gp = run[rn].gp + run[rn].glyphs - 1;
    } else {
        e->selection.a[1].gp = str->g;
    }
}

static void ui_edit_key_end(ui_edit_t* e) {
    const ui_edit_text_t* dt = &e->doc->text; // document text
    if (ui_app.ctrl) {
        int32_t py = e->inside.bottom;
        for (int32_t i = dt->np - 1; i >= 0 && py >= e->fm->height; i--) {
            int32_t runs = ui_edit_paragraph_run_count(e, i);
            for (int32_t j = runs - 1; j >= 0 && py >= e->fm->height; j--) {
                py -= e->fm->height;
                if (py < e->fm->height) {
                    e->scroll.pn = i;
                    e->scroll.rn = j;
                }
            }
        }
        e->selection.a[1] = ui_edit_text.end(dt);
        ui_edit_invalidate_view(e);
    } else {
        ui_edit_key_eol(e);
    }
    if (!ui_app.shift) {
        e->selection.a[0] = e->selection.a[1];
    }
    ui_edit_move_caret(e, e->selection.a[1]);
}

static void ui_edit_key_page_up(ui_edit_t* e) {
    int32_t n = ut_max(1, e->visible_runs - 1);
    ui_edit_pg_t scr = ui_edit_scroll_pg(e);
    const ui_edit_pg_t prev = (ui_edit_pg_t){
        .pn = ut_max(scr.pn - e->visible_runs - 1, 0),
        .gp = 0
    };
    const int32_t m = ui_edit_runs_between(e, prev, scr);
    if (m > n) {
        ui_point_t pt = ui_edit_pg_to_xy(e, e->selection.a[1]);
        ui_edit_pr_t scroll = e->scroll;
        ui_edit_scroll_down(e, n);
        if (scroll.pn != e->scroll.pn || scroll.rn != e->scroll.rn) {
            ui_edit_pg_t pg = ui_edit_xy_to_pg(e, pt.x, pt.y);
            ui_edit_move_caret(e, pg);
        }
    } else {
        const ui_edit_pg_t bof = {.pn = 0, .gp = 0};
        ui_edit_move_caret(e, bof);
    }
}

static void ui_edit_key_page_down(ui_edit_t* e) {
    const ui_edit_text_t* dt = &e->doc->text; // document text
    const int32_t n = ut_max(1, e->visible_runs - 1);
    const ui_edit_pg_t scr = ui_edit_scroll_pg(e);
    const ui_edit_pg_t next = (ui_edit_pg_t){
        .pn = ut_min(scr.pn + e->visible_runs + 1, dt->np - 1),
        .gp = 0
    };
    const int32_t m = ui_edit_runs_between(e, scr, next);
    if (m > n) {
        const ui_point_t pt = ui_edit_pg_to_xy(e, e->selection.a[1]);
        const ui_edit_pr_t scroll = e->scroll;
        ui_edit_scroll_up(e, n);
        if (scroll.pn != e->scroll.pn || scroll.rn != e->scroll.rn) {
            ui_edit_pg_t pg = ui_edit_xy_to_pg(e, pt.x, pt.y);
            ui_edit_move_caret(e, pg);
        }
    } else {
        const ui_edit_pg_t end = ui_edit_text.end(dt);
        ui_edit_move_caret(e, end);
    }
}

static void ui_edit_key_delete(ui_edit_t* e) {
    ui_edit_text_t* dt = &e->doc->text; // document text
    uint64_t f = ui_edit_range.uint64(e->selection.a[0]);
    uint64_t t = ui_edit_range.uint64(e->selection.a[1]);
    uint64_t end = ui_edit_range.uint64(ui_edit_text.end(dt));
    if (f == t && t != end) {
        ui_edit_pg_t s1 = e->selection.a[1];
        ui_edit.key_right(e);
        e->selection.a[1] = s1;
    }
    ui_edit.erase(e);
}

static void ui_edit_key_backspace(ui_edit_t* e) {
    uint64_t f = ui_edit_range.uint64(e->selection.a[0]);
    uint64_t t = ui_edit_range.uint64(e->selection.a[1]);
    if (t != 0 && f == t) {
        ui_edit_pg_t s1 = e->selection.a[1];
        ui_edit.key_left(e);
        e->selection.a[1] = s1;
    }
    ui_edit.erase(e);
}

static void ui_edit_key_enter(ui_edit_t* e) {
    assert(!e->ro);
    if (!e->sle) {
        ui_edit.erase(e);
        e->selection.a[1] = ui_edit_insert_paragraph_break(e, e->selection.a[1]);
        e->selection.a[0] = e->selection.a[1];
        ui_edit_move_caret(e, e->selection.a[1]);
    } else { // single line edit callback
        if (ui_edit.enter != null) { ui_edit.enter(e); }
    }
}

static bool ui_edit_key_pressed(ui_view_t* v, int64_t key) {
    bool swallow = false;
    assert(v->type == ui_view_text);
    ui_edit_t* e = (ui_edit_t*)v;
    ui_edit_text_t* dt = &e->doc->text; // document text
    if (e->focused) {
        swallow = true;
        if (key == ui.key.down && e->selection.a[1].pn < dt->np) {
            ui_edit.key_down(e);
        } else if (key == ui.key.up && dt->np > 1) {
            ui_edit.key_up(e);
        } else if (key == ui.key.left) {
            ui_edit.key_left(e);
        } else if (key == ui.key.right) {
            ui_edit.key_right(e);
        } else if (key == ui.key.pageup) {
            ui_edit.key_page_up(e);
        } else if (key == ui.key.pagedw) {
            ui_edit.key_page_down(e);
        } else if (key == ui.key.home) {
            ui_edit.key_home(e);
        } else if (key == ui.key.end) {
            ui_edit.key_end(e);
        } else if (key == ui.key.del && !e->ro) {
            ui_edit.key_delete(e);
        } else if (key == ui.key.back && !e->ro) {
            ui_edit.key_backspace(e);
        } else if (key == ui.key.enter && !e->ro) {
            ui_edit.key_enter(e);
        } else {
            swallow = false; // ignore other keys
        }
    }
    return swallow;
}

static void ui_edit_undo(ui_edit_t* e) {
    if (e->doc->undo != null) {
        ui_edit_doc.undo(e->doc);
    } else {
        ui_app.beep(ui.beep.error);
    }
}
static void ui_edit_redo(ui_edit_t* e) {
    if (e->doc->redo != null) {
        ui_edit_doc.redo(e->doc);
    } else {
        ui_app.beep(ui.beep.error);
    }
}

static void ui_edit_character(ui_view_t* v, const char* utf8) {
    assert(v->type == ui_view_text);
    assert(!ui_view.is_hidden(v) && !ui_view.is_disabled(v));
    #pragma push_macro("ui_edit_ctrl")
    #define ui_edit_ctrl(c) ((char)((c) - 'a' + 1))
    ui_edit_t* e = (ui_edit_t*)v;
    if (e->focused) {
        char ch = utf8[0];
        if (ui_app.ctrl) {
            if (ch == ui_edit_ctrl('a')) { ui_edit.select_all(e); }
            if (ch == ui_edit_ctrl('c')) { ui_edit.copy_to_clipboard(e); }
            if (!e->ro) {
                if (ch == ui_edit_ctrl('x')) { ui_edit.cut_to_clipboard(e); }
                if (ch == ui_edit_ctrl('v')) { ui_edit.paste_from_clipboard(e); }
                if (ch == ui_edit_ctrl('y')) { ui_edit_redo(e); }
                if (ch == ui_edit_ctrl('z') || ch == ui_edit_ctrl('Z')) {
                    if (ui_app.shift) { // Ctrl+Shift+Z
                        ui_edit_redo(e);
                    } else { // Ctrl+Z
                        ui_edit_undo(e);
                    }
                }
            }
        }
        if (0x20u <= (uint8_t)ch && !e->ro) { // 0x20 space
            int32_t len = (int32_t)strlen(utf8);
            int32_t bytes = ut_str.utf8bytes(utf8, len);
            if (bytes > 0) {
                ui_edit.erase(e); // remove selected text to be replaced by glyph
                e->selection.a[1] = ui_edit_insert_inline(e,
                    e->selection.a[1], utf8, bytes);
                e->selection.a[0] = e->selection.a[1];
                ui_edit_move_caret(e, e->selection.a[1]);
            } else {
                ut_traceln("invalid UTF8: 0x%02X%02X%02X%02X",
                        utf8[0], utf8[1], utf8[2], utf8[3]);
            }
        }
    }
    #pragma pop_macro("ui_edit_ctrl")
}

static void ui_edit_select_word(ui_edit_t* e, int32_t x, int32_t y) {
    ui_edit_invalidate_rect(e, ui_edit_selection_rect(e));
    ui_edit_pg_t pg = ui_edit_xy_to_pg(e, x, y);
    if (0 <= pg.pn && 0 <= pg.gp) {
        ui_edit_range_t r = ui_edit_word_range(e, pg);
        int32_t glyphs = ui_edit_glyphs_in_paragraph(e, r.to.pn);
        if (r.to.pn == r.from.pn && r.to.gp == r.from.gp && r.to.gp < glyphs) {
            r.to.gp++; // at least one glyph to the right
        }
        if (ui_edit_range.compare(r.from, pg) != 0 ||
            ui_edit_range.compare(r.to, pg) != 0) {
            e->selection = r;
            ui_edit_caret_to(e, r.to);
//          ut_traceln("e->selection.a[1] = %d.%d", to.pn, to.gp);
            ui_edit_invalidate_rect(e, ui_edit_selection_rect(e));
            e->edit.buttons = 0;
        }
    }
}

static void ui_edit_select_paragraph(ui_edit_t* e, int32_t x, int32_t y) {
    ui_edit_invalidate_rect(e, ui_edit_selection_rect(e));
    ui_edit_text_t* dt = &e->doc->text; // document text
    ui_edit_pg_t p = ui_edit_xy_to_pg(e, x, y);
    if (0 <= p.pn && 0 <= p.gp) {
        ui_edit_range_t r = ui_edit_text.ordered(dt, &e->selection);
        int32_t glyphs = ui_edit_glyphs_in_paragraph(e, p.pn);
        if (p.gp > glyphs) { p.gp = ut_max(0, glyphs); }
        if (p.pn == r.a[0].pn && r.a[0].pn == r.a[1].pn &&
            r.a[0].gp <= p.gp && p.gp <= r.a[1].gp) {
            r.a[0].gp = 0;
            if (p.pn < dt->np - 1) {
                r.a[1].pn = p.pn + 1;
                r.a[1].gp = 0;
            } else {
                r.a[1].gp = dt->ps[p.pn].g;
            }
            e->selection = r;
            ui_edit_caret_to(e, r.to);
        }
        ui_edit_invalidate_rect(e, ui_edit_selection_rect(e));
        e->edit.buttons = 0;
    }
}

static void ui_edit_click(ui_edit_t* e, int32_t x, int32_t y) {
    // x, y in 0..e->w, 0->e.h coordinate space
    assert(0 <= x && x < e->w && 0 <= y && y < e->h);
    ui_edit_text_t* dt = &e->doc->text; // document text
    ui_edit_pg_t pg = ui_edit_xy_to_pg(e, x, y);
//  TODO: remove
//  ut_traceln("x,y: %d,%d p:d %d:%d", e->caret.x, e->caret.y, pg.pn, pg.gp);
    if (0 <= pg.pn && 0 <= pg.gp && ui_view.has_focus(&e->view)) {
        swear(dt->np > 0 && pg.pn < dt->np);
        int32_t glyphs = ui_edit_glyphs_in_paragraph(e, pg.pn);
        if (pg.gp > glyphs) { pg.gp = ut_max(0, glyphs); }
//      ut_traceln("move_caret: %d.%d", p.pn, p.gp);
        ui_edit_move_caret(e, pg);
    }
}

static void ui_edit_mouse_button_down(ui_edit_t* e, int32_t ix) {
    e->edit.buttons |= (1 << ix);
}

static void ui_edit_mouse_button_up(ui_edit_t* e, int32_t ix) {
    e->edit.buttons &= ~(1 << ix);
}

static bool ui_edit_tap(ui_view_t* v, int32_t unused(ix), bool pressed) {
    // `ix` ignored for now till context menu (copy/paste/select...)
    ui_edit_t* e = (ui_edit_t*)v;
    assert(ui_view.inside(v, &ui_app.mouse));
    const int32_t x = ui_app.mouse.x - (v->x + e->inside.left);
    const int32_t y = ui_app.mouse.y - (v->y + e->inside.top);
    bool inside = 0 <= x && x < e->w && 0 <= y && y < e->h;
//  TODO: remove
//  ut_traceln("mouse: %d,%d x,y: %d %d inside: %d", ui_app.mouse.x, ui_app.mouse.y, x, y, inside);
    if (inside) {
        if (pressed) {
            e->edit.buttons = 0;
            ui_edit_click(e, x, y);
            ui_edit_mouse_button_down(e, ix);
        } else if (!pressed) {
            ui_edit_mouse_button_up(e, ix);
        }
    }
    return true;
}

static bool ui_edit_long_press(ui_view_t* v, int32_t unused(ix)) {
    ui_edit_t* e = (ui_edit_t*)v;
    const int32_t x = ui_app.mouse.x - (v->x + e->inside.left);
    const int32_t y = ui_app.mouse.y - (v->y + e->inside.top);
    bool inside = 0 <= x && x < e->w && 0 <= y && y < e->h;
    if (inside && ui_edit_range.is_empty(e->selection)) {
        ui_edit_select_paragraph(e, x, y);
    }
    return true;
}

static bool ui_edit_double_tap(ui_view_t* v, int32_t unused(ix)) {
    ui_edit_t* e = (ui_edit_t*)v;
    const int32_t x = ui_app.mouse.x - (v->x + e->inside.left);
    const int32_t y = ui_app.mouse.y - (v->y + e->inside.top);
    bool inside = 0 <= x && x < e->w && 0 <= y && y < e->h;
    if (inside && e->selection.a[0].pn == e->selection.a[1].pn) {
        ui_edit_select_word(e, x, y);
    }
    return false;
}

static void ui_edit_mouse_scroll(ui_view_t* v, ui_point_t dx_dy) {
    if (v->w > 0 && v->h > 0) {
        const int32_t dy = dx_dy.y;
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
            const int32_t x = e->caret.x - e->inside.left;
            const int32_t y = e->caret.y - e->inside.top;
            ui_edit_pg_t pg = ui_edit_xy_to_pg(e, x, y);
// TODO: remove
//          ut_traceln("x,y: %d,%d caret: %d,%d p:d %d:%d", x, y, e->caret.x, e->caret.y, pg.pn, pg.gp);
            if (pg.pn >= 0 && pg.gp >= 0) {
                assert(pg.gp <= e->doc->text.ps[pg.pn].g);
                ui_edit_move_caret(e, pg);
            } else {
                ui_edit_click(e, x, y);
            }
        }
    }
}

static bool ui_edit_focus_gained(ui_view_t* v) {
    assert(v->type == ui_view_text);
    ui_edit_t* e = (ui_edit_t*)v;
    assert(v->focusable);
    if (ui_app.focused() && !e->focused) {
        ui_edit_create_caret(e);
        ui_edit_show_caret(e);
        ui_edit_if_sle_layout(e);
    }
    e->edit.buttons = 0;
    ui_app.request_redraw();
    return true;
}

static void ui_edit_focus_lost(ui_view_t* v) {
    assert(v->type == ui_view_text);
    ui_edit_t* e = (ui_edit_t*)v;
    if (e->focused) {
        ui_edit_hide_caret(e);
        ui_edit_destroy_caret(e);
        ui_edit_if_sle_layout(e);
    }
    e->edit.buttons = 0;
    ui_app.request_redraw();
}

static void ui_edit_erase(ui_edit_t* e) {
    if (e->selection.from.pn != e->selection.to.pn) {
        ui_edit_invalidate_view(e);
    } else {
        ui_edit_invalidate_rect(e, ui_edit_selection_rect(e));
    }
    ui_edit_range_t r = ui_edit_range.order(e->selection);
    if (!ui_edit_range.is_empty(r) && ui_edit_doc.replace(e->doc, &r, null, 0)) {
        e->selection = r;
        e->selection.to = e->selection.from;
        ui_edit_move_caret(e, e->selection.from);
    }
}

static void ui_edit_select_all(ui_edit_t* e) {
    e->selection = ui_edit_text.all_on_null(&e->doc->text, null);
    ui_edit_invalidate_view(e);
}

static int32_t ui_edit_save(ui_edit_t* e, char* text, int32_t* bytes) {
    ut_not_null(bytes);
    enum {
        error_insufficient_buffer = 122, // ERROR_INSUFFICIENT_BUFFER
        error_more_data = 234            // ERROR_MORE_DATA
    };
    int32_t r = 0;
    const int32_t utf8bytes = ui_edit_doc.utf8bytes(e->doc, null);
    if (text == null) {
        *bytes = utf8bytes;
        r = ut_runtime.error.more_data;
    } else if (*bytes < utf8bytes) {
        r = ut_runtime.error.insufficient_buffer;
    } else {
        ui_edit_doc.copy(e->doc, null, text, utf8bytes);
        assert(text[utf8bytes - 1] == 0x00);
    }
    return r;
}

static void ui_edit_clipboard_copy(ui_edit_t* e) {
    int32_t utf8bytes = ui_edit_doc.utf8bytes(e->doc, &e->selection);
    if (utf8bytes > 0) {
        char* text = null;
        bool ok = ut_heap.alloc((void**)&text, utf8bytes) == 0;
        swear(ok);
        ui_edit_doc.copy(e->doc, &e->selection, text, utf8bytes);
        assert(text[utf8bytes - 1] == 0x00); // verify zero termination
        ut_clipboard.put_text(text);
        ut_heap.free(text);
        static ui_label_t hint = ui_label(0.0f, "copied to clipboard");
        int32_t x = e->x + e->caret.x;
        int32_t y = e->y + e->caret.y - e->fm->height;
        if (y < ui_app.content->y) {
            y += e->fm->height * 2;
        }
        if (y > ui_app.content->y + ui_app.content->h - e->fm->height) {
            y = e->caret.y;
        }
        ui_app.show_hint(&hint, x, y, 0.5);
    }
}

static void ui_edit_clipboard_cut(ui_edit_t* e) {
    int32_t utf8bytes = ui_edit_doc.utf8bytes(e->doc, &e->selection);
    if (utf8bytes > 0) { ui_edit_clipboard_copy(e); }
    if (!e->ro) { ui_edit.erase(e); }
}

static ui_edit_pg_t ui_edit_paste_text(ui_edit_t* e,
        const char* text, int32_t bytes) {
    assert(!e->ro);
    ui_edit_text_t t = {0};
    ui_edit_text.init(&t, text, bytes, false);
    ui_edit_range_t r = ui_edit_text.all_on_null(&t, null);
    ui_edit_doc.replace(e->doc, &e->selection, text, bytes);
    ui_edit_pg_t pg = e->selection.from;
    pg.pn += r.to.pn;
    if (e->selection.from.pn == e->selection.to.pn && r.to.pn == 0) {
        pg.gp = e->selection.from.gp + r.to.gp;
    } else {
        pg.gp = r.to.gp;
    }
    ui_edit_text.dispose(&t);
    return pg;
}

static void ui_edit_paste(ui_edit_t* e, const char* s, int32_t n) {
    if (!e->ro) {
        if (n < 0) { n = (int32_t)strlen(s); }
        ui_edit.erase(e);
        e->selection.a[1] = ui_edit_paste_text(e, s, n);
        e->selection.a[0] = e->selection.a[1];
        if (e->w > 0) { ui_edit_move_caret(e, e->selection.a[1]); }
    }
}

static void ui_edit_clipboard_paste(ui_edit_t* e) {
    if (!e->ro) {
        ui_edit_pg_t pg = e->selection.a[1];
        int32_t bytes = 0;
        ut_clipboard.get_text(null, &bytes);
        if (bytes > 0) {
            char* text = null;
            bool ok = ut_heap.alloc((void**)&text, bytes) == 0;
            swear(ok);
            int32_t r = ut_clipboard.get_text(text, &bytes);
            ut_fatal_if_error(r);
            if (bytes > 0 && text[bytes - 1] == 0) {
                bytes--; // clipboard includes zero terminator
            }
            if (bytes > 0) {
                ui_edit.erase(e);
                pg = ui_edit_paste_text(e, text, bytes);
                ui_edit_move_caret(e, pg);
            }
            ut_heap.free(text);
        }
    }
}

static void ui_edit_prepare_sle(ui_edit_t* e) {
    ui_view_t* v = &e->view;
    swear(e->sle && v->w > 0);
    // shingle line edit is capable of resizing itself to two
    // lines of text (and shrinking back) to avoid horizontal scroll
    int32_t runs = ut_max(1, ut_min(2, ui_edit_paragraph_run_count(e, 0)));
    const ui_ltrb_t insets = ui_view.margins(v, &v->insets);
    int32_t h = insets.top + v->fm->height * runs + insets.bottom;
    fp32_t min_h_em = (fp32_t)h / v->fm->em.h;
    if (v->min_h_em != min_h_em) {
        v->min_h_em = min_h_em;
    }
}

static void ui_edit_insets(ui_edit_t* e) {
    ui_view_t* v = &e->view;
    const ui_ltrb_t insets = ui_view.margins(v, &v->insets);
    e->inside = (ui_ltrb_t){
        .left   = insets.left,
        .top    = insets.top,
        .right  = v->w - insets.right,
        .bottom = v->h - insets.bottom
    };
    const int32_t width = e->edit.w; // previous width
    e->edit.w = e->inside.right  - e->inside.left;
    e->edit.h = e->inside.bottom - e->inside.top;
    if (e->edit.w != width) { ui_edit_invalidate_all_runs(e); }
}

static void ui_edit_measure(ui_view_t* v) { // bottom up
    assert(v->type == ui_view_text);
    ui_edit_t* e = (ui_edit_t*)v;
    if (v->w > 0 && e->sle) { ui_edit_prepare_sle(e); }
    v->w = (int32_t)((fp64_t)v->fm->em.w * (fp64_t)v->min_w_em + 0.5);
    v->h = (int32_t)((fp64_t)v->fm->em.h * (fp64_t)v->min_h_em + 0.5);
    const ui_ltrb_t i = ui_view.margins(v, &v->insets);
    // enforce minimum size - it makes it checking corner cases much simpler
    // and it's hard to edit anything in a smaller area - will result in bad UX
    if (v->w < v->fm->em.w * 4) { v->w = i.left + v->fm->em.w * 4 + i.right; }
    if (v->h < v->fm->height)   { v->h = i.top + v->fm->height + i.bottom; }
}

static void ui_edit_layout(ui_view_t* v) { // top down
    assert(v->type == ui_view_text);
    assert(v->w > 0 && v->h > 0); // could be `if'
    ui_edit_t* e = (ui_edit_t*)v;
    ui_edit_insets(e);
    // fully visible runs
    e->visible_runs = e->h / e->fm->height;
    ui_edit_invalidate_run(e, e->scroll.pn);
    // number of runs in e->scroll.pn may have changed with e->w change
    int32_t runs = ui_edit_paragraph_run_count(e, e->scroll.pn);
    // glyph position in scroll_pn paragraph:
    const ui_edit_pg_t scroll = v->w == 0 ? (ui_edit_pg_t){0, 0} :
                                            ui_edit_scroll_pg(e);
    e->scroll.rn = ui_edit_pg_to_pr(e, scroll).rn;
    assert(0 <= e->scroll.rn && e->scroll.rn < runs); (void)runs;
    if (e->sle) { // single line edit (if changed on the fly):
        e->selection.a[0].pn = 0; // only has single paragraph
        e->selection.a[1].pn = 0;
        // scroll line on top of current cursor position into view
        const ui_edit_run_t* run = ui_edit_paragraph_runs(e, 0, &runs);
        if (runs <= 2 && e->scroll.rn == 1) {
            ui_edit_pg_t top = scroll;
            top.gp = ut_max(0, top.gp - run[e->scroll.rn].glyphs - 1);
            ui_edit_scroll_into_view(e, top);
        }
    }
    ui_edit_scroll_into_view(e, e->selection.a[1]);
    ui_edit_caret_to(e, e->selection.a[1]);
    if (e->focused) {
        // recreate caret because fm->height may have changed
        ui_edit_hide_caret(e);
        ui_edit_destroy_caret(e);
        ui_edit_create_caret(e);
        ui_edit_show_caret(e);
        assert(e->focused);
    }
}

static void ui_edit_paint_selection(ui_edit_t* e, int32_t y, const ui_edit_run_t* r,
        const char* text, int32_t pn, int32_t c0, int32_t c1) {
    uint64_t s0 = ui_edit_range.uint64(e->selection.a[0]);
    uint64_t e0 = ui_edit_range.uint64(e->selection.a[1]);
    if (s0 > e0) {
        uint64_t swap = e0;
        e0 = s0;
        s0 = swap;
    }
    const ui_edit_pg_t pnc0 = {.pn = pn, .gp = c0};
    const ui_edit_pg_t pnc1 = {.pn = pn, .gp = c1};
    uint64_t s1 = ui_edit_range.uint64(pnc0);
    uint64_t e1 = ui_edit_range.uint64(pnc1);
    if (s0 <= e1 && s1 <= e0) {
        uint64_t start = ut_max(s0, s1) - (uint64_t)c0;
        uint64_t end = ut_min(e0, e1) - (uint64_t)c0;
        if (start < end) {
            int32_t fro = (int32_t)start;
            int32_t to  = (int32_t)end;
            int32_t ofs0 = ui_edit_str.gp_to_bp(text, r->bytes, fro);
            int32_t ofs1 = ui_edit_str.gp_to_bp(text, r->bytes, to);
            swear(ofs0 >= 0 && ofs1 >= 0);
            int32_t x0 = ui_edit_text_width(e, text, ofs0);
            int32_t x1 = ui_edit_text_width(e, text, ofs1);
            // selection_color is MSVC dark mode selection color
            // TODO: need light mode selection color tpp
            ui_color_t selection_color = ui_color_rgb(0x26, 0x4F, 0x78); // ui_color_rgb(64, 72, 96);
            if (!e->focused || !ui_app.focused()) {
                selection_color = ui_colors.darken(selection_color, 0.1f);
            }
            const ui_ltrb_t insets = ui_view.margins(&e->view, &e->insets);
            int32_t x = e->x + insets.left;
            ui_gdi.fill(x + x0, y, x1 - x0, e->fm->height, selection_color);
        }
    }
}

static int32_t ui_edit_paint_paragraph(ui_edit_t* e,
        const ui_gdi_ta_t* ta, int32_t x, int32_t y, int32_t pn,
        ui_rect_t rc) {
    static const char* ww = ut_glyph_south_west_arrow_with_hook;
    ui_edit_text_t* dt = &e->doc->text; // document text
    assert(0 <= pn && pn < dt->np);
    const ui_edit_str_t* str = &dt->ps[pn];
    int32_t runs = 0;
    const ui_edit_run_t* run = ui_edit_paragraph_runs(e, pn, &runs);
    for (int32_t j = ui_edit_first_visible_run(e, pn);
                 j < runs && y < e->y + e->inside.bottom; j++) {
//      ut_traceln("[%d.%d] @%d,%d bytes: %d", pn, j, x, y, run[j].bytes);
        if (rc.y - e->fm->height <= y && y < rc.y + rc.h) {
            const char* text = str->u + run[j].bp;
            ui_edit_paint_selection(e, y, &run[j], text, pn,
                                    run[j].gp, run[j].gp + run[j].glyphs);
            ui_gdi.text(ta, x, y, "%.*s", run[j].bytes, text);
            if (j < runs - 1 && !e->hide_word_wrap) {
                ui_gdi.text(ta, x + e->edit.w, y, "%s", ww);
            }
        }
        y += e->fm->height;
    }
    return y;
}

static void ui_edit_paint(ui_view_t* v) {
    assert(v->type == ui_view_text);
    assert(!ui_view.is_hidden(v));
    ui_edit_t* e = (ui_edit_t*)v;
    ui_edit_text_t* dt = &e->doc->text; // document text
    // drawing text is really expensive, only paint what's needed:
    ui_rect_t vrc = (ui_rect_t){v->x, v->y, v->w, v->h};
    ui_rect_t rc;
    if (ui.intersect_rect(&rc, &vrc, &ui_app.prc)) {
        // because last line of the view may extend over the bottom
        ui_gdi.set_clip(v->x, v->y, v->w, v->h);
        ui_gdi.fill(rc.x, rc.y, rc.w, rc.h, v->background);
        const ui_ltrb_t insets = ui_view.margins(v, &v->insets);
        int32_t x = v->x + insets.left;
        int32_t y = v->y + insets.top;
        const ui_gdi_ta_t ta = { .fm = v->fm, .color = v->color };
        const int32_t pn = e->scroll.pn;
        const int32_t bottom = v->y + e->inside.bottom;
        assert(pn < dt->np);
        for (int32_t i = pn; i < dt->np && y < bottom; i++) {
            y = ui_edit_paint_paragraph(e, &ta, x, y, i, rc);
        }
        ui_gdi.set_clip(0, 0, 0, 0);
    }
}

static void ui_edit_move(ui_edit_t* e, ui_edit_pg_t pg) {
    if (e->w > 0) {
        ui_edit_move_caret(e, pg); // may select text on move
    } else {
        e->selection.a[1] = pg;
    }
    e->selection.a[0] = e->selection.a[1];
}

static bool ui_edit_reallocate_runs(ui_edit_t* e, int32_t p, int32_t np) {
    // This function is called in after() callback when
    // d->text.np already changed to `new_np`.
    // It has to manipulate e->para[] array w/o calling
    // ui_edit_invalidate_runs() ui_edit_dispose_all_runs()
    // because they assume that e->para[] array is in sync
    // d->text.np.
    ui_edit_text_t* dt = &e->doc->text; // document text
    bool ok = true;
    int32_t old_np = np;     // old (before) number of paragraphs
    int32_t new_np = dt->np; // new (after)  number of paragraphs
    assert(old_np > 0 && new_np > 0 && e->para != null);
    assert(0 <= p && p < old_np);
    if (old_np == new_np) {
        ui_edit_invalidate_run(e, p);
    } else if (new_np < old_np) { // shrinking - delete runs
        const int32_t d = old_np - new_np; // `d` delta > 0
        if (p + d < old_np - 1) {
            const int32_t n = ut_max(0, old_np - p - d - 1);
            memcpy(e->para + p + 1, e->para + p + 1 + d, n * sizeof(e->para[0]));
        }
        if (p < new_np) { ui_edit_invalidate_run(e, p); }
        ok = ut_heap.realloc((void**)&e->para, new_np * sizeof(e->para[0])) == 0;
        swear(ok, "shrinking");
    } else { // growing - insert runs
        ui_edit_invalidate_run(e, p);
        int32_t d = new_np - old_np;  // `d` delta > 0
        ok = ut_heap.realloc_zero((void**)&e->para, new_np * sizeof(e->para[0])) == 0;
        if (ok) {
            const int32_t n = ut_max(0, new_np - p - d - 1);
            memmove(e->para + p + 1 + d, e->para + p + 1,
                    (size_t)n * sizeof(e->para[0]));
            const int32_t m = ut_min(new_np, p + 1 + d);
            for (int32_t i = p + 1; i < m; i++) {
                e->para[i].run = null;
                e->para[i].runs = 0;
            }
        }
    }
    return ok;
}

static void ui_edit_before(ui_edit_notify_t* notify,
         const ui_edit_notify_info_t* ni) {
    ui_edit_notify_view_t* n = (ui_edit_notify_view_t*)notify;
    ui_edit_t* e = (ui_edit_t*)n->that;
    swear(e->doc == ni->d);
    if (e->w > 0 && e->h > 0) {
        const ui_edit_text_t* dt = &e->doc->text; // document text
        assert(dt->np > 0);
        // `n->data` is number of paragraphs before replace():
        n->data = (uintptr_t)dt->np;
        if (e->selection.from.pn != e->selection.to.pn) {
            ui_edit_invalidate_view(e);
        } else {
            ui_edit_invalidate_rect(e, ui_edit_selection_rect(e));
        }
    }
}

static void ui_edit_after(ui_edit_notify_t* notify,
         const ui_edit_notify_info_t* ni) {
    ui_edit_notify_view_t* n = (ui_edit_notify_view_t*)notify;
    ui_edit_t* e = (ui_edit_t*)n->that;
    const ui_edit_text_t* dt = &ni->d->text; // document text
    assert(ni->d == e->doc && dt->np > 0);
    if (e->w > 0 && e->h > 0) {
        // number of paragraphs before replace():
        const int32_t np = (int32_t)n->data;
        swear(dt->np == np - ni->deleted + ni->inserted);
        ui_edit_reallocate_runs(e, ni->r->from.pn, np);
        e->selection = *ni->x;
        // this is needed by undo/redo: trim selection
        ui_edit_pg_t* pg = e->selection.a;
        for (int32_t i = 0; i < ut_count_of(e->selection.a); i++) {
            pg[i].pn = ut_max(0, ut_min(dt->np - 1, pg[i].pn));
            pg[i].gp = ut_max(0, ut_min(dt->ps[pg[i].pn].g, pg[i].gp));
        }
        if (ni->r->from.pn != ni->r->to.pn &&
            ni->x->from.pn != ni->x->to.pn &&
            ni->r->from.pn == ni->x->from.pn) {
            ui_edit_invalidate_rect(e, ui_edit_selection_rect(e));
        } else {
            ui_edit_invalidate_view(e);
        }
        ui_edit_scroll_into_view(e, e->selection.to);
    }
}

static void ui_edit_init(ui_edit_t* e, ui_edit_doc_t* d) {
    memset(e, 0, sizeof(*e));
    assert(d != null && d->text.np > 0);
    e->doc = d;
    assert(d->text.np > 0);
    e->listener.that = (void*)e;
    e->listener.data = 0;
    e->listener.notify.before = ui_edit_before;
    e->listener.notify.after  = ui_edit_after;
    ut_static_assertion(offsetof(ui_edit_notify_view_t, notify) == 0);
    ui_edit_doc.subscribe(d, &e->listener.notify);
    e->color_id = ui_color_id_window_text;
    e->background_id = ui_color_id_window;
    e->fm = &ui_app.fm.regular;
    e->insets  = (ui_margins_t){ 0.25, 0.25, 0.50, 0.25 };
    e->padding = (ui_margins_t){ 0.25, 0.25, 0.25, 0.25 };
    e->min_w_em = 1.0;
    e->min_h_em = 1.0;
    e->type = ui_view_text;
    e->focusable = true;
    e->last_x    = -1;
    e->focused   = false;
    e->sle       = false;
    e->ro        = false;
    e->caret        = (ui_point_t){-1, -1};
    e->paint        = ui_edit_paint;
    e->measure      = ui_edit_measure;
    e->layout       = ui_edit_layout;
    e->tap          = ui_edit_tap;
    e->long_press   = ui_edit_long_press;
    e->double_tap   = ui_edit_double_tap;
    e->character    = ui_edit_character;
    e->focus_gained = ui_edit_focus_gained;
    e->focus_lost   = ui_edit_focus_lost;
    e->key_pressed  = ui_edit_key_pressed;
    e->mouse_scroll = ui_edit_mouse_scroll;
    ui_edit_allocate_runs(e);
    if (e->debug.id == null) { e->debug.id = "#edit"; }
}

static void ui_edit_dispose(ui_edit_t* e) {
    ui_edit_doc.unsubscribe(e->doc, &e->listener.notify);
    ui_edit_dispose_all_runs(e);
    memset(e, 0, sizeof(*e));
}

ui_edit_if ui_edit = {
    .init                 = ui_edit_init,
    .set_font             = ui_edit_set_font,
    .move                 = ui_edit_move,
    .paste                = ui_edit_paste,
    .save                 = ui_edit_save,
    .erase                = ui_edit_erase,
    .cut_to_clipboard     = ui_edit_clipboard_cut,
    .copy_to_clipboard    = ui_edit_clipboard_copy,
    .paste_from_clipboard = ui_edit_clipboard_paste,
    .select_all           = ui_edit_select_all,
    .key_down             = ui_edit_key_down,
    .key_up               = ui_edit_key_up,
    .key_left             = ui_edit_key_left,
    .key_right            = ui_edit_key_right,
    .key_page_up          = ui_edit_key_page_up,
    .key_page_down        = ui_edit_key_page_down,
    .key_home             = ui_edit_key_home,
    .key_end              = ui_edit_key_end,
    .key_delete           = ui_edit_key_delete,
    .key_backspace        = ui_edit_key_backspace,
    .key_enter            = ui_edit_key_enter,
    .fuzz                 = null,
    .dispose              = ui_edit_dispose
};
