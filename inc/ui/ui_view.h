#pragma once
#include "rt/rt_std.h"

rt_begin_c

enum ui_view_type_t {
    ui_view_stack     = 'vwst',
    ui_view_label     = 'vwlb',
    ui_view_mbx       = 'vwmb',
    ui_view_button    = 'vwbt',
    ui_view_toggle    = 'vwtg',
    ui_view_slider    = 'vwsl',
    ui_view_image     = 'vwiv',
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
    fp64_t armed_until; // rt_clock.seconds() - when to release
    fp64_t hover_when;  // time in seconds when to call hovered()
    // use: ui_view.string(v) and ui_view.set_string()
} ui_view_private_t;

typedef struct ui_view_text_metrics_s { // ui_view.measure_text() fills these attributes:
    ui_wh_t    wh; // text width and height
    ui_point_t xy; // text offset inside view
    bool multiline; // text contains "\n"
} ui_view_text_metrics_t;

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
    ui_margins_t insets;
    ui_margins_t padding;
    ui_view_text_metrics_t text;
    // see ui.alignment values
    int32_t align; // align inside parent
    int32_t text_align; // align of the text inside control
    int32_t max_w; // > 0 maximum width in pixels the view agrees to
    int32_t max_h; // > 0 maximum height in pixels
    fp32_t  min_w_em; // > 0 minimum width  of a view in "em"s
    fp32_t  min_h_em; // > 0 minimum height of a view in "em"s
    ui_icon_t icon; // used instead of text if != null
    // updated on layout() call
    const ui_fm_t* fm; // font metrics
    int32_t  shortcut; // keyboard shortcut
    void* that;  // for the application use
    void (*notify)(ui_view_t* v, void* p); // for the application use
    // two pass layout: measure() .w, .h layout() .x .y
    // first  measure() bottom up - children.layout before parent.layout
    // second layout() top down - parent.layout before children.layout
    // before methods: called before measure()/layout()/paint()
    void (*prepare)(ui_view_t* v);    // called before measure()
    void (*measure)(ui_view_t* v);    // determine w, h (bottom up)
    void (*measured)(ui_view_t* v);   // called after measure()
    void (*layout)(ui_view_t* v);     // set x, y possibly adjust w, h (top down)
    void (*composed)(ui_view_t* v);   // after layout() is done (laid out)
    void (*erase)(ui_view_t* v);      // called before paint()
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
    void (*mouse_scroll)(ui_view_t* v, ui_point_t dx_dy); // touchpad scroll
    void (*mouse_hover)(ui_view_t* v); // hover over
    void (*mouse_move)(ui_view_t* v);
    void (*double_click)(ui_view_t* v, int32_t ix);
    // tap(ui, button_index) press(ui, button_index) see note below
    // button index 0: left, 1: middle, 2: right
    // bottom up (leaves to root or children to parent)
    // return true if consumed (halts further calls up the tree)
    bool (*tap)(ui_view_t* v, int32_t ix, bool pressed); // single click/tap inside ui
    bool (*long_press)(ui_view_t* v, int32_t ix); // two finger click/tap or long press
    bool (*double_tap)(ui_view_t* v, int32_t ix); // legacy double click
    bool (*context_menu)(ui_view_t* v); // right mouse click or long press
    void (*focus_gained)(ui_view_t* v);
    void (*focus_lost)(ui_view_t* v);
    // translated from key pressed/released to utf8:
    void (*character)(ui_view_t* v, const char* utf8);
    bool (*key_pressed)(ui_view_t* v, int64_t key);  // return true to stop
    bool (*key_released)(ui_view_t* v, int64_t key); // processing
    // timer() every_100ms() and every_sec() called
    // even for hidden and disabled views
    void (*timer)(ui_view_t* v, ui_timer_t id);
    void (*every_100ms)(ui_view_t* v); // ~10 x times per second
    void (*every_sec)(ui_view_t* v); // ~once a second
    int64_t (*hit_test)(const ui_view_t* v, ui_point_t pt);
    struct {
        bool hidden;    // measure()/ layout() paint() is not called on
        bool disabled;  // mouse, keyboard, key_up/down not called on
        bool armed;     // button is pressed but not yet released
        bool hover;     // cursor is hovering over the control
        bool pressed;   // for ui_button_t and ui_toggle_t
    } state;
    // TODO: instead of flat color scheme: undefined colors for
    // border rounded gradient etc.
    bool flat;                // no-border appearance of controls
    bool flip;                // flip button pressed / released
    bool focusable;           // can be target for keyboard focus
    bool highlightable;       // paint highlight rectangle when hover over label
    ui_color_t color;         // interpretation depends on view type
    int32_t    color_id;      // 0 is default meaning use color
    ui_color_t background;    // interpretation depends on view type
    int32_t    background_id; // 0 is default meaning use background
    char hint[256]; // tooltip hint text (to be shown while hovering over view)
    struct {
        struct {
            bool prc; // paint rect
            bool mt;  // measure text
        } trace;
        struct { // after painted():
            bool call;    // v->debug_paint()
            bool margins; // call debug_paint_margins()
            bool fm;      // paint font metrics
        } paint;
        const char* id; // for debugging purposes
    } debug; // debug flags
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
    void (*add_last)(ui_view_t*  parent, ui_view_t* child);
    void (*add_after)(ui_view_t* child,  ui_view_t* after);
    void (*add_before)(ui_view_t* child, ui_view_t* before);
    void (*remove)(ui_view_t* v); // removes view from it`s parent
    void (*remove_all)(ui_view_t* parent); // removes all children
    void (*disband)(ui_view_t* parent); // removes all children recursively
    bool (*is_parent_of)(const ui_view_t* p, const ui_view_t* c);
    bool (*inside)(const ui_view_t* v, const ui_point_t* pt);
    ui_ltrb_t (*margins)(const ui_view_t* v, const ui_margins_t* g); // to pixels
    void (*inbox)(const ui_view_t* v, ui_rect_t* r, ui_ltrb_t* insets);
    void (*outbox)(const ui_view_t* v, ui_rect_t* r, ui_ltrb_t* padding);
    void (*set_text)(ui_view_t* v, const char* format, ...);
    void (*set_text_va)(ui_view_t* v, const char* format, va_list va);
    // ui_view.invalidate() prone to 30ms delays don't use in r/t video code
    // ui_view.invalidate(v, ui_app.crc) invalidates whole client rect but
    // ui_view.redraw() (fast non blocking) is much better instead
    void (*invalidate)(const ui_view_t* v, const ui_rect_t* rect_or_null);
    bool (*is_orphan)(const ui_view_t* v);   // view parent chain has null
    bool (*is_hidden)(const ui_view_t* v);   // view or any parent is hidden
    bool (*is_disabled)(const ui_view_t* v); // view or any parent is disabled
    bool (*is_control)(const ui_view_t* v);
    bool (*is_container)(const ui_view_t* v);
    bool (*is_spacer)(const ui_view_t* v);
    const char* (*string)(ui_view_t* v);  // returns localized text
    void (*timer)(ui_view_t* v, ui_timer_t id);
    void (*every_sec)(ui_view_t* v);
    void (*every_100ms)(ui_view_t* v);
    int64_t (*hit_test)(const ui_view_t* v, ui_point_t pt);
    // key_pressed() key_released() return true to stop further processing
    bool (*key_pressed)(ui_view_t* v, int64_t v_key);
    bool (*key_released)(ui_view_t* v, int64_t v_key);
    void (*character)(ui_view_t* v, const char* utf8);
    void (*paint)(ui_view_t* v);
    bool (*has_focus)(const ui_view_t* v); // ui_app.focused() && ui_app.focus == v
    void (*set_focus)(ui_view_t* view_or_null);
    void (*lose_hidden_focus)(ui_view_t* v);
    void (*hovering)(ui_view_t* v, bool start);
    void (*mouse_hover)(ui_view_t* v); // hover over
    void (*mouse_move)(ui_view_t* v);
    void (*mouse_scroll)(ui_view_t* v, ui_point_t dx_dy); // touchpad scroll
    ui_wh_t (*text_metrics_va)(int32_t x, int32_t y, bool multiline, int32_t w,
        const ui_fm_t* fm, const char* format, va_list va);
    ui_wh_t (*text_metrics)(int32_t x, int32_t y, bool multiline, int32_t w,
        const ui_fm_t* fm, const char* format, ...);
    void (*text_measure)(ui_view_t* v, const char* s,
        ui_view_text_metrics_t* tm);
    void (*text_align)(ui_view_t* v, ui_view_text_metrics_t* tm);
    void (*measure_text)(ui_view_t* v); // fills v->text.mt and .xy
    // measure_control(): control is special case with v->text.mt and .xy
    void (*measure_control)(ui_view_t* v);
    void (*measure_children)(ui_view_t* v);
    void (*layout_children)(ui_view_t* v);
    void (*measure)(ui_view_t* v);
    void (*layout)(ui_view_t* v);
    void (*hover_changed)(ui_view_t* v);
    bool (*is_shortcut_key)(ui_view_t* v, int64_t key);
    bool (*context_menu)(ui_view_t* v);
    // `ix` 0: left 1: middle 2: right
    bool (*tap)(ui_view_t* v, int32_t ix, bool pressed);
    bool (*long_press)(ui_view_t* v, int32_t ix);
    bool (*double_tap)(ui_view_t* v, int32_t ix);
    bool (*message)(ui_view_t* v, int32_t m, int64_t wp, int64_t lp, int64_t* ret);
    void (*debug_paint_margins)(ui_view_t* v); // insets padding
    void (*debug_paint_fm)(ui_view_t* v);   // text font metrics
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

#define ui_view_debug_id(v) \
    ((v)->debug.id != null ? (v)->debug.id : (v)->p.text)

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
#define ui_view_i_lr (0.750f) // 3/4 of "em.w" on left and right
#define ui_view_i_tb (0.125f) // 1/8 em

// ui_view_padding
#define ui_view_p_lr (0.375f)
#define ui_view_p_tb (0.250f)

#define ui_view_call_init(v) do {                   \
    if ((v)->init != null) {                        \
        void (*_init_)(ui_view_t* _v_) = (v)->init; \
        (v)->init = null; /* before! call */        \
        _init_((v));                                \
    }                                               \
} while (0)


rt_end_c
