/* Copyright (c) Dmitry "Leo" Kuznetsov 2021-24 see LICENSE for details */
#include "ut/ut.h"
#include "ui/ui.h"

#undef UI_EDIT_STR_TEST
#undef UI_EDIT_DOC_TEST
#undef UI_STR_TEST_REPLACE_ALL_PERMUTATIONS
#undef UI_EDIT_DOC_TEST_PARAGRAPHS

#if 0 // flip to 1 to run tests

#define UI_EDIT_STR_TEST
#define UI_EDIT_DOC_TEST

#if 0 // flip to 1 to run exhausting lengthy tests
#define UI_STR_TEST_REPLACE_ALL_PERMUTATIONS
#define UI_EDIT_DOC_TEST_PARAGRAPHS
#endif

#endif

#pragma push_macro("ui_edit_check_zeros")
#pragma push_macro("ui_edit_check_pg_inside_text")
#pragma push_macro("ui_edit_check_range_inside_text")
#pragma push_macro("ui_edit_pg_dump")
#pragma push_macro("ui_edit_range_dump")
#pragma push_macro("ui_edit_text_dump")
#pragma push_macro("ui_edit_doc_dump")

#define ui_edit_pg_dump(pg)                              \
    ut_debug.println(__FILE__, __LINE__, __func__,       \
                    "pn:%d gp:%d", (pg)->pn, (pg)->gp)

#define ui_edit_range_dump(r)                            \
    ut_debug.println(__FILE__, __LINE__, __func__,       \
            "from {pn:%d gp:%d} to {pn:%d gp:%d}",       \
    (r)->from.pn, (r)->from.gp, (r)->to.pn, (r)->to.gp);

#define ui_edit_text_dump(t) do {                        \
    for (int32_t i_ = 0; i_ < (t)->np; i_++) {           \
        const ui_edit_str_t* p_ = &t->ps[i_];            \
        ut_debug.println(__FILE__, __LINE__, __func__,   \
            "ps[%d].%d: %.*s", i_, p_->b, p_->b, p_->u); \
    }                                                    \
} while (0)

// TODO: undo/redo stacks and listeners
#define ui_edit_doc_dump(d) do {                                \
    for (int32_t i_ = 0; i_ < (d)->text.np; i_++) {             \
        const ui_edit_str_t* p_ = &(d)->text.ps[i_];            \
        ut_debug.println(__FILE__, __LINE__, __func__,          \
            "ps[%d].b:%d.c:%d: %p %.*s", i_, p_->b, p_->c,      \
            p_, p_->b, p_->u);                                  \
    }                                                           \
} while (0)


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

static ui_edit_range_t ui_edit_text_all_on_null(const ui_edit_text_t* t,
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

static int ui_edit_range_compare(const ui_edit_pg_t pg1, const ui_edit_pg_t pg2) {
    int64_t d = (((int64_t)pg1.pn << 32) | pg1.gp) -
                (((int64_t)pg2.pn << 32) | pg2.gp);
    return d < 0 ? -1 : d > 0 ? 1 : 0;
}

static ui_edit_range_t ui_edit_range_order(const ui_edit_range_t range) {
    ui_edit_range_t r = range;
    uint64_t f = ((uint64_t)r.from.pn << 32) | r.from.gp;
    uint64_t t = ((uint64_t)r.to.pn   << 32) | r.to.gp;
    if (ui_edit_range.compare(r.from, r.to) > 0) {
        uint64_t swap = t; t = f; f = swap;
        r.from.pn = (int32_t)(f >> 32);
        r.from.gp = (int32_t)(f);
        r.to.pn   = (int32_t)(t >> 32);
        r.to.gp   = (int32_t)(t);
    }
    return r;
}

static ui_edit_range_t ui_edit_text_ordered(const ui_edit_text_t* t,
        const ui_edit_range_t* r) {
    return ui_edit_range.order(ui_edit_text.all_on_null(t, r));
}

static bool ui_edit_range_is_valid(const ui_edit_range_t r) {
    if (0 <= r.from.pn && 0 <= r.to.pn &&
        0 <= r.from.gp && 0 <= r.to.gp) {
        ui_edit_range_t o = ui_edit_range.order(r);
        return ui_edit_range.compare(o.from, o.to) <= 0;
    } else {
        return false;
    }
}

static bool ui_edit_range_is_empty(const ui_edit_range_t r) {
    return r.from.pn == r.to.pn && r.from.gp == r.to.gp;
}

static ui_edit_pg_t ui_edit_text_end(const ui_edit_text_t* t) {
    return (ui_edit_pg_t){ .pn = t->np - 1, .gp = t->ps[t->np - 1].g };
}

static ui_edit_range_t ui_edit_text_end_range(const ui_edit_text_t* t) {
    ui_edit_pg_t e = (ui_edit_pg_t){ .pn = t->np - 1,
                                     .gp = t->ps[t->np - 1].g };
    return (ui_edit_range_t){ .from = e, .to = e };
}

static uint64_t ui_edit_range_uint64(const ui_edit_pg_t pg) {
    assert(pg.pn >= 0 && pg.gp >= 0);
    return ((uint64_t)pg.pn << 32) | (uint64_t)pg.gp;
}

static ui_edit_pg_t ui_edit_range_pg(uint64_t uint64) {
    assert((int32_t)(uint64 >> 32) >= 0 && (int32_t)uint64 >= 0);
    return (ui_edit_pg_t){ .pn = (int32_t)(uint64 >> 32), .gp = (int32_t)uint64 };
}

static bool ui_edit_range_inside_text(const ui_edit_text_t* t,
        const ui_edit_range_t r) {
    return ui_edit_range.is_valid(r) &&
            0 <= r.from.pn && r.from.pn <= r.to.pn && r.to.pn < t->np &&
            0 <= r.from.gp && r.from.gp <= r.to.gp &&
            r.to.gp <= t->ps[r.to.pn - 1].g;
}

static ui_edit_range_t ui_edit_range_intersect(const ui_edit_range_t r1,
    const ui_edit_range_t r2) {
    if (ui_edit_range.is_valid(r1) && ui_edit_range.is_valid(r2)) {
        ui_edit_range_t o1 = ui_edit_range.order(r1);
        ui_edit_range_t o2 = ui_edit_range.order(r1);
        uint64_t f1 = ((uint64_t)o1.from.pn << 32) | o1.from.gp;
        uint64_t t1 = ((uint64_t)o1.to.pn   << 32) | o1.to.gp;
        uint64_t f2 = ((uint64_t)o2.from.pn << 32) | o2.from.gp;
        uint64_t t2 = ((uint64_t)o2.to.pn   << 32) | o2.to.gp;
        if (f1 <= f2 && f2 <= t1) { // f2 is inside r1
            if (t2 <= t1) { // r2 is fully inside r1
                return r2;
            } else { // r2 is partially inside r1
                ui_edit_range_t r = {0};
                r.from.pn = (int32_t)(f2 >> 32);
                r.from.gp = (int32_t)(f2);
                r.to.pn   = (int32_t)(t1 >> 32);
                r.to.gp   = (int32_t)(t1);
                return r;
            }
        } else if (f2 <= f1 && f1 <= t2) { // f1 is inside r2
            if (t1 <= t2) { // r1 is fully inside r2
                return r1;
            } else { // r1 is partially inside r2
                ui_edit_range_t r = {0};
                r.from.pn = (int32_t)(f1 >> 32);
                r.from.gp = (int32_t)(f1);
                r.to.pn   = (int32_t)(t2 >> 32);
                r.to.gp   = (int32_t)(t2);
                return r;
            }
        } else {
            return *ui_edit_range.invalid_range;
        }
    } else {
        return *ui_edit_range.invalid_range;
    }
}

static bool ui_edit_doc_realloc_ps_no_init(ui_edit_str_t* *ps,
        int32_t old_np, int32_t new_np) { // reallocate paragraphs
    for (int32_t i = new_np; i < old_np; i++) { ui_edit_str.free(&(*ps)[i]); }
    bool ok = true;
    if (new_np == 0) {
        ut_heap.free(*ps);
        *ps = null;
    } else {
        ok = ut_heap.realloc_zero((void**)ps, new_np * sizeof(ui_edit_str_t)) == 0;
    }
    return ok;
}

static bool ui_edit_doc_realloc_ps(ui_edit_str_t* *ps,
        int32_t old_np, int32_t new_np) { // reallocate paragraphs
    bool ok = ui_edit_doc_realloc_ps_no_init(ps, old_np, new_np);
    if (ok) {
        for (int32_t i = old_np; i < new_np; i++) {
            ok = ui_edit_str.init(&(*ps)[i], null, 0, false);
            swear(ok, "because .init(\"\", 0) does NOT allocate memory");
        }
    }
    return ok;
}

static bool ui_edit_text_init(ui_edit_text_t* t,
        const char* s, int32_t b, bool heap) {
    // When text comes from the source that lifetime is shorter
    // than text itself (e.g. paste from clipboard) the parameter
    // heap: true allows to make a copy of data on the heap
    ui_edit_check_zeros(t, sizeof(*t));
    memset(t, 0x00, sizeof(*t));
    if (b < 0) { b = (int32_t)strlen(s); }
    // if caller is concerned with best performance - it should pass b >= 0
    int32_t np = 0; // number of paragraphs
    int32_t n = ut_max(b / 64, 2); // initial number of allocated paragraphs
    ui_edit_str_t* ps = null; // ps[n]
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
                // insider knowledge about ui_edit_str allocation behaviour:
                assert(ps[np].c == 0 && ps[np].b == 0 &&
                       ps[np].g2b[0] == 0);
                ui_edit_str.free(&ps[np]);
                // process "\r\n" strings
                const int32_t e = k > i && s[k - 1] == '\r' ? k - 1 : k;
                const int32_t bytes = e - i; assert(bytes >= 0);
                const char* u = bytes == 0 ? null : s + i;
                // str.init may allocate str.g2b[] on the heap and may fail
                ok = ui_edit_str.init(&ps[np], u, bytes, heap && bytes > 0);
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
    const ui_edit_range_t r = ui_edit_text.ordered(t, range);
    ui_edit_check_range_inside_text(t, &r);
    int32_t bytes = 0;
    for (int32_t pn = r.from.pn; pn <= r.to.pn; pn++) {
        const ui_edit_str_t* p = &t->ps[pn];
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
    const ui_edit_range_t r = ui_edit_text.ordered(&d->text, range);
    int32_t bytes = ui_edit_text.bytes(&d->text, &r);
    // "\n" after each paragraph and 0x00
    return bytes + r.to.pn - r.from.pn + 1;
}

static void ui_edit_notify_before(ui_edit_doc_t* d,
        const ui_edit_notify_info_t* ni) {
    ui_edit_listener_t* o = d->listeners;
    while (o != null) {
        if (o->notify != null && o->notify->before != null) {
            o->notify->before(o->notify, ni);
        }
        o = o->next;
    }
}

static void ui_edit_notify_after(ui_edit_doc_t* d,
        const ui_edit_notify_info_t* ni) {
    ui_edit_listener_t* o = d->listeners;
    while (o != null) {
        if (o->notify != null && o->notify->after != null) {
            o->notify->after(o->notify, ni);
        }
        o = o->next;
    }
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
    const ui_edit_range_t r = ui_edit_text.ordered(&d->text, range);
    ui_edit_check_range_inside_text(&d->text, &r);
    int32_t np = r.to.pn - r.from.pn + 1;
    bool ok = ui_edit_doc_realloc_ps(&t->ps, 0, np);
    if (ok) { t->np = np; }
    for (int32_t pn = r.from.pn; ok && pn <= r.to.pn; pn++) {
        const ui_edit_str_t* p = &d->text.ps[pn];
        const char* u = p->u;
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
        const char* u_or_null = bytes == 0 ? null : u;
        ui_edit_str.replace(&t->ps[pn - r.from.pn], 0, 0, u_or_null, bytes);
    }
    if (!ok) {
        ui_edit_text.dispose(t);
        ui_edit_check_zeros(t, sizeof(*t));
    }
    return ok;
}

static void ui_edit_doc_copy(const ui_edit_doc_t* d,
        const ui_edit_range_t* range, char* text, int32_t b) {
    const ui_edit_range_t r = ui_edit_text.ordered(&d->text, range);
    ui_edit_check_range_inside_text(&d->text, &r);
    char* to = text;
    for (int32_t pn = r.from.pn; pn <= r.to.pn; pn++) {
        const ui_edit_str_t* p = &d->text.ps[pn];
        const char* u = p->u;
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
        const int32_t c = (int32_t)(uintptr_t)(to - text);
        if (bytes > 0) {
            swear(c + bytes < b, "c: %d bytes: %d b: %d", c, bytes, b);
            memmove(to, u, bytes);
            to += bytes;
        }
        if (pn < r.to.pn) {
            swear(c + bytes < b, "c: %d bytes: %d b: %d", c, bytes, b);
            *to++ = '\n';
        }
    }
    const int32_t c = (int32_t)(uintptr_t)(to - text);
    swear(c + 1 == b, "c: %d b: %d", c, b);
    *to++ = 0x00;
}

static bool ui_edit_text_insert_2_or_more(ui_edit_text_t* t, int32_t pn,
        const ui_edit_str_t* s, const ui_edit_text_t* insert,
        const ui_edit_str_t* e) {
    // insert 2 or more paragraphs
    assert(0 <= pn && pn < t->np);
    const int32_t np = t->np + insert->np - 1;
    assert(np > 0);
    ui_edit_str_t* ps = null; // ps[np]
    bool ok = ui_edit_doc_realloc_ps_no_init(&ps, 0, np);
    if (ok) {
        memmove(ps, t->ps, pn * sizeof(ui_edit_str_t));
        // `s` first line of `insert`
        ok = ui_edit_str.init(&ps[pn], s->u, s->b, true);
        // lines of `insert` between `s` and `e`
        for (int32_t i = 1; ok && i < insert->np - 1; i++) {
            ok = ui_edit_str.init(&ps[pn + i], insert->ps[i].u,
                                               insert->ps[i].b, true);
        }
        // `e` last line of `insert`
        if (ok) {
            const int32_t ix = pn + insert->np - 1; // last `insert` index
            ok = ui_edit_str.init(&ps[ix], e->u, e->b, true);
        }
        assert(t->np - pn - 1 >= 0);
        memmove(ps + pn + insert->np, t->ps + pn + 1,
               (t->np - pn - 1) * sizeof(ui_edit_str_t));
        if (ok) {
            // this two regions where moved to `ps`
            memset(t->ps, 0x00, pn * sizeof(ui_edit_str_t));
            memset(t->ps + pn + 1, 0x00, (t->np - pn - 1) * sizeof(ui_edit_str_t));
            // deallocate what was copied from `insert`
            ui_edit_doc_realloc_ps_no_init(&t->ps, t->np, 0);
            t->np = np;
            t->ps = ps;
        } else { // free allocated memory:
            ui_edit_doc_realloc_ps_no_init(&ps, np, 0);
        }
    }
    return ok;
}

static bool ui_edit_text_insert_1(ui_edit_text_t* t,
        const ui_edit_pg_t ip, // insertion point
        const ui_edit_text_t* insert) {
    assert(0 <= ip.pn && ip.pn < t->np);
    ui_edit_str_t* str = &t->ps[ip.pn]; // string in document text
    assert(insert->np == 1);
    ui_edit_str_t* ins = &insert->ps[0]; // string to insert
    assert(0 <= ip.gp && ip.gp <= str->g);
    // ui_edit_str.replace() is all or nothing:
    return ui_edit_str.replace(str, ip.gp, ip.gp, ins->u, ins->b);
}

static bool ui_edit_substr_append(ui_edit_str_t* d, const ui_edit_str_t* s1,
    int32_t gp1, const ui_edit_str_t* s2) { // s1[0:gp1] + s2
    assert(d != s1 && d != s2);
    const int32_t b = s1->g2b[gp1];
    bool ok = ui_edit_str.init(d, b == 0 ? null : s1->u, b, true);
    if (ok) {
        ok = ui_edit_str.replace(d, d->g, d->g, s2->u, s2->b);
    } else {
        *d = *ui_edit_str.empty;
    }
    return ok;
}

static bool ui_edit_append_substr(ui_edit_str_t* d, const ui_edit_str_t* s1,
    const ui_edit_str_t* s2, int32_t gp2) {  // s1 + s2[gp1:*]
    assert(d != s1 && d != s2);
    bool ok = ui_edit_str.init(d, s1->b == 0 ? null : s1->u, s1->b, true);
    if (ok) {
        const int32_t o = s2->g2b[gp2]; // offset (bytes)
        const int32_t b = s2->b - o;
        ok = ui_edit_str.replace(d, d->g, d->g, b == 0 ? null : s2->u + o, b);
    } else {
        *d = *ui_edit_str.empty;
    }
    return ok;
}

static bool ui_edit_text_insert(ui_edit_text_t* t, const ui_edit_pg_t ip,
        const ui_edit_text_t* i) {
    bool ok = true;
    if (ok) {
        if (i->np == 1) {
            ok = ui_edit_text_insert_1(t, ip, i);
        } else {
            ui_edit_str_t* str = &t->ps[ip.pn];
            ui_edit_str_t s = {0}; // start line of insert text `i`
            ui_edit_str_t e = {0}; // end   line
            if (ui_edit_substr_append(&s, str, ip.gp, &i->ps[0])) {
                if (ui_edit_append_substr(&e, &i->ps[i->np - 1], str, ip.gp)) {
                    ok = ui_edit_text_insert_2_or_more(t, ip.pn, &s, i, &e);
                    ui_edit_str.free(&e);
                }
                ui_edit_str.free(&s);
            }
        }
    }
    return ok;
}

static bool ui_edit_text_remove_lines(ui_edit_text_t* t,
    ui_edit_str_t* merge, int32_t from, int32_t to) {
    bool ok = true;
    for (int32_t pn = from + 1; pn <= to; pn++) {
        ui_edit_str.free(&t->ps[pn]);
    }
    if (t->np - to - 1 > 0) {
        memmove(&t->ps[from + 1], &t->ps[to + 1],
                (t->np - to - 1) * sizeof(ui_edit_str_t));
    }
    t->np -= to - from;
    if (ok) {
        ui_edit_str.swap(&t->ps[from], merge);
    }
    return ok;
}

static bool ui_edit_text_insert_remove(ui_edit_text_t* t,
        const ui_edit_range_t r, const ui_edit_text_t* i) {
    bool ok = true;
    ui_edit_str_t merge = {0};
    const ui_edit_str_t* s = &t->ps[r.from.pn];
    const ui_edit_str_t* e = &t->ps[r.to.pn];
    const int32_t o = e->g2b[r.to.gp];
    const int32_t b = e->b - o;
    const char* u = b == 0 ? null : e->u + o;
    ok = ui_edit_substr_append(&merge, s, r.from.gp, &i->ps[i->np - 1]) &&
         ui_edit_str.replace(&merge, merge.g, merge.g, u, b);
    if (ok) {
        const bool empty_text = i->np == 1 && i->ps[0].g == 0;
        if (!empty_text) {
            ok = ui_edit_text_insert(t, r.to, i);
        }
        if (ok) {
            ok = ui_edit_text_remove_lines(t, &merge, r.from.pn, r.to.pn);
        }
    }
    if (merge.c > 0 || merge.g > 0) { ui_edit_str.free(&merge); }
    return ok;
}

static bool ui_edit_text_copy_text(const ui_edit_text_t* t,
        const ui_edit_range_t* range, ui_edit_text_t* to) {
    ui_edit_check_zeros(to, sizeof(*to));
    memset(to, 0x00, sizeof(*to));
    const ui_edit_range_t r = ui_edit_text.ordered(t, range);
    ui_edit_check_range_inside_text(t, &r);
    int32_t np = r.to.pn - r.from.pn + 1;
    bool ok = ui_edit_doc_realloc_ps(&to->ps, 0, np);
    if (ok) { to->np = np; }
    for (int32_t pn = r.from.pn; ok && pn <= r.to.pn; pn++) {
        const ui_edit_str_t* p = &t->ps[pn];
        const char* u = p->u;
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
        assert(to->ps[pn - r.from.pn].g == 0);
        const char* u_or_null = bytes == 0 ? null : u;
        ui_edit_str.replace(&to->ps[pn - r.from.pn], 0, 0, u_or_null, bytes);
    }
    if (!ok) {
        ui_edit_text.dispose(to);
        ui_edit_check_zeros(to, sizeof(*to));
    }
    return ok;
}

static void ui_edit_text_copy(const ui_edit_text_t* t,
        const ui_edit_range_t* range, char* text, int32_t b) {
    const ui_edit_range_t r = ui_edit_text.ordered(t, range);
    ui_edit_check_range_inside_text(t, &r);
    char* to = text;
    for (int32_t pn = r.from.pn; pn <= r.to.pn; pn++) {
        const ui_edit_str_t* p = &t->ps[pn];
        const char* u = p->u;
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
        const int32_t c = (int32_t)(uintptr_t)(to - text);
        swear(c + bytes < b, "d: %d bytes:%d b: %d", c, bytes, b);
        if (bytes > 0) {
            memmove(to, u, bytes);
            to += bytes;
        }
        if (pn < r.to.pn) {
            swear(c + bytes + 1 < b, "d: %d bytes:%d b: %d", c, bytes, b);
            *to++ = '\n';
        }
    }
    const int32_t c = (int32_t)(uintptr_t)(to - text);
    swear(c + 1 == b, "d: %d b: %d", c, b);
    *to++ = 0x00;
}

static bool ui_edit_text_replace(ui_edit_text_t* t,
        const ui_edit_range_t* range, const ui_edit_text_t* i,
        ui_edit_to_do_t* undo) {
    const ui_edit_range_t r = ui_edit_text.ordered(t, range);
    bool ok = undo == null ? true : ui_edit_text.copy_text(t, &r, &undo->text);
    ui_edit_range_t x = r;
    if (ok) {
        if (ui_edit_range.is_empty(r)) {
            x.to.pn = r.from.pn + i->np - 1;
            x.to.gp = i->np == 1 ? r.from.gp + i->ps[0].g : i->ps[i->np - 1].g;
            ok = ui_edit_text_insert(t, r.from, i);
        } else if (i->np == 1 && r.from.pn == r.to.pn) {
            x.to.pn = r.from.pn + i->np - 1;
            x.to.gp = r.from.gp + i->ps[0].g;
            ok = ui_edit_str.replace(&t->ps[r.from.pn],
                    r.from.gp, r.to.gp, i->ps[0].u, i->ps[0].b);
        } else {
            x.to.pn = r.from.pn + i->np - 1;
            x.to.gp = i->np == 1 ? r.from.gp + i->ps[0].g : i->ps[0].g;
            ok = ui_edit_text_insert_remove(t, r, i);
        }
    }
    if (undo != null) { undo->range = x; }
    return ok;
}

static bool ui_edit_text_replace_utf8(ui_edit_text_t* t,
        const ui_edit_range_t* range,
        const char* utf8, int32_t b,
        ui_edit_to_do_t* undo) {
    if (b < 0) { b = (int32_t)strlen(utf8); }
    ui_edit_text_t i = {0};
    bool ok = ui_edit_text.init(&i, utf8, b, false);
    if (ok) {
        ok = ui_edit_text.replace(t, range, &i, undo);
        ui_edit_text.dispose(&i);
    }
    return ok;
}

static bool ui_edit_text_dup(ui_edit_text_t* t, const ui_edit_text_t* s) {
    ui_edit_check_zeros(t, sizeof(*t));
    memset(t, 0x00, sizeof(*t));
    bool ok = ui_edit_doc_realloc_ps(&t->ps, 0, s->np);
    if (ok) {
        t->np = s->np;
        for (int32_t i = 0; ok && i < s->np; i++) {
            const ui_edit_str_t* p = &s->ps[i];
            ok = ui_edit_str.replace(&t->ps[i], 0, 0, p->u, p->b);
        }
    }
    if (!ok) {
        ui_edit_text.dispose(t);
    }
    return ok;
}

static bool ui_edit_text_equal(const ui_edit_text_t* t1,
        const ui_edit_text_t* t2) {
    bool equal =  t1->np != t2->np;
    for (int32_t i = 0; equal && i < t1->np; i++) {
        const ui_edit_str_t* p1 = &t1->ps[i];
        const ui_edit_str_t* p2 = &t2->ps[i];
        equal = p1->b == p2->b &&
                memcmp(p1->u, p2->u, p1->b) == 0;
    }
    return equal;
}

static void ui_edit_doc_before_replace_text(ui_edit_doc_t* d,
        const ui_edit_range_t r, const ui_edit_text_t* t) {
    ui_edit_check_range_inside_text(&d->text, &r);
    ui_edit_range_t x = r;
    x.to.pn = r.from.pn + t->np - 1;
    if (r.from.pn == r.to.pn && t->np == 1) {
        x.to.gp = r.from.gp + t->ps[0].g;
    } else {
        x.to.gp = t->ps[t->np - 1].g;
    }
    const ui_edit_notify_info_t ni_before = {
        .ok = true, .d = d, .r = &r, .x = &x, .t = t,
        .pnf = r.from.pn, .pnt = r.to.pn,
        .deleted = 0, .inserted = 0
    };
    ui_edit_notify_before(d, &ni_before);
}

static void ui_edit_doc_after_replace_text(ui_edit_doc_t* d,
        bool ok,
        const ui_edit_range_t r,
        const ui_edit_range_t x,
        const ui_edit_text_t* t) {
    const ui_edit_notify_info_t ni_after = {
        .ok = ok, .d = d, .r = &r, .x = &x, .t = t,
        .pnf = r.from.pn, .pnt = x.to.pn,
        .deleted = r.to.pn - r.from.pn,
        .inserted = t->np - 1
    };
    ui_edit_notify_after(d, &ni_after);
}

static bool ui_edit_doc_replace_text(ui_edit_doc_t* d,
        const ui_edit_range_t* range, const ui_edit_text_t* i,
        ui_edit_to_do_t* undo) {
    ui_edit_text_t* t = &d->text;
    const ui_edit_range_t r = ui_edit_text.ordered(t, range);
    ui_edit_doc_before_replace_text(d, r, i);
#if 0
    bool ok = undo == null ? true : ui_edit_text.copy(t, &r, &undo->text);
    // TODO: remove
    ui_edit_range_t x = r;
    if (ok) {
        if (ui_edit_range.is_empty(r)) {
            x.to.pn = r.from.pn + i->np - 1;
            x.to.gp = i->np == 1 ?
                      r.from.gp + i->ps[0].g :
                      i->ps[i->np - 1].g;
            ok = ui_edit_text_insert(t, r.from, i);
        } else if (i->np == 1 && r.from.pn == r.to.pn) {
            x.to.pn = r.from.pn + i->np - 1;
            x.to.gp = r.from.gp + i->ps[0].g;
            ok = ui_edit_str.replace(&t->ps[r.from.pn],
                                r.from.gp, r.to.gp,
                                i->ps[0].u, i->ps[0].b);
        } else {
            x.to.pn = r.from.pn + i->np - 1;
            x.to.gp = i->np == 1 ? r.from.gp + i->ps[0].g : i->ps[0].g;
            ok = ui_edit_text_insert_remove(t, r, i);
        }
    }
    if (undo != null) { undo->range = x; }
#else
    bool ok = ui_edit_text.replace(t, &r, i, undo);
#endif
    ui_edit_doc_after_replace_text(d, ok, r, undo->range, i);
    return ok;
}

static bool ui_edit_doc_replace_undoable(ui_edit_doc_t* d,
        const ui_edit_range_t* r, const ui_edit_text_t* t,
        ui_edit_to_do_t* undo) {
    bool ok = ui_edit_doc_replace_text(d, r, t, undo);
    if (ok && undo != null) {
        undo->next = d->undo;
        d->undo = undo;
        // redo stack is not valid after new replace, empty it:
        while (d->redo != null) {
            ui_edit_to_do_t* next = d->redo->next;
            ui_edit_doc.dispose_to_do(d->redo);
            ut_heap.free(d->redo);
            d->redo = next;
        }
    }
    return ok;
}

static bool ui_edit_utf8_to_heap_text(const char* u, int32_t b,
        ui_edit_text_t* it) {
    assert((b == 0) == (u == null || u[0] == 0x00));
    return ui_edit_text.init(it, b != 0 ? u : null, b, true);
}

static bool ui_edit_doc_replace(ui_edit_doc_t* d,
        const ui_edit_range_t* range, const char* u, int32_t b) {
    ui_edit_text_t* t = &d->text;
    const ui_edit_range_t r = ui_edit_text.ordered(t, range);
    ui_edit_to_do_t* undo = null;
    bool ok = ut_heap.alloc_zero((void**)&undo, sizeof(ui_edit_to_do_t)) == 0;
    if (ok) {
        ui_edit_text_t i = {0};
        ok = ui_edit_utf8_to_heap_text(u, b, &i);
        if (ok) {
            ok = ui_edit_doc_replace_undoable(d, &r, &i, undo);
            ui_edit_text.dispose(&i);
        }
        if (!ok) {
            ui_edit_doc.dispose_to_do(undo);
            ut_heap.free(undo);
        }
    }
    return ok;
}

static bool ui_edit_doc_do(ui_edit_doc_t* d, ui_edit_to_do_t* to_do,
        ui_edit_to_do_t* *stack) {
    const ui_edit_range_t* r = &to_do->range;
    ui_edit_to_do_t* redo = null;
    bool ok = ut_heap.alloc_zero((void**)&redo, sizeof(ui_edit_to_do_t)) == 0;
    if (ok) {
        ok = ui_edit_doc_replace_text(d, r, &to_do->text, redo);
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
        to_do->next = null;
        return ui_edit_doc_do(d, to_do, &d->undo);
    }
}

static bool ui_edit_doc_undo(ui_edit_doc_t* d) {
    ui_edit_to_do_t* to_do = d->undo;
    if (to_do == null) {
        return false;
    } else {
        d->undo = d->undo->next;
        to_do->next = null;
        return ui_edit_doc_do(d, to_do, &d->redo);
    }
}

static bool ui_edit_doc_init(ui_edit_doc_t* d, const char* utf8,
        int32_t bytes, bool heap) {
    bool ok = true;
    ui_edit_check_zeros(d, sizeof(*d));
    memset(d, 0x00, sizeof(d));
    assert(bytes >= 0);
    assert((utf8 == null) == (bytes == 0));
    if (ok) {
        if (bytes == 0) { // empty string
            ok = ut_heap.alloc_zero((void**)&d->text.ps, sizeof(ui_edit_str_t)) == 0;
            if (ok) {
                d->text.np = 1;
                ok = ui_edit_str.init(&d->text.ps[0], null, 0, false);
            }
        } else {
            ok = ui_edit_text.init(&d->text, utf8, bytes, heap);
        }
    }
    return ok;
}

static void ui_edit_doc_dispose(ui_edit_doc_t* d) {
    for (int32_t i = 0; i < d->text.np; i++) {
        ui_edit_str.free(&d->text.ps[i]);
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
    assert(d->listeners == null, "unsubscribe listeners?");
    while (d->listeners != null) {
        ui_edit_listener_t* next = d->listeners->next;
        d->listeners->next = null;
        ut_heap.free(d->listeners->next);
        d->listeners = next;
    }
    ui_edit_check_zeros(d, sizeof(*d));
}

// ui_edit_str

static int32_t ui_edit_str_g2b_ascii[1024]; // ui_edit_str_g2b_ascii[i] == i for all "i"
static int8_t  ui_edit_str_empty_utf8[1] = {0x00};

static const ui_edit_str_t ui_edit_str_empty = {
    .u = ui_edit_str_empty_utf8,
    .g2b = ui_edit_str_g2b_ascii,
    .c = 0, .b = 0, .g = 0
};

static bool    ui_edit_str_init(ui_edit_str_t* s, const char* u, int32_t b, bool heap);
static void    ui_edit_str_swap(ui_edit_str_t* s1, ui_edit_str_t* s2);
static int32_t ui_edit_str_utf8_bytes(const char* u, int32_t b);
static int32_t ui_edit_str_glyphs(const char* utf8, int32_t bytes);
static int32_t ui_edit_str_gp_to_bp(const char* s, int32_t bytes, int32_t gp);
static int32_t ui_edit_str_bytes(ui_edit_str_t* s, int32_t f, int32_t t);
static bool    ui_edit_str_expand(ui_edit_str_t* s, int32_t c);
static void    ui_edit_str_shrink(ui_edit_str_t* s);
static bool    ui_edit_str_replace(ui_edit_str_t* s, int32_t f, int32_t t,
                                   const char* u, int32_t b);
static void    ui_edit_str_test(void);
static void    ui_edit_str_free(ui_edit_str_t* s);

ui_edit_str_if ui_edit_str = {
    .init        = ui_edit_str_init,
    .swap        = ui_edit_str_swap,
    .utf8bytes   = ui_edit_str_utf8_bytes,
    .glyphs      = ui_edit_str_glyphs,
    .gp_to_bp    = ui_edit_str_gp_to_bp,
    .bytes       = ui_edit_str_bytes,
    .expand      = ui_edit_str_expand,
    .shrink      = ui_edit_str_shrink,
    .replace     = ui_edit_str_replace,
    .test        = ui_edit_str_test,
    .free        = ui_edit_str_free,
    .empty       = &ui_edit_str_empty
};

#pragma push_macro("ui_edit_str_check")
#pragma push_macro("ui_edit_str_check_from_to")
#pragma push_macro("ui_edit_check_zeros")
#pragma push_macro("ui_edit_str_check_empty")
#pragma push_macro("ui_edit_str_parameters")

#ifdef DEBUG

#define ui_edit_str_check(s) do {                                   \
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
            ui_edit_str_utf8_bytes(                                 \
            s->u + s->g2b[i - 1], s->g2b[i] - s->g2b[i - 1]));      \
    }                                                               \
} while (0)

#define ui_edit_str_check_from_to(s, f, t) do {                     \
    assert(0 <= f && f <= s->g);                                    \
    assert(0 <= t && t <= s->g);                                    \
    assert(f <= t);                                                 \
} while (0)

#define ui_edit_str_check_empty(u, b) do {                          \
    if (b == 0) { assert(u != null && u[0] == 0x00); }              \
    if (u == null || u[0] == 0x00) { assert(b == 0); }              \
} while (0)



#else

#define ui_edit_str_check(s)               do { } while (0)
#define ui_edit_str_check_from_to(s, f, t) do { } while (0)
#define ui_edit_str_check_empty(u, b)      do { } while (0)

#endif

// ui_edit_str_foo(*, "...", -1) treat as 0x00 terminated
// ui_edit_str_foo(*, null, 0) treat as ("", 0)

#define ui_edit_str_parameters(u, b) do {                           \
    if (u == null) { u = ui_edit_str_empty_utf8; }                  \
    if (b < 0)  {                                                   \
        assert(strlen(u) < INT32_MAX);                              \
        b = (int32_t)strlen(u);                                     \
    }                                                               \
    ui_edit_str_check_empty(u, b);                                  \
} while (0)

static int32_t ui_edit_str_utf8_bytes(const char* s, int32_t b) {
    swear(b >= 1, "should not be called with bytes < 1");
    const uint8_t* const u = (const uint8_t*)s;
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

static int32_t ui_edit_str_glyphs(const char* utf8, int32_t bytes) {
    swear(bytes >= 0);
    bool ok = true;
    int32_t i = 0;
    int32_t k = 1;
    while (i < bytes && ok) {
        const int32_t b = ui_edit_str_utf8_bytes(utf8 + i, bytes - i);
        ok = 0 < b && i + b <= bytes;
        if (ok) { i += b; k++; }
    }
    return ok ? k - 1 : -1;
}

static int32_t ui_edit_str_gp_to_bp(const char* utf8, int32_t bytes, int32_t gp) {
    swear(bytes >= 0);
    bool ok = true;
    int32_t c = 0;
    int32_t i = 0;
    if (bytes > 0) {
        while (c < gp && ok) {
            assert(i < bytes);
            const int32_t b = ui_edit_str_utf8_bytes(utf8 + i, bytes - i);
            ok = 0 < b && i + b <= bytes;
            if (ok) { i += b; c++; }
        }
    }
    assert(i <= bytes);
    return ok ? i : -1;
}

static void ui_edit_str_free(ui_edit_str_t* s) {
    if (s->g2b != null && s->g2b != ui_edit_str_g2b_ascii) {
        ut_heap.free(s->g2b);
    } else {
        #ifdef UI_EDIT_STR_TEST // check ui_edit_str_g2b_ascii integrity
            for (int32_t i = 0; i < countof(ui_edit_str_g2b_ascii); i++) {
                assert(ui_edit_str_g2b_ascii[i] == i);
            }
        #endif
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
    ui_edit_check_zeros(s, sizeof(*s));
}

static bool ui_edit_str_init_g2b(ui_edit_str_t* s) {
    const int64_t _4_bytes = (int64_t)sizeof(int32_t);
    // start with number of glyphs == number of bytes (ASCII text):
    bool ok = ut_heap.alloc(&s->g2b, (s->b + 1) * _4_bytes) == 0;
    int32_t i = 0; // index in u[] string
    int32_t k = 1; // glyph number
    // g2b[k] start postion in uint8_t offset from utf8 text of glyph[k]
    while (i < s->b && ok) {
        const int32_t b = ui_edit_str_utf8_bytes(s->u + i, s->b - i);
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

static bool ui_edit_str_init(ui_edit_str_t* s, const char* u, int32_t b,
        bool heap) {
    enum { n = countof(ui_edit_str_g2b_ascii) };
    if (ui_edit_str_g2b_ascii[n - 1] != n - 1) {
        for (int32_t i = 0; i < n; i++) { ui_edit_str_g2b_ascii[i] = i; }
    }
    bool ok = true;
    ui_edit_check_zeros(s, sizeof(*s)); // caller must zero out
    memset(s, 0x00, sizeof(*s));
    ui_edit_str_parameters(u, b);
    if (b == 0) { // cast below intentionally removes "const" qualifier
        s->g2b = (int32_t*)ui_edit_str_g2b_ascii;
        s->u = (char*)u;
        assert(s->c == 0 && u[0] == 0x00);
    } else {
        if (heap) {
            ok = ut_heap.alloc((void**)&s->u, b) == 0;
            if (ok) { s->c = b; memmove(s->u, u, b); }
        } else {
            s->u = (char*)u;
        }
        if (ok) {
            s->b = b;
            if (b == 1 && u[0] <= 0x7F) {
                s->g2b = (int32_t*)ui_edit_str_g2b_ascii;
                s->g = 1;
            } else {
                ok = ui_edit_str_init_g2b(s);
            }
        }
    }
    if (ok) { ui_edit_str.shrink(s); } else { ui_edit_str.free(s); }
    return ok;
}

static void ui_edit_str_swap(ui_edit_str_t* s1, ui_edit_str_t* s2) {
    ui_edit_str_t s = *s1; *s1 = *s2; *s2 = s;
}

static int32_t ui_edit_str_bytes(ui_edit_str_t* s,
        int32_t f, int32_t t) { // glyph positions
    ui_edit_str_check_from_to(s, f, t);
    ui_edit_str_check(s);
    return s->g2b[t] - s->g2b[f];
}

static bool ui_edit_str_move_g2b_to_heap(ui_edit_str_t* s) {
    bool ok = true;
    if (s->g2b == ui_edit_str_g2b_ascii) { // even for s->g == 0
        if (s->b == s->g && s->g < countof(ui_edit_str_g2b_ascii) - 1) {
//          traceln("forcefully moving to heap");
            // this is usually done in the process of concatenation
            // of 2 ascii strings when result is known to be longer
            // than countof(ui_edit_str_g2b_ascii) - 1 but the
            // first string in concatenation is short. It's OK.
        }
        const int32_t bytes = (s->g + 1) * (int32_t)sizeof(int32_t);
        ok = ut_heap.alloc(&s->g2b, bytes) == 0;
        if (ok) { memmove(s->g2b, ui_edit_str_g2b_ascii, bytes); }
    }
    return ok;
}

static bool ui_edit_str_move_to_heap(ui_edit_str_t* s, int32_t c) {
    bool ok = true;
    assert(c >= s->b, "can expand cannot shrink");
    if (s->c == 0) { // s->u points outside of the heap
        const char* o = s->u;
        ok = ut_heap.alloc((void**)&s->u, c) == 0;
        if (ok) { memmove(s->u, o, s->b); }
    } else if (s->c < c) {
        ok = ut_heap.realloc((void**)&s->u, c) == 0;
    }
    if (ok) { s->c = c; }
    return ok;
}

static bool ui_edit_str_expand(ui_edit_str_t* s, int32_t c) {
    swear(c > 0);
    bool ok = ui_edit_str_move_to_heap(s, c);
    if (ok && c > s->c) {
        if (ut_heap.realloc((void**)&s->u, c) == 0) {
            s->c = c;
        } else {
            ok = false;
        }
    }
    return ok;
}

static void ui_edit_str_shrink(ui_edit_str_t* s) {
    if (s->c > s->b) { // s->c == 0 for empty and single byte ASCII strings
        assert(s->u != ui_edit_str_empty_utf8);
        if (s->b == 0) {
            ut_heap.free(s->u);
            s->u = ui_edit_str_empty_utf8;
        } else {
            bool ok = ut_heap.realloc((void**)&s->u, s->b) == 0;
            swear(ok, "smaller size is always expected to be ok");
        }
        s->c = s->b;
    }
    // Optimize memory for short ASCII only strings:
    if (s->g2b != ui_edit_str_g2b_ascii) {
        if (s->g == s->b && s->g < countof(ui_edit_str_g2b_ascii) - 1) {
            // If this is an ascii only utf8 string shorter than
            // ui_edit_str_g2b_ascii it does not need .g2b[] allocated:
            if (s->g2b != ui_edit_str_g2b_ascii) {
                ut_heap.free(s->g2b);
                s->g2b = ui_edit_str_g2b_ascii;
            }
        } else {
//          const int32_t b64 = ut_min(s->b, 64);
//          traceln("none ASCII: .b:%d .g:%d %*.*s", s->b, s->g, b64, b64, s->u);
        }
    }
}

static bool ui_edit_str_remove(ui_edit_str_t* s, int32_t f, int32_t t) {
    bool ok = true; // optimistic approach
    ui_edit_str_check_from_to(s, f, t);
    ui_edit_str_check(s);
    const int32_t bytes_to_remove = s->g2b[t] - s->g2b[f];
    assert(bytes_to_remove >= 0);
    if (bytes_to_remove > 0) {
        ok = ui_edit_str_move_to_heap(s, s->b);
        if (ok) {
            const int32_t bytes_to_shift = s->b - s->g2b[t];
            assert(0 <= bytes_to_shift && bytes_to_shift <= s->b);
            memmove(s->u + s->g2b[f], s->u + s->g2b[t], bytes_to_shift);
            if (s->g2b != ui_edit_str_g2b_ascii) {
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
    ui_edit_str_check(s);
    return ok;
}

static bool ui_edit_str_replace(ui_edit_str_t* s,
        int32_t f, int32_t t, const char* u, int32_t b) {
    const int64_t _4_bytes = (int64_t)sizeof(int32_t);
    bool ok = true; // optimistic approach
    ui_edit_str_check_from_to(s, f, t);
    ui_edit_str_check(s);
    ui_edit_str_parameters(u, b);
    // we are inserting "b" bytes and removing "t - f" glyphs
    const int32_t bytes_to_remove = s->g2b[t] - s->g2b[f];
    const int32_t bytes_to_insert = b; // only for readability
    if (b == 0) { // just remove glyphs
        ok = ui_edit_str_remove(s, f, t);
    } else { // remove and insert
        ui_edit_str_t ins = {0};
        // ui_edit_str_init_ro() verifies utf-8 and calculates g2b[]:
        ok = ui_edit_str_init(&ins, u, b, false);
        const int32_t glyphs_to_insert = ins.g; // only for readability
        const int32_t glyphs_to_remove = t - f; // only for readability
        if (ok) {
            const int32_t bytes = s->b + bytes_to_insert - bytes_to_remove;
            assert(ins.g2b != null); // pacify code analysis
            assert(bytes > 0);
            const int32_t c = ut_max(s->b, bytes);
            // keep g2b == ui_edit_str_g2b_ascii as much as possible
            const bool all_ascii = s->g2b == ui_edit_str_g2b_ascii &&
                                   ins.g2b == ui_edit_str_g2b_ascii &&
                                   bytes < countof(ui_edit_str_g2b_ascii) - 1;
            ok = ui_edit_str_move_to_heap(s, c);
            if (ok) {
                if (!all_ascii) {
                    ui_edit_str_move_g2b_to_heap(s);
                }
                // insert ui_edit_str_t "ins" at glyph position "f"
                // reusing ins.u[0..ins.b-1] and ins.g2b[0..ins.g]
                // moving memory using memmove() left to right:
                if (bytes_to_insert <= bytes_to_remove) {
                    memmove(s->u + s->g2b[f] + bytes_to_insert,
                           s->u + s->g2b[f] + bytes_to_remove,
                           s->b - s->g2b[f] - bytes_to_remove);
                    if (all_ascii) {
                        assert(s->g2b == ui_edit_str_g2b_ascii);
                    } else {
                        assert(s->g2b != ui_edit_str_g2b_ascii);
                        memmove(s->g2b + f + glyphs_to_insert,
                               s->g2b + f + glyphs_to_remove,
                               (s->g - t + 1) * _4_bytes);
                    }
                    memmove(s->u + s->g2b[f], ins.u, ins.b);
                } else {
                    if (all_ascii) {
                        assert(s->g2b == ui_edit_str_g2b_ascii);
                    } else {
                        assert(s->g2b != ui_edit_str_g2b_ascii);
                        const int32_t g = s->g + glyphs_to_insert -
                                                 glyphs_to_remove;
                        assert(g > s->g);
                        ok = ut_heap.realloc(&s->g2b, (g + 1) * _4_bytes) == 0;
                    }
                    // need to shift bytes staring with s.g2b[t] toward the end
                    if (ok) {
                        memmove(s->u + s->g2b[f] + bytes_to_insert,
                                s->u + s->g2b[f] + bytes_to_remove,
                                s->b - s->g2b[f] - bytes_to_remove);
                        if (all_ascii) {
                            assert(s->g2b == ui_edit_str_g2b_ascii);
                        } else {
                            assert(s->g2b != ui_edit_str_g2b_ascii);
                            memmove(s->g2b + f + glyphs_to_insert,
                                    s->g2b + f + glyphs_to_remove,
                                    (s->g - t + 1) * _4_bytes);
                        }
                        memmove(s->u + s->g2b[f], ins.u, ins.b);
                    }
                }
                if (ok) {
                    if (!all_ascii) {
                        assert(s->g2b != ui_edit_str_g2b_ascii);
                        for (int32_t i = f; i <= f + glyphs_to_insert; i++) {
                            s->g2b[i] = ins.g2b[i - f] + s->g2b[f];
                        }
                    } else {
                        assert(s->g2b == ui_edit_str_g2b_ascii);
                        for (int32_t i = f; i <= f + glyphs_to_insert; i++) {
                            assert(ui_edit_str_g2b_ascii[i] == i);
                            assert(ins.g2b[i - f] + s->g2b[f] == i);
                        }
                    }
                    s->b += bytes_to_insert - bytes_to_remove;
                    s->g += glyphs_to_insert - glyphs_to_remove;
                    assert(s->b == bytes);
                    if (!all_ascii) {
                        assert(s->g2b != ui_edit_str_g2b_ascii);
                        for (int32_t i = f + glyphs_to_insert + 1; i <= s->g; i++) {
                            s->g2b[i] += bytes_to_insert - bytes_to_remove;
                        }
                        s->g2b[s->g] = s->b;
                    } else {
                        assert(s->g2b == ui_edit_str_g2b_ascii);
                        for (int32_t i = f + glyphs_to_insert + 1; i <= s->g; i++) {
                            assert(s->g2b[i] == i);
                            assert(ui_edit_str_g2b_ascii[i] == i);
                        }
                        assert(s->g2b[s->g] == s->b);
                    }
                }
            }
            ui_edit_str_free(&ins);
        }
    }
    ui_edit_str_shrink(s);
    ui_edit_str_check(s);
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

static void ui_edit_str_test_replace(void) { // exhaustive permutations
    // Exhaustive 9,765,625 replace permutations may take
    // up to 5 minutes of CPU time in release.
    // Recommended to be invoked at least once after making any
    // changes to ui_edit_str.replace and around.
    // Menu: Debug / Windows / Show Diagnostic Tools allows to watch
    //       memory pressure for whole 3 minutes making sure code is
    //       not leaking memory profusely.
    const char* gs[] = { // glyphs
        "", ui_edit_usd, ui_edit_gbp, ui_edit_euro, ui_edit_money_bag
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
        ui_edit_str_t s = {0};
        // reference constructor does not copy to heap:
        bool ok = ui_edit_str_init(&s, src, -1, false);
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
                    ui_edit_str_t c = {0}; // copy
                    ok = ui_edit_str_init(&c, src, -1, true);
                    swear(ok);
                    ok = ui_edit_str_replace(&c, f, t, rep, -1);
                    swear(ok);
                    swear(memcmp(c.u, e1, c.b) == 0,
                           "s.u[%d:%d]: \"%.*s\" g:%d [%d:%d] rep=\"%s\" "
                           "expected: \"%s\"",
                           s.b, s.c, s.b, s.u, s.g,
                           f, t, rep, e1);
                    ui_edit_str_free(&c);
                }
            }
        }
        ui_edit_str_free(&s);
    }
}

static void ui_edit_str_test_glyph_bytes(void) {
    #pragma push_macro("glyph_bytes_test")
    #define glyph_bytes_test(s, b, expectancy) \
        swear(ui_edit_str_utf8_bytes(s, b) == expectancy)
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

static void ui_edit_str_test(void) {
    ui_edit_str_test_glyph_bytes();
    {
        ui_edit_str_t s = {0};
        bool ok = ui_edit_str_init(&s, "hello", -1, false);
        swear(ok);
        swear(s.b == 5 && s.c == 0 && memcmp(s.u, "hello", 5) == 0);
        swear(s.g == 5 && s.g2b != null);
        for (int32_t i = 0; i <= s.g; i++) {
            swear(s.g2b[i] == i);
        }
        ui_edit_str_free(&s);
    }
    const char* currencies = ui_edit_usd  ui_edit_gbp
                             ui_edit_euro ui_edit_money_bag;
    const char* money = currencies;
    {
        ui_edit_str_t s = {0};
        const int32_t n = (int32_t)strlen(currencies);
        bool ok = ui_edit_str_init(&s, money, n, true);
        swear(ok);
        swear(s.b == n && s.c == s.b && memcmp(s.u, money, s.b) == 0);
        swear(s.g == 4 && s.g2b != null);
        const int32_t g2b[] = {0, 1, 3, 6, 10};
        for (int32_t i = 0; i <= s.g; i++) {
            swear(s.g2b[i] == g2b[i]);
        }
        ui_edit_str_free(&s);
    }
    {
        ui_edit_str_t s = {0};
        bool ok = ui_edit_str_init(&s, "hello", -1, false);
        swear(ok);
        ok = ui_edit_str_replace(&s, 1, 4, null, 0);
        swear(ok);
        swear(s.b == 2 && memcmp(s.u, "ho", 2) == 0);
        swear(s.g == 2 && s.g2b[0] == 0 && s.g2b[1] == 1 && s.g2b[2] == 2);
        ui_edit_str_free(&s);
    }
    {
        ui_edit_str_t s = {0};
        bool ok = ui_edit_str_init(&s, "Hello world", -1, false);
        swear(ok);
        ok = ui_edit_str_replace(&s, 5, 6, " cruel ", -1);
        swear(ok);
        ok = ui_edit_str_replace(&s, 0, 5, "Goodbye", -1);
        swear(ok);
        ok = ui_edit_str_replace(&s, s.g - 5, s.g, "Universe", -1);
        swear(ok);
        swear(s.g == 22 && s.g2b[0] == 0 && s.g2b[s.g] == s.b);
        for (int32_t i = 1; i < s.g; i++) {
            swear(s.g2b[i] == i); // because every glyph is ASCII
        }
        swear(memcmp(s.u, "Goodbye cruel Universe", 22) == 0);
        ui_edit_str_free(&s);
    }
    #ifdef UI_STR_TEST_REPLACE_ALL_PERMUTATIONS
        ui_edit_str_test_replace();
    #else
        (void)(void*)ui_edit_str_test_replace; // mitigate unused warning
    #endif
}

#pragma push_macro("ui_edit_gothic_hwair")
#pragma push_macro("ui_edit_pot_of_honey")
#pragma push_macro("ui_edit_money_bag")
#pragma push_macro("ui_edit_euro")
#pragma push_macro("ui_edit_gbp")
#pragma push_macro("ui_edit_usd")

#pragma pop_macro("ui_edit_str_parameters")
#pragma pop_macro("ui_edit_str_check_empty")
#pragma pop_macro("ui_edit_check_zeros")
#pragma pop_macro("ui_edit_str_check_from_to")
#pragma pop_macro("ui_edit_str_check")

#ifdef UI_EDIT_STR_TEST
    ut_static_init(ui_edit_str) { ui_edit_str.test(); }
#endif

// tests:

static void ui_edit_doc_test_big_text(void) {
    enum { MB10 = 10 * 1000 * 1000 };
    char* text = null;
    ut_heap.alloc(&text, MB10);
    memset(text, 'a', MB10 - 1);
    char* p = text;
    uint32_t seed = 0x1;
    for (;;) {
        int32_t n = ut_num.random32(&seed) % 40 + 40;
        if (p + n >= text + MB10) { break; }
        p += n;
        *p = '\n';
    }
    text[MB10 - 1] = 0x00;
    ui_edit_text_t t = {0};
    bool ok = ui_edit_text.init(&t, text, MB10, false);
    swear(ok);
    ui_edit_text.dispose(&t);
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
            const char* hello = "hello";
            const int32_t n = (int32_t)strlen(hello);
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
            const char* hello = "hello\n";
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
            const char* hello = "hello\nworld";
            const char* world = hello + 6;
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
}

typedef struct ui_edit_doc_test_notify_s {
    ui_edit_notify_t notify;
    int32_t count_before;
    int32_t count_after;
} ui_edit_doc_test_notify_t;

static void ui_edit_doc_test_before(ui_edit_notify_t* n,
        const ui_edit_notify_info_t* unused(ni)) {
    ui_edit_doc_test_notify_t* notify = (ui_edit_doc_test_notify_t*)n;
    notify->count_before++;
}

static void ui_edit_doc_test_after(ui_edit_notify_t* n,
        const ui_edit_notify_info_t* unused(ni)) {
    ui_edit_doc_test_notify_t* notify = (ui_edit_doc_test_notify_t*)n;
    notify->count_after++;
}

static struct {
    ui_edit_notify_t notify;
} ui_edit_doc_test_notify;


static void ui_edit_doc_test_0(void) {
    ui_edit_doc_t edit_doc = {0};
    ui_edit_doc_t* d = &edit_doc;
    swear(ui_edit_doc.init(d, null, 0, false));
    ui_edit_text_t ins_text = {0};
    swear(ui_edit_text.init(&ins_text, "a", 1, false));
    ui_edit_to_do_t undo = {0};
    swear(ui_edit_text.replace(&d->text, null, &ins_text, &undo));
    ui_edit_doc.dispose_to_do(&undo);
    ui_edit_text.dispose(&ins_text);
    ui_edit_doc.dispose(d);
}

static void ui_edit_doc_test_1(void) {
    ui_edit_doc_t edit_doc = {0};
    ui_edit_doc_t* d = &edit_doc;
    swear(ui_edit_doc.init(d, null, 0, false));
    ui_edit_text_t ins_text = {0};
    swear(ui_edit_text.init(&ins_text, "a", 1, false));
    ui_edit_to_do_t undo = {0};
    swear(ui_edit_text.replace(&d->text, null, &ins_text, &undo));
    ui_edit_doc.dispose_to_do(&undo);
    ui_edit_text.dispose(&ins_text);
    ui_edit_doc.dispose(d);
}

static void ui_edit_doc_test_2(void) {
    {   // two string separated by "\n"
        ui_edit_doc_t edit_doc = {0};
        ui_edit_doc_t* d = &edit_doc;
        swear(ui_edit_doc.init(d, null, 0, false));
        ui_edit_notify_t notify1 = {0};
        ui_edit_notify_t notify2 = {0};
        ui_edit_doc_test_notify_t before_and_after = {0};
        before_and_after.notify.before = ui_edit_doc_test_before;
        before_and_after.notify.after  = ui_edit_doc_test_after;
        ui_edit_doc.subscribe(d, &notify1);
        ui_edit_doc.subscribe(d, &before_and_after.notify);
        ui_edit_doc.subscribe(d, &notify2);
        swear(ui_edit_doc.bytes(d, null) == 0, "expected empty");
        const char* hello = "hello\nworld";
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
        swear(ui_edit_doc.init(d, null, 0, false));
        const char* s = "Goodbye" "\n" "Cruel" "\n" "Universe";
        swear(ui_edit_doc.replace(d, null, s, -1));
        ui_edit_text_t t = {0};
        swear(ui_edit_doc.copy_text(d, null, &t));
        ui_edit_text.dispose(&t);
        ui_edit_range_t r = { .from = {.pn = 0, .gp = 4},
                              .to   = {.pn = 2, .gp = 3} };
        swear(ui_edit_doc.replace(d, &r, null, 0));
        swear(d->text.np == 1);
        swear(d->text.ps[0].b == 9);
        swear(d->text.ps[0].g == 9);
        swear(memcmp(d->text.ps[0].u, "Goodverse", 9) == 0);
        swear(ui_edit_doc.replace(d, null, null, 0)); // remove all
        swear(d->text.np == 1);
        swear(d->text.ps[0].b == 0);
        swear(d->text.ps[0].g == 0);
        ui_edit_doc.dispose(d);
    }
    // TODO: "GoodbyeCruelUniverse" insert 2x"\n" splitting in 3 paragraphs
    {
        ui_edit_doc_t edit_doc = {0};
        ui_edit_doc_t* d = &edit_doc;
        const char* ins[] = { "X\nY", "X\n", "\nY", "\n", "X\nY\nZ" };
        for (int32_t i = 0; i < countof(ins); i++) {
            swear(ui_edit_doc.init(d, null, 0, false));
            const char* s = "GoodbyeCruelUniverse";
            swear(ui_edit_doc.replace(d, null, s, -1));
            ui_edit_range_t r = { .from = {.pn = 0, .gp =  7},
                                  .to   = {.pn = 0, .gp = 12} };
            ui_edit_text_t ins_text = {0};
            ui_edit_text.init(&ins_text, ins[i], -1, false);
            ui_edit_to_do_t undo = {0};
            swear(ui_edit_text.replace(&d->text, &r, &ins_text, &undo));
            ui_edit_to_do_t redo = {0};
            swear(ui_edit_text.replace(&d->text, &undo.range, &undo.text, &redo));
            ui_edit_doc.dispose_to_do(&undo);
            undo.range = (ui_edit_range_t){0};
            swear(ui_edit_text.replace(&d->text, &redo.range, &redo.text, &undo));
            ui_edit_doc.dispose_to_do(&redo);
            ui_edit_doc.dispose_to_do(&undo);
            ui_edit_text.dispose(&ins_text);
            ui_edit_doc.dispose(d);
        }
    }
}

static void ui_edit_doc_test_3(void) {
    {
        ui_edit_doc_t edit_doc = {0};
        ui_edit_doc_t* d = &edit_doc;
        ui_edit_doc_test_notify_t before_and_after = {0};
        before_and_after.notify.before = ui_edit_doc_test_before;
        before_and_after.notify.after  = ui_edit_doc_test_after;
        swear(ui_edit_doc.init(d, null, 0, false));
        swear(ui_edit_doc.subscribe(d, &before_and_after.notify));
        const char* s = "Goodbye Cruel Universe";
        const int32_t before = before_and_after.count_before;
        const int32_t after  = before_and_after.count_after;
        swear(ui_edit_doc.replace(d, null, s, -1));
        const int32_t bytes = (int32_t)strlen(s);
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
        ui_edit_doc.copy(d, null, p, utf8bytes);
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
        swear(ui_edit_doc.init(d, null, 0, false));
        const char* s =
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
            ui_edit_doc.copy(d, null, p, utf8bytes);
            swear(p[utf8bytes - 1] == 0x00);
            swear(memcmp(p, s, utf8bytes) == 0);
            ut_heap.free(p);
        }
        ui_edit_doc.dispose(d);
    }
}

static void ui_edit_doc_test_4(void) {
    {
        ui_edit_doc_t edit_doc = {0};
        ui_edit_doc_t* d = &edit_doc;
        swear(ui_edit_doc.init(d, null, 0, false));
        ui_edit_range_t r = {0};
        r = ui_edit_text.end_range(&d->text);
        swear(ui_edit_doc.replace(d, &r, "a", -1));
        r = ui_edit_text.end_range(&d->text);
        swear(ui_edit_doc.replace(d, &r, "\n", -1));
        r = ui_edit_text.end_range(&d->text);
        swear(ui_edit_doc.replace(d, &r, "b", -1));
        r = ui_edit_text.end_range(&d->text);
        swear(ui_edit_doc.replace(d, &r, "\n", -1));
        r = ui_edit_text.end_range(&d->text);
        swear(ui_edit_doc.replace(d, &r, "c", -1));
        r = ui_edit_text.end_range(&d->text);
        swear(ui_edit_doc.replace(d, &r, "\n", -1));
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
    // use n = 10,000,000 and Diagnostic Tools to watch for memory leaks
    enum { n = 1000 };
//  enum { n = 10 * 1000 * 1000 };
    for (int32_t i = 0; i < n; i++) {
        ui_edit_doc_test_0();
        ui_edit_doc_test_1();
        ui_edit_doc_test_2();
        ui_edit_doc_test_3();
        ui_edit_doc_test_4();
    }
}

static const ui_edit_range_t ui_edit_invalid_range = {
    .from = { .pn = -1, .gp = -1},
    .to   = { .pn = -1, .gp = -1}
};

ui_edit_range_if ui_edit_range = {
    .compare       = ui_edit_range_compare,
    .order         = ui_edit_range_order,
    .is_valid      = ui_edit_range_is_valid,
    .is_empty      = ui_edit_range_is_empty,
    .uint64        = ui_edit_range_uint64,
    .pg            = ui_edit_range_pg,
    .inside        = ui_edit_range_inside_text,
    .intersect     = ui_edit_range_intersect,
    .invalid_range = &ui_edit_invalid_range
};

ui_edit_text_if ui_edit_text = {
    .init          = ui_edit_text_init,
    .bytes         = ui_edit_text_bytes,
    .all_on_null   = ui_edit_text_all_on_null,
    .ordered       = ui_edit_text_ordered,
    .end           = ui_edit_text_end,
    .end_range     = ui_edit_text_end_range,
    .dup           = ui_edit_text_dup,
    .equal         = ui_edit_text_equal,
    .copy_text     = ui_edit_text_copy_text,
    .copy          = ui_edit_text_copy,
    .replace       = ui_edit_text_replace,
    .replace_utf8  = ui_edit_text_replace_utf8,
    .dispose       = ui_edit_text_dispose
};

ui_edit_doc_if ui_edit_doc = {
    .init               = ui_edit_doc_init,
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

#pragma push_macro("ui_edit_doc_dump")
#pragma push_macro("ui_edit_text_dump")
#pragma push_macro("ui_edit_range_dump")
#pragma push_macro("ui_edit_pg_dump")
#pragma push_macro("ui_edit_check_range_inside_text")
#pragma push_macro("ui_edit_check_pg_inside_text")
#pragma push_macro("ui_edit_check_zeros")

#ifdef UI_EDIT_DOC_TEST
    ut_static_init(ui_edit_doc) { ui_edit_doc.test(); }
#endif

