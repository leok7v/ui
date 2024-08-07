#pragma once
/* Copyright (c) Dmitry "Leo" Kuznetsov 2021-24 see LICENSE for details */
#include "rt/rt.h"
#include "ui/ui.h"

rt_begin_c

// important ui_edit_view_t will refuse to layout into a box smaller than
// width 3 x fm->em.w height 1 x fm->em.h

typedef struct ui_edit_view_s ui_edit_view_t;

typedef struct ui_edit_str_s ui_edit_str_t;

typedef struct ui_edit_doc_s ui_edit_doc_t;

typedef struct ui_edit_notify_s ui_edit_notify_t;

typedef struct ui_edit_to_do_s ui_edit_to_do_t;

typedef struct ui_edit_pr_s { // page/run coordinates
    int32_t pn; // paragraph number
    int32_t rn; // run number inside paragraph
} ui_edit_pr_t;

typedef struct ui_edit_run_s {
    int32_t bp;     // position in bytes  since start of the paragraph
    int32_t gp;     // position in glyphs since start of the paragraph
    int32_t bytes;  // number of bytes in this `run`
    int32_t glyphs; // number of glyphs in this `run`
    int32_t pixels; // width in pixels
} ui_edit_run_t;

// ui_edit_paragraph_t.initially text will point to readonly memory
// with .allocated == 0; as text is modified it is copied to
// heap and reallocated there.

typedef struct ui_edit_paragraph_s { // "paragraph" view consists of wrapped runs
    int32_t runs;       // number of runs in this paragraph
    ui_edit_run_t* run; // heap allocated array[runs]
} ui_edit_paragraph_t;

typedef struct ui_edit_notify_view_s {
    ui_edit_notify_t notify;
    void*            that; // specific for listener
    uintptr_t        data; // before -> after listener data
} ui_edit_notify_view_t;

typedef struct ui_edit_view_s {
    union {
        ui_view_t view;
        struct ui_view_s;
    };
    ui_edit_doc_t* doc; // document
    ui_edit_notify_view_t listener;
    ui_edit_range_t selection; // "from" selection[0] "to" selection[1]
    ui_point_t caret; // (-1, -1) off
    int32_t caret_width; // in pixels
    ui_edit_pr_t scroll; // left top corner paragraph/run coordinates
    int32_t last_x;    // last_x for up/down caret movement
    ui_ltrb_t inside;  // inside insets space
    struct {
        int32_t w;       // inside.right - inside.left
        int32_t h;       // inside.bottom - inside.top
        int32_t buttons; // bit 0 and bit 1 for LEFT and RIGHT mouse buttons down
    } edit;
    // number of fully (not partially clipped) visible `runs' from top to bottom:
    int32_t visible_runs;
    // TODO: remove focused because it is the same as caret != (-1, -1)
    bool focused;     // is focused and created caret
    bool ro;          // Read Only
    bool sle;         // Single Line Edit
    bool hide_word_wrap; // do not paint word wrap
    int32_t shown;    // debug: caret show/hide counter 0|1
    // paragraphs memory:
    ui_edit_paragraph_t* para; // para[e->doc->text.np]
} ui_edit_view_t;

typedef struct ui_edit_view_if {
    void (*init)(ui_edit_view_t* e, ui_edit_doc_t* d);
    void (*set_font)(ui_edit_view_t* e, ui_fm_t* fm); // see notes below (*)
    void (*move)(ui_edit_view_t* e, ui_edit_pg_t pg); // move caret clear selection
    // replace selected text. If bytes < 0 text is treated as zero terminated
    void (*replace)(ui_edit_view_t* e, const char* text, int32_t bytes);
    // call save(e, null, &bytes) to retrieve number of utf8
    // bytes required to save whole text including 0x00 terminating bytes
    errno_t (*save)(ui_edit_view_t* e, char* text, int32_t* bytes);
    void (*copy)(ui_edit_view_t* e);  // to clipboard
    void (*cut)(ui_edit_view_t* e);   // to clipboard
    // replace selected text with content of clipboard:
    void (*paste)(ui_edit_view_t* e); // from clipboard
    void (*select_all)(ui_edit_view_t* e); // select whole text
    void (*erase)(ui_edit_view_t* e); // delete selected text
    // keyboard actions dispatcher:
    void (*key_down)(ui_edit_view_t* e);
    void (*key_up)(ui_edit_view_t* e);
    void (*key_left)(ui_edit_view_t* e);
    void (*key_right)(ui_edit_view_t* e);
    void (*key_page_up)(ui_edit_view_t* e);
    void (*key_page_down)(ui_edit_view_t* e);
    void (*key_home)(ui_edit_view_t* e);
    void (*key_end)(ui_edit_view_t* e);
    void (*key_delete)(ui_edit_view_t* e);
    void (*key_backspace)(ui_edit_view_t* e);
    void (*key_enter)(ui_edit_view_t* e);
    // called when ENTER keyboard key is pressed in single line mode
    void (*enter)(ui_edit_view_t* e);
    // fuzzer test:
    void (*fuzz)(ui_edit_view_t* e);      // start/stop fuzzing test
    void (*dispose)(ui_edit_view_t* e);
} ui_edit_view_if;

extern ui_edit_view_if ui_edit_view;

/*
    Notes:
    set_font()
        neither edit.view.font = font nor measure()/layout() functions
        do NOT dispose paragraphs layout unless geometry changed because
        it is quite expensive operation. But choosing different font
        on the fly needs to re-layout all paragraphs. Thus caller needs
        to set font via this function instead which also requests
        edit UI element re-layout.

    .ro
        readonly edit->ro is used to control readonly mode.
        If edit control is readonly its appearance does not change but it
        refuses to accept any changes to the rendered text.

    .wb
        wordbreak this attribute was removed as poor UX human experience
        along with single line scroll editing. See note below about .sle.

    .sle
        single line edit control.
        Edit UI element does NOT support horizontal scroll and breaking
        words semantics as it is poor UX human experience. This is not
        how humans (apart of software developers) edit text.
        If content of the edit UI element is wider than the bounding box
        width the content is broken on word boundaries and vertical scrolling
        semantics is supported. Layouts containing edit control of the single
        line height are strongly encouraged to enlarge edit control layout
        vertically on as needed basis similar to Google Search Box behavior
        change implemented in 2023.
        If multiline is set to true by the callers code the edit UI layout
        snaps text to the top of x,y,w,h box otherwise the vertical space
        is distributed evenly between single line of text and top bottom
        margins.
        IMPORTANT: SLE resizes itself vertically to accommodate for
        input that is too wide. If caller wants to limit vertical space it
        will need to hook .measure() function of SLE and do the math there.
*/

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

rt_end_c
