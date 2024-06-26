/* Copyright (c) Dmitry "Leo" Kuznetsov 2021-24 see LICENSE for details */
#include "ut/ut.h"
#include "ui/ui.h"

static bool ui_edit_debug_dump;


static void ui_edit_pg_dump(const ui_edit_pg_t* pg) {
    traceln("pn:%d gp:%d", pg->pn, pg->gp);
}

static void ui_edit_range_dump(const ui_edit_range_t* r) {
    traceln("from {pn:%d gp:%d} to {pn:%d gp:%d}",
            r->from.pn, r->from.gp, r->to.pn, r->to.gp);
}

static void ui_edit_text_dump(const ui_edit_text_t* t) {
    for (int32_t i = 0; i < t->np; i++) {
        const ui_str_t* p = &t->ps[i];
        traceln("ps[%d].%d: %.*s", i, p->b, p->b, p->u);
    }
}

static void ui_edit_doc_dump(const ui_edit_doc_t* d) {
    for (int32_t i = 0; i < d->text.np; i++) {
        const ui_str_t* p = &d->text.ps[i];
        traceln("ps[%d].%d: %.*s", i, p->b, p->b, p->u);
    }
    // TODO: undo/redo stacks and listeners
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
    assert(0 <= (pg_)->pn && (pg_)->pn < (t_)->np &&                        \
           0 <= (pg_)->gp && (pg_)->gp <= (t_)->ps[(pg_)->pn].g)

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

static ui_edit_range_t ui_edit_range_all_on_null(const ui_edit_text_t* t,
        const ui_edit_range_t* range) {
    ui_edit_range_t r;
    if (range != null) {
        r = *range;
    } else {
        assert(t->np >= 1);
        r.from.pn = 0;
        r.from.gp = 0;
        r.to.pn = t->np - 1;
        r.to.gp = t->ps[r.to.pn].g;
    }
    return r;
}

static int ui_edit_compare_pg(ui_edit_pg_t pg1, ui_edit_pg_t pg2) {
    int64_t d = (((int64_t)pg1.pn << 32) | pg1.gp) -
                (((int64_t)pg2.pn << 32) | pg2.gp);
    return d < 0 ? -1 : d > 0 ? 1 : 0;
}

static ui_edit_range_t ui_edit_ordered_range(ui_edit_range_t r) {
    uint64_t f = ((uint64_t)r.from.pn << 32) | r.from.gp;
    uint64_t t = ((uint64_t)r.to.pn   << 32) | r.to.gp;
    if (ui_edit_text.compare_pg(r.from, r.to) > 0) {
        uint64_t swap = t; t = f; f = swap;
        r.from.pn = (int32_t)(f >> 32);
        r.from.gp = (int32_t)(f);
        r.to.pn   = (int32_t)(t >> 32);
        r.to.gp   = (int32_t)(t);
    }
    return r;
}

static bool ui_edit_doc_realloc_ps(ui_str_t* *ps,
        int32_t old_np, int32_t new_np) { // reallocate paragraphs
    ui_str_t* p = *ps;
    for (int32_t i = new_np; i < old_np; i++) { ui_str.free(&p[i]); }
    bool ok = true;
    if (new_np == 0) {
        ut_heap.free(p);
        *ps = null;
    } else {
        ok = ut_heap.realloc_zero((void**)&p, new_np * sizeof(ui_str_t)) == 0;
        if (ok) {
            for (int32_t i = old_np; i < new_np; i++) {
                ok = ui_str.init(&p[i], null, 0, false);
                swear(ok, "because .init(\"\", 0) does NOT allocate memory");
            }
            *ps = p;
        }
    }
    return ok;
}

static bool ui_edit_text_init(ui_edit_text_t* t,
        const uint8_t* s, int32_t b, bool heap) {
    // When text comes from the source that lifetime is shorter
    // than text itself (e.g. paste from clipboard) the parameter
    // heap: true allows to make a copy of data on the heap
    ui_edit_check_zeros(t, sizeof(*t));
    memset(t, 0x00, sizeof(*t));
    if (b < 0) { b = (int32_t)strlen((const char*)s); }
    // if caller is concerned with best performance - it should pass b >= 0
    int32_t np = 0; // number of paragraphs
    int32_t n = ut_max(b / 64, 2); // initial number of allocated paragraphs
    ui_str_t* ps = null; // ps[n]
    bool ok = ui_edit_doc_realloc_ps(&ps, 0, n);
    if (ok) {
        bool lf = false;
        int32_t i = 0;
        while (ok && i < b) {
            int32_t k = i;
            while (k < b && s[k] != '\n') { k++; }
            lf = k < b && s[k] == '\n';
            if (np >= n) {
                int32_t n1_5 = n * 3 / 2; // n * 1.5
                assert(n1_5 > n);
                ok = ui_edit_doc_realloc_ps(&ps, n, n1_5);
                if (ok) { n = n1_5; }
            }
            if (ok) {
                // insider knowledge about ui_str allocation behaviour:
                assert(ps[np].c == 0 && ps[np].b == 0 &&
                       ps[np].g2b[0] == 0);
                ui_str.free(&ps[np]);
                // str.init may allocate str.g2b[] on the heap and may fail
                const int32_t bytes = k - i; assert(bytes >= 0);
                const uint8_t* u = bytes == 0 ? null : s + i;
                ok = ui_str.init(&ps[np], u, bytes, heap && bytes > 0);
                if (ok) { np++; }
            }
            i = k + lf;
        }
        if (ok && lf) { // last paragraph ended with line feed
            if (np + 1 >= n) {
                ok = ui_edit_doc_realloc_ps(&ps, n, n + 1);
                if (ok) { n = n + 1; }
            }
            if (ok) { np++; }
        }
    }
    if (ok && np == 0) { // special case empty string to a single paragraph
        assert(b <= 0 && (b == 0 || s[0] == 0x00));
        np = 1; // ps[0] is already initialized as empty str
        ok = ui_edit_doc_realloc_ps(&ps, n, 1);
        swear(ok, "shrinking ps[] above");
    }
    if (ok) {
        assert(np > 0);
        t->np = np;
        t->ps = ps;
    } else if (ps != null) {
        ok = ui_edit_doc_realloc_ps(&ps, n, 0); // free()
        swear(ok);
        ut_heap.free(ps);
        t->np = 0;
        t->ps = null;
    }
    return ok;
}

static void ui_edit_text_dispose(ui_edit_text_t* t) {
    if (t->np != 0) {
        ui_edit_doc_realloc_ps(&t->ps, t->np, 0);
        assert(t->ps == null);
        t->np = 0;
    } else {
        assert(t->np == 0 && t->ps == null);
    }
}

static void ui_edit_doc_dispose_to_do(ui_edit_to_do_t* to_do) {
    if (to_do->text.np > 0) {
        ui_edit_text_dispose(&to_do->text);
    }
    memset(&to_do->range, 0x00, sizeof(to_do->range));
    ui_edit_check_zeros(to_do, sizeof(*to_do));
}

static int32_t ui_edit_text_bytes(const ui_edit_text_t* t,
        const ui_edit_range_t* range) {
    const ui_edit_range_t r =
        ui_edit_text.ordered(ui_edit_text.all_on_null(t, range));
    ui_edit_check_range_inside_text(t, &r);
    int32_t bytes = 0;
    for (int32_t pn = r.from.pn; pn <= r.to.pn; pn++) {
        const ui_str_t* p = &t->ps[pn];
        if (pn == r.from.pn && pn == r.to.pn) {
            bytes += p->g2b[r.to.gp] - p->g2b[r.from.gp];
        } else if (pn == r.from.pn) {
            bytes += p->b - p->g2b[r.from.gp];
        } else if (pn == r.to.pn) {
            bytes += p->g2b[r.to.gp];
        } else {
            bytes += p->b;
        }
    }
    return bytes;
}

static int32_t ui_edit_doc_bytes(const ui_edit_doc_t* d,
        const ui_edit_range_t* r) {
    return ui_edit_text.bytes(&d->text, r);
}

static int32_t ui_edit_doc_utf8bytes(const ui_edit_doc_t* d,
        const ui_edit_range_t* range) {
    const ui_edit_range_t r =
        ui_edit_text.ordered(ui_edit_text.all_on_null(&d->text, range));
    int32_t bytes = ui_edit_text.bytes(&d->text, &r);
    // "\n" after each paragraph and 0x00
    return bytes + r.to.pn - r.from.pn + 1;
}

static bool ui_edit_doc_remove(ui_edit_doc_t* d,
        const ui_edit_range_t* range) {
    ui_edit_range_t r =
        ui_edit_text.ordered(ui_edit_text.all_on_null(&d->text, range));
    ui_edit_check_range_inside_text(&d->text, &r);
    // first paragraph:
    ui_str_t* p0 = &d->text.ps[r.from.pn];
    ui_str_t* p1 = &d->text.ps[r.to.pn];
    bool ok = // move first and last paragraphs to the heap
        ui_str.expand(p0, p0->b + 1) ||
        ui_str.expand(p1, p1->b + 1); // if p0 == p1 this is no-op
    if (ok) {
        ui_str_t merge = {0}; // merge of p0 and p1
        if (p0 != p1) {
            int32_t bytes = p0->g2b[r.from.gp];
            uint8_t* u = bytes == 0 ? null : p0->u;
            ok = ui_str.init(&merge, u, bytes, true); // allocate on heap
            if (ok) {
                bytes = p1->b - p1->g2b[r.to.gp];
                u = bytes == 0 ? null : p1->u + p1->g2b[r.to.gp];
                ok = ui_str.replace(&merge, merge.g, merge.g, u, bytes);
            }
        }
        if (ok) {
            if (p1 != p0) {
                int32_t deleted = 0;
                for (int32_t pn = r.from.pn + 1; pn <= r.to.pn; pn++) {
                    ui_str.free(&d->text.ps[pn]);
                    deleted++;
                }
                ui_str.free(&d->text.ps[r.from.pn]);
                d->text.ps[r.from.pn] = merge;
                // remove deleted paragraphs:
                if (deleted > 0 && d->text.np - r.to.pn - 1 > 0) {
                    memcpy(&d->text.ps[r.from.pn + 1],
                           &d->text.ps[r.from.pn + 1 + deleted],
                           (d->text.np - r.to.pn - 1) *
                            sizeof(ui_str_t));
                }
                d->text.np -= deleted;
            } else {
                ok = ui_str.replace(p0, r.from.gp, r.to.gp, null, 0);
            }
        } else if (merge.c > 0 || merge.g > 0) {
            ui_str.free(&merge);
        }
        assert(d->text.np >= 1);
    }
    return ok;
}

static bool ui_edit_doc_insert(ui_edit_doc_t* d,
        const ui_edit_pg_t* i, // insertion point
        const ui_edit_text_t* ins) {
    ui_edit_check_pg_inside_text(&d->text, i);
    bool ok = true;
    int32_t np = d->text.np + ins->np - 1;
    ui_str_t* ps = null;
    ok = ui_edit_doc_realloc_ps(&ps, 0, np);
    ui_str_t* f = null; // first string "ins" is merged with
    if (ok) {
        // ps[] will hold pointers to the original strings
        // from the document and copies of text->ps[] strings:
        for (int32_t pn = 0; pn < np; pn++) { ui_str.free(&ps[pn]); }
        for (int32_t pn = 0; ok && pn < np; pn++) {
            ui_str_t* s = &d->text.ps[pn];
            if (pn < i->pn || i->pn + ins->np <= pn) {
                ps[pn] = *s;
            } else if (i->pn <= pn && pn < i->pn + ins->np) {
                // insert `ins` into `i` insertion point
                if (pn == i->pn) {
                    ok = ui_str.init(&ps[pn], s->u, s->b, true);
                    if (ok) {
                        assert(0 <= i->gp && i->gp <= s->g);
                        // if there is continuation paragraph cut the tail
                        // it will be reinserted below as `f`
                        const int32_t to = ins->np > 1 ? s->g : i->gp;
                        ok = ui_str.replace(&ps[pn], i->gp, to,
                                            ins->ps[0].u, ins->ps[0].b);
                        f = s;
                    }
                } else if (ins->np > 1 &&
                    pn == i->pn + ins->np - 1) {
                    // last string of text
                    const ui_str_t* e = &ins->ps[ins->np - 1];
                    assert(f != null);
                    if (f != null) {
                        ok = ui_str.init(&ps[pn], f->u, f->b, true);
                        if (ok) {
                            ok = ui_str.replace(&ps[pn], 0, i->gp, e->u, e->b);
                        }
                    } else {
                        assert(false, "should not ever be executed");
                        ok = ui_str.init(&ps[pn], e->u, e->b, true);
                    }
                } else if (i->pn < pn && pn < i->pn + ins->np - 1) {
                    int32_t ix = pn - i->pn;
                    assert(0 < ix && ix < ins->np - 1);
                    const ui_str_t* p = &ins->ps[ix];
                    ok = ui_str.init(&ps[pn], p->u, p->b, true);
                } else {
                    assert(false, "should be handled above");
                }
            }
        }
        if (!ok) { // free allocated strings in partial result
            for (int32_t pn = 0; pn < np; pn++) {
                if (i->pn < pn && pn < i->pn + ins->np) {
                    // not all strings in this range actually initialized
                    if (ps[pn].c > 0 || ps[pn].g > 0) {
                        ui_str.free(&ps[pn]);
                    }
                }
            }
        }
        if (ok) {
            if (f != null) { ui_str.free(f); }
            ut_heap.free(d->text.ps); // free old paragraphs
            d->text.ps = ps;
            d->text.np = np;
        }
    }
    return ok;
}

static void ui_edit_notify_before(ui_edit_doc_t* d,
        const ui_edit_range_t* range, const ui_edit_text_t* text) {
    ui_edit_listener_t* o = d->listeners;
    while (o != null) {
        if (o->notify != null && o->notify->before != null) {
            o->notify->before(o->notify, d, range, text);
        }
        o = o->next;
    }
}

static void ui_edit_notify_after(ui_edit_doc_t* d,
        const ui_edit_range_t* range, const ui_edit_text_t* text) {
    ui_edit_listener_t* o = d->listeners;
    while (o != null) {
        if (o->notify != null && o->notify->after != null) {
            o->notify->after(o->notify, d, range, text);
        }
        o = o->next;
    }
}

static bool ui_edit_doc_replace_text(ui_edit_doc_t* d,
        const ui_edit_range_t* range, const ui_edit_text_t* text,
        ui_edit_to_do_t* undo) {
    ui_edit_range_t r =
        ui_edit_text.ordered(ui_edit_text.all_on_null(&d->text, range));
    if (ui_edit_debug_dump) {
        traceln("--->");
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
    ui_edit_notify_before(d, range, text);
    bool empty_range = ui_edit_text.compare_pg(r.from, r.to) == 0;
    bool ok = true;
    if (undo != null) {
        ui_edit_doc.copy_text(d, &r, &undo->text);
        if (ui_edit_debug_dump) {
            traceln("copy: (undo->text)");
            ui_edit_text_dump(&undo->text);
            traceln("");
        }
    }
    if (ok) {
        if (empty_range) {
            ok = ui_edit_doc_insert(d, &r.from, text);
        } else {
            ok = ui_edit_doc_insert(d, &r.to, text);
            if (ok) {
                // TODO: manually test reinsertion
                if (!ui_edit_doc_remove(d, &r)) {
                    // remove inserted text:
                    ui_edit_range_t inserted = { .from = r.to, .to = r.to };
                    inserted.to.pn += text->np - 1;
                    inserted.to.gp  = text->ps[text->np - 1].g;
                    ui_edit_check_range_inside_text(&d->text, &inserted);
                    (void)ui_edit_doc_remove(d, &inserted);
                }
            }
        }
        if (ok & ui_edit_debug_dump) {
            traceln("doc:");
            ui_edit_doc_dump(d);
        }
        if (ok && undo != null) {
            undo->range.from  = r.from;
            undo->range.to.pn = r.from.pn + text->np - 1;
            if (undo->range.from.pn == undo->range.to.pn) {
                undo->range.to.gp = undo->range.from.gp +
                                    text->ps[text->np - 1].g;
            } else {
                undo->range.to.gp = text->ps[text->np - 1].g;
            }
            if (ui_edit_debug_dump) {
                traceln("undo: ");
                ui_edit_range_dump(&undo->range);
                ui_edit_text_dump(&undo->text);
            }
            ui_edit_check_range_inside_text(&d->text, &undo->range);
        }
    }
    if (!ok && undo != null) { ui_edit_doc.dispose_to_do(undo); }
    if (ui_edit_debug_dump) {
        traceln("<---");
    }
    if (ok) {
        ui_edit_notify_after(d, range, text);
    }
    return ok;
}

static bool ui_edit_doc_subscribe(ui_edit_doc_t* t, ui_edit_notify_t* notify) {
    // TODO: not sure about double linked list.
    // heap allocated resizable array may serve better and may be easier to maintain
    bool ok = true;
    ui_edit_listener_t* o = t->listeners;
    if (o == null) {
        ok = ut_heap.alloc_zero((void**)&t->listeners, sizeof(*o)) == 0;
        if (ok) { o = t->listeners; }
    } else {
        while (o->next != null) { swear(o->notify != notify); o = o->next; }
        ok = ut_heap.alloc_zero((void**)&o->next, sizeof(*o)) == 0;
        if (ok) { o->next->prev = o; o = o->next; }
    }
    if (ok) { o->notify = notify; }
    return ok;
}

static void ui_edit_doc_unsubscribe(ui_edit_doc_t* t, ui_edit_notify_t* notify) {
    ui_edit_listener_t* o = t->listeners;
    bool removed = false;
    while (o != null) {
        ui_edit_listener_t* n = o->next;
        if (o->notify == notify) {
            assert(!removed);
            if (o->prev != null) { o->prev->next = n; }
            if (o->next != null) { o->next->prev = o->prev; }
            if (o == t->listeners) { t->listeners = n; }
            ut_heap.free(o);
            removed = true;
        }
        o = n;
    }
    swear(removed);
}

static bool ui_edit_doc_copy_text(const ui_edit_doc_t* d,
        const ui_edit_range_t* range, ui_edit_text_t* t) {
    ui_edit_check_zeros(t, sizeof(*t));
    memset(t, 0x00, sizeof(*t));
    ui_edit_range_t r =
        ui_edit_text.ordered(ui_edit_text.all_on_null(&d->text, range));
    ui_edit_check_range_inside_text(&d->text, &r);
    int32_t np = r.to.pn - r.from.pn + 1;
    bool ok = ui_edit_doc_realloc_ps(&t->ps, 0, np);
    if (ok) { t->np = np; }
    for (int32_t pn = r.from.pn; ok && pn <= r.to.pn; pn++) {
        const ui_str_t* p = &d->text.ps[pn];
        const uint8_t* u = p->u;
        int32_t bytes = 0;
        if (pn == r.from.pn && pn == r.to.pn) {
            bytes = p->g2b[r.to.gp] - p->g2b[r.from.gp];
            u += p->g2b[r.from.gp];
        } else if (pn == r.from.pn) {
            bytes = p->b - p->g2b[r.from.gp];
            u += p->g2b[r.from.gp];
        } else if (pn == r.to.pn) {
            bytes = p->g2b[r.to.gp];
        } else {
            bytes = p->b;
        }
        assert(t->ps[pn - r.from.pn].g == 0);
        ui_str.replace(&t->ps[pn - r.from.pn], 0, 0,
                          bytes == 0 ? null : u, bytes);
    }
    if (!ok) {
        ui_edit_text.dispose(t);
        ui_edit_check_zeros(t, sizeof(*t));
    }
    return ok;
}

static void ui_edit_doc_copy(const ui_edit_doc_t* d,
        const ui_edit_range_t* range, char* text) {
    ui_edit_range_t r =
        ui_edit_text.ordered(ui_edit_text.all_on_null(&d->text, range));
    ui_edit_check_range_inside_text(&d->text, &r);
    char* t = text;
    for (int32_t pn = r.from.pn; pn <= r.to.pn; pn++) {
        const ui_str_t* p = &d->text.ps[pn];
        const uint8_t* u = p->u;
        int32_t bytes = 0;
        if (pn == r.from.pn && pn == r.to.pn) {
            bytes = p->g2b[r.to.gp] - p->g2b[r.from.gp];
            u += p->g2b[r.from.gp];
        } else if (pn == r.from.pn) {
            bytes = p->b - p->g2b[r.from.gp];
            u += p->g2b[r.from.gp];
        } else if (pn == r.to.pn) {
            bytes = p->g2b[r.to.gp];
        } else {
            bytes = p->b;
        }
        if (bytes > 0) {
            memcpy(t, u, bytes);
            t += bytes;
            if (pn < r.to.pn) { *t++ = '\n'; }
        }
    }
    *t++ = 0x00;
    const int32_t utf8_bytes = (int32_t)(uintptr_t)(t - text);
    assert(utf8_bytes == ui_edit_doc.utf8bytes(d, &r));
    (void)utf8_bytes;
}

static bool ui_edit_doc_replace(ui_edit_doc_t* d, const ui_edit_range_t* r,
        const uint8_t* u, int32_t b) {
    ui_edit_text_t t = {0};
    ui_edit_to_do_t* undo = null;
    bool ok = ut_heap.alloc_zero((void**)&undo, sizeof(ui_edit_to_do_t)) == 0;
    if (ok) {
        ok = ui_edit_text.init(&t, u, b, false);
    }
    if (ok) {
        ok = ui_edit_doc.replace_text(d, r, &t, undo);
        ui_edit_text.dispose(&t);
    }
    if (ok) {
        undo->next = d->undo;
        d->undo = undo;
        // redo stack is not valid after new replace, empty it:
        while (d->redo != null) {
            ui_edit_doc.dispose_to_do(d->redo);
            d->redo = d->redo->next;
        }
    } else {
        if (undo != null) {
            ui_edit_doc.dispose_to_do(d->undo);
            ut_heap.free(undo);
        }
    }
    return ok;
}

static bool ui_edit_text_dup(ui_edit_text_t* d, const ui_edit_text_t* s) {
    ui_edit_check_zeros(d, sizeof(*d));
    memset(d, 0x00, sizeof(*d));
    bool ok = ui_edit_doc_realloc_ps(&d->ps, 0, s->np);
    if (ok) {
        d->np = s->np;
        for (int32_t i = 0; ok && i < s->np; i++) {
            const ui_str_t* p = &s->ps[i];
            ok = ui_str.replace(&d->ps[i], 0, 0, p->u, p->b);
        }
    }
    if (!ok) {
        ui_edit_text.dispose(d);
    }
    return ok;
}

static bool ui_edit_text_equal(ui_edit_text_t* s1, const ui_edit_text_t* s2) {
    bool equal =  s1->np != s2->np;
    for (int32_t i = 0; equal && i < s1->np; i++) {
        const ui_str_t* p1 = &s1->ps[i];
        const ui_str_t* p2 = &s2->ps[i];
        equal = p1->b == p2->b &&
                memcmp(p1->u, p2->u, p1->b) == 0;
    }
    return true;
}

static bool ui_edit_doc_do(ui_edit_doc_t* d, ui_edit_to_do_t* to_do,
        ui_edit_to_do_t* *stack) {
    const ui_edit_range_t* r = &to_do->range;
    ui_edit_to_do_t* redo = null;
    bool ok = ut_heap.alloc_zero((void**)&redo, sizeof(ui_edit_to_do_t)) == 0;
    if (ok) {
        ok = ui_edit_doc.replace_text(d, r, &to_do->text, redo);
        if (ok) {
            ui_edit_doc.dispose_to_do(to_do);
            ut_heap.free(to_do);
        }
        if (ok) {
            redo->next = *stack;
            *stack = redo;
        } else {
            if (redo != null) {
                ui_edit_doc.dispose_to_do(redo);
                ut_heap.free(redo);
            }
        }
    }
    return ok;
}

static bool ui_edit_doc_redo(ui_edit_doc_t* d) {
    ui_edit_to_do_t* to_do = d->redo;
    if (to_do == null) {
        return false;
    } else {
        d->redo = d->redo->next;
        return ui_edit_doc_do(d, to_do, &d->undo);
    }
}

static bool ui_edit_doc_undo(ui_edit_doc_t* d) {
    ui_edit_to_do_t* to_do = d->undo;
    if (to_do == null) {
        return false;
    } else {
        d->undo = d->undo->next;
        return ui_edit_doc_do(d, to_do, &d->redo);
    }
}

static bool ui_edit_doc_init(ui_edit_doc_t* d) {
    bool ok = true;
    ui_edit_check_zeros(d, sizeof(*d));
    memset(d, 0x00, sizeof(d));
    ok = ut_heap.alloc_zero((void**)&d->text.ps, sizeof(ui_str_t)) == 0;
    if (ok) {
        d->text.np = 1;
        ok = ui_str.init(&d->text.ps[0], null, 0, false);
    }
    return ok;
}

static void ui_edit_doc_dispose(ui_edit_doc_t* d) {
    for (int32_t i = 0; i < d->text.np; i++) {
        ui_str.free(&d->text.ps[i]);
    }
    if (d->text.ps != null) {
        ut_heap.free(d->text.ps);
        d->text.ps = null;
    }
    d->text.np  = 0;
    while (d->undo != null) {
        ui_edit_to_do_t* next = d->undo->next;
        d->undo->next = null;
        ui_edit_doc.dispose_to_do(d->undo);
        ut_heap.free(d->undo);
        d->undo = next;
    }
    while (d->redo != null) {
        ui_edit_to_do_t* next = d->redo->next;
        d->redo->next = null;
        ui_edit_doc.dispose_to_do(d->redo);
        ut_heap.free(d->redo);
        d->redo = next;
    }
    assert(d->listeners == null, "called must unsubscribe all listeners");
    while (d->listeners != null) {
        traceln("unsubscribed listener: 0x%p", d->listeners);
        ui_edit_listener_t* next = d->listeners->next;
        d->listeners->next = null;
        ut_heap.free(d->listeners->next);
        d->listeners = next;
    }
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
    ui_edit_text.dispose(&t);
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
            swear(t.ps != null && t.np == 1);
            swear(t.ps[0].u[0] == 0 &&
                  t.ps[0].c == 0);
            swear(t.ps[0].b == 0 &&
                  t.ps[0].g == 0);
            ui_edit_text.dispose(&t);
        }
        {   // string without "\n"
            const uint8_t* hello = (const uint8_t*)"hello";
            const int32_t n = (int32_t)strlen((const char*)hello);
            ui_edit_text_t t = {0};
            bool ok = ui_edit_text.init(&t, hello, n, false);
            swear(ok);
            swear(t.ps != null && t.np == 1);
            swear(t.ps[0].u == hello);
            swear(t.ps[0].c == 0);
            swear(t.ps[0].b == n);
            swear(t.ps[0].g == n);
            ui_edit_text.dispose(&t);
        }
        {   // string with "\n" at the end
            const uint8_t* hello = (const uint8_t*)"hello\n";
            ui_edit_text_t t = {0};
            bool ok = ui_edit_text.init(&t, hello, -1, false);
            swear(ok);
            swear(t.ps != null && t.np == 2);
            swear(t.ps[0].u == hello);
            swear(t.ps[0].c == 0);
            swear(t.ps[0].b == 5);
            swear(t.ps[0].g == 5);
            swear(t.ps[1].u[0] == 0x00);
            swear(t.ps[0].c == 0);
            swear(t.ps[1].b == 0);
            swear(t.ps[1].g == 0);
            ui_edit_text.dispose(&t);
        }
        {   // two string separated by "\n"
            const uint8_t* hello = (const uint8_t*)"hello\nworld";
            const uint8_t* world = hello + 6;
            ui_edit_text_t t = {0};
            bool ok = ui_edit_text.init(&t, hello, -1, false);
            swear(ok);
            swear(t.ps != null && t.np == 2);
            swear(t.ps[0].u == hello);
            swear(t.ps[0].c == 0);
            swear(t.ps[0].b == 5);
            swear(t.ps[0].g == 5);
            swear(t.ps[1].u == world);
            swear(t.ps[0].c == 0);
            swear(t.ps[1].b == 5);
            swear(t.ps[1].g == 5);
            ui_edit_text.dispose(&t);
        }
    }
    for (int i = 0; i < 10; i++) {
        ui_edit_doc_test_big_text();
    }
    traceln("done");
}

typedef struct ui_edit_doc_test_notify_s {
    ui_edit_notify_t notify;
    int32_t count_before;
    int32_t count_after;
} ui_edit_doc_test_notify_t;

static void ui_edit_doc_test_before(ui_edit_notify_t* n,
        const ui_edit_doc_t*   unused(d),
        const ui_edit_range_t* unused(r),
        const ui_edit_text_t*  unused(t)) {
    ui_edit_doc_test_notify_t* notify =
        (ui_edit_doc_test_notify_t*)n;
    notify->count_before++;
//  traceln("before: %d", notify->count_before);
}

static void ui_edit_doc_test_after(ui_edit_notify_t* n,
        const ui_edit_doc_t*   unused(d),
        const ui_edit_range_t* unused(r),
        const ui_edit_text_t*  unused(t)) {
    ui_edit_doc_test_notify_t* notify =
        (ui_edit_doc_test_notify_t*)n;
    notify->count_after++;
//  traceln("after: %d", notify->count_after);
}

static struct {
    ui_edit_notify_t notify;
} ui_edit_doc_test_notify;


static void ui_edit_doc_test_3(void) {
    {
        ui_edit_doc_t edit_doc = {0};
        ui_edit_doc_t* d = &edit_doc;
        ui_edit_doc_test_notify_t before_and_after = {0};
        before_and_after.notify.before = ui_edit_doc_test_before;
        before_and_after.notify.after  = ui_edit_doc_test_after;
        swear(ui_edit_doc.init(d));
        swear(ui_edit_doc.subscribe(d, &before_and_after.notify));
        const uint8_t* s = (const uint8_t*)"Goodbye Cruel Universe";
        const int32_t before = before_and_after.count_before;
        const int32_t after  = before_and_after.count_after;
        swear(ui_edit_doc.replace(d, null, s, -1));
        const int32_t bytes = (int32_t)strlen((const char*)s);
        swear(before + 1 == before_and_after.count_before);
        swear(after  + 1 == before_and_after.count_after);
        swear(d->text.np == 1);
        swear(ui_edit_doc.bytes(d, null) == bytes);
        ui_edit_text_t t = {0};
        swear(ui_edit_doc.copy_text(d, null, &t));
        swear(t.np == 1);
        swear(t.ps[0].b == bytes);
        swear(t.ps[0].g == bytes);
        swear(memcmp(t.ps[0].u, s, t.ps[0].b) == 0);
        // with "\n" and 0x00 at the end:
        int32_t utf8bytes = ui_edit_doc.utf8bytes(d, null);
        char* p = null;
        swear(ut_heap.alloc((void**)&p, utf8bytes) == 0);
        p[utf8bytes - 1] = 0xFF;
        ui_edit_doc.copy(d, null, p);
        swear(p[utf8bytes - 1] == 0x00);
        swear(memcmp(p, s, bytes) == 0);
        ut_heap.free(p);
        ui_edit_text.dispose(&t);
        ui_edit_doc.unsubscribe(d, &before_and_after.notify);
        ui_edit_doc.dispose(d);
    }
    {
        ui_edit_doc_t edit_doc = {0};
        ui_edit_doc_t* d = &edit_doc;
        swear(ui_edit_doc.init(d));
        const uint8_t* s = (const uint8_t*)
            "Hello World"
            "\n"
            "Goodbye Cruel Universe";
        swear(ui_edit_doc.replace(d, null, s, -1));
        swear(ui_edit_doc.undo(d));
        swear(ui_edit_doc.bytes(d, null) == 0);
        swear(ui_edit_doc.utf8bytes(d, null) == 1);
        swear(ui_edit_doc.redo(d));
        {
            int32_t utf8bytes = ui_edit_doc.utf8bytes(d, null);
            char* p = null;
            swear(ut_heap.alloc((void**)&p, utf8bytes) == 0);
            p[utf8bytes - 1] = 0xFF;
            ui_edit_doc.copy(d, null, p);
            swear(p[utf8bytes - 1] == 0x00);
            swear(memcmp(p, s, utf8bytes) == 0);
            ut_heap.free(p);
        }
        ui_edit_doc.dispose(d);
    }
}

static void ui_edit_doc_test_0(void) {
    ui_edit_doc_t edit_doc = {0};
    ui_edit_doc_t* d = &edit_doc;
    swear(ui_edit_doc.init(d));
    ui_edit_text_t ins_text = {0};
    swear(ui_edit_text.init(&ins_text, (const uint8_t*)"a", 1, false));
    ui_edit_to_do_t undo = {0};
    swear(ui_edit_doc.replace_text(d, null, &ins_text, &undo));
    ui_edit_doc.dispose_to_do(&undo);
    ui_edit_text.dispose(&ins_text);
    ui_edit_doc.dispose(d);
}

static void ui_edit_doc_test_1(void) {
    ui_edit_doc_t edit_doc = {0};
    ui_edit_doc_t* d = &edit_doc;
    swear(ui_edit_doc.init(d));
    ui_edit_text_t ins_text = {0};
    swear(ui_edit_text.init(&ins_text, (const uint8_t*)"a", 1, false));
    ui_edit_to_do_t undo = {0};
    swear(ui_edit_doc.replace_text(d, null, &ins_text, &undo));
    ui_edit_doc.dispose_to_do(&undo);
    ui_edit_text.dispose(&ins_text);
    ui_edit_doc.dispose(d);
}

static void ui_edit_doc_test_2(void) {
    {   // two string separated by "\n"
        ui_edit_doc_t edit_doc = {0};
        ui_edit_doc_t* d = &edit_doc;
        swear(ui_edit_doc.init(d));
        ui_edit_notify_t notify1 = {0};
        ui_edit_notify_t notify2 = {0};
        ui_edit_doc_test_notify_t before_and_after = {0};
        before_and_after.notify.before = ui_edit_doc_test_before;
        before_and_after.notify.after  = ui_edit_doc_test_after;
        ui_edit_doc.subscribe(d, &notify1);
        ui_edit_doc.subscribe(d, &before_and_after.notify);
        ui_edit_doc.subscribe(d, &notify2);
        swear(ui_edit_doc.bytes(d, null) == 0, "expected empty");
        const uint8_t* hello = (const uint8_t*)"hello\nworld";
        swear(ui_edit_doc.replace(d, null, hello, -1));
        ui_edit_text_t t = {0};
        swear(ui_edit_doc.copy_text(d, null, &t));
        swear(t.np == 2);
        swear(t.ps[0].b == 5);
        swear(t.ps[0].g == 5);
        swear(memcmp(t.ps[0].u, "hello", 5) == 0);
        swear(t.ps[1].b == 5);
        swear(t.ps[1].g == 5);
        swear(memcmp(t.ps[1].u, "world", 5) == 0);
        ui_edit_text.dispose(&t);
        ui_edit_doc.unsubscribe(d, &notify1);
        ui_edit_doc.unsubscribe(d, &before_and_after.notify);
        ui_edit_doc.unsubscribe(d, &notify2);
        ui_edit_doc.dispose(d);
    }
    // TODO: "GoodbyeCruelUniverse" insert 2x"\n" splitting in 3 paragraphs
    {   // three string separated by "\n"
        ui_edit_doc_t edit_doc = {0};
        ui_edit_doc_t* d = &edit_doc;
        swear(ui_edit_doc.init(d));
        const uint8_t* s = (const uint8_t*)"Goodbye" "\n" "Cruel" "\n" "Universe";
        swear(ui_edit_doc.replace(d, null, s, -1));
        ui_edit_text_t t = {0};
        swear(ui_edit_doc.copy_text(d, null, &t));
        ui_edit_text.dispose(&t);
        ui_edit_range_t r = { .from = {.pn = 0, .gp = 4},
                              .to   = {.pn = 2, .gp = 3} };
        swear(ui_edit_doc_remove(d, &r));
        swear(d->text.np == 1);
        swear(d->text.ps[0].b == 9);
        swear(d->text.ps[0].g == 9);
        swear(memcmp(d->text.ps[0].u, "Goodverse", 9) == 0);
        swear(ui_edit_doc_remove(d, null));
        swear(d->text.np == 1);
        swear(d->text.ps[0].b == 0);
        swear(d->text.ps[0].g == 0);
        ui_edit_doc.dispose(d);
    }
    // TODO: "GoodbyeCruelUniverse" insert 2x"\n" splitting in 3 paragraphs
    {
        ui_edit_doc_t edit_doc = {0};
        ui_edit_doc_t* d = &edit_doc;
        swear(ui_edit_doc.init(d));
        const char* ins[] = { "X\nY", "X\n", "\nY", "\n", "X\nY\nZ" };
        for (int32_t i = 0; i < countof(ins); i++) {
            const uint8_t* s = (const uint8_t*)"GoodbyeCruelUniverse";
            swear(ui_edit_doc.replace(d, null, s, -1));
            ui_edit_range_t r = { .from = {.pn = 0, .gp =  7},
                                  .to   = {.pn = 0, .gp = 12} };
            ui_edit_text_t ins_text = {0};
            ui_edit_text.init(&ins_text, (const uint8_t*)ins[i], -1, false);
            ui_edit_to_do_t undo = {0};
            swear(ui_edit_doc.replace_text(d, &r, &ins_text, &undo));
            if (ui_edit_debug_dump) { ui_edit_doc_dump(d); }
            ui_edit_to_do_t redo = {0};
            swear(ui_edit_doc.replace_text(d, &undo.range, &undo.text, &redo));
            if (ui_edit_debug_dump) { ui_edit_doc_dump(d); }
            ui_edit_doc.dispose_to_do(&undo);
            undo.range = (ui_edit_range_t){0};
            swear(ui_edit_doc.replace_text(d, &redo.range, &redo.text, &undo));
            if (ui_edit_debug_dump) { ui_edit_doc_dump(d); }
            ui_edit_doc.dispose_to_do(&redo);
            ui_edit_doc.dispose_to_do(&undo);
            ui_edit_text.dispose(&ins_text);
        }
        ui_edit_doc.dispose(d);
    }
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
    enum { n = 1000 };
    // use n = 10,000,000 and Diagnostic Tools to watch for memory leaks
//  enum { n = 10 * 1000 * 1000 };
    for (int32_t i = 0; i < n; i++) {
        ui_edit_doc_test_0();
        ui_edit_doc_test_1();
        ui_edit_doc_test_2();
        ui_edit_doc_test_3();
    }
}

ui_edit_text_if ui_edit_text = {
    .init        = ui_edit_text_init,
    .bytes       = ui_edit_text_bytes,
    .compare_pg  = ui_edit_compare_pg,
    .all_on_null = ui_edit_range_all_on_null,
    .ordered     = ui_edit_ordered_range,
    .dispose     = ui_edit_text_dispose
};

ui_edit_doc_if ui_edit_doc = {
    .init               = ui_edit_doc_init,
    .replace_text       = ui_edit_doc_replace_text,
    .replace            = ui_edit_doc_replace,
    .bytes              = ui_edit_doc_bytes,
    .copy_text          = ui_edit_doc_copy_text,
    .utf8bytes          = ui_edit_doc_utf8bytes,
    .copy               = ui_edit_doc_copy,
    .redo               = ui_edit_doc_redo,
    .undo               = ui_edit_doc_undo,
    .subscribe          = ui_edit_doc_subscribe,
    .unsubscribe        = ui_edit_doc_unsubscribe,
    .dispose_to_do      = ui_edit_doc_dispose_to_do,
    .dispose            = ui_edit_doc_dispose,
    .test               = ui_edit_doc_test
};

#pragma pop_macro("ui_edit_check_zeros")

// Uncomment following line for quick and dirty test run:
ut_static_init(ui_edit_text) { ui_edit_doc.test(); }
