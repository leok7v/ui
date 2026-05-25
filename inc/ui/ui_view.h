#pragma once
#include "posix.h"

posix_begin_c

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

struct ui_view;

struct ui_view_private { // do not access directly
    char text[1024]; // utf8 zero terminated
    int32_t strid;    // 0 for not yet localized, -1 no localization
    fp64_t armed_until; // posix_clock.seconds() - when to release
    fp64_t hover_when;  // time in seconds when to call hovered()
    // use: ui_view.string(v) and ui_view.set_string()
};

struct ui_view_text_metrics { // ui_view.measure_text() fills these attributes:
    struct ui_wh    wh; // text width and height
    struct ui_point xy; // text offset inside view
    bool multiline; // text contains "\n"
};

struct ui_view {
    enum ui_view_type_t type;
    struct ui_view_private p; // private
    void (*init)(struct ui_view* v); // called once before first layout
    struct ui_view* parent;
    struct ui_view* child; // first child, circular doubly linked list
    struct ui_view* prev;  // left or top sibling
    struct ui_view* next;  // right or top sibling
    int32_t x;
    int32_t y;
    int32_t w;
    int32_t h;
    struct ui_margins insets;
    struct ui_margins padding;
    struct ui_view_text_metrics text;
    // see ui.alignment values
    int32_t align; // align inside parent
    int32_t text_align; // align of the text inside control
    int32_t max_w; // > 0 maximum width in pixels the view agrees to
    int32_t max_h; // > 0 maximum height in pixels
    fp32_t  min_w_em; // > 0 minimum width  of a view in "em"s
    fp32_t  min_h_em; // > 0 minimum height of a view in "em"s
    ui_icon_t icon; // used instead of text if != null
    // updated on layout() call
    const struct ui_fm* fm; // font metrics
    int32_t  shortcut; // keyboard shortcut
    void* that;  // for the application use
    void (*notify)(struct ui_view* v, void* p); // for the application use
    // two pass layout: measure() .w, .h layout() .x .y
    // first  measure() bottom up - children.layout before parent.layout
    // second layout() top down - parent.layout before children.layout
    // before methods: called before measure()/layout()/paint()
    void (*prepare)(struct ui_view* v);    // called before measure()
    void (*measure)(struct ui_view* v);    // determine w, h (bottom up)
    void (*measured)(struct ui_view* v);   // called after measure()
    void (*layout)(struct ui_view* v);     // set x, y possibly adjust w, h (top down)
    void (*composed)(struct ui_view* v);   // after layout() is done (laid out)
    void (*erase)(struct ui_view* v);      // called before paint()
    void (*paint)(struct ui_view* v);
    void (*painted)(struct ui_view* v);  // called after paint()
    // composed() is effectively called right before paint() and
    // can be used to prepare for painting w/o need to override paint()
    void (*debug_paint)(struct ui_view* v); // called if .debug is set to true
    // any message:
    bool (*message)(struct ui_view* v, int32_t message, int64_t wp, int64_t lp,
        int64_t* rt); // return true and value in rt to stop processing
    void (*click)(struct ui_view* v);    // ui click callback - view action
    void (*format)(struct ui_view* v);   // format a value to text (e.g. slider)
    void (*callback)(struct ui_view* v); // state change callback
    void (*mouse_scroll)(struct ui_view* v, struct ui_point dx_dy); // touchpad scroll
    void (*mouse_hover)(struct ui_view* v); // hover over
    void (*mouse_move)(struct ui_view* v);
    void (*double_click)(struct ui_view* v, int32_t ix);
    // tap(ui, button_index) press(ui, button_index) see note below
    // button index 0: left, 1: middle, 2: right
    // bottom up (leaves to root or children to parent)
    // return true if consumed (halts further calls up the tree)
    bool (*tap)(struct ui_view* v, int32_t ix, bool pressed); // single click/tap inside ui
    bool (*long_press)(struct ui_view* v, int32_t ix); // two finger click/tap or long press
    bool (*double_tap)(struct ui_view* v, int32_t ix); // legacy double click
    bool (*context_menu)(struct ui_view* v); // right mouse click or long press
    void (*focus_gained)(struct ui_view* v);
    void (*focus_lost)(struct ui_view* v);
    // translated from key pressed/released to utf8:
    void (*character)(struct ui_view* v, const char* utf8);
    bool (*key_pressed)(struct ui_view* v, int64_t key);  // return true to stop
    bool (*key_released)(struct ui_view* v, int64_t key); // processing
    // timer() every_100ms() and every_sec() called
    // even for hidden and disabled views
    void (*timer)(struct ui_view* v, ui_timer_t id);
    void (*every_100ms)(struct ui_view* v); // ~10 x times per second
    void (*every_sec)(struct ui_view* v); // ~once a second
    int64_t (*hit_test)(const struct ui_view* v, struct ui_point pt);
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
};

// tap() / press() APIs guarantee that single tap() is not coming
// before fp64_t tap/click in expense of fp64_t click delay (0.5 seconds)
// which is OK for buttons and many other UI controls but absolutely not
// OK for text editing. Thus edit uses raw mouse events to react
// on clicks and fp64_t clicks.

struct ui_view_if {
    // children va_args must be null terminated
    struct ui_view* (*add)(struct ui_view* parent, ...);
    void (*add_first)(struct ui_view* parent, struct ui_view* child);
    void (*add_last)(struct ui_view*  parent, struct ui_view* child);
    void (*add_after)(struct ui_view* child,  struct ui_view* after);
    void (*add_before)(struct ui_view* child, struct ui_view* before);
    void (*remove)(struct ui_view* v); // removes view from it`s parent
    void (*remove_all)(struct ui_view* parent); // removes all children
    void (*disband)(struct ui_view* parent); // removes all children recursively
    bool (*is_parent_of)(const struct ui_view* p, const struct ui_view* c);
    bool (*inside)(const struct ui_view* v, const struct ui_point* pt);
    struct ui_ltrb (*margins)(const struct ui_view* v, const struct ui_margins* g); // to pixels
    void (*inbox)(const struct ui_view* v, struct ui_rect* r, struct ui_ltrb* insets);
    void (*outbox)(const struct ui_view* v, struct ui_rect* r, struct ui_ltrb* padding);
    void (*set_text)(struct ui_view* v, const char* format, ...);
    void (*set_text_va)(struct ui_view* v, const char* format, va_list va);
    // ui_view.invalidate() prone to 30ms delays don't use in r/t video code
    // ui_view.invalidate(v, ui_app.crc) invalidates whole client rect but
    // ui_view.redraw() (fast non blocking) is much better instead
    void (*invalidate)(const struct ui_view* v, const struct ui_rect* rect_or_null);
    bool (*is_orphan)(const struct ui_view* v);   // view parent chain has null
    bool (*is_hidden)(const struct ui_view* v);   // view or any parent is hidden
    bool (*is_disabled)(const struct ui_view* v); // view or any parent is disabled
    bool (*is_control)(const struct ui_view* v);
    bool (*is_container)(const struct ui_view* v);
    bool (*is_spacer)(const struct ui_view* v);
    const char* (*string)(struct ui_view* v);  // returns localized text
    void (*timer)(struct ui_view* v, ui_timer_t id);
    void (*every_sec)(struct ui_view* v);
    void (*every_100ms)(struct ui_view* v);
    int64_t (*hit_test)(const struct ui_view* v, struct ui_point pt);
    // key_pressed() key_released() return true to stop further processing
    bool (*key_pressed)(struct ui_view* v, int64_t v_key);
    bool (*key_released)(struct ui_view* v, int64_t v_key);
    void (*character)(struct ui_view* v, const char* utf8);
    void (*paint)(struct ui_view* v);
    bool (*has_focus)(const struct ui_view* v); // ui_app.focused() && ui_app.focus == v
    void (*set_focus)(struct ui_view* view_or_null);
    void (*lose_hidden_focus)(struct ui_view* v);
    void (*hovering)(struct ui_view* v, bool start);
    void (*mouse_hover)(struct ui_view* v); // hover over
    void (*mouse_move)(struct ui_view* v);
    void (*mouse_scroll)(struct ui_view* v, struct ui_point dx_dy); // touchpad scroll
    struct ui_wh (*text_metrics_va)(int32_t x, int32_t y, bool multiline, int32_t w,
        const struct ui_fm* fm, const char* format, va_list va);
    struct ui_wh (*text_metrics)(int32_t x, int32_t y, bool multiline, int32_t w,
        const struct ui_fm* fm, const char* format, ...);
    void (*text_measure)(struct ui_view* v, const char* s,
        struct ui_view_text_metrics* tm);
    void (*text_align)(struct ui_view* v, struct ui_view_text_metrics* tm);
    void (*measure_text)(struct ui_view* v); // fills v->text.mt and .xy
    // measure_control(): control is special case with v->text.mt and .xy
    void (*measure_control)(struct ui_view* v);
    void (*measure_children)(struct ui_view* v);
    void (*layout_children)(struct ui_view* v);
    void (*measure)(struct ui_view* v);
    void (*layout)(struct ui_view* v);
    void (*hover_changed)(struct ui_view* v);
    bool (*is_shortcut_key)(struct ui_view* v, int64_t key);
    bool (*context_menu)(struct ui_view* v);
    // `ix` 0: left 1: middle 2: right
    bool (*tap)(struct ui_view* v, int32_t ix, bool pressed);
    bool (*long_press)(struct ui_view* v, int32_t ix);
    bool (*double_tap)(struct ui_view* v, int32_t ix);
    bool (*message)(struct ui_view* v, int32_t m, int64_t wp, int64_t lp, int64_t* ret);
    void (*debug_paint_margins)(struct ui_view* v); // insets padding
    void (*debug_paint_fm)(struct ui_view* v);   // text font metrics
    void (*test)(void);
};

extern struct ui_view_if ui_view;

// view children iterator:

#define ui_view_for_each_begin(v, it) do {       \
    struct ui_view* it = (v)->child;                  \
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
        void (*_init_)(struct ui_view* _v_) = (v)->init; \
        (v)->init = null; /* before! call */        \
        _init_((v));                                \
    }                                               \
} while (0)


posix_end_c
