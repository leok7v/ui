#pragma once
/* Copyright (c) Dmitry "Leo" Kuznetsov 2021-24 see LICENSE for details */
#include <stdint.h>

typedef struct ut_begin_packed ui_str_s {
    uint8_t* u;    // always correct utf8 bytes not zero terminated(!) sequence
    // s.g2b[s.g + 1] glyph to byte position inside s.u[]
    // s.g2b[0] == 0, s.g2b[s.glyphs] == s.bytes
    int32_t* g2b;  // g2b_0 or heap allocated glyphs to bytes indices
    int32_t  b;    // number of bytes
    int32_t  c;    // when capacity is zero .u is not heap allocated
    int32_t  g;    // number of glyphs
} ut_end_packed ui_str_t;

typedef struct ui_str_if {
    bool (*init)(ui_str_t* s, const uint8_t* utf8, int32_t bytes, bool heap);
    int32_t (*bytes)(ui_str_t* s, int32_t from, int32_t to); // glyphs
    bool (*expand)(ui_str_t* s, int32_t capacity); // reallocate
    void (*shrink)(ui_str_t* s); // get rid of extra heap memory
    bool (*replace)(ui_str_t* s, int32_t from, int32_t to,
                    const uint8_t* utf8, int32_t bytes); // [from..to[ exclusive
    void (*test)(void);
    void (*free)(ui_str_t* s);
} ui_str_if;

extern ui_str_if ui_str;

/*
    For caller convenience the bytes parameter in all calls can be set
    to -1 for zero terminated utf8 strings which results in treating
    strlen(utf8) as number of bytes.

    ui_str.init()
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

    ui_str.bytes()
            returns number of bytes in utf8 string in the exclusive
            range [from..to[ between string glyphs.

    ui_str.replace()
            replaces utf8 string in the exclusive range [from..to[
            with the new utf8 string. The new string may be longer
            or shorter than the replaced string. The function returns
            false if the new string is invalid utf8 sequence or
            heap allocation fails. The called must ensure that the
            range [from..to[ is valid, failure to do so is a fatal
            error. ui_str.replace() moves string content to the heap.

    ui_str.free()
            deallocates all heap allocated memory and zero out string
            struct. It is incorrect to call free() on the string that
            was not initialized or already freed.

    All ui_str_t keep "precise" number of utf8 bytes.
    Caller may allocate extra byte and set it to 0x00
    after retrieving and copying data from ui_str if
    the string content is intended to be used by any
    other API that expects zero terminated strings.
*/
