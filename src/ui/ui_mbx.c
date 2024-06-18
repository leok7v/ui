#include "ut/ut.h"
#include "ui/ui.h"

static void ui_mbx_button(ui_button_t* b) {
    ui_mbx_t* mx = (ui_mbx_t*)b->parent;
    assert(mx->view.type == ui_view_mbx);
    mx->option = -1;
    for (int32_t i = 0; i < countof(mx->button) && mx->option < 0; i++) {
        if (b == &mx->button[i]) {
            mx->option = i;
            if (mx->view.callback != null) { mx->view.callback(&mx->view); }
        }
    }
    ui_app.show_toast(null, 0);
}

static void ui_mbx_measured(ui_view_t* view) {
    ui_mbx_t* mx = (ui_mbx_t*)view;
    assert(view->type == ui_view_mbx);
    int32_t n = 0;
    ui_view_for_each(view, c, { n++; });
    n--; // number of buttons
    const int32_t em_x = mx->label.fm->em.w;
    const int32_t em_y = mx->label.fm->em.h;
    const int32_t tw = mx->label.w;
    const int32_t th = mx->label.h;
    if (n > 0) {
        int32_t bw = 0;
        for (int32_t i = 0; i < n; i++) {
            bw += mx->button[i].w;
        }
        view->w = ut_max(tw, bw + em_x * 2);
        view->h = th + mx->button[0].h + em_y + em_y / 2;
    } else {
        view->h = th + em_y / 2;
        view->w = tw;
    }
}

static void ui_mbx_layout(ui_view_t* view) {
    ui_mbx_t* mx = (ui_mbx_t*)view;
    assert(view->type == ui_view_mbx);
    int32_t n = 0;
    ui_view_for_each(view, c, { n++; });
    n--; // number of buttons
    const int32_t em_y = mx->label.fm->em.h;
    mx->label.x = view->x;
    mx->label.y = view->y + em_y * 2 / 3;
    const int32_t tw = mx->label.w;
    const int32_t th = mx->label.h;
    if (n > 0) {
        int32_t bw = 0;
        for (int32_t i = 0; i < n; i++) {
            bw += mx->button[i].w;
        }
        // center text:
        mx->label.x = view->x + (view->w - tw) / 2;
        // spacing between buttons:
        int32_t sp = (view->w - bw) / (n + 1);
        int32_t x = sp;
        for (int32_t i = 0; i < n; i++) {
            mx->button[i].x = view->x + x;
            mx->button[i].y = view->y + th + em_y * 3 / 2;
            x += mx->button[i].w + sp;
        }
    }
}

void ui_view_init_mbx(ui_view_t* view) {
    assert(view->type == ui_view_mbx);
    ui_mbx_t* mx = (ui_mbx_t*)view;
    view->measured = ui_mbx_measured;
    view->layout = ui_mbx_layout;
    mx->view.fm = &ui_app.fonts.regular;
    int32_t n = 0;
    while (mx->options[n] != null && n < countof(mx->button) - 1) {
        mx->button[n] = (ui_button_t)ui_button("", 6.0, ui_mbx_button);
        ui_view.set_text(&mx->button[n], "%s", mx->options[n]);
        n++;
    }
    swear(n <= countof(mx->button), "inhumane: %d buttons is too many", n);
    if (n > countof(mx->button)) { n = countof(mx->button); }
    mx->label = (ui_label_t)ui_label(0, "");
    ui_view.set_text(&mx->label, "%s", ui_view.string(&mx->view));
    ui_view.add_last(&mx->view, &mx->label);
    for (int32_t i = 0; i < n; i++) {
        ui_view.add_last(&mx->view, &mx->button[i]);
        mx->button[i].fm = mx->view.fm;
    }
    mx->label.fm = mx->view.fm;
    ui_view.set_text(&mx->view, "");
    mx->option = -1;
}

void ui_mbx_init(ui_mbx_t* mx, const char* options[],
        const char* format, ...) {
    mx->view.type = ui_view_mbx;
    mx->view.measured  = ui_mbx_measured;
    mx->view.layout   = ui_mbx_layout;
    mx->view.color_id = ui_color_id_window;
    mx->options = options;
    va_list va;
    va_start(va, format);
    ui_view.set_text_va(&mx->view, format, va);
    ui_label_init(&mx->label, 0.0, ui_view.string(&mx->view));
    va_end(va);
    ui_view_init_mbx(&mx->view);
}
