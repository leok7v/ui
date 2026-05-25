#pragma once
/* Copyright (c) Dmitry "Leo" Kuznetsov 2021-24 see LICENSE for details */
#include "posix/posix.h"
#include "ui/ui.h"

posix_begin_c

struct ui_edit_str;

struct ui_edit_doc;

struct ui_edit_notify;

struct ui_edit_to_do;

struct ui_edit_pg { // page/glyph coordinates
    // humans used to line:column coordinates in text
    int32_t pn; // zero based paragraph number ("line number")
    int32_t gp; // zero based glyph position ("column")
};

union posix_begin_packed ui_edit_range {
    struct { struct ui_edit_pg from; struct ui_edit_pg to; };
    struct ui_edit_pg a[2];
} posix_end_packed; // "from"[0] "to"[1]

struct ui_edit_text {
    int32_t np;   // number of paragraphs
    struct ui_edit_str* ps; // ps[np] paragraphs
};

struct ui_edit_notify_info {
    bool ok; // false if ui_edit_view.replace() failed (bad utf8 or no memory)
    const struct ui_edit_doc*   const d;
    const union ui_edit_range* const r; // range to be replaced
    const union ui_edit_range* const x; // extended range (replacement)
    const struct ui_edit_text*  const t; // replacement text
    // d->text.np number of paragraphs may change after replace
    // before/after: [pnf..pnt] is inside [0..d->text.np-1]
    int32_t const pnf; // paragraph number from
    int32_t const pnt; // paragraph number to. (inclusive)
    // one can safely assume that ps[pnf] was modified
    // except empty range replace with empty text (which shouldn't be)
    // d->text.ps[pnf..pnf + deleted] were deleted
    // d->text.ps[pnf..pnf + inserted] were inserted
    int32_t const deleted;  // number of deleted  paragraphs (before: 0)
    int32_t const inserted; // paragraph inserted paragraphs (before: 0)
};

struct ui_edit_notify { // called before and after replace()
    void (*before)(struct ui_edit_notify* notify, const struct ui_edit_notify_info* ni);
    // after() is called even if replace() failed with ok: false
    void (*after)(struct ui_edit_notify* notify, const struct ui_edit_notify_info* ni);
};

struct ui_edit_listener;

struct ui_edit_listener {
    struct ui_edit_notify* notify;
    struct ui_edit_listener* prev;
    struct ui_edit_listener* next;
};

struct ui_edit_to_do { // undo/redo action
    union ui_edit_range  range;
    struct ui_edit_text   text;
    struct ui_edit_to_do* next; // inside undo or redo list
};

struct ui_edit_doc {
    struct ui_edit_text   text;
    struct ui_edit_to_do* undo; // undo stack
    struct ui_edit_to_do* redo; // redo stack
    struct ui_edit_listener* listeners;
};

struct ui_edit_doc_if {
    // init(utf8, bytes, heap:false) must have longer lifetime
    // than document, otherwise use heap: true to copy
    bool    (*init)(struct ui_edit_doc* d, const char* utf8_or_null,
                    int32_t bytes, bool heap);
    bool    (*replace)(struct ui_edit_doc* d, const union ui_edit_range* r,
                const char* utf8, int32_t bytes);
    int32_t (*bytes)(const struct ui_edit_doc* d, const union ui_edit_range* range);
    bool    (*copy_text)(struct ui_edit_doc* d, const union ui_edit_range* range,
                struct ui_edit_text* text); // retrieves range into string
    int32_t (*utf8bytes)(const struct ui_edit_doc* d, const union ui_edit_range* range);
    // utf8 must be at least ui_edit_doc.utf8bytes()
    void    (*copy)(struct ui_edit_doc* d, const union ui_edit_range* range,
                char* utf8, int32_t bytes);
    // undo() and push reverse into redo stack
    bool (*undo)(struct ui_edit_doc* d); // false if there is nothing to redo
    // redo() and push reverse into undo stack
    bool (*redo)(struct ui_edit_doc* d); // false if there is nothing to undo
    bool (*subscribe)(struct ui_edit_doc* d, struct ui_edit_notify* notify);
    void (*unsubscribe)(struct ui_edit_doc* d, struct ui_edit_notify* notify);
    void (*dispose_to_do)(struct ui_edit_to_do* to_do);
    void (*dispose)(struct ui_edit_doc* d);
    void (*test)(void);
};

extern struct ui_edit_doc_if ui_edit_doc;

struct ui_edit_range_if {
    int (*compare)(const struct ui_edit_pg pg1, const struct ui_edit_pg pg2);
    union ui_edit_range (*order)(const union ui_edit_range r);
    bool            (*is_valid)(const union ui_edit_range r);
    bool            (*is_empty)(const union ui_edit_range r);
    uint64_t        (*uint64)(const struct ui_edit_pg pg); // (p << 32 | g)
    struct ui_edit_pg    (*pg)(uint64_t ui64); // p: (ui64 >> 32) g: (int32_t)ui64
    bool            (*inside)(const struct ui_edit_text* t,
                              const union ui_edit_range r);
    union ui_edit_range (*intersect)(const union ui_edit_range r1,
                                 const union ui_edit_range r2);
    const union ui_edit_range* const invalid_range; // {{-1,-1},{-1,-1}}
};

extern struct ui_edit_range_if ui_edit_range;

struct ui_edit_text_if {
    bool    (*init)(struct ui_edit_text* t, const char* utf, int32_t b, bool heap);

    int32_t (*bytes)(const struct ui_edit_text* t, const union ui_edit_range* r);
    // end() last paragraph, last glyph in text
    struct ui_edit_pg    (*end)(const struct ui_edit_text* t);
    union ui_edit_range (*end_range)(const struct ui_edit_text* t);
    union ui_edit_range (*all_on_null)(const struct ui_edit_text* t,
                                   const union ui_edit_range* r);
    union ui_edit_range (*ordered)(const struct ui_edit_text* t,
                               const union ui_edit_range* r);
    bool    (*dup)(struct ui_edit_text* t, const struct ui_edit_text* s);
    bool    (*equal)(const struct ui_edit_text* t1, const struct ui_edit_text* t2);
    bool    (*copy_text)(const struct ui_edit_text* t, const union ui_edit_range* range,
                struct ui_edit_text* to);
    void    (*copy)(const struct ui_edit_text* t, const union ui_edit_range* range,
                char* to, int32_t bytes);
    bool    (*replace)(struct ui_edit_text* t, const union ui_edit_range* r,
                const struct ui_edit_text* text, struct ui_edit_to_do* undo_or_null);
    bool    (*replace_utf8)(struct ui_edit_text* t, const union ui_edit_range* r,
                const char* utf8, int32_t bytes, struct ui_edit_to_do* undo_or_null);
    void    (*dispose)(struct ui_edit_text* t);
};

extern struct ui_edit_text_if ui_edit_text;

struct posix_begin_packed ui_edit_str {
    char* u;    // always correct utf8 bytes not zero terminated(!) sequence
    // s.g2b[s.g + 1] glyph to byte position inside s.u[]
    // s.g2b[0] == 0, s.g2b[s.glyphs] == s.bytes
    int32_t* g2b;  // g2b_0 or heap allocated glyphs to bytes indices
    int32_t  b;    // number of bytes
    int32_t  c;    // when capacity is zero .u is not heap allocated
    int32_t  g;    // number of glyphs
} posix_end_packed;

struct ui_edit_str_if {
    bool (*init)(struct ui_edit_str* s, const char* utf8, int32_t bytes, bool heap);
    void (*swap)(struct ui_edit_str* s1, struct ui_edit_str* s2);
    int32_t (*gp_to_bp)(const char* s, int32_t bytes, int32_t gp); // or -1
    int32_t (*bytes)(struct ui_edit_str* s, int32_t from, int32_t to); // glyphs
    bool (*expand)(struct ui_edit_str* s, int32_t capacity); // reallocate
    void (*shrink)(struct ui_edit_str* s); // get rid of extra heap memory
    bool (*replace)(struct ui_edit_str* s, int32_t from, int32_t to, // glyphs
                    const char* utf8, int32_t bytes); // [from..to[ exclusive
    bool (*is_zwj)(uint32_t utf32); // zero width joiner
    bool (*is_letter)(uint32_t utf32); // in European Alphabets
    bool (*is_digit)(uint32_t utf32);
    bool (*is_symbol)(uint32_t utf32);
    bool (*is_alphanumeric)(uint32_t utf32);
    bool (*is_blank)(uint32_t utf32); // white space
    bool (*is_punctuation)(uint32_t utf32);
    bool (*is_combining)(uint32_t utf32);
    bool (*is_spacing)(uint32_t utf32); // spacing modifiers
    bool (*is_cjk_or_emoji)(uint32_t utf32);
    bool (*can_break)(uint32_t cp1, uint32_t cp2);
    void (*test)(void);
    void (*free)(struct ui_edit_str* s);
    const struct ui_edit_str* const empty;
};

extern struct ui_edit_str_if ui_edit_str;

/*
    For caller convenience the bytes parameter in all calls can be set
    to -1 for zero terminated utf8 strings which results in treating
    strlen(utf8) as number of bytes.

    ui_edit_str.init()
            initializes not zero terminated utf8 string that may be
            allocated on the heap or point out to an outside memory
            location that should have longer lifetime and will be
            treated as read only. init() may return false if
            heap.alloc() returns null or the utf8 bytes sequence
            is invalid.
            s.b is number of bytes in the initialized string;
            s.c is set to heap allocated capacity is set to zero
            for strings that are not allocated on the heap;
            s.g is number of the utf8 glyphs (aka Unicode codepoints)
            in the string;
            s.g2b[] is an array of s.g + 1 integers that maps glyph
            positions to byte positions in the utf8 string. The last
            element is number of bytes in the s.u memory.
            Called must zero out the string struct before calling init().

    ui_edit_str.bytes()
            returns number of bytes in utf8 string in the exclusive
            range [from..to[ between string glyphs.

    ui_edit_str.replace()
            replaces utf8 string in the exclusive range [from..to[
            with the new utf8 string. The new string may be longer
            or shorter than the replaced string. The function returns
            false if the new string is invalid utf8 sequence or
            heap allocation fails. The called must ensure that the
            range [from..to[ is valid, failure to do so is a fatal
            error. ui_edit_str.replace() moves string content to the heap.

    ui_edit_str.free()
            deallocates all heap allocated memory and zero out string
            struct. It is incorrect to call free() on the string that
            was not initialized or already freed.

    All struct ui_edit_str keep "precise" number of utf8 bytes.
    Caller may allocate extra byte and set it to 0x00
    after retrieving and copying data from ui_edit_str if
    the string content is intended to be used by any
    other API that expects zero terminated strings.
*/


posix_end_c
