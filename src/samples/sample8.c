/* Copyright (c) Dmitry "Leo" Kuznetsov 2021-24 see LICENSE for details */
#include "ut/ut.h"
#include "ui/ui.h"

static const char* title = "Sample8: Panels";

static void init(void);
static void opened(void);
static void app_view_paint(ui_view_t* v);
static void container_test(ui_view_t* parent);
static void span_test(ui_view_t* parent);
static void list_test(ui_view_t* parent);

static void init(void) {
    app.title  = title;
    app.opened = opened;
}

app_t app = {
    .class_name = "sample8",
    .no_decor = true,
    .init = init,
    .window_sizing = {
        .ini_w =  10.0f,
        .ini_h =   7.0f
    }
};

static ui_view_t test = ui_view(container);

static void toggle_test_container(ui_button_t* b) {
    ui_view_for_each(b->view.parent, c, { c->pressed = false; });
    b->view.pressed = !b->view.pressed;
    container_test(&test);
}

static void toggle_test_span(ui_button_t* b) {
    ui_view_for_each(b->view.parent, c, { c->pressed = false; });
    b->view.pressed = !b->view.pressed;
    span_test(&test);
}
static void toggle_test_list(ui_button_t* b) {
    ui_view_for_each(b->view.parent, c, { c->pressed = false; });
    b->view.pressed = !b->view.pressed;
    list_test(&test);
}

static ui_mbx_t mbx = ui_mbx( // message box
    "Orange frames are container, span or list\n"
    "inserts. Green frames are children padding.\n"
    "Both intentionally not the same on all sides.\n"
    "Container centers children by default\n"
    "unless a child specifies an align. \n"
    "When child.max_w or child.max_h = "
    ui_glyph_infinity
    "\nit is expanded in specified direction.\n"
    "Span arranges children horizontally and\n"
    "List vertically.\n"
    "Overflows are allowed.\n"
    "Try to resize application window.\n"
    "\n"
    "Press ESC to close this message.",
    null, null);

static void toggle_about(ui_button_t* b) {
    app.show_toast(&mbx.view, 10.0);
}

static void opened(void) {
    static ui_view_t list = ui_view(list);
    static ui_view_t span = ui_view(span);
    static ui_view_t tools = ui_view(list);
    static ui_button_t test_container = ui_button("&Container", 7.0, toggle_test_container);
    static ui_button_t test_span = ui_button("&Span", 7.0, toggle_test_span);
    static ui_button_t test_list = ui_button("&List", 7.0, toggle_test_list);
    static ui_button_t about     = ui_button("About", 7.0, toggle_about);
    ui_view.add(app.view,
        ui_view.add(&list,
            &ui_caption,
            ui_view.add(&span,
                &test,
                ui_view.add(&tools,
                    &test_container.view,
                    &test_span.view,
                    &test_list.view,
                    &about.view,
                null),
            null),
        null),
    null);
    list.max_w = ui.infinity;
//  list.max_h = ui.infinity;
    list.insets = (ui_gaps_t){ 0, 0, 0, 0 };
    span.max_w = ui.infinity;
    span.max_h = ui.infinity;
    span.insets = (ui_gaps_t){ 0, 0, 0, 0 };
    test.max_w = ui.infinity;
    test.max_h = ui.infinity;
    test.color = ui_color_transparent;
    test.align = ui.align.left;
    test.insets = (ui_gaps_t){ 0, 0, 0, 0 };
    strprintf(test.text, "%s", "test");
    test.paint = ui_view.debug_paint;
    // buttons to switch test content
    tools.max_h = ui.infinity;
    tools.color = ui_colors.onyx;
    tools.align = ui.align.right;
    strprintf(tools.text, "%s", "tools");
//  tools.paint = ui_view.debug_paint;
    ui_view_for_each(&tools, it, {
        it->padding = (ui_gaps_t){ .left = 0.5,  .top = 0.25,
                                   .right = 0.5, .bottom = 0.25 };
    });
    toggle_test_container(&test_container);
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
            align(&left.view,         ui.align.left),
            align(&right.view,        ui.align.right),
            align(&top.view,          ui.align.top),
            align(&bottom.view,       ui.align.bottom),
            align(&left_top.view,     ui.align.left |ui.align.top),
            align(&right_bottom.view, ui.align.right|ui.align.bottom),
            align(&right_top.view,    ui.align.right|ui.align.top),
            align(&left_bottom.view,  ui.align.left |ui.align.bottom),
            align(&center.view,       ui.align.center),
        null),
    null);
    container.paint  = ui_view.debug_paint;
    container.max_w  = ui.infinity;
    container.max_h  = ui.infinity;
    container.color  = ui_colors.gunmetal;
    container.insets = (ui_gaps_t){ 1.0, 0.5, 0.25, 2.0 };
    strprintf(container.text, "container");
    ui_view_for_each(&container, it, {
        it->paint   = ui_view.debug_paint;
        it->color   = ui_colors.onyx;
        it->font    = &app.fonts.H1;
// TODO: labels, buttons etc should define their own default padding != 0
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
            align(&left.view,   ui.align.center),
            align(&top.view,    ui.align.top),
            align(&spacer,      ui.align.center),
            align(&bottom.view, ui.align.bottom),
            align(&right.view,  ui.align.center),
        null),
    null);
    span.paint  = ui_view.debug_paint;
    span.max_w  = ui.infinity;
    span.max_h  = ui.infinity;
    span.color  = ui_colors.gunmetal;
    span.insets = (ui_gaps_t){ 1.0, 0.5, 0.25, 2.0 };
    strprintf(span.text, "span");
    ui_view_for_each(&span, it, {
        it->paint   = ui_view.debug_paint;
        it->color   = ui_colors.onyx;
// TODO: labels, buttons etc should define their own default padding != 0
        it->padding = (ui_gaps_t){ 2.0, 0.25, 0.5, 1.0 };
        it->max_h   = ui.infinity;
        it->font    = &app.fonts.H1;
//      traceln("%s 0x%02X", it->text, it->align);
    });
    top.view.max_h = 0;
    bottom.view.max_h = 0;
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
            align(&top.view,          ui.align.center),
            align(&left.view,         ui.align.left),
            align(&spacer,            ui.align.center),
            align(&right.view,        ui.align.right),
            align(&bottom.view,       ui.align.center),
        null),
    null);
    list.paint  = ui_view.debug_paint;
    list.max_w  = ui.infinity;
    list.max_h  = ui.infinity;
    list.color  = ui_colors.gunmetal;
    list.insets = (ui_gaps_t){ 1.0, 0.5, 0.25, 2.0 };
    strprintf(list.text, "list");
    ui_view_for_each(&list, it, {
        it->paint   = ui_view.debug_paint;
        it->color   = ui_colors.onyx;
// TODO: labels, buttons etc should define their own default padding != 0
        it->padding = (ui_gaps_t){ 2.0, 0.25, 0.5, 1.0 };
        it->max_w   = ui.infinity;
        it->font    = &app.fonts.H1;
    });
    left.view.max_w = 0;
    right.view.max_w = 0;
}

