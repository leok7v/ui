/* Copyright (c) Dmitry "Leo" Kuznetsov 2021-24 see LICENSE for details */
#include "single_file_lib/ut/ut.h"
#include "single_file_lib/ui/ui.h"
#include "edit.h"

static bool debug_layout; // = true;

const char* title = "Sample5";

// font scale:
static const fp64_t fs[] = {0.5, 0.75, 1.0, 1.25, 1.50, 1.75, 2.0};
// font scale index
static int32_t fx = 2; // fs[2] == 1.0

static ui_font_t mf; // mono font
static ui_font_t pf; // proportional font

static ui_edit_t edit0;
static ui_edit_t edit1;
static ui_edit_t edit2;
static ui_edit_t* edit[3] = { &edit0, &edit1, &edit2 };

static int32_t focused(void) {
    // ui_app.focus can point to a button, thus see which edit
    // control was focused last
    int32_t ix = -1;
    for (int32_t i = 0; i < countof(edit) && ix < 0; i++) {
        if (ui_app.focus == &edit[i]->view) { ix = i; }
        if (edit[i]->focused) { ix = i; }
    }
    static int32_t last_ix = -1;
    if (ix < 0) { ix = last_ix; }
    last_ix = ix;
    return ix;
}

static void focus_back_to_edit(void) {
    const int32_t ix = focused();
    if (ix >= 0) {
        ui_app.focus = &edit[ix]->view; // return focus where it was
    }
}

static void scaled_fonts(void) {
    assert(0 <= fx && fx < countof(fs));
    if (mf != null) { ui_gdi.delete_font(mf); }
    mf = ui_gdi.font(ui_app.fonts.mono,
                  (int32_t)(ui_gdi.font_height(ui_app.fonts.mono) * fs[fx] + 0.5),
                  -1);
    if (pf != null) { ui_gdi.delete_font(pf); }
    pf = ui_gdi.font(ui_app.fonts.regular,
                  (int32_t)(ui_gdi.font_height(ui_app.fonts.regular) * fs[fx] + 0.5),
                  -1);
}

static_ui_button(full_screen, "&Full Screen", 7.5, {
    ui_app.full_screen(!ui_app.is_full_screen);
});

static_ui_button(quit, "&Quit", 7.5, { ui_app.close(); });

static_ui_button(fuzz, "Fu&zz", 7.5, {
    int32_t ix = focused();
    if (ix >= 0) {
        edit[ix]->fuzz(edit[ix]);
        fuzz->pressed = edit[ix]->fuzzer != null;
        focus_back_to_edit();
    }
});

static_ui_toggle(ro, "&Read Only", 7.5, {
    int32_t ix = focused();
    if (ix >= 0) {
        edit[ix]->ro = ro->pressed;
//      traceln("edit[%d].readonly: %d", ix, edit[ix]->ro);
        focus_back_to_edit();
    }
});

static_ui_toggle(mono, "&Mono", 7.5, {
    int32_t ix = focused();
    if (ix >= 0) {
        edit[ix]->set_font(edit[ix], mono->pressed ? &mf : &pf);
        focus_back_to_edit();
    } else {
        mono->pressed = !mono->pressed;
    }
});

static_ui_toggle(sl, "&Single Line", 7.5, {
    int32_t ix = focused();
    if (ix == 2) {
        sl->pressed = true; // always single line
    } else if (0 <= ix && ix < 2) {
        ui_edit_t* e = edit[ix];
        e->sle = sl->pressed;
//      traceln("edit[%d].multiline: %d", ix, e->multiline);
        if (e->sle) {
            e->select_all(e);
            e->paste(e, "Hello World! Single Line Edit", -1);
        }
        // alternatively ui_app.layout() for everything or:
        e->view.measure(&e->view);
        e->view.layout(&e->view);
        focus_back_to_edit();
    }
});

static void font_plus(void) {
    if (fx < countof(fs) - 1) {
        fx++;
        scaled_fonts();
        ui_app.layout();
    }
}

static void font_minus(void) {
    if (fx > 0) {
        fx--;
        scaled_fonts();
        ui_app.layout();
    }
}

static void font_reset(void) {
    fx = 2;
    scaled_fonts();
    ui_app.layout();
}

static_ui_button(fp, "Font Ctrl+", 7.5, { font_plus(); });

static_ui_button(fm, "Font Ctrl-", 7.5, { font_minus(); });

static ui_label_t label = ui_label(0.0, "...");

static ui_view_t right  = ui_view(container);
static ui_view_t left   = ui_view(container);
static ui_view_t bottom = ui_view(container);

static void set_text(int32_t ix) {
    static char last[128];
    strprintf(label.text, "%d:%d %d:%d %dx%d\n"
        "scroll %03d:%03d",
        edit[ix]->selection[0].pn, edit[ix]->selection[0].gp,
        edit[ix]->selection[1].pn, edit[ix]->selection[1].gp,
        edit[ix]->view.w, edit[ix]->view.h,
        edit[ix]->scroll.pn, edit[ix]->scroll.rn);
    if (0) {
        traceln("%d:%d %d:%d %dx%d scroll %03d:%03d",
            edit[ix]->selection[0].pn, edit[ix]->selection[0].gp,
            edit[ix]->selection[1].pn, edit[ix]->selection[1].gp,
            edit[ix]->view.w, edit[ix]->view.h,
            edit[ix]->scroll.pn, edit[ix]->scroll.rn);
    }
    // can be called before text.ui initialized
    if (!strequ(last, label.text)) {
        ui_view.invalidate(&label);
    }
    strprintf(last, "%s", label.text);
}

static void after_paint(void) {
    // because of blinking caret paint is called frequently
    int32_t ix = focused();
    if (ix >= 0) {
        bool fuzzing = edit[ix]->fuzzer != null;
        if (fuzz.pressed != fuzzing) {
            fuzz.pressed = fuzzing;
            ui_view.invalidate(&fuzz);
        }
        set_text(ix);
    }
}

static void paint_frames(ui_view_t* view) {
    ui_view_for_each(view, c, { paint_frames(c); });
    ui_color_t fc[] = {
        ui_colors.red, ui_colors.green, ui_colors.blue, ui_colors.red,
        ui_colors.yellow, ui_colors.cyan, ui_colors.magenta
    };
    static int32_t color;
    ui_gdi.push(view->x, view->y + view->h - view->em.y);
    ui_gdi.frame_with(view->x, view->y, view->w, view->h, fc[color]);
    ui_color_t c = ui_gdi.set_text_color(fc[color]);
    ui_gdi.print("%s", view->text);
    ui_gdi.set_text_color(c);
    ui_gdi.pop();
    color = (color + 1) % countof(fc);
}

static void null_paint(ui_view_t* view) {
    ui_view_for_each(view, c, { null_paint(c); });
    if (view != ui_app.view) {
        view->paint = null;
    }
}

static void paint(ui_view_t* view) {
//  traceln("");
    if (debug_layout) { null_paint(view); }
    ui_gdi.set_brush(ui_gdi.brush_color);
    ui_gdi.set_brush_color(ui_colors.black);
    ui_gdi.fill(0, 0, view->w, view->h);
    int32_t ix = focused();
    for (int32_t i = 0; i < countof(edit); i++) {
        ui_view_t* e = &edit[i]->view;
        ui_color_t c = edit[i]->ro ?
            ui_colors.tone_red : ui_colors.btn_hover_highlight;
        ui_gdi.frame_with(e->x - 1, e->y - 1, e->w + 2, e->h + 2,
            i == ix ? c : ui_colors.dkgray4);
    }
    after_paint();
    if (debug_layout) { paint_frames(view); }
    if (ix >= 0) {
        ro.pressed = edit[ix]->ro;
        sl.pressed = edit[ix]->sle;
        mono.pressed = edit[ix]->view.font == &mf;
    }
}

static void open_file(const char* pathname) {
    char* file = null;
    int64_t bytes = 0;
    if (ut_mem.map_ro(pathname, &file, &bytes) == 0) {
        if (0 < bytes && bytes <= INT64_MAX) {
            edit[0]->select_all(edit[0]);
            edit[0]->paste(edit[0], file, (int32_t)bytes);
            ui_edit_pg_t start = { .pn = 0, .gp = 0 };
            edit[0]->move(edit[0], start);
        }
        ut_mem.unmap(file, bytes);
    } else {
        ui_app.toast(5.3, "\nFailed to open file \"%s\".\n%s\n",
                  pathname, ut_str.error(ut_runtime.err()));
    }
}

static void every_100ms(void) {
//  traceln("");
    static ui_view_t* last;
    if (last != ui_app.focus) { ui_app.redraw(); }
    last = ui_app.focus;
}

static void measure(ui_view_t* view) {
//  traceln("");
    // gaps:
    const int32_t gx = view->em.x;
    const int32_t gy = view->em.y;
    right.h = view->h - label.h - gy;
    right.w = 0;
    measurements.vertical(&right, gy / 2);
    right.w += gx;
    bottom.w = label.w - gx;
    bottom.h = label.h;
    int32_t h = (view->h - bottom.h - gy * 3) / countof(edit);
    for (int32_t i = 0; i < 2; i++) { // edit[0] and edit[1] only
        edit[i]->view.w = view->w - right.w - gx * 2;
        edit[i]->view.h = h; // TODO: remove me - bad idea
    }
    left.w = 0;
    measurements.vertical(&left, gy);
    left.w += gx;
    edit2.view.w = ro.w; // only "width" height determined by label
    if (debug_layout) {
        traceln("%d,%d %dx%d", view->x, view->y, view->w, view->h);
        traceln("right %d,%d %dx%d", right.x, right.y, right.w, right.h);
        ui_view_for_each(&right, c, {
            traceln("  %s %d,%d %dx%d", c->text, c->x, c->y, c->w, c->h);
        });
        for (int32_t i = 0; i < countof(edit); i++) {
            traceln("[%d] %d,%d %dx%d", i, edit[i]->view.x, edit[i]->view.y,
                edit[i]->view.w, edit[i]->view.h);
        }
        traceln("left %d,%d %dx%d", left.x, left.y, left.w, left.h);
        traceln("bottom %d,%d %dx%d", bottom.x, bottom.y, bottom.w, bottom.h);
    }
}

static void layout(ui_view_t* view) {
//  traceln("");
    // gaps:
    const int32_t gx2 = view->em.x / 2;
    const int32_t gy2 = view->em.y / 2;
    left.x = gx2;
    left.y = gy2;
    layouts.vertical(&left, left.x + gx2, left.y + gy2, gy2);
    right.x = left.x + left.w + gx2;
    right.y = left.y;
    bottom.x = gx2;
    bottom.y = view->h - bottom.h;
    layouts.vertical(&right, right.x + gx2, right.y, gy2);
    label.x = gx2;
    label.y = view->h - label.h;
}

// limiting vertical height of SLE to 3 lines of text:

static void (*hooked_sle_measure)(ui_view_t* unused(view));

static void measure_3_lines_sle(ui_view_t* view) {
    // UX design decision:
    // 3 vertical visible runs SLE is friendlier in UX term
    // than not implemented horizontal scroll.
    assert(view == &edit[2]->view);
//  traceln("WxH: %dx%d <- r/o button", ro.view.w, ro.view.h);
    view->w = ro.w; // r/o button
    hooked_sle_measure(view);
//  traceln("WxH: %dx%d (%dx%d) em: %d lines: %d",
//          edit[2]->view.w, edit[2]->view.h,
//          edit[2]->width, edit[2]->height,
//          edit[2]->view.em.y, edit[2]->view.h / edit[2]->view.em.y);
    int32_t max_lines = edit[2]->focused ? 3 : 1;
    if (view->h > view->em.y * max_lines) {
        view->h = view->em.y * max_lines;
    }
}

static void key_pressed(ui_view_t* unused(view), int64_t key) {
    if (ui_app.has_focus() && key == ui.key.escape) { ui_app.close(); }
    int32_t ix = focused();
    if (key == ui.key.f5) {
        if (ix >= 0) {
            ui_edit_t* e = edit[ix];
            if (ui_app.ctrl && ui_app.shift && e->fuzzer == null) {
                e->fuzz(e); // start on Ctrl+Shift+F5
            } else if (e->fuzzer != null) {
                e->fuzz(e); // stop on F5
            }
        }
    }
    if (ui_app.ctrl) {
        if (key == ui.key.minus) {
            font_minus();
        } else if (key == ui.key.plus) {
            font_plus();
        } else if (key == '0') {
            font_reset();
        }
    }
    if (ix >= 0) { set_text(ix); }
}

static void edit_enter(ui_edit_t* e) {
    assert(e->sle);
    if (!ui_app.shift) { // ignore shift ENRER:
        traceln("text: %.*s", e->para[0].bytes, e->para[0].text);
    }
}

// see edit.test.c

void ui_edit_init_with_lorem_ipsum(ui_edit_t* e);
void ui_edit_fuzz(ui_edit_t* e);
void ui_edit_next_fuzz(ui_edit_t* e);

static void opened(void) {
    ui_app.view->measure     = measure;
    ui_app.view->layout      = layout;
    ui_app.view->paint       = paint;
    ui_app.view->key_pressed = key_pressed;
    scaled_fonts();
    ui_view.add(ui_app.view, &left, &right, &bottom, null);
    label.font = &ui_app.fonts.mono;
    strprintf(fuzz.hint, "Ctrl+Shift+F5 to start / F5 to stop Fuzzing");
    for (int32_t i = 0; i < countof(edit); i++) {
        ui_edit_init(edit[i]);
        edit[i]->view.font = &pf;
        edit[i]->fuzz = ui_edit_fuzz;
        edit[i]->next_fuzz = ui_edit_next_fuzz;
        ui_edit_init_with_lorem_ipsum(edit[i]);
    }
    ui_app.focus = &edit[0]->view;
    ui_app.every_100ms = every_100ms;
    set_text(0); // need to be two lines for measure
    // edit[2] is SLE:
    ui_edit_init(edit[2]);
    hooked_sle_measure = edit[2]->view.measure;
    edit[2]->view.font = &pf;
    edit[2]->fuzz = ui_edit_fuzz;
    edit[2]->next_fuzz = ui_edit_next_fuzz;
    edit[2]->view.measure = measure_3_lines_sle;
    edit[2]->sle = true;
    edit[2]->select_all(edit[2]);
    edit[2]->paste(edit[2], "Single line edit", -1);
    edit[2]->enter = edit_enter;

    ui_view.add(&right, &full_screen, &quit, &fuzz,
                &fp, &fm, &mono, &sl, &ro,
                &edit2,null);
    ui_view.add(&left, &edit0, &edit1, null);
    ui_view.add(&bottom, &label, null);
    if (ut_args.c > 1) { open_file(ut_args.v[1]); }
}

static void init(void) {
    ui_app.title = title;
    ui_app.opened = opened;
}

ui_app_t ui_app = {
    .class_name = "sample5",
    .dark_mode = true,
    .init = init,
    .window_sizing = {
        .min_w =  3.0f,
        .min_h =  2.5f,
        .ini_w =  4.0f,
        .ini_h =  5.0f
    }
};

