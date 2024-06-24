/* Copyright (c) Dmitry "Leo" Kuznetsov 2021-24 see LICENSE for details */
#include "ut/ut.h"
#include "ui/ui.h"

#pragma push_macro("ui_edit_str_check_zeros")


#ifdef DEBUG

// ui_edit_str_check_zeros only works for packed structs:

#define ui_edit_str_check_zeros(a_, b_) do {                            \
    for (int32_t i_ = 0; i_ < (b_); i_++) {                             \
        assert(((const uint8_t*)(a_))[i_] == 0x00);                     \
    }                                                                   \
} while (0)

#else

#define ui_edit_str_check_zeros(a, b)      do { } while (0)

#endif


typedef struct ui_edit_text_s ui_edit_text_t;

typedef struct ui_edit_notify_s ui_edit_notify_t;

typedef struct ui_edit_replace_s ui_edit_replace_t;

typedef struct ui_edit_replace_s {
    ui_edit_replace_t* prev;
    ui_edit_replace_t* next;
     // text replaced by paste or erased by cut
    const ui_edit_range_t* range; // == null: entire document
    ui_str_t text;
    bool can_coalesce; // can be coalesced with previous operation
} ui_edit_replace_t;

typedef struct ui_edit_notify_s {
    void (*before)(ui_edit_notify_t* notify,
                   const ui_edit_text_t* d, const ui_edit_range_t* range);
    void (*after)(ui_edit_notify_t* notify,
                  const ui_edit_text_t* d, const ui_edit_range_t* range);
} ui_edit_notify_t;

typedef struct ui_edit_observer_s ui_edit_observer_t;

typedef struct ui_edit_observer_s {
    ui_edit_notify_t* notify;
    ui_edit_observer_t* prev;
    ui_edit_observer_t* next;
} ui_edit_observer_t;

typedef struct ui_edit_text_s {
    // replaces the range with text and pushes
    // undo ui_edit_replace_t struct into undo/redo stack
    // possibly coalescing it with previous replace
    void (*replace)(ui_edit_text_t* t, const ui_edit_range_t* range,
                    const ui_str_t* utf8, bool can_coalesce);
    int32_t (*bytes)(const ui_edit_text_t* t, const ui_edit_range_t* range);
    void (*copy)(ui_edit_text_t* t, const ui_edit_range_t* range,
                 ui_str_t* utf8); // retrieves range into string
    // redo() performs the operation pushes it into stack
    void (*redo)(ui_edit_text_t* t);
    // undo() pops operation from stack and undo it
    void (*undo)(ui_edit_text_t* t);
    void (*add)(ui_edit_text_t* t, ui_edit_notify_t* notify);
    void (*remove)(ui_edit_text_t* t, ui_edit_notify_t* notify);
    struct { // private: implementation
        int32_t capacity;       // number of bytes allocated for `para` array below
        int32_t paragraphs;     // number of lines in the text
        ui_edit_para_t* para;   // para[paragraphs]
        ui_edit_replace_t* head;  // points position on stack
        ui_edit_replace_t* stack; // heap allocated stack of saved operations
        ui_edit_observer_t* observers;
    } p;
} ui_edit_text_t;

typedef struct ui_edit_text_if {
    void (*init)(ui_edit_text_t* t);
    void (*fini)(ui_edit_text_t* t);
    void (*test)(void);
} ui_edit_text_if;

ui_edit_text_if ui_edit_text;

#if 0

static int32_t ui_edit_text_glyphs(const char* utf8, int32_t bytes) { // or -1
    return ui_edit_text_g2b(utf8, bytes, null);
}

static int32_t ui_edit_text_gp_to_bytes(const char* s, int32_t bytes, int32_t gp) {
    int32_t c = 0;
    int32_t i = 0;
    if (bytes > 0) {
        while (c < gp) {
            assert(i < bytes);
            const int32_t b = ui_edit_text_glyph_bytes(s + i, bytes - i);
            swear(b > 0 && i + b <= bytes);
            i += b;
            c++;
        }
    }
    assert(i <= bytes);
    return i;
}

static bool ui_edit_text_paragraph_g2b_ensure(ui_edit_para_t* p, int32_t n) {
    bool ok = true;
    enum { _4_bytes = (int32_t)sizeof(int32_t) };
    assert(p->g2b_capacity % _4_bytes == 0);
    const int64_t bytes = (int64_t)n * _4_bytes;
    assert(bytes < INT32_MAX);
    if (n > p->g2b_capacity / _4_bytes) {
        if (ut_heap.realloc((void**)&p->g2b, bytes) == 0) {
            p->g2b_capacity = (int32_t)bytes;
        } else {
            ok = false;
        }
    }
    return ok;
}

static bool ui_edit_text_paragraph_rebuild_g2b(ui_edit_para_t* p) {
    errno_t r = 0;
    bool ok = true;
    const int32_t bytes = p->bytes;
    const char* utf8 = p->text;
    r = ui_edit_text_paragraph_g2b_ensure(p, 128);
    if (r == 0) {
        p->g2b[0] = 0; // first glyph starts at 0
        int32_t i = 0;
        int32_t k = 1;
        // g2b[k] start postion in uint8_t offset from utf8 text of glyph[k]
        while (i < bytes && ok) {
            const int32_t b = ui_edit_text_glyph_bytes(utf8 + i, bytes - i);
            ok = b > 0 && i + b <= bytes;
            if (!ok) { break; }
            if (k >= p->g2b_capacity / (int32_t)sizeof(int32_t)) {
                ok = ui_edit_text_paragraph_g2b_ensure(p, k + 128) == 0;
            }
            if (!ok) { break; }
            i += b;
            p->g2b[k] = i;
            k++;
        }
        // if !ok the paragraph will be cut short but still correct
        p->glyphs = k - 1;
        p->bytes  = i;
    }
    return ok;
}

static ui_edit_range_t ui_edit_text_range_or_all(const ui_edit_text_t* t,
        const ui_edit_range_t* range) {
    ui_edit_range_t r;
    if (range != null) {
        r = *range;
    } else {
        r.from.pn = 0;
        r.from.gp = 0;
        r.to.pn = t->p.paragraphs;
        r.to.gp = 0;
    }
    return r;
}

static int ui_edit_text_compare_pg(ui_edit_pg_t pg1, ui_edit_pg_t pg2) {
    int64_t d = (((int64_t)pg1.pn << 32) | pg1.gp) -
                (((int64_t)pg2.pn << 32) | pg2.gp);
    return d < 0 ? -1 : d > 0 ? 1 : 0;
}

static ui_edit_range_t ui_edit_text_ordered_range(ui_edit_range_t r) {
    uint64_t f = ((uint64_t)r.from.pn << 32) | r.from.gp;
    uint64_t t = ((uint64_t)r.to.pn   << 32) | r.to.gp;
    if (ui_edit_text_compare_pg(r.from, r.to) > 0) {
        uint64_t swap = t; t = f; f = swap;
        r.from.pn = (int32_t)(f >> 32);
        r.from.gp = (int32_t)(f);
        r.to.pn   = (int32_t)(t >> 32);
        r.to.gp   = (int32_t)(t);
    }
    return r;
}

static ui_edit_replace_t* ui_edit_text_push_undo(ui_edit_text_t* t) {
    ui_edit_replace_t* u = null;
    (void)ut_heap.alloc_zero((void**)&u, sizeof(ui_edit_replace_t));
    if (u != null) {
        u->prev = null;
        u->next = t->p.stack;
        if (t->p.stack != null) { t->p.stack->prev = u; }
        t->p.stack = u;
    }
    return u;
}

static void ui_edit_text_pop_undo(ui_edit_text_t* t) {
    swear(t->p.stack != null);
    ui_edit_replace_t* u = t->p.stack;
    t->p.stack = t->p.stack->next;
    t->p.stack->prev = null;
    if (u->text.capacity > 0) { ut_heap.free(u->text.utf8); }
    ui_edit_text_free(&u);
}

static void ui_edit_text_free_paragraph(ui_edit_para_t* p) {
    if (p->capacity > 0) {
        ui_edit_text_free((void**)&p->text);
    } else {
        p->text = null;
    }
    if (p->g2b_capacity > 0) { ui_edit_text_free((void**)&p->g2b); }
    if (p->runs > 0) { ui_edit_text_free((void**)&p->run); }
}

static void ui_edit_text_cut(ui_edit_text_t* t,
        const ui_edit_range_t r) {
    // first paragraph:
    ui_edit_para_t* p = &t->p.para[r.from.pn];
    int32_t ix0 = p->g2b[r.from.gp];
    int32_t ix1 = r.from.pn == r.to.pn ? p->g2b[r.to.gp] : p->bytes;
    int32_t bytes = ix1 - ix0;
    memcpy(p->text + ix0, p->text + ix1, p->bytes - ix1);
    p->bytes -= bytes;
    // TODO: do NOT need to rebuild whole g2b
    //       optimize with memcpy
    ui_edit_text_paragraph_rebuild_g2b(p);
    // in between paragraphs:
    int32_t deleted = 0;
    for (int32_t pn = r.from.pn + 1; pn <= r.to.pn - 1; pn++) {
        ui_edit_text_free_paragraph(&t->p.para[pn]);
        deleted++;
    }
    if (r.from.pn != r.to.pn) {
        // last paragraph:
        p = &t->p.para[r.to.pn];
        ix0 = 0;
        ix1 = p->g2b[r.to.gp];
        bytes = ix1 - ix0;
        memcpy(p->text + ix0, p->text + ix1, p->bytes - ix1);
        p->bytes -= bytes;
        // TODO: do NOT need to rebuild whole g2b
        //       optimize with memcpy
        ui_edit_text_paragraph_rebuild_g2b(p);
        // remove deleted paragraphs
        if (deleted > 0) {
            memcpy(&t->p.para[r.from.pn + 1],
                   &t->p.para[r.from.pn + 1 + deleted],
                   (t->p.paragraphs - r.to.pn - 1) * sizeof(ui_edit_para_t));
            t->p.paragraphs -= deleted;
        }
    }
}

static errno_t ui_edit_text_insert_instead(ui_edit_text_t* t,
        const ui_edit_range_t r, const ui_str_t* str) {
    (void)t;
    (void)r;
    (void)str;
    return 0;
}

static bool ui_edit_text_replace(ui_edit_text_t* t,
        const ui_edit_range_t* range,
        const ui_str_t* str, bool can_coalesce) {
    errno_t err = 0;
    ui_edit_range_t r = ui_edit_text_ordered_range(
                        ui_edit_text_range_or_all(t, range));
    bool empty_range = ui_edit_text_compare_pg(r.from, r.to) == 0;
    // caller required not to make a call replacing empty range
    // with empty string
    swear(!empty_range || str->bytes != 0);
    // if range .to is past last paragraph
    // append to the end of previous paragraph:
    assert(t->p.paragraphs > 0);
    if (r.to.pn == t->p.paragraphs) {
        assert(r.to.gp == 0);
        r.to.pn = t->p.paragraphs - 1;
        r.to.gp = t->p.para[r.to.pn].glyphs;
        if (empty_range) { r.from = r.to; }
    }
    ui_edit_replace_t* undo = ui_edit_text_push_undo(t);
    if (undo != null) {
        undo->can_coalesce = can_coalesce;
        if (!empty_range) {
            const int32_t bytes = t->bytes(t, &r);
            err = ui_edit_text_alloc((void**)&undo->text.utf8, bytes);
            if (err == 0) {
                undo->text.bytes = bytes;
                undo->text.capacity = bytes;
                t->copy(t, &r, &undo->text);
            } else {
                ui_edit_text_pop_undo(t);
                undo = null;
            }
        } else {
            undo->text.bytes = 0;
            undo->text.capacity = 0;
            undo->text.utf8 = "";
        }
    };
    if (err == 0) {
        if (str->bytes == 0) {
            ui_edit_text_cut(t, r); // always succeeds
        } else {
            err = ui_edit_text_insert_instead(t, r, str);
        }
    }
    if (err != 0 && undo != null) {
        ui_edit_text_pop_undo(t);
    }
    return err == 0;
}


#endif

static bool ui_edit_text_add(ui_edit_text_t* t, ui_edit_notify_t* notify) {
    // TODO: not sure about double linked list.
    // heap allocated resizable array may serve better and may be easier to maintain
    bool ok = true;
    ui_edit_observer_t* o = t->p.observers;
    if (o == null) {
        ok = ut_heap.alloc((void**)&t->p.observers, sizeof(*o)) == 0;
        if (ok) { o = t->p.observers; }
    } else {
        while (o->next != null) { swear(o->notify != notify); o = o->next; }
        ok = ut_heap.alloc((void**)&o->next, sizeof(*o)) == 0;
        if (ok) { o->next->prev = o; o = o->next; }
    }
    if (ok) { o->notify = notify; }
    return ok;
}

static void ui_edit_text_remove(ui_edit_text_t* t, ui_edit_notify_t* notify) {
    ui_edit_observer_t* o = t->p.observers;
    bool removed = false;
    while (o != null) {
        ui_edit_observer_t* n = o->next;
        if (o->notify == notify) {
            assert(!removed);
            if (o->prev != null) { o->prev->next = n; }
            if (o->next != null) { o->next->prev = o->prev; }
            if (o == t->p.observers) { t->p.observers = n; }
            ut_heap.free(o);
            removed = true;
        }
        o = n;
    }
    swear(removed);
}

static int32_t ui_edit_text_bytes(const ui_edit_text_t* t, const ui_edit_range_t* range) {
    (void)t; // TODO: remove
    (void)range; // TODO: remove
    swear(false, "implement me");
    return 0;
}

static bool ui_edit_text_replace(ui_edit_text_t* t,
        const ui_edit_range_t* r,
        const ui_str_t* s, bool can_coalesce) {
    (void)t; // TODO: remove
    (void)r; // TODO: remove
    (void)s; // TODO: remove
    (void)can_coalesce; // TODO: remove
    return true;
}

static void ui_edit_text_redo(ui_edit_text_t* t) {
    (void)t; // TODO: remove
}

static void ui_edit_text_undo(ui_edit_text_t* t) {
    (void)t; // TODO: remove
}

static void ui_edit_text_test(void);

static void ui_edit_text_init(ui_edit_text_t* t) {
    memset(t, 0x00, sizeof(t));
    t->replace = ui_edit_text_replace;
    t->redo   = ui_edit_text_redo;
    t->undo   = ui_edit_text_undo;
    t->bytes  = ui_edit_text_bytes;
    t->add    = ui_edit_text_add;
    t->remove = ui_edit_text_remove;
}

static void ui_edit_text_fini(ui_edit_text_t* t) {
    for (int32_t i = 0; i < t->p.paragraphs; i++) {
        if (t->p.para[i].capacity > 0) {
            ut_heap.free(t->p.para[i].text);
            t->p.para[i].text = null;
            t->p.para[i].capacity = 0;
        }
        if (t->p.para[i].g2b != null) {
            ut_heap.free(t->p.para[i].g2b);
            t->p.para[i].g2b = null;
            t->p.para[i].capacity = 0;
        }
    }
    if (t->p.para != null) {
        ut_heap.free(t->p.para);
        t->p.para = null;
    }
    t->p.para = null;
    t->p.paragraphs = 0;
    t->replace = null;
    t->redo    = null;
    t->undo    = null;
    t->bytes   = null;
    t->add     = null;
    t->remove  = null;
    ui_edit_str_check_zeros(t, sizeof(*t));
//  ut_runtime.exit(0);
}

static void ui_edit_text_test(void) {
if (1) return;
    ui_edit_range_t r = { .from = {0,0}, .to = {0,0} };
    static_assertion(sizeof(r.from) + sizeof(r.from) == sizeof(r.a));
    swear(&r.from == &r.a[0] && &r.to == &r.a[1]);
    ui_edit_text_t edit_text = {0};
    ui_edit_text_t* t = &edit_text;
    ui_edit_notify_t notify1 = {0};
    ui_edit_notify_t notify2 = {0};
    ui_edit_text.init(t);
    swear(t->add != null);
    swear(t->remove != null);
    t->add(t, &notify1);
    t->add(t, &notify2);
/*
    swear(t->bytes(t, null) == 0, "expected empty");
    ui_str_t str = {
        .utf8 = "hello", .bytes = 5, .capacity = 0
    };
    t->replace(t, null, &str, true);
    swear(t->bytes(t, null) == 5, "expected 5 bytes");
    char utf8[6];
    swear(countof(utf8) > t->bytes(t, null));
    str.utf8 = utf8;
    str.bytes = t->bytes(t, null);
    str.capacity = 0; // capacity > 0 only if heap allocated
    t->copy(t, null, &str);
    str.utf8[str.bytes] = 0x00; // zero terminate post copy()
    swear(strcmp(str.utf8, "hello") == 0, "expected 'hello'");
    t->undo(t);
    swear(t->bytes(t, null) == 0, "expected empty text");
*/
    t->remove(t, &notify1);
    t->remove(t, &notify2);
    ui_edit_text.fini(t);
}

ui_edit_text_if ui_edit_text = {
    .init = ui_edit_text_init,
    .fini = ui_edit_text_fini,
    .test = ui_edit_text_test
};

// Uncomment following line for quick and dirty test run:
ut_static_init(ui_edit_text) { ui_edit_text.test(); }

#pragma pop_macro("ui_edit_str_check_zeros")
