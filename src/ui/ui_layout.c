#include "ut/ut.h"
#include "ui/ui.h"

static void measurements_center(ui_view_t* view) {
    assert(view->child != null && view->child->next == view->child,
        "must be a single child parent");
    ui_view_t* c = view->child; // even if hidden measure it
    c->w = view->w;
    c->h = view->h;
}

static void measurements_horizontal(ui_view_t* view, int32_t gap) {
    assert(view->child != null, "not a single child?");
    view->w = 0;
    view->h = 0;
    bool seen = false;
    ui_view_for_each(view, c, {
        if (!c->hidden) {
            if (seen) { view->w += gap; }
            view->w += c->w;
            view->h = ut_max(view->h, c->h);
            seen = true;
        }
    });
}

static void measurements_vertical(ui_view_t* view, int32_t gap) {
    assert(view->child != null, "not a single child?");
    view->h = 0;
    bool seen = false;
    ui_view_for_each(view, c, {
        if (!c->hidden) {
            if (seen) { view->h += gap; }
            view->h += c->h;
            view->w = ut_max(view->w, c->w);
            seen = true;
        }
    });
}

static void measurements_grid(ui_view_t* view, int32_t gap_h, int32_t gap_v) {
    int32_t cols = 0;
    ui_view_for_each(view, r, {
        int32_t n = 0;
        ui_view_for_each(r, c, { n++; });
        if (cols == 0) { cols = n; }
        assert(n > 0 && cols == n);
    });
    #pragma warning(push) // mxw[] IntelliSense confusion
    #pragma warning(disable: 6385)
    #pragma warning(disable: 6386)
    int32_t* mxw = (int32_t*)ut_stackalloc((size_t)cols * sizeof(int32_t));
    memset(mxw, 0, (size_t)cols * sizeof(int32_t));
    ui_view_for_each(view, row, {
        if (!row->hidden) {
            row->h = 0;
            row->fm->baseline = 0;
            int32_t i = 0;
            ui_view_for_each(row, col, {
                if (!col->hidden) {
                    mxw[i] = ut_max(mxw[i], col->w);
                    row->h = ut_max(row->h, col->h);
//                  traceln("[%d] row.fm->baseline: %d col.fm->baseline: %d ",
//                          i, row->fm->baseline, col->fm->baseline);
                    row->fm->baseline = ut_max(row->fm->baseline, col->fm->baseline);
                }
                i++;
            });
        }
    });
    view->h = 0;
    view->w = 0;
    int32_t rows_seen = 0; // number of visible rows so far
    ui_view_for_each(view, r, {
        if (!r->hidden) {
            r->w = 0;
            int32_t i = 0;
            int32_t cols_seen = 0; // number of visible columns so far
            ui_view_for_each(view, c, {
                if (!c->hidden) {
                    c->h = r->h; // all cells are same height
                    // TODO: label_dy needs to be transferred to containers
                    //       rationale: labels and buttons baselines must align
                    if (c->type == ui_view_label) { // lineup text baselines
                        c->label_dy = r->fm->baseline - c->fm->baseline;
                    }
                    c->w = mxw[i++];
                    r->w += c->w;
                    if (cols_seen > 0) { r->w += gap_h; }
                    view->w = ut_max(view->w, r->w);
                    cols_seen++;
                }
            });
            view->h += r->h;
            if (rows_seen > 0) { view->h += gap_v; }
            rows_seen++;
        }
    });
    #pragma warning(pop)
}

measurements_if measurements = {
    .center     = measurements_center,
    .horizontal = measurements_horizontal,
    .vertical   = measurements_vertical,
    .grid       = measurements_grid,
};

// layouts

static void layouts_center(ui_view_t* view) {
    assert(view->child != null && view->child->next == view->child,
        "must be a single child parent");
    ui_view_t* c = view->child;
    c->x = (view->w - c->w) / 2;
    c->y = (view->h - c->h) / 2;
}

static void layouts_horizontal(ui_view_t* view, int32_t x, int32_t y,
        int32_t gap) {
    assert(view->child != null, "not a single child?");
    bool seen = false;
    ui_view_for_each(view, c, {
        if (!c->hidden) {
            if (seen) { x += gap; }
            c->x = x;
            c->y = y;
            x += c->w;
            seen = true;
        }
    });
}

static void layouts_vertical(ui_view_t* view, int32_t x, int32_t y,
        int32_t gap) {
    assert(view->child != null, "not a single child?");
    bool seen = false;
    ui_view_for_each(view, c, {
        if (!c->hidden) {
            if (seen) { y += gap; }
            c->x = x;
            c->y = y;
            y += c->h;
            seen = true;
        }
    });
}

static void layouts_grid(ui_view_t* view, int32_t gap_h, int32_t gap_v) {
    assert(view->child != null, "not a single child?");
    int32_t x = view->x;
    int32_t y = view->y;
    bool row_seen = false;
    ui_view_for_each(view, row, {
        if (!row->hidden) {
            if (row_seen) { y += gap_v; }
            int32_t xc = x;
            bool col_seen = false;
            ui_view_for_each(row, col, {
                if (!col->hidden) {
                    if (col_seen) { xc += gap_h; }
                    col->x = xc;
                    col->y = y;
                    xc += col->w;
                    col_seen = true;
                }
            });
            y += row->h;
            row_seen = true;
        }
    });
}

layouts_if layouts = {
    .center     = layouts_center,
    .horizontal = layouts_horizontal,
    .vertical   = layouts_vertical,
    .grid       = layouts_grid
};
