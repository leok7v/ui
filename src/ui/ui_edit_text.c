/* Copyright (c) Dmitry "Leo" Kuznetsov 2021-24 see LICENSE for details */
#include "ut/ut.h"
#include "ui/ui.h"

static bool ui_edit_debug_dump;

typedef struct ui_edit_doc_s ui_edit_doc_t;

typedef struct ui_edit_notify_s ui_edit_notify_t;

typedef struct ui_edit_to_do_s ui_edit_to_do_t;

typedef struct ut_begin_packed ui_edit_paragraph_s { // "paragraph"
    ui_str_t str;
    int32_t runs;       // number of runs in this paragraph
    ui_edit_run_t* run; // [runs] array of pointers (heap)
} ut_end_packed ui_edit_paragraph_t;

typedef struct ui_edit_notify_s {
    void (*before)(ui_edit_notify_t* notify,
                   const ui_edit_doc_t* d, const ui_edit_range_t* range);
    void (*after)(ui_edit_notify_t* notify,
                  const ui_edit_doc_t* d, const ui_edit_range_t* range);
} ui_edit_notify_t;

typedef struct ui_edit_observer_s ui_edit_observer_t;

typedef struct ui_edit_observer_s {
    ui_edit_notify_t* notify;
    ui_edit_observer_t* prev;
    ui_edit_observer_t* next;
} ui_edit_observer_t;

typedef struct ui_edit_text_s {
    int32_t paragraphs;         // number of paragraphs
    ui_edit_paragraph_t* para;  // para[paragraphs]
} ui_edit_text_t;

typedef struct ui_edit_to_do_s { // undo/redo action
    ui_edit_range_t  range;
    ui_edit_text_t   text;
    ui_edit_to_do_t* next; // inside undo or redo list
} ui_edit_to_do_t;

typedef struct ui_edit_doc_s {
    // replaces the range with text and pushes
    // undo ui_edit_to_do_t struct into undo/redo stack
    // possibly coalescing it with previous replace
    ui_edit_text_t   text;
    ui_edit_to_do_t* undo; // undo stack
    ui_edit_to_do_t* redo; // redo stack
    ui_edit_observer_t* observers;
} ui_edit_doc_t;

typedef struct ui_edit_text_if {
    bool    (*init)(ui_edit_text_t *t, const uint8_t* s, int32_t b, bool heap);
    int32_t (*bytes)(const ui_edit_text_t* t, const ui_edit_range_t* r);
    void    (*free)(ui_edit_text_t* t);
} ui_edit_text_if;

ui_edit_text_if ui_edit_text;

typedef struct ui_edit_doc_if {
    bool (*init)(ui_edit_doc_t* d);
    bool (*replace)(ui_edit_doc_t* d,
                    const ui_edit_range_t* range,
                    const uint8_t* u, int32_t b,
                    ui_edit_to_do_t* undo);
    int32_t (*bytes)(const ui_edit_doc_t* d, const ui_edit_range_t* range);
    bool    (*copy)(ui_edit_doc_t* d, const ui_edit_range_t* range,
                    ui_edit_text_t* text); // retrieves range into string
    // redo() performs the operation pushes it into stack
    void (*redo)(ui_edit_doc_t* d);
    // undo() pops operation from stack and undo it
    void (*undo)(ui_edit_doc_t* d);
    void (*subscribe)(ui_edit_doc_t* d, ui_edit_notify_t* notify);
    void (*unsubscribe)(ui_edit_doc_t* d, ui_edit_notify_t* notify);
    void (*dispose)(ui_edit_doc_t* d);
    void (*test)(void);
} ui_edit_doc_if;

ui_edit_doc_if ui_edit_doc;

static void ui_edit_pg_dump(const ui_edit_pg_t* pg) {
    traceln("pn:%d gp:%d", pg->pn, pg->gp);
}

static void ui_edit_range_dump(const ui_edit_range_t* r) {
    traceln("from {pn:%d gp:%d} to {pn:%d gp:%d}",
            r->from.pn, r->from.gp, r->to.pn, r->to.gp);
}

static void ui_edit_text_dump(const ui_edit_text_t* t) {
    for (int32_t i = 0; i < t->paragraphs; i++) {
        const ui_edit_paragraph_t* p = &t->para[i];
        traceln("para[%d].%d: %.*s", i, p->str.b, p->str.b, p->str.u);
    }
}

static void ui_edit_doc_dump(const ui_edit_doc_t* d) {
    for (int32_t i = 0; i < d->text.paragraphs; i++) {
        const ui_edit_paragraph_t* p = &d->text.para[i];
        traceln("para[%d].%d: %.*s", i, p->str.b, p->str.b, p->str.u);
    }
    // TODO: undo/redo stacks and observers
}

#pragma push_macro("ui_edit_check_zeros")
#pragma push_macro("ui_edit_check_pg_inside_text")
#pragma push_macro("ui_edit_check_range_inside_text")

#ifdef DEBUG

// ui_edit_check_zeros only works for packed structs:

#define ui_edit_check_zeros(a_, b_) do {                                    \
    for (int32_t i_ = 0; i_ < (b_); i_++) {                                 \
        assert(((const uint8_t*)(a_))[i_] == 0x00);                         \
    }                                                                       \
} while (0)

#define ui_edit_check_pg_inside_text(t_, pg_)                               \
    assert(0 <= (pg_)->pn && (pg_)->pn < (t_)->paragraphs &&                \
           0 <= (pg_)->gp && (pg_)->gp <= (t_)->para[(pg_)->pn].str.g)

#define ui_edit_check_range_inside_text(t_, r_) do {                        \
    assert((r_)->from.pn <= (r_)->to.pn);                                   \
    assert((r_)->from.pn <  (r_)->to.pn || (r_)->from.gp <= (r_)->to.gp);   \
    ui_edit_check_pg_inside_text(t_, (&(r_)->from));                        \
    ui_edit_check_pg_inside_text(t_, (&(r_)->to));                          \
} while (0)

#else

#define ui_edit_check_zeros(a, b)             do { } while (0)
#define ui_edit_check_pg_inside_text(t, pg)   do { } while (0)
#define ui_edit_check_range_inside_text(t, r) do { } while (0)

#endif

static ui_edit_range_t ui_edit_doc_range_or_all(const ui_edit_text_t* t,
        const ui_edit_range_t* range) {
    ui_edit_range_t r;
    if (range != null) {
        r = *range;
    } else {
        assert(t->paragraphs >= 1);
        r.from.pn = 0;
        r.from.gp = 0;
        r.to.pn = t->paragraphs - 1;
        r.to.gp = t->para[r.to.pn].str.g;
    }
    return r;
}

static int ui_edit_doc_compare_pg(ui_edit_pg_t pg1, ui_edit_pg_t pg2) {
    int64_t d = (((int64_t)pg1.pn << 32) | pg1.gp) -
                (((int64_t)pg2.pn << 32) | pg2.gp);
    return d < 0 ? -1 : d > 0 ? 1 : 0;
}

static ui_edit_range_t ui_edit_doc_ordered_range(ui_edit_range_t r) {
    uint64_t f = ((uint64_t)r.from.pn << 32) | r.from.gp;
    uint64_t t = ((uint64_t)r.to.pn   << 32) | r.to.gp;
    if (ui_edit_doc_compare_pg(r.from, r.to) > 0) {
        uint64_t swap = t; t = f; f = swap;
        r.from.pn = (int32_t)(f >> 32);
        r.from.gp = (int32_t)(f);
        r.to.pn   = (int32_t)(t >> 32);
        r.to.gp   = (int32_t)(t);
    }
    return r;
}

static bool ui_edit_doc_realloc_paragraphs(ui_edit_paragraph_t* *para,
        int32_t old_np, int32_t new_np) {
    ui_edit_paragraph_t* p = *para;
    for (int32_t i = new_np; i < old_np; i++) { ui_str.free(&p[i].str); }
    bool ok = true;
    if (new_np == 0) {
        ut_heap.free(p);
        *para = null;
    } else {
        ok = ut_heap.realloc_zero((void**)&p, new_np *
                                  sizeof(ui_edit_paragraph_t)) == 0;
        if (ok) {
            for (int32_t i = old_np; i < new_np; i++) {
                ok = ui_str.init(&p[i].str, null, 0, false);
                swear(ok, "because .init(\"\", 0) does NOT allocate memory");
            }
            *para = p;
        }
    }
    return ok;
}

static bool ui_edit_text_init(ui_edit_text_t *t,
        const uint8_t* s, int32_t b, bool heap) {
    // When text comes from the source that lifetime is shorter
    // than text itself (e.g. paste from clipboard) the parameter
    // heap: true allows to make a copy of data on the heap
    if (b < 0) { b = (int32_t)strlen((const char*)s); }
    // if caller is concerned with best performance - it should pass b >= 0
    int32_t ps = 0; // number of paragraphs
    int32_t np = ut_max(b / 64, 2); // initial number of allocated paragraphs
    ui_edit_paragraph_t* para = null;
    bool ok = ui_edit_doc_realloc_paragraphs(&para, 0, np);
    if (ok) {
        bool lf = false;
        int32_t i = 0;
        while (ok && i < b) {
            int32_t k = i;
            while (k < b && s[k] != '\n') { k++; }
            lf = k < b && s[k] == '\n';
            if (ps >= np) {
                int32_t np1_5 = np * 3 / 2; // np * 1.5
                assert(np1_5 > np);
                ok = ui_edit_doc_realloc_paragraphs(&para, np, np1_5);
                if (ok) { np = np1_5; }
            }
            if (ok) {
                // insider knowledge about ui_str allocation behaviour:
                assert(para[ps].str.c == 0 && para[ps].str.b == 0 &&
                       para[ps].str.g2b[0] == 0);
                ui_str.free(&para[ps].str);
                // str.init may allocate str.g2b[] on the heap and may fail
                const int32_t bytes = k - i; assert(bytes >= 0);
                const uint8_t* u = bytes == 0 ? null : s + i;
                ok = ui_str.init(&para[ps].str, u, bytes, heap && bytes > 0);
                if (ok) { ps++; }
            }
            i = k + lf;
        }
        if (ok && lf) { // last paragraph ended with line feed
            if (ps + 1 >= np) {
                ok = ui_edit_doc_realloc_paragraphs(&para, np, np + 1);
                if (ok) { np = np + 1; }
            }
            if (ok) { ps++; }
        }
    }
    if (ok && ps == 0) { // special case empty string to a single paragraph
        assert(b <= 0 && (b == 0 || s[0] == 0x00));
        ps = 1; // para[0].str is already initialized as empty str
        ok = ui_edit_doc_realloc_paragraphs(&para, np, 1);
        swear(ok, "shrinking para[] above");
    }
    if (ok) {
        assert(ps > 0);
        t->paragraphs = ps;
        t->para = para;
    } else if (para != null) {
        ok = ui_edit_doc_realloc_paragraphs(&para, np, 0); // free()
        swear(ok);
        ut_heap.free(para);
        t->paragraphs = 0;
        t->para = null;
    }
    return ok;
}

static void ui_edit_text_free(ui_edit_text_t* t) {
    if (t->paragraphs != 0) {
        ui_edit_doc_realloc_paragraphs(&t->para, t->paragraphs, 0);
        ut_heap.free(t->para);
        t->paragraphs = 0;
        t->para = null;
    } else {
        assert(t->paragraphs == 0);
        assert(t->para == null);
    }
}

static int32_t ui_edit_text_bytes(const ui_edit_text_t* t,
        const ui_edit_range_t* range) {
    const ui_edit_range_t r =
        ui_edit_doc_ordered_range(ui_edit_doc_range_or_all(t, range));
    ui_edit_check_range_inside_text(t, &r);
    int32_t bytes = 0;
    for (int32_t pn = r.from.pn; pn <= r.to.pn; pn++) {
        const ui_edit_paragraph_t* p = &t->para[pn];
        if (pn == r.from.pn && pn == r.to.pn) {
            bytes += p->str.g2b[r.to.gp] - p->str.g2b[r.from.gp];
        } else if (pn == r.from.pn) {
            bytes += p->str.b - p->str.g2b[r.from.gp];
        } else if (pn == r.to.pn) {
            bytes += p->str.g2b[r.to.gp];
        } else {
            bytes += p->str.b;
        }
    }
    return bytes;
}

static int32_t ui_edit_doc_bytes(const ui_edit_doc_t* d,
        const ui_edit_range_t* r) {
    return ui_edit_text.bytes(&d->text, r);
}

static bool ui_edit_doc_remove(ui_edit_doc_t* d,
        const ui_edit_range_t* range) {
    ui_edit_range_t r =
        ui_edit_doc_ordered_range(ui_edit_doc_range_or_all(&d->text, range));
    ui_edit_check_range_inside_text(&d->text, &r);
    // first paragraph:
    ui_edit_paragraph_t* p0 = &d->text.para[r.from.pn];
    ui_edit_paragraph_t* p1 = &d->text.para[r.to.pn];
    bool ok = // move first and last paragraphs to the heap
        ui_str.expand(&p0->str, p0->str.b) ||
        ui_str.expand(&p1->str, p1->str.b); // if p0 == p1 this is no-op
    if (ok) {
        ui_str_t merge = {0}; // merge of p0 and p1
        if (p0 != p1) {
            ok = ui_str.init(&merge, p0->str.u, p0->str.g2b[r.from.gp], true); // allocate on heap
            if (ok) {
                ok = ui_str.replace(&merge, merge.g, merge.g,
                                    p1->str.u + p1->str.g2b[r.to.gp],
                                    p1->str.b - p1->str.g2b[r.to.gp]);
            }
        }
        if (ok) {
            if (p1 != p0) {
                int32_t deleted = 0;
                for (int32_t pn = r.from.pn + 1; pn <= r.to.pn; pn++) {
                    ui_str.free(&d->text.para[pn].str);
                    deleted++;
                }
                ui_str.free(&d->text.para[r.from.pn].str);
                d->text.para[r.from.pn].str = merge;
                // remove deleted paragraphs:
                if (deleted > 0 && d->text.paragraphs - r.to.pn - 1 > 0) {
                    memcpy(&d->text.para[r.from.pn + 1],
                           &d->text.para[r.from.pn + 1 + deleted],
                           (d->text.paragraphs - r.to.pn - 1) *
                            sizeof(ui_edit_paragraph_t));
                }
                d->text.paragraphs -= deleted;
            } else {
                ok = ui_str.replace(&p0->str, r.from.gp, r.to.gp, null, 0);
            }
        } else if (merge.c > 0 || merge.g > 0) {
            ui_str.free(&merge);
        }
        assert(d->text.paragraphs >= 1);
    }
    return ok;
}

static bool ui_edit_doc_insert(ui_edit_doc_t* d,
        const ui_edit_pg_t* i, // insertion point
        const ui_edit_text_t* ins) {
    ui_edit_check_pg_inside_text(&d->text, i);
    bool ok = true;
    int32_t np = d->text.paragraphs + ins->paragraphs - 1;
    ui_edit_paragraph_t* para = null;
    ok = ui_edit_doc_realloc_paragraphs(&para, 0, np);
    if (ok) {
        // para[] will hold pointers to the original strings
        // from the document and copies of text->para[] strings:
        for (int32_t pn = 0; pn < np; pn++) { ui_str.free(&para[pn].str); }
        const ui_str_t* f = null; // first string "ins" is inserted
        for (int32_t pn = 0; ok && pn < np; pn++) {
            const ui_str_t* s = &d->text.para[pn].str;
            if (pn < i->pn) {
                para[pn].str = *s;
            } else if (i->pn <= pn && pn < i->pn + ins->paragraphs) {
                // insert `ins` into `i` insertion point
                if (pn == i->pn) {
                    ok = ui_str.init(&para[pn].str, s->u, s->b, true);
                    if (ok) {
                        assert(0 <= i->gp && i->gp <= s->g);
                        // if there is continuation paragraph cut the tail
                        // it will be reinserted below as `f`
                        const int32_t to = ins->paragraphs > 1 ? s->g : i->gp;
                        ok = ui_str.replace(&para[pn].str, i->gp, to,
                            ins->para[0].str.u, ins->para[0].str.b);
                        f = s;
                    }
                } else if (ins->paragraphs > 1 &&
                    pn == i->pn + ins->paragraphs - 1) {
                    const ui_str_t* e = // last string of text
                        &ins->para[ins->paragraphs - 1].str;
                    assert(f != null);
                    ok = ui_str.init(&para[pn].str, f->u, f->b, true);
                    if (ok) {
                        ok = ui_str.replace(&para[pn].str, 0, i->gp,
                                            e->u, e->b);
                    }
                } else { // not first not last paragraph of the `ins` text
                    const ui_str_t* p = // string of text
                        &d->text.para[pn - ins->paragraphs - 1].str;
                    ok = ui_str.init(&para[pn].str, p->u, p->b, true);
                }
            }
        }
        if (!ok) { // free allocated strings in partial result
            for (int32_t pn = 0; pn < np; pn++) {
                if (i->pn < pn && pn < i->pn + ins->paragraphs) {
                    // not all strings in this range actually initialized
                    if (para[pn].str.c > 0 || para[pn].str.g > 0) {
                        ui_str.free(&para[pn].str);
                    }
                }
            }
        }
        if (ok) {
            ut_heap.free(d->text.para); // free old paragraphs
            d->text.para = para;
            d->text.paragraphs = np;
        }
    }
    return ok;
}

static bool ui_edit_doc_replace_with_text(ui_edit_doc_t* d,
        const ui_edit_range_t* range, const ui_edit_text_t* text,
        ui_edit_to_do_t* undo) {
    ui_edit_range_t r =
        ui_edit_doc_ordered_range(ui_edit_doc_range_or_all(&d->text, range));
    if (ui_edit_debug_dump) {
        traceln(">");
        traceln("doc:");
        ui_edit_doc_dump(d);
        traceln("");
        traceln("range: ");
        ui_edit_range_dump(&r);
        traceln("");
        traceln("text: ");
        ui_edit_text_dump(text);
        traceln("");
    }
    ui_edit_check_range_inside_text(&d->text, &r);
    bool empty_range = ui_edit_doc_compare_pg(r.from, r.to) == 0;
    bool ok = ui_edit_doc.copy(d, &r, &undo->text);
    if (ui_edit_debug_dump) {
        traceln("copy: (undo->text)");
        ui_edit_text_dump(&undo->text);
        traceln("");
    }
    if (ok) {
        if (empty_range) {
            ok = ui_edit_doc_insert(d, &r.from, text);
        } else {
            ok = ui_edit_doc_insert(d, &r.to, text);
            if (ok) {
                ui_edit_range_t inserted = { .from = r.to, .to = r.to };
                inserted.to.pn += text->paragraphs - 1;
                inserted.to.gp  = text->para[text->paragraphs - 1].str.g;
                ui_edit_check_range_inside_text(&d->text, &inserted);
                if (!ui_edit_doc_remove(d, &r)) {
                    // remove inserted text:
                    (void)ui_edit_doc_remove(d, &inserted);
                }
            }
        }
        if (ok) {
            undo->range.from  = r.from;
            undo->range.to.pn = r.from.pn + text->paragraphs - 1;
            if (undo->range.from.pn == undo->range.to.pn) {
                undo->range.to.gp = undo->range.from.gp +
                                    text->para[text->paragraphs - 1].str.g;
            } else {
                undo->range.to.gp = text->para[text->paragraphs - 1].str.g;
            }
            if (ui_edit_debug_dump) {
                traceln("doc:");
                ui_edit_doc_dump(d);
                traceln("undo: ");
                ui_edit_range_dump(&undo->range);
                ui_edit_text_dump(&undo->text);
            }
            ui_edit_check_range_inside_text(&d->text, &undo->range);
        }
    }
    if (!ok && undo->text.paragraphs > 0) {
        ui_edit_text.free(&undo->text);
    }
    if (ui_edit_debug_dump) {
        traceln("<");
    }
    return ok;
}

static bool ui_edit_doc_subscribe(ui_edit_doc_t* t, ui_edit_notify_t* notify) {
    // TODO: not sure about double linked list.
    // heap allocated resizable array may serve better and may be easier to maintain
    bool ok = true;
    ui_edit_observer_t* o = t->observers;
    if (o == null) {
        ok = ut_heap.alloc_zero((void**)&t->observers, sizeof(*o)) == 0;
        if (ok) { o = t->observers; }
    } else {
        while (o->next != null) { swear(o->notify != notify); o = o->next; }
        ok = ut_heap.alloc_zero((void**)&o->next, sizeof(*o)) == 0;
        if (ok) { o->next->prev = o; o = o->next; }
    }
    if (ok) { o->notify = notify; }
    return ok;
}

static void ui_edit_doc_unsubscribe(ui_edit_doc_t* t, ui_edit_notify_t* notify) {
    ui_edit_observer_t* o = t->observers;
    bool removed = false;
    while (o != null) {
        ui_edit_observer_t* n = o->next;
        if (o->notify == notify) {
            assert(!removed);
            if (o->prev != null) { o->prev->next = n; }
            if (o->next != null) { o->next->prev = o->prev; }
            if (o == t->observers) { t->observers = n; }
            ut_heap.free(o);
            removed = true;
        }
        o = n;
    }
    swear(removed);
}

static bool ui_edit_doc_copy(const ui_edit_doc_t* d,
        const ui_edit_range_t* range, ui_edit_text_t* t) {
    ui_edit_check_zeros(t, sizeof(*t));
    memset(t, 0x00, sizeof(*t));
    ui_edit_range_t r =
        ui_edit_doc_ordered_range(ui_edit_doc_range_or_all(&d->text, range));
    ui_edit_check_range_inside_text(&d->text, &r);
    int32_t np = r.to.pn - r.from.pn + 1;
    bool ok = ui_edit_doc_realloc_paragraphs(&t->para, 0, np);
    if (ok) { t->paragraphs = np; }
    for (int32_t pn = r.from.pn; ok && pn <= r.to.pn; pn++) {
        const ui_edit_paragraph_t* p = &d->text.para[pn];
        const uint8_t* u = p->str.u;
        int32_t bytes = 0;
        if (pn == r.from.pn && pn == r.to.pn) {
            bytes = p->str.g2b[r.to.gp] - p->str.g2b[r.from.gp];
            u += p->str.g2b[r.from.gp];
        } else if (pn == r.from.pn) {
            bytes = p->str.b - p->str.g2b[r.from.gp];
            u += p->str.g2b[r.from.gp];
        } else if (pn == r.to.pn) {
            bytes = p->str.g2b[r.to.gp];
        } else {
            bytes = p->str.b;
        }
        ui_str.free(&t->para[pn - r.from.pn].str);
        ok = ui_str.init(&t->para[pn - r.from.pn].str,
                          bytes == 0 ? null : u, bytes, true);
    }
    if (!ok) {
        ui_edit_text.free(t);
        ui_edit_check_zeros(t, sizeof(*t));
    }
    return ok;
}

static bool ui_edit_doc_replace(ui_edit_doc_t* d,
        const ui_edit_range_t* r, const uint8_t* u, int32_t b,
        ui_edit_to_do_t* undo) {
    ui_edit_text_t t = {0};
    bool ok = ui_edit_text.init(&t, u, b, false);
    if (ok) {
        ok = ui_edit_doc_replace_with_text(d, r, &t, undo);
        ui_edit_text.free(&t);
    }
    return ok;
}

static bool ui_edit_doc_do(ui_edit_doc_t* d, ui_edit_to_do_t* to_do,
        ui_edit_to_do_t* *stack) {
    const ui_edit_range_t* r = &to_do->range;
    ui_edit_to_do_t* redo = null;
    bool ok = ut_heap.alloc_zero((void**)&redo, sizeof(ui_edit_to_do_t));
    if (ok) {
        ok = ui_edit_doc_replace_with_text(d, r, &to_do->text, redo);
        if (ok) {
            ui_edit_text.free(&to_do->text);
            ut_heap.free(to_do);
        }
        if (ok) {
            redo->next = *stack;
            *stack = redo;
        } else {
            if (redo->text.paragraphs > 0) {
                ui_edit_text.free(&redo->text);
            }
            ut_heap.free(redo);
        }
    }
    return ok;
}

static void ui_edit_doc_redo(ui_edit_doc_t* d) {
    ui_edit_to_do_t* to_do = d->redo;
    if (to_do != null) {
        d->redo = d->redo->next;
        ui_edit_doc_do(d, to_do, &d->undo);
    }
}

static void ui_edit_doc_undo(ui_edit_doc_t* d) {
    ui_edit_to_do_t* to_do = d->undo;
    if (to_do != null) {
        d->undo = d->undo->next;
        ui_edit_doc_do(d, to_do, &d->redo);
    }
}

static void ui_edit_doc_test(void);

static bool ui_edit_doc_init(ui_edit_doc_t* d) {
    bool ok = true;
    ui_edit_check_zeros(d, sizeof(*d));
    memset(d, 0x00, sizeof(d));
    ok = ut_heap.alloc_zero((void**)&d->text.para, sizeof(ui_edit_paragraph_t)) == 0;
    if (ok) {
        d->text.paragraphs = 1;
        ok = ui_str.init(&d->text.para[0].str, null, 0, false);
    }
    return ok;
}

static void ui_edit_doc_dispose(ui_edit_doc_t* d) {
    for (int32_t i = 0; i < d->text.paragraphs; i++) {
        ui_str.free(&d->text.para[i].str);
    }
    if (d->text.para != null) {
        ut_heap.free(d->text.para);
        d->text.para = null;
    }
    d->text.paragraphs  = 0;
    ui_edit_check_zeros(d, sizeof(*d));
}

static void ui_edit_doc_test_big_text(void) {
    enum { MB10 = 10 * 1000 * 1000 };
    uint8_t* text = null;
    ut_heap.alloc(&text, MB10);
    memset(text, 'a', MB10 - 1);
    uint8_t* p = text;
    uint32_t seed = 0x1;
    for (;;) {
        int32_t n = ut_num.random32(&seed) % 40 + 40;
        if (p + n >= text + MB10) { break; }
        p += n;
        *p = '\n';
    }
    text[MB10 - 1] = 0x00;
//  fp64_t time = ut_clock.seconds();
    ui_edit_text_t t = {0};
    bool ok = ui_edit_text.init(&t, text, MB10, false);
    swear(ok);
    ui_edit_text.free(&t);
//  time = ut_clock.seconds() - time;
//  traceln("10MB of text split into %d paragraphs in %.3f seconds",
//          paragraphs, time);
    // release: 10MB of text split into 168,108 paragraphs in 0.130 seconds
    ut_heap.free(text);
}

static void ui_edit_doc_test_paragraphs(void) {
    // ui_edit_doc_to_paragraphs() is about 1 microsecond
    for (int i = 0; i < 100; i++)
    {
        {   // empty string to paragraphs:
            ui_edit_text_t t = {0};
            bool ok = ui_edit_text.init(&t, null, 0, false);
            swear(ok);
            swear(t.para != null && t.paragraphs == 1);
            swear(t.para[0].str.u[0] == 0 &&
                  t.para[0].str.c == 0);
            swear(t.para[0].str.b == 0 &&
                  t.para[0].str.g == 0);
            ui_edit_text.free(&t);
        }
        {   // string without "\n"
            const uint8_t* hello = (const uint8_t*)"hello";
            const int32_t n = (int32_t)strlen((const char*)hello);
            ui_edit_text_t t = {0};
            bool ok = ui_edit_text.init(&t, hello, n, false);
            swear(ok);
            swear(t.para != null && t.paragraphs == 1);
            swear(t.para[0].str.u == hello);
            swear(t.para[0].str.c == 0);
            swear(t.para[0].str.b == n);
            swear(t.para[0].str.g == n);
            ui_edit_text.free(&t);
        }
        {   // string with "\n" at the end
            const uint8_t* hello = (const uint8_t*)"hello\n";
            ui_edit_text_t t = {0};
            bool ok = ui_edit_text.init(&t, hello, -1, false);
            swear(ok);
            swear(t.para != null && t.paragraphs == 2);
            swear(t.para[0].str.u == hello);
            swear(t.para[0].str.c == 0);
            swear(t.para[0].str.b == 5);
            swear(t.para[0].str.g == 5);
            swear(t.para[1].str.u[0] == 0x00);
            swear(t.para[0].str.c == 0);
            swear(t.para[1].str.b == 0);
            swear(t.para[1].str.g == 0);
            ui_edit_text.free(&t);
        }
        {   // two string separated by "\n"
            const uint8_t* hello = (const uint8_t*)"hello\nworld";
            const uint8_t* world = hello + 6;
            ui_edit_text_t t = {0};
            bool ok = ui_edit_text.init(&t, hello, -1, false);
            swear(ok);
            swear(t.para != null && t.paragraphs == 2);
            swear(t.para[0].str.u == hello);
            swear(t.para[0].str.c == 0);
            swear(t.para[0].str.b == 5);
            swear(t.para[0].str.g == 5);
            swear(t.para[1].str.u == world);
            swear(t.para[0].str.c == 0);
            swear(t.para[1].str.b == 5);
            swear(t.para[1].str.g == 5);
            ui_edit_text.free(&t);
        }
    }
    for (int i = 0; i < 10; i++) {
        ui_edit_doc_test_big_text();
    }
    traceln("done");
}

static void ui_edit_doc_test(void) {
    {
        ui_edit_range_t r = { .from = {0,0}, .to = {0,0} };
        static_assertion(sizeof(r.from) + sizeof(r.from) == sizeof(r.a));
        swear(&r.from == &r.a[0] && &r.to == &r.a[1]);
    }
    #ifdef UI_EDIT_DOC_TEST_PARAGRAPHS
        ui_edit_doc_test_paragraphs();
    #else
        (void)(void*)ui_edit_doc_test_paragraphs; // unused
    #endif
    ui_edit_doc_t edit_doc = {0};
    ui_edit_doc_t* d = &edit_doc;
    ui_edit_notify_t notify1 = {0};
    ui_edit_notify_t notify2 = {0};
    bool ok = ui_edit_doc.init(d);
    swear(ok);
    ui_edit_doc.subscribe(d, &notify1);
    ui_edit_doc.subscribe(d, &notify2);
    swear(ui_edit_doc.bytes(d, null) == 0, "expected empty");

    // TODO: These two test cases use insider knowledge
    //       and implemented before ui_edit_doc.replace().
    //       The benefit of keeping them is to ensure that
    //       internal representation of document did not change.
    {   // two string separated by "\n"
        const uint8_t* hello = (const uint8_t*)"hello\nworld";
        ok = ui_edit_text.init(&d->text, hello, -1, false);
        swear(ok);
        ui_edit_text_t t = {0};
        ok = ui_edit_doc.copy(d, null, &t);
        swear(t.paragraphs == 2);
        swear(t.para[0].str.b == 5);
        swear(t.para[0].str.g == 5);
        swear(memcmp(t.para[0].str.u, "hello", 5) == 0);
        swear(t.para[1].str.b == 5);
        swear(t.para[1].str.g == 5);
        swear(memcmp(t.para[1].str.u, "world", 5) == 0);
        ui_edit_text.free(&t);
        ui_edit_text.free(&d->text);
    }
    // TODO: "GoodbyeCruelUniverse" insert 2x"\n" splitting in 3 paragraphs
    {   // two string separated by "\n"
        const uint8_t* s = (const uint8_t*)"Goodbye" "\n" "Cruel" "\n" "Universe";
        ok = ui_edit_text.init(&d->text, s, -1, false);
        swear(ok);
        ui_edit_text_t t = {0};
        ok = ui_edit_doc.copy(d, null, &t);
        ui_edit_text.free(&t);
        ui_edit_range_t r = { .from = {.pn = 0, .gp = 4},
                              .to   = {.pn = 2, .gp = 3} };
        ok = ui_edit_doc_remove(d, &r);
        swear(ok);
        swear(d->text.paragraphs == 1);
        swear(d->text.para[0].str.b == 9);
        swear(d->text.para[0].str.g == 9);
        swear(memcmp(d->text.para[0].str.u, "Goodverse", 9) == 0);
        ok = ui_edit_doc_remove(d, null); // everything
        swear(d->text.paragraphs == 1);
        swear(d->text.para[0].str.b == 0);
        swear(d->text.para[0].str.g == 0);
        ui_edit_text.free(&d->text);
    }
    {
        // TODO: "GoodbyeCruelUniverse" insert 2x"\n" splitting in 3 paragraphs
        const uint8_t* s = (const uint8_t*)"GoodbyeCruelUniverse";
        ok = ui_edit_text.init(&d->text, s, -1, false);
        ui_edit_range_t r = { .from = {.pn = 0, .gp =  7},
                              .to   = {.pn = 0, .gp = 12} };
        ui_edit_to_do_t undo = {0};
        ok = ui_edit_doc.replace(d, &r, (const uint8_t*)"X\nY", -1, &undo);
        swear(ok);
        if (ui_edit_debug_dump) { ui_edit_doc_dump(d); }
        ui_edit_to_do_t redo = {0};
        ok = ui_edit_doc_replace_with_text(d, &undo.range, &undo.text, &redo);
        swear(ok);
        if (ui_edit_debug_dump) { ui_edit_doc_dump(d); }
        ui_edit_text.free(&undo.text);
        swear(ui_edit_doc.bytes(d, null) == 20, "expected 20 bytes");
        ok = ui_edit_doc_replace_with_text(d, &redo.range, &redo.text, &undo);
        swear(ok);
        if (ui_edit_debug_dump) { ui_edit_doc_dump(d); }
        ui_edit_text.free(&undo.text);
        ui_edit_text.free(&redo.text);
    }
    ui_edit_doc.unsubscribe(d, &notify1);
    ui_edit_doc.unsubscribe(d, &notify2);
    ui_edit_doc.dispose(d);
}

ui_edit_text_if ui_edit_text = {
    .init  = ui_edit_text_init,
    .bytes = ui_edit_text_bytes,
    .free  = ui_edit_text_free
};


ui_edit_doc_if ui_edit_doc = {
    .init        = ui_edit_doc_init,
    .replace     = ui_edit_doc_replace,
    .bytes       = ui_edit_doc_bytes,
    .copy        = ui_edit_doc_copy,
    .redo        = ui_edit_doc_redo,
    .undo        = ui_edit_doc_undo,
    .subscribe   = ui_edit_doc_subscribe,
    .unsubscribe = ui_edit_doc_unsubscribe,
    .dispose     = ui_edit_doc_dispose,
    .test        = ui_edit_doc_test
};

#pragma pop_macro("ui_edit_check_zeros")

// Uncomment following line for quick and dirty test run:
ut_static_init(ui_edit_text) { ui_edit_doc.test(); }
