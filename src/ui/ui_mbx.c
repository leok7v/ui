#include "ut/ut.h"
#include "ui/ui.h"

static void ui_mbx_button(ui_button_t* b) {
    ui_mbx_t* m = (ui_mbx_t*)b->parent;
    ut_assert(m->type == ui_view_mbx);
    m->option = -1;
    for (int32_t i = 0; i < ut_countof(m->button) && m->option < 0; i++) {
        if (b == &m->button[i]) {
            m->option = i;
            if (m->callback != null) {
                m->callback(&m->view);
                // need to disarm button because message box about to close
                b->state.pressed = false;
                b->state.armed = false;
            }
        }
    }
    ui_app.show_toast(null, 0);
}

static void ui_mbx_measured(ui_view_t* v) {
    ui_mbx_t* m = (ui_mbx_t*)v;
    int32_t n = 0;
    ui_view_for_each(v, c, { n++; });
    n--; // number of buttons
    const int32_t em_x = m->label.fm->em.w;
    const int32_t em_y = m->label.fm->em.h;
    const int32_t tw = m->label.w;
    const int32_t th = m->label.h;
    if (n > 0) {
        int32_t bw = 0;
        for (int32_t i = 0; i < n; i++) {
            bw += m->button[i].w;
        }
        v->w = ut_max(tw, bw + em_x * 2);
        v->h = th + m->button[0].h + em_y + em_y / 2;
    } else {
        v->h = th + em_y / 2;
        v->w = tw;
    }
}

static void ui_mbx_layout(ui_view_t* v) {
    ui_mbx_t* m = (ui_mbx_t*)v;
    int32_t n = 0;
    ui_view_for_each(v, c, { n++; });
    n--; // number of buttons
    const int32_t em_y = m->label.fm->em.h;
    m->label.x = v->x;
    m->label.y = v->y + em_y * 2 / 3;
    const int32_t tw = m->label.w;
    const int32_t th = m->label.h;
    if (n > 0) {
        int32_t bw = 0;
        for (int32_t i = 0; i < n; i++) {
            bw += m->button[i].w;
        }
        // center text:
        m->label.x = v->x + (v->w - tw) / 2;
        // spacing between buttons:
        int32_t sp = (v->w - bw) / (n + 1);
        int32_t x = sp;
        for (int32_t i = 0; i < n; i++) {
            m->button[i].x = v->x + x;
            m->button[i].y = v->y + th + em_y * 3 / 2;
            x += m->button[i].w + sp;
        }
    }
}

void ui_view_init_mbx(ui_view_t* v) {
    ui_mbx_t* m = (ui_mbx_t*)v;
    v->measured = ui_mbx_measured;
    v->layout = ui_mbx_layout;
    m->fm = &ui_app.fm.prop.normal;
    int32_t n = 0;
    while (m->options[n] != null && n < ut_countof(m->button) - 1) {
        m->button[n] = (ui_button_t)ui_button("", 6.0, ui_mbx_button);
        ui_view.set_text(&m->button[n], "%s", m->options[n]);
        n++;
    }
    ut_swear(n <= ut_countof(m->button), "inhumane: %d buttons is too many", n);
    if (n > ut_countof(m->button)) { n = ut_countof(m->button); }
    m->label = (ui_label_t)ui_label(0, "");
    ui_view.set_text(&m->label, "%s", ui_view.string(&m->view));
    ui_view.add_last(&m->view, &m->label);
    for (int32_t i = 0; i < n; i++) {
        ui_view.add_last(&m->view, &m->button[i]);
        m->button[i].fm = m->fm;
    }
    m->label.fm = m->fm;
    ui_view.set_text(&m->view, "");
    m->option = -1;
    if (m->debug.id == null) { m->debug.id = "#mbx"; }
}

void ui_mbx_init(ui_mbx_t* m, const char* options[],
        const char* format, ...) {
    m->type = ui_view_mbx;
    m->measured  = ui_mbx_measured;
    m->layout    = ui_mbx_layout;
    m->color_id  = ui_color_id_window;
    m->options   = options;
    m->focusable = true;
    va_list va;
    va_start(va, format);
    ui_view.set_text_va(&m->view, format, va);
    ui_label_init(&m->label, 0.0, ui_view.string(&m->view));
    va_end(va);
    ui_view_init_mbx(&m->view);
}
