#include "ut/ut.h"
#include "ui/ui.h"

static void ui_mbx_button(ui_button_t* b) {
    ui_mbx_t* mx = (ui_mbx_t*)b->view.parent;
    assert(mx->view.type == ui_view_mbx);
    mx->option = -1;
    for (int32_t i = 0; i < countof(mx->button) && mx->option < 0; i++) {
        if (b == &mx->button[i]) {
            mx->option = i;
            mx->cb(mx, i);
        }
    }
    app.show_toast(null, 0);
}

static void ui_mbx_measure(ui_view_t* view) {
    ui_mbx_t* mx = (ui_mbx_t*)view;
    assert(view->type == ui_view_mbx);
    int32_t n = 0;
    ui_view_for_each(view, c, { n++; });
    n--; // number of buttons
    if (mx->text.view.measure != null) {
        mx->text.view.measure(&mx->text.view);
    } else {
        ui_view.measure(&mx->text.view);
    }
    const int32_t em_x = mx->text.view.em.x;
    const int32_t em_y = mx->text.view.em.y;
    const int32_t tw = mx->text.view.w;
    const int32_t th = mx->text.view.h;
    if (n > 0) {
        int32_t bw = 0;
        for (int32_t i = 0; i < n; i++) {
            bw += mx->button[i].view.w;
        }
        view->w = ut_max(tw, bw + em_x * 2);
        view->h = th + mx->button[0].view.h + em_y + em_y / 2;
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
    const int32_t em_y = mx->text.view.em.y;
    mx->text.view.x = view->x;
    mx->text.view.y = view->y + em_y * 2 / 3;
    const int32_t tw = mx->text.view.w;
    const int32_t th = mx->text.view.h;
    if (n > 0) {
        int32_t bw = 0;
        for (int32_t i = 0; i < n; i++) {
            bw += mx->button[i].view.w;
        }
        // center text:
        mx->text.view.x = view->x + (view->w - tw) / 2;
        // spacing between buttons:
        int32_t sp = (view->w - bw) / (n + 1);
        int32_t x = sp;
        for (int32_t i = 0; i < n; i++) {
            mx->button[i].view.x = view->x + x;
            mx->button[i].view.y = view->y + th + em_y * 3 / 2;
            x += mx->button[i].view.w + sp;
        }
    }
}

void ui_mbx_init_(ui_view_t* view) {
    assert(view->type == ui_view_mbx);
    ui_mbx_t* mx = (ui_mbx_t*)view;
    ui_view_init(view);
    view->measure = ui_mbx_measure;
    view->layout  = ui_mbx_layout;
    mx->view.font = &app.fonts.H3;
    const char** opts = mx->opts;
    int32_t n = 0;
    while (opts[n] != null && n < countof(mx->button) - 1) {
        ui_button_init(&mx->button[n], opts[n], 6.0, ui_mbx_button);
        n++;
    }
    swear(n <= countof(mx->button), "inhumane: %d buttons", n);
    if (n > countof(mx->button)) { n = countof(mx->button); }
    ui_view.add_last(&mx->view, &mx->text.view);
    for (int32_t i = 0; i < n; i++) {
        ui_view.add_last(&mx->view, &mx->button[i].view);
        mx->button[i].view.font = mx->view.font;
        ui_view.localize(&mx->button[i].view);
        // TODO: remove assert below
        assert(mx->button[i].view.parent == &mx->view);
    }
    ui_label_init(&mx->text, 0.0, "%s", mx->view.text);
    mx->text.view.font = mx->view.font;
    ui_view.localize(&mx->text.view);
    mx->view.text[0] = 0;
    mx->option = -1;
}

void ui_mbx_init(ui_mbx_t* mx, const char* opts[],
        void (*cb)(ui_mbx_t* m, int32_t option),
        const char* format, ...) {
    mx->view.type = ui_view_mbx;
    mx->view.measure = ui_mbx_measure;
    mx->view.layout  = ui_mbx_layout;
    mx->opts = opts;
    mx->cb = cb;
    va_list vl;
    va_start(vl, format);
    ut_str.format_va(mx->view.text, countof(mx->view.text), format, vl);
    ui_label_init(&mx->text, 0.0, mx->view.text);
    va_end(vl);
    ui_mbx_init_(&mx->view);
}
