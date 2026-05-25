/* Copyright (c) Dmitry "Leo" Kuznetsov 2021-24 see LICENSE for details */
#include "ui/ui.h"

rt_begin_c

struct ui_view;

// Usage:
//
// struct ui_view* stack  = ui_view(stack);
// struct ui_view* horizontal = ui_view(ui_view_span);
// struct ui_view* vertical   = ui_view(ui_view_list);
//
// containers automatically layout child views
// similar to SwiftUI HStack and VStack taking .align
// .insets and .padding into account.
//
// Container positions every child views in the center,
// top bottom left right edge or any of 4 corners
// depending on .align values.
// if child view has .max_w or .max_h set to ui.infinity == INT32_MAX
// the views are expanded to fill the container in specified
// direction. If child .max_w or .max_h is set to > .w or .h
// the child view .w .h measurement are expanded accordingly.
//
// All containers are transparent and inset by 1/4 of an "em"
// Except ui_app.root,caption,content which are also containers
// but are not inset or padded and have default background color.
//
// Application implementer can override this after
//
// void opened(void) {
//     ui_view.add(ui_app.view, ..., null);
//     ui_app.view->insets = (struct ui_margins) {
//         .left  = 0.25, .top    = 0.25,
//         .right = 0.25, .bottom = 0.25 };
//     ui_app.view->color = ui_colors.dark_scarlet;
// }

struct ui_view;

#define ui_view(view_type) {            \
    .type = (ui_view_ ## view_type),    \
    .init = ui_view_init_ ## view_type, \
    .fm   = &ui_app.fm.prop.normal,     \
    .color = ui_color_transparent,      \
    .color_id = 0                       \
}

void ui_view_init_stack(struct ui_view* v);
void ui_view_init_span(struct ui_view* v);
void ui_view_init_list(struct ui_view* v);
void ui_view_init_spacer(struct ui_view* v);

rt_end_c
