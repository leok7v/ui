/* Copyright (c) Dmitry "Leo" Kuznetsov 2021-24 see LICENSE for details */
#include "ut/ut.h"
#include "ui/ui.h"

static const char* title = "Sample8: Panels";

static void init(void);
static void opened(void);
static void container_test(ui_view_t* parent);
static void span_test(ui_view_t* parent);
static void list_test(ui_view_t* parent);
static void controls_test(ui_view_t* parent);

static void init(void) {
    ui_app.title  = title;
    ui_app.opened = opened;
}

ui_app_t ui_app = {
    .class_name = "sample8",
    .no_decor = true,
    .init = init,
    .window_sizing = {
        .ini_w =  10.0f,
        .ini_h =   7.0f
    }
};

static ui_view_t test = ui_view(container);

static void toggle_container(ui_button_t* b) {
    ui_view_for_each(b->parent, c, { c->pressed = false; });
    b->pressed = !b->pressed;
    container_test(&test);
}

static void toggle_span(ui_button_t* b) {
    ui_view_for_each(b->parent, c, { c->pressed = false; });
    b->pressed = !b->pressed;
    span_test(&test);
}

static void toggle_list(ui_button_t* b) {
    ui_view_for_each(b->parent, c, { c->pressed = false; });
    b->pressed = !b->pressed;
    list_test(&test);
}

static void toggle_controls(ui_button_t* b) {
    ui_view_for_each(b->parent, c, { c->pressed = false; });
    b->pressed = !b->pressed;
    controls_test(&test);
}

static void insets_callback(ui_view_t* v) {
    ui_slider_t* slider = (ui_slider_t*)v;
    traceln("value: %d", slider->value);
}

static void padding_callback(ui_view_t* v) {
    ui_slider_t* slider = (ui_slider_t*)v;
    traceln("value: %d", slider->value);
}

static ui_mbx_t mbx = ui_mbx( // message box
    "Orange frames represent container, span, or list\n"
    "components. Green frames indicate padding for\n"
    "children.\n"
    "\n"
    "These insets and padding are intentionally\n"
    "varied on different sides.\n"
    "\n"
    "By default, a container centers its children \n"
    "unless an alignment is specified by a child.\n"
    "\n"
    "When child.max_w = "
    ui_glyph_infinity
    "or child.max_h = "
    ui_glyph_infinity
    ",\n"
    "the child expands in the specified direction.\n"
    "\n"
    "Span aligns children horizontally, while List\n"
    "aligns them vertically.\n"
    "\n"
    "Overflows are permissible.\n"
    "\n"
    "Experiment with resizing the application window.\n"
    "\n"
    "Press ESC to close this message.",
    null, null);

static void toggle_about(ui_button_t* unused(b)) {
    ui_app.show_toast(&mbx.view, 10.0);
}

static void opened(void) {
    static ui_view_t list = ui_view(list);
    static ui_view_t span = ui_view(span);
    static ui_view_t tools = ui_view(list);
    static ui_button_t button_container = ui_button("&Container",  11.0, toggle_container);
    static ui_button_t button_span      = ui_button("&Span",       11.0, toggle_span);
    static ui_button_t button_list      = ui_button("&List",       11.0, toggle_list);
    static ui_button_t button_controls  = ui_button("&Controls",   11.0, toggle_controls);
    static ui_button_t button_about     = ui_button("&About",      11.0, toggle_about);
    static ui_slider_t slider_insets    = ui_slider("Insets:  %d",  5.5, 0, 3, insets_callback);
    static ui_slider_t slider_padding   = ui_slider("Padding: %d",  5.5, 0, 3, padding_callback);
    ui_view.add(ui_app.view,
        ui_view.add(&list,
            &ui_caption,
            ui_view.add(&span,
                &test,
                ui_view.add(&tools,
                    &button_container,
                    &button_span,
                    &button_list,
                    &button_controls,
                    &button_about,
                    &slider_insets,
                    &slider_padding,
                null),
            null),
        null),
    null);
    list.max_w = ui.infinity;
    list.insets = (ui_gaps_t){ 0, 0, 0, 0 };
    span.max_w = ui.infinity;
    span.max_h = ui.infinity;
    span.insets = (ui_gaps_t){ 0, 0, 0, 0 };
    test.max_w = ui.infinity;
    test.max_h = ui.infinity;
    test.color = ui_color_transparent;
    test.align = ui.align.left;
    test.insets = (ui_gaps_t){ 0, 0, 0, 0 };
    test.background_id = ui_color_id_window;
    strprintf(test.text, "%s", "test");
//test.paint = ui_view.debug_paint;
    test.debug = true;

    // buttons to switch test content
    tools.max_h = ui.infinity;
    tools.color_id = ui_color_id_window;
    tools.align = ui.align.right;
    strprintf(tools.text, "%s", "tools");
//  tools.paint = ui_view.debug_paint;
    ui_view_for_each(&tools, it, { it->align = ui.align.left; });
    strprintf(button_container.hint,
        "Shows ui_view(container) layout\n"
        "Resizing Window will allow\n"
        "too see how it behaves");
//  toggle_container(&button_container);
//  toggle_list(&button_list);
//  toggle_span(&button_span);
    toggle_controls(&button_controls);
}

static ui_view_t* align(ui_view_t* v, int32_t align) {
    v->align = align;
    return v;
}

static void container_test(ui_view_t* parent) {
    ui_view.disband(parent);
    static ui_view_t  container    = ui_view(container);
    static ui_label_t left         = ui_label(0, " left ");
    static ui_label_t right        = ui_label(0, " right ");
    static ui_label_t top          = ui_label(0, " top ");
    static ui_label_t bottom       = ui_label(0, " bottom ");
    static ui_label_t left_top     = ui_label(0, " left|top ");
    static ui_label_t right_bottom = ui_label(0, " right|bottom ");
    static ui_label_t right_top    = ui_label(0, " right|top ");
    static ui_label_t left_bottom  = ui_label(0, " left|bottom ");
    static ui_label_t center       = ui_label(0, " center ");
    container.insets = (ui_gaps_t){ 1.0, 0.5, 0.25, 2.0 };
    ui_view.add(parent,
        ui_view.add(&container,
            align(&left,         ui.align.left),
            align(&right,        ui.align.right),
            align(&top,          ui.align.top),
            align(&bottom,       ui.align.bottom),
            align(&left_top,     ui.align.left |ui.align.top),
            align(&right_bottom, ui.align.right|ui.align.bottom),
            align(&right_top,    ui.align.right|ui.align.top),
            align(&left_bottom,  ui.align.left |ui.align.bottom),
            align(&center,       ui.align.center),
        null),
    null);
    container.debug  = true;
    container.max_w  = ui.infinity;
    container.max_h  = ui.infinity;
    container.insets = (ui_gaps_t){ 1.0, 0.5, 0.25, 2.0 };
    container.background_id = ui_color_id_window;
    strprintf(container.text, "container");
    ui_view_for_each(&container, it, {
        it->debug = true;
        it->color = ui_colors.onyx;
        it->fm    = &ui_app.fonts.H1;
        it->padding = (ui_gaps_t){ 2.0, 0.25, 0.5, 1.0 };
    });
}

static void span_test(ui_view_t* parent) {
    ui_view.disband(parent);
    static ui_view_t  span   = ui_view(span);
    static ui_label_t left   = ui_label(0, " left ");
    static ui_label_t right  = ui_label(0, " right ");
    static ui_view_t  spacer = ui_view(spacer);
    static ui_label_t top    = ui_label(0, " top ");
    static ui_label_t bottom = ui_label(0, " bottom ");
    ui_view.add(parent,
        ui_view.add(&span,
            align(&left,   ui.align.center),
            align(&top,    ui.align.top),
            align(&spacer, ui.align.center),
            align(&bottom, ui.align.bottom),
            align(&right,  ui.align.center),
        null),
    null);
    span.debug    = true;
    span.max_w    = ui.infinity;
    span.max_h    = ui.infinity;
    span.insets   = (ui_gaps_t){ 1.0, 0.5, 0.25, 2.0 };
    strprintf(span.text, "span");
    span.background_id = ui_color_id_window;
    ui_view_for_each(&span, it, {
        it->debug   = true;
        it->color   = ui_colors.onyx;
        it->padding = (ui_gaps_t){ 2.0, 0.25, 0.5, 1.0 };
        it->max_h   = ui.infinity;
        it->fm      = &ui_app.fonts.H1;
//      traceln("%s 0x%02X", it->text, it->align);
    });
    top.max_h = 0;
    bottom.max_h = 0;
}

static void list_test(ui_view_t* parent) {
    ui_view.disband(parent);
    static ui_view_t  list         = ui_view(list);
    static ui_label_t left         = ui_label(0, " left ");
    static ui_label_t right        = ui_label(0, " right ");
    static ui_view_t  spacer       = ui_view(spacer);
    static ui_label_t top          = ui_label(0, " top ");
    static ui_label_t bottom       = ui_label(0, " bottom ");
    ui_view.add(&test,
        ui_view.add(&list,
            align(&top,    ui.align.center),
            align(&left,   ui.align.left),
            align(&spacer, ui.align.center),
            align(&right,  ui.align.right),
            align(&bottom, ui.align.center),
        null),
    null);
    list.debug  = true;
    list.max_w  = ui.infinity;
    list.max_h  = ui.infinity;
    list.insets = (ui_gaps_t){ 1.0, 0.5, 0.25, 2.0 };
    list.background_id = ui_color_id_window;
    strprintf(list.text, "list");
    ui_view_for_each(&list, it, {
        it->debug   = true;
        it->color   = ui_colors.onyx;
        // TODO: labels, buttons etc should define their own default padding != 0
        it->padding = (ui_gaps_t){ 2.0, 0.25, 0.5, 1.0 };
        it->max_w   = ui.infinity;
        it->fm      = &ui_app.fonts.H1;
    });
    left.max_w = 0;
    right.max_w = 0;
}

static void controls_test(ui_view_t* parent) {
    ui_view.disband(parent);
    static ui_view_t  list     = ui_view(list);
    static ui_view_t  span     = ui_view(span);
    static ui_label_t  left    = ui_label(0, "Left");
    static ui_button_t button1 = ui_button("&Button", 0, null);
    static ui_slider_t slider1 = ui_slider("Slider: %d", 0.0, 0, INT32_MAX, null);
    static ui_toggle_t toggle1 = ui_toggle("Toggle: ___", 0.0, null);
    static ui_label_t  right   = ui_label(0, "Right ");
    static ui_label_t  label   = ui_label(0, "Label");
    static ui_button_t button2 = ui_button("&Button", 0, null);
    static ui_slider_t slider2 = ui_slider("Slider: %d", 0.0, 0, INT32_MAX, null);
    static ui_toggle_t toggle2 = ui_toggle("Toggle", 0.0, null);
    static ui_view_t   spacer  = ui_view(spacer);
    ui_view.add(&test,
        ui_view.add(&list,
            ui_view.add(&span,
                align(&left,         ui.align.top),
                align(&button1,      ui.align.top),
                align(&right,        ui.align.top),
                align(&slider1.view, ui.align.top),
                align(&toggle1,      ui.align.top),
            null),
            align(&label,        ui.align.left),
            align(&button2,      ui.align.left),
            align(&slider2.view, ui.align.left),
            align(&toggle2,      ui.align.left),
            align(&spacer,       ui.align.left),
        null),
    null);
    list.debug  = true;
    list.max_w  = ui.infinity;
    list.max_h  = ui.infinity;
    strprintf(list.text, "list");
    list.background_id = ui_color_id_window;
    ui_view_for_each(&list, it, { it->fm = &ui_app.fonts.H1; it->debug = false; } );
    ui_view_for_each(&span, it, { it->fm = &ui_app.fonts.H1; it->debug = false; } );
}
