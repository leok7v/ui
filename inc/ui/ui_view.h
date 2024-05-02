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
    ui_view_edit      = 'vwed'
};

typedef struct ui_view_s ui_view_t;

typedef struct ui_view_s {
    enum ui_view_type_t type;
    void (*init)(ui_view_t* view); // called once before first layout
    fp64_t width;    // > 0 width of UI element in "em"s
    char text[2048];
    ui_view_t* parent;
    ui_view_t* child; // first child, circular doubly linked list
    ui_view_t* prev;  // left or top sibling
    ui_view_t* next;  // right or top sibling
    int32_t x;
    int32_t y;
    int32_t w;
    int32_t h;
    // updated on layout() call
    ui_point_t em; // cached pixel dimensions of "M"
    int32_t shortcut; // keyboard shortcut
    int32_t strid; // 0 for not localized ui
    void* that;  // for the application use
    void (*notify)(ui_view_t* view, void* p); // for the application use
    // two pass layout: measure() .w, .h layout() .x .y
    // first  measure() bottom up - children.layout before parent.layout
    // second layout() top down - parent.layout before children.layout
    void (*measure)(ui_view_t* view); // determine w, h (bottom up)
    void (*layout)(ui_view_t* view); // set x, y possibly adjust w, h (top down)
    void (*paint)(ui_view_t* view);
    bool (*message)(ui_view_t* view, int32_t message, int64_t wp, int64_t lp,
        int64_t* rt); // return true and value in rt to stop processing
    void (*click)(ui_view_t* view); // interpretation depends on ui element
    void (*mouse)(ui_view_t* view, int32_t message, int64_t flags);
    void (*mouse_wheel)(ui_view_t* view, int32_t dx, int32_t dy); // touchpad scroll
    // tap(ui, button_index) press(ui, button_index) see note below
    // button index 0: left, 1: middle, 2: right
    // bottom up (leaves to root or children to parent)
    // return true if consumed (halts further calls up the tree)
    bool (*tap)(ui_view_t* view, int32_t ix);   // single click/tap inside ui
    bool (*press)(ui_view_t* view, int32_t ix); // two finger click/tap or long press
    void (*context_menu)(ui_view_t* view); // right mouse click or long press
    bool (*set_focus)(ui_view_t* view); // returns true if focus is set
    void (*kill_focus)(ui_view_t* view);
    // translated from key pressed/released to utf8:
    void (*character)(ui_view_t* view, const char* utf8);
    void (*key_pressed)(ui_view_t* view, int64_t key);
    void (*key_released)(ui_view_t* view, int64_t key);
    // timer() every_100ms() and every_sec() called
    // even for hidden and disabled ui elements
    void (*timer)(ui_view_t* view, ui_timer_t id);
    void (*every_100ms)(ui_view_t* view); // ~10 x times per second
    void (*every_sec)(ui_view_t* view); // ~once a second
    int64_t (*hit_test)(int32_t x, int32_t y); // default: ui.hit_test.client
    fp64_t armed_until; // ut_clock.seconds() - when to release
    bool hidden; // paint() is not called on hidden
    bool armed;
    bool hover;
    bool pressed;   // for ui_button_t and ui_toggle_t
    bool disabled;  // mouse, keyboard, key_up/down not called on disabled
    bool focusable; // can be target for keyboard focus
    bool flat;      // no-border appearance of views
    fp64_t  hover_at;    // time in seconds when to call hovered()
    ui_color_t color;      // interpretation depends on ui element type
    ui_color_t background; // interpretation depends on ui element type
    ui_font_t* font;
    int32_t baseline; // font ascent; descent = height - baseline
    int32_t descent;  // font descent
    char    tip[256]; // tooltip text
} ui_view_t;

// tap() / press() APIs guarantee that single tap() is not coming
// before fp64_t tap/click in expense of fp64_t click delay (0.5 seconds)
// which is OK for buttons and many other UI controls but absolutely not
// OK for text editing. Thus edit uses raw mouse events to react
// on clicks and fp64_t clicks.

void ui_view_init(ui_view_t* view);

void ui_view_init_container(ui_view_t* view);

#define ui_view(view_type) { .type = (ui_view_ ## view_type),   \
                             .init = ui_view_init_ ## view_type }

typedef struct ui_view_if {
    // children va_args must be null terminated
    ui_view_t* (*add)(ui_view_t* parent, ...);
    void (*add_first)(ui_view_t* parent, ui_view_t* child);
    void (*add_last)(ui_view_t* parent,  ui_view_t* child);
    void (*add_after)(ui_view_t* child,  ui_view_t* after);
    void (*add_before)(ui_view_t* child, ui_view_t* before);
    void (*remove)(ui_view_t* view);
    bool (*inside)(ui_view_t* view, const ui_point_t* pt);
    void (*set_text)(ui_view_t* view, const char* text);
    void (*invalidate)(const ui_view_t* view); // prone to delays
    void (*measure)(ui_view_t* view);     // if text[] != "" sets w, h
    bool (*is_hidden)(ui_view_t* view);   // view or any parent is hidden
    bool (*is_disabled)(ui_view_t* view); // view or any parent is disabled
    const char* (*nls)(ui_view_t* view);  // returns localized text
    void (*localize)(ui_view_t* view);    // set strid based ui .text field
    void (*init_children)(ui_view_t* view);
    void (*set_parents)(ui_view_t* view);
    void (*timer)(ui_view_t* view, ui_timer_t id);
    void (*every_sec)(ui_view_t* view);
    void (*every_100ms)(ui_view_t* view);
    void (*key_pressed)(ui_view_t* view, int64_t v_key);
    void (*key_released)(ui_view_t* view, int64_t v_key);
    void (*character)(ui_view_t* view, const char* utf8);
    void (*paint)(ui_view_t* view);
    bool (*set_focus)(ui_view_t* view);
    void (*kill_focus)(ui_view_t* view);
    void (*kill_hidden_focus)(ui_view_t* view);
    void (*hovering)(ui_view_t* view, bool start);
    void (*mouse)(ui_view_t* view, int32_t m, int64_t f);
    void (*mouse_wheel)(ui_view_t* view, int32_t dx, int32_t dy);
    void (*measure_children)(ui_view_t* view);
    void (*layout_children)(ui_view_t* view);
    void (*hover_changed)(ui_view_t* view);
    bool (*is_shortcut_key)(ui_view_t* view, int64_t key);
    bool (*context_menu)(ui_view_t* view);
    bool (*tap)(ui_view_t* view, int32_t ix); // 0: left 1: middle 2: right
    bool (*press)(ui_view_t* view, int32_t ix); // 0: left 1: middle 2: right
    bool (*message)(ui_view_t* view, int32_t m, int64_t wp, int64_t lp,
                                     int64_t* ret);
    void (*test)(void);
} ui_view_if;

extern ui_view_if ui_view;

// view children iterator:

#define ui_view_for_each(v, it, code) do { \
    ui_view_t* it = (v)->child;            \
    if (it != null) {                      \
        do {                               \
            { code }                       \
            it = it->next;                 \
        } while (it != (v)->child);        \
    }                                      \
} while (0)


#define ui_view_call_init(v) do {               \
    if (v->init != null) {                      \
        void (*_init_)(ui_view_t* v) = v->init; \
        v->init = null; /* before! call */      \
        _init_(v);                              \
    }                                           \
} while (0)


end_c
