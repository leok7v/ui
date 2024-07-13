/* Copyright (c) Dmitry "Leo" Kuznetsov 2021-24 see LICENSE for details */
#include "ut/ut.h"
#include "ui/ui.h"
// #include "single_file_lib/ut/ut.h"
// #include "single_file_lib/ui/ui.h"

const char* title = "Sample5";

static ui_view_t left   = ui_view(list);
static ui_view_t right  = ui_view(list);
static ui_view_t bottom = ui_view(stack);

// font scale:
static const fp64_t fs[] = {0.5, 0.75, 1.0, 1.25, 1.50, 1.75, 2.0};
// font scale index
static int32_t fx = 2; // fs[2] == 1.0

static ui_fm_t mf; // mono font
static ui_fm_t pf; // proportional font

static ui_edit_t edit0;
static ui_edit_t edit1;
static ui_edit_t edit2;
static ui_edit_doc_t edit_doc_0;
static ui_edit_doc_t edit_doc_1;
static ui_edit_doc_t edit_doc_2;
static ui_edit_t* edit[] = { &edit0, &edit1, &edit2 };
static ui_edit_doc_t* doc[] = { &edit_doc_0, &edit_doc_1, &edit_doc_2 };

static int32_t focused(void) {
    // ui_app.focus can point to a button, thus see which edit
    // control was focused last
    int32_t ix = -1;
    for (int32_t i = 0; i < ut_count_of(edit) && ix < 0; i++) {
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
        ui_view.set_focus(&edit[ix]->view); // return focus where it was
    }
    ui_app.request_layout();
}

static void scaled_fonts(void) {
    assert(0 <= fx && fx < ut_count_of(fs));
    if (mf.font != null) { ui_gdi.delete_font(mf.font); }
    int32_t h = (int32_t)(ui_app.fm.mono.height * fs[fx] + 0.5);
    ui_gdi.update_fm(&mf, ui_gdi.font(ui_app.fm.mono.font, h, -1));
    if (pf.font != null) { ui_gdi.delete_font(pf.font); }
    h = (int32_t)(ui_app.fm.regular.height * fs[fx] + 0.5);
    ui_gdi.update_fm(&pf, ui_gdi.font(ui_app.fm.regular.font, h, -1));
}

ui_button_clicked(full_screen, "&Full Screen", 7.0f, {
    ui_app.full_screen(!ui_app.is_full_screen);
});

ui_button_clicked(quit, "&Quit", 7.0f, {
    if (!ui_fuzzing.from_inside()) { // ignore fuzzer clicks
        ui_app.close();
    }
});

ui_button_clicked(fuzz, "Fu&zz", 7.0f, {
    if (!ui_fuzzing.from_inside()) { // ignore fuzzer clicks
        if (!ui_fuzzing.is_running()) {
            ui_fuzzing.start(0x1);
        } else {
            ui_fuzzing.stop();
        }
        fuzz->state.pressed  = ui_fuzzing.is_running();
        focus_back_to_edit();
    }
});

ui_toggle_on_off(ro, "&Read Only", 7.0f, {
    if (!ui_fuzzing.from_inside()) { // ignore fuzzer clicks
        int32_t ix = focused();
        if (ix >= 0) {
            edit[ix]->ro = ro->state.pressed;
    //      traceln("edit[%d].readonly: %d", ix, edit[ix]->ro);
            focus_back_to_edit();
        }
    }
});

ui_toggle_on_off(ww, "Hide &Word Wrap", 7.0f, {
    int32_t ix = focused();
    if (ix >= 0) {
        edit[ix]->hide_word_wrap = ww->state.pressed;
//      traceln("edit[%d].hide_word_wrap: %d", ix, edit[ix]->hide_word_wrap);
        focus_back_to_edit();
    }
});


ui_toggle_on_off(mono, "&Mono", 7.0f, {
    int32_t ix = focused();
    if (ix >= 0) {
        ui_edit.set_font(edit[ix], mono->state.pressed ? &mf : &pf);
        focus_back_to_edit();
    } else {
        mono->state.pressed = !mono->state.pressed;
    }
});

ui_toggle_on_off(sl, "&Single Line", 7.0f, {
    if (!ui_fuzzing.from_inside()) { // ignore fuzzer clicks
        int32_t ix = focused();
        if (ix == 2) {
            sl->state.pressed = true; // always single line
        } else if (0 <= ix && ix < 2) {
            ui_edit_t* e = edit[ix];
            e->sle = sl->state.pressed;
    //      traceln("edit[%d].multiline: %d", ix, e->multiline);
            if (e->sle) {
                ui_edit.select_all(e);
                ui_edit.paste(e, "Hello World! Single Line Edit", -1);
            }
            ui_app.request_layout();
            focus_back_to_edit();
        }
    }
});

static void font_plus(void) {
    if (fx < ut_count_of(fs) - 1) {
        fx++;
        scaled_fonts();
        ui_app.request_layout();
    }
}

static void font_minus(void) {
    if (fx > 0) {
        fx--;
        scaled_fonts();
        ui_app.request_layout();
    }
}

static void font_reset(void) {
    fx = 2;
    scaled_fonts();
    ui_app.request_layout();
}

ui_button_clicked(fp, "Font Ctrl+", 7.0f, { font_plus(); });

ui_button_clicked(fm, "Font Ctrl-", 7.0f, { font_minus(); });

static ui_label_t label = ui_label(0.0, "...");

static void set_text(int32_t ix) {
    static char last[128];
    ui_view.set_text(&label, "%d:%d %d:%d %dx%d\n"
        "scroll %03d:%03d",
        edit[ix]->selection.a[0].pn, edit[ix]->selection.a[0].gp,
        edit[ix]->selection.a[1].pn, edit[ix]->selection.a[1].gp,
        edit[ix]->view.w, edit[ix]->view.h,
        edit[ix]->scroll.pn, edit[ix]->scroll.rn);
    if (0) {
        traceln("%d:%d %d:%d %dx%d scroll %03d:%03d",
            edit[ix]->selection.a[0].pn, edit[ix]->selection.a[0].gp,
            edit[ix]->selection.a[1].pn, edit[ix]->selection.a[1].gp,
            edit[ix]->view.w, edit[ix]->view.h,
            edit[ix]->scroll.pn, edit[ix]->scroll.rn);
    }
    // can be called before text.ui initialized
    if (strcmp(last, ui_view.string(&label)) != 0) {
        ui_view.invalidate(&label, null);
    }
    ut_str_printf(last, "%s", ui_view.string(&label));
}

static void paint(ui_view_t* v) {
    ui_gdi.fill(0, 0, v->w, v->h, ui_colors.black);
    int32_t ix = focused();
    for (int32_t i = 0; i < ut_count_of(edit); i++) {
        ui_view_t* e = &edit[i]->view;
        ui_color_t c = edit[i]->ro ?
            ui_colors.tone_red : ui_colors.blue;
        ui_gdi.frame(e->x - 1, e->y - 1, e->w + 2, e->h + 2,
            i == ix ? c : ui_color_rgb(63, 63, 70));
    }
    if (ix >= 0) {
        set_text(ix);
    }
    if (ix >= 0) {
        ro.state.pressed = edit[ix]->ro;
        sl.state.pressed = edit[ix]->sle;
        mono.state.pressed = edit[ix]->view.fm->font == mf.font;
    }
}

static void open_file(const char* pathname) {
    char* file = null;
    int64_t bytes = 0;
    if (ut_mem.map_ro(pathname, &file, &bytes) == 0) {
        if (0 < bytes && bytes <= INT64_MAX) {
            ui_edit.select_all(edit[0]);
            ui_edit.paste(edit[0], file, (int32_t)bytes);
            ui_edit_pg_t start = { .pn = 0, .gp = 0 };
            ui_edit.move(edit[0], start);
        }
        ut_mem.unmap(file, bytes);
    } else {
        ui_app.toast(5.3, "\nFailed to open file \"%s\".\n%s\n",
                  pathname, ut_strerr(ut_runtime.err()));
    }
}

static void every_100ms(void) {
//  traceln("");
    static ui_view_t* last;
    if (last != ui_app.focus) { ui_app.request_redraw(); }
//  last = ui_app.focus;
}

static bool key_pressed(ui_view_t* unused(view), int64_t key) {
    bool swallow = false;
    if (ui_app.focused() && key == ui.key.escape) { ui_app.close(); }
    int32_t ix = focused();
    if (key == ui.key.f5) {
        if (ui_app.ctrl && ui_app.shift && !ui_fuzzing.is_running()) {
            ui_fuzzing.start(0); // on Ctrl+Shift+F5
        } else if (ui_fuzzing.is_running()) {
            ui_fuzzing.stop(); // on F5
        }
        swallow = true;
    }
    if (ui_app.ctrl) {
        if (key == ui.key.minus) {
            font_minus();
            swallow = true;
        } else if (key == ui.key.plus) {
            font_plus();
            swallow = true;
        } else if (key == '0') {
            font_reset();
            swallow = true;
        }
    }
    if (ix >= 0) { set_text(ix); }
    return swallow;
}

static void edit_enter(ui_edit_t* e) {
    assert(e->sle);
    if (!ui_app.shift) { // ignore shift ENTER:
        traceln("text: %.*s", e->doc->text.ps[0].b, e->doc->text.ps[0].u);
    }
}

// see edit.test.c

void ui_edit_init_with_lorem_ipsum(ui_edit_text_t* t);

static bool can_close(void) {
    if (!ui_fuzzing.from_inside()) {
        if (ui_fuzzing.is_running()) { ui_fuzzing.stop(); }
        return true;
    } else {
        return false; // ignore Quit if fuzzing clicked on it
    }
}

static void opened(void) {
//  ui_app.view->measure     = measure;
//  ui_app.view->layout      = layout;
    ui_app.content->paint       = paint;
    ui_app.content->key_pressed = key_pressed;
    scaled_fonts();
    label.fm = &ui_app.fm.mono;
    ut_str_printf(fuzz.hint, "Ctrl+Shift+F5 to start / F5 to stop Fuzzing");
    for (int32_t i = 0; i < ut_count_of(edit); i++) {
        ui_edit_doc.init(doc[i], null, 0, false);
        if (i < 2) {
            ui_edit_init_with_lorem_ipsum(&doc[i]->text);
        }
        ui_edit.init(edit[i], doc[i]);
        edit[i]->view.max_w = ui.infinity;
        if (i < 2) { edit[i]->view.max_h = ui.infinity; }
        edit[i]->view.fm = &pf;
    }
    ui_app.every_100ms = every_100ms;
    edit[2]->sle = true;
    ut_str_printf(edit[0]->view.p.text, "edit.#0#");
    ut_str_printf(edit[1]->view.p.text, "edit.#1#");
    ut_str_printf(edit[2]->view.p.text, "edit.sle");
//  edit[2]->select_all(edit[2]);
//  edit[2]->paste(edit[2], "Single line", -1);
    ui_edit.enter = edit_enter;
    static ui_view_t span    = ui_view(span);
    static ui_view_t spacer1 = ui_view(spacer);
    static ui_view_t spacer2 = ui_view(spacer);
    ui_view.add(ui_app.content,
        ui_view.add(&span,
            ui_view.add(&left,
                &edit0,
                &edit1,
                &label,
            null),
            &spacer1,
            ui_view.add(&right,
                &full_screen,
                &quit,
                &fuzz,
                &fp,
                &fm,
                &mono,
                &sl,
                &ro,
                &ww,
                &edit2,
                &spacer2,
            null),
        null),
    null);
    ui_view_for_each(&right, it, { it->align = ui.align.left; });
    edit2.view.max_w = ui.infinity;
    span.max_w = ui.infinity;
    span.max_h = ui.infinity;
    label.align = ui.align.left;
    edit2.view.align = ui.align.left;
    left.max_w = ui.infinity;
    left.max_h = ui.infinity;
    right.max_h = ui.infinity;
    set_text(0); // need to be two lines for measure
    ui_view.set_focus(&edit[0]->view);
    if (ut_args.c > 1) { open_file(ut_args.v[1]); }
}

static void init(void) {
    ui_app.title = title;
    ui_app.opened = opened;
    ui_app.can_close = can_close;
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

