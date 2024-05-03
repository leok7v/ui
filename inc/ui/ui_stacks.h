/* Copyright (c) Dmitry "Leo" Kuznetsov 2021-24 see LICENSE for details */
#include "ui/ui.h"

begin_c

// TODO: renaming
// ui_container_t
// ui_span_t  horizontal
// ui_list_t  vertical

// Usage:
// ui_view_t* h_stack = ui_view(ui_view_h_stack);
// ui_view_t* v_stack = ui_view(ui_view_v_stack);
// containers that automatically layout child views
// similar to SwiftUI HStack and VStack

void ui_view_init_h_stack(ui_view_t* view);
void ui_view_init_v_stack(ui_view_t* view);
void ui_view_init_spacer(ui_view_t* view);

end_c
