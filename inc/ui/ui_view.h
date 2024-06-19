#pragma once
#include "ut/ut_std.h"

begin_c

enum ui_view_type_t {
    ui_view_container = 'vwct',
    ui_view_label     = 'vwlb',
    ui_view_mbx       = 'vwmb',
    ui_view_button    = 'vwbt',
    ui_view_toggle    = 'vwtg',
    ui_view_slider    = 'vwsl',
    ui_view_text      = 'vwtx',
    ui_view_span      = 'vwhs',
    ui_view_list      = 'vwvs',
    ui_view_spacer    = 'vwsp',
    ui_view_scroll    = 'vwsc'
};

typedef struct ui_view_s ui_view_t;

typedef struct ui_view_private_s { // do not access directly
    char text[1024]; // utf8 zero terminated
    int32_t strid;    // 0 for not yet localized, -1 no localization
    fp64_t armed_until; // ut_clock.seconds() - when to release
    fp64_t hover_when;  // time in seconds when to call hovered()
    // use: ui_view.string(v) and ui_view.set_string()
} ui_view_private_t;

typedef struct ui_view_s {
    enum ui_view_type_t type;
    ui_view_private_t p; // private
    void (*init)(ui_view_t* v); // called once before first layout
    ui_view_t* parent;
    ui_view_t* child; // first child, circular doubly linked list
    ui_view_t* prev;  // left or top sibling
    ui_view_t* next;  // right or top sibling
    int32_t x;
    int32_t y;
    int32_t w;
    int32_t h;
    ui_gaps_t insets;
    ui_gaps_t padding;
    int32_t align; // see ui.alignment values
    int32_t max_w; // > 0 maximum width in pixels the view agrees to
    int32_t max_h; // > 0 maximum height in pixels
    fp32_t  min_w_em; // > 0 minimum width  of a view in "em"s
    fp32_t  min_h_em; // > 0 minimum height of a view in "em"s
    ui_icon_t icon; // used instead of text if != null
    // updated on layout() call
    ui_fm_t* fm; // font metrics
    int32_t shortcut; // keyboard shortcut
    void* that;  // for the application use
    void (*notify)(ui_view_t* v, void* p); // for the application use
    // two pass layout: measure() .w, .h layout() .x .y
    // first  measure() bottom up - children.layout before parent.layout
    // second layout() top down - parent.layout before children.layout
    // before methods: called before measure()/layout()/paint()
    void (*prepare)(ui_view_t* v);  // called before measure()
    void (*measure)(ui_view_t* v);  // determine w, h (bottom up)
    void (*measured)(ui_view_t* v); // called after measure()
    void (*layout)(ui_view_t* v);   // set x, y possibly adjust w, h (top down)
    void (*composed)(ui_view_t* v); // after layout() is done (laid out)
    void (*paint)(ui_view_t* v);
    void (*painted)(ui_view_t* v);  // called after paint()
    // composed() is effectively called right before paint() and
    // can be used to prepare for painting w/o need to override paint()
    void (*debug_paint)(ui_view_t* v); // called if .debug is set to true
    // any message:
    bool (*message)(ui_view_t* v, int32_t message, int64_t wp, int64_t lp,
        int64_t* rt); // return true and value in rt to stop processing
    void (*click)(ui_view_t* v);    // ui click callback - view action
    void (*format)(ui_view_t* v);   // format a value to text (e.g. slider)
    void (*callback)(ui_view_t* v); // state change callback
    void (*mouse)(ui_view_t* v, int32_t message, int64_t flags);
    void (*mouse_wheel)(ui_view_t* v, int32_t dx, int32_t dy); // touchpad scroll
    // tap(ui, button_index) press(ui, button_index) see note below
    // button index 0: left, 1: middle, 2: right
    // bottom up (leaves to root or children to parent)
    // return true if consumed (halts further calls up the tree)
    bool (*tap)(ui_view_t* v, int32_t ix);   // single click/tap inside ui
    bool (*press)(ui_view_t* v, int32_t ix); // two finger click/tap or long press
    void (*context_menu)(ui_view_t* v); // right mouse click or long press
    bool (*set_focus)(ui_view_t* v); // returns true if focus is set
    void (*kill_focus)(ui_view_t* v);
    // translated from key pressed/released to utf8:
    void (*character)(ui_view_t* v, const char* utf8);
    void (*key_pressed)(ui_view_t* v, int64_t key);
    void (*key_released)(ui_view_t* v, int64_t key);
    // timer() every_100ms() and every_sec() called
    // even for hidden and disabled views
    void (*timer)(ui_view_t* v, ui_timer_t id);
    void (*every_100ms)(ui_view_t* v); // ~10 x times per second
    void (*every_sec)(ui_view_t* v); // ~once a second
    int64_t (*hit_test)(ui_view_t* v, int32_t x, int32_t y);
    bool hidden; // paint() is not called on hidden
    bool armed;
    bool hover;
    bool pressed;   // for ui_button_t and ui_toggle_t
    bool disabled;  // mouse, keyboard, key_up/down not called on disabled
    bool focusable; // can be target for keyboard focus
    bool flat;      // no-border appearance of views
    bool highlightable; // paint highlight rectangle when hover over label
    ui_color_t color;     // interpretation depends on view type
    int32_t    color_id;  // 0 is default meaning use color
    ui_color_t background;    // interpretation depends on view type
    int32_t    background_id; // 0 is default meaning use background
    bool       debug; // activates debug_paint() called after painted()
    char hint[256]; // tooltip hint text (to be shown while hovering over view)
} ui_view_t;

// tap() / press() APIs guarantee that single tap() is not coming
// before fp64_t tap/click in expense of fp64_t click delay (0.5 seconds)
// which is OK for buttons and many other UI controls but absolutely not
// OK for text editing. Thus edit uses raw mouse events to react
// on clicks and fp64_t clicks.

typedef struct ui_view_if {
    // children va_args must be null terminated
    ui_view_t* (*add)(ui_view_t* parent, ...);
    void (*add_first)(ui_view_t* parent, ui_view_t* child);
    void (*add_last)(ui_view_t* parent,  ui_view_t* child);
    void (*add_after)(ui_view_t* child,  ui_view_t* after);
    void (*add_before)(ui_view_t* child, ui_view_t* before);
    void (*remove)(ui_view_t* v); // removes view from it`s parent
    void (*remove_all)(ui_view_t* parent); // removes all children
    void (*disband)(ui_view_t* parent); // removes all children recursively
    bool (*is_parent_of)(const ui_view_t* p, const ui_view_t* c);
    bool (*inside)(const ui_view_t* v, const ui_point_t* pt);
    ui_ltrb_t (*gaps)(const ui_view_t* v, const ui_gaps_t* g); // gaps to pixels
    void (*inbox)(const ui_view_t* v, ui_rect_t* r, ui_ltrb_t* insets);
    void (*outbox)(const ui_view_t* v, ui_rect_t* r, ui_ltrb_t* padding);
    void (*set_text)(ui_view_t* v, const char* format, ...);
    void (*set_text_va)(ui_view_t* v, const char* format, va_list va);
    void (*invalidate)(const ui_view_t* v); // prone to delays
    bool (*is_hidden)(const ui_view_t* v);   // view or any parent is hidden
    bool (*is_disabled)(const ui_view_t* v); // view or any parent is disabled
    const char* (*string)(ui_view_t* v);  // returns localized text
    void (*timer)(ui_view_t* v, ui_timer_t id);
    void (*every_sec)(ui_view_t* v);
    void (*every_100ms)(ui_view_t* v);
    int64_t (*hit_test)(ui_view_t* v, int32_t x, int32_t y);
    void (*key_pressed)(ui_view_t* v, int64_t v_key);
    void (*key_released)(ui_view_t* v, int64_t v_key);
    void (*character)(ui_view_t* v, const char* utf8);
    void (*paint)(ui_view_t* v);
    bool (*set_focus)(ui_view_t* v);
    void (*kill_focus)(ui_view_t* v);
    void (*kill_hidden_focus)(ui_view_t* v);
    void (*hovering)(ui_view_t* v, bool start);
    void (*mouse)(ui_view_t* v, int32_t m, int64_t f);
    void (*mouse_wheel)(ui_view_t* v, int32_t dx, int32_t dy);
    ui_wh_t (*text_metrics_va)(int32_t x, int32_t y, bool multiline, int32_t w,
        const ui_fm_t* fm, const char* format, va_list va);
    ui_wh_t (*text_metrics)(int32_t x, int32_t y, bool multiline, int32_t w,
        const ui_fm_t* fm, const char* format, ...);
    void (*measure_text)(ui_view_t* v);
    void (*measure_children)(ui_view_t* v);
    void (*layout_children)(ui_view_t* v);
    void (*measure)(ui_view_t* v);
    void (*layout)(ui_view_t* v);
    void (*hover_changed)(ui_view_t* v);
    bool (*is_shortcut_key)(ui_view_t* v, int64_t key);
    bool (*context_menu)(ui_view_t* v);
    bool (*tap)(ui_view_t* v, int32_t ix); // 0: left 1: middle 2: right
    bool (*press)(ui_view_t* v, int32_t ix); // 0: left 1: middle 2: right
    bool (*message)(ui_view_t* v, int32_t m, int64_t wp, int64_t lp,
                                     int64_t* ret);
    void (*debug_paint)(ui_view_t* v);
    void (*test)(void);
} ui_view_if;

extern ui_view_if ui_view;

// view children iterator:

#define ui_view_for_each_begin(v, it) do {       \
    ui_view_t* it = (v)->child;                  \
    if (it != null) {                            \
        do {                                     \


#define ui_view_for_each_end(v, it)              \
            it = it->next;                       \
        } while (it != (v)->child);              \
    }                                            \
} while (0)

#define ui_view_for_each(v, it, ...) \
    ui_view_for_each_begin(v, it)    \
    { __VA_ARGS__ }                  \
    ui_view_for_each_end(v, it)

// #define code(statements) statements
//
// used as:
// {
//     macro({
//        foo();
//        bar();
//     })
// }
//
// except in m4 preprocessor loses new line
// between foo() and bar() and makes debugging and
// using __LINE__ difficult to impossible.
//
// Also
// #define code(...) { __VA_ARGS__ }
// is way easier on preprocessor

// ui_view_insets (fractions of 1/2 to keep float calculations precise):
#define ui_view_i_lr (0.375f)    // 3/4 of "em.w" on left and right
#define ui_view_i_t  (0.109375f) // 7/64 top
#define ui_view_i_b  (0.140625f) // 9/64 bottom

// Most of UI elements are lowercase latin with Capital letter
// to boot. The ascent/descent of the latin fonts lack vertical
// symmetry and thus the top inset is chosen to be a bit smaller
// than the bottom:
//
// i_t + i_b = 16/64 = 1/4
//                   = 5px for 20px font
//                   = 3px for 12px font
// for 20px font i_t + i_b is 1/4 of 20 equal 5px:
// i_t: (int32_t)(20 * 0.109375 + 0.5)   2px
// i_b: (int32_t)(20 * (0.109375f + 0.140625) - i_t + 0.5) 3px
//
// for 12px font i_t + i_b is 1/4 of 12 equal 3px:
// i_t: (int32_t)(12 * 0.109375 + 0.5)   1px for 12px font
// i_b: (int32_t)(12 * (0.109375f + 0.140625) - i_t + 0.5) 2px

// ui_view_padding
#define ui_view_p_l (0.375f)
#define ui_view_p_r (0.375f)
#define ui_view_p_t (0.25f)
#define ui_view_p_b (0.25f)

#define ui_view_call_init(v) do {                   \
    if ((v)->init != null) {                        \
        void (*_init_)(ui_view_t* _v_) = (v)->init; \
        (v)->init = null; /* before! call */        \
        _init_((v));                                \
    }                                               \
} while (0)


end_c
