#pragma once
/* Copyright (c) Dmitry "Leo" Kuznetsov 2021-24 see LICENSE for details */
#include <stdbool.h>
#include <stdint.h>

begin_c

// important ui_edit_t will refuse to layout into a box smaller than
// width 3 x em.x height 1 x em.y

typedef struct ui_edit_run_s {
    int32_t bp;     // position in bytes  since start of the paragraph
    int32_t gp;     // position in glyphs since start of the paragraph
    int32_t bytes;  // number of bytes in this `run`
    int32_t glyphs; // number of glyphs in this `run`
    int32_t pixels; // width in pixels
} ui_edit_run_t;

// ui_edit_para_t.initially text will point to readonly memory
// with .allocated == 0; as text is modified it is copied to
// heap and reallocated there.

typedef struct ui_edit_para_s { // "paragraph"
    char* text;          // text[bytes] utf-8
    int32_t capacity;   // if != 0 text copied to heap allocated bytes
    int32_t bytes;       // number of bytes in utf-8 text
    int32_t glyphs;      // number of glyphs in text <= bytes
    int32_t runs;        // number of runs in this paragraph
    ui_edit_run_t* run; // [runs] array of pointers (heap)
    int32_t* g2b;        // [bytes + 1] glyph to uint8_t positions g2b[0] = 0
    int32_t  g2b_capacity; // number of bytes on heap allocated for g2b[]
} ui_edit_para_t;

typedef struct ui_edit_pg_s { // page/glyph coordinates
    // humans used to line:column coordinates in text
    int32_t pn; // paragraph number ("line number")
    int32_t gp; // glyph position ("column")
} ui_edit_pg_t;

typedef struct ui_edit_pr_s { // page/run coordinates
    int32_t pn; // paragraph number
    int32_t rn; // run number inside paragraph
} ui_edit_pr_t;

typedef struct ui_edit_s ui_edit_t;

typedef struct ui_edit_s {
    ui_view_t view;
    void (*set_font)(ui_edit_t* e, ui_font_t* font); // see notes below (*)
    void (*move)(ui_edit_t* e, ui_edit_pg_t pg); // move caret clear selection
    // replace selected text. If bytes < 0 text is treated as zero terminated
    void (*paste)(ui_edit_t* e, const char* text, int32_t bytes);
    void (*copy)(ui_edit_t* e, char* text, int32_t* bytes); // copy whole text
    void (*copy_to_clipboard)(ui_edit_t* e); // selected text to clipboard
    void (*cut_to_clipboard)(ui_edit_t* e);  // copy selected text to clipboard and erase it
    // replace selected text with content of clipboard:
    void (*paste_from_clipboard)(ui_edit_t* e);
    void (*select_all)(ui_edit_t* e); // select whole text
    void (*erase)(ui_edit_t* e); // delete selected text
    // keyboard actions dispatcher:
    void (*key_down)(ui_edit_t* e);
    void (*key_up)(ui_edit_t* e);
    void (*key_left)(ui_edit_t* e);
    void (*key_right)(ui_edit_t* e);
    void (*key_pageup)(ui_edit_t* e);
    void (*key_pagedw)(ui_edit_t* e);
    void (*key_home)(ui_edit_t* e);
    void (*key_end)(ui_edit_t* e);
    void (*key_delete)(ui_edit_t* e);
    void (*key_backspace)(ui_edit_t* e);
    void (*key_enter)(ui_edit_t* e);
    // called when ENTER keyboard key is pressed in single line mode
    void (*enter)(ui_edit_t* e);
    // fuzzer test:
    void (*fuzz)(ui_edit_t* e);      // start/stop fuzzing test
    void (*next_fuzz)(ui_edit_t* e); // next fuzz input event(s)
    ui_edit_pg_t selection[2]; // from selection[0] to selection[1]
    ui_point_t caret; // (-1, -1) off
    ui_edit_pr_t scroll; // left top corner paragraph/run coordinates
    int32_t last_x;    // last_x for up/down caret movement
    int32_t mouse;     // bit 0 and bit 1 for LEFT and RIGHT buttons down
    int32_t top;       // y coordinate of the top of view
    int32_t bottom;    // '' (ditto) of the bottom
    // number of fully (not partially clipped) visible `runs' from top to bottom:
    int32_t visible_runs;
    bool focused;  // is focused and created caret
    bool ro;       // Read Only
    bool sle;      // Single Line Edit
    int32_t shown; // debug: caret show/hide counter 0|1
    // https://en.wikipedia.org/wiki/Fuzzing
    volatile thread_t fuzzer;     // fuzzer thread != null when fuzzing
    volatile int32_t  fuzz_count; // fuzzer event count
    volatile int32_t  fuzz_last;  // last processed fuzz
    volatile bool     fuzz_quit;  // last processed fuzz
    // random32 starts with 1 but client can seed it with (clock.nanoseconds() | 1)
    uint32_t fuzz_seed;    // fuzzer random32 seed (must start with odd number)
    // paragraphs memory:
    int32_t capacity;      // number of bytes allocated for `para` array below
    int32_t paragraphs;    // number of lines in the text
    ui_edit_para_t* para; // para[paragraphs]
} ui_edit_t;

/*
    Notes:
    set_font() - neither edit.view.font = font nor measure()/layout() functions
                 do NOT dispose paragraphs layout unless geometry changed because
                 it is quite expensive operation. But choosing different font
                 on the fly needs to re-layout all paragraphs. Thus caller needs
                 to set font via this function instead which also requests
                 edit UI element re-layout.

    .ro        - readonly edit->ro is used to control readonly mode.
                 If edit control is readonly its appearance does not change but it
                 refuses to accept any changes to the rendered text.

    .wb        - wordbreak this attribute was removed as poor UX human experience
                 along with single line scroll editing. See note below about .sle.

    .sle       - Single line edit control.
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
                 is distributed evenly between single line of text and top bottom gaps.
                 IMPORTANT: SLE resizes itself vertically to accommodate for
                 input that is too wide. If caller wants to limit vertical space it
                 will need to hook .measure() function of SLE and do the math there.
*/

void ui_edit_init(ui_edit_t* e);

end_c
