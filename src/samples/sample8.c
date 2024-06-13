/* Copyright (c) Dmitry "Leo" Kuznetsov 2021-24 see LICENSE for details */
#include "ut/ut.h"
#include "ui/ui.h"

static const char* title = "Sample8: Panels";

enum { version = 0x102 };

typedef ut_begin_packed struct app_data_t {
    int32_t version;
    int32_t menu_used;
    int32_t selected_view;
    int32_t light;
    int32_t debug;
} ut_end_packed app_data_t;

static app_data_t app_data = { .version = version };

static void init(void);
static void opened(void);
static void container_test(ui_view_t* parent);
static void span_test(ui_view_t* parent);
static void list_test(ui_view_t* parent);
static void controls_test(ui_view_t* parent);

static void fini(void) {
    ui_app.data_save("sample8", &app_data, sizeof(app_data));
}

static void init(void) {
    app_data_t data = {0};
    if (ui_app.data_load("sample8", &data, sizeof(data)) == sizeof(data) &&
        data.version == version) {
        app_data = data;
    }
    ui_app.title  = title;
    ui_app.fini   = fini;
    ui_app.opened = opened;
}

ui_app_t ui_app = {
    .class_name = "sample8",
    .no_decor = true,
    .dark_mode = false,
    .light_mode = false,
    .init = init,
    .window_sizing = {
        .ini_w =  10.0f,
        .ini_h =   7.0f
    }
};

static ui_view_t test = ui_view(container);

static ui_view_t tools_list = ui_view(list);

static void tools(ui_button_t* b) {
    b->pressed = !b->pressed;
    traceln("b->pressed: %d", b->pressed);
    tools_list.hidden = !b->pressed;
    app_data.menu_used = 1;
    ui_app.request_layout();
}

static void switch_view(ui_button_t* b, int32_t ix,
        void (*build_view)(ui_view_t* v)) {
    if (!b->pressed) {
        tools_list.hidden = true;
        ui_caption.menu.pressed = false;
        ui_view_for_each(b->parent, c, { c->pressed = false; });
        b->pressed = !b->pressed;
        app_data.selected_view = ix;
        build_view(&test);
    }
}

static void container(ui_button_t* b) {
    switch_view(b, 0, container_test);
}

static void span(ui_button_t* b) {
    switch_view(b, 1, span_test);
}

static void list(ui_button_t* b) {
    switch_view(b, 2, list_test);
}

static void controls(ui_button_t* b) {
    switch_view(b, 3, controls_test);
}

static void debug(ui_button_t* b) {
    b->pressed = !b->pressed;
    app_data.debug = b->pressed;
}

static ui_button_t button_debug =
        ui_button(ut_glyph_lady_beetle, 0.0f, debug);

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
    ut_glyph_infinity
    "or child.max_h = "
    ut_glyph_infinity
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

static void about(ui_button_t* unused(b)) {
    ui_app.show_toast(&mbx.view, 10.0);
}

static void crash(ui_button_t* b) {
    // two random ways to crash in release configuration
    if (ut_clock.nanoseconds() % 2 == 0) {
        swear(false, "should crash in release configuration");
    } else {
        void* p = (void*)b->click; // null
        memcpy(p, b->text, 4);
    }
}

static void dark_light(ui_toggle_t* b) {
    b->pressed = !b->pressed;
    ui_app.light_mode = b->pressed;
    ui_app.dark_mode = !b->pressed;
    ui_theme.refresh();
}

static void insert_into_caption(ui_button_t* b, const char* hint) {
    strprintf(b->hint, "%s", hint);
    b->flat = true;
    b->padding = (ui_gaps_t){0,0,0,0};
    ui_view.add_before(b,  &ui_caption.mini);
}

static void opened(void) {
    static ui_view_t list_view = ui_view(list);
    static ui_view_t span_view = ui_view(span);
    static ui_button_t button_container =
           ui_button("&Container", 4.25f, container);
    static ui_button_t button_span =
           ui_button("&Span",      4.25f, span);
    static ui_button_t button_list =
           ui_button("&List",      4.25f, list);
    static ui_button_t button_controls =
           ui_button("Con&trols",  4.25f, controls);
    ui_view.add(ui_app.content,
        ui_view.add(&list_view,
            ui_view.add(&span_view,
                ui_view.add(&tools_list,
                    &button_container,
                    &button_span,
                    &button_list,
                    &button_controls,
                null),
                &test,
            null),
        null),
    null);
    list_view.max_w = ui.infinity;
    list_view.max_h = ui.infinity;
    list_view.insets = (ui_gaps_t){ 0, 0, 0, 0 };
    span_view.max_w = ui.infinity;
    span_view.max_h = ui.infinity;
    span_view.insets = (ui_gaps_t){ 0, 0, 0, 0 };
    test.max_w = ui.infinity;
    test.max_h = ui.infinity;
    test.color = ui_colors.transparent;
    test.insets = (ui_gaps_t){ 0, 0, 0, 0 };
    test.background_id = ui_color_id_window;
    ut_str_printf(test.text, "%s", "test");
//  test.paint = ui_view.debug_paint;
    test.debug = true;
    // buttons to switch test content
    tools_list.max_h = ui.infinity;
    tools_list.color_id = ui_color_id_window;
    ut_str_printf(tools_list.text, "%s", "Tools");
//  tools_list.paint = ui_view.debug_paint;
    ui_view_for_each(&tools_list, it, {
        it->align = ui.align.left;
        it->padding.bottom = 0;
    });
    ut_str_printf(button_container.hint,
        "Shows ui_view(container) layout\n"
        "Resizing Window will allow\n"
        "too see how it behaves");
    switch (app_data.selected_view) {
        case  1: span(&button_span); break;
        case  2: list(&button_list); break;
        case  3: controls(&button_controls); break;
        case  0: // drop to default:
        default: container(&button_container); break;
    }
    ui_caption.menu.callback = tools;
    ui_caption.icon.hidden = true;
    tools_list.hidden = true;
    if (app_data.menu_used == 0) {
        ui_app.toast(4.5, "For tools click "
                          ut_glyph_trigram_for_heaven
                          " menu button");
    }
    // caption buttons:
    static ui_button_t button_info =
           ui_button(ut_glyph_circled_information_source, 0.0f, about);
    static ui_button_t button_light =
           ui_button(ut_glyph_electric_light_bulb, 0.0f, dark_light);
    static ui_button_t button_bomb =
           ui_button(ut_glyph_bomb, 0.0f, crash);
    insert_into_caption(&button_info, "About");
    insert_into_caption(&button_debug, "Debug");
    insert_into_caption(&button_light, "Dark/Light Mode");
    insert_into_caption(&button_bomb, "Intentionally Crash");
    traceln("TODO: serialize dark light and debug views state");
    if (app_data.light) { dark_light(&button_light); }
    if (app_data.debug) { debug(&button_debug); }
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
    ut_str_printf(container.text, "container");
    ui_view_for_each(&container, it, {
        it->debug = true;
        it->color = ui_colors.onyx;
//      it->fm    = &ui_app.fonts.H1;
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
    ut_str_printf(span.text, "span");
    span.background_id = ui_color_id_window;
    ui_view_for_each(&span, it, {
        it->debug   = true;
        it->color   = ui_colors.onyx;
        it->padding = (ui_gaps_t){ 2.0, 0.25, 0.5, 1.0 };
        it->max_h   = ui.infinity;
//      it->fm      = &ui_app.fonts.H1;
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
    ut_str_printf(list.text, "list");
    ui_view_for_each(&list, it, {
        it->debug   = true;
        it->color   = ui_colors.onyx;
        // TODO: labels, buttons etc should define their own default padding != 0
        it->padding = (ui_gaps_t){ 2.0, 0.25, 0.5, 1.0 };
        it->max_w   = ui.infinity;
//      it->fm      = &ui_app.fonts.H1;
    });
    left.max_w = 0;
    right.max_w = 0;
}

static void slider_format(ui_view_t* v) {
    ui_slider_t* slider = (ui_slider_t*)v;
    ut_str_printf(v->text, "%s", ut_str.uint64(slider->value));
}

static void slider_callback(ui_view_t* v) {
    ui_slider_t* slider = (ui_slider_t*)v;
    traceln("value: %d", slider->value);
}

static void controls_test(ui_view_t* parent) {
    ui_view.disband(parent);
    static ui_view_t  list     = ui_view(list);
    static ui_view_t  span     = ui_view(span);
    static ui_label_t  left    = ui_label(0, "Left");
    static ui_button_t button1 = ui_button("&Button",  0, null);
    static ui_slider_t slider1 = ui_slider("%d", 3.3f, 0, UINT16_MAX,
                                           slider_format, slider_callback);
    static ui_toggle_t toggle1 = ui_toggle("Toggle: ___", 0.0, null);
    static ui_label_t  right   = ui_label(0, "Right ");
    static ui_label_t  label   = ui_label(0, "Label");
    static ui_button_t button2 = ui_button("&Button",  0, null);
    static ui_slider_t slider2 = ui_slider("%d", 3.3f, 0, UINT16_MAX,
                                            slider_format, slider_callback);
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
    ut_str_printf(list.text, "list");
    list.background_id = ui_color_id_window;
//  ui_view_for_each(&list, it, { it->fm = &ui_app.fonts.H1; it->debug = false; } );
//  ui_view_for_each(&span, it, { it->fm = &ui_app.fonts.H1; it->debug = false; } );
    slider2.dec.hidden = true;
    slider2.inc.hidden = true;
}
