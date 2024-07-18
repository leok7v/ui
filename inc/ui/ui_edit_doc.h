#pragma once
/* Copyright (c) Dmitry "Leo" Kuznetsov 2021-24 see LICENSE for details */
#include "ut/ut.h"
#include "ui/ui.h"

ut_begin_c

typedef struct ui_edit_str_s ui_edit_str_t;

typedef struct ui_edit_doc_s ui_edit_doc_t;

typedef struct ui_edit_notify_s ui_edit_notify_t;

typedef struct ui_edit_to_do_s ui_edit_to_do_t;

typedef struct ui_edit_pg_s { // page/glyph coordinates
    // humans used to line:column coordinates in text
    int32_t pn; // zero based paragraph number ("line number")
    int32_t gp; // zero based glyph position ("column")
} ui_edit_pg_t;

typedef union ut_begin_packed ui_edit_range_s {
    struct { ui_edit_pg_t from; ui_edit_pg_t to; };
    ui_edit_pg_t a[2];
} ut_end_packed ui_edit_range_t; // "from"[0] "to"[1]

typedef struct ui_edit_text_s {
    int32_t np;   // number of paragraphs
    ui_edit_str_t* ps; // ps[np] paragraphs
} ui_edit_text_t;

typedef struct ui_edit_notify_info_s {
    bool ok; // false if ui_edit.replace() failed (bad utf8 or no memory)
    const ui_edit_doc_t*   const d;
    const ui_edit_range_t* const r; // range to be replaced
    const ui_edit_range_t* const x; // extended range (replacement)
    const ui_edit_text_t*  const t; // replacement text
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
} ui_edit_notify_info_t;

typedef struct ui_edit_notify_s { // called before and after replace()
    void (*before)(ui_edit_notify_t* notify, const ui_edit_notify_info_t* ni);
    // after() is called even if replace() failed with ok: false
    void (*after)(ui_edit_notify_t* notify, const ui_edit_notify_info_t* ni);
} ui_edit_notify_t;

typedef struct ui_edit_listener_s ui_edit_listener_t;

typedef struct ui_edit_listener_s {
    ui_edit_notify_t* notify;
    ui_edit_listener_t* prev;
    ui_edit_listener_t* next;
} ui_edit_listener_t;

typedef struct ui_edit_to_do_s { // undo/redo action
    ui_edit_range_t  range;
    ui_edit_text_t   text;
    ui_edit_to_do_t* next; // inside undo or redo list
} ui_edit_to_do_t;

typedef struct ui_edit_doc_s {
    ui_edit_text_t   text;
    ui_edit_to_do_t* undo; // undo stack
    ui_edit_to_do_t* redo; // redo stack
    ui_edit_listener_t* listeners;
} ui_edit_doc_t;

typedef struct ui_edit_doc_if {
    // init(utf8, bytes, heap:false) must have longer lifetime
    // than document, otherwise use heap: true to copy
    bool    (*init)(ui_edit_doc_t* d, const char* utf8_or_null,
                    int32_t bytes, bool heap);
    bool    (*replace)(ui_edit_doc_t* d, const ui_edit_range_t* r,
                const char* utf8, int32_t bytes);
    int32_t (*bytes)(const ui_edit_doc_t* d, const ui_edit_range_t* range);
    bool    (*copy_text)(ui_edit_doc_t* d, const ui_edit_range_t* range,
                ui_edit_text_t* text); // retrieves range into string
    int32_t (*utf8bytes)(const ui_edit_doc_t* d, const ui_edit_range_t* range);
    // utf8 must be at least ui_edit_doc.utf8bytes()
    void    (*copy)(ui_edit_doc_t* d, const ui_edit_range_t* range,
                char* utf8, int32_t bytes);
    // undo() and push reverse into redo stack
    bool (*undo)(ui_edit_doc_t* d); // false if there is nothing to redo
    // redo() and push reverse into undo stack
    bool (*redo)(ui_edit_doc_t* d); // false if there is nothing to undo
    bool (*subscribe)(ui_edit_doc_t* d, ui_edit_notify_t* notify);
    void (*unsubscribe)(ui_edit_doc_t* d, ui_edit_notify_t* notify);
    void (*dispose_to_do)(ui_edit_to_do_t* to_do);
    void (*dispose)(ui_edit_doc_t* d);
    void (*test)(void);
} ui_edit_doc_if;

extern ui_edit_doc_if ui_edit_doc;

typedef struct ui_edit_range_if {
    int (*compare)(const ui_edit_pg_t pg1, const ui_edit_pg_t pg2);
    ui_edit_range_t (*order)(const ui_edit_range_t r);
    bool            (*is_valid)(const ui_edit_range_t r);
    bool            (*is_empty)(const ui_edit_range_t r);
    uint64_t        (*uint64)(const ui_edit_pg_t pg); // (p << 32 | g)
    ui_edit_pg_t    (*pg)(uint64_t ui64); // p: (ui64 >> 32) g: (int32_t)ui64
    bool            (*inside)(const ui_edit_text_t* t,
                              const ui_edit_range_t r);
    ui_edit_range_t (*intersect)(const ui_edit_range_t r1,
                                 const ui_edit_range_t r2);
    const ui_edit_range_t* const invalid_range; // {{-1,-1},{-1,-1}}
} ui_edit_range_if;

extern ui_edit_range_if ui_edit_range;

typedef struct ui_edit_text_if {
    bool    (*init)(ui_edit_text_t* t, const char* utf, int32_t b, bool heap);

    int32_t (*bytes)(const ui_edit_text_t* t, const ui_edit_range_t* r);
    // end() last paragraph, last glyph in text
    ui_edit_pg_t    (*end)(const ui_edit_text_t* t);
    ui_edit_range_t (*end_range)(const ui_edit_text_t* t);
    ui_edit_range_t (*all_on_null)(const ui_edit_text_t* t,
                                   const ui_edit_range_t* r);
    ui_edit_range_t (*ordered)(const ui_edit_text_t* t,
                               const ui_edit_range_t* r);
    bool    (*dup)(ui_edit_text_t* t, const ui_edit_text_t* s);
    bool    (*equal)(const ui_edit_text_t* t1, const ui_edit_text_t* t2);
    bool    (*copy_text)(const ui_edit_text_t* t, const ui_edit_range_t* range,
                ui_edit_text_t* to);
    void    (*copy)(const ui_edit_text_t* t, const ui_edit_range_t* range,
                char* to, int32_t bytes);
    bool    (*replace)(ui_edit_text_t* t, const ui_edit_range_t* r,
                const ui_edit_text_t* text, ui_edit_to_do_t* undo_or_null);
    bool    (*replace_utf8)(ui_edit_text_t* t, const ui_edit_range_t* r,
                const char* utf8, int32_t bytes, ui_edit_to_do_t* undo_or_null);
    void    (*dispose)(ui_edit_text_t* t);
} ui_edit_text_if;

extern ui_edit_text_if ui_edit_text;

typedef struct ut_begin_packed ui_edit_str_s {
    char* u;    // always correct utf8 bytes not zero terminated(!) sequence
    // s.g2b[s.g + 1] glyph to byte position inside s.u[]
    // s.g2b[0] == 0, s.g2b[s.glyphs] == s.bytes
    int32_t* g2b;  // g2b_0 or heap allocated glyphs to bytes indices
    int32_t  b;    // number of bytes
    int32_t  c;    // when capacity is zero .u is not heap allocated
    int32_t  g;    // number of glyphs
} ut_end_packed ui_edit_str_t;

typedef struct ui_edit_str_if {
    bool (*init)(ui_edit_str_t* s, const char* utf8, int32_t bytes, bool heap);
    void (*swap)(ui_edit_str_t* s1, ui_edit_str_t* s2);
    int32_t (*gp_to_bp)(const char* s, int32_t bytes, int32_t gp); // or -1
    int32_t (*bytes)(ui_edit_str_t* s, int32_t from, int32_t to); // glyphs
    bool (*expand)(ui_edit_str_t* s, int32_t capacity); // reallocate
    void (*shrink)(ui_edit_str_t* s); // get rid of extra heap memory
    bool (*replace)(ui_edit_str_t* s, int32_t from, int32_t to, // glyphs
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
    void (*free)(ui_edit_str_t* s);
    const ui_edit_str_t* const empty;
} ui_edit_str_if;

extern ui_edit_str_if ui_edit_str;

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

    All ui_edit_str_t keep "precise" number of utf8 bytes.
    Caller may allocate extra byte and set it to 0x00
    after retrieving and copying data from ui_edit_str if
    the string content is intended to be used by any
    other API that expects zero terminated strings.
*/


ut_end_c
